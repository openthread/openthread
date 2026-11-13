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

#include <stdlib.h>
#include <string.h>

#include <openthread/ip6.h>
#include <openthread/tasklet.h>
#include <openthread/tcp.h>

#include "common/array.hpp"
#include "common/clearable.hpp"
#include "common/time.hpp"
#include "instance/instance.hpp"

namespace ot {

#if OPENTHREAD_CONFIG_TCP_ENABLE

static constexpr char     kClientAddrString[] = "fd00::1";
static constexpr char     kServerAddrString[] = "fd00::2";
static constexpr uint16_t kClientPort1        = 10001;
static constexpr uint16_t kClientPort2        = 10002;
static constexpr uint16_t kServerPort         = 20000;
static constexpr uint16_t kServerPort2        = 20001;
static constexpr size_t   kRecvBufSize        = OT_TCP_RECEIVE_BUFFER_SIZE_FEW_HOPS;
static constexpr uint32_t kInitialTimeMs      = 10000;
static constexpr uint32_t kDelAckWaitMs       = 150;

static uint32_t sNow       = kInitialTimeMs;
static uint32_t sAlarmTime = 0;
static bool     sAlarmOn   = false;

extern "C" {

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sAlarmOn = false;
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    sAlarmOn   = true;
    sAlarmTime = aT0 + aDt;
}

uint32_t otPlatAlarmMilliGetNow(void) { return sNow; }

} // extern "C"

static void ProcessAllTasklets(Instance *aInstance)
{
    while (otTaskletsArePending(aInstance))
    {
        otTaskletsProcess(aInstance);
    }
}

static void AdvanceTime(Instance *aInstance, uint32_t aDuration)
{
    uint32_t targetTime = sNow + aDuration;

    ProcessAllTasklets(aInstance);

    while (sAlarmOn && (TimeMilli(sAlarmTime) <= TimeMilli(targetTime)))
    {
        sNow     = sAlarmTime;
        sAlarmOn = false;
        otPlatAlarmMilliFired(aInstance);
        ProcessAllTasklets(aInstance);
    }

    sNow = targetTime;
}

enum CallbackEvent : uint8_t
{
    kEventEstablished,
    kEventSendDone,
    kEventForwardProgress,
    kEventReceiveAvailable,
    kEventDisconnected,
};

enum ForwardProgressAction : uint8_t
{
    kActionNone,
    kActionDeinitAndFree,
    kActionSendMore,
    kActionReinitAndConnect,
};

enum DisconnectAction : uint8_t
{
    kDisconnectNone,
    kDisconnectDeinitAndFree,
};

struct EndpointContext
{
    Instance                *mInstance;
    otTcpEndpoint           *mEndpoint;
    uint8_t                  mRecvBuffer[kRecvBufSize];
    ForwardProgressAction    mForwardProgressAction;
    DisconnectAction         mDisconnectAction;
    bool                     mDeinitializing;
    bool                     mDeinitialized;
    uint32_t                 mEstablishedCount;
    uint32_t                 mSendDoneCount;
    uint32_t                 mForwardProgressCount;
    uint32_t                 mReceiveAvailableCount;
    uint32_t                 mDisconnectedCount;
    otTcpDisconnectedReason  mLastDisconnectedReason;
    size_t                   mLastInSendBuffer;
    size_t                   mLastBacklog;
    size_t                   mLastBytesAvailable;
    bool                     mLastEndOfStream;
    otLinkedBuffer          *mNextSendBuffer;
    otSockAddr               mReinitBindAddr;
    otSockAddr               mReinitPeerAddr;
    Array<CallbackEvent, 16> mEvents;
};

struct ListenerContext
{
    otTcpListener    mListener;
    EndpointContext *mAcceptContexts[2];
    uint8_t          mAcceptIndex;
    uint8_t          mAcceptCount;
    uint32_t         mAcceptDoneCount;
};

static void HandleEstablished(otTcpEndpoint *aEndpoint);
static void HandleSendDone(otTcpEndpoint *aEndpoint, otLinkedBuffer *aData);
static void HandleForwardProgress(otTcpEndpoint *aEndpoint, size_t aInSendBuffer, size_t aBacklog);
static void HandleReceiveAvailable(otTcpEndpoint *aEndpoint,
                                   size_t         aBytesAvailable,
                                   bool           aEndOfStream,
                                   size_t         aBytesRemaining);
static void HandleDisconnected(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason);

