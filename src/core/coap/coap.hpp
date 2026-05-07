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

#ifndef OT_CORE_COAP_COAP_HPP_
#define OT_CORE_COAP_COAP_HPP_

#include "openthread-core-config.h"

#include <openthread/coap.h>

#include "coap/coap_message.hpp"
#include "common/as_core_type.hpp"
#include "common/callback.hpp"
#include "common/debug.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/message_allocator.hpp"
#include "common/non_copyable.hpp"
#include "common/owned_ptr.hpp"
#include "common/timer.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"
#include "thread/uri_paths.hpp"

/**
 * @file
 *   This file includes definitions for CoAP client and server functionality.
 */

namespace ot {

class UnitTester;

namespace Coap {

/**
 * @addtogroup core-coap
 *
 * @{
 */

class Msg;
class CoapBase;

/**
 * Represents a function pointer which is called when a CoAP response is received or on the request timeout.
 *
 * This is the callback function definition used by the public `otCoap` APIs where the `aMessage` and `aMessageInfo`
 * are passed as separate parameters. Core modules should use `ResponseHandler` callback instead which gets a single
 * `Msg` input (encapsulating both message and its `Ip6::MessageInfo`).
 *
 * Please see otCoapResponseHandler for details.
 */
typedef otCoapResponseHandler ResponseHandlerSeparateParams;

/**
 * Represents a function pointer which is called when a CoAP response is received or on the request timeout
 *
 * @param[in]  aContext      A pointer to application-specific context.
 * @param[in]  aMessage      A pointer to the received `Msg` response. `nullptr` if no response was received.
 * @param[in]  aResult       A result of the CoAP transaction.
 *
 * @retval  kErrorNone              A response was received successfully.
 * @retval  kErrorAbort             The CoAP transaction was reset by peer.
 * @retval  kErrorResponseTimeout   No response or acknowledgment received during timeout period.
 */
typedef void (*ResponseHandler)(void *aContext, Msg *aMsg, Error aResult);

/**
 * Represents a function pointer which is called when a CoAP request associated with a given URI path is
 * received.
 *
 * Please see otCoapRequestHandler for details.
 */
typedef otCoapRequestHandler RequestHandler;

/**
 * Represents a function pointer which is called when a CoAP response is not associated with a sent request.
 *
 * Please see otCoapResponseFallback for details.
 */
typedef otCoapResponseFallback ResponseFallback;

/**
 * Represents the CoAP transmission parameters.
 */
class TxParameters : public otCoapTxParameters
{
    friend class CoapBase;
    friend class ResponseCache;

public:
    /**
     * Returns the default CoAP tx parameters.
     *
     * @returns The default tx parameters.
     */
    static const TxParameters &GetDefault(void);

private:
    static constexpr uint32_t kDefaultAckTimeout                 = 2000; // in msec
    static constexpr uint8_t  kDefaultAckRandomFactorNumerator   = 3;
    static constexpr uint8_t  kDefaultAckRandomFactorDenominator = 2;
    static constexpr uint8_t  kDefaultMaxRetransmit              = 4;
    static constexpr uint32_t kDefaultMaxLatency                 = 100000; // in msec
    static constexpr uint8_t  kMaxRetransmit                     = OT_COAP_MAX_RETRANSMIT;
    static constexpr uint32_t kMinAckTimeout                     = OT_COAP_MIN_ACK_TIMEOUT;

    Error    ValidateFor(const Msg &aMsg) const;
    uint32_t CalculateInitialRetransmissionTimeout(void) const;
    uint32_t CalculateExchangeLifetime(void) const;
    uint32_t CalculateMaxTransmitWait(void) const;
    uint32_t CalculateSpan(uint8_t aMaxRetx) const;

    static const otCoapTxParameters kDefaultTxParameters;
};

/**
 * Represents a CoAP message and its associated `Ip6::MessageInfo` along with parse CoAP Header information.
 */
class Msg : public HeaderInfo
{
    friend class CoapBase;
    friend class ot::UnitTester;

public:
    Message                &mMessage;     ///< The CoAP message.
    const Ip6::MessageInfo &mMessageInfo; ///< The `Ip6::MessageInfo` associated with the message.

private:
    Msg(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
        : mMessage(aMessage)
        , mMessageInfo(aMessageInfo)
    {
    }

    enum PayloadMarkerMode : uint8_t
    {
        kRejectIfNoPayloadWithPayloadMarker,
        kRemovePayloadMarkerIfNoPayload,
    };

    Error    ParseHeaderAndOptions(PayloadMarkerMode aPayloadMarkerMode);
    uint16_t GetHeaderSize(void) const;
    void     UpdateType(Type aType);
    void     UpdateMessageId(uint16_t aMessageId);
};

/**
 * Implements CoAP resource handling.
 */
class Resource : public otCoapResource, public LinkedListEntry<Resource>
{
    friend class CoapBase;

public:
    /**
     * Initializes the resource.
     *
     * @param[in]  aUriPath  A pointer to a null-terminated string for the URI path.
     * @param[in]  aHandler  A function pointer that is called when receiving a CoAP message for @p aUriPath.
     * @param[in]  aContext  A pointer to arbitrary context information.
     */
    Resource(const char *aUriPath, RequestHandler aHandler, void *aContext);

