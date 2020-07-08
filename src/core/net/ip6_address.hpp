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

#include "openthread-core-config.h"

#include <stdint.h>

#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/equatable.hpp"
#include "common/string.hpp"
#include "mac/mac_types.hpp"
#include "thread/mle_types.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Ip6 {

/**
 * @addtogroup core-ip6-ip6
 *
 * @{
 *
 */

/**
 * This class represents the Interface Identifier of an IPv6 address.
 *
 */
OT_TOOL_PACKED_BEGIN
class InterfaceIdentifier : public otIp6InterfaceIdentifier,
                            public Equatable<InterfaceIdentifier>,
                            public Clearable<InterfaceIdentifier>
{
    friend class Address;

public:
    enum
    {
        kSize           = OT_IP6_IID_SIZE, ///< Size of an IPv6 Interface Identifier (in bytes).
        kInfoStringSize = 17,              ///< Max chars for the info string (`ToString()`).
    };

    /**
     * This type defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * This method indicates whether or not the Interface Identifier is unspecified.
     *
     * @retval true  If the Interface Identifier is unspecified.
     * @retval false If the Interface Identifier is not unspecified.
     *
     */
    bool IsUnspecified(void) const;

    /**
     * This method indicates whether or not the Interface Identifier is reserved (RFC 5453).
     *
     * @retval true  If the Interface Identifier is reserved.
     * @retval false If the Interface Identifier is not reserved.
     *
     */
    bool IsReserved(void) const;

    /**
     * This method indicates whether or not the Interface Identifier is Subnet-Router Anycast (RFC 4291).
     *
     * @retval TRUE   If the Interface Identifier is a Subnet-Router Anycast address.
     * @retval FALSE  If the Interface Identifier is not a Subnet-Router Anycast address.
     *
     */
    bool IsSubnetRouterAnycast(void) const;

    /**
     * This method indicates whether or not the Interface Identifier is Reserved Subnet Anycast (RFC 2526).
     *
     * @retval TRUE   If the Interface Identifier is a Reserved Subnet Anycast address.
     * @retval FALSE  If the Interface Identifier is not a Reserved Subnet Anycast address.
     *
     */
    bool IsReservedSubnetAnycast(void) const;

    /**
     * This method generates and sets the Interface Identifier to a crypto-secure random byte sequence.
     *
     */
    void GenerateRandom(void);

    /**
     * This method gets the Interface Identifier as a pointer to a byte array.
     *
     * @returns A pointer to a byte array (of size `kSize`) containing the Interface Identifier.
     *
     */
    const uint8_t *GetBytes(void) const { return mFields.m8; }

    /**
     * This method sets the Interface Identifier from a given byte array.
     *
     * @param[in] aBuffer    Pointer to an array containing the Interface Identifier. `kSize` bytes from the buffer
     *                       are copied to form the Interface Identifier.
     *
     */
    void SetBytes(const uint8_t *aBuffer);

    /**
     * This method sets the Interface Identifier from a given IEEE 802.15.4 Extended Address.
     *
     * @param[in] aExtAddress  An Extended Address.
     *
     */
    void SetFromExtAddress(const Mac::ExtAddress &aExtAddress);

    /**
     * This method converts the Interface Identifier to an IEEE 802.15.4 Extended Address.
     *
     * @param[out]  aExtAddress  A reference to an Extended Address where the converted address is placed.
     *
     */
    void ConvertToExtAddress(Mac::ExtAddress &aExtAddress) const;

    /**
     * This method converts the Interface Identifier to an IEEE 802.15.4 MAC Address.
     *
     * @param[out]  aMacAddress  A reference to a MAC Address where the converted address is placed.
     *
     */
    void ConvertToMacAddress(Mac::Address &aMacAddress) const;

    /**
     * This method sets the Interface Identifier to Routing/Anycast Locator pattern `0000:00ff:fe00:xxxx` with a given
     * locator (RLOC16 or ALOC16) value.
     *
     * @param[in]  aLocator    RLOC16 or ALOC16.
     *
     */
    void SetToLocator(uint16_t aLocator);

    /**
     * This method indicates whether or not the Interface Identifier matches the locator pattern `0000:00ff:fe00:xxxx`.
     *
     * @retval TRUE   If the IID matches the locator pattern.
     * @retval FALSE  If the IID does not match the locator pattern.
     *
     */
    bool IsLocator(void) const;

    /**
     * This method indicates whether or not the Interface Identifier (IID) matches a Routing Locator (RLOC).
     *
     * In addition to checking that the IID matches the locator pattern (`0000:00ff:fe00:xxxx`), this method also
     * checks that the locator value is a valid RLOC16.
     *
     * @retval TRUE   If the IID matches a RLOC address.
     * @retval FALSE  If the IID does not match a RLOC address.
     *
     */
    bool IsRoutingLocator(void) const;

    /**
     * This method indicates whether or not the Interface Identifier (IID) matches an Anycast Locator (ALOC).
     *
     * In addition to checking that the IID matches the locator pattern (`0000:00ff:fe00:xxxx`), this method also
     * checks that the locator value is any valid ALOC16 (0xfc00 - 0xfcff).
     *
     * @retval TRUE   If the IID matches a ALOC address.
     * @retval FALSE  If the IID does not match a ALOC address.
     *
     */
    bool IsAnycastLocator(void) const;

    /**
     * This method indicates whether or not the Interface Identifier (IID) matches a Service Anycast  Locator (ALOC).
     *
     * In addition to checking that the IID matches the locator pattern (`0000:00ff:fe00:xxxx`), this method also
     * checks that the locator value is a valid Service ALOC16 (0xfc10 – 0xfc2f).
     *
     * @retval TRUE   If the IID matches a ALOC address.
     * @retval FALSE  If the IID does not match a ALOC address.
     *
     */
    bool IsAnycastServiceLocator(void) const;

    /**
     * This method gets the Interface Identifier (IID) address locator fields.
     *
     * This method assumes the IID to match the locator pattern `0000:00ff:fe00:xxxx` (does not explicitly check this)
     * and returns the last `uint16` portion of the IID.
     *
     * @returns The RLOC16 or ALOC16.
     *
     */
    uint16_t GetLocator(void) const { return HostSwap16(mFields.m16[3]); }

    /**
     * This method sets the Interface Identifier (IID) address locator field.
     *
     * Unlike `SetToLocator()`, this method only changes the last 2 bytes of the IID and keeps the rest of the address
     * as before.
     *
     * @param[in]  aLocator   RLOC16 or ALOC16.
     *
     */
    void SetLocator(uint16_t aLocator) { mFields.m16[3] = HostSwap16(aLocator); }

    /**
     * This method converts an Interface Identifier to a string.
     *
     * @returns An `InfoString` containing the string representation of the Interface Identifier.
     *
     */
    InfoString ToString(void) const;

private:
    enum : uint8_t
    {
        kAloc16Mask            = 0xfc, // The mask for Aloc16.
        kRloc16ReservedBitMask = 0x02, // The mask for the reserved bit of Rloc16.
    };

} OT_TOOL_PACKED_END;

