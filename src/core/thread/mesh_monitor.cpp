/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include "mesh_monitor.hpp"

#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("MeshMon");

namespace MeshMonitor {

#if OPENTHREAD_CONFIG_MESH_MONITOR_SERVER_ENABLE

const TlvSet Server::kExtDelayTlvMask = [] {
    TlvSet set;
    set.Set(Tlv::kLastHeard);
    set.Set(Tlv::kConnectionTime);
    set.Set(Tlv::kLinkMarginIn);
    set.Set(Tlv::kMacLinkErrorRatesTx);
    set.Set(Tlv::kMacCounters);
    set.Set(Tlv::kMacLinkErrorRatesRx);
    set.Set(Tlv::kMleCounters);
    set.Set(Tlv::kLinkMarginOut);
    return set;
}();

const TlvSet Server::kStaticNeighborTlvMask = [] {
    TlvSet set;
    set.Set(Tlv::kExtAddress);
    set.Set(Tlv::kConnectionTime);
    set.Set(Tlv::kThreadSpecVersion);
    return set;
}();

const TlvSet Server::kChildAttachSkipTlvMask = [] {
    TlvSet set;
    set.Set(Tlv::kThreadStackVersion);
    set.Set(Tlv::kVendorName);
    set.Set(Tlv::kVendorModel);
    set.Set(Tlv::kVendorSwVersion);
    set.Set(Tlv::kVendorAppUrl);
    set.Set(Tlv::kEui64);
    set.Set(Tlv::kMacCounters);
    set.Set(Tlv::kMleCounters);
    return set;
}();

#if OPENTHREAD_FTD
void Server::ChildInfo::MarkHostProvidedTlvsChanged(TlvSet aTlvs)
{
    if (mIsFtd)
    {
        mDirtySet.SetAll(aTlvs.GetNonFtdChildProvided());
    }
    else
    {
        mDirtySet.SetAll(aTlvs.GetNonMtdChildProvided());
    }
}

void Server::ChildInfo::CommitCacheUpdate(void)
{
    OT_ASSERT(mDiagCacheLocked);
    mDiagCacheLocked = false;

    mAttachStateDirty = false;

    mDirtySet.Clear();
    mCache.Free();
    mCacheBuffers = 0;
}

void Server::ChildInfo::AbortCacheUpdate(void)
{
    OT_ASSERT(mDiagCacheLocked);
    mDiagCacheLocked = false;

    if (mCache == nullptr)
    {
        EvictCache();
    }
}

TlvSet Server::ChildInfo::GetDirtyHostProvided(TlvSet aFilter) const
{
    TlvSet set = mDirtySet.Intersect(aFilter);

    if (mIsFtd)
    {
        set = set.GetNonFtdChildProvided();
    }
    else
    {
        set = set.GetNonMtdChildProvided();
    }

    return set;
}

void Server::ChildInfo::EvictCache(void)
{
    TlvSet lost;

    OT_ASSERT(!mDiagCacheLocked);

    if (mIsFtd)
    {
        lost = mDirtySet.GetFtdChildProvided();
    }
    else
    {
        lost = mDirtySet.GetMtdChildProvided();
    }

    mDirtySet.ClearAll(lost);
    mLostSet.SetAll(lost);

    mCache.Free();
    mCacheBuffers = 0;
}

void Server::ChildInfo::ClearCache(void)
{
    OT_ASSERT(!mDiagCacheLocked);

    mDirtySet.Clear();
    mLostSet.Clear();

    mCache.Free();
    mCacheBuffers = 0;
}

Error Server::ChildInfo::RemoveCachedTlv(uint8_t aType)
{
    Error    error  = kErrorNone;
    uint16_t offset = 0;

    VerifyOrExit(mCache != nullptr);

    while (offset < mCache->GetLength())
    {
        uint32_t tlvSize;
        union
        {
            Tlv         tlv;
            ExtendedTlv extTlv;
        };

        SuccessOrExit(error = mCache->Read(offset, tlv));

        if (tlv.IsExtended())
        {
            SuccessOrExit(error = mCache->Read(offset, extTlv));
            tlvSize = sizeof(ExtendedTlv) + extTlv.GetLength();
        }
        else
        {
            tlvSize = sizeof(Tlv) + tlv.GetLength();
        }

        VerifyOrExit(static_cast<uint32_t>(offset) + tlvSize <= mCache->GetLength(), error = kErrorParse);

        if (tlv.GetType() == aType)
        {
            mCache->RemoveHeader(offset, static_cast<uint16_t>(tlvSize));
            break;
        }

        offset += static_cast<uint16_t>(tlvSize);
    }

exit:
    return error;
}

Error Server::ChildInfo::UpdateCache(const Message &aMessage, TlvSet aFilter)
{
    Error    error     = kErrorNone;
    uint16_t srcOffset = aMessage.GetOffset();

    TlvSet      set;
    OffsetRange srcRange;
    union
    {
        Tlv         tlv;
        ExtendedTlv extTlv;
    };

    // Prevent freeing the cache we are currently building
    // If we run out of memory, the error handler of this function will free it
    OT_ASSERT(!mDiagCacheLocked);
    mDiagCacheLocked = true;

    while (srcOffset < aMessage.GetLength())
    {
        uint32_t tlvSize;

        SuccessOrExit(error = aMessage.Read(srcOffset, tlv));

        if (tlv.IsExtended())
        {
            SuccessOrExit(error = aMessage.Read(srcOffset, extTlv));
            tlvSize = sizeof(ExtendedTlv) + extTlv.GetLength();
        }
        else
        {
            tlvSize = sizeof(Tlv) + tlv.GetLength();
        }

        VerifyOrExit(static_cast<uint32_t>(srcOffset) + tlvSize <= aMessage.GetLength(), error = kErrorParse);

        srcRange.Init(srcOffset, static_cast<uint16_t>(tlvSize));
        srcOffset = srcRange.GetEndOffset();

        set.Clear();
        set.SetValue(tlv.GetType());
        set = set.Intersect(aFilter);

        if (mIsFtd)
        {
            set = set.GetFtdChildProvided();
        }
        else
        {
            set = set.GetMtdChildProvided();
        }

        if (set.IsEmpty())
        {
            continue;
        }

        if (mCache == nullptr)
        {
            mCache.Reset(aMessage.Get<MessagePool>().Allocate(Message::kTypeOther));
            VerifyOrExit(mCache != nullptr, error = kErrorNoBufs);
        }

        // We already made sure the TLV is child provided. To keep exactly one
        // copy of each TLV in the cache, remove any previously cached copy of
        // this type before appending the newest value.
        if (mDirtySet.ContainsAll(set))
        {
            SuccessOrExit(error = RemoveCachedTlv(tlv.GetType()));
        }

        SuccessOrExit(error = mCache->AppendBytesFromMessage(aMessage, srcRange.GetOffset(), srcRange.GetLength()));

        mDirtySet.SetAll(set);
    }

    mLostSet.ClearAll(mDirtySet);

exit:
    mDiagCacheLocked = false;
    if (error != kErrorNone)
    {
        // An error here could render the cache invalid, so we just clear it and query
        // the tlvs again using the lost set
        LogCrit("Diag cache error %s", ErrorToString(error));
        EvictCache();
    }

    if (mCache != nullptr)
    {
        mCacheBuffers = mCache->GetBufferCount();
    }

    return error;
}

Error Server::ChildInfo::AppendCachedTlvs(Message &aMessage)
{
    Error error = kErrorNone;

    VerifyOrExit(mCache != nullptr);
    OT_ASSERT(mDiagCacheLocked);

    SuccessOrExit(error = aMessage.AppendBytesFromMessage(*mCache, 0, mCache->GetLength()));

exit:
    return error;
}
#endif // OPENTHREAD_FTD

Server::Server(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mActive(false)
    , mUpdateSent(false)
#if OPENTHREAD_FTD
    , mSendFullUpdate(false)
    , mSendChildBaseline(false)
    , mSendNeighborBaseline(false)
    , mUpdatePending(false)
    , mChildResumeIndex(0)
    , mNeighborResumeIndex(0)
    , mClientRloc(Mle::kInvalidRloc16)
    , mUpdateRetryCount(0)
    , mCacheSyncEvictions(0)
    , mCachePollEvictions(0)
    , mCacheErrors(0)
    , mRouterStateMask(0)
#endif // OPENTHREAD_FTD
    , mUpdateTimer(aInstance)
#if OPENTHREAD_FTD
    , mChildTimer(aInstance)
    , mRegistrationTimer(aInstance)
#endif // OPENTHREAD_FTD
{
    mSelfEnabled.Clear();
    mSelfPendingUpdate.Clear();
    mSelfDirty.Clear();

#if OPENTHREAD_FTD
    mChildEnabled.Clear();

    mNeighborEnabled.Clear();

    mClientRegistered = false;
#endif // OPENTHREAD_FTD
}

void Server::MarkDiagDirty(Tlv::Type aTlv)
{
    TlvSet set;
    set.Clear();
    set.Set(aTlv);
    MarkDiagDirty(set);
}

void Server::MarkDiagDirty(TlvSet aTlvs)
{
    aTlvs.Filter(mSelfEnabled);

    // If the server is inactive, the enabled set will always be 0 so this
    // check will always fail.
    if (!aTlvs.IsEmpty())
    {
        mSelfDirty.SetAll(aTlvs);
        if (IsOnlyExtDelayTlvs(aTlvs))
        {
            ScheduleUpdateTimer(kUpdateExtDelay);
        }
        else
        {
            ScheduleUpdateTimer(kUpdateBaseDelay);
        }
    }
}

#if OPENTHREAD_FTD
void Server::MarkChildDiagDirty(Child &aChild, Tlv::Type aTlv)
{
    TlvSet set;
    set.Clear();
    set.Set(aTlv);
    MarkChildDiagDirty(aChild, set);
}

void Server::MarkChildDiagDirty(Child &aChild, TlvSet aTlvs)
{
    VerifyOrExit(aChild.IsStateValid());

    aTlvs.Filter(mChildEnabled);
    // Use mIsFtd to allow the compiler to optimize away the check in MarkHostProvidedTlvsChanged
    if (aChild.mIsFtd)
    {
        aTlvs = aTlvs.GetNonFtdChildProvided();
    }
    else
    {
        aTlvs = aTlvs.GetNonMtdChildProvided();
    }

    // If the server is inactive, the enabled set will always be 0 so this
    // check will always fail.
    if (!aTlvs.IsEmpty())
    {
        aChild.MarkHostProvidedTlvsChanged(aTlvs);
        if (IsOnlyExtDelayTlvs(aTlvs))
        {
            ScheduleUpdateTimer(kUpdateExtDelay);
        }
        else
        {
            ScheduleUpdateTimer(kUpdateBaseDelay);
        }
    }

exit:
    return;
}

void Server::HandleChildAdded(Child &aChild)
{
    aChild.ClearCache();
    aChild.SetChildIsFtd(aChild.IsFullThreadDevice());
    aChild.SetDiagServerState(Child::kDiagServerStopped);

    VerifyOrExit(mActive);

    aChild.SetAttachStateDirty();

    if (!mChildEnabled.IsEmpty())
    {
        TlvSet set;

        if (aChild.IsFullThreadDevice())
        {
            set = mChildEnabled.GetFtdChildProvided();
        }
        else
        {
            set = mChildEnabled.GetMtdChildProvided();
        }

        set.ClearAll(static_cast<const TlvSet &>(kChildAttachSkipTlvMask));

        if (!set.IsEmpty())
        {
            IgnoreError(SendEndDeviceRequestStart(aChild, set, true));
        }

        ScheduleUpdateTimer(kUpdateBaseDelay);
    }

exit:
    return;
}

void Server::HandleChildRemoved(Child &aChild)
{
    aChild.ClearCache();
    aChild.SetDiagServerState(Child::kDiagServerStopped);

    VerifyOrExit(mActive);

    aChild.SetAttachStateDirty();

    if (!mChildEnabled.IsEmpty())
    {
        ScheduleUpdateTimer(kUpdateBaseDelay);
    }

exit:
    return;
}

void Server::HandleRouterAdded(Router &aRouter)
{
    VerifyOrExit(mActive);

    mRouterStateMask |= (1ULL << aRouter.GetRouterId());

    if (!mNeighborEnabled.IsEmpty())
    {
        ScheduleUpdateTimer(kUpdateBaseDelay);
    }

exit:
    return;
}

void Server::HandleRouterRemoved(Router &aRouter)
{
    VerifyOrExit(mActive);

    mRouterStateMask |= (1ULL << aRouter.GetRouterId());

    if (!mNeighborEnabled.IsEmpty())
    {
        ScheduleUpdateTimer(kUpdateBaseDelay);
    }

exit:
    return;
}

Error Server::EvictCache(bool aOnlyRxOn)
{
    Error error = kErrorNotFound;

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAny))
    {
        if (!child.CanEvictCache())
        {
            continue;
        }

        if (child.IsStateValid())
        {
            if (!child.IsRxOnWhenIdle())
            {
                if (aOnlyRxOn)
                {
                    continue;
                }

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
                if (!child.IsCslSynchronized())
                {
                    // First avoid non csl children as their poll intervals may be
                    // very large
                    continue;
                }
#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
            }

            child.EvictCache();
            mCacheSyncEvictions++;
        }
        else
        {
            child.ClearCache();
        }

        ExitNow(error = kErrorNone);
    }

