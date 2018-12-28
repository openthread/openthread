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
 *   This file includes definitions for IPv6 packet processing.
 */

#ifndef IP6_HEADERS_HPP_
#define IP6_HEADERS_HPP_

#include "openthread-core-config.h"

#include <stddef.h>

#include "common/encoding.hpp"
#include "common/message.hpp"
#include "net/ip6_address.hpp"
#include "net/netif.hpp"
#include "net/socket.hpp"

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {

/**
 * @namespace ot::Ip6
 *
 * @brief
 *   This namespace includes definitions for IPv6 networking.
 *
 */
namespace Ip6 {

/**
 * @addtogroup core-ipv6
 *
 * @brief
 *   This module includes definitions for the IPv6 network layer.
 *
 * @{
 *
 * @defgroup core-ip6-icmp6 ICMPv6
 * @defgroup core-ip6-ip6 IPv6
 * @defgroup core-ip6-mpl MPL
 * @defgroup core-ip6-netif Network Interfaces
 *
 * @}
 *
 */

/**
 * @addtogroup core-ip6-ip6
 *
 * @brief
 *   This module includes definitions for core IPv6 networking.
 *
 * @{
 *
 */

/**
 * Internet Protocol Numbers
 */
enum IpProto
{
    kProtoHopOpts  = 0,  ///< IPv6 Hop-by-Hop Option
    kProtoTcp      = 6,  ///< Transmission Control Protocol
    kProtoUdp      = 17, ///< User Datagram
    kProtoIp6      = 41, ///< IPv6 encapsulation
    kProtoRouting  = 43, ///< Routing Header for IPv6
    kProtoFragment = 44, ///< Fragment Header for IPv6
    kProtoIcmp6    = 58, ///< ICMP for IPv6
    kProtoNone     = 59, ///< No Next Header for IPv6
    kProtoDstOpts  = 60, ///< Destination Options for IPv6
};

/**
 * Class Selectors
 */
enum IpDscpCs
{
    kDscpCs0    = 0,    ///< Class selector codepoint 0
    kDscpCs1    = 8,    ///< Class selector codepoint 8
    kDscpCs2    = 16,   ///< Class selector codepoint 16
    kDscpCs3    = 24,   ///< Class selector codepoint 24
    kDscpCs4    = 32,   ///< Class selector codepoint 32
    kDscpCs5    = 40,   ///< Class selector codepoint 40
    kDscpCs6    = 48,   ///< Class selector codepoint 48
    kDscpCs7    = 56,   ///< Class selector codepoint 56
    kDscpCsMask = 0x38, ///< Class selector mask
};

enum
{
    kVersionClassFlowSize = 4, ///< Combined size of Version, Class, Flow Label in bytes.
};

/**
 * This structure represents an IPv6 header.
 *
 */
OT_TOOL_PACKED_BEGIN
struct HeaderPoD
{
    union OT_TOOL_PACKED_FIELD
    {
        uint8_t  m8[kVersionClassFlowSize / sizeof(uint8_t)];
        uint16_t m16[kVersionClassFlowSize / sizeof(uint16_t)];
        uint32_t m32[kVersionClassFlowSize / sizeof(uint32_t)];
    } mVersionClassFlow;         ///< Version, Class, Flow Label
    uint16_t     mPayloadLength; ///< Payload Length
    uint8_t      mNextHeader;    ///< Next Header
    uint8_t      mHopLimit;      ///< Hop Limit
    otIp6Address mSource;        ///< Source
    otIp6Address mDestination;   ///< Destination
} OT_TOOL_PACKED_END;

/**
 * This class implements IPv6 header generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Header : private HeaderPoD
{
public:
    /**
     * This method initializes the IPv6 header.
     *
     */
    void Init(void)
    {
        mVersionClassFlow.m32[0] = 0;
        mVersionClassFlow.m8[0]  = kVersion6;
    }

    /**
     * This method initializes the IPv6 header and sets Version, Traffic Control and Flow Label fields.
     *
     */
    void Init(uint32_t aVersionClassFlow) { mVersionClassFlow.m32[0] = HostSwap32(aVersionClassFlow); }

    /**
     * This method reads the IPv6 header from @p aMessage.
     *
     * @param[in]  aMessage  The IPv6 datagram.
     *
     * @retval OT_ERROR_NONE   Successfully read the IPv6 header.
     * @retval OT_ERROR_PARSE  Malformed IPv6 header.
     *
     */
    otError Init(const Message &aMessage);

    /**
     * This method indicates whether or not the header appears to be well-formed.
     *
     * @retval TRUE  if the header appears to be well-formed.
     * @retval FALSE if the header does not appear to be well-formed.
     *
     */
    bool IsValid(void) const;

    /**
     * This method indicates whether or not the IPv6 Version is set to 6.
     *
     * @retval TRUE   If the IPv6 Version is set to 6.
     * @retval FALSE  If the IPv6 Version is not set to 6.
     *
     */
    bool IsVersion6(void) const { return (mVersionClassFlow.m8[0] & kVersionMask) == kVersion6; }

    /**
     * This method returns the IPv6 DSCP value.
     *
     * @returns The IPv6 DSCP value.
     *
     */
    uint8_t GetDscp(void) const
    {
        return static_cast<uint8_t>((HostSwap32(mVersionClassFlow.m32[0]) & kDscpMask) >> kDscpOffset);
    }

    /**
     * This method sets the IPv6 DSCP value.
     *
     * @param[in]  aDscp  The IPv6 DSCP value.
     *
     */
    void SetDscp(uint8_t aDscp)
    {
        uint32_t tmp = HostSwap32(mVersionClassFlow.m32[0]);
        tmp = (tmp & static_cast<uint32_t>(~kDscpMask)) | ((static_cast<uint32_t>(aDscp) << kDscpOffset) & kDscpMask);
        mVersionClassFlow.m32[0] = HostSwap32(tmp);
    }

    /**
     * This method returns the IPv6 Payload Length value.
     *
     * @returns The IPv6 Payload Length value.
     *
     */
    uint16_t GetPayloadLength(void) const { return HostSwap16(mPayloadLength); }

    /**
     * This method sets the IPv6 Payload Length value.
     *
     * @param[in]  aLength  The IPv6 Payload Length value.
     *
     */
    void SetPayloadLength(uint16_t aLength) { mPayloadLength = HostSwap16(aLength); }

    /**
     * This method returns the IPv6 Next Header value.
     *
     * @returns The IPv6 Next Header value.
     *
     */
    IpProto GetNextHeader(void) const { return static_cast<IpProto>(mNextHeader); }

    /**
     * This method sets the IPv6 Next Header value.
     *
     * @param[in]  aNextHeader  The IPv6 Next Header value.
     *
     */
    void SetNextHeader(IpProto aNextHeader) { mNextHeader = static_cast<uint8_t>(aNextHeader); }

    /**
     * This method returns the IPv6 Hop Limit value.
     *
     * @returns The IPv6 Hop Limit value.
     *
     */
    uint8_t GetHopLimit(void) const { return mHopLimit; }

    /**
     * This method sets the IPv6 Hop Limit value.
     *
     * @param[in]  aHopLimit  The IPv6 Hop Limit value.
     *
     */
    void SetHopLimit(uint8_t aHopLimit) { mHopLimit = aHopLimit; }

    /**
     * This method returns the IPv6 Source address.
     *
     * @returns A reference to the IPv6 Source address.
     *
     */
    Address &GetSource(void) { return static_cast<Address &>(mSource); }

    /**
     * This method sets the IPv6 Source address.
     *
     * @param[in]  aSource  A reference to the IPv6 Source address.
     *
     */
    void SetSource(const Address &aSource) { mSource = aSource; }

    /**
     * This method returns the IPv6 Destination address.
     *
     * @returns A reference to the IPv6 Destination address.
     *
     */
    Address &GetDestination(void) { return static_cast<Address &>(mDestination); }

    /**
     * This method sets the IPv6 Destination address.
     *
     * @param[in]  aDestination  A reference to the IPv6 Destination address.
     *
     */
    void SetDestination(const Address &aDestination) { mDestination = aDestination; }

    /**
     * This static method returns the byte offset of the IPv6 Payload Length field.
     *
     * @returns The byte offset of the IPv6 Payload Length field.
     *
     */
    static uint8_t GetPayloadLengthOffset(void) { return offsetof(HeaderPoD, mPayloadLength); }

    /**
     * This static method returns the byte offset of the IPv6 Hop Limit field.
     *
     * @returns The byte offset of the IPv6 Hop Limit field.
     *
     */
    static uint8_t GetHopLimitOffset(void) { return offsetof(HeaderPoD, mHopLimit); }

    /**
     * This static method returns the size of the IPv6 Hop Limit field.
     *
     * @returns The size of the IPv6 Hop Limit field.
     *
     */
    static uint8_t GetHopLimitSize(void) { return sizeof(uint8_t); }

    /**
     * This static method returns the byte offset of the IPv6 Destination field.
     *
     * @returns The byte offset of the IPv6 Destination field.
     *
     */
    static uint8_t GetDestinationOffset(void) { return offsetof(HeaderPoD, mDestination); }

private:
    enum
    {
        kVersion6    = 0x60,
        kVersionMask = 0xf0,
        kDscpOffset  = 22,
        kDscpMask    = 0xfc00000,
    };
} OT_TOOL_PACKED_END;

/**
 * This class implements IPv6 Extension Header generation and processing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ExtensionHeader
{
public:
    /**
     * This method returns the IPv6 Next Header value.
     *
     * @returns The IPv6 Next Header value.
     *
     */
    IpProto GetNextHeader(void) const { return static_cast<IpProto>(mNextHeader); }

    /**
     * This method sets the IPv6 Next Header value.
     *
     * @param[in]  aNextHeader  The IPv6 Next Header value.
     *
     */
    void SetNextHeader(IpProto aNextHeader) { mNextHeader = static_cast<uint8_t>(aNextHeader); }

    /**
     * This method returns the IPv6 Header Extension Length value.
     *
     * @returns The IPv6 Header Extension Length value.
     *
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * This method sets the IPv6 Header Extension Length value.
     *
     * @param[in]  aLength  The IPv6 Header Extension Length value.
     *
     */
    void SetLength(uint8_t aLength) { mLength = aLength; }

private:
    uint8_t mNextHeader;
    uint8_t mLength;
} OT_TOOL_PACKED_END;