static void InitializeEndpointArgs(EndpointContext &aContext, otTcpEndpointInitializeArgs &aArgs)
{
    ClearAllBytes(aArgs);
    aArgs.mContext                  = &aContext;
    aArgs.mEstablishedCallback      = HandleEstablished;
    aArgs.mSendDoneCallback         = HandleSendDone;
    aArgs.mForwardProgressCallback  = HandleForwardProgress;
    aArgs.mReceiveAvailableCallback = HandleReceiveAvailable;
    aArgs.mDisconnectedCallback     = HandleDisconnected;
    aArgs.mReceiveBuffer            = aContext.mRecvBuffer;
    aArgs.mReceiveBufferSize        = sizeof(aContext.mRecvBuffer);
}

static void InitHeapEndpoint(Instance *aInstance, EndpointContext &aContext)
{
    otTcpEndpointInitializeArgs args;
    otTcpEndpoint              *endpoint;

    ClearAllBytes(aContext);
    aContext.mInstance = aInstance;

    endpoint = static_cast<otTcpEndpoint *>(calloc(1, sizeof(otTcpEndpoint)));
    VerifyOrQuit(endpoint != nullptr);
    aContext.mEndpoint = endpoint;

    InitializeEndpointArgs(aContext, args);
    SuccessOrQuit(otTcpEndpointInitialize(aInstance, endpoint, &args));
}

static void CleanupHeapEndpoint(EndpointContext &aContext)
{
    if (aContext.mEndpoint != nullptr)
    {
        aContext.mDeinitializing = true;
        SuccessOrQuit(otTcpEndpointDeinitialize(aContext.mEndpoint));
        aContext.mDeinitializing = false;
        aContext.mDeinitialized  = true;
        free(aContext.mEndpoint);
        aContext.mEndpoint = nullptr;
    }
}

static void HandleEstablished(otTcpEndpoint *aEndpoint)
{
    EndpointContext *ctx = static_cast<EndpointContext *>(otTcpEndpointGetContext(aEndpoint));

    VerifyOrQuit(ctx != nullptr);
    VerifyOrQuit(!ctx->mDeinitialized);
    VerifyOrQuit(ctx->mEndpoint == aEndpoint);

    ctx->mEstablishedCount++;
    SuccessOrQuit(ctx->mEvents.PushBack(kEventEstablished));
}

static void HandleSendDone(otTcpEndpoint *aEndpoint, otLinkedBuffer *aData)
{
    EndpointContext *ctx = static_cast<EndpointContext *>(otTcpEndpointGetContext(aEndpoint));

    VerifyOrQuit(ctx != nullptr);
    VerifyOrQuit(aData != nullptr);
    VerifyOrQuit(!ctx->mDeinitialized);
    VerifyOrQuit(ctx->mEndpoint == aEndpoint);

    ctx->mSendDoneCount++;
    SuccessOrQuit(ctx->mEvents.PushBack(kEventSendDone));
}

static void HandleForwardProgress(otTcpEndpoint *aEndpoint, size_t aInSendBuffer, size_t aBacklog)
{
    EndpointContext      *ctx = static_cast<EndpointContext *>(otTcpEndpointGetContext(aEndpoint));
    ForwardProgressAction action;

    VerifyOrQuit(ctx != nullptr);
    VerifyOrQuit(!ctx->mDeinitialized);
    VerifyOrQuit(ctx->mEndpoint == aEndpoint);

    action                      = ctx->mForwardProgressAction;
    ctx->mForwardProgressAction = kActionNone;

    ctx->mForwardProgressCount++;
    ctx->mLastInSendBuffer = aInSendBuffer;
    ctx->mLastBacklog      = aBacklog;
    SuccessOrQuit(ctx->mEvents.PushBack(kEventForwardProgress));

    switch (action)
    {
    case kActionNone:
        break;

    case kActionDeinitAndFree:
        ctx->mDeinitializing = true;
        SuccessOrQuit(otTcpEndpointDeinitialize(aEndpoint));
        ctx->mDeinitializing = false;
        ctx->mDeinitialized  = true;
        memset(aEndpoint, 0xa5, sizeof(*aEndpoint));
        free(aEndpoint);
        ctx->mEndpoint = nullptr;
        break;

    case kActionSendMore:
        VerifyOrQuit(ctx->mNextSendBuffer != nullptr);
        SuccessOrQuit(otTcpSendByReference(aEndpoint, ctx->mNextSendBuffer, 0));
        ctx->mNextSendBuffer = nullptr;
        break;

    case kActionReinitAndConnect:
    {
        otTcpEndpointInitializeArgs args;

        ctx->mDeinitializing = true;
        SuccessOrQuit(otTcpEndpointDeinitialize(aEndpoint));
        ctx->mDeinitializing = false;

        InitializeEndpointArgs(*ctx, args);
        SuccessOrQuit(otTcpEndpointInitialize(ctx->mInstance, aEndpoint, &args));
        SuccessOrQuit(otTcpBind(aEndpoint, &ctx->mReinitBindAddr));
        SuccessOrQuit(otTcpConnect(aEndpoint, &ctx->mReinitPeerAddr, OT_TCP_CONNECT_NO_FAST_OPEN));
        break;
    }
    }
}

