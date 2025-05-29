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

#ifndef DHCP6_TYPES_HPP_
#define DHCP6_TYPES_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE || OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE

#include "common/clearable.hpp"
#include "common/debug.hpp"
#include "common/equatable.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "mac/mac_types.hpp"

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

constexpr uint16_t kDhcpClientPort = 546; ///< DHCP Client port number.
constexpr uint16_t kDhcpServerPort = 547; ///< DHCP Server port number.

/**
 * DHCPv6 Message Types.
 */
enum MsgType : uint8_t
{
    kMsgTypeSolicit            = 1,  ///< Solicit message (client sends to locate servers).
    kMsgTypeAdvertise          = 2,  ///< Advertise message (server sends to indicate it is available).
    kMsgTypeRequest            = 3,  ///< Request message (client sends to request config parameters).
    kMsgTypeConfirm            = 4,  ///< Confirm message (client sends to determine if addresses are still valid).
    kMsgTypeRenew              = 5,  ///< Renew message (client sends to extend lifetime).
    kMsgTypeRebind             = 6,  ///< Rebind message (client sends to extend leases or update config).
    kMsgTypeReply              = 7,  ///< Reply message (server sends to reply to client)
    kMsgTypeRelease            = 8,  ///< Release message (client sends to release assigned leases).
    kMsgTypeDecline            = 9,  ///< Decline message (client sends to decline one or more addresses).
    kMsgTypeReconfigure        = 10, ///< Reconfigure message (server sends to inform of new config).
    kMsgTypeInformationRequest = 11, ///< Information-request message (client sends to request without lease).
    kMsgTypeRelayForward       = 12, ///< Relay-forward message (sent by a relay agent).
    kMsgTypeRelayReply         = 13, ///< Relay-reply message (sent by a relay agent).
    kMsgTypeLeaseQuery         = 14, ///< Lease query message (sent to server to obtain info about a client lease).
    kMsgTypeLeaseQueryReply    = 15, ///< Lease query reply message (server sends to reply to lease query).
};

/**
 * Represents a DHCPv6 transaction identifier.
 */
OT_TOOL_PACKED_BEGIN
class TransactionId : public Equatable<TransactionId>, public Clearable<TransactionId>
{
public:
    /**
     * Generates a cryptographically secure random sequence to populate the transaction identifier.
     */
    void GenerateRandom(void) { SuccessOrAssert(Random::Crypto::Fill(m8)); }

private:
    static constexpr uint16_t kSize = 3;

    uint8_t m8[kSize];
} OT_TOOL_PACKED_END;

/**
 * Represents a DHCPv6 header.
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
    uint8_t GetMsgType(void) const { return mMsgType; }

    /**
     * Sets the DHCPv6 message type.
     *
     * @param[in]  aType  The DHCPv6 message type.
     */
    void SetMsgType(MsgType aType) { mMsgType = aType; }

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
    uint8_t       mMsgType;
    TransactionId mTransactionId;
} OT_TOOL_PACKED_END;

/**
 * Represents a DHCPv6 option.
 */
OT_TOOL_PACKED_BEGIN
class Option
{
public:
    /**
     * Represents the DHCPv6 Option Codes.
     */
    enum Code : uint16_t
    {
        kClientId                  = 1,  ///< Client Identifier Option.
        kServerId                  = 2,  ///< Server Identifier Option.
        kIaNa                      = 3,  ///< Identity Association for Non-temporary Addresses Option.
        kIaTa                      = 4,  ///< Identity Association for Temporary Addresses Option.
        kIaAddress                 = 5,  ///< Identity Association Address Option.
        kRequestOption             = 6,  ///< Option Request Option.
        kPreference                = 7,  ///< Preference Option.
        kElapsedTime               = 8,  ///< Elapsed Time Option.
        kRelayMessage              = 9,  ///< Relay Message Option.
        kAuthentication            = 11, ///< Authentication Option.
        kServerUnicast             = 12, ///< Server Unicast Option.
        kStatusCode                = 13, ///< Status Code Option.
        kRapidCommit               = 14, ///< Rapid Commit Option.
        kUserClass                 = 15, ///< User Class Option.
        kVendorClass               = 16, ///< Vendor Class Option.
        kVendorSpecificInformation = 17, ///< Vendor-specific Information Option.
        kInterfaceId               = 18, ///< Interface-Id Option.
        kReconfigureMessage        = 19, ///< Reconfigure Message Option.
        kReconfigureAccept         = 20, ///< Reconfigure Accept Option.
        kIaPd                      = 25, ///< Identity Association for Prefix Delegation Option.
        kIaPrefix                  = 26, ///< IA Prefix Option.
        kLeaseQuery                = 44, ///< Lease Query Option.
        kClientData                = 45, ///< Client Data Option.
        kClientLastTransactionTime = 46, ///< Client Last Transaction Time Option.
    };

