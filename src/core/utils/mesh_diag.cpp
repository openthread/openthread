/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file implements the Mesh Diag module.
 */

#include "mesh_diag.hpp"

#if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "instance/instance.hpp"

namespace ot {
namespace Utils {

using namespace NetworkDiagnostic;

RegisterLogModule("MeshDiag");

//---------------------------------------------------------------------------------------------------------------------
// MeshDiag

MeshDiag::MeshDiag(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStateIdle)
    , mExpectedQueryId(0)
    , mExpectedAnswerIndex(0)
    , mTimer(aInstance)
{
}

Error MeshDiag::DiscoverTopology(const DiscoverConfig &aConfig, DiscoverCallback aCallback, void *aContext)
{
    static constexpr uint8_t kMaxTlvsToRequest = 6;

    Error   error = kErrorNone;
    uint8_t tlvs[kMaxTlvsToRequest];
    uint8_t tlvsLength = 0;

    VerifyOrExit(Get<Mle::Mle>().IsAttached(), error = kErrorInvalidState);
    VerifyOrExit(mState == kStateIdle, error = kErrorBusy);

    tlvs[tlvsLength++] = Address16Tlv::kType;
    tlvs[tlvsLength++] = ExtMacAddressTlv::kType;
    tlvs[tlvsLength++] = RouteTlv::kType;
    tlvs[tlvsLength++] = VersionTlv::kType;

    if (aConfig.mDiscoverIp6Addresses)
    {
        tlvs[tlvsLength++] = Ip6AddressListTlv::kType;
    }

    if (aConfig.mDiscoverChildTable)
    {
        tlvs[tlvsLength++] = ChildTableTlv::kType;
    }

    Get<RouterTable>().GetRouterIdSet(mDiscover.mExpectedRouterIdSet);

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        Ip6::Address destination;

        if (!mDiscover.mExpectedRouterIdSet.Contains(routerId))
        {
            continue;
        }

        destination = Get<Mle::MleRouter>().GetMeshLocal16();
        destination.GetIid().SetLocator(Mle::Rloc16FromRouterId(routerId));

        SuccessOrExit(error = Get<Client>().SendCommand(kUriDiagnosticGetRequest, Message::kPriorityLow, destination,
                                                        tlvs, tlvsLength, HandleDiagGetResponse, this));
    }

    mDiscover.mCallback.Set(aCallback, aContext);
    mState = kStateDicoverTopology;
    mTimer.Start(kResponseTimeout);

exit:
    return error;
}

void MeshDiag::HandleDiagGetResponse(void                *aContext,
                                     otMessage           *aMessage,
                                     const otMessageInfo *aMessageInfo,
                                     Error                aResult)
{
    static_cast<MeshDiag *>(aContext)->HandleDiagGetResponse(AsCoapMessagePtr(aMessage), AsCoreTypePtr(aMessageInfo),
                                                             aResult);
}

void MeshDiag::HandleDiagGetResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error           error;
    RouterInfo      routerInfo;
    Ip6AddrIterator ip6AddrIterator;
    ChildIterator   childIterator;

    SuccessOrExit(aResult);
    VerifyOrExit(aMessage != nullptr);
    VerifyOrExit(mState == kStateDicoverTopology);

    SuccessOrExit(routerInfo.ParseFrom(*aMessage));

    if (ip6AddrIterator.InitFrom(*aMessage) == kErrorNone)
    {
        routerInfo.mIp6AddrIterator = &ip6AddrIterator;
    }

    if (childIterator.InitFrom(*aMessage, routerInfo.mRloc16) == kErrorNone)
    {
        routerInfo.mChildIterator = &childIterator;
    }

    mDiscover.mExpectedRouterIdSet.Remove(routerInfo.mRouterId);

    if (mDiscover.mExpectedRouterIdSet.GetNumberOfAllocatedIds() == 0)
    {
        error  = kErrorNone;
        mState = kStateIdle;
        mTimer.Stop();
    }
    else
    {
        error = kErrorPending;
    }

    mDiscover.mCallback.InvokeIfSet(error, &routerInfo);

exit:
    return;
}

