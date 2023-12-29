/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file includes definitions for generating and processing DNS headers.
 */

#ifndef DNS_HEADER_HPP_
#define DNS_HEADER_HPP_

#include "openthread-core-config.h"

#include <openthread/dns.h>
#include <openthread/dns_client.h>

#include "common/appender.hpp"
#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/equatable.hpp"
#include "common/message.hpp"
#include "crypto/ecdsa.hpp"
#include "net/ip4_types.hpp"
#include "net/ip6_address.hpp"

namespace ot {

/**
 * @namespace ot::Dns
 * @brief
 *   This namespace includes definitions for DNS.
 *
 */
namespace Dns {

/**
 * @addtogroup core-dns
 *
 * @brief
 *   This module includes definitions for DNS.
 *
 * @{
 *
 */

/**
 * Implements DNS header generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Header : public Clearable<Header>
{
public:
    /**
     * Default constructor for DNS Header.
     *
     */
    Header(void) { Clear(); }

    /**
     * Returns the Message ID.
     *
     * @returns The Message ID value.
     *
     */
    uint16_t GetMessageId(void) const { return BigEndian::HostSwap16(mMessageId); }

    /**
     * Sets the Message ID.
     *
     * @param[in]  aMessageId The Message ID value.
     *
     */
    void SetMessageId(uint16_t aMessageId) { mMessageId = BigEndian::HostSwap16(aMessageId); }

    /**
     * Sets the Message ID to a crypto-secure randomly generated number.
     *
     * @retval  kErrorNone     Successfully generated random Message ID.
     * @retval  kErrorFailed   Could not generate random Message ID.
     *
     */
    Error SetRandomMessageId(void);

    /**
     * Defines types of DNS message.
     *
     */
    enum Type : uint8_t
    {
        kTypeQuery    = 0,
        kTypeResponse = 1,
    };

    /**
     * Returns the type of the message.
     *
     * @returns The type of the message.
     *
     */
    Type GetType(void) const { return static_cast<Type>((mFlags[0] & kQrFlagMask) >> kQrFlagOffset); }

    /**
     * Sets the type of the message.
     *
     * @param[in]  aType The type of the message.
     *
     */
    void SetType(Type aType)
    {
        mFlags[0] &= ~kQrFlagMask;
        mFlags[0] |= static_cast<uint8_t>(aType) << kQrFlagOffset;
    }

    /**
     * Defines types of query.
     *
     */
    enum QueryType : uint8_t
    {
        kQueryTypeStandard = 0,
        kQueryTypeInverse  = 1,
        kQueryTypeStatus   = 2,
        kQueryTypeNotify   = 4,
        kQueryTypeUpdate   = 5,
        kQueryTypeDso      = 6,
    };

    /**
     * Returns the type of the query.
     *
     * @returns The type of the query.
     *
     */
    QueryType GetQueryType(void) const { return static_cast<QueryType>((mFlags[0] & kOpCodeMask) >> kOpCodeOffset); }

    /**
     * Sets the type of the query.
     *
     * @param[in]  aType The type of the query.
     *
     */
    void SetQueryType(QueryType aType)
    {
        mFlags[0] &= ~kOpCodeMask;
        mFlags[0] |= static_cast<uint8_t>(aType) << kOpCodeOffset;
    }

    /**
     * Specifies in response message if the responding name server is an
     * authority for the domain name in question section.
     *
     * @returns True if Authoritative Answer flag (AA) is set in the header, false otherwise.
     *
     */
    bool IsAuthoritativeAnswerFlagSet(void) const { return (mFlags[0] & kAaFlagMask) == kAaFlagMask; }

    /**
     * Clears the Authoritative Answer flag (AA) in the header.
     *
     */
    void ClearAuthoritativeAnswerFlag(void) { mFlags[0] &= ~kAaFlagMask; }

    /**
     * Sets the Authoritative Answer flag (AA) in the header.
     *
     */
    void SetAuthoritativeAnswerFlag(void) { mFlags[0] |= kAaFlagMask; }

    /**
     * Specifies if message is truncated.
     *
     * @returns True if Truncation flag (TC) is set in the header, false otherwise.
     *
     */
    bool IsTruncationFlagSet(void) const { return (mFlags[0] & kTcFlagMask) == kTcFlagMask; }

    /**
     * Clears the Truncation flag (TC) in the header.
     *
     */
    void ClearTruncationFlag(void) { mFlags[0] &= ~kTcFlagMask; }

    /**
     * Sets the Truncation flag (TC) in the header.
     *
     */
    void SetTruncationFlag(void) { mFlags[0] |= kTcFlagMask; }

    /**
     * Specifies if resolver wants to direct the name server to pursue
     * the query recursively.
     *
     * @returns True if Recursion Desired flag (RD) is set in the header, false otherwise.
     *
     */
    bool IsRecursionDesiredFlagSet(void) const { return (mFlags[0] & kRdFlagMask) == kRdFlagMask; }

    /**
     * Clears the Recursion Desired flag (RD) in the header.
     *
     */
    void ClearRecursionDesiredFlag(void) { mFlags[0] &= ~kRdFlagMask; }

    /**
     * Sets the Recursion Desired flag (RD) in the header.
     *
     */
    void SetRecursionDesiredFlag(void) { mFlags[0] |= kRdFlagMask; }

    /**
     * Denotes whether recursive query support is available in the name server.
     *
     * @returns True if Recursion Available flag (RA) is set in the header, false otherwise.
     *
     */
    bool IsRecursionAvailableFlagSet(void) const { return (mFlags[1] & kRaFlagMask) == kRaFlagMask; }

    /**
     * Clears the Recursion Available flag (RA) in the header.
     *
     */
    void ClearRecursionAvailableFlag(void) { mFlags[1] &= ~kRaFlagMask; }

    /**
     * Sets the Recursion Available flag (RA) in the header.
     *
     */
    void SetRecursionAvailableFlag(void) { mFlags[1] |= kRaFlagMask; }

    /**
     * Defines response codes.
     *
     */
    enum Response : uint8_t
    {
        kResponseSuccess         = 0,  ///< Success (no error condition).
        kResponseFormatError     = 1,  ///< Server unable to interpret request due to format error.
        kResponseServerFailure   = 2,  ///< Server encountered an internal failure.
        kResponseNameError       = 3,  ///< Name that ought to exist, does not exists.
        kResponseNotImplemented  = 4,  ///< Server does not support the query type (OpCode).
        kResponseRefused         = 5,  ///< Server refused to perform operation for policy or security reasons.
        kResponseNameExists      = 6,  ///< Some name that ought not to exist, does exist.
        kResponseRecordExists    = 7,  ///< Some RRset that ought not to exist, does exist.
        kResponseRecordNotExists = 8,  ///< Some RRset that ought to exist, does not exist.
        kResponseNotAuth         = 9,  ///< Service is not authoritative for zone.
        kResponseNotZone         = 10, ///< A name is not in the zone.
        kDsoTypeNotImplemented   = 11, ///< DSO TLV TYPE is not implemented.
        kResponseBadName         = 20, ///< Bad name.
        kResponseBadAlg          = 21, ///< Bad algorithm.
        kResponseBadTruncation   = 22, ///< Bad truncation.
    };

    /**
     * Returns the response code.
     *
     * @returns The response code from the header.
     *
     */
    Response GetResponseCode(void) const { return static_cast<Response>((mFlags[1] & kRCodeMask) >> kRCodeOffset); }

    /**
     * Sets the response code.
     *
     * @param[in]  aResponse The type of the response.
     *
     */
    void SetResponseCode(Response aResponse)
    {
        mFlags[1] &= ~kRCodeMask;
        mFlags[1] |= static_cast<uint8_t>(aResponse) << kRCodeOffset;
    }

    /**
     * Converts a Response Code into a related `Error`.
     *
     * - kResponseSuccess (0)         : Success (no error condition)                    -> kErrorNone
     * - kResponseFormatError (1)     : Server unable to interpret due to format error  -> kErrorParse
     * - kResponseServerFailure (2)   : Server encountered an internal failure          -> kErrorFailed
     * - kResponseNameError (3)       : Name that ought to exist, does not exists       -> kErrorNotFound
     * - kResponseNotImplemented (4)  : Server does not support the query type (OpCode) -> kErrorNotImplemented
     * - kResponseRefused (5)         : Server refused for policy/security reasons      -> kErrorSecurity
     * - kResponseNameExists (6)      : Some name that ought not to exist, does exist   -> kErrorDuplicated
     * - kResponseRecordExists (7)    : Some RRset that ought not to exist, does exist  -> kErrorDuplicated
     * - kResponseRecordNotExists (8) : Some RRset that ought to exist, does not exist  -> kErrorNotFound
     * - kResponseNotAuth (9)         : Service is not authoritative for zone           -> kErrorSecurity
     * - kResponseNotZone (10)        : A name is not in the zone                       -> kErrorParse
     * - kDsoTypeNotImplemented (11)  : DSO TLV Type is not implemented                 -> kErrorNotImplemented
     * - kResponseBadName (20)        : Bad name                                        -> kErrorParse
     * - kResponseBadAlg (21)         : Bad algorithm                                   -> kErrorSecurity
     * - kResponseBadTruncation (22)  : Bad truncation                                  -> kErrorParse
     * - Other error                                                                    -> kErrorFailed
     *
     * @param[in] aResponse  The response code to convert.
     *
     */
    static Error ResponseCodeToError(Response aResponse);

    /**
     * Returns the number of entries in question section.
     *
     * @returns The number of entries in question section.
     *
     */
    uint16_t GetQuestionCount(void) const { return BigEndian::HostSwap16(mQdCount); }

    /**
     * Sets the number of entries in question section.
     *
     * @param[in]  aCount The number of entries in question section.
     *
     */
    void SetQuestionCount(uint16_t aCount) { mQdCount = BigEndian::HostSwap16(aCount); }

    /**
     * Returns the number of entries in answer section.
     *
     * @returns The number of entries in answer section.
     *
     */
    uint16_t GetAnswerCount(void) const { return BigEndian::HostSwap16(mAnCount); }

    /**
     * Sets the number of entries in answer section.
     *
     * @param[in]  aCount The number of entries in answer section.
     *
     */
    void SetAnswerCount(uint16_t aCount) { mAnCount = BigEndian::HostSwap16(aCount); }

    /**
     * Returns the number of entries in authority records section.
     *
     * @returns The number of entries in authority records section.
     *
     */
    uint16_t GetAuthorityRecordCount(void) const { return BigEndian::HostSwap16(mNsCount); }

    /**
     * Sets the number of entries in authority records section.
     *
     * @param[in]  aCount The number of entries in authority records section.
     *
     */
    void SetAuthorityRecordCount(uint16_t aCount) { mNsCount = BigEndian::HostSwap16(aCount); }

    /**
     * Returns the number of entries in additional records section.
     *
     * @returns The number of entries in additional records section.
     *
     */
    uint16_t GetAdditionalRecordCount(void) const { return BigEndian::HostSwap16(mArCount); }

    /**
     * Sets the number of entries in additional records section.
     *
     * @param[in]  aCount The number of entries in additional records section.
     *
     */
    void SetAdditionalRecordCount(uint16_t aCount) { mArCount = BigEndian::HostSwap16(aCount); }

private:
    // Protocol Constants (RFC 1035).
    static constexpr uint8_t kQrFlagOffset = 7;                     // QR Flag offset.
    static constexpr uint8_t kQrFlagMask   = 0x01 << kQrFlagOffset; // QR Flag mask.
    static constexpr uint8_t kOpCodeOffset = 3;                     // OpCode field offset.
    static constexpr uint8_t kOpCodeMask   = 0x0f << kOpCodeOffset; // OpCode field mask.
    static constexpr uint8_t kAaFlagOffset = 2;                     // AA Flag offset.
    static constexpr uint8_t kAaFlagMask   = 0x01 << kAaFlagOffset; // AA Flag mask.
    static constexpr uint8_t kTcFlagOffset = 1;                     // TC Flag offset.
    static constexpr uint8_t kTcFlagMask   = 0x01 << kTcFlagOffset; // TC Flag mask.
    static constexpr uint8_t kRdFlagOffset = 0;                     // RD Flag offset.
    static constexpr uint8_t kRdFlagMask   = 0x01 << kRdFlagOffset; // RD Flag mask.
    static constexpr uint8_t kRaFlagOffset = 7;                     // RA Flag offset.
    static constexpr uint8_t kRaFlagMask   = 0x01 << kRaFlagOffset; // RA Flag mask.
    static constexpr uint8_t kRCodeOffset  = 0;                     // RCODE field offset.
    static constexpr uint8_t kRCodeMask    = 0x0f << kRCodeOffset;  // RCODE field mask.

    uint16_t mMessageId; // Message identifier for requester to match up replies to outstanding queries.
    uint8_t  mFlags[2];  // DNS header flags.
    uint16_t mQdCount;   // Number of entries in the question section.
    uint16_t mAnCount;   // Number of entries in the answer section.
    uint16_t mNsCount;   // Number of entries in the authority records section.
    uint16_t mArCount;   // Number of entries in the additional records section.

} OT_TOOL_PACKED_END;

/**
 * Implements DNS Update message header generation and parsing.
 *
 * The DNS header specifies record counts for its four sections: Question, Answer, Authority, and Additional. A DNS
 * Update header uses the same fields, and the same section formats, but the naming and use of these sections differs:
 * DNS Update header uses Zone, Prerequisite, Update, Additional Data sections.
 *
 */
OT_TOOL_PACKED_BEGIN
class UpdateHeader : public Header
{
public:
    /**
     * Default constructor for DNS Update message header.
     *
     */
    UpdateHeader(void) { SetQueryType(kQueryTypeUpdate); }

