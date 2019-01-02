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

#include "utils/wrap_string.h"

#include "common/encoding.hpp"
#include "common/message.hpp"

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

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
 * This class implements DNS header generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Header
{
public:
    /**
     * Default constructor for DNS Header.
     *
     */
    Header(void) { memset(this, 0, sizeof(*this)); }

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
        kQrFlagOffset = 7,                     ///< QR Flag offset.
        kQrFlagMask   = 0x01 << kQrFlagOffset, ///< QR Flag mask.
        kOpCodeOffset = 3,                     ///< OpCode field offset.
        kOpCodeMask   = 0x0f << kOpCodeOffset, ///< OpCode field mask.
        kAaFlagOffset = 2,                     ///< AA Flag offset.
        kAaFlagMask   = 0x01 << kAaFlagOffset, ///< AA Flag mask.
        kTcFlagOffset = 1,                     ///< TC Flag offset.
        kTcFlagMask   = 0x01 << kTcFlagOffset, ///< TC Flag mask.
        kRdFlagOffset = 0,                     ///< RD Flag offset.
        kRdFlagMask   = 0x01 << kRdFlagOffset, ///< RD Flag mask.

        kRaFlagOffset = 7,                     ///< RA Flag offset.
        kRaFlagMask   = 0x01 << kRaFlagOffset, ///< RA Flag mask.
        kRCodeOffset  = 0,                     ///< RCODE field offset.
        kRCodeMask    = 0x0f << kRCodeOffset,  ///< RCODE field mask.
    };

    uint16_t
             mMessageId; ///< A message identifier that is used by the requester to match up replies to outstanding queries.
    uint8_t  mFlags[2]; ///< DNS header flags.
    uint16_t mQdCount;  ///< A number specifying the number of entries in the question section.
    uint16_t mAnCount;  ///< A number specifying the number of entries in the answer section.
    uint16_t mNsCount;  ///< A number specifying the number of entries in the authority records section.
    uint16_t mArCount;  ///< A number specifying the number of entries in the additional records section.

} OT_TOOL_PACKED_END;

/**
 * This class implements Resource Record body format (RR).
 *
 */
OT_TOOL_PACKED_BEGIN
class ResourceRecord
{
public:
    /**
     * This method returns the type of the resource record.
     *
     * @returns The type of the resource record.
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

private:
    uint16_t mType;   ///< The type of the data in RDATA section.
    uint16_t mClass;  ///< The class of the data in RDATA section.
    uint32_t mTtl;    ///< Specifies the maximum time that the resource record may be cached.
    uint16_t mLength; ///< The length of RDATA section in bytes.

} OT_TOOL_PACKED_END;

/**
 * This class implements Resource Record body format of AAAA type.
 *
 */
OT_TOOL_PACKED_BEGIN
class ResourceRecordAaaa : public ResourceRecord
{
public:
    enum
    {
        kType   = 0x1C, ///< AAAA Resource Record type.
        kClass  = 0x01, ///< The value of the Internet class.
        kLength = 16,   ///< Size of the AAAA Resource Record type.
    };

    /**
     * This method initializes the AAAA Resource Record.
     *
     */
    void Init(void)
    {
        ResourceRecord::SetType(kType);
        ResourceRecord::SetClass(kClass);
        ResourceRecord::SetTtl(0);
        ResourceRecord::SetLength(kLength);
        memset(&mAddress, 0, sizeof(mAddress));
    }

    /**
     * This method sets the IPv6 address of the resource record.
     *
     * @param[in]  aAddress The IPv6 address of the resource record.
     *
     */
    void SetAddress(otIp6Address &aAddress) { mAddress = aAddress; }

    /**
     * This method returns the reference to IPv6 address of the resource record.
     *
     * @returns The reference to IPv6 address of the resource record.
     */
    otIp6Address &GetAddress(void) { return mAddress; }

private:
    otIp6Address mAddress; ///< IPv6 Address of AAAA Resource Record.

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
    };

    /**
     * This method returns the type of the question.
     *
     * @returns The type of the question.
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
    uint16_t mType;  ///< The type of the data in question section.
    uint16_t mClass; ///< The class of the data in question section.

} OT_TOOL_PACKED_END;

/**
 * This class implements Question format of AAAA type.
 *
 */
class QuestionAaaa : public Question
{
public:
    enum
    {
        kType  = 0x1C, ///< AAAA Resource Record type.
        kClass = 0x01, ///< The value of the Internet class.
    };

    /**
     * Default constructor for AAAA Question.
     *
     */
    QuestionAaaa(void)
        : Question(kType, kClass){};

    /**
     * This method appends request data to the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the bytes.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to grow the message.
     *
     */
    otError AppendTo(Message &aMessage) const { return aMessage.Append(this, sizeof(*this)); };
};

/**
 * @}
 *
 */

} // namespace Dns
} // namespace ot

#endif // DNS_HEADER_HPP_
