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

#include <common/encoding.hpp>
#include <net/ip6_headers.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {
namespace Ip6 {

/**
 * @addtogroup core-ip6-icmp6
 *
 * @brief
 *   This module includes definitions for ICMPv6.
 *
 * @{
 *
 */

enum
{
    kIcmp6DataSize = 4,
};

/**
 * This structure represents an ICMPv6 header.
 *
 */
OT_TOOL_PACKED_BEGIN
struct IcmpHeaderPoD
{
    uint8_t      mType;      ///< Type
    uint8_t      mCode;      ///< Code
    uint16_t     mChecksum;  ///< Checksum
    union
    {
        uint8_t  m8[kIcmp6DataSize / sizeof(uint8_t)];
        uint16_t m16[kIcmp6DataSize / sizeof(uint16_t)];
        uint32_t m32[kIcmp6DataSize / sizeof(uint32_t)];
    } mData;                 ///< Message-specific data
} OT_TOOL_PACKED_END;

/**
 * This class implements ICMPv6 header generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class IcmpHeader: private IcmpHeaderPoD
{
public:
    /**
     * This method initializes the ICMPv6 header to all zeros.
     *
     */
    void Init(void) { mType = 0; mCode = 0; mChecksum = 0; mData.m32[0] = 0; }

    /**
     * ICMPv6 Message Types
     *
     */
    enum Type
    {
        kTypeDstUnreach  = 0,     ///< Destination Unreachable
        kTypeEchoRequest = 128,   ///< Echo Request
        kTypeEchoReply   = 129,   ///< Echo Reply
    };

    /**
     * This method returns the ICMPv6 message type.
     *
     * @returns The ICMPv6 message type.
     *
     */
    Type GetType(void) const { return static_cast<Type>(mType); }

    /**
     * This method sets the ICMPv6 message type.
     *
     * @param[in]  aType  The ICMPv6 message type.
     *
     */
    void SetType(Type aType) { mType = static_cast<uint8_t>(aType); }

    /**
     * ICMPv6 Message Codes
     *
     */
    enum Code
    {
        kCodeDstUnreachNoRoute = 0,  ///< Destination Unreachable No Route
    };

    /**
     * This method returns the ICMPv6 message code.
     *
     * @returns The ICMPv6 message code.
     *
     */
    Code GetCode(void) const { return static_cast<Code>(mCode); }

    /**
     * This method sets the ICMPv6 message code.
     *
     * @param[in]  aCode  The ICMPv6 message code.
     */
    void SetCode(Code aCode) { mCode = static_cast<uint8_t>(aCode); }

    /**
     * This method returns the ICMPv6 message checksum.
     *
     * @returns The ICMPv6 message checksum.
     *
     */
    uint16_t GetChecksum(void) const { return HostSwap16(mChecksum); }

    /**
     * This method sets the ICMPv6 message checksum.
     *
     * @param[in]  aChecksum  The ICMPv6 message checksum.
     *
     */
    void SetChecksum(uint16_t aChecksum) { mChecksum = HostSwap16(aChecksum); }

    /**
     * This method returns the ICMPv6 message ID for Echo Requests and Replies.
     *
     * @returns The ICMPv6 message ID.
     *
     */
    uint16_t GetId(void) const { return HostSwap16(mData.m16[0]); }

    /**
     * This method sets the ICMPv6 message ID for Echo Requests and Replies.
     *
     * @param[in]  aId  The ICMPv6 message ID.
     *
     */
    void SetId(uint16_t aId) { mData.m16[0] = HostSwap16(aId); }

    /**
     * This method returns the ICMPv6 message sequence for Echo Requests and Replies.
     *
     * @returns The ICMPv6 message sequence.
     *
     */
    uint16_t GetSequence(void) const { return HostSwap16(mData.m16[1]); }

    /**
     * This method sets the ICMPv6 message sequence for Echo Requests and Replies.
     *
     * @param[in]  aSequence  The ICMPv6 message sequence.
     *
     */
    void SetSequence(uint16_t aSequence) { mData.m16[1] = HostSwap16(aSequence); }

    /**
     * This static method returns the byte offset of the Checksum field in the ICMPv6 header.
     *
     * @returns The byte offset of the Checksum field.
     *
     */
    static uint8_t GetChecksumOffset(void) { return offsetof(IcmpHeaderPoD, mChecksum); }

    /**
     * This static method returns the byte offset of the ICMPv6 payload.
     *
     * @returns The Byte offset of the ICMPv6 payload.
     *
     */
    static uint8_t GetDataOffset(void) { return offsetof(IcmpHeaderPoD, mData); }

} OT_TOOL_PACKED_END;

/**
 * This class implements ICMPv6 message handlers.
 *
 */
class IcmpHandler
{
    friend class Icmp;

public:
    /**
     * This function pointer is called when receiving an ICMPv6 Destination Unreachable message.
     *
     * @param[in]  aContext      A pointer to arbitrary context information.
     * @param[in]  aMessage      A reference to the received message.
     * @param[in]  aMessageInfo  A reference to message information associated with @p aMessage.
     * @param[in]  aIcmpHeader   A reference to the received ICMPv6 header.
     *
     */
    typedef void (*DstUnreachHandler)(void *aContext, Message &aMesage, const MessageInfo &aMessageInfo,
                                      const IcmpHeader &aIcmpHeader);

