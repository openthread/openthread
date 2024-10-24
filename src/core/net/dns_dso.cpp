/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#include "dns_dso.hpp"

#if OPENTHREAD_CONFIG_DNS_DSO_ENABLE

#include "instance/instance.hpp"

/**
 * @file
 *   This file implements the DNS Stateful Operations (DSO) per RFC 8490.
 */

namespace ot {
namespace Dns {

RegisterLogModule("DnsDso");

//---------------------------------------------------------------------------------------------------------------------
// otPlatDso transport callbacks

extern "C" otInstance *otPlatDsoGetInstance(otPlatDsoConnection *aConnection)
{
    return &AsCoreType(aConnection).GetInstance();
}

extern "C" otPlatDsoConnection *otPlatDsoAccept(otInstance *aInstance, const otSockAddr *aPeerSockAddr)
{
    return AsCoreType(aInstance).Get<Dso>().AcceptConnection(AsCoreType(aPeerSockAddr));
}

extern "C" void otPlatDsoHandleConnected(otPlatDsoConnection *aConnection)
{
    AsCoreType(aConnection).HandleConnected();
}

extern "C" void otPlatDsoHandleReceive(otPlatDsoConnection *aConnection, otMessage *aMessage)
{
    AsCoreType(aConnection).HandleReceive(AsCoreType(aMessage));
}

extern "C" void otPlatDsoHandleDisconnected(otPlatDsoConnection *aConnection, otPlatDsoDisconnectMode aMode)
{
    AsCoreType(aConnection).HandleDisconnected(MapEnum(aMode));
}

//---------------------------------------------------------------------------------------------------------------------
// Dso::Connection

Dso::Connection::Connection(Instance            &aInstance,
                            const Ip6::SockAddr &aPeerSockAddr,
                            Callbacks           &aCallbacks,
                            uint32_t             aInactivityTimeout,
                            uint32_t             aKeepAliveInterval)
    : InstanceLocator(aInstance)
    , mNext(nullptr)
    , mCallbacks(aCallbacks)
    , mPeerSockAddr(aPeerSockAddr)
    , mState(kStateDisconnected)
    , mIsServer(false)
    , mInactivity(aInactivityTimeout)
    , mKeepAlive(aKeepAliveInterval)
{
    OT_ASSERT(aKeepAliveInterval >= kMinKeepAliveInterval);
    Init(/* aIsServer */ false);
}

void Dso::Connection::Init(bool aIsServer)
{
    mNextMessageId       = 1;
    mIsServer            = aIsServer;
    mStateDidChange      = false;
    mLongLivedOperation  = false;
    mRetryDelay          = 0;
    mRetryDelayErrorCode = Dns::Header::kResponseSuccess;
    mDisconnectReason    = kReasonUnknown;
}

void Dso::Connection::SetState(State aState)
{
    VerifyOrExit(mState != aState);

    LogInfo("State: %s -> %s on connection with %s", StateToString(mState), StateToString(aState),
            mPeerSockAddr.ToString().AsCString());

    mState          = aState;
    mStateDidChange = true;

exit:
    return;
}

void Dso::Connection::SignalAnyStateChange(void)
{
    VerifyOrExit(mStateDidChange);
    mStateDidChange = false;

    switch (mState)
    {
    case kStateDisconnected:
        mCallbacks.mHandleDisconnected(*this);
        break;

    case kStateConnectedButSessionless:
        mCallbacks.mHandleConnected(*this);
        break;

    case kStateSessionEstablished:
        mCallbacks.mHandleSessionEstablished(*this);
        break;

    case kStateConnecting:
    case kStateEstablishingSession:
        break;
    };

exit:
    return;
}

Message *Dso::Connection::NewMessage(void)
{
    return Get<MessagePool>().Allocate(Message::kTypeOther, sizeof(Dns::Header),
                                       Message::Settings(Message::kPriorityNormal));
}

void Dso::Connection::Connect(void)
{
    OT_ASSERT(mState == kStateDisconnected);

    Init(/* aIsServer */ false);
    Get<Dso>().mClientConnections.Push(*this);
    MarkAsConnecting();
    otPlatDsoConnect(this, &mPeerSockAddr);
}

void Dso::Connection::Accept(void)
{
    OT_ASSERT(mState == kStateDisconnected);

    Init(/* aIsServer */ true);
    Get<Dso>().mServerConnections.Push(*this);
    MarkAsConnecting();
}

void Dso::Connection::MarkAsConnecting(void)
{
    SetState(kStateConnecting);

    // While in `kStateConnecting` state we use the `mKeepAlive` to
    // track the `kConnectingTimeout` (if connection is not established
    // within the timeout, we consider it as failure and close it).

    mKeepAlive.SetExpirationTime(TimerMilli::GetNow() + kConnectingTimeout);
    Get<Dso>().mTimer.FireAtIfEarlier(mKeepAlive.GetExpirationTime());

    // Wait for `HandleConnected()` or `HandleDisconnected()` callbacks
    // or timeout.
}

void Dso::Connection::HandleConnected(void)
{
    OT_ASSERT(mState == kStateConnecting);

    SetState(kStateConnectedButSessionless);
    ResetTimeouts(/* aIsKeepAliveMessage */ false);

    SignalAnyStateChange();
}

void Dso::Connection::Disconnect(DisconnectMode aMode, DisconnectReason aReason)
{
    VerifyOrExit(mState != kStateDisconnected);

    mDisconnectReason = aReason;
    MarkAsDisconnected();

    otPlatDsoDisconnect(this, MapEnum(aMode));

exit:
    return;
}

void Dso::Connection::HandleDisconnected(DisconnectMode aMode)
{
    VerifyOrExit(mState != kStateDisconnected);

    if (mState == kStateConnecting)
    {
        mDisconnectReason = kReasonFailedToConnect;
    }
    else
    {
        switch (aMode)
        {
        case kGracefullyClose:
            mDisconnectReason = kReasonPeerClosed;
            break;

        case kForciblyAbort:
            mDisconnectReason = kReasonPeerAborted;
        }
    }

    MarkAsDisconnected();
    SignalAnyStateChange();

exit:
    return;
}

void Dso::Connection::MarkAsDisconnected(void)
{
    if (IsClient())
    {
        IgnoreError(Get<Dso>().mClientConnections.Remove(*this));
    }
    else
    {
        IgnoreError(Get<Dso>().mServerConnections.Remove(*this));
    }

    mPendingRequests.Clear();
    SetState(kStateDisconnected);

    LogInfo("Disconnect reason: %s", DisconnectReasonToString(mDisconnectReason));
}

void Dso::Connection::MarkSessionEstablished(void)
{
    switch (mState)
    {
    case kStateConnectedButSessionless:
    case kStateEstablishingSession:
    case kStateSessionEstablished:
        break;

    case kStateDisconnected:
    case kStateConnecting:
        OT_ASSERT(false);
    }

    SetState(kStateSessionEstablished);
}

Error Dso::Connection::SendRequestMessage(Message &aMessage, MessageId &aMessageId, uint32_t aResponseTimeout)
{
    return SendMessage(aMessage, kRequestMessage, aMessageId, Dns::Header::kResponseSuccess, aResponseTimeout);
}

Error Dso::Connection::SendUnidirectionalMessage(Message &aMessage)
{
    MessageId messageId = 0;

    return SendMessage(aMessage, kUnidirectionalMessage, messageId);
}

Error Dso::Connection::SendResponseMessage(Message &aMessage, MessageId aResponseId)
{
    return SendMessage(aMessage, kResponseMessage, aResponseId);
}

void Dso::Connection::SetLongLivedOperation(bool aLongLivedOperation)
{
    VerifyOrExit(mLongLivedOperation != aLongLivedOperation);

    mLongLivedOperation = aLongLivedOperation;

    LogInfo("Long-lived operation %s", mLongLivedOperation ? "started" : "stopped");

    if (!mLongLivedOperation)
    {
        NextFireTime nextTime;

        UpdateNextFireTime(nextTime);
        Get<Dso>().mTimer.FireAtIfEarlier(nextTime);
    }

exit:
    return;
}

Error Dso::Connection::SendRetryDelayMessage(uint32_t aDelay, Dns::Header::Response aResponseCode)
{
    Error         error   = kErrorNone;
    Message      *message = nullptr;
    RetryDelayTlv retryDelayTlv;
    MessageId     messageId;

    switch (mState)
    {
    case kStateSessionEstablished:
        OT_ASSERT(IsServer());
        break;

    case kStateConnectedButSessionless:
    case kStateEstablishingSession:
    case kStateDisconnected:
    case kStateConnecting:
        OT_ASSERT(false);
    }

    message = NewMessage();
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    retryDelayTlv.Init();
    retryDelayTlv.SetRetryDelay(aDelay);
    SuccessOrExit(error = message->Append(retryDelayTlv));
    error = SendMessage(*message, kUnidirectionalMessage, messageId, aResponseCode);

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Dso::Connection::SetTimeouts(uint32_t aInactivityTimeout, uint32_t aKeepAliveInterval)
{
    Error error = kErrorNone;

    VerifyOrExit(aKeepAliveInterval >= kMinKeepAliveInterval, error = kErrorInvalidArgs);

    // If acting as server, the timeout values are the ones we grant
    // to a connecting clients. If acting as client, the timeout
    // values are what to request when sending Keep Alive message.
    // If in `kStateDisconnected` we set both (since we don't know
    // yet whether we are going to connect as client or server).

    if ((mState == kStateDisconnected) || IsServer())
    {
        mKeepAlive.SetInterval(aKeepAliveInterval);
        AdjustInactivityTimeout(aInactivityTimeout);
    }

    if ((mState == kStateDisconnected) || IsClient())
    {
        mKeepAlive.SetRequestInterval(aKeepAliveInterval);
        mInactivity.SetRequestInterval(aInactivityTimeout);
    }

    switch (mState)
    {
    case kStateDisconnected:
    case kStateConnecting:
        break;

    case kStateConnectedButSessionless:
    case kStateEstablishingSession:
        if (IsServer())
        {
            break;
        }

        OT_FALL_THROUGH;

    case kStateSessionEstablished:
        error = SendKeepAliveMessage();
    }

exit:
    return error;
}

Error Dso::Connection::SendKeepAliveMessage(void)
{
    return SendKeepAliveMessage(IsServer() ? kUnidirectionalMessage : kRequestMessage, 0);
}

Error Dso::Connection::SendKeepAliveMessage(MessageType aMessageType, MessageId aResponseId)
{
    // Sends a Keep Alive message of a given type. This is a common
    // method used by both client and server. `aResponseId` is
    // applicable and used only when the message type is
    // `kResponseMessage`.

    Error        error   = kErrorNone;
    Message     *message = nullptr;
    KeepAliveTlv keepAliveTlv;

    switch (mState)
    {
    case kStateConnectedButSessionless:
    case kStateEstablishingSession:
        if (IsServer())
        {
            // While session is being established, server is only allowed
            // to send a Keep Alive response to a request from client.
            OT_ASSERT(aMessageType == kResponseMessage);
        }
        break;

    case kStateSessionEstablished:
        break;

    case kStateDisconnected:
    case kStateConnecting:
        OT_ASSERT(false);
    }

    // Server can send Keep Alive response (to a request from client)
    // or a unidirectional Keep Alive message. Client can send
    // KeepAlive request message.

    if (IsServer())
    {
        if (aMessageType == kResponseMessage)
        {
            OT_ASSERT(aResponseId != 0);
        }
        else
        {
            OT_ASSERT(aMessageType == kUnidirectionalMessage);
        }
    }
    else
    {
        OT_ASSERT(aMessageType == kRequestMessage);
    }

    message = NewMessage();
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    keepAliveTlv.Init();

    if (IsServer())
    {
        keepAliveTlv.SetInactivityTimeout(mInactivity.GetInterval());
        keepAliveTlv.SetKeepAliveInterval(mKeepAlive.GetInterval());
    }
    else
    {
        keepAliveTlv.SetInactivityTimeout(mInactivity.GetRequestInterval());
        keepAliveTlv.SetKeepAliveInterval(mKeepAlive.GetRequestInterval());
    }

    SuccessOrExit(error = message->Append(keepAliveTlv));

    error = SendMessage(*message, aMessageType, aResponseId);

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Dso::Connection::SendMessage(Message              &aMessage,
                                   MessageType           aMessageType,
                                   MessageId            &aMessageId,
                                   Dns::Header::Response aResponseCode,
                                   uint32_t              aResponseTimeout)
{
    Error       error          = kErrorNone;
    Tlv::Type   primaryTlvType = Tlv::kReservedType;
    Dns::Header header;

    switch (mState)
    {
    case kStateConnectedButSessionless:
        // To establish session, client MUST send a request message.
        // Server is not allowed to send any messages. Unidirectional
        // messages are not allowed before session is established.
        OT_ASSERT(IsClient());
        OT_ASSERT(aMessageType == kRequestMessage);
        break;

    case kStateEstablishingSession:
        // During session establishment, client is allowed to send
        // additional request messages, server is only allowed to
        // send response.
        if (IsClient())
        {
            OT_ASSERT(aMessageType == kRequestMessage);
        }
        else
        {
            OT_ASSERT(aMessageType == kResponseMessage);
        }
        break;

    case kStateSessionEstablished:
        // All message types are allowed.
        break;

    case kStateDisconnected:
    case kStateConnecting:
        OT_ASSERT(false);
    }

    // A DSO request or unidirectional message MUST contain at
    // least one TLV. The first TLV is the "Primary TLV" and
    // determines the nature of the operation being performed.
    // A DSO response message may contain no TLVs, or may contain
    // one or more TLVs. Response Primary TLV(s) MUST appear first
    // in a DSO response message.

    aMessage.SetOffset(0);
    IgnoreError(ReadPrimaryTlv(aMessage, primaryTlvType));

    switch (aMessageType)
    {
    case kResponseMessage:
        break;
    case kRequestMessage:
    case kUnidirectionalMessage:
        OT_ASSERT(primaryTlvType != Tlv::kReservedType);
    }

    // `header` is cleared from its constructor call so all fields
    // start as zero.

    switch (aMessageType)
    {
    case kRequestMessage:
        header.SetType(Dns::Header::kTypeQuery);
        aMessageId = mNextMessageId;
        break;

    case kResponseMessage:
        header.SetType(Dns::Header::kTypeResponse);
        break;

    case kUnidirectionalMessage:
        header.SetType(Dns::Header::kTypeQuery);
        aMessageId = 0;
        break;
    }

    header.SetMessageId(aMessageId);
    header.SetQueryType(Dns::Header::kQueryTypeDso);
    header.SetResponseCode(aResponseCode);
    SuccessOrExit(error = aMessage.Prepend(header));

    SuccessOrExit(error = AppendPadding(aMessage));

    // Update `mPendingRequests` list with the new request info

    if (aMessageType == kRequestMessage)
    {
        SuccessOrExit(
            error = mPendingRequests.Add(mNextMessageId, primaryTlvType, TimerMilli::GetNow() + aResponseTimeout));

        if (++mNextMessageId == 0)
        {
            mNextMessageId = 1;
        }
    }

    LogInfo("Sending %s message with id %u to %s", MessageTypeToString(aMessageType), aMessageId,
            mPeerSockAddr.ToString().AsCString());

    switch (mState)
    {
    case kStateConnectedButSessionless:
        // On client we transition from "connected" state to
        // "establishing session" state on successfully sending a
        // request message.
        if (IsClient())
        {
            SetState(kStateEstablishingSession);
        }
        break;

    case kStateEstablishingSession:
        // On server we transition from "establishing session" state
        // to "established" on sending a response with success
        // response code.
        if (IsServer() && (aResponseCode == Dns::Header::kResponseSuccess))
        {
            SetState(kStateSessionEstablished);
        }

    default:
        break;
    }

    ResetTimeouts(/* aIsKeepAliveMessage*/ (primaryTlvType == KeepAliveTlv::kType));

    otPlatDsoSend(this, &aMessage);

    // Signal any state changes. This is done at the very end when the
    // `SendMessage()` is fully processed (all state and local
    // variables are updated) to ensure that we do not have any
    // reentrancy issues (e.g., if the callback signalling state
    // change triggers another tx).

    SignalAnyStateChange();

exit:
    return error;
}

Error Dso::Connection::AppendPadding(Message &aMessage)
{
    // This method appends Encryption Padding TLV to a DSO message.
    // It uses the padding policy "Random-Block-Length Padding" from
    // RFC 8467.

    static const uint16_t kBlockLengths[] = {8, 11, 17, 21};

    Error                error = kErrorNone;
    uint16_t             blockLength;
    EncryptionPaddingTlv paddingTlv;

    // We pick a random block length. The random selection can be
    // based on a "weak" source of randomness (so the use of
    // `NonCrypto` is fine). We add padding to the message such
    // that its padded length is a multiple of the chosen block
    // length.

    blockLength = kBlockLengths[Random::NonCrypto::GetUint8InRange(0, GetArrayLength(kBlockLengths))];

    paddingTlv.Init((blockLength - ((aMessage.GetLength() + sizeof(Tlv)) % blockLength)) % blockLength);

    SuccessOrExit(error = aMessage.Append(paddingTlv));

    for (uint16_t len = paddingTlv.GetLength(); len > 0; len--)
    {
        SuccessOrExit(error = aMessage.Append<uint8_t>(0));
    }

exit:
    return error;
}

void Dso::Connection::HandleReceive(Message &aMessage)
{
    Error       error          = kErrorAbort;
    Tlv::Type   primaryTlvType = Tlv::kReservedType;
    Dns::Header header;

    SuccessOrExit(aMessage.Read(0, header));

    if (header.GetQueryType() != Dns::Header::kQueryTypeDso)
    {
        if (header.GetType() == Dns::Header::kTypeQuery)
        {
            SendErrorResponse(header, Dns::Header::kResponseNotImplemented);
            error = kErrorNone;
        }

        ExitNow();
    }

    switch (mState)
    {
    case kStateConnectedButSessionless:
        // After connection is established, client should initiate
        // establishing session (by sending a request). So no rx is
        // allowed before this. On server, we allow rx of a request
        // message only.
        VerifyOrExit(IsServer() && (header.GetType() == Dns::Header::kTypeQuery) && (header.GetMessageId() != 0));
        break;

    case kStateEstablishingSession:
        // Unidirectional message are allowed after session is
        // established. While session is being established, on client,
        // we allow rx on response message. On server we can rx
        // request or response.

        VerifyOrExit(header.GetMessageId() != 0);

        if (IsClient())
        {
            VerifyOrExit(header.GetType() == Dns::Header::kTypeResponse);
        }
        break;

    case kStateSessionEstablished:
        // All message types are allowed.
        break;

    case kStateDisconnected:
    case kStateConnecting:
        ExitNow();
    }

    // All count fields MUST be set to zero in the header.
    VerifyOrExit((header.GetQuestionCount() == 0) && (header.GetAnswerCount() == 0) &&
                 (header.GetAuthorityRecordCount() == 0) && (header.GetAdditionalRecordCount() == 0));

    aMessage.SetOffset(sizeof(header));

    switch (ReadPrimaryTlv(aMessage, primaryTlvType))
    {
    case kErrorNone:
        VerifyOrExit(primaryTlvType != Tlv::kReservedType);
        break;

    case kErrorNotFound:
        // The `primaryTlvType` is set to `Tlv::kReservedType`
        // (value zero) to indicate that there is no primary TLV.
        break;

    default:
        ExitNow();
    }

    switch (header.GetType())
    {
    case Dns::Header::kTypeQuery:
        error = ProcessRequestOrUnidirectionalMessage(header, aMessage, primaryTlvType);
        break;

    case Dns::Header::kTypeResponse:
        error = ProcessResponseMessage(header, aMessage, primaryTlvType);
        break;
    }

exit:
    aMessage.Free();

    if (error == kErrorNone)
    {
        ResetTimeouts(/* aIsKeepAliveMessage */ (primaryTlvType == KeepAliveTlv::kType));
    }
    else
    {
        Disconnect(kForciblyAbort, kReasonPeerMisbehavior);
    }

    // We signal any state change at the very end when the received
    // message is fully processed (all state and local variables are
    // updated) to ensure that we do not have any reentrancy issues
    // (e.g., if a `Connection` method happens to be called from the
    // callback).

    SignalAnyStateChange();
}

Error Dso::Connection::ReadPrimaryTlv(const Message &aMessage, Tlv::Type &aPrimaryTlvType) const
{
    // Read and validate the primary TLV (first TLV  after the header).
    // The `aMessage.GetOffset()` must point to the first TLV. If no
    // TLV then `kErrorNotFound` is returned. If TLV in message is not
    // well-formed `kErrorParse` is returned. The read TLV type is
    // returned in `aPrimaryTlvType` (set to `Tlv::kReservedType`
    // (value zero) when `kErrorNotFound`).

    Error error = kErrorNotFound;
    Tlv   tlv;

    aPrimaryTlvType = Tlv::kReservedType;

    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), tlv));
    VerifyOrExit(aMessage.GetOffset() + tlv.GetSize() <= aMessage.GetLength(), error = kErrorParse);
    aPrimaryTlvType = tlv.GetType();
    error           = kErrorNone;

exit:
    return error;
}

Error Dso::Connection::ProcessRequestOrUnidirectionalMessage(const Dns::Header &aHeader,
                                                             const Message     &aMessage,
                                                             Tlv::Type          aPrimaryTlvType)
{
    Error error = kErrorAbort;

    if (IsServer() && (mState == kStateConnectedButSessionless))
    {
        SetState(kStateEstablishingSession);
    }

    // A DSO request or unidirectional message MUST contain at
    // least one TLV which is the "Primary TLV" and determines
    // the nature of the operation being performed.

    switch (aPrimaryTlvType)
    {
    case KeepAliveTlv::kType:
        error = ProcessKeepAliveMessage(aHeader, aMessage);
        break;

    case RetryDelayTlv::kType:
        error = ProcessRetryDelayMessage(aHeader, aMessage);
        break;

    case Tlv::kReservedType:
    case EncryptionPaddingTlv::kType:
        // Misbehavior by peer.
        break;

    default:
        if (aHeader.GetMessageId() == 0)
        {
            LogInfo("Received unidirectional message from %s", mPeerSockAddr.ToString().AsCString());

            error = mCallbacks.mProcessUnidirectionalMessage(*this, aMessage, aPrimaryTlvType);
        }
        else
        {
            MessageId messageId = aHeader.GetMessageId();

            LogInfo("Received request message with id %u from %s", messageId, mPeerSockAddr.ToString().AsCString());

            error = mCallbacks.mProcessRequestMessage(*this, messageId, aMessage, aPrimaryTlvType);

            // `kErrorNotFound` indicates that TLV type is not known.

            if (error == kErrorNotFound)
            {
                SendErrorResponse(aHeader, Dns::Header::kDsoTypeNotImplemented);
                error = kErrorNone;
            }
        }
        break;
    }

    return error;
}

Error Dso::Connection::ProcessResponseMessage(const Dns::Header &aHeader,
                                              const Message     &aMessage,
                                              Tlv::Type          aPrimaryTlvType)
{
    Error     error = kErrorAbort;
    Tlv::Type requestPrimaryTlvType;

    // If a client or server receives a response where the message
    // ID is zero, or is any other value that does not match the
    // message ID of any of its outstanding operations, this is a
    // fatal error and the recipient MUST forcibly abort the
    // connection immediately.

    VerifyOrExit(aHeader.GetMessageId() != 0);
    VerifyOrExit(mPendingRequests.Contains(aHeader.GetMessageId(), requestPrimaryTlvType));

    // If the response has no error and contains a primary TLV, it
    // MUST match the request primary TLV.

    if ((aHeader.GetResponseCode() == Dns::Header::kResponseSuccess) && (aPrimaryTlvType != Tlv::kReservedType))
    {
        VerifyOrExit(aPrimaryTlvType == requestPrimaryTlvType);
    }

    mPendingRequests.Remove(aHeader.GetMessageId());

    switch (requestPrimaryTlvType)
    {
    case KeepAliveTlv::kType:
        SuccessOrExit(error = ProcessKeepAliveMessage(aHeader, aMessage));
        break;

    default:
        SuccessOrExit(error = mCallbacks.mProcessResponseMessage(*this, aHeader, aMessage, aPrimaryTlvType,
                                                                 requestPrimaryTlvType));
        break;
    }

    // DSO session is established when client sends a request message
    // and receives a response from server with no error code.

    if (IsClient() && (mState == kStateEstablishingSession) &&
        (aHeader.GetResponseCode() == Dns::Header::kResponseSuccess))
    {
        SetState(kStateSessionEstablished);
    }

exit:
    return error;
}

Error Dso::Connection::ProcessKeepAliveMessage(const Dns::Header &aHeader, const Message &aMessage)
{
    Error        error  = kErrorAbort;
    uint16_t     offset = aMessage.GetOffset();
    Tlv          tlv;
    KeepAliveTlv keepAliveTlv;

    if (aHeader.GetType() == Dns::Header::kTypeResponse)
    {
        // A Keep Alive response message is allowed on a client from a sever.

        VerifyOrExit(IsClient());

        if (aHeader.GetResponseCode() != Dns::Header::kResponseSuccess)
        {
            // We got an error response code from server for our
            // Keep Alive request message. If this happens while
            // establishing the DSO session, it indicates that server
            // does not support DSO, so we close the connection. If
            // this happens while session is already established, it
            // is a misbehavior (fatal error) by server.

            if (mState == kStateEstablishingSession)
            {
                Disconnect(kGracefullyClose, kReasonPeerDoesNotSupportDso);
                error = kErrorNone;
            }

            ExitNow();
        }
    }

    // Parse and validate the Keep Alive Message

    SuccessOrExit(aMessage.Read(offset, keepAliveTlv));
    offset += keepAliveTlv.GetSize();

    VerifyOrExit((keepAliveTlv.GetType() == KeepAliveTlv::kType) && keepAliveTlv.IsValid());

    // Keep Alive message MUST contain only one Keep Alive TLV.

    while (offset < aMessage.GetLength())
    {
        SuccessOrExit(aMessage.Read(offset, tlv));
        offset += tlv.GetSize();

        VerifyOrExit((tlv.GetType() != KeepAliveTlv::kType) && (tlv.GetType() != RetryDelayTlv::kType));
    }

    VerifyOrExit(offset == aMessage.GetLength());

    if (aHeader.GetType() == Dns::Header::kTypeQuery)
    {
        if (IsServer())
        {
            // Received a Keep Alive message from client. It MUST
            // be a request message (not unidirectional). We prepare
            // and send a Keep Alive response.

            VerifyOrExit(aHeader.GetMessageId() != 0);

            LogInfo("Received KeepAlive request message from client %s", mPeerSockAddr.ToString().AsCString());

            IgnoreError(SendKeepAliveMessage(kResponseMessage, aHeader.GetMessageId()));
            error = kErrorNone;
            ExitNow();
        }

        // Received a Keep Alive message on client from server. Server
        // Keep Alive message MUST be unidirectional (message ID
        // zero).

        VerifyOrExit(aHeader.GetMessageId() == 0);
    }

    LogInfo("Received Keep Alive %s message from server %s",
            (aHeader.GetMessageId() == 0) ? "unidirectional" : "response", mPeerSockAddr.ToString().AsCString());

    // Receiving a Keep Alive interval value from server less than the
    // minimum (ten seconds) is a fatal error and client MUST then
    // abort the connection.

    VerifyOrExit(keepAliveTlv.GetKeepAliveInterval() >= kMinKeepAliveInterval);

    // Update the timeout intervals on the connection from
    // the new values we got from the server. The receive
    // of the Keep Alive message does not itself reset the
    // inactivity timer. So we use `AdjustInactivityTimeout`
    // which takes into account the time elapsed since the
    // last activity.

    AdjustInactivityTimeout(keepAliveTlv.GetInactivityTimeout());
    mKeepAlive.SetInterval(keepAliveTlv.GetKeepAliveInterval());

    LogInfo("Timeouts Inactivity:%lu, KeepAlive:%lu", ToUlong(mInactivity.GetInterval()),
            ToUlong(mKeepAlive.GetInterval()));

    error = kErrorNone;

exit:
    return error;
}

Error Dso::Connection::ProcessRetryDelayMessage(const Dns::Header &aHeader, const Message &aMessage)

{
    Error         error = kErrorAbort;
    RetryDelayTlv retryDelayTlv;

    // Retry Delay TLV can be used as the Primary TLV only in
    // a unidirectional message sent from server to client.
    // It is used by the server to instruct the client to
    // close the session and its underlying connection, and not
    // to reconnect for the indicated time interval.

    VerifyOrExit(IsClient() && (aHeader.GetMessageId() == 0));

    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), retryDelayTlv));
    VerifyOrExit(retryDelayTlv.IsValid());

    mRetryDelayErrorCode = aHeader.GetResponseCode();
    mRetryDelay          = retryDelayTlv.GetRetryDelay();

    LogInfo("Received Retry Delay message from server %s", mPeerSockAddr.ToString().AsCString());
    LogInfo("   RetryDelay:%lu ms, ResponseCode:%d", ToUlong(mRetryDelay), mRetryDelayErrorCode);

    Disconnect(kGracefullyClose, kReasonServerRetryDelayRequest);

