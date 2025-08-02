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

#include "diagnostic_server.hpp"
#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/error.hpp"
#include "common/message.hpp"
#include "common/notifier.hpp"
#include "common/random.hpp"
#include "instance/instance.hpp"
#include "net/ip6_address.hpp"
#include "openthread/diag_server.h"
#include "openthread/ip6.h"
#include "openthread/platform/toolchain.h"
#include "openthread/thread.h"
#include "thread/child.hpp"
#include "thread/child_table.hpp"
#include "thread/diagnostic_server_tlvs.hpp"
#include "thread/diagnostic_server_types.hpp"
#include "thread/mle.hpp"
#include "thread/mle_types.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

RegisterLogModule("DiagServer");

namespace DiagnosticServer {

#if OPENTHREAD_CONFIG_DIAG_SERVER_ENABLE

constexpr otDiagServerTlvSet Server::kExtDelayTlvs;
constexpr otDiagServerTlvSet Server::kNeighborStaticTlvs;

#if OPENTHREAD_FTD
void Server::ChildInfo::MarkDiagsDirty(TlvSet aTlvs)
{
    if (mDiagFtd)
    {
        mDirtySet.SetAll(aTlvs.GetNotChildProvidedFtd());
    }
    else
    {
        mDirtySet.SetAll(aTlvs.GetNotChildProvidedMtd());
    }
}

void Server::ChildInfo::CommitDiagUpdate(void)
{
    OT_ASSERT(mDiagCacheLocked);
    mDiagCacheLocked = false;

    mAttachStateDirty = false;

    mDirtySet.Clear();
    mCache.Free();
    mCacheBuffers = 0;
}

void Server::ChildInfo::AbortDiagUpdate(void)
{
    OT_ASSERT(mDiagCacheLocked);
    mDiagCacheLocked = false;

    if (mCache == nullptr)
    {
        EvictDiagCache();
    }
}

TlvSet Server::ChildInfo::GetDirtyHostProvided(TlvSet aFilter) const
{
    TlvSet set = mDirtySet.Intersect(aFilter);

    if (mDiagFtd)
    {
        set = set.GetNotChildProvidedFtd();
    }
    else
    {
        set = set.GetNotChildProvidedMtd();
    }

    return set;
}

void Server::ChildInfo::EvictDiagCache(void)
{
    TlvSet lost;

    OT_ASSERT(!mDiagCacheLocked);

    if (mDiagFtd)
    {
        lost = mDirtySet.GetChildProvidedFtd();
    }
    else
    {
        lost = mDirtySet.GetChildProvidedMtd();
    }

    mDirtySet.ClearAll(lost);
    mLostSet.SetAll(lost);

    mCache.Free();
    mCacheBuffers = 0;
}

void Server::ChildInfo::ResetDiagCache(void)
{
    OT_ASSERT(!mDiagCacheLocked);

    mDirtySet.Clear();
    mLostSet.Clear();

    mCache.Free();
    mCacheBuffers = 0;
}

Error Server::ChildInfo::UpdateDiagCache(const Message &aMessage, TlvSet aFilter)
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

    // Prevent freeing the cache were currently building
    // If we run out of memory the error handler of this function will free it
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

        if (mDiagFtd)
        {
            set = set.GetChildProvidedFtd();
        }
        else
        {
            set = set.GetChildProvidedMtd();
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
            SuccessOrExit(error = Tlv::FindTlv(*mCache, tlv.GetType(), sizeof(ExtendedTlv), tlv, dstOffset));
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
        // A error here could render the cache invalid so we just clear it and query
        // the tlvs again using the lost set
        LogCrit("Diag cache error %s", ErrorToString(error));
        EvictDiagCache();
    }

    if (mCache != nullptr)
    {
        mCacheBuffers = mCache->GetBufferCount();
    }

    return error;
}

Error Server::ChildInfo::AppendDiagCache(Message &aMessage)
{
    Error error = kErrorNone;

    VerifyOrExit(mCache != nullptr);
    OT_ASSERT(mDiagCacheLocked);

    SuccessOrExit(error = aMessage.AppendBytesFromMessage(*mCache, 0, mCache->GetLength()));

    // We free the cache to provde extra message buffers for the diag message
    //
    // Since this function must be called within a child update block we temporarily
    // allow invalid state where the mCache and mDirtySet can diverge.
    //
    // This will be reconciled during CommitDiagUpdate() or AbortDiagUpdate().
    // Either by marking everything as clean or updating the lost set.
    mCache.Free();
    mCacheBuffers = 0;

exit:
    return error;
}
#endif

