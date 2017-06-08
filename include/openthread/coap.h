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

#include <openthread/config.h>

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
    OT_COAP_TYPE_CONFIRMABLE     = 0x00,  ///< Confirmable
    OT_COAP_TYPE_NON_CONFIRMABLE = 0x10,  ///< Non-confirmable
    OT_COAP_TYPE_ACKNOWLEDGMENT  = 0x20,  ///< Acknowledgment
    OT_COAP_TYPE_RESET           = 0x30,  ///< Reset
} otCoapType;

/**
 * Helper macro to define CoAP Code values.
 *
 */
#define OT_COAP_CODE(c, d) ((((c) & 0x7) << 5) | ((d) & 0x1f))

/**
 * CoAP Code values.
 *
 */
typedef enum otCoapCode
{
    OT_COAP_CODE_EMPTY               = OT_COAP_CODE(0, 0),  ///< Empty message code
    OT_COAP_CODE_GET                 = OT_COAP_CODE(0, 1),  ///< Get
    OT_COAP_CODE_POST                = OT_COAP_CODE(0, 2),  ///< Post
    OT_COAP_CODE_PUT                 = OT_COAP_CODE(0, 3),  ///< Put
    OT_COAP_CODE_DELETE              = OT_COAP_CODE(0, 4),  ///< Delete

    OT_COAP_CODE_RESPONSE_MIN        = OT_COAP_CODE(2, 0),  ///< 2.00
    OT_COAP_CODE_CREATED             = OT_COAP_CODE(2, 1),  ///< Created
    OT_COAP_CODE_DELETED             = OT_COAP_CODE(2, 2),  ///< Deleted
    OT_COAP_CODE_VALID               = OT_COAP_CODE(2, 3),  ///< Valid
    OT_COAP_CODE_CHANGED             = OT_COAP_CODE(2, 4),  ///< Changed
    OT_COAP_CODE_CONTENT             = OT_COAP_CODE(2, 5),  ///< Content

    OT_COAP_CODE_BAD_REQUEST         = OT_COAP_CODE(4, 0),  ///< Bad Request
    OT_COAP_CODE_UNAUTHORIZED        = OT_COAP_CODE(4, 1),  ///< Unauthorized
    OT_COAP_CODE_BAD_OPTION          = OT_COAP_CODE(4, 2),  ///< Bad Option
    OT_COAP_CODE_FORBIDDEN           = OT_COAP_CODE(4, 3),  ///< Forbidden
    OT_COAP_CODE_NOT_FOUND           = OT_COAP_CODE(4, 4),  ///< Not Found
    OT_COAP_CODE_METHOD_NOT_ALLOWED  = OT_COAP_CODE(4, 5),  ///< Method Not Allowed
    OT_COAP_CODE_NOT_ACCEPTABLE      = OT_COAP_CODE(4, 6),  ///< Not Acceptable
    OT_COAP_CODE_PRECONDITION_FAILED = OT_COAP_CODE(4, 12), ///< Precondition Failed
    OT_COAP_CODE_REQUEST_TOO_LARGE   = OT_COAP_CODE(4, 13), ///< Request Entity Too Large
    OT_COAP_CODE_UNSUPPORTED_FORMAT  = OT_COAP_CODE(4, 15), ///< Unsupported Content-Format

    OT_COAP_CODE_INTERNAL_ERROR      = OT_COAP_CODE(5, 0),  ///< Internal Server Error
    OT_COAP_CODE_NOT_IMPLEMENTED     = OT_COAP_CODE(5, 1),  ///< Not Implemented
    OT_COAP_CODE_BAD_GATEWAY         = OT_COAP_CODE(5, 2),  ///< Bad Gateway
    OT_COAP_CODE_SERVICE_UNAVAILABLE = OT_COAP_CODE(5, 3),  ///< Service Unavailable
    OT_COAP_CODE_GATEWAY_TIMEOUT     = OT_COAP_CODE(5, 4),  ///< Gateway Timeout
    OT_COAP_CODE_PROXY_NOT_SUPPORTED = OT_COAP_CODE(5, 5),  ///< Proxying Not Supported
} otCoapCode;

/**
 * CoAP Option Numbers
 */