    // Try evicting from any child
    VerifyOrExit(!aOnlyRxOn);
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        if (!child.CanEvictCache())
        {
            continue;
        }

        child.EvictCache();
        mCachePollEvictions++;
        ExitNow(error = kErrorNone);
    }

exit:
    if (error == kErrorNone)
    {
        ScheduleChildTimer();
    }
    return error;
}
#endif // OPENTHREAD_FTD

void Server::HandleNotifierEvents(Events aEvents)
{
    TlvSet tlvs;

    VerifyOrExit(mActive);

    if (aEvents.ContainsAny(kEventIp6AddressAdded | kEventIp6AddressRemoved | kEventIp6MulticastSubscribed |
                            kEventIp6MulticastUnsubscribed))
    {
        tlvs.Set(Tlv::kIp6AddressList);
        tlvs.Set(Tlv::kIp6LinkLocalAddressList);
        tlvs.Set(Tlv::kAlocList);
        MarkDiagDirty(tlvs);
    }

exit:
    return;
}

template <> void Server::HandleTmf<kUriMeshMonEndDeviceRequest>(Coap::Msg &aMsg)
{
    Error          error    = kErrorNone;
    Coap::Message *response = nullptr;

    ChildRequestHeader header;
    TlvSet             set;
    uint16_t           offset;
    bool               changed = false;

    VerifyOrExit(aMsg.IsPostRequest(), error = kErrorInvalidArgs);

    LogInfo("Received %s from %s", UriToString<kUriMeshMonEndDeviceRequest>(),
            aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

    VerifyOrExit(Get<Mle::Mle>().IsChild(), error = kErrorInvalidState);

    SuccessOrExit(error = aMsg.mMessage.Read(aMsg.mMessage.GetOffset(), header));

    offset = aMsg.mMessage.GetOffset() + sizeof(ChildRequestHeader);
    SuccessOrExit(error = set.ReadFrom(aMsg.mMessage, offset, header.GetRequestSetCount()));

    response = Get<Tmf::Agent>().AllocateAndInitResponseFor(aMsg.mMessage);
    VerifyOrExit(response != nullptr, error = kErrorNoBufs);

    switch (header.GetCommand())
    {
    case ChildRequestHeader::kStart:
        changed = ConfigureAsEndDevice(set);
        break;

    case ChildRequestHeader::kStop:
        StopServer();
        break;

    default:
        break;
    }

    if (header.GetQuery() || changed)
    {
        SuccessOrExit(error = AppendHostTlvs(*response, mSelfEnabled));
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*response, aMsg.mMessageInfo));

exit:
    FreeMessageOnError(response, error);
}