Server::Server(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mUpdateTimer(aInstance)
#if OPENTHREAD_FTD
    , mChildTimer(aInstance)
    , mRegistrationTimer(aInstance)
#endif
{
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

    // If the server is inactive the enabled set will always be 0 so this
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
    // Use mDiagFtd to allow the compiler to optimize away the check in MarkDiagsDirty
    if (aChild.mDiagFtd)
    {
        aTlvs = aTlvs.GetNotChildProvidedFtd();
    }
    else
    {
        aTlvs = aTlvs.GetNotChildProvidedMtd();
    }

    // If the server is inactive the enabled set will always be 0 so this
    // check will always fail.
    if (!aTlvs.IsEmpty())
    {
        aChild.MarkDiagsDirty(aTlvs);
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
    aChild.ResetDiagCache();
    aChild.SetDiagFtd(aChild.IsFullThreadDevice());
    aChild.SetDiagServerState(Child::kDiagServerStopped);

    VerifyOrExit(mActive);

    aChild.SetAttachStateDirty();

    if (!mChildEnabled.IsEmpty())
    {
        UpdateChildServer(aChild, true);
        ScheduleUpdateTimer(kUpdateBaseDelay);
    }

exit:
    return;
}

void Server::HandleChildRemoved(Child &aChild)
{
    aChild.ResetDiagCache();
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

Error Server::EvictDiagCache(bool aOnlyRxOn)
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
                    // First avoid non csl children as their poll intervalls may be
                    // very large
                    continue;
                }
#endif
            }

            child.EvictDiagCache();
            mCacheSyncEvictions++;
        }
        else
        {
            child.ResetDiagCache();
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

        child.EvictDiagCache();
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
void Server::HandleTmf<kUriDiagnosticEndDeviceRequest>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error          error    = kErrorNone;
    Coap::Message *response = nullptr;

    ChildRequestHeader header;
    TlvSet             set;
    uint16_t           offset;
    bool               changed = false;

    VerifyOrExit(aMessage.IsPostRequest(), error = kErrorInvalidArgs);

    LogInfo("Received %s from %s", UriToString<kUriDiagnosticEndDeviceRequest>(),
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
        changed = StartServerAsChild(set);
        break;

    case ChildRequestHeader::kStop:
        StopServer();
        break;

    default:
        break;
    }

    if (header.GetQuery() || changed)
    {
        SuccessOrExit(error = AppendSelfTlvs(*response, mSelfEnabled));
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*response, aMessageInfo));

exit:
    FreeMessageOnError(response, error);
}

#if OPENTHREAD_FTD
template <>
void Server::HandleTmf<kUriDiagnosticEndDeviceUpdate>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Child *child = nullptr;
    TlvSet set;

    VerifyOrExit(aMessageInfo.GetPeerAddr().GetIid().IsRoutingLocator());
    child = Get<ChildTable>().FindChild(aMessageInfo.GetPeerAddr().GetIid().GetLocator(), Child::kInStateValid);

    VerifyOrExit(child != nullptr);
    VerifyOrExit(aMessage.IsPostRequest());

    LogInfo("Received %s from %s", UriToString<kUriDiagnosticEndDeviceRequest>(),
            aMessageInfo.GetPeerAddr().ToString().AsCString());

    IgnoreError(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));

    // If the child should be disabled this will update it
    UpdateChildServer(*child, false);

    if (child->IsFullThreadDevice())
    {
        set = mChildEnabled.GetChildProvidedFtd();
    }
    else
    {
        set = mChildEnabled.GetChildProvidedMtd();
    }

    if (child->UpdateDiagCache(aMessage, set) != kErrorNone)
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
void Server::HandleTmf<kUriDiagnosticServerRequest>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
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

    LogInfo("Received %s from %s", UriToString<kUriDiagnosticServerRequest>(),
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
        uint16_t peer = aMessageInfo.GetPeerAddr().GetIid().GetLocator();
        if (mClientRloc != peer)
        {
            // If we are not currently active StartServerAsRouter will clear these
            mMultiClient      = true;
            mMultiClientRenew = true;
        }
        mClientRloc = peer;
    }

    SuccessOrExit(StartServerAsRouter(hostSet, childSet, neighborSet, header.GetQuery()));

    if (header.GetQuery())
    {
        IgnoreError(SendQueryResponse(aMessageInfo.GetPeerAddr()));
    }
    else if (header.GetRegistration())
    {
        IgnoreError(SendRegistrationResponse(aMessageInfo.GetPeerAddr()));
    }

exit:
    return;
}
#endif // OPENTHREAD_FTD

bool Server::StartServerAsChild(const TlvSet &aTypes)
{
    bool changed = false;

    OT_UNUSED_VARIABLE(aTypes);

    if (!mActive)
    {
        mActive     = true;
        mUpdateSent = false;

        mSelfEnabled.Clear();
        mSelfRenewUpdate.Clear();
        mSelfDirty.Clear();

#if OPENTHREAD_FTD
        mChildEnabled.Clear();
        mChildRenew.Clear();

        mNeighborEnabled.Clear();
        mNeighborRenew.Clear();
#endif
    }

    changed      = mSelfEnabled != aTypes;
    mSelfEnabled = aTypes;
    mSelfEnabled.FilterChildValid();

    // Suppress unused label warning in mtd build
    ExitNow();
exit:
    return changed;
}