/**
 * This class implements an IPv6 address object.
 *
 */
OT_TOOL_PACKED_BEGIN
class Address : public otIp6Address, public Equatable<Address>, public Clearable<Address>
{
public:
    /**
     * Masks
     *
     */
    enum
    {
        kAloc16Mask = InterfaceIdentifier::kAloc16Mask, ///< The mask for Aloc16.
    };

    /**
     * Constants
     *
     */
    enum
    {
        kSize                 = OT_IP6_ADDRESS_SIZE, ///< Size of an IPv6 Address (in bytes).
        kIp6AddressStringSize = 40, ///< Max buffer size in bytes to store an IPv6 address in string format.
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
     * This type defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<kIp6AddressStringSize> InfoString;

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
     * This method indicates whether or not the IPv6 address scope is Link-Local.
     *
     * @retval TRUE   If the IPv6 address scope is Link-Local.
     * @retval FALSE  If the IPv6 address scope is not Link-Local.
     *
     */
    bool IsLinkLocal(void) const;

    /**
     * This methods sets the IPv6 address to a Link-Local address with Interface Identifier generated from a given
     * MAC Extended Address.
     *
     * @param[in]  aExtAddress  A MAC Extended Address (used to generate the IID).
     *
     */
    void SetToLinkLocalAddress(const Mac::ExtAddress &aExtAddress);

    /**
     * This methods sets the IPv6 address to a Link-Local address with a given Interface Identifier.
     *
     * @param[in]  aIid   An Interface Identifier.
     *
     */
    void SetToLinkLocalAddress(const InterfaceIdentifier &aIid);

    /**
     * This method indicates whether or not the IPv6 address is multicast address.
     *
     * @retval TRUE   If the IPv6 address is a multicast address.
     * @retval FALSE  If the IPv6 address scope is not a multicast address.
     *
     */
    bool IsMulticast(void) const { return mFields.m8[0] == 0xff; }

    /**
     * This method indicates whether or not the IPv6 address is a link-local multicast address.
     *
     * @retval TRUE   If the IPv6 address is a link-local multicast address.
     * @retval FALSE  If the IPv6 address scope is not a link-local multicast address.
     *
     */
    bool IsLinkLocalMulticast(void) const;

