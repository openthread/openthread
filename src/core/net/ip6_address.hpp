/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file includes definitions for IPv6 addresses.
 */

#ifndef IP6_ADDRESS_HPP_
#define IP6_ADDRESS_HPP_

#include <stdint.h>

#include "openthread/types.h"

namespace Thread {
namespace Ip6 {

/**
 * @addtogroup core-ip6-ip6
 *
 * @{
 *
 */

/**
 * This class implements an IPv6 address object.
 *
 */
OT_TOOL_PACKED_BEGIN
class Address: public otIp6Address
{
public:
    /**
     * Constants
     *
     */
    enum
    {
        kInterfaceIdentifierSize   = 8,  ///< Interface Identifier size in bytes.
        kIp6AddressStringSize      = 40, ///< Max buffer size in bytes to store an IPv6 address in string format.
    };

    /**
     * IPv6 Address Scopes
     */
    enum
    {
        kNodeLocalScope      = 0,  ///< Node-Local scope
        kInterfaceLocalScope = 1,  ///< Interface-Local scope
        kLinkLocalScope      = 2,  ///< Link-Local scope
        kRealmLocalScope     = 3,  ///< Realm-Local scope
        kAdminLocalScope     = 4,  ///< Admin-Local scope
        kSiteLocalScope      = 5,  ///< Site-Local scope
        kOrgLocalScope       = 8,  ///< Organization-Local scope
        kGlobalScope         = 14, ///< Global scope
    };

    /**
     * This method indicates whether or not the IPv6 address is the Unspecified Address.
     *
     * @retval TRUE   If the IPv6 address is the Unspecified Address.
     * @retval FALSE  If the IPv6 address is not the Unspecified Address.
     *
     */
    bool IsUnspecified(void) const;

    /**
     * This method indicates whether or not the IPv6 address is the Loopback Address.
     *
     * @retval TRUE   If the IPv6 address is the Loopback Address.
     * @retval FALSE  If the IPv6 address is not the Loopback Address.
     *
     */
    bool IsLoopback(void) const;

    /**
     * This method indicates whether or not the IPv6 address scope is Interface-Local.
     *
     * @retval TRUE   If the IPv6 address scope is Interface-Local.
     * @retval FALSE  If the IPv6 address scope is not Interface-Local.
     *
     */
    bool IsInterfaceLocal(void) const;

    /**
     * This method indicates whether or not the IPv6 address scope is Link-Local.
     *
     * @retval TRUE   If the IPv6 address scope is Link-Local.
     * @retval FALSE  If the IPv6 address scope is not Link-Local.
     *
     */
    bool IsLinkLocal(void) const;

    /**
     * This method indicates whether or not the IPv6 address is multicast address.
     *
     * @retval TRUE   If the IPv6 address is a multicast address.
     * @retval FALSE  If the IPv6 address scope is not a multicast address.
     *
     */
    bool IsMulticast(void) const;

    /**
     * This method indicates whether or not the IPv6 address is a link-local multicast address.
     *
     * @retval TRUE   If the IPv6 address is a link-local multicast address.
     * @retval FALSE  If the IPv6 address scope is not a link-local multicast address.
     *
     */
    bool IsLinkLocalMulticast(void) const;

    /**
     * This method indicates whether or not the IPv6 address is a link-local all nodes multicast address.
     *
     * @retval TRUE   If the IPv6 address is a link-local all nodes multicast address.
     * @retval FALSE  If the IPv6 address is not a link-local all nodes multicast address.
     *
     */
    bool IsLinkLocalAllNodesMulticast(void) const;

    /**
     * This method indicates whether or not the IPv6 address is a link-local all routers multicast address.
     *
     * @retval TRUE   If the IPv6 address is a link-local all routers multicast address.
     * @retval FALSE  If the IPv6 address is not a link-local all routers multicast address.
     *
     */
    bool IsLinkLocalAllRoutersMulticast(void) const;

    /**
     * This method indicates whether or not the IPv6 address is a realm-local multicast address.
     *
     * @retval TRUE   If the IPv6 address is a realm-local multicast address.
     * @retval FALSE  If the IPv6 address scope is not a realm-local multicast address.
     *
     */
    bool IsRealmLocalMulticast(void) const;

    /**
     * This method indicates whether or not the IPv6 address is a realm-local all nodes multicast address.
     *
     * @retval TRUE   If the IPv6 address is a realm-local all nodes multicast address.
     * @retval FALSE  If the IPv6 address is not a realm-local all nodes multicast address.
     *
     */
    bool IsRealmLocalAllNodesMulticast(void) const;

    /**
     * This method indicates whether or not the IPv6 address is a realm-local all routers multicast address.
     *
     * @retval TRUE   If the IPv6 address is a realm-local all routers multicast address.
     * @retval FALSE  If the IPv6 address is not a realm-local all routers multicast address.
     *
     */
    bool IsRealmLocalAllRoutersMulticast(void) const;

