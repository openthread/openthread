/*
 *  Copyright (c) 2019-2021, The OpenThread Authors.
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
 *   This file implements platform for TREL using IPv6/UDP socket under POSIX.
 */

#include "openthread-posix-config.h"

#include "platform-posix.h"

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openthread/logging.h>
#include <openthread/platform/trel.h>

#include "radio_url.hpp"
#include "system.hpp"
#include "common/code_utils.hpp"

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

static constexpr uint16_t kMaxPacketSize = 1400; // The max size of a TREL packet.

typedef struct TxPacket
{
    struct TxPacket *mNext;
    uint8_t          mBuffer[kMaxPacketSize];
    uint16_t         mLength;
    otSockAddr       mDestSockAddr;
} TxPacket;

static uint8_t            sRxPacketBuffer[kMaxPacketSize];
static uint16_t           sRxPacketLength;
static TxPacket           sTxPacketPool[OPENTHREAD_POSIX_CONFIG_TREL_TX_PACKET_POOL_SIZE];
static TxPacket          *sFreeTxPacketHead;  // A singly linked list of free/available `TxPacket` from pool.
static TxPacket          *sTxPacketQueueTail; // A circular linked list for queued tx packets.
static otPlatTrelCounters sCounters;

static char sInterfaceName[IFNAMSIZ + 1];
static bool sInitialized = false;
static bool sEnabled     = false;
static int  sSocket      = -1;

static const char *Ip6AddrToString(const void *aAddress)
{
    static char string[INET6_ADDRSTRLEN];
    return inet_ntop(AF_INET6, aAddress, string, sizeof(string));
}

static const char *BufferToString(const uint8_t *aBuffer, uint16_t aLength)
{
    const uint16_t kMaxWrite = 16;
    static char    string[1600];

    uint16_t num = 0;
    char    *cur = &string[0];
    char    *end = &string[sizeof(string) - 1];

    cur += snprintf(cur, (uint16_t)(end - cur), "[(len:%d) ", aLength);
    VerifyOrExit(cur < end);

    while (aLength-- && (num < kMaxWrite))
    {
        cur += snprintf(cur, (uint16_t)(end - cur), "%02x ", *aBuffer++);
        VerifyOrExit(cur < end);

        num++;
    }

    if (aLength != 0)
    {
        cur += snprintf(cur, (uint16_t)(end - cur), "... ");
        VerifyOrExit(cur < end);
    }

    *cur++ = ']';
    VerifyOrExit(cur < end);

    *cur = '\0';

exit:
    *end = '\0';
    return string;
}

static void PrepareSocket(uint16_t &aUdpPort)
{
    int                 val;
    struct sockaddr_in6 sockAddr;
    socklen_t           sockLen;

    otLogDebgPlat("[trel] PrepareSocket()");

    sSocket = SocketWithCloseExec(AF_INET6, SOCK_DGRAM, 0, kSocketNonBlock);
    VerifyOrDie(sSocket >= 0, OT_EXIT_ERROR_ERRNO);

    // Make the socket non-blocking to allow immediate tx attempt.
    val = fcntl(sSocket, F_GETFL, 0);
    VerifyOrDie(val != -1, OT_EXIT_ERROR_ERRNO);
    val = val | O_NONBLOCK;
    VerifyOrDie(fcntl(sSocket, F_SETFL, val) == 0, OT_EXIT_ERROR_ERRNO);

    // Bind the socket.

    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin6_family = AF_INET6;
    sockAddr.sin6_addr   = in6addr_any;
    sockAddr.sin6_port   = OPENTHREAD_POSIX_CONFIG_TREL_UDP_PORT;

    if (bind(sSocket, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) == -1)
    {
        otLogCritPlat("[trel] Failed to bind socket");
        DieNow(OT_EXIT_ERROR_ERRNO);
    }

    sockLen = sizeof(sockAddr);

    if (getsockname(sSocket, (struct sockaddr *)&sockAddr, &sockLen) == -1)
    {
        otLogCritPlat("[trel] Failed to get the socket name");
        DieNow(OT_EXIT_ERROR_ERRNO);
    }

    aUdpPort = ntohs(sockAddr.sin6_port);
}

