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

#include <openthread/config.h>

#include "test_platform.h"
#include "test_util.hpp"

#include "common/arg_macros.hpp"
#include "common/array.hpp"
#include "common/as_core_type.hpp"
#include "common/instance.hpp"
#include "net/dns_dso.hpp"

#if OPENTHREAD_CONFIG_DNS_DSO_ENABLE

extern "C" {

static uint32_t    sNow = 0;
static uint32_t    sAlarmTime;
static bool        sAlarmOn = false;
static otInstance *sInstance;

// Logs a message and adds current time (sNow) as "<hours>:<min>:<secs>.<msec>"
#define Log(...)                                                                                          \
    printf("%02u:%02u:%02u.%03u " OT_FIRST_ARG(__VA_ARGS__) "\n", (sNow / 36000000), (sNow / 60000) % 60, \
           (sNow / 1000) % 60, sNow % 1000 OT_REST_ARGS(__VA_ARGS__))

void otPlatAlarmMilliStop(otInstance *)
{
    sAlarmOn = false;
}

void otPlatAlarmMilliStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    sAlarmOn   = true;
    sAlarmTime = aT0 + aDt;

    Log(" otPlatAlarmMilliStartAt(time:%u.%03u, dt:%u.%03u)", sAlarmTime / 1000, sAlarmTime % 1000,
        (sAlarmTime - sNow) / 1000, (sAlarmTime - sNow) % 1000);
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return sNow;
}

} // extern "C"

void AdvanceTime(uint32_t aDuration)
{
    uint32_t time = sNow + aDuration;

    Log(" AdvanceTime for %u.%03u", aDuration / 1000, aDuration % 1000);

    while (sAlarmTime <= time)
    {
        sNow = sAlarmTime;
        otPlatAlarmMilliFired(sInstance);
    }

    sNow = time;
}

namespace ot {
namespace Dns {

OT_TOOL_PACKED_BEGIN
class TestTlv : public Dso::Tlv
{
public:
    static constexpr Type kType = 0xf800;

    void Init(uint8_t aValue)
    {
        Tlv::Init(kType, sizeof(*this) - sizeof(Tlv));
        mValue = aValue;
    }

    bool    IsValid(void) const { return GetSize() >= sizeof(*this); }
    uint8_t GetValue(void) const { return mValue; }

private:
    uint8_t mValue;

} OT_TOOL_PACKED_END;

extern "C" void otPlatDsoSend(otPlatDsoConnection *aConnection, otMessage *aMessage);

class Connection : public Dso::Connection
{
    friend void otPlatDsoSend(otPlatDsoConnection *aConnection, otMessage *aMessage);

public:
    explicit Connection(Instance &           aInstance,
                        const char *         aName,
                        const Ip6::SockAddr &aLocalSockAddr,
                        const Ip6::SockAddr &aPeerSockAddr)
        : Dso::Connection(aInstance, aPeerSockAddr, sCallbacks)
        , mName(aName)
        , mLocalSockAddr(aLocalSockAddr)
    {
        ClearTestFlags();
    }

    const char *         GetName(void) const { return mName; }
    const Ip6::SockAddr &GetLocalSockAddr(void) const { return mLocalSockAddr; }

    void ClearTestFlags(void)
    {
        mDidGetConnectedSignal          = false;
        mDidGetSessionEstablishedSignal = false;
        mDidGetDisconnectSignal         = false;
        mDidSendMessage                 = false;
        mDidReceiveMessage              = false;
        mDidProcessRequest              = false;
        mDidProcessUnidirectional       = false;
        mDidProcessResponse             = false;
    }

    bool DidGetConnectedSignal(void) const { return mDidGetConnectedSignal; }
    bool DidGetSessionEstablishedSignal(void) const { return mDidGetSessionEstablishedSignal; }
    bool DidGetDisconnectSignal(void) const { return mDidGetDisconnectSignal; }
    bool DidSendMessage(void) const { return mDidSendMessage; }
    bool DidReceiveMessage(void) const { return mDidReceiveMessage; }
    bool DidProcessRequest(void) const { return mDidProcessRequest; }
    bool DidProcessUnidirectional(void) const { return mDidProcessUnidirectional; }
    bool DidProcessResponse(void) const { return mDidProcessResponse; }

    uint8_t               GetLastRxTestTlvValue(void) const { return mLastRxTestTlvValue; }
    Dns::Header::Response GetLastRxResponseCode(void) const { return mLastRxResponseCode; }

    void SendTestRequestMessage(uint8_t aValue = 0, uint32_t aResponseTimeout = Dso::kResponseTimeout)
    {
        MessageId messageId;

        mLastTxTestTlvValue = aValue;
        SuccessOrQuit(SendRequestMessage(PrepareTestMessage(aValue), messageId, aResponseTimeout));
    }

    void SendTestUnidirectionalMessage(uint8_t aValue = 0)
    {
        mLastTxTestTlvValue = aValue;
        SuccessOrQuit(SendUnidirectionalMessage(PrepareTestMessage(aValue)));
    }

private:
    Message &PrepareTestMessage(uint8_t aValue)
    {
        TestTlv  testTlv;
        Message *message = NewMessage();

        VerifyOrQuit(message != nullptr);
        testTlv.Init(aValue);
        SuccessOrQuit(message->Append(testTlv));

        return *message;
    }