    /**
     * This method indicates whether or not the IPv6 address is a realm-local all MPL forwarders address.
     *
     * @retval TRUE   If the IPv6 address is a realm-local all MPL forwarders address.
     * @retval FALSE  If the IPv6 address is not a realm-local all MPL forwarders address.
     *
     */
    bool IsRealmLocalAllMplForwarders(void) const;

    /**
     * This method indicates whether or not the IPv6 address is a RLOC address.
     *
     * @retval TRUE   If the IPv6 address is a RLOC address.
     * @retval FALSE  If the IPv6 address is not a RLOC address.
     *
     */
    bool IsRoutingLocator(void) const;

    /**
     * This method indicates whether or not the IPv6 address is an Anycast RLOC address.
     *
     * @retval TRUE   If the IPv6 address is an Anycast RLOC address.
     * @retval FALSE  If the IPv6 address is not an Anycast RLOC address.
     *
     */
    bool IsAnycastRoutingLocator(void) const;

    /**
     * This method indicates whether or not the IPv6 address is Subnet-Router Anycast (RFC 4291),
     *
     * @retval TRUE   If the IPv6 address is a Subnet-Router Anycast address.
     * @retval FALSE  If the IPv6 address is not a Subnet-Router Anycast address.
     *
     */
    bool IsSubnetRouterAnycast(void) const;

    /**
     * This method indicates whether or not the IPv6 address is Reserved Subnet Anycast (RFC 2526),
     *
     * @retval TRUE   If the IPv6 address is a Reserved Subnet Anycast address.
     * @retval FALSE  If the IPv6 address is not a Reserved Subnet Anycast address.
     *
     */
    bool IsReservedSubnetAnycast(void) const;

    /**
     * This method indicates whether or not the IPv6 address contains Reserved IPv6 IID (RFC 5453),
     *
     * @retval TRUE   If the IPv6 address contains a reserved IPv6 IID.
     * @retval FALSE  If the IPv6 address does not contain a reserved IPv6 IID.
     *
     */
    bool IsIidReserved(void) const;

    /**
     * This method returns a pointer to the Interface Identifier.
     *
     * @returns A pointer to the Interface Identifier.
     *
     */
    const uint8_t *GetIid(void) const;

    /**
     * This method returns a pointer to the Interface Identifier.
     *
     * @returns A pointer to the Interface Identifier.
     *
     */
    uint8_t *GetIid(void);

    /**
     * This method sets the Interface Identifier.
     *
     * @param[in]  aIid  A reference to the Interface Identifier.
     *
     */
    void SetIid(const uint8_t *aIid);

    /**
     * This method sets the Interface Identifier.
     *
     * @param[in]  aEui64  A reference to the IEEE EUI-64 address.
     *
     */
    void SetIid(const Mac::ExtAddress &aEui64);

    /**
     * This method returns the IPv6 address scope.
     *
     * @returns The IPv6 address scope.
     *
     */
    uint8_t GetScope(void) const;

    /**
     * This method returns the number of IPv6 prefix bits that match.
     *
     * @param[in]  aOther  The IPv6 address to match against.
     *
     * @returns The number of IPv6 prefix bits that match.
     *
     */
    uint8_t PrefixMatch(const Address &aOther) const;

    /**
     * This method evaluates whether or not the IPv6 addresses match.
     *
     * @param[in]  aOther  The IPv6 address to compare.
     *
     * @retval TRUE   If the IPv6 addresses match.
     * @retval FALSE  If the IPv6 addresses do not match.
     *
     */
    bool operator==(const Address &aOther) const;

    /**
     * This method evaluates whether or not the IPv6 addresses differ.
     *
     * @param[in]  aOther  The IPv6 address to compare.
     *
     * @retval TRUE   If the IPv6 addresses differ.
     * @retval FALSE  If the IPv6 addresses do not differ.
     *
     */
    bool operator!=(const Address &aOther) const;

    /**
     * This method converts an IPv6 address string to binary.
     *
     * @param[in]  aBuf  A pointer to the NULL-terminated string.
     *
     * @retval kThreadError_None         Successfully parsed the IPv6 address string.
     * @retval kTheradError_InvalidArgs  Failed to parse the IPv6 address string.
     *
     */
    ThreadError FromString(const char *aBuf);

    /**
     * This method converts an IPv6 address object to a NULL-terminated string.
     *
     * @param[out]  aBuf   A pointer to the buffer.
     * @param[in]   aSize  The maximum size of the buffer.
     *
     * @returns A pointer to the buffer.
     *
     */
    const char *ToString(char *aBuf, uint16_t aSize) const;

private:
    enum
    {
        kInterfaceIdentifierOffset = 8,  ///< Interface Identifier offset in bytes.
    };
} OT_TOOL_PACKED_END;

/**
 * @}
 *
 */

}  // namespace Ip6
}  // namespace Thread

#endif  // NET_IP6_ADDRESS_HPP_
