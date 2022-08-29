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

#include <openthread/ip6.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-nat64
 *
 * @brief This module includes functions and structs for the NAT64 function on the border router. These functions are
 * only available when `OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE` is enabled.
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

/**
 * Allocate a new message buffer for sending an IPv4 message to the NAT64 translator.
 *
 * Message buffers allocated by this function will have 20 bytes (difference between the size of IPv6 headers
 * and IPv4 header sizes) reserved.
 *
 * This function is available only when `OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is enabled.
 *
 * @note If @p aSettings is `NULL`, the link layer security is enabled and the message priority is set to
 * OT_MESSAGE_PRIORITY_NORMAL by default.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aSettings  A pointer to the message settings or NULL to set default settings.
 *
 * @returns A pointer to the message buffer or NULL if no message buffers are available or parameters are invalid.
 *
 * @sa otNat64Send
 *
 */
otMessage *otIp4NewMessage(otInstance *aInstance, const otMessageSettings *aSettings);

/**
 * Sets the CIDR used when setting the source address of the outgoing translated IPv4 packets.
 *
 * This function is available only when OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE is enabled.
 *
 * @note A valid CIDR must have a non-zero prefix length. The actual addresses pool is limited by the size of the
 * mapping pool and the number of addresses available in the CIDR block.
 *
 * @note This function can be called at any time, but the NAT64 translator will be reset and all existing sessions will
 * be expired when updating the configured CIDR.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 * @param[in] aCidr      A pointer to an otIp4Cidr for the IPv4 CIDR block for NAT64.
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
 * Translates an IPv4 datagram to an IPv6 datagram and sends via the Thread interface.
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
otError otNat64Send(otInstance *aInstance, otMessage *aMessage);

/**
 * This function pointer is called when an IPv4 datagram (translated by NAT64 translator) is received.
 *
 * @param[in]  aMessage  A pointer to the message buffer containing the received IPv6 datagram. This function transfers
 *                       the ownership of the @p aMessage to the receiver of the callback. The message should be
 *                       freed by the receiver of the callback after it is processed.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otNat64ReceiveIp4Callback)(otMessage *aMessage, void *aContext);

/**
 * Registers a callback to provide received IPv4 datagrams.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aCallback         A pointer to a function that is called when an IPv4 datagram is received or
 *                               NULL to disable the callback.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 */
void otNat64SetReceiveIp4Callback(otInstance *aInstance, otNat64ReceiveIp4Callback aCallback, void *aContext);

/**
 * Test if two IPv4 addresses are the same.
 *
 * @param[in]  aFirst   A pointer to the first IPv4 address to compare.
 * @param[in]  aSecond  A pointer to the second IPv4 address to compare.
 *
 * @retval TRUE   The two IPv4 addresses are the same.
 * @retval FALSE  The two IPv4 addresses are not the same.
 *
 */
bool otIp4IsAddressEqual(const otIp4Address *aFirst, const otIp4Address *aSecond);

/**
 * Set @p aIp4Address by performing NAT64 address translation from @p aIp6Address as specified
 * in RFC 6052.
 *
 * The NAT64 @p aPrefixLength MUST be one of the following values: 32, 40, 48, 56, 64, or 96, otherwise the behavior
 * of this method is undefined.
 *
 * @param[in]  aPrefixLength  The prefix length to use for IPv4/IPv6 translation.
 * @param[in]  aIp6Address    A pointer to an IPv6 address.
 * @param[out] aIp4Address    A pointer to output the IPv4 address.
 *
 */
void otIp4ExtractFromIp6Address(uint8_t aPrefixLength, const otIp6Address *aIp6Address, otIp4Address *aIp4Address);

/**
 * This function converts a human-readable IPv4 address string into a binary representation.
 *
 * @param[in]   aString   A pointer to a NULL-terminated string.
 * @param[out]  aAddress  A pointer to an IPv4 address.
 *
 * @retval OT_ERROR_NONE          Successfully parsed the string.
 * @retval OT_ERROR_INVALID_ARGS  Failed to parse the string.
 *
 */
otError otIp4AddressFromString(const char *aString, otIp4Address *aAddress);

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif

#endif // OPENTHREAD_NAT64_H_
