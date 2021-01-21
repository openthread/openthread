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

#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/message.hpp"
#include "net/ip6_address.hpp"

namespace ot {

/**
 * @namespace ot::Dns
 * @brief
 *   This namespace includes definitions for DNS.
 *
 */
namespace Dns {

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

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
 * This class implements DNS header generation and parsing.
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
     * This method returns the Message ID.
     *
     * @returns The Message ID value.
     *
     */
    uint16_t GetMessageId(void) const { return HostSwap16(mMessageId); }

    /**
     * This method sets the Message ID.
     *
     * @param[in]  aMessageId The Message ID value.
     *
     */
    void SetMessageId(uint16_t aMessageId) { mMessageId = HostSwap16(aMessageId); }

    /**
     * This method sets the Message ID to a crypto-secure randomly generated number.
     *
     * @retval  OT_ERROR_NONE     Successfully generated random Message ID.
     * @retval  OT_ERROR_FAILED   Could not generate random Message ID.
     *
     */
    otError SetRandomMessageId(void);

    /**
     * Defines types of DNS message.
     *
     */
    enum Type
    {
        kTypeQuery    = 0,
        kTypeResponse = 1,
    };

    /**
     * This method returns the type of the message.
     *
     * @returns The type of the message.
     *
     */
    Type GetType(void) const { return static_cast<Type>((mFlags[0] & kQrFlagMask) >> kQrFlagOffset); }

    /**
     * This method sets the type of the message.
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
    enum QueryType
    {
        kQueryTypeStandard = 0,
        kQueryTypeInverse  = 1,
        kQueryTypeStatus   = 2,
        kQueryTypeNotify   = 4,
        kQueryTypeUpdate   = 5
    };

    /**
     * This method returns the type of the query.
     *
     * @returns The type of the query.
     *
     */
    QueryType GetQueryType(void) const { return static_cast<QueryType>((mFlags[0] & kOpCodeMask) >> kOpCodeOffset); }

    /**
     * This method sets the type of the query.
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
     * This method specifies in response message if the responding name server is an
     * authority for the domain name in question section.
     *
     * @returns True if Authoritative Answer flag (AA) is set in the header, false otherwise.
     *
     */
    bool IsAuthoritativeAnswerFlagSet(void) const { return (mFlags[0] & kAaFlagMask) == kAaFlagMask; }

    /**
     * This method clears the Authoritative Answer flag (AA) in the header.
     *
     */
    void ClearAuthoritativeAnswerFlag(void) { mFlags[0] &= ~kAaFlagMask; }

    /**
     * This method sets the Authoritative Answer flag (AA) in the header.
     *
     */
    void SetAuthoritativeAnswerFlag(void) { mFlags[0] |= kAaFlagMask; }

    /**
     * This method specifies if message is truncated.
     *
     * @returns True if Truncation flag (TC) is set in the header, false otherwise.
     *
     */
    bool IsTruncationFlagSet(void) const { return (mFlags[0] & kTcFlagMask) == kTcFlagMask; }

    /**
     * This method clears the Truncation flag (TC) in the header.
     *
     */
    void ClearTruncationFlag(void) { mFlags[0] &= ~kTcFlagMask; }

    /**
     * This method sets the Truncation flag (TC) in the header.
     *
     */
    void SetTruncationFlag(void) { mFlags[0] |= kTcFlagMask; }

    /**
     * This method specifies if resolver wants to direct the name server to pursue
     * the query recursively.
     *
     * @returns True if Recursion Desired flag (RD) is set in the header, false otherwise.
     *
     */
    bool IsRecursionDesiredFlagSet(void) const { return (mFlags[0] & kRdFlagMask) == kRdFlagMask; }

    /**
     * This method clears the Recursion Desired flag (RD) in the header.
     *
     */
    void ClearRecursionDesiredFlag(void) { mFlags[0] &= ~kRdFlagMask; }

    /**
     * This method sets the Recursion Desired flag (RD) in the header.
     *
     */
    void SetRecursionDesiredFlag(void) { mFlags[0] |= kRdFlagMask; }

    /**
     * This method denotes whether recursive query support is available in the name server.
     *
     * @returns True if Recursion Available flag (RA) is set in the header, false otherwise.
     */
    bool IsRecursionAvailableFlagSet(void) const { return (mFlags[1] & kRaFlagMask) == kRaFlagMask; }

    /**
     * This method clears the Recursion Available flag (RA) in the header.
     *
     */
    void ClearRecursionAvailableFlag(void) { mFlags[1] &= ~kRaFlagMask; }

    /**
     * This method sets the Recursion Available flag (RA) in the header.
     *
     */
    void SetRecursionAvailableFlag(void) { mFlags[1] |= kRaFlagMask; }

    /**
     * Defines response codes.
     *
     */
    enum Response
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
        kResponseBadName         = 20, ///< Bad name.
        kResponseBadAlg          = 21, ///< Bad algorithm.
        kResponseBadTruncation   = 22, ///< Bad truncation.
    };

    /**
     * This method returns the response code.
     *
     * @returns The response code from the header.
     *
     */
    Response GetResponseCode(void) const { return static_cast<Response>((mFlags[1] & kRCodeMask) >> kRCodeOffset); }

