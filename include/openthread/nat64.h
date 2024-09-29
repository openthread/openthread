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
 */

#define OT_IP4_ADDRESS_SIZE 4 ///< Size of an IPv4 address (bytes)

/**
 * @struct otIp4Address
 *
 * Represents an IPv4 address.
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
 * Represents an IPv4 address.
 */
typedef struct otIp4Address otIp4Address;

/**
 * @struct otIp4Cidr
 *
 * Represents an IPv4 CIDR block.
 */
typedef struct otIp4Cidr
{
    otIp4Address mAddress;
    uint8_t      mLength;
} otIp4Cidr;

/**
 * Represents the counters for NAT64.
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
 */
void otNat64GetCounters(otInstance *aInstance, otNat64ProtocolCounters *aCounters);

/**
 * Gets the NAT64 translator error counters.
 *
 * The counters are initialized to zero when the OpenThread instance is initialized.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[out] aCounters A pointer to an `otNat64Counters` where the counters of NAT64 translator will be placed.
 */
void otNat64GetErrorCounters(otInstance *aInstance, otNat64ErrorCounters *aCounters);

/**
 * Represents an address mapping record for NAT64.
 *
 * @note The counters will be reset for each mapping session even for the same address pair. Applications can use `mId`
 * to identify different sessions to calculate the packets correctly.
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
 */
otError otNat64GetNextAddressMapping(otInstance                    *aInstance,
                                     otNat64AddressMappingIterator *aIterator,
                                     otNat64AddressMapping         *aMapping);

/**
 * States of NAT64.
 */
typedef enum
{
    OT_NAT64_STATE_DISABLED = 0, ///< NAT64 is disabled.
    OT_NAT64_STATE_NOT_RUNNING,  ///< NAT64 is enabled, but one or more dependencies of NAT64 are not running.
    OT_NAT64_STATE_IDLE,         ///< NAT64 is enabled, but this BR is not an active NAT64 BR.
    OT_NAT64_STATE_ACTIVE,       ///< The BR is publishing a NAT64 prefix and/or translating packets.
} otNat64State;

/**
 * Gets the state of NAT64 translator.
 *
 * Available when `OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is enabled.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 *
 * @retval OT_NAT64_STATE_DISABLED    NAT64 translator is disabled.
 * @retval OT_NAT64_STATE_NOT_RUNNING NAT64 translator is enabled, but the translator is not configured with a valid
 *                                    NAT64 prefix and a CIDR.
 * @retval OT_NAT64_STATE_ACTIVE      NAT64 translator is enabled, and is translating packets.
 */
otNat64State otNat64GetTranslatorState(otInstance *aInstance);

/**
 * Gets the state of NAT64 prefix manager.
 *
 * Available when `OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE` is enabled.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 *
 * @retval OT_NAT64_STATE_DISABLED    NAT64 prefix manager is disabled.
 * @retval OT_NAT64_STATE_NOT_RUNNING NAT64 prefix manager is enabled, but is not running (because the routing manager
 *                                    is not running).
 * @retval OT_NAT64_STATE_IDLE        NAT64 prefix manager is enabled, but is not publishing a NAT64 prefix. Usually
 *                                    when there is another border router publishing a NAT64 prefix with higher
 *                                    priority.
 * @retval OT_NAT64_STATE_ACTIVE      NAT64 prefix manager is enabled, and is publishing NAT64 prefix to the Thread
 *                                    network.
 */
otNat64State otNat64GetPrefixManagerState(otInstance *aInstance);

/**
 * Enable or disable NAT64 functions.
 *
 * Note: This includes the NAT64 Translator (when `OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is enabled) and the NAT64
 * Prefix Manager (when `OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE` is enabled).
 *
 * When `OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is enabled, setting disabled to true resets the
 * mapping table in the translator.
 *
 * Available when `OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` or `OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE` is
 * enabled.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aEnabled   A boolean to enable/disable the NAT64 functions
 *
 * @sa otNat64GetTranslatorState
 * @sa otNat64GetPrefixManagerState
 */
void otNat64SetEnabled(otInstance *aInstance, bool aEnabled);

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
 */
otMessage *otIp4NewMessage(otInstance *aInstance, const otMessageSettings *aSettings);

/**
 * Sets the CIDR used when setting the source address of the outgoing translated IPv4 packets.
 *
 * Is available only when OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE is enabled.
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
 */
otError otNat64Send(otInstance *aInstance, otMessage *aMessage);

/**
 * Pointer is called when an IPv4 datagram (translated by NAT64 translator) is received.
 *
 * @param[in]  aMessage  A pointer to the message buffer containing the received IPv6 datagram. This function transfers
 *                       the ownership of the @p aMessage to the receiver of the callback. The message should be
 *                       freed by the receiver of the callback after it is processed.
 * @param[in]  aContext  A pointer to application-specific context.
 */
