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
#include "thread/neighbor_table.hpp"

#include "link_metrics_tlvs.hpp"

namespace ot {

void LinkMetricsSeriesInfo::Init(uint8_t              aSeriesId,
                                 const SeriesFlags &  aSeriesFlags,
                                 const otLinkMetrics &aLinkMetricsFlags)
{
    mSeriesId                = aSeriesId;
    mSeriesFlags             = aSeriesFlags;
    mLinkMetrics.mPduCount   = aLinkMetricsFlags.mPduCount;
    mLinkMetrics.mLqi        = aLinkMetricsFlags.mLqi;
    mLinkMetrics.mLinkMargin = aLinkMetricsFlags.mLinkMargin;
    mLinkMetrics.mRssi       = aLinkMetricsFlags.mRssi;
    mRssAverager.Clear();
    mLqiAverager.Clear();
    mPduCount = 0;
}

void LinkMetricsSeriesInfo::AggregateLinkMetrics(uint8_t aFrameType, uint8_t aLqi, int8_t aRss)
{
    if (IsFrameTypeMatch(aFrameType))
    {
        mPduCount++;
        mLqiAverager.Add(aLqi);
        IgnoreError(mRssAverager.Add(aRss));
    }
}

void LinkMetricsSeriesInfo::GetLinkMetrics(otLinkMetrics &aLinkMetrics) const
{
    aLinkMetrics.mPduCount   = mLinkMetrics.mPduCount;
    aLinkMetrics.mLqi        = mLinkMetrics.mLqi;
    aLinkMetrics.mLinkMargin = mLinkMetrics.mLinkMargin;
    aLinkMetrics.mRssi       = mLinkMetrics.mRssi;
}

bool LinkMetricsSeriesInfo::IsFrameTypeMatch(uint8_t aFrameType) const
{
    bool match = false;

    switch (aFrameType)
    {
    case kSeriesTypeLinkProbe:
        VerifyOrExit(!mSeriesFlags.IsMacDataFlagSet()); // Ignore this when Mac Data is accounted
        match = mSeriesFlags.IsLinkProbeFlagSet();
        break;
    case Mac::Frame::kFcfFrameData:
        match = mSeriesFlags.IsMacDataFlagSet();
        break;
    case Mac::Frame::kFcfFrameMacCmd:
        match = mSeriesFlags.IsMacDataRequestFlagSet();
        break;
    case Mac::Frame::kFcfFrameAck:
        match = mSeriesFlags.IsMacAckFlagSet();
        break;
    default:
        break;
    }

exit:
    return match;
}

LinkMetrics::LinkMetrics(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mLinkMetricsReportCallback(nullptr)
    , mLinkMetricsReportCallbackContext(nullptr)
{
}

otError LinkMetrics::LinkMetricsQuery(const Ip6::Address & aDestination,
                                      uint8_t              aSeriesId,
                                      const otLinkMetrics *aLinkMetricsFlags)
{
    otError                error;
    LinkMetricsTypeIdFlags typeIdFlags[kMaxTypeIdFlags];
    uint8_t                typeIdFlagsCount = 0;
    Neighbor *             neighbor         = GetNeighborFromLinkLocalAddr(aDestination);

    VerifyOrExit(neighbor != nullptr, error = OT_ERROR_UNKNOWN_NEIGHBOR);
    VerifyOrExit(neighbor->IsThreadVersion1p2(), error = OT_ERROR_NOT_CAPABLE);

    if (aLinkMetricsFlags != nullptr)
    {
        typeIdFlagsCount = TypeIdFlagsFromLinkMetricsFlags(typeIdFlags, *aLinkMetricsFlags);
    }

    if (aSeriesId != 0)
    {
        VerifyOrExit(typeIdFlagsCount == 0, error = OT_ERROR_INVALID_ARGS);
    }

    error = SendLinkMetricsQuery(aDestination, aSeriesId, typeIdFlags, typeIdFlagsCount);

exit:
    return error;
}

otError LinkMetrics::SendMgmtRequestForwardTrackingSeries(const Ip6::Address &            aDestination,
                                                          uint8_t                         aSeriesId,
                                                          const otLinkMetricsSeriesFlags &aSeriesFlags,
                                                          const otLinkMetrics *           aLinkMetricsFlags)
{
    otError      error = OT_ERROR_NONE;
    uint8_t      subTlvs[sizeof(Tlv) + sizeof(uint8_t) * 2 + sizeof(LinkMetricsTypeIdFlags) * kMaxTypeIdFlags];
    Tlv *        forwardProbingRegistrationSubTlv = reinterpret_cast<Tlv *>(subTlvs);
    SeriesFlags *seriesFlags       = reinterpret_cast<SeriesFlags *>(subTlvs + sizeof(Tlv) + sizeof(aSeriesId));
    uint8_t      typeIdFlagsOffset = sizeof(Tlv) + sizeof(uint8_t) * 2;
    uint8_t      typeIdFlagsCount  = 0;
    Neighbor *   neighbor          = GetNeighborFromLinkLocalAddr(aDestination);

    VerifyOrExit(neighbor != nullptr, error = OT_ERROR_UNKNOWN_NEIGHBOR);
    VerifyOrExit(neighbor->IsThreadVersion1p2(), error = OT_ERROR_NOT_CAPABLE);

    // Directly transform `aLinkMetricsFlags` into LinkMetricsTypeIdFlags and put them into `subTlvs`
    if (aLinkMetricsFlags != nullptr)
    {
        typeIdFlagsCount = TypeIdFlagsFromLinkMetricsFlags(
            reinterpret_cast<LinkMetricsTypeIdFlags *>(subTlvs + typeIdFlagsOffset), *aLinkMetricsFlags);
    }

    VerifyOrExit(aSeriesId > kQueryIdSingleProbe, error = OT_ERROR_INVALID_ARGS);

    forwardProbingRegistrationSubTlv->SetType(kForwardProbingRegistration);
    forwardProbingRegistrationSubTlv->SetLength(
        sizeof(uint8_t) * 2 +
        sizeof(LinkMetricsTypeIdFlags) *
            typeIdFlagsCount); // SeriesId + SeriesFlags + typeIdFlagsCount * LinkMetricsTypeIdFlags

    memcpy(subTlvs + sizeof(Tlv), &aSeriesId, sizeof(aSeriesId));

    seriesFlags->SetFromOtSeriesFlags(aSeriesFlags);

    error = Get<Mle::MleRouter>().SendLinkMetricsManagementRequest(aDestination, subTlvs,
                                                                   forwardProbingRegistrationSubTlv->GetSize());

exit:
    otLogDebgMle("SendMgmtRequestForwardTrackingSeries, error:%s, Series ID:%u", otThreadErrorToString(error),
                 aSeriesId);
    return error;
}

otError LinkMetrics::SendMgmtRequestEnhAckProbing(const Ip6::Address &           aDestination,
                                                  const otLinkMetricsEnhAckFlags aEnhAckFlags,
                                                  const otLinkMetrics *          aLinkMetricsFlags)
{
    otError                              error = OT_ERROR_NONE;
    EnhAckLinkMetricsConfigurationSubTlv enhAckLinkMetricsConfigurationSubTlv;
    Mac::Address                         macAddress;
    Neighbor *                           neighbor = GetNeighborFromLinkLocalAddr(aDestination);

    VerifyOrExit(neighbor != nullptr, error = OT_ERROR_UNKNOWN_NEIGHBOR);
    VerifyOrExit(neighbor->IsThreadVersion1p2(), error = OT_ERROR_NOT_CAPABLE);

    if (aEnhAckFlags == OT_LINK_METRICS_ENH_ACK_CLEAR)
    {
        VerifyOrExit(aLinkMetricsFlags == nullptr, error = OT_ERROR_INVALID_ARGS);
    }

    enhAckLinkMetricsConfigurationSubTlv.SetEnhAckFlags(aEnhAckFlags);

    if (aLinkMetricsFlags != nullptr)
    {
        enhAckLinkMetricsConfigurationSubTlv.SetTypeIdFlags(aLinkMetricsFlags);
    }

    error = Get<Mle::MleRouter>().SendLinkMetricsManagementRequest(
        aDestination, reinterpret_cast<const uint8_t *>(&enhAckLinkMetricsConfigurationSubTlv),
        enhAckLinkMetricsConfigurationSubTlv.GetSize());

    if (aLinkMetricsFlags != nullptr)
    {
        neighbor->SetEnhAckProbingMetrics(*aLinkMetricsFlags);
    }
    else
    {
        otLinkMetrics linkMetrics;
        memset(&linkMetrics, 0, sizeof(linkMetrics));
        neighbor->SetEnhAckProbingMetrics(linkMetrics);
    }

exit:
    return error;
}

otError LinkMetrics::SendLinkProbe(const Ip6::Address &aDestination, uint8_t aSeriesId, uint8_t aLength)
{
    otError   error = OT_ERROR_NONE;
    uint8_t   buf[kLinkProbeMaxLen];
    Neighbor *neighbor = GetNeighborFromLinkLocalAddr(aDestination);

    VerifyOrExit(neighbor != nullptr, error = OT_ERROR_UNKNOWN_NEIGHBOR);
    VerifyOrExit(neighbor->IsThreadVersion1p2(), error = OT_ERROR_NOT_CAPABLE);

    VerifyOrExit(aLength <= LinkMetrics::kLinkProbeMaxLen && aSeriesId != kQueryIdSingleProbe &&
                     aSeriesId != kSeriesIdAllSeries,
                 error = OT_ERROR_INVALID_ARGS);

    error = Get<Mle::MleRouter>().SendLinkProbe(aDestination, aSeriesId, buf, aLength);
exit:
    otLogDebgMle("SendLinkProbe, error:%s, Series ID:%u", otThreadErrorToString(error), aSeriesId);
    return error;
}

otError LinkMetrics::AppendLinkMetricsReport(Message &aMessage, const Message &aRequestMessage, Neighbor &aNeighbor)
{
    otError             error = OT_ERROR_NONE;
    Tlv                 tlv;
    uint8_t             queryId;
    bool                hasQueryId  = false;
    uint8_t             length      = 0;
    uint16_t            startOffset = aMessage.GetLength();
    uint16_t            offset;
    uint16_t            endOffset;
    otLinkMetricsValues linkMetricsValues;

    memset(&linkMetricsValues, 0, sizeof(linkMetricsValues));

    SuccessOrExit(error = Tlv::FindTlvValueOffset(aRequestMessage, Mle::Tlv::Type::kLinkMetricsQuery, offset,
                                                  endOffset)); // `endOffset` is used to store tlv length here

    endOffset = offset + endOffset;

    while (offset < endOffset)
    {
        SuccessOrExit(error = aRequestMessage.Read(offset, tlv));

        switch (tlv.GetType())
        {
        case kLinkMetricsQueryId:
            SuccessOrExit(error = Tlv::Read<LinkMetricsQueryIdTlv>(aRequestMessage, offset, queryId));
            hasQueryId = true;
            break;

        case kLinkMetricsQueryOptions:
            SuccessOrExit(error = ReadTypeIdFlagsFromMessage(aRequestMessage, offset + sizeof(tlv),
                                                             static_cast<uint16_t>(offset + tlv.GetSize()),
                                                             linkMetricsValues.mMetrics));
            break;

        default:
            break;
        }

        offset += tlv.GetSize();
    }

    VerifyOrExit(hasQueryId, error = OT_ERROR_PARSE);

    // Link Metrics Report TLV
    tlv.SetType(Mle::Tlv::kLinkMetricsReport);
    SuccessOrExit(error = aMessage.Append(tlv));

    if (queryId == kQueryIdSingleProbe)
    {
        linkMetricsValues.mPduCountValue = aRequestMessage.GetPsduCount();
        linkMetricsValues.mLqiValue      = aRequestMessage.GetAverageLqi();
        linkMetricsValues.mLinkMarginValue =
            LinkQualityInfo::ConvertRssToLinkMargin(Get<Mac::Mac>().GetNoiseFloor(), aRequestMessage.GetAverageRss()) *
            255 / 130; // Linear scale Link Margin from [0, 130] to [0, 255]
        linkMetricsValues.mRssiValue =
            (aRequestMessage.GetAverageRss() + 130) * 255 / 130; // Linear scale rss from [-130, 0] to [0, 255]

        SuccessOrExit(error = AppendReportSubTlvToMessage(aMessage, length, linkMetricsValues));
    }
    else
    {
        LinkMetricsSeriesInfo *seriesInfo = aNeighbor.GetForwardTrackingSeriesInfo(queryId);
        if (seriesInfo == nullptr)
        {
            SuccessOrExit(error =
                              AppendStatusSubTlvToMessage(aMessage, length, kLinkMetricsStatusSeriesIdNotRecognized));
        }
        else if (seriesInfo->GetPduCount() == 0)
        {
            SuccessOrExit(
                error = AppendStatusSubTlvToMessage(aMessage, length, kLinkMetricsStatusNoMatchingFramesReceived));
        }
        else
        {
            seriesInfo->GetLinkMetrics(linkMetricsValues.mMetrics);
            linkMetricsValues.mPduCountValue = seriesInfo->GetPduCount();
            linkMetricsValues.mLqiValue      = seriesInfo->GetAverageLqi();
            linkMetricsValues.mLinkMarginValue =
                LinkQualityInfo::ConvertRssToLinkMargin(Get<Mac::Mac>().GetNoiseFloor(),
                                                        seriesInfo->GetAverageRss()) *
                255 / 130; // Linear scale Link Margin from [0, 130] to [0, 255]
            linkMetricsValues.mRssiValue =
                (seriesInfo->GetAverageRss() + 130) * 255 / 130; // Linear scale RSSI from [-130, 0] to [0, 255]
            SuccessOrExit(error = AppendReportSubTlvToMessage(aMessage, length, linkMetricsValues));
        }
    }

    tlv.SetLength(length);
    aMessage.Write(startOffset, tlv);

exit:
    otLogDebgMle("AppendLinkMetricsReport, error:%s", otThreadErrorToString(error));
    return error;
}

otError LinkMetrics::HandleLinkMetricsManagementRequest(const Message &    aMessage,
                                                        Neighbor &         aNeighbor,
                                                        LinkMetricsStatus &aStatus)
{
    otError                error = OT_ERROR_NONE;
    Tlv                    tlv;
    uint8_t                seriesId;
    SeriesFlags            seriesFlags;
    LinkMetricsEnhAckFlags enhAckFlags;
    otLinkMetrics          linkMetrics;
    bool                   hasForwardProbingRegistrationTlv = false;
    bool                   hasEnhAckProbingTlv              = false;
    uint16_t               offset;
    uint16_t               length;
    uint16_t               index = 0;

    SuccessOrExit(error = Tlv::FindTlvValueOffset(aMessage, Mle::Tlv::Type::kLinkMetricsManagement, offset, length));

    while (index < length)
    {
        uint16_t pos = offset + index;
        SuccessOrExit(aMessage.Read(pos, tlv));

        pos += sizeof(tlv);

        switch (tlv.GetType())
        {
        case kForwardProbingRegistration:
            VerifyOrExit(!hasForwardProbingRegistrationTlv && !hasEnhAckProbingTlv, error = OT_ERROR_PARSE);
            VerifyOrExit(tlv.GetLength() >= sizeof(seriesId) + sizeof(seriesFlags), error = OT_ERROR_PARSE);
            SuccessOrExit(aMessage.Read(pos, seriesId));
            pos += sizeof(seriesId);
            SuccessOrExit(aMessage.Read(pos, seriesFlags));
            pos += sizeof(seriesFlags);
            SuccessOrExit(error = ReadTypeIdFlagsFromMessage(
                              aMessage, pos, static_cast<uint16_t>(offset + index + tlv.GetSize()), linkMetrics));
            hasForwardProbingRegistrationTlv = true;
            break;

        case kEnhancedACKConfiguration:
            VerifyOrExit(!hasForwardProbingRegistrationTlv && !hasEnhAckProbingTlv, error = OT_ERROR_PARSE);
            VerifyOrExit(tlv.GetLength() >= sizeof(LinkMetricsEnhAckFlags), error = OT_ERROR_PARSE);
            SuccessOrExit(aMessage.Read(pos, enhAckFlags));
            pos += sizeof(enhAckFlags);
            SuccessOrExit(error = ReadTypeIdFlagsFromMessage(
                              aMessage, pos, static_cast<uint16_t>(offset + index + tlv.GetSize()), linkMetrics));
            hasEnhAckProbingTlv = true;
            break;

        default:
            break;
        }

        index += tlv.GetSize();
    }

    if (hasForwardProbingRegistrationTlv)
    {
        aStatus = ConfigureForwardTrackingSeries(seriesId, seriesFlags, linkMetrics, aNeighbor);
    }
    else if (hasEnhAckProbingTlv)
    {
        aStatus = ConfigureEnhAckProbing(enhAckFlags, linkMetrics, aNeighbor);
    }

exit:
    return error;
}

otError LinkMetrics::HandleLinkMetricsManagementResponse(const Message &aMessage, const Ip6::Address &aAddress)
{
    otError           error = OT_ERROR_NONE;
    Tlv               tlv;
    uint16_t          offset;
    uint16_t          length;
    uint16_t          index = 0;
    LinkMetricsStatus status;
    bool              hasStatus = false;

    VerifyOrExit(mLinkMetricsMgmtResponseCallback != nullptr);

    SuccessOrExit(error = Tlv::FindTlvValueOffset(aMessage, Mle::Tlv::Type::kLinkMetricsManagement, offset, length));

    while (index < length)
    {
        SuccessOrExit(aMessage.Read(offset + index, tlv));

        switch (tlv.GetType())
        {
        case kLinkMetricsStatus:
            VerifyOrExit(!hasStatus, error = OT_ERROR_PARSE);
            VerifyOrExit(tlv.GetLength() == sizeof(status), error = OT_ERROR_PARSE);
            SuccessOrExit(aMessage.Read(offset + index + sizeof(tlv), status));
            hasStatus = true;
            break;

        default:
            break;
        }

        index += tlv.GetSize();
    }

    VerifyOrExit(hasStatus, error = OT_ERROR_PARSE);

    mLinkMetricsMgmtResponseCallback(&aAddress, status, mLinkMetricsMgmtResponseCallbackContext);

exit:
    return error;
}

void LinkMetrics::HandleLinkMetricsReport(const Message &     aMessage,
                                          uint16_t            aOffset,
                                          uint16_t            aLength,
                                          const Ip6::Address &aAddress)
{
    otError                error = OT_ERROR_NONE;
    otLinkMetricsValues    metricsValues;
    uint8_t                metricsRawValue;
    uint16_t               pos    = aOffset;
    uint16_t               endPos = aOffset + aLength;
    Tlv                    tlv;
    LinkMetricsTypeIdFlags typeIdFlags;
    bool                   hasStatus = false;
    bool                   hasReport = false;
    LinkMetricsStatus      status;

    OT_UNUSED_VARIABLE(error);

    VerifyOrExit(mLinkMetricsReportCallback != nullptr);

    memset(&metricsValues, 0, sizeof(metricsValues));

    while (pos < endPos)
    {
        SuccessOrExit(aMessage.Read(pos, tlv));
        VerifyOrExit(tlv.GetType() == kLinkMetricsReportSub);
        pos += sizeof(Tlv);

        VerifyOrExit(pos + tlv.GetLength() <= endPos, error = OT_ERROR_PARSE);

        switch (tlv.GetType())
        {
        case kLinkMetricsStatus:
            VerifyOrExit(!hasStatus && !hasReport,
                         error = OT_ERROR_DROP); // There should be either: one Status TLV or some Report-Sub TLVs
            VerifyOrExit(tlv.GetLength() == sizeof(status), error = OT_ERROR_PARSE);
            SuccessOrExit(aMessage.Read(pos, status));
            hasStatus = true;
            pos += sizeof(status);
            break;

        case kLinkMetricsReportSub:
            VerifyOrExit(!hasStatus,
                         error = OT_ERROR_DROP); // There shouldn't be any Report-Sub TLV when there's a Status TLV
            VerifyOrExit(tlv.GetLength() > sizeof(typeIdFlags), error = OT_ERROR_PARSE);
            SuccessOrExit(aMessage.Read(pos, typeIdFlags));
            if (typeIdFlags.IsExtendedFlagSet())
            {
                pos += tlv.GetLength(); // Skip the whole sub-TLV if `E` flag is set
                continue;
            }
            hasReport = true;
            pos += sizeof(LinkMetricsTypeIdFlags);

            switch (typeIdFlags.GetRawValue())
            {
            case kTypeIdFlagPdu:
                metricsValues.mMetrics.mPduCount = true;
                SuccessOrExit(aMessage.Read(pos, metricsValues.mPduCountValue));
                pos += sizeof(uint32_t);
                otLogDebgMle(" - PDU Counter: %d (Count/Summation)", metricsValues.mPduCountValue);
                break;

            case kTypeIdFlagLqi:
                metricsValues.mMetrics.mLqi = true;
                SuccessOrExit(aMessage.Read(pos, metricsValues.mLqiValue));
                pos += sizeof(uint8_t);
                otLogDebgMle(" - LQI: %d (Exponential Moving Average)", metricsValues.mLqiValue);
                break;

            case kTypeIdFlagLinkMargin:
                metricsValues.mMetrics.mLinkMargin = true;
                SuccessOrExit(aMessage.Read(pos, metricsRawValue));
                metricsValues.mLinkMarginValue =
                    metricsRawValue * 130 / 255; // Reverse operation for linear scale, map from [0, 255] to [0, 130]
                pos += sizeof(uint8_t);
                otLogDebgMle(" - Margin: %d (dB) (Exponential Moving Average)", metricsValues.mLinkMarginValue);
                break;

            case kTypeIdFlagRssi:
                metricsValues.mMetrics.mRssi = true;
                SuccessOrExit(aMessage.Read(pos, metricsRawValue));
                metricsValues.mRssiValue = metricsRawValue * 130 / 255 -
                                           130; // Reverse operation for linear scale, map from [0, 255] to [-130, 0]
                pos += sizeof(uint8_t);
                otLogDebgMle(" - RSSI: %d (dBm) (Exponential Moving Average)", metricsValues.mRssiValue);
                break;

            default:
                break;
            }
            break;
        }
    }

    if (hasStatus)
    {
        mLinkMetricsReportCallback(&aAddress, nullptr, status, mLinkMetricsReportCallbackContext);
    }
    else if (hasReport)
    {
        mLinkMetricsReportCallback(&aAddress, &metricsValues, OT_LINK_METRICS_STATUS_SUCCESS,
                                   mLinkMetricsReportCallbackContext);
    }

exit:
    otLogDebgMle("HandleLinkMetricsReport, error:%s", otThreadErrorToString(error));
    return;
}

otError LinkMetrics::HandleLinkProbe(const Message &aMessage, uint8_t &aSeriesId)
{
    otError  error = OT_ERROR_NONE;
    uint16_t offset;
    uint16_t length;

    SuccessOrExit(error = Tlv::FindTlvValueOffset(aMessage, Mle::Tlv::Type::kLinkProbe, offset, length));
    VerifyOrExit(length >= sizeof(aSeriesId), error = OT_ERROR_PARSE);
    SuccessOrExit(error = aMessage.Read(offset, aSeriesId));
    VerifyOrExit(aSeriesId >= kQueryIdSingleProbe && aSeriesId <= kSeriesIdAllSeries, error = OT_ERROR_INVALID_ARGS);

exit:
    return error;
}

void LinkMetrics::SetLinkMetricsReportCallback(otLinkMetricsReportCallback aCallback, void *aCallbackContext)
{
    mLinkMetricsReportCallback        = aCallback;
    mLinkMetricsReportCallbackContext = aCallbackContext;
}

void LinkMetrics::SetLinkMetricsMgmtResponseCallback(otLinkMetricsMgmtResponseCallback aCallback,
                                                     void *                            aCallbackContext)
{
    mLinkMetricsMgmtResponseCallback        = aCallback;
    mLinkMetricsMgmtResponseCallbackContext = aCallbackContext;
}

void LinkMetrics::SetLinkMetricsEnhAckProbingCallback(otLinkMetricsEnhAckProbingIeReportCallback aCallback,
                                                      void *                                     aCallbackContext)
{
    mLinkMetricsEnhAckProbingIeReportCallback        = aCallback;
    mLinkMetricsEnhAckProbingIeReportCallbackContext = aCallbackContext;
}

void LinkMetrics::ProcessEnhAckIeData(const uint8_t *aData, uint8_t aLen, const Neighbor &aNeighbor)
{
    otLinkMetricsValues linkMetricsValues;
    uint8_t             idx = 0;

    VerifyOrExit(mLinkMetricsEnhAckProbingIeReportCallback != nullptr);

    linkMetricsValues.mMetrics = aNeighbor.GetEnhAckProbingMetrics();

    if (linkMetricsValues.mMetrics.mLqi && idx < aLen)
    {
        linkMetricsValues.mLqiValue = aData[idx++];
    }
    if (linkMetricsValues.mMetrics.mLinkMargin && idx < aLen)
    {
        linkMetricsValues.mLinkMarginValue = aData[idx++];
    }
    if (linkMetricsValues.mMetrics.mRssi && idx < aLen)
    {
        linkMetricsValues.mRssiValue = aData[idx++];
    }

    mLinkMetricsEnhAckProbingIeReportCallback(aNeighbor.GetRloc16(), &aNeighbor.GetExtAddress(), &linkMetricsValues,
                                              mLinkMetricsEnhAckProbingIeReportCallbackContext);

exit:
    return;
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

LinkMetrics::LinkMetricsStatus LinkMetrics::ConfigureForwardTrackingSeries(uint8_t              aSeriesId,
                                                                           const SeriesFlags &  aSeriesFlags,
                                                                           const otLinkMetrics &aLinkMetrics,
                                                                           Neighbor &           aNeighbor)
{
    LinkMetricsStatus status = kLinkMetricsStatusSuccess;

    VerifyOrExit(0 < aSeriesId, status = kLinkMetricsStatusOtherError);
    if (aSeriesFlags.GetRawValue() == 0) // Remove the series
    {
        if (aSeriesId == kSeriesIdAllSeries) // Remove all
        {
            aNeighbor.RemoveAllForwardTrackingSeriesInfo();
        }
        else
        {
            LinkMetricsSeriesInfo *seriesInfo = aNeighbor.RemoveForwardTrackingSeriesInfo(aSeriesId);
            VerifyOrExit(seriesInfo != nullptr, status = kLinkMetricsStatusSeriesIdNotRecognized);
            mLinkMetricsSeriesInfoPool.Free(*seriesInfo);
        }
    }
    else // Add a new series
    {
        LinkMetricsSeriesInfo *seriesInfo = aNeighbor.GetForwardTrackingSeriesInfo(aSeriesId);
        VerifyOrExit(seriesInfo == nullptr, status = kLinkMetricsStatusSeriesIdAlreadyRegistered);
        seriesInfo = mLinkMetricsSeriesInfoPool.Allocate();
        VerifyOrExit(seriesInfo != nullptr, status = kLinkMetricsStatusCannotSupportNewSeries);

        seriesInfo->Init(aSeriesId, aSeriesFlags, aLinkMetrics);

        aNeighbor.AddForwardTrackingSeriesInfo(*seriesInfo);
    }

exit:
    return status;
}

LinkMetrics::LinkMetricsStatus LinkMetrics::ConfigureEnhAckProbing(LinkMetricsEnhAckFlags aEnhAckFlags,
                                                                   const otLinkMetrics &  aLinkMetrics,
                                                                   Neighbor &             aNeighbor)
{
    LinkMetricsStatus status = kLinkMetricsStatusSuccess;
    otError           error  = OT_ERROR_NONE;

    VerifyOrExit(!aLinkMetrics.mReserved, status = kLinkMetricsStatusCannotSupportNewSeries);

    if (aEnhAckFlags == kEnhAckRegister)
    {
        VerifyOrExit(!aLinkMetrics.mPduCount, status = kLinkMetricsStatusOtherError);
        VerifyOrExit(aLinkMetrics.mLqi || aLinkMetrics.mLinkMargin || aLinkMetrics.mRssi,
                     status = kLinkMetricsStatusOtherError);
        VerifyOrExit(!(aLinkMetrics.mLqi && aLinkMetrics.mLinkMargin && aLinkMetrics.mRssi),
                     status = kLinkMetricsStatusOtherError);

        error = Get<Radio>().ConfigureEnhAckProbing(aLinkMetrics, aNeighbor.GetRloc16(), aNeighbor.GetExtAddress());
    }
    else if (aEnhAckFlags == kEnhAckClear)
    {
        VerifyOrExit(!aLinkMetrics.mLqi && !aLinkMetrics.mLinkMargin && !aLinkMetrics.mRssi,
                     status = kLinkMetricsStatusOtherError);
        error = Get<Radio>().ConfigureEnhAckProbing(aLinkMetrics, aNeighbor.GetRloc16(), aNeighbor.GetExtAddress());
    }
    else
    {
        status = kLinkMetricsStatusOtherError;
    }

    VerifyOrExit(error == OT_ERROR_NONE, status = kLinkMetricsStatusOtherError);

exit:
    return status;
}

Neighbor *LinkMetrics::GetNeighborFromLinkLocalAddr(const Ip6::Address &aDestination)
{
    Neighbor *   neighbor = nullptr;
    Mac::Address macAddress;

    VerifyOrExit(aDestination.IsLinkLocal());
    aDestination.GetIid().ConvertToMacAddress(macAddress);
    neighbor = Get<NeighborTable>().FindNeighbor(macAddress);

exit:
    return neighbor;
}

otError LinkMetrics::ReadTypeIdFlagsFromMessage(const Message &aMessage,
                                                uint8_t        aStartPos,
                                                uint8_t        aEndPos,
                                                otLinkMetrics &aLinkMetrics)
{
    otError error = OT_ERROR_NONE;

    memset(&aLinkMetrics, 0, sizeof(aLinkMetrics));

    for (uint16_t pos = aStartPos; pos < aEndPos; pos += sizeof(LinkMetricsTypeIdFlags))
    {
        LinkMetricsTypeIdFlags typeIdFlags;

        SuccessOrExit(aMessage.Read(pos, typeIdFlags));

        switch (typeIdFlags.GetRawValue())
        {
        case kTypeIdFlagPdu:
            VerifyOrExit(!aLinkMetrics.mPduCount, error = OT_ERROR_PARSE);
            aLinkMetrics.mPduCount = true;
            break;

        case kTypeIdFlagLqi:
            VerifyOrExit(!aLinkMetrics.mLqi, error = OT_ERROR_PARSE);
            aLinkMetrics.mLqi = true;
            break;

        case kTypeIdFlagLinkMargin:
            VerifyOrExit(!aLinkMetrics.mLinkMargin, error = OT_ERROR_PARSE);
            aLinkMetrics.mLinkMargin = true;
            break;

        case kTypeIdFlagRssi:
            VerifyOrExit(!aLinkMetrics.mRssi, error = OT_ERROR_PARSE);
            aLinkMetrics.mRssi = true;
            break;

        default:
            if (typeIdFlags.IsExtendedFlagSet())
            {
                pos += sizeof(uint8_t); // Skip the additional second flags byte.
            }
            else
            {
                aLinkMetrics.mReserved = true;
            }
            break;
        }
    }

exit:
    return error;
}

otError LinkMetrics::AppendReportSubTlvToMessage(Message &                  aMessage,
                                                 uint8_t &                  aLength,
                                                 const otLinkMetricsValues &aValues)
{
    otError                 error = OT_ERROR_NONE;
    LinkMetricsReportSubTlv metric;

    aLength = 0;

    // Link Metrics Report sub-TLVs
    if (aValues.mMetrics.mPduCount)
    {
        metric.Init();
        metric.SetMetricsTypeId(LinkMetricsTypeIdFlags(kTypeIdFlagPdu));
        metric.SetMetricsValue32(aValues.mPduCountValue);
        SuccessOrExit(error = aMessage.AppendBytes(&metric, metric.GetSize()));
        aLength += metric.GetSize();
    }

    if (aValues.mMetrics.mLqi)
    {
        metric.Init();
        metric.SetMetricsTypeId(LinkMetricsTypeIdFlags(kTypeIdFlagLqi));
        metric.SetMetricsValue8(aValues.mLqiValue);
        SuccessOrExit(error = aMessage.AppendBytes(&metric, metric.GetSize()));
        aLength += metric.GetSize();
    }

    if (aValues.mMetrics.mLinkMargin)
    {
        metric.Init();
        metric.SetMetricsTypeId(LinkMetricsTypeIdFlags(kTypeIdFlagLinkMargin));
        metric.SetMetricsValue8(aValues.mLinkMarginValue);
        SuccessOrExit(error = aMessage.AppendBytes(&metric, metric.GetSize()));
        aLength += metric.GetSize();
    }

    if (aValues.mMetrics.mRssi)
    {
        metric.Init();
        metric.SetMetricsTypeId(LinkMetricsTypeIdFlags(kTypeIdFlagRssi));
        metric.SetMetricsValue8(aValues.mRssiValue);
        SuccessOrExit(error = aMessage.AppendBytes(&metric, metric.GetSize()));
        aLength += metric.GetSize();
    }

exit:
    return error;
}

otError LinkMetrics::AppendStatusSubTlvToMessage(Message &aMessage, uint8_t &aLength, LinkMetricsStatus aStatus)
{
    otError error = OT_ERROR_NONE;
    Tlv     statusTlv;

    statusTlv.SetType(kLinkMetricsStatus);
    statusTlv.SetLength(sizeof(uint8_t));
    SuccessOrExit(error = aMessage.AppendBytes(&statusTlv, sizeof(statusTlv)));
    SuccessOrExit(error = aMessage.AppendBytes(&aStatus, sizeof(aStatus)));
    aLength += statusTlv.GetSize();

exit:
    return error;
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
