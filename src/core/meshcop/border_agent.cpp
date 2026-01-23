/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements the BorderAgent service.
 */

#include "border_agent.hpp"

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

#include "instance/instance.hpp"
#include "meshcop/border_agent_txt_data.hpp"

namespace ot {
namespace MeshCoP {
namespace BorderAgent {

RegisterLogModule("BorderAgent");

//----------------------------------------------------------------------------------------------------------------------
// `Manager`

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
const char Manager::kServiceType[]            = "_meshcop._udp";
const char Manager::kDefaultBaseServiceName[] = OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_BASE_NAME;
#endif

Manager::Manager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(true)
    , mIsRunning(false)
    , mSessionIndex(0)
    , mDtlsTransport(aInstance, kNoLinkSecurity)
    , mCommissionerSession(nullptr)
    , mCommissionerUdpReceiver(HandleUdpReceive, this)
#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    , mIdInitialized(false)
#endif
{
    mCommissionerAloc.InitAsThreadOriginMeshLocal();

    ClearAllBytes(mCounters);

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    ClearAllBytes(mServiceName);

    static_assert(sizeof(kDefaultBaseServiceName) - 1 <= kBaseServiceNameMaxLen,
                  "OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_BASE_NAME is too long");
#endif
}

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
void Manager::GetId(Id &aId)
{
    if (mIdInitialized)
    {
        aId = mId;
        ExitNow();
    }

    if (Get<Settings>().Read<Settings::BorderAgentId>(mId) != kErrorNone)
    {
        mId.GenerateRandom();
        Get<Settings>().Save<Settings::BorderAgentId>(mId);
    }

    mIdInitialized = true;
    aId            = mId;

exit:
    return;
}

void Manager::SetId(const Id &aId)
{
    if (mIdInitialized)
    {
        VerifyOrExit(aId != mId);
    }

    Get<Settings>().Save<Settings::BorderAgentId>(aId);
    mId            = aId;
    mIdInitialized = true;

    Get<TxtData>().Refresh();

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE

void Manager::SetEnabled(bool aEnabled)
{
    VerifyOrExit(mEnabled != aEnabled);
    mEnabled = aEnabled;
    LogInfo("%sabling Border Agent", mEnabled ? "En" : "Dis");

    Get<TxtData>().Refresh();

    UpdateState();

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    if (!mEnabled)
    {
        UnregisterService();
    }
#endif

exit:
    return;
}

void Manager::UpdateState(void)
{
    if (mEnabled && Get<Mle::Mle>().IsAttached())
    {
        Start();
    }
    else
    {
        Stop();
    }
}

void Manager::Start(void)
{
    Error error = kErrorNone;
    Pskc  pskc;

    VerifyOrExit(!mIsRunning);

    mDtlsTransport.SetAcceptCallback(Manager::HandleAcceptSession, this);
    mDtlsTransport.SetRemoveSessionCallback(Manager::HandleRemoveSession, this);

    SuccessOrExit(error = mDtlsTransport.Open());
    SuccessOrExit(error = mDtlsTransport.Bind(kUdpPort));

    Get<KeyManager>().GetPskc(pskc);
    SuccessOrExit(error = mDtlsTransport.SetPsk(pskc.m8, Pskc::kSize));
    pskc.Clear();

    mIsRunning = true;
    LogInfo("Border Agent start listening on port %u", GetUdpPort());

    Get<TxtData>().Refresh();

exit:
    if (!mIsRunning)
    {
        mDtlsTransport.Close();
    }

    LogWarnOnError(error, "start agent");
}

void Manager::Stop(void)
{
    VerifyOrExit(mIsRunning);

    if (mCommissionerSession != nullptr)
    {
        RevokeRoleIfActiveCommissioner(*mCommissionerSession);
    }

    mDtlsTransport.Close();
    mIsRunning = false;

    LogInfo("Border Agent stopped");

    Get<TxtData>().Refresh();

exit:
    return;
}

uint16_t Manager::GetUdpPort(void) const { return mDtlsTransport.GetUdpPort(); }

void Manager::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        UpdateState();
    }

    VerifyOrExit(mEnabled);