    /**
     * This method sets the response code.
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
     * This method converts a Response Code into a related `otError`.
     *
     * - kResponseSuccess (0)         : Success (no error condition)                    -> OT_ERROR_NONE
     * - kResponseFormatError (1)     : Server unable to interpret due to format error  -> OT_ERROR_PARSE
     * - kResponseServerFailure (2)   : Server encountered an internal failure          -> OT_ERROR_FAILED
     * - kResponseNameError (3)       : Name that ought to exist, does not exists       -> OT_ERROR_NOT_FOUND
     * - kResponseNotImplemented (4)  : Server does not support the query type (OpCode) -> OT_ERROR_NOT_IMPLEMENTED
     * - kResponseRefused (5)         : Server refused for policy/security reasons      -> OT_ERROR_SECURITY
     * - kResponseNameExists (6)      : Some name that ought not to exist, does exist   -> OT_ERROR_DUPLICATED
     * - kResponseRecordExists (7)    : Some RRset that ought not to exist, does exist  -> OT_ERROR_DUPLICATED
     * - kResponseRecordNotExists (8) : Some RRset that ought to exist, does not exist  -> OT_ERROR_NOT_FOUND
     * - kResponseNotAuth (9)         : Service is not authoritative for zone           -> OT_ERROR_SECURITY
     * - kResponseNotZone (10)        : A name is not in the zone                       -> OT_ERROR_PARSE
     * - kResponseBadName (20)        : Bad name                                        -> OT_ERROR_PARSE
     * - kResponseBadAlg (21)         : Bad algorithm                                   -> OT_ERROR_SECURITY
     * - kResponseBadTruncation (22)  : Bad truncation                                  -> OT_ERROR_PARSE
     * - Other error                                                                    -> OT_ERROR_FAILED
     *
     * @param[in] aResponse  The response code to convert.
     *
     */
    static otError ResponseCodeToError(Response aResponse);

    /**
     * This method returns the number of entries in question section.
     *
     * @returns The number of entries in question section.
     *
     */
    uint16_t GetQuestionCount(void) const { return HostSwap16(mQdCount); }

    /**
     * This method sets the number of entries in question section.
     *
     * @param[in]  aCount The number of entries in question section.
     *
     */
    void SetQuestionCount(uint16_t aCount) { mQdCount = HostSwap16(aCount); }

    /**
     * This method returns the number of entries in answer section.
     *
     * @returns The number of entries in answer section.
     *
     */
    uint16_t GetAnswerCount(void) const { return HostSwap16(mAnCount); }

    /**
     * This method sets the number of entries in answer section.
     *
     * @param[in]  aCount The number of entries in answer section.
     *
     */
    void SetAnswerCount(uint16_t aCount) { mAnCount = HostSwap16(aCount); }

    /**
     * This method returns the number of entries in authority records section.
     *
     * @returns The number of entries in authority records section.
     *
     */
    uint16_t GetAuthorityRecordCount(void) const { return HostSwap16(mNsCount); }

    /**
     * This method sets the number of entries in authority records section.
     *
     * @param[in]  aCount The number of entries in authority records section.
     *
     */
    void SetAuthorityRecordCount(uint16_t aCount) { mNsCount = HostSwap16(aCount); }

    /**
     * This method returns the number of entries in additional records section.
     *
     * @returns The number of entries in additional records section.
     *
     */
    uint16_t GetAdditionalRecordCount(void) const { return HostSwap16(mArCount); }

    /**
     * This method sets the number of entries in additional records section.
     *
     * @param[in]  aCount The number of entries in additional records section.
     *
     */
    void SetAdditionalRecordCount(uint16_t aCount) { mArCount = HostSwap16(aCount); }

private:
    /**
     * Protocol Constants (RFC 1035).
     *
     */
    enum
    {
        kQrFlagOffset = 7,                     // QR Flag offset.
        kQrFlagMask   = 0x01 << kQrFlagOffset, // QR Flag mask.
        kOpCodeOffset = 3,                     // OpCode field offset.
        kOpCodeMask   = 0x0f << kOpCodeOffset, // OpCode field mask.
        kAaFlagOffset = 2,                     // AA Flag offset.
        kAaFlagMask   = 0x01 << kAaFlagOffset, // AA Flag mask.
        kTcFlagOffset = 1,                     // TC Flag offset.
        kTcFlagMask   = 0x01 << kTcFlagOffset, // TC Flag mask.
        kRdFlagOffset = 0,                     // RD Flag offset.
        kRdFlagMask   = 0x01 << kRdFlagOffset, // RD Flag mask.
        kRaFlagOffset = 7,                     // RA Flag offset.
        kRaFlagMask   = 0x01 << kRaFlagOffset, // RA Flag mask.
        kRCodeOffset  = 0,                     // RCODE field offset.
        kRCodeMask    = 0x0f << kRCodeOffset,  // RCODE field mask.
    };

    uint16_t mMessageId; // Message identifier for requester to match up replies to outstanding queries.
    uint8_t  mFlags[2];  // DNS header flags.
    uint16_t mQdCount;   // Number of entries in the question section.
    uint16_t mAnCount;   // Number of entries in the answer section.
    uint16_t mNsCount;   // Number of entries in the authority records section.
    uint16_t mArCount;   // Number of entries in the additional records section.

} OT_TOOL_PACKED_END;

/**
 * This class implements DNS Update message header generation and parsing.
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
     * This method returns the number of records in Zone section.
     *
     * @returns The number of records in Zone section.
     *
     */
    uint16_t GetZoneRecordCount(void) const { return GetQuestionCount(); }

    /**
     * This method sets the number of records in Zone section.
     *
     * @param[in]  aCount The number of records in Zone section.
     *
     */
    void SetZoneRecordCount(uint16_t aCount) { SetQuestionCount(aCount); }

    /**
     * This method returns the number of records in Prerequisite section.
     *
     * @returns The number of records in Prerequisite section.
     *
     */
    uint16_t GetPrerequisiteRecordCount(void) const { return GetAnswerCount(); }

    /**
     * This method sets the number of records in Prerequisite section.
     *
     * @param[in]  aCount The number of records in Prerequisite section.
     *
     */
    void SetPrerequisiteRecordCount(uint16_t aCount) { SetAnswerCount(aCount); }

    /**
     * This method returns the number of records in Update section.
     *
     * @returns The number of records in Update section.
     *
     */
    uint16_t GetUpdateRecordCount(void) const { return GetAuthorityRecordCount(); }

    /**
     * This method sets the number of records in Update section.
     *
     * @param[in]  aCount The number of records in Update section.
     *
     */
    void SetUpdateRecordCount(uint16_t aCount) { SetAuthorityRecordCount(aCount); }

} OT_TOOL_PACKED_END;

