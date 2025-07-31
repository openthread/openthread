/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include "ext_network_diagnostic.hpp"
#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/error.hpp"
#include "common/message.hpp"
#include "common/notifier.hpp"
#include "common/random.hpp"
#include "instance/instance.hpp"
#include "net/ip6_address.hpp"
#include "openthread/ext_network_diagnostic.h"
#include "openthread/ip6.h"
#include "openthread/platform/toolchain.h"
#include "openthread/thread.h"
#include "thread/child.hpp"
#include "thread/child_table.hpp"
#include "thread/ext_network_diagnostic_tlvs.hpp"
#include "thread/ext_network_diagnostic_types.hpp"
#include "thread/mle.hpp"
#include "thread/mle_types.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

RegisterLogModule("ExtNetDiag");

namespace ExtNetworkDiagnostic {

#if OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_SERVER_ENABLE

constexpr otExtNetworkDiagnosticTlvSet Server::kExtDelayTlvMask;
constexpr otExtNetworkDiagnosticTlvSet Server::kStaticNeighborTlvMask;

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

Error Server::ChildInfo::UpdateCache(const Message &aMessage, TlvSet aFilter)
{
    Error    error     = kErrorNone;
    uint16_t srcOffset = aMessage.GetOffset();
    uint16_t dstOffset;

    TlvSet      set;
    OffsetRange srcRange;
    OffsetRange dstRange;
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
        SuccessOrExit(error = aMessage.Read(srcOffset, tlv));

        if (tlv.IsExtended())
        {
            SuccessOrExit(error = aMessage.Read(srcOffset, extTlv));
            srcRange.Init(srcOffset, extTlv.GetLength() + sizeof(ExtendedTlv));
        }
        else
        {
            srcRange.Init(srcOffset, tlv.GetLength() + sizeof(Tlv));
        }

        VerifyOrExit(srcRange.GetEndOffset() <= aMessage.GetLength(), error = kErrorParse);
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

        // We already made sure the tlv is child provided
        if (mDirtySet.ContainsAll(set))
        {
            SuccessOrExit(error = ot::Tlv::FindTlv(*mCache, tlv.GetType(), sizeof(ExtendedTlv), tlv, dstOffset));
            if (tlv.IsExtended())
            {
                dstRange.Init(dstOffset, extTlv.GetLength() + sizeof(ExtendedTlv));
            }
            else
            {
                dstRange.Init(dstOffset, tlv.GetLength() + sizeof(Tlv));
            }

            SuccessOrExit(error = mCache->ResizeRegion(dstOffset, dstRange.GetLength(), srcRange.GetLength()));
            mCache->WriteBytesFromMessage(dstOffset, aMessage, srcRange.GetOffset(), srcRange.GetLength());
        }
        else
        {
            SuccessOrExit(error = mCache->AppendBytesFromMessage(aMessage, srcRange.GetOffset(), srcRange.GetLength()));
        }

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

    // We free the cache to provide extra message buffers for the diag message
    //
    // Since this function must be called within a child update block we temporarily
    // allow invalid state where the mCache and mDirtySet can diverge.
    //
    // This will be reconciled during CommitCacheUpdate() or AbortCacheUpdate().
    // Either by marking everything as clean or updating the lost set.
    mCache.Free();
    mCacheBuffers = 0;

exit:
    return error;
}
#endif // OPENTHREAD_FTD

Server::Server(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mActive(false)
    , mUpdateSent(false)
#if OPENTHREAD_FTD
    , mSendChildBaseline(false)
    , mUpdatePending(false)
    , mChildResumeIndex(0)
    , mClientRloc(0)
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
        SyncChildDiagState(aChild, true);
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

    mRouterStateMask |= (1U << aRouter.GetRouterId());

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

    mRouterStateMask |= (1U << aRouter.GetRouterId());

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

template <>
void Server::HandleTmf<kUriExtDiagnosticEndDeviceRequest>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error          error    = kErrorNone;
    Coap::Message *response = nullptr;

    ChildRequestHeader header;
    TlvSet             set;
    uint16_t           offset;
    bool               changed = false;

    VerifyOrExit(aMessage.IsPostRequest(), error = kErrorInvalidArgs);

    LogInfo("Received %s from %s", UriToString<kUriExtDiagnosticEndDeviceRequest>(),
            aMessageInfo.GetPeerAddr().ToString().AsCString());

    VerifyOrExit(Get<Mle::Mle>().IsChild(), error = kErrorInvalidState);

    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), header));

    offset = aMessage.GetOffset() + sizeof(ChildRequestHeader);
    SuccessOrExit(error = set.ReadFrom(aMessage, offset, header.GetRequestSetCount()));

    response = Get<Tmf::Agent>().NewResponseMessage(aMessage);
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

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*response, aMessageInfo));

exit:
    FreeMessageOnError(response, error);
}

#if OPENTHREAD_FTD
template <>
void Server::HandleTmf<kUriExtDiagnosticEndDeviceUpdate>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Child *child = nullptr;
    TlvSet set;

    VerifyOrExit(aMessageInfo.GetPeerAddr().GetIid().IsRoutingLocator());
    child = Get<ChildTable>().FindChild(aMessageInfo.GetPeerAddr().GetIid().GetLocator(), Child::kInStateValid);

    VerifyOrExit(child != nullptr);
    VerifyOrExit(aMessage.IsPostRequest());

    LogInfo("Received %s from %s", UriToString<kUriExtDiagnosticEndDeviceRequest>(),
            aMessageInfo.GetPeerAddr().ToString().AsCString());

    IgnoreError(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));

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

    if (child->UpdateCache(aMessage, set) != kErrorNone)
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

