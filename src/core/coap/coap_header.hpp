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
 *   This file includes definitions for generating and processing CoAP headers.
 */

#ifndef COAP_HEADER_HPP_
#define COAP_HEADER_HPP_

#include <string.h>

#include <openthread-types.h>
#include <openthread-coap.h>
#include <common/encoding.hpp>
#include <common/message.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {

/**
 * @namespace Thread::Coap
 * @brief
 *   This namespace includes definitions for CoAP.
 *
 */
namespace Coap {

/**
 * @addtogroup core-coap
 *
 * @brief
 *   This module includes definitions for CoAP.
 *
 * @{
 *
 */

/**
 * This class implements CoAP header generation and parsing.
 *
 */
class Header : public otCoapHeader
{
public:
    enum
    {
        kVersion1           = 1,                         ///< Version 1
        kMinHeaderLength    = 4,                         ///< Minimum header length
        kMaxHeaderLength    = OT_COAP_HEADER_MAX_LENGTH, ///< Maximum header length
        kDefaultTokenLength = 2                          ///< Default token length
    };

    /**
     * CoAP Type values.
     *
     */
    typedef otCoapType Type;

    /**
     * CoAP Code values.
     *
     */
    typedef otCoapCode Code;

    /**
     * This method initializes the CoAP header.
     *
     */
    void Init(void);

    /**
     * This method initializes the CoAP header with specific Type and Code.
     *
     * @param[in]  aType  The Type value.
     * @param[in]  aCode  The Code value.
     *
     */
    void Init(Type aType, Code aCode);

    /**
     * This method parses the CoAP header from a message.
     *
     * @param[in]  aMessage       A reference to the message.
     * @param[in]  aCopiedMessage To indicate if this message is a copied one or not
     *
     * @retval kThreadError_None   Successfully parsed the message.
     * @retval kThreadError_Parse  Failed to parse the message.
     *
     */
    ThreadError FromMessage(const Message &aMessage, bool aCopiedMessage);

    /**
     * This method returns the Version value.
     *
     * @returns The Version value.
     *
     */
    uint8_t GetVersion(void) const { return (mHeader.mFields.mVersionTypeToken & kVersionMask) >> kVersionOffset; }

    /**
     * This method sets the Version value.
     *
     * @param[in]  aVersion  The Version value.
     *
     */
    void SetVersion(uint8_t aVersion) {
        mHeader.mFields.mVersionTypeToken &= ~kVersionMask;
        mHeader.mFields.mVersionTypeToken |= aVersion << kVersionOffset;
    }

    /**
     * This method returns the Type value.
     *
     * @returns The Type value.
     *
     */
    Type GetType(void) const { return static_cast<Header::Type>(mHeader.mFields.mVersionTypeToken & kTypeMask); }

    /**
     * This method sets the Type value.
     *
     * @param[in]  aType  The Type value.
     *
     */
    void SetType(Type aType) {
        mHeader.mFields.mVersionTypeToken &= ~kTypeMask;
        mHeader.mFields.mVersionTypeToken |= aType;
    }

    /**
     * This method returns the Code value.
     *
     * @returns The Code value.
     *
     */
    Code GetCode(void) const { return static_cast<Code>(mHeader.mFields.mCode); }

    /**
     * This method sets the Code value.
     *
     * @param[in]  aCode  The Code value.
     *
     */
    void SetCode(Code aCode) { mHeader.mFields.mCode = static_cast<uint8_t>(aCode); }

    /**
     * This method returns the Message ID value.
     *
     * @returns The Message ID value.
     *
     */
    uint16_t GetMessageId(void) const { return HostSwap16(mHeader.mFields.mMessageId); }

    /**
     * This method sets the Message ID value.
     *
     * @param[in]  aMessageId  The Message ID value.
     *
     */
    void SetMessageId(uint16_t aMessageId) { mHeader.mFields.mMessageId = HostSwap16(aMessageId); }

    /**
     * This method returns the Token length.
     *
     * @returns The Token length.
     *
     */
    uint8_t GetTokenLength(void) const { return (mHeader.mFields.mVersionTypeToken & kTokenLengthMask) >> kTokenLengthOffset; }

    /**
     * This method returns a pointer to the Token value.
     *
     * @returns A pointer to the Token value.
     *
     */
    const uint8_t *GetToken(void) const { return mHeader.mBytes + kTokenOffset; }