static void HandleReceiveAvailable(otTcpEndpoint *aEndpoint,
                                   size_t         aBytesAvailable,
                                   bool           aEndOfStream,
                                   size_t         aBytesRemaining)
{
    OT_UNUSED_VARIABLE(aBytesRemaining);

    EndpointContext *ctx = static_cast<EndpointContext *>(otTcpEndpointGetContext(aEndpoint));

    VerifyOrQuit(ctx != nullptr);
    VerifyOrQuit(!ctx->mDeinitialized);
    VerifyOrQuit(ctx->mEndpoint == aEndpoint);

    ctx->mReceiveAvailableCount++;
    ctx->mLastBytesAvailable = aBytesAvailable;
    ctx->mLastEndOfStream    = aEndOfStream;
    SuccessOrQuit(ctx->mEvents.PushBack(kEventReceiveAvailable));
}

static void HandleDisconnected(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason)
{
    EndpointContext *ctx = static_cast<EndpointContext *>(otTcpEndpointGetContext(aEndpoint));

    VerifyOrQuit(ctx != nullptr);
    VerifyOrQuit(!ctx->mDeinitialized);
    VerifyOrQuit(ctx->mEndpoint == aEndpoint);

    ctx->mDisconnectedCount++;
    ctx->mLastDisconnectedReason = aReason;

    if (!ctx->mDeinitializing)
    {
        DisconnectAction action = ctx->mDisconnectAction;

        ctx->mDisconnectAction = kDisconnectNone;
        SuccessOrQuit(ctx->mEvents.PushBack(kEventDisconnected));

        if (action == kDisconnectDeinitAndFree)
        {
            ctx->mDeinitializing = true;
            SuccessOrQuit(otTcpEndpointDeinitialize(aEndpoint));
            ctx->mDeinitializing = false;
            ctx->mDeinitialized  = true;
            memset(aEndpoint, 0xa5, sizeof(*aEndpoint));
            free(aEndpoint);
            ctx->mEndpoint = nullptr;
        }
    }
}

static otTcpIncomingConnectionAction HandleAcceptReady(otTcpListener    *aListener,
                                                       const otSockAddr *aPeer,
                                                       otTcpEndpoint   **aAcceptInto)
{
    OT_UNUSED_VARIABLE(aPeer);

    ListenerContext *ctx = static_cast<ListenerContext *>(otTcpListenerGetContext(aListener));

    VerifyOrQuit(ctx != nullptr);
    VerifyOrQuit(ctx->mAcceptIndex < ctx->mAcceptCount);
    VerifyOrQuit(ctx->mAcceptContexts[ctx->mAcceptIndex] != nullptr);
    VerifyOrQuit(ctx->mAcceptContexts[ctx->mAcceptIndex]->mEndpoint != nullptr);

    *aAcceptInto = ctx->mAcceptContexts[ctx->mAcceptIndex]->mEndpoint;
    ctx->mAcceptIndex++;

    return OT_TCP_INCOMING_CONNECTION_ACTION_ACCEPT;
}

static void HandleAcceptDone(otTcpListener *aListener, otTcpEndpoint *aEndpoint, const otSockAddr *aPeer)
{
    OT_UNUSED_VARIABLE(aEndpoint);
    OT_UNUSED_VARIABLE(aPeer);

    ListenerContext *ctx = static_cast<ListenerContext *>(otTcpListenerGetContext(aListener));

    VerifyOrQuit(ctx != nullptr);
    ctx->mAcceptDoneCount++;
}

static void MakeSockAddr(const char *aAddrString, uint16_t aPort, otSockAddr &aSockAddr)
{
    ClearAllBytes(aSockAddr);
    SuccessOrQuit(otIp6AddressFromString(aAddrString, &aSockAddr.mAddress));
    aSockAddr.mPort = aPort;
}

