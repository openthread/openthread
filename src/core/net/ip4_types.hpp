/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file includes types and constants for IPv4 processing.
 */

#ifndef IP4_TYPES_HPP_
#define IP4_TYPES_HPP_

#include "openthread-core-config.h"
#include "net/ip6.hpp"
#include "net/tcp6.hpp"
#include "net/udp6.hpp"

#include <stddef.h>
#include <stdint.h>

namespace ot {
namespace Ip4 {

/**
 * @addtogroup core-ip4-ip4
 *
 * @brief
 *   This module includes definitions for core IPv4 networking used by NAT64.
 *
 * @{
 *
 */

// Internet Protocol Numbers
static constexpr uint8_t kProtoTcp  = Ip6::kProtoTcp; ///< Transmission Control Protocol
static constexpr uint8_t kProtoUdp  = Ip6::kProtoUdp; ///< User Datagram
static constexpr uint8_t kProtoIcmp = 1;              ///< ICMP for IPv4

using Tcp = Ip6::Tcp; // TCP in IPv4 is the same as TCP in IPv6
using Udp = Ip6::Udp; // UDP in IPv4 is the same as UDP in IPv6

/**
 * @}
 *
 */

} // namespace Ip4
} // namespace ot

#endif // IP4_TYPES_HPP_