void Server::StopServer(void)
{
    mSelfEnabled.Clear();
    mSelfRenewUpdate.Clear();

    mUpdateTimer.Stop();
#if OPENTHREAD_FTD
    mRegistrationTimer.Stop();

    mChildEnabled.Clear();
    mChildRenew.Clear();

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAny))
    {
        child.ResetDiagCache();
    }

    if (Get<Mle::Mle>().IsRouterOrLeader())
    {
        // Stop child servers
        ScheduleChildTimer();
    }
#endif

    mActive = false;
}

Error Server::SendUpdateAsChild(void)
{
    Error error = kErrorNone;

    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    VerifyOrExit(Get<Mle::Mle>().IsChild(), error = kErrorInvalidState);
    VerifyOrExit(!mUpdateSent, error = kErrorAlready);

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriDiagnosticEndDeviceUpdate);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    messageInfo.SetSockAddrToRlocPeerAddrTo(Get<Mle::Mle>().GetParentRloc16());

    SuccessOrExit(error = AppendSelfTlvs(*message, mSelfDirty));
    mSelfRenewUpdate = mSelfDirty;
    mSelfDirty.Clear();

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleChildUpdateResponse, this));
    mUpdateSent = true;

exit:
    if (error != kErrorNone)
    {
        LogCrit("Failed to send child update: %s", ErrorToString(error));
    }
    FreeMessageOnError(message, error);
    return error;
}

void Server::HandleChildUpdateResponse(Coap::Message *aResponse, const Ip6::MessageInfo *aMessageInfo, Error aResult)
{
    OT_UNUSED_VARIABLE(aResponse);
    OT_UNUSED_VARIABLE(aMessageInfo);

    VerifyOrExit(mActive);

    mUpdateSent = false;

    if (aResult != kErrorNone)
    {
        mSelfDirty = mSelfDirty.Join(mSelfRenewUpdate);
    }

exit:
    return;
}

void Server::HandleChildUpdateResponse(void                *aContext,
                                       otMessage           *aResponse,
                                       const otMessageInfo *aMessageInfo,
                                       otError              aResult)
{
    OT_ASSERT(aContext != nullptr);

    Coap::Message          *response    = AsCoapMessagePtr(aResponse);
    const Ip6::MessageInfo *messageInfo = AsCoreTypePtr(aMessageInfo);

    static_cast<Server *>(aContext)->HandleChildUpdateResponse(response, messageInfo, aResult);
}

#if OPENTHREAD_FTD
Error Server::SendQueryResponse(const Ip6::Address &aPeerAddr)
{
    Error error = kErrorNone;

    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    UpdateHeader header;

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriDiagnosticServerUpdate);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    messageInfo.SetSockAddrToRlocPeerAddrTo(aPeerAddr);

    header.Init();
    header.SetComplete(true);
    header.SetRouterId(Mle::RouterIdFromRloc16(Get<Mle::Mle>().GetRloc16()));
    header.SetSeqNumberFull(mSequenceNumber);
    SuccessOrExit(error = header.AppendTo(*message));

    if (!mSelfEnabled.IsEmpty())
    {
        SuccessOrExit(error = AppendHostContext(*message, mSelfEnabled));
    }

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        SuccessOrExit(error = AppendChildContextQuery(*message, child));
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

            SuccessOrExit(error = AppendNeighborContextQuery(*message, *router));
        }
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, nullptr, nullptr));

exit:
    if (error != kErrorNone)
    {
        LogCrit("Failed to send response: %s", ErrorToString(error));
    }
    FreeMessageOnError(message, error);
    return error;
}

Error Server::SendRegistrationResponse(const Ip6::Address &aPeerAddr)
{
    Error error = kErrorNone;

    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    UpdateHeader header;

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriDiagnosticServerUpdate);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    messageInfo.SetSockAddrToRlocPeerAddrTo(aPeerAddr);

    header.Init();
    header.SetRouterId(Mle::RouterIdFromRloc16(Get<Mle::Mle>().GetRloc16()));
    header.SetSeqNumberFull(mSequenceNumber);
    SuccessOrExit(error = header.AppendTo(*message));

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, nullptr, nullptr));

exit:
    if (error != kErrorNone)
    {
        LogCrit("Failed to send response: %s", ErrorToString(error));
    }
    FreeMessageOnError(message, error);
    return error;
}

