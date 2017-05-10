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
 *   This file includes definitions for the CoAP server.
 */

#ifndef COAP_SERVER_HPP_
#define COAP_SERVER_HPP_

#include <openthread/coap.h>

#include "coap/coap_base.hpp"
#include "coap/coap_header.hpp"
#include "common/debug.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "net/ip6.hpp"
#include "net/udp6.hpp"

namespace ot {
namespace Coap {

/**
 * @addtogroup core-coap
 *
 * @{
 *
 */

/**
 * This class implements CoAP resource handling.
 *
 */
class Resource : public otCoapResource
{
    friend class Server;

public:
    enum
    {
        kMaxReceivedUriPath = 32,   ///< Maximum supported URI path on received messages.
    };

    /**
     * This constructor initializes the resource.
     *
     * @param[in]  aUriPath  A pointer to a NULL-terminated string for the Uri-Path.
     * @param[in]  aHandler  A function pointer that is called when receiving a CoAP message for @p aUriPath.
     * @param[in]  aContext  A pointer to arbitrary context information.
     */
    Resource(const char *aUriPath, otCoapRequestHandler aHandler, void *aContext) {
        mUriPath = aUriPath;
        mHandler = aHandler;
        mContext = aContext;
        mNext = NULL;
    }

    /**
     * This method returns a pointer to the next resource.
     *
     * @returns A Pointer to the next resource.
     *
     */
    Resource *GetNext(void) const { return static_cast<Resource *>(mNext); };

private:
    void HandleRequest(Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo) {
        mHandler(mContext, &aHeader, &aMessage, &aMessageInfo);
    }
};

/**
 * This class implements metadata required for caching CoAP responses.
 *
 */
class EnqueuedResponseHeader
{
public:
    /**
     * Default constructor creating empty object.
     *
     */
    EnqueuedResponseHeader(void): mDequeueTime(0), mMessageInfo() {}

    /**
     * Constructor creating object with valid dequeue time and message info.
     *
     * @param[in]  aMessageInfo  The message info containing source endpoint identification.
     *
     */
    EnqueuedResponseHeader(const Ip6::MessageInfo &aMessageInfo):
        mDequeueTime(Timer::GetNow() + Timer::SecToMsec(kExchangeLifetime)),
        mMessageInfo(aMessageInfo) {}

    /**
     * This method append metadata to the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None    Successfully appended the bytes.
     * @retval kThreadError_NoBufs  Insufficient available buffers to grow the message.
     */
    ThreadError AppendTo(Message &aMessage) const { return aMessage.Append(this, sizeof(*this)); }

    /**
     * This method reads request data from the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @returns The number of bytes read.
     *
     */
    uint16_t ReadFrom(const Message &aMessage) {
        return aMessage.Read(aMessage.GetLength() - sizeof(*this), sizeof(*this), this);
    }

    /**
     * This method removes metadata from the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     */
    static void RemoveFrom(Message &aMessage) {
        assert(aMessage.SetLength(aMessage.GetLength() - sizeof(EnqueuedResponseHeader)) == kThreadError_None);
    }

    /**
     * This method checks if the message shall be sent before the given time.
     *
     * @param[in]  aTime  A time to compare.
     *
     * @retval TRUE   If the message shall be sent before the given time.
     * @retval FALSE  Otherwise.
     *
     */
    bool IsEarlier(uint32_t aTime) const { return (static_cast<int32_t>(aTime - mDequeueTime) > 0); }

    /**
     * This method returns number of milliseconds in which the message should be sent.
     *
     * @returns  The number of milliseconds in which the message should be sent.
     *
     */
    uint32_t GetRemainingTime(void) const;

    /**
     * This method returns the message info of cached CoAP response.
     *
     * @returns  The message info of the cached CoAP response.
     *
     */
    const Ip6::MessageInfo &GetMessageInfo(void) const { return mMessageInfo; }

private:
    uint32_t mDequeueTime;
    const Ip6::MessageInfo mMessageInfo;
};

/**
 * This class caches CoAP responses to implement message deduplication.
 *
 */
class ResponsesQueue
{
public:
    /**
     * Default class constructor.
     *
     * @param[in]  aNetif  A reference to the network interface that CoAP server should be assigned to.
     *
     */
    ResponsesQueue(Ip6::Netif &aNetif):
        mTimer(aNetif.GetIp6().mTimerScheduler, &ResponsesQueue::HandleTimer, this) {}

