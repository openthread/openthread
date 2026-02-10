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
 *   This file implements the CoAP message generation and parsing.
 */

#include "coap_message.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace Coap {

uint16_t BlockSizeFromExponent(BlockSzx aBlockSzxq)
{
    static constexpr uint8_t kBlockSzxBase = 4;

    return static_cast<uint16_t>(1 << (static_cast<uint8_t>(aBlockSzxq) + kBlockSzxBase));
}

//---------------------------------------------------------------------------------------------------------------------
// `Token`

Error Token::SetToken(const uint8_t *aBytes, uint8_t aLength)
{
    Error error = kErrorNone;

    VerifyOrExit(aLength <= kMaxLength, error = kErrorInvalidArgs);

    mLength = aLength;
    memcpy(m8, aBytes, aLength);

exit:
    return error;
}

bool Token::operator==(const Token &aOther) const
{
    bool isEqual = false;

    VerifyOrExit(IsValid());
    VerifyOrExit(mLength == aOther.mLength);

    isEqual = (memcmp(m8, aOther.m8, mLength) == 0);

exit:
    return isEqual;
}

Error Token::GenerateRandom(uint8_t aLength)
{
    Error error;

    VerifyOrExit(aLength <= kMaxLength, error = kErrorInvalidArgs);
    mLength = aLength;
    error   = Random::Crypto::FillBuffer(m8, mLength);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// `HeaderInfo`

bool HeaderInfo::IsRequest(void) const { return IsValueInRange<uint8_t>(mCode, kCodeGet, kCodeDelete); }

bool HeaderInfo::IsConfirmablePostRequest(void) const { return IsConfirmable() && IsPostRequest(); }

bool HeaderInfo::IsNonConfirmablePostRequest(void) const { return IsNonConfirmable() && IsPostRequest(); }

//---------------------------------------------------------------------------------------------------------------------
// `Message`

Error Message::Init(Type aType, Code aCode) { return Init(aType, aCode, 0); }

Error Message::Init(Type aType, Code aCode, uint16_t aMessageId)
{
    Header header;

    // Erase any previously written content in the message.
    IgnoreError(SetLength(0));

    SetOffset(0);
    SetHeaderOffset(0);

    header.Clear();
    header.SetVersion(Header::kVersion1);
    header.SetType(aType);
    header.SetCode(aCode);
    header.SetMessageId(aMessageId);

    return Append(header);
}

Error Message::Init(Type aType, Code aCode, Uri aUri)
{
    Error error;

    SuccessOrExit(error = Init(aType, aCode));
    SuccessOrExit(error = WriteRandomToken(Token::kDefaultLength));
    SuccessOrExit(error = AppendUriPathOptions(PathForUri(aUri)));

exit:
    return error;
}

Error Message::InitAsPost(const Ip6::Address &aDestination, Uri aUri)
{
    return Init(aDestination.IsMulticast() ? kTypeNonConfirmable : kTypeConfirmable, kCodePost, aUri);
}

Error Message::InitAsResponse(Type aType, Code aCode, const Message &aRequest)
{
    Error error;

    SuccessOrExit(error = Init(aType, aCode, aRequest.ReadMessageId()));
    error = WriteTokenFromMessage(aRequest);

exit:
    return error;
}

Error Message::ReadHeader(Header &aHeader) const
{
    Error error;

    SuccessOrExit(error = Read(GetHeaderOffset(), aHeader));
    VerifyOrExit(aHeader.GetVersion() == Header::kVersion1, error = kErrorParse);
    VerifyOrExit(aHeader.GetTokenLength() <= Token::kMaxLength, error = kErrorParse);

exit:
    return error;
}

void Message::WriteHeader(const Header &aHeader) { Write(GetHeaderOffset(), aHeader); }

Error Message::ParseHeaderInfo(HeaderInfo &aInfo) const
{
    Error  error;
    Header header;

    aInfo.Clear();

    SuccessOrExit(error = ReadHeader(header));

    aInfo.mType      = header.GetType();
    aInfo.mCode      = header.GetCode();
    aInfo.mMessageId = header.GetMessageId();

    error = ReadToken(header, aInfo.mToken);

exit:
    return error;
}

uint8_t Message::ReadType(void) const
{
    uint8_t type = 0;
    Header  header;

    SuccessOrExit(ReadHeader(header));
    type = header.GetType();

exit:
    return type;
}

void Message::WriteType(Type aType)
{
    Header header;

    SuccessOrExit(ReadHeader(header));
    header.SetType(aType);
    WriteHeader(header);

exit:
    return;
}

uint8_t Message::ReadCode(void) const
{
    uint8_t code = 0;
    Header  header;

    SuccessOrExit(ReadHeader(header));
    code = header.GetCode();

exit:
    return code;
}

void Message::WriteCode(Code aCode)
{
    Header header;

    SuccessOrExit(ReadHeader(header));
    header.SetCode(aCode);
    WriteHeader(header);

exit:
    return;
}

uint16_t Message::ReadMessageId(void) const
{
    uint16_t messageId = 0;
    Header   header;

    SuccessOrExit(ReadHeader(header));
    messageId = header.GetMessageId();

exit:
    return messageId;
}

void Message::WriteMessageId(uint16_t aMessageId)
{
    Header header;

    SuccessOrExit(ReadHeader(header));
    header.SetMessageId(aMessageId);
    WriteHeader(header);

exit:
    return;
}

uint8_t Message::WriteExtendedOptionField(uint16_t aValue, uint8_t *&aBuffer)
{
    /*
     * Encodes a CoAP Option header field (Option Delta/Length) per
     * RFC 7252. The returned value is a 4-bit unsigned integer. Extended fields
     * (if needed) are written into the given buffer `aBuffer` and the pointer
     * would also be updated.
     *
     * If `aValue < 13 (kOption1ByteExtensionOffset)`, it is returned as is
     * (no extension).
     *
     * If `13 <= aValue < 269 (kOption2ByteExtensionOffset)`, one-byte
     * extension is used, and the value minus 13 is written in `aBuffer` as an
     * 8-bit unsigned integer, and `13 (kOption1ByteExtension)` is returned.
     *
     * If `269 <= aValue`, two-byte extension is used and the value minis 269
     * is written as a 16-bit unsigned integer and `14 (kOption2ByteExtension)`
     * is returned.
     */

    uint8_t rval;

    if (aValue < kOption1ByteExtensionOffset)
    {
        rval = static_cast<uint8_t>(aValue);
    }
    else if (aValue < kOption2ByteExtensionOffset)
    {
        rval     = kOption1ByteExtension;
        *aBuffer = static_cast<uint8_t>(aValue - kOption1ByteExtensionOffset);
        aBuffer += sizeof(uint8_t);
    }
    else
    {
        rval = kOption2ByteExtension;
        BigEndian::WriteUint16(aValue - kOption2ByteExtensionOffset, aBuffer);
        aBuffer += sizeof(uint16_t);
    }

    return rval;
}

Error Message::AppendOptionHeader(uint16_t aNumber, uint16_t aLength)
{
    // Appends a CoAP Option header field (Option Delta/Length) per RFC 7252.

    Error            error;
    Option::Iterator iterator;
    uint16_t         lastNumber;
    uint16_t         delta;
    uint8_t          header[kMaxOptionHeaderSize];
    uint16_t         headerLength;
    uint8_t         *cur;

    // Parses the already appended options in the message
    // to determine the last option number. Also ensures
    // that "payload marker" is not appended.

    SuccessOrExit(error = iterator.Init(*this));

    lastNumber = 0;

    while (!iterator.IsDone())
    {
        lastNumber = iterator.GetOption()->GetNumber();
        SuccessOrExit(error = iterator.Advance());
    }

    VerifyOrExit(!iterator.HasPayloadMarker(), error = kErrorParse);

    VerifyOrExit(aNumber >= lastNumber, error = kErrorInvalidArgs);
    delta = aNumber - lastNumber;

    cur = &header[1];

    header[0] = static_cast<uint8_t>(WriteExtendedOptionField(delta, cur) << kOptionDeltaOffset);
    header[0] |= static_cast<uint8_t>(WriteExtendedOptionField(aLength, cur) << kOptionLengthOffset);

    headerLength = static_cast<uint16_t>(cur - header);

    SuccessOrExit(error = AppendBytes(header, headerLength));

exit:
    return error;
}

Error Message::AppendOption(uint16_t aNumber, uint16_t aLength, const void *aValue)
{
    Error error = kErrorNone;

    SuccessOrExit(error = AppendOptionHeader(aNumber, aLength));
    SuccessOrExit(error = AppendBytes(aValue, aLength));

exit:
    return error;
}

Error Message::AppendOptionFromMessage(uint16_t aNumber, uint16_t aLength, const Message &aMessage, uint16_t aOffset)
{
    Error error = kErrorNone;

    SuccessOrExit(error = AppendOptionHeader(aNumber, aLength));
    SuccessOrExit(error = AppendBytesFromMessage(aMessage, aOffset, aLength));

exit:
    return error;
}

Error Message::AppendUintOption(uint16_t aNumber, uint32_t aValue)
{
    uint8_t        buffer[sizeof(uint32_t)];
    const uint8_t *value  = &buffer[0];
    uint16_t       length = sizeof(uint32_t);

    BigEndian::WriteUint32(aValue, buffer);

    while ((length > 0) && (value[0] == 0))
    {
        value++;
        length--;
    }

    return AppendOption(aNumber, length, value);
}

Error Message::AppendStringOption(uint16_t aNumber, const char *aValue)
{
    return AppendOption(aNumber, static_cast<uint16_t>(strlen(aValue)), aValue);
}

Error Message::AppendUriPathOptions(const char *aUriPath)
{
    Error       error = kErrorNone;
    const char *cur   = aUriPath;
    const char *end;

    while ((end = StringFind(cur, '/')) != nullptr)
    {
        SuccessOrExit(error = AppendOption(kOptionUriPath, static_cast<uint16_t>(end - cur), cur));
        cur = end + 1;
    }

    SuccessOrExit(error = AppendStringOption(kOptionUriPath, cur));

exit:
    return error;
}

Error Message::ReadUriPathOptions(UriPathStringBuffer &aUriPath) const
{
    char            *curUriPath = aUriPath;
    Error            error      = kErrorNone;
    Option::Iterator iterator;

    SuccessOrExit(error = iterator.Init(*this, kOptionUriPath));

    while (!iterator.IsDone())
    {
        uint16_t optionLength = iterator.GetOption()->GetLength();

        if (curUriPath != aUriPath)
        {
            *curUriPath++ = '/';
        }

        VerifyOrExit(curUriPath + optionLength < GetArrayEnd(aUriPath), error = kErrorParse);

        IgnoreError(iterator.ReadOptionValue(curUriPath));
        curUriPath += optionLength;

        SuccessOrExit(error = iterator.Advance(kOptionUriPath));
    }

    *curUriPath = kNullChar;

exit:
    return error;
}

Error Message::AppendUriQueryOptions(const char *aUriQuery)
{
    Error       error = kErrorNone;
    const char *cur   = aUriQuery;
    const char *end;

    while ((end = StringFind(cur, '&')) != nullptr)
    {
        SuccessOrExit(error = AppendOption(kOptionUriQuery, static_cast<uint16_t>(end - cur), cur));
        cur = end + 1;
    }

    SuccessOrExit(error = AppendStringOption(kOptionUriQuery, cur));

exit:
    return error;
}

Error Message::AppendBlockOption(uint16_t aBlockOptionNumber, const BlockInfo &aInfo)
{
    Error    error;
    uint32_t encoded;

    switch (aBlockOptionNumber)
    {
    case kOptionBlock1:
    case kOptionBlock2:
        break;
    default:
        ExitNow(error = kErrorInvalidArgs);
    }

    VerifyOrExit(aInfo.mBlockSzx <= kBlockSzx1024, error = kErrorInvalidArgs);
    VerifyOrExit(aInfo.mBlockNumber < kBlockNumMax, error = kErrorInvalidArgs);

    encoded = aInfo.mBlockSzx;
    encoded |= static_cast<uint32_t>(aInfo.mMoreBlocks << kBlockMOffset);
    encoded |= aInfo.mBlockNumber << kBlockNumOffset;

    error = AppendUintOption(aBlockOptionNumber, encoded);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

Error Message::ReadBlockOptionValues(uint16_t aBlockOptionNumber, BlockInfo &aInfo) const
{
    Error            error;
    uint8_t          buf[kMaxOptionHeaderSize] = {0};
    Option::Iterator iterator;

    switch (aBlockOptionNumber)
    {
    case kOptionBlock1:
    case kOptionBlock2:
        break;
    default:
        ExitNow(error = kErrorInvalidArgs);
    }

    SuccessOrExit(error = iterator.Init(*this, aBlockOptionNumber));
    SuccessOrExit(error = iterator.ReadOptionValue(buf));

    switch (iterator.GetOption()->GetLength())
    {
    case 0:
    case 1:
        aInfo.mBlockNumber = static_cast<uint32_t>((buf[0] & 0xf0) >> 4);
        aInfo.mMoreBlocks  = (((buf[0] & 0x08) >> 3) == 1);
        aInfo.mBlockSzx    = (static_cast<BlockSzx>(buf[0] & 0x07));
        break;
    case 2:
        aInfo.mBlockNumber = static_cast<uint32_t>((buf[0] << 4) + ((buf[1] & 0xf0) >> 4));
        aInfo.mMoreBlocks  = ((buf[1] & 0x08) >> 3 == 1);
        aInfo.mBlockSzx    = (static_cast<BlockSzx>(buf[1] & 0x07));
        break;
    case 3:
        aInfo.mBlockNumber = static_cast<uint32_t>((buf[0] << 12) + (buf[1] << 4) + ((buf[2] & 0xf0) >> 4));
        aInfo.mMoreBlocks  = ((buf[2] & 0x08) >> 3 == 1);
        aInfo.mBlockSzx    = (static_cast<BlockSzx>(buf[2] & 0x07));
        break;
    default:
        error = kErrorInvalidArgs;
        break;
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

Error Message::AppendPayloadMarker(void)
{
    Error            error;
    uint8_t          marker = kPayloadMarker;
    Option::Iterator iterator;

    SuccessOrExit(error = iterator.Init(*this));

    while (!iterator.IsDone())
    {
        SuccessOrExit(error = iterator.Advance());
    }

    VerifyOrExit(!iterator.HasPayloadMarker());

    SuccessOrExit(error = Append(marker));

    SetOffset(GetLength());

exit:
    return error;
}

uint16_t Message::DetermineTokenOffset(void) const
{
    uint16_t offset;

    if (CanAddSafely<uint16_t>(GetHeaderOffset(), sizeof(Header)))
    {
        offset = GetHeaderOffset() + sizeof(Header);
    }
    else
    {
        SetToUintMax(offset);
    }

    return offset;
}

Error Message::DetermineOptionStartOffset(uint16_t &aOffset) const
{
    Error    error;
    uint8_t  tokenLength;
    uint16_t offset;

    SuccessOrExit(error = ReadTokenLength(tokenLength));
    offset = DetermineTokenOffset();

    if (CanAddSafely<uint16_t>(offset, tokenLength))
    {
        offset += tokenLength;
    }
    else
    {
        SetToUintMax(offset);
    }

    aOffset = offset;

exit:
    return error;
}

Error Message::ReadTokenLength(uint8_t &aLength) const
{
    Error  error = kErrorNone;
    Header header;

    SuccessOrExit(error = ReadHeader(header));
    aLength = header.GetTokenLength();

exit:
    return error;
}

Error Message::ReadToken(Token &aToken) const
{
    Error  error;
    Header header;

    SuccessOrExit(error = ReadHeader(header));
    error = ReadToken(header, aToken);
exit:
    return error;
}

Error Message::ReadToken(const Header &aHeader, Token &aToken) const
{
    aToken.mLength = aHeader.GetTokenLength();

    return Read(DetermineTokenOffset(), aToken.m8, aToken.mLength);
}

Error Message::WriteToken(const Token &aToken)
{
    Error    error;
    Header   header;
    uint16_t tokenOffset = DetermineTokenOffset();

    VerifyOrExit(aToken.IsValid(), error = kErrorInvalidArgs);

    SuccessOrExit(error = ReadHeader(header));

    if (tokenOffset == GetLength())
    {
        // A token has not been written yet, so grow the message to make
        // space for it.

        SuccessOrExit(error = IncreaseLength(aToken.GetLength()));

        header.SetTokenLength(aToken.GetLength());
        WriteHeader(header);
    }
    else
    {
        // If a token was previously written, we only allow it to be
        // overwritten by a new token of the same length.

        VerifyOrExit(header.GetTokenLength() == aToken.GetLength(), error = kErrorInvalidArgs);
    }

    WriteBytes(tokenOffset, aToken.GetBytes(), aToken.GetLength());

exit:
    return error;
}

Error Message::WriteRandomToken(uint8_t aTokenLength)
{
    Error error;
    Token token;

    SuccessOrExit(error = token.GenerateRandom(aTokenLength));
    error = WriteToken(token);

exit:
    return error;
}

Error Message::WriteTokenFromMessage(const Message &aMessage)
{
    Error error;
    Token token;

    SuccessOrExit(error = aMessage.ReadToken(token));
    error = WriteToken(token);

exit:
    return error;
}

bool Message::HasSameTokenAs(const Message &aMessage) const
{
    bool  hasSame = false;
    Token token;
    Token msgToken;

    SuccessOrExit(ReadToken(token));
    SuccessOrExit(aMessage.ReadToken(msgToken));
    hasSame = (token == msgToken);

exit:
    return hasSame;
}

Message *Message::Clone(uint16_t aLength) const
{
    Message *message = static_cast<Message *>(ot::Message::Clone(aLength));

    VerifyOrExit(message != nullptr);

    message->SetHeaderOffset(GetHeaderOffset());

exit:
    return message;
}

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
const char *Message::CodeToString(void) const
{
    static constexpr Stringify::Entry kCodeTable[] = {
        {kCodeEmpty, "Empty"},
        {kCodeGet, "Get"},
        {kCodePost, "Post"},
        {kCodePut, "Put"},
        {kCodeDelete, "Delete"},
        {kCodeCreated, "Created"},
        {kCodeDeleted, "Deleted"},
        {kCodeValid, "Valid"},
        {kCodeChanged, "Changed"},
        {kCodeContent, "Content"},
        {kCodeContinue, "Continue"},
        {kCodeBadRequest, "BadRequest"},
        {kCodeUnauthorized, "Unauthorized"},
        {kCodeBadOption, "BadOption"},
        {kCodeForbidden, "Forbidden"},
        {kCodeNotFound, "NotFound"},
        {kCodeMethodNotAllowed, "MethodNotAllowed"},
        {kCodeNotAcceptable, "NotAcceptable"},
        {kCodeRequestIncomplete, "RequestIncomplete"},
        {kCodePreconditionFailed, "PreconditionFailed"},
        {kCodeRequestTooLarge, "RequestTooLarge"},
        {kCodeUnsupportedFormat, "UnsupportedFormat"},
        {kCodeInternalError, "InternalError"},
        {kCodeNotImplemented, "NotImplemented"},
        {kCodeBadGateway, "BadGateway"},
        {kCodeServiceUnavailable, "ServiceUnavailable"},
        {kCodeGatewayTimeout, "GatewayTimeout"},
        {kCodeProxyNotSupported, "ProxyNotSupported"},
    };

    static_assert(Stringify::IsSorted(kCodeTable), "kCodeTable is not sorted");

    return Stringify::Lookup(ReadCode(), kCodeTable, "Unknown");
}
#endif // OPENTHREAD_CONFIG_COAP_API_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// `Message::Iterator`

Message::Iterator MessageQueue::begin(void) { return Message::Iterator(GetHead()); }

Message::ConstIterator MessageQueue::begin(void) const { return Message::ConstIterator(GetHead()); }

//---------------------------------------------------------------------------------------------------------------------
// `Option::Iterator`

Error Option::Iterator::Init(const Message &aMessage)
{
    Error    error;
    uint16_t offset;

    SuccessOrExit(error = aMessage.DetermineOptionStartOffset(offset));

    // Note that the case where `offset == aMessage.GetLength())` is
    // valid and indicates an empty payload (no CoAP Option and no
    // Payload Marker).

    VerifyOrExit(offset <= aMessage.GetLength(), error = kErrorParse);

    mOption.mNumber   = 0;
    mOption.mLength   = 0;
    mMessage          = &aMessage;
    mNextOptionOffset = offset;

    error = Advance();

exit:
    if (error != kErrorNone)
    {
        MarkAsDone();
    }

    return error;
}

Error Option::Iterator::Advance(void)
{
    Error    error = kErrorNone;
    uint8_t  headerByte;
    uint16_t optionDelta;
    uint16_t optionLength;

    VerifyOrExit(!IsDone());

    error = Read(sizeof(uint8_t), &headerByte);

    if (error != kErrorNone)
    {
        // Reached the end without seeing the payload marker.

        MarkAsDone();
        SetHasPayloadMarker(false);
        ExitNow(error = kErrorNone);
    }

    if (headerByte == Message::kPayloadMarker)
    {
        // Payload Marker indicates end of options and start of payload.
        // Absence of a Payload Marker indicates a zero-length payload.

        MarkAsDone();
        SetHasPayloadMarker(true);
        ExitNow();
    }

    optionDelta = (headerByte & Message::kOptionDeltaMask) >> Message::kOptionDeltaOffset;
    SuccessOrExit(error = ReadExtendedOptionField(optionDelta));

    optionLength = (headerByte & Message::kOptionLengthMask) >> Message::kOptionLengthOffset;
    SuccessOrExit(error = ReadExtendedOptionField(optionLength));

    VerifyOrExit(optionLength <= GetMessage().GetLength() - mNextOptionOffset, error = kErrorParse);
    mNextOptionOffset += optionLength;

    mOption.mNumber += optionDelta;
    mOption.mLength = optionLength;

exit:
    if (error != kErrorNone)
    {
        MarkAsDone();
    }

    return error;
}

Error Option::Iterator::ReadOptionValue(void *aValue) const
{
    Error error = kErrorNone;

    VerifyOrExit(!IsDone(), error = kErrorNotFound);
    GetMessage().ReadBytes(mNextOptionOffset - mOption.mLength, aValue, mOption.mLength);

exit:
    return error;
}

Error Option::Iterator::ReadOptionValue(uint64_t &aUintValue) const
{
    Error   error = kErrorNone;
    uint8_t buffer[sizeof(uint64_t)];

    VerifyOrExit(!IsDone(), error = kErrorNotFound);

    VerifyOrExit(mOption.mLength <= sizeof(uint64_t), error = kErrorNoBufs);
    IgnoreError(ReadOptionValue(buffer));

    aUintValue = 0;

    for (uint16_t pos = 0; pos < mOption.mLength; pos++)
    {
        aUintValue <<= kBitsPerByte;
        aUintValue |= buffer[pos];
    }

exit:
    return error;
}

Error Option::Iterator::Read(uint16_t aLength, void *aBuffer)
{
    // Reads `aLength` bytes from the message into `aBuffer` at
    // `mNextOptionOffset` and updates the `mNextOptionOffset` on a
    // successful read (i.e., when entire `aLength` bytes can be read).

    Error error = kErrorNone;

    SuccessOrExit(error = GetMessage().Read(mNextOptionOffset, aBuffer, aLength));
    mNextOptionOffset += aLength;

exit:
    return error;
}

Error Option::Iterator::ReadExtendedOptionField(uint16_t &aValue)
{
    Error error = kErrorNone;

    VerifyOrExit(aValue >= Message::kOption1ByteExtension);

    if (aValue == Message::kOption1ByteExtension)
    {
        uint8_t value8;

        SuccessOrExit(error = Read(sizeof(uint8_t), &value8));
        aValue = static_cast<uint16_t>(value8) + Message::kOption1ByteExtensionOffset;
    }
    else if (aValue == Message::kOption2ByteExtension)
    {
        uint16_t value16;

        SuccessOrExit(error = Read(sizeof(uint16_t), &value16));
        value16 = BigEndian::HostSwap16(value16);
        aValue  = value16 + Message::kOption2ByteExtensionOffset;
    }
    else
    {
        error = kErrorParse;
    }

exit:
    return error;
}

Error Option::Iterator::InitOrAdvance(const Message *aMessage, uint16_t aNumber)
{
    Error error = (aMessage != nullptr) ? Init(*aMessage) : Advance();

    while ((error == kErrorNone) && !IsDone() && (GetOption()->GetNumber() != aNumber))
    {
        error = Advance();
    }

    return error;
}

} // namespace Coap
} // namespace ot
