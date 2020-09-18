/*
 *  Copyright (c) 2020, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes definitions for Thread Link Metrics.
 */

#include "link_metrics.hpp"

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"

#include "link_metrics_tlvs.hpp"

namespace ot {

LinkMetrics::LinkMetrics(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mLinkMetricsReportCallback(nullptr)
    , mLinkMetricsReportCallbackContext(nullptr)
{
}

otError LinkMetrics::LinkMetricsQuery(const Ip6::Address & aDestination,
                                      uint8_t              aSeriesId,
                                      const otLinkMetrics &aLinkMetricsFlags)
{
    otError                error;
    LinkMetricsTypeIdFlags typeIdFlags[kMaxTypeIdFlags];
    uint8_t                typeIdFlagsCount = GetTypeIdFlagsFromOtLinkMetricsFlags(typeIdFlags, aLinkMetricsFlags);

    error = SendLinkMetricsQuery(aDestination, aSeriesId, typeIdFlags, typeIdFlagsCount);

    return error;
}

otError LinkMetrics::AppendLinkMetricsReport(Message &                       aMessage,
                                             const Mle::LinkMetricsQueryTlv *aLinkMetricsQuery,
                                             const Message &                 aRequestMessage)
{
    otError                        error = OT_ERROR_NONE;
    Tlv                            tlv;
    const LinkMetricsQueryId *     queryId      = nullptr;
    const LinkMetricsQueryOptions *queryOptions = nullptr;
    uint8_t                        length       = 0;
    uint16_t                       startOffset  = aMessage.GetLength();
    const Tlv *                    subTlvCur    = nullptr;
    const Tlv *                    subTlvEnd    = nullptr;

    VerifyOrExit(aLinkMetricsQuery != nullptr && aLinkMetricsQuery->IsValid(), error = OT_ERROR_PARSE);
    subTlvCur = aLinkMetricsQuery->GetSubTlvs();
    subTlvEnd = aLinkMetricsQuery->GetNext();

    // Parse QueryId Sub-TLV
    if (subTlvCur->GetType() == kLinkMetricsQueryId)
    {
        queryId   = static_cast<const LinkMetricsQueryId *>(subTlvCur);
        subTlvCur = subTlvCur->GetNext();
        VerifyOrExit(queryId->IsValid(), error = OT_ERROR_PARSE);
    }
    else
    {
        ExitNow(error = OT_ERROR_PARSE);
    }

    // Link Metrics Report TLV
    tlv.SetType(Mle::Tlv::kLinkMetricsReport);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    if (queryId->GetSeriesId() == 0)
    {
        // Parse QueryOptions Sub-TLV
        if (subTlvCur->GetType() == kLinkMetricsQueryOptions)
        {
            queryOptions = static_cast<const LinkMetricsQueryOptions *>(subTlvCur);
            VerifyOrExit(queryOptions->IsValid(), error = OT_ERROR_PARSE);
            subTlvCur = subTlvCur->GetNext();
            VerifyOrExit(subTlvCur == subTlvEnd, error = OT_ERROR_PARSE);
        }
        else
        {
            ExitNow(error = OT_ERROR_PARSE);
        }
        SuccessOrExit(error = AppendSingleProbeLinkMetricsReport(aMessage, length, queryOptions,
                                                                 Get<Mac::Mac>().GetNoiseFloor(), aRequestMessage));
    }
    else
    {
        ExitNow(error = OT_ERROR_NOT_IMPLEMENTED);
    }

    tlv.SetLength(length);
    aMessage.Write(startOffset, sizeof(tlv), &tlv);

exit:
    otLogDebgMle("AppendLinkMetricsReport, error:%d", error);
    return error;
}

void LinkMetrics::HandleLinkMetricsReport(const Message &     aMessage,
                                          uint16_t            aOffset,
                                          uint16_t            aLength,
                                          const Ip6::Address &aAddress)
{
    otLinkMetricsValues    metricsValues;
    uint8_t                metricsRawValue;
    uint16_t               pos    = aOffset;
    uint16_t               endPos = aOffset + aLength;
    Tlv                    tlv;
    LinkMetricsTypeIdFlags typeIdFlags;

    VerifyOrExit(mLinkMetricsReportCallback != nullptr, OT_NOOP);

    memset(&metricsValues, 0, sizeof(otLinkMetricsValues));

    otLogDebgMle("Received Link Metrics Report");

    while (pos < endPos)
    {
        aMessage.Read(pos, sizeof(Tlv), &tlv);
        VerifyOrExit(tlv.GetType() == kLinkMetricsReportSub, OT_NOOP);
        pos += sizeof(Tlv);
        VerifyOrExit(pos + tlv.GetLength() <= endPos, OT_NOOP);

        aMessage.Read(pos, sizeof(LinkMetricsTypeIdFlags), &typeIdFlags);
        VerifyOrExit(!typeIdFlags.IsExtendedFlagSet(), OT_NOOP); ///< Extended Flag is not used in Thread 1.2
        pos += sizeof(LinkMetricsTypeIdFlags);

        switch (typeIdFlags.GetMetricEnum())
        {
        case OT_LINK_METRICS_PDU_COUNT:
            VerifyOrExit(typeIdFlags.IsLengthFlagSet(), OT_NOOP);
            metricsValues.mMetrics.mPduCount = true;
            aMessage.Read(pos, sizeof(uint32_t), &metricsValues.mPduCountValue);
            pos += sizeof(uint32_t);
            otLogDebgMle(" - PDU Counter: %d %s", metricsValues.mPduCountValue,
                         otLinkMetricsTypeEnumToString(OT_LINK_METRICS_TYPE_COUNT));
            break;

        case OT_LINK_METRICS_LQI:
            VerifyOrExit(!typeIdFlags.IsLengthFlagSet(), OT_NOOP);
            metricsValues.mMetrics.mLqi = true;
            aMessage.Read(pos, sizeof(uint8_t), &metricsValues.mLqiValue);
            pos += sizeof(uint8_t);
            otLogDebgMle(" - LQI: %d %s", metricsValues.mLqiValue,
                         otLinkMetricsTypeEnumToString(OT_LINK_METRICS_TYPE_EXPONENTIAL));
            break;

        case OT_LINK_METRICS_MARGIN:
            VerifyOrExit(!typeIdFlags.IsLengthFlagSet(), OT_NOOP);
            metricsValues.mMetrics.mLinkMargin = true;
            aMessage.Read(pos, sizeof(uint8_t), &metricsRawValue);
            metricsValues.mLinkMarginValue =
                metricsRawValue * 130 / 255; // Reverse operation for linear scale, map from [0, 255] to [0, 130]
            pos += sizeof(uint8_t);
            otLogDebgMle(" - Margin: %d (dB) %s", metricsValues.mLinkMarginValue,
                         otLinkMetricsTypeEnumToString(OT_LINK_METRICS_TYPE_EXPONENTIAL));
            break;

        case OT_LINK_METRICS_RSSI:
            VerifyOrExit(!typeIdFlags.IsLengthFlagSet(), OT_NOOP);
            metricsValues.mMetrics.mRssi = true;
            aMessage.Read(pos, sizeof(uint8_t), &metricsRawValue);
            metricsValues.mRssiValue =
                metricsRawValue * 130 / 255 - 130; // Reverse operation for linear scale, map from [0, 255] to [-130, 0]
            pos += sizeof(uint8_t);
            otLogDebgMle(" - RSSI: %d (dBm) %s", metricsValues.mRssiValue,
                         otLinkMetricsTypeEnumToString(OT_LINK_METRICS_TYPE_EXPONENTIAL));
            break;

        default:
            break;
        }
    }

    mLinkMetricsReportCallback(&aAddress, &metricsValues, mLinkMetricsReportCallbackContext);

exit:
    return;
}

void LinkMetrics::SetLinkMetricsReportCallback(otLinkMetricsReportCallback aCallback, void *aCallbackContext)
{
    mLinkMetricsReportCallback        = aCallback;
    mLinkMetricsReportCallbackContext = aCallbackContext;
}

otError LinkMetrics::SendLinkMetricsQuery(const Ip6::Address &          aDestination,
                                          uint8_t                       aSeriesId,
                                          const LinkMetricsTypeIdFlags *aTypeIdFlags,
                                          uint8_t                       aTypeIdFlagsCount)
{
    otError                 error = OT_ERROR_NONE;
    LinkMetricsQueryId      linkMetricsQueryId;
    LinkMetricsQueryOptions linkMetricsQueryOptions;
    uint8_t                 length = 0;
    static const uint8_t    tlvs[] = {Mle::Tlv::kLinkMetricsReport};
    uint8_t                 buf[sizeof(Tlv) * 3 + sizeof(LinkMetricsQueryId) + sizeof(LinkMetricsQueryOptions)];
    Tlv *                   tlv = reinterpret_cast<Tlv *>(buf);

    // Link Metrics Query TLV
    tlv->SetType(Mle::Tlv::kLinkMetricsQuery);
    length += sizeof(Tlv);

    // Link Metrics Query ID sub-TLV
    linkMetricsQueryId.Init();
    linkMetricsQueryId.SetSeriesId(aSeriesId);
    memcpy(buf + length, &linkMetricsQueryId, linkMetricsQueryId.GetSize());
    length += linkMetricsQueryId.GetSize();

    // Link Metrics Query Options sub-TLV
    if (aTypeIdFlagsCount > 0)
    {
        linkMetricsQueryOptions.Init();
        linkMetricsQueryOptions.SetLinkMetricsTypeIdFlagsList(aTypeIdFlags, aTypeIdFlagsCount);

        memcpy(buf + length, &linkMetricsQueryOptions, linkMetricsQueryOptions.GetSize());
        length += linkMetricsQueryOptions.GetSize();
    }

    // Set Length for Link Metrics Report Tlv
    tlv->SetLength(length - sizeof(Tlv));

    SuccessOrExit(error = Get<Mle::MleRouter>().SendDataRequest(aDestination, tlvs, sizeof(tlvs), 0, buf, length));

exit:
    return error;
}

otError LinkMetrics::AppendSingleProbeLinkMetricsReport(Message &                      aMessage,
                                                        uint8_t &                      aLength,
                                                        const LinkMetricsQueryOptions *aQueryOptions,
                                                        const int8_t                   aNoiseFloor,
                                                        const Message &                aRequestMessage)
{
    otError                       error = OT_ERROR_NONE;
    LinkMetricsReportSubTlv       metric;
    const LinkMetricsTypeIdFlags *linkMetricsTypeIds;
    uint8_t                       metricsCount = 0;

    OT_ASSERT(aQueryOptions != nullptr);
    metricsCount       = kLinkMetricsMaxTypeIdFlags;
    linkMetricsTypeIds = aQueryOptions->GetLinkMetricsTypeIdFlagsList(metricsCount);

    for (uint8_t i = 0; i < metricsCount; i++)
    {
        // Link Metrics Report sub-TLVs
        metric.Init();
        metric.SetMetricsTypeId(linkMetricsTypeIds[i]);

        switch (linkMetricsTypeIds[i].GetMetricEnum())
        {
        case OT_LINK_METRICS_PDU_COUNT:
            VerifyOrExit(linkMetricsTypeIds[i].IsLengthFlagSet(), error = OT_ERROR_INVALID_ARGS);
            metric.SetMetricsValue32(aRequestMessage.GetPsduCount());
            break;

        case OT_LINK_METRICS_LQI:
            VerifyOrExit(!linkMetricsTypeIds[i].IsLengthFlagSet(), error = OT_ERROR_INVALID_ARGS);
            metric.SetMetricsValue8(aRequestMessage.GetAverageLqi()); // IEEE 802.15.4 LQI is in scale 0-255
            break;

        case OT_LINK_METRICS_MARGIN:
            VerifyOrExit(!linkMetricsTypeIds[i].IsLengthFlagSet(), error = OT_ERROR_INVALID_ARGS);
            metric.SetMetricsValue8(
                LinkQualityInfo::ConvertRssToLinkMargin(aNoiseFloor, aRequestMessage.GetAverageRss()) * 255 /
                130); // Linear scale Link Margin from [0, 130] to [0, 255]
            break;

        case OT_LINK_METRICS_RSSI:
            VerifyOrExit(!linkMetricsTypeIds[i].IsLengthFlagSet(), error = OT_ERROR_INVALID_ARGS);
            metric.SetMetricsValue8((aRequestMessage.GetAverageRss() + 130) * 255 /
                                    130); // Linear scale rss from [-130, 0] to [0, 255]
            break;

        default:
            break;
        }

        SuccessOrExit(error = aMessage.Append(&metric, sizeof(Tlv) + metric.GetLength()));
        aLength += sizeof(Tlv) + metric.GetLength();
    }

exit:
    return error;
}

uint8_t LinkMetrics::GetTypeIdFlagsFromOtLinkMetricsFlags(LinkMetricsTypeIdFlags *aTypeIdFlags,
                                                          const otLinkMetrics &   aLinkMetricsFlags)
{
    uint8_t count = 0;
    uint8_t typeIdFlagValue;

    if (aLinkMetricsFlags.mPduCount)
    {
        typeIdFlagValue = kTypeIdFlagPdu;
        memcpy(&aTypeIdFlags[count++], &typeIdFlagValue, sizeof(uint8_t));
    }

    if (aLinkMetricsFlags.mLqi)
    {
        typeIdFlagValue = kTypeIdFlagLqi;
        memcpy(&aTypeIdFlags[count++], &typeIdFlagValue, sizeof(uint8_t));
    }

    if (aLinkMetricsFlags.mLinkMargin)
    {
        typeIdFlagValue = kTypeIdFlagLinkMargin;
        memcpy(&aTypeIdFlags[count++], &typeIdFlagValue, sizeof(uint8_t));
    }

    if (aLinkMetricsFlags.mRssi)
    {
        typeIdFlagValue = kTypeIdFlagRssi;
        memcpy(&aTypeIdFlags[count++], &typeIdFlagValue, sizeof(uint8_t));
    }

    return count;
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