    /**
     * Initializes the resource.
     *
     * @param[in]  aUri      A Thread URI.
     * @param[in]  aHandler  A function pointer that is called when receiving a CoAP message for the URI.
     * @param[in]  aContext  A pointer to arbitrary context information.
     */
    Resource(Uri aUri, RequestHandler aHandler, void *aContext);

    /**
     * Returns a pointer to the URI path.
     *
     * @returns A pointer to the URI path.
     */
    const char *GetUriPath(void) const { return mUriPath; }

protected:
    void HandleRequest(Msg &aRxMsg) const { mHandler(mContext, &aRxMsg.mMessage, &aRxMsg.mMessageInfo); }
};

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

/**
 * Represents a function pointer which is called when a CoAP message with a block-wise transfer option is received.
 *
 * Please see `otCoapBlockwiseReceiveHook` for details.
 */
typedef otCoapBlockwiseReceiveHook BlockwiseReceiveHook;

/**
 * Represents a function pointer which is called before the next block in a block-wise transfer is sent.
 *
 * Please see `otCoapBlockwiseTransmitHook` for details.
 */
typedef otCoapBlockwiseTransmitHook BlockwiseTransmitHook;

/**
 * Implements CoAP block-wise resource handling.
 */
class ResourceBlockWise : public otCoapBlockwiseResource, public LinkedListEntry<ResourceBlockWise>
{
    friend class CoapBase;

public:
    /**
     * Initializes the resource.
     *
     * @param[in]  aUriPath         A pointer to a NULL-terminated string for the Uri-Path.
     * @param[in]  aHandler         A function pointer that is called when receiving a CoAP message for @p aUriPath.
     * @param[in]  aContext         A pointer to arbitrary context information.
     * @param[in]  aReceiveHook     A function pointer that is called when receiving a CoAP block message for @p
     *                              aUriPath.
     * @param[in]  aTransmitHook    A function pointer that is called when transmitting a CoAP block message from @p
     *                              aUriPath.
     */
    ResourceBlockWise(const char           *aUriPath,
                      RequestHandler        aHandler,
                      void                 *aContext,
                      BlockwiseReceiveHook  aReceiveHook,
                      BlockwiseTransmitHook aTransmitHook)
    {
        mUriPath      = aUriPath;
        mHandler      = aHandler;
        mContext      = aContext;
        mReceiveHook  = aReceiveHook;
        mTransmitHook = aTransmitHook;
        mNext         = nullptr;
    }

    /**
     * Returns a pointer to the URI path.
     *
     * @returns A pointer to the URI path.
     */
    const char *GetUriPath(void) const { return mUriPath; }

protected:
    void HandleRequest(Msg &aMsg) const { mHandler(mContext, &aMsg.mMessage, &aMsg.mMessageInfo); }

    Error HandleBlockReceive(const uint8_t *aBlock,
                             uint32_t       aPosition,
                             uint16_t       aBlockLength,
                             bool           aMore,
                             uint32_t       aTotalLength) const
    {
        return mReceiveHook(mContext, aBlock, aPosition, aBlockLength, aMore, aTotalLength);
    }