    void ParseTestMessage(const Message &aMessage)
    {
        TestTlv  testTlv;
        Dso::Tlv tlv;
        uint16_t offset = aMessage.GetOffset();

        // Test message MUST only contain Test TLV and Encryption
        // Padding TLV.

        SuccessOrQuit(aMessage.Read(offset, testTlv));
        VerifyOrQuit(testTlv.GetType() == TestTlv::kType);
        VerifyOrQuit(testTlv.IsValid());
        offset += testTlv.GetSize();
        mLastRxTestTlvValue = testTlv.GetValue();

        SuccessOrQuit(aMessage.Read(offset, tlv));
        VerifyOrQuit(tlv.GetType() == Dso::Tlv::kEncryptionPaddingType);
        offset += tlv.GetSize();

        VerifyOrQuit(offset == aMessage.GetLength());
    }

    void SendTestResponseMessage(MessageId aResponseId, uint8_t aValue)
    {
        mLastTxTestTlvValue = aValue;
        SuccessOrQuit(SendResponseMessage(PrepareTestMessage(aValue), aResponseId));
    }

    //---------------------------------------------------------------------
    // Callback methods

    void HandleConnected(void) { mDidGetConnectedSignal = true; }
    void HandleSessionEstablished(void) { mDidGetSessionEstablishedSignal = true; }
    void HandleDisconnected(void) { mDidGetDisconnectSignal = true; }

    Error ProcessRequestMessage(MessageId aMessageId, const Message &aMessage, Dso::Tlv::Type aPrimaryTlvType)
    {
        Error error = kErrorNone;

        Log(" ProcessRequestMessage(primaryTlv:0x%04x) on %s", aPrimaryTlvType, mName);
        mDidProcessRequest = true;

        VerifyOrExit(aPrimaryTlvType == TestTlv::kType, error = kErrorNotFound);
        ParseTestMessage(aMessage);
        SendTestResponseMessage(aMessageId, mLastRxTestTlvValue);

    exit:
        return error;
    }

    Error ProcessUnidirectionalMessage(const Message &aMessage, Dso::Tlv::Type aPrimaryTlvType)
    {
        Log(" ProcessUnidirectionalMessage(primaryTlv:0x%04x) on %s", aPrimaryTlvType, mName);
        mDidProcessUnidirectional = true;

        if (aPrimaryTlvType == TestTlv::kType)
        {
            ParseTestMessage(aMessage);
        }

        return kErrorNone;
    }

    Error ProcessResponseMessage(const Dns::Header &aHeader,
                                 const Message &    aMessage,
                                 Dso::Tlv::Type     aResponseTlvType,
                                 Dso::Tlv::Type     aRequestTlvType)
    {
        Error error = kErrorNone;

        mDidProcessResponse = true;
        mLastRxResponseCode = aHeader.GetResponseCode();
        Log(" ProcessResponseMessage(responseTlv:0x%04x) on %s (response-Code:%u) ", aResponseTlvType, mName,
            mLastRxResponseCode);

        VerifyOrExit(mLastRxResponseCode == Dns::Header::kResponseSuccess);

        // During test we only expect a Test TLV response with
        // a matching TLV value to what was sent last.

        VerifyOrQuit(aResponseTlvType == TestTlv::kType);
        VerifyOrQuit(aRequestTlvType == TestTlv::kType);
        ParseTestMessage(aMessage);
        VerifyOrQuit(mLastRxTestTlvValue == mLastTxTestTlvValue);

    exit:
        return error;
    }

    static void HandleConnected(Dso::Connection &aConnection)
    {
        static_cast<Connection &>(aConnection).HandleConnected();
    }

    static void HandleSessionEstablished(Dso::Connection &aConnection)
    {
        static_cast<Connection &>(aConnection).HandleSessionEstablished();
    }

    static void HandleDisconnected(Dso::Connection &aConnection)
    {
        static_cast<Connection &>(aConnection).HandleDisconnected();
    }

    static Error ProcessRequestMessage(Dso::Connection &aConnection,
                                       MessageId        aMessageId,
                                       const Message &  aMessage,
                                       Dso::Tlv::Type   aPrimaryTlvType)
    {
        return static_cast<Connection &>(aConnection).ProcessRequestMessage(aMessageId, aMessage, aPrimaryTlvType);
    }

    static Error ProcessUnidirectionalMessage(Dso::Connection &aConnection,
                                              const Message &  aMessage,
                                              Dso::Tlv::Type   aPrimaryTlvType)
    {
        return static_cast<Connection &>(aConnection).ProcessUnidirectionalMessage(aMessage, aPrimaryTlvType);
    }

    static Error ProcessResponseMessage(Dso::Connection &  aConnection,
                                        const Dns::Header &aHeader,
                                        const Message &    aMessage,
                                        Dso::Tlv::Type     aResponseTlvType,
                                        Dso::Tlv::Type     aRequestTlvType)
    {
        return static_cast<Connection &>(aConnection)
            .ProcessResponseMessage(aHeader, aMessage, aResponseTlvType, aRequestTlvType);
    }

    const char *          mName;
    Ip6::SockAddr         mLocalSockAddr;
    bool                  mDidGetConnectedSignal;
    bool                  mDidGetSessionEstablishedSignal;
    bool                  mDidGetDisconnectSignal;
    bool                  mDidSendMessage;
    bool                  mDidReceiveMessage;
    bool                  mDidProcessRequest;
    bool                  mDidProcessUnidirectional;
    bool                  mDidProcessResponse;
    uint8_t               mLastTxTestTlvValue;
    uint8_t               mLastRxTestTlvValue;
    Dns::Header::Response mLastRxResponseCode;

