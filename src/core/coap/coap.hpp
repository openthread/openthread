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

#include "coap/coap_message.hpp"
#include "common/debug.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"

/**
 * @file
 *   This file includes definitions for CoAP client and server functionality.
 */

namespace ot {

namespace Coap {

/**
 * @addtogroup core-coap
 *
 * @{
 *
 */

/**
 * This type represents a function pointer which is called when a CoAP response is received or on the request timeout.
 *
 * Please see otCoapResponseHandler for details.
 *
 */
typedef otCoapResponseHandler ResponseHandler;

/**
 * This type represents a function pointer which is called when a CoAP request associated with a given URI path is
 * received.
 *
 * Please see otCoapRequestHandler for details.
 *
 */
typedef otCoapRequestHandler RequestHandler;

/**
 * This structure represents the CoAP transmission parameters.
 *
 */
class TxParameters : public otCoapTxParameters
{
    friend class CoapBase;
    friend class ResponsesQueue;

public:
    /**
     * This static method coverts a pointer to `otCoapTxParameters` to `Coap::TxParamters`
     *
     * If the pointer is nullptr, the default parameters are used instead.
     *
     * @param[in] aTxParameters   A pointer to tx parameter.
     *
     * @returns A reference to corresponding `TxParamters` if  @p aTxParameters is not nullptr, otherwise the default tx
     * parameters.
     *
     */
    static const TxParameters &From(const otCoapTxParameters *aTxParameters)
    {
        return aTxParameters ? *static_cast<const TxParameters *>(aTxParameters) : GetDefault();
    }

    /**
     * This method validates whether the CoAP transmission parameters are valid.
     *
     * @returns Whether the parameters are valid.
     *
     */
    bool IsValid(void) const;

    /**
     * This static method returns default CoAP tx parameters.
     *
     * @returns The default tx parameters.
     *
     */
    static const TxParameters &GetDefault(void) { return static_cast<const TxParameters &>(kDefaultTxParameters); }

private:
    enum
    {
        kDefaultAckTimeout                 = 2000, // in millisecond
        kDefaultAckRandomFactorNumerator   = 3,
        kDefaultAckRandomFactorDenominator = 2,
        kDefaultMaxRetransmit              = 4,
        kDefaultMaxLatency                 = 100000, // in millisecond
    };

    uint32_t CalculateInitialRetransmissionTimeout(void) const;
    uint32_t CalculateExchangeLifetime(void) const;
    uint32_t CalculateMaxTransmitWait(void) const;
    uint32_t CalculateSpan(uint8_t aMaxRetx) const;

    static const otCoapTxParameters kDefaultTxParameters;
};

/**
 * This class implements CoAP resource handling.
 *
 */
class Resource : public otCoapResource, public LinkedListEntry<Resource>
{
    friend class CoapBase;

public:
    enum
    {
        kMaxReceivedUriPath = 32, ///< Maximum supported URI path on received messages.
    };

    /**
     * This constructor initializes the resource.
     *
     * @param[in]  aUriPath  A pointer to a null-terminated string for the URI path.
     * @param[in]  aHandler  A function pointer that is called when receiving a CoAP message for @p aUriPath.
     * @param[in]  aContext  A pointer to arbitrary context information.
     */
    Resource(const char *aUriPath, RequestHandler aHandler, void *aContext)
    {
        mUriPath = aUriPath;
        mHandler = aHandler;
        mContext = aContext;
        mNext    = nullptr;
    }

