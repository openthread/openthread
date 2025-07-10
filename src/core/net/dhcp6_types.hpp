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
    kMsgTypeNone               = 0,  ///< Unused message type (reserved).
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
        kOptionRequest             = 6,  ///< Option Request Option.
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
        kSolMaxRt                  = 82, ///< SOL_MAX_RT Option (Max Solicit timeout value).
    };

    /**
     * Represents an iterator for searching for and iterating over DHCPv6 options with a specific code within a message.
     */
    class Iterator : private Clearable<Iterator>
    {
        friend class Clearable<Iterator>;

    public:
        /**
         * This is an iterator constructor that initializes the iterator to a cleared (invalid) state.
         *
         * An iterator in this state must be initialized using one of the `Init()` methods before use.
         */
        Iterator(void) { Clear(); }

        /**
         * Initializes the iterator and finds the first matching option within an entire message.
         *
         * The search is performed from `aMessage.GetOffset()` to the end of the message.
         *
         * @param[in] aMessage  The message to search in.
         * @param[in] aCode     The option code to search for.
         */
        void Init(const Message &aMessage, Code aCode);

        /**
         * Initializes the iterator and finds the first matching option within a specific range of a message.
         *
         * @param[in] aMessage          The message to search in.
         * @param[in] aMsgOffsetRange   The specific range within @p aMessage to search.
         * @param[in] aCode             The option code to search for.
         */
        void Init(const Message &aMessage, const OffsetRange &aMsgOffsetRange, Code aCode);

        /**
         * Indicates whether the iteration is complete.
         *
         * The iteration is considered done when all matching options have been iterated through, or if an error
         * occurred during iteration. The `GetError()` method can be used to get the error status.
         *
         * Particularly, `IsDone() && GetError() == kErrorNone` indicates a successful end of the iteration (i.e., no
         * more matching options were found).
         *
         * @returns `true` if the iteration is complete, `false` otherwise.
         */
        bool IsDone(void) const { return mIsDone; }

        /**
         * Advances the iterator to the next matching option.
         */
        void Advance(void);

        /**
         * Gets the offset range of the current option matched by the iterator.
         *
         * The returned offset range refers to the matched option in the message when the iterator is not done
         * (`IsDone()` is `false`). Otherwise, an empty offset range is returned.
         *
         * @returns The `OffsetRange` of the current option.
         */
        const OffsetRange &GetOptionOffsetRange(void) const { return mOptionOffsetRange; }

        /**
         * Gets any error that occurred during the iteration.
         *
         * @retval kErrorNone           Successfully iterated over options (so far).
         * @retval kErrorParse          The options in the message were malformed and failed to parse.
         * @retval kErrorInvalidState   The iterator was not initialized.
         */
        Error GetError(void) const { return mError; }

    private:
        const Message *mMessage;
        OffsetRange    mMsgOffsetRange;
        OffsetRange    mOptionOffsetRange;
        Code           mCode;
        Error          mError;
        bool           mIsDone;
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

    /**
     * Appends a DHCPv6 Option with a given code and data to a message.
     *
     * @param[in,out]  aMessage            The message to append to.
     * @param[in]      aCode               The option code to append.
     * @param[in]      aData               A pointer to buffer containing the option data to append.
     * @param[in]      aDataLength         The length of @p aData (in bytes).
     *
     * @retval kErrorNone     Successfully appended the Option to the message.
     * @retval kErrorNoBufs   Insufficient available buffers to grow the message.
     */
    static Error AppendOption(Message &aMessage, Code aCode, const void *aData, uint16_t aDataLength);

private:
    uint16_t mCode;
    uint16_t mLength;
} OT_TOOL_PACKED_END;

/**
 * Defines constants and types for DHCPv6 DUID (DHCP Unique Identifier).
 */
class Duid
{
public:
    static constexpr uint16_t kMinSize = sizeof(uint16_t) + 1;   ///< Minimum size of DUID in bytes.
    static constexpr uint16_t kMaxSize = sizeof(uint16_t) + 128; ///< Maximum size of DUID in bytes.

    /**
     * DHCPv6 Unique Identifier (DUID) Type.
     */
    enum Type : uint16_t
    {
        kTypeLinkLayerAddressPlusTime = 1, ///< Link-layer address plus time (DUID-LLT).
        kTypeVendorAssigned           = 2, ///< Vendor-assigned unique ID based on Enterprise Number (DUID-EN).
        kTypeLinkLayerAddress         = 3, ///< Link-layer address (DUID-LL).
        kTypeUniversallyUniqueId      = 4, ///< Universally Unique Identifier (DUID-UUID).
    };

