/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for IPv6 Neighbor Discovery (ND).
 *
 * See RFC 4861 (https://tools.ietf.org/html/rfc4861) and RFC 4191 (https://tools.ietf.org/html/rfc4191).
 *
 */

#ifndef ND6_HPP_
#define ND6_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

#include <openthread/netdata.h>
#include <openthread/platform/toolchain.h>

#include "common/const_cast.hpp"
#include "common/encoding.hpp"
#include "common/equatable.hpp"
#include "net/icmp6.hpp"
#include "net/ip6.hpp"
#include "thread/network_data_types.hpp"

namespace ot {
namespace Ip6 {
namespace Nd {

typedef NetworkData::RoutePreference RoutePreference; ///< Route Preference

/**
 * Represents the variable length options in Neighbor Discovery messages.
 *
 * @sa PrefixInfoOption
 * @sa RouteInfoOption
 *
 */
OT_TOOL_PACKED_BEGIN
class Option
{
    friend class RouterAdvertMessage;

public:
    enum Type : uint8_t
    {
        kTypePrefixInfo       = 3,  ///< Prefix Information Option.
        kTypeRouteInfo        = 24, ///< Route Information Option.
        kTypeRaFlagsExtension = 26, ///< RA Flags Extension Option.
    };

    static constexpr uint16_t kLengthUnit = 8; ///< The unit of length in octets.

    /**
     * Gets the option type.
     *
     * @returns  The option type.
     *
     */
    uint8_t GetType(void) const { return mType; }

    /**
     * Sets the option type.
     *
     * @param[in] aType  The option type.
     *
     *
     */
    void SetType(Type aType) { mType = aType; }

    /**
     * Sets the length based on a given total option size in bytes.
     *
     * Th option must end on a 64-bit boundary, so the length is derived as `(aSize + 7) / 8 * 8`.
     *
     * @param[in]  aSize  The size of option in bytes.
     *
     */
    void SetSize(uint16_t aSize) { mLength = static_cast<uint8_t>((aSize + kLengthUnit - 1) / kLengthUnit); }

    /**
     * Returns the size of the option in bytes.
     *
     * @returns  The size of the option in bytes.
     *
     */
    uint16_t GetSize(void) const { return mLength * kLengthUnit; }

    /**
     * Sets the length of the option (in unit of 8 bytes).
     *
     * @param[in]  aLength  The length of the option in unit of 8 bytes.
     *
     */
    void SetLength(uint8_t aLength) { mLength = aLength; }

    /**
     * Returns the length of the option (in unit of 8 bytes).
     *
     * @returns  The length of the option in unit of 8 bytes.
     *
     */
    uint16_t GetLength(void) const { return mLength; }

    /**
     * Indicates whether or not this option is valid.
     *
     * @retval TRUE   The option is valid.
     * @retval FALSE  The option is not valid.
     *
     */
    bool IsValid(void) const { return mLength > 0; }

private:
    class Iterator : public Unequatable<Iterator>
    {
    public:
        Iterator(void);
        Iterator(const void *aStart, const void *aEnd);

        const Option &operator*(void) { return *mOption; }
        void          operator++(void) { Advance(); }
        void          operator++(int) { Advance(); }
        bool          operator==(const Iterator &aOther) const { return mOption == aOther.mOption; }

    private:
        static const Option *Next(const Option *aOption);
        void                 Advance(void);
        const Option        *Validate(const Option *aOption) const;

        const Option *mOption;
        const Option *mEnd;
    };

    uint8_t mType;   // Type of the option.
    uint8_t mLength; // Length of the option in unit of 8 octets, including the `mType` and `mLength` fields.
} OT_TOOL_PACKED_END;

/**
 * Represents the Prefix Information Option.
 *
 * See section 4.6.2 of RFC 4861 for definition of this option [https://tools.ietf.org/html/rfc4861#section-4.6.2]
 *
 */
OT_TOOL_PACKED_BEGIN
class PrefixInfoOption : public Option, private Clearable<PrefixInfoOption>
{
    friend class Clearable<PrefixInfoOption>;

public:
    static constexpr Type kType = kTypePrefixInfo; ///< Prefix Information Option Type.

    /**
     * Initializes the Prefix Info option with proper type and length and sets all other fields to zero.
     *
     */
    void Init(void);

    /**
     * Indicates whether or not the on-link flag is set.
     *
     * @retval TRUE  The on-link flag is set.
     * @retval FALSE The on-link flag is not set.
     *
     */
    bool IsOnLinkFlagSet(void) const { return (mFlags & kOnLinkFlagMask) != 0; }

    /**
     * Sets the on-link (L) flag.
     *
     */
    void SetOnLinkFlag(void) { mFlags |= kOnLinkFlagMask; }

    /**
     * Clears the on-link (L) flag.
     *
     */
    void ClearOnLinkFlag(void) { mFlags &= ~kOnLinkFlagMask; }

    /**
     * Indicates whether or not the autonomous address-configuration (A) flag is set.
     *
     * @retval TRUE  The auto address-config flag is set.
     * @retval FALSE The auto address-config flag is not set.
     *
     */
    bool IsAutoAddrConfigFlagSet(void) const { return (mFlags & kAutoConfigFlagMask) != 0; }

    /**
     * Sets the autonomous address-configuration (A) flag.
     *
     */
    void SetAutoAddrConfigFlag(void) { mFlags |= kAutoConfigFlagMask; }

    /**
     * Clears the autonomous address-configuration (A) flag.
     *
     */
    void ClearAutoAddrConfigFlag(void) { mFlags &= ~kAutoConfigFlagMask; }

    /**
     * Sets the valid lifetime of the prefix in seconds.
     *
     * @param[in]  aValidLifetime  The valid lifetime in seconds.
     *
     */
    void SetValidLifetime(uint32_t aValidLifetime) { mValidLifetime = BigEndian::HostSwap32(aValidLifetime); }

    /**
     * THis method gets the valid lifetime of the prefix in seconds.
     *
     * @returns  The valid lifetime in seconds.
     *
     */
    uint32_t GetValidLifetime(void) const { return BigEndian::HostSwap32(mValidLifetime); }

    /**
     * Sets the preferred lifetime of the prefix in seconds.
     *
     * @param[in]  aPreferredLifetime  The preferred lifetime in seconds.
     *
     */
    void SetPreferredLifetime(uint32_t aPreferredLifetime)
    {
        mPreferredLifetime = BigEndian::HostSwap32(aPreferredLifetime);
    }

    /**
     * THis method returns the preferred lifetime of the prefix in seconds.
     *
     * @returns  The preferred lifetime in seconds.
     *
     */
    uint32_t GetPreferredLifetime(void) const { return BigEndian::HostSwap32(mPreferredLifetime); }

    /**
     * Sets the prefix.
     *
     * @param[in]  aPrefix  The prefix contained in this option.
     *
     */
    void SetPrefix(const Prefix &aPrefix);

    /**
     * Gets the prefix in this option.
     *
     * @param[out] aPrefix   Reference to a `Prefix` to return the prefix.
     *
     */
    void GetPrefix(Prefix &aPrefix) const;

    /**
     * Indicates whether or not the option is valid.
     *
     * @retval TRUE  The option is valid
     * @retval FALSE The option is not valid.
     *
     */
    bool IsValid(void) const;

    PrefixInfoOption(void) = delete;

private:
    // Prefix Information Option
    //
    //   0                   1                   2                   3
    //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |     Type      |    Length     | Prefix Length |L|A| Reserved1 |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |                         Valid Lifetime                        |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |                       Preferred Lifetime                      |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |                           Reserved2                           |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |                                                               |
    //  +                                                               +
    //  |                                                               |
    //  +                            Prefix                             +
    //  |                                                               |
    //  +                                                               +
    //  |                                                               |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    static constexpr uint8_t kAutoConfigFlagMask = 0x40; // Autonomous address-configuration flag.
    static constexpr uint8_t kOnLinkFlagMask     = 0x80; // On-link flag.

    uint8_t  mPrefixLength;      // The prefix length in bits.
    uint8_t  mFlags;             // The flags field.
    uint32_t mValidLifetime;     // The valid lifetime of the prefix.
    uint32_t mPreferredLifetime; // The preferred lifetime of the prefix.
    uint32_t mReserved2;         // The reserved field.
    Address  mPrefix;            // The prefix.
} OT_TOOL_PACKED_END;

static_assert(sizeof(PrefixInfoOption) == 32, "invalid PrefixInfoOption structure");

/**
 * Represents the Route Information Option.
 *
 * See section 2.3 of RFC 4191 for definition of this option. [https://tools.ietf.org/html/rfc4191#section-2.3]
 *
 */
OT_TOOL_PACKED_BEGIN
class RouteInfoOption : public Option, private Clearable<RouteInfoOption>
{
    friend class Clearable<RouteInfoOption>;

public:
    static constexpr uint16_t kMinSize = kLengthUnit;    ///< Minimum size (in bytes) of a Route Info Option
    static constexpr Type     kType    = kTypeRouteInfo; ///< Route Information Option Type.

    /**
     * Initializes the option setting the type and clearing (setting to zero) all other fields.
     *
     */
    void Init(void);

    /**
     * Sets the route preference.
     *
     * @param[in]  aPreference  The route preference.
     *
     */
    void SetPreference(RoutePreference aPreference);

    /**
     * Gets the route preference.
     *
     * @returns  The route preference.
     *
     */
    RoutePreference GetPreference(void) const;

    /**
     * Sets the lifetime of the route in seconds.
     *
     * @param[in]  aLifetime  The lifetime of the route in seconds.
     *
     */
    void SetRouteLifetime(uint32_t aLifetime) { mRouteLifetime = BigEndian::HostSwap32(aLifetime); }

    /**
     * Gets Route Lifetime in seconds.
     *
     * @returns  The Route Lifetime in seconds.
     *
     */
    uint32_t GetRouteLifetime(void) const { return BigEndian::HostSwap32(mRouteLifetime); }

    /**
     * Sets the prefix and adjusts the option length based on the prefix length.
     *
     * @param[in]  aPrefix  The prefix contained in this option.
     *
     */
    void SetPrefix(const Prefix &aPrefix);

    /**
     * Gets the prefix in this option.
     *
     * @param[out] aPrefix   Reference to a `Prefix` to return the prefix.
     *
     */
    void GetPrefix(Prefix &aPrefix) const;

    /**
     * Tells whether this option is valid.
     *
     * @returns  A boolean indicates whether this option is valid.
     *
     */
    bool IsValid(void) const;

    /**
     * Calculates the minimum option length for a given prefix length.
     *
     * The option length (which is in unit of 8 octets) can be 1, 2, or 3 depending on the prefix length. It can be 1
     * for a zero prefix length, 2 if the prefix length is not greater than 64, and 3 otherwise.
     *
     * @param[in] aPrefixLength   The prefix length (in bits).
     *
     * @returns The option length (in unit of 8 octet) for @p aPrefixLength.
     *
     */
    static uint8_t OptionLengthForPrefix(uint8_t aPrefixLength);

    /**
     * Calculates the minimum option size (in bytes) for a given prefix length.
     *
     * @param[in] aPrefixLength   The prefix length (in bits).
     *
     * @returns The option size (in bytes) for @p aPrefixLength.
     *
     */
    static uint16_t OptionSizeForPrefix(uint8_t aPrefixLength)
    {
        return kLengthUnit * OptionLengthForPrefix(aPrefixLength);
    }

    RouteInfoOption(void) = delete;

private:
    // Route Information Option
    //
    //   0                   1                   2                   3
    //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |     Type      |    Length     | Prefix Length |Resvd|Prf|Resvd|
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |                        Route Lifetime                         |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |                   Prefix (Variable Length)                    |
    //  .                                                               .
    //  .                                                               .
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    static constexpr uint8_t kPreferenceOffset = 3;
    static constexpr uint8_t kPreferenceMask   = 3 << kPreferenceOffset;

    uint8_t       *GetPrefixBytes(void) { return AsNonConst(AsConst(this)->GetPrefixBytes()); }
    const uint8_t *GetPrefixBytes(void) const { return reinterpret_cast<const uint8_t *>(this) + sizeof(*this); }

    uint8_t  mPrefixLength;  // The prefix length in bits.
    uint8_t  mResvdPrf;      // The preference.
    uint32_t mRouteLifetime; // The lifetime in seconds.
    // Followed by prefix bytes (variable length).

} OT_TOOL_PACKED_END;

static_assert(sizeof(RouteInfoOption) == 8, "invalid RouteInfoOption structure");

/**
 * Represents an RA Flags Extension Option.
 *
 * See RFC-5175 [https://tools.ietf.org/html/rfc5175]
 *
 */
OT_TOOL_PACKED_BEGIN
class RaFlagsExtOption : public Option, private Clearable<RaFlagsExtOption>
{
    friend class Clearable<RaFlagsExtOption>;

public:
    static constexpr Type kType = kTypeRaFlagsExtension; ///< RA Flags Extension Option type.

    /**
     * Initializes the RA Flags Extension option with proper type and length and sets all flags to zero.
     *
     */
    void Init(void);

    /**
     * Tells whether this option is valid.
     *
     * @returns  A boolean indicates whether this option is valid.
     *
     */
    bool IsValid(void) const { return GetSize() >= sizeof(*this); }

    /**
     * Indicates whether or not the Stub Router Flag is set.
     *
     * @retval TRUE   The Stub Router Flag is set.
     * @retval FALSE  The Stub Router Flag is not set.
     *
     */
    bool IsStubRouterFlagSet(void) const { return (mFlags[0] & kStubRouterFlag) != 0; }

    /**
     * Sets the Stub Router Flag.
     *
     */
    void SetStubRouterFlag(void) { mFlags[0] |= kStubRouterFlag; }

    RaFlagsExtOption(void) = delete;

private:
    // RA Flags Extension Option
    //
    //   0                   1                   2                   3
    //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |     Type      |    Length     |         Bit fields available ..
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  ... for assignment                                              |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                                .

    // Stub router flags defined in [https://www.ietf.org/archive/id/draft-hui-stub-router-ra-flag-01.txt]

    static constexpr uint8_t kStubRouterFlag = 1 << 7;

    uint8_t mFlags[6];
} OT_TOOL_PACKED_END;

static_assert(sizeof(RaFlagsExtOption) == 8, "invalid RaFlagsExtOption structure");

/**
 * Represents a Router Advertisement message.
 *
 */
class RouterAdvertMessage
{
public:
    /**
     * Implements the RA message header.
     *
     * See section 2.2 of RFC 4191 [https://datatracker.ietf.org/doc/html/rfc4191]
     *
     */
    OT_TOOL_PACKED_BEGIN
    class Header : public Equatable<Header>, private Clearable<Header>
    {
        friend class Clearable<Header>;

    public:
        /**
         * Initializes the Router Advertisement message with
         * zero router lifetime, reachable time and retransmission timer.
         *
         */
        Header(void) { SetToDefault(); }

        /**
         * Sets the RA message to default values.
         *
         */
        void SetToDefault(void);

        /**
         * Sets the checksum value.
         *
         * @param[in]  aChecksum  The checksum value.
         *
         */
        void SetChecksum(uint16_t aChecksum) { mChecksum = BigEndian::HostSwap16(aChecksum); }

        /**
         * Sets the Router Lifetime in seconds.
         *
         * @param[in]  aRouterLifetime  The router lifetime in seconds.
         *
         */
        void SetRouterLifetime(uint16_t aRouterLifetime) { mRouterLifetime = BigEndian::HostSwap16(aRouterLifetime); }

        /**
         * Gets the Router Lifetime (in seconds).
         *
         * Router Lifetime set to zero indicates that the sender is not a default router.
         *
         * @returns  The router lifetime in seconds.
         *
         */
        uint16_t GetRouterLifetime(void) const { return BigEndian::HostSwap16(mRouterLifetime); }

        /**
         * Sets the default router preference.
         *
         * @param[in]  aPreference  The router preference.
         *
         */
        void SetDefaultRouterPreference(RoutePreference aPreference);

        /**
         * Gets the default router preference.
         *
         * @returns  The router preference.
         *
         */
        RoutePreference GetDefaultRouterPreference(void) const;

        /**
         * Indicates whether or not the Managed Address Config Flag is set in the RA message header.
         *
         * @retval TRUE   The Managed Address Config Flag is set.
         * @retval FALSE  The Managed Address Config Flag is not set.
         *
         */
        bool IsManagedAddressConfigFlagSet(void) const { return (mFlags & kManagedAddressConfigFlag) != 0; }

        /**
         * Sets the Managed Address Config Flag in the RA message.
         *
         */
        void SetManagedAddressConfigFlag(void) { mFlags |= kManagedAddressConfigFlag; }

        /**
         * Indicates whether or not the Other Config Flag is set in the RA message header.
         *
         * @retval TRUE   The Other Config Flag is set.
         * @retval FALSE  The Other Config Flag is not set.
         *
         */
        bool IsOtherConfigFlagSet(void) const { return (mFlags & kOtherConfigFlag) != 0; }

        /**
         * Sets the Other Config Flag in the RA message.
         *
         */
        void SetOtherConfigFlag(void) { mFlags |= kOtherConfigFlag; }

        /**
         * This method returns the ICMPv6 message type.
         *
         * @returns The ICMPv6 message type.
         *
         */
        Icmp::Header::Type GetType(void) const { return static_cast<Icmp::Header::Type>(mType); }

    private:
        // Router Advertisement Message
        //
        //   0                   1                   2                   3
        //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //  |     Type      |     Code      |          Checksum             |
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //  | Cur Hop Limit |M|O| |Prf|     |       Router Lifetime         |
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //  |                         Reachable Time                        |
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //  |                          Retrans Timer                        |
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //  |   Options ...
        //  +-+-+-+-+-+-+-+-+-+-+-+-

        static constexpr uint8_t kManagedAddressConfigFlag = 1 << 7;
        static constexpr uint8_t kOtherConfigFlag          = 1 << 6;
        static constexpr uint8_t kPreferenceOffset         = 3;
        static constexpr uint8_t kPreferenceMask           = 3 << kPreferenceOffset;

        uint8_t  mType;
        uint8_t  mCode;
        uint16_t mChecksum;
        uint8_t  mCurHopLimit;
        uint8_t  mFlags;
        uint16_t mRouterLifetime;
        uint32_t mReachableTime;
        uint32_t mRetransTimer;
    } OT_TOOL_PACKED_END;

    static_assert(sizeof(Header) == 16, "Invalid RA `Header`");

    typedef Data<kWithUint16Length> Icmp6Packet; ///< A data buffer containing an ICMPv6 packet.

    /**
     * Initializes the RA message from a received packet data buffer.
     *
     * @param[in] aPacket   A received packet data.
     *
     */
    explicit RouterAdvertMessage(const Icmp6Packet &aPacket)
        : mData(aPacket)
        , mMaxLength(0)
    {
    }

    /**
     * This template constructor initializes the RA message with a given header using a given buffer to store the RA
     * message.
     *
     * @tparam kBufferSize   The size of the buffer used to store the RA message.
     *
     * @param[in] aHeader    The RA message header.
     * @param[in] aBuffer    The data buffer to store the RA message in.
     *
     */
    template <uint16_t kBufferSize>
    RouterAdvertMessage(const Header &aHeader, uint8_t (&aBuffer)[kBufferSize])
        : mMaxLength(kBufferSize)
    {
        static_assert(kBufferSize >= sizeof(Header), "Buffer for RA msg is too small");

        memcpy(aBuffer, &aHeader, sizeof(Header));
        mData.Init(aBuffer, sizeof(Header));
    }

    /**
     * Gets the RA message as an `Icmp6Packet`.
     *
     * @returns The RA message as an `Icmp6Packet`.
     *
     */
    const Icmp6Packet &GetAsPacket(void) const { return mData; }

    /**
     * Indicates whether or not the RA message is valid.
     *
     * @retval TRUE   If the RA message is valid.
     * @retval FALSE  If the RA message is not valid.
     *
     */
    bool IsValid(void) const
    {
        return (mData.GetBytes() != nullptr) && (mData.GetLength() >= sizeof(Header)) &&
               (GetHeader().GetType() == Icmp::Header::kTypeRouterAdvert);
    }

    /**
     * Gets the RA message's header.
     *
     * @returns The RA message's header.
     *
     */
    const Header &GetHeader(void) const { return *reinterpret_cast<const Header *>(mData.GetBytes()); }

    /**
     * Gets the RA message's header.
     *
     * @returns The RA message's header.
     *
     */
    Header &GetHeader(void) { return *reinterpret_cast<Header *>(AsNonConst(mData.GetBytes())); }

    /**
     * Appends a Prefix Info Option to the RA message.
     *
     * The appended Prefix Info Option will have both on-link (L) and autonomous address-configuration (A) flags set.
     *
     * @param[in] aPrefix             The prefix.
     * @param[in] aValidLifetime      The valid lifetime in seconds.
     * @param[in] aPreferredLifetime  The preferred lifetime in seconds.
     *
     * @retval kErrorNone    Option is appended successfully.
     * @retval kErrorNoBufs  No more space in the buffer to append the option.
     *
     */
    Error AppendPrefixInfoOption(const Prefix &aPrefix, uint32_t aValidLifetime, uint32_t aPreferredLifetime);

    /**
     * Appends a Route Info Option to the RA message.
     *
     * @param[in] aPrefix             The prefix.
     * @param[in] aRouteLifetime      The route lifetime in seconds.
     * @param[in] aPreference         The route preference.
     *
     * @retval kErrorNone    Option is appended successfully.
     * @retval kErrorNoBufs  No more space in the buffer to append the option.
     *
     */
    Error AppendRouteInfoOption(const Prefix &aPrefix, uint32_t aRouteLifetime, RoutePreference aPreference);

    /**
     * Appends a Flags Extension Option to the RA message.
     *
     * @param[in] aStubRouterFlag    The stub router flag.
     *
     * @retval kErrorNone    Option is appended successfully.
     * @retval kErrorNoBufs  No more space in the buffer to append the option.
     *
     */
    Error AppendFlagsExtensionOption(bool aStubRouterFlag);

    /**
     * Indicates whether or not the RA message contains any options.
     *
     * @retval TRUE   If the RA message contains at least one option.
     * @retval FALSE  If the RA message contains no options.
     *
     */
    bool ContainsAnyOptions(void) const { return (mData.GetLength() > sizeof(Header)); }

    // The following methods are intended to support range-based `for`
    // loop iteration over `Option`s in the RA message.

    Option::Iterator begin(void) const { return Option::Iterator(GetOptionStart(), GetDataEnd()); }
    Option::Iterator end(void) const { return Option::Iterator(); }

private:
    const uint8_t *GetOptionStart(void) const { return (mData.GetBytes() + sizeof(Header)); }
    const uint8_t *GetDataEnd(void) const { return mData.GetBytes() + mData.GetLength(); }
    Option        *AppendOption(uint16_t aOptionSize);

    Data<kWithUint16Length> mData;
    uint16_t                mMaxLength;
};

/**
 * Implements the Router Solicitation message.
 *
 * See section 4.1 of RFC 4861 for definition of this message.
 * https://tools.ietf.org/html/rfc4861#section-4.1
 *
 */
OT_TOOL_PACKED_BEGIN
class RouterSolicitMessage
{
public:
    /**
     * Initializes the Router Solicitation message.
     *
     */
    RouterSolicitMessage(void);

private:
    Icmp::Header mHeader; // The common ICMPv6 header.
} OT_TOOL_PACKED_END;

static_assert(sizeof(RouterSolicitMessage) == 8, "invalid RouterSolicitMessage structure");

/**
 * Represents a Neighbor Solicitation (NS) message.
 *
 */
OT_TOOL_PACKED_BEGIN
class NeighborSolicitMessage : public Clearable<NeighborSolicitMessage>
{
public:
    /**
     * Initializes the Neighbor Solicitation message.
     *
     */
    NeighborSolicitMessage(void);

    /**
     * Indicates whether the Neighbor Solicitation message is valid (proper Type and Code).
     *
     * @retval TRUE  If the message is valid.
     * @retval FALSE If the message is not valid.
     *
     */
    bool IsValid(void) const { return (mType == Icmp::Header::kTypeNeighborSolicit) && (mCode == 0); }

    /**
     * Gets the Target Address field.
     *
     * @returns The Target Address.
     *
     */
    const Address &GetTargetAddress(void) const { return mTargetAddress; }

    /**
     * Sets the Target Address field.
     *
     * @param[in] aTargetAddress  The Target Address.
     *
     */
    void SetTargetAddress(const Address &aTargetAddress) { mTargetAddress = aTargetAddress; }

private:
    // Neighbor Solicitation Message (RFC 4861)
    //
    //   0                   1                   2                   3
    //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |     Type      |     Code      |          Checksum             |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |                           Reserved                            |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |                                                               |
    //  +                                                               +
    //  |                                                               |
    //  +                       Target Address                          +
    //  |                                                               |
    //  +                                                               +
    //  |                                                               |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |   Options ...
    //  +-+-+-+-+-+-+-+-+-+-+-+-

    uint8_t  mType;
    uint8_t  mCode;
    uint16_t mChecksum;
    uint32_t mReserved;
    Address  mTargetAddress;
} OT_TOOL_PACKED_END;

static_assert(sizeof(NeighborSolicitMessage) == 24, "Invalid NeighborSolicitMessage definition");

/**
 * Represents a Neighbor Advertisement (NA) message.
 *
 */
OT_TOOL_PACKED_BEGIN
class NeighborAdvertMessage : public Clearable<NeighborAdvertMessage>
{
public:
    NeighborAdvertMessage(void);

    /**
     * Indicates whether the Neighbor Advertisement message is valid (proper Type and Code).
     *
     * @retval TRUE  If the message is valid.
     * @retval FALSE If the message is not valid.
     *
     */
    bool IsValid(void) const { return (mType == Icmp::Header::kTypeNeighborAdvert) && (mCode == 0); }

    /**
     * Indicates whether or not the Router Flag is set in the NA message.
     *
     * @retval TRUE   The Router Flag is set.
     * @retval FALSE  The Router Flag is not set.
     *
     */
    bool IsRouterFlagSet(void) const { return (mFlags & kRouterFlag) != 0; }

    /**
     * Sets the Router Flag in the NA message.
     *
     */
    void SetRouterFlag(void) { mFlags |= kRouterFlag; }

    /**
     * Indicates whether or not the Solicited Flag is set in the NA message.
     *
     * @retval TRUE   The Solicited Flag is set.
     * @retval FALSE  The Solicited Flag is not set.
     *
     */
    bool IsSolicitedFlagSet(void) const { return (mFlags & kSolicitedFlag) != 0; }

    /**
     * Sets the Solicited Flag in the NA message.
     *
     */
    void SetSolicitedFlag(void) { mFlags |= kSolicitedFlag; }

    /**
     * Indicates whether or not the Override Flag is set in the NA message.
     *
     * @retval TRUE   The Override Flag is set.
     * @retval FALSE  The Override Flag is not set.
     *
     */
    bool IsOverrideFlagSet(void) const { return (mFlags & kOverrideFlag) != 0; }

    /**
     * Sets the Override Flag in the NA message.
     *
     */
    void SetOverrideFlag(void) { mFlags |= kOverrideFlag; }

    /**
     * Gets the Target Address field.
     *
     * @returns The Target Address.
     *
     */
    const Address &GetTargetAddress(void) const { return mTargetAddress; }

    /**
     * Sets the Target Address field.
     *
     * @param[in] aTargetAddress  The Target Address.
     *
     */
    void SetTargetAddress(const Address &aTargetAddress) { mTargetAddress = aTargetAddress; }

private:
    // Neighbor Advertisement Message (RFC 4861)
    //
    //   0                   1                   2                   3
    //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |     Type      |     Code      |          Checksum             |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |R|S|O|                     Reserved                            |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |                                                               |
    //  +                                                               +
    //  |                                                               |
    //  +                       Target Address                          +
    //  |                                                               |
    //  +                                                               +
    //  |                                                               |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |   Options ...
    //  +-+-+-+-+-+-+-+-+-+-+-+-

    static constexpr uint8_t kRouterFlag    = (1 << 7);
    static constexpr uint8_t kSolicitedFlag = (1 << 6);
    static constexpr uint8_t kOverrideFlag  = (1 << 5);

    uint8_t  mType;
    uint8_t  mCode;
    uint16_t mChecksum;
    uint8_t  mFlags;
    uint8_t  mReserved[3];
    Address  mTargetAddress;
} OT_TOOL_PACKED_END;

static_assert(sizeof(NeighborAdvertMessage) == 24, "Invalid NeighborAdvertMessage definition");

} // namespace Nd
} // namespace Ip6
} // namespace ot

#endif // ND6_HPP_
