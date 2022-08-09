/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 * @brief
 *  This file defines the OpenThread API for NAT64 on a border router.
 */

#ifndef OPENTHREAD_NAT64_H_
#define OPENTHREAD_NAT64_H_

#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-nat64
 *
 * @brief This module includes functions and structs for the NAT64 function on the border router. These functions are
 * only available when `OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE` is enabled.
 *
 * @{
 *
 */

#define OT_IP4_ADDRESS_SIZE 4 ///< Size of an IPv4 address (bytes)

/**
 * @struct otIp4Address
 *
 * This structure represents an IPv4 address.
 *
 */
OT_TOOL_PACKED_BEGIN
struct otIp4Address
{
    union OT_TOOL_PACKED_FIELD
    {
        uint8_t  m8[OT_IP4_ADDRESS_SIZE]; ///< 8-bit fields
        uint32_t m32;                     ///< 32-bit representation
    } mFields;
} OT_TOOL_PACKED_END;

/**
 * This structure represents an IPv4 address.
 *
 */
typedef struct otIp4Address otIp4Address;

/**
 * @struct otIp4Cidr
 *
 * This structure represents an IPv4 CIDR block.
 *
 */
typedef struct otIp4Cidr
{
    otIp4Address mAddress;
    uint8_t      mLength;
} otIp4Cidr;

typedef struct otIp4Message otIp4Message;

/**
 * Allocate a new message buffer for sending an IPv4 message (which will be translated into an IPv6 packet by NAT64
 * later). Message buffers allocated by this function will have 20 bytes (The differences between the size of IPv6
 * headers and the size of IPv4 headers) reserved.
 *
 * This function is available only when OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE is enabled.
 *
 * @note If @p aSettings is 'NULL', the link layer security is enabled and the message priority is set to
 * OT_MESSAGE_PRIORITY_NORMAL by default.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aSettings  A pointer to the message settings or NULL to set default settings.
 *
 * @returns A pointer to the message buffer or NULL if no message buffers are available or parameters are invalid.
 *
 * @sa otIp4MessageFree
 *
 */
otIp4Message *otIp4NewMessage(otInstance *aInstance, const otMessageSettings *aSettings);

/**
 * Free an allocated IPv4 message buffer.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 */
void otIp4MessageFree(otIp4Message *aMessage);

/**
 * Casts an otIp4Message instance to otMessage instance. For using functions like otMessageRead etc.
 *
 * @sa otIp4Message
 *
 */
otMessage *otCastIp4Message(otIp4Message *aMessage);

/**
 * This function sets the CIDR used when setting the source address of the outgoing translated IPv4 packets. A valid
 * CIDR must have a non-zero prefix length.
 *
 * @note The actual addresses pool is limited by the size of the mapping pool and the number of addresses available in
 * the CIDR block. If the provided is a valid IPv4 CIDR for NAT64, and it is different from the one already configured,
 * the NAT64 translator will be reset and all existing sessions will be expired.
 *
 * This function is available only when OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE is enabled.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aCidr A pointer to an otIp4Cidr for the IPv4 CIDR block for NAT64.
 *
 * @retval  OT_ERROR_INVALID_ARGS   The given CIDR is not a valid IPv4 CIDR for NAT64.
 * @retval  OT_ERROR_NONE           Successfully set the CIDR for NAT64.
 *
 * @sa otBorderRouterSend
 * @sa otBorderRouterSetReceiveCallback
 *
 */
otError otNat64SetIp4Cidr(otInstance *aInstance, const otIp4Cidr *aCidr);

/**
 * This function translates an IPv4 datagram to IPv6 datagram and send via the Thread interface.
 *
 * The caller transfers ownership of @p aMessage when making this call. OpenThread will free @p aMessage when
 * processing is complete, including when a value other than `OT_ERROR_NONE` is returned.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aMessage  A pointer to the message buffer containing the IPv4 datagram.
 *
 * @retval OT_ERROR_NONE                    Successfully processed the message.
 * @retval OT_ERROR_DROP                    Message was well-formed but not fully processed due to packet processing
 *                                          rules.
 * @retval OT_ERROR_NO_BUFS                 Could not allocate necessary message buffers when processing the datagram.
 * @retval OT_ERROR_NO_ROUTE                No route to host.
 * @retval OT_ERROR_INVALID_SOURCE_ADDRESS  Source address is invalid, e.g. an anycast address or a multicast address.
 * @retval OT_ERROR_PARSE                   Encountered a malformed header when processing the message.
 *
 */
otError otNat64Send(otInstance *aInstance, otIp4Message *aMessage);

/**
 * This function pointer is called when an IPv4 datagram (translated by NAT64 translator) is received.
 *
 * @param[in]  aMessage  A pointer to the message buffer containing the received IPv6 datagram. This function transfers
 *                       the ownership of the @p aMessage to the receiver of the callback. The message should be
 *                       freed by the receiver of the callback after it is processed (see otIp4MessageFree()).
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otNat64ReceiveIp4Callback)(otIp4Message *aMessage, void *aContext);

/**
 * This function registers a callback to provide received IPv4 datagrams.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aCallback         A pointer to a function that is called when an IPv4 datagram is received or
 *                               NULL to disable the callback.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 */
void otNat64SetReceiveIp4Callback(otInstance *aInstance, otNat64ReceiveIp4Callback aCallback, void *aContext);

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif

#endif // OPENTHREAD_NAT64_H_
