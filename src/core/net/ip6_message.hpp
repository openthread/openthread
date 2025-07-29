/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#ifndef IP6_MESSAGE_HPP_
#define IP6_MESSAGE_HPP_

#include "openthread-core-config.h"

#include <openthread/ip6.h>

#include "common/error.hpp"
#include "common/message.hpp"
#include "net/socket.hpp"

namespace ot {
namespace Ip6 {

class Ip6;

/**
 * Represents a received IPv6 message.
 */
class RxMessage : public ot::Message
{
    friend class Ip6;

public:
    /**
     * Gets the `MessageInfo` associated with this message.
     *
     * @returns  The associated `Ip6::MessageInfo`.
     */
    MessageInfo &GetInfo(void) { return *static_cast<MessageInfo *>(Message::GetInfo()); }

    /**
     * Gets the `MessageInfo` associated with this message.
     *
     * @returns  The associated `Ip6::MessageInfo`.
     */
    const MessageInfo &GetInfo(void) const { return *static_cast<const MessageInfo *>(Message::GetInfo()); }

private:
    void SetInfo(MessageInfo &aMessageInfo) { Message::SetInfo(&aMessageInfo); }

    static RxMessage &From(Message &aMessage, MessageInfo &aMessageInfo)
    {
        RxMessage *rxMsg = static_cast<RxMessage *>(&aMessage);

        rxMsg->SetInfo(aMessageInfo);
        return *rxMsg;
    }
};

} // namespace Ip6
} // namespace ot

#endif // IP6_MESSAGE_HPP_