/**
 * This class implements IPv6 Hop-by-Hop Options Header generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class HopByHopHeader : public ExtensionHeader
{
} OT_TOOL_PACKED_END;

/**
 * This class implements IPv6 Options generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class OptionHeader
{
public:
    /**
     * Default constructor.
     *
     */
    OptionHeader(void)
        : mType(0)
        , mLength(0)
    {
    }

    /**
     * This method returns the IPv6 Option Type value.
     *
     * @returns The IPv6 Option Type value.
     *
     */
    uint8_t GetType(void) const { return mType; }

    /**
     * This method sets the IPv6 Option Type value.
     *
     * @param[in]  aType  The IPv6 Option Type value.
     *
     */
    void SetType(uint8_t aType) { mType = aType; }

    /**
     * IPv6 Option Type actions for unrecognized IPv6 Options.
     *
     */
    enum Action
    {
        kActionSkip      = 0x00, ///< skip over this option and continue processing the header
        kActionDiscard   = 0x40, ///< discard the packet
        kActionForceIcmp = 0x80, ///< discard the packet and forcibly send an ICMP Parameter Problem
        kActionIcmp      = 0xc0, ///< discard packet and conditionally send an ICMP Parameter Problem
        kActionMask      = 0xc0, ///< mask for action bits
    };

    /**
     * This method returns the IPv6 Option action for unrecognized IPv6 Options.
     *
     * @returns The IPv6 Option action for unrecognized IPv6 Options.
     *
     */
    Action GetAction(void) const { return static_cast<Action>(mType & kActionMask); }

    /**
     * This method returns the IPv6 Option Length value.
     *
     * @returns The IPv6 Option Length value.
     *
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * This method sets the IPv6 Option Length value.
     *
     * @param[in]  aLength  The IPv6 Option Length value.
     *
     */
    void SetLength(uint8_t aLength) { mLength = aLength; }