    /**
     * DHCPv6 Unique Identifier (DUID) Hardware Type.
     */
    enum HardwareType : uint16_t
    {
        kHardwareTypeEthernet = 1,  ///< Ethernet HW Type.
        kHardwareTypeEui64    = 27, ///< EUI64 HW Type.
    };

private:
    Duid(void) = delete;
};

/**
 * Represents a DHCPv6 DUID based on EUI64 Link-layer address (DUID-LL).
 */
OT_TOOL_PACKED_BEGIN
class Eui64Duid
{
public:
    /**
     * Initializes the DUID-LL from a given Extended Address.
     *
     * @param[in] aExtAddress   The Extended Address.
     */
    void Init(const Mac::ExtAddress &aExtAddress);

    /**
     * Indicates whether or not the DUID-LL is valid, i.e. using the correct DUID type (`kTypeLinkLayerAddress`) and
     * Hardware Type `kHardwareTypeEui64`.
     *
     * @returns TRUE   The DUID is valid.
     * @returns FALSE  The DUID is not valid.
     */
    bool IsValid(void) const;

    /**
     * Gets the Link-layer address.
     *
     * @returns The Link-layer address.
     */
    const Mac::ExtAddress &GetLinkLayerAddress(void) const { return mLinkLayerAddress; }

private:
    uint16_t GetType(void) const { return BigEndian::HostSwap16(mDuidType); }
    void     SetType(Duid::Type aDuidType) { mDuidType = BigEndian::HostSwap16(aDuidType); }
    uint16_t GetHardwareType(void) const { return BigEndian::HostSwap16(mHardwareType); }
    void     SetHardwareType(uint16_t aHardwareType) { mHardwareType = BigEndian::HostSwap16(aHardwareType); }

    uint16_t        mDuidType;
    uint16_t        mHardwareType;
    Mac::ExtAddress mLinkLayerAddress;
} OT_TOOL_PACKED_END;

/**
 *  Implements parsing and generation of Client/Server Identifier Options.
 */
class IdOption
{
protected:
    IdOption(void) = delete;
    static Error Read(Option::Code aCode, const Message &aMessage, OffsetRange &aDuidOffsetRange);
    static Error ReadEui64(Option::Code aCode, const Message &aMessage, Mac::ExtAddress &aExtAddress);
    static Error MatchesEui64(Option::Code aCode, const Message &aMessage, const Mac::ExtAddress &aExtAddress);
    static Error Append(Option::Code aCode, Message &aMessage, const void *aDuid, uint16_t aDuidLength);
    static Error AppendEui64(Option::Code aCode, Message &aMessage, const Mac::ExtAddress &aExtAddress);
};

/**
 * Implements Client Identifier Option generation and parsing.
 */
class ClientIdOption : private IdOption
{
public:
    /**
     * Searches and reads the Client ID option from a DHCPv6 message, validating that it is a DUID based on an
     * EUI-64 Link-Layer address (DUID-LL).
     *
     * @param[in]  aMessage     The message to search and read the Client ID option from.
     * @param[out] aExtAddress  A reference to populate with the EUI-64 link-layer address on a successful read.
     *
     * @retval kErrorNone      Successfully read the Client ID as a DUID-LL. @p aExtAddress is updated.
     * @retval kErrorNotFound  The Client ID option was not found in the message.
     * @retval kErrorParse     The message is malformed, or the option was found but is not a valid DUID-LL format.
     */
    static Error ReadAsEui64Duid(const Message &aMessage, Mac::ExtAddress &aExtAddress)
    {
        return IdOption::ReadEui64(Option::kClientId, aMessage, aExtAddress);
    }

    /**
     * Appends a Client Identifier option to a DHCPv6 message.
     *
     * The appended option uses the EUI-64 Link-Layer address (DUID-LL) format.
     *
     * @param[in,out] aMessage      The message to which to append the Client ID option.
     * @param[in]     aExtAddress   The EUI-64 address to use for creating the DUID.
     *
     * @retval kErrorNone     Successfully appended the Client ID option.
     * @retval kErrorNoBufs   Insufficient available buffers to grow the message.
     */
    static Error AppendWithEui64Duid(Message &aMessage, const Mac::ExtAddress &aExtAddress)
    {
        return IdOption::AppendEui64(Option::kClientId, aMessage, aExtAddress);
    }