    if (aEvents.ContainsAny(kEventPskcChanged))
    {
        Pskc pskc;

        VerifyOrExit(mIsRunning);

        Get<KeyManager>().GetPskc(pskc);

        // If there is secure session already established, it won't be impacted,
        // new pskc will be applied for next connection.
        SuccessOrExit(mDtlsTransport.SetPsk(pskc.m8, Pskc::kSize));
        pskc.Clear();
    }

exit:
    return;
}

SecureSession *Manager::HandleAcceptSession(void *aContext, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    return static_cast<Manager *>(aContext)->HandleAcceptSession();
}

Manager::CoapDtlsSession *Manager::HandleAcceptSession(void)
{
    return CoapDtlsSession::Allocate(GetInstance(), mDtlsTransport);
}

void Manager::HandleRemoveSession(void *aContext, SecureSession &aSession)
{
    static_cast<Manager *>(aContext)->HandleRemoveSession(aSession);
}

void Manager::HandleRemoveSession(SecureSession &aSession)
{
    CoapDtlsSession &coapSession = static_cast<CoapDtlsSession &>(aSession);

    LogInfo("Deleting session %u", coapSession.GetIndex());

    coapSession.Cleanup();
    coapSession.Free();
}

void Manager::HandleSessionConnected(CoapDtlsSession &aSession)
{
    OT_UNUSED_VARIABLE(aSession);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if (Get<EphemeralKeyManager>().OwnsSession(aSession))
    {
        Get<EphemeralKeyManager>().HandleSessionConnected();
    }
    else
#endif
    {
        mCounters.mPskcSecureSessionSuccesses++;
    }
}

void Manager::HandleSessionDisconnected(CoapDtlsSession &aSession, CoapDtlsSession::ConnectEvent aEvent)
{
    RevokeRoleIfActiveCommissioner(aSession);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if (Get<EphemeralKeyManager>().OwnsSession(aSession))
    {
        Get<EphemeralKeyManager>().HandleSessionDisconnected(aEvent);
    }
    else
#endif
    {
        if (aEvent == CoapDtlsSession::kDisconnectedError)
        {
            mCounters.mPskcSecureSessionFailures++;
        }
    }
}

void Manager::HandleCommissionerPetitionAccepted(CoapDtlsSession &aSession, uint16_t aSessionId)
{
    if (mCommissionerSession != nullptr)
    {
        RevokeRoleIfActiveCommissioner(*mCommissionerSession);
    }

    mCommissionerSession = &aSession;

    Get<Mle::Mle>().GetCommissionerAloc(aSessionId, mCommissionerAloc.GetAddress());
    Get<ThreadNetif>().AddUnicastAddress(mCommissionerAloc);

    IgnoreError(Get<Ip6::Udp>().AddReceiver(mCommissionerUdpReceiver));

    LogInfo("Session %u accepted as active commissioner - Id:0x%04x ALOC:%s", aSession.GetIndex(), aSessionId,
            mCommissionerAloc.GetAddress().ToString().AsCString());

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if (Get<EphemeralKeyManager>().OwnsSession(aSession))
    {
        Get<EphemeralKeyManager>().HandleCommissionerPetitionAccepted();
    }
    else
#endif
    {
        mCounters.mPskcCommissionerPetitions++;
    }
}

void Manager::RevokeRoleIfActiveCommissioner(CoapDtlsSession &aSession)
{
    VerifyOrExit(IsCommissionerSession(aSession));

    LogInfo("Revoked active commissioner role from session %u", aSession.GetIndex());

    IgnoreError(Get<Ip6::Udp>().RemoveReceiver(mCommissionerUdpReceiver));
    Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);

    mCommissionerSession = nullptr;

exit:
    return;
}

bool Manager::HandleUdpReceive(void *aContext, const otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    return static_cast<Manager *>(aContext)->HandleUdpReceive(AsCoreType(aMessage), AsCoreType(aMessageInfo));
}

bool Manager::HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    bool didHandle = false;

    VerifyOrExit(mCommissionerSession != nullptr);
    VerifyOrExit(aMessageInfo.GetSockAddr() == mCommissionerAloc.GetAddress());

    mCommissionerSession->ForwardUdpProxyToCommissioner(aMessage, aMessageInfo);
    didHandle = true;

exit:
    return didHandle;
}

