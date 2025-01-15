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

namespace ot {
namespace MeshCoP {

RegisterLogModule("BorderAgent");

//----------------------------------------------------------------------------------------------------------------------
// `BorderAgent`

BorderAgent::BorderAgent(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStateStopped)
    , mDtlsTransport(aInstance, kNoLinkSecurity)
    , mCoapDtlsSession(nullptr)
#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    , mIdInitialized(false)
#endif
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    , mIsEphemeralKeyFeatureEnabled(OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_FEATURE_ENABLED_BY_DEFAULT)
    , mUsingEphemeralKey(false)
    , mDidConnectWithEphemeralKey(false)
    , mOldUdpPort(0)
    , mEphemeralKeyTimer(aInstance)
    , mEphemeralKeyTask(aInstance)
#endif
{
    ClearAllBytes(mCounters);
}

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
Error BorderAgent::GetId(Id &aId)
{
    Error error = kErrorNone;

    if (mIdInitialized)
    {
        aId = mId;
        ExitNow();
    }

    if (Get<Settings>().Read<Settings::BorderAgentId>(mId) != kErrorNone)
    {
        Random::NonCrypto::Fill(mId);
        SuccessOrExit(error = Get<Settings>().Save<Settings::BorderAgentId>(mId));
    }

    mIdInitialized = true;
    aId            = mId;

exit:
    return error;
}

Error BorderAgent::SetId(const Id &aId)
{
    Error error = kErrorNone;

    SuccessOrExit(error = Get<Settings>().Save<Settings::BorderAgentId>(aId));
    mId            = aId;
    mIdInitialized = true;

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE

Error BorderAgent::Start(uint16_t aUdpPort)
{
    Error error;
    Pskc  pskc;

    Get<KeyManager>().GetPskc(pskc);
    error = Start(aUdpPort, pskc.m8, Pskc::kSize);
    pskc.Clear();

    return error;
}

Error BorderAgent::Start(uint16_t aUdpPort, const uint8_t *aPsk, uint8_t aPskLength)
{
    Error error = kErrorNone;

    VerifyOrExit(mState == kStateStopped);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if (mUsingEphemeralKey)
    {
        SuccessOrExit(error = mDtlsTransport.SetMaxConnectionAttempts(kMaxEphemeralKeyConnectionAttempts,
                                                                      HandleDtlsTransportClosed, this));
    }
#endif

    mDtlsTransport.SetAcceptCallback(HandleAcceptSession, this);
    mDtlsTransport.SetRemoveSessionCallback(HandleRemoveSession, this);

    SuccessOrExit(error = mDtlsTransport.Open());
    SuccessOrExit(error = mDtlsTransport.Bind(aUdpPort));

    SuccessOrExit(error = mDtlsTransport.SetPsk(aPsk, aPskLength));

    mState = kStateStarted;

    LogInfo("Border Agent start listening on port %u", GetUdpPort());

exit:
    LogWarnOnError(error, "start agent");
    return error;
}

void BorderAgent::Stop(void)
{
    VerifyOrExit(mState != kStateStopped);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if (mUsingEphemeralKey)
    {
        mUsingEphemeralKey = false;
        mEphemeralKeyTimer.Stop();
        mEphemeralKeyTask.Post();
    }
#endif

    mDtlsTransport.Close();

    mState = kStateStopped;
    LogInfo("Border Agent stopped");

exit:
    return;
}

void BorderAgent::Disconnect(void)
{
    VerifyOrExit(mState == kStateConnected || mState == kStateAccepted);
    VerifyOrExit(mCoapDtlsSession != nullptr);

    mCoapDtlsSession->Disconnect();

exit:
    return;
}

uint16_t BorderAgent::GetUdpPort(void) const { return mDtlsTransport.GetUdpPort(); }

void BorderAgent::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        if (Get<Mle::MleRouter>().IsAttached())
        {
            Start();
        }
        else
        {
            Stop();
        }
    }

