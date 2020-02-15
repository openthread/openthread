/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements link metrics probing protocol.
 */

#include "link_probing.hpp"

#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_CONFIG_LINK_PROBE_ENABLE

namespace ot {
namespace LinkProbing {

bool LinkMetricsInfoEntry::IsFrameTypeMatch(uint8_t aFrameType)
{
    bool isMatch = false;
    switch (aFrameType)
    {
    case Mac::Frame::kFcfFrameData:
        isMatch = mSeriesFlags.mMacData;
        break;
    case Mac::Frame::kFcfFrameMacCmd:
        isMatch = mSeriesFlags.mMacDataRequest;
        break;
    case Mac::Frame::kFcfFrameAck:
        isMatch = mSeriesFlags.mAck;
        break;
    case kFrameTypeLinkProbe:
        // Special case, when mac data is set, ignore Mle Link Probe
        isMatch = (!mSeriesFlags.mMacData) & mSeriesFlags.mMleLinkProbes;
        break;
    }
    return isMatch;
}

void LinkMetricsInfoEntry::AggregateLinkMetrics(uint8_t aLqi, int8_t aRss)
{
    mPsduCount++;
    mLqiAverager.Add(aLqi);
    mRssAverager.Add(static_cast<int16_t>(aRss));
}

void LinkMetricsInfoEntry::Reset()
{
    mPsduCount = 0;
    mLqiAverager.Reset();
    mRssAverager.Reset();
}

void LinkMetricsInfoEntry::GetLinkMetricsReport(int8_t                   aNoiseFloor,
                                                LinkMetricsReportSubTlv *aDstMetrics,
                                                uint8_t &                aMetricsCount)
{
    aMetricsCount = mTypeIdFlagsCount;
    for (uint8_t i = 0; i < aMetricsCount; i++)
    {
        aDstMetrics[i].Init();
        aDstMetrics[i].SetMetricTypeId(mTypeIdFlags[i]);
        switch (mTypeIdFlags[i].GetMetricId())
        {
        case OT_LINK_PDU_COUNT:
            if (mTypeIdFlags[i].IsLengthFlagSet())
            {
                aDstMetrics[i].SetMetricValue32(mPsduCount);
            }
            else
            {
                aDstMetrics[i].SetMetricValue8(mPsduCount);
            }
            break;

        case OT_LINK_LQI:
            if (mTypeIdFlags[i].IsLengthFlagSet())
            {
                aDstMetrics[i].SetMetricValue32(static_cast<uint8_t>(mLqiAverager.GetAverage()));
            }
            else
            {
                aDstMetrics[i].SetMetricValue8(static_cast<uint8_t>(mLqiAverager.GetAverage()));
            }
            break;

        case OT_LINK_RSSI:
            if (mTypeIdFlags[i].IsLengthFlagSet())
            {
                aDstMetrics[i].SetMetricValue32(static_cast<uint8_t>(mRssAverager.GetAverage()));
            }
            else
            {
                aDstMetrics[i].SetMetricValue8(static_cast<uint8_t>(mRssAverager.GetAverage()));
            }
            break;

        case OT_LINK_MARGIN:
            if (mTypeIdFlags[i].IsLengthFlagSet())
            {
                aDstMetrics[i].SetMetricValue32(
                    LinkQualityInfo::ConvertRssToLinkMargin(aNoiseFloor, mRssAverager.GetAverage()));
            }
            else
            {
                aDstMetrics[i].SetMetricValue8(
                    LinkQualityInfo::ConvertRssToLinkMargin(aNoiseFloor, mRssAverager.GetAverage()));
            }
            break;

        default:
            break;
        }
    }
}

inline void LinkMetricsInfoEntry::GetLinkMetricsValue(int8_t aNoiseFloor, uint8_t *aContent, uint8_t &aCount)
{
    aCount = mTypeIdFlagsCount > 2 ? 2 : mTypeIdFlagsCount;
    for (uint8_t i = 0; i < aCount; i++)
    {
        switch (mTypeIdFlags[i].GetMetricId())
        {
        case OT_LINK_LQI:
            aContent[i] = static_cast<uint8_t>(mLqiAverager.GetAverage());
            break;

        case OT_LINK_RSSI:
            aContent[i] = static_cast<uint8_t>(mRssAverager.GetAverage());
            break;

        case OT_LINK_MARGIN:
            aContent[i] = LinkQualityInfo::ConvertRssToLinkMargin(aNoiseFloor, mRssAverager.GetAverage());
            break;

        default:
            break;
        }
    }
}

inline void LinkMetricsInfoEntry::SetTypeIdFlags(LinkMetricTypeId *aTypeIdFlags, uint8_t aTypeIdFlagsCount)
{
    memcpy(mTypeIdFlags, aTypeIdFlags, aTypeIdFlagsCount);
    mTypeIdFlagsCount = aTypeIdFlagsCount;
}

void LinkMetricsInfo::PrintEntries()
{
    otLogWarnMac("Current series count:%u", mLinkMetricsInfoEntriesCount);
    for (int i = 0; i < mLinkMetricsInfoEntriesCount; i++)
    {
        otLogWarnMac("SeriesId:%u metricsCount:%u", mLinkMetricsInfoEntries[i].GetSeriesId(),
                     mLinkMetricsInfoEntries[i].GetTmp());
    }
}

uint8_t LinkMetricsInfo::RegisterForwardProbing(uint8_t           aSeriesId,
                                                uint8_t           aSeriesFlags,
                                                LinkMetricTypeId *aTypeIdFlags,
                                                uint8_t           aTypeIdFlagsCount)
{
    uint8_t               status = kLinkMetricsStatusSuccess;
    LinkMetricsInfoEntry *entry  = NULL;

    VerifyOrExit(0 < aSeriesId, status = kLinkMetricsStatusSeriesIdNotRecognized);
    if (aSeriesFlags == 0) // Clear, remove the entry
    {
        if (aSeriesId == SERIES_ID_MAX_SIZE) // Clear all
        {
            RemoveAll();
        }
        else
        {
            status = RemoveLinkMetricsInfoEntry(aSeriesId);
        }
    }
    else
    {
        entry = FindLinkMetricsInfoEntry(aSeriesId);
        VerifyOrExit(entry == NULL, status = kLinkMetricsStatusSeriesIdAlreadyRegistered);
        AddLinkMetricsInfoEntry(aSeriesId, aSeriesFlags, aTypeIdFlags, aTypeIdFlagsCount);
    }

exit:
    otLogWarnMac("RegisterForwardProbing, status:%u", status);
    PrintEntries();
    return status;
}

uint8_t LinkMetricsInfo::ConfigureEnhancedAckProbing(Instance &          aInstance,
                                                     uint8_t             aEnhancedAckFlags,
                                                     LinkMetricTypeId *  aTypeIdFlags,
                                                     uint8_t             aTypeIdFlagsCount,
                                                     uint16_t            aRloc16,
                                                     const otExtAddress &aExtAddress)
{
    uint8_t status = kLinkMetricsStatusSuccess;
    VerifyOrExit(aTypeIdFlags->GetMetricType() <= kTypeAverageEnumExponential &&
                     aTypeIdFlagsCount <= kEnhancedAckProbingTypeIdCountMax,
                 status = kLinkMetricsStatusOtherError);

    if (aEnhancedAckFlags == 0) // Clear
    {
        mEnhancedAckProbingEnabled = false;
        otPlatRadioEnableEnhAckLinkMetrics(&aInstance, false, 0, &aRloc16, &aExtAddress);
    }
    else if (aEnhancedAckFlags == 1) // Register
    {
        mEnhancedAckProbingEnabled = true;
        mLinkMetricsInfoEnhancedAckEntry.Reset();
        mLinkMetricsInfoEnhancedAckEntry.SetTypeIdFlags(aTypeIdFlags, aTypeIdFlagsCount);
        otPlatRadioEnableEnhAckLinkMetrics(&aInstance, true, aTypeIdFlagsCount, &aRloc16, &aExtAddress);
    }
    else
    {
        status = kLinkMetricsStatusOtherError;
    }

exit:
    return status;
}

void LinkMetricsInfo::AggregateLinkMetrics(uint8_t aFrameType, uint8_t aLqi, int8_t aRss)
{
    // Aggregate for forward tracking series
    for (uint8_t i = 0; i < mLinkMetricsInfoEntriesCount; i++)
    {
        if (mLinkMetricsInfoEntries[i].IsFrameTypeMatch(aFrameType))
        {
            mLinkMetricsInfoEntries[i].AggregateLinkMetrics(aLqi, aRss);
        }
    }

    // Aggregate for enhanced-ack based probing
    if (mEnhancedAckProbingEnabled)
    {
        mLinkMetricsInfoEnhancedAckEntry.AggregateLinkMetrics(aLqi, aRss);
    }

    return;
}

uint8_t LinkMetricsInfo::GetForwardMetricsReport(uint8_t                  aQueryId,
                                                 int8_t                   aNoiseFloor,
                                                 LinkMetricsReportSubTlv *aDstMetrics,
                                                 uint8_t &                aMetricsCount)
{
    uint8_t               status = kLinkMetricsStatusSuccess;
    LinkMetricsInfoEntry *entry  = FindLinkMetricsInfoEntry(aQueryId);

    VerifyOrExit(entry != NULL, status = kLinkMetricsStatusSeriesIdNotRecognized);
    entry->GetLinkMetricsReport(aNoiseFloor, aDstMetrics, aMetricsCount);

exit:
    return status;
}

void LinkMetricsInfo::GetEnhancedAckMetricsValue(int8_t aNoiseFloor, uint8_t *aContent, uint8_t &aCount)
{
    mLinkMetricsInfoEnhancedAckEntry.GetLinkMetricsValue(aNoiseFloor, aContent, aCount);
}

void LinkMetricsInfo::Clear()
{
    mEnhancedAckProbingEnabled = false;
    for (int i = 0; i < SERIES_ID_MAX_SIZE; i++)
    {
        mLinkMetricsInfoEntries[i].Reset();
    }
    mLinkMetricsInfoEnhancedAckEntry.Reset();
}

void LinkMetricsInfo::AddLinkMetricsInfoEntry(uint8_t           aSeriesId,
                                              uint8_t           aSeriesFlags,
                                              LinkMetricTypeId *aTypeIdFlags,
                                              uint8_t           aTypeIdFlagsCount)
{
    VerifyOrExit(mLinkMetricsInfoEntriesCount < SERIES_ID_MAX_SIZE);
    VerifyOrExit(aTypeIdFlagsCount <= kMaxTypeIdFlagsCount);

    mLinkMetricsInfoEntries[mLinkMetricsInfoEntriesCount].Reset();
    mLinkMetricsInfoEntries[mLinkMetricsInfoEntriesCount].SetSeriesId(aSeriesId);
    mLinkMetricsInfoEntries[mLinkMetricsInfoEntriesCount].SetSeriesFlags(aSeriesFlags);
    mLinkMetricsInfoEntries[mLinkMetricsInfoEntriesCount].SetTypeIdFlags(aTypeIdFlags, aTypeIdFlagsCount);

    mLinkMetricsInfoEntriesCount++;
exit:
    return;
}

uint8_t LinkMetricsInfo::GetLinkMetricsInfoEntryIndex(uint8_t aSeriesId)
{
    uint8_t index = SERIES_ID_MAX_SIZE;
    for (uint8_t i = 0; i < mLinkMetricsInfoEntriesCount; i++)
    {
        if (mLinkMetricsInfoEntries[i].GetSeriesId() == aSeriesId)
        {
            index = i;
            break;
        }
    }
    return index;
}

LinkMetricsInfoEntry *LinkMetricsInfo::FindLinkMetricsInfoEntry(uint8_t aSeriesId)
{
    LinkMetricsInfoEntry *entry = NULL;
    uint8_t               index = GetLinkMetricsInfoEntryIndex(aSeriesId);

    if (index != SERIES_ID_MAX_SIZE)
    {
        entry = &mLinkMetricsInfoEntries[index];
    }

    return entry;
}

uint8_t LinkMetricsInfo::RemoveLinkMetricsInfoEntry(uint8_t aSeriesId)
{
    uint8_t error = kLinkMetricsStatusSuccess;
    uint8_t index = GetLinkMetricsInfoEntryIndex(aSeriesId);
    VerifyOrExit(index != SERIES_ID_MAX_SIZE, error = kLinkMetricsStatusSeriesIdNotRecognized);

    for (int i = index; i + 1 < mLinkMetricsInfoEntriesCount; i++)
    {
        memcpy(mLinkMetricsInfoEntries + i, mLinkMetricsInfoEntries + i + 1, sizeof(LinkMetricsInfoEntry));
    }
    mLinkMetricsInfoEntriesCount--;

exit:
    return error;
}

void LinkMetricsInfo::RemoveAll()
{
    mLinkMetricsInfoEntriesCount = 0;
}

LinkProbing::LinkProbing(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mLinkMetricsReportCallback(NULL)
    , mContext(NULL)
{
}

void LinkProbing::SetLinkProbingReportCallback(otLinkMetricsReportCallback aCallback, void *aCallbackContext)
{
    mLinkMetricsReportCallback = aCallback;
    mContext                   = aCallbackContext;
}

uint8_t LinkProbing::SetDefaultLinkMetricTypeIds(LinkMetricTypeId *aLinkMetricTypeIds)
{
    uint8_t metricCount = 0;

    // count
    aLinkMetricTypeIds[metricCount].ClearFollowFlag();
    aLinkMetricTypeIds[metricCount].ClearLengthFlag();
    aLinkMetricTypeIds[metricCount].SetMetricType(OT_LINK_METRIC_COUNT_SUMMATION);
    aLinkMetricTypeIds[metricCount++].SetMetricId(OT_LINK_PDU_COUNT);

    // LQI
    aLinkMetricTypeIds[metricCount].ClearFollowFlag();
    aLinkMetricTypeIds[metricCount].ClearLengthFlag();
    aLinkMetricTypeIds[metricCount].SetMetricType(OT_LINK_METRIC_EXPONENTIAL_MOVING_AVERAGE);
    aLinkMetricTypeIds[metricCount++].SetMetricId(OT_LINK_LQI);

    // RSSI
    aLinkMetricTypeIds[metricCount].ClearFollowFlag();
    aLinkMetricTypeIds[metricCount].ClearLengthFlag();
    aLinkMetricTypeIds[metricCount].SetMetricType(OT_LINK_METRIC_EXPONENTIAL_MOVING_AVERAGE);
    aLinkMetricTypeIds[metricCount++].SetMetricId(OT_LINK_RSSI);

    // Margin
    aLinkMetricTypeIds[metricCount].ClearFollowFlag();
    aLinkMetricTypeIds[metricCount].ClearLengthFlag();
    aLinkMetricTypeIds[metricCount].SetMetricType(OT_LINK_METRIC_EXPONENTIAL_MOVING_AVERAGE);
    aLinkMetricTypeIds[metricCount++].SetMetricId(OT_LINK_MARGIN);

    return metricCount;
}

otError LinkProbing::LinkProbeQuery(const Ip6::Address &aDestination,
                                    uint8_t             aSeriesId,
                                    LinkMetricTypeId *  aTypeIdFlags,
                                    uint8_t             aTypeIdFlagsCount)
{
    otError          error = OT_ERROR_NONE;
    Neighbor *       neighbor;
    LinkMetricTypeId linkMetricTypeIds[kLinkMetricsMaxNum];
    uint8_t          metricsCount = 0;

    VerifyOrExit(aTypeIdFlagsCount <= sizeof(linkMetricTypeIds) / sizeof(linkMetricTypeIds[0]),
                 error = OT_ERROR_INVALID_ARGS);

    // Ensure the destination is a neighbor, and the neighbor's state is valid.
    neighbor = Get<Mle::MleRouter>().GetNeighbor(aDestination);
    VerifyOrExit(neighbor != NULL && neighbor->GetState() == Neighbor::kStateValid, error = OT_ERROR_NOT_FOUND);

    if (aTypeIdFlagsCount > 0)
    {
        memcpy(linkMetricTypeIds, aTypeIdFlags, aTypeIdFlagsCount);
        metricsCount = aTypeIdFlagsCount;
    }

    error = SendLinkMetricsQuery(neighbor->GetRloc16(), aSeriesId, linkMetricTypeIds, metricsCount);

exit:
    return error;
}

otError LinkProbing::ForwardMgmtRequest(const Ip6::Address &aDestination,
                                        uint8_t             aForwardSeriesId,
                                        uint8_t             aForwardSeriesFlags,
                                        LinkMetricTypeId *  aTypeIdFlags,
                                        uint8_t             aTypeIdFlagsCount)
{
    otError                       error = OT_ERROR_NONE;
    ForwardProbingRegistrationTlv forwardTlv;
    Tlv                           tlv;
    Message *                     message;
    uint16_t                      startOffset;
    uint8_t                       length = 0;
    Neighbor *                    neighbor;

    // Ensure the destination is a neighbor, and the neighbor's state is valid.
    neighbor = Get<Mle::MleRouter>().GetNeighbor(aDestination);
    VerifyOrExit(neighbor != NULL && neighbor->GetState() == Neighbor::kStateValid, error = OT_ERROR_NOT_FOUND);

    VerifyOrExit((message = Get<Mle::MleRouter>().NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error =
                      Get<Mle::MleRouter>().AppendHeader(*message, Mle::Header::kCommandLinkMetricsManagementRequest));

    startOffset = message->GetLength();

    // Link Metrics Management TLV
    tlv.SetType(Mle::Tlv::kLinkMetricsMGMT);
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));

    // Forward Probing Registration sub-TLV
    forwardTlv.Init();
    forwardTlv.SetSeriesId(aForwardSeriesId);
    forwardTlv.SetSeriesFlags(aForwardSeriesFlags);

    if (aTypeIdFlagsCount > 0)
    {
        forwardTlv.SetLinkMetricTypeIdList(aTypeIdFlags, aTypeIdFlagsCount);
    }

    SuccessOrExit(error = message->Append(&forwardTlv, sizeof(Tlv) + forwardTlv.GetLength()));
    length += sizeof(Tlv) + forwardTlv.GetLength();

    tlv.SetLength(length);
    message->Write(startOffset, sizeof(tlv), &tlv);

    SuccessOrExit(error = Get<Mle::MleRouter>().SendMessage(*message, aDestination));

    otLogInfoMle("Sent Forward Tracking Request");

exit:
    return error;
}

otError LinkProbing::SendLinkProbeTo(const Ip6::Address &aDestination, uint8_t aDataLength)
{
    otError   error = OT_ERROR_NONE;
    Neighbor *neighbor;

    // Ensure the destination is a neighbor, and the neighbor's state is valid.
    neighbor = Get<Mle::MleRouter>().GetNeighbor(aDestination);
    VerifyOrExit(neighbor != NULL && neighbor->GetState() == Neighbor::kStateValid, error = OT_ERROR_NOT_FOUND);

    SuccessOrExit(error = SendLinkProbe(neighbor->GetRloc16(), 0, aDataLength));

exit:
    return error;
}

otError LinkProbing::EnhancedAckConfigureRequest(const Ip6::Address &aDestination,
                                                 uint8_t             aEnhAckFlags,
                                                 LinkMetricTypeId *  aTypeIdFlags,
                                                 uint8_t             aTypeIdFlagsCount)
{
    otError                     error = OT_ERROR_NONE;
    EnhancedAckConfigurationTlv configTlv;
    Tlv                         tlv;
    Message *                   message;
    uint16_t                    startOffset;
    uint8_t                     length = 0;
    Neighbor *                  neighbor;

    // Ensure the destination is a neighbor, and the neighbor's state is valid.
    neighbor = Get<Mle::MleRouter>().GetNeighbor(aDestination);
    VerifyOrExit(neighbor != NULL && neighbor->GetState() == Neighbor::kStateValid, error = OT_ERROR_NOT_FOUND);

    VerifyOrExit((message = Get<Mle::MleRouter>().NewMleMessage()) != NULL);
    SuccessOrExit(error =
                      Get<Mle::MleRouter>().AppendHeader(*message, Mle::Header::kCommandLinkMetricsManagementRequest));

    startOffset = message->GetLength();

    // Link Metrics Management TLV
    tlv.SetType(Mle::Tlv::kLinkMetricsMGMT);
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));