    /**
     * Returns the DHCPv6 option code.
     *
     * @returns The DHCPv6 option code.
     */
    uint16_t GetCode(void) const { return BigEndian::HostSwap16(mCode); }

    /**
     * Sets the DHCPv6 option code.
     *
     * @param[in]  aCode  The DHCPv6 option code.
     */
    void SetCode(Code aCode) { mCode = BigEndian::HostSwap16(aCode); }

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

    /**
     * Returns the total size of DHCPv6 option in bytes.
     *
     * @returns The size of option in bytes (which includes the Code and Length fields).
     */
    uint32_t GetSize(void) const { return GetLength() + sizeof(Option); }

    /**
     * Finds the first DHCPv6 option with a given code in a message.
     *
     * This method searches the message starting from `aMessage.GetOffset()` to the end.
     *
     * @param[in]  aMessage            The message to search.
     * @param[in]  aCode               The option code to find.
     * @param[out] aOptionOffsetRange  On success, is updated to contain the offset range of the found option.
     *
     * @retval kErrorNone      The option was found successfully.
     * @retval kErrorNotFound  The option was not found in the message.
     * @retval kErrorParse     The message is malformed and cannot be parsed.
     */
    static Error FindOption(const Message &aMessage, Code aCode, OffsetRange &aOptionOffsetRange);

    /**
     * Finds the first DHCPv6 option with a given code within a specified range of a message.
     *
     * @param[in]  aMessage            The message to search.
     * @param[in]  aMsgOffsetRange     The specific range within `aMessage` to search.
     * @param[in]  aCode               The option code to find.
     * @param[out] aOptionOffsetRange  On success, is updated to contain the offset range of the found option.
     *
     * @retval kErrorNone      The option was found successfully.
     * @retval kErrorNotFound  The option was not found in the given message range.
     * @retval kErrorParse     The message is malformed and cannot be parsed.
     */
    static Error FindOption(const Message     &aMessage,
                            const OffsetRange &aMsgOffsetRange,
                            Code               aCode,
                            OffsetRange       &aOptionOffsetRange);

    /**
     * Updates the Option length in a message.
     *
     * This method should be called after all option contents are appended to the message. It uses the current
     * message length along with @p aOffset to determine the option length and then updates it within the @p aMessage.
     * The @p aOffset should point to to the start of the option in @p aMessage.
     *
     * @param[in] aMessage   The message to update.
     * @param[in] aOffset    The offset to the start of `Option` in @p aMessage.
     */
    static void UpdateOptionLengthInMessage(Message &aMessage, uint16_t aOffset);

private:
    uint16_t mCode;
    uint16_t mLength;
} OT_TOOL_PACKED_END;

/**
 * DHCPv6 Unique Identifier (DUID) Type.
 */
enum DuidType : uint16_t
{
    kDuidLinkLayerAddressPlusTime = 1, ///< Link-layer address plus time (DUID-LLT).
    kDuidEnterpriseNumber         = 2, ///< Vendor-assigned unique ID based on Enterprise Number (DUID-EN).
    kDuidLinkLayerAddress         = 3, ///< Link-layer address (DUID-LL).
    kDuidUniversallyUniqueId      = 4, ///< Universally Unique Identifier (DUID-UUID).
};