    if (aEvents.ContainsAny(kEventPskcChanged))
    {
        VerifyOrExit(mState != kStateStopped);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
        // No-op if Ephemeralkey mode is activated, new pskc will be applied
        // when Ephemeralkey mode is deactivated.
        VerifyOrExit(!mUsingEphemeralKey);
#endif

        {
            Pskc pskc;
            Get<KeyManager>().GetPskc(pskc);

            // If there is secure session already established, it won't be impacted,
            // new pskc will be applied for next connection.
            SuccessOrExit(mDtlsTransport.SetPsk(pskc.m8, Pskc::kSize));
            pskc.Clear();
        }
    }

exit:
    return;
}

SecureSession *BorderAgent::HandleAcceptSession(void *aContext, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    return static_cast<BorderAgent *>(aContext)->HandleAcceptSession();
}

BorderAgent::CoapDtlsSession *BorderAgent::HandleAcceptSession(void)
{
    CoapDtlsSession *session = nullptr;

    VerifyOrExit(mCoapDtlsSession == nullptr);

    session = CoapDtlsSession::Allocate(GetInstance(), mDtlsTransport);
    VerifyOrExit(session != nullptr);

    mCoapDtlsSession = session;

exit:
    return session;
}

void BorderAgent::HandleRemoveSession(void *aContext, SecureSession &aSesssion)
{
    static_cast<BorderAgent *>(aContext)->HandleRemoveSession(aSesssion);
}

void BorderAgent::HandleRemoveSession(SecureSession &aSesssion)
{
    CoapDtlsSession &coapSession = static_cast<CoapDtlsSession &>(aSesssion);

    coapSession.Cleanup();
    coapSession.Free();
    mCoapDtlsSession = nullptr;
}

void BorderAgent::HandleSessionConnected(CoapDtlsSession &aSesssion)
{
    OT_UNUSED_VARIABLE(aSesssion);

    mState = kStateConnected;

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if (mUsingEphemeralKey)
    {
        mDidConnectWithEphemeralKey = true;
        mCounters.mEpskcSecureSessionSuccesses++;
        mEphemeralKeyTask.Post();
    }
    else
#endif
    {
        mCounters.mPskcSecureSessionSuccesses++;
    }
}

void BorderAgent::HandleSessionDisconnected(CoapDtlsSession &aSesssion, CoapDtlsSession::ConnectEvent aEvent)
{
    OT_UNUSED_VARIABLE(aSesssion);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if (mUsingEphemeralKey)
    {
        if (mDidConnectWithEphemeralKey)
        {
            RestartAfterRemovingEphemeralKey();
        }

        if (aEvent == CoapDtlsSession::kDisconnectedError)
        {
            mCounters.mEpskcSecureSessionFailures++;
        }
        else if (aEvent == CoapDtlsSession::kDisconnectedPeerClosed)
        {
            mCounters.mEpskcDeactivationDisconnects++;
        }
    }
    else
#endif
    {
        mState = kStateStarted;

        if (aEvent == CoapDtlsSession::kDisconnectedError)
        {
            mCounters.mPskcSecureSessionFailures++;
        }
    }
}

void BorderAgent::HandleCommissionerPetitionAccepted(CoapDtlsSession &aSesssion)
{
    OT_UNUSED_VARIABLE(aSesssion);

    mState = kStateAccepted;

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if (mUsingEphemeralKey)
    {
        mCounters.mEpskcCommissionerPetitions++;
    }
    else
#endif
    {
        mCounters.mPskcCommissionerPetitions++;
    }
}

Coap::Message::Code BorderAgent::CoapCodeFromError(Error aError)
{
    Coap::Message::Code code;

    switch (aError)
    {
    case kErrorNone:
        code = Coap::kCodeChanged;
        break;

    case kErrorParse:
        code = Coap::kCodeBadRequest;
        break;

    default:
        code = Coap::kCodeInternalError;
        break;
    }

    return code;
}

template <> void BorderAgent::HandleTmf<kUriRelayRx>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    // This is from TMF agent.

    OT_UNUSED_VARIABLE(aMessageInfo);

    Coap::Message *message = nullptr;
    Error          error   = kErrorNone;

    VerifyOrExit(mState != kStateStopped);

    VerifyOrExit(aMessage.IsNonConfirmablePostRequest(), error = kErrorDrop);

    VerifyOrExit(mCoapDtlsSession != nullptr);