    // Enhanced ACK configuration sub-TLV
    configTlv.Init();
    configTlv.SetEnhAckFlags(aEnhAckFlags);
    configTlv.SetLinkMetricTypeIdList(aTypeIdFlags, aTypeIdFlagsCount);

    SuccessOrExit(error = message->Append(&configTlv, sizeof(Tlv) + configTlv.GetLength()));
    length += sizeof(Tlv) + configTlv.GetLength();

    tlv.SetLength(length);
    message->Write(startOffset, sizeof(tlv), &tlv);

    SuccessOrExit(error = Get<Mle::MleRouter>().SendMessage(*message, aDestination));

    otLogInfoMle("Sent Enhanced ACK configuration request");

exit:
    return error;
}

void LinkProbing::HandleLinkProbe(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Neighbor *neighbor;

    // Ensure the destination is a neighbor, and the neighbor's state is valid.
    neighbor = Get<Mle::MleRouter>().GetNeighbor(aMessageInfo.GetPeerAddr());
    VerifyOrExit(neighbor != NULL && neighbor->GetState() == Neighbor::kStateValid);

    neighbor->GetLinkMetricsInfo().AggregateLinkMetrics(kFrameTypeLinkProbe, 0,
                                                        aMessage.GetAverageRss());

    otLogInfoMle("Received Link Probe");

exit:
    return;
}

void LinkProbing::HandleLinkMetricsManagementRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError                       error = OT_ERROR_NONE;
    ForwardProbingRegistrationTlv forwardTlv;
    EnhancedAckConfigurationTlv   enhancedAckConfigTlv;
    Neighbor *                    neighbor;
    Tlv                           tlv;
    uint8_t                       status = kLinkMetricsStatusInvalid;
    uint16_t                      offset;

    // Ensure the destination is a neighbor, and the neighbor's state is valid.
    neighbor = Get<Mle::MleRouter>().GetNeighbor(aMessageInfo.GetPeerAddr());
    VerifyOrExit(neighbor != NULL && neighbor->GetState() == Neighbor::kStateValid, error = OT_ERROR_NOT_FOUND);

    otLogInfoMle("Received Link Metrics Management Request");
    // kLinkMetricsMGMT Tlv
    SuccessOrExit(error = Tlv::GetOffset(aMessage, Mle::Tlv::kLinkMetricsMGMT, offset));
    aMessage.Read(offset, sizeof(tlv), &tlv);
    VerifyOrExit(tlv.GetLength() > 0 && aMessage.GetLength() >= offset + sizeof(tlv) + tlv.GetLength(),
                 error = OT_ERROR_PARSE);

    // skip kLinkMetricsMGMT Tlv header, and read its sub-TLV's header;
    aMessage.Read(offset + sizeof(tlv), sizeof(tlv), &tlv);

    switch (tlv.GetType())
    {
    case LinkMetricsTlv::kForwardProbingRegistration:
        aMessage.Read(offset + sizeof(tlv), tlv.GetLength() + sizeof(tlv), &forwardTlv);
        status = neighbor->GetLinkMetricsInfo().RegisterForwardProbing(
            forwardTlv.GetSeriesId(), forwardTlv.GetSeriesFlags(),
            (LinkMetricTypeId *)(forwardTlv.GetLinkMetricsTypeIdList()), forwardTlv.GetLinkMetricsTypeIdCount());
        break;

    case LinkMetricsTlv::kEnhancedACKConfiguration:
        aMessage.Read(offset + sizeof(tlv), tlv.GetLength() + sizeof(tlv), &enhancedAckConfigTlv);
        status = neighbor->GetLinkMetricsInfo().ConfigureEnhancedAckProbing(
            GetInstance(), enhancedAckConfigTlv.GetEnhAckFlags(),
            (LinkMetricTypeId *)(enhancedAckConfigTlv.GetLinkMetricsTypeIdList()),
            enhancedAckConfigTlv.GetLinkMetricsTypeIdCount(), neighbor->GetRloc16(), neighbor->GetExtAddress());
        break;

    default:
        break;
    }

