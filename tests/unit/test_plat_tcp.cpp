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

#include <openthread/config.h>

#include "test_platform.h"
#include "test_util.hpp"

#include "common/arg_macros.hpp"
#include "common/as_core_type.hpp"
#include "instance/instance.hpp"
#include "net/plat_tcp.hpp"

namespace ot {

#if OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE

// Logs a message
#define Log(...) printf(OT_FIRST_ARG(__VA_ARGS__) "\n" OT_REST_ARGS(__VA_ARGS__))

using SockAddr   = Ip6::PlatTcp::SockAddr;
using Listener   = Ip6::PlatTcp::Listener;
using Connection = Ip6::PlatTcp::Connection;

struct ListenerInfo
{
    Listener *mListener;
    SockAddr  mLocalSockAddr;
    bool      mEnabled;
};

static constexpr uint16_t kTxBufferSize = 1000;

struct ConnInfo
{
    Connection *mConnection;
    bool        mConnected;
    bool        mConnecting;
    bool        mClosing;
    bool        mAborted;
    bool        mNotifyTxPending;
    SockAddr    mLocalSockAddr;
    SockAddr    mPeerSockAddr;
    uint16_t    mTxBufferUsed;
    uint8_t     mTxBuffer[kTxBufferSize];
};

static ListenerInfo sListenerInfo;
static ConnInfo     sConnInfo;

static bool              sAcceptHandlerCalled;
static bool              sDeleteHandlerCalled;
static bool              sEventHandlerCalled;
static Connection::Event sLastEvent;

static void ResetTestState(void)
{
    ClearAllBytes(sListenerInfo);
    ClearAllBytes(sConnInfo);

    sAcceptHandlerCalled = false;
    sDeleteHandlerCalled = false;
    sEventHandlerCalled  = false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// otPlatTcp

extern "C" {

otError otPlatTcpEnableListener(otPlatTcpListener *aListener, const otPlatTcpSockAddr *aLocalSockAddr)
{
    sListenerInfo.mListener      = AsCoreTypePtr(aListener);
    sListenerInfo.mLocalSockAddr = AsCoreType(aLocalSockAddr);
    sListenerInfo.mEnabled       = true;

    Log(" - otPlatTcpEnableListener(%s)", sListenerInfo.mLocalSockAddr.ToString().AsCString());

    return kErrorNone;
}

void otPlatTcpDisableListener(otPlatTcpListener *aListener)
{
    Log(" - otPlatTcpDisableListener()");

    VerifyOrQuit(sListenerInfo.mListener == aListener);
    sListenerInfo.mEnabled = false;
}

otError otPlatTcpConnect(otPlatTcpConnection     *aConn,
                         const otPlatTcpSockAddr *aPeerSockAddr,
                         const otPlatTcpSockAddr *aLocalSockAddr)
{
    sConnInfo.mConnection   = AsCoreTypePtr(aConn);
    sConnInfo.mConnecting   = true;
    sConnInfo.mPeerSockAddr = AsCoreType(aPeerSockAddr);

    Log(" - otPlatTcpConnect(peerAddr:%s)", sConnInfo.mPeerSockAddr.ToString().AsCString());

    if (aLocalSockAddr != nullptr)
    {
        sConnInfo.mLocalSockAddr = AsCoreType(aLocalSockAddr);
        Log("   localAddr:%s", sConnInfo.mLocalSockAddr.ToString().AsCString());
    }

    return kErrorNone;
}

void otPlatTcpNotifyTxPending(otPlatTcpConnection *aConn)
{
    Log(" - otPlatTcpNotifyTxPending()");
    VerifyOrQuit(sConnInfo.mConnection == aConn);
    sConnInfo.mNotifyTxPending = true;
}

uint16_t otPlatTcpSend(otPlatTcpConnection *aConn, const uint8_t *aBuffer, uint16_t aLength)
{
    uint16_t copyLength;

    VerifyOrQuit(sConnInfo.mConnection == aConn);

    copyLength = Min<uint16_t>(aLength, kTxBufferSize - sConnInfo.mTxBufferUsed);

    memcpy(&sConnInfo.mTxBuffer[sConnInfo.mTxBufferUsed], aBuffer, copyLength);
    sConnInfo.mTxBufferUsed += copyLength;

    Log(" - otPlatTcpSend(aLength:%u), copied:%u", aLength, copyLength);

    return copyLength;
}

void otPlatTcpClose(otPlatTcpConnection *aConn)
{
    Log(" - otPlatTcpClose()");

    VerifyOrQuit(sConnInfo.mConnection == aConn);
    sConnInfo.mClosing = true;
}

void otPlatTcpAbort(otPlatTcpConnection *aConn)
{
    Log(" - otPlatTcpAbort()");

    VerifyOrQuit(sConnInfo.mConnection == aConn);
    sConnInfo.mAborted = true;
}

} // extern "C"

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Callbacks

static Connection *AcceptHandler(Listener &aListener, const SockAddr &aPeerSockAddr)
{
    Log(" - AcceptHandler(peerAddr:%s)", aPeerSockAddr.ToString().AsCString());

    OT_UNUSED_VARIABLE(aListener);
    sAcceptHandlerCalled = true;

    return sConnInfo.mConnection;
}

static void ListenerDeleteHandler(Listener &aListener)
{
    Log(" - ListenerDeleteHandler()");

    sDeleteHandlerCalled = true;
    OT_UNUSED_VARIABLE(aListener);
}

static void ConnectionDeleteHandler(Connection &aConnection)
{
    Log(" - ConnectionDeleteHandler()");

    sDeleteHandlerCalled = true;
    OT_UNUSED_VARIABLE(aConnection);
}

static void EventHandler(Connection &aConnection, Connection::Event aEvent)
{
    Log(" - EventHandler(%s)", Connection::EventToString(aEvent));

    sLastEvent          = aEvent;
    sEventHandlerCalled = true;
    OT_UNUSED_VARIABLE(aConnection);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void TestPlatTcpListener(void)
{
    Instance *instance = testInitInstance();
    Listener  listener(*instance, AcceptHandler, ListenerDeleteHandler);
    SockAddr  localAddr;

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("TestPlatTcpListener");

    ResetTestState();

    localAddr.SetPort(1234);

    Log(" Enable listener");
    SuccessOrQuit(listener.Enable(localAddr));
    VerifyOrQuit(listener.GetState() == Listener::kStateEnabled);
    VerifyOrQuit(sListenerInfo.mEnabled);
    VerifyOrQuit(sListenerInfo.mLocalSockAddr == localAddr);

    Log(" Check active listeners");
    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetListeners().GetHead() == &listener);
    VerifyOrQuit(listener.GetNext() == nullptr);

    Log(" Disable listener");
    sDeleteHandlerCalled = false;
    listener.Disable();
    VerifyOrQuit(listener.GetState() == Listener::kStateDisabled);
    VerifyOrQuit(!sListenerInfo.mEnabled);

    Log(" Verify delete handler is called");
    VerifyOrQuit(!sDeleteHandlerCalled);
    otTaskletsProcess(instance);
    VerifyOrQuit(sDeleteHandlerCalled);
    VerifyOrQuit(listener.GetState() == Listener::kStateUnused);

    Log(" Check active listeners list is empty");
    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetListeners().IsEmpty());

    testFreeInstance(instance);

    Log("End of TestPlatTcpListener");
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void TestPlatTcpConnection(void)
{
    Instance  *instance = testInitInstance();
    Connection connection(*instance, EventHandler, ConnectionDeleteHandler);
    SockAddr   peerAddr;

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("TestPlatTcpConnection");

    ResetTestState();

    SuccessOrQuit(peerAddr.GetAddress().FromString("fd00:1234::cafe"));
    peerAddr.SetPort(5678);
    peerAddr.SetIfIndex(4);

    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetConnections().IsEmpty());

    Log(" Connect");
    sEventHandlerCalled = false;
    SuccessOrQuit(connection.Connect(peerAddr));
    VerifyOrQuit(connection.GetState() == Connection::kStateConnecting);
    VerifyOrQuit(sConnInfo.mConnecting);
    VerifyOrQuit(!sEventHandlerCalled);

    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetConnections().GetHead() == &connection);
    VerifyOrQuit(connection.GetNext() == nullptr);