    message = mCoapDtlsSession->NewPriorityNonConfirmablePostMessage(kUriRelayRx);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = mCoapDtlsSession->ForwardToCommissioner(*message, aMessage));
    LogInfo("Sent to commissioner on RelayRx (c/rx)");

exit:
    FreeMessageOnError(message, error);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Ephemeral Key

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

Error BorderAgent::SetEphemeralKey(const char *aKeyString, uint32_t aTimeout, uint16_t aUdpPort)
{
    Error    error  = kErrorNone;
    uint16_t length = StringLength(aKeyString, kMaxEphemeralKeyLength + 1);

    VerifyOrExit(mIsEphemeralKeyFeatureEnabled, error = kErrorNotCapable);
    VerifyOrExit(mState == kStateStarted, error = kErrorInvalidState);
    VerifyOrExit((length >= kMinEphemeralKeyLength) && (length <= kMaxEphemeralKeyLength), error = kErrorInvalidArgs);

    if (!mUsingEphemeralKey)
    {
        mOldUdpPort = GetUdpPort();
    }

    Stop();

    // We set the `mUsingEphemeralKey` before `Start()` since
    // callbacks (like `HandleConnected()`) may be invoked from
    // `Start()` itself.

    mUsingEphemeralKey          = true;
    mDidConnectWithEphemeralKey = false;

    error = Start(aUdpPort, reinterpret_cast<const uint8_t *>(aKeyString), static_cast<uint8_t>(length));

    if (error != kErrorNone)
    {
        mUsingEphemeralKey = false;
        IgnoreError(Start(mOldUdpPort));
        mCounters.mEpskcStartSecureSessionErrors++;
        ExitNow();
    }

    mEphemeralKeyTask.Post();

    if (aTimeout == 0)
    {
        aTimeout = kDefaultEphemeralKeyTimeout;
    }

    aTimeout = Min(aTimeout, kMaxEphemeralKeyTimeout);

    mEphemeralKeyTimer.Start(aTimeout);
    mCounters.mEpskcActivations++;

    LogInfo("Allow ephemeral key for %lu msec on port %u", ToUlong(aTimeout), GetUdpPort());

exit:
    switch (error)
    {
    case kErrorInvalidState:
        mCounters.mEpskcInvalidBaStateErrors++;
        break;
    case kErrorInvalidArgs:
        mCounters.mEpskcInvalidArgsErrors++;
        break;
    default:
        break;
    }

    return error;
}

void BorderAgent::ClearEphemeralKey(void)
{
    VerifyOrExit(mUsingEphemeralKey);

    LogInfo("Clearing ephemeral key");

    mCounters.mEpskcDeactivationClears++;

    switch (mState)
    {
    case kStateStarted:
        RestartAfterRemovingEphemeralKey();
        break;

    case kStateStopped:
    case kStateConnected:
    case kStateAccepted:
        // If a commissioner connection is currently active, we'll
        // wait for it to disconnect or for the ephemeral key timeout
        // or `kKeepAliveTimeout` to expire before removing the key
        // and restarting the agent.
        break;
    }

exit:
    return;
}

void BorderAgent::HandleEphemeralKeyTimeout(void)
{
    LogInfo("Ephemeral key timed out");
    mCounters.mEpskcDeactivationTimeouts++;
    RestartAfterRemovingEphemeralKey();
}

void BorderAgent::InvokeEphemeralKeyCallback(void) { mEphemeralKeyCallback.InvokeIfSet(); }

void BorderAgent::RestartAfterRemovingEphemeralKey(void)
{
    LogInfo("Removing ephemeral key and restarting agent");

    Stop();
    IgnoreError(Start(mOldUdpPort));
}

void BorderAgent::HandleDtlsTransportClosed(void *aContext)
{
    reinterpret_cast<BorderAgent *>(aContext)->HandleDtlsTransportClosed();
}

void BorderAgent::HandleDtlsTransportClosed(void)
{
    LogInfo("Reached max allowed connection attempts with ephemeral key");
    mCounters.mEpskcDeactivationMaxAttempts++;
    RestartAfterRemovingEphemeralKey();
}

