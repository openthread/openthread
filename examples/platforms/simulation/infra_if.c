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

#include "platform-simulation.h"

#include <openthread/icmp6.h>
#include <openthread/ip6.h>
#include <openthread/logging.h>
#include <openthread/platform/infra_if.h>

#include "simul_utils.h"
#include "utils/code_utils.h"

#if OPENTHREAD_SIMULATION_IMPLEMENT_INFRA_IF && OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#define DEBUG_LOG 0

#if DEBUG_LOG
#define LOG(...) otLogNotePlat("[infra-if] "__VA_ARGS__)
#else
#define LOG(...) \
    do           \
    {            \
    } while (0)
#endif

#define INFRA_IF_SIM_PORT 9800
#define INFRA_IF_MAX_PACKET_SIZE 1800
#define INFRA_IF_MAX_PENDING_TX 64
#define INFRA_IF_NEIGHBOR_ADVERT_SIZE 24

typedef struct Message
{
    uint32_t     mIfIndex;
    otIp6Address mSrc;
    otIp6Address mDst;
    uint16_t     mDataLength;
    uint8_t      mData[INFRA_IF_MAX_PACKET_SIZE];
} Message;

static bool         sInitialized = false;
static otIp6Address sIp6Address;
static otIp6Address sLinkLocalAllNodes;
static otIp6Address sLinkLocalAllRouters;
static utilsSocket  sSocket;
static uint16_t     sPortOffset   = 0;
static uint8_t      sNumPendingTx = 0;
static Message      sPendingTx[INFRA_IF_MAX_PENDING_TX];

//---------------------------------------------------------------------------------------------------------------------

static bool addressesMatch(const otIp6Address *aFirstAddr, const otIp6Address *aSecondAddr)
{
    return memcmp(aFirstAddr, aSecondAddr, sizeof(otIp6Address)) == 0;
}

static uint16_t getMessageSize(const Message *aMessage)
{
    return (uint16_t)(&aMessage->mData[aMessage->mDataLength] - (const uint8_t *)aMessage);
}

static void sendPendingTxMessages(void)
{
    for (uint8_t i = 0; i < sNumPendingTx; i++)
    {
        utilsSendOverSocket(&sSocket, &sPendingTx[i], getMessageSize(&sPendingTx[i]));
    }

    sNumPendingTx = 0;
}

static void sendNeighborAdvert(const Message *aNsMessage)
{
    Message *message;
    uint8_t  index;

    assert(sNumPendingTx < INFRA_IF_MAX_PENDING_TX);

    message = &sPendingTx[sNumPendingTx++];

    message->mIfIndex = aNsMessage->mIfIndex;
    message->mSrc     = sIp6Address;
    message->mDst     = aNsMessage->mSrc;

    // Neighbor Advertisement Message (RFC 4861)
    //
    //   0                   1                   2                   3
    //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |     Type      |     Code      |          Checksum             |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |R|S|O|                     Reserved                            |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |                                                               |
    //  +                                                               +
    //  |                                                               |
    //  +                       Target Address                          +
    //  |                                                               |
    //  +                                                               +
    //  |                                                               |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    index = 0;
    memset(message->mData, 0, INFRA_IF_NEIGHBOR_ADVERT_SIZE);

    message->mData[index++] = OT_ICMP6_TYPE_NEIGHBOR_ADVERT;           // Type.
    index += 3;                                                        // Code is zero. Checksum (uint16) as zero.
    message->mData[index++] = 0xd0;                                    // Flags, set R and S bits.
    index += 3;                                                        // Skip over the reserved bytes.
    memcpy(&message->mData[index], &sIp6Address, sizeof(sIp6Address)); // Set the target address field.
    index += sizeof(sIp6Address);

    assert(index == INFRA_IF_NEIGHBOR_ADVERT_SIZE);

    message->mDataLength = INFRA_IF_NEIGHBOR_ADVERT_SIZE;
}