template <> void Manager::HandleTmf<kUriRelayRx>(Coap::Msg &aMsg)
{
    // This is from TMF agent.

    VerifyOrExit(mIsRunning);

    VerifyOrExit(aMsg.IsNonConfirmablePostRequest());

    LogInfo("Received %s from %s", UriToString<kUriRelayRx>(), aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

    VerifyOrExit(mCommissionerSession != nullptr);
    mCommissionerSession->ForwardUdpRelayToCommissioner(aMsg.mMessage);

exit:
    return;
}

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE

Error Manager::SetServiceBaseName(const char *aBaseName)
{
    Error                  error = kErrorNone;
    Dns::Name::LabelBuffer newName;

    VerifyOrExit(StringLength(aBaseName, kBaseServiceNameMaxLen + 1) <= kBaseServiceNameMaxLen,
                 error = kErrorInvalidArgs);

    ConstrcutServiceName(aBaseName, newName);

    VerifyOrExit(!StringMatch(newName, mServiceName));

    UnregisterService();
    IgnoreError(StringCopy(mServiceName, newName));
    RegisterService();

exit:
    return error;
}

const char *Manager::GetServiceName(void)
{
    if (IsServiceNameEmpty())
    {
        ConstrcutServiceName(kDefaultBaseServiceName, mServiceName);
    }

    return mServiceName;
}

void Manager::ConstrcutServiceName(const char *aBaseName, Dns::Name::LabelBuffer &aNameBuffer)
{
    StringWriter writer(aNameBuffer, sizeof(Dns::Name::LabelBuffer));

    writer.Append("%.*s%s", kBaseServiceNameMaxLen, aBaseName, Get<Mac::Mac>().GetExtAddress().ToString().AsCString());
}

void Manager::RegisterService(void)
{
    Dnssd::Service service;
    uint16_t       vendorDataLength;
    uint8_t       *txtDataBuffer;
    uint16_t       txtDataBufferSize;
    uint16_t       txtDataLength;

    VerifyOrExit(IsEnabled());
    VerifyOrExit(Get<Dnssd>().IsReady());

    // Allocate a large enough buffer to fit both the TXT data
    // generated by Border Agent itself and the vendor extra
    // TXT data. The vendor TXT Data is appended at the
    // end.

    vendorDataLength  = Get<TxtData>().GetVendorData().GetLength();
    txtDataBufferSize = kTxtDataMaxSize + vendorDataLength;
    txtDataBuffer     = reinterpret_cast<uint8_t *>(Heap::CAlloc(txtDataBufferSize, sizeof(uint8_t)));
    OT_ASSERT(txtDataBuffer != nullptr);

    SuccessOrAssert(Get<TxtData>().Prepare(txtDataBuffer, txtDataBufferSize, txtDataLength));

    if (vendorDataLength != 0)
    {
        Get<TxtData>().GetVendorData().CopyBytesTo(txtDataBuffer + txtDataLength);
        txtDataLength += vendorDataLength;
    }

    service.Clear();
    service.mServiceInstance = GetServiceName();
    service.mServiceType     = kServiceType;
    service.mPort            = IsRunning() ? GetUdpPort() : kDummyUdpPort;
    service.mTxtData         = txtDataBuffer;
    service.mTxtDataLength   = txtDataLength;

    Get<Dnssd>().RegisterService(service, /* aRequestId */ 0, /* aCallback */ nullptr);

    Heap::Free(txtDataBuffer);

exit:
    return;
}

void Manager::UnregisterService(void)
{
    Dnssd::Service service;

    VerifyOrExit(Get<Dnssd>().IsReady());
    VerifyOrExit(!IsServiceNameEmpty());

    service.Clear();
    service.mServiceInstance = GetServiceName();
    service.mServiceType     = kServiceType;

    Get<Dnssd>().UnregisterService(service, /* aRequestId */ 0, /* aCallback */ nullptr);

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE

#if OPENTHREAD_CONFIG_BORDER_AGENT_COMMISSIONER_EVICTION_API_ENABLE

Error Manager::EvictActiveCommissioner(void)
{
    Error                   error = kErrorNone;
    uint16_t                sessionId;
    uint16_t                baRloc16;
    Tmf::MessageInfo        messageInfo(GetInstance());
    OwnedPtr<Coap::Message> message;

    SuccessOrExit(error = Get<NetworkData::Leader>().FindBorderAgentRloc(baRloc16));
    SuccessOrExit(error = Get<NetworkData::Leader>().FindCommissioningSessionId(sessionId));

    message.Reset(Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriLeaderKeepAlive));
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<StateTlv>(*message, StateTlv::kReject));
    SuccessOrExit(error = Tlv::Append<CommissionerSessionIdTlv>(*message, sessionId));

    messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
    messageInfo.SetSockPortToTmf();

    error = Get<Tmf::Agent>().SendMessage(message.PassOwnership(), messageInfo);

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_COMMISSIONER_EVICTION_API_ENABLE

//----------------------------------------------------------------------------------------------------------------------
// Manager::SessionIterator

void Manager::SessionIterator::Init(Instance &aInstance)
{
    SetSession(static_cast<CoapDtlsSession *>(aInstance.Get<Manager>().mDtlsTransport.GetSessions().GetHead()));
    SetInitTime(aInstance.Get<UptimeTracker>().GetUptime());
}

Error Manager::SessionIterator::GetNextSessionInfo(SessionInfo &aSessionInfo)
{
    Error            error   = kErrorNone;
    CoapDtlsSession *session = GetSession();

    VerifyOrExit(session != nullptr, error = kErrorNotFound);

    SetSession(static_cast<CoapDtlsSession *>(session->GetNext()));

    aSessionInfo.mPeerSockAddr.mAddress = session->GetMessageInfo().GetPeerAddr();
    aSessionInfo.mPeerSockAddr.mPort    = session->GetMessageInfo().GetPeerPort();
    aSessionInfo.mIsConnected           = session->IsConnected();
    aSessionInfo.mIsCommissioner        = session->IsActiveCommissioner();
    aSessionInfo.mLifetime              = GetInitTime() - session->GetAllocationTime();

exit:
    return error;
}

//----------------------------------------------------------------------------------------------------------------------
// `Manager::CoapDtlsSession

Manager::CoapDtlsSession::CoapDtlsSession(Instance &aInstance, Dtls::Transport &aDtlsTransport)
    : Coap::SecureSession(aInstance, aDtlsTransport)
    , mTimer(aInstance, HandleTimer, this)
    , mAllocationTime(aInstance.Get<UptimeTracker>().GetUptime())
    , mIndex(aInstance.Get<Manager>().GetNextSessionIndex())
{
    SetResourceHandler(&HandleResource);
    SetConnectCallback(&HandleConnected, this);

    LogInfo("Allocating session %u", mIndex);
}

Error Manager::CoapDtlsSession::SendMessage(OwnedPtr<Coap::Message> aMessage)
{
    Error error;

    // On success the ownership is transferred.
    SuccessOrExit(error = Coap::SecureSession::SendMessage(*aMessage));
    aMessage.Release();

exit:
    return error;
}

bool Manager::CoapDtlsSession::IsActiveCommissioner(void) const { return Get<Manager>().IsCommissionerSession(*this); }

void Manager::CoapDtlsSession::Cleanup(void)
{
    while (!mForwardContexts.IsEmpty())
    {
        ForwardContext *forwardContext = mForwardContexts.Pop();

        IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleLeaderResponseToFwdTmf, forwardContext));
    }

    mTimer.Stop();

    Get<Manager>().RevokeRoleIfActiveCommissioner(*this);

    Coap::SecureSession::Cleanup();
}

