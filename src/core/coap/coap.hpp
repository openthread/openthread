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

#ifndef COAP_HPP_
#define COAP_HPP_

#include "openthread-core-config.h"

#include <openthread/coap.h>

#include "coap/coap_header.hpp"
#include "common/debug.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"

/**
 * @file
 *   This file includes definitions for CoAP client and server functionality.
 */

namespace ot {

class ThreadNetif;

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
 * This class implements metadata required for CoAP retransmission.
 *
 */
OT_TOOL_PACKED_BEGIN
class CoapMetadata
{
    friend class CoapBase;

public:

    /**
     * Default constructor for the object.
     *
     */
    CoapMetadata(void):
        mDestinationPort(0),
        mResponseHandler(NULL),
        mResponseContext(NULL),
        mNextTimerShot(0),
        mRetransmissionTimeout(0),
        mRetransmissionCount(0),
        mAcknowledged(false),
        mConfirmable(false) {};

    /**
     * This constructor initializes the object with specific values.
     *
     * @param[in]  aConfirmable  Information if the request is confirmable or not.
     * @param[in]  aMessageInfo  Addressing information.
     * @param[in]  aHandler      Pointer to a handler function for the response.
     * @param[in]  aContext      Context for the handler function.
     *
     */
    CoapMetadata(bool aConfirmable, const Ip6::MessageInfo &aMessageInfo,
                 otCoapResponseHandler aHandler, void *aContext);

    /**
     * This method appends request data to the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the bytes.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to grow the message.
     *
     */
    otError AppendTo(Message &aMessage) const {
        return aMessage.Append(this, sizeof(*this));
    };

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
    };

    /**
     * This method updates request data in the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @returns The number of bytes updated.
     *
     */
    int UpdateIn(Message &aMessage) const {
        return aMessage.Write(aMessage.GetLength() - sizeof(*this), sizeof(*this), this);
    }

    /**
     * This method checks if the message shall be sent before the given time.
     *
     * @param[in]  aTime  A time to compare.
     *
     * @retval TRUE   If the message shall be sent before the given time.
     * @retval FALSE  Otherwise.
     */
    bool IsEarlier(uint32_t aTime) const { return (static_cast<int32_t>(aTime - mNextTimerShot) > 0); };

    /**
     * This method checks if the message shall be sent after the given time.
     *
     * @param[in]  aTime  A time to compare.
     *
     * @retval TRUE   If the message shall be sent after the given time.
     * @retval FALSE  Otherwise.
     */
    bool IsLater(uint32_t aTime) const { return (static_cast<int32_t>(aTime - mNextTimerShot) < 0); };

private:
    Ip6::Address          mSourceAddress;         ///< IPv6 address of the message source.
    Ip6::Address          mDestinationAddress;    ///< IPv6 address of the message destination.
    uint16_t              mDestinationPort;       ///< UDP port of the message destination.
    otCoapResponseHandler mResponseHandler;       ///< A function pointer that is called on response reception.
    void                  *mResponseContext;      ///< A pointer to arbitrary context information.
    uint32_t              mNextTimerShot;         ///< Time when the timer should shoot for this message.
    uint32_t              mRetransmissionTimeout; ///< Delay that is applied to next retransmission.
    uint8_t               mRetransmissionCount;   ///< Number of retransmissions.
    bool                  mAcknowledged: 1;       ///< Information that request was acknowledged.
    bool                  mConfirmable: 1;        ///< Information that message is confirmable.
} OT_TOOL_PACKED_END;

/**
 * This class implements CoAP resource handling.
 *
 */