    /**
     * Returns the number of records in Zone section.
     *
     * @returns The number of records in Zone section.
     *
     */
    uint16_t GetZoneRecordCount(void) const { return GetQuestionCount(); }

    /**
     * Sets the number of records in Zone section.
     *
     * @param[in]  aCount The number of records in Zone section.
     *
     */
    void SetZoneRecordCount(uint16_t aCount) { SetQuestionCount(aCount); }

    /**
     * Returns the number of records in Prerequisite section.
     *
     * @returns The number of records in Prerequisite section.
     *
     */
    uint16_t GetPrerequisiteRecordCount(void) const { return GetAnswerCount(); }

    /**
     * Sets the number of records in Prerequisite section.
     *
     * @param[in]  aCount The number of records in Prerequisite section.
     *
     */
    void SetPrerequisiteRecordCount(uint16_t aCount) { SetAnswerCount(aCount); }

    /**
     * Returns the number of records in Update section.
     *
     * @returns The number of records in Update section.
     *
     */
    uint16_t GetUpdateRecordCount(void) const { return GetAuthorityRecordCount(); }

    /**
     * Sets the number of records in Update section.
     *
     * @param[in]  aCount The number of records in Update section.
     *
     */
    void SetUpdateRecordCount(uint16_t aCount) { SetAuthorityRecordCount(aCount); }

} OT_TOOL_PACKED_END;

/**
 * Represents a DNS name and implements helper methods for encoding/decoding of DNS Names.
 *
 */
class Name : public Clearable<Name>
{
public:
    /**
     * Max size (number of chars) in a name string array (includes null char at the end of string).
     *
     */
    static constexpr uint8_t kMaxNameSize = OT_DNS_MAX_NAME_SIZE;

    /**
     * Maximum length in a name string (does not include null char at the end of string).
     *
     */
    static constexpr uint8_t kMaxNameLength = kMaxNameSize - 1;

    /**
     * Max size (number of chars) in a label string array (includes null char at the end of the string).
     *
     */
    static constexpr uint8_t kMaxLabelSize = OT_DNS_MAX_LABEL_SIZE;

    /**
     * Maximum length in a label string (does not include null char at the end of string).
     *
     */
    static constexpr uint8_t kMaxLabelLength = kMaxLabelSize - 1;

    /**
     * Dot character separating labels in a name.
     *
     */
    static constexpr char kLabelSeparatorChar = '.';

    /**
     * Represents a string buffer (with `kMaxNameSize`) intended to hold a DNS name.
     *
     */
    typedef char Buffer[kMaxNameSize];

    /**
     * Represents a string buffer (with `kMaxLabelSize`) intended to hold a DNS label.
     *
     */
    typedef char LabelBuffer[kMaxLabelSize];

    /**
     * Represents the name type.
     *
     */
    enum Type : uint8_t
    {
        kTypeEmpty,   ///< The name is empty (not specified).
        kTypeCString, ///< The name is given as a C string (dot '.' separated sequence of labels).
        kTypeMessage, ///< The name is specified from a message at a given offset (encoded in the message).
    };

    /**
     * Initializes the `Name` object as empty (not specified).
     *
     */
    Name(void)
        : Name(nullptr, nullptr, 0)
    {
    }

    /**
     * Initializes the `Name` object with a given string.
     *
     * @param[in] aString   A C string specifying the name (dot '.' separated sequence of labels').
     *
     */
    explicit Name(const char *aString)
        : Name(aString, nullptr, 0)
    {
    }

    /**
     * Initializes the `Name` object from a message at a given offset.
     *
     * @param[in] aMessage   The message containing the encoded name. `aMessage.GetOffset()` MUST point to the start of
     *                       the DNS header in the message (used to parse compressed name).
     * @param[in] aOffset    The offset in @p aMessage pointing to the start of the name.
     *
     */
    Name(const Message &aMessage, uint16_t aOffset)
        : Name(nullptr, &aMessage, aOffset)
    {
    }

    /**
     * Indicates whether the name is empty (not specified).
     *
     * @returns TRUE if the name is empty, FALSE otherwise.
     *
     */
    bool IsEmpty(void) const { return (mString == nullptr) && (mMessage == nullptr); }

    /**
     * Indicates whether the name is specified from a C string.
     *
     * @returns TRUE if the name is specified from a string, FALSE otherwise.
     *
     */
    bool IsFromCString(void) const { return mString != nullptr; }

    /**
     * Indicates whether the name is specified from a message.
     *
     * @returns TRUE if the name is specified from a message, FALSE otherwise.
     *
     */
    bool IsFromMessage(void) const { return mMessage != nullptr; }

    /**
     * Gets the type of `Name` object indicating whether it is empty, specified by a C string or from a
     * message
     *
     * @returns The name type.
     *
     */
    Type GetFromType(void) const
    {
        return IsFromCString() ? kTypeCString : (IsFromMessage() ? kTypeMessage : kTypeEmpty);
    }

    /**
     * Sets the name from a given C string.
     *
     * @param[in] aString   A C string specifying the name (dot '.' separated sequence of labels).
     *
     */
    void Set(const char *aString)
    {
        mString  = aString;
        mMessage = nullptr;
    }

    /**
     * Sets the name from a message at a given offset.
     *
     * @param[in] aMessage   The message containing the encoded name. `aMessage.GetOffset()` MUST point to the start of
     *                       the DNS header in the message (used to parse compressed name).
     * @param[in] aOffset    The offset in @p aMessage pointing to the start of the name.
     *
     */
    void SetFromMessage(const Message &aMessage, uint16_t aOffset)
    {
        mString  = nullptr;
        mMessage = &aMessage;
        mOffset  = aOffset;
    }

    /**
     * Gets the name as a C string.
     *
     * MUST be used only when the type is `kTypeString`. Otherwise its behavior is undefined.
     *
     * @returns A pointer to the C string.
     *
     */
    const char *GetAsCString(void) const { return mString; }

    /**
     * Gets the name message and offset.
     *
     * MUST be used only when the type is `kTypeMessage`. Otherwise its behavior is undefined.
     *
     * @param[out]  aOffset    A reference to a variable to output the offset of the start of the name in the message.
     *
     * @returns A reference to the message containing the name.
     *
     */
    const Message &GetAsMessage(uint16_t &aOffset) const
    {
        aOffset = mOffset;
        return *mMessage;
    }

    /**
     * Matches the `Name` with a given set of labels and domain name.
     *
     * This method allows the caller to specify name components separately, enabling scenarios like comparing "service
     * instance name" with separate instance label (which can include dot character), service type, and domain strings.
     *
     * @p aFirstLabel can be `nullptr` if not needed. But if non-null, it is treated as a single label and can itself
     * include dot `.` character.
     *
     * The @p aLabels MUST NOT be `nullptr` and MUST follow  "<label1>.<label2>.<label3>", i.e., a sequence of one or
     * more labels separated by dot '.' char, and it MUST NOT end with dot `.`.
     *
     * @p aDomain MUST NOT be `nullptr` and MUST have at least one label and MUST always end with a dot `.` character.
     *
     * If the above conditions are not satisfied, the behavior of this method is undefined.
     *
     * @param[in] aFirstLabel     A first label to check. Can be `nullptr`.
     * @param[in] aLabels         A string of dot separated labels, MUST NOT end with dot.
     * @param[in] aDomain         Domain name. MUST end with dot.
     *
     * @retval TRUE   The name matches the given components.
     * @retval FALSE  The name does not match the given components.
     *
     */
    bool Matches(const char *aFirstLabel, const char *aLabels, const char *aDomain) const;

    /**
     * Encodes and appends the name to a message.
     *
     * If the name is empty (not specified), then root "." is appended to @p aMessage. If the name is from a C string
     * then the string is checked and appended (similar to static `AppendName(const char *aName, Message &)` method).
     * If the the name is from a message, then it is read from the message and appended to @p aMessage. Note that in
     * this case independent of whether the name is compressed or not in its original message, the name is appended
     * as full (uncompressed) in @p aMessage.
     *
     * @param[in] aMessage        The message to append to.
     *
     * @retval kErrorNone         Successfully encoded and appended the name to @p aMessage.
     * @retval kErrorInvalidArgs  Name is not valid.
     * @retval kErrorNoBufs       Insufficient available buffers to grow the message.
     *
     */
    Error AppendTo(Message &aMessage) const;

    /**
     * Encodes and appends a single name label to a message.
     *
     * The @p aLabel is assumed to contain a single name label as a C string (null-terminated). Unlike
     * `AppendMultipleLabels()` which parses the label string and treats it as sequence of multiple (dot-separated)
     * labels, this method always appends @p aLabel as a single whole label. This allows the label string to even
     * contain dot '.' character, which, for example, is useful for "Service Instance Names" where <Instance> portion
     * is a user-friendly name and can contain dot characters.
     *
     * @param[in] aLabel              The label string to append. MUST NOT be `nullptr`.
     * @param[in] aMessage            The message to append to.
     *
     * @retval kErrorNone         Successfully encoded and appended the name label to @p aMessage.
     * @retval kErrorInvalidArgs  @p aLabel is not valid (e.g., label length is not within valid range).
     * @retval kErrorNoBufs       Insufficient available buffers to grow the message.
     *
     */
    static Error AppendLabel(const char *aLabel, Message &aMessage);

    /**
     * Encodes and appends a sequence of name labels to a given message.
     *
     * The @p aLabels must follow  "<label1>.<label2>.<label3>", i.e., a sequence of labels separated by dot '.' char.
     * E.g., "_http._tcp", "_http._tcp." (same as previous one), "host-1.test".
     *
     * Validates that the @p aLabels is a valid name format, i.e., no empty label, and labels are
     * `kMaxLabelLength` (63) characters or less.
     *
     * @note This method NEVER adds a label terminator (empty label) to the message, even in the case where @p aLabels
     * ends with a dot character, e.g., "host-1.test." is treated same as "host-1.test".
     *
     * @param[in]  aLabels            A name label string. Can be `nullptr` (then treated as "").
     * @param[in]  aMessage           The message to which to append the encoded name.
     *
     * @retval kErrorNone         Successfully encoded and appended the name label(s) to @p aMessage.
     * @retval kErrorInvalidArgs  Name label @p aLabels is not valid.
     * @retval kErrorNoBufs       Insufficient available buffers to grow the message.
     *
     */
    static Error AppendMultipleLabels(const char *aLabels, Message &aMessage);

    /**
     * Appends a name label terminator to a message.
     *
     * An encoded name is terminated by an empty label (a zero byte).
     *
     * @param[in] aMessage            The message to append to.
     *
     * @retval kErrorNone         Successfully encoded and appended the terminator label to @p aMessage.
     * @retval kErrorNoBufs       Insufficient available buffers to grow the message.
     *
     */
    static Error AppendTerminator(Message &aMessage);

    /**
     * Appends a pointer type name label to a message.
     *
     * Pointer label is used for name compression. It allows an entire name or a list of labels at the end of an
     * encoded name to be replaced with a pointer to a prior occurrence of the same name within the message.
     *
     * @param[in] aOffset             The offset from the start of DNS header to use for pointer value.
     * @param[in] aMessage            The message to append to.
     *
     * @retval kErrorNone         Successfully encoded and appended the pointer label to @p aMessage.
     * @retval kErrorNoBufs       Insufficient available buffers to grow the message.
     *
     */
    static Error AppendPointerLabel(uint16_t aOffset, Message &aMessage);

    /**
     * Encodes and appends a full name to a message.
     *
     * The @p aName must follow  "<label1>.<label2>.<label3>", i.e., a sequence of labels separated by dot '.' char.
     * E.g., "example.com", "example.com." (same as previous one), "local.", "default.service.arpa", "." or "" (root).
     *
     * Validates that the @p aName is a valid name format, i.e. no empty labels, and labels are
     * `kMaxLabelLength` (63) characters or less, and the name is `kMaxLength` (255) characters or less.
     *
     * @param[in]  aName              A name string. Can be `nullptr` (then treated as "." or root).
     * @param[in]  aMessage           The message to append to.
     *
     * @retval kErrorNone         Successfully encoded and appended the name to @p aMessage.
     * @retval kErrorInvalidArgs  Name @p aName is not valid.
     * @retval kErrorNoBufs       Insufficient available buffers to grow the message.
     *
     */
    static Error AppendName(const char *aName, Message &aMessage);

    /**
     * Parses and skips over a full name in a message.
     *
     * @param[in]     aMessage        The message to parse the name from. `aMessage.GetOffset()` MUST point to
     *                                the start of DNS header (this is used to handle compressed names).
     * @param[in,out] aOffset         On input the offset in @p aMessage pointing to the start of the name field.
     *                                On exit (when parsed successfully), @p aOffset is updated to point to the byte
     *                                after the end of name field.
     *
     * @retval kErrorNone          Successfully parsed and skipped over name, @p Offset is updated.
     * @retval kErrorParse         Name could not be parsed (invalid format).
     *
     */
    static Error ParseName(const Message &aMessage, uint16_t &aOffset);