Error MeshDiag::SendQuery(uint16_t aRloc16, const uint8_t *aTlvs, uint8_t aTlvsLength)
{
    Error        error = kErrorNone;
    Ip6::Address destination;

    VerifyOrExit(Get<Mle::Mle>().IsAttached(), error = kErrorInvalidState);
    VerifyOrExit(mState == kStateIdle, error = kErrorBusy);
    VerifyOrExit(Mle::IsActiveRouter(aRloc16), error = kErrorInvalidArgs);
    VerifyOrExit(Get<RouterTable>().IsAllocated(Mle::RouterIdFromRloc16(aRloc16)), error = kErrorNotFound);

    destination = Get<Mle::MleRouter>().GetMeshLocal16();
    destination.GetIid().SetLocator(aRloc16);

    SuccessOrExit(error = Get<Client>().SendCommand(kUriDiagnosticGetQuery, Message::kPriorityNormal, destination,
                                                    aTlvs, aTlvsLength));

    mExpectedQueryId     = Get<Client>().GetLastQueryId();
    mExpectedAnswerIndex = 0;

    mTimer.Start(kResponseTimeout);

exit:
    return error;
}

Error MeshDiag::QueryChildTable(uint16_t aRloc16, QueryChildTableCallback aCallback, void *aContext)
{
    static const uint8_t kTlvTypes[] = {ChildTlv::kType};

    Error error;

    SuccessOrExit(error = SendQuery(aRloc16, kTlvTypes, sizeof(kTlvTypes)));

    mQueryChildTable.mCallback.Set(aCallback, aContext);
    mQueryChildTable.mRouterRloc16 = aRloc16;
    mState                         = kStateQueryChildTable;

exit:
    return error;
}

Error MeshDiag::QueryChildrenIp6Addrs(uint16_t aRloc16, ChildIp6AddrsCallback aCallback, void *aContext)
{
    static const uint8_t kTlvTypes[] = {ChildIp6AddressListTlv::kType};

    Error error;

    SuccessOrExit(error = SendQuery(aRloc16, kTlvTypes, sizeof(kTlvTypes)));

    mQueryChildrenIp6Addrs.mCallback.Set(aCallback, aContext);
    mQueryChildrenIp6Addrs.mParentRloc16 = aRloc16;
    mState                               = kStateQueryChildrenIp6Addrs;

exit:
    return error;
}

Error MeshDiag::QueryRouterNeighborTable(uint16_t aRloc16, RouterNeighborTableCallback aCallback, void *aContext)
{
    static const uint8_t kTlvTypes[] = {RouterNeighborTlv::kType};

    Error error;

    SuccessOrExit(error = SendQuery(aRloc16, kTlvTypes, sizeof(kTlvTypes)));

    mQueryRouterNeighborTable.mCallback.Set(aCallback, aContext);
    mQueryRouterNeighborTable.mRouterRloc16 = aRloc16;
    mState                                  = kStateQueryRouterNeighborTable;

exit:
    return error;
}

bool MeshDiag::HandleDiagnosticGetAnswer(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    bool didPorcess = false;

    switch (mState)
    {
    case kStateQueryChildTable:
        didPorcess = ProcessChildTableAnswer(aMessage, aMessageInfo);
        break;

    case kStateQueryChildrenIp6Addrs:
        didPorcess = ProcessChildrenIp6AddrsAnswer(aMessage, aMessageInfo);
        break;

    case kStateQueryRouterNeighborTable:
        didPorcess = ProcessRouterNeighborTableAnswer(aMessage, aMessageInfo);
        break;

    default:
        break;
    }

    return didPorcess;
}

Error MeshDiag::ProcessMessage(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, uint16_t aSenderRloc16)
{
    // This method processes the received answer message to
    // check whether it is from the intended sender and matches
    // the expected query ID and answer index.

    Error     error = kErrorFailed;
    AnswerTlv answerTlv;
    uint16_t  queryId;

    VerifyOrExit(Get<Mle::Mle>().IsRoutingLocator(aMessageInfo.GetPeerAddr()));
    VerifyOrExit(aMessageInfo.GetPeerAddr().GetIid().GetLocator() == aSenderRloc16);

    SuccessOrExit(Tlv::Find<QueryIdTlv>(aMessage, queryId));
    VerifyOrExit(queryId == mExpectedQueryId);

    SuccessOrExit(Tlv::FindTlv(aMessage, answerTlv));

    if (answerTlv.GetIndex() != mExpectedAnswerIndex)
    {
        Finalize(kErrorResponseTimeout);
        ExitNow();
    }

    mExpectedAnswerIndex++;
    error = kErrorNone;

exit:
    return error;
}