static otError SendPacket(const uint8_t *aBuffer, uint16_t aLength, const otSockAddr *aDestSockAddr)
{
    otError             error = OT_ERROR_NONE;
    struct sockaddr_in6 sockAddr;
    ssize_t             ret;

    VerifyOrExit(sSocket >= 0, error = OT_ERROR_INVALID_STATE);

    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin6_family = AF_INET6;
    sockAddr.sin6_port   = htons(aDestSockAddr->mPort);
    memcpy(&sockAddr.sin6_addr, &aDestSockAddr->mAddress, sizeof(otIp6Address));

    ret = sendto(sSocket, aBuffer, aLength, 0, (struct sockaddr *)&sockAddr, sizeof(sockAddr));

    if (ret != aLength)
    {
        otLogDebgPlat("[trel] SendPacket() -- sendto() failed errno %d", errno);

        switch (errno)
        {
        case ENETUNREACH:
        case ENETDOWN:
        case EHOSTUNREACH:
            error = OT_ERROR_ABORT;
            break;

        default:
            error = OT_ERROR_INVALID_STATE;
        }
    }
    else
    {
        ++sCounters.mTxPackets;
        sCounters.mTxBytes += aLength;
    }

exit:
    otLogDebgPlat("[trel] SendPacket([%s]:%u) err:%s pkt:%s", Ip6AddrToString(&aDestSockAddr->mAddress),
                  aDestSockAddr->mPort, otThreadErrorToString(error), BufferToString(aBuffer, aLength));
    if (error != OT_ERROR_NONE)
    {
        ++sCounters.mTxFailure;
    }
    return error;
}

static void ReceivePacket(int aSocket, otInstance *aInstance)
{
    struct sockaddr_in6 sockAddr;
    socklen_t           sockAddrLen = sizeof(sockAddr);
    ssize_t             ret;

    memset(&sockAddr, 0, sizeof(sockAddr));

    ret = recvfrom(aSocket, (char *)sRxPacketBuffer, sizeof(sRxPacketBuffer), 0, (struct sockaddr *)&sockAddr,
                   &sockAddrLen);
    VerifyOrDie(ret >= 0, OT_EXIT_ERROR_ERRNO);

    sRxPacketLength = (uint16_t)(ret);

    if (sRxPacketLength > sizeof(sRxPacketBuffer))
    {
        sRxPacketLength = sizeof(sRxPacketLength);
    }

    otLogDebgPlat("[trel] ReceivePacket() - received from [%s]:%d, id:%d, pkt:%s", Ip6AddrToString(&sockAddr.sin6_addr),
                  ntohs(sockAddr.sin6_port), sockAddr.sin6_scope_id, BufferToString(sRxPacketBuffer, sRxPacketLength));

    if (sEnabled)
    {
        ++sCounters.mRxPackets;
        sCounters.mRxBytes += sRxPacketLength;
        otPlatTrelHandleReceived(aInstance, sRxPacketBuffer, sRxPacketLength);
    }
}

static void InitPacketQueue(void)
{
    sTxPacketQueueTail = NULL;

    // Chain all the packets in pool in the free linked list.
    sFreeTxPacketHead = NULL;

    for (uint16_t index = 0; index < OT_ARRAY_LENGTH(sTxPacketPool); index++)
    {
        TxPacket *packet = &sTxPacketPool[index];

        packet->mNext     = sFreeTxPacketHead;
        sFreeTxPacketHead = packet;
    }
}

static void SendQueuedPackets(void)
{
    while (sTxPacketQueueTail != NULL)
    {
        TxPacket *packet = sTxPacketQueueTail->mNext; // tail->mNext is the head of the list.

        if (SendPacket(packet->mBuffer, packet->mLength, &packet->mDestSockAddr) == OT_ERROR_INVALID_STATE)
        {
            otLogDebgPlat("[trel] SendQueuedPackets() - SendPacket() would block");
            break;
        }

        // Remove the `packet` from the packet queue (circular
        // linked list).

        if (packet == sTxPacketQueueTail)
        {
            sTxPacketQueueTail = NULL;
        }
        else
        {
            sTxPacketQueueTail->mNext = packet->mNext;
        }

        // Add the `packet` to the free packet singly linked list.

        packet->mNext     = sFreeTxPacketHead;
        sFreeTxPacketHead = packet;
    }
}