    /**
     * This method sets the Token value and length.
     *
     * @param[in]  aToken        A pointer to the Token value.
     * @param[in]  aTokenLength  The Length of @p aToken.
     *
     */
    void SetToken(const uint8_t *aToken, uint8_t aTokenLength) {
        mHeader.mFields.mVersionTypeToken = (mHeader.mFields.mVersionTypeToken & ~kTokenLengthMask) |
                                            ((aTokenLength << kTokenLengthOffset) & kTokenLengthMask);
        memcpy(mHeader.mBytes + kTokenOffset, aToken, aTokenLength);
        mHeaderLength += aTokenLength;
    }

    /**
     * This method sets the Token length and randomizes its value.
     *
     * @param[in]  aTokenLength  The Length of a Token to set.
     *
     */
    void SetToken(uint8_t aTokenLength);

    /**
     *  This method checks if Tokens in two CoAP headers are equal.
     *
     *  @param[in]  aHeader  A header to compare.
     *
     * @retval TRUE   If two Tokens are equal.
     * @retval FALSE  If Tokens differ in length or value.
     *
     */
    bool IsTokenEqual(const Header &aHeader) const {
        return ((this->GetTokenLength() == aHeader.GetTokenLength()) &&
                (memcmp(this->GetToken(), aHeader.GetToken(), this->GetTokenLength()) == 0));
    }

    /**
     * This structure represents a CoAP option.
     *
     */
    struct Option : public otCoapOption
    {
        /**
         * Protocol Constants
         *
         */
        enum
        {
            kOptionDeltaOffset   = 4,                          ///< Delta Offset
            kOptionDeltaMask     = 0xf << kOptionDeltaOffset,  ///< Delta Mask
        };

        /**
         * Option Numbers
         */
        typedef otCoapOptionType Type;
    };

    /**
     * This method appends a CoAP option.
     *
     * @param[in]  aOption  The CoAP Option.
     *
     * @retval kThreadError_None         Successfully appended the option.
     * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kThreadError_NoBufs       The option length exceeds the buffer size.
     *
     */
    ThreadError AppendOption(const Option &aOption);

    /**
     * This method appends an Observe option.
     *
     * @param[in]  aObserve  Observe field value.
     *
     * @retval kThreadError_None         Successfully appended the option.
     * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kThreadError_NoBufs       The option length exceeds the buffer size.
     */
    ThreadError AppendObserveOption(uint32_t aObserve);

    /**
     * This method appends a Uri-Path option.
     *
     * @param[in]  aUriPath  A pointer to a NULL-terminated string.
     *
     * @retval kThreadError_None         Successfully appended the option.
     * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kThreadError_NoBufs       The option length exceeds the buffer size.
     *
     */
    ThreadError AppendUriPathOptions(const char *aUriPath);

    /**
     * Media Types
     *
     */
    enum MediaType
    {
        kApplicationOctetStream = 42,  ///< application/octet-stream
    };

    /**
     * This method appends a Content-Format option.
     *
     * @param[in]  aType  The Media Type value.
     *
     * @retval kThreadError_None         Successfully appended the option.
     * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kThreadError_NoBufs       The option length exceeds the buffer size.
     *
     */
    ThreadError AppendContentFormatOption(MediaType aType);

    /**
     * This method appends a Max-Age option.
     *
     * @param[in]  aMaxAge  The Max-Age value.
     *
     * @retval kThreadError_None         Successfully appended the option.
     * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kThreadError_NoBufs       The option length exceeds the buffer size.
     */
    ThreadError AppendMaxAgeOption(uint32_t aMaxAge);

    /**
     * This method appends a single Uri-Query option.
     *
     * @param[in]  aUriQuery  A pointer to NULL-terminated string, which should contain
     *  a single key=value pair.
     *
     * @retval kThreadError_None         Successfully appended the option.
     * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kThreadError_NoBufs       The option length exceeds the buffer size.
     */
    ThreadError AppendUriQueryOption(const char *aUriQuery);

    /**
     * This method returns a pointer to the current option.
     *
     * @returns A pointer to the current option.
     *
     */
    const Option *GetCurrentOption(void) const;