static void AddUnicastAddress(Instance *aInstance, const char *aAddressString)
{
    otNetifAddress netifAddr;

    ClearAllBytes(netifAddr);
    SuccessOrQuit(otIp6AddressFromString(aAddressString, &netifAddr.mAddress));
    netifAddr.mPrefixLength  = 64;
    netifAddr.mAddressOrigin = OT_ADDRESS_ORIGIN_MANUAL;
    netifAddr.mPreferred     = true;
    netifAddr.mValid         = true;

    SuccessOrQuit(otIp6AddUnicastAddress(aInstance, &netifAddr));
}

static void RemoveUnicastAddress(Instance *aInstance, const char *aAddressString)
{
    otIp6Address addr;

    SuccessOrQuit(otIp6AddressFromString(aAddressString, &addr));
    SuccessOrQuit(otIp6RemoveUnicastAddress(aInstance, &addr));
}

static Instance *InitTestInstance(void)
{
    Instance *instance;

    sNow     = kInitialTimeMs;
    sAlarmOn = false;

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    SuccessOrQuit(otIp6SetEnabled(instance, true));
    AddUnicastAddress(instance, kClientAddrString);
    AddUnicastAddress(instance, kServerAddrString);
    ProcessAllTasklets(instance);

    return instance;
}

static void InitListenerOnPort(Instance        *aInstance,
                               ListenerContext &aListenerCtx,
                               uint16_t         aPort,
                               EndpointContext *aServerCtx0,
                               EndpointContext *aServerCtx1)
{
    otTcpListenerInitializeArgs listenerArgs;
    otSockAddr                  listenAddr;

    ClearAllBytes(aListenerCtx);
    aListenerCtx.mAcceptContexts[0] = aServerCtx0;
    aListenerCtx.mAcceptContexts[1] = aServerCtx1;
    aListenerCtx.mAcceptCount       = (aServerCtx1 != nullptr) ? 2 : 1;

    ClearAllBytes(listenerArgs);
    listenerArgs.mContext             = &aListenerCtx;
    listenerArgs.mAcceptReadyCallback = HandleAcceptReady;
    listenerArgs.mAcceptDoneCallback  = HandleAcceptDone;

    SuccessOrQuit(otTcpListenerInitialize(aInstance, &aListenerCtx.mListener, &listenerArgs));
    MakeSockAddr(kServerAddrString, aPort, listenAddr);
    SuccessOrQuit(otTcpListen(&aListenerCtx.mListener, &listenAddr));
}

static void InitListener(Instance        *aInstance,
                         ListenerContext &aListenerCtx,
                         EndpointContext *aServerCtx0,
                         EndpointContext *aServerCtx1)
{
    InitListenerOnPort(aInstance, aListenerCtx, kServerPort, aServerCtx0, aServerCtx1);
}

static void ConnectClient(Instance *aInstance, EndpointContext &aClientCtx, uint16_t aClientPort)
{
    otSockAddr clientAddr;
    otSockAddr serverAddr;

    MakeSockAddr(kClientAddrString, aClientPort, clientAddr);
    MakeSockAddr(kServerAddrString, kServerPort, serverAddr);

    SuccessOrQuit(otTcpBind(aClientCtx.mEndpoint, &clientAddr));
    SuccessOrQuit(otTcpConnect(aClientCtx.mEndpoint, &serverAddr, OT_TCP_CONNECT_NO_FAST_OPEN));
    ProcessAllTasklets(aInstance);

    VerifyOrQuit(aClientCtx.mEstablishedCount == 1);
    aClientCtx.mForwardProgressCount = 0;
    aClientCtx.mEvents.Clear();
}

static void InitLinkedBuffer(otLinkedBuffer &aBuffer, const uint8_t *aData, size_t aLength)
{
    ClearAllBytes(aBuffer);
    aBuffer.mData   = aData;
    aBuffer.mLength = aLength;
}