/**
 * This class implement helper methods for encoding/decoding of DNS Names.
 *
 */
class Name
{
public:
    enum : uint8_t
    {
        kMaxLabelLength = 63,  ///< Max number of characters in a label.
        kMaxLength      = 255, ///< Max number of characters in a name.
    };

    /**
     * This static method encodes and appends a single name label to a message.
     *
     * The @p aLabel is assumed to contain a single name label as a C string (null-terminated). Unlike
     * `AppendMultipleLabels()` which parses the label string and treats it as sequence of multiple (dot-separated)
     * labels, this method always appends @p aLabel as a single whole label. This allows the label string to even
     * contain dot '.' character, which, for example, is useful for "Service Instance Names" where <Instance> portion
     * is a user-friendly name and can contain dot characters.
     *
     * @param[in] aLabel              The label string to append. MUST NOT be nullptr.
     * @param[in] aMessage            The message to append to.
     *
     * @retval OT_ERROR_NONE          Successfully encoded and appended the name label to @p aMessage.
     * @retval OT_ERROR_INVALID_ARGS  @p aLabel is not valid (e.g., label length is not within valid range).
     * @retval OT_ERROR_NO_BUFS       Insufficient available buffers to grow the message.
     *
     */
    static otError AppendLabel(const char *aLabel, Message &aMessage);

    /**
     * This static method encodes and appends a sequence of name labels to a given message.
     *
     * The @p aLabels must follow  "<label1>.<label2>.<label3>", i.e., a sequence of labels separated by dot '.' char.
     * E.g., "_http._tcp", "_http._tcp." (same as previous one), "host-1.test".
     *
     * This method validates that the @p aLabels is a valid name format, i.e., no empty label, and labels are
     * `kMaxLabelLength` (63) characters or less.
     *
     * @note This method NEVER adds a label terminator (empty label) to the message, even in the case where @p aLabels
     * ends with a dot character, e.g., "host-1.test." is treated same as "host-1.test".
     *
     * @param[in]  aLabels            A name label string. Can be nullptr (then treated as "").
     * @param[in]  aMessage           The message to which to append the encoded name.
     *
     * @retval OT_ERROR_NONE          Successfully encoded and appended the name label(s) to @p aMessage.
     * @retval OT_ERROR_INVALID_ARGS  Name label @p aLabels is not valid.
     * @retval OT_ERROR_NO_BUFS       Insufficient available buffers to grow the message.
     *
     */
    static otError AppendMultipleLabels(const char *aLabels, Message &aMessage);

    /**
     * This static method appends a name label terminator to a message.
     *
     * An encoded name is terminated by an empty label (a zero byte).
     *
     * @param[in] aMessage            The message to append to.
     *
     * @retval OT_ERROR_NONE          Successfully encoded and appended the terminator label to @p aMessage.
     * @retval OT_ERROR_NO_BUFS       Insufficient available buffers to grow the message.
     *
     */
    static otError AppendTerminator(Message &aMessage);

    /**
     * This static method appends a pointer type name label to a message.
     *
     * Pointer label is used for name compression. It allows an entire name or a list of labels at the end of an
     * encoded name to be replaced with a pointer to a prior occurrence of the same name within the message.
     *
     * @param[in] aOffset             The offset from the start of DNS header to use for pointer value.
     * @param[in] aMessage            The message to append to.
     *
     * @retval OT_ERROR_NONE          Successfully encoded and appended the pointer label to @p aMessage.
     * @retval OT_ERROR_NO_BUFS       Insufficient available buffers to grow the message.
     *
     */
    static otError AppendPointerLabel(uint16_t aOffset, Message &aMessage);

    /**
     * This static method encodes and appends a full name to a message.
     *
     * The @p aName must follow  "<label1>.<label2>.<label3>", i.e., a sequence of labels separated by dot '.' char.
     * E.g., "example.com", "example.com." (same as previous one), local.", "default.service.arpa", "." or "" (root).
     *
     * This method validates that the @p aName is a valid name format, i.e. no empty labels, and labels are
     * `kMaxLabelLength` (63) characters or less, and the name is `kMaxLength` (255) characters or less.
     *
     * @param[in]  aName              A name string. Can be nullptr (then treated as "." or root).
     * @param[in]  aMessage           The message to append to.
     *
     * @retval OT_ERROR_NONE          Successfully encoded and appended the name to @p aMessage.
     * @retval OT_ERROR_INVALID_ARGS  Name @p aName is not valid.
     * @retval OT_ERROR_NO_BUFS       Insufficient available buffers to grow the message.
     *
     */
    static otError AppendName(const char *aName, Message &aMessage);

    /**
     * This static method parses and skips over a full name in a message.
     *
     * @param[in]    aMessage         The message to parse the name from.
     * @param[inout] aOffset          On input the offset in @p aMessage pointing to the start of the name field.
     *                                On exit (when parsed successfully), @p aOffset is updated to point to the byte
     *                                after the end of name field.
     *
     * @retval OT_ERROR_NONE          Successfully parsed and skipped over name, @p Offset is updated.
     * @retval OT_ERROR_PARSE         Name could not be parsed (invalid format).
     *
     */
    static otError ParseName(const Message &aMessage, uint16_t &aOffset);