template <>
void Server::HandleTmf<kUriExtDiagnosticServerRequest>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    RequestHeader  header;
    RequestContext context;
    uint16_t       offset;
    uint16_t       setOffset;
    TlvSet         set;

    TlvSet hostSet;
    TlvSet childSet;
    TlvSet neighborSet;

    VerifyOrExit(aMessage.IsPostRequest());
    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), header));

    LogInfo("Received %s from %s", UriToString<kUriExtDiagnosticServerRequest>(),
            aMessageInfo.GetPeerAddr().ToString().AsCString());

    VerifyOrExit(Get<Mle::Mle>().IsRouterOrLeader());
    VerifyOrExit(aMessageInfo.GetPeerAddr().GetIid().IsRoutingLocator());

    hostSet.Clear();
    childSet.Clear();
    neighborSet.Clear();

    offset = aMessage.GetOffset() + sizeof(RequestHeader);
    while (offset < aMessage.GetLength())
    {
        SuccessOrExit(aMessage.Read(offset, context));

        setOffset = offset + sizeof(RequestContext);
        SuccessOrExit(set.ReadFrom(aMessage, setOffset, context.GetRequestSetCount()));

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

        offset += context.GetLength();
    }

    if (header.GetRegistration())
    {
        mClientRloc = aMessageInfo.GetPeerAddr().GetIid().GetLocator();
    }

    SuccessOrExit(ConfigureAsRouter(hostSet, childSet, neighborSet, header.GetQuery()));

    if (header.GetQuery())
    {
        IgnoreError(SendFullServerUpdate(aMessageInfo.GetPeerAddr()));
    }
    else if (header.GetRegistration())
    {
        IgnoreError(SendEmptyServerUpdate(aMessageInfo.GetPeerAddr()));
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
    Error error = kErrorNone;

    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    VerifyOrExit(Get<Mle::Mle>().IsChild(), error = kErrorInvalidState);
    VerifyOrExit(!mUpdateSent, error = kErrorAlready);

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriExtDiagnosticEndDeviceUpdate);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    messageInfo.SetSockAddrToRlocPeerAddrTo(Get<Mle::Mle>().GetParentRloc16());

    SuccessOrExit(error = AppendHostTlvs(*message, mSelfDirty));
    mSelfPendingUpdate = mSelfDirty;
    mSelfDirty.Clear();

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleEndDeviceUpdateAck, this));
    mUpdateSent = true;

exit:
    if (error != kErrorNone)
    {
        LogCrit("Failed to send child update: %s", ErrorToString(error));
    }
    FreeMessageOnError(message, error);
    return error;
}

void Server::HandleEndDeviceUpdateAck(Coap::Message *aResponse, const Ip6::MessageInfo *aMessageInfo, Error aResult)
{
    OT_UNUSED_VARIABLE(aResponse);
    OT_UNUSED_VARIABLE(aMessageInfo);

    VerifyOrExit(mActive);

    mUpdateSent = false;

    if (aResult != kErrorNone)
    {
        mSelfDirty = mSelfDirty.Join(mSelfPendingUpdate);
    }

exit:
    return;
}

void Server::HandleEndDeviceUpdateAck(void                *aContext,
                                      otMessage           *aResponse,
                                      const otMessageInfo *aMessageInfo,
                                      otError              aResult)
{
    OT_ASSERT(aContext != nullptr);

    Coap::Message          *response    = AsCoapMessagePtr(aResponse);
    const Ip6::MessageInfo *messageInfo = AsCoreTypePtr(aMessageInfo);

    static_cast<Server *>(aContext)->HandleEndDeviceUpdateAck(response, messageInfo, aResult);
}

#if OPENTHREAD_FTD
Error Server::SendFullServerUpdate(const Ip6::Address &aClientAddr)
{
    Error error = kErrorNone;

    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    UpdateHeader header;

    // Don't send if we have a pending update awaiting ACK
    VerifyOrExit(!mUpdatePending, error = kErrorBusy);

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriExtDiagnosticServerUpdate);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    messageInfo.SetSockAddrToRlocPeerAddrTo(aClientAddr);

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
        Router *router;

        for (uint8_t id = 0; id < Mle::kMaxRouterId; id++)
        {
            router = Get<RouterTable>().FindRouterById(id);

            if (router == nullptr || !router->IsStateValid())
            {
                continue;
            }

            SuccessOrExit(error = AppendNeighborContextBaseline(*message, *router));
        }
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleServerUpdateAck, this));
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

    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    UpdateHeader header;

    // Don't send if we have a pending update awaiting ACK
    VerifyOrExit(!mUpdatePending, error = kErrorBusy);

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriExtDiagnosticServerUpdate);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    messageInfo.SetSockAddrToRlocPeerAddrTo(aClientAddr);

    header.Init();
    header.SetRouterId(Mle::RouterIdFromRloc16(Get<Mle::Mle>().GetRloc16()));
    header.SetFullSeqNumber(mSequenceNumber + 1);
    SuccessOrExit(error = header.AppendTo(*message));

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleServerUpdateAck, this));
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

    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    UpdateHeader header;
    TlvSet       hostSet;
    Context      hostContext;
    uint16_t     offset;
    uint16_t     startIndex         = mChildResumeIndex;
    bool         needsAnotherUpdate = false;

    // Don't send if we have a pending update awaiting ACK
    VerifyOrExit(!mUpdatePending, error = kErrorBusy);

    LockChildCaches();

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriExtDiagnosticServerUpdate);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    messageInfo.SetSockAddrToRlocPeerAddrTo(mClientRloc);

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

    // Neighbor updates only in final batch
    if (!mNeighborEnabled.IsEmpty() && !needsAnotherUpdate && mChildResumeIndex == 0)
    {
        VerifyOrExit(AppendNeighborContextBatch(*message), error = kErrorFailed);
    }

    SuccessOrExit(Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleServerUpdateAck, this));

    mUpdatePending = true;

    if (startIndex == 0)
    {
        mSelfDirty.Clear();
    }