static void TestDeinitInForwardProgressTasklet(void)
{
    static const uint8_t kSendData1[] = {'t', 'a', 's', 'k', '1'};
    static const uint8_t kSendData2[] = {'t', 'a', 's', 'k', '2'};

    Instance       *instance = InitTestInstance();
    EndpointContext server1Ctx;
    EndpointContext server2Ctx;
    EndpointContext client1Ctx;
    EndpointContext client2Ctx;
    ListenerContext listenerCtx;
    otLinkedBuffer  sendBuf1;
    otLinkedBuffer  sendBuf2;

    printf("TestDeinitInForwardProgressTasklet\n");

    InitHeapEndpoint(instance, server1Ctx);
    InitHeapEndpoint(instance, server2Ctx);
    InitHeapEndpoint(instance, client2Ctx);
    InitHeapEndpoint(instance, client1Ctx);
    InitListener(instance, listenerCtx, &server1Ctx, &server2Ctx);

    ConnectClient(instance, client1Ctx, kClientPort1);
    ConnectClient(instance, client2Ctx, kClientPort2);
    VerifyOrQuit(listenerCtx.mAcceptDoneCount == 2);

    InitLinkedBuffer(sendBuf1, kSendData1, sizeof(kSendData1));
    InitLinkedBuffer(sendBuf2, kSendData2, sizeof(kSendData2));

    client1Ctx.mForwardProgressAction = kActionDeinitAndFree;

    SuccessOrQuit(otTcpSendByReference(client1Ctx.mEndpoint, &sendBuf1, 0));
    SuccessOrQuit(otTcpSendByReference(client2Ctx.mEndpoint, &sendBuf2, 0));

    ProcessAllTasklets(instance);

    VerifyOrQuit(client1Ctx.mForwardProgressCount == 1);
    VerifyOrQuit(client1Ctx.mLastInSendBuffer == sizeof(kSendData1));
    VerifyOrQuit(client1Ctx.mLastBacklog == 0);
    VerifyOrQuit(client1Ctx.mEndpoint == nullptr);

    VerifyOrQuit(client2Ctx.mForwardProgressCount == 1);
    VerifyOrQuit(client2Ctx.mLastInSendBuffer == sizeof(kSendData2));
    VerifyOrQuit(client2Ctx.mLastBacklog == 0);

    SuccessOrQuit(otTcpListenerDeinitialize(&listenerCtx.mListener));
    CleanupHeapEndpoint(client1Ctx);
    CleanupHeapEndpoint(client2Ctx);
    CleanupHeapEndpoint(server1Ctx);
    CleanupHeapEndpoint(server2Ctx);
    testFreeInstance(instance);
}

static void TestSendMoreInForwardProgressCallbacks(void)
{
    static const uint8_t kData1[] = {'a', 'b', 'c', 'd'};
    static const uint8_t kData2[] = {'e', 'f', 'g', 'h', 'i', 'j'};

    Instance       *instance = InitTestInstance();
    EndpointContext serverCtx;
    EndpointContext clientCtx;
    ListenerContext listenerCtx;
    uint8_t         data3[512];
    otLinkedBuffer  sendBuf1;
    otLinkedBuffer  sendBuf2;
    otLinkedBuffer  sendBuf3;

    printf("TestSendMoreInForwardProgressCallbacks\n");

    memset(data3, 'x', sizeof(data3));

    InitHeapEndpoint(instance, serverCtx);
    InitHeapEndpoint(instance, clientCtx);
    InitListener(instance, listenerCtx, &serverCtx, nullptr);

    ConnectClient(instance, clientCtx, kClientPort1);

    // Send more data from inside the ACK-driven ProcessSignals forward progress callback first.
    InitLinkedBuffer(sendBuf1, kData1, sizeof(kData1));
    InitLinkedBuffer(sendBuf2, kData2, sizeof(kData2));
    InitLinkedBuffer(sendBuf3, data3, sizeof(data3));

    SuccessOrQuit(otTcpSendByReference(clientCtx.mEndpoint, &sendBuf1, 0));
    ProcessAllTasklets(instance);
    VerifyOrQuit(clientCtx.mForwardProgressCount == 1);

    clientCtx.mForwardProgressAction = kActionSendMore;
    clientCtx.mNextSendBuffer        = &sendBuf2;

    AdvanceTime(instance, kDelAckWaitMs);

    VerifyOrQuit(clientCtx.mSendDoneCount == 1);
    VerifyOrQuit(clientCtx.mForwardProgressCount == 3);
    VerifyOrQuit(clientCtx.mLastInSendBuffer == sizeof(kData2));
    VerifyOrQuit(clientCtx.mLastBacklog == 0);

    AdvanceTime(instance, kDelAckWaitMs);
    VerifyOrQuit(clientCtx.mSendDoneCount == 2);
    VerifyOrQuit(clientCtx.mForwardProgressCount == 4);

    // Remove the server unicast address so outgoing segments are not ACKed, isolating the
    // tasklet-driven FirePendingCallbacks path when otTcpSendByReference is called inside the callback.
    RemoveUnicastAddress(instance, kServerAddrString);
    InitLinkedBuffer(sendBuf1, kData1, sizeof(kData1));

    clientCtx.mForwardProgressAction = kActionSendMore;
    clientCtx.mNextSendBuffer        = &sendBuf3;
    SuccessOrQuit(otTcpSendByReference(clientCtx.mEndpoint, &sendBuf1, 0));

    ProcessAllTasklets(instance);

    VerifyOrQuit(clientCtx.mForwardProgressCount == 6);
    VerifyOrQuit(clientCtx.mLastInSendBuffer == sizeof(kData1) + sizeof(data3));
    VerifyOrQuit(clientCtx.mLastBacklog < sizeof(data3));

    SuccessOrQuit(otTcpListenerDeinitialize(&listenerCtx.mListener));
    CleanupHeapEndpoint(clientCtx);
    CleanupHeapEndpoint(serverCtx);
    testFreeInstance(instance);
}