    /**
     * This static method reads a name label from a message.
     *
     * This method can be used to read labels one by one in a name. After a successful label read, @p aOffset is
     * updated to point to the start of the next label. When we reach the end of the name, OT_ERROR_NOT_FOUND is
     * returned. This method handles compressed names which use pointer labels. So as the labels in a name are read,
     * the @p aOffset may jump back in the message and at the end the @p aOffset does not necessarily point to the end
     * of the original name field.
     *
     * Unlike `ReadName()` which requires and verifies that the read label to contain no dot '.' character, this method
     * allows the read label to include any character.
     *
     * @param[in]    aMessage         The message to read the label from.
     * @param[inout] aOffset          On input, the offset in @p aMessage pointing to the start of the label to read.
     *                                On exit, when successfully read, @p aOffset is updated to point to the start of
     *                                the next label.
     * @param[in]    aHeaderOffset    The offset in @p aMessage to the start of the DNS header.
     * @param[out]   aLabelBuffer     A pointer to a char array to output the read label as a null-terminated C string.
     * @param[inout] aLabelLength     On input, the maximum number chars in @p aLabelBuffer array.
     *                                On output, when label is successfully read, @aLabelLength is updated to return
     *                                the label's length (number of chars in the label string, excluding the null char).
     *
     * @retval OT_ERROR_NONE          Successfully read the label and updated @p aLabelBuffer, @p aLabelLength, and
     *                                @p aOffset.
     * @retval OT_ERROR_NOT_FOUND     Reached the end of name and no more label to read.
     * @retval OT_ERROR_PARSE         Name could not be parsed (invalid format).
     * @retval OT_ERROR_NO_BUFS       Label could not fit in @p aLabelLength chars.
     *
     */
    static otError ReadLabel(const Message &aMessage,
                             uint16_t &     aOffset,
                             uint16_t       aHeaderOffset,
                             char *         aLabelBuffer,
                             uint8_t &      aLabelLength);

    /**
     * This static method reads a full name from a message.
     *
     * On successful read, the read name follows  "<label1>.<label2>.<label3>.", i.e., a sequence of labels separated by
     * dot '.' character. The read name will ALWAYS end with a dot.
     *
     * This method verifies that the read labels in message do not contain any dot character, otherwise it returns
     * `OT_ERROR_PARSE`).
     *
     * @param[in]    aMessage         The message to read the name from.
     * @param[inout] aOffset          On input, the offset in @p aMessage pointing to the start of the name field.
     *                                On exit (when parsed successfully), @p aOffset is updated to point to the byte
     *                                after the end of name field.
     * @param[in]    aHeaderOffset    The offset in @p aMessage to the start of the DNS header.
     * @param[out]   aNameBuffer      A pointer to a char array to output the read name as a null-terminated C string.
     * @param[inout] aNameBufferSize  The maximum number chars in @p aNameBuffer array.
     *
     * @retval OT_ERROR_NONE          Successfully read the name, @p aNameBuffer and @p Offset are updated.
     * @retval OT_ERROR_PARSE         Name could not be parsed (invalid format).
     * @retval OT_ERROR_NO_BUFS       Name could not fit in @p aNameBufferSize chars.
     *
     */
    static otError ReadName(const Message &aMessage,
                            uint16_t &     aOffset,
                            uint16_t       aHeaderOffset,
                            char *         aNameBuffer,
                            uint16_t       aNameBufferSize);

private:
    enum : char
    {
        kNullChar           = '\0',
        kLabelSeperatorChar = '.',
    };

    enum : uint8_t
    {
        // The first 2 bits of the encoded label specifies label type.
        //
        // - Value 00 indicates normal text label (lower 6-bits indicates the label length).
        // - Value 11 indicates pointer label type (lower 14-bits indicates the pointer offset).
        // - Values 01,10 are reserved (RFC 6891 recommends to not use)

        kLabelTypeMask    = 0xc0, // 0b1100_0000 (first two bits)
        kTextLabelType    = 0x00, // Text label type (00)
        kPointerLabelType = 0xc0, // Pointer label type - compressed name (11)
    };

    enum : uint16_t
    {
        kPointerLabelTypeUint16 = 0xc000, // Pointer label type as `uint16_t` mask (first 2 bits).
        kPointerLabelOffsetMask = 0x3fff, // Mask to get the offset field in a pointer label (lower 14 bits).
    };

    struct LabelIterator
    {
        enum : uint16_t
        {
            kUnsetNameEndOffset = 0, // Special value indicating `mNameEndOffset` is not yet set.
        };

        LabelIterator(const Message &aMessage, uint16_t aLabelOffset, uint16_t aHeaderOffset = 0)
            : mMessage(aMessage)
            , mHeaderOffset(aHeaderOffset)
            , mNextLabelOffset(aLabelOffset)
            , mNameEndOffset(kUnsetNameEndOffset)
        {
        }

        bool    IsEndOffsetSet(void) const { return (mNameEndOffset != kUnsetNameEndOffset); }
        otError GetNextLabel(void);
        otError ReadLabel(char *aLabelBuffer, uint8_t &aLabelLength, bool aAllowDotCharInLabel) const;

        const Message &mMessage;          // Message to read labels from.
        const uint16_t mHeaderOffset;     // Offset in `mMessage` to the start of DNS header.
        uint16_t       mLabelStartOffset; // Offset in `mMessage` to the first char of current label text.
        uint8_t        mLabelLength;      // Length of current label (number of chars).
        uint16_t       mNextLabelOffset;  // Offset in `mMessage` to the start of the next label.
        uint16_t       mNameEndOffset;    // Offset in `mMessage` to the byte after the end of domain name field.
    };

    Name(void) = default;

    static otError AppendLabel(const char *aLabel, uint8_t aLabelLength, Message &aMessage);
};

/**
 * This class implements Resource Record (RR) body format.
 *
 */
OT_TOOL_PACKED_BEGIN
class ResourceRecord
{
    friend class OptRecord;

public:
    /**
     * Resource Record Types.
     *
     */
    enum : uint16_t
    {
        kTypeZero = 0,   ///< Zero is used as a special indicator for the SIG RR (SIG(0) from RFC 2931).
        kTypeA    = 1,   ///< Address record (IPv4).
        kTypeSoa  = 6,   ///< Start of (zone of) authority.
        kTypePtr  = 12,  ///< PTR record.
        kTypeTxt  = 16,  ///< TXT record.
        kTypeSig  = 24,  ///< SIG record.
        kTypeKey  = 25,  ///< KEY record.
        kTypeAaaa = 28,  ///< IPv6 address record.
        kTypeSrv  = 33,  ///< SRV locator record.
        kTypeOpt  = 41,  ///< Option record.
        kTypeAny  = 255, ///< ANY record.
    };