    /**
     * This method indicates whether or not the IPv6 address is a link-local all nodes multicast address (ff02::01).
     *
     * @retval TRUE   If the IPv6 address is a link-local all nodes multicast address.
     * @retval FALSE  If the IPv6 address is not a link-local all nodes multicast address.
     *
     */
    bool IsLinkLocalAllNodesMulticast(void) const;

    /**
     * This method sets the IPv6 address to the link-local all nodes multicast address (ff02::01).
     *
     */
    void SetToLinkLocalAllNodesMulticast(void);

    /**
     * This method indicates whether or not the IPv6 address is a link-local all routers multicast address (ff02::02).
     *
     * @retval TRUE   If the IPv6 address is a link-local all routers multicast address.
     * @retval FALSE  If the IPv6 address is not a link-local all routers multicast address.
     *
     */
    bool IsLinkLocalAllRoutersMulticast(void) const;

    /**
     * This method sets the IPv6 address to the link-local all routers multicast address (ff02::02).
     *
     */
    void SetToLinkLocalAllRoutersMulticast(void);

    /**
     * This method indicates whether or not the IPv6 address is a realm-local multicast address.
     *
     * @retval TRUE   If the IPv6 address is a realm-local multicast address.
     * @retval FALSE  If the IPv6 address scope is not a realm-local multicast address.
     *
     */
    bool IsRealmLocalMulticast(void) const;

    /**
     * This method indicates whether or not the IPv6 address is a realm-local all nodes multicast address (ff03::01).
     *
     * @retval TRUE   If the IPv6 address is a realm-local all nodes multicast address.
     * @retval FALSE  If the IPv6 address is not a realm-local all nodes multicast address.
     *
     */
    bool IsRealmLocalAllNodesMulticast(void) const;

    /**
     * This method sets the IPv6 address to the realm-local all nodes multicast address (ff03::01)
     *
     */
    void SetToRealmLocalAllNodesMulticast(void);

    /**
     * This method indicates whether or not the IPv6 address is a realm-local all routers multicast address (ff03::02).
     *
     * @retval TRUE   If the IPv6 address is a realm-local all routers multicast address.
     * @retval FALSE  If the IPv6 address is not a realm-local all routers multicast address.
     *
     */
    bool IsRealmLocalAllRoutersMulticast(void) const;

    /**
     * This method sets the IPv6 address to the realm-local all routers multicast address (ff03::02).
     *
     */
    void SetToRealmLocalAllRoutersMulticast(void);

    /**
     * This method indicates whether or not the IPv6 address is a realm-local all MPL forwarders address (ff03::fc).
     *
     * @retval TRUE   If the IPv6 address is a realm-local all MPL forwarders address.
     * @retval FALSE  If the IPv6 address is not a realm-local all MPL forwarders address.
     *
     */
    bool IsRealmLocalAllMplForwarders(void) const;

    /**
     * This method sets the the IPv6 address to the realm-local all MPL forwarders address (ff03::fc).
     *
     */
    void SetToRealmLocalAllMplForwarders(void);

    /**
     * This method indicates whether or not the IPv6 address is multicast larger than realm local.
     *
     * @retval TRUE   If the IPv6 address is multicast larger than realm local.
     * @retval FALSE  If the IPv6 address is not multicast or the scope is not larger than realm local.
     *
     */
    bool IsMulticastLargerThanRealmLocal(void) const;

    /**
     * This method sets the IPv6 address to a Routing Locator (RLOC) IPv6 address with a given Mesh-local prefix and
     * RLOC16 value.
     *
     * @param[in]  aMeshLocalPrefix  A Mesh Local Prefix.
     * @param[in]  aRloc16           A RLOC16 value.
     *
     */
    void SetToRoutingLocator(const Mle::MeshLocalPrefix &aMeshLocalPrefix, uint16_t aRloc16)
    {
        SetToLocator(aMeshLocalPrefix, aRloc16);
    }

    /**
     * This method sets the IPv6 address to a Anycast Locator (ALOC) IPv6 address with a given Mesh-local prefix and
     * ALOC16 value.
     *
     * @param[in]  aMeshLocalPrefix  A Mesh Local Prefix.
     * @param[in]  aAloc16           A ALOC16 value.
     *
     */
    void SetToAnycastLocator(const Mle::MeshLocalPrefix &aMeshLocalPrefix, uint16_t aAloc16)
    {
        SetToLocator(aMeshLocalPrefix, aAloc16);
    }

    /**
     * This method sets the IPv6 address prefix.
     *
     * This method only changes the first @p aPrefixLength bits of the address and keeps the rest of the bits in the
     * address as before.
     *
     * @param[in]  aPrefix         A buffer containing the prefix
     * @param[in]  aPrefixLength   The prefix length (in bits).
     *
     */
    void SetPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength);