Error Server::SendUpdateAsRouter(void)
{
    Error error = kErrorNone;

    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    UpdateHeader header;
    TlvSet       hostSet;
    Context      hostContext;
    uint16_t     offset;

    BeginDiagUpdate();

    message = Get<Tmf::Agent>().NewNonConfirmablePostMessage(kUriDiagnosticServerUpdate);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    if (mMultiClient)
    {
        messageInfo.SetSockAddrToRlocPeerAddrToRealmLocalAllRoutersMulticast();
    }
    else
    {
        messageInfo.SetSockAddrToRlocPeerAddrTo(mClientRloc);
    }

    header.Init();
    header.SetRouterId(Mle::RouterIdFromRloc16(Get<Mle::Mle>().GetRloc16()));
    header.SetSeqNumberShort(mSequenceNumber + 1);

    SuccessOrExit(error = header.AppendTo(*message));

    hostSet = mSelfDirty.Intersect(mSelfEnabled);
    if (!hostSet.IsEmpty())
    {
        hostContext.Init();
        hostContext.SetType(kTypeHost);

        offset = message->GetLength();
        SuccessOrExit(error = message->Append(hostContext));
        SuccessOrExit(error = AppendSelfTlvs(*message, hostSet));
        hostContext.SetLength(message->GetLength() - offset);
        message->Write(offset, hostContext);
    }

    if (!mChildEnabled.IsEmpty())
    {
        for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAny))
        {
            SuccessOrExit(error = AppendChildContextUpdate(*message, child));
        }
    }

    if (!mNeighborEnabled.IsEmpty())
    {
        for (uint8_t id = 0; id < Mle::kMaxRouterId; id++)
        {
            SuccessOrExit(error = AppendNeighborContextUpdate(*message, id));
        }
    }

    SuccessOrExit(Get<Tmf::Agent>().SendMessage(*message, messageInfo, nullptr, nullptr));

    mSequenceNumber++;

    mSelfDirty.Clear();

exit:
    if (error == kErrorNone)
    {
        CommitDiagUpdate();
    }
    else
    {
        LogCrit("Failed to send router update %s", ErrorToString(error));
        AbortDiagUpdate();
        FreeMessage(message);

        // We may have lost some diag data
        ScheduleChildTimer();
    }
    return error;
}

Error Server::StartServerAsRouter(const TlvSet &aSelf, const TlvSet &aChild, const TlvSet &aNeighbor, bool aQuery)
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
        mSelfRenewUpdate.Clear();

        mChildEnabled.Clear();
        mChildRenew.Clear();

        mNeighborEnabled.Clear();
        mNeighborRenew.Clear();

        mActive = true;

        mMultiClient      = false;
        mMultiClientRenew = false;

        mRouterStateMask = 0;

        mRegistrationTimer.Start(kRegistrationInterval);
    }

    mSelfRenewUpdate = mSelfRenewUpdate.Join(aSelf);
    mSelfRenewUpdate.FilterHostValid();
    mSelfEnabled = mSelfEnabled.Join(aSelf);
    mSelfEnabled.FilterHostValid();

    oldFtd = mChildEnabled.GetChildProvidedFtd();
    oldMtd = mChildEnabled.GetChildProvidedMtd();

    mChildRenew = mChildRenew.Join(aChild);
    mChildRenew.FilterChildValid();
    mChildEnabled = mChildEnabled.Join(aChild);
    mChildEnabled.FilterChildValid();

    UpdateChildServers(oldFtd != mChildEnabled.GetChildProvidedFtd(), oldMtd != mChildEnabled.GetChildProvidedMtd(),
                       aQuery);

    mNeighborRenew = mNeighborRenew.Join(aNeighbor);
    mNeighborRenew.FilterNeighborValid();
    mNeighborEnabled = mNeighborEnabled.Join(aNeighbor);
    mNeighborEnabled.FilterNeighborValid();

exit:
    return error;
}

void Server::UpdateChildServers(bool aMtdChanged, bool aFtdChanged, bool aQuery)
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

        UpdateChildServer(child, changed | aQuery);
    }
}

void Server::UpdateChildServer(Child &aChild, bool aQuery)
{
    TlvSet set;

    if (aChild.IsFullThreadDevice())
    {
        set = mChildEnabled.GetChildProvidedFtd();
    }
    else
    {
        set = mChildEnabled.GetChildProvidedMtd();
    }

    if (set.IsEmpty())
    {
        aChild.ResetDiagCache();

        switch (aChild.GetDiagServerState())
        {
        case Child::kDiagServerActive:
        case Child::kDiagServerActivePending:
        case Child::kDiagServerUnknown:
            IgnoreError(SendChildStop(aChild));
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
            // If SendChildStart fails it will still stop the pending transaction
            // and set state to unknown so the next update will retry even
            // without aQuery being set.
            OT_FALL_THROUGH;

        case Child::kDiagServerUnknown:
            // Make sure we always query after failed updates
            aQuery = true;
            OT_FALL_THROUGH;

        case Child::kDiagServerStopped:
        case Child::kDiagServerStopPending:
            IgnoreError(SendChildStart(aChild, set, aQuery));
            break;
        }
    }
}