void BorderAgent::SetEphemeralKeyFeatureEnabled(bool aEnabled)
{
    VerifyOrExit(mIsEphemeralKeyFeatureEnabled != aEnabled);
    mIsEphemeralKeyFeatureEnabled = aEnabled;

    if (!mIsEphemeralKeyFeatureEnabled)
    {
        // If there is an active session connected with ephemeral key, we disconnect
        // the session.
        if (mUsingEphemeralKey)
        {
            Disconnect();
        }
        ClearEphemeralKey();
    }

    // TODO: Update MeshCoP service after new module is added.

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

//----------------------------------------------------------------------------------------------------------------------
// `BorderAgent::CoapDtlsSession

BorderAgent::CoapDtlsSession::CoapDtlsSession(Instance &aInstance, Dtls::Transport &aDtlsTransport)
    : Coap::SecureSession(aInstance, aDtlsTransport)
    , mIsActiveCommissioner(false)
    , mTimer(aInstance, HandleTimer, this)
    , mUdpReceiver(HandleUdpReceive, this)
{
    mCommissionerAloc.InitAsThreadOriginMeshLocal();

    SetResourceHandler(&HandleResource);
    SetConnectCallback(&HandleConnected, this);
}

void BorderAgent::CoapDtlsSession::Cleanup(void)
{
    mTimer.Stop();
    IgnoreError(Get<Ip6::Udp>().RemoveReceiver(mUdpReceiver));
    Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);

    Coap::SecureSession::Cleanup();
}

bool BorderAgent::CoapDtlsSession::HandleResource(CoapBase               &aCoapBase,
                                                  const char             *aUriPath,
                                                  Coap::Message          &aMessage,
                                                  const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<CoapDtlsSession &>(aCoapBase).HandleResource(aUriPath, aMessage, aMessageInfo);
}

bool BorderAgent::CoapDtlsSession::HandleResource(const char             *aUriPath,
                                                  Coap::Message          &aMessage,
                                                  const Ip6::MessageInfo &aMessageInfo)
{
    bool didHandle = true;
    Uri  uri       = UriFromPath(aUriPath);

    switch (uri)
    {
    case kUriCommissionerPetition:
        IgnoreError(ForwardToLeader(aMessage, aMessageInfo, kUriLeaderPetition));
        break;
    case kUriCommissionerKeepAlive:
        HandleTmfCommissionerKeepAlive(aMessage, aMessageInfo);
        break;
    case kUriRelayTx:
        HandleTmfRelayTx(aMessage);
        break;
    case kUriCommissionerGet:
    case kUriActiveGet:
    case kUriPendingGet:
        HandleTmfDatasetGet(aMessage, uri);
        break;
    case kUriProxyTx:
        HandleTmfProxyTx(aMessage);
        break;
    default:
        didHandle = false;
        break;
    }

    return didHandle;
}

void BorderAgent::CoapDtlsSession::HandleConnected(ConnectEvent aEvent, void *aContext)
{
    static_cast<CoapDtlsSession *>(aContext)->HandleConnected(aEvent);
}

void BorderAgent::CoapDtlsSession::HandleConnected(ConnectEvent aEvent)
{
    if (aEvent == kConnected)
    {
        LogInfo("SecureSession connected");
        mTimer.Start(kKeepAliveTimeout);
        Get<BorderAgent>().HandleSessionConnected(*this);
    }
    else
    {
        LogInfo("SecureSession disconnected");
        IgnoreError(Get<Ip6::Udp>().RemoveReceiver(mUdpReceiver));
        Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);

        Get<BorderAgent>().HandleSessionDisconnected(*this, aEvent);
    }
}

void BorderAgent::CoapDtlsSession::HandleTmfCommissionerKeepAlive(Coap::Message          &aMessage,
                                                                  const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(mIsActiveCommissioner);
    SuccessOrExit(ForwardToLeader(aMessage, aMessageInfo, kUriLeaderKeepAlive));
    mTimer.Start(kKeepAliveTimeout);

exit:
    return;
}