static void processMessage(otInstance *aInstance, Message *aMessage, uint16_t aLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    otEXPECT(aLength > 0);
    otEXPECT(getMessageSize(aMessage) == aLength);
    otEXPECT(aMessage->mDataLength > 0);

    // Validate the dest address.
    otEXPECT(addressesMatch(&aMessage->mDst, &sIp6Address) || addressesMatch(&aMessage->mDst, &sLinkLocalAllNodes) ||
             addressesMatch(&aMessage->mDst, &sLinkLocalAllRouters));

    if (aMessage->mData[0] == OT_ICMP6_TYPE_NEIGHBOR_SOLICIT)
    {
        LOG("Received NS, responding with NA");
        sendNeighborAdvert(aMessage);
    }
    else
    {
        LOG("Received msg, len:%u", aMessage->mDataLength);
        otPlatInfraIfRecvIcmp6Nd(aInstance, aMessage->mIfIndex, &aMessage->mSrc, aMessage->mData,
                                 aMessage->mDataLength);
    }

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// otPlatInfraIf

bool otPlatInfraIfHasAddress(uint32_t aInfraIfIndex, const otIp6Address *aAddress)
{
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    return addressesMatch(aAddress, &sIp6Address);
}

otError otPlatInfraIfSendIcmp6Nd(uint32_t            aInfraIfIndex,
                                 const otIp6Address *aDestAddress,
                                 const uint8_t      *aBuffer,
                                 uint16_t            aBufferLength)
{
    otError  error = OT_ERROR_FAILED;
    Message *message;

    otEXPECT(sInitialized);
    otEXPECT(sNumPendingTx < INFRA_IF_MAX_PENDING_TX);

    message = &sPendingTx[sNumPendingTx++];

    message->mIfIndex = aInfraIfIndex;
    message->mSrc     = sIp6Address;
    message->mDst     = *aDestAddress;

    assert(aBufferLength <= INFRA_IF_MAX_PACKET_SIZE);
    message->mDataLength = aBufferLength;
    memcpy(message->mData, aBuffer, aBufferLength);
    error = OT_ERROR_NONE;

    LOG("otPlatInfraIfSendIcmp6Nd() msg-len:%u", aBufferLength);

exit:
    return error;
}

otError otPlatInfraIfDiscoverNat64Prefix(uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    return OT_ERROR_NONE;
}

//---------------------------------------------------------------------------------------------------------------------
// platformInfraIf

void platformInfraIfInit(void)
{
    char *str;

    otEXPECT(!sInitialized);

    sInitialized = true;

    memset(&sIp6Address, 0, sizeof(sIp6Address));
    sIp6Address.mFields.m8[0]  = 0xfe;
    sIp6Address.mFields.m8[1]  = 0x80;
    sIp6Address.mFields.m8[15] = (uint8_t)(gNodeId & 0xff);

    // "ff02::01"
    memset(&sLinkLocalAllNodes, 0, sizeof(sLinkLocalAllNodes));
    sLinkLocalAllNodes.mFields.m8[0]  = 0xff;
    sLinkLocalAllNodes.mFields.m8[1]  = 0x02;
    sLinkLocalAllNodes.mFields.m8[15] = 0x01;

    // "ff02::02"
    memset(&sLinkLocalAllRouters, 0, sizeof(sLinkLocalAllRouters));
    sLinkLocalAllRouters.mFields.m8[0]  = 0xff;
    sLinkLocalAllRouters.mFields.m8[1]  = 0x02;
    sLinkLocalAllRouters.mFields.m8[15] = 0x02;

    str = getenv("PORT_OFFSET");

    if (str != NULL)
    {
        char *endptr;

        sPortOffset = (uint16_t)strtol(str, &endptr, 0);

        if (*endptr != '\0')
        {
            fprintf(stderr, "\r\nInvalid PORT_OFFSET: %s\r\n", str);
            exit(EXIT_FAILURE);
        }

        sPortOffset *= (MAX_NETWORK_SIZE + 1);
    }

    utilsInitSocket(&sSocket, INFRA_IF_SIM_PORT + sPortOffset);

exit:
    return;
}

void platformInfraIfDeinit(void)
{
    otEXPECT(sInitialized);
    sInitialized = false;
    utilsDeinitSocket(&sSocket);

exit:
    return;
}

void platformInfraIfUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd)
{
    otEXPECT(sInitialized);

    utilsAddSocketRxFd(&sSocket, aReadFdSet, aMaxFd);

    if (sNumPendingTx > 0)
    {
        utilsAddSocketTxFd(&sSocket, aWriteFdSet, aMaxFd);
    }

exit:
    return;
}

void platformInfraIfProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet)
{
    OT_UNUSED_VARIABLE(aInstance);

    otEXPECT(sInitialized);

    if ((sNumPendingTx > 0) && utilsCanSocketSend(&sSocket, aWriteFdSet))
    {
        sendPendingTxMessages();
    }

    if (utilsCanSocketReceive(&sSocket, aReadFdSet))
    {
        Message  message;
        uint16_t len;

        message.mDataLength = 0;

        len = utilsReceiveFromSocket(&sSocket, &message, sizeof(message), NULL);
        processMessage(aInstance, &message, len);
    }

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// Provide weak implementation (used for RCP builds).
// `OPENTHREAD_RADIO` is not available in simulation platform

OT_TOOL_WEAK void otPlatInfraIfRecvIcmp6Nd(otInstance         *aInstance,
                                           uint32_t            aInfraIfIndex,
                                           const otIp6Address *aSrcAddress,
                                           const uint8_t      *aBuffer,
                                           uint16_t            aBufferLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aInfraIfIndex);
    OT_UNUSED_VARIABLE(aSrcAddress);
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aBufferLength);

    fprintf(stderr, "\n\r Weak otPlatInfraIfRecvIcmp6Nd is being used\n\r");
    exit(1);
}

#endif // OPENTHREAD_SIMULATION_IMPLEMENT_INFRA_IF && OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