Error Server::SendChildStop(Child &aChild)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    ChildRequestHeader header;

    if (aChild.IsDiagServerPending())
    {
        IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleChildCommandResponse, &aChild));
    }

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriDiagnosticEndDeviceRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    messageInfo.SetSockAddrToRlocPeerAddrTo(aChild.GetRloc16());

    header.Clear();
    header.SetCommand(ChildRequestHeader::kStop);

    SuccessOrExit(error = message->Append(header));

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleChildCommandResponse, &aChild));
    aChild.SetDiagServerState(Child::kDiagServerStopPending);

    LogInfo("Sent DiagServer stop to child %04x", aChild.GetRloc16());

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Server::SendChildStart(Child &aChild, const TlvSet &aTypes, bool aQuery)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    ChildRequestHeader header;
    uint16_t           offset;
    uint8_t            setCount = 0;

    if (aChild.IsDiagServerPending())
    {
        IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleChildCommandResponse, &aChild));
    }

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriDiagnosticEndDeviceRequest);
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

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleChildCommandResponse, &aChild));
    aChild.SetDiagServerState(Child::kDiagServerActivePending);

    LogInfo("Sent DiagServer start to child %04x", aChild.GetRloc16());

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Server::SendChildQuery(Child &aChild, const TlvSet &aTypes, bool aLost)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    ChildRequestHeader header;
    uint16_t           offset;
    uint8_t            setCount = 0;

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriDiagnosticEndDeviceRequest);
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

    if (aLost)
    {
        SuccessOrExit(error =
                          Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleChildLostQueryResponse, &aChild));
        aChild.SetLostDiagQueryPending(true);
    }
    else
    {
        // TODO
        error = kErrorInvalidState;
    }

    if (aLost)
    {
        LogInfo("Sent DiagServer query to child %04x", aChild.GetRloc16());
    }
    else
    {
        LogInfo("Sent DiagServer lost query to child %04x", aChild.GetRloc16());
    }

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Server::HandleChildCommandResponse(Child                  &aChild,
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
        if (aChild.UpdateDiagCache(*aResponse, mChildEnabled) == kErrorNone)
        {
            ScheduleUpdateTimer(kUpdateBaseDelay);
        }
        else
        {
            mCacheErrors++;
            // TODO
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

void Server::HandleChildCommandResponse(void                *aContext,
                                        otMessage           *aResponse,
                                        const otMessageInfo *aMessageInfo,
                                        Error                aResult)
{
    OT_ASSERT(aContext != nullptr);

    Child                  *child       = static_cast<Child *>(aContext);
    Coap::Message          *response    = AsCoapMessagePtr(aResponse);
    const Ip6::MessageInfo *messageInfo = AsCoreTypePtr(aMessageInfo);

    child->GetInstance().Get<Server>().HandleChildCommandResponse(*child, response, messageInfo, aResult);
}

void Server::HandleChildLostQueryResponse(Child                  &aChild,
                                          Coap::Message          *aResponse,
                                          const Ip6::MessageInfo *aMessageInfo,
                                          Error                   aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    aChild.SetLostDiagQueryPending(false);

    if (aResult == kErrorNone)
    {
        OT_ASSERT(aResponse != nullptr);
        if (aChild.UpdateDiagCache(*aResponse, aChild.GetLostDiag()) != kErrorNone)
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

void Server::HandleChildLostQueryResponse(void                *aContext,
                                          otMessage           *aResponse,
                                          const otMessageInfo *aMessageInfo,
                                          Error                aResult)
{
    OT_ASSERT(aContext != nullptr);

    Child                  *child       = static_cast<Child *>(aContext);
    Coap::Message          *response    = AsCoapMessagePtr(aResponse);
    const Ip6::MessageInfo *messageInfo = AsCoreTypePtr(aMessageInfo);

    child->GetInstance().Get<Server>().HandleChildLostQueryResponse(*child, response, messageInfo, aResult);
}
#endif // OPENTHREAD_FTD

Error Server::AppendSelfTlvs(Message &aMessage, TlvSet aTlvs)
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
            SuccessOrExit(error = AppendSelfIp6AddressList(aMessage));
            break;

        case Tlv::kAlocList:
            SuccessOrExit(error = AppendSelfAlocList(aMessage));
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
            SuccessOrExit(error = AppendSelfIp6LinkLocalAddressList(aMessage));
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
            // TODO
            break;
        }

        case Tlv::kMleCounters:
        {
            MleCountersTlv tlv;

            tlv.Init(Get<Mle::Mle>().GetCounters());
            SuccessOrExit(error = tlv.AppendTo(aMessage));
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

    SuccessOrExit(error = AppendSelfTlvs(aMessage, aTlvs));

    context.SetLength(aMessage.GetLength() - offset);
    aMessage.Write(offset, context);

exit:
    return error;
}

Error Server::AppendChildContextQuery(Message &aMessage, Child &aChild)
{
    Error error = kErrorNone;

    ChildContext context;
    uint16_t     offset = aMessage.GetLength();
    TlvSet       set;

    VerifyOrExit(aChild.IsStateValid());

    context.Init();
    context.SetType(kTypeChild);
    context.SetId(Mle::ChildIdFromRloc16(aChild.GetRloc16()));
    SuccessOrExit(error = aMessage.Append(context));

    context.SetUpdateMode(kModeAdded);

    if (aChild.IsFullThreadDevice())
    {
        set = mChildEnabled.GetNotChildProvidedFtd();
    }
    else
    {
        set = mChildEnabled.GetNotChildProvidedMtd();
    }

    SuccessOrExit(error = AppendChildTlvs(aMessage, set, aChild));

    context.SetLength(aMessage.GetLength() - offset);
    aMessage.Write(offset, context);

exit:
    return error;
}

Error Server::AppendChildContextUpdate(Message &aMessage, Child &aChild)
{
    Error error = kErrorNone;

    ChildContext context;
    uint16_t     offset = aMessage.GetLength();

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
        }
        else
        {
            context.SetUpdateMode(kModeRemove);
        }
    }

    if (aChild.IsStateValid())
    {
        SuccessOrExit(error = AppendChildTlvs(aMessage, aChild.GetDirtyHostProvided(mChildEnabled), aChild));
        SuccessOrExit(error = aChild.AppendDiagCache(aMessage));
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
        }
        break;

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

Error Server::AppendNeighborContextQuery(Message &aMessage, const Router &aRouter)
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

            tlvs.Filter(static_cast<const TlvSet &>(kNeighborStaticTlvs));
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

void Server::BeginDiagUpdate(void)
{
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAny))
    {
        child.BeginDiagUpdate();
    }
}

