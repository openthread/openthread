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
 *   This file includes definitions for DHCPv6 Service.
 */

#ifndef DHCP6_HPP_
#define DHCP6_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE || OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE

#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "mac/mac_types.hpp"
#include "net/udp6.hpp"

namespace ot {
namespace Dhcp6 {

/**
 * @addtogroup core-dhcp6
 *
 * @brief
 *   This module includes definitions for DHCPv6.
 *
 * @{
 */

constexpr uint16_t kDhcpClientPort       = 546;
constexpr uint16_t kDhcpServerPort       = 547;
constexpr uint16_t kHardwareTypeEui64    = 27;
constexpr uint16_t kHardwareTypeEthernet = 1;

/**
 * DHCPv6 Message Types
 */
enum Type : uint8_t
{
    kTypeNone               = 0,
    kTypeSolicit            = 1,
    kTypeAdvertise          = 2,
    kTypeRequest            = 3,
    kTypeConfirm            = 4,
    kTypeRenew              = 5,
    kTypeRebind             = 6,
    kTypeReply              = 7,
    kTypeRelease            = 8,
    kTypeDecline            = 9,
    kTypeReconfigure        = 10,
    kTypeInformationRequest = 11,
    kTypeRelayForward       = 12,
    kTypeRelayReply         = 13,
    kTypeLeaseQuery         = 14,
    kTypeLeaseQueryReply    = 15,
};

/**
 * Represents a DHCP6 transaction identifier.
 */
OT_TOOL_PACKED_BEGIN
class TransactionId : public Equatable<TransactionId>, public Clearable<TransactionId>
{
public:
    static constexpr uint16_t kSize = 3; ///< Transaction Id size (in bytes).

    /**
     * Generates a cryptographically secure random sequence to populate the transaction identifier.
     *
     * @retval kErrorNone     Successfully generated a random transaction identifier.
     * @retval kErrorFailed   Failed to generate random sequence.
     */
    Error GenerateRandom(void)
    {
        OT_UNUSED_VARIABLE(m8);

        return Random::Crypto::Fill(*this);
    }

private:
    uint8_t m8[kSize];
} OT_TOOL_PACKED_END;

/**
 * Implements DHCPv6 header.
 */
OT_TOOL_PACKED_BEGIN
class Header : public Clearable<Header>
{
public:
    /**
     * Returns the DHCPv6 message type.
     *
     * @returns The DHCPv6 message type.
     */
    Type GetType(void) const { return mType; }

    /**
     * Sets the DHCPv6 message type.
     *
     * @param[in]  aType  The DHCPv6 message type.
     */
    void SetType(Type aType) { mType = aType; }

    /**
     * Returns the DHCPv6 message transaction identifier.
     *
     * @returns The DHCPv6 message transaction identifier.
     */
    const TransactionId &GetTransactionId(void) const { return mTransactionId; }

    /**
     * Sets the DHCPv6 message transaction identifier.
     *
     * @param[in]  aTransactionId  The DHCPv6 message transaction identifier.
     */
    void SetTransactionId(const TransactionId &aTransactionId) { mTransactionId = aTransactionId; }

private:
    Type          mType;
    TransactionId mTransactionId;
} OT_TOOL_PACKED_END;

/**
 * DHCPv6 Option Codes.
 */
enum Code : uint16_t
{
    kOptionClientIdentifier          = 1,
    kOptionServerIdentifier          = 2,
    kOptionIaNa                      = 3,
    kOptionIaTa                      = 4,
    kOptionIaAddress                 = 5,
    kOptionRequestOption             = 6,
    kOptionPreference                = 7,
    kOptionElapsedTime               = 8,
    kOptionRelayMessage              = 9,
    kOptionAuthentication            = 11,
    kOptionServerUnicast             = 12,
    kOptionStatusCode                = 13,
    kOptionRapidCommit               = 14,
    kOptionUserClass                 = 15,
    kOptionVendorClass               = 16,
    kOptionVendorSpecificInformation = 17,
    kOptionInterfaceId               = 18,
    kOptionReconfigureMessage        = 19,
    kOptionReconfigureAccept         = 20,
    kOptionLeaseQuery                = 44,
    kOptionClientData                = 45,
    kOptionClientLastTransactionTime = 46,
};

/**
 * Implements DHCPv6 option.
 */
OT_TOOL_PACKED_BEGIN
class Option
{
public:
    /**
     * Initializes the DHCPv6 option to all zeros.
     */
    void Init(void)
    {
        mCode   = 0;
        mLength = 0;
    }

