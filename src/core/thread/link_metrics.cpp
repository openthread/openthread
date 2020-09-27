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
    uint8_t                typeIdFlagsCount = TypeIdFlagsFromLinkMetricsFlags(typeIdFlags, aLinkMetricsFlags);

    error = SendLinkMetricsQuery(aDestination, aSeriesId, typeIdFlags, typeIdFlagsCount);

    return error;
}

otError LinkMetrics::AppendLinkMetricsReport(Message &aMessage, const Message &aRequestMessage)
{
    otError       error = OT_ERROR_NONE;
    Tlv           tlv;
    uint8_t       queryId;
    bool          hasQueryId  = false;
    uint8_t       length      = 0;
    uint16_t      startOffset = aMessage.GetLength();
    uint16_t      offset;
    uint16_t      endOffset;
    otLinkMetrics linkMetrics;

    memset(&linkMetrics, 0, sizeof(linkMetrics));

    SuccessOrExit(error = Tlv::FindTlvValueOffset(aRequestMessage, Mle::Tlv::Type::kLinkMetricsQuery, offset,
                                                  endOffset)); // `endOffset` is used to store tlv length here

    endOffset = offset + endOffset;

    while (offset < endOffset)
    {
        VerifyOrExit(aRequestMessage.Read(offset, sizeof(tlv), &tlv) == sizeof(tlv), error = OT_ERROR_PARSE);

        switch (tlv.GetType())
        {
        case kLinkMetricsQueryId:
            SuccessOrExit(error = Tlv::ReadUint8Tlv(aRequestMessage, offset, queryId));
            hasQueryId = true;
            break;

        case kLinkMetricsQueryOptions:
            for (uint16_t index = offset + sizeof(tlv), endIndex = static_cast<uint16_t>(offset + tlv.GetSize());
                 index < endIndex; index += sizeof(LinkMetricsTypeIdFlags))
            {
                LinkMetricsTypeIdFlags typeIdFlags;

                VerifyOrExit(aRequestMessage.Read(index, sizeof(typeIdFlags), &typeIdFlags) == sizeof(typeIdFlags),
                             error = OT_ERROR_PARSE);

                switch (typeIdFlags.GetRawValue())
                {
                case kTypeIdFlagPdu:
                    VerifyOrExit(!linkMetrics.mPduCount, error = OT_ERROR_PARSE);
                    linkMetrics.mPduCount = true;
                    break;

                case kTypeIdFlagLqi:
                    VerifyOrExit(!linkMetrics.mLqi, error = OT_ERROR_PARSE);
                    linkMetrics.mLqi = true;
                    break;

                case kTypeIdFlagLinkMargin:
                    VerifyOrExit(!linkMetrics.mLinkMargin, error = OT_ERROR_PARSE);
                    linkMetrics.mLinkMargin = true;
                    break;

                case kTypeIdFlagRssi:
                    VerifyOrExit(!linkMetrics.mRssi, error = OT_ERROR_PARSE);
                    linkMetrics.mRssi = true;
                    break;

                default:
                    if (typeIdFlags.IsExtendedFlagSet())
                    {
                        index += sizeof(uint8_t); // Skip the additional second flags byte.
                    }
                    break;
                }
            }
            break;

        default:
            break;
        }

        offset += tlv.GetSize();
    }

    VerifyOrExit(hasQueryId, error = OT_ERROR_PARSE);

    // Link Metrics Report TLV
    tlv.SetType(Mle::Tlv::kLinkMetricsReport);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    if (queryId == 0)
    {
        SuccessOrExit(error = AppendSingleProbeLinkMetricsReport(aMessage, length, linkMetrics, aRequestMessage));
    }
    else
    {
        ExitNow(error = OT_ERROR_NOT_IMPLEMENTED);
    }

    tlv.SetLength(length);
    aMessage.Write(startOffset, sizeof(tlv), &tlv);

exit:
    otLogDebgMle("AppendLinkMetricsReport, error:%s", otThreadErrorToString(error));
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

    memset(&metricsValues, 0, sizeof(metricsValues));

    otLogDebgMle("Received Link Metrics Report");

    while (pos < endPos)
    {
        VerifyOrExit(aMessage.Read(pos, sizeof(Tlv), &tlv) == sizeof(Tlv), OT_NOOP);
        VerifyOrExit(tlv.GetType() == kLinkMetricsReportSub, OT_NOOP);
        pos += sizeof(Tlv);
        VerifyOrExit(pos + tlv.GetLength() <= endPos, OT_NOOP);

        aMessage.Read(pos, sizeof(LinkMetricsTypeIdFlags), &typeIdFlags);
        if (typeIdFlags.IsExtendedFlagSet())
        {
            pos += tlv.GetLength(); // Skip the whole sub-TLV if `E` flag is set
            continue;
        }
        pos += sizeof(LinkMetricsTypeIdFlags);

        switch (typeIdFlags.GetRawValue())
        {
        case kTypeIdFlagPdu:
            metricsValues.mMetrics.mPduCount = true;
            aMessage.Read(pos, sizeof(uint32_t), &metricsValues.mPduCountValue);
            pos += sizeof(uint32_t);
            otLogDebgMle(" - PDU Counter: %d (Count/Summation)", metricsValues.mPduCountValue);
            break;

        case kTypeIdFlagLqi:
            metricsValues.mMetrics.mLqi = true;
            aMessage.Read(pos, sizeof(uint8_t), &metricsValues.mLqiValue);
            pos += sizeof(uint8_t);
            otLogDebgMle(" - LQI: %d (Exponential Moving Average)", metricsValues.mLqiValue);
            break;

        case kTypeIdFlagLinkMargin:
            metricsValues.mMetrics.mLinkMargin = true;
            aMessage.Read(pos, sizeof(uint8_t), &metricsRawValue);
            metricsValues.mLinkMarginValue =
                metricsRawValue * 130 / 255; // Reverse operation for linear scale, map from [0, 255] to [0, 130]
            pos += sizeof(uint8_t);
            otLogDebgMle(" - Margin: %d (dB) (Exponential Moving Average)", metricsValues.mLinkMarginValue);
            break;

        case kTypeIdFlagRssi:
            metricsValues.mMetrics.mRssi = true;
            aMessage.Read(pos, sizeof(uint8_t), &metricsRawValue);
            metricsValues.mRssiValue =
                metricsRawValue * 130 / 255 - 130; // Reverse operation for linear scale, map from [0, 255] to [-130, 0]
            pos += sizeof(uint8_t);
            otLogDebgMle(" - RSSI: %d (dBm) (Exponential Moving Average)", metricsValues.mRssiValue);
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
    otError                    error = OT_ERROR_NONE;
    LinkMetricsQueryOptionsTlv linkMetricsQueryOptionsTlv;
    uint8_t                    length = 0;
    static const uint8_t       tlvs[] = {Mle::Tlv::kLinkMetricsReport};
    uint8_t                    buf[sizeof(Tlv) * 3 + sizeof(uint8_t) +
                sizeof(LinkMetricsTypeIdFlags) *
                    kMaxTypeIdFlags]; // LinkMetricsQuery Tlv + LinkMetricsQueryId sub-TLV (value-length: 1 byte) +
                                      // LinkMetricsQueryOptions sub-TLV (value-length: `kMaxTypeIdFlags` bytes)
    Tlv *tlv = reinterpret_cast<Tlv *>(buf);
    Tlv  subTlv;

    // Link Metrics Query TLV
    tlv->SetType(Mle::Tlv::kLinkMetricsQuery);
    length += sizeof(Tlv);

    // Link Metrics Query ID sub-TLV
    subTlv.SetType(kLinkMetricsQueryId);
    subTlv.SetLength(sizeof(uint8_t));
    memcpy(buf + length, &subTlv, sizeof(subTlv));
    length += sizeof(subTlv);
    memcpy(buf + length, &aSeriesId, sizeof(aSeriesId));
    length += sizeof(aSeriesId);

    // Link Metrics Query Options sub-TLV
    if (aTypeIdFlagsCount > 0)
    {
        linkMetricsQueryOptionsTlv.Init();
        linkMetricsQueryOptionsTlv.SetLength(aTypeIdFlagsCount * sizeof(LinkMetricsTypeIdFlags));

        memcpy(buf + length, &linkMetricsQueryOptionsTlv, sizeof(linkMetricsQueryOptionsTlv));
        length += sizeof(linkMetricsQueryOptionsTlv);
        memcpy(buf + length, aTypeIdFlags, linkMetricsQueryOptionsTlv.GetLength());
        length += linkMetricsQueryOptionsTlv.GetLength();
    }

    // Set Length for Link Metrics Report TLV
    tlv->SetLength(length - sizeof(Tlv));

    SuccessOrExit(error = Get<Mle::MleRouter>().SendDataRequest(aDestination, tlvs, sizeof(tlvs), 0, buf, length));

exit:
    return error;
}

otError LinkMetrics::AppendSingleProbeLinkMetricsReport(Message &            aMessage,
                                                        uint8_t &            aLength,
                                                        const otLinkMetrics &aLinkMetrics,
                                                        const Message &      aRequestMessage)
{
    otError                 error = OT_ERROR_NONE;
    LinkMetricsReportSubTlv metric;

    aLength = 0;

    // Link Metrics Report sub-TLVs
    if (aLinkMetrics.mPduCount)
    {
        metric.Init();
        metric.SetMetricsTypeId(kTypeIdFlagPdu);
        metric.SetMetricsValue32(aRequestMessage.GetPsduCount());
        SuccessOrExit(error = aMessage.Append(&metric, metric.GetSize()));
        aLength += metric.GetSize();
    }

    if (aLinkMetrics.mLqi)
    {
        metric.Init();
        metric.SetMetricsTypeId(kTypeIdFlagLqi);
        metric.SetMetricsValue8(aRequestMessage.GetAverageLqi()); // IEEE 802.15.4 LQI is in scale 0-255
        SuccessOrExit(error = aMessage.Append(&metric, metric.GetSize()));
        aLength += metric.GetSize();
    }

    if (aLinkMetrics.mLinkMargin)
    {
        metric.Init();
        metric.SetMetricsTypeId(kTypeIdFlagLinkMargin);
        metric.SetMetricsValue8(
            LinkQualityInfo::ConvertRssToLinkMargin(Get<Mac::Mac>().GetNoiseFloor(), aRequestMessage.GetAverageRss()) *
            255 / 130); // Linear scale Link Margin from [0, 130] to [0, 255]
        SuccessOrExit(error = aMessage.Append(&metric, metric.GetSize()));
        aLength += metric.GetSize();
    }

    if (aLinkMetrics.mRssi)
    {
        metric.Init();
        metric.SetMetricsTypeId(kTypeIdFlagRssi);
        metric.SetMetricsValue8((aRequestMessage.GetAverageRss() + 130) * 255 /
                                130); // Linear scale rss from [-130, 0] to [0, 255]
        SuccessOrExit(error = aMessage.Append(&metric, metric.GetSize()));
        aLength += metric.GetSize();
    }

exit:
    return error;
}

uint8_t LinkMetrics::TypeIdFlagsFromLinkMetricsFlags(LinkMetricsTypeIdFlags *aTypeIdFlags,
                                                     const otLinkMetrics &   aLinkMetricsFlags)
{
    uint8_t count = 0;

    if (aLinkMetricsFlags.mPduCount)
    {
        aTypeIdFlags[count++].SetRawValue(kTypeIdFlagPdu);
    }

    if (aLinkMetricsFlags.mLqi)
    {
        aTypeIdFlags[count++].SetRawValue(kTypeIdFlagLqi);
    }

    if (aLinkMetricsFlags.mLinkMargin)
    {
        aTypeIdFlags[count++].SetRawValue(kTypeIdFlagLinkMargin);
    }

    if (aLinkMetricsFlags.mRssi)
    {
        aTypeIdFlags[count++].SetRawValue(kTypeIdFlagRssi);
    }

    return count;
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