bool Manager::CoapDtlsSession::HandleResource(CoapBase &aCoapBase, const char *aUriPath, Coap::Msg &aMsg)
{
    return static_cast<CoapDtlsSession &>(aCoapBase).HandleResource(aUriPath, aMsg);
}

bool Manager::CoapDtlsSession::HandleResource(const char *aUriPath, Coap::Msg &aMsg)

{
    bool didHandle = true;
    Uri  uri       = UriFromPath(aUriPath);

    switch (uri)
    {
    case kUriCommissionerPetition:
        Log<kUriCommissionerPetition>(kReceive);
        IgnoreError(ForwardToLeader(aMsg, kUriLeaderPetition));
        break;
    case kUriCommissionerKeepAlive:
        HandleTmfCommissionerKeepAlive(aMsg);
        break;
    case kUriRelayTx:
        HandleTmfRelayTx(aMsg);
        break;
    case kUriCommissionerGet:
    case kUriActiveGet:
    case kUriPendingGet:
        HandleTmfDatasetGet(aMsg.mMessage, uri);
        break;
    case kUriProxyTx:
        HandleTmfProxyTx(aMsg);
        break;
    default:
        didHandle = false;
        break;
    }

    return didHandle;
}

void Manager::CoapDtlsSession::HandleConnected(ConnectEvent aEvent, void *aContext)
{
    static_cast<CoapDtlsSession *>(aContext)->HandleConnected(aEvent);
}