    /**
     * Resource Record Class Codes.
     *
     */
    enum : uint16_t
    {
        kClassInternet = 1,   ///< Class code Internet (IN).
        kClassNone     = 254, ///< Class code None (NONE) - RFC 2136.
        kClassAny      = 255, ///< Class code Any (ANY).
    };

    /**
     * This method initializes the resource record by setting its type and class.
     *
     * This method only sets the type and class fields. Other fields (TTL and length) remain unchanged/uninitialized.
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
     * This method indicates whether the resources records matches a given type and class code.
     *
     * @param[in] aType   The resource record type to compare with.
     * @param[in] aClass  The resource record class code to compare with (default is `kClassInternet`).
     *
     * @returns TRUE if the resources records matches @p aType and @p aClass, FALSE otherwise.
     *
     */
    bool Matches(uint16_t aType, uint16_t aClass = kClassInternet)
    {
        return (mType == HostSwap16(aType)) && (mClass == HostSwap16(aClass));
    }

    /**
     * This method returns the type of the resource record.
     *
     * @returns The type of the resource record.
     *
     */
    uint16_t GetType(void) const { return HostSwap16(mType); }

    /**
     * This method sets the type of the resource record.
     *
     * @param[in]  aType The type of the resource record.
     *
     */
    void SetType(uint16_t aType) { mType = HostSwap16(aType); }

    /**
     * This method returns the class of the resource record.
     *
     * @returns The class of the resource record.
     *
     */
    uint16_t GetClass(void) const { return HostSwap16(mClass); }

    /**
     * This method sets the class of the resource record.
     *
     * @param[in]  aClass The class of the resource record.
     *
     */
    void SetClass(uint16_t aClass) { mClass = HostSwap16(aClass); }

    /**
     * This method returns the time to live field of the resource record.
     *
     * @returns The time to live field of the resource record.
     *
     */
    uint32_t GetTtl(void) const { return HostSwap32(mTtl); }

    /**
     * This method sets the time to live field of the resource record.
     *
     * @param[in]  aTtl The time to live field of the resource record.
     *
     */
    void SetTtl(uint32_t aTtl) { mTtl = HostSwap32(aTtl); }

    /**
     * This method returns the length of the resource record data.
     *
     * @returns The length of the resource record data.
     *
     */
    uint16_t GetLength(void) const { return HostSwap16(mLength); }

    /**
     * This method sets the length of the resource record data.
     *
     * @param[in]  aLength The length of the resource record data.
     *
     */
    void SetLength(uint16_t aLength) { mLength = HostSwap16(aLength); }

    /**
     * This method returns the size of (number of bytes) in resource record and its data RDATA section (excluding the
     * name field).
     *
     * @returns Size (number of bytes) of resource record and its data section (excluding the name field)
     *
     */
    uint32_t GetSize(void) const { return sizeof(ResourceRecord) + GetLength(); }

private:
    uint16_t mType;   // The type of the data in RDATA section.
    uint16_t mClass;  // The class of the data in RDATA section.
    uint32_t mTtl;    // Specifies the maximum time that the resource record may be cached.
    uint16_t mLength; // The length of RDATA section in bytes.

} OT_TOOL_PACKED_END;

/**
 * This class implements Resource Record body format of PTR type.
 *
 */
OT_TOOL_PACKED_BEGIN
class PtrRecord : public ResourceRecord
{
public:
    /**
     * This method initializes the PTR Resource Record by setting its type and class.
     *
     * Other record fields (TTL, length) remain unchanged/uninitialized.
     *
     * @param[in] aClass  The class of the resource record (default is `kClassInternet`).
     *
     */
    void Init(uint16_t aClass = kClassInternet) { ResourceRecord::Init(kTypePtr, aClass); }

} OT_TOOL_PACKED_END;

/**
 * This class implements Resource Record body format of TXT type.
 *
 */
OT_TOOL_PACKED_BEGIN
class TxtRecord : public ResourceRecord
{
public:
    /**
     * This method initializes the TXT Resource Record by setting its type and class.
     *
     * Other record fields (TTL, length) remain unchanged/uninitialized.
     *
     * @param[in] aClass  The class of the resource record (default is `kClassInternet`).
     *
     */
    void Init(uint16_t aClass = kClassInternet) { ResourceRecord::Init(kTypeTxt, aClass); }

} OT_TOOL_PACKED_END;

/**
 * This class implements Resource Record body format of AAAA type.
 *
 */
OT_TOOL_PACKED_BEGIN
class AaaaRecord : public ResourceRecord
{
public:
    /**
     * This method initializes the AAAA Resource Record by setting its type, class, and length.
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
     * This method sets the IPv6 address of the resource record.
     *
     * @param[in]  aAddress The IPv6 address of the resource record.
     *
     */
    void SetAddress(const Ip6::Address &aAddress) { mAddress = aAddress; }

    /**
     * This method returns the reference to IPv6 address of the resource record.
     *
     * @returns The reference to IPv6 address of the resource record.
     *
     */
    const Ip6::Address &GetAddress(void) const { return mAddress; }

private:
    Ip6::Address mAddress; // IPv6 Address of AAAA Resource Record.
} OT_TOOL_PACKED_END;

/**
 * This class implements Resource Record body format of SRV type (RFC 2782).
 *
 */
OT_TOOL_PACKED_BEGIN
class SrvRecord : public ResourceRecord
{
public:
    /**
     * This method initializes the SRV Resource Record by settings its type and class.
     *
     * Other record fields (TTL, length, propriety, weight, port, ...) remain unchanged/uninitialized.
     *
     * @param[in] aClass  The class of the resource record (default is `kClassInternet`).
     *
     */
    void Init(uint16_t aClass = kClassInternet) { ResourceRecord::Init(kTypeSrv, aClass); }

    /**
     * This method returns the SRV record's priority value.
     *
     * @returns The priority value.
     *
     */
    uint16_t GetPriority(void) const { return HostSwap16(mPriority); }