exit:
    return error;
}

void Dso::Connection::SendErrorResponse(const Dns::Header &aHeader, Dns::Header::Response aResponseCode)
{
    Message    *response = NewMessage();
    Dns::Header header;

    VerifyOrExit(response != nullptr);

    header.SetMessageId(aHeader.GetMessageId());
    header.SetType(Dns::Header::kTypeResponse);
    header.SetQueryType(aHeader.GetQueryType());
    header.SetResponseCode(aResponseCode);

    SuccessOrExit(response->Prepend(header));

    otPlatDsoSend(this, response);
    response = nullptr;

exit:
    FreeMessage(response);
}

void Dso::Connection::AdjustInactivityTimeout(uint32_t aNewTimeout)
{
    // This method sets the inactivity timeout interval to a new value
    // and updates the expiration time based on the new timeout value.
    //
    // On client, it is called on receiving a Keep Alive response or
    // unidirectional message from server. Note that the receive of
    // the Keep Alive message does not itself reset the inactivity
    // timer. So the time elapsed since the last activity should be
    // taken into account with the new inactivity timeout value.
    //
    // On server this method is called from `SetTimeouts()` when a new
    // inactivity timeout value is set.

    TimeMilli now = TimerMilli::GetNow();
    TimeMilli start;
    TimeMilli newExpiration;

    if (mState == kStateDisconnected)
    {
        mInactivity.SetInterval(aNewTimeout);
        ExitNow();
    }

    VerifyOrExit(aNewTimeout != mInactivity.GetInterval());

    // Calculate the start time (i.e., the last time inactivity timer
    // was cleared). If the previous inactivity time is set to
    // `kInfinite` value (`IsUsed()` returns `false`) then
    // `GetExpirationTime()` returns the start time. Otherwise, we
    // calculate it going back from the current expiration time with
    // the current wait interval.

    if (!mInactivity.IsUsed())
    {
        start = mInactivity.GetExpirationTime();
    }
    else if (IsClient())
    {
        start = mInactivity.GetExpirationTime() - mInactivity.GetInterval();
    }
    else
    {
        start = mInactivity.GetExpirationTime() - CalculateServerInactivityWaitTime();
    }

    mInactivity.SetInterval(aNewTimeout);

    if (!mInactivity.IsUsed())
    {
        newExpiration = start;
    }
    else if (IsClient())
    {
        newExpiration = start + aNewTimeout;

        if (newExpiration < now)
        {
            newExpiration = now;
        }
    }
    else
    {
        newExpiration = start + CalculateServerInactivityWaitTime();

        if (newExpiration < now)
        {
            // If the server abruptly reduces the inactivity timeout
            // such that current elapsed time is already more than
            // twice the new inactivity timeout, then the client is
            // immediately considered delinquent (server can forcibly
            // abort the connection). So to give the client time to
            // close the connection gracefully, the server SHOULD
            // give the client an additional grace period of either
            // five seconds or one quarter of the new inactivity
            // timeout, whichever is greater [RFC 8490 - 7.1.1].

            newExpiration = now + Max(kMinServerInactivityWaitTime, aNewTimeout / 4);
        }
    }

    mInactivity.SetExpirationTime(newExpiration);

exit:
    return;
}

