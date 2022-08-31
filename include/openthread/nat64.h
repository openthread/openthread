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
 * Represents the counters for NAT64.
 *
 */
typedef struct otNat64Counters
{
    uint64_t m4To6Packets; ///< Number of packets translated from IPv4 to IPv6.
    uint64_t m4To6Bytes;   ///< Sum of size of packets translated from IPv4 to IPv6.
    uint64_t m6To4Packets; ///< Number of packets translated from IPv6 to IPv4.
    uint64_t m6To4Bytes;   ///< Sum of size of packets translated from IPv6 to IPv4.
} otNat64Counters;

/**
 * Represents the counters for the protocols supported by NAT64.
 *
 */
typedef struct otNat64ProtocolCounters
{
    otNat64Counters mTotal; ///< Counters for sum of all protocols.
    otNat64Counters mIcmp;  ///< Counters for ICMP and ICMPv6.
    otNat64Counters mUdp;   ///< Counters for UDP.
    otNat64Counters mTcp;   ///< Counters for TCP.
} otNat64ProtocolCounters;

/**
 * Packet drop reasons.
 *
 */
typedef enum otNat64DropReason
{
    OT_NAT64_DROP_REASON_UNKNOWN = 0,       ///< Packet drop for unknown reasons.
    OT_NAT64_DROP_REASON_ILLEGAL_PACKET,    ///< Packet drop due to failed to parse the datagram.
    OT_NAT64_DROP_REASON_UNSUPPORTED_PROTO, ///< Packet drop due to unsupported IP protocol.
    OT_NAT64_DROP_REASON_NO_MAPPING,        ///< Packet drop due to no mappings found or mapping pool exhausted.
    //---
    OT_NAT64_DROP_REASON_COUNT,
} otNat64DropReason;

/**
 * Represents the counters of dropped packets due to errors when handling NAT64 packets.
 *
 */
typedef struct otNat64ErrorCounters
{
    uint64_t mCount4To6[OT_NAT64_DROP_REASON_COUNT]; ///< Errors translating IPv4 packets.
    uint64_t mCount6To4[OT_NAT64_DROP_REASON_COUNT]; ///< Errors translating IPv6 packets.
} otNat64ErrorCounters;

/**
 * Gets NAT64 translator counters.
 *
 * The counter is counted since the instance initialized.
 *
 * Available when `OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[out] aCounters A pointer to an `otNat64Counters` where the counters of NAT64 translator will be placed.
 *
 */
void otNat64GetCounters(otInstance *aInstance, otNat64ProtocolCounters *aCounters);

/**
 * Gets the NAT64 translator error counters.
 *
 * The counters are initialized to zero when the OpenThread instance is initialized.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[out] aCounters A pointer to an `otNat64Counters` where the counters of NAT64 translator will be placed.
 *
 */
void otNat64GetErrorCounters(otInstance *aInstance, otNat64ErrorCounters *aCounters);

/**
 * Represents an address mapping record for NAT64.
 *
 * @note The counters will be reset for each mapping session even for the same address pair. Applications can use `mId`
 * to identify different sessions to calculate the packets correctly.
 *
 */
typedef struct otNat64AddressMapping
{
    uint64_t mId; ///< The unique id for a mapping session.

    otIp4Address mIp4;             ///< The IPv4 address of the mapping.
    otIp6Address mIp6;             ///< The IPv6 address of the mapping.
    uint32_t     mRemainingTimeMs; ///< Remaining time before expiry in milliseconds.

    otNat64ProtocolCounters mCounters;
} otNat64AddressMapping;

/**
 * Used to iterate through NAT64 address mappings.
 *
 * The fields in this type are opaque (intended for use by OpenThread core only) and therefore should not be
 * accessed or used by caller.
 *
 * Before using an iterator, it MUST be initialized using `otNat64AddressMappingIteratorInit()`.
 *
 */
typedef struct otNat64AddressMappingIterator
{
    void *mPtr;
} otNat64AddressMappingIterator;