static void TestDeinitInForwardProgressOnPureAck(void)
{
    static const uint8_t kSendData[] = {'p', 'i', 'n', 'g'};

    Instance       *instance = InitTestInstance();
    EndpointContext serverCtx;
    EndpointContext clientCtx;
    ListenerContext listenerCtx;
    otLinkedBuffer  sendBuf;

    printf("TestDeinitInForwardProgressOnPureAck\n");

    InitHeapEndpoint(instance, serverCtx);
    InitHeapEndpoint(instance, clientCtx);
    InitListener(instance, listenerCtx, &serverCtx, nullptr);

    ConnectClient(instance, clientCtx, kClientPort1);

    InitLinkedBuffer(sendBuf, kSendData, sizeof(kSendData));
    SuccessOrQuit(otTcpSendByReference(clientCtx.mEndpoint, &sendBuf, 0));
    ProcessAllTasklets(instance);

    VerifyOrQuit(clientCtx.mForwardProgressCount == 1);
    VerifyOrQuit(clientCtx.mLastInSendBuffer == sizeof(kSendData));

    clientCtx.mForwardProgressAction = kActionDeinitAndFree;

    AdvanceTime(instance, kDelAckWaitMs);

    VerifyOrQuit(clientCtx.mForwardProgressCount == 2);
    VerifyOrQuit(clientCtx.mLastInSendBuffer == 0);
    VerifyOrQuit(clientCtx.mLastBacklog == 0);
    VerifyOrQuit(clientCtx.mEndpoint == nullptr);
    VerifyOrQuit(clientCtx.mReceiveAvailableCount == 0);

    SuccessOrQuit(otTcpListenerDeinitialize(&listenerCtx.mListener));
    CleanupHeapEndpoint(clientCtx);
    CleanupHeapEndpoint(serverCtx);
    testFreeInstance(instance);
}

static void TestDeinitInForwardProgressOnPiggybackedAckDataFin(void)
{
    static const uint8_t kSendData[]  = {'r', 'e', 'q', 'u', 'e', 's', 't'};
    static const uint8_t kReplyData[] = {'r', 'e', 's', 'p', 'o', 'n', 's', 'e'};

    Instance       *instance = InitTestInstance();
    EndpointContext serverCtx;
    EndpointContext clientCtx;
    ListenerContext listenerCtx;
    otLinkedBuffer  sendBuf;
    otLinkedBuffer  replyBuf;

    printf("TestDeinitInForwardProgressOnPiggybackedAckDataFin\n");

    InitHeapEndpoint(instance, serverCtx);
    InitHeapEndpoint(instance, clientCtx);
    InitListener(instance, listenerCtx, &serverCtx, nullptr);

    ConnectClient(instance, clientCtx, kClientPort1);

    InitLinkedBuffer(sendBuf, kSendData, sizeof(kSendData));
    SuccessOrQuit(otTcpSendByReference(clientCtx.mEndpoint, &sendBuf, 0));
    ProcessAllTasklets(instance);

    VerifyOrQuit(clientCtx.mForwardProgressCount == 1);
    VerifyOrQuit(clientCtx.mLastInSendBuffer == sizeof(kSendData));
    VerifyOrQuit(serverCtx.mReceiveAvailableCount == 1);

    clientCtx.mForwardProgressAction = kActionDeinitAndFree;

    // Before the server's delayed ACK timer fires, send data and FIN from the server so a single
    // segment carries ACK + payload + FIN into ProcessSignals on the client.
    InitLinkedBuffer(replyBuf, kReplyData, sizeof(kReplyData));
    SuccessOrQuit(otTcpSendByReference(serverCtx.mEndpoint, &replyBuf, OT_TCP_SEND_MORE_TO_COME));
    SuccessOrQuit(otTcpSendEndOfStream(serverCtx.mEndpoint));
    ProcessAllTasklets(instance);

    VerifyOrQuit(clientCtx.mForwardProgressCount == 2);
    VerifyOrQuit(clientCtx.mLastInSendBuffer == 0);
    VerifyOrQuit(clientCtx.mLastBacklog == 0);
    VerifyOrQuit(clientCtx.mEndpoint == nullptr);
    VerifyOrQuit(clientCtx.mReceiveAvailableCount == 0);

    SuccessOrQuit(otTcpListenerDeinitialize(&listenerCtx.mListener));
    CleanupHeapEndpoint(clientCtx);
    CleanupHeapEndpoint(serverCtx);
    testFreeInstance(instance);
}