bool MeshDiag::ProcessChildTableAnswer(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    bool       didPorcess = false;
    ChildTlv   childTlv;
    ChildEntry entry;
    uint16_t   offset;

    SuccessOrExit(ProcessMessage(aMessage, aMessageInfo, mQueryChildTable.mRouterRloc16));

    while (true)
    {
        SuccessOrExit(Tlv::FindTlv(aMessage, childTlv, offset));
        VerifyOrExit(!childTlv.IsExtended());

        didPorcess = true;

        if (childTlv.GetLength() == 0)
        {
            // We reached end of the list.
            mState = kStateIdle;
            mTimer.Stop();
            mQueryChildTable.mCallback.InvokeIfSet(kErrorNone, nullptr);
            ExitNow();
        }

        VerifyOrExit(childTlv.GetLength() >= sizeof(ChildTlv) - sizeof(Tlv));
        IgnoreError(aMessage.Read(offset, childTlv));

        entry.SetFrom(childTlv);
        mQueryChildTable.mCallback.InvokeIfSet(kErrorPending, &entry);

        // Make sure query operation is not canceled from the
        // callback.
        VerifyOrExit(mState == kStateQueryChildTable);

        aMessage.SetOffset(static_cast<uint16_t>(offset + childTlv.GetSize()));
    }

exit:
    return didPorcess;
}

bool MeshDiag::ProcessRouterNeighborTableAnswer(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    bool                didPorcess = false;
    RouterNeighborTlv   neighborTlv;
    RouterNeighborEntry entry;
    uint16_t            offset;

    SuccessOrExit(ProcessMessage(aMessage, aMessageInfo, mQueryRouterNeighborTable.mRouterRloc16));

    while (true)
    {
        SuccessOrExit(Tlv::FindTlv(aMessage, neighborTlv, offset));
        VerifyOrExit(!neighborTlv.IsExtended());

        didPorcess = true;

        if (neighborTlv.GetLength() == 0)
        {
            // We reached end of the list.
            mState = kStateIdle;
            mTimer.Stop();
            mQueryRouterNeighborTable.mCallback.InvokeIfSet(kErrorNone, nullptr);
            ExitNow();
        }

        VerifyOrExit(neighborTlv.GetLength() >= sizeof(RouterNeighborTlv) - sizeof(Tlv));

        entry.SetFrom(neighborTlv);
        mQueryRouterNeighborTable.mCallback.InvokeIfSet(kErrorPending, &entry);

        // Make sure query operation is not canceled from the
        // callback.
        VerifyOrExit(mState == kStateQueryRouterNeighborTable);

        aMessage.SetOffset(static_cast<uint16_t>(offset + neighborTlv.GetSize()));
    }

exit:
    return didPorcess;
}

bool MeshDiag::ProcessChildrenIp6AddrsAnswer(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    bool                        didPorcess = false;
    uint16_t                    offset;
    uint16_t                    endOffset;
    ChildIp6AddressListTlvValue tlvValue;
    Ip6AddrIterator             ip6AddrIterator;

    SuccessOrExit(ProcessMessage(aMessage, aMessageInfo, mQueryChildrenIp6Addrs.mParentRloc16));

    while (true)
    {
        SuccessOrExit(Tlv::FindTlvValueStartEndOffsets(aMessage, ChildIp6AddressListTlv::kType, offset, endOffset));

        didPorcess = true;

        if (offset == endOffset)
        {
            // We reached end of the list
            mState = kStateIdle;
            mTimer.Stop();
            mQueryChildrenIp6Addrs.mCallback.InvokeIfSet(kErrorNone, Mle::kInvalidRloc16, nullptr);
            ExitNow();
        }

        // Read the `ChildIp6AddressListTlvValue` (which contains the
        // child RLOC16) and then prepare the `Ip6AddrIterator`.

        VerifyOrExit(offset + sizeof(tlvValue) <= endOffset);
        IgnoreError(aMessage.Read(offset, tlvValue));
        offset += sizeof(tlvValue);

        ip6AddrIterator.mMessage   = &aMessage;
        ip6AddrIterator.mCurOffset = offset;
        ip6AddrIterator.mEndOffset = endOffset;

        mQueryChildrenIp6Addrs.mCallback.InvokeIfSet(kErrorPending, tlvValue.GetRloc16(), &ip6AddrIterator);

        // Make sure query operation is not canceled from the
        // callback.
        VerifyOrExit(mState == kStateQueryChildrenIp6Addrs);

        aMessage.SetOffset(endOffset);
    }

exit:
    return didPorcess;
}

