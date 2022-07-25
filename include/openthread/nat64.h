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

/**
 * Sets the CIDR block used for the source address of the translated address. A valid CIDR must have a non-zero prefix
 * length. Note: The actual addresses used in the CIDR is limited by the size of mapping pool.
 *
 * The NAT64 translator will expire all existing sessions when the provided CIDR is valid and is not the one configured.
 *
 * This function is available only when OPENTHREAD_CONFIG_NAT64_MANAGER_ENABLE is enabled.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aCidr A pointer to an otIp4Cidr for the IPv4 CIDR block for NAT64.
 *
 * @retval  OT_ERROR_INVALID_ARGS   The given CIDR is not a valid IPv4 CIDR for NAT64.
 * @retval  OT_ERROR_NONE           Successfully set the CIDR for NAT64.
 *
 * @sa otBorderRouterSend
 * @sa otBorderRouterSetReceiveCallback
 * @sa otBorderRouterSetNat64TranslatorEnabled
 *
 */
otError otBorderRouterSetIp4CidrForNat64(otInstance *aInstance, const otIp4Cidr *aCidr);

/**
 * This method enables/disables the NAT64 translator.
 *
 * @note  The NAT64 translator is disabled by default. If the NAT64 translator is disabled, all packets will be
 * forwarded without any checks. The NAT64 translator must be configured with a valid IPv4 CIDR before being enabled.
 *
 * This function is available only when OPENTHREAD_CONFIG_NAT64_MANAGER_ENABLE is enabled.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aEnabled A boolean to enable/disable the NAT64 translator.
 *
 * @retval  OT_ERROR_INVALID_STATE  The NAT64 module is not configured with a valid IPv4 CIDR.
 * @retval  OT_ERROR_NONE           Successfully enabled/disabled the NAT64 translator.
 *
 * @sa otBorderRouterSetIp4CidrForNat64
 *
 */
otError otBorderRouterSetNat64TranslatorEnabled(otInstance *aInstance, bool aEnabled);

/**
 * Allocate a new message buffer for sending an IPv4 message (which will be translated into an IPv6 packet by NAT64
 * later). Message buffers allocated by this function will have 20 bytes (The differences between the size of IPv6
 * headers and the size of IPv4 headers) reserved.
 *
 * This function is available only when OPENTHREAD_CONFIG_NAT64_MANAGER_ENABLE is enabled.
 *
 * @note If @p aSettings is 'NULL', the link layer security is enabled and the message priority is set to
 * OT_MESSAGE_PRIORITY_NORMAL by default.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aSettings  A pointer to the message settings or NULL to set default settings.
 *
 * @returns A pointer to the message buffer or NULL if no message buffers are available or parameters are invalid.
 *
 * @sa otMessageFree
 * @sa otBorderRouterSend
 *
 */
otMessage *otIp6NewMessageForNat64(otInstance *aInstance, const otMessageSettings *aSettings);

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif

#endif // OPENTHREAD_NAT64_H_
