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
    , mUdpReceiver(BorderAgent::HandleUdpReceive, this)
    , mTimer(aInstance)
    , mDtlsTransport(aInstance, kNoLinkSecurity)
#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    , mIdInitialized(false)
#endif
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    , mUsingEphemeralKey(false)
    , mOldUdpPort(0)
    , mEphemeralKeyTimer(aInstance)
    , mEphemeralKeyTask(aInstance)
#endif
{
    mCommissionerAloc.InitAsThreadOriginMeshLocal();
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

    mCoapDtlsSession.Reset(CoapDtlsSession::Allocate(GetInstance(), mDtlsTransport));
    VerifyOrExit(mCoapDtlsSession != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = mDtlsTransport.Open());
    SuccessOrExit(error = mDtlsTransport.Bind(aUdpPort));

    SuccessOrExit(error = mDtlsTransport.SetPsk(aPsk, aPskLength));

    mCoapDtlsSession->SetConnectCallback(HandleConnected, this);

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

    mTimer.Stop();
    mDtlsTransport.Close();
    mCoapDtlsSession.Free();

    mState = kStateStopped;
    LogInfo("Border Agent stopped");

exit:
    return;
}

void BorderAgent::Disconnect(void)
{
    VerifyOrExit(mState == kStateConnected || mState == kStateAccepted);

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

void BorderAgent::HandleTimeout(void)
{
    if (mCoapDtlsSession->IsConnected())
    {
        mCoapDtlsSession->Disconnect();
        LogWarn("Reset secure session");
    }
}

void BorderAgent::HandleConnected(Dtls::Session::ConnectEvent aEvent, void *aContext)
{
    static_cast<BorderAgent *>(aContext)->HandleConnected(aEvent);
}

void BorderAgent::HandleConnected(Dtls::Session::ConnectEvent aEvent)
{
    if (aEvent == Dtls::Session::kConnected)
    {
        LogInfo("SecureSession connected");
        mState = kStateConnected;
        mTimer.Start(kKeepAliveTimeout);
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
        if (mUsingEphemeralKey)
        {
            mCounters.mEpskcSecureSessionSuccesses++;
            mEphemeralKeyTask.Post();
        }
        else
#endif
        {
            mCounters.mPskcSecureSessionSuccesses++;
        }
    }
    else
    {
        LogInfo("SecureSession disconnected");
        IgnoreError(Get<Ip6::Udp>().RemoveReceiver(mUdpReceiver));
        Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
        if (mUsingEphemeralKey)
        {
            RestartAfterRemovingEphemeralKey();

            if (aEvent == Dtls::Session::kDisconnectedError)
            {
                mCounters.mEpskcSecureSessionFailures++;
            }
            else if (aEvent == Dtls::Session::kDisconnectedPeerClosed)
            {
                mCounters.mEpskcDeactivationDisconnects++;
            }
        }
        else
#endif
        {
            mState = kStateStarted;

            if (aEvent == Dtls::Session::kDisconnectedError)
            {
                mCounters.mPskcSecureSessionFailures++;
            }
        }
    }
}

void BorderAgent::HandleTmfCommissionerKeepAlive(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(mState == kStateAccepted);

    SuccessOrExit(ForwardToLeader(aMessage, aMessageInfo, kUriLeaderKeepAlive));
    mTimer.Start(kKeepAliveTimeout);

exit:
    return;
}

Error BorderAgent::ForwardToLeader(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, Uri aUri)
{
    Error                    error = kErrorNone;
    OwnedPtr<ForwardContext> forwardContext;
    Tmf::MessageInfo         messageInfo(GetInstance());
    Coap::Message           *message  = nullptr;
    bool                     petition = false;
    bool                     separate = false;
    OffsetRange              offsetRange;

    VerifyOrExit(mState != kStateStopped);

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
        SuccessOrExit(error = mCoapDtlsSession->SendAck(aMessage, aMessageInfo));
    }

    forwardContext.Reset(ForwardContext::AllocateAndInit(GetInstance(), aMessage, petition, separate));
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

void BorderAgent::HandleCoapResponse(void                *aContext,
                                     otMessage           *aMessage,
                                     const otMessageInfo *aMessageInfo,
                                     otError              aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    OwnedPtr<ForwardContext> forwardContext(static_cast<ForwardContext *>(aContext));

    forwardContext->Get<BorderAgent>().HandleCoapResponse(*forwardContext.Get(), AsCoapMessagePtr(aMessage), aResult);
}

void BorderAgent::HandleCoapResponse(const ForwardContext &aForwardContext,
                                     const Coap::Message  *aResponse,
                                     Error                 aResult)
{
    Coap::Message *message = nullptr;
    Error          error;

    SuccessOrExit(error = aResult);
    VerifyOrExit((message = mCoapDtlsSession->NewPriorityMessage()) != nullptr, error = kErrorNoBufs);

    if (aForwardContext.IsPetition() && aResponse->GetCode() == Coap::kCodeChanged)
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

        LogWarn("Commissioner request[%u] failed: %s", aForwardContext.GetMessageId(), ErrorToString(error));

        SendErrorMessage(aForwardContext, error);
    }
}

bool BorderAgent::HandleUdpReceive(void *aContext, const otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    return static_cast<BorderAgent *>(aContext)->HandleUdpReceive(AsCoreType(aMessage), AsCoreType(aMessageInfo));
}

bool BorderAgent::HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
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

    message = mCoapDtlsSession->NewPriorityNonConfirmablePostMessage(kUriProxyRx);
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

    SuccessOrExit(error = mCoapDtlsSession->SendMessage(*message));

    LogInfo("Sent ProxyRx (c/ur) to commissioner");

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send ProxyRx (c/ur)");

    return didHandle;
}