/**
 * Initializes an `otNat64AddressMappingIterator`.
 *
 * An iterator MUST be initialized before it is used.
 *
 * An iterator can be initialized again to restart from the beginning of the mapping info.
 *
 * @param[in]  aInstance  The OpenThread instance.
 * @param[out] aIterator  A pointer to the iterator to initialize.
 *
 */
void otNat64InitAddressMappingIterator(otInstance *aInstance, otNat64AddressMappingIterator *aIterator);

/**
 * Gets the next AddressMapping info (using an iterator).
 *
 * Available when `OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is enabled.
 *
 * @param[in]      aInstance      A pointer to an OpenThread instance.
 * @param[in,out]  aIterator      A pointer to the iterator. On success the iterator will be updated to point to next
 *                                NAT64 address mapping record. To get the first entry the iterator should be set to
 *                                OT_NAT64_ADDRESS_MAPPING_ITERATOR_INIT.
 * @param[out]     aMapping       A pointer to an `otNat64AddressMapping` where information of next NAT64 address
 *                                mapping record is placed (on success).
 *
 * @retval OT_ERROR_NONE       Successfully found the next NAT64 address mapping info (@p aMapping was successfully
 *                             updated).
 * @retval OT_ERROR_NOT_FOUND  No subsequent NAT64 address mapping info was found.
 *
 */
otError otNat64GetNextAddressMapping(otInstance *                   aInstance,
                                     otNat64AddressMappingIterator *aIterator,
                                     otNat64AddressMapping *        aMapping);

/**
 * Allocate a new message buffer for sending an IPv4 message to the NAT64 translator.
 *
 * Message buffers allocated by this function will have 20 bytes (difference between the size of IPv6 headers
 * and IPv4 header sizes) reserved.
 *
 * Available when `OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is enabled.
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
 * Gets the configured IPv4 CIDR in the NAT64 translator.
 *
 * Available when `OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is enabled.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[out] aCidr             A pointer to an otIp4Cidr. Where the configured CIDR will be filled.
 *
 */
otError otNat64GetConfiguredCidr(otInstance *aInstance, otIp4Cidr *aCidr);

/**
 * Gets the configured IPv6 prefix in the NAT64 translator.
 *
 * Available when `OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is enabled.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[out] aPrefix           A pointer to an otIp6Prefix. Where the configured NAT64 prefix will be filled.
 *
 */
otError otNat64GetConfiguredPrefix(otInstance *aInstance, otIp6Prefix *aPrefix);

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

#define OT_IP4_ADDRESS_STRING_SIZE 17 ///< Length of 000.000.000.000 plus a suffix NUL

/**
 * Converts the address to a string.
 *
 * The string format uses quad-dotted notation of four bytes in the address (e.g., "127.0.0.1").
 *
 * If the resulting string does not fit in @p aBuffer (within its @p aSize characters), the string will be
 * truncated but the outputted string is always null-terminated.
 *
 * @param[in]  aAddress  A pointer to an IPv4 address (MUST NOT be NULL).
 * @param[out] aBuffer   A pointer to a char array to output the string (MUST NOT be `nullptr`).
 * @param[in]  aSize     The size of @p aBuffer (in bytes).
 *
 */
void otIp4AddressToString(const otIp4Address *aAddress, char *aBuffer, uint16_t aSize);

#define OT_IP4_CIDR_STRING_SIZE 20 ///< Length of 000.000.000.000/00 plus a suffix NUL

/**
 * Converts the IPv4 CIDR to a string.
 *
 * The string format uses quad-dotted notation of four bytes in the address with the length of prefix (e.g.,
 * "127.0.0.1/32").
 *
 * If the resulting string does not fit in @p aBuffer (within its @p aSize characters), the string will be
 * truncated but the outputted string is always null-terminated.
 *
 * @param[in]  aCidr     A pointer to an IPv4 CIDR (MUST NOT be NULL).
 * @param[out] aBuffer   A pointer to a char array to output the string (MUST NOT be `nullptr`).
 * @param[in]  aSize     The size of @p aBuffer (in bytes).
 *
 */
void otIp4CidrToString(const otIp4Cidr *aCidr, char *aBuffer, uint16_t aSize);

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif

#endif // OPENTHREAD_NAT64_H_