    /**
     * Returns the DHCPv6 option code.
     *
     * @returns The DHCPv6 option code.
     */
    Code GetCode(void) const { return static_cast<Code>(BigEndian::HostSwap16(mCode)); }

    /**
     * Sets the DHCPv6 option code.
     *
     * @param[in]  aCode  The DHCPv6 option code.
     */
    void SetCode(Code aCode) { mCode = BigEndian::HostSwap16(static_cast<uint16_t>(aCode)); }

    /**
     * Returns the length of DHCPv6 option.
     *
     * @returns The length of DHCPv6 option.
     */
    uint16_t GetLength(void) const { return BigEndian::HostSwap16(mLength); }

    /**
     * Sets the length of DHCPv6 option.
     *
     * @param[in]  aLength  The length of DHCPv6 option.
     */
    void SetLength(uint16_t aLength) { mLength = BigEndian::HostSwap16(aLength); }

private:
    uint16_t mCode;
    uint16_t mLength;
} OT_TOOL_PACKED_END;

/**
 * DHCP6 Unique Identifier (DUID) Type.
 */
enum DuidType : uint16_t
{
    kDuidLinkLayerAddressPlusTime = 1, ///< Link-layer address plus time (DUID-LLT).
    kDuidEnterpriseNumber         = 2, ///< Vendor-assigned unique ID based on Enterprise Number (DUID-EN).
    kDuidLinkLayerAddress         = 3, ///< Link-layer address (DUID-LL).
};

OT_TOOL_PACKED_BEGIN
class ClientIdentifier : public Option
{
public:
    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void)
    {
        SetCode(kOptionClientIdentifier);
        SetLength(sizeof(*this) - sizeof(Option));
    }

    /**
     * Returns the client DUID Type.
     *
     * @returns The client DUID Type.
     */
    DuidType GetDuidType(void) const { return static_cast<DuidType>(BigEndian::HostSwap16(mDuidType)); }

    /**
     * Sets the client DUID Type.
     *
     * @param[in]  aDuidType  The client DUID Type.
     */
    void SetDuidType(DuidType aDuidType) { mDuidType = BigEndian::HostSwap16(static_cast<uint16_t>(aDuidType)); }

    /**
     * Returns the client Duid HardwareType.
     *
     * @returns The client Duid HardwareType.
     */
    uint16_t GetDuidHardwareType(void) const { return BigEndian::HostSwap16(mDuidHardwareType); }

    /**
     * Sets the client Duid HardwareType.
     *
     * @param[in]  aDuidHardwareType  The client Duid HardwareType.
     */
    void SetDuidHardwareType(uint16_t aDuidHardwareType)
    {
        mDuidHardwareType = BigEndian::HostSwap16(aDuidHardwareType);
    }

    /**
     * Returns the client LinkLayerAddress.
     *
     * @returns The link-layer address.
     */
    const Mac::ExtAddress &GetDuidLinkLayerAddress(void) const { return mDuidLinkLayerAddress; }

    /**
     * Sets the client LinkLayerAddress.
     *
     * @param[in]  aDuidLinkLayerAddress  The client LinkLayerAddress.
     */
    void SetDuidLinkLayerAddress(const Mac::ExtAddress &aDuidLinkLayerAddress)
    {
        mDuidLinkLayerAddress = aDuidLinkLayerAddress;
    }

private:
    uint16_t        mDuidType;
    uint16_t        mDuidHardwareType;
    Mac::ExtAddress mDuidLinkLayerAddress;
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class ServerIdentifier : public Option
{
public:
    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void)
    {
        SetCode(kOptionServerIdentifier);
        SetLength(sizeof(*this) - sizeof(Option));
    }

    /**
     * Returns the server DUID Type.
     *
     * @returns The server DUID Type.
     */
    DuidType GetDuidType(void) const { return static_cast<DuidType>(BigEndian::HostSwap16(mDuidType)); }

