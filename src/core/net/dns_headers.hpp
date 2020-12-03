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
        kResponseSuccess        = 0,
        kResponseFormatError    = 1,
        kResponseServerFailure  = 2,
        kResponseNameError      = 3,
        kResponseNotImplemented = 4,
        kResponseRefused        = 5,
        kResponseNotAuth        = 9,
        kResponseNotZone        = 10,
        kResponseBadName        = 20,
        kResponseBadAlg         = 21,
        kResponseBadTruncation  = 22,
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
    uint16_t GetAuthorityRecordsCount(void) const { return HostSwap16(mNsCount); }

    /**
     * This method sets the number of entries in authority records section.
     *
     * @param[in]  aCount The number of entries in authority records section.
     *
     */
    void SetAuthorityRecordsCount(uint16_t aCount) { mNsCount = HostSwap16(aCount); }

    /**
     * This method returns the number of entries in additional records section.
     *
     * @returns The number of entries in additional records section.
     *
     */
    uint16_t GetAdditionalRecordsCount(void) const { return HostSwap16(mArCount); }

    /**
     * This method sets the number of entries in additional records section.
     *
     * @param[in]  aCount The number of entries in additional records section.
     *
     */
    void SetAdditionalRecordsCount(uint16_t aCount) { mArCount = HostSwap16(aCount); }

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
 * This class implements Resource Record body format (RR).
 *
 */
OT_TOOL_PACKED_BEGIN
class ResourceRecord
{
public:
    /**
     * Resource Record Types.
     *
     */
    enum : uint16_t
    {
        kTypeA    = 1,  ///< Address record (IPv4).
        kTypePtr  = 12, ///< PTR record.
        kTypeTxt  = 16, ///< TXT record.
        kTypeSrv  = 33, ///< SRV locator record.
        kTypeAaaa = 28, ///< IPv6 address record.
        kTypeKey  = 25, ///< Key record.
        kTypeOpt  = 41, ///< Option record.
    };

    /**
     * Resource Record Class Codes.
     *
     */
    enum : uint16_t
    {
        kClassInternet = 1, ///< Class code Internet (IN).
    };

    /**
     * This method initializes the resource record.
     *
     * @param[in] aType   The type of the resource record.
     * @param[in] aClass  The class of the resource record (default is `kClassInternet`).
     * @param[in] aTtl    The time to live field of the resource record (default is zero).
     *
     */
    void Init(uint16_t aType, uint16_t aClass = kClassInternet, uint32_t aTtl = 0);

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
     * This method returns the length of the resource record.
     *
     * @returns The length of the resource record.
     */
    uint16_t GetLength(void) const { return HostSwap16(mLength); }

    /**
     * This method sets the length of the resource record.
     *
     * @param[in]  aLength The length of the resource record.
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
 * This class implements Resource Record body format of AAAA type.
 *
 */
OT_TOOL_PACKED_BEGIN
class ResourceRecordAaaa : public ResourceRecord
{
public:
    /**
     * This method initializes the AAAA Resource Record.
     *
     */
    void Init(void);

    /**
     * This method sets the IPv6 address of the resource record.
     *
     * @param[in]  aAddress The IPv6 address of the resource record.
     *
     */
    void SetAddress(Ip6::Address &aAddress) { mAddress = aAddress; }

    /**
     * This method returns the reference to IPv6 address of the resource record.
     *
     * @returns The reference to IPv6 address of the resource record.
     *
     */
    Ip6::Address &GetAddress(void) { return mAddress; }

private:
    Ip6::Address mAddress; // IPv6 Address of AAAA Resource Record.
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
    Question(uint16_t aType, uint16_t aClass)
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

private:
    uint16_t mType;  // The type of the data in question section.
    uint16_t mClass; // The class of the data in question section.

} OT_TOOL_PACKED_END;

/**
 * This class implements Question format of AAAA type.
 *
 */
class QuestionAaaa : public Question
{
public:
    /**
     * Default constructor for AAAA Question.
     *
     */
    QuestionAaaa(void)
        : Question(kType, kClass)
    {
    }

    /**
     * This method appends request data to the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the bytes.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to grow the message.
     *
     */
    otError AppendTo(Message &aMessage) const { return aMessage.Append(*this); }

private:
    enum
    {
        kType  = 0x1C, // AAAA Resource Record type.
        kClass = 0x01, // The value of the Internet class.
    };
};

/**
 * @}
 *
 */

} // namespace Dns
} // namespace ot

#endif // DNS_HEADER_HPP_