private:
    uint8_t mType;
    uint8_t mLength;
} OT_TOOL_PACKED_END;

/**
 * This class implements IPv6 PadN Option generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class OptionPadN : public OptionHeader
{
public:
    enum
    {
        kType      = 0x01, ///< PadN type
        kData      = 0x00, ///< PadN specific data
        kMaxLength = 0x05  ///< Maximum length of PadN option data
    };

    /**
     * This method initializes the PadN header.
     *
     * @param[in]  aPadLength  The length of needed padding. Allowed value from
     *                         range 2-7.
     *
     */
    void Init(uint8_t aPadLength)
    {
        OptionHeader::SetType(kType);
        OptionHeader::SetLength(aPadLength - sizeof(OptionHeader));
        memset(mPad, kData, aPadLength - sizeof(OptionHeader));
    }

    /**
     * This method returns the total IPv6 Option Length value including option
     * header.
     *
     * @returns The total IPv6 Option Length.
     *
     */
    uint8_t GetTotalLength(void) const { return OptionHeader::GetLength() + sizeof(OptionHeader); }

private:
    uint8_t mPad[kMaxLength];
} OT_TOOL_PACKED_END;

/**
 * This class implements IPv6 Pad1 Option generation and parsing. Pad1 does not follow default option header structure.
 *
 */