#if OPENTHREAD_FTD
template <> void Server::HandleTmf<kUriMeshMonEndDeviceUpdate>(Coap::Msg &aMsg)
{
    Child *child = nullptr;
    TlvSet set;

    VerifyOrExit(aMsg.mMessageInfo.GetPeerAddr().GetIid().IsRoutingLocator());
    child = Get<ChildTable>().FindChild(aMsg.mMessageInfo.GetPeerAddr().GetIid().GetLocator(), Child::kInStateValid);

    VerifyOrExit(child != nullptr);
    VerifyOrExit(aMsg.IsPostRequest());

    LogInfo("Received %s from %s", UriToString<kUriMeshMonEndDeviceRequest>(),
            aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

    IgnoreError(Get<Tmf::Agent>().SendAckResponse(aMsg));

    // If the child should be disabled, this will update it
    SyncChildDiagState(*child, false);

    if (child->IsFullThreadDevice())
    {
        set = mChildEnabled.GetFtdChildProvided();
    }
    else
    {
        set = mChildEnabled.GetMtdChildProvided();
    }

    if (child->UpdateCache(aMsg.mMessage, set) != kErrorNone)
    {
        mCacheErrors++;
    }

    if (child->ShouldSendDiagUpdate())
    {
        ScheduleUpdateTimer(kUpdateBaseDelay);
    }

    UpdateIfCacheBufferLimit();

exit:
    return;
}

template <> void Server::HandleTmf<kUriMeshMonServerRequest>(Coap::Msg &aMsg)
{
    RequestHeader  header;
    RequestContext context;
    uint16_t       offset;
    uint16_t       setOffset;
    TlvSet         set;

    TlvSet hostSet;
    TlvSet childSet;
    TlvSet neighborSet;

    VerifyOrExit(aMsg.IsPostRequest());
    SuccessOrExit(aMsg.mMessage.Read(aMsg.mMessage.GetOffset(), header));

    LogInfo("Received %s from %s", UriToString<kUriMeshMonServerRequest>(),
            aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

    VerifyOrExit(Get<Mle::Mle>().IsRouterOrLeader());
    VerifyOrExit(aMsg.mMessageInfo.GetPeerAddr().GetIid().IsRoutingLocator());

    if (header.GetRegistration() && (mClientRloc != Mle::kInvalidRloc16) &&
        (aMsg.mMessageInfo.GetPeerAddr().GetIid().GetLocator() != mClientRloc))
    {
        LogInfo("Rejecting registration from %04x, client %04x already registered",
                aMsg.mMessageInfo.GetPeerAddr().GetIid().GetLocator(), mClientRloc);
        IgnoreError(Get<Tmf::Agent>().SendAckResponse(aMsg, Coap::kCodeTooManyRequests));
        ExitNow();
    }

    hostSet.Clear();
    childSet.Clear();
    neighborSet.Clear();

    offset = aMsg.mMessage.GetOffset() + sizeof(RequestHeader);
    while (offset < aMsg.mMessage.GetLength())
    {
        SuccessOrExit(aMsg.mMessage.Read(offset, context));

        setOffset = offset + sizeof(RequestContext);
        SuccessOrExit(set.ReadFrom(aMsg.mMessage, setOffset, context.GetRequestSetCount()));

        switch (context.GetType())
        {
        case kTypeHost:
            hostSet.SetAll(set);
            break;

        case kTypeChild:
            childSet.SetAll(set);
            break;

        case kTypeNeighbor:
            neighborSet.SetAll(set);
            break;

        default:
            break;
        }

        VerifyOrExit(context.GetLength() >= sizeof(RequestContext) &&
                     static_cast<uint32_t>(offset) + context.GetLength() <= aMsg.mMessage.GetLength());
        offset += context.GetLength();
    }

    if (header.GetRegistration())
    {
        mClientRloc = aMsg.mMessageInfo.GetPeerAddr().GetIid().GetLocator();
    }

    SuccessOrExit(ConfigureAsRouter(hostSet, childSet, neighborSet, header.GetQuery()));

    if (header.GetQuery())
    {
        uint32_t jitter = Random::NonCrypto::GenerateFromMinUpToExcluding<uint32_t>(0, 500);

        mSendFullUpdate = true;
        ScheduleUpdateTimer(jitter);
    }
    else if (header.GetRegistration())
    {
        IgnoreError(SendEmptyServerUpdate(aMsg.mMessageInfo.GetPeerAddr()));
    }

exit:
    return;
}
#endif // OPENTHREAD_FTD

bool Server::ConfigureAsEndDevice(const TlvSet &aTypes)
{
    bool changed = false;

    if (!mActive)
    {
        mActive     = true;
        mUpdateSent = false;

        mSelfEnabled.Clear();
        mSelfPendingUpdate.Clear();
        mSelfDirty.Clear();

#if OPENTHREAD_FTD
        mChildEnabled.Clear();

        mNeighborEnabled.Clear();

        mClientRegistered = false;
#endif
    }

    changed = mSelfEnabled != aTypes;

    // Filter incoming requested TLVs to child-valid and seed enabled set
    mSelfEnabled = aTypes;
    mSelfEnabled.FilterChildSupportedTlv();

    // Seed dirty set with existing ExtDelay policy
    mSelfDirty.SetAll(GetExtDelayTlvs(mSelfEnabled));

    // Mark dirty any requested TLVs that are child-provided (MTD/FTD)
    // so they are sent via EU when explicitly requested by the router/client
#if OPENTHREAD_FTD
    {
        TlvSet childProvidedRequested;
        if (Get<Mle::Mle>().IsFullThreadDevice())
        {
            childProvidedRequested = mSelfEnabled.GetFtdChildProvided();
        }
        else
        {
            childProvidedRequested = mSelfEnabled.GetMtdChildProvided();
        }

        // Union requested child-provided TLVs into dirty set
        mSelfDirty.SetAll(mSelfDirty.Join(childProvidedRequested));
    }
#endif

    if (!mSelfDirty.IsEmpty())
    {
        ScheduleUpdateTimer(Random::NonCrypto::AddJitter(kUpdateExtDelay, kUpdateJitter));
    }

    return changed;
}

void Server::StopServer(void)
{
    mSelfEnabled.Clear();
    mSelfPendingUpdate.Clear();

    mUpdateTimer.Stop();
#if OPENTHREAD_FTD
    mRegistrationTimer.Stop();
    mUpdatePending = false;

    mChildEnabled.Clear();
    mNeighborEnabled.Clear();

    mClientRegistered = false;
    mClientRloc       = Mle::kInvalidRloc16;

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAny))
    {
        child.ClearCache();
    }

    if (Get<Mle::Mle>().IsRouterOrLeader())
    {
        // Stop child servers
        ScheduleChildTimer();
    }
#endif

    mActive = false;
}

Error Server::SendEndDeviceUpdate(void)
{
    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;

    VerifyOrExit(Get<Mle::Mle>().IsChild(), error = kErrorInvalidState);
    VerifyOrExit(!mUpdateSent, error = kErrorAlready);

    message = Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriMeshMonEndDeviceUpdate);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = AppendHostTlvs(*message, mSelfDirty));
    mSelfPendingUpdate = mSelfDirty;
    mSelfDirty.Clear();

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessageToRloc(*message, Get<Mle::Mle>().GetParentRloc16(),
                                                              HandleEndDeviceUpdateAck, this));
    mUpdateSent = true;

exit:
    if (error != kErrorNone)
    {
        LogCrit("Failed to send child update: %s", ErrorToString(error));
    }
    FreeMessageOnError(message, error);
    return error;
}

void Server::HandleEndDeviceUpdateAck(Coap::Msg *aMsg, Error aResult)
{
    OT_UNUSED_VARIABLE(aMsg);

    VerifyOrExit(mActive);

    mUpdateSent = false;

    if (aResult != kErrorNone)
    {
        mSelfDirty = mSelfDirty.Join(mSelfPendingUpdate);
    }

exit:
    return;
}

#if OPENTHREAD_FTD
Error Server::SendFullServerUpdate(const Ip6::Address &aClientAddr)
{
    Error error = kErrorNone;

    Coap::Message *message = nullptr;

    UpdateHeader header;

    // Don't send if we have a pending update awaiting ACK
    VerifyOrExit(!mUpdatePending, error = kErrorBusy);

    message = Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriMeshMonServerUpdate);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    header.Init();
    header.SetComplete(true);
    header.SetRouterId(Mle::RouterIdFromRloc16(Get<Mle::Mle>().GetRloc16()));
    header.SetFullSeqNumber(mSequenceNumber + 1);
    SuccessOrExit(error = header.AppendTo(*message));

    if (!mSelfEnabled.IsEmpty())
    {
        SuccessOrExit(error = AppendHostContext(*message, mSelfEnabled));
    }

    if (!mChildEnabled.IsEmpty())
    {
        mSendChildBaseline = true;
        ScheduleUpdateTimer(0);
    }

    if (!mNeighborEnabled.IsEmpty())
    {
        mSendNeighborBaseline = true;
        // Initialize router state mask to mark all valid routers for baseline
        mRouterStateMask = 0;

        for (uint8_t id = 0; id < Mle::kMaxRouterId; id++)
        {
            Router *router = Get<RouterTable>().FindRouterById(id);

            if (router != nullptr && router->IsStateValid())
            {
                mRouterStateMask |= (1ULL << id);
            }
        }

        ScheduleUpdateTimer(0);
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessageTo(*message, aClientAddr, HandleServerUpdateAck, this));
    mUpdatePending = true;

exit:
    if (error != kErrorNone)
    {
        LogCrit("Failed to send response: %s", ErrorToString(error));
    }
    FreeMessageOnError(message, error);
    return error;
}

Error Server::SendEmptyServerUpdate(const Ip6::Address &aClientAddr)
{
    Error error = kErrorNone;

    Coap::Message *message = nullptr;

    UpdateHeader header;

    // Don't send if we have a pending update awaiting ACK
    VerifyOrExit(!mUpdatePending, error = kErrorBusy);

    message = Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriMeshMonServerUpdate);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    header.Init();
    header.SetRouterId(Mle::RouterIdFromRloc16(Get<Mle::Mle>().GetRloc16()));
    header.SetFullSeqNumber(mSequenceNumber + 1);
    SuccessOrExit(error = header.AppendTo(*message));

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessageTo(*message, aClientAddr, HandleServerUpdateAck, this));
    mUpdatePending = true;