uint32_t Dso::Connection::CalculateServerInactivityWaitTime(void) const
{
    // A server will abort an idle session after five seconds
    // (`kMinServerInactivityWaitTime`) or twice the inactivity
    // timeout value, whichever is greater [RFC 8490 - 6.4.1].

    OT_ASSERT(mInactivity.IsUsed());

    return Max(mInactivity.GetInterval() * 2, kMinServerInactivityWaitTime);
}

void Dso::Connection::ResetTimeouts(bool aIsKeepAliveMessage)
{
    NextFireTime nextTime;

    // At both servers and clients, the generation or reception of any
    // complete DNS message resets both timers for that DSO
    // session, with the one exception being that a DSO Keep Alive
    // message resets only the keep alive timer, not the inactivity
    // timeout timer [RFC 8490 - 6.3]

    if (mKeepAlive.IsUsed())
    {
        // On client, we wait for the Keep Alive interval but on server
        // we wait for twice the interval before considering Keep Alive
        // timeout.
        //
        // Note that we limit the interval to `Timeout::kMaxInterval`
        // (which is ~12 days). This max limit ensures that even twice
        // the interval is less than max OpenThread timer duration so
        // that the expiration time calculations below stay within the
        // `TimerMilli` range.

        mKeepAlive.SetExpirationTime(nextTime.GetNow() + mKeepAlive.GetInterval() * (IsServer() ? 2 : 1));
    }

    if (!aIsKeepAliveMessage)
    {
        if (mInactivity.IsUsed())
        {
            mInactivity.SetExpirationTime(
                nextTime.GetNow() + (IsServer() ? CalculateServerInactivityWaitTime() : mInactivity.GetInterval()));
        }
        else
        {
            // When Inactivity timeout is not used (i.e., interval is set
            // to the special `kInfinite` value), we still need to track
            // the time so that if/when later the inactivity interval
            // gets changed, we can adjust the remaining time correctly
            // from `AdjustInactivityTimeout()`. In this case, we just
            // track the current time as "expiration time".

            mInactivity.SetExpirationTime(nextTime.GetNow());
        }
    }

    UpdateNextFireTime(nextTime);

    Get<Dso>().mTimer.FireAtIfEarlier(nextTime);
}

void Dso::Connection::UpdateNextFireTime(NextFireTime &aNextTime) const
{
    switch (mState)
    {
    case kStateDisconnected:
        break;

    case kStateConnecting:
        // While in `kStateConnecting`, Keep Alive timer is
        // used for `kConnectingTimeout`.
        aNextTime.UpdateIfEarlier(mKeepAlive.GetExpirationTime());
        break;

    case kStateConnectedButSessionless:
    case kStateEstablishingSession:
    case kStateSessionEstablished:
        mPendingRequests.UpdateNextFireTime(aNextTime);

        if (mKeepAlive.IsUsed())
        {
            aNextTime.UpdateIfEarlier(mKeepAlive.GetExpirationTime());
        }

        if (mInactivity.IsUsed() && mPendingRequests.IsEmpty() && !mLongLivedOperation)
        {
            // An operation being active on a DSO Session includes
            // a request message waiting for a response, or an
            // active long-lived operation.

            aNextTime.UpdateIfEarlier(mInactivity.GetExpirationTime());
        }

        break;
    }
}

void Dso::Connection::HandleTimer(NextFireTime &aNextTime)
{
    switch (mState)
    {
    case kStateDisconnected:
        break;

    case kStateConnecting:
        if (mKeepAlive.IsExpired(aNextTime.GetNow()))
        {
            Disconnect(kGracefullyClose, kReasonFailedToConnect);
        }
        break;

    case kStateConnectedButSessionless:
    case kStateEstablishingSession:
    case kStateSessionEstablished:
        if (mPendingRequests.HasAnyTimedOut(aNextTime.GetNow()))
        {
            // If server sends no response to a request, client
            // waits for 30 seconds (`kResponseTimeout`) after which
            // client MUST forcibly abort the connection.
            Disconnect(kForciblyAbort, kReasonResponseTimeout);
            ExitNow();
        }

        // The inactivity timer is kept clear, while an operation is
        // active on the session (which includes a request waiting for
        // response or an active long-lived operation).

        if (mInactivity.IsUsed() && mPendingRequests.IsEmpty() && !mLongLivedOperation &&
            mInactivity.IsExpired(aNextTime.GetNow()))
        {
            // On client, if the inactivity timeout is reached, the
            // connection is closed gracefully. On server, if too much
            // time (`CalculateServerInactivityWaitTime()`, i.e., five
            // seconds or twice the current inactivity timeout interval,
            // whichever is grater) elapses server MUST consider the
            // client delinquent and MUST forcibly abort the connection.

            Disconnect(IsClient() ? kGracefullyClose : kForciblyAbort, kReasonInactivityTimeout);
            ExitNow();
        }

        if (mKeepAlive.IsUsed() && mKeepAlive.IsExpired(aNextTime.GetNow()))
        {
            // On client, if the Keep Alive interval elapses without any
            // DNS messages being sent or received, the client MUST take
            // action and send a DSO Keep Alive message.
            //
            // On server, if twice the Keep Alive interval value elapses
            // without any messages being sent or received, the server
            // considers the client delinquent and aborts the connection.

            if (IsClient())
            {
                IgnoreError(SendKeepAliveMessage());
            }
            else
            {
                Disconnect(kForciblyAbort, kReasonKeepAliveTimeout);
                ExitNow();
            }
        }
        break;
    }

exit:
    UpdateNextFireTime(aNextTime);
    SignalAnyStateChange();
}

const char *Dso::Connection::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Disconnected",            // (0) kStateDisconnected,
        "Connecting",              // (1) kStateConnecting,
        "ConnectedButSessionless", // (2) kStateConnectedButSessionless,
        "EstablishingSession",     // (3) kStateEstablishingSession,
        "SessionEstablished",      // (4) kStateSessionEstablished,
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kStateDisconnected);
        ValidateNextEnum(kStateConnecting);
        ValidateNextEnum(kStateConnectedButSessionless);
        ValidateNextEnum(kStateEstablishingSession);
        ValidateNextEnum(kStateSessionEstablished);
    };

    return kStateStrings[aState];
}

const char *Dso::Connection::MessageTypeToString(MessageType aMessageType)
{
    static const char *const kMessageTypeStrings[] = {
        "Request",        // (0) kRequestMessage
        "Response",       // (1) kResponseMessage
        "Unidirectional", // (2) kUnidirectionalMessage
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kRequestMessage);
        ValidateNextEnum(kResponseMessage);
        ValidateNextEnum(kUnidirectionalMessage);
    };

    return kMessageTypeStrings[aMessageType];
}

const char *Dso::Connection::DisconnectReasonToString(DisconnectReason aReason)
{
    static const char *const kDisconnectReasonStrings[] = {
        "FailedToConnect",         // (0) kReasonFailedToConnect
        "ResponseTimeout",         // (1) kReasonResponseTimeout
        "PeerDoesNotSupportDso",   // (2) kReasonPeerDoesNotSupportDso
        "PeerClosed",              // (3) kReasonPeerClosed
        "PeerAborted",             // (4) kReasonPeerAborted
        "InactivityTimeout",       // (5) kReasonInactivityTimeout
        "KeepAliveTimeout",        // (6) kReasonKeepAliveTimeout
        "ServerRetryDelayRequest", // (7) kReasonServerRetryDelayRequest
        "PeerMisbehavior",         // (8) kReasonPeerMisbehavior
        "Unknown",                 // (9) kReasonUnknown
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kReasonFailedToConnect);
        ValidateNextEnum(kReasonResponseTimeout);
        ValidateNextEnum(kReasonPeerDoesNotSupportDso);
        ValidateNextEnum(kReasonPeerClosed);
        ValidateNextEnum(kReasonPeerAborted);
        ValidateNextEnum(kReasonInactivityTimeout);
        ValidateNextEnum(kReasonKeepAliveTimeout);
        ValidateNextEnum(kReasonServerRetryDelayRequest);
        ValidateNextEnum(kReasonPeerMisbehavior);
        ValidateNextEnum(kReasonUnknown);
    };

    return kDisconnectReasonStrings[aReason];
}