exit:
    SendLinkMetricsManagementResponse(aMessageInfo.GetPeerAddr(), status);
}

void LinkProbing::HandleLinkMetricsManagementResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError              error = OT_ERROR_NONE;
    LinkMetricsStatusTlv statusTlv;
    Tlv                  tlv;
    Neighbor *           neighbor;
    uint16_t             offset;

    neighbor = Get<Mle::MleRouter>().GetNeighbor(aMessageInfo.GetPeerAddr());
    VerifyOrExit(neighbor != NULL, error = OT_ERROR_NOT_FOUND);

    SuccessOrExit(error = Tlv::GetOffset(aMessage, Mle::Tlv::kLinkMetricsMGMT, offset));

    // skip kLinkMetricsMGMT Tlv header, and read its sub-TLV;
    aMessage.Read(offset + sizeof(tlv), sizeof(statusTlv), &statusTlv);
    VerifyOrExit(statusTlv.IsValid(), error = OT_ERROR_PARSE);
    VerifyOrExit(statusTlv.GetStatus() == kLinkMetricsStatusValid, error = OT_ERROR_INVALID_STATE);

    otLogInfoMle("Received Link Metrics Management Response, status: %d", statusTlv.GetStatus());

exit:
    return;
}

void LinkProbing::HandleLinkMetricsReport(const Ip6::MessageInfo &aMessageInfo,
                                          const Message &         aMessage,
                                          uint16_t                aOffset,
                                          uint16_t                aLength)
{
    otLinkMetric metrics[kLinkMetricsMaxNum];
    uint8_t      index  = 0;
    uint16_t     offset = aOffset;
    Tlv          tlv;

    // Verify first Tlv
    aMessage.Read(offset, sizeof(Tlv), &tlv);
    VerifyOrExit(tlv.GetType() == LinkMetricsTlv::kLinkMetricsReportSub);

    while (offset < aOffset + aLength)
    {
        // Read LinkMetricsReportSubTlv into otLinkMetric
        aMessage.Read(offset, sizeof(Tlv), &tlv);
        offset += sizeof(Tlv);
        aMessage.Read(offset, sizeof(otLinkMetricTypeId), &metrics[index].mLinkMetricTypeId);
        offset += sizeof(otLinkMetricTypeId);

        if (metrics[index].mLinkMetricTypeId.mLinkMetricFlagL)
        {
            aMessage.Read(offset, sizeof(uint32_t), &metrics[index].mLinkMetricValue.m32);
            offset += sizeof(uint32_t);
        }
        else
        {
            aMessage.Read(offset, sizeof(uint8_t), &metrics[index].mLinkMetricValue.m8);
            offset += sizeof(uint8_t);
        }

        index++;
    }

    if (mLinkMetricsReportCallback != NULL)
    {
        mLinkMetricsReportCallback(&aMessageInfo.GetPeerAddr(), metrics, index, mContext);
    }

exit:
    return;
}