    /**
     * This method sets the SRV record's priority value.
     *
     * @param[in]  aPriority  The priority value.
     *
     */
    void SetPriority(uint16_t aPriority) { mPriority = HostSwap16(aPriority); }

    /**
     * This method returns the SRV record's weight value.
     *
     * @returns The weight value.
     *
     */
    uint16_t GetWeight(void) const { return HostSwap16(mWeight); }

    /**
     * This method sets the SRV record's weight value.
     *
     * @param[in]  aWeight  The weight value.
     *
     */
    void SetWeight(uint16_t aWeight) { mWeight = HostSwap16(aWeight); }

    /**
     * This method returns the SRV record's port number on the target host for this service.
     *
     * @returns The port number.
     *
     */
    uint16_t GetPort(void) const { return HostSwap16(mPort); }

    /**
     * This method sets the SRV record's port number on the target host for this service.
     *
     * @param[in]  aPort  The port number.
     *
     */
    void SetPort(uint16_t aPort) { mPort = HostSwap16(aPort); }

private:
    uint16_t mPriority;
    uint16_t mWeight;
    uint16_t mPort;
    // Followed by the target host domain name.

} OT_TOOL_PACKED_END;

/**
 * This class implements Resource Record body format of KEY type (RFC 2535).
 *
 */
OT_TOOL_PACKED_BEGIN
class KeyRecord : public ResourceRecord
{
public:
    /**
     * This enumeration defines protocol field values (RFC 2535 - section 3.1.3).
     *
     */
    enum : uint8_t
    {
        kProtocolTls    = 1, ///< TLS protocol code.
        kProtocolDnsSec = 3, ///< DNS security protocol code.
    };

    /**
     * This enumeration defines algorithm field values (RFC 8624 - section 3.1).
     *
     */
    enum : uint8_t
    {
        kAlgorithmEcdsaP256Sha256 = 13, ///< ECDSA-P256-SHA256 algorithm.
        kAlgorithmEcdsaP384Sha384 = 14, ///< ECDSA-P384-SHA384 algorithm.
        kAlgorithmEd25519         = 15, ///< ED25519 algorithm.
        kAlgorithmEd448           = 16, ///< ED448 algorithm.
    };

    /**
     * This enumeration type represents the use (or key type) flags (RFC 2535 - section 3.1.2).
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
     * This enumeration type represents key owner (or name type) flags (RFC 2535 - section 3.1.2).
     *
     */
    enum OwnerFlags : uint8_t
    {
        kOwnerUser     = 0x00, ///< Key is associated with a "user" or "account" at end entity.
        kOwnerZone     = 0x01, ///< Key is a zone key (used for data origin authentication).
        kOwnerNonZone  = 0x02, ///< Key is associated with a non-zone "entity".
        kOwnerReserved = 0x03, ///< Reserved for future use.
    };

    /**
     * This enumeration defines flag bits for the "signatory" flags (RFC 2137).
     *
     * The flags defined are for non-zone (`kOwnerNoneZone`) keys (RFC 2137 - section 3.1.3).
     *
     */
    enum : uint8_t
    {
        kSignatoryFlagZone    = 1 << 3, ///< Key is authorized to attach, detach, and move zones.
        kSignatoryFlagStrong  = 1 << 2, ///< Key is authorized to add and delete RRs even if RRs auth with other key.
        kSignatoryFlagUnique  = 1 << 1, ///< Key is authorized to add and update RRs for only a single owner name.
        kSignatoryFlagGeneral = 1 << 0, ///< If the other flags are zero, this is used to indicate it is an update key.
    };

    /**
     * This method initializes the KEY Resource Record by setting its type and class.
     *
     * Other record fields (TTL, length, flags, protocol, algorithm) remain unchanged/uninitialized.
     *
     * @param[in] aClass  The class of the resource record (default is `kClassInternet`).
     *
     */
    void Init(uint16_t aClass = kClassInternet) { ResourceRecord::Init(kTypeKey, aClass); }

    /**
     * This method gets the key use (or key type) flags.
     *
     * @returns The key use flags.
     *
     */
    UseFlags GetUseFlags(void) const { return static_cast<UseFlags>(mFlags[0] & kUseFlagsMask); }

    /**
     * This method gets the owner (or name type) flags.
     *
     * @returns The key owner flags.
     *
     */
    OwnerFlags GetOwnerFlags(void) const { return static_cast<OwnerFlags>(mFlags[0] & kOwnerFlagsMask); }

    /**
     * This method gets the signatory flags.
     *
     * @returns The signatory flags.
     *
     */
    uint8_t GetSignatoryFlags(void) const { return (mFlags[1] & kSignatoryFlagsMask); }

    /**
     * This method sets the flags field.
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
     * This method returns the KEY record's protocol value.
     *
     * @returns The protocol value.
     *
     */
    uint8_t GetProtocol(void) const { return mProtocol; }

    /**
     * This method sets the KEY record's protocol value.
     *
     * @param[in]  aProtocol  The protocol value.
     *
     */
    void SetProtocol(uint8_t aProtocol) { mProtocol = aProtocol; }

    /**
     * This method returns the KEY record's algorithm value.
     *
     * @returns The algorithm value.
     *
     */
    uint8_t GetAlgorithm(void) const { return mAlgorithm; }

    /**
     * This method sets the KEY record's algorithm value.
     *
     * @param[in]  aAlgorithm  The algorithm value.
     *
     */
    void SetAlgorithm(uint8_t aAlgorithm) { mAlgorithm = aAlgorithm; }

private:
    enum : uint8_t
    {
        kUseFlagsMask       = 0xc0, // top two bits in the first flag byte.
        kOwnerFlagsMask     = 0x03, // lowest two bits in the first flag byte.
        kSignatoryFlagsMask = 0x0f, // lower 4 bits in the second flag byte.
    };

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

/**
 * This class implements Resource Record body format of SIG type (RFC 2535 - section-4.1).
 *
 *
 */
OT_TOOL_PACKED_BEGIN
class SigRecord : public ResourceRecord, public Clearable<SigRecord>
{
public:
    /**
     * This method initializes the SIG Resource Record by setting its type and class.
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
     * This method returns the SIG record's type-covered value.
     *
     * @returns The type-covered value.
     *
     */
    uint16_t GetTypeCovered(void) const { return HostSwap16(mTypeCovered); }

    /**
     * This method sets the SIG record's type-covered value.
     *
     * @param[in]  aTypeCovered  The type-covered value.
     *
     */
    void SetTypeCovered(uint8_t aTypeCovered) { mTypeCovered = HostSwap16(aTypeCovered); }

    /**
     * This method returns the SIG record's algorithm value.
     *
     * @returns The algorithm value.
     *
     */
    uint8_t GetAlgorithm(void) const { return mAlgorithm; }

    /**
     * This method sets the SIG record's algorithm value.
     *
     * @param[in]  aAlgorithm  The algorithm value.
     *
     */
    void SetAlgorithm(uint8_t aAlgorithm) { mAlgorithm = aAlgorithm; }

    /**
     * This method returns the SIG record's labels-count (number of labels, not counting null label, in the original
     * name of the owner).
     *
     * @returns The labels-count value.
     *
     */
    uint8_t GetLabels(void) const { return mLabels; }

    /**
     * This method sets the SIG record's labels-count (number of labels, not counting null label, in the original
     * name of the owner).
     *
     * @param[in]  aLabels  The labels-count value.
     *
     */
    void SetLabels(uint8_t aLabels) { mLabels = aLabels; }

    /**
     * This method returns the SIG record's original TTL value.
     *
     * @returns The original TTL value.
     *
     */
    uint32_t GetOriginalTtl(void) const { return HostSwap32(mOriginalTtl); }

    /**
     * This method sets the SIG record's original TTL value.
     *
     * @param[in]  aOriginalTtl  The original TTL value.
     *
     */
    void SetOriginalTtl(uint32_t aOriginalTtl) { mOriginalTtl = HostSwap32(aOriginalTtl); }

    /**
     * This method returns the SIG record's expiration time value.
     *
     * @returns The expiration time value (seconds since Jan 1, 1970).
     *
     */
    uint32_t GetExpiration(void) const { return HostSwap32(mExpiration); }

    /**
     * This method sets the SIG record's expiration time value.
     *
     * @param[in]  aExpiration  The expiration time value (seconds since Jan 1, 1970).
     *
     */
    void SetExpiration(uint32_t aExpiration) { mExpiration = HostSwap32(aExpiration); }

    /**
     * This method returns the SIG record's inception time value.
     *
     * @returns The inception time value (seconds since Jan 1, 1970).
     *
     */
    uint32_t GetInception(void) const { return HostSwap32(mInception); }

    /**
     * This method sets the SIG record's inception time value.
     *
     * @param[in]  aInception  The inception time value (seconds since Jan 1, 1970).
     *
     */
    void SetInception(uint32_t aInception) { mInception = HostSwap32(aInception); }

    /**
     * This method returns the SIG record's key tag value.
     *
     * @returns The key tag value.
     *
     */
    uint16_t GetKeyTag(void) const { return HostSwap16(mKeyTag); }

    /**
     * This method sets the SIG record's key tag value.
     *
     * @param[in]  aKeyTag  The key tag value.
     *
     */
    void SetKeyTag(uint16_t aKeyTag) { mKeyTag = HostSwap16(aKeyTag); }

    /**
     * This method returns a pointer to the start of the record data fields.
     *
     * @returns A pointer to the start of the record data fields.
     *
     */
    const uint8_t *GetRecordData(void) const { return reinterpret_cast<const uint8_t *>(&mTypeCovered); }

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
 * This class implements DNS OPT Pseudo Resource Record header for EDNS(0) (RFC 6981 - Section 6.1).
 *
 */
OT_TOOL_PACKED_BEGIN
class OptRecord : public ResourceRecord
{
public:
    /**
     * This method initializes the OPT Resource Record by setting its type and clearing extended Response Code, version
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
     * This method gets the requester's UDP payload size (the number of bytes of the largest UDP payload that can be
     * delivered in the requester's network).
     *
     * The field is encoded in the CLASS field.
     *
     * @returns The UDP payload size.
     *
     */
    uint16_t GetUdpPayloadSize(void) const { return GetClass(); }

    /**
     * This method gets the requester's UDP payload size (the number of bytes of the largest UDP payload that can be
     * delivered in the requester's network).
     *
     * @param[in] aPayloadSize  The UDP payload size.
     *
     */
    void SetUdpPayloadSize(uint16_t aPayloadSize) { SetClass(aPayloadSize); }

    /**
     * This method gets the upper 8-bit of the extended 12-bit Response Code.
     *
     * Value of 0 indicates that an unextended Response code is in use.
     *
     * @return The upper 8-bit of the extended 12-bit Response Code.
     *
     */
    uint8_t GetExtendedResponseCode(void) const
    {
        return static_cast<uint8_t>((mTtl & kExtRCodeMask) >> kExtRCodeffset);
    }

    /**
     * This method sets the upper 8-bit of the extended 12-bit Response Code.
     *
     * Value of 0 indicates that an unextended Response code is in use.
     *
     * @param[in] aExtendedResponse The upper 8-bit of the extended 12-bit Response Code.
     *
     */
    void SetExtnededResponseCode(uint8_t aExtendedResponse)
    {
        mTtl &= ~kExtRCodeffset;
        mTtl |= (static_cast<uint32_t>(aExtendedResponse) << kExtRCodeffset);
    }

    /**
     * This method gets the Version field.
     *
     * @returns The version.
     *
     */
    uint8_t GetVersion(void) const { return static_cast<uint8_t>((mTtl & kVersionMask) >> kVersionOffset); }

    /**
     * This method set the Version field.
     *
     * @param[in] aVersion  The version.
     *
     */
    void SetVersion(uint8_t aVersion)
    {
        mTtl &= ~kVersionMask;
        mTtl |= (static_cast<uint32_t>(aVersion) << kVersionOffset);
    }

