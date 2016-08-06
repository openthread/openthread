/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <common/message.hpp>

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
class Header
{
public:
    /**
     * This method initializes the CoAP header.
     *
     */
    void Init(void);

    /**
     * This method parses the CoAP header from a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None   Successfully parsed the message.
     * @retval kThreadError_Parse  Failed to parse the message.
     *
     */
    ThreadError FromMessage(const Message &aMessage);

    /**
     * This method returns the Version value.
     *
     * @returns The Version value.
     *
     */
    uint8_t GetVersion(void) const { return (mHeader[0] & kVersionMask) >> kVersionOffset; }

    /**
     * This method sets the Version value.
     *
     * @param[in]  aVersion  The Version value.
     *
     */
    void SetVersion(uint8_t aVersion) { mHeader[0] &= ~kVersionMask; mHeader[0] |= aVersion << kVersionOffset; }

    /**
     * CoAP Type values.
     *
     */
    enum Type
    {
        kTypeConfirmable    = 0x00,  ///< Confirmable
        kTypeNonConfirmable = 0x10,  ///< Non-confirmable
        kTypeAcknowledgment = 0x20,  ///< Acknowledgment
        kTypeReset          = 0x30,  ///< Reset
    };

    /**
     * This method returns the Type value.
     *
     * @returns The Type value.
     *
     */
    Type GetType(void) const { return static_cast<Header::Type>(mHeader[0] & kTypeMask); }

    /**
     * This method sets the Type value.
     *
     * @param[in]  aType  The Type value.
     *
     */
    void SetType(Type aType) { mHeader[0] &= ~kTypeMask; mHeader[0] |= aType; }

    /**
     * CoAP Code values.
     *
     */
    enum Code
    {
        kCodeGet     = 0x01,  ///< Get
        kCodePost    = 0x02,  ///< Post
        kCodePut     = 0x03,  ///< Put
        kCodeDelete  = 0x04,  ///< Delete
        kCodeChanged = 0x44,  ///< Changed
        kCodeContent = 0x45,  ///< Content
    };

    /**
     * This method returns the Code value.
     *
     * @returns The Code value.
     *
     */
    Code GetCode(void) const { return static_cast<Header::Code>(mHeader[1]); }

    /**
     * This method sets the Code value.
     *
     * @param[in]  aCode  The Code value.
     *
     */
    void SetCode(Code aCode) { mHeader[1] = (uint8_t)aCode; }

    /**
     * This method returns the Message ID value.
     *
     * @returns The Message ID value.
     *
     */
    uint16_t GetMessageId(void) const { return (static_cast<uint16_t>(mHeader[2]) << 8) | mHeader[3]; }

    /**
     * This method sets the Message ID value.
     *
     * @param[in]  aMessageId  The Message ID value.
     *
     */
    void SetMessageId(uint16_t aMessageId) { mHeader[2] = aMessageId >> 8; mHeader[3] = (uint8_t)aMessageId; }

    /**
     * This method returns the Token length.
     *
     * @returns The Token length.
     *
     */
    uint8_t GetTokenLength(void) const { return (mHeader[0] & kTokenLengthMask) >> kTokenLengthOffset; }

    /**
     * This method returns a pointer to the Token value.
     *
     * @returns A pointer to the Token value.
     *
     */
    const uint8_t *GetToken(void) const { return mHeader + kTokenOffset; }

    /**
     * This method sets the Token value and length.
     *
     * @param[in]  aToken        A pointer to the Token value.
     * @param[in]  aTokenLength  The Length of @p aToken.
     *
     */
    void SetToken(const uint8_t *aToken, uint8_t aTokenLength) {
        mHeader[0] = (mHeader[0] & ~kTokenLengthMask) | (aTokenLength << kTokenLengthOffset);
        memcpy(mHeader + kTokenOffset, aToken, aTokenLength);
        mHeaderLength += aTokenLength;
    }

    /**
     * This structure represents a CoAP option.
     *
     */
    struct Option
    {
        /**
         * Protocol Constants
         *
         */
        enum
        {
            kOptionDeltaOffset   = 4,    ///< Delta
        };

        /**
         * Option Numbers
         */
        enum Type
        {
            kOptionUriPath       = 11,   ///< Uri-Path
            kOptionContentFormat = 12,   ///< Content-Format
        };

        uint16_t       mNumber;  ///< Option Number
        uint16_t       mLength;  ///< Option Length
        const uint8_t *mValue;   ///< A pointer to the Option Value
    };

    /**
     * This method appends a CoAP option.
     *
     * @param[in]  aOption  The CoAP Option.
     *
     * @retval kThreadError_None         Successfully appended the option.
     * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
     *
     */
    ThreadError AppendOption(const Option &aOption);

    /**
     * This method appends a Uri-Path option.
     *
     * @param[in]  aUriPath  A pointer to a NULL-terminated string.
     *
     * @retval kThreadError_None         Successfully appended the option.
     * @retval kThreadError_InvalidArgs  The option type is not equal or greater than the last option type.
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
     *
     */
    ThreadError AppendContentFormatOption(MediaType aType);

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
     * This method terminates the CoAP header.
     *
     */
    void Finalize(void) { mHeader[mHeaderLength++] = 0xff; }

    /**
     * This method returns a pointer to the first byte of the header.
     *
     * @returns A pointer to the first byte of the header.
     *
     */
    const uint8_t *GetBytes(void) const { return mHeader; }

    /**
     * This method returns the header length in bytes.
     *
     * @returns The header length in bytes.
     *
     */
    uint8_t GetLength(void) const { return mHeaderLength; }

private:
    /**
     * Protocol Constants (RFC 7252).
     *
     */
    enum
    {
        kVersionMask                = 0xc0,  ///< Version mask as specified (RFC 7252).
        kVersionOffset              = 6,     ///< Version offset as specified (RFC 7252).

        kTokenLengthMask            = 0x0f,  ///< Token Length mask as specified (RFC 7252).
        kTokenLengthOffset          = 0,     ///< Token Length offset as specified (RFC 7252).
        kTokenOffset                = 4,     ///< Token offset as specified (RFC 7252).
        kMaxTokenLength             = 8,     ///< Max token length as specified (RFC 7252).

        kOption1ByteExtension       = 13,    ///< Indicates a 1 byte extension (RFC 7252).
        kOption2ByteExtension       = 14,    ///< Indicates a 1 byte extension (RFC 7252).

        kOption1ByteExtensionOffset = 13,    ///< Delta/Length offset as specified (RFC 7252).
        kOption2ByteExtensionOffset = 269,   ///< Delta/Length offset as specified (RFC 7252).
    };

    enum
    {
        kTypeMask = 0x30,
        kMinHeaderLength = 4,
        kMaxHeaderLength = 128,
    };
    uint8_t mHeader[kMaxHeaderLength];
    uint8_t mHeaderLength;
    uint16_t mOptionLast;
    uint16_t mNextOptionOffset;
    Option mOption;
};

/**
 * @}
 *
 */

}  // namespace Coap
}  // namespace Thread

#endif  // COAP_HEADER_HPP_