otError LinkProbing::AppendLinkMetricsReport(Message &                       aMessage,
                                             const Ip6::Address &            aSource,
                                             const Mle::LinkMetricsQueryTlv *aLinkMetricsQuery,
                                             const otThreadLinkInfo *        aLinkInfo)
{
    otError                        error = OT_ERROR_NONE;
    Neighbor *                     neighbor;
    Tlv                            tlv;
    const LinkMetricsQueryId *     queryId      = NULL;
    const LinkMetricsQueryOptions *queryOptions = NULL;
    uint8_t                        length       = 0;
    uint16_t                       startOffset  = aMessage.GetLength();

    neighbor = Get<Mle::MleRouter>().GetNeighbor(aSource);
    VerifyOrExit(neighbor != NULL && neighbor->GetState() == Neighbor::kStateValid, error = OT_ERROR_NOT_FOUND);

    VerifyOrExit(aLinkMetricsQuery != NULL && aLinkMetricsQuery->GetLength() > 0, error = OT_ERROR_INVALID_ARGS);
    queryId = aLinkMetricsQuery->GetQueryId();
    VerifyOrExit(queryId->IsValid(), error = OT_ERROR_PARSE);
    VerifyOrExit(
        queryId->GetLength() > 0 && queryId->GetSeriesId() < 255,
        error =
            OT_ERROR_INVALID_ARGS); // The Link Metrics Query TLV payload MUST include the Link Metrics Query ID Sub-TLV

    // Link Metrics Report TLV
    tlv.SetType(Mle::Tlv::kLinkMetricsReport);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    if (queryId->GetSeriesId() == 0)
    {
        queryOptions = aLinkMetricsQuery->GetQueryOptions();
        VerifyOrExit(queryOptions->IsValid(), error = OT_ERROR_PARSE);
        SuccessOrExit(error = AppendSingleProbeLinkMetricsReport(queryOptions, aLinkInfo,
                                                                 Get<Mac::Mac>().GetNoiseFloor(), aMessage, length));
    }
    else
    {
        SuccessOrExit(error = AppendForwardTrackingSeriesLinkMetricsReport(
                          queryId->GetSeriesId(), Get<Mac::Mac>().GetNoiseFloor(), neighbor->GetLinkMetricsInfo(),
                          aMessage, length));
    }

    tlv.SetLength(length);
    aMessage.Write(startOffset, sizeof(tlv), &tlv);

exit:
    return error;
}

