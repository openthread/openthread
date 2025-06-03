/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

typedef MeshCoP::Dtls Dtls;

static constexpr uint16_t kMaxNodes    = 3;
static constexpr uint16_t kUdpPort     = 1234;
static constexpr uint16_t kMessageSize = 100;
static constexpr uint16_t kMaxAttempts = 3;

static const uint8_t kPsk[] = {0x10, 0x20, 0x03, 0x15, 0x10, 0x00, 0x60, 0x16};

static Dtls::Session::ConnectEvent  sDtlsEvent[kMaxNodes];
static Array<uint8_t, kMessageSize> sDtlsLastReceive[kMaxNodes];
static bool                         sDtlsAutoClosed[kMaxNodes];
static uint32_t                     sHeapSessionsAllocated = 0;

const char *ConnectEventToString(Dtls::Session::ConnectEvent aEvent)
{
    const char *str = "";

    switch (aEvent)
    {
    case Dtls::Session::kConnected:
        str = "kConnected";
        break;
    case Dtls::Session::kDisconnectedPeerClosed:
        str = "kDisconnectedPeerClosed";
        break;
    case Dtls::Session::kDisconnectedLocalClosed:
        str = "kDisconnectedLocalClosed";
        break;
    case Dtls::Session::kDisconnectedMaxAttempts:
        str = "kDisconnectedMaxAttempts";
        break;
    case Dtls::Session::kDisconnectedError:
        str = "kDisconnectedError";
        break;
    case Dtls::Session::kDisconnectedTimeout:
        str = "kDisconnectedTimeout";
        break;
    }

    return str;
}

void HandleReceive(void *aContext, uint8_t *aBuf, uint16_t aLength)
{
    Node *node = static_cast<Node *>(aContext);

    VerifyOrQuit(node != nullptr);
    VerifyOrQuit(node->GetId() < kMaxNodes);

    Log("   node%u: HandleReceive(aLength:%u)", node->GetId(), aLength);

    sDtlsLastReceive[node->GetId()].Clear();

    for (; aLength > 0; aLength--, aBuf++)
    {
        SuccessOrQuit(sDtlsLastReceive[node->GetId()].PushBack(*aBuf));
    }
}

void HandleConnectEvent(Dtls::Session::ConnectEvent aEvent, void *aContext)
{
    Node *node = static_cast<Node *>(aContext);

    VerifyOrQuit(node != nullptr);
    VerifyOrQuit(node->GetId() < kMaxNodes);
    sDtlsEvent[node->GetId()] = aEvent;

    Log("   node%u: HandleConnectEvent(%s)", node->GetId(), ConnectEventToString(aEvent));
}

void HandleAutoClose(void *aContext)
{
    Node *node = static_cast<Node *>(aContext);

    VerifyOrQuit(node != nullptr);
    VerifyOrQuit(node->GetId() < kMaxNodes);
    sDtlsAutoClosed[node->GetId()] = true;

    Log("   node%u: HandleAutoClose()", node->GetId());
}

OwnedPtr<Message> PrepareMessage(Node &aNode)
{
    Message *message = aNode.Get<MessagePool>().Allocate(Message::kTypeOther);
    uint16_t length;

    VerifyOrQuit(message != nullptr);

    length = Random::NonCrypto::GetUint16InRange(1, kMessageSize);

    for (uint16_t i = 0; i < length; i++)
    {
        SuccessOrQuit(message->Append(Random::NonCrypto::GetUint8()));
    }

    return OwnedPtr<Message>(message);
}

class DtlsTransportAndSingleSession : public InstanceLocator, public Dtls::Transport, public Dtls::Session
{
    // A DTLS transport and single session
public:
    explicit DtlsTransportAndSingleSession(Node &aNode)
        : InstanceLocator(aNode.GetInstance())
        , Dtls::Transport(aNode.GetInstance(), kWithLinkSecurity)
        , Dtls::Session(static_cast<Dtls::Transport &>(*this))
        , mNode(aNode)
    {
        SetAcceptCallback(HandleAccept, this);

        VerifyOrQuit(!IsSessionInUse());
    }

private:
    static MeshCoP::SecureSession *HandleAccept(void *aContext, const Ip6::MessageInfo &aMessageInfo)
    {
        return static_cast<DtlsTransportAndSingleSession *>(aContext)->HandleAccept();
    }

    Dtls::Session *HandleAccept(void)
    {
        Dtls::Session *session = IsSessionInUse() ? nullptr : static_cast<Dtls::Session *>(this);

        Log("   node%u: HandleAccept(), %s", mNode.GetId(), (session != nullptr) ? "accepted" : "rejected");
        return session;
    }