void Server::CommitDiagUpdate(void)
{
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAny))
    {
        child.CommitDiagUpdate();
    }
}

void Server::AbortDiagUpdate(void)
{
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        child.AbortDiagUpdate();
    }
}
#endif // OPENTHREAD_FTD

bool Server::FilterIp6Address(const Ip6::Address &aAddress)
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

bool Server::FilterAloc(const Ip6::Address &aAddress, uint8_t &aAloc)
{
    bool pass = false;

    VerifyOrExit(aAddress.GetIid().IsAnycastLocator());
    aAloc = static_cast<uint8_t>(aAddress.GetIid().GetLocator());

    pass = true;

exit:
    return pass;
}

bool Server::FilterIp6LinkLocalAddress(const Ip6::Address &aAddress)
{
    bool pass = false;

    VerifyOrExit(aAddress.IsLinkLocalUnicastOrMulticast());
    VerifyOrExit(!aAddress.IsLinkLocalAllNodesMulticast());
    VerifyOrExit(!aAddress.IsLinkLocalAllRoutersMulticast());

exit:
    return pass;
}

Error Server::AppendSelfIp6AddressList(Message &aMessage)
{
    Error    error = kErrorNone;
    uint16_t count = 0;

    for (const Ip6::Netif::UnicastAddress &address : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (FilterIp6Address(address.GetAddress()))
        {
            count++;
        }
    }
    for (const Ip6::Netif::MulticastAddress &address : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (FilterIp6Address(address.GetAddress()))
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
        if (FilterIp6Address(address.GetAddress()))
        {
            SuccessOrExit(error = aMessage.Append(address.GetAddress()));
        }
    }
    for (const Ip6::Netif::MulticastAddress &address : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (FilterIp6Address(address.GetAddress()))
        {
            SuccessOrExit(error = aMessage.Append(address.GetAddress()));
        }
    }

exit:
    return error;
}

Error Server::AppendSelfAlocList(Message &aMessage)
{
    Error    error = kErrorNone;
    uint16_t count = 0;
    uint8_t  aloc;

    for (const Ip6::Netif::UnicastAddress &address : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (FilterAloc(address.GetAddress(), aloc))
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
        if (FilterAloc(address.GetAddress(), aloc))
        {
            SuccessOrExit(error = aMessage.Append(aloc));
        }
    }

exit:
    return error;
}

Error Server::AppendSelfIp6LinkLocalAddressList(Message &aMessage)
{
    Error    error = kErrorNone;
    uint16_t count = 0;

    for (const Ip6::Netif::UnicastAddress &address : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (FilterIp6LinkLocalAddress(address.GetAddress()))
        {
            count++;
        }
    }
    for (const Ip6::Netif::MulticastAddress &address : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (FilterIp6LinkLocalAddress(address.GetAddress()))
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
        if (FilterIp6LinkLocalAddress(address.GetAddress()))
        {
            SuccessOrExit(error = aMessage.Append(address.GetAddress()));
        }
    }
    for (const Ip6::Netif::MulticastAddress &address : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (FilterIp6LinkLocalAddress(address.GetAddress()))
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
        if (FilterIp6Address(address))
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
        if (FilterIp6Address(address))
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
        if (FilterAloc(address, aloc))
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
        if (FilterAloc(address, aloc))
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
        error = SendUpdateAsRouter();
    }
    else
#endif // OPENTHREAD_FTD
    {
        error = SendUpdateAsChild();
    }

    if (error != kErrorNone)
    {
        ScheduleUpdateTimer(kUpdateBaseDelay);
    }
    else if (HasExtDelayTlvs(mSelfEnabled)
#if OPENTHREAD_FTD
             || HasExtDelayTlvs(mChildEnabled) || HasExtDelayTlvs(mNeighborEnabled)
#endif
    )
    {
        ScheduleUpdateTimer(kUpdateExtDelay);
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
        UpdateChildServer(child, false);

        // Potential future enhancement is to only try this when message
        // buffers are available
        if (child.ShouldSendLostDiagQuery())
        {
            IgnoreError(SendChildQuery(child, child.GetLostDiag(), true));
        }
    }

exit:
    return;
}