    /**
     * This method sets the IPv6 address prefix to the given Mesh Local Prefix.
     *
     * @param[in]  aMeshLocalPrefix   A Mesh Local Prefix.
     *
     */
    void SetPrefix(const Mle::MeshLocalPrefix &aMeshLocalPrefix)
    {
        SetPrefix(aMeshLocalPrefix.m8, Mle::MeshLocalPrefix::kLength);
    }

    /**
     * This method sets the prefix content of the Prefix-Based Multicast Address.
     *
     * @param[in]  aPrefix         A buffer containing the prefix.
     * @param[in]  aPrefixLength   The prefix length (in bits).
     *
     */
    void SetMulticastNetworkPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength);

    /**
     * This method sets the prefix content of Mesh Local Prefix-Based Multicast Address.
     *
     * @param[in]  aMeshLocalPrefix   A reference to the Mesh Local Prefix.
     *
     */
    void SetMulticastNetworkPrefix(const Mle::MeshLocalPrefix &aMeshLocalPrefix)
    {
        SetMulticastNetworkPrefix(aMeshLocalPrefix.m8, Mle::MeshLocalPrefix::kLength);
    }

    /**
     * This method sets the prefix content of Prefix-Based Multicast Address.
     *
     * @param[in]  aPrefix A reference to an IPv6 Prefix.
     *
     */
    void SetMulticastNetworkPrefix(const otIp6Prefix &aPrefix)
    {
        SetMulticastNetworkPrefix(aPrefix.mPrefix.mFields.m8, aPrefix.mLength);
    }

    /**
     * This method returns the Interface Identifier of the IPv6 address.
     *
     * @returns A reference to the Interface Identifier.
     *
     */
    const InterfaceIdentifier &GetIid(void) const
    {
        return static_cast<const InterfaceIdentifier &>(mFields.mComponents.mIid);
    }

    /**
     * This method returns the Interface Identifier of the IPv6 address.
     *
     * @returns A reference to the Interface Identifier.
     *
     */
    InterfaceIdentifier &GetIid(void) { return static_cast<InterfaceIdentifier &>(mFields.mComponents.mIid); }

    /**
     * This method sets the Interface Identifier.
     *
     * @param[in]  aIid  An Interface Identifier.
     *
     */
    void SetIid(const InterfaceIdentifier &aIid) { GetIid() = aIid; }

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
    uint8_t PrefixMatch(const otIp6Address &aOther) const;

    /**
     * This method converts an IPv6 address string to binary.
     *
     * @param[in]  aBuf  A pointer to the null-terminated string.
     *
     * @retval OT_ERROR_NONE          Successfully parsed the IPv6 address string.
     * @retval OT_ERROR_INVALID_ARGS  Failed to parse the IPv6 address string.
     *
     */
    otError FromString(const char *aBuf);

    /**
     * This method converts an IPv6 address object to a string
     *
     * @returns An `InfoString` representing the IPv6 address.
     *
     */
    InfoString ToString(void) const;

    /**
     * This method returns the number of IPv6 prefix bits that match.
     *
     * @param[in]  aPrefixA     A pointer to the prefix to match.
     * @param[in]  aPrefixB     A pointer to the prefix to match against.
     * @param[in]  aMaxLength   Number of bytes of the two prefixes.
     *
     * @returns The number of prefix bits that match.
     *
     */
    static uint8_t PrefixMatch(const uint8_t *aPrefixA, const uint8_t *aPrefixB, uint8_t aMaxLength);

private:
    void SetPrefix(uint8_t aOffset, const uint8_t *aPrefix, uint8_t aPrefixLength);
    void SetToLocator(const Mle::MeshLocalPrefix &aMeshLocalPrefix, uint16_t aLocator);

    static const Address &GetLinkLocalAllNodesMulticast(void);
    static const Address &GetLinkLocalAllRoutersMulticast(void);
    static const Address &GetRealmLocalAllNodesMulticast(void);
    static const Address &GetRealmLocalAllRoutersMulticast(void);
    static const Address &GetRealmLocalAllMplForwarders(void);

    enum
    {
        kIp4AddressSize                     = 4, ///< Size of the IPv4 address.
        kMulticastNetworkPrefixLengthOffset = 3, ///< Prefix-Based Multicast Address (RFC3306).
        kMulticastNetworkPrefixOffset       = 4, ///< Prefix-Based Multicast Address (RFC3306).
    };
} OT_TOOL_PACKED_END;

/**
 * @}
 *
 */

} // namespace Ip6
} // namespace ot

#endif // IP6_ADDRESS_HPP_