    Node &mNode;
};

class DtlsTransportAndHeapSession : public InstanceLocator, public Dtls::Transport
{
    // A DTLS session with heap allocated sessions.

public:
    explicit DtlsTransportAndHeapSession(Node &aNode)
        : InstanceLocator(aNode.GetInstance())
        , Dtls::Transport(aNode.GetInstance(), kWithLinkSecurity)
        , mNode(aNode)
    {
        SetAcceptCallback(HandleAccept, this);
        SetRemoveSessionCallback(HandleRemoveSession, this);
    }

private:
    class HeapDtlsSession : public Dtls::Session, public Heap::Allocatable<HeapDtlsSession>
    {
        friend Heap::Allocatable<HeapDtlsSession>;

    private:
        HeapDtlsSession(Dtls::Transport &aTransport)
            : Dtls::Session(aTransport)
        {
            sHeapSessionsAllocated++;
        }
    };

    static MeshCoP::SecureSession *HandleAccept(void *aContext, const Ip6::MessageInfo &aMessageInfo)
    {
        DtlsTransportAndHeapSession *transport;
        HeapDtlsSession             *session;

        VerifyOrQuit(aContext != nullptr);
        transport = static_cast<DtlsTransportAndHeapSession *>(aContext);

        Log("   node%u: HandleAccept()", transport->mNode.GetId());

        session = HeapDtlsSession::Allocate(*transport);
        VerifyOrQuit(session != nullptr);

        session->SetReceiveCallback(&ot::Nexus::HandleReceive, &transport->mNode);
        session->SetConnectCallback(&ot::Nexus::HandleConnectEvent, &transport->mNode);

        return session;
    }

    static void HandleRemoveSession(void *aContext, MeshCoP::SecureSession &aSesssion)
    {
        DtlsTransportAndHeapSession *transport;

        VerifyOrQuit(aContext != nullptr);
        transport = static_cast<DtlsTransportAndHeapSession *>(aContext);

        Log("   node%u: HandleRemoveSession()", transport->mNode.GetId());

        VerifyOrQuit(sHeapSessionsAllocated > 0);

        static_cast<HeapDtlsSession &>(aSesssion).Free();
        sHeapSessionsAllocated--;
    }

private:
    Node &mNode;
};