    /**
     * This method returns a pointer to the next option.
     *
     * @returns A pointer to the next option.
     *
     */
    const Option *GetNextOption(void);

    /**
     * This method adds Payload Marker indicating beginning of the payload to the CoAP header.
     *
     * @retval kThreadError_None    Payload Marker successfully added.
     * @retval kThreadError_NoBufs  Header Payload Marker exceeds the buffer size.
     *
     */
    ThreadError SetPayloadMarker(void);

    /**
     * This method returns a pointer to the first byte of the header.
     *
     * @returns A pointer to the first byte of the header.
     *
     */
    const uint8_t *GetBytes(void) const { return mHeader.mBytes; }

    /**
     * This method returns the header length in bytes.
     *
     * @returns The header length in bytes.
     *
     */
    uint8_t GetLength(void) const { return mHeaderLength; }

    /**
     * This method sets a default response header based on request header.
     *
     * @param[in]  aRequestHeader  Request header to base on.
     *
     */
    void SetDefaultResponseHeader(const Header &aRequestHeader);

    /**
     * This method checks if a header is an empty message header.
     *
     * @retval TRUE   Header is an empty message header.
     * @retval FALSE  Header is not an empty message header.
     *
     */
    bool IsEmpty(void) const { return (GetCode() == 0); };

    /**
     * This method checks if a header is a request header.
     *
     * @retval TRUE   Header is a request header.
     * @retval FALSE  Header is not a request header.
     *
     */
    bool IsRequest(void) const { return (GetCode() >= kCoapRequestGet && GetCode() <= kCoapRequestDelete); };

    /**
     * This method checks if a header is a response header.
     *
     * @retval TRUE   Header is a response header.
     * @retval FALSE  Header is not a response header.
     *
     */
    bool IsResponse(void) const { return (GetCode() >= kCoapResponseChanged); };

    /**
     * This method checks if a header is a CON message header.
     *
     * @retval TRUE   Header is a CON message header.
     * @retval FALSE  Header is not is a CON message header.
     *
     */
    bool IsConfirmable(void) const { return (GetType() == kCoapTypeConfirmable); };

    /**
     * This method checks if a header is a NON message header.
     *
     * @retval TRUE   Header is a NON message header.
     * @retval FALSE  Header is not is a NON message header.
     *
     */
    bool IsNonConfirmable(void) const { return (GetType() == kCoapTypeNonConfirmable); };

    /**
     * This method checks if a header is a ACK message header.
     *
     * @retval TRUE   Header is a ACK message header.
     * @retval FALSE  Header is not is a ACK message header.
     *
     */
    bool IsAck(void) const { return (GetType() == kCoapTypeAcknowledgment); };

    /**
     * This method checks if a header is a RST message header.
     *
     * @retval TRUE   Header is a RST message header.
     * @retval FALSE  Header is not is a RST message header.
     *
     */
    bool IsReset(void) const { return (GetType() == kCoapTypeReset);  };

private:
    /**
     * Protocol Constants (RFC 7252).
     *
     */
    enum
    {
        kVersionMask                = 0xc0,  ///< Version mask as specified (RFC 7252).
        kVersionOffset              = 6,     ///< Version offset as specified (RFC 7252).

        kTypeMask                   = 0x30,  ///< Type mask as specified (RFC 7252).

        kTokenLengthMask            = 0x0f,  ///< Token Length mask as specified (RFC 7252).
        kTokenLengthOffset          = 0,     ///< Token Length offset as specified (RFC 7252).
        kTokenOffset                = 4,     ///< Token offset as specified (RFC 7252).
        kMaxTokenLength             = 8,     ///< Max token length as specified (RFC 7252).

        kMaxOptionHeaderSize        = 5,     ///< Maximum size of an Option header

        kOption1ByteExtension       = 13,    ///< Indicates a 1 byte extension (RFC 7252).
        kOption2ByteExtension       = 14,    ///< Indicates a 1 byte extension (RFC 7252).

        kOption1ByteExtensionOffset = 13,    ///< Delta/Length offset as specified (RFC 7252).
        kOption2ByteExtensionOffset = 269,   ///< Delta/Length offset as specified (RFC 7252).
    };
};

/**
 * @}
 *
 */

}  // namespace Coap
}  // namespace Thread

#endif  // COAP_HEADER_HPP_