    Error HandleBlockTransmit(uint8_t *aBlock, uint32_t aPosition, uint16_t *aBlockLength, bool *aMore) const
    {
        return mTransmitHook(mContext, aBlock, aPosition, aBlockLength, aMore);
    }
};

#endif // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

/**
 * Implements the CoAP client and server.
 */
class CoapBase
    : public InstanceLocator,
      public MessageAllocator<CoapBase, ReservedHeaderSize::kCoapMessage, Message::kTypeIp6, ot::Coap::Message>,
      private NonCopyable
{
public:
    /**
     * Function pointer callback invoked before CoAP processing a received CoAP message.
     *
     * @param[in]   aContext    A pointer to arbitrary context information.
     * @param[in]   aRxMsg      The received `Msg` (CoAP message and its associated `Ip6::MessageInfo`).
     *
     * @retval  kErrorNone      Continue processing this message
     * @retval  kErrorNotTmf    The message is not a TMF message.
     */
    typedef Error (*Interceptor)(void *aContext, const Msg &aRxMsg);

    /**
     * Clears all requests and responses used by this CoAP agent and stops all timers.
     */
    void ClearAllRequestsAndResponses(void);

    /**
     * Clears requests with specified source address used by this CoAP agent.
     *
     * @param[in]  aAddress A reference to the specified address.
     */
    void ClearRequests(const Ip6::Address &aAddress) { mPendingRequests.AbortRequestsMatching(aAddress); }

    /**
     * Adds a resource to the CoAP server.
     *
     * @param[in]  aResource  A reference to the resource.
     */
    void AddResource(Resource &aResource);

    /**
     * Removes a resource from the CoAP server.
     *
     * @param[in]  aResource  A reference to the resource.
     */
    void RemoveResource(Resource &aResource);

    /**
     * Sets the default handler for unhandled CoAP requests.
     *
     * @param[in]  aHandler   A function pointer that shall be called when an unhandled request arrives.
     * @param[in]  aContext   A pointer to arbitrary context information. May be `nullptr` if not used.
     */
    void SetDefaultHandler(RequestHandler aHandler, void *aContext) { mDefaultHandler.Set(aHandler, aContext); }

    /**
     * Sets a fallback handler for CoAP responses not matching any active/pending request.
     *
     * @param[in]  aHandler   A function pointer that shall be called as a fallback for responses that do not match any
     *                        pending request.
     * @param[in]  aContext   A pointer to arbitrary context information. May be `nullptr` if not used.
     */
    void SetResponseFallback(ResponseFallback aHandler, void *aContext) { mResponseFallback.Set(aHandler, aContext); }

    /**
     * Allocates and initializes a new CoAP Confirmable Post message with Network Control priority level.
     *
     * The CoAP header is initialized as `kTypeConfirmable` and `kCodePost` with a given URI path and a randomly
     * generated token (of default length). This method also sets the payload marker (`AppendPayloadMarker()` on
     * message. Even if message has no payload, calling `AppendPayloadMarker()` is harmless, since `SendMessage()` will
     * check and remove the payload marker when there is no payload.
     *
     * @param[in] aUri      The URI.
     *
     * @returns A pointer to the message or `nullptr` if failed to allocate message.
     */
    Message *AllocateAndInitPriorityConfirmablePostMessage(Uri aUri);

    /**
     * Allocates and initializes a new CoAP Confirmable Post message with normal priority level.
     *
     * The CoAP header is initialized as `kTypeConfirmable` and `kCodePost` with a given URI and a randomly
     * generated token (of default length). This method also sets the payload marker (calling `AppendPayloadMarker()`).
     * Even if message has no payload, calling `AppendPayloadMarker()` is harmless, since `SendMessage()` will check and
     * remove the payload marker when there is no payload.
     *
     * @param[in] aUri      The URI.
     *
     * @returns A pointer to the message or `nullptr` if failed to allocate message.
     */
    Message *AllocateAndInitConfirmablePostMessage(Uri aUri);

    /**
     * Allocates and initializes a new CoAP Non-confirmable Post message with Network Control priority level.
     *
     * The CoAP header is initialized as `kTypeNonConfirmable` and `kCodePost` with a given URI and a randomly
     * generated token (of default length). This method also sets the payload marker (calling `AppendPayloadMarker()`).
     * Even if message has no payload, calling `AppendPayloadMarker()` is harmless, since `SendMessage()` will check and
     * remove the payload marker when there is no payload.
     *
     * @param[in] aUri      The URI.
     *
     * @returns A pointer to the message or `nullptr` if failed to allocate message.
     */
    Message *AllocateAndInitPriorityNonConfirmablePostMessage(Uri aUri);

    /**
     * Allocates and initializes a new CoAP Non-confirmable Post message with normal priority level.
     *
     * The CoAP header is initialized as `kTypeNonConfirmable` and `kCodePost` with a given URI and a randomly
     * generated token (of default length). This method also sets the payload marker (calling `AppendPayloadMarker()`).
     * Even if message has no payload, calling `AppendPayloadMarker()` is harmless, since `SendMessage()` will check and
     * remove the payload marker when there is no payload.
     *
     * @param[in] aUri      The URI.
     *
     * @returns A pointer to the message or `nullptr` if failed to allocate message.
     */
    Message *AllocateAndInitNonConfirmablePostMessage(Uri aUri);

    /**
     * Allocates and initializes a new CoAP Post message with normal priority level.
     *
     * If the provided @p aDestination address is multicast, the message will be Non-Confirmable. Otherwise, it will be
     * Confirmable.
     *
     * The CoAP header is initialized as either `kTypeNonConfirmable` or `kTypeConfirmable` and `kCodePost` with the
     * given URI and a randomly generated token (of default length). This method also sets the payload marker (calling
     * `AppendPayloadMarker()`). Even if the message has no payload, calling `AppendPayloadMarker()` is harmless, since
     * `SendMessage()` will check and remove the payload marker when there is no payload.
     *
     * @param[in] aUri         The URI.
     * @param[in] aDestination The destination IPv6 address.
     *
     * @returns A pointer to the message or `nullptr` if failed to allocate message.
     */
    Message *AllocateAndInitPostMessageTo(Uri aUri, const Ip6::Address &aDestination);

    /**
     * Allocates and initializes a new CoAP Post message with Network Control priority level.
     *
     * If the provided @p aDestination address is multicast, the message will be Non-Confirmable. Otherwise, it will be
     * Confirmable.
     *
     * The CoAP header is initialized as either `kTypeNonConfirmable` or `kTypeConfirmable` and `kCodePost` with the
     * given URI and a randomly generated token (of default length). This method also sets the payload marker (calling
     * `AppendPayloadMarker()`). Even if the message has no payload, calling `AppendPayloadMarker()` is harmless, since
     * `SendMessage()` will check and remove the payload marker when there is no payload.
     *
     * @param[in] aUri         The URI.
     * @param[in] aDestination The destination IPv6 address.
     *
     * @returns A pointer to the message or `nullptr` if failed to allocate message.
     */
    Message *AllocateAndInitPriorityPostMessageTo(Uri aUri, const Ip6::Address &aDestination);

    /**
     * Allocates and initializes a new CoAP response message with Network Control priority level for a
     * given request message.
     *
     * The CoAP header is initialized as `kTypeAck` with `kCodeChanged`. The token and message ID is copied from
     * @p aRequest. This method also sets the payload marker (calling `AppendPayloadMarker()`). Even if message has
     * no payload, calling `AppendPayloadMarker()` is harmless, since `SendMessage()` will check and remove the payload
     * marker when there is no payload.
     *
     * @returns A pointer to the message or `nullptr` if failed to allocate message.
     */
    Message *AllocateAndInitPriorityResponseFor(const Message &aRequest);

    /**
     * Allocates and initializes a new CoAP response message with regular priority level for a given
     * request message.
     *
     * The CoAP header is initialized as `kTypeAck` with `kCodeChanged`. The token and message ID is copied from
     * @p aRequest. This method also sets the payload marker (calling `AppendPayloadMarker()`). Even if message has
     * no payload, calling `AppendPayloadMarker()` is harmless, since `SendMessage()` will check and remove the payload
     * marker when there is no payload.
     *
     * @returns A pointer to the message or `nullptr` if failed to allocate message.
     */
    Message *AllocateAndInitResponseFor(const Message &aRequest);

    /**
     * Sends a CoAP message with custom transmission parameters.
     *
     * If a response for a request is expected, respective function and context information should be provided.
     * If no response is expected, these arguments should be `nullptr` pointers.
     * If Message ID was not set in the header (equal to 0), this method will assign unique Message ID to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aTxParameters A pointer to `TxParameters`. If `nullptr`, default `TxParameters` will be used.
     * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     * @retval kErrorNone    Successfully sent CoAP message.
     * @retval kErrorNoBufs  Insufficient buffers available to send the CoAP message.
     */
    Error SendMessage(Message                &aMessage,
                      const Ip6::MessageInfo &aMessageInfo,
                      const TxParameters     *aTxParameters,
                      ResponseHandler         aHandler,
                      void                   *aContext);

    /**
     * Sends a CoAP message with custom transmission parameters.
     *
     * If Message ID was not set in the header (equal to 0), this method will assign unique Message ID to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aTxParameters A reference to transmission parameters for this message.
     *
     * @retval kErrorNone    Successfully sent CoAP message.
     * @retval kErrorNoBufs  Insufficient buffers available to send the CoAP message.
     */
    Error SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo, const TxParameters &aTxParameters);