void LinkProbing::HandleLinkMetrics(const Mac::Address &aAddr, otLinkMetric *aMetrics, uint8_t aMetricsCount)
{
    Ip6::Address src;
    Neighbor *   neighbor = Get<Mle::MleRouter>().GetNeighbor(aAddr);

    if (neighbor != NULL)
    {
        if (mLinkMetricsReportCallback != NULL)
        {
            memset(&src, 0, sizeof(src));
            src.mFields.m16[0] = HostSwap16(0xfe80);
            src.SetIid(neighbor->GetExtAddress());

            mLinkMetricsReportCallback(&src, aMetrics, aMetricsCount, mContext);
        }
    }
}

otError LinkProbing::AppendLinkProbe(Message &aMessage, uint8_t aSeriesId, uint8_t aDataLength)
{
    otError           error = OT_ERROR_NONE;
    Mle::LinkProbeTlv tlv;
    char              data[kMaxLinkProbingDataLength];

    VerifyOrExit(aDataLength <= kMaxLinkProbingDataLength, error = OT_ERROR_INVALID_ARGS);

    tlv.Init();
    tlv.SetSeriesId(aSeriesId);
    tlv.SetLength(tlv.GetLength() + aDataLength); // no data field in the tlv, but add the length for its function
    VerifyOrExit((error = aMessage.Append(&tlv, sizeof(tlv))) == OT_ERROR_NONE);
    error = aMessage.Append(data, aDataLength);

exit:
    return error;
}

