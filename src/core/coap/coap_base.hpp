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

#ifndef COAP_BASE_HPP_
#define COAP_BASE_HPP_

#include "openthread/coap.h"

#include <coap/coap_header.hpp>
#include <common/message.hpp>
#include <net/netif.hpp>
#include <net/udp6.hpp>

/**
 * @file
 *   This file contains common code base for CoAP client and server.
 */

namespace Thread {
namespace Coap {

/**
 * @addtogroup core-coap
 *
 * @{
 *
 */

/**
 * Protocol Constants (RFC 7252).
 *
 */
enum
{
    kAckTimeout                 = OPENTHREAD_CONFIG_COAP_ACK_TIMEOUT,
    kAckRandomFactorNumerator   = OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_NUMERATOR,
    kAckRandomFactorDenominator = OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_DENOMINATOR,
    kMaxRetransmit              = OPENTHREAD_CONFIG_COAP_MAX_RETRANSMIT,
    kNStart                     = 1,
    kDefaultLeisure             = 5,
    kProbingRate                = 1,

    // Note that 2 << (kMaxRetransmit - 1) is equal to kMaxRetransmit power of 2
    kMaxTransmitSpan            = kAckTimeout * ((2 << (kMaxRetransmit - 1)) - 1) *
                                  kAckRandomFactorNumerator / kAckRandomFactorDenominator,
    kMaxTransmitWait            = kAckTimeout * ((2 << kMaxRetransmit) - 1) *
                                  kAckRandomFactorNumerator / kAckRandomFactorDenominator,
    kMaxLatency                 = 100,
    kProcessingDelay            = kAckTimeout,
    kMaxRtt                     = 2 * kMaxLatency + kProcessingDelay,
    kExchangeLifetime           = kMaxTransmitSpan + 2 * (kMaxLatency) + kProcessingDelay,
    kNonLifetime                = kMaxTransmitSpan + kMaxLatency
};

/**
 * This class implements a common code base for CoAP client/server.
 *
 */
class CoapBase
{
public:

    /**
     * This function pointer is called when CoAP client/server wants to send a message.
     *
     * @param[in]  aContext      A pointer to arbitrary context information.
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     */
    typedef ThreadError(* SenderFunction)(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This function pointer is called when CoAP client/server receives a message.
     *
     * @param[in]  aContext      A pointer to arbitrary context information.
     * @param[in]  aMessage      A reference to the received message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     */
    typedef void (* ReceiverFunction)(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aUdp      A reference to the UDP object.
     * @param[in]  aSender   A pointer to a function for sending messages.
     * @param[in]  aReceiver A pointer to a function for handling received messages.
     *
     */
    CoapBase(Ip6::Udp &aUdp, SenderFunction aSender, ReceiverFunction aReceiver):
        mSocket(aUdp),
        mSender(aSender),
        mReceiver(aReceiver) {};

    /**
     * This method creates a new message with a CoAP header.
     *
     * @param[in]  aHeader      A reference to a CoAP header that is used to create the message.
     * @param[in]  aPrority     The message priority level.
     *
     * @returns A pointer to the message or NULL if failed to allocate message.
     *
     */
    Message *NewMessage(const Header &aHeader, uint8_t aPriority = kDefaultCoapMessagePriority);

    /**
     * This method returns a port number used by CoAP client.
     *
     * @returns A port number.
     *
     */
    uint16_t GetPort(void) { return mSocket.GetSockName().mPort; };

    /**
     * This method sends a CoAP reset message.
     *
     * @param[in]  aRequestHeader  A reference to the CoAP Header that was used in CoAP request.
     * @param[in]  aMessageInfo    The message info corresponding to the CoAP request.
     *
     * @retval kThreadError_None         Successfully enqueued the CoAP response message.
     * @retval kThreadError_NoBufs       Insufficient buffers available to send the CoAP response.
     * @retval kThreadError_InvalidArgs  The @p aRequestHeader header is not of confirmable type.
     *
     */
    ThreadError SendReset(Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo) {
        return SendEmptyMessage(kCoapTypeReset, aRequestHeader, aMessageInfo);
    };

    /**
     * This method sends a CoAP ACK empty message which is used in Separate Response for confirmable requests.
     *
     * @param[in]  aRequestHeader  A reference to the CoAP Header that was used in CoAP request.
     * @param[in]  aMessageInfo    The message info corresponding to the CoAP request.
     *
     * @retval kThreadError_None         Successfully enqueued the CoAP response message.
     * @retval kThreadError_NoBufs       Insufficient buffers available to send the CoAP response.
     * @retval kThreadError_InvalidArgs  The @p aRequestHeader header is not of confirmable type.
     *
     */
    ThreadError SendAck(Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo) {
        return SendEmptyMessage(kCoapTypeAcknowledgment, aRequestHeader, aMessageInfo);
    };

protected:
    ThreadError Start(const Ip6::SockAddr &aSockAddr);
    ThreadError Stop(void);

    Ip6::UdpSocket mSocket;
    SenderFunction mSender;
    ReceiverFunction mReceiver;

private:
    /**
     * This method sends a CoAP empty message, i.e. a header-only message with code equals kCoapCodeEmpty.
     *
     * @param[in]  aType           The message type
     * @param[in]  aRequestHeader  A reference to the CoAP Header that was used in CoAP request.
     * @param[in]  aMessageInfo    The message info corresponding to the CoAP request.
     *
     * @retval kThreadError_None         Successfully enqueued the CoAP response message.
     * @retval kThreadError_NoBufs       Insufficient buffers available to send the CoAP response.
     * @retval kThreadError_InvalidArgs  The @p aRequestHeader header is not of confirmable type.
     *
     */
    ThreadError SendEmptyMessage(Header::Type aType, const Header &aRequestHeader,
                                 const Ip6::MessageInfo &aMessageInfo);

    enum
    {
        kDefaultCoapMessagePriority = Message::kPriorityLow,
    };

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
};

}  // namespace Coap
}  // namespace Thread

#endif  // COAP_BASE_HPP_