OT_TOOL_PACKED_BEGIN
class OptionPad1
{
public:
    enum
    {
        kType = 0x00
    };

    /**
     * This method initializes the Pad1 header.
     *
     */
    void Init(void) { mType = kType; }

private:
    uint8_t mType;
} OT_TOOL_PACKED_END;

/**
 * This class implements IPv6 Fragment Header generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class FragmentHeader
{
public:
    /**
     * This method initializes the IPv6 Fragment header.
     *
     */
    void Init(void)
    {
        mReserved       = 0;
        mIdentification = 0;
    }

    /**
     * This method returns the IPv6 Next Header value.
     *
     * @returns The IPv6 Next Header value.
     *
     */
    IpProto GetNextHeader(void) const { return static_cast<IpProto>(mNextHeader); }

    /**
     * This method sets the IPv6 Next Header value.
     *
     * @param[in]  aNextHeader  The IPv6 Next Header value.
     *
     */
    void SetNextHeader(IpProto aNextHeader) { mNextHeader = static_cast<uint8_t>(aNextHeader); }

    /**
     * This method returns the Fragment Offset value.
     *
     * @returns The Fragment Offset value.
     *
     */
    uint16_t GetOffset(void) { return (HostSwap16(mOffsetMore) & kOffsetMask) >> kOffsetOffset; }

    /**
     * This method sets the Fragment Offset value.
     *
     * @param[in]  aOffset  The Fragment Offset value.
     */
    void SetOffset(uint16_t aOffset)
    {
        uint16_t tmp = HostSwap16(mOffsetMore);
        tmp          = (tmp & ~kOffsetMask) | ((aOffset << kOffsetOffset) & kOffsetMask);
        mOffsetMore  = HostSwap16(tmp);
    }

    /**
     * This method returns the M flag value.
     *
     * @returns The M flag value.
     *
     */
    bool IsMoreFlagSet(void) { return HostSwap16(mOffsetMore) & kMoreFlag; }

    /**
     * This method clears the M flag value.
     *
     */
    void ClearMoreFlag(void) { mOffsetMore = HostSwap16(HostSwap16(mOffsetMore) & ~kMoreFlag); }

    /**
     * This method sets the M flag value.
     *
     */
    void SetMoreFlag(void) { mOffsetMore = HostSwap16(HostSwap16(mOffsetMore) | kMoreFlag); }

private:
    uint8_t mNextHeader;
    uint8_t mReserved;

    enum
    {
        kOffsetOffset = 3,
        kOffsetMask   = 0xfff8,
        kMoreFlag     = 1,
    };
    uint16_t mOffsetMore;
    uint32_t mIdentification;
} OT_TOOL_PACKED_END;

/**
 * @}
 *
 */

} // namespace Ip6
} // namespace ot

#endif // NET_IP6_HEADERS_HPP_