static void TestForwardProgressOrderingAndReinit(void)
{
    static const uint8_t kSendData[]    = {'h', 'e', 'l', 'l', 'o'};
    static const uint8_t kReplyData[]   = {'w', 'o', 'r', 'l', 'd', '!'};
    static const uint8_t kNewConnData[] = {'r', 'e', 'c', 'o', 'n', 'n', 'e', 'c', 't'};

    Instance             *instance = InitTestInstance();
    EndpointContext       server1Ctx;
    EndpointContext       server2Ctx;
    EndpointContext       clientCtx;
    ListenerContext       listenerCtx;
    ListenerContext       listener2Ctx;
    otLinkedBuffer        sendBuf;
    otLinkedBuffer        replyBuf;
    otLinkedBuffer        newSendBuf;
    const otLinkedBuffer *rxBuf = nullptr;

    printf("TestForwardProgressOrderingAndReinit\n");

    // Part 1: Verify synchronous callback ordering (SendDone -> ForwardProgress -> ReceiveAvailable)
    // when a single incoming segment simultaneously ACKs sent data and delivers payload + FIN.
    InitHeapEndpoint(instance, server1Ctx);
    InitHeapEndpoint(instance, server2Ctx);
    InitHeapEndpoint(instance, clientCtx);
    InitListener(instance, listenerCtx, &server1Ctx, &server2Ctx);

    ConnectClient(instance, clientCtx, kClientPort1);

    InitLinkedBuffer(sendBuf, kSendData, sizeof(kSendData));
    SuccessOrQuit(otTcpSendByReference(clientCtx.mEndpoint, &sendBuf, 0));
    ProcessAllTasklets(instance);

    VerifyOrQuit(clientCtx.mForwardProgressCount == 1);
    clientCtx.mEvents.Clear();

    InitLinkedBuffer(replyBuf, kReplyData, sizeof(kReplyData));
    SuccessOrQuit(otTcpSendByReference(server1Ctx.mEndpoint, &replyBuf, OT_TCP_SEND_MORE_TO_COME));
    SuccessOrQuit(otTcpSendEndOfStream(server1Ctx.mEndpoint));
    ProcessAllTasklets(instance);

    VerifyOrQuit(clientCtx.mEvents.GetLength() == 3);
    VerifyOrQuit(clientCtx.mEvents[0] == kEventSendDone);
    VerifyOrQuit(clientCtx.mEvents[1] == kEventForwardProgress);
    VerifyOrQuit(clientCtx.mEvents[2] == kEventReceiveAvailable);
    VerifyOrQuit(clientCtx.mLastInSendBuffer == 0);
    VerifyOrQuit(clientCtx.mLastBacklog == 0);
    VerifyOrQuit(clientCtx.mLastBytesAvailable == sizeof(kReplyData));
    VerifyOrQuit(clientCtx.mLastEndOfStream);

    SuccessOrQuit(otTcpListenerDeinitialize(&listenerCtx.mListener));
    CleanupHeapEndpoint(clientCtx);
    CleanupHeapEndpoint(server1Ctx);
    CleanupHeapEndpoint(server2Ctx);
    testFreeInstance(instance);

    // Part 2: Verify deinitializing, re-initializing, binding, and connecting the same otTcpEndpoint
    // to a second listener inside mForwardProgressCallback on a piggybacked ACK + payload + FIN segment,
    // so the endpoint is immediately in SYN_SENT (!IsClosed()) when mForwardProgressCallback returns.
    instance = InitTestInstance();
    InitHeapEndpoint(instance, server1Ctx);
    InitHeapEndpoint(instance, server2Ctx);
    InitHeapEndpoint(instance, clientCtx);
    InitListenerOnPort(instance, listenerCtx, kServerPort, &server1Ctx, nullptr);
    InitListenerOnPort(instance, listener2Ctx, kServerPort2, &server2Ctx, nullptr);

    ConnectClient(instance, clientCtx, kClientPort1);

    InitLinkedBuffer(sendBuf, kSendData, sizeof(kSendData));
    SuccessOrQuit(otTcpSendByReference(clientCtx.mEndpoint, &sendBuf, 0));
    ProcessAllTasklets(instance);
    clientCtx.mEvents.Clear();

    MakeSockAddr(kClientAddrString, kClientPort2, clientCtx.mReinitBindAddr);
    MakeSockAddr(kServerAddrString, kServerPort2, clientCtx.mReinitPeerAddr);
    clientCtx.mForwardProgressAction = kActionReinitAndConnect;

    InitLinkedBuffer(replyBuf, kReplyData, sizeof(kReplyData));
    SuccessOrQuit(otTcpSendByReference(server1Ctx.mEndpoint, &replyBuf, OT_TCP_SEND_MORE_TO_COME));
    SuccessOrQuit(otTcpSendEndOfStream(server1Ctx.mEndpoint));
    ProcessAllTasklets(instance);

    // Stale ReceiveAvailable / Disconnected signals from the old connection must not fire on the new connection.
    VerifyOrQuit(clientCtx.mReceiveAvailableCount == 0);
    VerifyOrQuit(!clientCtx.mEvents.Contains(kEventDisconnected));
    VerifyOrQuit(clientCtx.mEndpoint != nullptr);
    VerifyOrQuit(clientCtx.mEstablishedCount == 2);
    VerifyOrQuit(listener2Ctx.mAcceptDoneCount == 1);

    // Verify the new connection transfers data cleanly end-to-end.
    InitLinkedBuffer(newSendBuf, kNewConnData, sizeof(kNewConnData));
    SuccessOrQuit(otTcpSendByReference(clientCtx.mEndpoint, &newSendBuf, 0));
    ProcessAllTasklets(instance);

    VerifyOrQuit(server2Ctx.mReceiveAvailableCount == 1);
    VerifyOrQuit(server2Ctx.mLastBytesAvailable == sizeof(kNewConnData));
    SuccessOrQuit(otTcpReceiveByReference(server2Ctx.mEndpoint, &rxBuf));
    VerifyOrQuit(rxBuf != nullptr);
    VerifyOrQuit(rxBuf->mLength == sizeof(kNewConnData));
    VerifyOrQuit(memcmp(rxBuf->mData, kNewConnData, sizeof(kNewConnData)) == 0);

    SuccessOrQuit(otTcpListenerDeinitialize(&listenerCtx.mListener));
    SuccessOrQuit(otTcpListenerDeinitialize(&listener2Ctx.mListener));
    CleanupHeapEndpoint(clientCtx);
    CleanupHeapEndpoint(server1Ctx);
    CleanupHeapEndpoint(server2Ctx);
    testFreeInstance(instance);
}