Error BorderAgent::ForwardToCommissioner(Coap::Message &aForwardMessage, const Message &aMessage)
{
    Error       error;
    OffsetRange offsetRange;

    offsetRange.InitFromMessageOffsetToEnd(aMessage);
    SuccessOrExit(error = aForwardMessage.AppendBytesFromMessage(aMessage, offsetRange));

    SuccessOrExit(error = mCoapDtlsSession->SendMessage(aForwardMessage));

    LogInfo("Sent to commissioner");

exit:
    LogWarnOnError(error, "send to commissioner");
    return error;
}

void BorderAgent::SendErrorMessage(const ForwardContext &aForwardContext, Error aError)
{
    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;

    VerifyOrExit((message = mCoapDtlsSession->NewPriorityMessage()) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = aForwardContext.ToHeader(*message, CoapCodeFromError(aError)));
    SuccessOrExit(error = mCoapDtlsSession->SendMessage(*message));

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send error CoAP message");
}

void BorderAgent::SendErrorMessage(const Coap::Message &aRequest, bool aSeparate, Error aError)
{
    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;

    VerifyOrExit((message = mCoapDtlsSession->NewPriorityMessage()) != nullptr, error = kErrorNoBufs);

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

    SuccessOrExit(error = mCoapDtlsSession->SendMessage(*message));

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send error CoAP message");
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

    message = mCoapDtlsSession->NewPriorityNonConfirmablePostMessage(kUriRelayRx);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = ForwardToCommissioner(*message, aMessage));
    LogInfo("Sent to commissioner on RelayRx (c/rx)");

exit:
    FreeMessageOnError(message, error);
}

void BorderAgent::HandleTmfProxyTx(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error                     error   = kErrorNone;
    Message                  *message = nullptr;
    Ip6::MessageInfo          messageInfo;
    OffsetRange               offsetRange;
    UdpEncapsulationTlvHeader udpEncapHeader;

    VerifyOrExit(mState != kStateStopped);

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

void BorderAgent::HandleTmfRelayTx(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error            error = kErrorNone;
    uint16_t         joinerRouterRloc;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());
    OffsetRange      offsetRange;

    VerifyOrExit(mState != kStateStopped);

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

void BorderAgent::HandleTmfDatasetGet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, Uri aUri)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error          error    = kErrorNone;
    Coap::Message *response = nullptr;

    // When processing `MGMT_GET` request directly on Border Agent,
    // the Security Policy flags (O-bit) should be ignored to allow
    // the commissioner candidate to get the full Operational Dataset.

    switch (aUri)
    {
    case kUriActiveGet:
        response = Get<ActiveDatasetManager>().ProcessGetRequest(aMessage, DatasetManager::kIgnoreSecurityPolicyFlags);
        mCounters.mMgmtActiveGets++;
        break;

    case kUriPendingGet:
        response = Get<PendingDatasetManager>().ProcessGetRequest(aMessage, DatasetManager::kIgnoreSecurityPolicyFlags);
        mCounters.mMgmtPendingGets++;
        break;

    case kUriCommissionerGet:
        response = Get<NetworkData::Leader>().ProcessCommissionerGetRequest(aMessage);
        break;

    default:
        break;
    }

    VerifyOrExit(response != nullptr, error = kErrorParse);

    SuccessOrExit(error = mCoapDtlsSession->SendMessage(*response));

    LogInfo("Sent %s response to non-active commissioner", PathForUri(aUri));

exit:
    LogWarnOnError(error, "send Active/Pending/CommissionerGet response");
    FreeMessageOnError(response, error);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Ephemeral Key

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

Error BorderAgent::SetEphemeralKey(const char *aKeyString, uint32_t aTimeout, uint16_t aUdpPort)
{
    Error    error  = kErrorNone;
    uint16_t length = StringLength(aKeyString, kMaxEphemeralKeyLength + 1);

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

    mUsingEphemeralKey = true;

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

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

//----------------------------------------------------------------------------------------------------------------------
// `BorderAgent::CoapDtlsSession

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
        IgnoreError(Get<BorderAgent>().ForwardToLeader(aMessage, aMessageInfo, kUriLeaderPetition));
        break;
    case kUriCommissionerKeepAlive:
        Get<BorderAgent>().HandleTmfCommissionerKeepAlive(aMessage, aMessageInfo);
        break;
    case kUriRelayTx:
        Get<BorderAgent>().HandleTmfRelayTx(aMessage, aMessageInfo);
        break;
    case kUriCommissionerGet:
    case kUriActiveGet:
    case kUriPendingGet:
        Get<BorderAgent>().HandleTmfDatasetGet(aMessage, aMessageInfo, uri);
        break;
    case kUriProxyTx:
        Get<BorderAgent>().HandleTmfProxyTx(aMessage, aMessageInfo);
        break;
    default:
        didHandle = false;
        break;
    }

    return didHandle;
}

//----------------------------------------------------------------------------------------------------------------------
// `BorderAgent::ForwardContext`

Error BorderAgent::ForwardContext::Init(Instance            &aInstance,
                                        const Coap::Message &aMessage,
                                        bool                 aPetition,
                                        bool                 aSeparate)
{
    InstanceLocatorInit::Init(aInstance);
    mMessageId   = aMessage.GetMessageId();
    mPetition    = aPetition;
    mSeparate    = aSeparate;
    mType        = aMessage.GetType();
    mTokenLength = aMessage.GetTokenLength();
    memcpy(mToken, aMessage.GetToken(), mTokenLength);

    return kErrorNone;
}

Error BorderAgent::ForwardContext::ToHeader(Coap::Message &aMessage, uint8_t aCode) const
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