    /**
     * Sends a CoAP message with default transmission parameters.
     *
     * If Message ID was not set in the header (equal to 0), this method will assign unique Message ID to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     * @retval kErrorNone    Successfully sent CoAP message.
     * @retval kErrorNoBufs  Insufficient buffers available to send the CoAP response.
     */
    Error SendMessage(Message                &aMessage,
                      const Ip6::MessageInfo &aMessageInfo,
                      ResponseHandler         aHandler,
                      void                   *aContext);

    /**
     * Sends a CoAP message with default transmission parameters.
     *
     * If Message ID was not set in the header (equal to 0), this method will assign unique Message ID to the message.
     *
     * This flavor of `SendMessage()` accepts an `OwnedPtr<Message>` and therefore takes ownership of the passed-in
     * message.
     *
     * @param[in]  aMessage      An `OwnedPtr` to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     * @retval kErrorNone    Successfully sent CoAP message.
     * @retval kErrorNoBufs  Insufficient buffers available to send the CoAP response.
     */
    Error SendMessage(OwnedPtr<Message>       aMessage,
                      const Ip6::MessageInfo &aMessageInfo,
                      ResponseHandler         aHandler,
                      void                   *aContext);

    /**
     * Sends a CoAP message with default transmission parameters.
     *
     * If Message ID was not set in the header (equal to 0), this method will assign unique Message ID to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     * @retval kErrorNone    Successfully sent CoAP message.
     * @retval kErrorNoBufs  Insufficient buffers available to send the CoAP response.
     */
    Error SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * Sends a CoAP message with default transmission parameters.
     *
     * If Message ID was not set in the header (equal to 0), this method will assign unique Message ID to the message.
     *
     * This flavor of `SendMessage()` accepts an `OwnedPtr<Message>` and therefore takes ownership of the passed-in
     * message.
     *
     * @param[in]  aMessage      An `OwnedPtr` to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     * @retval kErrorNone    Successfully sent CoAP message.
     * @retval kErrorNoBufs  Insufficient buffers available to send the CoAP response.
     */
    Error SendMessage(OwnedPtr<Message> aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * Gets a copy of the request message currently being dispatched (`ResponseHandler` callback is invoked).
     *
     * This method can only be used from a `ResponseHandler` callback. It returns a copy of the original request
     * message if the request was confirmable.
     *
     * @param[out] aMessage    A reference to an `OwnedPtr` to return a copy of original request message.
     *
     * @retval kErrorNone      Successfully retrieved and cloned the request message.
     * @retval kErrorNotFound  Not currently dispatching a confirmable request.
     * @retval kErrorNoBufs    Failed to allocate a message for the clone.
     */
    Error GetDispatchingRequest(OwnedPtr<Message> &aMessage) const
    {
        return mPendingRequests.GetDispatchingRequest(aMessage);
    }

    /**
     * Sends an empty CoAP message (using `kCodeEmpty` Code 0.00).
     *
     * An empty CoAP message has no options and no payload (only a 4-byte header). This is typically used for sending
     * Empty Acknowledgments or Resets.
     *
     * @param[in]  aType           The CoAP Type of the empty message (e.g., `kTypeAck`, `kTypeReset`).
     * @param[in]  aRxMsg          The received CoAP message to which this message responds (provides Message ID).
     *
     * @retval kErrorNone          Successfully enqueued the CoAP message.
     * @retval kErrorNoBufs        Insufficient buffers available to send the CoAP message.
     * @retval kErrorInvalidArgs   The @p aType is not valid for an empty message (e.g., `kTypeNonConfirmable`),
     *                             or is `kTypeAck` but the @p aRxMsg is not confirmable.
     */
    Error SendEmptyMessage(Type aType, const Msg &aRxMsg);

    /**
     * Sends a CoAP response message without a payload to a given request message.
     *
     * The response will dynamically be an ACK (`kTypeAck`) or NON (`kTypeNonConfirmable`) message depending on the
     * request @p aRxMsg type, containing the specified response code @p aCode and matching token, without any payload.
     *
     * @param[in]  aCode           The CoAP response Code.
     * @param[in]  aRxMsg          The received CoAP request message.
     *
     * @retval kErrorNone          Successfully enqueued the CoAP response message.
     * @retval kErrorNoBufs        Insufficient buffers available to send the CoAP response.
     * @retval kErrorInvalidArgs   The @p aRxMsg is not a request, or is neither confirmable nor non-confirmable.
     */
    Error SendResponse(Message::Code aCode, const Msg &aRxMsg);

    /**
     * Sends a CoAP ACK (`kTypeAck`) response without a payload and a given status code.
     *
     * This method strictly ensures the @p aRxMsg is a confirmable request message. It then sends a piggybacked
     * ACK (`kTypeAck`) response containing the provided CoAP @p aCode and matching token without a payload.
     *
     * @param[in]  aRxMsg          The received CoAP request message.
     * @param[in]  aCode           The CoAP Code of the ACK response.
     *
     * @retval kErrorNone          Successfully enqueued the CoAP response message.
     * @retval kErrorNoBufs        Insufficient buffers available to send the CoAP response.
     * @retval kErrorInvalidArgs   The @p aRxMsg is not a confirmable request.
     */
    Error SendAckResponse(const Msg &aRxMsg, Code aCode);

    /**
     * Sends a CoAP ACK response without a payload using `kCodeChanged` Code 2.04.
     *
     * This method strictly ensures the @p aRxMsg is a confirmable request message. It then sends a piggybacked
     * ACK (`kTypeAck`) response containing `kCodeChanged` and matching token without a payload.
     *
     * @param[in]  aRxMsg          The received CoAP request message.
     *
     * @retval kErrorNone          Successfully enqueued the CoAP response message.
     * @retval kErrorNoBufs        Insufficient buffers available to send the CoAP response.
     * @retval kErrorInvalidArgs   The @p aRxMsg is not a confirmable request.
     */
    Error SendAckResponse(const Msg &aRxMsg);

    /**
     * Sends a CoAP ACK response without a payload mapping an `Error` to a CoAP Code.
     *
     * This method strictly ensures the @p aRxMsg is a confirmable request message and the request was not sent to
     * a multicast address. It then sends a piggybacked ACK (`kTypeAck`) response containing a CoAP Code mapped
     * from @p aError and matching token without a payload.
     *
     * @param[in]  aRxMsg          The received CoAP request message.
     * @param[in]  aError          The error to map to a CoAP Code.
     *
     * @retval kErrorNone          Successfully enqueued the CoAP response message.
     * @retval kErrorNoBufs        Insufficient buffers available to send the CoAP response.
     * @retval kErrorInvalidArgs   The @p aRxMsg is not a confirmable request or was sent to a multicast address.
     */
    Error SendAckResponseIfUnicastRequest(const Msg &aRxMsg, Error aError);

    /**
     * Aborts CoAP transactions associated with given handler and context.
     *
     * The associated response handler will be called with kErrorAbort.
     *
     * @param[in]  aHandler  A function pointer that should be called when the transaction ends.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     * @retval kErrorNone      Successfully aborted CoAP transactions.
     * @retval kErrorNotFound  CoAP transaction associated with given handler was not found.
     */
    Error AbortTransaction(ResponseHandler aHandler, void *aContext);

    /**
     * Sets interceptor to be called before processing a CoAP packet.
     *
     * @param[in]   aInterceptor    A pointer to the interceptor.
     * @param[in]   aContext        A pointer to arbitrary context information.
     */
    void SetInterceptor(Interceptor aInterceptor, void *aContext) { mInterceptor.Set(aInterceptor, aContext); }

    /**
     * Retrieves aggregated information about the CoAP request and cached response message queues.
     *
     * Provides combined statistics, such as the total number of messages and data buffers currently used across all
     * queues associated with CoAP requests and cached responses within this CoAP instance.
     *
     * @param[out] aQueueInfo     A `MessageQueue::Info` to populate with info about the queues.
     */
    void GetRequestAndCachedResponsesQueueInfo(MessageQueue::Info &aQueueInfo) const;

    /**
     * Sends a CoAP message with custom transmission parameters using `ResponseHandlerSeparateParams` handle type.
     *
     * This version of `SendMessage()` is intended for use by the public OT CoAP APIs, `otCoap*` and should not be used
     * within the OpenThread core modules.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aTxParameters A pointer to `TxParameters`. If `nullptr`, default `TxParameters will be used.
     * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
     * @param[in]  aTransmitHook A pointer to a hook function for outgoing block-wise transfer.
     * @param[in]  aReceiveHook  A pointer to a hook function for incoming block-wise transfer.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     * @retval kErrorNone     Successfully sent CoAP message.
     * @retval kErrorNoBufs   Failed to allocate retransmission data.
     */
    Error SendMessageWithResponseHandlerSeparateParams(Message                      &aMessage,
                                                       const Ip6::MessageInfo       &aMessageInfo,
                                                       const TxParameters           *aTxParameters,
                                                       ResponseHandlerSeparateParams aHandler,
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
                                                       BlockwiseTransmitHook aTransmitHook,
                                                       BlockwiseReceiveHook  aReceiveHook,
#endif
                                                       void *aContext);

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    /**
     * Adds a block-wise resource to the CoAP server.
     *
     * @param[in]  aResource  A reference to the resource.
     */
    void AddBlockWiseResource(ResourceBlockWise &aResource);

    /**
     * Removes a block-wise resource from the CoAP server.
     *
     * @param[in]  aResource  A reference to the resource.
     */
    void RemoveBlockWiseResource(ResourceBlockWise &aResource);

    /**
     * Sends a header-only CoAP message to indicate not all blocks have been sent or
     * were sent out of order.
     *
     * @param[in]  aRxMsg          The received CoAP request message.
     *
     * @retval kErrorNone          Successfully enqueued the CoAP response message.
     * @retval kErrorNoBufs        Insufficient buffers available to send the CoAP response.
     */
    Error SendRequestEntityIncomplete(const Msg &aRxMsg) { return SendResponse(kCodeRequestIncomplete, aRxMsg); }
#endif // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

protected:
    /**
     * Defines function pointer to handle a CoAP resource.
     *
     * When processing a received request, this handler is called first with the URI path before checking the list of
     * added `Resource` entries to match against the URI path.
     *
     * @param[in] aCoapBase     A reference the CoAP agent.
     * @param[in] aUriPath      The URI Path string.
     * @param[in] aRxMsg        The received message
     *
     * @retval TRUE   Indicates that the URI path was known and the message was processed by the handler.
     * @retval FALSE  Indicates that URI path was not known and the message was not processed by the handler.
     */
    typedef bool (*ResourceHandler)(CoapBase &aCoapBase, const char *aUriPath, Msg &aRxMsg);

    /**
     * Represents a function reference used to pass a prepared CoAP message to the transport layer for transmission.
     *
     * @param[in]  aCoapBase     A reference to the CoAP agent.
     * @param[in]  aMessage      A reference to the message to transmit.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     * @retval kErrorNone    Successfully sent CoAP message.
     * @retval kErrorNoBufs  Failed to allocate retransmission data.
     */
    typedef Error (&Transmitter)(CoapBase &aCoapBase, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * Initializes the `CoapBase` object.
     *
     * @param[in]  aInstance       The OpenThread instance.
     * @param[in]  aTransmitter    A `Transmitter` function reference used to pass a CoAP message to the transport
     *                             layer for transmission.
     */
    CoapBase(Instance &aInstance, Transmitter aTransmitter);

    /**
     * Receives a CoAP message from the transport layer.
     *
     * @param[in]  aMessage      The received message.
     * @param[in]  aMessageInfo  The message info associated with @p aMessage.
     */
    void Receive(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * Sets the resource handler function.
     *
     * @param[in] aHandler   The resource handler function pointer.
     */
    void SetResourceHandler(ResourceHandler aHandler) { mResourceHandler = aHandler; }

private:
    static constexpr uint16_t kMaxBlockSize = OPENTHREAD_CONFIG_COAP_MAX_BLOCK_LENGTH;

    struct SendCallbacks
    {
        void Clear(void);
        bool HasResponseHandler(void) const;
        bool Matches(ResponseHandler aHandler, void *aContext) const;
        void InvokeResponseHandler(Msg *aMsg, Error aResult) const;
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        bool HasBlockwiseReceiveHook(void) const { return mBlockwiseReceiveHook != nullptr; }
        bool HasBlockwiseTransmitHook(void) const { return mBlockwiseTransmitHook != nullptr; }
#endif

        void                         *mContext;
        ResponseHandler               mResponseHandler;
        ResponseHandlerSeparateParams mResponseHandlerSeparateParams;
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        BlockwiseReceiveHook  mBlockwiseReceiveHook;
        BlockwiseTransmitHook mBlockwiseTransmitHook;
#endif
    };

    class PendingRequests;

    class Request
    {
        friend PendingRequests;

    public:
        void Clear(void) { mMessage = nullptr; }
        bool HasMessage(void) const { return (mMessage != nullptr); }
        void InitFrom(Message &aMessage) { mMessage = &aMessage, ReadMetadataFromMessage(); }
        bool IsAcknowledged(void) const { return mMetadata.mAcknowledged; }
        void MarkAsAcknowledged(void);
        bool IsConfirmable(void) const { return mMetadata.mConfirmable; }
        bool HasSamePeerAddrAndPort(const Ip6::MessageInfo &aMessageInfo) const;
        bool ShouldRetransmit(void) const;
        void UpdateRetxCounterAndTimeout(TimeMilli aNow);
        bool HasResponseHandler(void) const { return GetCallbacks().HasResponseHandler(); }

        const Message       &GetMessage(void) const { return *mMessage; }
        const Ip6::Address  &GetSourceAddress(void) const { return mMetadata.mSourceAddress; }
        const Ip6::Address  &GetDestinationAddress(void) const { return mMetadata.mDestinationAddress; }
        const SendCallbacks &GetCallbacks(void) const { return mMetadata.mCallbacks; }
        const TimeMilli     &GetTimerFireTime(void) const { return mMetadata.mTimerFireTime; }

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        bool IsObserve(void) const { return mMetadata.mObserve; }
        bool IsRequest(void) const { return mMetadata.mIsRequest; }
        bool IsObserveSubscription(void) const;
#endif
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        bool HasBlockwiseReceiveHook(void) const { return GetCallbacks().HasBlockwiseReceiveHook(); }
        bool HasBlockwiseTransmitHook(void) const { return GetCallbacks().HasBlockwiseTransmitHook(); }
#endif
        void  ReadMetadataFromMessage(void) { mMetadata.ReadFrom(*mMessage); }
        void  WriteMetadataInMessage(void) { mMetadata.UpdateIn(*mMessage); }
        void  RemoveMetadataFromMessage(void) { mMetadata.RemoveFrom(*mMessage); }
        Error AppendMetadataToMessage(void) { return mMetadata.AppendTo(*mMessage); }

    private:
        struct Metadata : public Message::FooterData<Metadata>
        {
            void Init(const Msg &aTxMsg, const TxParameters &aTxParams, const SendCallbacks &aCallbacks);
            void CopyInfoTo(Ip6::MessageInfo &aMessageInfo) const;

            Ip6::Address  mSourceAddress;
            Ip6::Address  mDestinationAddress;
            uint16_t      mDestinationPort;
            SendCallbacks mCallbacks;
            TimeMilli     mTimerFireTime;
            uint32_t      mRetxTimeout;
            uint8_t       mRetxRemaining;
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
            uint8_t mHopLimit;
#endif
            bool mAcknowledged : 1;
            bool mConfirmable : 1;
            bool mMulticastLoop : 1;
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
            bool mIsHostInterface : 1;
#endif
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
            bool mObserve : 1;
            bool mIsRequest : 1;
#endif
        };

        Message *mMessage;
        Metadata mMetadata;
    };

    class PendingRequests
    {
        struct Iterator;

    public:
        explicit PendingRequests(Instance &aInstance, CoapBase &aCoapBase);

        Error Add(const Msg &aTxMsg, const TxParameters &aTxParams, const SendCallbacks &aCallbacks, Request &aRequest);
        void  Remove(Request &aRequest);
        Error FindRelatedRequest(const Msg &aMsg, Request &aRequest);
        void  FinalizeRequest(Request &aRequest, Error aResult);
        void  FinalizeRequest(Request &aRequest, Error aResult, Msg *aResponse);
        void  AbortAllRequests(void);
        void  AbortRequestsMatching(const Ip6::Address &aAddress);
        Error AbortRequestsMatching(ResponseHandler aHandler, void *aContext);
        void  DispatchResponse(Request &aRequest, Error aResult, Msg *aResponse);
        void  DispatchResponse(Request &aRequest, Error aResult);
        Error GetDispatchingRequest(OwnedPtr<Message> &aMessage) const;
        void  GetInfo(MessageQueue::Info &aInfo) const { mRequestMessages.GetInfo(aInfo); }

    private:
        class Matcher
        {
        public:
            using Handler = ResponseHandler;

            Matcher(void) { mMode = kAny; }
            Matcher(const Ip6::Address &aSourceAddress) { mMode = kAddress, mAddress = &aSourceAddress; }
            Matcher(Handler aHandler, void *aContext) { mMode = kHandler, mHandler = aHandler, mContext = aContext; }

            bool Matches(const Request &aRequest) const;

        private:
            enum MatchMode
            {
                kAny,
                kAddress,
                kHandler,
            };

            MatchMode           mMode;
            const Ip6::Address *mAddress;
            Handler             mHandler;
            void               *mContext;
        };

        Error       AbortAllMatching(const Matcher &aMatcher);
        void        FinalizeRemovedRequestsIn(MessageQueue &aQueue, Error aResult);
        void        RetransmitRequest(const Request &aRequest);
        static void HandleTimer(Timer &aTimer);
        void        HandleTimer(void);
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        Error ProcessObserveSend(const Msg &aTxMsg, Request &aRequest);
#endif

        CoapBase         &mCoapBase;
        MessageQueue      mRequestMessages;
        const Request    *mDispatchingRequest;
        TimerMilliContext mTimer;
    };

    class ResponseCache
    {
    public:
        explicit ResponseCache(Instance &aInstance);

        void  Add(const Msg &aTxMsg, uint32_t aExchangeLifetime);
        void  RemoveAll(void);
        Error SendCachedResponse(const Msg &aRxMsg, CoapBase &aCoapBase);
        void  GetInfo(MessageQueue::Info &aInfo) const { mResponses.GetInfo(aInfo); }

    private:
        static constexpr uint16_t kMaxCacheSize = OPENTHREAD_CONFIG_COAP_SERVER_MAX_CACHED_RESPONSES;

        static_assert(kMaxCacheSize != 0, "kMaxCacheSize MUST be non-zero");

        struct ResponseMetadata : public Message::FooterData<ResponseMetadata>
        {
            TimeMilli        mExpireTime;
            Ip6::MessageInfo mMessageInfo;
        };

        const Message *FindMatching(const Msg &aRxMsg) const;
        void           MaintainCacheSize(void);
        static void    HandleTimer(Timer &aTimer);
        void           HandleTimer(void);

        MessageQueue      mResponses;
        TimerMilliContext mTimer;
    };

    Message *InitMessage(Message *aMessage, Type aType, Uri aUri);
    Message *InitResponse(Message *aMessage, const Message &aRequest);
    bool     InvokeResponseFallback(Msg &aRxMsg) const;
    void     ProcessReceivedRequest(Msg &aRxMsg);
    void     ProcessReceivedResponse(Msg &aRxMsg);
    Error    SendMessage(Message                &aMessage,
                         const Ip6::MessageInfo &aMessageInfo,
                         const TxParameters     *aTxParameters,
                         const SendCallbacks    &aCallbacks);
    Error    Transmit(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

    Error ProcessBlockwiseSend(Msg &aMsg, const SendCallbacks &aCallbacks);
    Error ProcessBlockwiseResponse(Msg &aRxMsg, Request &aRequest);
    Error ProcessBlockwiseRequest(Msg &aRxMsg, const Message::UriPathStringBuffer &aUriPath, bool &aDidHandle);
    void  FreeLastBlockResponse(void);
    Error CacheLastBlockResponse(Message *aResponse);
    Error PrepareNextBlockRequest(uint16_t         aBlockOptionNumber,
                                  Request         &aRequestOld,
                                  Message         &aRequest,
                                  const BlockInfo &aBlockInfo);
    Error ProcessBlock1Request(Msg &aRxMsg, const ResourceBlockWise &aResource, uint32_t aTotalLength);
    Error ProcessBlock2Request(Msg &aRxMsg, const ResourceBlockWise &aResource);
    Error SendNextBlock1Request(Request &aRequest, Msg &aRxMsg);
    Error SendNextBlock2Request(Request &aRequest, Msg &aRxMsg, uint32_t aTotalLength, bool aBeginBlock1Transfer);

    static Error DetermineBlockSzxFromSize(uint16_t aSize, BlockSzx &aBlockSzx);

#endif // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

    PendingRequests            mPendingRequests;
    ResponseCache              mResponseCache;
    LinkedList<Resource>       mResources;
    Callback<Interceptor>      mInterceptor;
    Callback<RequestHandler>   mDefaultHandler;
    Callback<ResponseFallback> mResponseFallback;
    ResourceHandler            mResourceHandler;
    Transmitter                mTransmitter;
    uint16_t                   mMessageId;
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    LinkedList<ResourceBlockWise> mBlockWiseResources;
    Message                      *mLastResponse;
#endif
};

/**
 * Implements the CoAP client and server.
 */
class Coap : public CoapBase
{
public:
    /**
     * Initializes the object.
     *
     * @param[in] aInstance      A reference to the OpenThread instance.
     */
    explicit Coap(Instance &aInstance);

    /**
     * Starts the CoAP service.
     *
     * @param[in]  aPort             The local UDP port to bind to.
     * @param[in]  aNetifIdentifier  The network interface identifier to bind.
     *
     * @retval kErrorNone    Successfully started the CoAP service.
     * @retval kErrorFailed  Failed to start CoAP agent.
     */
    Error Start(uint16_t aPort, Ip6::NetifIdentifier aNetifIdentifier = Ip6::kNetifUnspecified);

    /**
     * Stops the CoAP service.
     *
     * @retval kErrorNone    Successfully stopped the CoAP service.
     * @retval kErrorFailed  Failed to stop CoAP agent.
     */
    Error Stop(void);

protected:
    void HandleUdpReceive(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    using CoapSocket = Ip6::Udp::SocketIn<Coap, &Coap::HandleUdpReceive>;

    CoapSocket mSocket;

private:
    static Error Transmit(CoapBase &aCoapBase, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    Error        Transmit(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
};

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
/**
 * Represents an Application CoAP.
 */
class ApplicationCoap : public Coap
{
public:
    /**
     * Initializes the object.
     *
     * @param[in] aInstance      A reference to the OpenThread instance.
     */
    explicit ApplicationCoap(Instance &aInstance)
        : Coap(aInstance)
    {
    }
};
#endif

} // namespace Coap

DefineCoreType(otCoapTxParameters, Coap::TxParameters);
DefineCoreType(otCoapResource, Coap::Resource);
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
DefineCoreType(otCoapBlockwiseResource, Coap::ResourceBlockWise);
#endif

} // namespace ot

#endif // OT_CORE_COAP_COAP_HPP_
