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

#include "link_metrics_tlvs.hpp"

namespace ot {

LinkMetrics::LinkMetrics(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mLinkMetricsReportCallback(nullptr)
    , mLinkMetricsReportCallbackContext(nullptr)
{
}

otError LinkMetrics::LinkMetricsQuery(const otIp6Address *aDestination,
                                      uint8_t             aSeriesId,
                                      const uint8_t *     aTypeIdFlags,
                                      uint8_t             aTypeIdFlagsCount)
{
    OT_UNUSED_VARIABLE(aDestination);

    otError error = OT_ERROR_NONE;

    VerifyOrExit(aTypeIdFlagsCount <= kLinkMetricsMaxTypeIdFlags, error = OT_ERROR_INVALID_ARGS);

    error = SendLinkMetricsQuery(*static_cast<const Ip6::Address *>(aDestination), aSeriesId,
                                 reinterpret_cast<const LinkMetricsTypeIdFlags *>(aTypeIdFlags), aTypeIdFlagsCount);

exit:
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

    VerifyOrExit(aLinkMetricsQuery != nullptr && aLinkMetricsQuery->IsValid(), error = OT_ERROR_PARSE);
    queryId = aLinkMetricsQuery->GetQueryId();
    VerifyOrExit(queryId->IsValid(), error = OT_ERROR_PARSE);
    VerifyOrExit(queryId->GetSeriesId() < 255, error = OT_ERROR_INVALID_ARGS);

    // Link Metrics Report TLV
    tlv.SetType(Mle::Tlv::kLinkMetricsReport);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    if (queryId->GetSeriesId() == 0)
    {
        queryOptions = aLinkMetricsQuery->GetQueryOptions();
        VerifyOrExit(queryOptions->IsValid(), error = OT_ERROR_PARSE);
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
    otLogDebgMle("AppendLinkMetricsReport, error:%d\n", error);
    return error;
}

void LinkMetrics::HandleLinkMetricsReport(const Message &     aMessage,
                                          uint16_t            aOffset,
                                          uint16_t            aLength,
                                          const Ip6::Address &aAddress)
{
    otLinkMetrics          metrics[kLinkMetricsMaxTypeIdFlags];
    uint8_t                metricsCount = 0;
    uint16_t               pos          = aOffset;
    uint16_t               end_pos      = aOffset + aLength;
    Tlv                    tlv;
    LinkMetricsTypeIdFlags typeId;

    while (pos < end_pos)
    {
        aMessage.Read(pos, sizeof(Tlv), &tlv);
        VerifyOrExit(tlv.GetType() == kLinkMetricsReportSub, OT_NOOP);
        pos += sizeof(Tlv);
        aMessage.Read(pos, sizeof(LinkMetricsTypeIdFlags), &typeId);
        pos += sizeof(otLinkMetricsTypeIdFlags);
        SetLinkMetricsTypeIdFlagsFromTlv(metrics[metricsCount].mLinkMetricsTypeIdFlags, typeId);

        if (metrics[metricsCount].mLinkMetricsTypeIdFlags.mLinkMetricsFlagL)
        {
            aMessage.Read(pos, sizeof(uint32_t), &metrics[metricsCount].mLinkMetricsValue.m32);
            pos += sizeof(uint32_t);
        }
        else
        {
            aMessage.Read(pos, sizeof(uint8_t), &metrics[metricsCount].mLinkMetricsValue.m8);
            pos += sizeof(uint8_t);
        }

        metricsCount++;
    }

    if (mLinkMetricsReportCallback != nullptr)
    {
        mLinkMetricsReportCallback(&aAddress, metrics, metricsCount, mLinkMetricsReportCallbackContext);
    }

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

        switch (linkMetricsTypeIds[i].GetMetricsId())
        {
        case OT_LINK_METRICS_PDU_COUNT:
            if (linkMetricsTypeIds[i].IsLengthFlagSet())
            {
                metric.SetMetricsValue32(aRequestMessage.GetPsduCount());
            }
            else
            {
                metric.SetMetricsValue8(aRequestMessage.GetPsduCount());
            }
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
                                    130); // Linear scale rss from [-130, 0] to [0 to 255]
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

void LinkMetrics::SetLinkMetricsTypeIdFlagsFromTlv(otLinkMetricsTypeIdFlags &aOtTypeId,
                                                   LinkMetricsTypeIdFlags &  aTlvTypeId)
{
    aOtTypeId.mLinkMetricsId    = aTlvTypeId.GetMetricsId();
    aOtTypeId.mLinkMetricsType  = aTlvTypeId.GetMetricsType();
    aOtTypeId.mLinkMetricsFlagE = aTlvTypeId.IsExtendedFlagSet();
    aOtTypeId.mLinkMetricsFlagL = aTlvTypeId.IsLengthFlagSet();
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