exit:
    if (error == kErrorNone)
    {
        CommitChildCacheUpdates();

        if (!mUpdatePending && (needsAnotherUpdate || mSendChildBaseline))
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
        VerifyOrExit(!aSelf.IsEmpty() || !aChild.IsEmpty(), error = kErrorInvalidArgs);

        mSequenceNumber = Random::NonCrypto::GetUint32();
        mSequenceNumber |= static_cast<uint64_t>(Random::NonCrypto::GetUint32()) << 32;

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
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    ChildRequestHeader header;

    if (aChild.IsDiagServerPending())
    {
        IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleEndDeviceRequestAck, &aChild));
    }

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriExtDiagnosticEndDeviceRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    messageInfo.SetSockAddrToRlocPeerAddrTo(aChild.GetRloc16());

    header.Clear();
    header.SetCommand(ChildRequestHeader::kStop);

    SuccessOrExit(error = message->Append(header));

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleEndDeviceRequestAck, &aChild));
    aChild.SetDiagServerState(Child::kDiagServerStopPending);

    LogInfo("Sent DiagServer stop to child %04x", aChild.GetRloc16());

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Server::SendEndDeviceRequestStart(Child &aChild, const TlvSet &aTypes, bool aQuery)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    ChildRequestHeader header;
    uint16_t           offset;
    uint8_t            setCount = 0;

    if (aChild.IsDiagServerPending())
    {
        IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleEndDeviceRequestAck, &aChild));
    }

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriExtDiagnosticEndDeviceRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    messageInfo.SetSockAddrToRlocPeerAddrTo(aChild.GetRloc16());

    offset = message->GetLength();

    header.Clear();
    header.SetCommand(ChildRequestHeader::kStart);
    header.SetQuery(aQuery);

    SuccessOrExit(error = message->Append(header));
    SuccessOrExit(error = aTypes.AppendTo(*message, setCount));

    header.SetRequestSetCount(setCount);
    message->Write(offset, header);

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleEndDeviceRequestAck, &aChild));
    aChild.SetDiagServerState(Child::kDiagServerActivePending);

    LogInfo("Sent DiagServer start to child %04x", aChild.GetRloc16());

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Server::SendEndDeviceRecoveryQuery(Child &aChild, const TlvSet &aTypes)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    ChildRequestHeader header;
    uint16_t           offset;
    uint8_t            setCount = 0;

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriExtDiagnosticEndDeviceRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    messageInfo.SetSockAddrToRlocPeerAddrTo(aChild.GetRloc16());

    offset = message->GetLength();

    header.Clear();
    header.SetCommand(ChildRequestHeader::kStart);
    header.SetQuery(true);

    SuccessOrExit(error = message->Append(header));
    SuccessOrExit(error = aTypes.AppendTo(*message, setCount));

    header.SetRequestSetCount(setCount);
    message->Write(offset, header);

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleEndDeviceRecoveryAck, &aChild));
    aChild.SetLostDiagQueryPending(true);

    LogInfo("Sent DiagServer lost query to child %04x", aChild.GetRloc16());

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Server::HandleEndDeviceRequestAck(Child                  &aChild,
                                       Coap::Message          *aResponse,
                                       const Ip6::MessageInfo *aMessageInfo,
                                       Error                   aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

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

        VerifyOrExit(aResponse != nullptr);
        if (aChild.UpdateCache(*aResponse, mChildEnabled) == kErrorNone)
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

void Server::HandleEndDeviceRequestAck(void                *aContext,
                                       otMessage           *aResponse,
                                       const otMessageInfo *aMessageInfo,
                                       Error                aResult)
{
    OT_ASSERT(aContext != nullptr);

    Child                  *child       = static_cast<Child *>(aContext);
    Coap::Message          *response    = AsCoapMessagePtr(aResponse);
    const Ip6::MessageInfo *messageInfo = AsCoreTypePtr(aMessageInfo);

    child->GetInstance().Get<Server>().HandleEndDeviceRequestAck(*child, response, messageInfo, aResult);
}

void Server::HandleEndDeviceRecoveryAck(Child                  &aChild,
                                        Coap::Message          *aResponse,
                                        const Ip6::MessageInfo *aMessageInfo,
                                        Error                   aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    aChild.SetLostDiagQueryPending(false);

    if (aResult == kErrorNone)
    {
        OT_ASSERT(aResponse != nullptr);
        if (aChild.UpdateCache(*aResponse, aChild.GetLostDiag()) != kErrorNone)
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

void Server::HandleEndDeviceRecoveryAck(void                *aContext,
                                        otMessage           *aResponse,
                                        const otMessageInfo *aMessageInfo,
                                        Error                aResult)
{
    OT_ASSERT(aContext != nullptr);

    Child                  *child       = static_cast<Child *>(aContext);
    Coap::Message          *response    = AsCoapMessagePtr(aResponse);
    const Ip6::MessageInfo *messageInfo = AsCoreTypePtr(aMessageInfo);

    child->GetInstance().Get<Server>().HandleEndDeviceRecoveryAck(*child, response, messageInfo, aResult);
}

void Server::HandleServerUpdateAck(Coap::Message *aResponse, const Ip6::MessageInfo *aMessageInfo, Error aResult)
{
    OT_UNUSED_VARIABLE(aResponse);
    OT_UNUSED_VARIABLE(aMessageInfo);

    mUpdatePending = false;

    if (aResult == kErrorNone)
    {
        mSequenceNumber++;
        mUpdateRetryCount = 0; // Reset backoff counter on success

        // If more batches pending, schedule next send
        if (mSendChildBaseline || mChildResumeIndex != 0)
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
            mChildResumeIndex  = 0;
            mSendChildBaseline = false;
        }
        else
        {
            uint32_t backoffDelay = kUpdateBaseDelay << (mUpdateRetryCount - 1);
            backoffDelay          = Min(backoffDelay, kMaxUpdateBackoff);

            ScheduleUpdateTimer(backoffDelay);
        }
    }
}

void Server::HandleServerUpdateAck(void                *aContext,
                                   otMessage           *aResponse,
                                   const otMessageInfo *aMessageInfo,
                                   otError              aResult)
{
    OT_ASSERT(aContext != nullptr);

    Server                 *server      = static_cast<Server *>(aContext);
    Coap::Message          *response    = AsCoapMessagePtr(aResponse);
    const Ip6::MessageInfo *messageInfo = AsCoreTypePtr(aMessageInfo);

    server->HandleServerUpdateAck(response, messageInfo, static_cast<Error>(aResult));
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
        case Tlv::kMacAddress:
            SuccessOrExit(error = Tlv::Append<ExtMacAddressTlv>(aMessage, Get<Mac::Mac>().GetExtAddress()));
            break;

        case Tlv::kMode:
            SuccessOrExit(error = Tlv::Append<ModeTlv>(aMessage, Get<Mle::Mle>().GetDeviceMode().Get()));
            break;

        case Tlv::kRoute64:
        {
            Route64Tlv tlv;

            tlv.Init();
            Get<RouterTable>().FillRouteTlv(tlv);
            SuccessOrExit(error = tlv.AppendTo(aMessage));
            break;
        }
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
            SuccessOrExit(error =
                              Tlv::Append<VendorNameTlv>(aMessage, Get<NetworkDiagnostic::Server>().GetVendorName()));
            break;

        case Tlv::kVendorModel:
            SuccessOrExit(error =
                              Tlv::Append<VendorModelTlv>(aMessage, Get<NetworkDiagnostic::Server>().GetVendorModel()));
            break;

        case Tlv::kVendorSwVersion:
            SuccessOrExit(error = Tlv::Append<VendorSwVersionTlv>(
                              aMessage, Get<NetworkDiagnostic::Server>().GetVendorSwVersion()));
            break;

        case Tlv::kVendorAppUrl:
            SuccessOrExit(
                error = Tlv::Append<VendorAppUrlTlv>(aMessage, Get<NetworkDiagnostic::Server>().GetVendorAppUrl()));
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
            MacCountersTlv       tlv;
            const otMacCounters &counters = Get<Mac::Mac>().GetCounters();

            tlv.Init();
            tlv.SetIfInUnknownProtos(counters.mRxOther);
            tlv.SetIfInErrors(counters.mRxErrNoFrame + counters.mRxErrUnknownNeighbor + counters.mRxErrInvalidSrcAddr +
                              counters.mRxErrSec + counters.mRxErrFcs + counters.mRxErrOther);
            tlv.SetIfOutErrors(counters.mTxErrCca);
            tlv.SetIfInUcastPkts(counters.mRxUnicast);
            tlv.SetIfInBroadcastPkts(counters.mRxBroadcast);
            tlv.SetIfInDiscards(counters.mRxAddressFiltered + counters.mRxDestAddrFiltered + counters.mRxDuplicated);
            tlv.SetIfOutUcastPkts(counters.mTxUnicast);
            tlv.SetIfOutBroadcastPkts(counters.mTxBroadcast);
            tlv.SetIfOutDiscards(counters.mTxErrBusyChannel);

            SuccessOrExit(tlv.AppendTo(aMessage));
            break;
        }

        case Tlv::kMacLinkErrorRatesIn:
        {
            MacLinkErrorRatesInTlv tlv;
            Parent                &parent = Get<Mle::Mle>().GetParent();
            tlv.Init();

            if (!Get<Mle::Mle>().IsChild())
            {
                break;
            }

            tlv.SetMessageErrorRates(parent.GetLinkInfo().GetMessageErrorRate());
            tlv.SetFrameErrorRates(parent.GetLinkInfo().GetFrameErrorRate());

            SuccessOrExit(error = aMessage.Append(tlv));
            break;
        }

        case Tlv::kMleCounters:
        {
            MleCountersTlv tlv;

            tlv.Init(Get<Mle::Mle>().GetCounters());
            SuccessOrExit(error = tlv.AppendTo(aMessage));
            break;
        }

        case Tlv::kLinkMarginOut:
        {
            LinkMarginOutTlv tlv;
            Parent          &parent = Get<Mle::Mle>().GetParent();
            tlv.Init();

            if (!Get<Mle::Mle>().IsChild())
            {
                break;
            }

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

    context.SetUpdateMode(kModeUpdate);

    if (aChild.IsAttachStateDirty())
    {
        if (aChild.IsStateValid())
        {
            context.SetUpdateMode(kModeAdded);
            allTlvs = true;
        }
        else
        {
            context.SetUpdateMode(kModeRemove);
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
        case Tlv::kMacAddress:
            SuccessOrExit(error = Tlv::Append<ExtMacAddressTlv>(aMessage, aChild.GetExtAddress()));
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

        case Tlv::kMacLinkErrorRatesOut:
        {
            MacLinkErrorRatesOutTlv tlv;
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

    VerifyOrExit(valid || (mRouterStateMask & (1U << aId)) != 0);

    context.Init();
    context.SetType(kTypeNeighbor);
    context.SetId(aId);
    if (valid)
    {
        if (mRouterStateMask & (1U << aId))
        {
            context.SetUpdateMode(kModeAdded);
        }
        else
        {
            context.SetUpdateMode(kModeUpdate);

            tlvs.ClearAll(static_cast<const TlvSet &>(kStaticNeighborTlvMask));
            VerifyOrExit(!tlvs.IsEmpty());
        }
    }
    else
    {
        context.SetUpdateMode(kModeRemove);
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
        case Tlv::kMacAddress:
            SuccessOrExit(error = Tlv::Append<ExtMacAddressTlv>(aMessage, aNeighbor.GetExtAddress()));
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

        case Tlv::kMacLinkErrorRatesOut:
        {
            MacLinkErrorRatesOutTlv tlv;
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
    bool               success    = true;
    uint16_t           childIndex = 0;
    uint16_t           startIndex = mChildResumeIndex;
    Child::StateFilter filter     = mSendChildBaseline ? Child::kInStateValid : Child::kInStateAny;

    aNeedsMore = false;

    for (Child &child : Get<ChildTable>().Iterate(filter))
    {
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
            break;
        }

        if (error != kErrorNone)
        {
            success = false;
            break;
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

bool Server::AppendNeighborContextBatch(Message &aMessage)
{
    bool success = true;

    for (uint8_t id = 0; id < Mle::kMaxRouterId; id++)
    {
        uint16_t beforeLen = aMessage.GetLength();
        Error    error     = AppendNeighborContextUpdate(aMessage, id);

        if (error == kErrorNoBufs || aMessage.GetLength() > kMaxUpdateMessageLength)
        {
            IgnoreError(aMessage.SetLength(beforeLen));
            break;
        }

        if (error != kErrorNone)
        {
            success = false;
            break;
        }
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
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        child.AbortCacheUpdate();
    }
}
#endif // OPENTHREAD_FTD

bool Server::ShouldIncludeIp6Address(const Ip6::Address &aAddress)
{
    bool pass = false;

    VerifyOrExit(!Get<Mle::Mle>().IsMeshLocalAddress(aAddress));
    VerifyOrExit(!aAddress.IsLinkLocalUnicastOrMulticast());
    VerifyOrExit(!aAddress.IsRealmLocalAllNodesMulticast());
    VerifyOrExit(!aAddress.IsRealmLocalAllRoutersMulticast());
    VerifyOrExit(!aAddress.IsRealmLocalAllMplForwarders());
    VerifyOrExit(!aAddress.GetIid().IsAnycastLocator());

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
    VerifyOrExit(!aAddress.IsLinkLocalAllNodesMulticast());
    VerifyOrExit(!aAddress.IsLinkLocalAllRoutersMulticast());

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
        error = SendServerUpdate();
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
    TlvSet mtd = mChildEnabled.GetMtdChildProvided();
    TlvSet ftd = mChildEnabled.GetFtdChildProvided();

    VerifyOrExit(mActive && Get<Mle::Mle>().IsRouterOrLeader());

    // If client didn't register this interval, stop server
    if (!mClientRegistered)
    {
        StopServer();
        ExitNow();
    }

    // Reset registration flag for next interval
    mClientRegistered = false;

    if (mSelfEnabled.IsEmpty() && mChildEnabled.IsEmpty())
    {
        StopServer();
    }
    else
    {
        mRegistrationTimer.Start(kRegistrationInterval);
        SyncAllChildDiagStates(mtd != mChildEnabled.GetMtdChildProvided(), ftd != mChildEnabled.GetFtdChildProvided(),
                               false);
    }

exit:
    return;
}
#endif // OPENTHREAD_FTD
#endif // OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_SERVER_ENABLE

#if OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE
Client::Client(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer{aInstance}
{
}

void Client::Start(const TlvSet                              *aHost,
                   const TlvSet                              *aChild,
                   const TlvSet                              *aNeighbor,
                   otExtNetworkDiagnosticServerUpdateCallback aCallback,
                   void                                      *aContext)
{
    mActive          = true;
    mCallback        = aCallback;
    mCallbackContext = aContext;

    mHostSet.Clear();
    mChildSet.Clear();
    mNeighborSet.Clear();

    if (aHost != nullptr)
    {
        mHostSet = *aHost;
        mHostSet.FilterHostSupportedTlv();
    }

    if (aChild != nullptr)
    {
        mChildSet = *aChild;
        mChildSet.FilterChildSupportedTlv();
    }

    if (aNeighbor != nullptr)
    {
        mNeighborSet = *aNeighbor;
        mNeighborSet.FilterNeighborSupportedTlv();
    }

    mQueryPending = true;

    if (SendServerRequest(true) == kErrorNone)
    {
        ScheduleNextRegistration();
    }
    else
    {
        ScheduleRegistrationRetry();
        mQueryPending = true;
    }
}

void Client::Stop(void)
{
    mActive = false;

    mCallback        = nullptr;
    mCallbackContext = nullptr;

    mTimer.Stop();
}

Error Client::GetNextContext(const Coap::Message            &aMessage,
                             otExtNetworkDiagnosticIterator &aIterator,
                             otExtNetworkDiagnosticContext  &aContext)
{
    Error    error = kErrorNone;
    uint16_t offset;
    union
    {
        Context         context;
        ChildContext    childContext;
        NeighborContext neighborContext;
    };

    UpdateHeader header;
    SuccessOrExit(error = header.ReadFrom(aMessage));

    if (aIterator == OT_EXT_NETWORK_DIAGNOSTIC_ITERATOR_INIT)
    {
        aIterator = aMessage.GetOffset() + header.GetLength();
    }

    while (aIterator < aMessage.GetLength())
    {
        SuccessOrExit(error = aMessage.Read(aIterator, context));
        offset = aIterator;
        aIterator += context.GetLength();

        aContext.mTlvIteratorEnd = aIterator;

        switch (context.GetType())
        {
        case kTypeHost:
            aContext.mType        = OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_HOST;
            aContext.mRloc16      = Mle::Rloc16FromRouterId(header.GetRouterId());
            aContext.mTlvIterator = offset + sizeof(Context);
            ExitNow();

        case kTypeChild:
            SuccessOrExit(error = aMessage.Read(offset, childContext));

            aContext.mType        = OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_CHILD;
            aContext.mRloc16      = Mle::Rloc16FromRouterId(header.GetRouterId()) | childContext.GetId();
            aContext.mTlvIterator = offset + sizeof(ChildContext);
            aContext.mLegacy      = childContext.GetLegacy();
            aContext.mUpdateMode  = UpdateModeToApiValue(childContext.GetUpdateMode());
            ExitNow();

        case kTypeNeighbor:
            SuccessOrExit(error = aMessage.Read(offset, neighborContext));

            aContext.mType        = OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_NEIGHBOR;
            aContext.mRloc16      = Mle::Rloc16FromRouterId(neighborContext.GetId());
            aContext.mTlvIterator = offset + sizeof(NeighborContext);
            aContext.mUpdateMode  = UpdateModeToApiValue(neighborContext.GetUpdateMode());
            ExitNow();

        default:
            break;
        }
    }

    error = kErrorNotFound;

exit:
    return error;
}

Error Client::GetNextTlv(const Coap::Message           &aMessage,
                         otExtNetworkDiagnosticContext &aContext,
                         otExtNetworkDiagnosticTlv     &aTlv)
{
    Error       error = kErrorNone;
    uint16_t    offset;
    OffsetRange value;
    union
    {
        Tlv         tlv;
        ExtendedTlv extTlv;
    };

    VerifyOrExit(aContext.mTlvIterator < aContext.mTlvIteratorEnd, error = kErrorNotFound);

    while (aContext.mTlvIterator < aContext.mTlvIteratorEnd)
    {
        offset = aContext.mTlvIterator;
        SuccessOrExit(error = aMessage.Read(offset, tlv));

        if (tlv.IsExtended())
        {
            SuccessOrExit(error = aMessage.Read(offset, extTlv));
            value.Init(offset + sizeof(ExtendedTlv), extTlv.GetLength());
            aContext.mTlvIterator += extTlv.GetSize();
        }
        else
        {
            value.Init(offset + sizeof(Tlv), tlv.GetLength());
            aContext.mTlvIterator += tlv.GetSize();
        }

        VerifyOrExit(aContext.mTlvIterator <= aContext.mTlvIteratorEnd, error = kErrorParse);

        aTlv.mType = tlv.GetType();
        switch (tlv.GetType())
        {
        case Tlv::kMacAddress:
            SuccessOrExit(error = Tlv::Read<ExtMacAddressTlv>(aMessage, offset, AsCoreType(&aTlv.mData.mExtAddress)));
            ExitNow();

        case Tlv::kEui64:
            SuccessOrExit(error = Tlv::Read<Eui64Tlv>(aMessage, offset, AsCoreType(&aTlv.mData.mEui64)));
            ExitNow();

        case Tlv::kMode:
        {
            uint8_t mode;

            SuccessOrExit(error = Tlv::Read<ModeTlv>(aMessage, offset, mode));
            Mle::DeviceMode(mode).Get(aTlv.mData.mMode);
            ExitNow();
        }

        case Tlv::kTimeout:
            SuccessOrExit(error = Tlv::Read<TimeoutTlv>(aMessage, offset, aTlv.mData.mTimeout));
            ExitNow();

        case Tlv::kLastHeard:
            SuccessOrExit(error = Tlv::Read<LastHeardTlv>(aMessage, offset, aTlv.mData.mLastHeard));
            ExitNow();

        case Tlv::kConnectionTime:
            SuccessOrExit(error = Tlv::Read<ConnectionTimeTlv>(aMessage, offset, aTlv.mData.mConnectionTime));
            ExitNow();

        case Tlv::kCsl:
        {
            CslTlv cslTlv;
            SuccessOrExit(error = aMessage.Read(offset, cslTlv));

            aTlv.mData.mCsl.mTimeout = cslTlv.GetTimeout();
            aTlv.mData.mCsl.mPeriod  = cslTlv.GetPeriod();
            aTlv.mData.mCsl.mChannel = cslTlv.GetChannel();
            ExitNow();
        }

        case Tlv::kMlEid:
            SuccessOrExit(error = Tlv::Read<MlEidTlv>(aMessage, offset, AsCoreType(&aTlv.mData.mMlEid)));
            ExitNow();

        case Tlv::kIp6AddressList:
        {
            uint16_t count = value.GetLength() / sizeof(otIp6Address);
            VerifyOrExit(count * sizeof(otIp6Address) == value.GetLength(), error = kErrorParse);

            aTlv.mData.mIp6AddressList.mCount      = count;
            aTlv.mData.mIp6AddressList.mDataOffset = value.GetOffset();
            ExitNow();
        }

        case Tlv::kIp6LinkLocalAddressList:
        {
            uint16_t count = value.GetLength() / sizeof(otIp6Address);
            VerifyOrExit(count * sizeof(otIp6Address) == value.GetLength(), error = kErrorParse);

            aTlv.mData.mIp6LinkLocalAddressList.mCount      = count;
            aTlv.mData.mIp6LinkLocalAddressList.mDataOffset = value.GetOffset();
            ExitNow();
        }

        case Tlv::kAlocList:
            aTlv.mData.mAlocList.mCount      = value.GetLength();
            aTlv.mData.mAlocList.mDataOffset = value.GetOffset();
            ExitNow();

        case Tlv::kThreadSpecVersion:
            SuccessOrExit(error = Tlv::Read<Mle::VersionTlv>(aMessage, offset, aTlv.mData.mThreadSpecVersion));
            ExitNow();

        case Tlv::kThreadStackVersion:
            SuccessOrExit(error = Tlv::Read<ThreadStackVersionTlv>(aMessage, offset, aTlv.mData.mThreadStackVersion));
            ExitNow();

        case Tlv::kVendorName:
            SuccessOrExit(error = Tlv::Read<VendorNameTlv>(aMessage, offset, aTlv.mData.mVendorName));
            ExitNow();

        case Tlv::kVendorModel:
            SuccessOrExit(error = Tlv::Read<VendorModelTlv>(aMessage, offset, aTlv.mData.mVendorModel));
            ExitNow();

        case Tlv::kVendorSwVersion:
            SuccessOrExit(error = Tlv::Read<VendorSwVersionTlv>(aMessage, offset, aTlv.mData.mVendorSwVersion));
            ExitNow();

        case Tlv::kVendorAppUrl:
            SuccessOrExit(error = Tlv::Read<VendorAppUrlTlv>(aMessage, offset, aTlv.mData.mVendorAppUrl));
            ExitNow();

        case Tlv::kLinkMarginIn:
        {
            LinkMarginInTlv data;
            SuccessOrExit(error = aMessage.Read(offset, data));

            aTlv.mData.mLinkMarginIn.mLinkMargin  = data.GetLinkMargin();
            aTlv.mData.mLinkMarginIn.mAverageRssi = data.GetAverageRssi();
            aTlv.mData.mLinkMarginIn.mLastRssi    = data.GetLastRssi();
            ExitNow();
        }

        case Tlv::kMacLinkErrorRatesOut:
        {
            MacLinkErrorRatesOutTlv data;
            SuccessOrExit(error = aMessage.Read(offset, data));

            aTlv.mData.mMacLinkErrorRatesOut.mMessageErrorRate = data.GetMessageErrorRates();
            aTlv.mData.mMacLinkErrorRatesOut.mFrameErrorRate   = data.GetFrameErrorRates();
            ExitNow();
        }

        case Tlv::kMacCounters:
        {
            MacCountersTlv data;
            SuccessOrExit(error = aMessage.Read(offset, data));

            aTlv.mData.mMacCounters.mIfInUnknownProtos  = data.GetIfInUnknownProtos();
            aTlv.mData.mMacCounters.mIfInErrors         = data.GetIfInErrors();
            aTlv.mData.mMacCounters.mIfOutErrors        = data.GetIfOutErrors();
            aTlv.mData.mMacCounters.mIfInUcastPkts      = data.GetIfInUcastPkts();
            aTlv.mData.mMacCounters.mIfInBroadcastPkts  = data.GetIfInBroadcastPkts();
            aTlv.mData.mMacCounters.mIfInDiscards       = data.GetIfInDiscards();
            aTlv.mData.mMacCounters.mIfOutUcastPkts     = data.GetIfOutUcastPkts();
            aTlv.mData.mMacCounters.mIfOutBroadcastPkts = data.GetIfOutBroadcastPkts();
            aTlv.mData.mMacCounters.mIfOutDiscards      = data.GetIfOutDiscards();
            ExitNow();
        }

        case Tlv::kMacLinkErrorRatesIn:
        {
            MacLinkErrorRatesInTlv data;
            SuccessOrExit(error = aMessage.Read(offset, data));

            aTlv.mData.mMacLinkErrorRatesIn.mMessageErrorRate = data.GetMessageErrorRates();
            aTlv.mData.mMacLinkErrorRatesIn.mFrameErrorRate   = data.GetFrameErrorRates();
            ExitNow();
        }

        case Tlv::kMleCounters:
        {
            MleCountersTlv data;
            SuccessOrExit(error = aMessage.Read(offset, data));

            aTlv.mData.mMleCounters.mDisabledRole                  = data.GetDisabledRole();
            aTlv.mData.mMleCounters.mDetachedRole                  = data.GetDetachedRole();
            aTlv.mData.mMleCounters.mChildRole                     = data.GetChildRole();
            aTlv.mData.mMleCounters.mRouterRole                    = data.GetRouterRole();
            aTlv.mData.mMleCounters.mLeaderRole                    = data.GetLeaderRole();
            aTlv.mData.mMleCounters.mAttachAttempts                = data.GetAttachAttempts();
            aTlv.mData.mMleCounters.mPartitionIdChanges            = data.GetPartitionIdChanges();
            aTlv.mData.mMleCounters.mBetterPartitionAttachAttempts = data.GetBetterPartitionAttachAttempts();
            aTlv.mData.mMleCounters.mParentChanges                 = data.GetParentChanges();
            aTlv.mData.mMleCounters.mTrackedTime                   = data.GetTrackedTime();
            aTlv.mData.mMleCounters.mDisabledTime                  = data.GetDisabledTime();
            aTlv.mData.mMleCounters.mDetachedTime                  = data.GetDetachedTime();
            aTlv.mData.mMleCounters.mChildTime                     = data.GetChildTime();
            aTlv.mData.mMleCounters.mRouterTime                    = data.GetRouterTime();
            aTlv.mData.mMleCounters.mLeaderTime                    = data.GetLeaderTime();
            ExitNow();
        }

        case Tlv::kLinkMarginOut:
        {
            LinkMarginOutTlv data;
            SuccessOrExit(error = aMessage.Read(offset, data));

            aTlv.mData.mLinkMarginOut.mLinkMargin  = data.GetLinkMargin();
            aTlv.mData.mLinkMarginOut.mAverageRssi = data.GetAverageRssi();
            aTlv.mData.mLinkMarginOut.mLastRssi    = data.GetLastRssi();
            ExitNow();
        }

        default:
            break;
        }
    }

    error = kErrorNotFound;

exit:
    return error;
}

Error Client::GetIp6Addresses(const Coap::Message &aMessage,
                              uint16_t             aDataOffset,
                              uint16_t             aCount,
                              otIp6Address        *aAddresses)
{
    Error error = kErrorNone;

    VerifyOrExit(aCount != 0);
    VerifyOrExit(aAddresses != nullptr, error = kErrorInvalidArgs);

    for (uint16_t i = 0; i < aCount; i++)
    {
        SuccessOrExit(error = aMessage.Read(aDataOffset + (i * sizeof(otIp6Address)), aAddresses[i]));
    }

exit:
    return error;
}

Error Client::GetAlocs(const Coap::Message &aMessage, uint16_t aDataOffset, uint16_t aCount, uint8_t *aAlocs)
{
    Error error = kErrorNone;

    VerifyOrExit(aCount != 0);
    VerifyOrExit(aAlocs != nullptr, error = kErrorInvalidArgs);

    VerifyOrExit(aMessage.ReadBytes(aDataOffset, aAlocs, aCount) == aCount, error = kErrorParse);

exit:
    return error;
}

Error Client::SendServerRequest(bool aQuery)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    messageInfo.SetSockAddrToRlocPeerAddrToRealmLocalAllRoutersMulticast();
    messageInfo.SetMulticastLoop(true);

    message = Get<Tmf::Agent>().NewNonConfirmablePostMessage(kUriExtDiagnosticServerRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = AppendServerRequestPayload(*message, aQuery, true));
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, nullptr, nullptr));

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Client::SendServerRequestForRecovery(uint16_t aRloc16)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    messageInfo.SetSockAddrToRlocPeerAddrTo(aRloc16);

    message = Get<Tmf::Agent>().NewNonConfirmablePostMessage(kUriExtDiagnosticServerRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = AppendServerRequestPayload(*message, true, false));
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, nullptr, nullptr));

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Client::AppendServerRequestPayload(Message &aMessage, bool aQuery, bool aIncludeNeighbors)
{
    Error         error = kErrorNone;
    RequestHeader header;

    header.Clear();
    header.SetQuery(aQuery);
    header.SetRegistration(true);
    SuccessOrExit(error = aMessage.Append(header));

    if (!mHostSet.IsEmpty())
    {
        SuccessOrExit(error = AppendRequestContext(aMessage, kTypeHost, mHostSet));
    }

    if (!mChildSet.IsEmpty())
    {
        SuccessOrExit(error = AppendRequestContext(aMessage, kTypeChild, mChildSet));
    }

    if (aIncludeNeighbors && !mNeighborSet.IsEmpty())
    {
        SuccessOrExit(error = AppendRequestContext(aMessage, kTypeNeighbor, mNeighborSet));
    }

exit:
    return error;
}

Error Client::AppendRequestContext(Message &aMessage, DeviceType aType, const TlvSet &aSet)
{
    Error          error = kErrorNone;
    RequestContext header;
    uint16_t       offset;
    uint8_t        setCount = 0;

    header.Clear();
    offset = aMessage.GetLength();
    SuccessOrExit(error = aMessage.Append(header));
    SuccessOrExit(error = aSet.AppendTo(aMessage, setCount));

    header.SetType(aType);
    header.SetRequestSetCount(setCount);
    header.SetLength(aMessage.GetLength() - offset);
    aMessage.Write(offset, header);

exit:
    return error;
}

template <>
void Client::HandleTmf<kUriExtDiagnosticServerUpdate>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error error = kErrorNone;

    VerifyOrExit(mActive);

    ProcessServerUpdate(aMessage, aMessageInfo);

    if (aMessage.IsConfirmable())
    {
        error = Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo);
    }

    if (error != kErrorNone)
    {
        LogCrit("Failed to parse response: %s", ErrorToString(error));
    }

exit:
    return;
}

void Client::ProcessServerUpdate(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    UpdateHeader header;
    bool         sequenceError = false;

    OT_UNUSED_VARIABLE(aMessageInfo);

    SuccessOrExit(header.ReadFrom(aMessage));
    VerifyOrExit(header.GetRouterId() <= Mle::kMaxRouterId);

    // Clear query pending when we receive ANY update from ANY router
    // This proves at least one router received our registration
    mQueryPending = false;

    if (header.GetComplete())
    {
        mServerSeqNumbers[header.GetRouterId()] = header.GetFullSeqNumber();
    }
    else
    {
        uint64_t next = mServerSeqNumbers[header.GetRouterId()] + 1;

        if (header.HasFullSeqNumber())
        {
            sequenceError = next != header.GetFullSeqNumber();
        }
        else
        {
            sequenceError = static_cast<uint8_t>(next) != header.GetShortSeqNumber();
        }

        if (sequenceError)
        {
            LogCrit("Sequence error occurred!");
            IgnoreError(SendServerRequestForRecovery(Mle::Rloc16FromRouterId(header.GetRouterId())));
            ExitNow();
        }

        mServerSeqNumbers[header.GetRouterId()] = next;
    }

    if (mCallback != nullptr && (aMessage.GetOffset() + header.GetLength()) < aMessage.GetLength())
    {
        (mCallback)(&aMessage, Mle::Rloc16FromRouterId(header.GetRouterId()), header.GetComplete(), mCallbackContext);
    }

exit:
    return;
}

void Client::ScheduleNextRegistration(void)
{
    mTimer.Start(Random::NonCrypto::AddJitter(kRegistrationInterval, kRegistrationJitter));
}

void Client::ScheduleRegistrationRetry(void)
{
    mTimer.Start(Random::NonCrypto::GetUint32InRange(0, kRegistrationJitter / 5));
}

void Client::HandleRegistrationTimer(void)
{
    VerifyOrExit(mActive);

    if (SendServerRequest(mQueryPending) == kErrorNone)
    {
        ScheduleNextRegistration();
    }
    else
    {
        ScheduleRegistrationRetry();
    }

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE

} // namespace ExtNetworkDiagnostic

} // namespace ot