otError LinkProbing::SendLinkMetricsQuery(uint16_t          aRloc16,
                                          uint8_t           aSeriesId,
                                          LinkMetricTypeId *aTypeIdFlags,
                                          uint8_t           aTypeIdFlagsCount)
{
    otError                 error = OT_ERROR_NONE;
    Tlv                     tlv;
    LinkMetricsQueryId      linkMetricsQueryId;
    LinkMetricsQueryOptions linkMetricsQueryOptions;
    Message *               message;
    Ip6::Address            destination;
    Neighbor *              neighbor;
    uint16_t                startOffset;
    uint8_t                 length = 0;
    static const uint8_t    tlvs[] = {Mle::Tlv::kLinkMetricsReport};

    neighbor = Get<Mle::MleRouter>().GetNeighbor(aRloc16);
    VerifyOrExit(neighbor != NULL, error = OT_ERROR_NOT_FOUND);

    VerifyOrExit((message = Get<Mle::MleRouter>().NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = Get<Mle::MleRouter>().AppendHeader(*message, Mle::Header::kCommandDataRequest));
    SuccessOrExit(error = Get<Mle::MleRouter>().AppendTlvRequest(*message, tlvs, sizeof(tlvs)));
    SuccessOrExit(error = Get<Mle::MleRouter>().AppendActiveTimestamp(*message));

    startOffset = message->GetLength();

    // Link Metrics Query TLV
    tlv.SetType(Mle::Tlv::kLinkMetricsQuery);
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));

    // Link Metrics Query Id sub-TLV
    linkMetricsQueryId.Init();
    linkMetricsQueryId.SetSeriesId(aSeriesId);

    SuccessOrExit(error = message->Append(&linkMetricsQueryId, sizeof(Tlv) + linkMetricsQueryId.GetLength()));
    length += sizeof(Tlv) + linkMetricsQueryId.GetLength();

    // Link Metrics Query Options sub-TLV
    if (aTypeIdFlagsCount > 0)
    {
        linkMetricsQueryOptions.Init();
        linkMetricsQueryOptions.SetLinkMetricTypeIdList(aTypeIdFlags, aTypeIdFlagsCount);

        SuccessOrExit(error =
                          message->Append(&linkMetricsQueryOptions, sizeof(Tlv) + linkMetricsQueryOptions.GetLength()));
        length += sizeof(Tlv) + linkMetricsQueryOptions.GetLength();
    }

    tlv.SetLength(length);
    message->Write(startOffset, sizeof(tlv), &tlv);

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(neighbor->GetExtAddress());

    SuccessOrExit(error = Get<Mle::MleRouter>().SendMessage(*message, destination));

    otLogInfoMle("Sent Link Metrics Query");

exit:
    return error;
}

otError LinkProbing::SendLinkProbe(uint16_t aRloc16, uint8_t aSeriesId, uint8_t aDataLength)
{
    otError      error = OT_ERROR_FAILED;
    Neighbor *   neighbor;
    Message *    message;
    Ip6::Address destination;

    neighbor = Get<Mle::MleRouter>().GetNeighbor(aRloc16);
    VerifyOrExit(neighbor != NULL, error = OT_ERROR_NOT_FOUND);

    VerifyOrExit((message = Get<Mle::MleRouter>().NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = Get<Mle::MleRouter>().AppendHeader(*message, Mle::Header::kCommandLinkProbe));
    SuccessOrExit(error = AppendLinkProbe(*message, aSeriesId, aDataLength));

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(neighbor->GetExtAddress());

    SuccessOrExit(error = Get<Mle::MleRouter>().SendMessage(*message, destination));

    otLogInfoMle("Sent Link Probe");

exit:
    return error;
}

otError LinkProbing::SendLinkMetricsManagementResponse(const Ip6::Address &aDestination, uint8_t aStatus)
{
    otError              error = OT_ERROR_NONE;
    LinkMetricsStatusTlv statusTlv;
    Tlv                  tlv;
    Message *            message;
    uint16_t             startOffset;
    uint8_t              length = 0;

    VerifyOrExit((message = Get<Mle::MleRouter>().NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error =
                      Get<Mle::MleRouter>().AppendHeader(*message, Mle::Header::kCommandLinkMetricsManagementResponse));

    startOffset = message->GetLength();

    // Link Metrics Management TLV
    tlv.SetType(Mle::Tlv::kLinkMetricsMGMT);
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));

    // Link Metrics Status sub-TLV
    statusTlv.Init();
    statusTlv.SetStatus(aStatus);

    SuccessOrExit(error = message->Append(&statusTlv, sizeof(Tlv) + statusTlv.GetLength()));
    length += sizeof(Tlv) + statusTlv.GetLength();

    tlv.SetLength(length);
    message->Write(startOffset, sizeof(tlv), &tlv);

    error = Get<Mle::MleRouter>().SendMessage(*message, aDestination);

    otLogInfoMle("Sent Link Metrics Management Response");

exit:
    return error;
}