    /**
     * Checks if the Client Identifier option in a DHCPv6 message matches a given EUI-64 address.
     *
     * This method searches for the Client ID option, verifies it is a DUID-LL, and checks if its EUI-64 value
     * matches the given extended address.
     *
     * @param[in] aMessage     The message containing the Client ID option to check.
     * @param[in] aExtAddress  The EUI-64 address to compare against.
     *
     * @retval kErrorNone      The Client ID option was found, is a valid DUID-LL, and matches @p aExtAddress.
     * @retval kErrorNotFound  The Client ID option was not found or did not match the EUI-64 address.
     * @retval kErrorParse     The message is malformed, or the option was found but is not a valid DUID-LL format.
     */
    static Error MatchesEui64Duid(const Message &aMessage, const Mac::ExtAddress &aExtAddress)
    {
        return IdOption::MatchesEui64(Option::kClientId, aMessage, aExtAddress);
    }
};

/**
 * Implements Server Identifier Option generation and parsing.
 */
class ServerIdOption : private IdOption
{
public:
    /**
     * Searches and reads the raw DUID from the Server Identifier option in a DHCPv6 message.
     *
     * This method does not interpret the DUID type. It validates that DUID has the expected minimum DUID size. It
     * returns the `OffsetRange` corresponding to DUID in the message.
     *
     * @param[in]  aMessage          The message to search and read the Server ID option from.
     * @param[out] aDuidOffsetRange  On success, is updated to return the offset range of the Server DUID.
     *
     * @retval kErrorNone      Successfully read the Server ID option. @p aDuidOffsetRange is updated.
     * @retval kErrorNotFound  The Server ID option was not found in the message.
     * @retval kErrorParse     The message is malformed and cannot be parsed.
     */
    static Error ReadDuid(const Message &aMessage, OffsetRange &aDuidOffsetRange)
    {
        return IdOption::Read(Option::kServerId, aMessage, aDuidOffsetRange);
    }

    /**
     * Searches and reads the Server ID option, validating that it is a DUID based on an EUI-64 Link-Layer address
     * (DUID-LL).
     *
     * @param[in]  aMessage     The message to search and read the Server ID option from.
     * @param[out] aExtAddress  A reference to populate with the EUI-64 link-layer address on a successful read.
     *
     * @retval kErrorNone      Successfully read the Server ID as a DUID-LL. @p aExtAddress is updated.
     * @retval kErrorNotFound  The Server ID option was not found in the message.
     * @retval kErrorParse     The message is malformed, or the option was found but is not a valid DUID-LL format.
     */
    static Error ReadAsEui64Duid(const Message &aMessage, Mac::ExtAddress &aExtAddress)
    {
        return IdOption::ReadEui64(Option::kServerId, aMessage, aExtAddress);
    }

    /**
     * Appends a Server Identifier option to a DHCPv6 message with a given raw DUID.
     *
     * @param[in,out] aMessage      The message to which to append the Server ID option.
     * @param[in]     aDuid         A pointer to a buffer containing the DUID bytes.
     * @param[in]     aDuidLength   The DUID length in bytes.
     *
     * @retval kErrorNone     Successfully appended the Server ID option.
     * @retval kErrorNoBufs   Insufficient available buffers to grow the message.
     */
    static Error AppendWithDuid(Message &aMessage, const void *aDuid, uint16_t aDuidLength)
    {
        return IdOption::Append(Option::kServerId, aMessage, aDuid, aDuidLength);
    }

    /**
     * Appends a Server Identifier option to a DHCPv6 message.
     *
     * The appended option uses the DUID based on an EUI-64 Link-Layer address (DUID-LL) format.
     *
     * @param[in,out] aMessage      The message to which to append the Server ID option.
     * @param[in]     aExtAddress   The EUI-64 address to use for creating the DUID.
     *
     * @retval kErrorNone     Successfully appended the Server ID option.
     * @retval kErrorNoBufs   Insufficient available buffers to grow the message.
     */
    static Error AppendWithEui64Duid(Message &aMessage, const Mac::ExtAddress &aExtAddress)
    {
        return IdOption::AppendEui64(Option::kServerId, aMessage, aExtAddress);
    }
};

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
 * Represents a Preference DHCPv6 Option.
 */
OT_TOOL_PACKED_BEGIN
class PreferenceOption : public Option
{
public:
    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void) { SetCode(kPreference), SetLength(sizeof(*this) - sizeof(Option)); }

    /**
     * Returns the preference value.
     *
     * @returns The preference value. Higher value is preferred.
     */
    uint8_t GetPreference(void) const { return mPreference; }

    /**
     * Sets the preference.
     *
     * @param[in] aPreference  The preference value.
     */
    void SetPreference(uint8_t aPreference) { mPreference = aPreference; }