static void EnqueuePacket(const uint8_t *aBuffer, uint16_t aLength, const otSockAddr *aDestSockAddr)
{
    TxPacket *packet;

    // Allocate an available packet entry (from the free packet list)
    // and copy the packet content into it.

    VerifyOrExit(sFreeTxPacketHead != NULL, otLogWarnPlat("[trel] EnqueuePacket failed, queue is full"));
    packet            = sFreeTxPacketHead;
    sFreeTxPacketHead = sFreeTxPacketHead->mNext;

    memcpy(packet->mBuffer, aBuffer, aLength);
    packet->mLength       = aLength;
    packet->mDestSockAddr = *aDestSockAddr;

    // Add packet to the tail of TxPacketQueue circular linked-list.

    if (sTxPacketQueueTail == NULL)
    {
        packet->mNext      = packet;
        sTxPacketQueueTail = packet;
    }
    else
    {
        packet->mNext             = sTxPacketQueueTail->mNext;
        sTxPacketQueueTail->mNext = packet;
        sTxPacketQueueTail        = packet;
    }

    otLogDebgPlat("[trel] EnqueuePacket([%s]:%u) - %s", Ip6AddrToString(&aDestSockAddr->mAddress), aDestSockAddr->mPort,
                  BufferToString(aBuffer, aLength));

exit:
    return;
}

static void ResetCounters() { memset(&sCounters, 0, sizeof(sCounters)); }

//---------------------------------------------------------------------------------------------------------------------
// trelDnssd
//
// The functions below are tied to mDNS or DNS-SD library being used on
// a device and need to be implemented per project/platform. A weak empty
// implementation is provided here which describes the expected
// behavior. They need to be overridden during project/platform
// integration.

OT_TOOL_WEAK void trelDnssdInitialize(const char *aTrelNetif)
{
    // This function initialize the TREL DNS-SD module on the given
    // TREL Network Interface.

    OT_UNUSED_VARIABLE(aTrelNetif);
}

OT_TOOL_WEAK void trelDnssdStartBrowse(void)
{
    // This function initiates an ongoing DNS-SD browse on the service
    // name "_trel._udp" within the local browsing domain to discover
    // other devices supporting TREL. The ongoing browse will produce
    // two different types of events: `add` events and `remove` events.
    // When the browse is started, it should produce an `add` event for
    // every TREL peer currently present on the network. Whenever a
    // TREL peer goes offline, a "remove" event should be produced.
    // `Remove` events are not guaranteed, however. When a TREL service
    // instance is discovered, a new ongoing DNS-SD query for an AAAA
    // record MUST be started on the hostname indicated in the SRV
    // record of the discovered instance. If multiple host IPv6
    // addressees are discovered for a peer, one with highest scope
    // among all addresses MUST be reported (if there are multiple
    // address at same scope, one must be selected randomly).
    //
    // The platform MUST signal back the discovered peer info using
    // `otPlatTrelHandleDiscoveredPeerInfo()` callback. This callback
    // MUST be invoked when a new peer is discovered, or when there is
    // a change in an existing entry (e.g., new TXT record or new port
    // number or new IPv6 address), or when the peer is removed.
}

OT_TOOL_WEAK void trelDnssdStopBrowse(void)
{
    // This function stops the ongoing DNS-SD browse started from an
    // earlier call to `trelDnssdStartBrowse()`.
}

OT_TOOL_WEAK void trelDnssdRegisterService(uint16_t aPort, const uint8_t *aTxtData, uint8_t aTxtLength)
{
    // This function registers a new service to be advertised using
    // DNS-SD.
    //
    // The service name is "_trel._udp". The platform should use its own
    // hostname, which when combined with the service name and the
    // local DNS-SD domain name will produce the full service instance
    // name, for example "example-host._trel._udp.local.".
    //
    // The domain under which the service instance name appears will
    // be 'local' for mDNS, and will be whatever domain is used for
    // service registration in the case of a non-mDNS local DNS-SD
    // service.
    //
    // A subsequent call to this function updates the previous service.
    // It is used to update the TXT record data and/or the port
    // number.
    //
    // The `aTxtData` buffer is not persisted after the return from this
    // function. The platform layer MUST not keep the pointer and
    // instead copy the content if needed.

    OT_UNUSED_VARIABLE(aPort);
    OT_UNUSED_VARIABLE(aTxtData);
    OT_UNUSED_VARIABLE(aTxtLength);
}

OT_TOOL_WEAK void trelDnssdRemoveService(void)
{
    // This function removes any previously registered "_trel._udp"
    // service using `platTrelRegisterService()`. Device must stop
    // advertising TREL service after this call.
}

OT_TOOL_WEAK void trelDnssdUpdateFdSet(otSysMainloopContext *aContext)
{
    // This function can be used to update the file descriptor sets
    // by DNS-SD layer (if needed).

    OT_UNUSED_VARIABLE(aContext);
}

OT_TOOL_WEAK void trelDnssdProcess(otInstance *aInstance, const otSysMainloopContext *aContext)
{
    // This function performs processing by DNS-SD (if needed).

    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aContext);
}