    static Callbacks sCallbacks;
};

Dso::Connection::Callbacks Connection::sCallbacks(Connection::HandleConnected,
                                                  Connection::HandleSessionEstablished,
                                                  Connection::HandleDisconnected,
                                                  Connection::ProcessRequestMessage,
                                                  Connection::ProcessUnidirectionalMessage,
                                                  Connection::ProcessResponseMessage);

static constexpr uint16_t kMaxConnections = 5;

static Array<Connection *, kMaxConnections> sConnections;

static Connection *FindPeerConnection(const Connection &aConnetion)
{
    Connection *peerConn = nullptr;

    for (Connection *conn : sConnections)
    {
        if (conn->GetLocalSockAddr() == aConnetion.GetPeerSockAddr())
        {
            peerConn = conn;
            break;
        }
    }

    return peerConn;
}

extern "C" {

static bool sDsoListening = false;

// This test flag indicates whether the `otPlatDso` API should
// forward a sent message to the peer connection. It can be set to
// `false` to drop the messages to test timeout behaviors on the
// peer.
static bool sTestDsoForwardMessageToPeer = true;

// This test flag indicate whether when disconnecting a connection
// (using `otPlatDsoDisconnect()` to signal the peer connection about
// the disconnect. Default behavior is set to `true`. It can be set
// to `false` to test certain timeout behavior on peer side.
static bool sTestDsoSignalDisconnectToPeer = true;

void otPlatDsoEnableListening(otInstance *, bool aEnable)
{
    Log(" otPlatDsoEnableListening(%s)", aEnable ? "true" : "false");
    sDsoListening = aEnable;
}

void otPlatDsoConnect(otPlatDsoConnection *aConnection, const otSockAddr *aPeerSockAddr)
{
    Connection &         conn         = *static_cast<Connection *>(aConnection);
    Connection *         peerConn     = nullptr;
    const Ip6::SockAddr &peerSockAddr = AsCoreType(aPeerSockAddr);

    Log(" otPlatDsoConnect(%s, aPeer:0x%04x)", conn.GetName(), peerSockAddr.GetPort());

    VerifyOrQuit(conn.GetPeerSockAddr() == peerSockAddr);
    VerifyOrQuit(conn.GetState() == Connection::kStateConnecting);

    if (!sDsoListening)
    {
        Log("   Server is not listening");
        ExitNow();
    }

    peerConn = static_cast<Connection *>(otPlatDsoAccept(otPlatDsoGetInstance(aConnection), aPeerSockAddr));

    if (peerConn == nullptr)
    {
        Log("   Request rejected");
        ExitNow();
    }

    Log("   Request accepted");
    VerifyOrQuit(peerConn->GetState() == Connection::kStateConnecting);

    Log("   Signalling `Connected` on peer connection (%s)", peerConn->GetName());
    otPlatDsoHandleConnected(peerConn);

    Log("   Signalling `Connected` on connection (%s)", conn.GetName());
    otPlatDsoHandleConnected(aConnection);

exit:
    return;
}

void otPlatDsoSend(otPlatDsoConnection *aConnection, otMessage *aMessage)
{
    Connection &conn     = *static_cast<Connection *>(aConnection);
    Connection *peerConn = nullptr;

    Log(" otPlatDsoSend(%s), message-len:%u", conn.GetName(), AsCoreType(aMessage).GetLength());

    VerifyOrQuit(conn.GetState() != Connection::kStateDisconnected);
    VerifyOrQuit(conn.GetState() != Connection::kStateConnecting);
    conn.mDidSendMessage = true;

    if (sTestDsoForwardMessageToPeer)
    {
        peerConn = FindPeerConnection(conn);
        VerifyOrQuit(peerConn != nullptr);

        VerifyOrQuit(peerConn->GetState() != Connection::kStateDisconnected);
        VerifyOrQuit(peerConn->GetState() != Connection::kStateConnecting);

        Log("   Sending the message to peer connection (%s)", peerConn->GetName());

        peerConn->mDidReceiveMessage = true;
        otPlatDsoHandleReceive(peerConn, aMessage);
    }
    else
    {
        Log("   Dropping the message");
    }
}

void otPlatDsoDisconnect(otPlatDsoConnection *aConnection, otPlatDsoDisconnectMode aMode)
{
    Connection &conn     = *static_cast<Connection *>(aConnection);
    Connection *peerConn = nullptr;

    Log(" otPlatDsoDisconnect(%s, mode:%s)", conn.GetName(),
        (aMode == OT_PLAT_DSO_DISCONNECT_MODE_GRACEFULLY_CLOSE) ? "close" : "abort");

    VerifyOrQuit(conn.GetState() == Connection::kStateDisconnected);

    if (sTestDsoSignalDisconnectToPeer)
    {
        peerConn = FindPeerConnection(conn);

        if (peerConn == nullptr)
        {
            Log("   No peer connection found");
        }
        else if (peerConn->GetState() == Connection::kStateDisconnected)
        {
            Log("   Peer connection (%s) already disconnected", peerConn->GetName());
        }
        else
        {
            Log("   Signaling `Disconnected` on peer connection (%s)", peerConn->GetName());
            otPlatDsoHandleDisconnected(peerConn, aMode);
        }
    }
}

} // extern "C"

Dso::Connection *AcceptConnection(Instance &aInstance, const Ip6::SockAddr &aPeerSockAddr)
{
    OT_UNUSED_VARIABLE(aInstance);

    Connection *rval = nullptr;

    Log("  AcceptConnection(peer:0x%04x)", aPeerSockAddr.GetPort());

    for (Connection *conn : sConnections)
    {
        if (conn->GetLocalSockAddr() == aPeerSockAddr)
        {
            VerifyOrQuit(conn->GetState() == Connection::kStateDisconnected);
            rval = conn;
            break;
        }
    }

    if (rval != nullptr)
    {
        Log("   Accepting and returning connection %s", rval->GetName());
    }
    else
    {
        Log("   Rejecting");
    }

    return rval;
}

static constexpr uint8_t kKeepAliveTestIterations = 3;

static void VerifyKeepAliveExchange(Connection &aClientConn,
                                    Connection &aServerConn,
                                    uint32_t    aKeepAliveInterval,
                                    uint8_t     aNumIterations = kKeepAliveTestIterations)
{
    for (uint8_t n = 0; n < aNumIterations; n++)
    {
        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Test Keep Alive message exchange, iter %d", n + 1);

        aClientConn.ClearTestFlags();
        aServerConn.ClearTestFlags();

        AdvanceTime(aKeepAliveInterval - 1);
        VerifyOrQuit(!aClientConn.DidSendMessage());
        VerifyOrQuit(!aServerConn.DidReceiveMessage());
        Log("No message before keep alive timeout");

        AdvanceTime(1);
        VerifyOrQuit(aClientConn.DidSendMessage());
        VerifyOrQuit(aServerConn.DidReceiveMessage());
        Log("KeepAlive message exchanged after keep alive time elapses");
    }
}

void TestDso(void)
{
    static constexpr uint16_t kPortA = 0xaaaa;
    static constexpr uint16_t kPortB = 0xbbbb;

    static constexpr Dso::Tlv::Type kUnknownTlvType = 0xf801;

    static constexpr uint32_t kRetryDelayInterval  = TimeMilli::SecToMsec(3600);
    static constexpr uint32_t kLongResponseTimeout = Dso::kResponseTimeout + TimeMilli::SecToMsec(17);

    Instance &            instance = *static_cast<Instance *>(testInitInstance());
    Ip6::SockAddr         serverSockAddr(kPortA);
    Ip6::SockAddr         clientSockAddr(kPortB);
    Connection            serverConn(instance, "serverConn", serverSockAddr, clientSockAddr);
    Connection            clientConn(instance, "clinetConn", clientSockAddr, serverSockAddr);
    Message *             message;
    Dso::Tlv              tlv;
    Connection::MessageId messageId;

    sNow      = 0;
    sInstance = &instance;

    SuccessOrQuit(sConnections.PushBack(&serverConn));
    SuccessOrQuit(sConnections.PushBack(&clientConn));

    VerifyOrQuit(serverConn.GetPeerSockAddr() == clientSockAddr);
    VerifyOrQuit(clientConn.GetPeerSockAddr() == serverSockAddr);

    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);