    /**
     * Sets the server DUID Type.
     *
     * @param[in]  aDuidType  The server DUID Type.
     */
    void SetDuidType(DuidType aDuidType) { mDuidType = BigEndian::HostSwap16(static_cast<uint16_t>(aDuidType)); }

    /**
     * Returns the server DUID HardwareType.
     *
     * @returns The server DUID HardwareType.
     */
    uint16_t GetDuidHardwareType(void) const { return BigEndian::HostSwap16(mDuidHardwareType); }

    /**
     * Sets the server DUID HardwareType.
     *
     * @param[in]  aDuidHardwareType  The server DUID HardwareType.
     */
    void SetDuidHardwareType(uint16_t aDuidHardwareType)
    {
        mDuidHardwareType = BigEndian::HostSwap16(aDuidHardwareType);
    }

    /**
     * Returns the server LinkLayerAddress.
     *
     * @returns The link-layer address.
     */
    const Mac::ExtAddress &GetDuidLinkLayerAddress(void) const { return mDuidLinkLayerAddress; }

    /**
     * Sets the server LinkLayerAddress.
     *
     * @param[in]  aDuidLinkLayerAddress  The server LinkLayerAddress.
     */
    void SetDuidLinkLayerAddress(const Mac::ExtAddress &aDuidLinkLayerAddress)
    {
        mDuidLinkLayerAddress = aDuidLinkLayerAddress;
    }

private:
    uint16_t        mDuidType;
    uint16_t        mDuidHardwareType;
    Mac::ExtAddress mDuidLinkLayerAddress;
} OT_TOOL_PACKED_END;

/**
 * Represents an Identity Association for Non-temporary Address DHCPv6 option.
 */
OT_TOOL_PACKED_BEGIN
class IaNa : public Option
{
public:
    static constexpr uint32_t kDefaultT1 = 0xffffffffU; ///< Default T1 value.
    static constexpr uint32_t kDefaultT2 = 0xffffffffU; ///< Default T2 value.

    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void)
    {
        SetCode(kOptionIaNa);
        SetLength(sizeof(*this) - sizeof(Option));
    }

    /**
     * Returns client IAID.
     *
     * @returns The client IAID.
     */
    uint32_t GetIaid(void) const { return BigEndian::HostSwap32(mIaid); }

    /**
     * Sets the client IAID.
     *
     * @param[in]  aIaid  The client IAID.
     */
    void SetIaid(uint32_t aIaid) { mIaid = BigEndian::HostSwap32(aIaid); }

    /**
     * Returns T1.
     *
     * @returns The value of T1.
     */
    uint32_t GetT1(void) const { return BigEndian::HostSwap32(mT1); }

    /**
     * Sets the value of T1.
     *
     * @param[in]  aT1  The value of T1.
     */
    void SetT1(uint32_t aT1) { mT1 = BigEndian::HostSwap32(aT1); }

    /**
     * Returns T2.
     *
     * @returns The value of T2.
     */
    uint32_t GetT2(void) const { return BigEndian::HostSwap32(mT2); }

    /**
     * Sets the value of T2.
     *
     * @param[in]  aT2  The value of T2.
     */
    void SetT2(uint32_t aT2) { mT2 = BigEndian::HostSwap32(aT2); }

private:
    uint32_t mIaid;
    uint32_t mT1;
    uint32_t mT2;
} OT_TOOL_PACKED_END;

/**
 * Represents an Identity Association Address DHCPv6 option.
 */