otError LinkProbing::AppendSingleProbeLinkMetricsReport(const LinkMetricsQueryOptions *aQueryOptions,
                                                        const otThreadLinkInfo *       aLinkInfo,
                                                        const int8_t                   aNoiseFloor,
                                                        Message &                      aMessage,
                                                        uint8_t &                      aLength)
{
    otError                 error = OT_ERROR_NONE;
    LinkMetricsReportSubTlv metric;
    LinkMetricTypeId        linkMetricTypeIds[kLinkMetricsMaxNum];
    uint8_t                 metricsCount = 0;

    if (aQueryOptions != NULL && aQueryOptions->GetLength() > 0)
    {
        metricsCount = kLinkMetricsMaxNum;
        aQueryOptions->GetLinkMetricTypeIdList(linkMetricTypeIds, &metricsCount);
    }
    else
    {
        metricsCount = SetDefaultLinkMetricTypeIds(linkMetricTypeIds);
    }

    for (uint8_t i = 0; i < metricsCount; i++)
    {
        // Link Metrics Report sub-TLVs
        metric.Init();
        metric.SetMetricTypeId(linkMetricTypeIds[i]);

        switch (linkMetricTypeIds[i].GetMetricId())
        {
        case OT_LINK_PDU_COUNT:
            if (linkMetricTypeIds[i].IsLengthFlagSet())
            {
                metric.SetMetricValue32(1); // 1 for single probe
            }
            else
            {
                metric.SetMetricValue8(1); // 1 for single probe
            }
            break;

        case OT_LINK_LQI:
            if (linkMetricTypeIds[i].IsLengthFlagSet())
            {
                metric.SetMetricValue32(aLinkInfo->mLqi);
            }
            else
            {
                metric.SetMetricValue8(aLinkInfo->mLqi);
            }
            break;

        case OT_LINK_RSSI:
            if (linkMetricTypeIds[i].IsLengthFlagSet())
            {
                metric.SetMetricValue32((uint16_t)(aLinkInfo->mRss + 130) * 255 /
                                        130); // Linear scale rss from 0 to 255
            }
            else
            {
                metric.SetMetricValue8((uint8_t)(aLinkInfo->mRss + 130 * 255 / 130)); // Linear scale rss from 0 to 255
            }
            break;

        case OT_LINK_MARGIN:
            if (linkMetricTypeIds[i].IsLengthFlagSet())
            {
                metric.SetMetricValue32(LinkQualityInfo::ConvertRssToLinkMargin(aNoiseFloor, aLinkInfo->mRss));
            }
            else
            {
                metric.SetMetricValue8(LinkQualityInfo::ConvertRssToLinkMargin(aNoiseFloor, aLinkInfo->mRss));
            }
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

otError LinkProbing::AppendForwardTrackingSeriesLinkMetricsReport(const uint8_t    aSeriesId,
                                                                  const int8_t     aNoiseFloor,
                                                                  LinkMetricsInfo &aLinkMetricsInfo,
                                                                  Message &        aMessage,
                                                                  uint8_t &        aLength)
{
    otError                 error = OT_ERROR_NONE;
    LinkMetricsReportSubTlv metrics[kLinkMetricsMaxNum];
    uint8_t                 status       = 0;
    uint8_t                 metricsCount = 0;

    status = aLinkMetricsInfo.GetForwardMetricsReport(aSeriesId, aNoiseFloor, metrics, metricsCount);
    if (status == LinkMetricsInfo::kLinkMetricsStatusSuccess)
    {
        for (uint8_t i = 0; i < metricsCount; i++)
        {
            SuccessOrExit(error = aMessage.Append(&metrics[i], sizeof(Tlv) + metrics[i].GetLength()));
            aLength += sizeof(Tlv) + metrics[i].GetLength();
        }
    }
    else
    {
        LinkMetricsStatusTlv statusTlv;
        statusTlv.Init();
        statusTlv.SetStatus(status);
        SuccessOrExit(error = aMessage.Append(&statusTlv, sizeof(Tlv) + statusTlv.GetLength()));
        aLength += sizeof(Tlv) + statusTlv.GetLength();
    }

exit:
    return error;
}

} // namespace LinkProbing
} // namespace ot
#endif // OPENTHREAD_CONFIG_LINK_PROBE_ENABLE