    instance.Get<Dso>().StartListening(AcceptConnection);

    VerifyOrQuit(instance.Get<Dso>().FindClientConnection(clientSockAddr) == nullptr);
    VerifyOrQuit(instance.Get<Dso>().FindServerConnection(clientSockAddr) == nullptr);
    VerifyOrQuit(instance.Get<Dso>().FindClientConnection(serverSockAddr) == nullptr);
    VerifyOrQuit(instance.Get<Dso>().FindServerConnection(serverSockAddr) == nullptr);

    Log("-------------------------------------------------------------------------------------------");
    Log("Connect from client to server");

    clientConn.Connect();

    VerifyOrQuit(clientConn.GetState() == Connection::kStateConnectedButSessionless);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateConnectedButSessionless);

    VerifyOrQuit(clientConn.IsClient());
    VerifyOrQuit(!clientConn.IsServer());

    VerifyOrQuit(!serverConn.IsClient());
    VerifyOrQuit(serverConn.IsServer());

    // Note that we find connection with a peer address
    VerifyOrQuit(instance.Get<Dso>().FindClientConnection(serverSockAddr) == &clientConn);
    VerifyOrQuit(instance.Get<Dso>().FindServerConnection(serverSockAddr) == nullptr);
    VerifyOrQuit(instance.Get<Dso>().FindClientConnection(clientSockAddr) == nullptr);
    VerifyOrQuit(instance.Get<Dso>().FindServerConnection(clientSockAddr) == &serverConn);

    VerifyOrQuit(clientConn.DidGetConnectedSignal());
    VerifyOrQuit(!clientConn.DidGetSessionEstablishedSignal());
    VerifyOrQuit(!clientConn.DidGetDisconnectSignal());

    VerifyOrQuit(serverConn.DidGetConnectedSignal());
    VerifyOrQuit(!serverConn.DidGetSessionEstablishedSignal());
    VerifyOrQuit(!serverConn.DidGetDisconnectSignal());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send keep alive message to establish connection");

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    SuccessOrQuit(clientConn.SendKeepAliveMessage());

    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    VerifyOrQuit(!clientConn.DidGetConnectedSignal());
    VerifyOrQuit(clientConn.DidGetSessionEstablishedSignal());
    VerifyOrQuit(!clientConn.DidGetDisconnectSignal());

    VerifyOrQuit(!serverConn.DidGetConnectedSignal());
    VerifyOrQuit(serverConn.DidGetSessionEstablishedSignal());
    VerifyOrQuit(!serverConn.DidGetDisconnectSignal());

