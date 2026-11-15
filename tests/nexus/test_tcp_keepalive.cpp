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

#include <stdio.h>
#include <string.h>

#include "net/tcp6.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#ifndef TCPTV_KEEP_INIT
#define TCPTV_KEEP_INIT (75 * hz)
#endif
#ifndef TCPTV_KEEP_IDLE
#define TCPTV_KEEP_IDLE (120 * 60 * hz)
#endif
#ifndef TCPTV_KEEPINTVL
#define TCPTV_KEEPINTVL (75 * hz)
#endif
#ifndef TCPTV_KEEPCNT
#define TCPTV_KEEPCNT 8
#endif

namespace ot {
namespace Nexus {

static constexpr uint16_t kServerPort = 12345;

static constexpr uint32_t GetKeepInitMs(void)
{
    constexpr uint32_t hz = 1000;

    return TCPTV_KEEP_INIT;
}

static constexpr uint32_t GetKeepIdleMs(void)
{
    constexpr uint32_t hz = 1000;

    return TCPTV_KEEP_IDLE;
}

static constexpr uint32_t GetKeepIntvlMs(void)
{
    constexpr uint32_t hz = 1000;

    return TCPTV_KEEPINTVL;
}

static constexpr uint32_t GetKeepCnt(void) { return TCPTV_KEEPCNT; }

struct EndpointContext
{
    void Initialize(Node &aNode)
    {
        otTcpEndpointInitializeArgs args;

        ClearAllBytes(mEndpoint);
        ClearAllBytes(mReceiveBuffer);
        ClearAllBytes(mReceivedData);
        mReceivedLength     = 0;
        mEstablished        = false;
        mSendDone           = false;
        mDisconnected       = false;
        mDisconnectedReason = OT_TCP_DISCONNECTED_REASON_NORMAL;

        ClearAllBytes(args);
        args.mContext                  = this;
        args.mEstablishedCallback      = HandleEstablished;
        args.mSendDoneCallback         = HandleSendDone;
        args.mReceiveAvailableCallback = HandleReceiveAvailable;
        args.mDisconnectedCallback     = HandleDisconnected;
        args.mReceiveBuffer            = mReceiveBuffer;
        args.mReceiveBufferSize        = sizeof(mReceiveBuffer);

        SuccessOrQuit(mEndpoint.Initialize(aNode, args));
    }

    static void HandleEstablished(otTcpEndpoint *aEndpoint)
    {
        EndpointContext *context = static_cast<EndpointContext *>(AsCoreType(aEndpoint).GetContext());

        VerifyOrQuit(context != nullptr);
        context->mEstablished = true;
    }

    static void HandleSendDone(otTcpEndpoint *aEndpoint, otLinkedBuffer *aData)
    {
        EndpointContext *context = static_cast<EndpointContext *>(AsCoreType(aEndpoint).GetContext());

        OT_UNUSED_VARIABLE(aData);

        VerifyOrQuit(context != nullptr);
        context->mSendDone = true;
    }

    static void HandleReceiveAvailable(otTcpEndpoint *aEndpoint,
                                       size_t         aBytesAvailable,
                                       bool           aEndOfStream,
                                       size_t         aBytesRemaining)
    {
        EndpointContext      *context = static_cast<EndpointContext *>(AsCoreType(aEndpoint).GetContext());
        const otLinkedBuffer *buffer  = nullptr;
        size_t                total   = 0;

        OT_UNUSED_VARIABLE(aEndOfStream);
        OT_UNUSED_VARIABLE(aBytesRemaining);

        VerifyOrQuit(context != nullptr);

        if (aBytesAvailable > 0)
        {
            SuccessOrQuit(context->mEndpoint.ReceiveByReference(buffer));

            for (; buffer != nullptr; buffer = buffer->mNext)
            {
                VerifyOrQuit(context->mReceivedLength + buffer->mLength <= sizeof(context->mReceivedData));
                memcpy(&context->mReceivedData[context->mReceivedLength], buffer->mData, buffer->mLength);
                context->mReceivedLength += buffer->mLength;
                total += buffer->mLength;
            }

            VerifyOrQuit(total == aBytesAvailable);
            SuccessOrQuit(context->mEndpoint.CommitReceive(total, 0));
        }
    }

    static void HandleDisconnected(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason)
    {
        EndpointContext *context = static_cast<EndpointContext *>(AsCoreType(aEndpoint).GetContext());

        VerifyOrQuit(context != nullptr);
        context->mDisconnected       = true;
        context->mDisconnectedReason = aReason;
    }

