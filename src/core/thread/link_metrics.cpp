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

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/num_utils.hpp"
#include "common/numeric_limits.hpp"
#include "instance/instance.hpp"
#include "mac/mac.hpp"
#include "thread/link_metrics_tlvs.hpp"
#include "thread/neighbor_table.hpp"

namespace ot {
namespace LinkMetrics {

RegisterLogModule("LinkMetrics");

static constexpr uint8_t kQueryIdSingleProbe = 0;   // This query ID represents Single Probe.
static constexpr uint8_t kSeriesIdAllSeries  = 255; // This series ID represents all series.

// Constants for scaling Link Margin and RSSI to raw value
static constexpr uint8_t kMaxLinkMargin = 130;
static constexpr int32_t kMinRssi       = -130;
static constexpr int32_t kMaxRssi       = 0;

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

Initiator::Initiator(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

Error Initiator::Query(const Ip6::Address &aDestination, uint8_t aSeriesId, const Metrics *aMetrics)
{
    Error     error;
    Neighbor *neighbor;
    QueryInfo info;

    SuccessOrExit(error = FindNeighbor(aDestination, neighbor));

    info.Clear();
    info.mSeriesId = aSeriesId;

    if (aMetrics != nullptr)
    {
        info.mTypeIdCount = aMetrics->ConvertToTypeIds(info.mTypeIds);
    }

    if (aSeriesId != 0)
    {
        VerifyOrExit(info.mTypeIdCount == 0, error = kErrorInvalidArgs);
    }

    error = Get<Mle::Mle>().SendDataRequestForLinkMetricsReport(aDestination, info);

exit:
    return error;
}

Error Initiator::AppendLinkMetricsQueryTlv(Message &aMessage, const QueryInfo &aInfo)
{
    Error error = kErrorNone;
    Tlv   tlv;

    // The MLE Link Metrics Query TLV has two sub-TLVs:
    // - Query ID sub-TLV with series ID as value.
    // - Query Options sub-TLV with Type IDs as value.

    tlv.SetType(Mle::Tlv::kLinkMetricsQuery);
    tlv.SetLength(sizeof(Tlv) + sizeof(uint8_t) + ((aInfo.mTypeIdCount == 0) ? 0 : (sizeof(Tlv) + aInfo.mTypeIdCount)));

    SuccessOrExit(error = aMessage.Append(tlv));

    SuccessOrExit(error = Tlv::Append<QueryIdSubTlv>(aMessage, aInfo.mSeriesId));

    if (aInfo.mTypeIdCount != 0)
    {
        QueryOptionsSubTlv queryOptionsTlv;

        queryOptionsTlv.Init();
        queryOptionsTlv.SetLength(aInfo.mTypeIdCount);
        SuccessOrExit(error = aMessage.Append(queryOptionsTlv));
        SuccessOrExit(error = aMessage.AppendBytes(aInfo.mTypeIds, aInfo.mTypeIdCount));
    }

exit:
    return error;
}

void Initiator::HandleReport(const Message &aMessage, uint16_t aOffset, uint16_t aLength, const Ip6::Address &aAddress)
{
    Error         error     = kErrorNone;
    uint16_t      offset    = aOffset;
    uint16_t      endOffset = aOffset + aLength;
    bool          hasStatus = false;
    bool          hasReport = false;
    Tlv           tlv;
    ReportSubTlv  reportTlv;
    MetricsValues values;
    uint8_t       status;
    uint8_t       typeId;

    OT_UNUSED_VARIABLE(error);

    VerifyOrExit(mReportCallback.IsSet());

    values.Clear();

    while (offset < endOffset)
    {
        SuccessOrExit(error = aMessage.Read(offset, tlv));

        VerifyOrExit(offset + sizeof(Tlv) + tlv.GetLength() <= endOffset, error = kErrorParse);

        // The report must contain either:
        // - One or more Report Sub-TLVs (in case of success), or
        // - A single Status Sub-TLV (in case of failure).

        switch (tlv.GetType())
        {
        case StatusSubTlv::kType:
            VerifyOrExit(!hasStatus && !hasReport, error = kErrorDrop);
            SuccessOrExit(error = Tlv::Read<StatusSubTlv>(aMessage, offset, status));
            hasStatus = true;
            break;

        case ReportSubTlv::kType:
            VerifyOrExit(!hasStatus, error = kErrorDrop);

            // Read the report sub-TLV assuming minimum length
            SuccessOrExit(error = aMessage.Read(offset, &reportTlv, sizeof(Tlv) + ReportSubTlv::kMinLength));
            VerifyOrExit(reportTlv.IsValid(), error = kErrorParse);
            hasReport = true;

            typeId = reportTlv.GetMetricsTypeId();

            if (TypeId::IsExtended(typeId))
            {
                // Skip the sub-TLV if `E` flag is set.
                break;
            }

            if (TypeId::GetValueLength(typeId) > sizeof(uint8_t))
            {
                // If Type ID indicates metric value has 4 bytes length, we
                // read the full `reportTlv`.
                SuccessOrExit(error = aMessage.Read(offset, reportTlv));
            }

            switch (typeId)
            {
            case TypeId::kPdu:
                values.mMetrics.mPduCount = true;
                values.mPduCountValue     = reportTlv.GetMetricsValue32();
                LogDebg(" - PDU Counter: %lu (Count/Summation)", ToUlong(values.mPduCountValue));
                break;

            case TypeId::kLqi:
                values.mMetrics.mLqi = true;
                values.mLqiValue     = reportTlv.GetMetricsValue8();
                LogDebg(" - LQI: %u (Exponential Moving Average)", values.mLqiValue);
                break;

            case TypeId::kLinkMargin:
                values.mMetrics.mLinkMargin = true;
                values.mLinkMarginValue     = ScaleRawValueToLinkMargin(reportTlv.GetMetricsValue8());
                LogDebg(" - Margin: %u (dB) (Exponential Moving Average)", values.mLinkMarginValue);
                break;

            case TypeId::kRssi:
                values.mMetrics.mRssi = true;
                values.mRssiValue     = ScaleRawValueToRssi(reportTlv.GetMetricsValue8());
                LogDebg(" - RSSI: %u (dBm) (Exponential Moving Average)", values.mRssiValue);
                break;
            }

            break;
        }

        offset += sizeof(Tlv) + tlv.GetLength();
    }

    VerifyOrExit(hasStatus || hasReport);

    mReportCallback.Invoke(&aAddress, hasStatus ? nullptr : &values,
                           hasStatus ? MapEnum(static_cast<Status>(status)) : MapEnum(kStatusSuccess));

exit:
    LogDebg("HandleReport, error:%s", ErrorToString(error));
}

Error Initiator::SendMgmtRequestForwardTrackingSeries(const Ip6::Address &aDestination,
                                                      uint8_t             aSeriesId,
                                                      const SeriesFlags  &aSeriesFlags,
                                                      const Metrics      *aMetrics)
{
    Error               error;
    Neighbor           *neighbor;
    uint8_t             typeIdCount = 0;
    FwdProbingRegSubTlv fwdProbingSubTlv;

    SuccessOrExit(error = FindNeighbor(aDestination, neighbor));

    VerifyOrExit(aSeriesId > kQueryIdSingleProbe, error = kErrorInvalidArgs);

    fwdProbingSubTlv.Init();
    fwdProbingSubTlv.SetSeriesId(aSeriesId);
    fwdProbingSubTlv.SetSeriesFlagsMask(aSeriesFlags.ConvertToMask());

    if (aMetrics != nullptr)
    {
        typeIdCount = aMetrics->ConvertToTypeIds(fwdProbingSubTlv.GetTypeIds());
    }

    fwdProbingSubTlv.SetLength(sizeof(aSeriesId) + sizeof(uint8_t) + typeIdCount);

    error = Get<Mle::Mle>().SendLinkMetricsManagementRequest(aDestination, fwdProbingSubTlv);

exit:
    LogDebg("SendMgmtRequestForwardTrackingSeries, error:%s, Series ID:%u", ErrorToString(error), aSeriesId);
    return error;
}

Error Initiator::SendMgmtRequestEnhAckProbing(const Ip6::Address &aDestination,
                                              EnhAckFlags         aEnhAckFlags,
                                              const Metrics      *aMetrics)
{
    Error              error;
    Neighbor          *neighbor;
    uint8_t            typeIdCount = 0;
    EnhAckConfigSubTlv enhAckConfigSubTlv;

    SuccessOrExit(error = FindNeighbor(aDestination, neighbor));

    if (aEnhAckFlags == kEnhAckClear)
    {
        VerifyOrExit(aMetrics == nullptr, error = kErrorInvalidArgs);
    }

    enhAckConfigSubTlv.Init();
    enhAckConfigSubTlv.SetEnhAckFlags(aEnhAckFlags);

    if (aMetrics != nullptr)
    {
        typeIdCount = aMetrics->ConvertToTypeIds(enhAckConfigSubTlv.GetTypeIds());
    }

    enhAckConfigSubTlv.SetLength(EnhAckConfigSubTlv::kMinLength + typeIdCount);

    error = Get<Mle::Mle>().SendLinkMetricsManagementRequest(aDestination, enhAckConfigSubTlv);

    if (aMetrics != nullptr)
    {
        neighbor->SetEnhAckProbingMetrics(*aMetrics);
    }
    else
    {
        Metrics metrics;

        metrics.Clear();
        neighbor->SetEnhAckProbingMetrics(metrics);
    }

exit:
    return error;
}

Error Initiator::HandleManagementResponse(const Message &aMessage, const Ip6::Address &aAddress)
{
    Error    error = kErrorNone;
    uint16_t offset;
    uint16_t endOffset;
    uint8_t  status;
    bool     hasStatus = false;

    VerifyOrExit(mMgmtResponseCallback.IsSet());

    SuccessOrExit(
        error = Tlv::FindTlvValueStartEndOffsets(aMessage, Mle::Tlv::Type::kLinkMetricsManagement, offset, endOffset));

    while (offset < endOffset)
    {
        Tlv tlv;

        SuccessOrExit(error = aMessage.Read(offset, tlv));

        switch (tlv.GetType())
        {
        case StatusSubTlv::kType:
            VerifyOrExit(!hasStatus, error = kErrorParse);
            SuccessOrExit(error = Tlv::Read<StatusSubTlv>(aMessage, offset, status));
            hasStatus = true;
            break;

        default:
            break;
        }

        offset += sizeof(Tlv) + tlv.GetLength();
    }

    VerifyOrExit(hasStatus, error = kErrorParse);

    mMgmtResponseCallback.Invoke(&aAddress, MapEnum(static_cast<Status>(status)));

exit:
    return error;
}

Error Initiator::SendLinkProbe(const Ip6::Address &aDestination, uint8_t aSeriesId, uint8_t aLength)
{
    Error     error;
    uint8_t   buf[kLinkProbeMaxLen];
    Neighbor *neighbor;

    SuccessOrExit(error = FindNeighbor(aDestination, neighbor));

    VerifyOrExit(aLength <= kLinkProbeMaxLen && aSeriesId != kQueryIdSingleProbe && aSeriesId != kSeriesIdAllSeries,
                 error = kErrorInvalidArgs);

    error = Get<Mle::Mle>().SendLinkProbe(aDestination, aSeriesId, buf, aLength);
exit:
    LogDebg("SendLinkProbe, error:%s, Series ID:%u", ErrorToString(error), aSeriesId);
    return error;
}

void Initiator::ProcessEnhAckIeData(const uint8_t *aData, uint8_t aLength, const Neighbor &aNeighbor)
{
    MetricsValues values;
    uint8_t       idx = 0;

    VerifyOrExit(mEnhAckProbingIeReportCallback.IsSet());

    values.SetMetrics(aNeighbor.GetEnhAckProbingMetrics());

    if (values.GetMetrics().mLqi && idx < aLength)
    {
        values.mLqiValue = aData[idx++];
    }
    if (values.GetMetrics().mLinkMargin && idx < aLength)
    {
        values.mLinkMarginValue = ScaleRawValueToLinkMargin(aData[idx++]);
    }
    if (values.GetMetrics().mRssi && idx < aLength)
    {
        values.mRssiValue = ScaleRawValueToRssi(aData[idx++]);
    }

    mEnhAckProbingIeReportCallback.Invoke(aNeighbor.GetRloc16(), &aNeighbor.GetExtAddress(), &values);

exit:
    return;
}

Error Initiator::FindNeighbor(const Ip6::Address &aDestination, Neighbor *&aNeighbor)
{
    Error        error = kErrorUnknownNeighbor;
    Mac::Address macAddress;

    aNeighbor = nullptr;

    VerifyOrExit(aDestination.IsLinkLocal());
    aDestination.GetIid().ConvertToMacAddress(macAddress);

    aNeighbor = Get<NeighborTable>().FindNeighbor(macAddress);
    VerifyOrExit(aNeighbor != nullptr);

    VerifyOrExit(aNeighbor->GetVersion() >= kThreadVersion1p2, error = kErrorNotCapable);
    error = kErrorNone;

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
Subject::Subject(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

Error Subject::AppendReport(Message &aMessage, const Message &aRequestMessage, Neighbor &aNeighbor)
{
    Error         error = kErrorNone;
    Tlv           tlv;
    uint8_t       queryId;
    bool          hasQueryId = false;
    uint16_t      length;
    uint16_t      offset;
    uint16_t      endOffset;
    MetricsValues values;

    values.Clear();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Parse MLE Link Metrics Query TLV and its sub-TLVs from
    // `aRequestMessage`.

    SuccessOrExit(error = Tlv::FindTlvValueStartEndOffsets(aRequestMessage, Mle::Tlv::Type::kLinkMetricsQuery, offset,
                                                           endOffset));

    while (offset < endOffset)
    {
        SuccessOrExit(error = aRequestMessage.Read(offset, tlv));

        switch (tlv.GetType())
        {
        case SubTlv::kQueryId:
            SuccessOrExit(error = Tlv::Read<QueryIdSubTlv>(aRequestMessage, offset, queryId));
            hasQueryId = true;
            break;

        case SubTlv::kQueryOptions:
            SuccessOrExit(error = ReadTypeIdsFromMessage(aRequestMessage, offset + sizeof(tlv),
                                                         static_cast<uint16_t>(offset + tlv.GetSize()),
                                                         values.GetMetrics()));
            break;

        default:
            break;
        }

        offset += static_cast<uint16_t>(tlv.GetSize());
    }

    VerifyOrExit(hasQueryId, error = kErrorParse);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Append MLE Link Metrics Report TLV and its sub-TLVs to
    // `aMessage`.

    offset = aMessage.GetLength();
    tlv.SetType(Mle::Tlv::kLinkMetricsReport);
    SuccessOrExit(error = aMessage.Append(tlv));

    if (queryId == kQueryIdSingleProbe)
    {
        values.mPduCountValue   = aRequestMessage.GetPsduCount();
        values.mLqiValue        = aRequestMessage.GetAverageLqi();
        values.mLinkMarginValue = Get<Mac::Mac>().ComputeLinkMargin(aRequestMessage.GetAverageRss());
        values.mRssiValue       = aRequestMessage.GetAverageRss();
        SuccessOrExit(error = AppendReportSubTlvToMessage(aMessage, values));
    }
    else
    {
        SeriesInfo *seriesInfo = aNeighbor.GetForwardTrackingSeriesInfo(queryId);

        if (seriesInfo == nullptr)
        {
            SuccessOrExit(error = Tlv::Append<StatusSubTlv>(aMessage, kStatusSeriesIdNotRecognized));
        }
        else if (seriesInfo->GetPduCount() == 0)
        {
            SuccessOrExit(error = Tlv::Append<StatusSubTlv>(aMessage, kStatusNoMatchingFramesReceived));
        }
        else
        {
            values.SetMetrics(seriesInfo->GetLinkMetrics());
            values.mPduCountValue   = seriesInfo->GetPduCount();
            values.mLqiValue        = seriesInfo->GetAverageLqi();
            values.mLinkMarginValue = Get<Mac::Mac>().ComputeLinkMargin(seriesInfo->GetAverageRss());
            values.mRssiValue       = seriesInfo->GetAverageRss();
            SuccessOrExit(error = AppendReportSubTlvToMessage(aMessage, values));
        }
    }

    // Update the TLV length in message.
    length = aMessage.GetLength() - offset - sizeof(Tlv);
    tlv.SetLength(static_cast<uint8_t>(length));
    aMessage.Write(offset, tlv);

exit:
    LogDebg("AppendReport, error:%s", ErrorToString(error));
    return error;
}

Error Subject::HandleManagementRequest(const Message &aMessage, Neighbor &aNeighbor, Status &aStatus)
{
    Error               error = kErrorNone;
    uint16_t            offset;
    uint16_t            endOffset;
    uint16_t            tlvEndOffset;
    FwdProbingRegSubTlv fwdProbingSubTlv;
    EnhAckConfigSubTlv  enhAckConfigSubTlv;
    Metrics             metrics;

    SuccessOrExit(
        error = Tlv::FindTlvValueStartEndOffsets(aMessage, Mle::Tlv::Type::kLinkMetricsManagement, offset, endOffset));

    // Set sub-TLV lengths to zero to indicate that we have
    // not yet seen them in the message.
    fwdProbingSubTlv.SetLength(0);
    enhAckConfigSubTlv.SetLength(0);

    for (; offset < endOffset; offset = tlvEndOffset)
    {
        Tlv      tlv;
        uint16_t minTlvSize;
        Tlv     *subTlv;

        SuccessOrExit(error = aMessage.Read(offset, tlv));

        VerifyOrExit(offset + tlv.GetSize() <= endOffset, error = kErrorParse);
        tlvEndOffset = static_cast<uint16_t>(offset + tlv.GetSize());

        switch (tlv.GetType())
        {
        case SubTlv::kFwdProbingReg:
            subTlv     = &fwdProbingSubTlv;
            minTlvSize = sizeof(Tlv) + FwdProbingRegSubTlv::kMinLength;
            break;

        case SubTlv::kEnhAckConfig:
            subTlv     = &enhAckConfigSubTlv;
            minTlvSize = sizeof(Tlv) + EnhAckConfigSubTlv::kMinLength;
            break;

        default:
            continue;
        }

        // Ensure message contains only one sub-TLV.
        VerifyOrExit(fwdProbingSubTlv.GetLength() == 0, error = kErrorParse);
        VerifyOrExit(enhAckConfigSubTlv.GetLength() == 0, error = kErrorParse);

        VerifyOrExit(tlv.GetSize() >= minTlvSize, error = kErrorParse);

        // Read `subTlv` with its `minTlvSize`, followed by the Type IDs.
        SuccessOrExit(error = aMessage.Read(offset, subTlv, minTlvSize));
        SuccessOrExit(error = ReadTypeIdsFromMessage(aMessage, offset + minTlvSize, tlvEndOffset, metrics));
    }

    if (fwdProbingSubTlv.GetLength() != 0)
    {
        aStatus = ConfigureForwardTrackingSeries(fwdProbingSubTlv.GetSeriesId(), fwdProbingSubTlv.GetSeriesFlagsMask(),
                                                 metrics, aNeighbor);
    }

    if (enhAckConfigSubTlv.GetLength() != 0)
    {
        aStatus = ConfigureEnhAckProbing(enhAckConfigSubTlv.GetEnhAckFlags(), metrics, aNeighbor);
    }

exit:
    return error;
}

Error Subject::HandleLinkProbe(const Message &aMessage, uint8_t &aSeriesId)
{
    Error    error = kErrorNone;
    uint16_t offset;
    uint16_t length;

    SuccessOrExit(error = Tlv::FindTlvValueOffset(aMessage, Mle::Tlv::Type::kLinkProbe, offset, length));
    VerifyOrExit(length >= sizeof(aSeriesId), error = kErrorParse);
    error = aMessage.Read(offset, aSeriesId);

exit:
    return error;
}

Error Subject::AppendReportSubTlvToMessage(Message &aMessage, const MetricsValues &aValues)
{
    Error        error = kErrorNone;
    ReportSubTlv reportTlv;

    reportTlv.Init();

    if (aValues.mMetrics.mPduCount)
    {
        reportTlv.SetMetricsTypeId(TypeId::kPdu);
        reportTlv.SetMetricsValue32(aValues.mPduCountValue);
        SuccessOrExit(error = reportTlv.AppendTo(aMessage));
    }

    if (aValues.mMetrics.mLqi)
    {
        reportTlv.SetMetricsTypeId(TypeId::kLqi);
        reportTlv.SetMetricsValue8(aValues.mLqiValue);
        SuccessOrExit(error = reportTlv.AppendTo(aMessage));
    }

    if (aValues.mMetrics.mLinkMargin)
    {
        reportTlv.SetMetricsTypeId(TypeId::kLinkMargin);
        reportTlv.SetMetricsValue8(ScaleLinkMarginToRawValue(aValues.mLinkMarginValue));
        SuccessOrExit(error = reportTlv.AppendTo(aMessage));
    }

    if (aValues.mMetrics.mRssi)
    {
        reportTlv.SetMetricsTypeId(TypeId::kRssi);
        reportTlv.SetMetricsValue8(ScaleRssiToRawValue(aValues.mRssiValue));
        SuccessOrExit(error = reportTlv.AppendTo(aMessage));
    }

exit:
    return error;
}

void Subject::Free(SeriesInfo &aSeriesInfo) { mSeriesInfoPool.Free(aSeriesInfo); }

Error Subject::ReadTypeIdsFromMessage(const Message &aMessage,
                                      uint16_t       aStartOffset,
                                      uint16_t       aEndOffset,
                                      Metrics       &aMetrics)
{
    Error error = kErrorNone;

    aMetrics.Clear();

    for (uint16_t offset = aStartOffset; offset < aEndOffset; offset++)
    {
        uint8_t typeId;

        SuccessOrExit(aMessage.Read(offset, typeId));

        switch (typeId)
        {
        case TypeId::kPdu:
            VerifyOrExit(!aMetrics.mPduCount, error = kErrorParse);
            aMetrics.mPduCount = true;
            break;

        case TypeId::kLqi:
            VerifyOrExit(!aMetrics.mLqi, error = kErrorParse);
            aMetrics.mLqi = true;
            break;

        case TypeId::kLinkMargin:
            VerifyOrExit(!aMetrics.mLinkMargin, error = kErrorParse);
            aMetrics.mLinkMargin = true;
            break;

        case TypeId::kRssi:
            VerifyOrExit(!aMetrics.mRssi, error = kErrorParse);
            aMetrics.mRssi = true;
            break;

        default:
            if (TypeId::IsExtended(typeId))
            {
                offset += sizeof(uint8_t); // Skip the additional second byte.
            }
            else
            {
                aMetrics.mReserved = true;
            }
            break;
        }
    }

exit:
    return error;
}

Status Subject::ConfigureForwardTrackingSeries(uint8_t        aSeriesId,
                                               uint8_t        aSeriesFlagsMask,
                                               const Metrics &aMetrics,
                                               Neighbor      &aNeighbor)
{
    Status status = kStatusSuccess;

    VerifyOrExit(0 < aSeriesId, status = kStatusOtherError);

    if (aSeriesFlagsMask == 0) // Remove the series
    {
        if (aSeriesId == kSeriesIdAllSeries) // Remove all
        {
            aNeighbor.RemoveAllForwardTrackingSeriesInfo();
        }
        else
        {
            SeriesInfo *seriesInfo = aNeighbor.RemoveForwardTrackingSeriesInfo(aSeriesId);
            VerifyOrExit(seriesInfo != nullptr, status = kStatusSeriesIdNotRecognized);
            mSeriesInfoPool.Free(*seriesInfo);
        }
    }
    else // Add a new series
    {
        SeriesInfo *seriesInfo = aNeighbor.GetForwardTrackingSeriesInfo(aSeriesId);
        VerifyOrExit(seriesInfo == nullptr, status = kStatusSeriesIdAlreadyRegistered);
        seriesInfo = mSeriesInfoPool.Allocate();
        VerifyOrExit(seriesInfo != nullptr, status = kStatusCannotSupportNewSeries);

        seriesInfo->Init(aSeriesId, aSeriesFlagsMask, aMetrics);

        aNeighbor.AddForwardTrackingSeriesInfo(*seriesInfo);
    }

exit:
    return status;
}

Status Subject::ConfigureEnhAckProbing(uint8_t aEnhAckFlags, const Metrics &aMetrics, Neighbor &aNeighbor)
{
    Status status = kStatusSuccess;
    Error  error  = kErrorNone;

    VerifyOrExit(!aMetrics.mReserved, status = kStatusOtherError);

    if (aEnhAckFlags == kEnhAckRegister)
    {
        VerifyOrExit(!aMetrics.mPduCount, status = kStatusOtherError);
        VerifyOrExit(aMetrics.mLqi || aMetrics.mLinkMargin || aMetrics.mRssi, status = kStatusOtherError);
        VerifyOrExit(!(aMetrics.mLqi && aMetrics.mLinkMargin && aMetrics.mRssi), status = kStatusOtherError);

        error = Get<Radio>().ConfigureEnhAckProbing(aMetrics, aNeighbor.GetRloc16(), aNeighbor.GetExtAddress());
    }
    else if (aEnhAckFlags == kEnhAckClear)
    {
        VerifyOrExit(!aMetrics.mLqi && !aMetrics.mLinkMargin && !aMetrics.mRssi, status = kStatusOtherError);
        error = Get<Radio>().ConfigureEnhAckProbing(aMetrics, aNeighbor.GetRloc16(), aNeighbor.GetExtAddress());
    }
    else
    {
        status = kStatusOtherError;
    }

    VerifyOrExit(error == kErrorNone, status = kStatusOtherError);

exit:
    return status;
}

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

uint8_t ScaleLinkMarginToRawValue(uint8_t aLinkMargin)
{
    // Linearly scale Link Margin from [0, 130] to [0, 255].
    // `kMaxLinkMargin = 130`.

    uint16_t value;

    value = Min(aLinkMargin, kMaxLinkMargin);
    value = value * NumericLimits<uint8_t>::kMax;
    value = DivideAndRoundToClosest<uint16_t>(value, kMaxLinkMargin);

    return static_cast<uint8_t>(value);
}

uint8_t ScaleRawValueToLinkMargin(uint8_t aRawValue)
{
    // Scale back raw value of [0, 255] to Link Margin from [0, 130].

    uint16_t value = aRawValue;

    value = value * kMaxLinkMargin;
    value = DivideAndRoundToClosest<uint16_t>(value, NumericLimits<uint8_t>::kMax);
    return static_cast<uint8_t>(value);
}

uint8_t ScaleRssiToRawValue(int8_t aRssi)
{
    // Linearly scale RSSI from [-130, 0] to [0, 255].
    // `kMinRssi = -130`, `kMaxRssi = 0`.

    int32_t value = aRssi;

    value = Clamp(value, kMinRssi, kMaxRssi) - kMinRssi;
    value = value * NumericLimits<uint8_t>::kMax;
    value = DivideAndRoundToClosest<int32_t>(value, kMaxRssi - kMinRssi);

    return static_cast<uint8_t>(value);
}

int8_t ScaleRawValueToRssi(uint8_t aRawValue)
{
    int32_t value = aRawValue;

    value = value * (kMaxRssi - kMinRssi);
    value = DivideAndRoundToClosest<int32_t>(value, NumericLimits<uint8_t>::kMax);
    value += kMinRssi;

    return ClampToInt8(value);
}

} // namespace LinkMetrics
} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
