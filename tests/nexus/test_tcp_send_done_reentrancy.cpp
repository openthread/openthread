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
#include <sys/mman.h>
#include <unistd.h>

#include <new>

#include "net/tcp6.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/mle.hpp"

namespace ot {
namespace Nexus {

static constexpr uint16_t kPort               = 12345;
static constexpr uint32_t kFormNetworkTime    = 13 * 1000;
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;
static constexpr uint32_t kTcpTime            = 5 * 1000;

class GuardedBuffer
{
public:
    GuardedBuffer(void)
    {
        long pageSize = sysconf(_SC_PAGESIZE);

        VerifyOrQuit(pageSize > 0);

        mPageSize = static_cast<size_t>(pageSize);
        mPage     = mmap(nullptr, mPageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        VerifyOrQuit(mPage != MAP_FAILED);

        mBuffer = new (mPage) otLinkedBuffer{};
    }

    ~GuardedBuffer(void) { VerifyOrQuit(munmap(mPage, mPageSize) == 0); }

    otLinkedBuffer &Get(void) { return *mBuffer; }

    void Protect(void) { VerifyOrQuit(mprotect(mPage, mPageSize, PROT_NONE) == 0); }

private:
    void           *mPage;
    size_t          mPageSize;
    otLinkedBuffer *mBuffer;
};

struct ClientContext
{
    GuardedBuffer    *mSecondBuffer;
    const otSockAddr *mReconnectAddress;
    uint16_t          mSendDoneCalls;
    bool              mEstablished;
};

struct ServerContext
{
    Ip6::Tcp::Endpoint *mAcceptEndpoint;
    bool                mAccepted;
};

static void HandleClientEstablished(otTcpEndpoint *aEndpoint)
{
    static_cast<ClientContext *>(otTcpEndpointGetContext(aEndpoint))->mEstablished = true;
}

static void HandleClientSendDone(otTcpEndpoint *aEndpoint, otLinkedBuffer *aBuffer)
{
    ClientContext &context = *static_cast<ClientContext *>(otTcpEndpointGetContext(aEndpoint));

    context.mSendDoneCalls++;

    if (context.mSendDoneCalls == 1)
    {
        VerifyOrQuit(context.mSecondBuffer != nullptr);
        VerifyOrQuit(aBuffer != &context.mSecondBuffer->Get());
        SuccessOrQuit(otTcpAbort(aEndpoint));

        // Abort returns all referenced send buffers to the application. Reclaim
        // the second buffer immediately to verify that TCP does not access it
        // again after the callback closes the endpoint.
        context.mSecondBuffer->Protect();
        context.mSecondBuffer = nullptr;

        // Abort permits the endpoint to be reused immediately. Reconnect to
        // ensure that checking only whether the endpoint is closed cannot make
        // TCP resume walking the old send-buffer list.
        VerifyOrQuit(context.mReconnectAddress != nullptr);
        SuccessOrQuit(otTcpConnect(aEndpoint, context.mReconnectAddress, OT_TCP_CONNECT_NO_FAST_OPEN));
    }
}

static otTcpIncomingConnectionAction HandleServerAcceptReady(otTcpListener *aListener,
                                                             const otSockAddr *,
                                                             otTcpEndpoint **aAcceptInto)
{
    ServerContext &context = *static_cast<ServerContext *>(otTcpListenerGetContext(aListener));

    *aAcceptInto = context.mAcceptEndpoint;

    return OT_TCP_INCOMING_CONNECTION_ACTION_ACCEPT;
}

static void HandleServerAcceptDone(otTcpListener *aListener, otTcpEndpoint *, const otSockAddr *)
{
    static_cast<ServerContext *>(otTcpListenerGetContext(aListener))->mAccepted = true;
}

void TestTcpSendDoneReentrancy(void)
{
    Core  nexus;
    Node &serverNode = nexus.CreateNode();
    Node &clientNode = nexus.CreateNode();

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    serverNode.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(serverNode.Get<Mle::Mle>().IsLeader());

    clientNode.Join(serverNode);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(clientNode.Get<Mle::Mle>().IsChild() || clientNode.Get<Mle::Mle>().IsRouter());

    Ip6::Tcp::Endpoint serverEndpoint;
    Ip6::Tcp::Listener serverListener;
    Ip6::Tcp::Endpoint clientEndpoint;
    uint8_t            serverReceiveBuffer[128];
    uint8_t            clientReceiveBuffer[128];
    ServerContext      serverContext{&serverEndpoint, false};
    ClientContext      clientContext{nullptr, nullptr, 0, false};

    otTcpEndpointInitializeArgs serverEndpointArgs{};
    serverEndpointArgs.mReceiveBuffer     = serverReceiveBuffer;
    serverEndpointArgs.mReceiveBufferSize = sizeof(serverReceiveBuffer);

    otTcpListenerInitializeArgs listenerArgs{};
    listenerArgs.mContext             = &serverContext;
    listenerArgs.mAcceptReadyCallback = HandleServerAcceptReady;
    listenerArgs.mAcceptDoneCallback  = HandleServerAcceptDone;

    otTcpEndpointInitializeArgs clientEndpointArgs{};
    clientEndpointArgs.mContext             = &clientContext;
    clientEndpointArgs.mEstablishedCallback = HandleClientEstablished;
    clientEndpointArgs.mSendDoneCallback    = HandleClientSendDone;
    clientEndpointArgs.mReceiveBuffer       = clientReceiveBuffer;
    clientEndpointArgs.mReceiveBufferSize   = sizeof(clientReceiveBuffer);

    SuccessOrQuit(serverEndpoint.Initialize(serverNode, serverEndpointArgs));
    SuccessOrQuit(serverListener.Initialize(serverNode, listenerArgs));
    SuccessOrQuit(clientEndpoint.Initialize(clientNode, clientEndpointArgs));

    Ip6::SockAddr listenAddress;
    listenAddress.SetPort(kPort);
    SuccessOrQuit(serverListener.Listen(listenAddress));

    Ip6::SockAddr serverAddress;
    serverAddress.SetAddress(serverNode.Get<Mle::Mle>().GetMeshLocalEid());
    serverAddress.SetPort(kPort);

    Ip6::SockAddr reconnectAddress = serverAddress;
    reconnectAddress.SetPort(kPort + 1);
    clientContext.mReconnectAddress = &reconnectAddress;

    SuccessOrQuit(clientEndpoint.Connect(serverAddress, OT_TCP_CONNECT_NO_FAST_OPEN));

    nexus.AdvanceTime(kTcpTime);
    VerifyOrQuit(clientContext.mEstablished);
    VerifyOrQuit(serverContext.mAccepted);

    static const uint8_t kFirstData[]  = {1, 2, 3, 4, 5, 6, 7};
    static const uint8_t kSecondData[] = {8, 9, 10, 11, 12, 13, 14, 15, 16};
    otLinkedBuffer       first{};
    GuardedBuffer        second;

    first.mData                 = kFirstData;
    first.mLength               = sizeof(kFirstData);
    second.Get().mData          = kSecondData;
    second.Get().mLength        = sizeof(kSecondData);
    clientContext.mSecondBuffer = &second;

    // `MORE_TO_COME` allows one cumulative ACK to retire both linked buffers
    // during a single call to `Tcp::ProcessSignals()`.
    SuccessOrQuit(clientEndpoint.SendByReference(first, OT_TCP_SEND_MORE_TO_COME));
    SuccessOrQuit(clientEndpoint.SendByReference(second.Get(), 0));

    nexus.AdvanceTime(kTcpTime);

    VerifyOrQuit(clientContext.mSendDoneCalls == 1);
    VerifyOrQuit(clientContext.mSecondBuffer == nullptr);

    SuccessOrQuit(clientEndpoint.Deinitialize());
    SuccessOrQuit(serverEndpoint.Deinitialize());
    SuccessOrQuit(serverListener.StopListening());
    SuccessOrQuit(serverListener.Deinitialize());

    nexus.SaveTestInfo("test_tcp_send_done_reentrancy.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestTcpSendDoneReentrancy();
    printf("All tests passed\n");
    return 0;
}