void TestDtlsSingleSession(void)
{
    Core  nexus;
    Node &node0 = nexus.CreateNode();
    Node &node1 = nexus.CreateNode();
    Node &node2 = nexus.CreateNode();

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestDtlsSingleSession");

    nexus.AdvanceTime(0);

    // Form the topology: node0 leader, with node1 & node2 as its FTD children

    node0.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    VerifyOrQuit(node0.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(node1.Get<Mle::Mle>().SetRouterEligible(false));
    node1.Join(node0);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(node1.Get<Mle::Mle>().IsChild());

    SuccessOrQuit(node2.Get<Mle::Mle>().SetRouterEligible(false));
    node2.Join(node0);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(node2.Get<Mle::Mle>().IsChild());

    {
        DtlsTransportAndSingleSession dtls0(node0);
        DtlsTransportAndSingleSession dtls1(node1);
        DtlsTransportAndSingleSession dtls2(node2);
        Ip6::SockAddr                 sockAddr;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Start DTLS (server) on node0 bound to port %u", kUdpPort);

        SuccessOrQuit(dtls0.SetPsk(kPsk, sizeof(kPsk)));
        dtls0.SetReceiveCallback(HandleReceive, &node0);
        dtls0.SetConnectCallback(HandleConnectEvent, &node0);
        SuccessOrQuit(dtls0.Open());
        SuccessOrQuit(dtls0.Bind(kUdpPort));

        nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

        VerifyOrQuit(dtls0.GetUdpPort() == kUdpPort);
        VerifyOrQuit(!dtls0.IsConnectionActive());

        sockAddr.SetAddress(node0.Get<Mle::Mle>().GetMeshLocalRloc());
        sockAddr.SetPort(kUdpPort);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Try to establish a DTLS connection from node 1 using a wrong PSK multiple times");

        SuccessOrQuit(dtls1.SetPsk(kPsk, sizeof(kPsk) - 1));
        dtls1.SetReceiveCallback(HandleReceive, &node1);
        dtls1.SetConnectCallback(HandleConnectEvent, &node1);
        SuccessOrQuit(dtls1.Open());

        for (uint16_t iter = 0; iter <= kMaxAttempts + 1; iter++)
        {
            memset(sDtlsEvent, Dtls::Session::kConnected, sizeof(sDtlsEvent));

            SuccessOrQuit(dtls1.Connect(sockAddr));
            nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

            VerifyOrQuit(!dtls0.IsConnected());
            VerifyOrQuit(!dtls1.IsConnected());

            VerifyOrQuit(sDtlsEvent[node0.GetId()] == Dtls::Session::kDisconnectedError);
            VerifyOrQuit(sDtlsEvent[node1.GetId()] == Dtls::Session::kDisconnectedError);
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Establish a DTLS connection from node1 with node0 using the correct PSK");

        dtls1.Close();

        SuccessOrQuit(dtls1.SetPsk(kPsk, sizeof(kPsk)));
        dtls1.SetReceiveCallback(HandleReceive, &node1);
        dtls1.SetConnectCallback(HandleConnectEvent, &node1);
        SuccessOrQuit(dtls1.Open());
        SuccessOrQuit(dtls1.Connect(sockAddr));

        nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

        VerifyOrQuit(dtls0.IsConnected());
        VerifyOrQuit(dtls1.IsConnected());

        VerifyOrQuit(sDtlsEvent[node0.GetId()] == Dtls::Session::kConnected);
        VerifyOrQuit(sDtlsEvent[node1.GetId()] == Dtls::Session::kConnected);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Send message (random data and length) over DTLS session from node0 to node1");

        for (uint16_t iter = 0; iter < 20; iter++)
        {
            OwnedPtr<Message> msg(PrepareMessage(node0));

            SuccessOrQuit(dtls0.Send(*msg->Clone()));
            nexus.AdvanceTime(100);

            VerifyOrQuit(sDtlsLastReceive[node1.GetId()].GetLength() == msg->GetLength());
            VerifyOrQuit(msg->CompareBytes(0, sDtlsLastReceive[node1.GetId()].GetArrayBuffer(), msg->GetLength()));
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Now send from node1 to node0");

        for (uint16_t iter = 0; iter < 20; iter++)
        {
            OwnedPtr<Message> msg(PrepareMessage(node1));

            SuccessOrQuit(dtls1.Send(*msg->Clone()));
            nexus.AdvanceTime(100);

            VerifyOrQuit(sDtlsLastReceive[node0.GetId()].GetLength() == msg->GetLength());
            VerifyOrQuit(msg->CompareBytes(0, sDtlsLastReceive[node0.GetId()].GetArrayBuffer(), msg->GetLength()));
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Disconnect from node1 - validate the disconnect events (local/peer)");

        dtls1.Disconnect();

        nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

        VerifyOrQuit(!dtls0.IsConnected());
        VerifyOrQuit(!dtls1.IsConnected());

        VerifyOrQuit(sDtlsEvent[node0.GetId()] == Dtls::Session::kDisconnectedPeerClosed);
        VerifyOrQuit(sDtlsEvent[node1.GetId()] == Dtls::Session::kDisconnectedLocalClosed);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Establish a DTLS connection again");

        SuccessOrQuit(dtls1.Connect(sockAddr));

        nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

        VerifyOrQuit(dtls0.IsConnected());
        VerifyOrQuit(dtls1.IsConnected());

        VerifyOrQuit(sDtlsEvent[node0.GetId()] == Dtls::Session::kConnected);
        VerifyOrQuit(sDtlsEvent[node1.GetId()] == Dtls::Session::kConnected);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Try to connect from node2 - validate that it fails to connect since already connected");

        SuccessOrQuit(dtls2.SetPsk(kPsk, sizeof(kPsk)));
        dtls2.SetReceiveCallback(HandleReceive, &node2);
        dtls2.SetReceiveCallback(HandleReceive, &node2);
        SuccessOrQuit(dtls2.Open());
        SuccessOrQuit(dtls2.Connect(sockAddr));

        nexus.AdvanceTime(20 * Time::kOneSecondInMsec);

        VerifyOrQuit(dtls0.IsConnected());
        VerifyOrQuit(dtls1.IsConnected());
        VerifyOrQuit(!dtls2.IsConnected());

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Disconnect from node0 - validate the disconnect events");

        dtls0.Disconnect();

        nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

        VerifyOrQuit(!dtls0.IsConnected());
        VerifyOrQuit(!dtls1.IsConnected());
        VerifyOrQuit(!dtls2.IsConnected());

        VerifyOrQuit(sDtlsEvent[node0.GetId()] == Dtls::Session::kDisconnectedLocalClosed);
        VerifyOrQuit(sDtlsEvent[node1.GetId()] == Dtls::Session::kDisconnectedPeerClosed);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        dtls0.Close();
        dtls1.Close();
        dtls2.Close();

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        Log("Start DTLS (server) on node0 bound to port %u with auto-close max attempt %u", kUdpPort, kMaxAttempts);

        memset(sDtlsAutoClosed, false, sizeof(sDtlsAutoClosed));

        SuccessOrQuit(dtls0.SetMaxConnectionAttempts(kMaxAttempts, HandleAutoClose, &node0));
        SuccessOrQuit(dtls0.SetPsk(kPsk, sizeof(kPsk)));
        dtls0.SetReceiveCallback(HandleReceive, &node0);
        dtls0.SetConnectCallback(HandleConnectEvent, &node0);
        SuccessOrQuit(dtls0.Open());
        SuccessOrQuit(dtls0.Bind(kUdpPort));

        nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

        VerifyOrQuit(dtls0.GetUdpPort() == kUdpPort);
        VerifyOrQuit(!dtls0.IsConnectionActive());

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Using wrong PSK try to establish DTLS connection with node0 %u times", kMaxAttempts - 1);

        SuccessOrQuit(dtls1.SetPsk(kPsk, sizeof(kPsk) - 1));
        dtls1.SetReceiveCallback(HandleReceive, &node1);
        dtls1.SetConnectCallback(HandleConnectEvent, &node1);
        SuccessOrQuit(dtls1.Open());

        for (uint16_t iter = 0; iter < kMaxAttempts - 1; iter++)
        {
            memset(sDtlsEvent, Dtls::Session::kConnected, sizeof(sDtlsEvent));

            SuccessOrQuit(dtls1.Connect(sockAddr));
            nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

            VerifyOrQuit(!dtls0.IsConnected());
            VerifyOrQuit(!dtls1.IsConnected());

            VerifyOrQuit(sDtlsEvent[node0.GetId()] == Dtls::Session::kDisconnectedError);
            VerifyOrQuit(sDtlsEvent[node1.GetId()] == Dtls::Session::kDisconnectedError);
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Using wrong PSK try one last time, validate the auto-close behavior");

        memset(sDtlsEvent, Dtls::Session::kConnected, sizeof(sDtlsEvent));

        SuccessOrQuit(dtls1.Connect(sockAddr));
        nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

        VerifyOrQuit(sDtlsEvent[node0.GetId()] == Dtls::Session::kDisconnectedError);
        VerifyOrQuit(sDtlsEvent[node1.GetId()] == Dtls::Session::kDisconnectedError);

        VerifyOrQuit(sDtlsAutoClosed[node0.GetId()]);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        dtls0.Close();
        dtls1.Close();
        dtls2.Close();
    }
}

void TestDtlsMultiSession(void)
{
    Core  nexus;
    Node &node0 = nexus.CreateNode();
    Node &node1 = nexus.CreateNode();
    Node &node2 = nexus.CreateNode();

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestDtlsMultiSession");

    nexus.AdvanceTime(0);

    // Form the topology: node0 leader, with node1 & node2 as its FTD children

    node0.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    VerifyOrQuit(node0.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(node1.Get<Mle::Mle>().SetRouterEligible(false));
    node1.Join(node0);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(node1.Get<Mle::Mle>().IsChild());

    SuccessOrQuit(node2.Get<Mle::Mle>().SetRouterEligible(false));
    node2.Join(node0);
    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);
    VerifyOrQuit(node2.Get<Mle::Mle>().IsChild());

    {
        DtlsTransportAndHeapSession   dtls0(node0);
        DtlsTransportAndSingleSession dtls1(node1);
        DtlsTransportAndSingleSession dtls2(node2);
        Ip6::SockAddr                 sockAddr;
        uint16_t                      numSessions;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Start DTLS (server) on node0 bound to port %u", kUdpPort);

        SuccessOrQuit(dtls0.SetPsk(kPsk, sizeof(kPsk)));
        SuccessOrQuit(dtls0.Open());
        SuccessOrQuit(dtls0.Bind(kUdpPort));

        nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

        VerifyOrQuit(dtls0.GetUdpPort() == kUdpPort);

        sockAddr.SetAddress(node0.Get<Mle::Mle>().GetMeshLocalRloc());
        sockAddr.SetPort(kUdpPort);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Establish a DTLS connection with node 0 from node1");

        memset(sDtlsEvent, Dtls::Session::kDisconnectedError, sizeof(sDtlsEvent));

        SuccessOrQuit(dtls1.SetPsk(kPsk, sizeof(kPsk)));
        dtls1.SetReceiveCallback(HandleReceive, &node1);
        dtls1.SetConnectCallback(HandleConnectEvent, &node1);
        SuccessOrQuit(dtls1.Open());
        SuccessOrQuit(dtls1.Connect(sockAddr));

        nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

        VerifyOrQuit(dtls1.IsConnected());

        VerifyOrQuit(sDtlsEvent[node0.GetId()] == Dtls::Session::kConnected);
        VerifyOrQuit(sDtlsEvent[node1.GetId()] == Dtls::Session::kConnected);

        numSessions = 0;

        for (MeshCoP::SecureSession &session : dtls0.GetSessions())
        {
            VerifyOrQuit(session.IsConnected());
            numSessions++;
        }

        VerifyOrQuit(numSessions == 1);
        VerifyOrQuit(sHeapSessionsAllocated == 1);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Establish a second DTLS connection with node0 from node2");

        memset(sDtlsEvent, Dtls::Session::kDisconnectedError, sizeof(sDtlsEvent));

        SuccessOrQuit(dtls2.SetPsk(kPsk, sizeof(kPsk)));
        dtls2.SetReceiveCallback(HandleReceive, &node2);
        dtls2.SetConnectCallback(HandleConnectEvent, &node2);
        SuccessOrQuit(dtls2.Open());
        SuccessOrQuit(dtls2.Connect(sockAddr));

        nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

        VerifyOrQuit(dtls2.IsConnected());

        VerifyOrQuit(sDtlsEvent[node0.GetId()] == Dtls::Session::kConnected);
        VerifyOrQuit(sDtlsEvent[node2.GetId()] == Dtls::Session::kConnected);

        numSessions = 0;

        for (MeshCoP::SecureSession &session : dtls0.GetSessions())
        {
            VerifyOrQuit(session.IsConnected());
            numSessions++;
        }

        VerifyOrQuit(numSessions == 2);
        VerifyOrQuit(sHeapSessionsAllocated == 2);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Disconnect from node1 - validate the disconnect events");

        dtls1.Disconnect();

        nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

        VerifyOrQuit(!dtls1.IsConnected());

        VerifyOrQuit(sDtlsEvent[node0.GetId()] == Dtls::Session::kDisconnectedPeerClosed);
        VerifyOrQuit(sDtlsEvent[node1.GetId()] == Dtls::Session::kDisconnectedLocalClosed);

        numSessions = 0;

        for (MeshCoP::SecureSession &session : dtls0.GetSessions())
        {
            VerifyOrQuit(session.IsConnected());
            numSessions++;
        }

        VerifyOrQuit(numSessions == 1);
        VerifyOrQuit(sHeapSessionsAllocated == 1);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Disconnect session with node2 from node0 (server) - validate the disconnect events");

        memset(sDtlsEvent, Dtls::Session::kConnected, sizeof(sDtlsEvent));

        dtls0.GetSessions().GetHead()->Disconnect();

        nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

        VerifyOrQuit(!dtls2.IsConnected());

        VerifyOrQuit(sDtlsEvent[node0.GetId()] == Dtls::Session::kDisconnectedLocalClosed);
        VerifyOrQuit(sDtlsEvent[node2.GetId()] == Dtls::Session::kDisconnectedPeerClosed);

        VerifyOrQuit(dtls0.GetSessions().IsEmpty());
        VerifyOrQuit(sHeapSessionsAllocated == 0);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Establish two DTLS connections from node1 and node2 at the same time");

        memset(sDtlsEvent, Dtls::Session::kDisconnectedError, sizeof(sDtlsEvent));

        SuccessOrQuit(dtls1.Connect(sockAddr));
        SuccessOrQuit(dtls2.Connect(sockAddr));

        nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

        VerifyOrQuit(dtls1.IsConnected());
        VerifyOrQuit(dtls2.IsConnected());

        VerifyOrQuit(sDtlsEvent[node0.GetId()] == Dtls::Session::kConnected);
        VerifyOrQuit(sDtlsEvent[node1.GetId()] == Dtls::Session::kConnected);
        VerifyOrQuit(sDtlsEvent[node2.GetId()] == Dtls::Session::kConnected);

        numSessions = 0;

        for (MeshCoP::SecureSession &session : dtls0.GetSessions())
        {
            VerifyOrQuit(session.IsConnected());
            numSessions++;
        }

        VerifyOrQuit(numSessions == 2);
        VerifyOrQuit(sHeapSessionsAllocated == 2);
    }
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestDtlsSingleSession();
    ot::Nexus::TestDtlsMultiSession();
    printf("All tests passed\n");
    return 0;
}