    /**
     * This method returns a pointer to the URI path.
     *
     * @returns A pointer to the URI path.
     *
     */
    const char *GetUriPath(void) const { return mUriPath; }

private:
    void HandleRequest(Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
    {
        mHandler(mContext, &aMessage, &aMessageInfo);
    }
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
     *
     */
    explicit ResponsesQueue(Instance &aInstance);

    /**
     * This method adds a given response to the cache.
     *
     * If matching response (the same Message ID, source endpoint address and port) exists in the cache given
     * response is not added.
     *
     * The CoAP response is copied before it is added to the cache.
     *
     * @param[in]  aMessage      The CoAP response to add to the cache.
     * @param[in]  aMessageInfo  The message info corresponding to @p aMessage.
     * @param[in]  aTxParameters Transmission parameters.
     *
     */
    void EnqueueResponse(Message &aMessage, const Ip6::MessageInfo &aMessageInfo, const TxParameters &aTxParameters);

    /**
     * This method removes all responses from the cache.
     *
     */
    void DequeueAllResponses(void);

    /**
     * This method gets a copy of CoAP response from the cache that matches a given Message ID and source endpoint.
     *
     * @param[in]  aRequest      The CoAP message containing Message ID.
     * @param[in]  aMessageInfo  The message info containing source endpoint address and port.
     * @param[out] aResponse     A pointer to return a copy of a cached CoAP response matching given arguments.
     *
     * @retval OT_ERROR_NONE       Matching response found and successfully created a copy.
     * @retval OT_ERROR_NO_BUFS    Matching response found but there is not sufficient buffer to create a copy.
     * @retval OT_ERROR_NOT_FOUND  Matching response not found.
     *
     */
    otError GetMatchedResponseCopy(const Message &aRequest, const Ip6::MessageInfo &aMessageInfo, Message **aResponse);

    /**
     * This method gets a reference to the cached CoAP responses queue.
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

    struct ResponseMetadata
    {
        otError AppendTo(Message &aMessage) const { return aMessage.Append(this, sizeof(*this)); }
        void    ReadFrom(const Message &aMessage);

        TimeMilli        mDequeueTime;
        Ip6::MessageInfo mMessageInfo;
    };

    const Message *FindMatchedResponse(const Message &aRequest, const Ip6::MessageInfo &aMessageInfo) const;
    void           DequeueResponse(Message &aMessage);
    void           UpdateQueue(void);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    MessageQueue      mQueue;
    TimerMilliContext mTimer;
};

/**
 * This class implements the CoAP client and server.
 *
 */
class CoapBase : public InstanceLocator, private NonCopyable
{
    friend class ResponsesQueue;

public:
    /**
     * This function pointer is called before CoAP server processing a CoAP message.
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
    typedef otError (*Interceptor)(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, void *aContext);

    /**
     * This method clears requests and responses used by this CoAP agent.
     *
     */
    void ClearRequestsAndResponses(void);

    /**
     * This method clears requests with specified source address used by this CoAP agent.
     *
     * @param[in]  aAddress A reference to the specified address.
     *
     */
    void ClearRequests(const Ip6::Address &aAddress);

    /**
     * This method adds a resource to the CoAP server.
     *
     * @param[in]  aResource  A reference to the resource.
     *
     */
    void AddResource(Resource &aResource);

    /**
     * This method removes a resource from the CoAP server.
     *
     * @param[in]  aResource  A reference to the resource.
     *
     */
    void RemoveResource(Resource &aResource);

    /* This method sets the default handler for unhandled CoAP requests.
     *
     * @param[in]  aHandler   A function pointer that shall be called when an unhandled request arrives.
     * @param[in]  aContext   A pointer to arbitrary context information. May be nullptr if not used.
     *
     */
    void SetDefaultHandler(RequestHandler aHandler, void *aContext);

    /**
     * This method creates a new message with a CoAP header.
     *
     * @param[in]  aSettings  The message settings.
     *
     * @returns A pointer to the message or nullptr if failed to allocate message.
     *
     */
    Message *NewMessage(const Message::Settings &aSettings = Message::Settings::GetDefault());

    /**
     * This method creates a new message with a CoAP header that has Network Control priority level.
     *
     * @returns A pointer to the message or nullptr if failed to allocate message.
     *
     */
    Message *NewPriorityMessage(void)
    {
        return NewMessage(Message::Settings(Message::kWithLinkSecurity, Message::kPriorityNet));
    }

    /**
     * This method sends a CoAP message with custom transmission parameters.
     *
     * If a response for a request is expected, respective function and context information should be provided.
     * If no response is expected, these arguments should be nullptr pointers.
     * If Message Id was not set in the header (equal to 0), this function will assign unique Message Id to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aTxParameters A reference to transmission parameters for this message.
     * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE     Successfully sent CoAP message.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to send the CoAP message.
     *
     */
    otError SendMessage(Message &               aMessage,
                        const Ip6::MessageInfo &aMessageInfo,
                        const TxParameters &    aTxParameters,
                        ResponseHandler         aHandler = nullptr,
                        void *                  aContext = nullptr);

    /**
     * This method sends a CoAP message with default transmission parameters.
     *
     * If a response for a request is expected, respective function and context information should be provided.
     * If no response is expected, these arguments should be nullptr pointers.
     * If Message Id was not set in the header (equal to 0), this function will assign unique Message Id to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE     Successfully sent CoAP message.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to send the CoAP response.
     *
     */
    otError SendMessage(Message &               aMessage,
                        const Ip6::MessageInfo &aMessageInfo,
                        ResponseHandler         aHandler = nullptr,
                        void *                  aContext = nullptr);

    /**
     * This method sends a CoAP reset message.
     *
     * @param[in]  aRequest        A reference to the CoAP Message that was used in CoAP request.
     * @param[in]  aMessageInfo    The message info corresponding to the CoAP request.
     *
     * @retval OT_ERROR_NONE          Successfully enqueued the CoAP response message.
     * @retval OT_ERROR_NO_BUFS       Insufficient buffers available to send the CoAP response.
     * @retval OT_ERROR_INVALID_ARGS  The @p aRequest is not of confirmable type.
     *
     */
    otError SendReset(Message &aRequest, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method sends header-only CoAP response message.
     *
     * @param[in]  aCode           The CoAP code of this response.
     * @param[in]  aRequest        A reference to the CoAP Message that was used in CoAP request.
     * @param[in]  aMessageInfo    The message info corresponding to the CoAP request.
     *
     * @retval OT_ERROR_NONE          Successfully enqueued the CoAP response message.
     * @retval OT_ERROR_NO_BUFS       Insufficient buffers available to send the CoAP response.
     * @retval OT_ERROR_INVALID_ARGS  The @p aRequest header is not of confirmable type.
     *
     */
    otError SendHeaderResponse(Message::Code aCode, const Message &aRequest, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method sends a CoAP ACK empty message which is used in Separate Response for confirmable requests.
     *
     * @param[in]  aRequest        A reference to the CoAP Message that was used in CoAP request.
     * @param[in]  aMessageInfo    The message info corresponding to the CoAP request.
     *
     * @retval OT_ERROR_NONE          Successfully enqueued the CoAP response message.
     * @retval OT_ERROR_NO_BUFS       Insufficient buffers available to send the CoAP response.
     * @retval OT_ERROR_INVALID_ARGS  The @p aRequest header is not of confirmable type.
     *
     */
    otError SendAck(const Message &aRequest, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method sends a CoAP ACK message on which a dummy CoAP response is piggybacked.
     *
     * @param[in]  aRequest        A reference to the CoAP Message that was used in CoAP request.
     * @param[in]  aMessageInfo    The message info corresponding to the CoAP request.
     *
     * @retval OT_ERROR_NONE          Successfully enqueued the CoAP response message.
     * @retval OT_ERROR_NO_BUFS       Insufficient buffers available to send the CoAP response.
     * @retval OT_ERROR_INVALID_ARGS  The @p aRequest header is not of confirmable type.
     *
     */
    otError SendEmptyAck(const Message &aRequest, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method sends a header-only CoAP message to indicate no resource matched for the request.
     *
     * @param[in]  aRequest        A reference to the CoAP Message that was used in CoAP request.
     * @param[in]  aMessageInfo          The message info corresponding to the CoAP request.
     *
     * @retval OT_ERROR_NONE         Successfully enqueued the CoAP response message.
     * @retval OT_ERROR_NO_BUFS      Insufficient buffers available to send the CoAP response.
     *
     */
    otError SendNotFound(const Message &aRequest, const Ip6::MessageInfo &aMessageInfo);

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
    otError AbortTransaction(ResponseHandler aHandler, void *aContext);

    /**
     * This method sets interceptor to be called before processing a CoAP packet.
     *
     * @param[in]   aInterceptor    A pointer to the interceptor.
     * @param[in]   aContext        A pointer to arbitrary context information.
     *
     */
    void SetInterceptor(Interceptor aInterceptor, void *aContext);

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
     * This function pointer is called to send a CoAP message.
     *
     * @param[in]  aCoapBase     A reference to the CoAP agent.
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     * @retval OT_ERROR_NONE     Successfully sent CoAP message.
     * @retval OT_ERROR_NO_BUFS  Failed to allocate retransmission data.
     *
     */
    typedef otError (*Sender)(CoapBase &aCoapBase, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance        A reference to the OpenThread instance.
     * @param[in]  aSender          A function pointer to send CoAP message, which SHOULD be a static
     *                              member method of a descendant of this class.
     *
     */
    CoapBase(Instance &aInstance, Sender aSender);

    /**
     * This method receives a CoAP message.
     *
     * @param[in]  aMessage      A reference to the received message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     */
    void Receive(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

private:
    struct Metadata
    {
        otError AppendTo(Message &aMessage) const { return aMessage.Append(this, sizeof(*this)); }
        void    ReadFrom(const Message &aMessage);
        void    UpdateIn(Message &aMessage) const;

        Ip6::Address    mSourceAddress;            // IPv6 address of the message source.
        Ip6::Address    mDestinationAddress;       // IPv6 address of the message destination.
        uint16_t        mDestinationPort;          // UDP port of the message destination.
        ResponseHandler mResponseHandler;          // A function pointer that is called on response reception.
        void *          mResponseContext;          // A pointer to arbitrary context information.
        TimeMilli       mNextTimerShot;            // Time when the timer should shoot for this message.
        uint32_t        mRetransmissionTimeout;    // Delay that is applied to next retransmission.
        uint8_t         mRetransmissionsRemaining; // Number of retransmissions remaining.
        bool            mAcknowledged : 1;         // Information that request was acknowledged.
        bool            mConfirmable : 1;          // Information that message is confirmable.
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        bool mObserve : 1; // Information that this request involves Observations.
#endif
    };

    static void HandleRetransmissionTimer(Timer &aTimer);
    void        HandleRetransmissionTimer(void);

    void     ClearRequests(const Ip6::Address *aAddress);
    Message *CopyAndEnqueueMessage(const Message &aMessage, uint16_t aCopyLength, const Metadata &aMetadata);
    void     DequeueMessage(Message &aMessage);
    Message *FindRelatedRequest(const Message &aResponse, const Ip6::MessageInfo &aMessageInfo, Metadata &aMetadata);
    void     FinalizeCoapTransaction(Message &               aRequest,
                                     const Metadata &        aMetadata,
                                     Message *               aResponse,
                                     const Ip6::MessageInfo *aMessageInfo,
                                     otError                 aResult);

    void ProcessReceivedRequest(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void ProcessReceivedResponse(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void    SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError SendEmptyMessage(Type aType, const Message &aRequest, const Ip6::MessageInfo &aMessageInfo);

    otError Send(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    MessageQueue      mPendingRequests;
    uint16_t          mMessageId;
    TimerMilliContext mRetransmissionTimer;

    LinkedList<Resource> mResources;

    void *         mContext;
    Interceptor    mInterceptor;
    ResponsesQueue mResponsesQueue;

    RequestHandler mDefaultHandler;
    void *         mDefaultHandlerContext;

    const Sender mSender;
};

/**
 * This class implements the CoAP client and server.
 *
 */
class Coap : public CoapBase
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in] aInstance      A reference to the OpenThread instance.
     *
     */
    explicit Coap(Instance &aInstance);

    /**
     * This method starts the CoAP service.
     *
     * @param[in]  aPort             The local UDP port to bind to.
     * @param[in]  aNetifIdentifier  The network interface identifier to bind.
     *
     * @retval OT_ERROR_NONE    Successfully started the CoAP service.
     * @retval OT_ERROR_FAILED  Failed to start CoAP agent.
     *
     */
    otError Start(uint16_t aPort, otNetifIdentifier aNetifIdentifier = OT_NETIF_UNSPECIFIED);

    /**
     * This method stops the CoAP service.
     *
     * @retval OT_ERROR_NONE    Successfully stopped the CoAP service.
     * @retval OT_ERROR_FAILED  Failed to stop CoAP agent.
     *
     */
    otError Stop(void);

protected:
    Ip6::Udp::Socket mSocket;

private:
    static otError Send(CoapBase &aCoapBase, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    static void    HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    otError        Send(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
};

} // namespace Coap
} // namespace ot

#endif // COAP_HPP_