Error BorderAgent::CoapDtlsSession::ForwardToLeader(const Coap::Message    &aMessage,
                                                    const Ip6::MessageInfo &aMessageInfo,
                                                    Uri                     aUri)
{
    Error                    error = kErrorNone;
    OwnedPtr<ForwardContext> forwardContext;
    Tmf::MessageInfo         messageInfo(GetInstance());
    Coap::Message           *message  = nullptr;
    bool                     petition = false;
    bool                     separate = false;
    OffsetRange              offsetRange;

    VerifyOrExit(Get<BorderAgent>().mState != kStateStopped);

    switch (aUri)
    {
    case kUriLeaderPetition:
        petition = true;
        separate = true;
        break;
    case kUriLeaderKeepAlive:
        separate = true;
        break;
    default:
        break;
    }

    if (separate)
    {
        SuccessOrExit(error = SendAck(aMessage, aMessageInfo));
    }

    forwardContext.Reset(ForwardContext::Allocate(*this, aMessage, petition, separate));
    VerifyOrExit(!forwardContext.IsNull(), error = kErrorNoBufs);

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(aUri);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    offsetRange.InitFromMessageOffsetToEnd(aMessage);
    SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, offsetRange));

    messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
    messageInfo.SetSockPortToTmf();

    SuccessOrExit(error =
                      Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleCoapResponse, forwardContext.Get()));

    // Release the ownership of `forwardContext` since `SendMessage()`
    // will own it. We take back ownership from `HandleCoapResponse()`
    // callback.

    forwardContext.Release();

    LogInfo("Forwarded request to leader on %s", PathForUri(aUri));

exit:
    LogWarnOnError(error, "forward to leader");

    if (error != kErrorNone)
    {
        FreeMessage(message);
        SendErrorMessage(aMessage, separate, error);
    }

    return error;
}

void BorderAgent::CoapDtlsSession::HandleCoapResponse(void                *aContext,
                                                      otMessage           *aMessage,
                                                      const otMessageInfo *aMessageInfo,
                                                      otError              aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    OwnedPtr<ForwardContext> forwardContext(static_cast<ForwardContext *>(aContext));

    forwardContext->mSession.HandleCoapResponse(*forwardContext.Get(), AsCoapMessagePtr(aMessage), aResult);
}

void BorderAgent::CoapDtlsSession::HandleCoapResponse(const ForwardContext &aForwardContext,
                                                      const Coap::Message  *aResponse,
                                                      Error                 aResult)
{
    Coap::Message *message = nullptr;
    Error          error;

    SuccessOrExit(error = aResult);
    VerifyOrExit((message = NewPriorityMessage()) != nullptr, error = kErrorNoBufs);

    if (aForwardContext.mPetition && aResponse->GetCode() == Coap::kCodeChanged)
    {
        uint8_t state;

        SuccessOrExit(error = Tlv::Find<StateTlv>(*aResponse, state));

        if (state == StateTlv::kAccept)
        {
            uint16_t sessionId;

            SuccessOrExit(error = Tlv::Find<CommissionerSessionIdTlv>(*aResponse, sessionId));

            Get<Mle::Mle>().GetCommissionerAloc(sessionId, mCommissionerAloc.GetAddress());
            Get<ThreadNetif>().AddUnicastAddress(mCommissionerAloc);
            IgnoreError(Get<Ip6::Udp>().AddReceiver(mUdpReceiver));
            mIsActiveCommissioner = true;
            Get<BorderAgent>().HandleCommissionerPetitionAccepted(*this);

            LogInfo("Commissioner accepted - SessionId:%u ALOC:%s", sessionId,
                    mCommissionerAloc.GetAddress().ToString().AsCString());
        }
        else
        {
            LogInfo("Commissioner rejected");
        }
    }

    SuccessOrExit(error = aForwardContext.ToHeader(*message, aResponse->GetCode()));

    if (aResponse->GetLength() > aResponse->GetOffset())
    {
        SuccessOrExit(error = message->SetPayloadMarker());
    }

    SuccessOrExit(error = ForwardToCommissioner(*message, *aResponse));

exit:

    if (error != kErrorNone)
    {
        FreeMessage(message);

        LogWarn("Commissioner request[%u] failed: %s", aForwardContext.mMessageId, ErrorToString(error));

        SendErrorMessage(aForwardContext, error);
    }
}

bool BorderAgent::CoapDtlsSession::HandleUdpReceive(void                *aContext,
                                                    const otMessage     *aMessage,
                                                    const otMessageInfo *aMessageInfo)
{
    return static_cast<CoapDtlsSession *>(aContext)->HandleUdpReceive(AsCoreType(aMessage), AsCoreType(aMessageInfo));
}