void Server::HandleRegistrationTimer(void)
{
    TlvSet mtd = mSelfEnabled.GetChildProvidedMtd();
    TlvSet ftd = mSelfEnabled.GetChildProvidedFtd();

    VerifyOrExit(mActive && Get<Mle::Mle>().IsRouterOrLeader());

    mMultiClient      = mMultiClientRenew;
    mMultiClientRenew = false;

    mSelfEnabled = mSelfRenewUpdate;
    mSelfRenewUpdate.Clear();

    mChildEnabled = mChildRenew;
    mChildRenew.Clear();

    mNeighborEnabled = mNeighborRenew;
    mNeighborRenew.Clear();

    if (mSelfEnabled.IsEmpty() && mChildEnabled.IsEmpty())
    {
        StopServer();
    }
    else
    {
        mRegistrationTimer.Start(kRegistrationInterval);
        UpdateChildServers(mtd != mChildEnabled.GetChildProvidedMtd(), ftd != mChildEnabled.GetChildProvidedFtd(),
                           false);
    }

exit:
    return;
}
#endif // OPENTHREAD_FTD
#endif // OPENTHREAD_CONFIG_DIAG_SERVER_ENABLE

#if OPENTHREAD_CONFIG_DIAG_CLIENT_ENABLE
Client::Client(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer{aInstance}
{
}

void Client::Start(const TlvSet              *aHost,
                   const TlvSet              *aChild,
                   const TlvSet              *aNeighbor,
                   otDiagServerUpdateCallback aCallback,
                   void                      *aContext)
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
        mHostSet.FilterHostValid();
    }

    if (aChild != nullptr)
    {
        mChildSet = *aChild;
        mChildSet.FilterChildValid();
    }

    if (aNeighbor != nullptr)
    {
        mNeighborSet = *aNeighbor;
        mNeighborSet.FilterNeighborValid();
    }

    if (SendRegistration(true) == kErrorNone)
    {
        ScheduleNextUpdate();
        mQueryPending = false;
    }
    else
    {
        ScheduleRetry();
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

Error Client::GetNextContext(const Coap::Message  &aMessage,
                             otDiagServerIterator &aIterator,
                             otDiagServerContext  &aContext)
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

    if (aIterator == OT_DIAG_SERVER_ITERATOR_INIT)
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
            aContext.mType        = OT_DIAG_SERVER_DEVICE_HOST;
            aContext.mRloc16      = Mle::Rloc16FromRouterId(header.GetRouterId());
            aContext.mTlvIterator = offset + sizeof(Context);
            ExitNow();

        case kTypeChild:
            SuccessOrExit(error = aMessage.Read(offset, childContext));

            aContext.mType        = OT_DIAG_SERVER_DEVICE_CHILD;
            aContext.mRloc16      = Mle::Rloc16FromRouterId(header.GetRouterId()) | childContext.GetId();
            aContext.mTlvIterator = offset + sizeof(ChildContext);
            aContext.mLegacy      = childContext.GetLegacy();
            aContext.mUpdateMode  = UpdateModeToApiValue(childContext.GetUpdateMode());
            ExitNow();

        case kTypeNeighbor:
            SuccessOrExit(error = aMessage.Read(offset, neighborContext));

            aContext.mType        = OT_DIAG_SERVER_DEVICE_NEIGHBOR;
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

Error Client::GetNextTlv(const Coap::Message &aMessage, otDiagServerContext &aContext, otDiagServerTlv &aTlv)
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

        case Tlv::kMlEid:
            SuccessOrExit(error = Tlv::Read<MlEidTlv>(aMessage, offset, AsCoreType(&aTlv.mData.mMlEid)));
            ExitNow();

        case Tlv::kIp6AddressList:
        case Tlv::kIp6LinkLocalAddressList:
        {
            // mIp6AddressList and mIp6LinkLocalAddress list are identical structs
            // in the union so it is fine to use them interchangably here.
            uint16_t count = value.GetLength() / sizeof(otIp6Address);
            VerifyOrExit(count * sizeof(otIp6Address) == value.GetLength(), error = kErrorParse);

            aTlv.mData.mIp6AddressList.mCount      = count;
            aTlv.mData.mIp6AddressList.mDataOffset = value.GetOffset();
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

        case Tlv::kVendorAppUrl:
            SuccessOrExit(error = Tlv::Read<VendorAppUrlTlv>(aMessage, offset, aTlv.mData.mVendorAppUrl));
            ExitNow();

        case Tlv::kMleCounters:
        {
            MleCountersTlv data;

            SuccessOrExit(aMessage.Read(offset, data));
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

Error Client::SendRegistration(bool aQuery)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    RequestHeader header;

    messageInfo.SetSockAddrToRlocPeerAddrToRealmLocalAllRoutersMulticast();

    message = Get<Tmf::Agent>().NewNonConfirmablePostMessage(kUriDiagnosticServerRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    header.Clear();
    header.SetQuery(aQuery);
    header.SetRegistration(true);
    SuccessOrExit(error = message->Append(header));

    if (!mHostSet.IsEmpty())
    {
        SuccessOrExit(error = AppendContextTo(*message, kTypeHost, mHostSet));
    }

    if (!mChildSet.IsEmpty())
    {
        SuccessOrExit(error = AppendContextTo(*message, kTypeChild, mChildSet));
    }

    if (!mNeighborSet.IsEmpty())
    {
        SuccessOrExit(error = AppendContextTo(*message, kTypeNeighbor, mNeighborSet));
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleResponse, nullptr));

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Client::SendErrorQuery(uint16_t aRloc16)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    RequestHeader header;

    messageInfo.SetSockAddrToRlocPeerAddrTo(aRloc16);

    message = Get<Tmf::Agent>().NewNonConfirmablePostMessage(kUriDiagnosticServerRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    header.Clear();
    header.SetQuery(true);
    header.SetRegistration(true);
    SuccessOrExit(error = message->Append(header));

    if (!mHostSet.IsEmpty())
    {
        SuccessOrExit(error = AppendContextTo(*message, kTypeHost, mHostSet));
    }

    if (!mChildSet.IsEmpty())
    {
        SuccessOrExit(error = AppendContextTo(*message, kTypeChild, mChildSet));
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleResponse, nullptr));

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Client::AppendContextTo(Message &aMessage, DeviceType aType, const TlvSet &aSet)
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
void Client::HandleTmf<kUriDiagnosticServerUpdate>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error error = kErrorNone;

    VerifyOrExit(mActive);

    ProcessUpdate(aMessage, aMessageInfo);

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

void Client::ProcessUpdate(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    UpdateHeader header;
    bool         sequenceError = false;
    bool         mayDuplicate  = false;

    OT_UNUSED_VARIABLE(aMessageInfo);

    SuccessOrExit(header.ReadFrom(aMessage));
    VerifyOrExit(header.GetRouterId() <= Mle::kMaxRouterId);

    if (header.GetComplete())
    {
        VerifyOrExit(header.HasFullSeqNumber());
        mServerSeqNumbers[header.GetRouterId()] = header.GetSeqNumberFull();
    }
    else
    {
        uint64_t next = mServerSeqNumbers[header.GetRouterId()] + 1;

        if (header.HasFullSeqNumber())
        {
            sequenceError = next != header.GetSeqNumberFull();
            mayDuplicate  = (next - header.GetSeqNumberFull()) < 4;
        }
        else
        {
            sequenceError = static_cast<uint8_t>(next) != header.GetSeqNumberShort();
            mayDuplicate  = (static_cast<uint8_t>(next) - header.GetSeqNumberShort()) < 4;
        }

        mServerSeqNumbers[header.GetRouterId()] = next;
    }

    if (sequenceError)
    {
        if (!mayDuplicate)
        {
            LogCrit("Sequence error!");
            IgnoreError(SendErrorQuery(Mle::Rloc16FromRouterId(header.GetRouterId())));
        }
        ExitNow();
    }

    if (mCallback != nullptr)
    {
        (mCallback)(&aMessage, Mle::Rloc16FromRouterId(header.GetRouterId()), header.GetComplete(), mCallbackContext);
    }

exit:
    return;
}

void Client::HandleResponse(Coap::Message *aResponse, const Ip6::MessageInfo *aMessageInfo, Error aResult)
{
    if (aResult == kErrorNone)
    {
        VerifyOrExit(aResponse != nullptr);
        VerifyOrExit(aMessageInfo != nullptr);

        ProcessUpdate(*aResponse, *aMessageInfo);
    }
    else
    {
        // TODO
    }

exit:
    return;
}

void Client::HandleResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aResult)
{
    Client *client = static_cast<Client *>(aContext);

    VerifyOrExit(client != nullptr);
    client->HandleResponse(AsCoapMessagePtr(aMessage), AsCoreTypePtr(aMessageInfo), aResult);

exit:
    return;
}

void Client::ScheduleNextUpdate(void)
{
    mTimer.Start(Random::NonCrypto::AddJitter(kRegistrationInterval, kRegistrationJitter));
}

void Client::ScheduleRetry(void) { mTimer.Start(Random::NonCrypto::GetUint32InRange(0, kRegistrationJitter / 5)); }

void Client::HandleUpdateTimer(void)
{
    VerifyOrExit(mActive);

    if (SendRegistration(mQueryPending) == kErrorNone)
    {
        mQueryPending = false;
        ScheduleNextUpdate();
    }
    else
    {
        ScheduleRetry();
    }

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_DIAG_CLIENT_ENABLE

} // namespace DiagnosticServer

} // namespace ot
