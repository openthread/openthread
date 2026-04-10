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
 *   This file includes definitions for ICMPv6.
 */

#ifndef OT_CORE_NET_ICMP6_HPP_
#define OT_CORE_NET_ICMP6_HPP_

#include "openthread-core-config.h"

#include <openthread/icmp6.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/message_allocator.hpp"
#include "common/non_copyable.hpp"
#include "net/ip6_headers.hpp"

namespace ot {
namespace Ip6 {

/**
 * @addtogroup core-ip6-icmp6
 *
 * @brief
 *   This module includes definitions for ICMPv6.
 *
 * @{
 */

class Headers;

/**
 * Implements ICMPv6.
 */
class Icmp : public InstanceLocator,
             public MessageAllocator<Icmp, ReservedHeaderSize::kIcmp6Message>,
             private NonCopyable
{
public:
    typedef Icmp6Header Header; ///< ICMPv6 header

    /**
     * Implements ICMPv6 message handlers.
     */
    class Handler : public otIcmp6Handler, public LinkedListEntry<Handler>
    {
        friend class Icmp;

    public:
        /**
         * Creates an ICMPv6 message handler.
         *
         * @param[in]  aCallback  A pointer to the function that is called when receiving an ICMPv6 message.
         * @param[in]  aContext   A pointer to arbitrary context information.
         */
        Handler(otIcmp6ReceiveCallback aCallback, void *aContext)
        {
            mReceiveCallback = aCallback;
            mContext         = aContext;
            mNext            = nullptr;
        }

    private:
        void HandleReceiveMessage(Message &aMessage, const MessageInfo &aMessageInfo, const Header &aIcmp6Header)
        {
            mReceiveCallback(mContext, &aMessage, &aMessageInfo, &aIcmp6Header);
        }
    };

    /**
     * Initializes the object.
     *
     * @param[in]  aInstance A reference to the OpenThread instance.
     */
    explicit Icmp(Instance &aInstance);

    /**
     * Registers ICMPv6 handler.
     *
     * @param[in]  aHandler  A reference to the ICMPv6 handler.
     *
     * @retval kErrorNone     Successfully registered the ICMPv6 handler.
     * @retval kErrorAlready  The ICMPv6 handler is already registered.
     */
    Error RegisterHandler(Handler &aHandler);

    /**
     * Unregisters an ICMPv6 handler.
     *
     * @param[in] aHandler  The ICMPv6 handler.
     *
     * @retval kErrorNone      The handler was successfully removed from the list.
     * @retval kErrorNotFound  Could not find the handler in the list.
     */
    Error UnregisterHandler(Handler &aHandler);

    /**
     * Sends an ICMPv6 Echo Request message.
     *
     * @param[in]  aMessage      A reference to the Echo Request payload.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aIdentifier   An identifier to aid in matching Echo Replies to this Echo Request.
     *                           May be zero.
     *
     * @retval kErrorNone     Successfully enqueued the ICMPv6 Echo Request message.
     * @retval kErrorNoBufs   Insufficient buffers available to generate an ICMPv6 Echo Request message.
     */
    Error SendEchoRequest(Message &aMessage, const MessageInfo &aMessageInfo, uint16_t aIdentifier);

    /**
     * Sends an ICMPv6 error message.
     *
     * @param[in]  aType         The ICMPv6 message type.
     * @param[in]  aCode         The ICMPv6 message code.
     * @param[in]  aMessageInfo  A reference to the message info.
     * @param[in]  aMessage      The error-causing IPv6 message.
     *
     * @retval kErrorNone     Successfully enqueued the ICMPv6 error message.
     * @retval kErrorNoBufs   Insufficient buffers available.
     */
    Error SendError(Header::Type aType, Header::Code aCode, const MessageInfo &aMessageInfo, const Message &aMessage);

    /**
     * Sends an ICMPv6 error message.
     *
     * @param[in]  aType         The ICMPv6 message type.
     * @param[in]  aCode         The ICMPv6 message code.
     * @param[in]  aMessageInfo  A reference to the message info.
     * @param[in]  aHeaders      The parsed headers from the error-causing IPv6 message.
     *
     * @retval kErrorNone     Successfully enqueued the ICMPv6 error message.
     * @retval kErrorNoBufs   Insufficient buffers available.
     */
    Error SendError(Header::Type aType, Header::Code aCode, const MessageInfo &aMessageInfo, const Headers &aHeaders);

    /**
     * Handles an ICMPv6 message.
     *
     * @param[in]  aMessage      A reference to the ICMPv6 message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     * @retval kErrorNone     Successfully processed the ICMPv6 message.
     * @retval kErrorNoBufs   Insufficient buffers available to generate the reply.
     * @retval kErrorDrop     The ICMPv6 message was invalid and dropped.
     */
    Error HandleMessage(Message &aMessage, MessageInfo &aMessageInfo);

    /**
     * Indicates whether or not ICMPv6 Echo processing is enabled.
     *
     * @retval TRUE   ICMPv6 Echo processing is enabled.
     * @retval FALSE  ICMPv6 Echo processing is disabled.
     */
    otIcmp6EchoMode GetEchoMode(void) const { return mEchoMode; }

    /**
     * Sets the ICMPv6 echo mode.
     *
     * @param[in]  aMode  The ICMPv6 echo mode.
     */
    void SetEchoMode(otIcmp6EchoMode aMode) { mEchoMode = aMode; }

    /**
     * Indicates whether or not the ICMPv6 Echo Request should be handled.
     *
     * @param[in] aAddress    The ICMPv6 destination IPv6 address.
     *
     * @retval TRUE if OpenThread should respond with an ICMPv6 Echo Reply.
     * @retval FALSE if OpenThread should not respond with an ICMPv6 Echo Reply.
     */
    bool ShouldHandleEchoRequest(const Address &aAddress);

    /**
     * Returns the ICMPv6 Echo sequence number.
     *
     * @returns The sequence number of the next ICMPv6 Echo request.
     */
    uint16_t GetEchoSequence(void) const { return mEchoSequence; }

private:
    Error HandleEchoRequest(Message &aRequestMessage, const MessageInfo &aMessageInfo);

    LinkedList<Handler> mHandlers;

    uint16_t        mEchoSequence;
    otIcmp6EchoMode mEchoMode;
};

/**
 * @}
 */

} // namespace Ip6

DefineCoreType(otIcmp6Handler, Ip6::Icmp::Handler);

} // namespace ot

#endif // OT_CORE_NET_ICMP6_HPP_