exit:
    if (error != kErrorNone)
    {
        LogCrit("Failed to send response: %s", ErrorToString(error));
    }
    FreeMessageOnError(message, error);
    return error;
}

Error Server::SendServerUpdate(void)
{
    Error error = kErrorNone;

    Coap::Message *message = nullptr;

    UpdateHeader header;
    TlvSet       hostSet;
    Context      hostContext;
    uint16_t     offset;
    uint16_t     startIndex         = mChildResumeIndex;
    uint8_t      neighborStartIndex = mNeighborResumeIndex;
    bool         needsAnotherUpdate = false;
    bool         neighborNeedsMore  = false;

    // Don't send if we have a pending update awaiting ACK
    VerifyOrExit(!mUpdatePending, error = kErrorBusy);

    LockChildCaches();

    message = Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriMeshMonServerUpdate);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    header.Init();
    header.SetRouterId(Mle::RouterIdFromRloc16(Get<Mle::Mle>().GetRloc16()));
    header.SetShortSeqNumber(mSequenceNumber + 1);

    SuccessOrExit(error = header.AppendTo(*message));

    // Host TLVs only in first batch
    if (startIndex == 0)
    {
        hostSet = mSelfDirty.Intersect(mSelfEnabled);
        if (!hostSet.IsEmpty())
        {
            hostContext.Init();
            hostContext.SetType(kTypeHost);

            offset = message->GetLength();
            SuccessOrExit(error = message->Append(hostContext));
            SuccessOrExit(error = AppendHostTlvs(*message, hostSet));
            hostContext.SetLength(message->GetLength() - offset);
            message->Write(offset, hostContext);
        }
    }

    // Child updates
    if (!mChildEnabled.IsEmpty())
    {
        VerifyOrExit(AppendChildContextBatch(*message, needsAnotherUpdate), error = kErrorFailed);
    }

    // Neighbor updates only after children are done
    if (!mNeighborEnabled.IsEmpty() && !needsAnotherUpdate && mChildResumeIndex == 0)
    {
        VerifyOrExit(AppendNeighborContextBatch(*message, neighborNeedsMore), error = kErrorFailed);
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessageToRloc(*message, mClientRloc, HandleServerUpdateAck, this));

    mUpdatePending = true;

    if (startIndex == 0 && neighborStartIndex == 0)
    {
        mSelfDirty.Clear();
    }

exit:
    if (error == kErrorNone)
    {
        CommitChildCacheUpdates();

        if (!mUpdatePending && (needsAnotherUpdate || mSendChildBaseline || neighborNeedsMore || mSendNeighborBaseline))
        {
            ScheduleUpdateTimer(0);
        }
    }
    else if (error == kErrorBusy)
    {
        // If busy due to pending ACK, don't treat as error
    }
    else
    {
        LogCrit("Failed to send router update %s", ErrorToString(error));
        UnlockChildCaches();
        FreeMessage(message);
        mChildResumeIndex = 0;

        ScheduleChildTimer();
    }
    return error;
}

Error Server::ConfigureAsRouter(const TlvSet &aSelf, const TlvSet &aChild, const TlvSet &aNeighbor, bool aQuery)
{
    Error  error = kErrorNone;
    TlvSet oldFtd;
    TlvSet oldMtd;

    if (!mActive)
    {
        VerifyOrExit(!aSelf.IsEmpty() || !aChild.IsEmpty() || !aNeighbor.IsEmpty(), error = kErrorInvalidArgs);

        mSequenceNumber = Random::NonCrypto::Generate<uint32_t>();
        mSequenceNumber |= static_cast<uint64_t>(Random::NonCrypto::Generate<uint32_t>()) << 32;

        mSelfEnabled.Clear();
        mSelfPendingUpdate.Clear();
        mSelfDirty.Clear();

        mChildEnabled.Clear();
        mChildResumeIndex = 0;
        mUpdatePending    = false;

        mNeighborEnabled.Clear();

        mClientRegistered = false;

        mActive = true;

        mRouterStateMask = 0;

        mRegistrationTimer.Start(kRegistrationInterval);
    }

    mSelfEnabled = aSelf;
    mSelfEnabled.FilterHostSupportedTlv();

    mSelfDirty.SetAll(GetExtDelayTlvs(mSelfEnabled));

    oldFtd = mChildEnabled.GetFtdChildProvided();
    oldMtd = mChildEnabled.GetMtdChildProvided();

    mChildEnabled = aChild;
    mChildEnabled.FilterChildSupportedTlv();

    SyncAllChildDiagStates(oldFtd != mChildEnabled.GetFtdChildProvided(), oldMtd != mChildEnabled.GetMtdChildProvided(),
                           aQuery);

    mNeighborEnabled = aNeighbor;
    mNeighborEnabled.FilterNeighborSupportedTlv();

    // Mark client registered for this interval
    mClientRegistered = true;

    if (!mSelfDirty.IsEmpty())
    {
        ScheduleUpdateTimer(Random::NonCrypto::AddJitter(kUpdateExtDelay, kUpdateJitter));
    }

exit:
    return error;
}

void Server::SyncAllChildDiagStates(bool aMtdChanged, bool aFtdChanged, bool aQuery)
{
    bool changed;

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        if (child.IsFullThreadDevice())
        {
            changed = aFtdChanged;
        }
        else
        {
            changed = aMtdChanged;
        }

        SyncChildDiagState(child, changed | aQuery);
    }
}

void Server::SyncChildDiagState(Child &aChild, bool aQuery)
{
    TlvSet set;

    if (aChild.IsFullThreadDevice())
    {
        set = mChildEnabled.GetFtdChildProvided();
    }
    else
    {
        set = mChildEnabled.GetMtdChildProvided();
    }

    if (set.IsEmpty())
    {
        aChild.ClearCache();

        switch (aChild.GetDiagServerState())
        {
        case Child::kDiagServerActive:
        case Child::kDiagServerActivePending:
        case Child::kDiagServerUnknown:
            IgnoreError(SendEndDeviceRequestStop(aChild));
            break;

        case Child::kDiagServerStopped:
        case Child::kDiagServerStopPending:
            break;
        }
    }
    else
    {
        switch (aChild.GetDiagServerState())
        {
        case Child::kDiagServerActive:
        case Child::kDiagServerActivePending:
            if (!aQuery)
            {
                break;
            }
            // If SendEndDeviceRequestStart fails it will still stop the pending transaction
            // and set state to unknown so the next update will retry even
            // without aQuery being set.
            OT_FALL_THROUGH;

        case Child::kDiagServerUnknown:
            // Make sure we always query after failed updates
            aQuery = true;
            OT_FALL_THROUGH;

        case Child::kDiagServerStopped:
        case Child::kDiagServerStopPending:
            IgnoreError(SendEndDeviceRequestStart(aChild, set, aQuery));
            break;
        }
    }
}