bool BorderAgent::CoapDtlsSession::HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error                     error     = kErrorNone;
    Coap::Message            *message   = nullptr;
    bool                      didHandle = false;
    ExtendedTlv               extTlv;
    UdpEncapsulationTlvHeader udpEncapHeader;
    OffsetRange               offsetRange;

    VerifyOrExit(aMessageInfo.GetSockAddr() == mCommissionerAloc.GetAddress());

    didHandle = true;

    VerifyOrExit(aMessage.GetLength() > 0);

    message = NewPriorityNonConfirmablePostMessage(kUriProxyRx);
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

    SuccessOrExit(error = SendMessage(*message));

    LogInfo("Sent ProxyRx (c/ur) to commissioner");

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send ProxyRx (c/ur)");

    return didHandle;
}

Error BorderAgent::CoapDtlsSession::ForwardToCommissioner(Coap::Message &aForwardMessage, const Message &aMessage)
{
    Error       error = kErrorNone;
    OffsetRange offsetRange;

    offsetRange.InitFromMessageOffsetToEnd(aMessage);
    SuccessOrExit(error = aForwardMessage.AppendBytesFromMessage(aMessage, offsetRange));

    SuccessOrExit(error = SendMessage(aForwardMessage));

    LogInfo("Sent to commissioner");

exit:
    LogWarnOnError(error, "send to commissioner");
    return error;
}

void BorderAgent::CoapDtlsSession::SendErrorMessage(const ForwardContext &aForwardContext, Error aError)
{
    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;

    VerifyOrExit((message = NewPriorityMessage()) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = aForwardContext.ToHeader(*message, CoapCodeFromError(aError)));
    SuccessOrExit(error = SendMessage(*message));

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send error CoAP message");
}

void BorderAgent::CoapDtlsSession::SendErrorMessage(const Coap::Message &aRequest, bool aSeparate, Error aError)
{
    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;

    VerifyOrExit((message = NewPriorityMessage()) != nullptr, error = kErrorNoBufs);

    if (aRequest.IsNonConfirmable() || aSeparate)
    {
        message->Init(Coap::kTypeNonConfirmable, CoapCodeFromError(aError));
    }
    else
    {
        message->Init(Coap::kTypeAck, CoapCodeFromError(aError));
    }

    if (!aSeparate)
    {
        message->SetMessageId(aRequest.GetMessageId());
    }

    SuccessOrExit(error = message->SetTokenFromMessage(aRequest));

    SuccessOrExit(error = SendMessage(*message));

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send error CoAP message");
}

void BorderAgent::CoapDtlsSession::HandleTmfProxyTx(Coap::Message &aMessage)
{
    Error                     error   = kErrorNone;
    Message                  *message = nullptr;
    Ip6::MessageInfo          messageInfo;
    OffsetRange               offsetRange;
    UdpEncapsulationTlvHeader udpEncapHeader;

    VerifyOrExit(Get<BorderAgent>().mState != kStateStopped);

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aMessage, Tlv::kUdpEncapsulation, offsetRange));

    SuccessOrExit(error = aMessage.Read(offsetRange, udpEncapHeader));
    offsetRange.AdvanceOffset(sizeof(UdpEncapsulationTlvHeader));

    VerifyOrExit(udpEncapHeader.GetSourcePort() > 0 && udpEncapHeader.GetDestinationPort() > 0, error = kErrorDrop);

    VerifyOrExit((message = Get<Ip6::Udp>().NewMessage()) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, offsetRange));

    messageInfo.SetSockPort(udpEncapHeader.GetSourcePort());
    messageInfo.SetSockAddr(mCommissionerAloc.GetAddress());
    messageInfo.SetPeerPort(udpEncapHeader.GetDestinationPort());

    SuccessOrExit(error = Tlv::Find<Ip6AddressTlv>(aMessage, messageInfo.GetPeerAddr()));

    SuccessOrExit(error = Get<Ip6::Udp>().SendDatagram(*message, messageInfo));

    LogInfo("Proxy transmit sent to %s", messageInfo.GetPeerAddr().ToString().AsCString());

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send proxy stream");
}