    /**
     * Reads a name label from a message.
     *
     * Can be used to read labels one by one in a name. After a successful label read, @p aOffset is
     * updated to point to the start of the next label. When we reach the end of the name, kErrorNotFound is
     * returned. This method handles compressed names which use pointer labels. So as the labels in a name are read,
     * the @p aOffset may jump back in the message and at the end the @p aOffset does not necessarily point to the end
     * of the original name field.
     *
     * Unlike `ReadName()` which requires and verifies that the read label to contain no dot '.' character, this method
     * allows the read label to include any character.
     *
     * @param[in]      aMessage       The message to read the label from. `aMessage.GetOffset()` MUST point to
     *                                the start of DNS header (this is used to handle compressed names).
     * @param[in,out]  aOffset        On input, the offset in @p aMessage pointing to the start of the label to read.
     *                                On exit, when successfully read, @p aOffset is updated to point to the start of
     *                                the next label.
     * @param[out]     aLabelBuffer   A pointer to a char array to output the read label as a null-terminated C string.
     * @param[in,out]  aLabelLength   On input, the maximum number chars in @p aLabelBuffer array.
     *                                On output, when label is successfully read, @p aLabelLength is updated to return
     *                                the label's length (number of chars in the label string, excluding the null char).
     *
     * @retval kErrorNone      Successfully read the label and updated @p aLabelBuffer, @p aLabelLength, and @p aOffset.
     * @retval kErrorNotFound  Reached the end of name and no more label to read.
     * @retval kErrorParse     Name could not be parsed (invalid format).
     * @retval kErrorNoBufs    Label could not fit in @p aLabelLength chars.
     *
     */
    static Error ReadLabel(const Message &aMessage, uint16_t &aOffset, char *aLabelBuffer, uint8_t &aLabelLength);

    /**
     * Reads a full name from a message.
     *
     * On successful read, the read name follows  "<label1>.<label2>.<label3>.", i.e., a sequence of labels separated by
     * dot '.' character. The read name will ALWAYS end with a dot.
     *
     * Verifies that the labels after the first label in message do not contain any dot character. If they do,
     * returns `kErrorParse`.
     *
     * @param[in]     aMessage         The message to read the name from. `aMessage.GetOffset()` MUST point to
     *                                 the start of DNS header (this is used to handle compressed names).
     * @param[in,out] aOffset          On input, the offset in @p aMessage pointing to the start of the name field.
     *                                 On exit (when parsed successfully), @p aOffset is updated to point to the byte
     *                                 after the end of name field.
     * @param[out]    aNameBuffer      A pointer to a char array to output the read name as a null-terminated C string.
     * @param[in,out] aNameBufferSize  The maximum number of chars in @p aNameBuffer array.
     *
     * @retval kErrorNone         Successfully read the name, @p aNameBuffer and @p Offset are updated.
     * @retval kErrorParse        Name could not be parsed (invalid format).
     * @retval kErrorNoBufs       Name could not fit in @p aNameBufferSize chars.
     *
     */
    static Error ReadName(const Message &aMessage, uint16_t &aOffset, char *aNameBuffer, uint16_t aNameBufferSize);

    /**
     * Reads a full name from a message.
     *
     * On successful read, the read name follows  "<label1>.<label2>.<label3>.", i.e., a sequence of labels separated by
     * dot '.' character. The read name will ALWAYS end with a dot.
     *
     * Verifies that the labels after the first label in message do not contain any dot character. If they do,
     * returns `kErrorParse`.
     *
     * @tparam kNameBufferSize         Size of the string buffer array.
     *
     * @param[in]     aMessage         The message to read the name from. `aMessage.GetOffset()` MUST point to
     *                                 the start of DNS header (this is used to handle compressed names).
     * @param[in,out] aOffset          On input, the offset in @p aMessage pointing to the start of the name field.
     *                                 On exit (when parsed successfully), @p aOffset is updated to point to the byte
     *                                 after the end of name field.
     * @param[out]    aNameBuffer      Reference to a name string buffer to output the read name.
     *
     * @retval kErrorNone         Successfully read the name, @p aNameBuffer and @p Offset are updated.
     * @retval kErrorParse        Name could not be parsed (invalid format).
     * @retval kErrorNoBufs       Name could not fit in @p aNameBuffer.
     *
     */
    template <uint16_t kNameBufferSize>
    static Error ReadName(const Message &aMessage, uint16_t &aOffset, char (&aNameBuffer)[kNameBufferSize])
    {
        return ReadName(aMessage, aOffset, aNameBuffer, kNameBufferSize);
    }

    /**
     * Compares a single name label from a message with a given label string.
     *
     * Can be used to compare labels one by one. It checks whether the label read from @p aMessage matches
     * @p aLabel string (case-insensitive comparison).
     *
     * Unlike `CompareName()` which requires the labels in the the name string to contain no dot '.' character, this
     * method allows @p aLabel to include any character.
     *
     * @param[in]     aMessage        The message to read the label from to compare. `aMessage.GetOffset()` MUST point
     *                                to the start of DNS header (this is used to handle compressed names).
     * @param[in,out] aOffset         On input, the offset in @p aMessage pointing to the start of the label to read.
     *                                On exit and only when label is successfully read and does match @p aLabel,
     *                                @p aOffset is updated to point to the start of the next label.
     * @param[in]     aLabel          A pointer to a null terminated string containing the label to compare with.
     *
     * @retval kErrorNone          The label from @p aMessage matches @p aLabel. @p aOffset is updated.
     * @retval kErrorNotFound      The label from @p aMessage does not match @p aLabel (note that @p aOffset is not
     *                             updated in this case).
     * @retval kErrorParse         Name could not be parsed (invalid format).
     *
     */
    static Error CompareLabel(const Message &aMessage, uint16_t &aOffset, const char *aLabel);

    /**
     * Parses and compares multiple name labels from a message.
     *
     * Can be used to read and compare a group of labels from an encoded DNS name in a message with possibly more
     * labels remaining to read.
     *
     * The @p aLabels must follow  "<label1>.<label2>.<label3>", i.e., a sequence of labels separated by dot '.' char.
     *
     * @param[in]     aMessage        The message to read the labels from to compare. `aMessage.GetOffset()` MUST point
     *                                to the start of DNS header (this is used to handle compressed names).
     * @param[in,out] aOffset         On input, the offset in @p aMessage pointing to the start of the labels to read.
     *                                On exit and only when all labels are successfully read and match @p aLabels,
     *                                @p aOffset is updated to point to the start of the next label.
     * @param[in]     aLabels         A pointer to a null terminated string containing the labels to compare with.
     *
     * @retval kErrorNone          The labels from @p aMessage matches @p aLabels. @p aOffset is updated.
     * @retval kErrorNotFound      The labels from @p aMessage does not match @p aLabel (note that @p aOffset is not
     *                             updated in this case).
     * @retval kErrorParse         Name could not be parsed (invalid format).
     *
     */
    static Error CompareMultipleLabels(const Message &aMessage, uint16_t &aOffset, const char *aLabels);

    /**
     * Parses and compares a full name from a message with a given name.
     *
     * Checks whether the encoded name in a message matches a given name string (using case-insensitive
     * comparison). It checks the name in the message in place and handles compressed names. If the name read from the
     * message does not match @p aName, it returns `kErrorNotFound`. `kErrorNone` indicates that the name matches
     * @p aName.
     *
     * The @p aName must follow  "<label1>.<label2>.<label3>", i.e., a sequence of labels separated by dot '.' char.
     * E.g., "example.com", "example.com." (same as previous one), "local.", "default.service.arpa", "." or "" (root).
     *
     * @param[in]     aMessage        The message to read the name from and compare with @p aName.
     *                                `aMessage.GetOffset()` MUST point to the start of DNS header (this is used to
     *                                handle compressed names).
     * @param[in,out] aOffset         On input, the offset in @p aMessage pointing to the start of the name field.
     *                                On exit (when parsed successfully independent of whether the read name matches
     *                                @p aName or not), @p aOffset is updated to point to the byte after the end of
     *                                the name field.
     * @param[in]     aName           A pointer to a null terminated string containing the name to compare with.
     *
     * @retval kErrorNone          The name from @p aMessage matches @p aName. @p aOffset is updated.
     * @retval kErrorNotFound      The name from @p aMessage does not match @p aName. @p aOffset is updated.
     * @retval kErrorParse         Name could not be parsed (invalid format).
     * @retval kErrorInvalidArgs   The @p aName is not a valid name (e.g. back to back "." chars)
     *
     */
    static Error CompareName(const Message &aMessage, uint16_t &aOffset, const char *aName);

    /**
     * Parses and compares a full name from a message with a name from another message.
     *
     * Checks whether the encoded name in @p aMessage matches the name from @p aMessage2 (using
     * case-insensitive comparison). It compares the names in both messages in place and handles compressed names. Note
     * that this method works correctly even when the same message instance is used for both @p aMessage and
     * @p aMessage2 (e.g., at different offsets).
     *
     * Only the name in @p aMessage is fully parsed and checked for parse errors. This method assumes that the name in
     * @p aMessage2 was previously parsed and validated before calling this method (if there is a parse error in
     * @p aMessage2, it is treated as a name mismatch with @p aMessage).
     *
     * If the name in @p aMessage can be parsed fully (independent of whether the name matches or not with the name
     * from @p aMessage2), the @p aOffset is updated (note that @p aOffset2 for @p aMessage2 is not changed).
     *
     * @param[in]     aMessage        The message to read the name from and compare. `aMessage.GetOffset()` MUST point
     *                                to the start of DNS header (this is used to handle compressed names).
     * @param[in,out] aOffset         On input, the offset in @p aMessage pointing to the start of the name field.
     *                                On exit (when parsed successfully independent of whether the read name matches
     *                                or not), @p aOffset is updated to point to the byte after the end of the name
     *                                field.
     * @param[in]     aMessage2       The second message to read the name from and compare with name from @p aMessage.
     *                                `aMessage2.GetOffset()` MUST point to the start of DNS header.
     * @param[in]     aOffset2        The offset in @p aMessage2 pointing to the start of the name field.
     *
     * @retval kErrorNone       The name from @p aMessage matches the name from @p aMessage2. @p aOffset is updated.
     * @retval kErrorNotFound   The name from @p aMessage does not match the name from @p aMessage2. @p aOffset is
     *                          updated.
     * @retval kErrorParse      Name in @p aMessage could not be parsed (invalid format).
     *
     */
    static Error CompareName(const Message &aMessage, uint16_t &aOffset, const Message &aMessage2, uint16_t aOffset2);

    /**
     * Parses and compares a full name from a message with a given name (using case-insensitive
     * comparison).
     *
     * If @p aName is empty (not specified), then any name in @p aMessage is considered a match to it.
     *
     * @param[in]     aMessage        The message to read the name from and compare. `aMessage.GetOffset()` MUST point
     *                                to the start of DNS header (this is used to handle compressed names).
     * @param[in,out] aOffset         On input, the offset in @p aMessage pointing to the start of the name field.
     *                                On exit (when parsed successfully independent of whether the read name matches
     *                                or not), @p aOffset is updated to point to the byte after the end of the name
     *                                field.
     * @param[in]     aName           A reference to a name to compare with.
     *
     * @retval kErrorNone          The name from @p aMessage matches @p aName. @p aOffset is updated.
     * @retval kErrorNotFound      The name from @p aMessage does not match @p aName. @p aOffset is updated.
     * @retval kErrorParse         Name in @p aMessage could not be parsed (invalid format).
     *
     */
    static Error CompareName(const Message &aMessage, uint16_t &aOffset, const Name &aName);

    /**
     * Extracts label(s) from a name by checking that it contains a given suffix name (e.g., suffix name can be
     * a domain name) and removing it.
     *
     * Both @p aName and @p aSuffixName MUST follow the same style regarding inclusion of trailing dot ('.'). Otherwise
     * `kErrorParse` is returned.
     *
     * @param[in]   aName           The name to extract labels from.
     * @param[in]   aSuffixName     The suffix name (e.g., can be domain name).
     * @param[out]  aLabels         Pointer to buffer to copy the extracted labels.
     * @param[in]   aLabelsSize     Size of @p aLabels buffer.
     *
     * @retval kErrorNone     Successfully extracted the labels, @p aLabels is updated.
     * @retval kErrorParse    @p aName does not contain @p aSuffixName.
     * @retval kErrorNoBufs   Could not fit the labels in @p aLabelsSize.
     *
     */
    static Error ExtractLabels(const char *aName, const char *aSuffixName, char *aLabels, uint16_t aLabelsSize);

    /**
     * Extracts label(s) from a name by checking that it contains a given suffix name (e.g., suffix name can be
     * a domain name) and removing it.
     *
     * Both @p aName and @p aSuffixName MUST follow the same style regarding inclusion of trailing dot ('.'). Otherwise
     * `kErrorParse` is returned.
     *
     * @tparam      kLabelsBufferSize   Size of the buffer string.
     *
     * @param[in]   aName           The name to extract labels from.
     * @param[in]   aSuffixName     The suffix name (e.g., can be domain name).
     * @param[out]  aLabelsBuffer   A buffer to copy the extracted labels.
     *
     * @retval kErrorNone     Successfully extracted the labels, @p aLabels is updated.
     * @retval kErrorParse    @p aName does not contain @p aSuffixName.
     * @retval kErrorNoBufs   Could not fit the labels in @p aLabels.
     *
     */
    template <uint16_t kLabelsBufferSize>
    static Error ExtractLabels(const char *aName, const char *aSuffixName, char (&aLabels)[kLabelsBufferSize])
    {
        return ExtractLabels(aName, aSuffixName, aLabels, kLabelsBufferSize);
    }

    /**
     * Tests if a DNS name is a sub-domain of a given domain.
     *
     * Both @p aName and @p aDomain can end without dot ('.').
     *
     * @param[in]  aName    The dot-separated name.
     * @param[in]  aDomain  The dot-separated domain.
     *
     * @returns  TRUE if the name is a sub-domain of @p aDomain, FALSE if is not.
     *
     */
    static bool IsSubDomainOf(const char *aName, const char *aDomain);

    /**
     * Tests if the two DNS name are the same domain.
     *
     * Both @p aDomain1 and @p aDomain2 can end without dot ('.').
     *
     * @param[in]  aDomain1  The dot-separated name.
     * @param[in]  aDomain2  The dot-separated domain.
     *
     * @retval  TRUE   If the two DNS names are the same domain.
     * @retval  FALSE  If the two DNS names are not the same domain.
     *
     */
    static bool IsSameDomain(const char *aDomain1, const char *aDomain2);

private:
    // The first 2 bits of the encoded label specifies label type.
    //
    // - Value 00 indicates normal text label (lower 6-bits indicates the label length).
    // - Value 11 indicates pointer label type (lower 14-bits indicates the pointer offset).
    // - Values 01,10 are reserved (RFC 6891 recommends to not use)

    static constexpr uint8_t kLabelTypeMask    = 0xc0; // 0b1100_0000 (first two bits)
    static constexpr uint8_t kTextLabelType    = 0x00; // Text label type (00)
    static constexpr uint8_t kPointerLabelType = 0xc0; // Pointer label type - compressed name (11)

    static constexpr uint8_t kMaxEncodedLength = 255; ///< Max length of an encoded name.

    static constexpr uint16_t kPointerLabelTypeUint16 = 0xc000; // Pointer label type mask (first 2 bits).
    static constexpr uint16_t kPointerLabelOffsetMask = 0x3fff; // Mask for offset in a pointer label (lower 14 bits).

    static constexpr bool kIsSingleLabel = true; // Used in `LabelIterator::CompareLabel()`.

    struct LabelIterator
    {
        static constexpr uint16_t kUnsetNameEndOffset = 0; // Special value indicating `mNameEndOffset` is not yet set.

        LabelIterator(const Message &aMessage, uint16_t aLabelOffset)
            : mMessage(aMessage)
            , mNextLabelOffset(aLabelOffset)
            , mNameEndOffset(kUnsetNameEndOffset)
        {
        }

        bool  IsEndOffsetSet(void) const { return (mNameEndOffset != kUnsetNameEndOffset); }
        Error GetNextLabel(void);
        Error ReadLabel(char *aLabelBuffer, uint8_t &aLabelLength, bool aAllowDotCharInLabel) const;
        bool  CompareLabel(const char *&aName, bool aIsSingleLabel) const;
        bool  CompareLabel(const LabelIterator &aOtherIterator) const;
        Error AppendLabel(Message &aMessage) const;

        static bool CaseInsensitiveMatch(uint8_t aFirst, uint8_t aSecond);

        const Message &mMessage;          // Message to read labels from.
        uint16_t       mLabelStartOffset; // Offset in `mMessage` to the first char of current label text.
        uint8_t        mLabelLength;      // Length of current label (number of chars).
        uint16_t       mNextLabelOffset;  // Offset in `mMessage` to the start of the next label.
        uint16_t       mNameEndOffset;    // Offset in `mMessage` to the byte after the end of domain name field.
    };

    Name(const char *aString, const Message *aMessage, uint16_t aOffset)
        : mString(aString)
        , mMessage(aMessage)
        , mOffset(aOffset)
    {
    }

    static bool  CompareAndSkipLabels(const char *&aNamePtr, const char *aLabels, char aExpectedNextChar);
    static Error AppendLabel(const char *aLabel, uint8_t aLength, Message &aMessage);

    const char    *mString;  // String containing the name or `nullptr` if name is not from string.
    const Message *mMessage; // Message containing the encoded name, or `nullptr` if `Name` is not from message.
    uint16_t       mOffset;  // Offset in `mMessage` to the start of name (used when name is from `mMessage`).
};

/**
 * Represents a TXT record entry representing a key/value pair (RFC 6763 - section 6.3).
 *
 */
class TxtEntry : public otDnsTxtEntry
{
    friend class TxtRecord;

public:
    /**
     * Minimum length of key string (RFC 6763 - section 6.4).
     *
     */
    static constexpr uint8_t kMinKeyLength = OT_DNS_TXT_KEY_MIN_LENGTH;

    /**
     * Recommended max length of key string (RFC 6763 - section 6.4).
     *
     */
    static constexpr uint8_t kMaxKeyLength = OT_DNS_TXT_KEY_MAX_LENGTH;

    /**
     * Maximum length of TXT key string supported by `Iterator`.
     *
     * This is selected to be longer than recommended `kMaxKeyLength` to handle cases where longer keys are used.
     *
     */
    static constexpr uint8_t kMaxIterKeyLength = OT_DNS_TXT_KEY_ITER_MAX_LENGTH;

    /**
     * Represents an iterator for TXT record entries (key/value pairs).
     *
     */
    class Iterator : public otDnsTxtEntryIterator
    {
        friend class TxtEntry;

    public:
        /**
         * Initializes a TXT record iterator.
         *
         * The buffer pointer @p aTxtData and its content MUST persist and remain unchanged while the iterator object
         * is being used.
         *
         * @param[in] aTxtData        A pointer to buffer containing the encoded TXT data.
         * @param[in] aTxtDataLength  The length (number of bytes) of @p aTxtData.
         *
         */
        void Init(const uint8_t *aTxtData, uint16_t aTxtDataLength);

        /**
         * Parses the TXT data from the `Iterator` and gets the next TXT record entry (key/value pair).
         *
         * The `Iterator` instance MUST be initialized using `Init()` before calling this method and the TXT data
         * buffer used to initialize the iterator MUST persist and remain unchanged.
         *
         * If the parsed key string length is smaller than or equal to `kMaxIterKeyLength` the key string is returned
         * in `mKey` in @p aEntry. But if the key is longer, then `mKey` is set to `nullptr` the entire encoded TXT
         * entry is returned in `mValue` and `mValueLength`.
         *
         * @param[out] aEntry          A reference to a `TxtEntry` to output the parsed/read entry.
         *
         * @retval kErrorNone       The next entry was parsed successfully. @p aEntry is updated.
         * @retval kErrorNotFound   No more entries in TXT data.
         * @retval kErrorParse      The TXT data from `Iterator` is not well-formed.
         *
         */
        Error GetNextEntry(TxtEntry &aEntry);

    private:
        static constexpr uint8_t kIndexTxtLength   = 0;
        static constexpr uint8_t kIndexTxtPosition = 1;

        const char *GetTxtData(void) const { return reinterpret_cast<const char *>(mPtr); }
        void        SetTxtData(const uint8_t *aTxtData) { mPtr = aTxtData; }
        uint16_t    GetTxtDataLength(void) const { return mData[kIndexTxtLength]; }
        void        SetTxtDataLength(uint16_t aLength) { mData[kIndexTxtLength] = aLength; }
        uint16_t    GetTxtDataPosition(void) const { return mData[kIndexTxtPosition]; }
        void        SetTxtDataPosition(uint16_t aValue) { mData[kIndexTxtPosition] = aValue; }
        void        IncreaseTxtDataPosition(uint16_t aIncrement) { mData[kIndexTxtPosition] += aIncrement; }
        char       *GetKeyBuffer(void) { return mChar; }
        const char *GetTxtDataEnd(void) const { return GetTxtData() + GetTxtDataLength(); }
    };

    /**
     * This is the default constructor for a `TxtEntry` object.
     *
     */
    TxtEntry(void) = default;

    /**
     * Initializes a `TxtEntry` object.
     *
     * @param[in] aKey           A pointer to the key string.
     * @param[in] aValue         A pointer to a buffer containing the value.
     * @param[in] aValueLength   Number of bytes in @p aValue buffer.
     *
     */
    TxtEntry(const char *aKey, const uint8_t *aValue, uint8_t aValueLength) { Init(aKey, aValue, aValueLength); }

    /**
     * Initializes a `TxtEntry` object.
     *
     * @param[in] aKey           A pointer to the key string.
     * @param[in] aValue         A pointer to a buffer containing the value.
     * @param[in] aValueLength   Number of bytes in @p aValue buffer.
     *
     */
    void Init(const char *aKey, const uint8_t *aValue, uint8_t aValueLength)
    {
        mKey         = aKey;
        mValue       = aValue;
        mValueLength = aValueLength;
    }

    /**
     * Encodes and appends the `TxtEntry` to a message.
     *
     * @param[in] aMessage  The message to append to.
     *
     * @retval kErrorNone          Entry was appended successfully to @p aMessage.
     * @retval kErrorInvalidArgs   The `TxTEntry` info is not valid.
     * @retval kErrorNoBufs        Insufficient available buffers to grow the message.
     *
     */
    Error AppendTo(Message &aMessage) const;

    /**
     * Appends an array of `TxtEntry` items to a message.
     *
     * @param[in] aEntries     A pointer to array of `TxtEntry` items.
     * @param[in] aNumEntries  The number of entries in @p aEntries array.
     * @param[in] aMessage     The message to append to.
     *
     * @retval kErrorNone          Entries appended successfully to @p aMessage.
     * @retval kErrorInvalidArgs   The `TxTEntry` info is not valid.
     * @retval kErrorNoBufs        Insufficient available buffers to grow the message.
     *
     */
    static Error AppendEntries(const TxtEntry *aEntries, uint16_t aNumEntries, Message &aMessage);

    /**
     * Appends an array of `TxtEntry` items to a `MutableData` buffer.
     *
     * @param[in] aEntries     A pointer to array of `TxtEntry` items.
     * @param[in] aNumEntries  The number of entries in @p aEntries array.
     * @param[in] aData        The `MutableData` to append in.
     *
     * @retval kErrorNone          Entries appended successfully .
     * @retval kErrorInvalidArgs   The `TxTEntry` info is not valid.
     * @retval kErrorNoBufs        Insufficient available buffers.
     *
     */
    static Error AppendEntries(const TxtEntry *aEntries, uint16_t aNumEntries, MutableData<kWithUint16Length> &aData);

private:
    Error        AppendTo(Appender &aAppender) const;
    static Error AppendEntries(const TxtEntry *aEntries, uint16_t aNumEntries, Appender &aAppender);

    static constexpr uint8_t kMaxKeyValueEncodedSize = 255;
    static constexpr char    kKeyValueSeparator      = '=';
};

/**
 * Implements Resource Record (RR) body format.
 *
 */
OT_TOOL_PACKED_BEGIN
class ResourceRecord
{
    friend class OptRecord;

public:
    // Resource Record Types.
    static constexpr uint16_t kTypeZero  = 0;   ///< Zero as special indicator for the SIG RR (SIG(0) from RFC 2931).
    static constexpr uint16_t kTypeA     = 1;   ///< Address record (IPv4).
    static constexpr uint16_t kTypeSoa   = 6;   ///< Start of (zone of) authority.
    static constexpr uint16_t kTypeCname = 5;   ///< CNAME record.
    static constexpr uint16_t kTypePtr   = 12;  ///< PTR record.
    static constexpr uint16_t kTypeTxt   = 16;  ///< TXT record.
    static constexpr uint16_t kTypeSig   = 24;  ///< SIG record.
    static constexpr uint16_t kTypeKey   = 25;  ///< KEY record.
    static constexpr uint16_t kTypeAaaa  = 28;  ///< IPv6 address record.
    static constexpr uint16_t kTypeSrv   = 33;  ///< SRV locator record.
    static constexpr uint16_t kTypeOpt   = 41;  ///< Option record.
    static constexpr uint16_t kTypeAny   = 255; ///< ANY record.

    // Resource Record Class Codes.
    static constexpr uint16_t kClassInternet = 1;   ///< Class code Internet (IN).
    static constexpr uint16_t kClassNone     = 254; ///< Class code None (NONE) - RFC 2136.
    static constexpr uint16_t kClassAny      = 255; ///< Class code Any (ANY).

    /**
     * Initializes the resource record by setting its type and class.
     *
     * Only sets the type and class fields. Other fields (TTL and length) remain unchanged/uninitialized.
     *
     * @param[in] aType   The type of the resource record.
     * @param[in] aClass  The class of the resource record (default is `kClassInternet`).
     *
     */
    void Init(uint16_t aType, uint16_t aClass = kClassInternet)
    {
        SetType(aType);
        SetClass(aClass);
    }

    /**
     * Indicates whether the resources records matches a given type and class code.
     *
     * @param[in] aType   The resource record type to compare with.
     * @param[in] aClass  The resource record class code to compare with (default is `kClassInternet`).
     *
     * @returns TRUE if the resources records matches @p aType and @p aClass, FALSE otherwise.
     *
     */
    bool Matches(uint16_t aType, uint16_t aClass = kClassInternet) const
    {
        return (mType == BigEndian::HostSwap16(aType)) && (mClass == BigEndian::HostSwap16(aClass));
    }

    /**
     * Returns the type of the resource record.
     *
     * @returns The type of the resource record.
     *
     */
    uint16_t GetType(void) const { return BigEndian::HostSwap16(mType); }

    /**
     * Sets the type of the resource record.
     *
     * @param[in]  aType The type of the resource record.
     *
     */
    void SetType(uint16_t aType) { mType = BigEndian::HostSwap16(aType); }

    /**
     * Returns the class of the resource record.
     *
     * @returns The class of the resource record.
     *
     */
    uint16_t GetClass(void) const { return BigEndian::HostSwap16(mClass); }

    /**
     * Sets the class of the resource record.
     *
     * @param[in]  aClass The class of the resource record.
     *
     */
    void SetClass(uint16_t aClass) { mClass = BigEndian::HostSwap16(aClass); }

    /**
     * Returns the time to live field of the resource record.
     *
     * @returns The time to live field of the resource record.
     *
     */
    uint32_t GetTtl(void) const { return BigEndian::HostSwap32(mTtl); }

    /**
     * Sets the time to live field of the resource record.
     *
     * @param[in]  aTtl The time to live field of the resource record.
     *
     */
    void SetTtl(uint32_t aTtl) { mTtl = BigEndian::HostSwap32(aTtl); }

    /**
     * Returns the length of the resource record data.
     *
     * @returns The length of the resource record data.
     *
     */
    uint16_t GetLength(void) const { return BigEndian::HostSwap16(mLength); }

    /**
     * Sets the length of the resource record data.
     *
     * @param[in]  aLength The length of the resource record data.
     *
     */
    void SetLength(uint16_t aLength) { mLength = BigEndian::HostSwap16(aLength); }

    /**
     * Returns the size of (number of bytes) in resource record and its data RDATA section (excluding the
     * name field).
     *
     * @returns Size (number of bytes) of resource record and its data section (excluding the name field)
     *
     */
    uint32_t GetSize(void) const { return sizeof(ResourceRecord) + GetLength(); }

    /**
     * Parses and skips over a given number of resource records in a message from a given offset.
     *
     * @param[in]     aMessage     The message from which to parse/read the resource records. `aMessage.GetOffset()`
     *                             MUST point to the start of DNS header.
     * @param[in,out] aOffset      On input the offset in @p aMessage pointing to the start of the first record.
     *                             On exit (when parsed successfully), @p aOffset is updated to point to the byte after
     *                             the last parsed record.
     * @param[in]     aNumRecords  Number of resource records to parse.
     *
     * @retval kErrorNone      Parsed records successfully. @p aOffset is updated.
     * @retval kErrorParse     Could not parse the records from @p aMessage (e.g., ran out of bytes in @p aMessage).
     *
     */
    static Error ParseRecords(const Message &aMessage, uint16_t &aOffset, uint16_t aNumRecords);

    /**
     * Searches in a given message to find the first resource record matching a given record name.
     *
     * @param[in]     aMessage       The message in which to search for a matching resource record.
     *                               `aMessage.GetOffset()` MUST point to the start of DNS header.
     * @param[in,out] aOffset        On input, the offset in @p aMessage pointing to the start of the first record.
     *                               On exit, if a matching record is found, @p aOffset is updated to point to the byte
     *                               after the record name.
     *                               If a matching record could not be found, @p aOffset is updated to point to the byte
     *                               after the last record that was checked.
     * @param[in,out] aNumRecords    On input, the maximum number of records to check (starting from @p aOffset).
     *                               On exit and if a matching record is found, @p aNumRecords is updated to give the
     *                               number of remaining records after @p aOffset (excluding the matching record).
     * @param[in]     aName          The record name to match against.
     *
     * @retval kErrorNone         A matching record was found. @p aOffset, @p aNumRecords are updated.
     * @retval kErrorNotFound     A matching record could not be found. @p aOffset and @p aNumRecords are updated.
     * @retval kErrorParse        Could not parse records from @p aMessage (e.g., ran out of bytes in @p aMessage).
     *
     */
    static Error FindRecord(const Message &aMessage, uint16_t &aOffset, uint16_t &aNumRecords, const Name &aName);

    /**
     * This template static method searches in a message to find the i-th occurrence of resource records of specific
     * type with a given record name and if found, reads the record from the message.
     *
     * Searches in @p aMessage starting from @p aOffset up to maximum of @p aNumRecords, for the
     * `(aIndex+1)`th occurrence of a resource record of `RecordType` with record name @p aName.
     *
     * On success (i.e., when a matching record is found and read from the message), @p aOffset is updated to point
     * to after the last byte read from the message and copied into @p aRecord. This allows the caller to read any
     * remaining fields in the record data.
     *
     * @tparam        RecordType     The resource record type (i.e., a sub-class of `ResourceRecord`).
     *
     * @param[in]     aMessage       The message to search within for matching resource records.
     *                               `aMessage.GetOffset()` MUST point to the start of DNS header.
     * @param[in,out] aOffset        On input, the offset in @p aMessage pointing to the start of the first record.
     *                               On exit and only if a matching record is found, @p aOffset is updated to point to
     *                               the last read byte in the record (allowing caller to read any remaining fields in
     *                               the record data from the message).
     * @param[in]     aNumRecords    The maximum number of records to check (starting from @p aOffset).
     * @param[in]     aIndex         The matching record index to find. @p aIndex value of zero returns the first
     *                               matching record.
     * @param[in]     aName          The record name to match against.
     * @param[in]     aRecord        A reference to a record object to read a matching record into.
     *                               If a matching record is found, `sizeof(RecordType)` bytes from @p aMessage are
     *                               read and copied into @p aRecord.
     *
     * @retval kErrorNone         A matching record was found. @p aOffset is updated.
     * @retval kErrorNotFound     A matching record could not be found.
     * @retval kErrorParse        Could not parse records from @p aMessage (e.g., ran out of bytes in @p aMessage).
     *
     */
    template <class RecordType>
    static Error FindRecord(const Message &aMessage,
                            uint16_t      &aOffset,
                            uint16_t       aNumRecords,
                            uint16_t       aIndex,
                            const Name    &aName,
                            RecordType    &aRecord)
    {
        return FindRecord(aMessage, aOffset, aNumRecords, aIndex, aName, RecordType::kType, aRecord,
                          sizeof(RecordType));
    }

    /**
     * This template static method tries to read a resource record of a given type from a message. If the record type
     * does not matches the type, it skips over the record.
     *
     * Requires the record name to be already parsed/read from the message. On input, @p aOffset should
     * point to the start of the `ResourceRecord` fields (type, class, TTL, data length) in @p aMessage.
     *
     * Verifies that the record is well-formed in the message. It then reads the record type and compares
     * it with `RecordType::kType` and ensures that the record size is at least `sizeof(RecordType)`. If it all matches,
     * the record is read into @p aRecord.
     *
     * On success (i.e., when a matching record is read from the message), the @p aOffset is updated to point to after
     * the last byte read from the message and copied into @p aRecord and not necessarily the end of the record.
     *  Depending on the `RecordType` format, there may still be more data bytes left in the record to be read. For
     * example, when reading a SRV record using `SrvRecord` type, @p aOffset would point to after the last field in
     * `SrvRecord`  which is the start of "target host domain name" field.
     *
     * @tparam        RecordType     The resource record type (i.e., a sub-class of `ResourceRecord`).
     *
     * @param[in]     aMessage       The message from which to read the record.
     * @param[in,out] aOffset        On input, the offset in @p aMessage pointing to the byte after the record name.
     *                               On exit, if a matching record is read, @p aOffset is updated to point to the last
     *                               read byte in the record.
     *                               If a matching record could not be read, @p aOffset is updated to point to the byte
     *                               after the entire record (skipping over the record).
     * @param[out]    aRecord        A reference to a record to read a matching record into.
     *                               If a matching record is found, `sizeof(RecordType)` bytes from @p aMessage are
     *                               read and copied into @p aRecord.
     *
     * @retval kErrorNone         A matching record was read successfully. @p aOffset, and @p aRecord are updated.
     * @retval kErrorNotFound     A matching record could not be found. @p aOffset is updated.
     * @retval kErrorParse        Could not parse records from @p aMessage (e.g., ran out of bytes in @p aMessage).
     *
     */
    template <class RecordType> static Error ReadRecord(const Message &aMessage, uint16_t &aOffset, RecordType &aRecord)
    {
        return ReadRecord(aMessage, aOffset, RecordType::kType, aRecord, sizeof(RecordType));
    }

protected:
    Error ReadName(const Message &aMessage,
                   uint16_t      &aOffset,
                   uint16_t       aStartOffset,
                   char          *aNameBuffer,
                   uint16_t       aNameBufferSize,
                   bool           aSkipRecord) const;
    Error SkipRecord(const Message &aMessage, uint16_t &aOffset) const;

private:
    static constexpr uint16_t kType = kTypeAny; // This is intended for used by `ReadRecord<RecordType>()` only.

    static Error FindRecord(const Message  &aMessage,
                            uint16_t       &aOffset,
                            uint16_t        aNumRecords,
                            uint16_t        aIndex,
                            const Name     &aName,
                            uint16_t        aType,
                            ResourceRecord &aRecord,
                            uint16_t        aMinRecordSize);

    static Error ReadRecord(const Message  &aMessage,
                            uint16_t       &aOffset,
                            uint16_t        aType,
                            ResourceRecord &aRecord,
                            uint16_t        aMinRecordSize);

    Error CheckRecord(const Message &aMessage, uint16_t aOffset) const;
    Error ReadFrom(const Message &aMessage, uint16_t aOffset);

    uint16_t mType;   // The type of the data in RDATA section.
    uint16_t mClass;  // The class of the data in RDATA section.
    uint32_t mTtl;    // Specifies the maximum time that the resource record may be cached.
    uint16_t mLength; // The length of RDATA section in bytes.

} OT_TOOL_PACKED_END;

/**
 * Implements Resource Record body format of A type.
 *
 */
OT_TOOL_PACKED_BEGIN
class ARecord : public ResourceRecord
{
public:
    static constexpr uint16_t kType = kTypeA; ///< The A record type.

    /**
     * Initializes the A Resource Record by setting its type, class, and length.
     *
     * Other record fields (TTL, address) remain unchanged/uninitialized.
     *
     */
    void Init(void)
    {
        ResourceRecord::Init(kTypeA);
        SetLength(sizeof(Ip4::Address));
    }

    /**
     * Sets the IPv4 address of the resource record.
     *
     * @param[in]  aAddress The IPv4 address of the resource record.
     *
     */
    void SetAddress(const Ip4::Address &aAddress) { mAddress = aAddress; }

    /**
     * Returns the reference to IPv4 address of the resource record.
     *
     * @returns The reference to IPv4 address of the resource record.
     *
     */
    const Ip4::Address &GetAddress(void) const { return mAddress; }

private:
    Ip4::Address mAddress; // IPv4 Address of A Resource Record.
} OT_TOOL_PACKED_END;

/**
 * Implements Resource Record body format of CNAME type.
 *
 */
OT_TOOL_PACKED_BEGIN
class CnameRecord : public ResourceRecord
{
public:
    static constexpr uint16_t kType = kTypeCname; ///< The CNAME record type.

    /**
     * Initializes the CNAME Resource Record by setting its type and class.
     *
     * Other record fields (TTL, length) remain unchanged/uninitialized.
     *
     * @param[in] aClass  The class of the resource record (default is `kClassInternet`).
     *
     */
    void Init(uint16_t aClass = kClassInternet) { ResourceRecord::Init(kTypeCname, aClass); }

    /**
     * Parses and reads the CNAME alias name from a message.
     *
     * Also verifies that the CNAME record is well-formed (e.g., the record data length `GetLength()`
     * matches the CNAME encoded name).
     *
     * @param[in]      aMessage         The message to read from. `aMessage.GetOffset()` MUST point to the start of
     *                                  DNS header.
     * @param[in,out]  aOffset          On input, the offset in @p aMessage to start of CNAME name field.
     *                                  On exit when successfully read, @p aOffset is updated to point to the byte
     *                                  after the entire PTR record (skipping over the record).
     * @param[out]     aNameBuffer      A pointer to a char array to output the read name as a null-terminated C string
     *                                  (MUST NOT be `nullptr`).
     * @param[in]      aNameBufferSize  The size of @p aNameBuffer.
     *
     * @retval kErrorNone           The CNAME name was read successfully. @p aOffset and @p aNameBuffer are updated.
     * @retval kErrorParse          The CNAME record in @p aMessage could not be parsed (invalid format).
     * @retval kErrorNoBufs         Name could not fit in @p aNameBufferSize chars.
     *
     */
    Error ReadCanonicalName(const Message &aMessage,
                            uint16_t      &aOffset,
                            char          *aNameBuffer,
                            uint16_t       aNameBufferSize) const
    {
        return ResourceRecord::ReadName(aMessage, aOffset, /* aStartOffset */ aOffset - sizeof(CnameRecord),
                                        aNameBuffer, aNameBufferSize, /* aSkipRecord */ true);
    }

} OT_TOOL_PACKED_END;

/**
 * Implements Resource Record body format of PTR type.
 *
 */
OT_TOOL_PACKED_BEGIN
class PtrRecord : public ResourceRecord
{
public:
    static constexpr uint16_t kType = kTypePtr; ///< The PTR record type.