Error Server::SendEndDeviceRequestStop(Child &aChild)
{
    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;

    ChildRequestHeader header;

    if (aChild.IsDiagServerPending())
    {
        IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleEndDeviceRequestAck, &aChild));
    }

    message = Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriMeshMonEndDeviceRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    header.Clear();
    header.SetCommand(ChildRequestHeader::kStop);

    SuccessOrExit(error = message->Append(header));

    SuccessOrExit(
        error = Get<Tmf::Agent>().SendMessageToRloc(*message, aChild.GetRloc16(), HandleEndDeviceRequestAck, &aChild));
    aChild.SetDiagServerState(Child::kDiagServerStopPending);

    LogInfo("Sent DiagServer stop to child %04x", aChild.GetRloc16());

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Server::SendEndDeviceRequestStart(Child &aChild, const TlvSet &aTypes, bool aQuery)
{
    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;

    ChildRequestHeader header;
    uint16_t           offset;
    uint8_t            setCount = 0;

    if (aChild.IsDiagServerPending())
    {
        IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleEndDeviceRequestAck, &aChild));
    }

    message = Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriMeshMonEndDeviceRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    offset = message->GetLength();

    header.Clear();
    header.SetCommand(ChildRequestHeader::kStart);
    header.SetQuery(aQuery);

    SuccessOrExit(error = message->Append(header));
    SuccessOrExit(error = aTypes.AppendTo(*message, setCount));

    header.SetRequestSetCount(setCount);
    message->Write(offset, header);

    SuccessOrExit(
        error = Get<Tmf::Agent>().SendMessageToRloc(*message, aChild.GetRloc16(), HandleEndDeviceRequestAck, &aChild));
    aChild.SetDiagServerState(Child::kDiagServerActivePending);

    LogInfo("Sent DiagServer start to child %04x", aChild.GetRloc16());

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Server::SendEndDeviceRecoveryQuery(Child &aChild, const TlvSet &aTypes)
{
    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;

    ChildRequestHeader header;
    uint16_t           offset;
    uint8_t            setCount = 0;

    message = Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriMeshMonEndDeviceRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    offset = message->GetLength();

    header.Clear();
    header.SetCommand(ChildRequestHeader::kStart);
    header.SetQuery(true);

    SuccessOrExit(error = message->Append(header));
    SuccessOrExit(error = aTypes.AppendTo(*message, setCount));

    header.SetRequestSetCount(setCount);
    message->Write(offset, header);

    SuccessOrExit(
        error = Get<Tmf::Agent>().SendMessageToRloc(*message, aChild.GetRloc16(), HandleEndDeviceRecoveryAck, &aChild));
    aChild.SetLostDiagQueryPending(true);

    LogInfo("Sent DiagServer lost query to child %04x", aChild.GetRloc16());

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Server::HandleEndDeviceRequestAck(Child &aChild, Coap::Msg *aMsg, Error aResult)
{
    ChildInfo::DiagState state = aChild.GetDiagServerState();

    if (aResult == kErrorNone)
    {
        if (state == Child::kDiagServerActivePending)
        {
            state = Child::kDiagServerActive;
            LogInfo("Child %04x state changed to active", aChild.GetRloc16());
        }
        else if (state == Child::kDiagServerStopPending)
        {
            state = Child::kDiagServerStopped;
            LogInfo("Child %04x state changed to stopped", aChild.GetRloc16());
        }
        else
        {
            LogWarn("Received response for child but state is not pending");
            state = Child::kDiagServerUnknown;
        }

        aChild.SetDiagServerState(state);

        VerifyOrExit(aMsg != nullptr);
        if (aChild.UpdateCache(aMsg->mMessage, mChildEnabled) == kErrorNone)
        {
            ScheduleUpdateTimer(kUpdateBaseDelay);
        }
        else
        {
            mCacheErrors++;
            // TODO: Cache allocation failure handling
            //
            // When UpdateCache() fails, typically kErrorNoBufs, we face a recovery dilemma:
            // 1. The CoAP ACK was already sent -> child thinks we received the data
            // 2. We are out of memory -> cannot allocate buffers to store the response
            // 3. The child won't retry -> it believes transmission succeeded
            //
            // Current mitigation: mCacheErrors counter tracks failures, mLostSet marks
            // missing TLVs for later re-query via SendEndDeviceRecoveryQuery().
        }

        UpdateIfCacheBufferLimit();
    }
    else
    {
        aChild.SetDiagServerState(Child::kDiagServerUnknown);
    }

exit:
    // Verify child state
    ScheduleChildTimer();
}

void Server::HandleEndDeviceRequestAck(void *aContext, Coap::Msg *aMsg, Error aResult)
{
    OT_ASSERT(aContext != nullptr);

    Child *child = static_cast<Child *>(aContext);

    child->GetInstance().Get<Server>().HandleEndDeviceRequestAck(*child, aMsg, aResult);
}

void Server::HandleEndDeviceRecoveryAck(Child &aChild, Coap::Msg *aMsg, Error aResult)
{
    aChild.SetLostDiagQueryPending(false);

    if (aResult == kErrorNone)
    {
        OT_ASSERT(aMsg != nullptr);
        if (aChild.UpdateCache(aMsg->mMessage, aChild.GetLostDiag()) != kErrorNone)
        {
            mCacheErrors++;
        }

        UpdateIfCacheBufferLimit();
    }
    else
    {
        // Retry later
        ScheduleChildTimer();
    }
}

void Server::HandleEndDeviceRecoveryAck(void *aContext, Coap::Msg *aMsg, Error aResult)
{
    OT_ASSERT(aContext != nullptr);

    Child *child = static_cast<Child *>(aContext);

    child->GetInstance().Get<Server>().HandleEndDeviceRecoveryAck(*child, aMsg, aResult);
}

void Server::HandleServerUpdateAck(Coap::Msg *aMsg, Error aResult)
{
    OT_UNUSED_VARIABLE(aMsg);

    mUpdatePending = false;

    if (aResult == kErrorNone)
    {
        mSequenceNumber++;
        mUpdateRetryCount = 0; // Reset backoff counter on success

        // If more batches pending, schedule next send
        if (mSendChildBaseline || mChildResumeIndex != 0 || mSendNeighborBaseline || mNeighborResumeIndex != 0)
        {
            ScheduleUpdateTimer(0);
        }
    }
    else
    {
        mUpdateRetryCount++;

        if (mUpdateRetryCount >= kMaxUpdateRetries)
        {
            // Give up after 5 retry attempts. Increment sequence to maintain at-most-once semantics.
            //
            // Case 1: Client received the message but ACK was lost
            //   -> Incrementing keeps our sequence in sync with client (both at N+1)
            //   -> Without increment, we would send duplicate seq=(N+1) later
            //
            // Case 2: Client never received the message
            //   -> Client detects sequence gap (expected N+1, gets N+2 later)
            //   -> Client sends error query to request resync
            //
            // NOTE: We clear mChildResumeIndex and mSendChildBaseline to discard any pending
            // child updates that haven't been sent yet. The client will detect missing data
            // and request a full resync.

            mSequenceNumber++;
            mUpdateRetryCount = 0;
            mSelfDirty.Clear();
            mChildResumeIndex     = 0;
            mSendChildBaseline    = false;
            mNeighborResumeIndex  = 0;
            mSendNeighborBaseline = false;
        }
        else
        {
            uint32_t backoffDelay = kUpdateBaseDelay << (mUpdateRetryCount - 1);
            backoffDelay          = Min(backoffDelay, kMaxUpdateBackoff);

            ScheduleUpdateTimer(backoffDelay);
        }
    }
}
#endif // OPENTHREAD_FTD

Error Server::AppendHostTlvs(Message &aMessage, TlvSet aTlvs)
{
    Error error = kErrorNone;

    for (Tlv::Type type : aTlvs)
    {
        switch (type)
        {
#if OPENTHREAD_FTD
        case Tlv::kExtAddress:
            SuccessOrExit(error = Tlv::Append<ExtAddressTlv>(aMessage, Get<Mac::Mac>().GetExtAddress()));
            break;

        case Tlv::kMode:
            SuccessOrExit(error = Tlv::Append<ModeTlv>(aMessage, Get<Mle::Mle>().GetDeviceMode().Get()));
            break;

        case Tlv::kRoute64:
            SuccessOrExit(error = Get<RouterTable>().AppendRouteTlv(aMessage, Tlv::kRoute64));
            break;
#endif

        case Tlv::kMlEid:
            SuccessOrExit(error = Tlv::Append<MlEidTlv>(aMessage, Get<Mle::Mle>().GetMeshLocalEid().GetIid()));
            break;

        case Tlv::kIp6AddressList:
            SuccessOrExit(error = AppendHostIp6AddressList(aMessage));
            break;

        case Tlv::kAlocList:
            SuccessOrExit(error = AppendHostAlocList(aMessage));
            break;

#if OPENTHREAD_FTD
        case Tlv::kThreadSpecVersion:
            SuccessOrExit(error = Tlv::Append<ThreadSpecVersionTlv>(aMessage, kThreadVersion));
            break;
#endif

        case Tlv::kThreadStackVersion:
            SuccessOrExit(error = Tlv::Append<ThreadStackVersionTlv>(aMessage, otGetVersionString()));
            break;

        case Tlv::kVendorName:
            SuccessOrExit(error = Tlv::Append<VendorNameTlv>(aMessage, Get<VendorInfo>().GetName()));
            break;

        case Tlv::kVendorModel:
            SuccessOrExit(error = Tlv::Append<VendorModelTlv>(aMessage, Get<VendorInfo>().GetModel()));
            break;

        case Tlv::kVendorSwVersion:
            SuccessOrExit(error = Tlv::Append<VendorSwVersionTlv>(aMessage, Get<VendorInfo>().GetSwVersion()));
            break;

        case Tlv::kVendorAppUrl:
            SuccessOrExit(error = Tlv::Append<VendorAppUrlTlv>(aMessage, Get<VendorInfo>().GetAppUrl()));
            break;

        case Tlv::kIp6LinkLocalAddressList:
            SuccessOrExit(error = AppendHostLinkLocalAddressList(aMessage));
            break;

        case Tlv::kEui64:
        {
            Mac::ExtAddress eui64;

            Get<Radio>().GetIeeeEui64(eui64);
            SuccessOrExit(error = Tlv::Append<Eui64Tlv>(aMessage, eui64));
            break;
        }

        case Tlv::kMacCounters:
        {
            NetDiag::MacCountersTlvValue tlvValue;
            const Mac::Counters         &counters = Get<Mac::Mac>().GetCounters();

            tlvValue.InitFrom(counters);
            SuccessOrExit(error = Tlv::Append<MacCountersTlv>(aMessage, tlvValue));
            break;
        }

        case Tlv::kMacLinkErrorRatesRx:
        {
            MacLinkErrorRatesRxTlv tlv;
            Parent                &parent = Get<Mle::Mle>().GetParent();

            VerifyOrExit(Get<Mle::Mle>().IsChild());

            tlv.Init();
            tlv.SetMessageErrorRates(parent.GetLinkInfo().GetMessageErrorRate());
            tlv.SetFrameErrorRates(parent.GetLinkInfo().GetFrameErrorRate());

            SuccessOrExit(error = aMessage.Append(tlv));
            break;
        }

        case Tlv::kMleCounters:
        {
            NetDiag::MleCountersTlvValue tlvValue;

            tlvValue.InitFrom(Get<Mle::Mle>().GetCounters());
            SuccessOrExit(error = Tlv::Append<MleCountersTlv>(aMessage, tlvValue));
            break;
        }

        case Tlv::kLinkMarginOut:
        {
            LinkMarginOutTlv tlv;
            Parent          &parent = Get<Mle::Mle>().GetParent();

            VerifyOrExit(Get<Mle::Mle>().IsChild());

            tlv.Init();
            tlv.SetLinkMargin(parent.GetLinkInfo().GetLinkMargin());
            tlv.SetAverageRssi(parent.GetLinkInfo().GetAverageRss());
            tlv.SetLastRssi(parent.GetLinkInfo().GetLastRss());

            SuccessOrExit(error = aMessage.Append(tlv));
            break;
        }

        default:
            break;
        }
    }

exit:
    return error;
}

#if OPENTHREAD_FTD
Error Server::AppendHostContext(Message &aMessage, TlvSet aTlvs)
{
    Error error = kErrorNone;

    Context  context;
    uint16_t offset = aMessage.GetLength();

    context.Init();
    context.SetType(kTypeHost);
    SuccessOrExit(error = aMessage.Append(context));

    SuccessOrExit(error = AppendHostTlvs(aMessage, aTlvs));

    context.SetLength(aMessage.GetLength() - offset);
    aMessage.Write(offset, context);

exit:
    return error;
}

Error Server::AppendChildContext(Message &aMessage, Child &aChild)
{
    Error error = kErrorNone;

    ChildContext context;
    uint16_t     offset  = aMessage.GetLength();
    bool         allTlvs = false;

    VerifyOrExit(aChild.ShouldSendDiagUpdate());

    context.Init();
    context.SetType(kTypeChild);
    context.SetId(Mle::ChildIdFromRloc16(aChild.GetRloc16()));
    SuccessOrExit(error = aMessage.Append(context));

    context.SetUpdateMode(kModeUpdated);

    if (aChild.IsAttachStateDirty())
    {
        if (aChild.IsStateValid())
        {
            context.SetUpdateMode(kModeAdded);
            allTlvs = true;
        }
        else
        {
            context.SetUpdateMode(kModeRemoved);
        }
    }

    if (aChild.IsStateValid())
    {
        TlvSet tlvs = aChild.GetDirtyHostProvided(mChildEnabled);
        if (allTlvs)
        {
            if (aChild.mIsFtd)
            {
                tlvs = mChildEnabled.GetNonFtdChildProvided();
            }
            else
            {
                tlvs = mChildEnabled.GetNonMtdChildProvided();
            }
        }

        SuccessOrExit(error = AppendChildTlvs(aMessage, tlvs, aChild));
        SuccessOrExit(error = aChild.AppendCachedTlvs(aMessage));
    }

    context.SetLength(aMessage.GetLength() - offset);
    aMessage.Write(offset, context);

exit:
    return error;
}

Error Server::AppendChildTlvs(Message &aMessage, TlvSet aTlvs, const Child &aChild)
{
    Error error = kErrorNone;

    for (Tlv::Type type : aTlvs)
    {
        switch (type)
        {
        case Tlv::kExtAddress:
            SuccessOrExit(error = Tlv::Append<ExtAddressTlv>(aMessage, aChild.GetExtAddress()));
            break;

        case Tlv::kMode:
            SuccessOrExit(error = Tlv::Append<ModeTlv>(aMessage, aChild.GetDeviceMode().Get()));
            break;

        case Tlv::kTimeout:
            SuccessOrExit(error = Tlv::Append<TimeoutTlv>(aMessage, aChild.GetTimeout()));
            break;

        case Tlv::kLastHeard:
            SuccessOrExit(error = Tlv::Append<LastHeardTlv>(aMessage, TimerMilli::GetNow() - aChild.GetLastHeard()));
            break;

        case Tlv::kConnectionTime:
            SuccessOrExit(error = Tlv::Append<ConnectionTimeTlv>(aMessage, aChild.GetConnectionTime()));
            break;

        case Tlv::kCsl:
        {
            CslTlv tlv;
            tlv.Init();

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
            tlv.SetChannel(aChild.GetCslChannel());
            tlv.SetTimeout(aChild.GetCslTimeout());

            if (aChild.IsCslSynchronized())
            {
                tlv.SetPeriod(aChild.GetCslPeriod());
            }
#endif

            SuccessOrExit(error = aMessage.Append(tlv));
            break;
        }

        case Tlv::kLinkMarginIn:
        {
            LinkMarginInTlv tlv;
            tlv.Init();

            tlv.SetLinkMargin(aChild.GetLinkInfo().GetLinkMargin());
            tlv.SetAverageRssi(aChild.GetLinkInfo().GetAverageRss());
            tlv.SetLastRssi(aChild.GetLinkInfo().GetLastRss());

            SuccessOrExit(error = aMessage.Append(tlv));
            break;
        }

        case Tlv::kMacLinkErrorRatesTx:
        {
            MacLinkErrorRatesTxTlv tlv;
            tlv.Init();

            tlv.SetMessageErrorRates(aChild.GetLinkInfo().GetMessageErrorRate());
            tlv.SetFrameErrorRates(aChild.GetLinkInfo().GetFrameErrorRate());

            SuccessOrExit(error = aMessage.Append(tlv));
            break;
        }

        case Tlv::kMlEid:
            SuccessOrExit(error = Tlv::Append<MlEidTlv>(aMessage, aChild.GetMeshLocalIid()));
            break;

        case Tlv::kIp6AddressList:
            SuccessOrExit(error = AppendChildIp6AddressList(aMessage, aChild));
            break;

        case Tlv::kAlocList:
            SuccessOrExit(error = AppendChildAlocList(aMessage, aChild));
            break;

        case Tlv::kThreadSpecVersion:
            SuccessOrExit(error = Tlv::Append<ThreadSpecVersionTlv>(aMessage, aChild.GetVersion()));
            break;

        default:
            break;
        }
    }

exit:
    return error;
}

Error Server::AppendNeighborContextBaseline(Message &aMessage, const Router &aRouter)
{
    Error error = kErrorNone;

    NeighborContext context;
    uint16_t        offset = aMessage.GetLength();

    context.Init();
    context.SetType(kTypeNeighbor);
    context.SetId(aRouter.GetRouterId());
    context.SetUpdateMode(kModeAdded);
    SuccessOrExit(error = aMessage.Append(context));
    SuccessOrExit(error = AppendNeighborTlvs(aMessage, mNeighborEnabled, aRouter));

    context.SetLength(aMessage.GetLength() - offset);
    aMessage.Write(offset, context);

exit:
    return error;
}

Error Server::AppendNeighborContextUpdate(Message &aMessage, uint8_t aId)
{
    Error   error  = kErrorNone;
    Router *router = Get<RouterTable>().FindRouterById(aId);
    bool    valid  = (router != nullptr) && router->IsStateValid();
    TlvSet  tlvs   = mNeighborEnabled;

    NeighborContext context;
    uint16_t        offset = aMessage.GetLength();

    VerifyOrExit(valid || (mRouterStateMask & (1ULL << aId)) != 0);

    context.Init();
    context.SetType(kTypeNeighbor);
    context.SetId(aId);
    if (valid)
    {
        if (mRouterStateMask & (1ULL << aId))
        {
            context.SetUpdateMode(kModeAdded);
        }
        else
        {
            context.SetUpdateMode(kModeUpdated);

            tlvs.ClearAll(static_cast<const TlvSet &>(kStaticNeighborTlvMask));
            VerifyOrExit(!tlvs.IsEmpty());
        }
    }
    else
    {
        context.SetUpdateMode(kModeRemoved);
    }
    SuccessOrExit(error = aMessage.Append(context));

    if (valid)
    {
        SuccessOrExit(error = AppendNeighborTlvs(aMessage, tlvs, *router));
    }

    context.SetLength(aMessage.GetLength() - offset);
    aMessage.Write(offset, context);

exit:
    return error;
}

Error Server::AppendNeighborTlvs(Message &aMessage, TlvSet aTlvs, const Router &aNeighbor)
{
    Error error = kErrorNone;

    for (Tlv::Type type : aTlvs)
    {
        switch (type)
        {
        case Tlv::kExtAddress:
            SuccessOrExit(error = Tlv::Append<ExtAddressTlv>(aMessage, aNeighbor.GetExtAddress()));
            break;

        case Tlv::kLastHeard:
            SuccessOrExit(error = Tlv::Append<LastHeardTlv>(aMessage, TimerMilli::GetNow() - aNeighbor.GetLastHeard()));
            break;

        case Tlv::kConnectionTime:
            SuccessOrExit(error = Tlv::Append<ConnectionTimeTlv>(aMessage, aNeighbor.GetConnectionTime()));
            break;

        case Tlv::kLinkMarginIn:
        {
            LinkMarginInTlv tlv;
            tlv.Init();

            tlv.SetLinkMargin(aNeighbor.GetLinkInfo().GetLinkMargin());
            tlv.SetAverageRssi(aNeighbor.GetLinkInfo().GetAverageRss());
            tlv.SetLastRssi(aNeighbor.GetLinkInfo().GetLastRss());

            SuccessOrExit(error = aMessage.Append(tlv));
            break;
        }

        case Tlv::kMacLinkErrorRatesTx:
        {
            MacLinkErrorRatesTxTlv tlv;
            tlv.Init();

            tlv.SetMessageErrorRates(aNeighbor.GetLinkInfo().GetMessageErrorRate());
            tlv.SetFrameErrorRates(aNeighbor.GetLinkInfo().GetFrameErrorRate());

            SuccessOrExit(error = aMessage.Append(tlv));
            break;
        }

        case Tlv::kThreadSpecVersion:
            SuccessOrExit(error = Tlv::Append<ThreadSpecVersionTlv>(aMessage, aNeighbor.GetVersion()));
            break;

        default:
            break;
        }
    }

exit:
    return error;
}

bool Server::AppendChildContextBatch(Message &aMessage, bool &aNeedsMore)
{
    bool               success        = true;
    uint16_t           childIndex     = 0;
    uint16_t           startIndex     = mChildResumeIndex;
    Child::StateFilter filter         = mSendChildBaseline ? Child::kInStateValid : Child::kInStateAny;
    bool               abortRemaining = false;

    aNeedsMore = false;

    for (Child &child : Get<ChildTable>().Iterate(filter))
    {
        if (abortRemaining)
        {
            child.AbortCacheUpdate();
            childIndex++;
            continue;
        }

        if (childIndex < startIndex)
        {
            childIndex++;
            continue;
        }

        if (mSendChildBaseline)
        {
            child.SetAttachStateDirty();
        }
        else if (!child.ShouldSendDiagUpdate())
        {
            childIndex++;
            continue;
        }

        uint16_t beforeLen = aMessage.GetLength();
        Error    error     = AppendChildContext(aMessage, child);

        if (error == kErrorNoBufs || aMessage.GetLength() > kMaxUpdateMessageLength)
        {
            IgnoreError(aMessage.SetLength(beforeLen));
            child.AbortCacheUpdate();
            aNeedsMore        = true;
            mChildResumeIndex = childIndex;
            abortRemaining    = true;
            childIndex++;
            continue;
        }

        if (error != kErrorNone)
        {
            success        = false;
            abortRemaining = true;
            childIndex++;
            continue;
        }

        childIndex++;
    }

    if (!aNeedsMore)
    {
        if (mSendChildBaseline)
        {
            mSendChildBaseline = false;
        }
        mChildResumeIndex = 0;
    }

    return success;
}

bool Server::AppendNeighborContextBatch(Message &aMessage, bool &aNeedsMore)
{
    bool    success    = true;
    uint8_t startIndex = mNeighborResumeIndex;
    aNeedsMore         = false;

    for (uint8_t id = startIndex; id < Mle::kMaxRouterId; id++)
    {
        Router *router = Get<RouterTable>().FindRouterById(id);
        bool    valid  = (router != nullptr) && router->IsStateValid();

        // In baseline mode, only process valid routers that are in the state mask
        // In update mode, process if there's a state change (added/removed)
        if (mSendNeighborBaseline)
        {
            if (!valid || (mRouterStateMask & (1ULL << id)) == 0)
            {
                continue;
            }
        }
        else
        {
            // Skip if no state change and router not valid (nothing to report)
            if (!valid && (mRouterStateMask & (1ULL << id)) == 0)
            {
                continue;
            }
        }

        uint16_t beforeLen = aMessage.GetLength();
        Error    error;

        if (mSendNeighborBaseline)
        {
            error = AppendNeighborContextBaseline(aMessage, *router);
        }
        else
        {
            error = AppendNeighborContextUpdate(aMessage, id);
        }

        if (error == kErrorNoBufs || aMessage.GetLength() > kMaxUpdateMessageLength)
        {
            IgnoreError(aMessage.SetLength(beforeLen));
            aNeedsMore           = true;
            mNeighborResumeIndex = id;
            break;
        }

        if (error != kErrorNone)
        {
            // kErrorNotFound is expected when neighbor has no changes
            if (error != kErrorNotFound)
            {
                success = false;
                break;
            }
        }
    }

    if (!aNeedsMore)
    {
        if (mSendNeighborBaseline)
        {
            mSendNeighborBaseline = false;
        }
        mNeighborResumeIndex = 0;
        mRouterStateMask     = 0;
    }

    return success;
}

void Server::LockChildCaches(void)
{
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAny))
    {
        child.LockCache();
    }
}