void MeshDiag::Cancel(void)
{
    switch (mState)
    {
    case kStateIdle:
    case kStateQueryChildTable:
    case kStateQueryChildrenIp6Addrs:
    case kStateQueryRouterNeighborTable:
        break;

    case kStateDicoverTopology:
        IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleDiagGetResponse, this));
        break;
    }

    mState = kStateIdle;
    mTimer.Stop();
}

void MeshDiag::Finalize(Error aError)
{
    // Finalize an ongoing query operation (if any) invoking
    // the corresponding callback with `aError`.

    State oldState = mState;

    Cancel();

    switch (oldState)
    {
    case kStateIdle:
        break;

    case kStateDicoverTopology:
        mDiscover.mCallback.InvokeIfSet(aError, nullptr);
        break;

    case kStateQueryChildTable:
        mQueryChildTable.mCallback.InvokeIfSet(aError, nullptr);
        break;

    case kStateQueryChildrenIp6Addrs:
        mQueryChildrenIp6Addrs.mCallback.InvokeIfSet(aError, Mle::kInvalidRloc16, nullptr);
        break;

    case kStateQueryRouterNeighborTable:
        mQueryRouterNeighborTable.mCallback.InvokeIfSet(aError, nullptr);
        break;
    }
}

void MeshDiag::HandleTimer(void) { Finalize(kErrorResponseTimeout); }

//---------------------------------------------------------------------------------------------------------------------
// MeshDiag::RouterInfo

Error MeshDiag::RouterInfo::ParseFrom(const Message &aMessage)
{
    Error     error = kErrorNone;
    Mle::Mle &mle   = aMessage.Get<Mle::Mle>();
    RouteTlv  routeTlv;

    Clear();

    SuccessOrExit(error = Tlv::Find<Address16Tlv>(aMessage, mRloc16));
    SuccessOrExit(error = Tlv::Find<ExtMacAddressTlv>(aMessage, AsCoreType(&mExtAddress)));
    SuccessOrExit(error = Tlv::FindTlv(aMessage, routeTlv));

    switch (error = Tlv::Find<VersionTlv>(aMessage, mVersion))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        mVersion = kVersionUnknown;
        error    = kErrorNone;
        break;
    default:
        ExitNow();
    }

    mRouterId           = Mle::RouterIdFromRloc16(mRloc16);
    mIsThisDevice       = (mRloc16 == mle.GetRloc16());
    mIsThisDeviceParent = mle.IsChild() && (mRloc16 == mle.GetParent().GetRloc16());
    mIsLeader           = (mRouterId == mle.GetLeaderId());
    mIsBorderRouter     = aMessage.Get<NetworkData::Leader>().ContainsBorderRouterWithRloc(mRloc16);

    for (uint8_t id = 0, index = 0; id <= Mle::kMaxRouterId; id++)
    {
        if (routeTlv.IsRouterIdSet(id))
        {
            mLinkQualities[id] = routeTlv.GetLinkQualityIn(index);
            index++;
        }
    }

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// MeshDiag::Ip6AddrIterator

Error MeshDiag::Ip6AddrIterator::InitFrom(const Message &aMessage)
{
    Error error;

    SuccessOrExit(error = Tlv::FindTlvValueStartEndOffsets(aMessage, Ip6AddressListTlv::kType, mCurOffset, mEndOffset));
    mMessage = &aMessage;

exit:
    return error;
}