    /**
     * Initializes the PTR Resource Record by setting its type and class.
     *
     * Other record fields (TTL, length) remain unchanged/uninitialized.
     *
     * @param[in] aClass  The class of the resource record (default is `kClassInternet`).
     *
     */
    void Init(uint16_t aClass = kClassInternet) { ResourceRecord::Init(kTypePtr, aClass); }

    /**
     * Parses and reads the PTR name from a message.
     *
     * Also verifies that the PTR record is well-formed (e.g., the record data length `GetLength()` matches
     * the PTR encoded name).
     *
     * @param[in]      aMessage         The message to read from.  `aMessage.GetOffset()` MUST point to the start of
     *                                  DNS header.
     * @param[in,out]  aOffset          On input, the offset in @p aMessage to start of PTR name field.
     *                                  On exit when successfully read, @p aOffset is updated to point to the byte
     *                                  after the entire PTR record (skipping over the record).
     * @param[out]     aNameBuffer      A pointer to a char array to output the read name as a null-terminated C string
     *                                  (MUST NOT be `nullptr`).
     * @param[in]      aNameBufferSize  The size of @p aNameBuffer.
     *
     * @retval kErrorNone           The PTR name was read successfully. @p aOffset and @p aNameBuffer are updated.
     * @retval kErrorParse          The PTR record in @p aMessage could not be parsed (invalid format).
     * @retval kErrorNoBufs         Name could not fit in @p aNameBufferSize chars.
     *
     */
    Error ReadPtrName(const Message &aMessage, uint16_t &aOffset, char *aNameBuffer, uint16_t aNameBufferSize) const
    {
        return ResourceRecord::ReadName(aMessage, aOffset, /* aStartOffset */ aOffset - sizeof(PtrRecord), aNameBuffer,
                                        aNameBufferSize,
                                        /* aSkipRecord */ true);
    }

    /**
     * Parses and reads the PTR name from a message.
     *
     * Also verifies that the PTR record is well-formed (e.g., the record data length `GetLength()` matches
     * the PTR encoded name).
     *
     * Unlike the previous method which reads the entire PTR name into a single char buffer, this method reads the
     * first label separately and into a different buffer @p aLabelBuffer and the rest of the name into @p aNameBuffer.
     * The @p aNameBuffer can be set to `nullptr` if the caller is only interested in the first label. This method is
     * intended for "Service Instance Name" where first label (`<Instance>` portion) can be a user-friendly string and
     * can contain dot character.
     *
     * @param[in]      aMessage          The message to read from. `aMessage.GetOffset()` MUST point to the start of
     *                                   DNS header.
     * @param[in,out]  aOffset           On input, the offset in @p aMessage to the start of PTR name field.
     *                                   On exit, when successfully read, @p aOffset is updated to point to the byte
     *                                   after the entire PTR record (skipping over the record).
     * @param[out]     aLabelBuffer      A pointer to a char array to output the first label as a null-terminated C
     *                                   string (MUST NOT be `nullptr`).
     * @param[in]      aLabelBufferSize  The size of @p aLabelBuffer.
     * @param[out]     aNameBuffer       A pointer to a char array to output the rest of name (after first label). Can
     *                                   be `nullptr` if caller is only interested in the first label.
     * @param[in]      aNameBufferSize   The size of @p aNameBuffer.
     *
     * @retval kErrorNone    The PTR name was read successfully. @p aOffset, @aLabelBuffer and @aNameBuffer are updated.
     * @retval kErrorParse   The PTR record in @p aMessage could not be parsed (invalid format).
     * @retval kErrorNoBufs  Either label or name could not fit in the related char buffers.
     *
     */
    Error ReadPtrName(const Message &aMessage,
                      uint16_t      &aOffset,
                      char          *aLabelBuffer,
                      uint8_t        aLabelBufferSize,
                      char          *aNameBuffer,
                      uint16_t       aNameBufferSize) const;

    /**
     * Parses and reads the PTR name from a message.
     *
     * This is a template variation of the previous method with name and label buffer sizes as template parameters.
     *
     * @tparam kLabelBufferSize          The size of label buffer.
     * @tparam kNameBufferSize           The size of name buffer.
     *
     * @param[in]      aMessage          The message to read from. `aMessage.GetOffset()` MUST point to the start of
     *                                   DNS header.
     * @param[in,out]  aOffset           On input, the offset in @p aMessage to the start of PTR name field.
     *                                   On exit, when successfully read, @p aOffset is updated to point to the byte
     *                                   after the entire PTR record (skipping over the record).
     * @param[out]     aLabelBuffer      A char array buffer to output the first label as a null-terminated C string.
     * @param[out]     aNameBuffer       A char array to output the rest of name (after first label).
     *
     * @retval kErrorNone    The PTR name was read successfully. @p aOffset, @aLabelBuffer and @aNameBuffer are updated.
     * @retval kErrorParse   The PTR record in @p aMessage could not be parsed (invalid format).
     * @retval kErrorNoBufs  Either label or name could not fit in the related given buffers.
     *
     */
    template <uint16_t kLabelBufferSize, uint16_t kNameBufferSize>
    Error ReadPtrName(const Message &aMessage,
                      uint16_t      &aOffset,
                      char (&aLabelBuffer)[kLabelBufferSize],
                      char (&aNameBuffer)[kNameBufferSize]) const
    {
        return ReadPtrName(aMessage, aOffset, aLabelBuffer, kLabelBufferSize, aNameBuffer, kNameBufferSize);
    }

} OT_TOOL_PACKED_END;

/**
 * Implements Resource Record body format of TXT type.
 *
 */
OT_TOOL_PACKED_BEGIN
class TxtRecord : public ResourceRecord
{
public:
    static constexpr uint16_t kType = kTypeTxt; ///< The TXT record type.

    /**
     * Initializes the TXT Resource Record by setting its type and class.
     *
     * Other record fields (TTL, length) remain unchanged/uninitialized.
     *
     * @param[in] aClass  The class of the resource record (default is `kClassInternet`).
     *
     */
    void Init(uint16_t aClass = kClassInternet) { ResourceRecord::Init(kTypeTxt, aClass); }

    /**
     * Parses and reads the TXT record data from a message.
     *
     * Also checks if the TXT data is well-formed by calling `VerifyTxtData()` when it is successfully
     * read.
     *
     * @param[in]      aMessage         The message to read from.
     * @param[in,out]  aOffset          On input, the offset in @p aMessage to start of TXT record data.
     *                                  On exit when successfully read, @p aOffset is updated to point to the byte
     *                                  after the entire TXT record (skipping over the record).
     * @param[out]     aTxtBuffer       A pointer to a byte array to output the read TXT data.
     * @param[in,out]  aTxtBufferSize   On input, the size of @p aTxtBuffer (max bytes that can be read).
     *                                  On exit, @p aTxtBufferSize gives number of bytes written to @p aTxtBuffer.
     *
     * @retval kErrorNone           The TXT data was read successfully. @p aOffset, @p aTxtBuffer and @p aTxtBufferSize
     *                              are updated.
     * @retval kErrorParse          The TXT record in @p aMessage could not be parsed (invalid format).
     * @retval kErrorNoBufs         TXT data could not fit in @p aTxtBufferSize bytes. TXT data is still partially read
     *                              into @p aTxtBuffer up to its size and @p aOffset is updated to skip over the full
     *                              TXT record.
     *
     */
    Error ReadTxtData(const Message &aMessage, uint16_t &aOffset, uint8_t *aTxtBuffer, uint16_t &aTxtBufferSize) const;

    /**
     * Tests if a buffer contains valid encoded TXT data.
     *
     * @param[in]  aTxtData     The TXT data buffer.
     * @param[in]  aTxtLength   The length of the TXT data buffer.
     * @param[in]  aAllowEmpty  True if zero-length TXT data is allowed.
     *
     * @returns  TRUE if @p aTxtData contains valid encoded TXT data, FALSE if not.
     *
     */
    static bool VerifyTxtData(const uint8_t *aTxtData, uint16_t aTxtLength, bool aAllowEmpty);

} OT_TOOL_PACKED_END;

/**
 * Implements Resource Record body format of AAAA type.
 *
 */
OT_TOOL_PACKED_BEGIN
class AaaaRecord : public ResourceRecord
{
public:
    static constexpr uint16_t kType = kTypeAaaa; ///< The AAAA record type.

    /**
     * Initializes the AAAA Resource Record by setting its type, class, and length.
     *
     * Other record fields (TTL, address) remain unchanged/uninitialized.
     *
     */
    void Init(void)
    {
        ResourceRecord::Init(kTypeAaaa);
        SetLength(sizeof(Ip6::Address));
    }

    /**
     * Tells whether this is a valid AAAA record.
     *
     * @returns  A boolean indicates whether this is a valid AAAA record.
     *
     */
    bool IsValid(void) const;

    /**
     * Sets the IPv6 address of the resource record.
     *
     * @param[in]  aAddress The IPv6 address of the resource record.
     *
     */
    void SetAddress(const Ip6::Address &aAddress) { mAddress = aAddress; }

    /**
     * Returns the reference to IPv6 address of the resource record.
     *
     * @returns The reference to IPv6 address of the resource record.
     *
     */
    const Ip6::Address &GetAddress(void) const { return mAddress; }

private:
    Ip6::Address mAddress; // IPv6 Address of AAAA Resource Record.
} OT_TOOL_PACKED_END;

/**
 * Implements Resource Record body format of SRV type (RFC 2782).
 *
 */
OT_TOOL_PACKED_BEGIN
class SrvRecord : public ResourceRecord
{
public:
    static constexpr uint16_t kType = kTypeSrv; ///< The SRV record type.

    /**
     * Initializes the SRV Resource Record by settings its type and class.
     *
     * Other record fields (TTL, length, propriety, weight, port, ...) remain unchanged/uninitialized.
     *
     * @param[in] aClass  The class of the resource record (default is `kClassInternet`).
     *
     */
    void Init(uint16_t aClass = kClassInternet) { ResourceRecord::Init(kTypeSrv, aClass); }

    /**
     * Returns the SRV record's priority value.
     *
     * @returns The priority value.
     *
     */
    uint16_t GetPriority(void) const { return BigEndian::HostSwap16(mPriority); }

    /**
     * Sets the SRV record's priority value.
     *
     * @param[in]  aPriority  The priority value.
     *
     */
    void SetPriority(uint16_t aPriority) { mPriority = BigEndian::HostSwap16(aPriority); }

    /**
     * Returns the SRV record's weight value.
     *
     * @returns The weight value.
     *
     */
    uint16_t GetWeight(void) const { return BigEndian::HostSwap16(mWeight); }

    /**
     * Sets the SRV record's weight value.
     *
     * @param[in]  aWeight  The weight value.
     *
     */
    void SetWeight(uint16_t aWeight) { mWeight = BigEndian::HostSwap16(aWeight); }

    /**
     * Returns the SRV record's port number on the target host for this service.
     *
     * @returns The port number.
     *
     */
    uint16_t GetPort(void) const { return BigEndian::HostSwap16(mPort); }

    /**
     * Sets the SRV record's port number on the target host for this service.
     *
     * @param[in]  aPort  The port number.
     *
     */
    void SetPort(uint16_t aPort) { mPort = BigEndian::HostSwap16(aPort); }

    /**
     * Parses and reads the SRV target host name from a message.
     *
     * Also verifies that the SRV record is well-formed (e.g., the record data length `GetLength()` matches
     * the SRV encoded name).
     *
     * @param[in]      aMessage         The message to read from. `aMessage.GetOffset()` MUST point to the start of
     *                                  DNS header.
     * @param[in,out]  aOffset          On input, the offset in @p aMessage to start of target host name field.
     *                                  On exit when successfully read, @p aOffset is updated to point to the byte
     *                                  after the entire SRV record (skipping over the record).
     * @param[out]     aNameBuffer      A pointer to a char array to output the read name as a null-terminated C string
     *                                  (MUST NOT be `nullptr`).
     * @param[in]      aNameBufferSize  The size of @p aNameBuffer.
     *
     * @retval kErrorNone            The host name was read successfully. @p aOffset and @p aNameBuffer are updated.
     * @retval kErrorParse           The SRV record in @p aMessage could not be parsed (invalid format).
     * @retval kErrorNoBufs          Name could not fit in @p aNameBufferSize chars.
     *
     */
    Error ReadTargetHostName(const Message &aMessage,
                             uint16_t      &aOffset,
                             char          *aNameBuffer,
                             uint16_t       aNameBufferSize) const
    {
        return ResourceRecord::ReadName(aMessage, aOffset, /* aStartOffset */ aOffset - sizeof(SrvRecord), aNameBuffer,
                                        aNameBufferSize,
                                        /* aSkipRecord */ true);
    }

    /**
     * Parses and reads the SRV target host name from a message.
     *
     * Also verifies that the SRV record is well-formed (e.g., the record data length `GetLength()` matches
     * the SRV encoded name).
     *
     * @tparam         kNameBufferSize  Size of the name buffer.
     *
     * @param[in]      aMessage         The message to read from. `aMessage.GetOffset()` MUST point to the start of
     *                                  DNS header.
     * @param[in,out]  aOffset          On input, the offset in @p aMessage to start of target host name field.
     *                                  On exit when successfully read, @p aOffset is updated to point to the byte
     *                                  after the entire SRV record (skipping over the record).
     * @param[out]     aNameBuffer      A char array to output the read name as a null-terminated C string
     *
     * @retval kErrorNone            The host name was read successfully. @p aOffset and @p aNameBuffer are updated.
     * @retval kErrorParse           The SRV record in @p aMessage could not be parsed (invalid format).
     * @retval kErrorNoBufs          Name could not fit in @p aNameBuffer.
     *
     */
    template <uint16_t kNameBufferSize>
    Error ReadTargetHostName(const Message &aMessage, uint16_t &aOffset, char (&aNameBuffer)[kNameBufferSize]) const
    {
        return ReadTargetHostName(aMessage, aOffset, aNameBuffer, kNameBufferSize);
    }

private:
    uint16_t mPriority;
    uint16_t mWeight;
    uint16_t mPort;
    // Followed by the target host domain name.

} OT_TOOL_PACKED_END;

/**
 * Implements Resource Record body format of KEY type (RFC 2535).
 *
 */
OT_TOOL_PACKED_BEGIN
class KeyRecord : public ResourceRecord
{
public:
    static constexpr uint16_t kType = kTypeKey; ///< The KEY record type.

    // Protocol field values (RFC 2535 - section 3.1.3).
    static constexpr uint8_t kProtocolTls    = 1; ///< TLS protocol code.
    static constexpr uint8_t kProtocolDnsSec = 3; ///< DNS security protocol code.

    // Algorithm field values (RFC 8624 - section 3.1).
    static constexpr uint8_t kAlgorithmEcdsaP256Sha256 = 13; ///< ECDSA-P256-SHA256 algorithm.
    static constexpr uint8_t kAlgorithmEcdsaP384Sha384 = 14; ///< ECDSA-P384-SHA384 algorithm.
    static constexpr uint8_t kAlgorithmEd25519         = 15; ///< ED25519 algorithm.
    static constexpr uint8_t kAlgorithmEd448           = 16; ///< ED448 algorithm.

    /**
     * Type represents the use (or key type) flags (RFC 2535 - section 3.1.2).
     *
     */
    enum UseFlags : uint8_t
    {
        kAuthConfidPermitted = 0x00, ///< Use of the key for authentication and/or confidentiality is permitted.
        kAuthPermitted       = 0x40, ///< Use of the key is only permitted for authentication.
        kConfidPermitted     = 0x80, ///< Use of the key is only permitted for confidentiality.
        kNoKey               = 0xc0, ///< No key value (e.g., can indicate zone is not secure).
    };

    /**
     * Type represents key owner (or name type) flags (RFC 2535 - section 3.1.2).
     *
     */
    enum OwnerFlags : uint8_t
    {
        kOwnerUser     = 0x00, ///< Key is associated with a "user" or "account" at end entity.
        kOwnerZone     = 0x01, ///< Key is a zone key (used for data origin authentication).
        kOwnerNonZone  = 0x02, ///< Key is associated with a non-zone "entity".
        kOwnerReserved = 0x03, ///< Reserved for future use.
    };

    // Constants for flag bits for the "signatory" flags (RFC 2137).
    //
    // The flags defined are for non-zone (`kOwnerNoneZone`) keys (RFC 2137 - section 3.1.3).

    /**
     * Key is authorized to attach, detach, and move zones.
     *
     */
    static constexpr uint8_t kSignatoryFlagZone = 1 << 3;

    /**
     * Key is authorized to add and delete RRs even if RRs auth with other key.
     *
     */
    static constexpr uint8_t kSignatoryFlagStrong = 1 << 2;

    /**
     * Key is authorized to add and update RRs for only a single owner name.
     *
     */
    static constexpr uint8_t kSignatoryFlagUnique = 1 << 1;

    /**
     * If the other flags are zero, this is used to indicate it is an update key.
     *
     */
    static constexpr uint8_t kSignatoryFlagGeneral = 1 << 0;

    /**
     * Initializes the KEY Resource Record by setting its type and class.
     *
     * Other record fields (TTL, length, flags, protocol, algorithm) remain unchanged/uninitialized.
     *
     * @param[in] aClass  The class of the resource record (default is `kClassInternet`).
     *
     */
    void Init(uint16_t aClass = kClassInternet) { ResourceRecord::Init(kTypeKey, aClass); }

    /**
     * Tells whether the KEY record is valid.
     *
     * @returns  TRUE if this is a valid KEY record, FALSE if an invalid KEY record.
     *
     */
    bool IsValid(void) const;

    /**
     * Gets the key use (or key type) flags.
     *
     * @returns The key use flags.
     *
     */
    UseFlags GetUseFlags(void) const { return static_cast<UseFlags>(mFlags[0] & kUseFlagsMask); }

    /**
     * Gets the owner (or name type) flags.
     *
     * @returns The key owner flags.
     *
     */
    OwnerFlags GetOwnerFlags(void) const { return static_cast<OwnerFlags>(mFlags[0] & kOwnerFlagsMask); }

    /**
     * Gets the signatory flags.
     *
     * @returns The signatory flags.
     *
     */
    uint8_t GetSignatoryFlags(void) const { return (mFlags[1] & kSignatoryFlagsMask); }

    /**
     * Sets the flags field.
     *
     * @param[in] aUseFlags        The `UseFlags` value.
     * @param[in] aOwnerFlags      The `OwnerFlags` value.
     * @param[in] aSignatoryFlags  The signatory flags.
     *
     */
    void SetFlags(UseFlags aUseFlags, OwnerFlags aOwnerFlags, uint8_t aSignatoryFlags)
    {
        mFlags[0] = (static_cast<uint8_t>(aUseFlags) | static_cast<uint8_t>(aOwnerFlags));
        mFlags[1] = (aSignatoryFlags & kSignatoryFlagsMask);
    }

    /**
     * Returns the KEY record's protocol value.
     *
     * @returns The protocol value.
     *
     */
    uint8_t GetProtocol(void) const { return mProtocol; }

    /**
     * Sets the KEY record's protocol value.
     *
     * @param[in]  aProtocol  The protocol value.
     *
     */
    void SetProtocol(uint8_t aProtocol) { mProtocol = aProtocol; }

    /**
     * Returns the KEY record's algorithm value.
     *
     * @returns The algorithm value.
     *
     */
    uint8_t GetAlgorithm(void) const { return mAlgorithm; }

    /**
     * Sets the KEY record's algorithm value.
     *
     * @param[in]  aAlgorithm  The algorithm value.
     *
     */
    void SetAlgorithm(uint8_t aAlgorithm) { mAlgorithm = aAlgorithm; }

private:
    static constexpr uint8_t kUseFlagsMask       = 0xc0; // top two bits in the first flag byte.
    static constexpr uint8_t kOwnerFlagsMask     = 0x03; // lowest two bits in the first flag byte.
    static constexpr uint8_t kSignatoryFlagsMask = 0x0f; // lower 4 bits in the second flag byte.

    // Flags format:
    //
    //    0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5
    //  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //  |  Use  | Z | XT| Z | Z | Owner | Z | Z | Z | Z |      SIG      |
    //  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //  \                              / \                             /
    //   ---------- mFlags[0] ---------   -------- mFlags[1] ----------

    uint8_t mFlags[2];
    uint8_t mProtocol;
    uint8_t mAlgorithm;
    // Followed by the public key

} OT_TOOL_PACKED_END;

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
OT_TOOL_PACKED_BEGIN
class Ecdsa256KeyRecord : public KeyRecord, public Clearable<Ecdsa256KeyRecord>, public Equatable<Ecdsa256KeyRecord>
{
public:
    /**
     * Initializes the KEY Resource Record to ECDSA with curve P-256.
     *
     * Other record fields (TTL, length, flags, protocol) remain unchanged/uninitialized.
     *
     */
    void Init(void);

    /**
     * Tells whether this is a valid ECDSA DNSKEY with curve P-256.
     *
     * @returns  A boolean that indicates whether this is a valid ECDSA DNSKEY RR with curve P-256.
     *
     */
    bool IsValid(void) const;

    /**
     * Returns the ECDSA P-256 public kek.
     *
     * @returns  A reference to the public key.
     *
     */
    const Crypto::Ecdsa::P256::PublicKey &GetKey(void) const { return mKey; }

private:
    Crypto::Ecdsa::P256::PublicKey mKey;
} OT_TOOL_PACKED_END;
#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

/**
 * Implements Resource Record body format of SIG type (RFC 2535 - section-4.1).
 *
 *
 */
OT_TOOL_PACKED_BEGIN
class SigRecord : public ResourceRecord, public Clearable<SigRecord>
{
public:
    static constexpr uint16_t kType = kTypeSig; ///< The SIG record type.

    /**
     * Initializes the SIG Resource Record by setting its type and class.
     *
     * Other record fields (TTL, length, ...) remain unchanged/uninitialized.
     *
     * SIG(0) requires SIG RR to set class field as ANY or `kClassAny` (RFC 2931 - section 3).
     *
     * @param[in] aClass  The class of the resource record.
     *
     */
    void Init(uint16_t aClass) { ResourceRecord::Init(kTypeSig, aClass); }

    /**
     * Tells whether the SIG record is valid.
     *
     * @returns  TRUE if this is a valid SIG record, FALSE if not a valid SIG record.
     *
     */
    bool IsValid(void) const;

    /**
     * Returns the SIG record's type-covered value.
     *
     * @returns The type-covered value.
     *
     */
    uint16_t GetTypeCovered(void) const { return BigEndian::HostSwap16(mTypeCovered); }

    /**
     * Sets the SIG record's type-covered value.
     *
     * @param[in]  aTypeCovered  The type-covered value.
     *
     */
    void SetTypeCovered(uint8_t aTypeCovered) { mTypeCovered = BigEndian::HostSwap16(aTypeCovered); }

    /**
     * Returns the SIG record's algorithm value.
     *
     * @returns The algorithm value.
     *
     */
    uint8_t GetAlgorithm(void) const { return mAlgorithm; }

    /**
     * Sets the SIG record's algorithm value.
     *
     * @param[in]  aAlgorithm  The algorithm value.
     *
     */
    void SetAlgorithm(uint8_t aAlgorithm) { mAlgorithm = aAlgorithm; }

    /**
     * Returns the SIG record's labels-count (number of labels, not counting null label, in the original
     * name of the owner).
     *
     * @returns The labels-count value.
     *
     */
    uint8_t GetLabels(void) const { return mLabels; }

    /**
     * Sets the SIG record's labels-count (number of labels, not counting null label, in the original
     * name of the owner).
     *
     * @param[in]  aLabels  The labels-count value.
     *
     */
    void SetLabels(uint8_t aLabels) { mLabels = aLabels; }

    /**
     * Returns the SIG record's original TTL value.
     *
     * @returns The original TTL value.
     *
     */
    uint32_t GetOriginalTtl(void) const { return BigEndian::HostSwap32(mOriginalTtl); }

    /**
     * Sets the SIG record's original TTL value.
     *
     * @param[in]  aOriginalTtl  The original TTL value.
     *
     */
    void SetOriginalTtl(uint32_t aOriginalTtl) { mOriginalTtl = BigEndian::HostSwap32(aOriginalTtl); }

    /**
     * Returns the SIG record's expiration time value.
     *
     * @returns The expiration time value (seconds since Jan 1, 1970).
     *
     */
    uint32_t GetExpiration(void) const { return BigEndian::HostSwap32(mExpiration); }

    /**
     * Sets the SIG record's expiration time value.
     *
     * @param[in]  aExpiration  The expiration time value (seconds since Jan 1, 1970).
     *
     */
    void SetExpiration(uint32_t aExpiration) { mExpiration = BigEndian::HostSwap32(aExpiration); }

    /**
     * Returns the SIG record's inception time value.
     *
     * @returns The inception time value (seconds since Jan 1, 1970).
     *
     */
    uint32_t GetInception(void) const { return BigEndian::HostSwap32(mInception); }

    /**
     * Sets the SIG record's inception time value.
     *
     * @param[in]  aInception  The inception time value (seconds since Jan 1, 1970).
     *
     */
    void SetInception(uint32_t aInception) { mInception = BigEndian::HostSwap32(aInception); }

    /**
     * Returns the SIG record's key tag value.
     *
     * @returns The key tag value.
     *
     */
    uint16_t GetKeyTag(void) const { return BigEndian::HostSwap16(mKeyTag); }

    /**
     * Sets the SIG record's key tag value.
     *
     * @param[in]  aKeyTag  The key tag value.
     *
     */
    void SetKeyTag(uint16_t aKeyTag) { mKeyTag = BigEndian::HostSwap16(aKeyTag); }

    /**
     * Returns a pointer to the start of the record data fields.
     *
     * @returns A pointer to the start of the record data fields.
     *
     */
    const uint8_t *GetRecordData(void) const { return reinterpret_cast<const uint8_t *>(&mTypeCovered); }