    /**
     * Add given response to the cache.
     *
     * If matching response (the same Message ID, source endpoint address and port) exists in the cache given
     * response is not added.
     * The CoAP response is copied before it is added to the cache.
     *
     * @param[in]  aMessage      The CoAP response to add to the cache.
     * @param[in]  aMessageInfo  The message info corresponding to @p aMessage.
     *
     */
    void EnqueueResponse(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * Remove the oldest response from the cache.
     *
     */
    void DequeueOldestResponse(void);

    /**
     * Remove all responses from the cache.
     *
     */
    void DequeueAllResponses(void);

    /**
     * Get a copy of CoAP response from the cache that matches given Message ID and source endpoint.
     *
     * @param[in]  aHeader       The CoAP message header containing Message ID.
     * @param[in]  aMessageInfo  The message info containing source endpoint address and port.
     * @param[out] aResponse     A pointer to a copy of a cached CoAP response matching given arguments.
     *
     * @retval kThreadError_None      Matching response found and successfully created a copy.
     * @retval kThreadError_NoBufs    Matching response found but there is not sufficient buffer to create a copy.
     * @retval kThreadError_NotFound  Matching response not found.
     *
     */
    ThreadError GetMatchedResponseCopy(const Header &aHeader,
                                       const Ip6::MessageInfo &aMessageInfo,
                                       Message **aResponse);

    /**
     * Get a copy of CoAP response from the cache that matches given Message ID and source endpoint.
     *
     * @param[in]  aRequest      The CoAP message containing Message ID.
     * @param[in]  aMessageInfo  The message info containing source endpoint address and port.
     * @param[out] aResponse     A pointer to a copy of a cached CoAP response matching given arguments.
     *
     * @retval kThreadError_None      Matching response found and successfully created a copy.
     * @retval kThreadError_NoBufs    Matching response found but there is not sufficient buffer to create a copy.
     * @retval kThreadError_NotFound  Matching response not found.
     * @retval kThreadError_Parse     Could not parse CoAP header in the request message.
     *
     */
    ThreadError GetMatchedResponseCopy(const Message &aRequest,
                                       const Ip6::MessageInfo &aMessageInfo,
                                       Message **aResponse);

    /**
     * Get a reference to the cached CoAP responses queue.
     *
     * @returns  A reference to the cached CoAP responses queue.
     *
     */
    const MessageQueue &GetResponses(void) const { return mQueue; }

private:
    enum
    {
        kMaxCachedResponses = OPENTHREAD_CONFIG_COAP_SERVER_MAX_CACHED_RESPONSES,
    };

    void DequeueResponse(Message &aMessage) { mQueue.Dequeue(aMessage); aMessage.Free(); }

    MessageQueue mQueue;
    Timer        mTimer;

    static void HandleTimer(void *aContext);
    void HandleTimer(void);
};

/**
 * This class implements the CoAP server.
 *
 */
class Server : public CoapBase
{
public:
    /**
     * This function pointer is called before CoAP server processing a CoAP packets.
     *
     * @param[in]   aMessage        A reference to the message.
     @ @param[in]   aMessageInfo    A reference to the message info associated with @p aMessage.
     *
     * @retval  kThreadError_None       Server should continue processing this message, other
     *                                  return values indicates the server should stop processing
     *                                  this message.
     * @retval  kThreadError_NotTmf     The message is not a TMF message.
     *
     */
    typedef ThreadError(* Interceptor)(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aNetif    A reference to the network interface that CoAP server should be assigned to.
     * @param[in]  aPort     The port to listen on.
     * @param[in]  aSender   A pointer to a function for sending messages.
     * @param[in]  aReceiver A pointer to a function for handling received messages.
     *
     */
    Server(Ip6::Netif &aNetif, uint16_t aPort);

    /**
     * This method starts the CoAP server.
     *
     * @retval kThreadError_None  Successfully started the CoAP server.
     *
     */
    ThreadError Start(void);

    /**
     * This method stops the CoAP server.
     *
     * @retval kThreadError_None  Successfully stopped the CoAP server.
     *
     */
    ThreadError Stop(void);

    /**
     * This method adds a resource to the CoAP server.
     *
     * @param[in]  aResource  A reference to the resource.
     *
     * @retval kThreadError_None     Successfully added @p aResource.
     * @retval kThreadError_Already  The @p aResource was already added.
     *
     */
    ThreadError AddResource(Resource &aResource);

    /**
     * This method removes a resource from the CoAP server.
     *
     * @param[in]  aResource  A reference to the resource.
     *
     */
    void RemoveResource(Resource &aResource);

    /**
      * This method sends a CoAP response from the server.
      *
      * @param[in]  aMessage      The CoAP response to send.
      * @param[in]  aMessageInfo  The message info corresponding to @p aMessage.
      *
      * @retval kThreadError_None    Successfully enqueued the CoAP response message.
      * @retval kThreadError_NoBufs  Insufficient buffers available to send the CoAP response.
      *
      */
    ThreadError SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method sends a CoAP ACK message on which a dummy CoAP response is piggybacked.
     *
     * @param[in]  aRequestHeader  A reference to the CoAP Header that was used in CoAP request.
     * @param[in]  aMessageInfo    The message info corresponding to the CoAP request.
     *
     * @retval kThreadError_None         Successfully enqueued the CoAP response message.
     * @retval kThreadError_NoBufs       Insufficient buffers available to send the CoAP response.
     * @retval kThreadError_InvalidArgs  The @p aRequestHeader header is not of confirmable type.
     *
     */
    ThreadError SendEmptyAck(const Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method sets CoAP server's port number.
     *
     * @param[in]  aPort  A port number to set.
     *
     * @retval kThreadError_None  Binding with a port succeeded.
     *
     */
    ThreadError SetPort(uint16_t aPort);

    const MessageQueue &GetCachedResponses(void) const { return mResponsesQueue.GetResponses(); }

    /**
     * This method sets interceptor to be called before processing a CoAP packet.
     *
     * @param[in]   aInterceptor    A pointer to the interceptor.
     *
     */
    void SetInterceptor(Interceptor aInterpreter) {
        mInterceptor = aInterpreter;
    }

protected:
    virtual void Receive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    virtual ThreadError Send(Message &aMessage, const Ip6::MessageInfo &aMessageInfo) {
        return mSocket.SendTo(aMessage, aMessageInfo);
    }

private:
    uint16_t mPort;
    Resource *mResources;

    Interceptor    mInterceptor;
    ResponsesQueue mResponsesQueue;
};


/**
 * @}
 *
 */

}  // namespace Coap
}  // namespace ot

#endif  // COAP_SERVER_HPP_
