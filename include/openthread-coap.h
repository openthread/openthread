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

#include <stdint.h>

#include <openthread-types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup coap  CoAP
 *
 * @brief
 *   This module includes functions that control CoAP communication.
 *
 * @{
 *
 */

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
 * CoAP Code values.
 *
 */
typedef enum otCoapCode
{
    kCoapRequestGet      = 0x01,  ///< Get
    kCoapRequestPost     = 0x02,  ///< Post
    kCoapRequestPut      = 0x03,  ///< Put
    kCoapRequestDelete   = 0x04,  ///< Delete
    kCoapResponseChanged = 0x44,  ///< Changed
    kCoapResponseContent = 0x45,  ///< Content
} otCoapCode;

/**
 * CoAP Option Numbers
 */
typedef enum otCoapOptionType
{
    kCoapOptionUriPath       = 11,   ///< Uri-Path
    kCoapOptionContentFormat = 12,   ///< Content-Format
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
            uint8_t mVersionTypeToken;
            uint8_t mCode;
            uint16_t mMessageId;
        } mFields;
        uint8_t mBytes[OT_COAP_HEADER_MAX_LENGTH];
    } mHeader;
    uint8_t mHeaderLength;
    uint16_t mOptionLast;
    uint16_t mNextOptionOffset;
    otCoapOption mOption;
} otCoapHeader;

/**
 * This function pointer is called when a CoAP response is received or on the request timeout.
 *
 * @param[in]  aContext  A pointer to application-specific context.
 * @param[in[  aHeader   A pointer to the received CoAP header. NULL if no response was received.
 * @param[in]  aMessage  A pointer to the message buffer containing the response. NULL if no response was received.
 * @param[in]  aResult   A result of the CoAP transaction.
 *
 * @retval  kThreadError_None             A response was received successfully.
 * @retval  kThreadError_Abort            A CoAP transaction was reseted by peer.
 * @retval  kThreadError_ResponseTimeout  No response or acknowledgment received during timeout period.
 *
 */
typedef void (*otCoapResponseHandler)(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                      ThreadError aResult);

/**
 * This method initializes the CoAP header.
 *
 * @param[inout] aHeader  A pointer to the CoAP header to initialize.
 * @param[in]    aType    CoAP message type.
 * @param[in]    aCode    CoAP message code.
 *
 */
void otCoapHeaderInit(otCoapHeader *aHeader, otCoapType aType, otCoapCode aCode);

/**
 * This method sets the Token value and length in a header.
 *
 * @param[inout]  aHeader       A pointer to the CoAP header.
 * @param[in]     aToken        A pointer to the Token value.
 * @param[in]     aTokenLength  The Length of @p aToken.
 *
 */
void otCoapHeaderSetToken(otCoapHeader *aHeader, const uint8_t *aToken, uint8_t aTokenLength);

/**
 * This method appends a CoAP option in a header.
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
 * This method adds Payload Marker indicating beginning of the payload to the CoAP header.
 *
 * @param[inout]  aHeader  A pointer to the CoAP header.
 *
 * @retval kThreadError_None    Payload Marker successfully added.
 * @retval kThreadError_NoBufs  Header Payload Marker exceeds the buffer size.
 *
 */
void otCoapHeaderSetPayloadMarker(otCoapHeader *aHeader);

/**
 * This method returns a pointer to the current option.
 *
 * @param[in]  aHeader  A pointer to the CoAP header.
 *
 * @returns A pointer to the current option. If no option is present NULL pointer is returned.
 *
 */
const otCoapOption *otCoapGetCurrentOption(const otCoapHeader *aHeader);

/**
 * This method returns a pointer to the next option.
 *
 * @param[in]  aHeader  A pointer to the CoAP header.
 *
 * @returns A pointer to the next option. If no more options are present NULL pointer is returned.
 *
 */
const otCoapOption *otCoapGetNextOption(otCoapHeader *aHeader);

/**
 * This method creates a new message with a CoAP header.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aHeader  A pointer to a CoAP header that is used to create the message.
 *
 * @returns A pointer to the message or NULL if failed to allocate message.
 *
 */
otMessage otNewCoapMessage(otInstance *aInstance, const otCoapHeader *aHeader);

/**
 * This method sends a CoAP message.
 *
 * If a response for a request is expected, respective function and contex information should be provided.
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
ThreadError otSendCoapMessage(otInstance *aInstance, otMessage aMessage, const otMessageInfo *aMessageInfo,
                              otCoapResponseHandler aHandler, void *aContext);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif /* OPENTHREAD_COAP_H_ */
