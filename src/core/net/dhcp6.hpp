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

#include "common/message.hpp"
#include "net/udp6.hpp"

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {
namespace Dhcp6 {

/**
 * @addtogroup core-dhcp6
 *
 * @brief
 *   This module includes definitions for DHCPv6.
 *
 * @{
 *
 */

/**
 * DHCPv6 constant
 *
 */
enum
{
    kDhcpClientPort      = 546,
    kDhcpServerPort      = 547,
    kTransactionIdSize   = 3,
    kLinkLayerAddressLen = 8,
    kHardwareTypeEui64   = 27,
};

/**
 * DHCPv6 Message Types
 *
 */
typedef enum Type
{
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
} Type;

/**
 * This class implements DHCPv6 header.
 *
 */
OT_TOOL_PACKED_BEGIN
class Dhcp6Header
{
public:
    /**
     * This method initializes the DHCPv6 header to all zeros.
     *
     */
    void Init(void)
    {
        mType             = 0;
        mTransactionId[0] = 0;
    }

    /**
     * This method returns the DHCPv6 message type.
     *
     * @returns The DHCPv6 message type.
     *
     */
    Type GetType(void) const { return static_cast<Type>(mType); }

    /**
     * This method sets the DHCPv6 message type.
     *
     * @param[in]  aType  The DHCPv6 message type.
     *
     */
    void SetType(Type aType) { mType = static_cast<uint8_t>(aType); }

    /**
     * This method returns the DHCPv6 message transaction id.
     *
     * @returns A pointer of DHCPv6 message transaction id.
     *
     */
    uint8_t *GetTransactionId(void) { return mTransactionId; }

    /**
     * This method sets the DHCPv6 message transaction id.
     *
     * @param[in]  aBuf  The DHCPv6 message transaction id.
     *
     */
    void SetTransactionId(uint8_t *aBuf) { memcpy(mTransactionId, aBuf, kTransactionIdSize); }

private:
    uint8_t mType;                              ///< Type
    uint8_t mTransactionId[kTransactionIdSize]; ///< Transaction Id
} OT_TOOL_PACKED_END;

/**
 * DHCPv6 Option Codes
 *
 */
typedef enum Code
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
} Code;

/**
 * This class implements DHCPv6 option.
 *
 */
OT_TOOL_PACKED_BEGIN
class Dhcp6Option
{
public:
    /**
     * This method initializes the DHCPv6 option to all zeros.
     *
     */
    void Init(void)
    {
        mCode   = 0;
        mLength = 0;
    }

    /**
     * This method returns the DHCPv6 option code.
     *
     * @returns The DHCPv6 option code.
     *
     */
    Code GetCode(void) const { return static_cast<Code>(HostSwap16(mCode)); }

    /**
     * This method sets the DHCPv6 option code.
     *
     * @param[in]  aCode  The DHCPv6 option code.
     *
     */
    void SetCode(Code aCode) { mCode = HostSwap16(static_cast<uint16_t>(aCode)); }

    /**
     * This method returns the Length of DHCPv6 option.
     *
     * @returns The length of DHCPv6 option.
     *
     */
    uint16_t GetLength(void) const { return HostSwap16(mLength); }

    /**
     * This method sets the length of DHCPv6 option.
     *
     * @param[in]  aLength  The length of DHCPv6 option.
     *
     */
    void SetLength(uint16_t aLength) { mLength = HostSwap16(aLength); }

private:
    uint16_t mCode;   ///< Code
    uint16_t mLength; ///< Length
} OT_TOOL_PACKED_END;

/**
 * Duid Type
 *
 */
typedef enum DuidType
{
    kDuidLLT = 1,
    kDuidEN  = 2,
    kDuidLL  = 3,
} DuidType;

OT_TOOL_PACKED_BEGIN
class ClientIdentifier : public Dhcp6Option
{
public:
    /**
     * This method initializes the DHCPv6 Option.
     *
     */
    void Init(void)
    {
        SetCode(kOptionClientIdentifier);
        SetLength(sizeof(*this) - sizeof(Dhcp6Option));
    }

    /**
     * This method returns the client Duid Type.
     *
     * @returns The client Duid Type.
     *
     */
    DuidType GetDuidType(void) const { return static_cast<DuidType>(HostSwap16(mDuidType)); }

    /**
     * This method sets the client Duid Type.
     *
     * @param[in]  aDuidType  The client Duid Type.
     *
     */
    void SetDuidType(DuidType aDuidType) { mDuidType = HostSwap16(static_cast<uint16_t>(aDuidType)); }

    /**
     * This method returns the client Duid HardwareType.
     *
     * @returns The client Duid HardwareType.
     *
     */
    uint16_t GetDuidHardwareType(void) const { return HostSwap16(mDuidHardwareType); }

    /**
     * This method sets the client Duid HardwareType.
     *
     * @param[in]  aDuidHardwareType  The client Duid HardwareType.
     *
     */
    void SetDuidHardwareType(uint16_t aDuidHardwareType) { mDuidHardwareType = HostSwap16(aDuidHardwareType); }

    /**
     * This method returns the client LinkLayerAddress.
     *
     * @returns A pointer to the client LinkLayerAddress.
     *
     */
    uint8_t *GetDuidLinkLayerAddress(void) { return mDuidLinkLayerAddress; }

    /**
     * This method sets the client LinkLayerAddress.
     *
     * @param[in]  aLinkLayerAddress The client LinkLayerAddress.
     *
     */
    void SetDuidLinkLayerAddress(const Mac::ExtAddress &aDuidLinkLayerAddress)
    {
        memcpy(mDuidLinkLayerAddress, &aDuidLinkLayerAddress, sizeof(Mac::ExtAddress));
    }

private:
    uint16_t mDuidType;                                   ///< Duid Type
    uint16_t mDuidHardwareType;                           ///< Duid HardwareType
    uint8_t  mDuidLinkLayerAddress[kLinkLayerAddressLen]; ///< Duid LinkLayerAddress
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class ServerIdentifier : public Dhcp6Option
{
public:
    /**
     * This method initializes the DHCPv6 Option.
     *
     */
    void Init(void)
    {
        SetCode(kOptionServerIdentifier);
        SetLength(sizeof(*this) - sizeof(Dhcp6Option));
    }

    /**
     * This method returns the server Duid Type.
     *
     * @returns The server Duid Type.
     *
     */
    DuidType GetDuidType(void) const { return static_cast<DuidType>(HostSwap16(mDuidType)); }

    /**
     * This method sets the server Duid Type.
     *
     * @param[in]  aDuidType  The server Duid Type.
     *
     */
    void SetDuidType(DuidType aDuidType) { mDuidType = HostSwap16(static_cast<uint16_t>(aDuidType)); }

    /**
     * This method returns the server Duid HardwareType.
     *
     * @returns The server Duid HardwareType.
     *
     */
    uint16_t GetDuidHardwareType(void) const { return HostSwap16(mDuidHardwareType); }

    /**
     * This method sets the server Duid HardwareType.
     *
     * @param[in]  aDuidHardwareType  The server Duid HardwareType.
     *
     */
    void SetDuidHardwareType(uint16_t aDuidHardwareType) { mDuidHardwareType = HostSwap16(aDuidHardwareType); }

    /**
     * This method returns the server LinkLayerAddress.
     *
     * @returns A pointer to the server LinkLayerAddress.
     *
     */
    uint8_t *GetDuidLinkLayerAddress(void) { return mDuidLinkLayerAddress; }

    /**
     * This method sets the server LinkLayerAddress.
     *
     * @param[in]  aLinkLayerAddress The server LinkLayerAddress.
     *
     */
    void SetDuidLinkLayerAddress(const Mac::ExtAddress &aDuidLinkLayerAddress)
    {
        memcpy(mDuidLinkLayerAddress, &aDuidLinkLayerAddress, sizeof(Mac::ExtAddress));
    }

private:
    uint16_t mDuidType;                                   ///< Duid Type
    uint16_t mDuidHardwareType;                           ///< Duid HardwareType
    uint8_t  mDuidLinkLayerAddress[kLinkLayerAddressLen]; ///< Duid LinkLayerAddress
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class IaNa : public Dhcp6Option
{
public:
    /**
     * This method initializes the DHCPv6 Option.
     *
     */
    void Init(void)
    {
        SetCode(kOptionIaNa);
        SetLength(sizeof(*this) - sizeof(Dhcp6Option));
    }

    /**
     * This method returns client IAID.
     *
     * @returns The client IAID.
     *
     */
    uint32_t GetIaid(void) const { return HostSwap32(mIaid); }

    /**
     * This method sets the client IAID.
     *
     * @param[in]  aIaId  The client IAID.
     *
     */
    void SetIaid(uint32_t aIaid) { mIaid = HostSwap32(aIaid); }

    /**
     * This method returns T1.
     *
     * @returns The value of T1.
     *
     */
    uint32_t GetT1(void) const { return HostSwap32(mT1); }

    /**
     * This method sets the value of T1.
     *
     * @param[in]  aT1  The value of T1.
     *
     */
    void SetT1(uint32_t aT1) { mT1 = HostSwap32(aT1); }

    /**
     * This method returns T2.
     *
     * @returns The value of T2.
     *
     */
    uint32_t GetT2(void) const { return HostSwap32(mT2); }

    /**
     * This method sets the value of T2.
     *
     * @param[in]  aT2  The value of T2.
     *
     */
    void SetT2(uint32_t aT2) { mT2 = HostSwap32(aT2); }

private:
    uint32_t mIaid; ///< IAID
    uint32_t mT1;   ///< T1
    uint32_t mT2;   ///< T2
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class IaAddress : public Dhcp6Option
{
public:
    /**
     * This method initializes the DHCPv6 Option.
     *
     */
    void Init(void)
    {
        SetCode(kOptionIaAddress);
        SetLength(sizeof(*this) - sizeof(Dhcp6Option));
    }

    /**
     * This method returns the pointer to the IPv6 address.
     *
     * @returns A pointer to the IPv6 address.
     *
     */
    Ip6::Address &GetAddress(void) { return mAddress; }

    /**
     * This method sets the IPv6 address.
     *
     * @param[in]  aAddress  The reference to the IPv6 address to set.
     *
     */
    void SetAddress(otIp6Address &aAddress) { memcpy(mAddress.mFields.m8, aAddress.mFields.m8, sizeof(otIp6Address)); }

    /**
     * This method returns the preferred lifetime of the IPv6 address.
     *
     * @returns The preferred lifetime of the IPv6 address.
     *
     */
    uint32_t GetPreferredLifetime(void) const { return HostSwap32(mPreferredLifetime); }

    /**
     * This method sets the preferred lifetime of the IPv6 address.
     *
     * @param[in]  aPreferredLifetime  The preferred lifetime of the IPv6 address.
     *
     */
    void SetPreferredLifetime(uint32_t aPreferredLifetime) { mPreferredLifetime = HostSwap32(aPreferredLifetime); }

    /**
     * This method returns the valid lifetime of the IPv6 address.
     *
     * @returns The valid lifetime of the IPv6 address.
     *
     */
    uint32_t GetValidLifetime(void) const { return HostSwap32(mValidLifetime); }

    /**
     * This method sets the valid lifetime of the IPv6 address.
     *
     * @param[in]  aValidLifetime  The valid lifetime of the IPv6 address.
     *
     */
    void SetValidLifetime(uint32_t aValidLifetime) { mValidLifetime = HostSwap32(aValidLifetime); }

private:
    Ip6::Address mAddress;           ///< IPv6 address
    uint32_t     mPreferredLifetime; ///< Preferred Lifetime
    uint32_t     mValidLifetime;     ///< Valid Lifetime
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class ElapsedTime : public Dhcp6Option
{
public:
    /**
     * This method initializes the DHCPv6 Option.
     *
     */
    void Init(void)
    {
        SetCode(kOptionElapsedTime);
        SetLength(sizeof(*this) - sizeof(Dhcp6Option));
    }

    /**
     * This method returns the elapsed time since solicit starts.
     *
     * @returns The elapsed time since solicit starts.
     *
     */
    uint16_t GetElapsedTime(void) const { return HostSwap16(mElapsedTime); }

    /**
     * This method sets the elapsed time since solicit starts.
     *
     * @param[in] aElapsedTime The elapsed time since solicit starts.
     *
     */
    void SetElapsedTime(uint16_t aElapsedTime) { mElapsedTime = HostSwap16(aElapsedTime); }

private:
    uint16_t mElapsedTime; ///< Elapsed time
} OT_TOOL_PACKED_END;

/**
 * Status Code
 *
 */
typedef enum Status
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
} Status;

OT_TOOL_PACKED_BEGIN
class StatusCode : public Dhcp6Option
{
public:
    /**
     * This method initializes the DHCPv6 Option.
     *
     */
    void Init(void)
    {
        SetCode(kOptionStatusCode);
        SetLength(sizeof(*this) - sizeof(Dhcp6Option));
    }

    /**
     * This method returns the status code.
     *
     * @returns The status code.
     *
     */
    Status GetStatusCode(void) const { return static_cast<Status>(HostSwap16(mStatus)); }

    /**
     * This method sets the status code.
     *
     * @param[in] aStatus The status code.
     *
     */
    void SetStatusCode(Status aStatus) { mStatus = HostSwap16(static_cast<uint16_t>(aStatus)); }

private:
    uint16_t mStatus; ///< Status Code
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class RapidCommit : public Dhcp6Option
{
public:
    /**
     * This method initializes the DHCPv6 Option.
     *
     */
    void Init(void)
    {
        SetCode(kOptionRapidCommit);
        SetLength(sizeof(*this) - sizeof(Dhcp6Option));
    }
} OT_TOOL_PACKED_END;

} // namespace Dhcp6
} // namespace ot

#endif // DHCP6_HPP_