    VerifyOrQuit(clientConn.GetKeepAliveInterval() == Dso::kDefaultTimeout);
    VerifyOrQuit(clientConn.GetInactivityTimeout() == Dso::kDefaultTimeout);
    VerifyOrQuit(serverConn.GetKeepAliveInterval() == Dso::kDefaultTimeout);
    VerifyOrQuit(serverConn.GetInactivityTimeout() == Dso::kDefaultTimeout);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Close connection");

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    clientConn.Disconnect(Connection::kGracefullyClose, Connection::kReasonInactivityTimeout);

    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonInactivityTimeout);

    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonPeerClosed);

    VerifyOrQuit(!clientConn.DidGetConnectedSignal());
    VerifyOrQuit(!clientConn.DidGetSessionEstablishedSignal());
    VerifyOrQuit(!clientConn.DidGetDisconnectSignal());

    VerifyOrQuit(!serverConn.DidGetConnectedSignal());
    VerifyOrQuit(!serverConn.DidGetSessionEstablishedSignal());
    VerifyOrQuit(serverConn.DidGetDisconnectSignal());

    VerifyOrQuit(instance.Get<Dso>().FindClientConnection(clientSockAddr) == nullptr);
    VerifyOrQuit(instance.Get<Dso>().FindServerConnection(clientSockAddr) == nullptr);
    VerifyOrQuit(instance.Get<Dso>().FindClientConnection(serverSockAddr) == nullptr);
    VerifyOrQuit(instance.Get<Dso>().FindServerConnection(serverSockAddr) == nullptr);

    Log("-------------------------------------------------------------------------------------------");
    Log("Connection timeout when server is not listening");

    instance.Get<Dso>().StopListening();

    clientConn.ClearTestFlags();

    clientConn.Connect();
    VerifyOrQuit(clientConn.GetState() == Connection::kStateConnecting);
    VerifyOrQuit(instance.Get<Dso>().FindClientConnection(serverSockAddr) == &clientConn);
    VerifyOrQuit(instance.Get<Dso>().FindServerConnection(serverSockAddr) == nullptr);

    AdvanceTime(Dso::kConnectingTimeout);

    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonFailedToConnect);
    VerifyOrQuit(instance.Get<Dso>().FindClientConnection(serverSockAddr) == nullptr);
    VerifyOrQuit(instance.Get<Dso>().FindServerConnection(serverSockAddr) == nullptr);

    VerifyOrQuit(!clientConn.DidGetConnectedSignal());
    VerifyOrQuit(!clientConn.DidGetSessionEstablishedSignal());
    VerifyOrQuit(clientConn.DidGetDisconnectSignal());

    Log("-------------------------------------------------------------------------------------------");
    Log("Keep Alive Timeout behavior");

    // Keep Alive timeout smaller than min value should be rejected.
    VerifyOrQuit(clientConn.SetTimeouts(Dso::kInfiniteTimeout, Dso::kMinKeepAliveInterval - 1) == kErrorInvalidArgs);

    instance.Get<Dso>().StartListening(AcceptConnection);
    SuccessOrQuit(serverConn.SetTimeouts(Dso::kInfiniteTimeout, Dso::kMinKeepAliveInterval));

    VerifyOrQuit(serverConn.GetKeepAliveInterval() == Dso::kMinKeepAliveInterval);
    VerifyOrQuit(serverConn.GetInactivityTimeout() == Dso::kInfiniteTimeout);

    clientConn.Connect();
    SuccessOrQuit(clientConn.SendKeepAliveMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    VerifyOrQuit(serverConn.GetKeepAliveInterval() == Dso::kMinKeepAliveInterval);
    VerifyOrQuit(serverConn.GetInactivityTimeout() == Dso::kInfiniteTimeout);
    VerifyOrQuit(clientConn.GetKeepAliveInterval() == Dso::kMinKeepAliveInterval);
    VerifyOrQuit(clientConn.GetInactivityTimeout() == Dso::kInfiniteTimeout);

    VerifyKeepAliveExchange(clientConn, serverConn, Dso::kMinKeepAliveInterval);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Change Keep Alive interval on server");

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    SuccessOrQuit(serverConn.SetTimeouts(Dso::kInfiniteTimeout, Dso::kDefaultTimeout));

    VerifyOrQuit(serverConn.DidSendMessage());
    VerifyOrQuit(clientConn.DidReceiveMessage());
    VerifyOrQuit(!clientConn.DidSendMessage());

    VerifyOrQuit(serverConn.GetKeepAliveInterval() == Dso::kDefaultTimeout);
    VerifyOrQuit(serverConn.GetInactivityTimeout() == Dso::kInfiniteTimeout);
    VerifyOrQuit(clientConn.GetKeepAliveInterval() == Dso::kDefaultTimeout);
    VerifyOrQuit(clientConn.GetInactivityTimeout() == Dso::kInfiniteTimeout);

    VerifyKeepAliveExchange(clientConn, serverConn, Dso::kDefaultTimeout);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Keep Alive timer clear on message send or receive");

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    AdvanceTime(Dso::kDefaultTimeout / 2);

    clientConn.SendTestUnidirectionalMessage();
    VerifyOrQuit(clientConn.DidSendMessage());
    VerifyOrQuit(serverConn.DidReceiveMessage());
    VerifyOrQuit(!serverConn.DidSendMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);
    Log("Sent unidirectional message (client->server) at half the keep alive interval");
    VerifyKeepAliveExchange(clientConn, serverConn, Dso::kDefaultTimeout, 1);

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    AdvanceTime(Dso::kDefaultTimeout / 2);

    serverConn.SendTestUnidirectionalMessage();
    VerifyOrQuit(serverConn.DidSendMessage());
    VerifyOrQuit(clientConn.DidReceiveMessage());
    VerifyOrQuit(!clientConn.DidSendMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);
    Log("Sent unidirectional message (server->client) at half the keep alive interval");
    VerifyKeepAliveExchange(clientConn, serverConn, Dso::kDefaultTimeout, 1);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Keep Alive timeout on server");

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    Log("Drop all sent message (drop Keep Alive msg from client->server)");
    sTestDsoForwardMessageToPeer = false;

    AdvanceTime(Dso::kDefaultTimeout);
    VerifyOrQuit(clientConn.DidSendMessage());
    VerifyOrQuit(!serverConn.DidReceiveMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    Log("Sever waits for twice the interval before Keep Alive timeout");
    AdvanceTime(Dso::kDefaultTimeout);

    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonKeepAliveTimeout);

    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonPeerAborted);
    Log("Server aborted connection on Keep Alive timeout");
    sTestDsoForwardMessageToPeer = true;

    Log("-------------------------------------------------------------------------------------------");
    Log("Inactivity Timeout behavior");

    SuccessOrQuit(serverConn.SetTimeouts(Dso::kDefaultTimeout, Dso::kMinKeepAliveInterval));

    VerifyOrQuit(serverConn.GetKeepAliveInterval() == Dso::kMinKeepAliveInterval);
    VerifyOrQuit(serverConn.GetInactivityTimeout() == Dso::kDefaultTimeout);

    clientConn.Connect();
    SuccessOrQuit(clientConn.SendKeepAliveMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    VerifyOrQuit(serverConn.GetKeepAliveInterval() == Dso::kMinKeepAliveInterval);
    VerifyOrQuit(serverConn.GetInactivityTimeout() == Dso::kDefaultTimeout);
    VerifyOrQuit(clientConn.GetKeepAliveInterval() == Dso::kMinKeepAliveInterval);
    VerifyOrQuit(clientConn.GetInactivityTimeout() == Dso::kDefaultTimeout);

    Log("Sending a unidirectional message should clear inactivity timer");
    AdvanceTime(Dso::kDefaultTimeout / 2);
    clientConn.SendTestUnidirectionalMessage();

    AdvanceTime(Dso::kDefaultTimeout - 1);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);
    Log("Client keeps the connection up to the inactivity timeout");

    AdvanceTime(1);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonInactivityTimeout);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonPeerClosed);
    Log("Client closes the connection gracefully on inactivity timeout");

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Increasing inactivity timeout in middle");

    clientConn.Connect();
    SuccessOrQuit(clientConn.SendKeepAliveMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    AdvanceTime(TimeMilli::SecToMsec(10));
    Log("After 10 sec elapses, change the inactivity timeout from 15 to 20 sec");
    SuccessOrQuit(serverConn.SetTimeouts(TimeMilli::SecToMsec(20), Dso::kMinKeepAliveInterval));

    AdvanceTime(TimeMilli::SecToMsec(10) - 1);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);
    Log("Client keeps the connection up to new 20 sec inactivity timeout");

    AdvanceTime(1);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonInactivityTimeout);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonPeerClosed);
    Log("Client closes the connection gracefully on inactivity timeout of 20 sec");

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Decreasing inactivity timeout in middle");

    clientConn.Connect();
    SuccessOrQuit(clientConn.SendKeepAliveMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    AdvanceTime(TimeMilli::SecToMsec(10));
    Log("After 10 sec elapses, change the inactivity timeout from 15 to 10 sec");
    SuccessOrQuit(serverConn.SetTimeouts(TimeMilli::SecToMsec(10), Dso::kMinKeepAliveInterval));

    AdvanceTime(0);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonInactivityTimeout);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonPeerClosed);
    Log("Client closes the connection gracefully on new shorter inactivity timeout of 10 sec");

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Changing inactivity timeout from infinite to finite");

    SuccessOrQuit(serverConn.SetTimeouts(Dso::kDefaultTimeout, Dso::kInfiniteTimeout));
    clientConn.Connect();
    SuccessOrQuit(clientConn.SendKeepAliveMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    AdvanceTime(TimeMilli::SecToMsec(6));
    Log("After 6 sec, change the inactivity to infinite");
    SuccessOrQuit(serverConn.SetTimeouts(Dso::kInfiniteTimeout, Dso::kInfiniteTimeout));

    AdvanceTime(TimeMilli::SecToMsec(4));
    Log("After 4 sec, change the inactivity timeout from infinite to 20 sec");
    SuccessOrQuit(serverConn.SetTimeouts(TimeMilli::SecToMsec(20), Dso::kInfiniteTimeout));

    AdvanceTime(TimeMilli::SecToMsec(10) - 1);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    AdvanceTime(1);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonInactivityTimeout);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonPeerClosed);
    Log("Client closes the connection gracefully after 20 sec since last activity");

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Tracking activity while inactivity timeout is infinite");

    SuccessOrQuit(serverConn.SetTimeouts(Dso::kInfiniteTimeout, Dso::kInfiniteTimeout));
    clientConn.Connect();
    SuccessOrQuit(clientConn.SendKeepAliveMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    AdvanceTime(TimeMilli::SecToMsec(7));
    Log("After 7 sec, send a test message, this clears inactivity timer");
    serverConn.SendTestUnidirectionalMessage();

    AdvanceTime(TimeMilli::SecToMsec(10));
    Log("After 10 sec, change the inactivity timeout from infinite to 15 sec");
    SuccessOrQuit(serverConn.SetTimeouts(TimeMilli::SecToMsec(15), Dso::kInfiniteTimeout));

    AdvanceTime(TimeMilli::SecToMsec(5) - 1);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    AdvanceTime(1);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonInactivityTimeout);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonPeerClosed);
    Log("Client closes the connection gracefully after 15 sec since last activity");

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Inactivity timeout on server");

    clientConn.Connect();
    SuccessOrQuit(clientConn.SendKeepAliveMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    SuccessOrQuit(serverConn.SetTimeouts(Dso::kDefaultTimeout, Dso::kInfiniteTimeout));

    Log("Wait for inactivity timeout and ensure client disconnect");
    Log("Configure test for client not to signal its disconnect to server");
    sTestDsoSignalDisconnectToPeer = false;

    AdvanceTime(Dso::kDefaultTimeout);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonInactivityTimeout);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);
    sTestDsoSignalDisconnectToPeer = true;

    Log("Server should disconnect after twice the inactivity timeout");
    AdvanceTime(Dso::kDefaultTimeout - 1);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);
    AdvanceTime(1);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonInactivityTimeout);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Server reducing inactivity timeout to expired (on server)");

    clientConn.Connect();
    SuccessOrQuit(clientConn.SendKeepAliveMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);
    SuccessOrQuit(serverConn.SetTimeouts(Dso::kDefaultTimeout, Dso::kInfiniteTimeout));

    AdvanceTime(TimeMilli::SecToMsec(10));
    Log("After 11 sec elapses, change the inactivity timeout from 15 to 2 sec");
    SuccessOrQuit(serverConn.SetTimeouts(TimeMilli::SecToMsec(2), Dso::kMinKeepAliveInterval));

    sTestDsoSignalDisconnectToPeer = false;
    AdvanceTime(0);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonInactivityTimeout);
    sTestDsoSignalDisconnectToPeer = true;
    Log("Client closes the connection gracefully on expired timeout");
    Log("Configure test for client not to signal its disconnect to server");

    AdvanceTime(Dso::kMinServerInactivityWaitTime - 1);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);
    AdvanceTime(1);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonInactivityTimeout);
    Log("Server wait for kMinServerInactivityWaitTime (5 sec) before closing on expired timeout");

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Long-lived operation");

    clientConn.Connect();
    SuccessOrQuit(clientConn.SendKeepAliveMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);
    SuccessOrQuit(serverConn.SetTimeouts(Dso::kDefaultTimeout, Dso::kInfiniteTimeout));

    clientConn.SetLongLivedOperation(true);
    serverConn.SetLongLivedOperation(true);

    AdvanceTime(2 * Dso::kDefaultTimeout);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    clientConn.SetLongLivedOperation(false);
    AdvanceTime(0);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);

    Log("-------------------------------------------------------------------------------------------");
    Log("Request, response, and unidirectional message exchange");

    SuccessOrQuit(serverConn.SetTimeouts(Dso::kDefaultTimeout, Dso::kDefaultTimeout));
    clientConn.Connect();

    VerifyOrQuit(clientConn.GetState() == Connection::kStateConnectedButSessionless);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateConnectedButSessionless);

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Establish connection using test message request/response");
    clientConn.SendTestRequestMessage();

    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.DidProcessRequest());
    VerifyOrQuit(clientConn.DidProcessResponse());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send unidirectional test message");

    serverConn.ClearTestFlags();
    clientConn.SendTestUnidirectionalMessage(0x10);
    VerifyOrQuit(serverConn.DidProcessUnidirectional());
    VerifyOrQuit(serverConn.GetLastRxTestTlvValue() == 0x10);

    clientConn.ClearTestFlags();
    serverConn.SendTestUnidirectionalMessage(0x20);
    VerifyOrQuit(clientConn.DidProcessUnidirectional());
    VerifyOrQuit(clientConn.GetLastRxTestTlvValue() == 0x20);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Exchange request and response");

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();
    serverConn.SendTestRequestMessage(0x30);
    VerifyOrQuit(clientConn.DidProcessRequest());
    VerifyOrQuit(!clientConn.DidProcessResponse());
    VerifyOrQuit(!serverConn.DidProcessRequest());
    VerifyOrQuit(serverConn.DidProcessResponse());
    VerifyOrQuit(serverConn.GetLastRxTestTlvValue() == 0x30);
    VerifyOrQuit(clientConn.GetLastRxTestTlvValue() == 0x30);

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();
    clientConn.SendTestRequestMessage(0x40);
    VerifyOrQuit(!clientConn.DidProcessRequest());
    VerifyOrQuit(clientConn.DidProcessResponse());
    VerifyOrQuit(serverConn.DidProcessRequest());
    VerifyOrQuit(!serverConn.DidProcessResponse());
    VerifyOrQuit(serverConn.GetLastRxTestTlvValue() == 0x40);
    VerifyOrQuit(clientConn.GetLastRxTestTlvValue() == 0x40);

    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send unknown TLV request");

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    message = clientConn.NewMessage();
    VerifyOrQuit(message != nullptr);
    tlv.Init(kUnknownTlvType, 0);
    SuccessOrQuit(message->Append(tlv));
    SuccessOrQuit(clientConn.SendRequestMessage(*message, messageId));

    VerifyOrQuit(!clientConn.DidProcessRequest());
    VerifyOrQuit(clientConn.DidProcessResponse());
    VerifyOrQuit(serverConn.DidProcessRequest());
    VerifyOrQuit(!serverConn.DidProcessResponse());
    VerifyOrQuit(clientConn.GetLastRxResponseCode() == Dns::Header::kDsoTypeNotImplemented);
    Log("Received a response with DSO Type Unknown error code");

    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send unknown TLV unidirectional");

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    message = clientConn.NewMessage();
    VerifyOrQuit(message != nullptr);
    tlv.Init(kUnknownTlvType, 0);
    SuccessOrQuit(message->Append(tlv));
    SuccessOrQuit(clientConn.SendUnidirectionalMessage(*message));
    VerifyOrQuit(serverConn.DidProcessUnidirectional());
    Log("Unknown TLV unidirectional is correctly ignored");
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send malformed/invalid request");

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    message = clientConn.NewMessage();
    VerifyOrQuit(message != nullptr);
    tlv.Init(Dso::Tlv::kEncryptionPaddingType, 0);
    SuccessOrQuit(message->Append(tlv));

    SuccessOrQuit(serverConn.SendRequestMessage(*message, messageId));
    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonPeerMisbehavior);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonPeerAborted);
    VerifyOrQuit(clientConn.DidGetDisconnectSignal());
    VerifyOrQuit(serverConn.DidGetDisconnectSignal());
    Log("Client aborted on invalid request message");

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Response timeout during session establishment");

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    SuccessOrQuit(serverConn.SetTimeouts(Dso::kResponseTimeout, Dso::kInfiniteTimeout));
    clientConn.Connect();
    VerifyOrQuit(clientConn.GetState() == Connection::kStateConnectedButSessionless);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateConnectedButSessionless);

    sTestDsoForwardMessageToPeer = false;
    clientConn.SendTestRequestMessage();
    sTestDsoForwardMessageToPeer = true;

    VerifyOrQuit(clientConn.GetState() == Connection::kStateEstablishingSession);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateConnectedButSessionless);

    sTestDsoSignalDisconnectToPeer = false;
    AdvanceTime(Dso::kResponseTimeout);
    sTestDsoSignalDisconnectToPeer = true;
    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonResponseTimeout);
    VerifyOrQuit(clientConn.DidGetDisconnectSignal());
    VerifyOrQuit(serverConn.GetState() == Connection::kStateConnectedButSessionless);
    Log("Client disconnected after response timeout");

    AdvanceTime(Dso::kResponseTimeout);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonInactivityTimeout);
    VerifyOrQuit(serverConn.DidGetDisconnectSignal());
    Log("Server disconnected after twice the inactivity timeout");

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Response timeout after session establishment");

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    SuccessOrQuit(serverConn.SetTimeouts(Dso::kInfiniteTimeout, Dso::kInfiniteTimeout));
    clientConn.Connect();
    SuccessOrQuit(clientConn.SendKeepAliveMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    sTestDsoForwardMessageToPeer = false;
    serverConn.SendTestRequestMessage();
    sTestDsoForwardMessageToPeer = true;

    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    AdvanceTime(Dso::kResponseTimeout - 1);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    AdvanceTime(1);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonResponseTimeout);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonPeerAborted);
    VerifyOrQuit(serverConn.DidGetDisconnectSignal());
    VerifyOrQuit(clientConn.DidGetDisconnectSignal());

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    SuccessOrQuit(serverConn.SetTimeouts(Dso::kInfiniteTimeout, Dso::kInfiniteTimeout));
    clientConn.Connect();
    SuccessOrQuit(clientConn.SendKeepAliveMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    sTestDsoForwardMessageToPeer = false;
    serverConn.SendTestRequestMessage(0, kLongResponseTimeout);
    sTestDsoForwardMessageToPeer = true;

    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    AdvanceTime(kLongResponseTimeout - 1);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    AdvanceTime(1);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonResponseTimeout);
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonPeerAborted);
    VerifyOrQuit(serverConn.DidGetDisconnectSignal());
    VerifyOrQuit(clientConn.DidGetDisconnectSignal());

    Log("-------------------------------------------------------------------------------------------");
    Log("Retry Delay message");

    clientConn.ClearTestFlags();
    serverConn.ClearTestFlags();

    SuccessOrQuit(serverConn.SetTimeouts(Dso::kInfiniteTimeout, Dso::kInfiniteTimeout));
    clientConn.Connect();
    SuccessOrQuit(clientConn.SendKeepAliveMessage());
    VerifyOrQuit(clientConn.GetState() == Connection::kStateSessionEstablished);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateSessionEstablished);

    SuccessOrQuit(serverConn.SendRetryDelayMessage(kRetryDelayInterval, Dns::Header::kResponseServerFailure));

    VerifyOrQuit(clientConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(serverConn.GetState() == Connection::kStateDisconnected);
    VerifyOrQuit(clientConn.DidGetDisconnectSignal());
    VerifyOrQuit(serverConn.DidGetDisconnectSignal());
    VerifyOrQuit(clientConn.GetDisconnectReason() == Connection::kReasonServerRetryDelayRequest);
    VerifyOrQuit(serverConn.GetDisconnectReason() == Connection::kReasonPeerClosed);

    VerifyOrQuit(clientConn.GetRetryDelay() == kRetryDelayInterval);
    VerifyOrQuit(clientConn.GetRetryDelayErrorCode() == Dns::Header::kResponseServerFailure);

    Log("End of test");

    testFreeInstance(&instance);
}

} // namespace Dns
} // namespace ot

#endif // OPENTHREAD_CONFIG_DNS_DSO_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_DNS_DSO_ENABLE
    ot::Dns::TestDso();
    printf("All tests passed\n");
#else
    printf("DSO feature is not enabled\n");
#endif

    return 0;
}