private:
    uint8_t mPreference;
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
     * Returns the elapsed time.
     *
     * @returns The elapsed time (in unit of hundredths of a second).
     */
    uint16_t GetElapsedTime(void) const { return BigEndian::HostSwap16(mElapsedTime); }

    /**
     * Sets the elapsed time.
     *
     * @param[in] aElapsedTime  The elapsed time (in unit of hundredths of a second).
     */
    void SetElapsedTime(uint16_t aElapsedTime) { mElapsedTime = BigEndian::HostSwap16(aElapsedTime); }

    /**
     * Append an Elapsed Time Option to a message.
     *
     * @param[in,out] aMessage       The message to append to.
     * @param[in]     aElapsedTime   The elapsed time (in unit of hundredths of a second).
     *
     * @retval kErrorNone    Successfully appended the option.
     * @retval kErrorNoBufs  Insufficient available buffers to grow the message.
     */
    static Error AppendTo(Message &aMessage, uint16_t aElapsedTime);

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
 * Represents an Identity Association for Prefix Delegation Option.
 */
OT_TOOL_PACKED_BEGIN
class IaPdOption : public Option
{
public:
    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void) { SetCode(kIaPd), SetLength(sizeof(*this) - sizeof(Option)); }

    /**
     * Returns IAID.
     *
     * @returns The IAID.
     */
    uint32_t GetIaid(void) const { return BigEndian::HostSwap32(mIaid); }

    /**
     * Sets the IAID.
     *
     * @param[in]  aIaid  The IAID.
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
    // Followed by sub-options
} OT_TOOL_PACKED_END;

/**
 * Represents an Identity Association Prefix Option.
 */
OT_TOOL_PACKED_BEGIN
class IaPrefixOption : public Option
{
public:
    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void) { SetCode(kIaPrefix), SetLength(sizeof(*this) - sizeof(Option)); }

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

    /**
     * Returns the prefix length in bits.
     *
     * @returns The prefix length in bits.
     */
    uint8_t GetPrefixLength(void) const { return mPrefixLength; }

    /**
     * Reads the prefix and its length from the option.
     *
     * @param[out] aPrefix  A reference to an `Ip6::Prefix` to return the prefix and its length.
     */
    void GetPrefix(Ip6::Prefix &aPrefix) const;

    /**
     * Sets the prefix and its length in the option.
     *
     * @param[in] aPrefix  An IPv6 prefix.
     */
    void SetPrefix(const Ip6::Prefix &aPrefix);

private:
    uint32_t     mPreferredLifetime;
    uint32_t     mValidLifetime;
    uint8_t      mPrefixLength;
    Ip6::Address mPrefix;
    // Can be followed by sub-options.
} OT_TOOL_PACKED_END;

/**
 * Represents a Server Unicast Option.
 */
OT_TOOL_PACKED_BEGIN
class ServerUnicastOption : public Option
{
public:
    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void) { SetCode(kServerUnicast), SetLength(sizeof(*this) - sizeof(Option)); }

    /**
     * Returns the server IPv6 address.
     *
     * @returns the server IPv6 address.
     */
    const Ip6::Address &GetServerAddress(void) const { return mServerAddress; }

    /**
     * Sets the server IPv6 address.
     *
     * @param[in] aServerAddress  The server IPv6 address.
     */
    void SetServerAddress(const Ip6::Address &aServerAddress) { mServerAddress = aServerAddress; }

private:
    Ip6::Address mServerAddress;
} OT_TOOL_PACKED_END;

/**
 * Represents an SOL_MAX_RT Option (Max Solicit timeout value).
 */
OT_TOOL_PACKED_BEGIN
class SolMaxRtOption : public Option
{
public:
    static constexpr uint32_t kMinSolMaxRt = 60;    ///< Minimum SOL_MAX_RT value.
    static constexpr uint32_t kMaxSolMaxRt = 86400; ///< Maximum SOL_MAX_RT value.

    /**
     * Initializes the DHCPv6 Option.
     */
    void Init(void) { SetCode(kSolMaxRt), SetLength(sizeof(*this) - sizeof(Option)); }

    /**
     * Returns the SOL_MAX_RT value.
     *
     * @returns The SOL_MAX_RT value (in seconds).
     */
    uint32_t GetSolMaxRt(void) const { return BigEndian::HostSwap32(mSolMaxRt); }

    /**
     * Sets the SOL_MAX_RT.
     *
     * @param[in] aSolMaxRt  The SOL_MAX_RT value (in seconds).
     */
    void SetSolMaxRt(uint32_t aSolMaxRt) { mSolMaxRt = BigEndian::HostSwap32(aSolMaxRt); }

private:
    uint32_t mSolMaxRt;
} OT_TOOL_PACKED_END;

/**
 * @}
 */

} // namespace Dhcp6
} // namespace ot

#endif // DHCP6_TYPES_HPP_