    /**
     * This constructor creates an ICMPv6 message handler.
     *
     * @param[in]  aDstUnreachHandler  A pointer to the function that is called when receiving a Destination
     *                                 Unreachable message.
     * @param[in]  aContext            A pointer to arbitrary context information.
     *
     */
    IcmpHandler(DstUnreachHandler aDstUnreachHandler, void *aContext) {
        mDstUnreachHandler = aDstUnreachHandler;
        mContext = aContext;
        mNext = NULL;
    }

private:
    void HandleDstUnreach(Message &message, const MessageInfo &messageInfo, const IcmpHeader &icmp6Header) {
        mDstUnreachHandler(mContext, message, messageInfo, icmp6Header);
    }

    DstUnreachHandler    mDstUnreachHandler;
    void                *mContext;
    IcmpHandler         *mNext;
};

/**
 * This class implements ICMPv6.
 *
 */
class Icmp
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aIp6  A reference to the IPv6 network object.
     *
     */
    Icmp(Ip6 &aIp6);

    /**
     * This method returns a new ICMP message with sufficient header space reserved.
     *
     * @param[in]  aReserved  The number of header bytes to reserve after the ICMP header.
     *
     * @returns A pointer to the message or NULL if no buffers are available.
     *
     */
    Message *NewMessage(uint16_t aReserved);

    /**
     * This method registers ICMPv6 handlers.
     *
     * @param[in]  aHandler  A reference to the ICMPv6 handler.
     *
     * @retval kThreadError_None  Successfully registered the ICMPv6 handler.
     * @retval kThreadError_Already  The ICMPv6 handler is already registered.
     *
     */
    ThreadError RegisterCallbacks(IcmpHandler &aHandler);

    /**
     * This function pointer is called when receiving an ICMPv6 Echo Reply in response to an Echo Request.
     *
     * @param[in]  aContext      A pointer to arbitrary context information.
     * @param[in]  aMessage      A reference to the received message.
     * @param[in]  aMessageInfo  A reference to message information associated with @p aMessage.
     *
     */
    typedef void (*EchoReplyHandler)(void *aContext, Message &aMessage, const MessageInfo &aMessageInfo);

    /**
     * This method sets the Echo Reply handler.
     *
     * @param[in]  aHandler  A pointer to a function that is called when receiving an ICMPv6 Echo Reply.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     */
    void SetEchoReplyHandler(EchoReplyHandler aHandler, void *aContext);

    /**
     * This method sends an ICMPv6 Echo Request message.
     *
     * @param[in]  aMessage      A reference to the Echo Request payload.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     * @retval kThreadError_None    Successfully enqueued the ICMPv6 Echo Request message.
     * @retval kThreadError_NoBufs  Insufficient buffers available to generate an ICMPv6 Echo Request message.
     *
     */
    ThreadError SendEchoRequest(Message &aMessage, const MessageInfo &aMessageInfo);

    /**
     * This method sends an ICMPv6 error message.
     *
     * @param[in]  aDestination  The IPv6 destination address.
     * @param[in]  aType         The ICMPv6 message type.
     * @param[in]  aCode         The ICMPv6 message code.
     * @param[in]  aHeader       The IPv6 header of the error-causing message.
     *
     * @retval kThreadError_None    Successfully enqueued the ICMPv6 error message.
     * @retval kThreadError_NoBufs  Insufficient buffers available.
     *
     */
    ThreadError SendError(const Address &aDestination, IcmpHeader::Type aType, IcmpHeader::Code aCode,
                          const Header &aHeader);

    /**
     * This method handles an ICMPv6 message.
     *
     * @param[in]  aMessage      A reference to the ICMPv6 message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     * @retval kThreadError_None    Successfully processed the ICMPv6 message.
     * @retval kThreadError_NoBufs  Insufficient buffers available to generate the reply.
     * @retval kThreadError_Drop    The ICMPv6 message was invalid and dropped.
     *
     */
    ThreadError HandleMessage(Message &aMessage, MessageInfo &aMessageInfo);

    /**
     * This method updates the ICMPv6 checksum.
     *
     * @param[in]  aMessage               A reference to the ICMPv6 message.
     * @param[in]  aPseudoHeaderChecksum  The pseudo-header checksum value.
     *
     * @retval kThreadError_None         Successfully updated the ICMPv6 checksum.
     * @retval kThreadError_InvalidArgs  The message was invalid.
     *
     */
    ThreadError UpdateChecksum(Message &aMessage, uint16_t aPseudoHeaderChecksum);

    /**
     * This method indicates whether or not ICMPv6 Echo processing is enabled.
     *
     * @retval TRUE   ICMPv6 Echo processing is enabled.
     * @retval FALSE  ICMPv6 Echo processing is disabled.
     *
     */
    bool IsEchoEnabled(void);

    /**
     * This method sets whether or not ICMPv6 Echo processing is enabled.
     *
     * @param[in]  aEnabled  TRUE to enable ICMPv6 Echo processing, FALSE otherwise.
     *
     */
    void SetEchoEnabled(bool aEnabled);

private:
    ThreadError HandleDstUnreach(Message &aMessage, const MessageInfo &aMessageInfo, const IcmpHeader &aIcmpHeader);
    ThreadError HandleEchoRequest(Message &aMessage, const MessageInfo &aMessageInfo);
    ThreadError HandleEchoReply(Message &aMessage, const MessageInfo &aMessageInfo);

    IcmpHandler *mHandlers;

    uint16_t mEchoSequence;
    EchoReplyHandler mEchoReplyHandler;
    void *mEchoReplyContext;
    bool mIsEchoEnabled;

    Ip6 &mIp6;
};

/**
 * @}
 *
 */

}  // namespace Ip6
}  // namespace Thread

#endif  // NET_ICMP6_HPP_
