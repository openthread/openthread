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
 * @brief
 *  This file defines the top-level functions for the OpenThread CoAP implementation.
 */

#ifndef OPENTHREAD_COAP_H_
#define OPENTHREAD_COAP_H_

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <stdint.h>

#include <openthread/message.h>
#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-coap
 *
 * @brief
 *   This module includes functions that control CoAP communication.
 *
 * @{
 *
 */

#define OT_DEFAULT_COAP_PORT  5683  ///< Default CoAP port, as specified in RFC 7252

/**
 * CoAP Type values.
 *
 */
typedef enum otCoapType
{
    kCoapTypeConfirmable    = 0x00,  ///< Confirmable
    kCoapTypeNonConfirmable = 0x10,  ///< Non-confirmable
    kCoapTypeAcknowledgment = 0x20,  ///< Acknowledgment
    kCoapTypeReset          = 0x30,  ///< Reset
} otCoapType;

/**
 * Helper macro to define CoAP Code values.
 *
 */
#define COAP_CODE(c, d) ((((c) & 0x7) << 5) | ((d) & 0x1f))

/**
 * CoAP Code values.
 *
 */
typedef enum otCoapCode
{
    kCoapCodeEmpty                  = COAP_CODE(0, 0),  ///< Empty message code
    kCoapRequestGet                 = COAP_CODE(0, 1),  ///< Get
    kCoapRequestPost                = COAP_CODE(0, 2),  ///< Post
    kCoapRequestPut                 = COAP_CODE(0, 3),  ///< Put
    kCoapRequestDelete              = COAP_CODE(0, 4),  ///< Delete

    kCoapResponseCodeMin            = COAP_CODE(2, 0),  ///< 2.00
    kCoapResponseCreated            = COAP_CODE(2, 1),  ///< Created
    kCoapResponseDeleted            = COAP_CODE(2, 2),  ///< Deleted
    kCoapResponseValid              = COAP_CODE(2, 3),  ///< Valid
    kCoapResponseChanged            = COAP_CODE(2, 4),  ///< Changed
    kCoapResponseContent            = COAP_CODE(2, 5),  ///< Content

    kCoapResponseBadRequest         = COAP_CODE(4, 0),  ///< Bad Request
    kCoapResponseUnauthorized       = COAP_CODE(4, 1),  ///< Unauthorized
    kCoapResponseBadOption          = COAP_CODE(4, 2),  ///< Bad Option
    kCoapResponseForbidden          = COAP_CODE(4, 3),  ///< Forbidden
    kCoapResponseNotFound           = COAP_CODE(4, 4),  ///< Not Found
    kCoapResponseMethodNotAllowed   = COAP_CODE(4, 5),  ///< Method Not Allowed
    kCoapResponseNotAcceptable      = COAP_CODE(4, 6),  ///< Not Acceptable
    kCoapResponsePreconditionFailed = COAP_CODE(4, 12), ///< Precondition Failed
    kCoapResponseRequestTooLarge    = COAP_CODE(4, 13), ///< Request Entity Too Large
    kCoapResponseUnsupportedFormat  = COAP_CODE(4, 15), ///< Unsupported Content-Format

    kCoapResponseInternalError      = COAP_CODE(5, 0),  ///< Internal Server Error
    kCoapResponseNotImplemented     = COAP_CODE(5, 1),  ///< Not Implemented
    kCoapResponseBadGateway         = COAP_CODE(5, 2),  ///< Bad Gateway
    kCoapResponseServiceUnavailable = COAP_CODE(5, 3),  ///< Service Unavailable
    kCoapResponseGatewayTimeout     = COAP_CODE(5, 4),  ///< Gateway Timeout
    kCoapResponseProxyNotSupported  = COAP_CODE(5, 5),  ///< Proxying Not Supported
} otCoapCode;

/**
 * CoAP Option Numbers
 */
typedef enum otCoapOptionType
{
    kCoapOptionIfMatch       = 1,    ///< If-Match
    kCoapOptionUriHost       = 3,    ///< Uri-Host
    kCoapOptionETag          = 4,    ///< ETag
    kCoapOptionIfNoneMatch   = 5,    ///< If-None-Match
    kCoapOptionObserve       = 6,    ///< Observe
    kCoapOptionUriPort       = 7,    ///< Uri-Port
    kCoapOptionLocationPath  = 8,    ///< Location-Path
    kCoapOptionUriPath       = 11,   ///< Uri-Path
    kCoapOptionContentFormat = 12,   ///< Content-Format
    kCoapOptionMaxAge        = 14,   ///< Max-Age
    kCoapOptionUriQuery      = 15,   ///< Uri-Query
    kCoapOptionAccept        = 17,   ///< Accept
    kCoapOptionLocationQuery = 20,   ///< Location-Query
    kCoapOptionProxyUri      = 35,   ///< Proxy-Uri
    kCoapOptionProxyScheme   = 39,   ///< Proxy-Scheme
    kCoapOptionSize1         = 60,   ///< Size1
} otCoapOptionType;

/**
 * This structure represents a CoAP option.
 *
 */
typedef struct otCoapOption
{
    uint16_t       mNumber;  ///< Option Number
    uint16_t       mLength;  ///< Option Length
    const uint8_t *mValue;   ///< A pointer to the Option Value
} otCoapOption;

#define OT_COAP_HEADER_MAX_LENGTH       128  ///< Max CoAP header length (bytes)

/**
 * This structure represents a CoAP header.
 *
 */
typedef struct otCoapHeader
{
    union
    {
        struct
        {
            uint8_t mVersionTypeToken;              ///< The CoAP Version, Type, and Token Length
            uint8_t mCode;                          ///< The CoAP Code
            uint16_t mMessageId;                    ///< The CoAP Message ID
        } mFields;                                  ///< Structure representing a CoAP base header
        uint8_t mBytes[OT_COAP_HEADER_MAX_LENGTH];  ///< The raw byte encoding for the CoAP header
    } mHeader;                                      ///< The CoAP header encoding
    uint8_t mHeaderLength;                          ///< The CoAP header length (bytes)
    uint16_t mOptionLast;                           ///< The last CoAP Option Number value
    uint16_t mFirstOptionOffset;                    ///< The byte offset for the first CoAP Option
    uint16_t mNextOptionOffset;                     ///< The byte offset for the next CoAP Option
    otCoapOption mOption;                           ///< A structure representing the current CoAP Option.
} otCoapHeader;

/**
 * This function pointer is called when a CoAP response is received or on the request timeout.
 *
 * @param[in]  aContext      A pointer to application-specific context.
 * @param[in]  aHeader       A pointer to the received CoAP header. NULL if no response was received.
 * @param[in]  aMessage      A pointer to the message buffer containing the response. NULL if no response was received.
 * @param[in]  aMessageInfo  A pointer to the message info for @p aMessage. NULL if no response was received.
 * @param[in]  aResult       A result of the CoAP transaction.
 *
 * @retval  kThreadError_None             A response was received successfully.
 * @retval  kThreadError_Abort            A CoAP transaction was reseted by peer.
 * @retval  kThreadError_ResponseTimeout  No response or acknowledgment received during timeout period.
 *
 */
typedef void (*otCoapResponseHandler)(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                      const otMessageInfo *aMessageInfo, ThreadError aResult);

/**
 * This function pointer is called when a CoAP request with a given Uri-Path is received.
 *
 * @param[in]  aContext      A pointer to arbitrary context information.
 * @param[in]  aHeader       A pointer to the CoAP header.
 * @param[in]  aMessage      A pointer to the message.
 * @param[in]  aMessageInfo  A pointer to the message info for @p aMessage.
 *
 */
typedef void (*otCoapRequestHandler)(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                     const otMessageInfo *aMessageInfo);

/**
 * This structure represents a CoAP resource.
 *
 */
typedef struct otCoapResource
{
    const char *mUriPath;           ///< The URI Path string
    otCoapRequestHandler mHandler;  ///< The callback for handling a received request
    void *mContext;                 ///< Application-specific context
    struct otCoapResource *mNext;   ///< The next CoAP resource in the list
} otCoapResource;

#if OPENTHREAD_ENABLE_APPLICATION_COAP
/**
 * This function initializes the CoAP header.
 *
 * @param[inout] aHeader  A pointer to the CoAP header to initialize.
 * @param[in]    aType    CoAP message type.
 * @param[in]    aCode    CoAP message code.
 *
 */
void otCoapHeaderInit(otCoapHeader *aHeader, otCoapType aType, otCoapCode aCode);

/**
 * This function sets the Token value and length in a header.
 *
 * @param[inout]  aHeader       A pointer to the CoAP header.
 * @param[in]     aToken        A pointer to the Token value.
 * @param[in]     aTokenLength  The Length of @p aToken.
 *
 */
void otCoapHeaderSetToken(otCoapHeader *aHeader, const uint8_t *aToken, uint8_t aTokenLength);

/**
 * This function sets the Token length and randomizes its value.
 *
 * @param[inout]  aHeader       A pointer to the CoAP header.
 * @param[in]     aTokenLength  The Length of a Token to set.
 *
 */
void otCoapHeaderGenerateToken(otCoapHeader *aHeader, uint8_t aTokenLength);

/**
 * This function appends a CoAP option in a header.
 *
 * @param[inout]  aHeader  A pointer to the CoAP header.
 * @param[in]     aOption  A pointer to the CoAP option.
 *
 * @retval kThreadError_None         Successfully appended the option.
 * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
 * @retval kThreadError_NoBufs       The option length exceeds the buffer size.
 *
 */
ThreadError otCoapHeaderAppendOption(otCoapHeader *aHeader, const otCoapOption *aOption);

/**
 * This function appends an unsigned integer CoAP option as specified in
 * https://tools.ietf.org/html/rfc7252#section-3.2
 *
 * @param[inout]  aHeader  A pointer to the CoAP header.
 * @param[in]     aNumber  The CoAP Option number.
 * @param[in]     aValue   The CoAP Option unsigned integer value.
 *
 * @retval kThreadError_None         Successfully appended the option.
 * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
 * @retval kThreadError_NoBufs       The option length exceeds the buffer size.
 *
 */
ThreadError otCoapHeaderAppendUintOption(otCoapHeader *aHeader, uint16_t aNumber, uint32_t aValue);

/**
 * This function appends an Observe option.
 *
 * @param[inout]  aHeader   A pointer to the CoAP header.
 * @param[in]     aObserve  Observe field value.
 *
 * @retval kThreadError_None         Successfully appended the option.
 * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
 * @retval kThreadError_NoBufs       The option length exceeds the buffer size.
 */
ThreadError otCoapHeaderAppendObserveOption(otCoapHeader *aHeader, uint32_t aObserve);

/**
 * This function appends an Uri-Path option.
 *
 * @param[inout]  aHeader   A pointer to the CoAP header.
 * @param[in]     aUriPath  A pointer to a NULL-terminated string.
 *
 * @retval kThreadError_None         Successfully appended the option.
 * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
 * @retval kThreadError_NoBufs       The option length exceeds the buffer size.
 *
 */
ThreadError otCoapHeaderAppendUriPathOptions(otCoapHeader *aHeader, const char *aUriPath);

/**
 * This function appends a Max-Age option.
 *
 * @param[inout]  aHeader   A pointer to the CoAP header.
 * @param[in]     aMaxAge   The Max-Age value.
 *
 * @retval kThreadError_None         Successfully appended the option.
 * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
 * @retval kThreadError_NoBufs       The option length exceeds the buffer size.
 */
ThreadError otCoapHeaderAppendMaxAgeOption(otCoapHeader *aHeader, uint32_t aMaxAge);

/**
 * This function appends a single Uri-Query option.
 *
 * @param[inout]  aHeader   A pointer to the CoAP header.
 * @param[in]     aUriQuery A pointer to NULL-terminated string, which should contain a single key=value pair.
 *
 * @retval kThreadError_None         Successfully appended the option.
 * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
 * @retval kThreadError_NoBufs       The option length exceeds the buffer size.
 */
ThreadError otCoapHeaderAppendUriQueryOption(otCoapHeader *aHeader, const char *aUriQuery);

/**
 * This function adds Payload Marker indicating beginning of the payload to the CoAP header.
 *
 * @param[inout]  aHeader  A pointer to the CoAP header.
 *
 * @retval kThreadError_None    Payload Marker successfully added.
 * @retval kThreadError_NoBufs  Header Payload Marker exceeds the buffer size.
 *
 */
void otCoapHeaderSetPayloadMarker(otCoapHeader *aHeader);

/**
 * This function sets the Message ID value.
 *
 * @param[in]  aHeader     A pointer to the CoAP header.
 * @param[in]  aMessageId  The Message ID value.
 *
 */
void otCoapHeaderSetMessageId(otCoapHeader *aHeader, uint16_t aMessageId);

/**
 * This function returns the Type value.
 *
 * @param[in]  aHeader  A pointer to the CoAP header.
 *
 * @returns The Type value.
 *
 */
otCoapType otCoapHeaderGetType(const otCoapHeader *aHeader);

/**
 * This function returns the Code value.
 *
 * @param[in]  aHeader  A pointer to the CoAP header.
 *
 * @returns The Code value.
 *
 */
otCoapCode otCoapHeaderGetCode(const otCoapHeader *aHeader);

/**
* This function returns the Message ID value.
*
* @param[in]  aHeader  A pointer to the CoAP header.
*
* @returns The Message ID value.
*
*/
uint16_t otCoapHeaderGetMessageId(const otCoapHeader *aHeader);

/**
 * This function returns the Token length.
 *
 * @param[in]  aHeader  A pointer to the CoAP header.
 *
 * @returns The Token length.
 *
 */
uint8_t otCoapHeaderGetTokenLength(const otCoapHeader *aHeader);

/**
 * This function returns a pointer to the Token value.
 *
 * @param[in]  aHeader  A pointer to the CoAP header.
 *
 * @returns A pointer to the Token value.
 *
 */
const uint8_t *otCoapHeaderGetToken(const otCoapHeader *aHeader);

/**
 * This function returns a pointer to the first option.
 *
 * @param[in]  aHeader  A pointer to the CoAP header.
 *
 * @returns A pointer to the first option. If no option is present NULL pointer is returned.
 *
 */
const otCoapOption *otCoapHeaderGetFirstOption(otCoapHeader *aHeader);

/**
 * This function returns a pointer to the next option.
 *
 * @param[in]  aHeader  A pointer to the CoAP header.
 *
 * @returns A pointer to the next option. If no more options are present NULL pointer is returned.
 *
 */
const otCoapOption *otCoapHeaderGetNextOption(otCoapHeader *aHeader);

/**
 * This function creates a new message with a CoAP header.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aHeader  A pointer to a CoAP header that is used to create the message.
 *
 * @returns A pointer to the message or NULL if failed to allocate message.
 *
 */
otMessage *otCoapNewMessage(otInstance *aInstance, const otCoapHeader *aHeader);

/**
 * This function sends a CoAP request.
 *
 * If a response for a request is expected, respective function and context information should be provided.
 * If no response is expected, these arguments should be NULL pointers.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aMessage      A pointer to the message to send.
 * @param[in]  aMessageInfo  A pointer to the message info associated with @p aMessage.
 * @param[in]  aHandler      A function pointer that shall be called on response reception or timeout.
 * @param[in]  aContext      A pointer to arbitrary context information. May be NULL if not used.
 *
 * @retval kThreadError_None   Successfully sent CoAP message.
 * @retval kThreadError_NoBufs Failed to allocate retransmission data.
 *
 */
ThreadError otCoapSendRequest(otInstance *aInstance, otMessage *aMessage, const otMessageInfo *aMessageInfo,
                              otCoapResponseHandler aHandler, void *aContext);

/**
 * This function starts the CoAP server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval kThreadError_None  Successfully started the CoAP server.
 *
 */
ThreadError otCoapServerStart(otInstance *aInstance);

/**
 * This function stops the CoAP server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval kThreadError_None  Successfully stopped the CoAP server.
 *
 */
ThreadError otCoapServerStop(otInstance *aInstance);

/**
 * This function sets CoAP server's port number.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aPort  A port number to set.
 *
 * @retval kThreadError_None  Binding with a port succeeded.
 *
 */
ThreadError otCoapServerSetPort(otInstance *aInstance, uint16_t aPort);

/**
 * This function adds a resource to the CoAP server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aResource  A pointer to the resource.
 *
 * @retval kThreadError_None     Successfully added @p aResource.
 * @retval kThreadError_Already  The @p aResource was already added.
 *
 */
ThreadError otCoapServerAddResource(otInstance *aInstance, otCoapResource *aResource);

/**
 * This function removes a resource from the CoAP server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aResource  A pointer to the resource.
 *
 */
void otCoapServerRemoveResource(otInstance *aInstance, otCoapResource *aResource);

/**
 * This function sends a CoAP response from the server.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aMessage      A pointer to the CoAP response to send.
 * @param[in]  aMessageInfo  A pointer to the message info associated with @p aMessage.
 *
 * @retval kThreadError_None    Successfully enqueued the CoAP response message.
 * @retval kThreadError_NoBufs  Insufficient buffers available to send the CoAP response.
 *
 */
ThreadError otCoapSendResponse(otInstance *aInstance, otMessage *aMessage, const otMessageInfo *aMessageInfo);
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif /* OPENTHREAD_COAP_H_ */