class Resource : public otCoapResource
{
    friend class CoapBase;

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
    void HandleRequest(Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const {
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
        mDequeueTime(TimerMilli::GetNow() + TimerMilli::SecToMsec(kExchangeLifetime)),
        mMessageInfo(aMessageInfo) {}

    /**
     * This method append metadata to the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the bytes.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to grow the message.
     */
    otError AppendTo(Message &aMessage) const { return aMessage.Append(this, sizeof(*this)); }

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
        assert(aMessage.SetLength(aMessage.GetLength() - sizeof(EnqueuedResponseHeader)) == OT_ERROR_NONE);
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
     * @param[in]  aInstance  A reference to the OpenThread instance.
     * @param[in]  aHandler   A timer handler provided by owner of `RespponseQueue`.
     * @param[in]  aContext   A pointer to arbitrary context information (used along with timer handler).
     *
     */
    ResponsesQueue(Instance &aInstance, Timer::Handler aHandler, void *aContext);

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
     * @retval OT_ERROR_NONE       Matching response found and successfully created a copy.
     * @retval OT_ERROR_NO_BUFS    Matching response found but there is not sufficient buffer to create a copy.
     * @retval OT_ERROR_NOT_FOUND  Matching response not found.
     *
     */
    otError GetMatchedResponseCopy(const Header &aHeader,
                                   const Ip6::MessageInfo &aMessageInfo,
                                   Message **aResponse);

    /**
     * Get a copy of CoAP response from the cache that matches given Message ID and source endpoint.
     *
     * @param[in]  aRequest      The CoAP message containing Message ID.
     * @param[in]  aMessageInfo  The message info containing source endpoint address and port.
     * @param[out] aResponse     A pointer to a copy of a cached CoAP response matching given arguments.
     *
     * @retval OT_ERROR_NONE       Matching response found and successfully created a copy.
     * @retval OT_ERROR_NO_BUFS    Matching response found but there is not sufficient buffer to create a copy.
     * @retval OT_ERROR_NOT_FOUND  Matching response not found.
     * @retval OT_ERROR_PARSE      Could not parse CoAP header in the request message.
     *
     */
    otError GetMatchedResponseCopy(const Message &aRequest,
                                   const Ip6::MessageInfo &aMessageInfo,
                                   Message **aResponse);

    /**
     * Get a reference to the cached CoAP responses queue.
     *
     * @returns  A reference to the cached CoAP responses queue.
     *
     */
    const MessageQueue &GetResponses(void) const { return mQueue; }

    /**
     * Callback handler for timer.
     *
     * This method must be invoked by the owner of `ResponsesQueue` instance when the timer expires from the `aHandler`
     * callback function provided in the constructor.
     *
     */
    void HandleTimer(void);

private:
    enum
    {
        kMaxCachedResponses = OPENTHREAD_CONFIG_COAP_SERVER_MAX_CACHED_RESPONSES,
    };

    void DequeueResponse(Message &aMessage) { mQueue.Dequeue(aMessage); aMessage.Free(); }

    MessageQueue mQueue;
    TimerMilli   mTimer;
};

/**
 * This class implements the common base for CoAP client and server.
 *
 */
class CoapBase: public InstanceLocator
{
    friend class ResponsesQueue;

public:
    /**
     * This function pointer is called before CoAP server processing a CoAP packets.
     *
     * @param[in]   aMessage        A reference to the message.
     @ @param[in]   aMessageInfo    A reference to the message info associated with @p aMessage.
     * @param[in]   aContext        A pointer to arbitrary context information.
     *
     * @retval  OT_ERROR_NONE       Server should continue processing this message, other
     *                              return values indicates the server should stop processing
     *                              this message.
     * @retval  OT_ERROR_NOT_TMF    The message is not a TMF message.
     *
     */
    typedef otError(* Interceptor)(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, void *aContext);

    /**
     * This method starts the CoAP service.
     *
     * @param[in]  aPort  The local UDP port to bind to.
     *
     * @retval OT_ERROR_NONE  Successfully started the CoAP service.
     *
     */
    otError Start(uint16_t aPort);

    /**
     * This method stops the CoAP service.
     *
     * @retval OT_ERROR_NONE  Successfully stopped the CoAP service.
     *
     */
    otError Stop(void);

    /**
     * This method returns a port number used by CoAP service.
     *
     * @returns A port number.
     *
     */
    uint16_t GetPort(void) { return mSocket.GetSockName().mPort; };

    /**
     * This method adds a resource to the CoAP server.
     *
     * @param[in]  aResource  A reference to the resource.
     *
     * @retval OT_ERROR_NONE     Successfully added @p aResource.
     * @retval OT_ERROR_ALREADY  The @p aResource was already added.
     *
     */
    otError AddResource(Resource &aResource);

    /**
     * This method removes a resource from the CoAP server.
     *
     * @param[in]  aResource  A reference to the resource.
     *
     */
    void RemoveResource(Resource &aResource);

    /* This function sets the default handler for unhandled CoAP requests.
     *
     * @param[in]  aHandler   A function pointer that shall be called when an unhandled request arrives.
     * @param[in]  aContext   A pointer to arbitrary context information. May be NULL if not used.
     */
    void SetDefaultHandler(otCoapRequestHandler aHandler, void *aContext);

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
     * This method sends a CoAP message.
     *
     * If a response for a request is expected, respective function and context information should be provided.
     * If no response is expected, these arguments should be NULL pointers.
     * If Message Id was not set in the header (equal to 0), this function will assign unique Message Id to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE     Successfully sent CoAP message.
     * @retval OT_ERROR_NO_BUFS  Failed to allocate retransmission data.
     *
     */
    otError SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                        otCoapResponseHandler aHandler = NULL, void *aContext = NULL);

    /**
     * This method sends a CoAP reset message.
     *
     * @param[in]  aRequestHeader  A reference to the CoAP Header that was used in CoAP request.
     * @param[in]  aMessageInfo    The message info corresponding to the CoAP request.
     *
     * @retval OT_ERROR_NONE          Successfully enqueued the CoAP response message.
     * @retval OT_ERROR_NO_BUFS       Insufficient buffers available to send the CoAP response.
     * @retval OT_ERROR_INVALID_ARGS  The @p aRequestHeader header is not of confirmable type.
     *
     */
    otError SendReset(Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo) {
        return SendEmptyMessage(OT_COAP_TYPE_RESET, aRequestHeader, aMessageInfo);
    };

    /**
     * This method sends header-only CoAP response message.
     *
     * @param[in]  aCode           The CoAP code of this response.
     * @param[in]  aRequestHeader  A reference to the CoAP Header that was used in CoAP request.
     * @param[in]  aMessageInfo    The message info corresponding to the CoAP request.
     *
     * @retval OT_ERROR_NONE          Successfully enqueued the CoAP response message.
     * @retval OT_ERROR_NO_BUFS       Insufficient buffers available to send the CoAP response.
     * @retval OT_ERROR_INVALID_ARGS  The @p aRequestHeader header is not of confirmable type.
     *
     */
    otError SendHeaderResponse(Header::Code aCode, const Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method sends a CoAP ACK empty message which is used in Separate Response for confirmable requests.
     *
     * @param[in]  aRequestHeader  A reference to the CoAP Header that was used in CoAP request.
     * @param[in]  aMessageInfo    The message info corresponding to the CoAP request.
     *
     * @retval OT_ERROR_NONE          Successfully enqueued the CoAP response message.
     * @retval OT_ERROR_NO_BUFS       Insufficient buffers available to send the CoAP response.
     * @retval OT_ERROR_INVALID_ARGS  The @p aRequestHeader header is not of confirmable type.
     *
     */
    otError SendAck(Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo) {
        return SendEmptyMessage(OT_COAP_TYPE_ACKNOWLEDGMENT, aRequestHeader, aMessageInfo);
    };

    /**
     * This method sends a CoAP ACK message on which a dummy CoAP response is piggybacked.
     *
     * @param[in]  aRequestHeader  A reference to the CoAP Header that was used in CoAP request.
     * @param[in]  aMessageInfo    The message info corresponding to the CoAP request.
     *
     * @retval OT_ERROR_NONE          Successfully enqueued the CoAP response message.
     * @retval OT_ERROR_NO_BUFS       Insufficient buffers available to send the CoAP response.
     * @retval OT_ERROR_INVALID_ARGS  The @p aRequestHeader header is not of confirmable type.
     *
     */
    otError SendEmptyAck(const Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo) {
        return (aRequestHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE ?
                SendHeaderResponse(OT_COAP_CODE_CHANGED, aRequestHeader, aMessageInfo) :
                OT_ERROR_INVALID_ARGS);
    }

    /**
     * This method sends a header-only CoAP message to indicate no resource matched for the request.
     *
     * @param[in]  aRequestHeader        A reference to the CoAP Header that was used in CoAP request.
     * @param[in]  aMessageInfo          The message info corresponding to the CoAP request.
     *
     * @retval OT_ERROR_NONE         Successfully enqueued the CoAP response message.
     * @retval OT_ERROR_NO_BUFS      Insufficient buffers available to send the CoAP response.
     *
     */
    otError SendNotFound(const Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo) {
        return SendHeaderResponse(OT_COAP_CODE_NOT_FOUND, aRequestHeader, aMessageInfo);
    }

    /**
     * This method aborts CoAP transactions associated with given handler and context.
     *
     * The associated response handler will be called with OT_ERROR_ABORT.
     *
     * @param[in]  aHandler  A function pointer that should be called when the transaction ends.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE       Successfully aborted CoAP transactions.
     * @retval OT_ERROR_NOT_FOUND  CoAP transaction associated with given handler was not found.
     *
     */
    otError AbortTransaction(otCoapResponseHandler aHandler, void *aContext);

    /**
     * This method sets interceptor to be called before processing a CoAP packet.
     *
     * @param[in]   aInterceptor    A pointer to the interceptor.
     * @param[in]   aContext        A pointer to arbitrary context information.
     *
     */
    void SetInterceptor(Interceptor aInterceptor, void *aContext) {
        mInterceptor = aInterceptor;
        mContext = aContext;
    }

    /**
     * This method returns a reference to the request message list.
     *
     * @returns A reference to the request message list.
     *
     */
    const MessageQueue &GetRequestMessages(void) const { return mPendingRequests; }

    /**
     * This method returns a reference to the cached response list.
     *
     * @returns A reference to the cached response list.
     *
     */
    const MessageQueue &GetCachedResponses(void) const { return mResponsesQueue.GetResponses(); }

protected:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance                      A reference to the OpenThread instance.
     * @param[in]  aRetransmissionTimerHandler    A timer handler provided by sub-class for `mRetranmissionTimer`.
     * @param[in]  aResponsesQueueTimerHandler    A timer handler provided by sub-class for `mReponsesQueue` timer.
     *
     */
    CoapBase(Instance &aInstance, Timer::Handler aRetransmissionTimerHandler,
             Timer::Handler aResponsesQueueTimerHandler);

    /**
     * Retransmission timer handler.
     *
     * This method must be invoked by sub-class when the timer expires from the `aRetransmissionTimerHandler`
     * callback function provided in the constructor.
     *
     */
    void HandleRetransmissionTimer(void);

    /**
     * `ResponsesQueue` timer handler.
     *
     * This method must be invoked by sub-class when the timer expires from the `aResponsesQueueTimerHandler`
     * callback function provided in the constructor.
     *
     */
    void HandleResponsesQueueTimer(void) { mResponsesQueue.HandleTimer(); }

    /**
     * This method sends a message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     */
    virtual otError Send(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method receives a message.
     *
     * @param[in]  aMessage      A reference to the received message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     */
    virtual void Receive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    Ip6::UdpSocket mSocket;

private:
    enum
    {
        kDefaultCoapMessagePriority = Message::kPriorityLow,
    };

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);

    Message *CopyAndEnqueueMessage(const Message &aMessage, uint16_t aCopyLength,
                                   const CoapMetadata &aCoapMetadata);
    void DequeueMessage(Message &aMessage);
    Message *FindRelatedRequest(const Header &aResponseHeader, const Ip6::MessageInfo &aMessageInfo,
                                Header &aRequestHeader, CoapMetadata &aCoapMetadata);
    void FinalizeCoapTransaction(Message &aRequest, const CoapMetadata &aCoapMetadata, Header *aResponseHeader,
                                 Message *aResponse, const Ip6::MessageInfo *aMessageInfo, otError aResult);

    void ProcessReceivedRequest(Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void ProcessReceivedResponse(Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    otError SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError SendEmptyMessage(Header::Type aType, const Header &aRequestHeader,
                             const Ip6::MessageInfo &aMessageInfo);

    MessageQueue mPendingRequests;
    uint16_t mMessageId;
    TimerMilli mRetransmissionTimer;

    Resource *mResources;

    void           *mContext;
    Interceptor    mInterceptor;
    ResponsesQueue mResponsesQueue;

    otCoapRequestHandler mDefaultHandler;
    void *mDefaultHandlerContext;
};

/**
 * This class implements the CoAP client and server.
 *
 */
class Coap: public CoapBase
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in] aInstance      A reference to the OpenThread instance.
     *
     */
    explicit Coap(Instance &aInstance);

private:
    static void HandleRetransmissionTimer(Timer &aTimer);
    static void HandleResponsesQueueTimer(Timer &aTimer);
};

#if OPENTHREAD_ENABLE_APPLICATION_COAP

/**
 * This class implements the application CoAP client and server.
 *
 */
class ApplicationCoap: public CoapBase
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in] aInstance      A reference to the OpenThread instance.
     *
     */
    explicit ApplicationCoap(Instance &aInstance);

private:
    static void HandleRetransmissionTimer(Timer &aTimer);
    static void HandleResponsesQueueTimer(Timer &aTimer);
};

#endif

}  // namespace Coap
}  // namespace ot

#endif  // COAP_HPP_
