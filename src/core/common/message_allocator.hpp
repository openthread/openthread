/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes definitions for the message allocator.
 */

#ifndef OT_CORE_COMMON_MESSAGE_ALLOCATOR_HPP_
#define OT_CORE_COMMON_MESSAGE_ALLOCATOR_HPP_

#include "openthread-core-config.h"

#include "common/message.hpp"
#include "net/ip6_headers.hpp"

namespace ot {

/**
 * @addtogroup core-message
 *
 * @brief
 *   This module includes definitions for the message allocator base class.
 *
 * @{
 */

/**
 * Defines constants for the reserved header sizes for different message types.
 */
struct ReservedHeaderSize
{
    /**
     * The reserved header size for an IPv6 message.
     */
    static constexpr uint16_t kIp6Message = sizeof(Ip6::Header) + sizeof(Ip6::HopByHopHeader) + sizeof(Ip6::MplOption);

    /**
     * The reserved header size for a UDP/IPv6 message
     */
    static constexpr uint16_t kUdpMessage = kIp6Message + sizeof(Ip6::UdpHeader);

    /**
     * The reserved header size for an ICMPv6 message.
     */
    static constexpr uint16_t kIcmp6Message = kIp6Message + sizeof(Ip6::Icmp6Header);

    /**
     * The reserved header size for a CoAP message.
     */
    static constexpr uint16_t kCoapMessage = kUdpMessage;
};

/**
 * Defines a `MessageAllocator` object which provides `NewMessage` methods with a fixed reserved header size.
 *
 * Users of this class should follow CRTP-style inheritance, i.e., the `Type` class itself should publicly inherit
 * from `MessageAllocator<Type, kReservedHeader>`.
 *
 * @tparam Type               The type of the class that inherits from this one (CRTP).
 * @tparam kReservedHeader    The number of header bytes to reserve.
 * @tparam kType              The `Message::Type` value to use for the allocated message.
 * @tparam MessageType        The allocated message's type. MUST be sub-class of `ot::Message` (e.g. `Coap::Message`).
 */
template <typename Type,
          uint16_t      kReservedHeader,
          Message::Type kType  = Message::kTypeIp6,
          typename MessageType = ot::Message>
class MessageAllocator
{
public:
    /**
     * Allocates a new message with default settings (link security enabled and `kPriorityNormal`) and the
     * `kReservedHeader` reserved header size.
     *
     * @returns A pointer to the message or `nullptr` if no buffers are available.
     */
    MessageType *NewMessage(void)
    {
        return AsMessageType(static_cast<Type *>(this)->template Get<MessagePool>().Allocate(kType, kReservedHeader));
    }

    /**
     * Allocates a new message with given settings and the `kReservedHeader` reserved header size.
     *
     * @param[in]  aSettings       The message settings.
     *
     * @returns A pointer to the message or `nullptr` if no buffers are available.
     */
    MessageType *NewMessage(const Message::Settings &aSettings)
    {
        return AsMessageType(
            static_cast<Type *>(this)->template Get<MessagePool>().Allocate(kType, kReservedHeader, aSettings));
    }

    /**
     * Allocates a new message with link security enabled and `kPriorityNet` and the `kReservedHeader` reserved header
     * size.
     *
     * @returns A pointer to the message or `nullptr` if no buffers are available.
     */
    MessageType *NewNetPriorityMessage(void) { return NewMessage(Message::Settings(Message::kPriorityNet)); }

protected:
    MessageAllocator(void) = default;

private:
    static MessageType *AsMessageType(Message *aMessage) { return static_cast<MessageType *>(aMessage); }
};

/**
 * @}
 */

} // namespace ot

#endif // OT_CORE_COMMON_MESSAGE_ALLOCATOR_HPP_