    Log(" Signal connected `otPlatTcpHandleConnected`");
    otPlatTcpHandleConnected(&connection);
    VerifyOrQuit(sEventHandlerCalled);
    VerifyOrQuit(sLastEvent == Connection::kEventConnected);
    VerifyOrQuit(connection.GetState() == Connection::kStateConnected);

    Log(" Disconnect from platform `otPlatTcpHandleDisconnected`");
    sEventHandlerCalled = false;
    otPlatTcpHandleDisconnected(&connection, OT_PLAT_TCP_DISCONNECT_REASON_CLOSED);
    VerifyOrQuit(sEventHandlerCalled);
    VerifyOrQuit(sLastEvent == Connection::kEventDisconnected);
    VerifyOrQuit(connection.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(connection.GetDisconnectReason() == Ip6::PlatTcp::kDisconnectReasonClosed);

    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetConnections().GetHead() == &connection);
    VerifyOrQuit(connection.GetNext() == nullptr);

    Log(" Cleanup");
    sDeleteHandlerCalled = false;
    otTaskletsProcess(instance);
    VerifyOrQuit(sDeleteHandlerCalled);
    VerifyOrQuit(connection.GetState() == Connection::kStateUnused);

    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetConnections().IsEmpty());

    testFreeInstance(instance);

    Log("End of TestPlatTcpConnection");
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void TestPlatTcpSendRecv(void)
{
    static const uint8_t kData[]  = {1, 2, 3, 4, 5, 6, 7};
    static const uint8_t kData2[] = {0xfe, 0xdc, 0xba};

    Instance         *instance = testInitInstance();
    Connection        connection(*instance, EventHandler, ConnectionDeleteHandler);
    SockAddr          localAddr;
    SockAddr          peerAddr;
    OwnedPtr<Message> message;
    const Message    *rxMessage;

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("TestPlatTcpSendRecv");

    ResetTestState();
    sConnInfo.mConnection = &connection;

    SuccessOrQuit(localAddr.GetAddress().FromString("fd00:beef:cafe::2"));
    localAddr.SetPort(4321);
    localAddr.SetIfIndex(7);

    SuccessOrQuit(peerAddr.GetAddress().FromString("fd00:beef:cafe::1"));
    peerAddr.SetPort(5678);
    peerAddr.SetIfIndex(7);

    Log(" Connect");

    SuccessOrQuit(connection.BindAndConnect(localAddr, peerAddr));
    otPlatTcpHandleConnected(&connection);
    VerifyOrQuit(connection.GetState() == Connection::kStateConnected);

    VerifyOrQuit(sConnInfo.mConnection == &connection);
    VerifyOrQuit(sConnInfo.mLocalSockAddr == localAddr);
    VerifyOrQuit(sConnInfo.mPeerSockAddr == peerAddr);

    Log(" Send data");

    message.Reset(instance->Get<MessagePool>().Allocate(Message::kTypeOther));
    SuccessOrQuit(message->AppendBytes(kData, sizeof(kData)));

    sEventHandlerCalled = false;
    SuccessOrQuit(connection.Send(message.PassOwnership()));
    VerifyOrQuit(!sEventHandlerCalled);
    VerifyOrQuit(sConnInfo.mNotifyTxPending);

    Log(" Signal `otPlatTcpHandleTxReady`");
    otPlatTcpHandleTxReady(&connection);
    VerifyOrQuit(sConnInfo.mTxBufferUsed == sizeof(kData));
    VerifyOrQuit(memcmp(sConnInfo.mTxBuffer, kData, sizeof(kData)) == 0);

    VerifyOrQuit(sEventHandlerCalled);
    VerifyOrQuit(sLastEvent == Connection::kEventSendDone);

    Log(" Send more data");

    message.Reset(instance->Get<MessagePool>().Allocate(Message::kTypeOther));
    SuccessOrQuit(message->AppendBytes(kData2, sizeof(kData2)));

    sEventHandlerCalled = false;
    SuccessOrQuit(connection.Send(message.PassOwnership()));
    VerifyOrQuit(!sEventHandlerCalled);
    VerifyOrQuit(sConnInfo.mNotifyTxPending);

    Log(" Signal `otPlatTcpHandleTxReady`");
    otPlatTcpHandleTxReady(&connection);
    VerifyOrQuit(sConnInfo.mTxBufferUsed == sizeof(kData) + sizeof(kData2));
    VerifyOrQuit(memcmp(sConnInfo.mTxBuffer, kData, sizeof(kData)) == 0);
    VerifyOrQuit(memcmp(&sConnInfo.mTxBuffer[sizeof(kData)], kData2, sizeof(kData2)) == 0);

    VerifyOrQuit(sEventHandlerCalled);
    VerifyOrQuit(sLastEvent == Connection::kEventSendDone);

    Log(" Receive data `otPlatTcpHandleReceive`");
    sEventHandlerCalled = false;
    otPlatTcpHandleReceive(&connection, kData, sizeof(kData));
    VerifyOrQuit(sEventHandlerCalled);
    VerifyOrQuit(sLastEvent == Connection::kEventReceive);

    rxMessage = connection.GetRxMessage();
    VerifyOrQuit(rxMessage != nullptr);
    VerifyOrQuit(rxMessage->GetLength() == sizeof(kData));
    VerifyOrQuit(rxMessage->CompareBytes(/* aOffset */ 0, kData, sizeof(kData)));

    Log(" Remove part of rx data");
    connection.RemoveParsedLengthFromRxMessage(2);
    rxMessage = connection.GetRxMessage();
    VerifyOrQuit(rxMessage->GetLength() == sizeof(kData) - 2);

    Log(" Receive more data `otPlatTcpHandleReceive`");
    sEventHandlerCalled = false;
    otPlatTcpHandleReceive(&connection, kData2, sizeof(kData2));
    VerifyOrQuit(sEventHandlerCalled);
    VerifyOrQuit(sLastEvent == Connection::kEventReceive);

    rxMessage = connection.GetRxMessage();
    VerifyOrQuit(rxMessage != nullptr);
    VerifyOrQuit(rxMessage->GetLength() == sizeof(kData) + sizeof(kData2) - 2);
    VerifyOrQuit(rxMessage->CompareBytes(/* aOffset */ sizeof(kData) - 2, kData2, sizeof(kData2)));
    VerifyOrQuit(rxMessage->GetOffset() == sizeof(kData) - 2);

    Log(" Free Rx message");
    connection.FreeRxMessage();
    VerifyOrQuit(connection.GetRxMessage() == nullptr);

    Log(" Close");
    sEventHandlerCalled = false;
    connection.Close();
    VerifyOrQuit(sConnInfo.mClosing);
    VerifyOrQuit(!sEventHandlerCalled);

    Log(" Signal `otPlatTcpHandleDisconnected`");
    otPlatTcpHandleDisconnected(&connection, OT_PLAT_TCP_DISCONNECT_REASON_CLOSED);

    VerifyOrQuit(sEventHandlerCalled);
    VerifyOrQuit(sLastEvent == Connection::kEventDisconnected);
    VerifyOrQuit(connection.GetState() == Connection::kStateDisconnected);

    Log(" Cleanup");
    sDeleteHandlerCalled = false;
    otTaskletsProcess(instance);
    VerifyOrQuit(sDeleteHandlerCalled);
    VerifyOrQuit(connection.GetState() == Connection::kStateUnused);

    testFreeInstance(instance);

    Log("End of TestPlatTcpSendRecv");
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void TestPlatTcpFlowControl(void)
{
    Instance         *instance = testInitInstance();
    Connection        connection(*instance, EventHandler, ConnectionDeleteHandler);
    SockAddr          peerAddr;
    OwnedPtr<Message> message1, message2;
    uint8_t           data[600];

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("TestPlatTcpFlowControl");

    ResetTestState();

    sConnInfo.mConnection = &connection;

    SuccessOrQuit(peerAddr.GetAddress().FromString("fd00:3f12::aaaa"));
    peerAddr.SetPort(9876);
    peerAddr.SetIfIndex(12);

    for (uint16_t i = 0; i < sizeof(data); i++)
    {
        data[i] = static_cast<uint8_t>(i & 0xff) ^ static_cast<uint8_t>(i >> 8);
    }

    Log(" Connect");
    SuccessOrQuit(connection.Connect(peerAddr));
    otPlatTcpHandleConnected(&connection);

    Log(" Partial send (buffer full)");

    message1.Reset(instance->Get<MessagePool>().Allocate(Message::kTypeIp6));
    SuccessOrQuit(message1->AppendBytes(data, 600));
    message2.Reset(instance->Get<MessagePool>().Allocate(Message::kTypeIp6));
    SuccessOrQuit(message2->AppendBytes(data, 600));

    SuccessOrQuit(connection.Send(message1.PassOwnership()));
    SuccessOrQuit(connection.Send(message2.PassOwnership()));

    Log(" Platform tx buffer size 1000. Validate partial write");

    sEventHandlerCalled = false;

    Log(" Signal `otPlatTcpHandleTxReady`");
    otPlatTcpHandleTxReady(&connection);

    VerifyOrQuit(sConnInfo.mTxBufferUsed == kTxBufferSize);
    VerifyOrQuit(memcmp(sConnInfo.mTxBuffer, data, sizeof(data)) == 0);
    VerifyOrQuit(memcmp(&sConnInfo.mTxBuffer[sizeof(data)], data, kTxBufferSize - sizeof(data)) == 0);

    VerifyOrQuit(!sEventHandlerCalled, "SendDone should NOT be called yet");

    Log(" Now clear platform tx buffer. Validate the rest of data is passed to platform");
    sConnInfo.mTxBufferUsed = 0;

    Log(" Signal `otPlatTcpHandleTxReady`");
    otPlatTcpHandleTxReady(&connection);

    VerifyOrQuit(sConnInfo.mTxBufferUsed == 2 * sizeof(data) - kTxBufferSize);
    VerifyOrQuit(memcmp(sConnInfo.mTxBuffer, &data[sizeof(data) - sConnInfo.mTxBufferUsed], sConnInfo.mTxBufferUsed) ==
                 0);

    VerifyOrQuit(sEventHandlerCalled, "SendDone should be called now");
    VerifyOrQuit(sLastEvent == Connection::kEventSendDone);

    Log(" Abort");
    connection.Abort();
    VerifyOrQuit(connection.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(sConnInfo.mAborted);

    testFreeInstance(instance);
    Log("End of TestPlatTcpFlowControl");
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void TestPlatTcpAccept(void)
{
    Instance   *instance = testInitInstance();
    Listener    listener(*instance, AcceptHandler, ListenerDeleteHandler);
    Connection  connection(*instance, EventHandler, ConnectionDeleteHandler);
    Connection *acceptedConn;
    SockAddr    localAddr;
    SockAddr    peerAddr;

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("TestPlatTcpAccept");

    ResetTestState();

    sConnInfo.mConnection = &connection;

    localAddr.SetPort(1234);

    SuccessOrQuit(peerAddr.GetAddress().FromString("fe80::1234"));
    peerAddr.SetPort(5678);

    Log(" Enable listener");
    SuccessOrQuit(listener.Enable(localAddr));

    Log(" Signal incoming connection request `otPlatTcpAccept`");
    sAcceptHandlerCalled = false;
    acceptedConn         = AsCoreTypePtr(otPlatTcpAccept(&listener, &peerAddr));

    VerifyOrQuit(sAcceptHandlerCalled);
    VerifyOrQuit(acceptedConn == &connection);
    VerifyOrQuit(connection.GetState() == Connection::kStateConnecting);

    Log(" Signal `otPlatTcpHandleConnected`");
    otPlatTcpHandleConnected(&connection);
    VerifyOrQuit(connection.GetState() == Connection::kStateConnected);

    testFreeInstance(instance);

    Log("End of TestPlatTcpAccept");
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void TestPlatTcpIteration(void)
{
    Instance   *instance = testInitInstance();
    Listener    listener1(*instance, AcceptHandler, ListenerDeleteHandler);
    Listener    listener2(*instance, AcceptHandler, ListenerDeleteHandler);
    Connection  connection1(*instance, EventHandler, ConnectionDeleteHandler);
    Connection  connection2(*instance, EventHandler, ConnectionDeleteHandler);
    Listener   *listener;
    Connection *connection;
    SockAddr    sockAddr;

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("TestPlatTcpIteration");

    ResetTestState();

    Log(" Enable two listeners");

    sockAddr.SetPort(1);
    SuccessOrQuit(listener1.Enable(sockAddr));
    sockAddr.SetPort(2);
    SuccessOrQuit(listener2.Enable(sockAddr));

    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetListeners().GetHead() == &listener2);
    VerifyOrQuit(listener2.GetNext() == &listener1);
    VerifyOrQuit(listener1.GetNext() == nullptr);

    Log(" Validate IterateListeners");

    listener = nullptr;
    listener = instance->Get<Ip6::PlatTcp>().IterateListeners(listener);
    VerifyOrQuit(listener == &listener2);
    listener = instance->Get<Ip6::PlatTcp>().IterateListeners(listener);
    VerifyOrQuit(listener == &listener1);
    listener = instance->Get<Ip6::PlatTcp>().IterateListeners(listener);
    VerifyOrQuit(listener == nullptr);

    Log(" Disable first listener");

    sListenerInfo.mListener = &listener1;
    listener1.Disable();

    Log(" Validate IterateListeners skips the disabled entry");
    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetListeners().GetHead() == &listener2);
    VerifyOrQuit(listener2.GetNext() == &listener1);
    VerifyOrQuit(listener1.GetNext() == nullptr);

    listener = nullptr;
    listener = instance->Get<Ip6::PlatTcp>().IterateListeners(listener);
    VerifyOrQuit(listener == &listener2);
    listener = instance->Get<Ip6::PlatTcp>().IterateListeners(listener);
    VerifyOrQuit(listener == nullptr);

    Log(" otTaskletsProcess");
    otTaskletsProcess(instance);
    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetListeners().GetHead() == &listener2);
    VerifyOrQuit(listener2.GetNext() == nullptr);

    Log(" Disable second listener");

    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetListeners().GetHead() == &listener2);
    VerifyOrQuit(listener2.GetNext() == nullptr);

    sListenerInfo.mListener = &listener2;
    listener2.Disable();

    listener = nullptr;
    listener = instance->Get<Ip6::PlatTcp>().IterateListeners(listener);
    VerifyOrQuit(listener == nullptr);

    Log(" otTaskletsProcess");
    otTaskletsProcess(instance);
    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetListeners().IsEmpty());

    listener = nullptr;
    listener = instance->Get<Ip6::PlatTcp>().IterateListeners(listener);
    VerifyOrQuit(listener == nullptr);

    Log(" Start two connections");

    sockAddr.SetPort(3);
    SuccessOrQuit(connection1.Connect(sockAddr));
    sockAddr.SetPort(4);
    SuccessOrQuit(connection2.Connect(sockAddr));

    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetConnections().GetHead() == &connection2);
    VerifyOrQuit(connection2.GetNext() == &connection1);
    VerifyOrQuit(connection1.GetNext() == nullptr);

    connection = nullptr;
    connection = instance->Get<Ip6::PlatTcp>().IterateConnections(connection);
    VerifyOrQuit(connection == &connection2);
    connection = instance->Get<Ip6::PlatTcp>().IterateConnections(connection);
    VerifyOrQuit(connection == &connection1);
    connection = instance->Get<Ip6::PlatTcp>().IterateConnections(connection);
    VerifyOrQuit(connection == nullptr);

    Log(" Abort second connection");

    sConnInfo.mConnection = &connection2;
    connection2.Abort();

    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetConnections().GetHead() == &connection2);
    VerifyOrQuit(connection2.GetNext() == &connection1);
    VerifyOrQuit(connection1.GetNext() == nullptr);

    connection = nullptr;
    connection = instance->Get<Ip6::PlatTcp>().IterateConnections(connection);
    VerifyOrQuit(connection == &connection1);
    connection = instance->Get<Ip6::PlatTcp>().IterateConnections(connection);
    VerifyOrQuit(connection == nullptr);

    Log(" otTaskletsProcess");
    otTaskletsProcess(instance);
    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetConnections().GetHead() == &connection1);
    VerifyOrQuit(connection1.GetNext() == nullptr);

    connection = nullptr;
    connection = instance->Get<Ip6::PlatTcp>().IterateConnections(connection);
    VerifyOrQuit(connection == &connection1);
    connection = instance->Get<Ip6::PlatTcp>().IterateConnections(connection);
    VerifyOrQuit(connection == nullptr);

    Log(" Abort first connection");

    sConnInfo.mConnection = &connection1;
    connection1.Abort();

    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetConnections().GetHead() == &connection1);
    VerifyOrQuit(connection1.GetNext() == nullptr);

    connection = nullptr;
    connection = instance->Get<Ip6::PlatTcp>().IterateConnections(connection);
    VerifyOrQuit(connection == nullptr);

    Log(" otTaskletsProcess");
    otTaskletsProcess(instance);
    VerifyOrQuit(instance->Get<Ip6::PlatTcp>().GetConnections().IsEmpty());

    connection = nullptr;
    connection = instance->Get<Ip6::PlatTcp>().IterateConnections(connection);
    VerifyOrQuit(connection == nullptr);

    testFreeInstance(instance);

    Log("End of TestPlatTcpIteration");
}

#endif // OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE

} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE
    ot::TestPlatTcpListener();
    ot::TestPlatTcpConnection();
    ot::TestPlatTcpSendRecv();
    ot::TestPlatTcpFlowControl();
    ot::TestPlatTcpAccept();
    ot::TestPlatTcpIteration();
#endif

    printf("All tests passed\n");
    return 0;
}