    /**
     * This method indicates whether the DNSSEC OK flag is set or not.
     *
     * @returns True if DNSSEC OK flag is set in the header, false otherwise.
     *
     */
    bool IsDnsSecurityFlagSet(void) const { return (mTtl & kDnsSecFlag) != 0; }

    /**
     * This method clears the DNSSEC OK bit flag.
     *
     */
    void ClearDnsSecurityFlag(void) { mTtl &= ~kDnsSecFlag; }

    /**
     * This method sets the DNSSEC OK bit flag.
     *
     */
    void SetDnsSecurityFlag(void) { mTtl |= kDnsSecFlag; }

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
    //  | DO|                           Z                               |
    //  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //
    // The variable data part of OPT RR can contain zero of more `Option`.

    enum : uint32_t
    {
        kExtRCodeffset = 24,                    // Extended RCODE field offset.
        kExtRCodeMask  = 0xf << kExtRCodeffset, // Extended RCODE field mask.
        kVersionOffset = 16,                    // Version field offset.
        kVersionMask   = 0xf << kVersionOffset, // Version field mask.
        kDnsSecFlag    = 1 << 15,               // DnsSec bit flag.
    };

} OT_TOOL_PACKED_END;

/**
 * This class implements the body of an Option in OPT Pseudo Resource Record (RFC 6981 - Section 6.1).
 *
 */
OT_TOOL_PACKED_BEGIN
class Option
{
public:
    /**
     * This enumeration defines option code values.
     *
     */
    enum : uint16_t
    {
        kUpdateLease = 2, ///< Update lease option code.
    };

    /**
     * This method returns the option code value.
     *
     * @returns The option code value.
     *
     */
    uint16_t GetOptionCode(void) const { return HostSwap16(mOptionCode); }

    /**
     * This method sets the option code value.
     *
     * @param[in]  aOptionCode  The option code value.
     *
     */
    void SetOptionCode(uint16_t aOptionCode) { mOptionCode = HostSwap16(aOptionCode); }

    /**
     * This method returns the option length value.
     *
     * @returns The option length (size of option data in bytes).
     *
     */
    uint16_t GetOptionLength(void) const { return HostSwap16(mOptionLength); }

    /**
     * This method sets the option length value.
     *
     * @param[in]  aOptionLength  The option length (size of option data in bytes).
     *
     */
    void SetOptionLength(uint16_t aOptionLength) { mOptionLength = HostSwap16(aOptionLength); }

    /**
     * This method returns the size of (number of bytes) in the Option and its data.
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
 * This class implements an Update Lease Option body.
 *
 * This implementation is intended for use in Dynamic DNS Update Lease Requests and Responses as specified in
 * https://tools.ietf.org/html/draft-sekar-dns-ul-02.
 *
 */
OT_TOOL_PACKED_BEGIN
class LeaseOption : public Option
{
public:
    enum : uint16_t
    {
        kOptionLength = sizeof(uint32_t) + sizeof(uint32_t), ///< Option length (lease and key lease values)
    };

    /**
     * This method initialize the Update Lease Option by setting the Option Code and Option Length.
     *
     * The lease and key lease intervals remain unchanged/uninitialized.
     *
     */
    void Init(void)
    {
        SetOptionCode(kUpdateLease);
        SetOptionLength(kOptionLength);
    }

    /**
     * This method returns the Update Lease OPT record's lease interval value.
     *
     * @returns The lease interval value (in seconds).
     *
     */
    uint32_t GetLeaseInterval(void) const { return HostSwap32(mLeaseInterval); }

    /**
     * This method sets the Update Lease OPT record's lease interval value.
     *
     * @param[in]  aLeaseInterval  The lease interval value.
     *
     */
    void SetLeaseInterval(uint32_t aLeaseInterval) { mLeaseInterval = HostSwap32(aLeaseInterval); }

    /**
     * This method returns the Update Lease OPT record's key lease interval value.
     *
     * @returns The key lease interval value (in seconds).
     *
     */
    uint32_t GetKeyLeaseInterval(void) const { return HostSwap32(mKeyLeaseInterval); }

    /**
     * This method sets the Update Lease OPT record's key lease interval value.
     *
     * @param[in]  aKeyLeaseInterval  The key lease interval value (in seconds).
     *
     */
    void SetKeyLeaseInterval(uint32_t aKeyLeaseInterval) { mKeyLeaseInterval = HostSwap32(aKeyLeaseInterval); }

private:
    uint32_t mLeaseInterval;
    uint32_t mKeyLeaseInterval;
} OT_TOOL_PACKED_END;

/**
 * This class implements Question format.
 *
 */
OT_TOOL_PACKED_BEGIN
class Question
{
public:
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
     * This method returns the type of the question.
     *
     * @returns The type of the question.
     *
     */
    uint16_t GetType(void) const { return HostSwap16(mType); }

    /**
     * This method sets the type of the question.
     *
     * @param[in]  aType The type of the question.
     *
     */
    void SetType(uint16_t aType) { mType = HostSwap16(aType); }

    /**
     * This method returns the class of the question.
     *
     * @returns The class of the question.
     *
     */
    uint16_t GetClass(void) const { return HostSwap16(mClass); }

    /**
     * This method sets the class of the question.
     *
     * @param[in]  aClass The class of the question.
     *
     */
    void SetClass(uint16_t aClass) { mClass = HostSwap16(aClass); }

    /**
     * This method appends the question data to the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the question data.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to grow the message.
     *
     */
    otError AppendTo(Message &aMessage) const { return aMessage.Append(*this); }

private:
    uint16_t mType;  // The type of the data in question section.
    uint16_t mClass; // The class of the data in question section.

} OT_TOOL_PACKED_END;

/**
 * This class implements Zone section body for DNS Update (RFC 2136 - section 2.3).
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
} // namespace ot

#endif // DNS_HEADER_HPP_