//---------------------------------------------------------------------------------------------------------------------
// otPlatTrel

void otPlatTrelEnable(otInstance *aInstance, uint16_t *aUdpPort)
{
    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(!IsSystemDryRun());

    assert(sInitialized);

    VerifyOrExit(!sEnabled);

    PrepareSocket(*aUdpPort);
    trelDnssdStartBrowse();

    sEnabled = true;

exit:
    return;
}

void otPlatTrelDisable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(!IsSystemDryRun());

    assert(sInitialized);
    VerifyOrExit(sEnabled);

    close(sSocket);
    sSocket = -1;
    trelDnssdStopBrowse();
    trelDnssdRemoveService();
    sEnabled = false;

exit:
    return;
}

void otPlatTrelSend(otInstance       *aInstance,
                    const uint8_t    *aUdpPayload,
                    uint16_t          aUdpPayloadLen,
                    const otSockAddr *aDestSockAddr)
{
    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(!IsSystemDryRun());

    VerifyOrExit(sEnabled);

    assert(aUdpPayloadLen <= kMaxPacketSize);

    // We try to send the packet immediately. If it fails (e.g.,
    // network is down) `SendPacket()` returns `OT_ERROR_ABORT`. If
    // the send operation would block (e.g., socket is not yet ready
    // or is out of buffer) we get `OT_ERROR_INVALID_STATE`. In that
    // case we enqueue the packet to send it later when socket becomes
    // ready.

    if ((sTxPacketQueueTail != NULL) ||
        (SendPacket(aUdpPayload, aUdpPayloadLen, aDestSockAddr) == OT_ERROR_INVALID_STATE))
    {
        EnqueuePacket(aUdpPayload, aUdpPayloadLen, aDestSockAddr);
    }

exit:
    return;
}

void otPlatTrelRegisterService(otInstance *aInstance, uint16_t aPort, const uint8_t *aTxtData, uint8_t aTxtLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    VerifyOrExit(!IsSystemDryRun());

    trelDnssdRegisterService(aPort, aTxtData, aTxtLength);

exit:
    return;
}

// We keep counters at the platform layer because TREL failures can only be captured accurately within
// the platform layer as the platform sometimes only queues the packet and the packet will be sent later
// and the error is only known after sent.
const otPlatTrelCounters *otPlatTrelGetCounters(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return &sCounters;
}

void otPlatTrelResetCounters(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    ResetCounters();
}

//---------------------------------------------------------------------------------------------------------------------
// platformTrel system

void platformTrelInit(const char *aTrelUrl)
{
    otLogDebgPlat("[trel] platformTrelInit(aTrelUrl:\"%s\")", aTrelUrl != nullptr ? aTrelUrl : "");

    assert(!sInitialized);

    if (aTrelUrl != nullptr)
    {
        ot::Posix::RadioUrl url(aTrelUrl);

        strncpy(sInterfaceName, url.GetPath(), sizeof(sInterfaceName) - 1);
        sInterfaceName[sizeof(sInterfaceName) - 1] = '\0';
    }

    trelDnssdInitialize(sInterfaceName);

    InitPacketQueue();
    sInitialized = true;

    ResetCounters();
}

void platformTrelDeinit(void)
{
    VerifyOrExit(sInitialized);

    otPlatTrelDisable(nullptr);
    sInterfaceName[0] = '\0';
    sInitialized      = false;
    otLogDebgPlat("[trel] platformTrelDeinit()");

exit:
    return;
}

void platformTrelUpdateFdSet(otSysMainloopContext *aContext)
{
    assert(aContext != nullptr);

    VerifyOrExit(sEnabled);

    FD_SET(sSocket, &aContext->mReadFdSet);

    if (sTxPacketQueueTail != nullptr)
    {
        FD_SET(sSocket, &aContext->mWriteFdSet);
    }

    if (aContext->mMaxFd < sSocket)
    {
        aContext->mMaxFd = sSocket;
    }

    trelDnssdUpdateFdSet(aContext);

exit:
    return;
}

void platformTrelProcess(otInstance *aInstance, const otSysMainloopContext *aContext)
{
    VerifyOrExit(sEnabled);

    if (FD_ISSET(sSocket, &aContext->mWriteFdSet))
    {
        SendQueuedPackets();
    }

    if (FD_ISSET(sSocket, &aContext->mReadFdSet))
    {
        ReceivePacket(sSocket, aInstance);
    }

    trelDnssdProcess(aInstance, aContext);

exit:
    return;
}

#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