void Server::CommitChildCacheUpdates(void)
{
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAny))
    {
        // Only commit if the cache is still locked (meaning it wasn't aborted)
        if (child.IsDiagCacheLocked())
        {
            child.CommitCacheUpdate();
        }
    }
}

void Server::UnlockChildCaches(void)
{
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAny))
    {
        if (child.IsDiagCacheLocked())
        {
            child.AbortCacheUpdate();
        }
    }
}
#endif // OPENTHREAD_FTD

bool Server::ShouldIncludeIp6Address(const Ip6::Address &aAddress)
{
    bool pass = false;

    VerifyOrExit(!Get<Mle::Mle>().IsMeshLocalAddress(aAddress));
    VerifyOrExit(!aAddress.IsLinkLocalUnicastOrMulticast());
    VerifyOrExit(aAddress != Ip6::Address::GetRealmLocalAllNodesMulticast());
    VerifyOrExit(aAddress != Ip6::Address::GetRealmLocalAllRoutersMulticast());
    VerifyOrExit(aAddress != Ip6::Address::GetRealmLocalAllMplForwarders());
    VerifyOrExit(aAddress.IsMulticast() || !aAddress.GetIid().IsAnycastLocator());

    pass = true;

exit:
    return pass;
}

bool Server::ShouldIncludeAloc(const Ip6::Address &aAddress, uint8_t &aAloc)
{
    bool pass = false;

    VerifyOrExit(aAddress.GetIid().IsAnycastLocator());
    aAloc = static_cast<uint8_t>(aAddress.GetIid().GetLocator());

    pass = true;

exit:
    return pass;
}