Error MeshDiag::Ip6AddrIterator::GetNextAddress(Ip6::Address &aAddress)
{
    Error error = kErrorNone;

    VerifyOrExit(mMessage != nullptr, error = kErrorNotFound);
    VerifyOrExit(mCurOffset + sizeof(Ip6::Address) <= mEndOffset, error = kErrorNotFound);

    IgnoreError(mMessage->Read(mCurOffset, aAddress));
    mCurOffset += sizeof(Ip6::Address);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// MeshDiag::ChildIterator

Error MeshDiag::ChildIterator::InitFrom(const Message &aMessage, uint16_t aParentRloc16)
{
    Error error;

    SuccessOrExit(error = Tlv::FindTlvValueStartEndOffsets(aMessage, ChildTableTlv::kType, mCurOffset, mEndOffset));
    mMessage      = &aMessage;
    mParentRloc16 = aParentRloc16;

exit:
    return error;
}

Error MeshDiag::ChildIterator::GetNextChildInfo(ChildInfo &aChildInfo)
{
    Error           error = kErrorNone;
    ChildTableEntry entry;

    VerifyOrExit(mMessage != nullptr, error = kErrorNotFound);
    VerifyOrExit(mCurOffset + sizeof(ChildTableEntry) <= mEndOffset, error = kErrorNotFound);

    IgnoreError(mMessage->Read(mCurOffset, entry));
    mCurOffset += sizeof(ChildTableEntry);

    aChildInfo.mRloc16 = mParentRloc16 + entry.GetChildId();
    entry.GetMode().Get(aChildInfo.mMode);
    aChildInfo.mLinkQuality = entry.GetLinkQuality();

    aChildInfo.mIsThisDevice   = (aChildInfo.mRloc16 == mMessage->Get<Mle::Mle>().GetRloc16());
    aChildInfo.mIsBorderRouter = mMessage->Get<NetworkData::Leader>().ContainsBorderRouterWithRloc(aChildInfo.mRloc16);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// MeshDiag::ChildEntry

void MeshDiag::ChildEntry::SetFrom(const ChildTlv &aChildTlv)
{
    mRxOnWhenIdle        = (aChildTlv.GetFlags() & ChildTlv::kFlagsRxOnWhenIdle);
    mDeviceTypeFtd       = (aChildTlv.GetFlags() & ChildTlv::kFlagsFtd);
    mFullNetData         = (aChildTlv.GetFlags() & ChildTlv::kFlagsFullNetdta);
    mCslSynchronized     = (aChildTlv.GetFlags() & ChildTlv::kFlagsCslSync);
    mSupportsErrRate     = (aChildTlv.GetFlags() & ChildTlv::kFlagsTrackErrRate);
    mRloc16              = aChildTlv.GetRloc16();
    mExtAddress          = aChildTlv.GetExtAddress();
    mVersion             = aChildTlv.GetVersion();
    mTimeout             = aChildTlv.GetTimeout();
    mAge                 = aChildTlv.GetAge();
    mConnectionTime      = aChildTlv.GetConnectionTime();
    mSupervisionInterval = aChildTlv.GetSupervisionInterval();
    mLinkMargin          = aChildTlv.GetLinkMargin();
    mAverageRssi         = aChildTlv.GetAverageRssi();
    mLastRssi            = aChildTlv.GetLastRssi();
    mFrameErrorRate      = aChildTlv.GetFrameErrorRate();
    mMessageErrorRate    = aChildTlv.GetMessageErrorRate();
    mQueuedMessageCount  = aChildTlv.GetQueuedMessageCount();
    mCslPeriod           = aChildTlv.GetCslPeriod();
    mCslTimeout          = aChildTlv.GetCslTimeout();
    mCslChannel          = aChildTlv.GetCslChannel();
}

//---------------------------------------------------------------------------------------------------------------------
// MeshDiag::RouterNeighborEntry

void MeshDiag::RouterNeighborEntry::SetFrom(const RouterNeighborTlv &aTlv)
{
    mSupportsErrRate  = (aTlv.GetFlags() & RouterNeighborTlv::kFlagsTrackErrRate);
    mRloc16           = aTlv.GetRloc16();
    mExtAddress       = aTlv.GetExtAddress();
    mVersion          = aTlv.GetVersion();
    mConnectionTime   = aTlv.GetConnectionTime();
    mLinkMargin       = aTlv.GetLinkMargin();
    mAverageRssi      = aTlv.GetAverageRssi();
    mLastRssi         = aTlv.GetLastRssi();
    mFrameErrorRate   = aTlv.GetFrameErrorRate();
    mMessageErrorRate = aTlv.GetMessageErrorRate();
}

} // namespace Utils
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD
