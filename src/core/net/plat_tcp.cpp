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

/**
 * @file
 *   This file implements platform TCP.
 */

#include "plat_tcp.hpp"

#if OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Ip6 {

RegisterLogModule("PlatTcp");

//---------------------------------------------------------------------------------------------------------------------
// otPlatTcp functions (callbacks)

extern "C" otPlatTcpConnection *otPlatTcpAccept(otPlatTcpListener *aListener, const otPlatTcpSockAddr *aPeerSockAddr)
{
    return AsCoreType(aListener).Accept(AsCoreType(aPeerSockAddr));
}

extern "C" bool otPlatTcpIsConnecting(otPlatTcpConnection *aConn)
{
    return AsCoreType(aConn).GetState() == PlatTcp::Connection::kStateConnecting;
}

extern "C" void otPlatTcpHandleConnected(otPlatTcpConnection *aConn) { AsCoreType(aConn).HandleConnected(); }

extern "C" bool otPlatTcpIsTxPending(otPlatTcpConnection *aConn) { return AsCoreType(aConn).IsTxPending(); }

extern "C" void otPlatTcpHandleTxReady(otPlatTcpConnection *aConn) { AsCoreType(aConn).HandleTxReady(); }

extern "C" void otPlatTcpHandleReceive(otPlatTcpConnection *aConn, const uint8_t *aBuffer, uint16_t aLength)
{
    AsCoreType(aConn).HandleReceive(aBuffer, aLength);
}

extern "C" void otPlatTcpHandleDisconnected(otPlatTcpConnection *aConn, otPlatTcpDisconnectReason aReason)
{
    AsCoreType(aConn).HandleDisconnected(static_cast<PlatTcp::DisconnectReason>(aReason));
}

extern "C" otInstance *otPlatTcpGetInstanceForConnection(otPlatTcpConnection *aConn)
{
    return &AsCoreType(aConn).GetInstance();
}

extern "C" otInstance *otPlatTcpGetInstanceForListener(otPlatTcpListener *aListener)
{
    return &AsCoreType(aListener).GetInstance();
}

extern "C" otPlatTcpListener *otPlatTcpIterateListeners(otInstance *aInstance, otPlatTcpListener *aPrevListener)
{
    return AsCoreType(aInstance).Get<PlatTcp>().IterateListeners(AsCoreTypePtr(aPrevListener));
}

extern "C" otPlatTcpConnection *otPlatTcpIterateConnections(otInstance *aInstance, otPlatTcpConnection *aPrevConn)
{
    return AsCoreType(aInstance).Get<PlatTcp>().IterateConnections(AsCoreTypePtr(aPrevConn));
}

//---------------------------------------------------------------------------------------------------------------------
// PlatTcp::SockAddr

bool PlatTcp::SockAddr::IsAllUnspecified(void) const
{
    return (GetPort() == 0) && (mIfIndex == 0) && GetAddress().IsUnspecified();
}

PlatTcp::SockAddr::InfoString PlatTcp::SockAddr::ToString(void) const
{
    InfoString string;

    string.Append("[");
    GetAddress().ToString(string);
    string.Append("%%%lu]:%u", ToUlong(GetIfIndex()), GetPort());

    return string;
}

//---------------------------------------------------------------------------------------------------------------------
// PlatTcp::Listener

PlatTcp::Listener::Listener(Instance &aInstance, AcceptHandler aAcceptHandler, DeleteHandler aDeleteHandler)
    : InstanceLocator(aInstance)
    , mNext(nullptr)
    , mState(kStateUnused)
    , mAcceptHandler(aAcceptHandler)
    , mDeleteHandler(aDeleteHandler)
{
    ClearAllBytes(mData);
}

Error PlatTcp::Listener::Enable(const SockAddr &aLocalSockAddr)
{
    Error error = kErrorNone;

    switch (GetState())
    {
    case kStateUnused:
        Get<PlatTcp>().AddListener(*this);
        break;
    case kStateDisabled:
        break;
    case kStateEnabled:
        error = kErrorAlready;
        ExitNow();
    }

    ClearAllBytes(mData);
    mLocalSockAddr = aLocalSockAddr;

    SetState(kStateEnabled);

    error = otPlatTcpEnableListener(this, &mLocalSockAddr);

    if (error != kErrorNone)
    {
        SetState(kStateDisabled);
    }

exit:
    return error;
}

void PlatTcp::Listener::Disable(void)
{
    VerifyOrExit(GetState() == kStateEnabled);

    SetState(kStateDisabled);

    otPlatTcpDisableListener(this);

exit:
    return;
}

void PlatTcp::Listener::SetState(State aState)
{
    VerifyOrExit(mState != aState);

    mState = aState;

    switch (mState)
    {
    case kStateUnused:
        VerifyOrExit(mDeleteHandler != nullptr);
        mDeleteHandler(*this);
        break;

    case kStateEnabled:
        break;

    case kStateDisabled:
        Get<PlatTcp>().PostListenerTask();
        break;
    }

exit:
    return;
}

PlatTcp::Connection *PlatTcp::Listener::Accept(const SockAddr &aPeerSockAddr)
{
    Connection *connection = nullptr;

    VerifyOrExit(GetState() == kStateEnabled);

    VerifyOrExit(mAcceptHandler != nullptr);

    connection = mAcceptHandler(*this, aPeerSockAddr);
    VerifyOrExit(connection != nullptr);

    if (connection->Prepare(mLocalSockAddr, aPeerSockAddr) != kErrorNone)
    {
        connection = nullptr;
    }

exit:
    return connection;
}

//---------------------------------------------------------------------------------------------------------------------
// PlatTcp::Connection

PlatTcp::Connection::Connection(Instance &aInstance, EventHandler aEventHandler, DeleteHandler aDeleteHandler)
    : InstanceLocator(aInstance)
    , mNext(nullptr)
    , mState(kStateUnused)
    , mDisconnectReason(kDisconnectReasonClosed)
    , mRxMessage(nullptr)
    , mEventHandler(aEventHandler)
    , mDeleteHandler(aDeleteHandler)
{
    ClearAllBytes(mData);
}

void PlatTcp::Connection::SetState(State aState)
{
    VerifyOrExit(mState != aState);

    mState = aState;

    switch (mState)
    {
    case kStateUnused:
        FreeRxMessage();
        mTxQueue.DequeueAndFreeAll();
        VerifyOrExit(mDeleteHandler != nullptr);
        mDeleteHandler(*this);
        break;

    case kStateConnecting:
    case kStateConnected:
    case kStateToClose:
    case kStateClosing:
        break;

    case kStateDisconnected:
        mTxQueue.DequeueAndFreeAll();
        Get<PlatTcp>().PostConnectionTask();
        break;
    }

exit:
    return;
}

Error PlatTcp::Connection::Prepare(const SockAddr &aLocalSockAddr, const SockAddr &aPeerSockAddr)
{
    Error error = kErrorNone;

    switch (GetState())
    {
    case kStateUnused:
        Get<PlatTcp>().AddConnection(*this);
        break;
    case kStateDisconnected:
        break;

    case kStateConnecting:
    case kStateConnected:
    case kStateToClose:
    case kStateClosing:
        error = kErrorInvalidState;
        ExitNow();
    }

    ClearAllBytes(mData);
    FreeRxMessage();
    mLocalSockAddr = aLocalSockAddr;
    mPeerSockAddr  = aPeerSockAddr;

    SetState(kStateConnecting);

exit:
    return error;
}

Error PlatTcp::Connection::Connect(const SockAddr &aPeerSockAddr) { return BindAndConnect(SockAddr(), aPeerSockAddr); }

Error PlatTcp::Connection::BindAndConnect(const SockAddr &aLocalSockAddr, const SockAddr &aPeerSockAddr)
{
    Error error;

    SuccessOrExit(error = Prepare(aLocalSockAddr, aPeerSockAddr));

    error = otPlatTcpConnect(this, &mPeerSockAddr, mLocalSockAddr.IsAllUnspecified() ? nullptr : &mLocalSockAddr);

    if (error != kErrorNone)
    {
        SetState(kStateDisconnected);
    }

exit:
    return error;
}

void PlatTcp::Connection::HandleConnected(void)
{
    VerifyOrExit(GetState() == kStateConnecting);

    SetState(kStateConnected);
    SignalEvent(kEventConnected);

exit:
    return;
}

Error PlatTcp::Connection::Send(OwnedPtr<Message> aMessage)
{
    Error error = kErrorNone;
    bool  shouldNotify;

    VerifyOrExit(GetState() == kStateConnected, error = kErrorInvalidState);

    VerifyOrExit(aMessage != nullptr);
    VerifyOrExit(aMessage->GetLength() > 0);

    aMessage->SetOffset(0);

    shouldNotify = mTxQueue.IsEmpty();
    mTxQueue.Enqueue(*aMessage.Release());

    if (shouldNotify)
    {
        otPlatTcpNotifyTxPending(this);
    }

exit:
    return error;
}

void PlatTcp::Connection::HandleTxReady(void)
{
    switch (GetState())
    {
    case kStateConnected:
    case kStateToClose:
        break;
    case kStateUnused:
    case kStateConnecting:
    case kStateClosing:
    case kStateDisconnected:
        ExitNow();
    }

    while (!mTxQueue.IsEmpty())
    {
        Message       &message         = *mTxQueue.GetHead();
        uint16_t       remainingLength = message.DetermineLengthAfterOffset();
        uint16_t       bytesSent;
        Message::Chunk chunk;

        message.GetFirstChunk(message.GetOffset(), remainingLength, chunk);

        while (chunk.GetLength() > 0)
        {
            bytesSent = otPlatTcpSend(this, chunk.GetBytes(), chunk.GetLength());

            message.SetOffset(message.GetOffset() + bytesSent);

            if (bytesSent < chunk.GetLength())
            {
                otPlatTcpNotifyTxPending(this);
                ExitNow();
            }

            message.GetNextChunk(remainingLength, chunk);
        }

        mTxQueue.DequeueAndFree(message);
    }

    SignalEvent(kEventSendDone);

    if (GetState() == kStateToClose)
    {
        Close();
    }

exit:
    return;
}

void PlatTcp::Connection::HandleReceive(const uint8_t *aBuffer, uint16_t aLength)
{
    Error error = kErrorNone;

    VerifyOrExit(aLength > 0);

    switch (GetState())
    {
    case kStateConnecting:
    case kStateConnected:
    case kStateToClose:
    case kStateClosing:
        break;
    case kStateUnused:
    case kStateDisconnected:
        ExitNow();
    }

    if (mRxMessage == nullptr)
    {
        mRxMessage = Get<MessagePool>().Allocate(Message::kTypeOther);
        VerifyOrExit(mRxMessage != nullptr, error = kErrorNoBufs);
    }

    mRxMessage->SetOffset(mRxMessage->GetLength());

    SuccessOrExit(error = mRxMessage->AppendBytes(aBuffer, aLength));

    SignalEvent(kEventReceive);

exit:
    if (error != kErrorNone)
    {
        Abort();
        mDisconnectReason = kDisconnectReasonNoBufs;
        SignalEvent(kEventDisconnected);
    }
}

void PlatTcp::Connection::RemoveParsedLengthFromRxMessage(uint16_t aRemoveLength)
{
    uint16_t newLength = 0;
    uint16_t newOffset = 0;

    VerifyOrExit(mRxMessage != nullptr);

    if (mRxMessage->GetLength() > aRemoveLength)
    {
        newLength = mRxMessage->GetLength() - aRemoveLength;
        newOffset = (mRxMessage->GetOffset() > aRemoveLength) ? mRxMessage->GetOffset() - aRemoveLength : 0;

        mRxMessage->WriteBytesFromMessage(/* aWriteOffset */ 0, *mRxMessage, /* aReadOffset */ aRemoveLength,
                                          newLength);
    }

    IgnoreError(mRxMessage->SetLength(newLength));
    mRxMessage->SetOffset(newOffset);

exit:
    return;
}

void PlatTcp::Connection::FreeRxMessage(void)
{
    VerifyOrExit(mRxMessage != nullptr);

    mRxMessage->Free();
    mRxMessage = nullptr;

exit:
    return;
}

void PlatTcp::Connection::Close(void)
{
    switch (GetState())
    {
    case kStateConnecting:
    case kStateConnected:
    case kStateToClose:
        break;

    case kStateUnused:
    case kStateDisconnected:
    case kStateClosing:
        ExitNow();
    }

    if (!mTxQueue.IsEmpty())
    {
        SetState(kStateToClose);
    }
    else
    {
        SetState(kStateClosing);
        otPlatTcpClose(this);
    }

exit:
    return;
}

void PlatTcp::Connection::Abort(void)
{
    switch (GetState())
    {
    case kStateConnecting:
    case kStateConnected:
    case kStateToClose:
    case kStateClosing:
        break;

    case kStateUnused:
    case kStateDisconnected:
        ExitNow();
    }

    mDisconnectReason = kDisconnectReasonAbort;
    SetState(kStateDisconnected);

    otPlatTcpAbort(this);

exit:
    return;
}

void PlatTcp::Connection::HandleDisconnected(DisconnectReason aReason)
{
    switch (GetState())
    {
    case kStateConnecting:
    case kStateConnected:
    case kStateToClose:
    case kStateClosing:
        break;

    case kStateUnused:
    case kStateDisconnected:
        ExitNow();
    }

    mDisconnectReason = aReason;
    SetState(kStateDisconnected);

    SignalEvent(kEventDisconnected);

exit:
    return;
}

void PlatTcp::Connection::SignalEvent(Event aEvent)
{
    VerifyOrExit(mEventHandler != nullptr);
    mEventHandler(*this, aEvent);

exit:
    return;
}

const char *PlatTcp::Connection::EventToString(Event aEvent)
{
#define ConnEventMapList(_)               \
    _(kEventConnected, "Connected")       \
    _(kEventDisconnected, "Disconnected") \
    _(kEventSendDone, "SendDone")         \
    _(kEventReceive, "Receive")

    DefineEnumStringArray(ConnEventMapList);

    return kStrings[aEvent];
}

const char *PlatTcp::Connection::DisconnectReasonToString(DisconnectReason aDisconnectReason)
{
#define ConnDisconnectReasonMapList(_)     \
    _(kDisconnectReasonClosed, "Closed")   \
    _(kDisconnectReasonTimeout, "Timeout") \
    _(kDisconnectReasonRefused, "Refused") \
    _(kDisconnectReasonReset, "Reset")     \
    _(kDisconnectReasonError, "Error")     \
    _(kDisconnectReasonAbort, "Abort")     \
    _(kDisconnectReasonNoBufs, "NoBufs")

    DefineEnumStringArray(ConnDisconnectReasonMapList);

    return kStrings[aDisconnectReason];
}

//---------------------------------------------------------------------------------------------------------------------
// PlatTcp

PlatTcp::PlatTcp(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mListenerTask(aInstance)
    , mConnectionTask(aInstance)
{
}

PlatTcp::~PlatTcp(void)
{
    for (Listener &listener : mListeners)
    {
        listener.Disable();
    }

    for (Connection &connection : mConnections)
    {
        connection.Abort();
    }

    HandleListenerTask();
    HandleConnectionTask();

    mListenerTask.Unpost();
    mConnectionTask.Unpost();
}

void PlatTcp::HandleListenerTask(void)
{
    LinkedList<Listener> disabledListeners;
    Listener            *listener;

    mListeners.RemoveAllMatching(disabledListeners, Listener::kStateDisabled);

    while ((listener = disabledListeners.Pop()) != nullptr)
    {
        listener->mNext = nullptr;
        listener->SetState(Listener::kStateUnused);
    }
}

void PlatTcp::HandleConnectionTask(void)
{
    LinkedList<Connection> disconnectedConnections;
    Connection            *connection;

    mConnections.RemoveAllMatching(disconnectedConnections, Connection::kStateDisconnected);

    while ((connection = disconnectedConnections.Pop()) != nullptr)
    {
        connection->mNext = nullptr;
        connection->SetState(Connection::kStateUnused);
    }
}

PlatTcp::Listener *PlatTcp::IterateListeners(Listener *aPrev)
{
    Listener *listener = (aPrev == nullptr) ? mListeners.GetHead() : aPrev->GetNext();

    while (listener != nullptr)
    {
        switch (listener->GetState())
        {
        case Listener::kStateUnused:
        case Listener::kStateDisabled:
            break;
        case Listener::kStateEnabled:
            ExitNow();
        }

        listener = listener->GetNext();
    }

exit:
    return listener;
}

PlatTcp::Connection *PlatTcp::IterateConnections(Connection *aPrev)
{
    Connection *connection = (aPrev == nullptr) ? mConnections.GetHead() : aPrev->GetNext();

    while (connection != nullptr)
    {
        switch (connection->GetState())
        {
        case Connection::kStateUnused:
        case Connection::kStateDisconnected:
            break;
        case Connection::kStateConnecting:
        case Connection::kStateConnected:
        case Connection::kStateToClose:
        case Connection::kStateClosing:
            ExitNow();
        }

        connection = connection->GetNext();
    }

exit:
    return connection;
}

} // namespace Ip6
} // namespace ot

#endif // OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE
