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
 *   This file includes definitions for generating and processing CoAP messages.
 */

#ifndef OT_CORE_COAP_COAP_MESSAGE_HPP_
#define OT_CORE_COAP_COAP_MESSAGE_HPP_

#include "openthread-core-config.h"

#include <openthread/coap.h>

#include "common/as_core_type.hpp"
#include "common/bit_utils.hpp"
#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "common/const_cast.hpp"
#include "common/encoding.hpp"
#include "common/equatable.hpp"
#include "common/message.hpp"
#include "net/ip6.hpp"
#include "net/ip6_address.hpp"
#include "net/udp6.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

/**
 * @namespace ot::Coap
 * @brief
 *   This namespace includes definitions for CoAP.
 */
namespace Coap {

/**
 * @addtogroup core-coap
 *
 * @brief
 *   This module includes definitions for CoAP.
 *
 * @{
 */

class Msg;
class Message;
class Option;
class CoapBase;

/**
 * CoAP Type values.
 */
enum Type : uint8_t
{
    kTypeConfirmable    = OT_COAP_TYPE_CONFIRMABLE,     ///< Confirmable type.
    kTypeNonConfirmable = OT_COAP_TYPE_NON_CONFIRMABLE, ///< Non-confirmable type.
    kTypeAck            = OT_COAP_TYPE_ACKNOWLEDGMENT,  ///< Acknowledgment type.
    kTypeReset          = OT_COAP_TYPE_RESET,           ///< Reset type.
};

/**
 * CoAP Code values.
 */
enum Code : uint8_t
{
    // Request Codes:

    kCodeEmpty  = OT_COAP_CODE_EMPTY,  ///< Empty message code
    kCodeGet    = OT_COAP_CODE_GET,    ///< Get
    kCodePost   = OT_COAP_CODE_POST,   ///< Post
    kCodePut    = OT_COAP_CODE_PUT,    ///< Put
    kCodeDelete = OT_COAP_CODE_DELETE, ///< Delete

    // Response Codes:

    kCodeResponseMin = OT_COAP_CODE_RESPONSE_MIN, ///< 2.00
    kCodeCreated     = OT_COAP_CODE_CREATED,      ///< Created
    kCodeDeleted     = OT_COAP_CODE_DELETED,      ///< Deleted
    kCodeValid       = OT_COAP_CODE_VALID,        ///< Valid
    kCodeChanged     = OT_COAP_CODE_CHANGED,      ///< Changed
    kCodeContent     = OT_COAP_CODE_CONTENT,      ///< Content
    kCodeContinue    = OT_COAP_CODE_CONTINUE,     ///< RFC7959 Continue

    // Client Error Codes:

    kCodeBadRequest         = OT_COAP_CODE_BAD_REQUEST,         ///< Bad Request
    kCodeUnauthorized       = OT_COAP_CODE_UNAUTHORIZED,        ///< Unauthorized
    kCodeBadOption          = OT_COAP_CODE_BAD_OPTION,          ///< Bad Option
    kCodeForbidden          = OT_COAP_CODE_FORBIDDEN,           ///< Forbidden
    kCodeNotFound           = OT_COAP_CODE_NOT_FOUND,           ///< Not Found
    kCodeMethodNotAllowed   = OT_COAP_CODE_METHOD_NOT_ALLOWED,  ///< Method Not Allowed
    kCodeNotAcceptable      = OT_COAP_CODE_NOT_ACCEPTABLE,      ///< Not Acceptable
    kCodeRequestIncomplete  = OT_COAP_CODE_REQUEST_INCOMPLETE,  ///< RFC7959 Request Entity Incomplete
    kCodePreconditionFailed = OT_COAP_CODE_PRECONDITION_FAILED, ///< Precondition Failed
    kCodeRequestTooLarge    = OT_COAP_CODE_REQUEST_TOO_LARGE,   ///< Request Entity Too Large
    kCodeUnsupportedFormat  = OT_COAP_CODE_UNSUPPORTED_FORMAT,  ///< Unsupported Content-Format

    // Server Error Codes:

    kCodeInternalError      = OT_COAP_CODE_INTERNAL_ERROR,      ///< Internal Server Error
    kCodeNotImplemented     = OT_COAP_CODE_NOT_IMPLEMENTED,     ///< Not Implemented
    kCodeBadGateway         = OT_COAP_CODE_BAD_GATEWAY,         ///< Bad Gateway
    kCodeServiceUnavailable = OT_COAP_CODE_SERVICE_UNAVAILABLE, ///< Service Unavailable
    kCodeGatewayTimeout     = OT_COAP_CODE_GATEWAY_TIMEOUT,     ///< Gateway Timeout
    kCodeProxyNotSupported  = OT_COAP_CODE_PROXY_NOT_SUPPORTED, ///< Proxying Not Supported
};

/**
 * CoAP Option Numbers.
 */
enum OptionNumber : uint16_t
{
    kOptionIfMatch       = OT_COAP_OPTION_IF_MATCH,       ///< If-Match
    kOptionUriHost       = OT_COAP_OPTION_URI_HOST,       ///< Uri-Host
    kOptionETag          = OT_COAP_OPTION_E_TAG,          ///< ETag
    kOptionIfNoneMatch   = OT_COAP_OPTION_IF_NONE_MATCH,  ///< If-None-Match
    kOptionObserve       = OT_COAP_OPTION_OBSERVE,        ///< Observe [RFC7641]
    kOptionUriPort       = OT_COAP_OPTION_URI_PORT,       ///< Uri-Port
    kOptionLocationPath  = OT_COAP_OPTION_LOCATION_PATH,  ///< Location-Path
    kOptionUriPath       = OT_COAP_OPTION_URI_PATH,       ///< Uri-Path
    kOptionContentFormat = OT_COAP_OPTION_CONTENT_FORMAT, ///< Content-Format
    kOptionMaxAge        = OT_COAP_OPTION_MAX_AGE,        ///< Max-Age
    kOptionUriQuery      = OT_COAP_OPTION_URI_QUERY,      ///< Uri-Query
    kOptionAccept        = OT_COAP_OPTION_ACCEPT,         ///< Accept
    kOptionLocationQuery = OT_COAP_OPTION_LOCATION_QUERY, ///< Location-Query
    kOptionBlock2        = OT_COAP_OPTION_BLOCK2,         ///< Block2 (RFC7959)
    kOptionBlock1        = OT_COAP_OPTION_BLOCK1,         ///< Block1 (RFC7959)
    kOptionSize2         = OT_COAP_OPTION_SIZE2,          ///< Size2 (RFC7959)
    kOptionProxyUri      = OT_COAP_OPTION_PROXY_URI,      ///< Proxy-Uri
    kOptionProxyScheme   = OT_COAP_OPTION_PROXY_SCHEME,   ///< Proxy-Scheme
    kOptionSize1         = OT_COAP_OPTION_SIZE1,          ///< Size1
};

/**
 * CoAP Block Size Exponents
 */
enum BlockSzx : uint8_t
{
    kBlockSzx16   = OT_COAP_OPTION_BLOCK_SZX_16,   ///< 16  bytes.
    kBlockSzx32   = OT_COAP_OPTION_BLOCK_SZX_32,   ///< 32  bytes.
    kBlockSzx64   = OT_COAP_OPTION_BLOCK_SZX_64,   ///< 64  bytes.
    kBlockSzx128  = OT_COAP_OPTION_BLOCK_SZX_128,  ///< 128 bytes.
    kBlockSzx256  = OT_COAP_OPTION_BLOCK_SZX_256,  ///< 256 bytes.
    kBlockSzx512  = OT_COAP_OPTION_BLOCK_SZX_512,  ///< 512 bytes.
    kBlockSzx1024 = OT_COAP_OPTION_BLOCK_SZX_1024, ///< 1024 bytes.
};

/**
 * Converts a CoAP Block Size Exponent (SZX) to the actual block size (in bytes).
 *
 * @param[in]   aBlockSzx     Block size exponent.
 *
 * @returns The actual size corresponding to @o aBlockSzx.
 */
uint16_t BlockSizeFromExponent(BlockSzx aBlockSzx);

/**
 * Represents information in a Block1 or Block2 Option (for block-wise transfer).
 */
struct BlockInfo
{
    /**
     * Returns the block size in bytes.
     *
     * @returns The block size in bytes derived from block size exponent (`mBlockSzx`).
     */
    uint16_t GetBlockSize(void) const { return BlockSizeFromExponent(mBlockSzx); }

    /**
     * Returns the current block offset position.
     *
     * @returns The block offset position, i.e. the current block number multiplied by the block size.
     */
    uint32_t GetBlockOffsetPosition(void) const { return mBlockNumber * GetBlockSize(); }

    uint32_t mBlockNumber; ///< The block number.
    BlockSzx mBlockSzx;    ///< The block size exponent.
    bool     mMoreBlocks;  ///< Whether more blocks are following (`M` flag).
};

/**
 * Represents a CoAP message Token.
 */
class Token : public otCoapToken, public Clearable<Token>, public Unequatable<Token>
{
    friend class Message;

public:
    static const uint8_t kMaxLength     = OT_COAP_MAX_TOKEN_LENGTH;     ///< Maximum token length.
    static const uint8_t kDefaultLength = OT_COAP_DEFAULT_TOKEN_LENGTH; ///< Default token length.

    /**
     * Indicates whether the Token is valid.
     *
     * A Token is valid if its length is less than or equal to `kMaxLength`.
     *
     * @retval TRUE  If the Token is valid.
     * @retval FALSE If the Token is not valid.
     */
    bool IsValid(void) const { return mLength <= kMaxLength; }

    /**
     * Returns a pointer to the Token bytes.
     *
     * @returns A pointer to the Token bytes.
     */
    const uint8_t *GetBytes(void) const { return m8; }

    /**
     * Returns the Token length in bytes.
     *
     * @returns The Token length in bytes.
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * Sets the Token bytes and length.
     *
     * @param[in] aBytes   A pointer to the Token bytes.
     * @param[in] aLength  The Token length in bytes.
     *
     * @retval kErrorNone         Successfully set the Token.
     * @retval kErrorInvalidArgs  The specified length @p aLength is greater than `kMaxLength`.
     */
    Error SetToken(const uint8_t *aBytes, uint8_t aLength);

    /**
     * Overloads the `==` operator to compare two CoAP Tokens.
     *
     * @param[in] aOther  The other Token to compare with.
     *
     * @retval TRUE   If the two Tokens are equal.
     * @retval FALSE  If the two Tokens are not equal.
     */
    bool operator==(const Token &aOther) const;

private:
    Error GenerateRandom(uint8_t aLength);
};

/**
 * Represents information from a parsed CoAP Header in a CoAP message.
 */
class HeaderInfo : public Clearable<HeaderInfo>
{
    friend class Message;
    friend class Msg;

public:
    /**
     * Returns the Type value.
     *
     * @returns The Type value.
     */
    uint8_t GetType(void) const { return mType; }

    /**
     * Returns the Code value.
     *
     * @returns The Code value.
     */
    uint8_t GetCode(void) const { return mCode; }

    /**
     * Returns the Message ID value.
     *
     * @returns The Message ID value.
     */
    uint16_t GetMessageId(void) const { return mMessageId; }

    /**
     * Returns the Token.
     *
     * @returns The Token.
     */
    const Token &GetToken(void) const { return mToken; }

    /**
     * Checks if a header is an empty message header (`kCodeEmpty`).
     *
     * @retval TRUE   Message is an empty message header.
     * @retval FALSE  Message is not an empty message header.
     */
    bool IsEmpty(void) const { return (mCode == kCodeEmpty); }

    /**
     * Checks if a header is a request header.
     *
     * @retval TRUE   Message is a request header.
     * @retval FALSE  Message is not a request header.
     */
    bool IsRequest(void) const;

    /**
     * Indicates whether or not the CoAP code in header is "Get" request.
     *
     * @retval TRUE   Message is a Get request.
     * @retval FALSE  Message is not a Get request.
     */
    bool IsGetRequest(void) const { return (mCode == kCodeGet); }

    /**
     * Indicates whether or not the CoAP code in header is "Post" request.
     *
     * @retval TRUE   Message is a Post request.
     * @retval FALSE  Message is not a Post request.
     */
    bool IsPostRequest(void) const { return (mCode == kCodePost); }

    /**
     * Indicates whether or not the CoAP code in header is "Put" request.
     *
     * @retval TRUE   Message is a Put request.
     * @retval FALSE  Message is not a Put request.
     */
    bool IsPutRequest(void) const { return (mCode == kCodePut); }

    /**
     * Indicates whether or not the CoAP code in header is "Delete" request.
     *
     * @retval TRUE   Message is a Delete request.
     * @retval FALSE  Message is not a Delete request.
     */
    bool IsDeleteRequest(void) const { return (mCode == kCodeDelete); }

    /**
     * Checks if a header is a response header.
     *
     * @retval TRUE   Message is a response header.
     * @retval FALSE  Message is not a response header.
     */
    bool IsResponse(void) const { return mCode >= kCodeResponseMin; }

    /**
     * Checks if a header is a CON message header.
     *
     * @retval TRUE   Message is a CON message header.
     * @retval FALSE  Message is not is a CON message header.
     */
    bool IsConfirmable(void) const { return (GetType() == kTypeConfirmable); }

    /**
     * Checks if a header is a NON message header.
     *
     * @retval TRUE   Message is a NON message header.
     * @retval FALSE  Message is not is a NON message header.
     */
    bool IsNonConfirmable(void) const { return (mType == kTypeNonConfirmable); }

    /**
     * Checks if a header is a ACK message header.
     *
     * @retval TRUE   Message is a ACK message header.
     * @retval FALSE  Message is not is a ACK message header.
     */
    bool IsAck(void) const { return (mType == kTypeAck); }

    /**
     * Checks if a header is a RST message header.
     *
     * @retval TRUE   Message is a RST message header.
     * @retval FALSE  Message is not is a RST message header.
     */
    bool IsReset(void) const { return (mType == kTypeReset); }

    /**
     * Indicates whether or not the header is a confirmable Post request (`kTypeConfirmable` with`kCodePost`).
     *
     * @retval TRUE   Message is a confirmable Post request.
     * @retval FALSE  Message is not a confirmable Post request.
     */
    bool IsConfirmablePostRequest(void) const;

    /**
     * Indicates whether the message is a non-confirmable Post request (`kTypeNonConfirmable` with `kCodePost`).
     *
     * @retval TRUE   Message is a non-confirmable Post request.
     * @retval FALSE  Message is not a non-confirmable Post request.
     */
    bool IsNonConfirmablePostRequest(void) const;

    /**
     * Checks if the message requires a reset response if an error during low level CoAP processing occurred.
     *
     * A reset message is expected to be sent for NON and CON messages if the message can not be processed or a
     * duplicated message has been received.
     *
     * @retval  TRUE   Expect to respond with CoAP reset message on error.
     * @retval  FALSE  No CoAP reset message should be sent on error.
     */
    bool RequireResetOnError(void) { return IsConfirmable() || IsNonConfirmable(); }

private:
    uint8_t  mType;
    uint8_t  mCode;
    uint16_t mMessageId;
    Token    mToken;
};

/**
 * Implements CoAP message generation and parsing.
 */
class Message : public ot::Message
{
    friend class Msg;
    friend class Option;
    friend class MessageQueue;
    friend class CoapBase;

public:
    static constexpr uint8_t kMaxReceivedUriPath = 32; ///< Max URI path length on rx msgs.

    typedef ot::Coap::Type Type; ///< CoAP Type.
    typedef ot::Coap::Code Code; ///< CoAP Code.

    typedef char UriPathStringBuffer[kMaxReceivedUriPath + 1]; ///< Buffer to store a received URI Path string.

    /**
     * Initializes the CoAP message with a given Type and Code.
     *
     * This method erases any previously written content in the message. The Message ID is set to zero, and the token
     * is empty (zero-length).
     *
     * @param[in] aType  The CoAP Type value.
     * @param[in] aCode  The CoAP Code value.
     *
     * @retval kErrorNone    Successfully initialized the message.
     * @retval kErrorNoBufs  Could not grow the message to write the CoAP header.
     */
    Error Init(Type aType, Code aCode);

    /**
     * Initializes the CoAP message with a given Type, Code, and Message ID.
     *
     * This method erases any previously written content in the message. The token is empty (zero-length).
     *
     * @param[in] aType       The CoAP Type value.
     * @param[in] aCode       The CoAP Code value.
     * @param[in] aMessageId  The CoAP Message ID value.
     *
     * @retval kErrorNone    Successfully initialized the message.
     * @retval kErrorNoBufs  Could not grow the message to write the CoAP header.
     */
    Error Init(Type aType, Code aCode, uint16_t aMessageId);

    /**
     * Initializes the message with a Type, Code, adds a random token, and appends a URI-path option.
     *
     * This method erases any previously written content in the message. The Message ID is set to zero. A random token
     * of default length (`Token::kDefaultLength`) is generated and added to the message.
     *
     * @param[in] aType  The CoAP Type value.
     * @param[in] aCode  The CoAP Code value.
     * @param[in] aUri   The URI string.
     *
     * @retval kErrorNone    Successfully initialized the message and appended the URI-path option.
     * @retval kErrorNoBufs  Could not grow the message to append the option.
     */
    Error Init(Type aType, Code aCode, Uri aUri);

    /**
     * Initializes a CoAP POST message, appends a URI Path, and adds a random token.
     *
     * This method erases any previously written content in the message.
     *
     * The CoAP Type is determined from the destination IPv6 address: `kTypeNonConfirmable` for multicast and
     * `kTypeConfirmable` otherwise. The Message ID is set to zero. A random token of default length
     * (`Token::kDefaultLength`) is generated and added.
     *
     * @param[in] aDestination  The message destination IPv6 address, used to determine the CoAP Type.
     * @param[in] aUri          The URI string.
     *
     * @retval kErrorNone    Successfully initialized the message and appended the URI-path option.
     * @retval kErrorNoBufs  Could not grow the message to append the option.
     */
    Error InitAsPost(const Ip6::Address &aDestination, Uri aUri);

    /**
     * Initializes a CoAP message as a response to a request message.
     *
     * This method erases any previously written content in the message. The Message ID and Token are copied from the
     * request message.
     *
     * @param[in] aType     The CoAP Type value.
     * @param[in] aCode     The CoAP Code value.
     * @param[in] aRequest  The request message to respond to.
     *
     * @retval kErrorNone    Successfully initialized the message.
     * @retval kErrorNoBufs  Could not grow the message to write the CoAP header.
     */
    Error InitAsResponse(Type aType, Code aCode, const Message &aRequest);

    /**
     * Parses the CoAP header and token.
     *
     * @param[out] aInfo   A reference to `HeaderInfo` to populate.
     *
     * @retval kErrorNone    Successfully parsed the CoAP header. @p aInfo is updated.
     * @retval kErrorParse   Failed to parse.
     */
    Error ParseHeaderInfo(HeaderInfo &aInfo) const;

    /**
     * Reads the Type value.
     *
     * @returns The Type value, or zero if the CoAP header is invalid or cannot be parsed.
     */
    uint8_t ReadType(void) const;

    /**
     * Writes the Type value in CoAP header.
     *
     * This method requires that the message contains a valid CoAP header. Otherwise, no change is made.
     *
     * @param[in]  aType  The Type value.
     */
    void WriteType(Type aType);

    /**
     * Reads the Code value.
     *
     * @returns The Code value, or zero if the CoAP header is invalid or cannot be parsed.
     */
    uint8_t ReadCode(void) const;

    /**
     * Writes the Code value.
     *
     * This method requires that the message contains a valid CoAP header. Otherwise, no change is made.
     *
     * @param[in]  aCode  The Code value.
     */
    void WriteCode(Code aCode);

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
    /**
     * Returns the CoAP Code as human readable string.
     *
     * @ returns The CoAP Code as string.
     */
    const char *CodeToString(void) const;
#endif

    /**
     * Reads the Message ID value.
     *
     * @returns The Message ID value, or zero if the CoAP header is invalid or cannot be parsed.
     */
    uint16_t ReadMessageId(void) const;

    /**
     * Writes the Message ID value in CoAP header.
     *
     * This method requires that the message contains a valid CoAP header. Otherwise, no change is made.
     *
     * @param[in]  aMessageId  The Message ID value.
     */
    void WriteMessageId(uint16_t aMessageId);

    /**
     * Reads the Token length from the message
     *
     * @param[out] aLength   A reference to a `uint8_t` to return the read token length (in bytes).
     *
     * @retval kErrorNone   Successfully parsed the CoaP header and read the Token length.
     * @retval kErrorParse  Failed to parse the CoAP header.
     */
    Error ReadTokenLength(uint8_t &aLength) const;

    /**
     * Reads the Token from the message
     *
     * @param[out] aToken   A reference to return the read `Token`.
     *
     * @retval kErrorNone   Successfully parsed the CoaP header and read the Token. @p aToken is updated.
     * @retval kErrorParse  Failed to parse the CoAP header.
     */
    Error ReadToken(Token &aToken) const;

    /**
     * Writes the Token in the message.
     *
     * @param[in]  aToken    The new token.
     *
     * @retval kErrorNone    Successfully wrote the Token.
     * @retval kErrorNoBufs  Insufficient message buffers available to write.
     */
    Error WriteToken(const Token &aToken);

    /**
     * Writes the Token by copying it from another given message.
     *
     * @param[in] aMessage   The message to copy the Token from.
     *
     * @retval kErrorNone    Successfully wrote the Token.
     * @retval kErrorNoBufs  Insufficient message buffers available to write.
     */
    Error WriteTokenFromMessage(const Message &aMessage);

    /**
     * Writes a randomly generated Token of a given length in the message.
     *
     * @param[in]  aTokenLength  The Token length (in bytes).
     *
     * @retval kErrorNone    Successfully wrote the Token.
     * @retval kErrorNoBufs  Insufficient message buffers available to write.
     */
    Error WriteRandomToken(uint8_t aTokenLength);

    /**
     * Checks whether the Token in the message is the same as the one from another message.
     *
     * If parsing/reading the Token fails for either message, the Tokens are considered unequal.
     *
     * @param[in]  aMessage  The other message.
     *
     * @retval TRUE   If the two Tokens are equal.
     * @retval FALSE  If the two Tokens are not equal.
     */
    bool HasSameTokenAs(const Message &aMessage) const;

    /**
     * Appends a CoAP option.
     *
     * @param[in] aNumber   The CoAP Option number.
     * @param[in] aLength   The CoAP Option length.
     * @param[in] aValue    A pointer to the CoAP Option value (@p aLength bytes are used as Option value).
     *
     * @retval kErrorNone         Successfully appended the option.
     * @retval kErrorInvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kErrorNoBufs       The option length exceeds the buffer size.
     */
    Error AppendOption(uint16_t aNumber, uint16_t aLength, const void *aValue);

    /**
     * Appends a CoAP option reading Option value from another or potentially the same message.
     *
     * @param[in] aNumber   The CoAP Option number.
     * @param[in] aLength   The CoAP Option length.
     * @param[in] aMessage  The message to read the CoAP Option value from (it can be the same as the current message).
     * @param[in] aOffset   The offset in @p aMessage to start reading the CoAP Option value from (@p aLength bytes are
     *                      used as Option value).
     *
     * @retval kErrorNone         Successfully appended the option.
     * @retval kErrorInvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kErrorNoBufs       The option length exceeds the buffer size.
     * @retval kErrorParse        Not enough bytes in @p aMessage to read @p aLength bytes from @p aOffset.
     */
    Error AppendOptionFromMessage(uint16_t aNumber, uint16_t aLength, const Message &aMessage, uint16_t aOffset);

    /**
     * Appends an unsigned integer CoAP option as specified in RFC-7252 section-3.2
     *
     * @param[in]  aNumber  The CoAP Option number.
     * @param[in]  aValue   The CoAP Option unsigned integer value.
     *
     * @retval kErrorNone         Successfully appended the option.
     * @retval kErrorInvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kErrorNoBufs       The option length exceeds the buffer size.
     */
    Error AppendUintOption(uint16_t aNumber, uint32_t aValue);

    /**
     * Appends a string CoAP option.
     *
     * @param[in]  aNumber  The CoAP Option number.
     * @param[in]  aValue   The CoAP Option string value.
     *
     * @retval kErrorNone         Successfully appended the option.
     * @retval kErrorInvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kErrorNoBufs       The option length exceeds the buffer size.
     */
    Error AppendStringOption(uint16_t aNumber, const char *aValue);

    /**
     * Appends an Observe option.
     *
     * @param[in]  aObserve  Observe field value.
     *
     * @retval kErrorNone         Successfully appended the option.
     * @retval kErrorInvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kErrorNoBufs       The option length exceeds the buffer size.
     */
    Error AppendObserveOption(uint32_t aObserve) { return AppendUintOption(kOptionObserve, aObserve & kObserveMask); }

    /**
     * Appends a Uri-Path option.
     *
     * @param[in]  aUriPath           A pointer to a null-terminated string.
     *
     * @retval kErrorNone         Successfully appended the option.
     * @retval kErrorInvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kErrorNoBufs       The option length exceeds the buffer size.
     */
    Error AppendUriPathOptions(const char *aUriPath);

    /**
     * Reads the Uri-Path options and constructs the URI path in the buffer referenced by @p aUriPath.
     *
     * @param[out] aUriPath  A reference to the buffer to output the read URI path.
     *
     * @retval  kErrorNone   Successfully read the Uri-Path options.
     * @retval  kErrorParse  CoAP Option header not well-formed.
     */
    Error ReadUriPathOptions(UriPathStringBuffer &aUriPath) const;

    /**
     * Appends a Uri-Query option.
     *
     * @param[in]  aUriQuery          A pointer to a null-terminated string.
     *
     * @retval kErrorNone         Successfully appended the option.
     * @retval kErrorInvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kErrorNoBufs       The option length exceeds the buffer size.
     */
    Error AppendUriQueryOptions(const char *aUriQuery);

    /**
     * Appends a Block1 or Block2 option.
     *
     * @param[in]  aBlockOptionNumber  Block1 or Block2 option number.
     * @param[out] aInfo               A `BlockInfo` specifying block number, size, and more blocks flags.

     * @retval kErrorNone         Successfully appended the option.
     * @retval kErrorInvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kErrorNoBufs       The option length exceeds the buffer size.
     */
    Error AppendBlockOption(uint16_t aBlockOptionNumber, const BlockInfo &aInfo);

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    /**
     * Reads the information contained in a Block1 or Block2 option from the CoAP message.
     *
     * @param[in] aBlockOptionNumber  Block1 or Block2 option number.
     * @param[out] aInfo              A reference to `BlockInfo` to return the read Option
     *
     * @retval  kErrorNone          The option was read successfully. @p aInfo is updated.
     * @retval  kErrorNotFound      The option has not been found.
     * @retval  kErrorInvalidArgs   The option is invalid.
     */
    Error ReadBlockOptionValues(uint16_t aBlockOptionNumber, BlockInfo &aInfo) const;
#endif

    /**
     * Appends a Proxy-Uri option.
     *
     * @param[in]  aProxyUri          A pointer to a null-terminated string.
     *
     * @retval kErrorNone         Successfully appended the option.
     * @retval kErrorInvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kErrorNoBufs       The option length exceeds the buffer size.
     */
    Error AppendProxyUriOption(const char *aProxyUri) { return AppendStringOption(kOptionProxyUri, aProxyUri); }

    /**
     * Appends a Content-Format option.
     *
     * @param[in]  aContentFormat  The Content Format value.
     *
     * @retval kErrorNone         Successfully appended the option.
     * @retval kErrorInvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kErrorNoBufs       The option length exceeds the buffer size.
     */
    Error AppendContentFormatOption(otCoapOptionContentFormat aContentFormat)
    {
        return AppendUintOption(kOptionContentFormat, static_cast<uint32_t>(aContentFormat));
    }

    /**
     * Appends a Max-Age option.
     *
     * @param[in]  aMaxAge  The Max-Age value.
     *
     * @retval kErrorNone         Successfully appended the option.
     * @retval kErrorInvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kErrorNoBufs       The option length exceeds the buffer size.
     */
    Error AppendMaxAgeOption(uint32_t aMaxAge) { return AppendUintOption(kOptionMaxAge, aMaxAge); }

    /**
     * Appends a single Uri-Query option.
     *
     * @param[in]  aUriQuery  A pointer to null-terminated string, which should contain a single key=value pair.
     *
     * @retval kErrorNone         Successfully appended the option.
     * @retval kErrorInvalidArgs  The option type is not equal or greater than the last option type.
     * @retval kErrorNoBufs       The option length exceeds the buffer size.
     */
    Error AppendUriQueryOption(const char *aUriQuery) { return AppendStringOption(kOptionUriQuery, aUriQuery); }

    /**
     * Appends a Payload Marker indicating the beginning of the payload.
     *
     * It also sets the offset to the start of the payload.
     *
     * @retval kErrorNone    Payload Marker was successfully added.
     * @retval kErrorNoBufs  Could not grow the message to append the payload marker.
     */
    Error AppendPayloadMarker(void);

    /**
     * Creates a copy of this CoAP message.
     *
     * It allocates the new message from the same message pool as the original one and copies @p aLength octets
     * of the payload. The `Type`, `SubType`, `LinkSecurity`, `Offset`, `InterfaceId`, and `Priority` fields on the
     * cloned message are also copied from the original one.
     *
     * @param[in] aLength  Number of payload bytes to copy.
     *
     * @returns A pointer to the message or `nullptr` if insufficient message buffers are available.
     */
    Message *Clone(uint16_t aLength) const;

    /**
     * Creates a copy of the message.
     *
     * It allocates the new message from the same message pool as the original one and copies the entire payload. The
     * `Type`, `SubType`, `LinkSecurity`, `Offset`, `InterfaceId`, and `Priority` fields on the cloned message are also
     * copied from the original one.
     *
     * @returns A pointer to the message or `nullptr` if insufficient message buffers are available.
     */
    Message *Clone(void) const { return Clone(GetLength()); }

    /**
     * Returns a pointer to the next message after this as a `Coap::Message`.
     *
     * Should be used when the message is in a `Coap::MessageQueue` (i.e., a queue containing only CoAP
     * messages).
     *
     * @returns A pointer to the next message in the queue or `nullptr` if at the end of the queue.
     */
    Message *GetNextCoapMessage(void) { return static_cast<Message *>(GetNext()); }

    /**
     * Returns a pointer to the next message after this as a `Coap::Message`.
     *
     * Should be used when the message is in a `Coap::MessageQueue` (i.e., a queue containing only CoAP
     * messages).
     *
     * @returns A pointer to the next message in the queue or `nullptr` if at the end of the queue.
     */
    const Message *GetNextCoapMessage(void) const { return static_cast<const Message *>(GetNext()); }

private:
    /*
     *
     * Option Format (RFC 7252).
     *
     *      7   6   5   4   3   2   1   0
     *    +---------------+---------------+
     *    |  Option Delta | Option Length |   1 byte
     *    +---------------+---------------+
     *    /         Option Delta          /   0-2 bytes
     *    \          (extended)           \
     *    +-------------------------------+
     *    /         Option Length         /   0-2 bytes
     *    \          (extended)           \
     *    +-------------------------------+
     *    /         Option Value          /   0 or more bytes
     *    +-------------------------------+
     */

    static constexpr uint8_t kOptionDeltaOffset  = 4;
    static constexpr uint8_t kOptionDeltaMask    = 0xf << kOptionDeltaOffset;
    static constexpr uint8_t kOptionLengthOffset = 0;
    static constexpr uint8_t kOptionLengthMask   = 0xf << kOptionLengthOffset;

    static constexpr uint8_t kMaxOptionHeaderSize = 5;

    static constexpr uint8_t kOption1ByteExtension = 13; // Indicates a one-byte extension.
    static constexpr uint8_t kOption2ByteExtension = 14; // Indicates a two-byte extension.

    static constexpr uint8_t kPayloadMarker = 0xff;

    static constexpr uint16_t kOption1ByteExtensionOffset = 13;  // Delta/Length offset as specified (RFC 7252).
    static constexpr uint16_t kOption2ByteExtensionOffset = 269; // Delta/Length offset as specified (RFC 7252).

    static constexpr uint8_t kBlockSzxOffset = 0;
    static constexpr uint8_t kBlockMOffset   = 3;
    static constexpr uint8_t kBlockNumOffset = 4;

    static constexpr uint32_t kObserveMask = 0xffffff;
    static constexpr uint32_t kBlockNumMax = 0xffff;

    OT_TOOL_PACKED_BEGIN
    class Header : public Clearable<Header>
    {
    public:
        static constexpr uint8_t kVersion1 = 1;

        uint8_t  GetVersion(void) const { return ReadBits<uint8_t, kVersionMask>(mVersionTypeToken); }
        void     SetVersion(uint8_t aVersion) { WriteBits<uint8_t, kVersionMask>(mVersionTypeToken, aVersion); }
        uint8_t  GetType(void) const { return ReadBits<uint8_t, kTypeMask>(mVersionTypeToken); }
        void     SetType(Type aType) { WriteBits<uint8_t, kTypeMask>(mVersionTypeToken, aType); }
        uint8_t  GetCode(void) const { return mCode; }
        void     SetCode(Code aCode) { mCode = aCode; }
        uint16_t GetMessageId(void) const { return BigEndian::HostSwap16(mMessageId); }
        void     SetMessageId(uint16_t aMessageId) { mMessageId = BigEndian::HostSwap16(aMessageId); }
        uint8_t  GetTokenLength(void) const { return ReadBits<uint8_t, kTokenLengthMask>(mVersionTypeToken); }
        void     SetTokenLength(uint8_t aLength) { WriteBits<uint8_t, kTokenLengthMask>(mVersionTypeToken, aLength); }

    private:
        /*
         * Header field first byte (RFC 7252).
         *
         *    7 6 5 4 3 2 1 0
         *   +-+-+-+-+-+-+-+-+
         *   |Ver| T |  TKL  |  (Version, Type and Token Length).
         *   +-+-+-+-+-+-+-+-+
         */
        static constexpr uint8_t kVersionMask     = 0x3 << 6;
        static constexpr uint8_t kTypeMask        = 0x3 << 4;
        static constexpr uint8_t kTokenLengthMask = 0xf << 0;

        uint8_t  mVersionTypeToken; // The CoAP Version, Type, and Token Length
        uint8_t  mCode;
        uint16_t mMessageId;
    } OT_TOOL_PACKED_END;

    class ConstIterator : public ot::Message::ConstIterator
    {
    public:
        using ot::Message::ConstIterator::ConstIterator;

        const Message &operator*(void) { return static_cast<const Message &>(ot::Message::ConstIterator::operator*()); }
        const Message *operator->(void)
        {
            return static_cast<const Message *>(ot::Message::ConstIterator::operator->());
        }
    };

    class Iterator : public ot::Message::Iterator
    {
    public:
        using ot::Message::Iterator::Iterator;

        Message &operator*(void) { return static_cast<Message &>(ot::Message::Iterator::operator*()); }
        Message *operator->(void) { return static_cast<Message *>(ot::Message::Iterator::operator->()); }
    };

    uint16_t GetHeaderOffset(void) const { return GetMeshDest(); }
    void     SetHeaderOffset(uint16_t aOffset) { SetMeshDest(aOffset); }
    uint16_t DetermineTokenOffset(void) const;
    Error    DetermineOptionStartOffset(uint16_t &aOffset) const;
    Error    ReadHeader(Header &aHeader) const;
    void     WriteHeader(const Header &aHeader);
    Error    ReadToken(const Header &aHeader, Token &aToken) const;
    uint8_t  WriteExtendedOptionField(uint16_t aValue, uint8_t *&aBuffer);
    Error    AppendOptionHeader(uint16_t aNumber, uint16_t aLength);
};

/**
 * Implements a CoAP message queue.
 */
class MessageQueue : public ot::MessageQueue
{
public:
    /**
     * Initializes the message queue.
     */
    MessageQueue(void) = default;

    /**
     * Returns a pointer to the first message.
     *
     * @returns A pointer to the first message.
     */
    Message *GetHead(void) { return static_cast<Message *>(ot::MessageQueue::GetHead()); }

    /**
     * Returns a pointer to the first message.
     *
     * @returns A pointer to the first message.
     */
    const Message *GetHead(void) const { return static_cast<const Message *>(ot::MessageQueue::GetHead()); }

    /**
     * Adds a message to the end of the queue.
     *
     * @param[in]  aMessage  The message to add.
     */
    void Enqueue(Message &aMessage) { Enqueue(aMessage, kQueuePositionTail); }

    /**
     * Adds a message at a given position (head/tail) of the queue.
     *
     * @param[in]  aMessage  The message to add.
     * @param[in]  aPosition The position (head or tail) where to add the message.
     */
    void Enqueue(Message &aMessage, QueuePosition aPosition) { ot::MessageQueue::Enqueue(aMessage, aPosition); }

    /**
     * Removes a message from the queue.
     *
     * @param[in]  aMessage  The message to remove.
     */
    void Dequeue(Message &aMessage) { ot::MessageQueue::Dequeue(aMessage); }

    /**
     * Removes a message from the queue and frees it.
     *
     * @param[in]  aMessage  The message to remove and free.
     */
    void DequeueAndFree(Message &aMessage) { ot::MessageQueue::DequeueAndFree(aMessage); }

    // The following methods are intended to support range-based `for`
    // loop iteration over the queue entries and should not be used
    // directly. The range-based `for` works correctly even if the
    // current entry is removed from the queue during iteration.

    Message::Iterator begin(void);
    Message::Iterator end(void) { return Message::Iterator(); }

    Message::ConstIterator begin(void) const;
    Message::ConstIterator end(void) const { return Message::ConstIterator(); }
};

/**
 * Represents a CoAP option.
 */
class Option : public otCoapOption
{
public:
    /**
     * Represents an iterator for CoAP options.
     */
    class Iterator : public otCoapOptionIterator
    {
    public:
        /**
         * Initializes the iterator to iterate over CoAP Options in a CoAP message.
         *
         * The iterator MUST be initialized before any other methods are used, otherwise its behavior is undefined.
         *
         * After initialization, the iterator is either updated to point to the first option, or it is marked as done
         * (i.e., `IsDone()` returns `true`) when there is no option or if there is a parse error.
         *
         * @param[in] aMessage  The CoAP message.
         *
         * @retval kErrorNone   Successfully initialized. Iterator is either at the first option or done.
         * @retval kErrorParse  CoAP Option header in @p aMessage is not well-formed.
         */
        Error Init(const Message &aMessage);

        /**
         * Initializes the iterator to iterate over CoAP Options in a CoAP message matching a given Option
         * Number value.
         *
         * The iterator MUST be initialized before any other methods are used, otherwise its behavior is undefined.
         *
         * After initialization, the iterator is either updated to point to the first option matching the given Option
         * Number value, or it is marked as done (i.e., `IsDone()` returns `true`) when there is no matching option or
         * if there is a parse error.
         *
         * @param[in] aMessage  The CoAP message.
         * @param[in] aNumber   The CoAP Option Number.
         *
         * @retval  kErrorNone   Successfully initialized. Iterator is either at the first matching option or done.
         * @retval  kErrorParse  CoAP Option header in @p aMessage is not well-formed.
         */
        Error Init(const Message &aMessage, uint16_t aNumber) { return InitOrAdvance(&aMessage, aNumber); }

        /**
         * Indicates whether or not the iterator is done (i.e., has reached the end of CoAP Option Header).
         *
         * @retval TRUE   Iterator is done (reached end of Option header).
         * @retval FALSE  Iterator is not done and currently pointing to a CoAP Option.
         */
        bool IsDone(void) const { return mOption.mLength == kIteratorDoneLength; }

        /**
         * Advances the iterator to the next CoAP Option in the header.
         *
         * The iterator is updated to point to the next option or marked as done when there are no more options.
         *
         * @retval  kErrorNone   Successfully advanced the iterator.
         * @retval  kErrorParse  CoAP Option header is not well-formed.
         */
        Error Advance(void);

        /**
         * Advances the iterator to the next CoAP Option in the header matching a given Option Number value.
         *
         * The iterator is updated to point to the next matching option or marked as done when there are no more
         * matching options.
         *
         * @param[in] aNumber   The CoAP Option Number.
         *
         * @retval  kErrorNone   Successfully advanced the iterator.
         * @retval  kErrorParse  CoAP Option header is not well-formed.
         */
        Error Advance(uint16_t aNumber) { return InitOrAdvance(nullptr, aNumber); }

        /**
         * Gets the CoAP message associated with the iterator.
         *
         * @returns A reference to the CoAP message.
         */
        const Message &GetMessage(void) const { return *static_cast<const Message *>(mMessage); }

        /**
         * Gets a pointer to the current CoAP Option to which the iterator is currently pointing.
         *
         * @returns A pointer to the current CoAP Option, or `nullptr` if iterator is done (or there was an earlier
         *          parse error).
         */
        const Option *GetOption(void) const { return IsDone() ? nullptr : static_cast<const Option *>(&mOption); }

        /**
         * Reads the current Option Value into a given buffer.
         *
         * @param[out]  aValue   The pointer to a buffer to copy the Option Value. The buffer is assumed to be
         *                       sufficiently large (i.e. at least `GetOption()->GetLength()` bytes).
         *
         * @retval kErrorNone       Successfully read and copied the Option Value into given buffer.
         * @retval kErrorNotFound   Iterator is done (not pointing to any option).
         */
        Error ReadOptionValue(void *aValue) const;

        /**
         * Read the current Option Value which is assumed to be an unsigned integer.
         *
         * @param[out]  aUintValue      A reference to `uint64_t` to output the read Option Value.
         *
         * @retval kErrorNone       Successfully read the Option value.
         * @retval kErrorNoBufs     Value is too long to fit in an `uint64_t`.
         * @retval kErrorNotFound   Iterator is done (not pointing to any option).
         */
        Error ReadOptionValue(uint64_t &aUintValue) const;

        /**
         * Gets the offset of beginning of the CoAP message payload (after the CoAP header).
         *
         * MUST be used after the iterator is done (i.e. successfully iterated through all options).
         *
         * @returns The offset of beginning of the CoAP message payload
         */
        uint16_t GetPayloadMessageOffset(void) const { return mNextOptionOffset; }

        /**
         * Inidcated whether or not the option ended with a payload marker.
         *
         * MUST be used after the iterator is done (i.e. successfully iterated through all options).
         *
         * @retval TRUE  The message contains a payload marker.
         * @retval FALAE The message does not contain a payload marker.
         */
        bool HasPayloadMarker(void) const { return IsDone() && (mOption.mNumber != 0); }

        /**
         * Gets the offset of beginning of the CoAP Option Value.
         *
         * MUST be used during the iterator is in progress.
         *
         * @returns The offset of beginning of the CoAP Option Value
         */
        uint16_t GetOptionValueMessageOffset(void) const { return mNextOptionOffset - mOption.mLength; }

    private:
        // `mOption.mLength` value to indicate iterator is done.
        static constexpr uint16_t kIteratorDoneLength = 0xffff;

        void  MarkAsDone(void) { mOption.mLength = kIteratorDoneLength; }
        void  SetHasPayloadMarker(bool aHasPayloadMarker) { mOption.mNumber = aHasPayloadMarker; }
        Error Read(uint16_t aLength, void *aBuffer);
        Error ReadExtendedOptionField(uint16_t &aValue);
        Error InitOrAdvance(const Message *aMessage, uint16_t aNumber);
    };

    /**
     * Gets the CoAP Option Number.
     *
     * @returns The CoAP Option Number.
     */
    uint16_t GetNumber(void) const { return mNumber; }

    /**
     * Gets the CoAP Option Length (length of Option Value in bytes).
     *
     * @returns The CoAP Option Length (in bytes).
     */
    uint16_t GetLength(void) const { return mLength; }
};

/**
 * @}
 */

} // namespace Coap

DefineCoreType(otCoapOption, Coap::Option);
DefineCoreType(otCoapOptionIterator, Coap::Option::Iterator);
DefineCoreType(otCoapToken, Coap::Token);
DefineMapEnum(otCoapType, Coap::Type);
DefineMapEnum(otCoapCode, Coap::Code);
DefineMapEnum(otCoapBlockSzx, Coap::BlockSzx);

/**
 * Casts an `otMessage` pointer to a `Coap::Message` reference.
 *
 * @param[in] aMessage   A pointer to an `otMessage`.
 *
 * @returns A reference to `Coap::Message` matching @p aMessage.
 */
inline Coap::Message &AsCoapMessage(otMessage *aMessage) { return *static_cast<Coap::Message *>(aMessage); }

/**
 * Casts an `otMessage` pointer to a `Coap::Message` reference.
 *
 * @param[in] aMessage   A pointer to an `otMessage`.
 *
 * @returns A reference to `Coap::Message` matching @p aMessage.
 */
inline Coap::Message *AsCoapMessagePtr(otMessage *aMessage) { return static_cast<Coap::Message *>(aMessage); }

/**
 * Casts an `otMessage` pointer to a `Coap::Message` pointer.
 *
 * @param[in] aMessage   A pointer to an `otMessage`.
 *
 * @returns A pointer to `Coap::Message` matching @p aMessage.
 */
inline const Coap::Message &AsCoapMessage(const otMessage *aMessage)
{
    return *static_cast<const Coap::Message *>(aMessage);
}

/**
 * Casts an `otMessage` pointer to a `Coap::Message` reference.
 *
 * @param[in] aMessage   A pointer to an `otMessage`.
 *
 * @returns A pointer to `Coap::Message` matching @p aMessage.
 */
inline const Coap::Message *AsCoapMessagePtr(const otMessage *aMessage)
{
    return static_cast<const Coap::Message *>(aMessage);
}

} // namespace ot

#endif // OT_CORE_COAP_COAP_MESSAGE_HPP_