//---------------------------------------------------------------------------------------------------------------------
// Dso::Connection::PendingRequests

bool Dso::Connection::PendingRequests::Contains(MessageId aMessageId, Tlv::Type &aPrimaryTlvType) const
{
    bool         contains = true;
    const Entry *entry    = mRequests.FindMatching(aMessageId);

    VerifyOrExit(entry != nullptr, contains = false);
    aPrimaryTlvType = entry->mPrimaryTlvType;

exit:
    return contains;
}

Error Dso::Connection::PendingRequests::Add(MessageId aMessageId, Tlv::Type aPrimaryTlvType, TimeMilli aResponseTimeout)
{
    Error  error = kErrorNone;
    Entry *entry = mRequests.PushBack();

    VerifyOrExit(entry != nullptr, error = kErrorNoBufs);
    entry->mMessageId      = aMessageId;
    entry->mPrimaryTlvType = aPrimaryTlvType;
    entry->mTimeout        = aResponseTimeout;

exit:
    return error;
}

void Dso::Connection::PendingRequests::Remove(MessageId aMessageId) { mRequests.RemoveMatching(aMessageId); }

bool Dso::Connection::PendingRequests::HasAnyTimedOut(TimeMilli aNow) const
{
    bool timedOut = false;

    for (const Entry &entry : mRequests)
    {
        if (entry.mTimeout <= aNow)
        {
            timedOut = true;
            break;
        }
    }

    return timedOut;
}