void Manager::CoapDtlsSession::HandleConnected(ConnectEvent aEvent)
{
    if (aEvent == kConnected)
    {
        LogInfo("Session %u connected", mIndex);
        mTimer.Start(kKeepAliveTimeout);
        Get<Manager>().HandleSessionConnected(*this);
    }
    else
    {
        LogInfo("Session %u disconnected", mIndex);
        Get<Manager>().HandleSessionDisconnected(*this, aEvent);
    }
}

void Manager::CoapDtlsSession::HandleTmfCommissionerKeepAlive(Coap::Msg &aMsg)
{
    VerifyOrExit(IsActiveCommissioner());

    Log<kUriCommissionerKeepAlive>(kReceive);

    SuccessOrExit(ForwardToLeader(aMsg, kUriLeaderKeepAlive));
    mTimer.Start(kKeepAliveTimeout);
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    if (Get<EphemeralKeyManager>().OwnsSession(*this))
    {
        Get<HistoryTracker::Local>().RecordEpskcEvent(HistoryTracker::Local::kEpskcKeepAlive);
    }
#endif

exit:
    return;
}

Error Manager::CoapDtlsSession::ForwardToLeader(const Coap::Msg &aMsg, Uri aUri)
{
    Error                    error = kErrorNone;
    OwnedPtr<ForwardContext> forwardContext;
    Tmf::MessageInfo         messageInfo(GetInstance());
    OwnedPtr<Coap::Message>  message;
    OffsetRange              offsetRange;
    Coap::Token              token;

    switch (aUri)
    {
    case kUriLeaderPetition:
    case kUriLeaderKeepAlive:
        break;
    default:
        OT_ASSERT(false);
    }

    SuccessOrExit(error = SendAck(aMsg));

    forwardContext.Reset(ForwardContext::Allocate(*this, aMsg.mMessage, aUri));
    VerifyOrExit(!forwardContext.IsNull(), error = kErrorNoBufs);

    message.Reset(Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(aUri));
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    offsetRange.InitFromMessageOffsetToEnd(aMsg.mMessage);
    SuccessOrExit(error = message->AppendBytesFromMessage(aMsg.mMessage, offsetRange));

    messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
    messageInfo.SetSockPortToTmf();

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(message.PassOwnership(), messageInfo,
                                                        HandleLeaderResponseToFwdTmf, forwardContext.Get()));

    // Release the ownership of `forwardContext` since `SendMessage()`
    // will own it. We take back ownership when the callback
    // `HandleLeaderResponseToFwdTmf()` is invoked.

    mForwardContexts.Push(*forwardContext.Release());

    switch (aUri)
    {
    case kUriLeaderPetition:
        Log<kUriLeaderPetition>(kForward, " to leader");
        break;
    case kUriLeaderKeepAlive:
        Log<kUriLeaderKeepAlive>(kForward, " to leader");
        break;
    default:
        break;
    }

exit:
    LogWarnOnError(error, "forward to leader");

    if ((error != kErrorNone) && (aMsg.mMessage.ReadToken(token) == kErrorNone))
    {
        SendErrorMessage(error, token);
    }

    return error;
}

void Manager::CoapDtlsSession::HandleLeaderResponseToFwdTmf(void                *aContext,
                                                            otMessage           *aMessage,
                                                            const otMessageInfo *aMessageInfo,
                                                            otError              aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    OwnedPtr<ForwardContext> forwardContext(static_cast<ForwardContext *>(aContext));

    forwardContext->mSession.HandleLeaderResponseToFwdTmf(*forwardContext.Get(), AsCoapMessagePtr(aMessage), aResult);
}