    Ip6::Tcp::Endpoint      mEndpoint;
    uint8_t                 mReceiveBuffer[OT_TCP_RECEIVE_BUFFER_SIZE_FEW_HOPS];
    uint8_t                 mReceivedData[64];
    size_t                  mReceivedLength;
    bool                    mEstablished;
    bool                    mSendDone;
    bool                    mDisconnected;
    otTcpDisconnectedReason mDisconnectedReason;
};

struct ListenerContext
{
    void Initialize(Node &aNode, EndpointContext &aAcceptEndpoint)
    {
        otTcpListenerInitializeArgs args;

        ClearAllBytes(mListener);
        mAcceptEndpoint = &aAcceptEndpoint;
        mAcceptDone     = false;

        ClearAllBytes(args);
        args.mContext             = this;
        args.mAcceptReadyCallback = HandleAcceptReady;
        args.mAcceptDoneCallback  = HandleAcceptDone;

        SuccessOrQuit(mListener.Initialize(aNode, args));
    }

    static otTcpIncomingConnectionAction HandleAcceptReady(otTcpListener    *aListener,
                                                           const otSockAddr *aPeer,
                                                           otTcpEndpoint   **aAcceptInto)
    {
        ListenerContext *context = static_cast<ListenerContext *>(AsCoreType(aListener).GetContext());

        OT_UNUSED_VARIABLE(aPeer);

        VerifyOrQuit(context != nullptr);
        VerifyOrQuit(context->mAcceptEndpoint != nullptr);
        *aAcceptInto = &context->mAcceptEndpoint->mEndpoint;

        return OT_TCP_INCOMING_CONNECTION_ACTION_ACCEPT;
    }

    static void HandleAcceptDone(otTcpListener *aListener, otTcpEndpoint *aEndpoint, const otSockAddr *aPeer)
    {
        ListenerContext *context = static_cast<ListenerContext *>(AsCoreType(aListener).GetContext());

        OT_UNUSED_VARIABLE(aPeer);

        VerifyOrQuit(context != nullptr);
        VerifyOrQuit(aEndpoint == &context->mAcceptEndpoint->mEndpoint);
        context->mAcceptDone = true;
    }

    Ip6::Tcp::Listener mListener;
    EndpointContext   *mAcceptEndpoint;
    bool               mAcceptDone;
};

void TestTcpKeepalive(void)
{
    static constexpr uint32_t kKeepInitMs  = GetKeepInitMs();
    static constexpr uint32_t kKeepIdleMs  = GetKeepIdleMs();
    static constexpr uint32_t kKeepIntvlMs = GetKeepIntvlMs();
    static constexpr uint32_t kKeepCnt     = GetKeepCnt();
    static const char         kPayload[]   = "keepalive-check";

    Core            nexus;
    Node           &serverNode = nexus.CreateNode();
    Node           &clientNode = nexus.CreateNode();
    EndpointContext serverCtx;
    EndpointContext clientCtx;
    EndpointContext initTimeoutCtx;
    ListenerContext listenerCtx;
    Ip6::SockAddr   listenSockAddr;
    Ip6::SockAddr   serverSockAddr;
    Ip6::SockAddr   clientSockAddr;
    otLinkedBuffer  sendBuffer;

    serverNode.SetName("SERVER");
    clientNode.SetName("CLIENT");

    AllowLinkBetween(serverNode, clientNode);

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Form network and establish TCP connection (idle=%lu ms, intvl=%lu ms, cnt=%lu)", ToUlong(kKeepIdleMs),
        ToUlong(kKeepIntvlMs), ToUlong(kKeepCnt));

    serverNode.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(serverNode.Get<Mle::Mle>().IsLeader());

    clientNode.Join(serverNode);
    nexus.AdvanceTime(200 * 1000);
    VerifyOrQuit(clientNode.Get<Mle::Mle>().IsRouter());

    nexus.SendAndVerifyEchoRequest(clientNode, serverNode.Get<Mle::Mle>().GetMeshLocalEid());

    serverCtx.Initialize(serverNode);
    listenerCtx.Initialize(serverNode, serverCtx);

    listenSockAddr.Clear();
    listenSockAddr.SetPort(kServerPort);
    SuccessOrQuit(listenerCtx.mListener.Listen(listenSockAddr));

    clientCtx.Initialize(clientNode);

    serverSockAddr.Clear();
    serverSockAddr.SetAddress(serverNode.Get<Mle::Mle>().GetMeshLocalEid());
    serverSockAddr.SetPort(kServerPort);
    SuccessOrQuit(clientCtx.mEndpoint.Connect(serverSockAddr, OT_TCP_CONNECT_NO_FAST_OPEN));

    nexus.AdvanceTime(1000);

