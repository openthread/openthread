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
 *   This file includes definitions for IPv6 Router Advertisement.
 *
 * See RFC 4861: Neighbor Discovery for IP version 6 (https://tools.ietf.org/html/rfc4861).
 *
 */

#ifndef ROUTER_ADVERTISEMENT_HPP_
#define ROUTER_ADVERTISEMENT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include <stdint.h>

#include <openthread/platform/toolchain.h>

#include "common/encoding.hpp"
#include "net/icmp6.hpp"
#include "net/ip6.hpp"

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {

namespace RouterAdv {

static constexpr uint32_t kInfiniteLifetime = 0xffffffff;

/**
 * This class represents the variable length options in Neighbor
 * Discovery messages.
 *
 * @sa PrefixInfoOption
 * @sa RouteInfoOption
 *
 */
OT_TOOL_PACKED_BEGIN
class Option
{
public:
    enum class Type : uint8_t
    {
        kPrefixInfo = 3,  ///< Prefix Information Option.
        kRouteInfo  = 24, ///< Route Information Option.
    };

    /**
     * This constructor initializes the option with given type and length.
     *
     * @param[in]  aType    The type of this option.
     * @param[in]  aLength  The length of this option in unit of 8 octets.
     *
     */
    explicit Option(Type aType, uint8_t aLength = 0)
        : mType(aType)
        , mLength(aLength)
    {
    }

    /**
     * This method returns the type of this option.
     *
     * @return  The option type.
     *
     */
    Type GetType() const { return mType; }

    /**
     * This method sets the length of the option (in bytes).
     *
     * Since the option must end on their natural 64-bits boundaries,
     * the actual length set to the option is padded to (aLength + 7) / 8 * 8.
     *
     * @param[in]  aLength  The length of the option in unit of 1 byte.
     *
     */
    void SetLength(uint16_t aLength) { mLength = (aLength + 7) / 8; }

    /**
     * This method returns the length of the option (in bytes).
     *
     * @return  The length of the option.
     *
     */
    uint16_t GetLength() const { return mLength * 8; }

    /**
     * This helper method returns the starting address of the next valid option in the buffer.
     *
     * @param[in]  aCurOption     The current option. Use NULL to get the first option.
     * @param[in]  aBuffer        The buffer within which the options are holded.
     * @param[in]  aBufferLength  The length of the buffer.
     *
     * @returns  The address of the next option if there are a valid one. Otherwise, NULL.
     *
     */
    static const Option *GetNextOption(const Option *aCurOption, const uint8_t *aBuffer, uint16_t aBufferLength);

private:
    Type    mType;   ///< Type of the option.
    uint8_t mLength; ///< Length of the option in unit of 8 octets,
                     ///< including the `type` and `length` fields.
} OT_TOOL_PACKED_END;

/**
 * This class represents the Prefix Information Option.
 *
 * See section 4.6.2 of RFC 4861 for definition of this option.
 * https://tools.ietf.org/html/rfc4861#section-4.6.2
 *
 */
OT_TOOL_PACKED_BEGIN
class PrefixInfoOption : public Option
{
public:
    /**
     * This constructor initializes this option with zero prefix
     * length, valid lifetime and preferred lifetime.
     *
     */
    PrefixInfoOption();

    /**
     * This method sets the on on-link (L) flag.
     *
     * @param[in]  aOnLink  A boolean indicates whether the prefix is on-link or off-link.
     *
     */
    void SetOnLink(bool aOnLink);

    /**
     * This method sets the autonomous address-configuration (A) flag.
     *
     * @param[in]  aAutoAddrConfig  A boolean indicates whether this prefix can be used
     *                              for SLAAC.
     *
     */
    void SetAutoAddrConfig(bool aAutoAddrConfig);

    /**
     * This method set the valid lifetime of the prefix in seconds.
     *
     * @param[in]  aValidLifetime  The valid lifetime in seconds.
     *
     */
    void SetValidLifetime(uint32_t aValidLifetime) { mValidLifetime = HostSwap32(aValidLifetime); }

    /**
     * THis method returns the valid lifetime of the prefix in seconds.
     *
     * @return  The valid lifetime in seconds.
     *
     */
    uint32_t GetValidLifetime() const { return HostSwap32(mValidLifetime); }

    /**
     * This method sets the preferred lifetime of the prefix in seconds.
     *
     * @param[in]  aPreferredLifetime  The preferred lifetime in seconds.
     *
     */
    void SetPreferredLifetime(uint32_t aPreferredLifetime) { mPreferredLifetime = HostSwap32(aPreferredLifetime); }

    /**
     * This method sets the prefix.
     *
     * @param[in]  aPrefix  The prefix contained in this option.
     *
     */
    void SetPrefix(const Ip6::Prefix &aPrefix);

    /**
     * This method returns the prefix length in unit bits.
     *
     * @return  The prefix length in bits.
     *
     */
    uint8_t GetPrefixLength() const { return mPrefixLength; }

    /**
     * This method returns a pointer to the prefix.
     *
     * @return  The pointer to the prefix.
     *
     */
    const uint8_t *GetPrefix() const { return mPrefix; }

private:
    static constexpr uint8_t kOnLinkFlagMask     = 0x80;
    static constexpr uint8_t kAutoConfigFlagMask = 0x40;

    uint8_t  mPrefixLength;                ///< The prefix length in bits.
    uint8_t  mReserved1;                   ///< The reserved field.
    uint32_t mValidLifetime;               ///< The valid lifetime of the prefix.
    uint32_t mPreferredLifetime;           ///< The preferred lifetime of the prefix.s
    uint32_t mReserved2;                   ///< The reserved field.
    uint8_t  mPrefix[OT_IP6_ADDRESS_SIZE]; ///< The prefix.
} OT_TOOL_PACKED_END;

/**
 * This class represents the Route Information Option.
 *
 * See section 2.3 of RFC 4191 for definition of this option.
 * https://tools.ietf.org/html/rfc4191#section-2.3
 *
 */
OT_TOOL_PACKED_BEGIN
class RouteInfoOption : public Option
{
public:
    /**
     * This constructor initializes this option with zero prefix length.
     *
     */
    RouteInfoOption();

    /**
     * This method sets the lifetime of the route in seconds.
     *
     * @param[in]  aLifetime  The lifetime of the route in seconds.
     *
     */
    void SetRouteLifetime(uint32_t aLifetime) { mRouteLifetime = HostSwap32(aLifetime); }

    /**
     * This method sets the prefix.
     *
     * @param[in]  aPrefix  The prefix contained in this option.
     *
     */
    void SetPrefix(const Ip6::Prefix &aPrefix);

private:
    uint8_t  mPrefixLength;                ///< The prefix length in bits.
    uint8_t  mReserved;                    ///< The reserved field.
    uint32_t mRouteLifetime;               ///< The lifetime in seconds.
    uint8_t  mPrefix[OT_IP6_ADDRESS_SIZE]; ///< The prefix.
} OT_TOOL_PACKED_END;

/**
 * This class implements the Router Advertisement message.
 *
 * See section 4.2 of RFC 4861 for definition of this message.
 * https://tools.ietf.org/html/rfc4861#section-4.2
 *
 */
OT_TOOL_PACKED_BEGIN
class RouterAdvMessage
{
public:
    /**
     * This constructor initializes the Router Advertisement message with
     * zero router lifetime, reachable time and retransmission timer.
     *
     */
    RouterAdvMessage();

    /**
     * This method sets the Router Lifetime.
     *
     * Zero Router Lifetime means we are not a default router.
     *
     * @param[in]  aRouterLifetime  The router lifetime.
     *
     */
    void SetRouterLifetime(uint16_t aRouterLifetime) { mHeader.mData.m16[1] = HostSwap16(aRouterLifetime); }

private:
    Ip6::Icmp::Header mHeader;        ///< The common ICMPv6 header.
    uint32_t          mReachableTime; ///< The reachable time. In milliseconds.
    uint32_t          mRetransTimer;  ///< The retransmission timer. In milliseconds.
} OT_TOOL_PACKED_END;

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
    RouterSolicitMessage();

private:
    Ip6::Icmp::Header mHeader; ///< The common ICMPv6 header.
} OT_TOOL_PACKED_END;

} // namespace RouterAdv

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // ROUTER_ADVERTISEMENT_HPP_