bool Server::ShouldIncludeLinkLocalAddress(const Ip6::Address &aAddress)
{
    bool pass = false;

    VerifyOrExit(aAddress.IsLinkLocalUnicastOrMulticast());
    VerifyOrExit(aAddress != Ip6::Address::GetLinkLocalAllNodesMulticast());
    VerifyOrExit(aAddress != Ip6::Address::GetLinkLocalAllRoutersMulticast());

    pass = true;

exit:
    return pass;
}

Error Server::AppendHostIp6AddressList(Message &aMessage)
{
    Error    error = kErrorNone;
    uint16_t count = 0;

    for (const Ip6::Netif::UnicastAddress &address : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (ShouldIncludeIp6Address(address.GetAddress()))
        {
            count++;
        }
    }
    for (const Ip6::Netif::MulticastAddress &address : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (ShouldIncludeIp6Address(address.GetAddress()))
        {
            count++;
        }
    }

    if (count * Ip6::Address::kSize <= Tlv::kBaseTlvMaxLength)
    {
        Tlv tlv;

        tlv.SetType(Tlv::kIp6AddressList);
        tlv.SetLength(static_cast<uint8_t>(count * Ip6::Address::kSize));
        SuccessOrExit(error = aMessage.Append(tlv));
    }
    else
    {
        ExtendedTlv tlv;

        tlv.SetType(Tlv::kIp6AddressList);
        tlv.SetLength(count * Ip6::Address::kSize);
        SuccessOrExit(error = aMessage.Append(tlv));
    }

    for (const Ip6::Netif::UnicastAddress &address : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (ShouldIncludeIp6Address(address.GetAddress()))
        {
            SuccessOrExit(error = aMessage.Append(address.GetAddress()));
        }
    }
    for (const Ip6::Netif::MulticastAddress &address : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (ShouldIncludeIp6Address(address.GetAddress()))
        {
            SuccessOrExit(error = aMessage.Append(address.GetAddress()));
        }
    }

exit:
    return error;
}