void Manager::CoapDtlsSession::HandleLeaderResponseToFwdTmf(const ForwardContext &aForwardContext,
                                                            const Coap::Message  *aResponse,
                                                            Error                 aResult)
{
    OwnedPtr<Coap::Message> forwardMessage;
    Error                   error;

    IgnoreError(mForwardContexts.Remove(aForwardContext));

    switch (aForwardContext.mUri)
    {
    case kUriLeaderPetition:
        Log<kUriLeaderPetition>(kReceive, " response from leader");
        break;
    case kUriLeaderKeepAlive:
        Log<kUriLeaderKeepAlive>(kReceive, " response from leader");
        break;
    default:
        break;
    }

    SuccessOrExit(error = aResult);

    forwardMessage.Reset(NewPriorityMessage());
    VerifyOrExit(forwardMessage != nullptr, error = kErrorNoBufs);

    if (aResponse->ReadCode() == Coap::kCodeChanged)
    {
        uint8_t state;

        SuccessOrExit(error = Tlv::Find<StateTlv>(*aResponse, state));

        switch (state)
        {
        case StateTlv::kAccept:
            if (aForwardContext.mUri == kUriLeaderPetition)
            {
                uint16_t sessionId;

                SuccessOrExit(error = Tlv::Find<CommissionerSessionIdTlv>(*aResponse, sessionId));
                Get<Manager>().HandleCommissionerPetitionAccepted(*this, sessionId);
            }

            break;

        case StateTlv::kReject:
            Get<Manager>().RevokeRoleIfActiveCommissioner(*this);
            break;

        default:
            break;
        }
    }

    SuccessOrExit(error =
                      forwardMessage->Init(Coap::kTypeNonConfirmable, static_cast<Coap::Code>(aResponse->ReadCode())));
    SuccessOrExit(error = forwardMessage->WriteToken(aForwardContext.mToken));

    if (aResponse->GetLength() > aResponse->GetOffset())
    {
        SuccessOrExit(error = forwardMessage->AppendPayloadMarker());
    }

    SuccessOrExit(error = ForwardToCommissioner(forwardMessage.PassOwnership(), *aResponse));

exit:
    if (error != kErrorNone)
    {
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
        const char *uriString = "Unknown";

        switch (aForwardContext.mUri)
        {
        case kUriLeaderPetition:
            uriString = UriToString<kUriLeaderPetition>();
            break;
        case kUriLeaderKeepAlive:
            uriString = UriToString<kUriLeaderKeepAlive>();
            break;
        default:
            break;
        }

        LogWarn("Forwarded %s failed - session %u, error:%s", uriString, mIndex, ErrorToString(error));
#endif

        SendErrorMessage(error, aForwardContext.mToken);
    }
}

void Manager::CoapDtlsSession::ForwardUdpProxyToCommissioner(const Message          &aMessage,
                                                             const Ip6::MessageInfo &aMessageInfo)
{
    Error                     error = kErrorNone;
    OwnedPtr<Coap::Message>   message;
    ExtendedTlv               extTlv;
    UdpEncapsulationTlvHeader udpEncapHeader;
    OffsetRange               offsetRange;

    VerifyOrExit(aMessage.GetLength() > 0);

    message.Reset(NewPriorityNonConfirmablePostMessage(kUriProxyRx));
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    offsetRange.InitFromMessageOffsetToEnd(aMessage);

    extTlv.SetType(Tlv::kUdpEncapsulation);
    extTlv.SetLength(sizeof(UdpEncapsulationTlvHeader) + offsetRange.GetLength());

    udpEncapHeader.SetSourcePort(aMessageInfo.GetPeerPort());
    udpEncapHeader.SetDestinationPort(aMessageInfo.GetSockPort());

    SuccessOrExit(error = message->Append(extTlv));
    SuccessOrExit(error = message->Append(udpEncapHeader));
    SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, offsetRange));

    SuccessOrExit(error = Tlv::Append<Ip6AddressTlv>(*message, aMessageInfo.GetPeerAddr()));

    SuccessOrExit(error = SendMessage(message.PassOwnership()));

    Log<kUriProxyRx>(kForward);

exit:
    LogWarnOnError(error, "forward UDP proxy");
}