typedef enum otCoapOptionType
{
    OT_COAP_OPTION_IF_MATCH       = 1,    ///< If-Match
    OT_COAP_OPTION_URI_HOST       = 3,    ///< Uri-Host
    OT_COAP_OPTION_E_TAG          = 4,    ///< ETag
    OT_COAP_OPTION_IF_NONE_MATCH  = 5,    ///< If-None-Match
    OT_COAP_OPTION_OBSERVE        = 6,    ///< Observe
    OT_COAP_OPTION_URI_PORT       = 7,    ///< Uri-Port
    OT_COAP_OPTION_LOCATION_PATH  = 8,    ///< Location-Path
    OT_COAP_OPTION_URI_PATH       = 11,   ///< Uri-Path
    OT_COAP_OPTION_CONTENT_FORMAT = 12,   ///< Content-Format
    OT_COAP_OPTION_MAX_AGE        = 14,   ///< Max-Age
    OT_COAP_OPTION_URI_QUERY      = 15,   ///< Uri-Query
    OT_COAP_OPTION_ACCEPT         = 17,   ///< Accept
    OT_COAP_OPTION_LOCATION_QUERY = 20,   ///< Location-Query
    OT_COAP_OPTION_PROXY_URI      = 35,   ///< Proxy-Uri
    OT_COAP_OPTION_PROXY_SCHEME   = 39,   ///< Proxy-Scheme
    OT_COAP_OPTION_SIZE1          = 60,   ///< Size1
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

/**
 * CoAP Content Format codes.  The full list is documented at
 * https://tools.ietf.org/html/rfc7252#page-92
 */
typedef enum otCoapOptionContentFormat
{
    OT_COAP_OPTION_CONTENT_FORMAT_TEXT_PLAIN    = 0,    ///< text/plain
    OT_COAP_OPTION_CONTENT_FORMAT_LINK_FORMAT   = 40,   ///< application/link-format
    OT_COAP_OPTION_CONTENT_FORMAT_XML           = 41,   ///< application/xml
    OT_COAP_OPTION_CONTENT_FORMAT_OCTET_STREAM  = 42,   ///< application/octet-stream
    OT_COAP_OPTION_CONTENT_FORMAT_EXI           = 47,   ///< application/exi
    OT_COAP_OPTION_CONTENT_FORMAT_JSON          = 50,   ///< application/json
} otCoapOptionContentFormat;

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
 * @retval  OT_ERROR_NONE              A response was received successfully.
 * @retval  OT_ERROR_ABORT             A CoAP transaction was reseted by peer.
 * @retval  OT_ERROR_RESPONSE_TIMEOUT  No response or acknowledgment received during timeout period.
 *
 */
typedef void (*otCoapResponseHandler)(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                      const otMessageInfo *aMessageInfo, otError aResult);

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
 * This function appends the Content Format CoAP option as specified in
 * https://tools.ietf.org/html/rfc7252#page-92.  This *must* be called before
 * setting otCoapHeaderSetPayloadMarker if a payload is to be included in the
 * message.
 *
 * The function is a convenience wrapper around otCoapHeaderAppendUintOption,
 * and if the desired format type code isn't listed in otCoapOptionContentFormat,
 * this base function should be used instead.
 *
 * @param[inout]  aHeader           A pointer to the CoAP header.
 * @param[in]     aContentFormat    One of the content formats listed in
 *                                  otCoapOptionContentFormat above.
 * @retval OT_ERROR_NONE          Successfully appended the option.
 * @retval OT_ERROR_INVALID_ARGS  The option type is not equal or greater than the last option type.
 * @retval OT_ERROR_NO_BUFS       The option length exceeds the buffer size.
 */
otError otCoapHeaderAppendContentFormatOption(otCoapHeader *aHeader, otCoapOptionContentFormat aContentFormat);

/**
 * This function appends a CoAP option in a header.
 *
 * @param[inout]  aHeader  A pointer to the CoAP header.
 * @param[in]     aOption  A pointer to the CoAP option.
 *
 * @retval OT_ERROR_NONE          Successfully appended the option.
 * @retval OT_ERROR_INVALID_ARGS  The option type is not equal or greater than the last option type.
 * @retval OT_ERROR_NO_BUFS       The option length exceeds the buffer size.
 *
 */
otError otCoapHeaderAppendOption(otCoapHeader *aHeader, const otCoapOption *aOption);

/**
 * This function appends an unsigned integer CoAP option as specified in
 * https://tools.ietf.org/html/rfc7252#section-3.2
 *
 * @param[inout]  aHeader  A pointer to the CoAP header.
 * @param[in]     aNumber  The CoAP Option number.
 * @param[in]     aValue   The CoAP Option unsigned integer value.
 *
 * @retval OT_ERROR_NONE          Successfully appended the option.
 * @retval OT_ERROR_INVALID_ARGS  The option type is not equal or greater than the last option type.
 * @retval OT_ERROR_NO_BUFS       The option length exceeds the buffer size.
 *
 */
otError otCoapHeaderAppendUintOption(otCoapHeader *aHeader, uint16_t aNumber, uint32_t aValue);

/**
 * This function appends an Observe option.
 *
 * @param[inout]  aHeader   A pointer to the CoAP header.
 * @param[in]     aObserve  Observe field value.
 *
 * @retval OT_ERROR_NONE          Successfully appended the option.
 * @retval OT_ERROR_INVALID_ARGS  The option type is not equal or greater than the last option type.
 * @retval OT_ERROR_NO_BUFS       The option length exceeds the buffer size.
 */
otError otCoapHeaderAppendObserveOption(otCoapHeader *aHeader, uint32_t aObserve);

/**
 * This function appends an Uri-Path option.
 *
 * @param[inout]  aHeader   A pointer to the CoAP header.
 * @param[in]     aUriPath  A pointer to a NULL-terminated string.
 *
 * @retval OT_ERROR_NONE          Successfully appended the option.
 * @retval OT_ERROR_INVALID_ARGS  The option type is not equal or greater than the last option type.
 * @retval OT_ERROR_NO_BUFS       The option length exceeds the buffer size.
 *
 */
otError otCoapHeaderAppendUriPathOptions(otCoapHeader *aHeader, const char *aUriPath);

/**
 * This function appends a Max-Age option.
 *
 * @param[inout]  aHeader   A pointer to the CoAP header.
 * @param[in]     aMaxAge   The Max-Age value.
 *
 * @retval OT_ERROR_NONE          Successfully appended the option.
 * @retval OT_ERROR_INVALID_ARGS  The option type is not equal or greater than the last option type.
 * @retval OT_ERROR_NO_BUFS       The option length exceeds the buffer size.
 */
otError otCoapHeaderAppendMaxAgeOption(otCoapHeader *aHeader, uint32_t aMaxAge);

/**
 * This function appends a single Uri-Query option.
 *
 * @param[inout]  aHeader   A pointer to the CoAP header.
 * @param[in]     aUriQuery A pointer to NULL-terminated string, which should contain a single key=value pair.
 *
 * @retval OT_ERROR_NONE          Successfully appended the option.
 * @retval OT_ERROR_INVALID_ARGS  The option type is not equal or greater than the last option type.
 * @retval OT_ERROR_NO_BUFS       The option length exceeds the buffer size.
 */
otError otCoapHeaderAppendUriQueryOption(otCoapHeader *aHeader, const char *aUriQuery);

/**
 * This function adds Payload Marker indicating beginning of the payload to the CoAP header.
 *
 * @param[inout]  aHeader  A pointer to the CoAP header.
 *
 * @retval OT_ERROR_NONE     Payload Marker successfully added.
 * @retval OT_ERROR_NO_BUFS  Header Payload Marker exceeds the buffer size.
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
 * @retval OT_ERROR_NONE    Successfully sent CoAP message.
 * @retval OT_ERROR_NO_BUFS Failed to allocate retransmission data.
 *
 */
otError otCoapSendRequest(otInstance *aInstance, otMessage *aMessage, const otMessageInfo *aMessageInfo,
                          otCoapResponseHandler aHandler, void *aContext);

/**
 * This function starts the CoAP server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aPort      The local UDP port to bind to.
 *
 * @retval OT_ERROR_NONE  Successfully started the CoAP server.
 *
 */
otError otCoapStart(otInstance *aInstance, uint16_t aPort);

/**
 * This function stops the CoAP server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE  Successfully stopped the CoAP server.
 *
 */
otError otCoapStop(otInstance *aInstance);

/**
 * This function adds a resource to the CoAP server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aResource  A pointer to the resource.
 *
 * @retval OT_ERROR_NONE     Successfully added @p aResource.
 * @retval OT_ERROR_ALREADY  The @p aResource was already added.
 *
 */
otError otCoapAddResource(otInstance *aInstance, otCoapResource *aResource);

/**
 * This function removes a resource from the CoAP server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aResource  A pointer to the resource.
 *
 */
void otCoapRemoveResource(otInstance *aInstance, otCoapResource *aResource);

/**
 * This function sets the default handler for unhandled CoAP requests.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aHandler   A function pointer that shall be called when an unhandled request arrives.
 * @param[in]  aContext   A pointer to arbitrary context information. May be NULL if not used.
 */
void otCoapSetDefaultHandler(otInstance *aInstance, otCoapRequestHandler aHandler, void *aContext);

/**
 * This function sends a CoAP response from the server.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aMessage      A pointer to the CoAP response to send.
 * @param[in]  aMessageInfo  A pointer to the message info associated with @p aMessage.
 *
 * @retval OT_ERROR_NONE     Successfully enqueued the CoAP response message.
 * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to send the CoAP response.
 *
 */
otError otCoapSendResponse(otInstance *aInstance, otMessage *aMessage, const otMessageInfo *aMessageInfo);
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif /* OPENTHREAD_COAP_H_ */