OT_TOOL_PACKED_BEGIN
class IaAddress : public Option
{
public:
    static constexpr uint32_t kDefaultPreferredLifetime = 0xffffffffU; ///< Default preferred lifetime.
    static constexpr uint32_t kDefaultValidLifetime     = 0xffffffffU; ///< Default valid lifetime.

    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void)
    {
        SetCode(kOptionIaAddress);
        SetLength(sizeof(*this) - sizeof(Option));
    }

    /**
     * Returns a reference to the IPv6 address.
     *
     * @returns A reference to the IPv6 address.
     */
    Ip6::Address &GetAddress(void) { return mAddress; }

    /**
     * Returns a reference to the IPv6 address.
     *
     * @returns A reference to the IPv6 address.
     */
    const Ip6::Address &GetAddress(void) const { return mAddress; }

    /**
     * Sets the IPv6 address.
     *
     * @param[in]  aAddress  The reference to the IPv6 address to set.
     */
    void SetAddress(const Ip6::Address &aAddress) { mAddress = aAddress; }

    /**
     * Returns the preferred lifetime of the IPv6 address.
     *
     * @returns The preferred lifetime of the IPv6 address.
     */
    uint32_t GetPreferredLifetime(void) const { return BigEndian::HostSwap32(mPreferredLifetime); }

    /**
     * Sets the preferred lifetime of the IPv6 address.
     *
     * @param[in]  aPreferredLifetime  The preferred lifetime of the IPv6 address.
     */
    void SetPreferredLifetime(uint32_t aPreferredLifetime)
    {
        mPreferredLifetime = BigEndian::HostSwap32(aPreferredLifetime);
    }

    /**
     * Returns the valid lifetime of the IPv6 address.
     *
     * @returns The valid lifetime of the IPv6 address.
     */
    uint32_t GetValidLifetime(void) const { return BigEndian::HostSwap32(mValidLifetime); }

    /**
     * Sets the valid lifetime of the IPv6 address.
     *
     * @param[in]  aValidLifetime  The valid lifetime of the IPv6 address.
     */
    void SetValidLifetime(uint32_t aValidLifetime) { mValidLifetime = BigEndian::HostSwap32(aValidLifetime); }

private:
    Ip6::Address mAddress;
    uint32_t     mPreferredLifetime;
    uint32_t     mValidLifetime;
} OT_TOOL_PACKED_END;

/**
 * Represents an Elapsed Time DHCPv6 option.
 */
OT_TOOL_PACKED_BEGIN
class ElapsedTime : public Option
{
public:
    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void)
    {
        SetCode(kOptionElapsedTime);
        SetLength(sizeof(*this) - sizeof(Option));
    }

    /**
     * Returns the elapsed time since solicit starts.
     *
     * @returns The elapsed time since solicit starts.
     */
    uint16_t GetElapsedTime(void) const { return BigEndian::HostSwap16(mElapsedTime); }

    /**
     * Sets the elapsed time since solicit starts.
     *
     * @param[in] aElapsedTime The elapsed time since solicit starts.
     */
    void SetElapsedTime(uint16_t aElapsedTime) { mElapsedTime = BigEndian::HostSwap16(aElapsedTime); }

private:
    uint16_t mElapsedTime;
} OT_TOOL_PACKED_END;

/**
 * Status Code.
 */
enum Status : uint16_t
{
    kStatusSuccess      = 0,
    kStatusUnspecFail   = 1,
    kStatusNoAddrsAvail = 2,
    kStatusNoBinding    = 3,
    kStatusNotOnLink    = 4,
    kStatusUseMulticast = 5,
    kUnknownQueryType   = 7,
    kMalformedQuery     = 8,
    kNotConfigured      = 9,
    kNotAllowed         = 10,
};

/**
 * Represents an Status Code DHCPv6 option.
 */
OT_TOOL_PACKED_BEGIN
class StatusCode : public Option
{
public:
    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void)
    {
        SetCode(kOptionStatusCode);
        SetLength(sizeof(*this) - sizeof(Option));
    }

    /**
     * Returns the status code.
     *
     * @returns The status code.
     */
    Status GetStatusCode(void) const { return static_cast<Status>(BigEndian::HostSwap16(mStatus)); }

    /**
     * Sets the status code.
     *
     * @param[in] aStatus The status code.
     */
    void SetStatusCode(Status aStatus) { mStatus = BigEndian::HostSwap16(static_cast<uint16_t>(aStatus)); }

private:
    uint16_t mStatus;
} OT_TOOL_PACKED_END;

/**
 * Represents an Rapid Commit DHCPv6 option.
 */
OT_TOOL_PACKED_BEGIN
class RapidCommit : public Option
{
public:
    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void)
    {
        SetCode(kOptionRapidCommit);
        SetLength(sizeof(*this) - sizeof(Option));
    }
} OT_TOOL_PACKED_END;

} // namespace Dhcp6
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE || OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE

#endif // DHCP6_HPP_