void Dso::Connection::PendingRequests::UpdateNextFireTime(NextFireTime &aNextTime) const
{
    for (const Entry &entry : mRequests)
    {
        aNextTime.UpdateIfEarlier(entry.mTimeout);
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Dso

Dso::Dso(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mAcceptHandler(nullptr)
    , mTimer(aInstance)
{
}

void Dso::StartListening(AcceptHandler aAcceptHandler)
{
    mAcceptHandler = aAcceptHandler;
    otPlatDsoEnableListening(&GetInstance(), true);
}

void Dso::StopListening(void) { otPlatDsoEnableListening(&GetInstance(), false); }

Dso::Connection *Dso::FindClientConnection(const Ip6::SockAddr &aPeerSockAddr)
{
    return mClientConnections.FindMatching(aPeerSockAddr);
}

Dso::Connection *Dso::FindServerConnection(const Ip6::SockAddr &aPeerSockAddr)
{
    return mServerConnections.FindMatching(aPeerSockAddr);
}

Dso::Connection *Dso::AcceptConnection(const Ip6::SockAddr &aPeerSockAddr)
{
    Connection *connection = nullptr;

    VerifyOrExit(mAcceptHandler != nullptr);
    connection = mAcceptHandler(GetInstance(), aPeerSockAddr);

    VerifyOrExit(connection != nullptr);
    connection->Accept();

exit:
    return connection;
}

void Dso::HandleTimer(void)
{
    NextFireTime nextTime;
    Connection  *conn;
    Connection  *next;

    for (conn = mClientConnections.GetHead(); conn != nullptr; conn = next)
    {
        next = conn->GetNext();
        conn->HandleTimer(nextTime);
    }

    for (conn = mServerConnections.GetHead(); conn != nullptr; conn = next)
    {
        next = conn->GetNext();
        conn->HandleTimer(nextTime);
    }

    mTimer.FireAtIfEarlier(nextTime);
}

} // namespace Dns
} // namespace ot

#if OPENTHREAD_CONFIG_DNS_DSO_MOCK_PLAT_APIS_ENABLE

OT_TOOL_WEAK void otPlatDsoEnableListening(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);
}

OT_TOOL_WEAK void otPlatDsoConnect(otPlatDsoConnection *aConnection, const otSockAddr *aPeerSockAddr)
{
    OT_UNUSED_VARIABLE(aConnection);
    OT_UNUSED_VARIABLE(aPeerSockAddr);
}

OT_TOOL_WEAK void otPlatDsoSend(otPlatDsoConnection *aConnection, otMessage *aMessage)
{
    OT_UNUSED_VARIABLE(aConnection);
    OT_UNUSED_VARIABLE(aMessage);
}

OT_TOOL_WEAK void otPlatDsoDisconnect(otPlatDsoConnection *aConnection, otPlatDsoDisconnectMode aMode)
{
    OT_UNUSED_VARIABLE(aConnection);
    OT_UNUSED_VARIABLE(aMode);
}

#endif // OPENTHREAD_CONFIG_DNS_DSO_MOCK_PLAT_APIS_ENABLE

#endif // OPENTHREAD_CONFIG_DNS_DSO_ENABLE