/**
 * DHCPv6 Unique Identifier (DUID) Hardware Type.
 */
enum HardwareType : uint16_t
{
    kHardwareTypeEthernet = 1,  ///< Ethernet HW Type.
    kHardwareTypeEui64    = 27, ///< EUI64 HW Type.
};

/**
 * Represents a Client Identifier Option.
 */
OT_TOOL_PACKED_BEGIN
class ClientIdOption : public Option
{
public:
    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void) { SetCode(kClientId), SetLength(sizeof(*this) - sizeof(Option)); }

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
     * Returns the client DUID HardwareType.
     *
     * @returns The client DUID HardwareType.
     */
    uint16_t GetDuidHardwareType(void) const { return BigEndian::HostSwap16(mDuidHardwareType); }

    /**
     * Sets the client DUID HardwareType.
     *
     * @param[in]  aHardwareType  The client DUID HardwareType.
     */
    void SetDuidHardwareType(uint16_t aHardwareType) { mDuidHardwareType = BigEndian::HostSwap16(aHardwareType); }

    /**
     * Returns the client link-layer address.
     *
     * @returns The link-layer address.
     */
    const Mac::ExtAddress &GetDuidLinkLayerAddress(void) const { return mDuidLinkLayerAddress; }

    /**
     * Sets the client LinkLayerAddress.
     *
     * @param[in]  aAddress  The client LinkLayerAddress.
     */
    void SetDuidLinkLayerAddress(const Mac::ExtAddress &aAddress) { mDuidLinkLayerAddress = aAddress; }

private:
    uint16_t        mDuidType;
    uint16_t        mDuidHardwareType;
    Mac::ExtAddress mDuidLinkLayerAddress;
} OT_TOOL_PACKED_END;

/**
 * Represents a Server Identifier Option.
 */
OT_TOOL_PACKED_BEGIN
class ServerIdOption : public Option
{
public:
    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void) { SetCode(kServerId), SetLength(sizeof(*this) - sizeof(Option)); }

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
     * Sets the server DUID Hardware Type.
     *
     * @param[in]  aHardwareType  The server DUID HardwareType.
     */
    void SetDuidHardwareType(uint16_t aHardwareType) { mDuidHardwareType = BigEndian::HostSwap16(aHardwareType); }

    /**
     * Returns the server link-layer address.
     *
     * @returns The link-layer address.
     */
    const Mac::ExtAddress &GetDuidLinkLayerAddress(void) const { return mDuidLinkLayerAddress; }

    /**
     * Sets the server link-layer address.
     *
     * @param[in]  aAddress  The server link-layer address.
     */
    void SetDuidLinkLayerAddress(const Mac::ExtAddress &aAddress) { mDuidLinkLayerAddress = aAddress; }

private:
    uint16_t        mDuidType;
    uint16_t        mDuidHardwareType;
    Mac::ExtAddress mDuidLinkLayerAddress;
} OT_TOOL_PACKED_END;

/**
 * Represents an Identity Association for Non-temporary Address DHCPv6 Option.
 */
OT_TOOL_PACKED_BEGIN
class IaNaOption : public Option
{
public:
    static constexpr uint32_t kDefaultT1 = 0xffffffffU; ///< Default T1 value.
    static constexpr uint32_t kDefaultT2 = 0xffffffffU; ///< Default T2 value.

    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void) { SetCode(kIaNa), SetLength(sizeof(*this) - sizeof(Option)); }

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
 * Represents an Identity Association Address DHCPv6 Option.
 */
OT_TOOL_PACKED_BEGIN
class IaAddressOption : public Option
{
public:
    static constexpr uint32_t kDefaultPreferredLifetime = 0xffffffffU; ///< Default preferred lifetime.
    static constexpr uint32_t kDefaultValidLifetime     = 0xffffffffU; ///< Default valid lifetime.

    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void) { SetCode(kIaAddress), SetLength(sizeof(*this) - sizeof(Option)); }

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
 * Represents an Elapsed Time DHCPv6 Option.
 */