    /**
     * Parses and reads the SIG signer name from a message.
     *
     * @param[in]      aMessage         The message to read from. `aMessage.GetOffset()` MUST point to the start of DNS
     *                                  header.
     * @param[in,out]  aOffset          On input, the offset in @p aMessage to start of signer name field.
     *                                  On exit when successfully read, @p aOffset is updated to point to the byte
     *                                  after the name field (i.e., start of signature field).
     * @param[out]     aNameBuffer      A pointer to a char array to output the read name as a null-terminated C string
     *                                  (MUST NOT be `nullptr`).
     * @param[in]      aNameBufferSize  The size of @p aNameBuffer.
     *
     * @retval kErrorNone           The name was read successfully. @p aOffset and @p aNameBuffer are updated.
     * @retval kErrorParse          The SIG record in @p aMessage could not be parsed (invalid format).
     * @retval kErrorNoBufs         Name could not fit in @p aNameBufferSize chars.
     *
     */
    Error ReadSignerName(const Message &aMessage, uint16_t &aOffset, char *aNameBuffer, uint16_t aNameBufferSize) const
    {
        return ResourceRecord::ReadName(aMessage, aOffset, /* aStartOffset */ aOffset - sizeof(SigRecord), aNameBuffer,
                                        aNameBufferSize,
                                        /* aSkipRecord */ false);
    }

private:
    uint16_t mTypeCovered; // type of the other RRs covered by this SIG. set to zero for SIG(0).
    uint8_t  mAlgorithm;   // Algorithm number (see `KeyRecord` enumeration).
    uint8_t  mLabels;      // Number of labels (not counting null label) in the original name of the owner of RR.
    uint32_t mOriginalTtl; // Original time-to-live (should set to zero for SIG(0)).
    uint32_t mExpiration;  // Signature expiration time (seconds since Jan 1, 1970).
    uint32_t mInception;   // Signature inception time (seconds since Jan 1, 1970).
    uint16_t mKeyTag;      // Key tag.
    // Followed by signer name fields and signature fields
} OT_TOOL_PACKED_END;

/**
 * Implements DNS OPT Pseudo Resource Record header for EDNS(0) (RFC 6891 - Section 6.1).
 *
 */
OT_TOOL_PACKED_BEGIN
class OptRecord : public ResourceRecord
{
public:
    static constexpr uint16_t kType = kTypeOpt; ///< The OPT record type.

    /**
     * Initializes the OPT Resource Record by setting its type and clearing extended Response Code, version
     * and all flags.
     *
     * Other record fields (UDP payload size, length) remain unchanged/uninitialized.
     *
     */
    void Init(void)
    {
        SetType(kTypeOpt);
        SetTtl(0);
    }

    /**
     * Gets the requester's UDP payload size (the number of bytes of the largest UDP payload that can be
     * delivered in the requester's network).
     *
     * The field is encoded in the CLASS field.
     *
     * @returns The UDP payload size.
     *
     */
    uint16_t GetUdpPayloadSize(void) const { return GetClass(); }

    /**
     * Gets the requester's UDP payload size (the number of bytes of the largest UDP payload that can be
     * delivered in the requester's network).
     *
     * @param[in] aPayloadSize  The UDP payload size.
     *
     */
    void SetUdpPayloadSize(uint16_t aPayloadSize) { SetClass(aPayloadSize); }

    /**
     * Gets the upper 8-bit of the extended 12-bit Response Code.
     *
     * Value of 0 indicates that an unextended Response code is in use.
     *
     * @return The upper 8-bit of the extended 12-bit Response Code.
     *
     */
    uint8_t GetExtendedResponseCode(void) const { return GetTtlByteAt(kExtRCodeByteIndex); }

    /**
     * Sets the upper 8-bit of the extended 12-bit Response Code.
     *
     * Value of 0 indicates that an unextended Response code is in use.
     *
     * @param[in] aExtendedResponse The upper 8-bit of the extended 12-bit Response Code.
     *
     */
    void SetExtendedResponseCode(uint8_t aExtendedResponse) { GetTtlByteAt(kExtRCodeByteIndex) = aExtendedResponse; }

    /**
     * Gets the Version field.
     *
     * @returns The version.
     *
     */
    uint8_t GetVersion(void) const { return GetTtlByteAt(kVersionByteIndex); }

    /**
     * Set the Version field.
     *
     * @param[in] aVersion  The version.
     *
     */
    void SetVersion(uint8_t aVersion) { GetTtlByteAt(kVersionByteIndex) = aVersion; }

    /**
     * Indicates whether the DNSSEC OK flag is set or not.
     *
     * @returns True if DNSSEC OK flag is set in the header, false otherwise.
     *
     */
    bool IsDnsSecurityFlagSet(void) const { return (GetTtlByteAt(kFlagByteIndex) & kDnsSecFlag) != 0; }

    /**
     * Clears the DNSSEC OK bit flag.
     *
     */
    void ClearDnsSecurityFlag(void) { GetTtlByteAt(kFlagByteIndex) &= ~kDnsSecFlag; }

    /**
     * Sets the DNSSEC OK bit flag.
     *
     */
    void SetDnsSecurityFlag(void) { GetTtlByteAt(kFlagByteIndex) |= kDnsSecFlag; }

private:
    // The OPT RR re-purposes the existing CLASS and TTL fields in the
    // RR. The CLASS field (`uint16_t`) is used for requester UDP
    // payload size. The TTL field is used for extended Response Code,
    // version and flags as follows:
    //
    //    0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5
    //  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //  |         EXTENDED-RCODE        |            VERSION            |
    //  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //  | DO|                Z          |             Z                 |
    //  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //
    // The variable data part of OPT RR can contain zero of more `Option`.

    static constexpr uint8_t kExtRCodeByteIndex = 0;      // Byte index of Extended RCODE within the TTL field.
    static constexpr uint8_t kVersionByteIndex  = 1;      // Byte index of Version within the TTL field.
    static constexpr uint8_t kFlagByteIndex     = 2;      // Byte index of flag byte within the TTL field.
    static constexpr uint8_t kDnsSecFlag        = 1 << 7; // DNSSec OK bit flag.

    uint8_t  GetTtlByteAt(uint8_t aIndex) const { return reinterpret_cast<const uint8_t *>(&mTtl)[aIndex]; }
    uint8_t &GetTtlByteAt(uint8_t aIndex) { return reinterpret_cast<uint8_t *>(&mTtl)[aIndex]; }

} OT_TOOL_PACKED_END;

/**
 * Implements the body of an Option in OPT Pseudo Resource Record (RFC 6981 - Section 6.1).
 *
 */
OT_TOOL_PACKED_BEGIN
class Option
{
public:
    static constexpr uint16_t kUpdateLease = 2; ///< Update lease option code.

    /**
     * Returns the option code value.
     *
     * @returns The option code value.
     *
     */
    uint16_t GetOptionCode(void) const { return BigEndian::HostSwap16(mOptionCode); }

    /**
     * Sets the option code value.
     *
     * @param[in]  aOptionCode  The option code value.
     *
     */
    void SetOptionCode(uint16_t aOptionCode) { mOptionCode = BigEndian::HostSwap16(aOptionCode); }

    /**
     * Returns the option length value.
     *
     * @returns The option length (size of option data in bytes).
     *
     */
    uint16_t GetOptionLength(void) const { return BigEndian::HostSwap16(mOptionLength); }

    /**
     * Sets the option length value.
     *
     * @param[in]  aOptionLength  The option length (size of option data in bytes).
     *
     */
    void SetOptionLength(uint16_t aOptionLength) { mOptionLength = BigEndian::HostSwap16(aOptionLength); }

    /**
     * Returns the size of (number of bytes) in the Option and its data.
     *
     * @returns Size (number of bytes) of the Option its data section.
     *
     */
    uint32_t GetSize(void) const { return sizeof(Option) + GetOptionLength(); }

private:
    uint16_t mOptionCode;
    uint16_t mOptionLength;
    // Followed by Option data (varies per option code).

} OT_TOOL_PACKED_END;

/**
 * Implements an Update Lease Option body.
 *
 * This implementation is intended for use in Dynamic DNS Update Lease Requests and Responses as specified in
 * https://tools.ietf.org/html/draft-sekar-dns-ul-02.
 *
 */
OT_TOOL_PACKED_BEGIN
class LeaseOption : public Option
{
public:
    /**
     * Initializes the Update Lease Option using the short variant format which contains lease interval
     * only.
     *
     * @param[in] aLeaseInterval     The lease interval in seconds.
     *
     */
    void InitAsShortVariant(uint32_t aLeaseInterval);

    /**
     * Initializes the Update Lease Option using the long variant format which contains both lease and
     * key lease intervals.
     *
     * @param[in] aLeaseInterval     The lease interval in seconds.
     * @param[in] aKeyLeaseInterval  The key lease interval in seconds.
     *
     */
    void InitAsLongVariant(uint32_t aLeaseInterval, uint32_t aKeyLeaseInterval);

    /**
     * Indicates whether or not the Update Lease Option follows the short variant format which contains
     * only the lease interval.
     *
     * @retval TRUE   The Update Lease Option follows the short variant format.
     * @retval FALSE  The Update Lease Option follows the long variant format.
     *
     */
    bool IsShortVariant(void) const { return (GetOptionLength() == kShortLength); }

    /**
     * Tells whether this is a valid Lease Option.
     *
     * Validates that option follows either short or long variant format.
     *
     * @returns  TRUE if this is a valid Lease Option, FALSE if not a valid Lease Option.
     *
     */
    bool IsValid(void) const;

    /**
     * Returns the Update Lease OPT record's lease interval value.
     *
     * @returns The lease interval value (in seconds).
     *
     */
    uint32_t GetLeaseInterval(void) const { return BigEndian::HostSwap32(mLeaseInterval); }

    /**
     * Returns the Update Lease OPT record's key lease interval value.
     *
     * If the Update Lease Option follows the short variant format the lease interval is returned as key lease interval.
     *
     * @returns The key lease interval value (in seconds).
     *
     */
    uint32_t GetKeyLeaseInterval(void) const
    {
        return IsShortVariant() ? GetLeaseInterval() : BigEndian::HostSwap32(mKeyLeaseInterval);
    }

    /**
     * Searches among the Options is a given message and reads and validates the Update Lease Option if
     * found.
     *
     * Reads the Update Lease Option whether it follows the short or long variant formats.
     *
     * @param[in] aMessage   The message to read the Option from.
     * @param[in] aOffset    Offset in @p aMessage to the start of Options (start of OPT Record data).
     * @param[in] aLength    Length of Option data in OPT record.
     *
     * @retval kErrorNone      Successfully read and validated the Update Lease Option from @p aMessage.
     * @retval kErrorNotFound  Did not find any Update Lease Option.
     * @retval kErrorParse     Failed to parse the Options.
     *
     */
    Error ReadFrom(const Message &aMessage, uint16_t aOffset, uint16_t aLength);

private:
    static constexpr uint16_t kShortLength = sizeof(uint32_t);                    // lease only.
    static constexpr uint16_t kLongLength  = sizeof(uint32_t) + sizeof(uint32_t); // lease and key lease values

    void SetLeaseInterval(uint32_t aLeaseInterval) { mLeaseInterval = BigEndian::HostSwap32(aLeaseInterval); }
    void SetKeyLeaseInterval(uint32_t aKeyLeaseInterval)
    {
        mKeyLeaseInterval = BigEndian::HostSwap32(aKeyLeaseInterval);
    }

    uint32_t mLeaseInterval;
    uint32_t mKeyLeaseInterval;
} OT_TOOL_PACKED_END;

/**
 * Implements Question format.
 *
 */
OT_TOOL_PACKED_BEGIN
class Question
{
public:
    /**
     * Default constructor for Question
     *
     */
    Question(void) = default;

    /**
     * Constructor for Question.
     *
     */
    explicit Question(uint16_t aType, uint16_t aClass = ResourceRecord::kClassInternet)
    {
        SetType(aType);
        SetClass(aClass);
    }

    /**
     * Returns the type of the question.
     *
     * @returns The type of the question.
     *
     */
    uint16_t GetType(void) const { return BigEndian::HostSwap16(mType); }

    /**
     * Sets the type of the question.
     *
     * @param[in]  aType The type of the question.
     *
     */
    void SetType(uint16_t aType) { mType = BigEndian::HostSwap16(aType); }

    /**
     * Returns the class of the question.
     *
     * @returns The class of the question.
     *
     */
    uint16_t GetClass(void) const { return BigEndian::HostSwap16(mClass); }

    /**
     * Sets the class of the question.
     *
     * @param[in]  aClass The class of the question.
     *
     */
    void SetClass(uint16_t aClass) { mClass = BigEndian::HostSwap16(aClass); }

private:
    uint16_t mType;  // The type of the data in question section.
    uint16_t mClass; // The class of the data in question section.
} OT_TOOL_PACKED_END;

/**
 * Implements Zone section body for DNS Update (RFC 2136 - section 2.3).
 *
 */
OT_TOOL_PACKED_BEGIN
class Zone : public Question
{
public:
    /**
     * Constructor for Zone.
     *
     * @param[in] aClass  The class of the zone (default is `kClassInternet`).
     *
     */
    explicit Zone(uint16_t aClass = ResourceRecord::kClassInternet)
        : Question(ResourceRecord::kTypeSoa, aClass)
    {
    }
} OT_TOOL_PACKED_END;

/**
 * @}
 *
 */

} // namespace Dns

DefineCoreType(otDnsTxtEntry, Dns::TxtEntry);
DefineCoreType(otDnsTxtEntryIterator, Dns::TxtEntry::Iterator);

} // namespace ot

#endif // DNS_HEADER_HPP_