static void TestDeinitInDisconnectedOnRst(void)
{
    Instance       *instance = InitTestInstance();
    EndpointContext serverCtx;
    EndpointContext clientCtx;
    ListenerContext listenerCtx;

    printf("TestDeinitInDisconnectedOnRst\n");

    InitHeapEndpoint(instance, serverCtx);
    InitHeapEndpoint(instance, clientCtx);
    InitListener(instance, listenerCtx, &serverCtx, nullptr);

    ConnectClient(instance, clientCtx, kClientPort1);

    clientCtx.mDisconnectAction = kDisconnectDeinitAndFree;

    // Aborting the server endpoint sends an RST segment to the client, triggering
    // tcplp_sys_connection_lost -> mDisconnectedCallback inside tcplp_input.
    SuccessOrQuit(otTcpAbort(serverCtx.mEndpoint));
    ProcessAllTasklets(instance);

    VerifyOrQuit(clientCtx.mDisconnectedCount == 1);
    VerifyOrQuit(clientCtx.mLastDisconnectedReason == OT_TCP_DISCONNECTED_REASON_RESET);
    VerifyOrQuit(clientCtx.mEndpoint == nullptr);
    VerifyOrQuit(clientCtx.mReceiveAvailableCount == 0);

    SuccessOrQuit(otTcpListenerDeinitialize(&listenerCtx.mListener));
    CleanupHeapEndpoint(clientCtx);
    CleanupHeapEndpoint(serverCtx);
    testFreeInstance(instance);
}

#endif // OPENTHREAD_CONFIG_TCP_ENABLE

} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_TCP_ENABLE
    ot::TestDeinitInForwardProgressTasklet();
    ot::TestSendMoreInForwardProgressCallbacks();
    ot::TestDeinitInForwardProgressOnPureAck();
    ot::TestDeinitInForwardProgressOnPiggybackedAckDataFin();
    ot::TestForwardProgressOrderingAndReinit();
    ot::TestDeinitInDisconnectedOnRst();
#endif

    printf("All tests passed\n");
    return 0;
}
