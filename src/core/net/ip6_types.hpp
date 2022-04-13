/*
 *  Copyright (c) 2016-2022, The OpenThread Authors.
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
 *   This file includes types and constants for IPv6 processing.
 */

#ifndef IP6_TYPES_HPP_
#define IP6_TYPES_HPP_

#include "openthread-core-config.h"

#include <stddef.h>
#include <stdint.h>

namespace ot {
namespace Ip6 {

/**
 * @addtogroup core-ip6-ip6
 *
 * @brief
 *   This module includes definitions for core IPv6 networking.
 *
 * @{
 *
 */

// Internet Protocol Numbers
static constexpr uint8_t kProtoHopOpts  = OT_IP6_PROTO_HOP_OPTS; ///< IPv6 Hop-by-Hop Option
static constexpr uint8_t kProtoTcp      = OT_IP6_PROTO_TCP;      ///< Transmission Control Protocol
static constexpr uint8_t kProtoUdp      = OT_IP6_PROTO_UDP;      ///< User Datagram
static constexpr uint8_t kProtoIp6      = OT_IP6_PROTO_IP6;      ///< IPv6 encapsulation
static constexpr uint8_t kProtoRouting  = OT_IP6_PROTO_ROUTING;  ///< Routing Header for IPv6
static constexpr uint8_t kProtoFragment = OT_IP6_PROTO_FRAGMENT; ///< Fragment Header for IPv6
static constexpr uint8_t kProtoIcmp6    = OT_IP6_PROTO_ICMP6;    ///< ICMP for IPv6
static constexpr uint8_t kProtoNone     = OT_IP6_PROTO_NONE;     ///< No Next Header for IPv6
static constexpr uint8_t kProtoDstOpts  = OT_IP6_PROTO_DST_OPTS; ///< Destination Options for IPv6

/**
 * The max datagram length (in bytes) of an IPv6 message.
 *
 */
static constexpr uint16_t kMaxDatagramLength = OPENTHREAD_CONFIG_IP6_MAX_DATAGRAM_LENGTH;

/**
 * The max datagram length (in bytes) of an unfragmented IPv6 message.
 *
 */
static constexpr uint16_t kMaxAssembledDatagramLength = OPENTHREAD_CONFIG_IP6_MAX_ASSEMBLED_DATAGRAM;

/**
 * 6-bit Differentiated Services Code Point (DSCP) values.
 *
 */
enum IpDscpCs : uint8_t
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

/**
 * This enumeration represents the 2-bit Explicit Congestion Notification (ECN) values.
 *
 */
enum Ecn : uint8_t
{
    kEcnNotCapable = OT_ECN_NOT_CAPABLE, ///< Non ECN-Capable Transport (ECT).
    kEcnCapable0   = OT_ECN_CAPABLE_0,   ///< ECN Capable Transport, ECT(0).
    kEcnCapable1   = OT_ECN_CAPABLE_1,   ///< ECN Capable Transport, ECT(1).
    kEcnMarked     = OT_ECN_MARKED,      ///< Congestion encountered.
};

/**
 * @}
 *
 */

} // namespace Ip6
} // namespace ot

#endif // IP6_TYPES_HPP_