typedef void (*otNat64ReceiveIp4Callback)(otMessage *aMessage, void *aContext);

/**
 * Registers a callback to provide received IPv4 datagrams.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aCallback         A pointer to a function that is called when an IPv4 datagram is received or
 *                               NULL to disable the callback.
 * @param[in]  aContext          A pointer to application-specific context.
 */
void otNat64SetReceiveIp4Callback(otInstance *aInstance, otNat64ReceiveIp4Callback aCallback, void *aContext);

/**
 * Gets the IPv4 CIDR configured in the NAT64 translator.
 *
 * Available when `OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is enabled.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[out] aCidr             A pointer to an otIp4Cidr. Where the CIDR will be filled.
 */
otError otNat64GetCidr(otInstance *aInstance, otIp4Cidr *aCidr);

/**
 * Test if two IPv4 addresses are the same.
 *
 * @param[in]  aFirst   A pointer to the first IPv4 address to compare.
 * @param[in]  aSecond  A pointer to the second IPv4 address to compare.
 *
 * @retval TRUE   The two IPv4 addresses are the same.
 * @retval FALSE  The two IPv4 addresses are not the same.
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
 */
void otIp4ExtractFromIp6Address(uint8_t aPrefixLength, const otIp6Address *aIp6Address, otIp4Address *aIp4Address);

/**
 * Extracts the IPv4 address from a given IPv4-mapped IPv6 address.
 *
 * An IPv4-mapped IPv6 address consists of an 80-bit prefix of zeros, the next 16 bits set to ones, and the remaining,
 * least-significant 32 bits contain the IPv4 address, e.g., `::ffff:192.0.2.128` representing `192.0.2.128`.
 *
 * @param[in]  aIp6Address  An IPv6 address to extract IPv4 from.
 * @param[out] aIp4Address  An IPv4 address to output the extracted address.
 *
 * @retval OT_ERROR_NONE   Extracted the IPv4 address successfully. @p aIp4Address is updated.
 * @retval OT_ERROR_PARSE  The @p aIp6Address does not follow the IPv4-mapped IPv6 address format.
 */
otError otIp4FromIp4MappedIp6Address(const otIp6Address *aIp6Address, otIp4Address *aIp4Address);

/**
 * Converts a given IP4 address to an IPv6 address following the IPv4-mapped IPv6 address format.
 *
 * @param[in]  aIp4Address  An IPv4 address to convert.
 * @param[out] aIp6Address  An IPv6 address to set.
 */
void otIp4ToIp4MappedIp6Address(const otIp4Address *aIp4Address, otIp6Address *aIp6Address);

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
 */
void otIp4AddressToString(const otIp4Address *aAddress, char *aBuffer, uint16_t aSize);

#define OT_IP4_CIDR_STRING_SIZE 20 ///< Length of 000.000.000.000/00 plus a suffix NUL

/**
 * Converts a human-readable IPv4 CIDR string into a binary representation.
 *
 * @param[in]   aString   A pointer to a NULL-terminated string.
 * @param[out]  aCidr     A pointer to an IPv4 CIDR.
 *
 * @retval OT_ERROR_NONE          Successfully parsed the string.
 * @retval OT_ERROR_INVALID_ARGS  Failed to parse the string.
 */
otError otIp4CidrFromString(const char *aString, otIp4Cidr *aCidr);

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
 */
void otIp4CidrToString(const otIp4Cidr *aCidr, char *aBuffer, uint16_t aSize);

/**
 * Converts a human-readable IPv4 address string into a binary representation.
 *
 * @param[in]   aString   A pointer to a NULL-terminated string.
 * @param[out]  aAddress  A pointer to an IPv4 address.
 *
 * @retval OT_ERROR_NONE          Successfully parsed the string.
 * @retval OT_ERROR_INVALID_ARGS  Failed to parse the string.
 */
otError otIp4AddressFromString(const char *aString, otIp4Address *aAddress);

/**
 * Sets the IPv6 address by performing NAT64 address translation from the preferred NAT64 prefix and the given IPv4
 * address as specified in RFC 6052.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[in]   aIp4Address  A pointer to the IPv4 address to translate to IPv6.
 * @param[out]  aIp6Address  A pointer to the synthesized IPv6 address.
 *
 * @returns  OT_ERROR_NONE           Successfully synthesized the IPv6 address from NAT64 prefix and IPv4 address.
 * @returns  OT_ERROR_INVALID_STATE  No valid NAT64 prefix in the network data.
 */
otError otNat64SynthesizeIp6Address(otInstance *aInstance, const otIp4Address *aIp4Address, otIp6Address *aIp6Address);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif // OPENTHREAD_NAT64_H_