OT_TOOL_PACKED_BEGIN
class ElapsedTimeOption : public Option
{
public:
    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void) { SetCode(kElapsedTime), SetLength(sizeof(*this) - sizeof(Option)); }

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
 * Represents an Status Code DHCPv6 Option.
 */
OT_TOOL_PACKED_BEGIN
class StatusCodeOption : public Option
{
public:
    /**
     * Status Code.
     */
    enum Status : uint16_t
    {
        kSuccess          = 0,  ///< Success.
        kUnspecFail       = 1,  ///< Failure, reason unspecified.
        kNoAddrsAvail     = 2,  ///< No addresses available.
        kNoBinding        = 3,  ///< Client record (binding) unavailable.
        kNotOnLink        = 4,  ///< The prefix is not appropriate for the link.
        kUseMulticast     = 5,  ///< Force the client to send messages using All-DHCP multicast address.
        kNoPrefixAvail    = 6,  ///< Server has no prefixes available to assign.
        kUnknownQueryType = 7,  ///< The query-type is unknown to or not supported by the server.
        kMalformedQuery   = 8,  ///< The query is not valid.
        kNotConfigured    = 9,  ///< The server does not have the target address or link in its configuration.
        kNotAllowed       = 10, ///< The server does not allow the requestor to issue this LEASEQUERY.
    };

    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void) { SetCode(kStatusCode), SetLength(sizeof(*this) - sizeof(Option)); }

    /**
     * Returns the status code.
     *
     * @returns The status code.
     */
    uint16_t GetStatusCode(void) const { return BigEndian::HostSwap16(mStatus); }

    /**
     * Sets the status code.
     *
     * @param[in] aStatus The status code.
     */
    void SetStatusCode(Status aStatus) { mStatus = BigEndian::HostSwap16(aStatus); }

    /**
     * Reads the status code from a DHCPv6 message.
     *
     * This method searches the message (starting from `aMessage.GetOffset()` to the end) for a Status Code option. Per
     * RFC 8415, the absence of a Status Code option implies success. Therefore, if no Status Code option is found,
     * this method returns `kSuccess`.
     *
     * @param[in] aMessage The message to read the status code from.
     *
     * @returns The status code from the first found Status Code option, or `kSuccess` if none is found.
     */
    static Status ReadStatusFrom(const Message &aMessage);

    /**
     * Reads the status code from a specified range within a DHCPv-6 message.
     *
     * This method searches for a Status Code option only within the given `aMsgOffsetRange`. If no Status Code
     * option is found within the range, it is considered a success, and `kSuccess` is returned.
     *
     * @param[in] aMessage         The message to read the status code from.
     * @param[in] aMsgOffsetRange  The specific range within `aMessage` to search.
     *
     * @returns The status code from the first found Status Code option within the range, or `kSuccess` if none is
     *          found.
     */
    static Status ReadStatusFrom(const Message &aMessage, const OffsetRange &aMsgOffsetRange);

private:
    uint16_t mStatus;
} OT_TOOL_PACKED_END;

/**
 * Implements Rapid Commit DHCPv6 Option generation and parsing.
 */
class RapidCommitOption
{
public:
    static constexpr uint16_t kCode = Option::kRapidCommit; ///< Rapid Commit Option code.

    /*
     * Searches in a given message for Rapid Commit Option.
     *
     * @param[in] aMessage   The message to search in.
     *
     * @retval kErrorNone       The option was found successfully.
     * @retval kErrorNotFound   Did not find the option in @p aMessage.
     * @retval kErrorParse      Failed to parse the options in @p aMessage (invalid format).
     */
    static Error FindIn(const Message &aMessage);

    /**
     * Append a Rapid Commit Option to a message.
     *
     * The Rapid Commit Option contains no data fields (zero length).
     *
     * @param[in,out] aMessage   The message to append to.
     *
     * @retval kErrorNone    Successfully appended the option.
     * @retval kErrorNoBufs  Insufficient available buffers to grow the message.
     */
    static Error AppendTo(Message &aMessage);
};

/**
 * @}
 */

} // namespace Dhcp6
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE || OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE

#endif // DHCP6_TYPES_HPP_
