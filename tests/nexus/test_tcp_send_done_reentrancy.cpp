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

struct in6_addr;

extern "C" bool tcplp_sys_accepted_connection(struct tcpcb_listen *aTcbListen,
                                              struct tcpcb        *aAccepted,
                                              struct in6_addr     *aAddr,
                                              uint16_t             aPort);

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

class GuardedEndpoint
{
public:
    using Endpoint = Ip6::Tcp::Endpoint;

    GuardedEndpoint(void)
        : mProtected(false)
    {
        long pageSize = sysconf(_SC_PAGESIZE);

        VerifyOrQuit(pageSize > 0);

        mPageSize = static_cast<size_t>(pageSize);
        VerifyOrQuit(sizeof(Endpoint) <= mPageSize);

        mPage = mmap(nullptr, mPageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        VerifyOrQuit(mPage != MAP_FAILED);

        mEndpoint = new (mPage) Endpoint;
    }

    ~GuardedEndpoint(void)
    {
        if (mProtected)
        {
            VerifyOrQuit(mprotect(mPage, mPageSize, PROT_READ | PROT_WRITE) == 0);
        }

        mEndpoint->~Endpoint();
        VerifyOrQuit(munmap(mPage, mPageSize) == 0);
    }

    Endpoint &Get(void) { return *mEndpoint; }

    void Protect(void)
    {
        VerifyOrQuit(mprotect(mPage, mPageSize, PROT_NONE) == 0);
        mProtected = true;
    }

private:
    void     *mPage;
    size_t    mPageSize;
    Endpoint *mEndpoint;
    bool      mProtected;
};

struct ClientContext
{
    GuardedBuffer    *mSecondBuffer;
    const otSockAddr *mReconnectAddress;
    uint16_t          mSendDoneCalls;
    uint16_t          mDisconnectedCalls;
    bool              mEstablished;
    bool              mEndOfStream;
};

struct ServerContext
{
    Ip6::Tcp::Endpoint *mAcceptEndpoint;
    bool                mAccepted;
};

struct DeinitializingServerContext
{
    GuardedEndpoint *mEndpoint;
    bool             mAcceptDoneCalled;
};

struct ForwardProgressContext
{
    GuardedEndpoint *mEndpoint;
    uint16_t         mCalls;
};

struct ReconnectingServerContext
{
    const otSockAddr *mReconnectAddress;
    bool              mAcceptDoneCalled;
};

static void HandleClientEstablished(otTcpEndpoint *aEndpoint)
{
    static_cast<ClientContext *>(otTcpEndpointGetContext(aEndpoint))->mEstablished = true;
}

static void HandleClientSendDoneAndAbort(otTcpEndpoint *aEndpoint, otLinkedBuffer *aBuffer)
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

static void HandleClientSendDoneAfterDisconnect(otTcpEndpoint *aEndpoint, otLinkedBuffer *)
{
    static_cast<ClientContext *>(otTcpEndpointGetContext(aEndpoint))->mSendDoneCalls++;
}

static void HandleClientReceiveAvailable(otTcpEndpoint *aEndpoint, size_t, bool aEndOfStream, size_t)
{
    static_cast<ClientContext *>(otTcpEndpointGetContext(aEndpoint))->mEndOfStream = aEndOfStream;
}

static void HandleClientDisconnected(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason)
{
    ClientContext &context = *static_cast<ClientContext *>(otTcpEndpointGetContext(aEndpoint));

    context.mDisconnectedCalls++;

    if (context.mDisconnectedCalls == 1)
    {
        VerifyOrQuit(aReason == OT_TCP_DISCONNECTED_REASON_NORMAL);
        VerifyOrQuit(context.mSecondBuffer != nullptr);

        // The disconnected callback returns all send buffers to the
        // application. Reclaim one immediately and reconnect the endpoint so
        // stale signal processing cannot use the old buffer chain.
        context.mSecondBuffer->Protect();
        context.mSecondBuffer = nullptr;

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

static otTcpIncomingConnectionAction HandleDeinitializingServerAcceptReady(otTcpListener *aListener,
                                                                           const otSockAddr *,
                                                                           otTcpEndpoint **aAcceptInto)
{
    DeinitializingServerContext &context =
        *static_cast<DeinitializingServerContext *>(otTcpListenerGetContext(aListener));

    *aAcceptInto = &context.mEndpoint->Get();

    return OT_TCP_INCOMING_CONNECTION_ACTION_ACCEPT;
}

static void HandleServerAcceptDoneAndDeinitialize(otTcpListener *aListener,
                                                  otTcpEndpoint *aEndpoint,
                                                  const otSockAddr *)
{
    DeinitializingServerContext &context =
        *static_cast<DeinitializingServerContext *>(otTcpListenerGetContext(aListener));

    VerifyOrQuit(aEndpoint == &context.mEndpoint->Get());
    context.mAcceptDoneCalled = true;

    SuccessOrQuit(otTcpEndpointDeinitialize(aEndpoint));
    context.mEndpoint->Protect();
}

static void HandleServerAcceptDoneAndReconnect(otTcpListener *aListener, otTcpEndpoint *aEndpoint, const otSockAddr *)
{
    ReconnectingServerContext &context = *static_cast<ReconnectingServerContext *>(otTcpListenerGetContext(aListener));

    context.mAcceptDoneCalled = true;

    SuccessOrQuit(otTcpAbort(aEndpoint));
    VerifyOrQuit(context.mReconnectAddress != nullptr);
    SuccessOrQuit(otTcpConnect(aEndpoint, context.mReconnectAddress, OT_TCP_CONNECT_NO_FAST_OPEN));
}

static void HandleForwardProgressAndDeinitialize(otTcpEndpoint *aEndpoint, size_t, size_t)
{
    ForwardProgressContext &context = *static_cast<ForwardProgressContext *>(otTcpEndpointGetContext(aEndpoint));

    context.mCalls++;

    SuccessOrQuit(otTcpEndpointDeinitialize(aEndpoint));
    context.mEndpoint->Protect();
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
    ClientContext      clientContext{nullptr, nullptr, 0, 0, false, false};

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
    clientEndpointArgs.mSendDoneCallback    = HandleClientSendDoneAndAbort;
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

    // Exercise the same endpoint-reuse hazard when tcplp_input() closes a
    // connection before ProcessSignals() is entered. Have the server close its
    // send side first so the client reaches LAST_ACK when it sends data and its
    // FIN. The ACK then retires the buffers and closes the client connection in
    // one input pass.
    Ip6::Tcp::Endpoint closingServerEndpoint;
    Ip6::Tcp::Listener closingServerListener;
    Ip6::Tcp::Endpoint closingClientEndpoint;
    uint8_t            closingServerReceiveBuffer[128];
    uint8_t            closingClientReceiveBuffer[128];
    ServerContext      closingServerContext{&closingServerEndpoint, false};
    ClientContext      closingClientContext{nullptr, nullptr, 0, 0, false, false};

    otTcpEndpointInitializeArgs closingServerEndpointArgs{};
    closingServerEndpointArgs.mReceiveBuffer     = closingServerReceiveBuffer;
    closingServerEndpointArgs.mReceiveBufferSize = sizeof(closingServerReceiveBuffer);

    otTcpListenerInitializeArgs closingListenerArgs{};
    closingListenerArgs.mContext             = &closingServerContext;
    closingListenerArgs.mAcceptReadyCallback = HandleServerAcceptReady;
    closingListenerArgs.mAcceptDoneCallback  = HandleServerAcceptDone;

    otTcpEndpointInitializeArgs closingClientEndpointArgs{};
    closingClientEndpointArgs.mContext                  = &closingClientContext;
    closingClientEndpointArgs.mEstablishedCallback      = HandleClientEstablished;
    closingClientEndpointArgs.mSendDoneCallback         = HandleClientSendDoneAfterDisconnect;
    closingClientEndpointArgs.mReceiveAvailableCallback = HandleClientReceiveAvailable;
    closingClientEndpointArgs.mDisconnectedCallback     = HandleClientDisconnected;
    closingClientEndpointArgs.mReceiveBuffer            = closingClientReceiveBuffer;
    closingClientEndpointArgs.mReceiveBufferSize        = sizeof(closingClientReceiveBuffer);

    SuccessOrQuit(closingServerEndpoint.Initialize(serverNode, closingServerEndpointArgs));
    SuccessOrQuit(closingServerListener.Initialize(serverNode, closingListenerArgs));
    SuccessOrQuit(closingClientEndpoint.Initialize(clientNode, closingClientEndpointArgs));
    SuccessOrQuit(closingServerListener.Listen(listenAddress));

    closingClientContext.mReconnectAddress = &reconnectAddress;
    SuccessOrQuit(closingClientEndpoint.Connect(serverAddress, OT_TCP_CONNECT_NO_FAST_OPEN));

    nexus.AdvanceTime(kTcpTime);
    VerifyOrQuit(closingClientContext.mEstablished);
    VerifyOrQuit(closingServerContext.mAccepted);

    SuccessOrQuit(closingServerEndpoint.SendEndOfStream());
    nexus.AdvanceTime(kTcpTime);
    VerifyOrQuit(closingClientContext.mEndOfStream);

    static const uint8_t kClosingFirstData[]  = {17, 18, 19, 20};
    static const uint8_t kClosingSecondData[] = {21, 22, 23, 24, 25};
    otLinkedBuffer       closingFirst{};
    GuardedBuffer        closingSecond;

    closingFirst.mData                 = kClosingFirstData;
    closingFirst.mLength               = sizeof(kClosingFirstData);
    closingSecond.Get().mData          = kClosingSecondData;
    closingSecond.Get().mLength        = sizeof(kClosingSecondData);
    closingClientContext.mSecondBuffer = &closingSecond;

    SuccessOrQuit(closingClientEndpoint.SendByReference(closingFirst, OT_TCP_SEND_MORE_TO_COME));
    SuccessOrQuit(closingClientEndpoint.SendByReference(closingSecond.Get(), OT_TCP_SEND_MORE_TO_COME));
    SuccessOrQuit(closingClientEndpoint.SendEndOfStream());

    nexus.AdvanceTime(kTcpTime);

    VerifyOrQuit(closingClientContext.mDisconnectedCalls >= 1);
    VerifyOrQuit(closingClientContext.mSecondBuffer == nullptr);
    VerifyOrQuit(closingClientContext.mSendDoneCalls == 0);

    SuccessOrQuit(closingClientEndpoint.Deinitialize());
    SuccessOrQuit(closingServerEndpoint.Deinitialize());
    SuccessOrQuit(closingServerListener.StopListening());
    SuccessOrQuit(closingServerListener.Deinitialize());

    // Verify that reconnecting an endpoint from the accept-done callback
    // invalidates signal processing for the connection that was accepted.
    {
        Ip6::Tcp::Endpoint        acceptingServerEndpoint;
        Ip6::Tcp::Listener        acceptingServerListener;
        uint8_t                   acceptingServerReceiveBuffer[128];
        ReconnectingServerContext acceptingServerContext{&reconnectAddress, false};
        otIp6Address              peerAddress{};

        otTcpEndpointInitializeArgs acceptingServerEndpointArgs{};
        acceptingServerEndpointArgs.mReceiveBuffer     = acceptingServerReceiveBuffer;
        acceptingServerEndpointArgs.mReceiveBufferSize = sizeof(acceptingServerReceiveBuffer);

        otTcpListenerInitializeArgs acceptingListenerArgs{};
        acceptingListenerArgs.mContext            = &acceptingServerContext;
        acceptingListenerArgs.mAcceptDoneCallback = HandleServerAcceptDoneAndReconnect;

        SuccessOrQuit(acceptingServerEndpoint.Initialize(serverNode, acceptingServerEndpointArgs));
        SuccessOrQuit(acceptingServerListener.Initialize(serverNode, acceptingListenerArgs));
        VerifyOrQuit(serverNode.Get<Ip6::Tcp>().StartProcessingSignals(acceptingServerEndpoint));

        VerifyOrQuit(!tcplp_sys_accepted_connection(&acceptingServerListener.GetTcbListen(),
                                                    &acceptingServerEndpoint.GetTcb(),
                                                    reinterpret_cast<struct in6_addr *>(&peerAddress),
                                                    BigEndian::HostSwap16(kPort)));
        VerifyOrQuit(acceptingServerContext.mAcceptDoneCalled);

        SuccessOrQuit(acceptingServerEndpoint.Deinitialize());
        SuccessOrQuit(acceptingServerListener.Deinitialize());
    }

    // Verify that deinitializing and reclaiming an accepted endpoint from the
    // accept-done callback does not leave TCP using the endpoint to send a
    // duplicate reset or to process signals for the abandoned connection.
    {
        GuardedEndpoint             acceptingServerEndpoint;
        Ip6::Tcp::Listener          acceptingServerListener;
        Ip6::Tcp::Endpoint          acceptingClientEndpoint;
        uint8_t                     acceptingServerReceiveBuffer[128];
        uint8_t                     acceptingClientReceiveBuffer[128];
        DeinitializingServerContext acceptingServerContext{&acceptingServerEndpoint, false};

        otTcpEndpointInitializeArgs acceptingServerEndpointArgs{};
        acceptingServerEndpointArgs.mReceiveBuffer     = acceptingServerReceiveBuffer;
        acceptingServerEndpointArgs.mReceiveBufferSize = sizeof(acceptingServerReceiveBuffer);

        otTcpListenerInitializeArgs acceptingListenerArgs{};
        acceptingListenerArgs.mContext             = &acceptingServerContext;
        acceptingListenerArgs.mAcceptReadyCallback = HandleDeinitializingServerAcceptReady;
        acceptingListenerArgs.mAcceptDoneCallback  = HandleServerAcceptDoneAndDeinitialize;

        otTcpEndpointInitializeArgs acceptingClientEndpointArgs{};
        acceptingClientEndpointArgs.mReceiveBuffer     = acceptingClientReceiveBuffer;
        acceptingClientEndpointArgs.mReceiveBufferSize = sizeof(acceptingClientReceiveBuffer);

        SuccessOrQuit(acceptingServerEndpoint.Get().Initialize(serverNode, acceptingServerEndpointArgs));
        SuccessOrQuit(acceptingServerListener.Initialize(serverNode, acceptingListenerArgs));
        SuccessOrQuit(acceptingClientEndpoint.Initialize(clientNode, acceptingClientEndpointArgs));
        SuccessOrQuit(acceptingServerListener.Listen(listenAddress));
        SuccessOrQuit(acceptingClientEndpoint.Connect(serverAddress, OT_TCP_CONNECT_NO_FAST_OPEN));

        nexus.AdvanceTime(kTcpTime);
        VerifyOrQuit(acceptingServerContext.mAcceptDoneCalled);

        SuccessOrQuit(acceptingClientEndpoint.Deinitialize());
        SuccessOrQuit(acceptingServerListener.StopListening());
        SuccessOrQuit(acceptingServerListener.Deinitialize());
    }

    // Exercise a pending forward-progress callback dispatched by the TCP
    // tasklet. The callback deinitializes the endpoint and immediately makes
    // its backing memory inaccessible.
    {
        Ip6::Tcp::Endpoint     progressServerEndpoint;
        Ip6::Tcp::Listener     progressServerListener;
        GuardedEndpoint        progressClientEndpoint;
        uint8_t                progressServerReceiveBuffer[128];
        uint8_t                progressClientReceiveBuffer[128];
        ServerContext          progressServerContext{&progressServerEndpoint, false};
        ForwardProgressContext progressClientContext{&progressClientEndpoint, 0};

        otTcpEndpointInitializeArgs progressServerEndpointArgs{};
        progressServerEndpointArgs.mReceiveBuffer     = progressServerReceiveBuffer;
        progressServerEndpointArgs.mReceiveBufferSize = sizeof(progressServerReceiveBuffer);

        otTcpListenerInitializeArgs progressListenerArgs{};
        progressListenerArgs.mContext             = &progressServerContext;
        progressListenerArgs.mAcceptReadyCallback = HandleServerAcceptReady;
        progressListenerArgs.mAcceptDoneCallback  = HandleServerAcceptDone;

        otTcpEndpointInitializeArgs progressClientEndpointArgs{};
        progressClientEndpointArgs.mContext           = &progressClientContext;
        progressClientEndpointArgs.mReceiveBuffer     = progressClientReceiveBuffer;
        progressClientEndpointArgs.mReceiveBufferSize = sizeof(progressClientReceiveBuffer);

        SuccessOrQuit(progressServerEndpoint.Initialize(serverNode, progressServerEndpointArgs));
        SuccessOrQuit(progressServerListener.Initialize(serverNode, progressListenerArgs));
        SuccessOrQuit(progressClientEndpoint.Get().Initialize(clientNode, progressClientEndpointArgs));
        SuccessOrQuit(progressServerListener.Listen(listenAddress));
        SuccessOrQuit(progressClientEndpoint.Get().Connect(serverAddress, OT_TCP_CONNECT_NO_FAST_OPEN));

        nexus.AdvanceTime(kTcpTime);
        VerifyOrQuit(progressServerContext.mAccepted);

        progressClientEndpoint.Get().mForwardProgressCallback = HandleForwardProgressAndDeinitialize;

        static const uint8_t kProgressData[] = {26, 27, 28, 29};
        otLinkedBuffer       progressBuffer{};

        progressBuffer.mData   = kProgressData;
        progressBuffer.mLength = sizeof(kProgressData);

        SuccessOrQuit(progressClientEndpoint.Get().SendByReference(progressBuffer, 0));
        nexus.AdvanceTime(0);
        VerifyOrQuit(progressClientContext.mCalls == 1);

        SuccessOrQuit(progressServerEndpoint.Deinitialize());
        SuccessOrQuit(progressServerListener.StopListening());
        SuccessOrQuit(progressServerListener.Deinitialize());
    }

    // Exercise the forward-progress callback dispatched directly from
    // ProcessSignals() after an ACK. Cancel the tasklet callback so the ACK is
    // the first event to report progress.
    {
        Ip6::Tcp::Endpoint     progressServerEndpoint;
        Ip6::Tcp::Listener     progressServerListener;
        GuardedEndpoint        progressClientEndpoint;
        uint8_t                progressServerReceiveBuffer[128];
        uint8_t                progressClientReceiveBuffer[128];
        ServerContext          progressServerContext{&progressServerEndpoint, false};
        ForwardProgressContext progressClientContext{&progressClientEndpoint, 0};

        otTcpEndpointInitializeArgs progressServerEndpointArgs{};
        progressServerEndpointArgs.mReceiveBuffer     = progressServerReceiveBuffer;
        progressServerEndpointArgs.mReceiveBufferSize = sizeof(progressServerReceiveBuffer);

        otTcpListenerInitializeArgs progressListenerArgs{};
        progressListenerArgs.mContext             = &progressServerContext;
        progressListenerArgs.mAcceptReadyCallback = HandleServerAcceptReady;
        progressListenerArgs.mAcceptDoneCallback  = HandleServerAcceptDone;

        otTcpEndpointInitializeArgs progressClientEndpointArgs{};
        progressClientEndpointArgs.mContext           = &progressClientContext;
        progressClientEndpointArgs.mReceiveBuffer     = progressClientReceiveBuffer;
        progressClientEndpointArgs.mReceiveBufferSize = sizeof(progressClientReceiveBuffer);

        SuccessOrQuit(progressServerEndpoint.Initialize(serverNode, progressServerEndpointArgs));
        SuccessOrQuit(progressServerListener.Initialize(serverNode, progressListenerArgs));
        SuccessOrQuit(progressClientEndpoint.Get().Initialize(clientNode, progressClientEndpointArgs));
        SuccessOrQuit(progressServerListener.Listen(listenAddress));
        SuccessOrQuit(progressClientEndpoint.Get().Connect(serverAddress, OT_TCP_CONNECT_NO_FAST_OPEN));

        nexus.AdvanceTime(kTcpTime);
        VerifyOrQuit(progressServerContext.mAccepted);

        progressClientEndpoint.Get().mForwardProgressCallback = HandleForwardProgressAndDeinitialize;

        static const uint8_t kProgressData[] = {30, 31, 32, 33};
        otLinkedBuffer       progressBuffer{};

        progressBuffer.mData   = kProgressData;
        progressBuffer.mLength = sizeof(kProgressData);

        SuccessOrQuit(progressClientEndpoint.Get().SendByReference(progressBuffer, 0));
        progressClientEndpoint.Get().mPendingCallbacks = 0;

        nexus.AdvanceTime(kTcpTime);
        VerifyOrQuit(progressClientContext.mCalls == 1);

        SuccessOrQuit(progressServerEndpoint.Deinitialize());
        SuccessOrQuit(progressServerListener.StopListening());
        SuccessOrQuit(progressServerListener.Deinitialize());
    }

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