void Manager::CoapDtlsSession::ForwardUdpRelayToCommissioner(const Message &aMessage)
{
    OwnedPtr<Coap::Message> forwardMessage;
    Error                   error = kErrorNone;

    forwardMessage.Reset(NewPriorityNonConfirmablePostMessage(kUriRelayRx));
    VerifyOrExit(forwardMessage != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = ForwardToCommissioner(forwardMessage.PassOwnership(), aMessage));

    Log<kUriRelayRx>(kForward);

exit:
    LogWarnOnError(error, "forward UDP relay");
}

Error Manager::CoapDtlsSession::ForwardToCommissioner(OwnedPtr<Coap::Message> aForwardMessage, const Message &aMessage)
{
    Error       error = kErrorNone;
    OffsetRange offsetRange;

    offsetRange.InitFromMessageOffsetToEnd(aMessage);
    SuccessOrExit(error = aForwardMessage->AppendBytesFromMessage(aMessage, offsetRange));

    error = SendMessage(aForwardMessage.PassOwnership());

exit:
    return error;
}

void Manager::CoapDtlsSession::SendErrorMessage(Error aError, const Coap::Token &aToken)
{
    Error                   error = kErrorNone;
    OwnedPtr<Coap::Message> message;
    Coap::Message::Code     code;

    message.Reset(NewPriorityMessage());
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    code = (aError == kErrorParse) ? Coap::kCodeBadRequest : Coap::kCodeInternalError;

    SuccessOrExit(error = message->Init(Coap::kTypeNonConfirmable, code));
    SuccessOrExit(error = message->WriteToken(aToken));

    SuccessOrExit(error = SendMessage(message.PassOwnership()));

exit:
    LogWarnOnError(error, "send error CoAP message");
}

void Manager::CoapDtlsSession::HandleTmfProxyTx(Coap::Msg &aMsg)
{
    Error                     error = kErrorNone;
    OwnedPtr<Message>         message;
    Ip6::MessageInfo          messageInfo;
    OffsetRange               offsetRange;
    UdpEncapsulationTlvHeader udpEncapHeader;

    Log<kUriProxyTx>(kReceive);

    VerifyOrExit(IsActiveCommissioner(), error = kErrorInvalidState);

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aMsg.mMessage, Tlv::kUdpEncapsulation, offsetRange));

    SuccessOrExit(error = aMsg.mMessage.Read(offsetRange, udpEncapHeader));
    offsetRange.AdvanceOffset(sizeof(UdpEncapsulationTlvHeader));

    VerifyOrExit(udpEncapHeader.GetSourcePort() > 0 && udpEncapHeader.GetDestinationPort() > 0, error = kErrorDrop);

    message.Reset(Get<Ip6::Udp>().NewMessage());
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = message->AppendBytesFromMessage(aMsg.mMessage, offsetRange));

    messageInfo.SetSockPort(udpEncapHeader.GetSourcePort());
    messageInfo.SetSockAddr(Get<Manager>().GetCommissionerAloc());
    messageInfo.SetPeerPort(udpEncapHeader.GetDestinationPort());

    SuccessOrExit(error = Tlv::Find<Ip6AddressTlv>(aMsg.mMessage, messageInfo.GetPeerAddr()));

    // On success the message ownership is transferred.
    SuccessOrExit(error = Get<Ip6::Udp>().SendDatagram(*message, messageInfo));
    message.Release();

    LogInfo("Sent proxy UDP to %s", messageInfo.GetPeerAddr().ToString().AsCString());

exit:
    LogWarnOnError(error, "send proxy stream");
}

void Manager::CoapDtlsSession::HandleTmfRelayTx(Coap::Msg &aMsg)
{
    Error                   error = kErrorNone;
    uint16_t                joinerRouterRloc;
    OwnedPtr<Coap::Message> message;
    Tmf::MessageInfo        messageInfo(GetInstance());
    OffsetRange             offsetRange;

    VerifyOrExit(aMsg.IsNonConfirmablePostRequest());

    VerifyOrExit(IsActiveCommissioner(), error = kErrorInvalidState);

    Log<kUriRelayTx>(kReceive);

    SuccessOrExit(error = Tlv::Find<JoinerRouterLocatorTlv>(aMsg.mMessage, joinerRouterRloc));

    message.Reset(Get<Tmf::Agent>().NewPriorityNonConfirmablePostMessage(kUriRelayTx));
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    offsetRange.InitFromMessageOffsetToEnd(aMsg.mMessage);
    SuccessOrExit(error = message->AppendBytesFromMessage(aMsg.mMessage, offsetRange));

    messageInfo.SetSockAddrToRlocPeerAddrTo(joinerRouterRloc);
    messageInfo.SetSockPortToTmf();

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(message.PassOwnership(), messageInfo));

    LogInfo("Forward %s to joiner router 0x%04x", UriToString<kUriRelayTx>(), joinerRouterRloc);

