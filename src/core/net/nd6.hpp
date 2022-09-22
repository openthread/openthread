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

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {
namespace Ip6 {
namespace Nd {

typedef NetworkData::RoutePreference RoutePreference; ///< Route Preference

/**
 * This class represents the variable length options in Neighbor Discovery messages.
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
        kTypePrefixInfo = 3,  ///< Prefix Information Option.
        kTypeRouteInfo  = 24, ///< Route Information Option.
    };

    static constexpr uint16_t kLengthUnit = 8; ///< The unit of length in octets.

    /**
     * This method gets the option type.
     *
     * @returns  The option type.
     *
     */
    uint8_t GetType(void) const { return mType; }

    /**
     * This method sets the option type.
     *
     * @param[in] aType  The option type.
     *
     *
     */
    void SetType(Type aType) { mType = aType; }

    /**
     * This method sets the length based on a given total option size in bytes.
     *
     * Th option must end on a 64-bit boundary, so the length is derived as `(aSize + 7) / 8 * 8`.
     *
     * @param[in]  aSize  The size of option in bytes.
     *
     */
    void SetSize(uint16_t aSize) { mLength = static_cast<uint8_t>((aSize + kLengthUnit - 1) / kLengthUnit); }

    /**
     * This method returns the size of the option in bytes.
     *
     * @returns  The size of the option in bytes.
     *
     */
    uint16_t GetSize(void) const { return mLength * kLengthUnit; }

    /**
     * This method sets the length of the option (in unit of 8 bytes).
     *
     * @param[in]  aLength  The length of the option in unit of 8 bytes.
     *
     */
    void SetLength(uint8_t aLength) { mLength = aLength; }

    /**
     * This method returns the length of the option (in unit of 8 bytes).
     *
     * @returns  The length of the option in unit of 8 bytes.
     *
     */
    uint16_t GetLength(void) const { return mLength; }

    /**
     * This method indicates whether or not this option is valid.
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
        const Option *       Validate(const Option *aOption) const;

        const Option *mOption;
        const Option *mEnd;
    };

    uint8_t mType;   // Type of the option.
    uint8_t mLength; // Length of the option in unit of 8 octets, including the `mType` and `mLength` fields.
} OT_TOOL_PACKED_END;

/**
 * This class represents the Prefix Information Option.
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
     * This method initializes the Prefix Info option with proper type and length and sets all other fields to zero.
     *
     */
    void Init(void);

    /**
     * This method indicates whether or not the on-link flag is set.
     *
     * @retval TRUE  The on-link flag is set.
     * @retval FALSE The on-link flag is not set.
     *
     */
    bool IsOnLinkFlagSet(void) const { return (mFlags & kOnLinkFlagMask) != 0; }

    /**
     * This method sets the on-link (L) flag.
     *
     */
    void SetOnLinkFlag(void) { mFlags |= kOnLinkFlagMask; }

    /**
     * This method clears the on-link (L) flag.
     *
     */
    void ClearOnLinkFlag(void) { mFlags &= ~kOnLinkFlagMask; }

    /**
     * This method indicates whether or not the autonomous address-configuration (A) flag is set.
     *
     * @retval TRUE  The auto address-config flag is set.
     * @retval FALSE The auto address-config flag is not set.
     *
     */
    bool IsAutoAddrConfigFlagSet(void) const { return (mFlags & kAutoConfigFlagMask) != 0; }

    /**
     * This method sets the autonomous address-configuration (A) flag.
     *
     */
    void SetAutoAddrConfigFlag(void) { mFlags |= kAutoConfigFlagMask; }

    /**
     * This method clears the autonomous address-configuration (A) flag.
     *
     */
    void ClearAutoAddrConfigFlag(void) { mFlags &= ~kAutoConfigFlagMask; }

    /**
     * This method sets the valid lifetime of the prefix in seconds.
     *
     * @param[in]  aValidLifetime  The valid lifetime in seconds.
     *
     */
    void SetValidLifetime(uint32_t aValidLifetime) { mValidLifetime = HostSwap32(aValidLifetime); }

    /**
     * THis method gets the valid lifetime of the prefix in seconds.
     *
     * @returns  The valid lifetime in seconds.
     *
     */
    uint32_t GetValidLifetime(void) const { return HostSwap32(mValidLifetime); }

    /**
     * This method sets the preferred lifetime of the prefix in seconds.
     *
     * @param[in]  aPreferredLifetime  The preferred lifetime in seconds.
     *
     */
    void SetPreferredLifetime(uint32_t aPreferredLifetime) { mPreferredLifetime = HostSwap32(aPreferredLifetime); }

    /**
     * THis method returns the preferred lifetime of the prefix in seconds.
     *
     * @returns  The preferred lifetime in seconds.
     *
     */
    uint32_t GetPreferredLifetime(void) const { return HostSwap32(mPreferredLifetime); }

    /**
     * This method sets the prefix.
     *
     * @param[in]  aPrefix  The prefix contained in this option.
     *
     */
    void SetPrefix(const Prefix &aPrefix);

    /**
     * This method gets the prefix in this option.
     *
     * @param[out] aPrefix   Reference to a `Prefix` to return the prefix.
     *
     */
    void GetPrefix(Prefix &aPrefix) const;

    /**
     * This method indicates whether or not the option is valid.
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
 * This class represents the Route Information Option.
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
     * This method initializes the option setting the type and clearing (setting to zero) all other fields.
     *
     */
    void Init(void);

    /**
     * This method sets the route preference.
     *
     * @param[in]  aPreference  The route preference.
     *
     */
    void SetPreference(RoutePreference aPreference);

    /**
     * This method gets the route preference.
     *
     * @returns  The route preference.
     *
     */
    RoutePreference GetPreference(void) const;

    /**
     * This method sets the lifetime of the route in seconds.
     *
     * @param[in]  aLifetime  The lifetime of the route in seconds.
     *
     */
    void SetRouteLifetime(uint32_t aLifetime) { mRouteLifetime = HostSwap32(aLifetime); }

    /**
     * This method gets Route Lifetime in seconds.
     *
     * @returns  The Route Lifetime in seconds.
     *
     */
    uint32_t GetRouteLifetime(void) const { return HostSwap32(mRouteLifetime); }

    /**
     * This method sets the prefix and adjusts the option length based on the prefix length.
     *
     * @param[in]  aPrefix  The prefix contained in this option.
     *
     */
    void SetPrefix(const Prefix &aPrefix);

    /**
     * This method gets the prefix in this option.
     *
     * @param[out] aPrefix   Reference to a `Prefix` to return the prefix.
     *
     */
    void GetPrefix(Prefix &aPrefix) const;

    /**
     * This method tells whether this option is valid.
     *
     * @returns  A boolean indicates whether this option is valid.
     *
     */
    bool IsValid(void) const;

    /**
     * This static method calculates the minimum option length for a given prefix length.
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
     * This static method calculates the minimum option size (in bytes) for a given prefix length.
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

    uint8_t *      GetPrefixBytes(void) { return AsNonConst(AsConst(this)->GetPrefixBytes()); }
    const uint8_t *GetPrefixBytes(void) const { return reinterpret_cast<const uint8_t *>(this) + sizeof(*this); }

    uint8_t  mPrefixLength;  // The prefix length in bits.
    uint8_t  mResvdPrf;      // The preference.
    uint32_t mRouteLifetime; // The lifetime in seconds.
    // Followed by prefix bytes (variable length).

} OT_TOOL_PACKED_END;

static_assert(sizeof(RouteInfoOption) == 8, "invalid RouteInfoOption structure");

/**
 * This class represents a Router Advertisement message.
 *
 */
class RouterAdvertMessage
{
public:
    /**
     * This class implements the RA message header.
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
         * This constructor initializes the Router Advertisement message with
         * zero router lifetime, reachable time and retransmission timer.
         *
         */
        Header(void) { SetToDefault(); }

        /**
         * This method sets the RA message to default values.
         *
         */
        void SetToDefault(void);

        /**
         * This method sets the checksum value.
         *
         * @param[in]  aChecksum  The checksum value.
         *
         */
        void SetChecksum(uint16_t aChecksum) { mChecksum = HostSwap16(aChecksum); }

        /**
         * This method sets the Router Lifetime in seconds.
         *
         * @param[in]  aRouterLifetime  The router lifetime in seconds.
         *
         */
        void SetRouterLifetime(uint16_t aRouterLifetime) { mRouterLifetime = HostSwap16(aRouterLifetime); }

        /**
         * This method gets the Router Lifetime (in seconds).
         *
         * Router Lifetime set to zero indicates that the sender is not a default router.
         *
         * @returns  The router lifetime in seconds.
         *
         */
        uint16_t GetRouterLifetime(void) const { return HostSwap16(mRouterLifetime); }

        /**
         * This method sets the default router preference.
         *
         * @param[in]  aPreference  The router preference.
         *
         */
        void SetDefaultRouterPreference(RoutePreference aPreference);

        /**
         * This method gets the default router preference.
         *
         * @returns  The router preference.
         *
         */
        RoutePreference GetDefaultRouterPreference(void) const;

    private:
        // Router Advertisement Message
        //
        //   0                   1                   2                   3
        //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //  |     Type      |     Code      |          Checksum             |
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //  | Cur Hop Limit |M|O|H|Prf|Resvd|       Router Lifetime         |
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //  |                         Reachable Time                        |
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //  |                          Retrans Timer                        |
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //  |   Options ...
        //  +-+-+-+-+-+-+-+-+-+-+-+-

        static constexpr uint8_t kPreferenceOffset = 3;
        static constexpr uint8_t kPreferenceMask   = 3 << kPreferenceOffset;

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
     * This constructor initializes the RA message from a received packet data buffer.
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
     * This method gets the RA message as an `Icmp6Packet`.
     *
     * @returns The RA message as an `Icmp6Packet`.
     *
     */
    const Icmp6Packet &GetAsPacket(void) const { return mData; }

    /**
     * This method indicates whether or not the RA message is valid.
     *
     * @retval TRUE   If the RA message is valid.
     * @retval FALSE  If the RA message is not valid.
     *
     */
    bool IsValid(void) const { return (mData.GetBytes() != nullptr) && (mData.GetLength() >= sizeof(Header)); }

    /**
     * This method gets the RA message's header.
     *
     * @returns The RA message's header.
     *
     */
    const Header &GetHeader(void) const { return *reinterpret_cast<const Header *>(mData.GetBytes()); }

    /**
     * This method appends a Prefix Info Option to the RA message.
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
     * This method appends a Route Info Option to the RA message.
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
     * This method indicates whether or not the RA message contains any options.
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
    Option *       AppendOption(uint16_t aOptionSize);

    Data<kWithUint16Length> mData;
    uint16_t                mMaxLength;
};

/**
 * This class implements the Router Solicitation message.
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
     * This constructor initializes the Router Solicitation message.
     *
     */
    RouterSolicitMessage(void);

private:
    Icmp::Header mHeader; // The common ICMPv6 header.
} OT_TOOL_PACKED_END;

static_assert(sizeof(RouterSolicitMessage) == 8, "invalid RouterSolicitMessage structure");

} // namespace Nd
} // namespace Ip6
} // namespace ot

#endif // ND6_HPP_