void BorderAgent::CoapDtlsSession::HandleTmfRelayTx(Coap::Message &aMessage)
{
    Error            error = kErrorNone;
    uint16_t         joinerRouterRloc;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());
    OffsetRange      offsetRange;

    VerifyOrExit(Get<BorderAgent>().mState != kStateStopped);

    VerifyOrExit(aMessage.IsNonConfirmablePostRequest());

    SuccessOrExit(error = Tlv::Find<JoinerRouterLocatorTlv>(aMessage, joinerRouterRloc));

    message = Get<Tmf::Agent>().NewPriorityNonConfirmablePostMessage(kUriRelayTx);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    offsetRange.InitFromMessageOffsetToEnd(aMessage);
    SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, offsetRange));

    messageInfo.SetSockAddrToRlocPeerAddrTo(joinerRouterRloc);
    messageInfo.SetSockPortToTmf();

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    LogInfo("Sent to joiner router request on RelayTx (c/tx)");

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send to joiner router request RelayTx (c/tx)");
}

void BorderAgent::CoapDtlsSession::HandleTmfDatasetGet(Coap::Message &aMessage, Uri aUri)
{
    Error          error    = kErrorNone;
    Coap::Message *response = nullptr;

    // When processing `MGMT_GET` request directly on Border Agent,
    // the Security Policy flags (O-bit) should be ignored to allow
    // the commissioner candidate to get the full Operational Dataset.

    switch (aUri)
    {
    case kUriActiveGet:
        response = Get<ActiveDatasetManager>().ProcessGetRequest(aMessage, DatasetManager::kIgnoreSecurityPolicyFlags);
        Get<BorderAgent>().mCounters.mMgmtActiveGets++;
        break;

    case kUriPendingGet:
        response = Get<PendingDatasetManager>().ProcessGetRequest(aMessage, DatasetManager::kIgnoreSecurityPolicyFlags);
        Get<BorderAgent>().mCounters.mMgmtPendingGets++;
        break;

    case kUriCommissionerGet:
        response = Get<NetworkData::Leader>().ProcessCommissionerGetRequest(aMessage);
        break;

    default:
        break;
    }

    VerifyOrExit(response != nullptr, error = kErrorParse);

    SuccessOrExit(error = SendMessage(*response));

    LogInfo("Sent %s response to non-active commissioner", PathForUri(aUri));

exit:
    LogWarnOnError(error, "send Active/Pending/CommissionerGet response");
    FreeMessageOnError(response, error);
}

void BorderAgent::CoapDtlsSession::HandleTimer(Timer &aTimer)
{
    static_cast<CoapDtlsSession *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleTimer();
}

void BorderAgent::CoapDtlsSession::HandleTimer(void)
{
    if (IsConnected())
    {
        LogInfo("Session timed out - disconnecting");
        Disconnect();
    }
}

//----------------------------------------------------------------------------------------------------------------------
// `BorderAgent::CoapDtlsSession::ForwardContext`

BorderAgent::CoapDtlsSession::ForwardContext::ForwardContext(CoapDtlsSession     &aSesssion,
                                                             const Coap::Message &aMessage,
                                                             bool                 aPetition,
                                                             bool                 aSeparate)
    : mSession(aSesssion)
    , mMessageId(aMessage.GetMessageId())
    , mPetition(aPetition)
    , mSeparate(aSeparate)
    , mTokenLength(aMessage.GetTokenLength())
    , mType(aMessage.GetType())
{
    memcpy(mToken, aMessage.GetToken(), mTokenLength);
}

Error BorderAgent::CoapDtlsSession::ForwardContext::ToHeader(Coap::Message &aMessage, uint8_t aCode) const
{
    if ((mType == Coap::kTypeNonConfirmable) || mSeparate)
    {
        aMessage.Init(Coap::kTypeNonConfirmable, static_cast<Coap::Code>(aCode));
    }
    else
    {
        aMessage.Init(Coap::kTypeAck, static_cast<Coap::Code>(aCode));
    }

    if (!mSeparate)
    {
        aMessage.SetMessageId(mMessageId);
    }

    return aMessage.SetToken(mToken, mTokenLength);
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