exit:
    LogWarnOnError(error, "forward to joiner router");
}

void Manager::CoapDtlsSession::HandleTmfDatasetGet(Coap::Message &aMessage, Uri aUri)
{
    Error                   error = kErrorNone;
    OwnedPtr<Coap::Message> response;

    // When processing `MGMT_GET` request directly on Border Agent,
    // the Security Policy flags (O-bit) should be ignored to allow
    // the commissioner candidate to get the full Operational Dataset.

    switch (aUri)
    {
    case kUriActiveGet:
        Log<kUriActiveGet>(kReceive);
        response.Reset(
            Get<ActiveDatasetManager>().ProcessGetRequest(aMessage, DatasetManager::kIgnoreSecurityPolicyFlags));
        Get<Manager>().mCounters.mMgmtActiveGets++;
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
        if (Get<EphemeralKeyManager>().OwnsSession(*this))
        {
            Get<HistoryTracker::Local>().RecordEpskcEvent(HistoryTracker::Local::kEpskcRetrievedActiveDataset);
        }
#endif
        break;

    case kUriPendingGet:
        Log<kUriPendingGet>(kReceive);
        response.Reset(
            Get<PendingDatasetManager>().ProcessGetRequest(aMessage, DatasetManager::kIgnoreSecurityPolicyFlags));
        Get<Manager>().mCounters.mMgmtPendingGets++;
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
        if (Get<EphemeralKeyManager>().OwnsSession(*this))
        {
            Get<HistoryTracker::Local>().RecordEpskcEvent(HistoryTracker::Local::kEpskcRetrievedPendingDataset);
        }
#endif
        break;

    case kUriCommissionerGet:
        Log<kUriCommissionerGet>(kReceive);
        response.Reset(Get<NetworkData::Leader>().ProcessCommissionerGetRequest(aMessage));
        break;

    default:
        break;
    }

    VerifyOrExit(response != nullptr, error = kErrorParse);

    SuccessOrExit(error = SendMessage(response.PassOwnership()));

    switch (aUri)
    {
    case kUriActiveGet:
        Log<kUriActiveGet>(kSend, " response");
        break;
    case kUriPendingGet:
        Log<kUriPendingGet>(kSend, " response");
        break;
    case kUriCommissionerGet:
        Log<kUriCommissionerGet>(kSend, " response");
        break;
    default:
        break;
    }

exit:
    LogWarnOnError(error, "send Active/Pending/CommissionerGet response");
}

void Manager::CoapDtlsSession::HandleTimer(Timer &aTimer)
{
    static_cast<CoapDtlsSession *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleTimer();
}

void Manager::CoapDtlsSession::HandleTimer(void)
{
    if (IsConnected())
    {
        LogInfo("Session %u timed out - disconnecting", mIndex);
        DisconnectTimeout();
    }
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void Manager::CoapDtlsSession::LogUri(Action aAction, const char *aUriString, const char *aTxt)
{
#define ActionMapList(_)   \
    _(kReceive, "Receive") \
    _(kSend, "Send")       \
    _(kForward, "Forward")

    DefineEnumStringArray(ActionMapList);

    LogInfo("%s %s%s - session %u", kStrings[aAction], aUriString, aTxt, mIndex);
}

#endif

//----------------------------------------------------------------------------------------------------------------------
// `Manager::CoapDtlsSession::ForwardContext`

Manager::CoapDtlsSession::ForwardContext::ForwardContext(CoapDtlsSession     &aSession,
                                                         const Coap::Message &aMessage,
                                                         Uri                  aUri)
    : mSession(aSession)
    , mUri(aUri)
{
    if (aMessage.ReadToken(mToken) != kErrorNone)
    {
        mToken.Clear();
    }
}

} // namespace BorderAgent
} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