Error Server::AppendHostAlocList(Message &aMessage)
{
    Error    error = kErrorNone;
    uint16_t count = 0;
    uint8_t  aloc;

    for (const Ip6::Netif::UnicastAddress &address : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (ShouldIncludeAloc(address.GetAddress(), aloc))
        {
            count++;
        }
    }

    if (count <= Tlv::kBaseTlvMaxLength)
    {
        Tlv tlv;

        tlv.SetType(Tlv::kAlocList);
        tlv.SetLength(static_cast<uint8_t>(count));
        SuccessOrExit(error = aMessage.Append(tlv));
    }
    else
    {
        ExtendedTlv tlv;

        tlv.SetType(Tlv::kAlocList);
        tlv.SetLength(count);
        SuccessOrExit(error = aMessage.Append(tlv));
    }

    for (const Ip6::Netif::UnicastAddress &address : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (ShouldIncludeAloc(address.GetAddress(), aloc))
        {
            SuccessOrExit(error = aMessage.Append(aloc));
        }
    }

exit:
    return error;
}

Error Server::AppendHostLinkLocalAddressList(Message &aMessage)
{
    Error    error = kErrorNone;
    uint16_t count = 0;

    for (const Ip6::Netif::UnicastAddress &address : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (ShouldIncludeLinkLocalAddress(address.GetAddress()))
        {
            count++;
        }
    }
    for (const Ip6::Netif::MulticastAddress &address : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (ShouldIncludeLinkLocalAddress(address.GetAddress()))
        {
            count++;
        }
    }

    if (count * Ip6::Address::kSize <= Tlv::kBaseTlvMaxLength)
    {
        Tlv tlv;

        tlv.SetType(Tlv::kIp6LinkLocalAddressList);
        tlv.SetLength(static_cast<uint8_t>(count * Ip6::Address::kSize));
        SuccessOrExit(error = aMessage.Append(tlv));
    }
    else
    {
        ExtendedTlv tlv;

        tlv.SetType(Tlv::kIp6LinkLocalAddressList);
        tlv.SetLength(count * Ip6::Address::kSize);
        SuccessOrExit(error = aMessage.Append(tlv));
    }

    for (const Ip6::Netif::UnicastAddress &address : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (ShouldIncludeLinkLocalAddress(address.GetAddress()))
        {
            SuccessOrExit(error = aMessage.Append(address.GetAddress()));
        }
    }
    for (const Ip6::Netif::MulticastAddress &address : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (ShouldIncludeLinkLocalAddress(address.GetAddress()))
        {
            SuccessOrExit(error = aMessage.Append(address.GetAddress()));
        }
    }

exit:
    return error;
}

#if OPENTHREAD_FTD
Error Server::AppendChildIp6AddressList(Message &aMessage, const Child &aChild)
{
    Error    error = kErrorNone;
    uint16_t count = 0;

    for (const Ip6::Address &address : aChild.GetIp6Addresses())
    {
        if (ShouldIncludeIp6Address(address))
        {
            count++;
        }
    }

    if (count * Ip6::Address::kSize <= Tlv::kBaseTlvMaxLength)
    {
        Tlv tlv;

        tlv.SetType(Tlv::kIp6AddressList);
        tlv.SetLength(static_cast<uint8_t>(count * Ip6::Address::kSize));
        SuccessOrExit(error = aMessage.Append(tlv));
    }
    else
    {
        ExtendedTlv tlv;

        tlv.SetType(Tlv::kIp6AddressList);
        tlv.SetLength(count * Ip6::Address::kSize);
        SuccessOrExit(error = aMessage.Append(tlv));
    }

    for (const Ip6::Address &address : aChild.GetIp6Addresses())
    {
        if (ShouldIncludeIp6Address(address))
        {
            SuccessOrExit(error = aMessage.Append(address));
        }
    }

exit:
    return error;
}

Error Server::AppendChildAlocList(Message &aMessage, const Child &aChild)
{
    Error    error = kErrorNone;
    uint16_t count = 0;
    uint8_t  aloc;

    for (const Ip6::Address &address : aChild.GetIp6Addresses())
    {
        if (ShouldIncludeAloc(address, aloc))
        {
            count++;
        }
    }

    if (count <= Tlv::kBaseTlvMaxLength)
    {
        Tlv tlv;

        tlv.SetType(Tlv::kAlocList);
        tlv.SetLength(static_cast<uint8_t>(count));
        SuccessOrExit(error = aMessage.Append(tlv));
    }
    else
    {
        ExtendedTlv tlv;

        tlv.SetType(Tlv::kAlocList);
        tlv.SetLength(count);
        SuccessOrExit(error = aMessage.Append(tlv));
    }

    for (const Ip6::Address &address : aChild.GetIp6Addresses())
    {
        if (ShouldIncludeAloc(address, aloc))
        {
            SuccessOrExit(error = aMessage.Append(aloc));
        }
    }

exit:
    return error;
}
#endif // OPENTHREAD_FTD

void Server::HandleUpdateTimer(void)
{
    Error error = kErrorNone;
    VerifyOrExit(mActive);

#if OPENTHREAD_FTD
    if (Get<Mle::Mle>().IsRouterOrLeader())
    {
        if (mSendFullUpdate)
        {
            mSendFullUpdate = false;

            Ip6::Address clientAddr;
            clientAddr.InitAsRoutingLocator(Get<Mle::Mle>().GetMeshLocalPrefix(), mClientRloc);
            error = SendFullServerUpdate(clientAddr);
        }
        else
        {
            error = SendServerUpdate();
        }
    }
    else
#endif // OPENTHREAD_FTD
    {
        error = SendEndDeviceUpdate();
    }

    if (error != kErrorNone)
    {
        ScheduleUpdateTimer(kUpdateBaseDelay);
    }
    else
    {
        mSelfDirty = GetExtDelayTlvs(mSelfEnabled);

#if OPENTHREAD_FTD
        {
            TlvSet childExtDelay = GetExtDelayTlvs(mChildEnabled);

            if (!childExtDelay.IsEmpty())
            {
                for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
                {
                    child.MarkHostProvidedTlvsChanged(childExtDelay);
                }
            }
        }
#endif

        if (!mSelfDirty.IsEmpty()
#if OPENTHREAD_FTD
            || HasExtDelayTlvs(mChildEnabled) || HasExtDelayTlvs(mNeighborEnabled)
#endif
        )
        {
            ScheduleUpdateTimer(Random::NonCrypto::AddJitter(kUpdateExtDelay, kUpdateJitter));
        }
    }

exit:
    return;
}

#if OPENTHREAD_FTD
void Server::UpdateIfCacheBufferLimit(void)
{
    uint16_t total = 0;

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        total += child.GetUsedCacheBuffers();
    }

    if (total > kCacheBuffersLimit)
    {
        ScheduleUpdateTimer(0);
    }
}

void Server::HandleChildTimer(void)
{
    VerifyOrExit(Get<Mle::Mle>().IsRouterOrLeader());

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        SyncChildDiagState(child, false);

        // Potential future enhancement is to only try this when message
        // buffers are available
        if (child.ShouldSendLostDiagQuery())
        {
            IgnoreError(SendEndDeviceRecoveryQuery(child, child.GetLostDiag()));
        }
    }

exit:
    return;
}

void Server::HandleRegistrationTimer(void)
{
    VerifyOrExit(mActive && Get<Mle::Mle>().IsRouterOrLeader());

    // If client didn't register this interval, stop server
    if (!mClientRegistered)
    {
        StopServer();
        ExitNow();
    }

    // Reset registration flag for next interval
    mClientRegistered = false;

    if (mSelfEnabled.IsEmpty() && mChildEnabled.IsEmpty() && mNeighborEnabled.IsEmpty())
    {
        StopServer();
    }
    else
    {
        mRegistrationTimer.Start(kRegistrationInterval);
        SyncAllChildDiagStates(false, false, false);
    }

exit:
    return;
}
#endif // OPENTHREAD_FTD
#endif // OPENTHREAD_CONFIG_MESH_MONITOR_SERVER_ENABLE

} // namespace MeshMonitor

} // namespace ot
