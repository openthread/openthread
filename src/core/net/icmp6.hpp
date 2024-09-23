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

#ifndef ICMP6_HPP_
#define ICMP6_HPP_

#include "openthread-core-config.h"

#include <openthread/icmp6.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
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
class Icmp : public InstanceLocator, private NonCopyable
{
public:
    /*
     * Implements ICMPv6 header generation and parsing.
     */
    OT_TOOL_PACKED_BEGIN
    class Header : public otIcmp6Header, public Clearable<Header>
    {
    public:
        /**
         * ICMPv6 Message Types
         */
        enum Type : uint8_t
        {
            kTypeDstUnreach       = OT_ICMP6_TYPE_DST_UNREACH,       ///< Destination Unreachable
            kTypePacketToBig      = OT_ICMP6_TYPE_PACKET_TO_BIG,     ///< Packet To Big
            kTypeTimeExceeded     = OT_ICMP6_TYPE_TIME_EXCEEDED,     ///< Time Exceeded
            kTypeParameterProblem = OT_ICMP6_TYPE_PARAMETER_PROBLEM, ///< Parameter Problem
            kTypeEchoRequest      = OT_ICMP6_TYPE_ECHO_REQUEST,      ///< Echo Request
            kTypeEchoReply        = OT_ICMP6_TYPE_ECHO_REPLY,        ///< Echo Reply
            kTypeRouterSolicit    = OT_ICMP6_TYPE_ROUTER_SOLICIT,    ///< Router Solicitation
            kTypeRouterAdvert     = OT_ICMP6_TYPE_ROUTER_ADVERT,     ///< Router Advertisement
            kTypeNeighborSolicit  = OT_ICMP6_TYPE_NEIGHBOR_SOLICIT,  ///< Neighbor Solicitation
            kTypeNeighborAdvert   = OT_ICMP6_TYPE_NEIGHBOR_ADVERT,   ///< Neighbor Advertisement
        };

        /**
         * ICMPv6 Message Codes
         */
        enum Code : uint8_t
        {
            kCodeDstUnreachNoRoute    = OT_ICMP6_CODE_DST_UNREACH_NO_ROUTE,   ///< Dest Unreachable - No Route
            kCodeDstUnreachProhibited = OT_ICMP6_CODE_DST_UNREACH_PROHIBITED, ///< Dest Unreachable - Admin Prohibited
            kCodeFragmReasTimeEx      = OT_ICMP6_CODE_FRAGM_REAS_TIME_EX,     ///< Time Exceeded - Frag Reassembly
        };

        static constexpr uint8_t kTypeFieldOffset     = 0; ///< The byte offset of Type field in ICMP6 header.
        static constexpr uint8_t kCodeFieldOffset     = 1; ///< The byte offset of Code field in ICMP6 header.
        static constexpr uint8_t kChecksumFieldOffset = 2; ///< The byte offset of Checksum field in ICMP6 header.
        static constexpr uint8_t kDataFieldOffset     = 4; ///< The byte offset of Data field in ICMP6 header.

        /**
         * Indicates whether the ICMPv6 message is an error message.
         *
         * @retval TRUE if the ICMPv6 message is an error message.
         * @retval FALSE if the ICMPv6 message is an informational message.
         */
        bool IsError(void) const { return mType < OT_ICMP6_TYPE_ECHO_REQUEST; }

        /**
         * Returns the ICMPv6 message type.
         *
         * @returns The ICMPv6 message type.
         */
        Type GetType(void) const { return static_cast<Type>(mType); }

        /**
         * Sets the ICMPv6 message type.
         *
         * @param[in]  aType  The ICMPv6 message type.
         */
        void SetType(Type aType) { mType = static_cast<uint8_t>(aType); }

        /**
         * Returns the ICMPv6 message code.
         *
         * @returns The ICMPv6 message code.
         */
        Code GetCode(void) const { return static_cast<Code>(mCode); }

        /**
         * Sets the ICMPv6 message code.
         *
         * @param[in]  aCode  The ICMPv6 message code.
         */
        void SetCode(Code aCode) { mCode = static_cast<uint8_t>(aCode); }

        /**
         * Returns the ICMPv6 message checksum.
         *
         * @returns The ICMPv6 message checksum.
         */
        uint16_t GetChecksum(void) const { return BigEndian::HostSwap16(mChecksum); }

        /**
         * Sets the ICMPv6 message checksum.
         *
         * @param[in]  aChecksum  The ICMPv6 message checksum.
         */
        void SetChecksum(uint16_t aChecksum) { mChecksum = BigEndian::HostSwap16(aChecksum); }

        /**
         * Returns the ICMPv6 message ID for Echo Requests and Replies.
         *
         * @returns The ICMPv6 message ID.
         */
        uint16_t GetId(void) const { return BigEndian::HostSwap16(mData.m16[0]); }

        /**
         * Sets the ICMPv6 message ID for Echo Requests and Replies.
         *
         * @param[in]  aId  The ICMPv6 message ID.
         */
        void SetId(uint16_t aId) { mData.m16[0] = BigEndian::HostSwap16(aId); }

        /**
         * Returns the ICMPv6 message sequence for Echo Requests and Replies.
         *
         * @returns The ICMPv6 message sequence.
         */
        uint16_t GetSequence(void) const { return BigEndian::HostSwap16(mData.m16[1]); }

        /**
         * Sets the ICMPv6 message sequence for Echo Requests and Replies.
         *
         * @param[in]  aSequence  The ICMPv6 message sequence.
         */
        void SetSequence(uint16_t aSequence) { mData.m16[1] = BigEndian::HostSwap16(aSequence); }
    } OT_TOOL_PACKED_END;

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
     * Returns a new ICMP message with sufficient header space reserved.
     *
     * @returns A pointer to the message or `nullptr` if no buffers are available.
     */
    Message *NewMessage(void);

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

DefineCoreType(otIcmp6Header, Ip6::Icmp::Header);
DefineCoreType(otIcmp6Handler, Ip6::Icmp::Handler);

} // namespace ot

#endif // ICMP6_HPP_