    VerifyOrQuit(listenerCtx.mAcceptDone);
    VerifyOrQuit(serverCtx.mEstablished);
    VerifyOrQuit(clientCtx.mEstablished);
    VerifyOrQuit(!serverCtx.mDisconnected);
    VerifyOrQuit(!clientCtx.mDisconnected);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Verify idle connection stays alive across multiple keepalive intervals");

    nexus.AdvanceTime(kKeepIdleMs + (kKeepIntvlMs / 2));
    VerifyOrQuit(!serverCtx.mDisconnected);
    VerifyOrQuit(!clientCtx.mDisconnected);
    VerifyOrQuit(!serverCtx.mEndpoint.IsClosed());
    VerifyOrQuit(!clientCtx.mEndpoint.IsClosed());

    nexus.AdvanceTime(kKeepIdleMs + kKeepIntvlMs);
    VerifyOrQuit(!serverCtx.mDisconnected);
    VerifyOrQuit(!clientCtx.mDisconnected);
    VerifyOrQuit(!serverCtx.mEndpoint.IsClosed());
    VerifyOrQuit(!clientCtx.mEndpoint.IsClosed());

    ClearAllBytes(sendBuffer);
    sendBuffer.mData   = reinterpret_cast<const uint8_t *>(kPayload);
    sendBuffer.mLength = sizeof(kPayload) - 1;
    SuccessOrQuit(clientCtx.mEndpoint.SendByReference(sendBuffer, 0));

    nexus.AdvanceTime(200);

    VerifyOrQuit(clientCtx.mSendDone);
    VerifyOrQuit(serverCtx.mReceivedLength == sizeof(kPayload) - 1);
    VerifyOrQuit(memcmp(serverCtx.mReceivedData, kPayload, sizeof(kPayload) - 1) == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Block peer radio and verify keepalive drops connection after max probes");

    clientNode.Get<Mac::Mac>().SetRadioFilterEnabled(true);

    // Before TCPTV_KEEP_IDLE expires, connection remains established.
    nexus.AdvanceTime(kKeepIdleMs - 500);
    VerifyOrQuit(!serverCtx.mDisconnected);
    VerifyOrQuit(!serverCtx.mEndpoint.IsClosed());

    // After all TCPTV_KEEPCNT probes are sent, connection still waits one final TCPTV_KEEPINTVL before dropping.
    nexus.AdvanceTime((kKeepCnt * kKeepIntvlMs) - (kKeepIntvlMs / 2));
    VerifyOrQuit(!serverCtx.mDisconnected);
    VerifyOrQuit(!serverCtx.mEndpoint.IsClosed());

    // After the final probe interval elapses without response, connection drops with TIMED_OUT.
    nexus.AdvanceTime(kKeepIntvlMs);
    VerifyOrQuit(serverCtx.mDisconnected);
    VerifyOrQuit(serverCtx.mDisconnectedReason == OT_TCP_DISCONNECTED_REASON_TIMED_OUT);
    VerifyOrQuit(serverCtx.mEndpoint.IsClosed());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Verify initial connect keepalive timeout (init=%lu ms)", ToUlong(kKeepInitMs));

    initTimeoutCtx.Initialize(serverNode);

    clientSockAddr.Clear();
    clientSockAddr.SetAddress(clientNode.Get<Mle::Mle>().GetMeshLocalEid());
    clientSockAddr.SetPort(kServerPort);
    SuccessOrQuit(initTimeoutCtx.mEndpoint.Connect(clientSockAddr, OT_TCP_CONNECT_NO_FAST_OPEN));

    nexus.AdvanceTime(kKeepInitMs - 500);
    VerifyOrQuit(!initTimeoutCtx.mEstablished);
    VerifyOrQuit(!initTimeoutCtx.mDisconnected);
    VerifyOrQuit(!initTimeoutCtx.mEndpoint.IsClosed());

    nexus.AdvanceTime(1000);
    VerifyOrQuit(initTimeoutCtx.mDisconnected);
    VerifyOrQuit(initTimeoutCtx.mDisconnectedReason == OT_TCP_DISCONNECTED_REASON_TIMED_OUT);
    VerifyOrQuit(initTimeoutCtx.mEndpoint.IsClosed());

    SuccessOrQuit(initTimeoutCtx.mEndpoint.Deinitialize());
    SuccessOrQuit(serverCtx.mEndpoint.Deinitialize());
    SuccessOrQuit(clientCtx.mEndpoint.Deinitialize());
    SuccessOrQuit(listenerCtx.mListener.Deinitialize());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestTcpKeepalive();
    printf("All tests passed\n");
    return 0;
}
