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

#include "coap/coap.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/random.hpp"

namespace ot {
namespace Coap {

void Message::Init(void)
{
    GetHelpData().Clear();
    SetVersion(kVersion1);
    SetOffset(0);
    GetHelpData().mHeaderLength = kMinHeaderLength;

    IgnoreError(SetLength(GetHelpData().mHeaderLength));
}

void Message::Init(Type aType, Code aCode)
{
    Init();
    SetType(aType);
    SetCode(aCode);
}

otError Message::Init(Type aType, Code aCode, const char *aUriPath)
{
    otError error;

    Init(aType, aCode);
    SuccessOrExit(error = SetToken(kDefaultTokenLength));
    SuccessOrExit(error = AppendUriPathOptions(aUriPath));

exit:
    return error;
}

void Message::InitAsConfirmablePost(void)
{
    Init(kTypeConfirmable, kCodePost);
}

void Message::InitAsNonConfirmablePost(void)
{
    Init(kTypeNonConfirmable, kCodePost);
}

otError Message::InitAsConfirmablePost(const char *aUriPath)
{
    return Init(kTypeConfirmable, kCodePost, aUriPath);
}

otError Message::InitAsNonConfirmablePost(const char *aUriPath)
{
    return Init(kTypeNonConfirmable, kCodePost, aUriPath);
}

otError Message::InitAsPost(const Ip6::Address &aDestination, const char *aUriPath)
{
    return Init(aDestination.IsMulticast() ? kTypeNonConfirmable : kTypeConfirmable, kCodePost, aUriPath);
}

bool Message::IsConfirmablePostRequest(void) const
{
    return IsConfirmable() && IsPostRequest();
}

bool Message::IsNonConfirmablePostRequest(void) const
{
    return IsNonConfirmable() && IsPostRequest();
}

void Message::Finish(void)
{
    Write(0, GetOptionStart(), &GetHelpData().mHeader);
}

otError Message::AppendOption(uint16_t aNumber, uint16_t aLength, const void *aValue)
{
    otError  error       = OT_ERROR_NONE;
    uint16_t optionDelta = aNumber - GetHelpData().mOptionLast;
    uint16_t optionLength;
    uint8_t  buf[kMaxOptionHeaderSize] = {0};
    uint8_t *cur                       = &buf[1];

    // Assure that no option is inserted out of order.
    VerifyOrExit(aNumber >= GetHelpData().mOptionLast, error = OT_ERROR_INVALID_ARGS);

    // Calculate the total option size and check the buffers.
    optionLength = 1 + aLength;
    optionLength += optionDelta < kOption1ByteExtensionOffset ? 0 : (optionDelta < kOption2ByteExtensionOffset ? 1 : 2);
    optionLength += aLength < kOption1ByteExtensionOffset ? 0 : (aLength < kOption2ByteExtensionOffset ? 1 : 2);
    VerifyOrExit(GetLength() + optionLength < kMaxHeaderLength, error = OT_ERROR_NO_BUFS);

    // Insert option delta.
    if (optionDelta < kOption1ByteExtensionOffset)
    {
        *buf = (optionDelta << kOptionDeltaOffset) & kOptionDeltaMask;
    }
    else if (optionDelta < kOption2ByteExtensionOffset)
    {
        *buf |= kOption1ByteExtension << kOptionDeltaOffset;
        *cur++ = (optionDelta - kOption1ByteExtensionOffset) & 0xff;
    }
    else
    {
        *buf |= kOption2ByteExtension << kOptionDeltaOffset;
        optionDelta -= kOption2ByteExtensionOffset;
        *cur++ = optionDelta >> 8;
        *cur++ = optionDelta & 0xff;
    }

    // Insert option length.
    if (aLength < kOption1ByteExtensionOffset)
    {
        *buf |= aLength;
    }
    else if (aLength < kOption2ByteExtensionOffset)
    {
        *buf |= kOption1ByteExtension;
        *cur++ = (aLength - kOption1ByteExtensionOffset) & 0xff;
    }
    else
    {
        *buf |= kOption2ByteExtension;
        optionLength = aLength - kOption2ByteExtensionOffset;
        *cur++       = optionLength >> 8;
        *cur++       = optionLength & 0xff;
    }

    SuccessOrExit(error = Append(buf, static_cast<uint16_t>(cur - buf)));
    SuccessOrExit(error = Append(aValue, aLength));

    GetHelpData().mOptionLast = aNumber;

    GetHelpData().mHeaderLength = GetLength();

exit:
    return error;
}

otError Message::AppendUintOption(uint16_t aNumber, uint32_t aValue)
{
    uint16_t length = sizeof(aValue);
    uint8_t *value;

    aValue = Encoding::BigEndian::HostSwap32(aValue);
    value  = reinterpret_cast<uint8_t *>(&aValue);

    // skip preceding zeros
    while (value[0] == 0 && length > 0)
    {
        value++;
        length--;
    }

    return AppendOption(aNumber, length, value);
}

otError Message::AppendStringOption(uint16_t aNumber, const char *aValue)
{
    return AppendOption(aNumber, static_cast<uint16_t>(strlen(aValue)), aValue);
}

otError Message::AppendObserveOption(uint32_t aObserve)
{
    return AppendUintOption(kOptionObserve, aObserve & 0xFFFFFF);
}

otError Message::AppendUriPathOptions(const char *aUriPath)
{
    otError     error = OT_ERROR_NONE;
    const char *cur   = aUriPath;
    const char *end;

    while ((end = strchr(cur, '/')) != nullptr)
    {
        SuccessOrExit(error = AppendOption(kOptionUriPath, static_cast<uint16_t>(end - cur), cur));
        cur = end + 1;
    }

    SuccessOrExit(error = AppendStringOption(kOptionUriPath, cur));

exit:
    return error;
}

otError Message::AppendBlockOption(Message::BlockType aType, uint32_t aNum, bool aMore, otCoapBlockSize aSize)
{
    otError  error   = OT_ERROR_NONE;
    uint32_t encoded = aSize;

    VerifyOrExit(aType == kBlockType1 || aType == kBlockType2, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aSize <= OT_COAP_BLOCK_SIZE_1024, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aNum < kBlockNumMax, error = OT_ERROR_INVALID_ARGS);

    encoded |= static_cast<uint32_t>(aMore << kBlockMOffset);
    encoded |= aNum << kBlockNumOffset;

    error = AppendUintOption((aType == kBlockType1) ? kOptionBlock1 : kOptionBlock2, encoded);

exit:
    return error;
}

otError Message::AppendProxyUriOption(const char *aProxyUri)
{
    return AppendStringOption(kOptionProxyUri, aProxyUri);
}

otError Message::AppendContentFormatOption(otCoapOptionContentFormat aContentFormat)
{
    return AppendUintOption(kOptionContentFormat, static_cast<uint32_t>(aContentFormat));
}

otError Message::AppendMaxAgeOption(uint32_t aMaxAge)
{
    return AppendUintOption(kOptionMaxAge, aMaxAge);
}

otError Message::AppendUriQueryOption(const char *aUriQuery)
{
    return AppendStringOption(kOptionUriQuery, aUriQuery);
}

otError Message::SetPayloadMarker(void)
{
    otError error  = OT_ERROR_NONE;
    uint8_t marker = 0xff;

    VerifyOrExit(GetLength() < kMaxHeaderLength, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = Append(&marker, sizeof(marker)));
    GetHelpData().mHeaderLength = GetLength();

    // Set offset to the start of payload.
    SetOffset(GetHelpData().mHeaderLength);

exit:
    return error;
}

otError Message::ParseHeader(void)
{
    otError        error = OT_ERROR_NONE;
    OptionIterator iterator;

    OT_ASSERT(mBuffer.mHead.mMetadata.mReserved >=
              sizeof(GetHelpData()) +
                  static_cast<size_t>((reinterpret_cast<uint8_t *>(&GetHelpData()) - mBuffer.mHead.mData)));

    GetHelpData().Clear();

    GetHelpData().mHeaderOffset = GetOffset();
    Read(GetHelpData().mHeaderOffset, sizeof(GetHelpData().mHeader), &GetHelpData().mHeader);

    VerifyOrExit(GetTokenLength() <= kMaxTokenLength, error = OT_ERROR_PARSE);

    SuccessOrExit(error = iterator.Init(this));
    for (const otCoapOption *option = iterator.GetFirstOption(); option != nullptr; option = iterator.GetNextOption())
    {
    }

    VerifyOrExit(iterator.mNextOptionOffset > 0, error = OT_ERROR_PARSE);
    GetHelpData().mHeaderLength = iterator.mNextOptionOffset - GetHelpData().mHeaderOffset;
    MoveOffset(GetHelpData().mHeaderLength);

exit:
    return error;
}

otError Message::SetToken(const uint8_t *aToken, uint8_t aTokenLength)
{
    OT_ASSERT(aTokenLength <= kMaxTokenLength);

    SetTokenLength(aTokenLength);
    memcpy(GetToken(), aToken, aTokenLength);
    GetHelpData().mHeaderLength += aTokenLength;

    return SetLength(GetHelpData().mHeaderLength);
}

otError Message::SetToken(uint8_t aTokenLength)
{
    uint8_t token[kMaxTokenLength];

    OT_ASSERT(aTokenLength <= sizeof(token));

    IgnoreError(Random::Crypto::FillBuffer(token, aTokenLength));

    return SetToken(token, aTokenLength);
}

otError Message::SetDefaultResponseHeader(const Message &aRequest)
{
    Init(kTypeAck, kCodeChanged);

    SetMessageId(aRequest.GetMessageId());

    return SetToken(aRequest.GetToken(), aRequest.GetTokenLength());
}

Message *Message::Clone(uint16_t aLength) const
{
    Message *message = static_cast<Message *>(ot::Message::Clone(aLength));

    VerifyOrExit(message != nullptr, OT_NOOP);

    message->GetHelpData() = GetHelpData();

exit:
    return message;
}

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
const char *Message::CodeToString(void) const
{
    const char *string;

    switch (GetCode())
    {
    case kCodeEmpty:
        string = "Empty";
        break;
    case kCodeGet:
        string = "Get";
        break;
    case kCodePost:
        string = "Post";
        break;
    case kCodePut:
        string = "Put";
        break;
    case kCodeDelete:
        string = "Delete";
        break;
    case kCodeCreated:
        string = "Created";
        break;
    case kCodeDeleted:
        string = "Deleted";
        break;
    case kCodeValid:
        string = "Valid";
        break;
    case kCodeChanged:
        string = "Changed";
        break;
    case kCodeContent:
        string = "Content";
        break;
    case kCodeContinue:
        string = "Continue";
        break;
    case kCodeBadRequest:
        string = "BadRequest";
        break;
    case kCodeUnauthorized:
        string = "Unauthorized";
        break;
    case kCodeBadOption:
        string = "BadOption";
        break;
    case kCodeForbidden:
        string = "Forbidden";
        break;
    case kCodeNotFound:
        string = "NotFound";
        break;
    case kCodeMethodNotAllowed:
        string = "MethodNotAllowed";
        break;
    case kCodeNotAcceptable:
        string = "NotAcceptable";
        break;
    case kCodeRequestIncomplete:
        string = "RequestIncomplete";
        break;
    case kCodePreconditionFailed:
        string = "PreconditionFailed";
        break;
    case kCodeRequestTooLarge:
        string = "RequestTooLarge";
        break;
    case kCodeUnsupportedFormat:
        string = "UnsupportedFormat";
        break;
    case kCodeInternalError:
        string = "InternalError";
        break;
    case kCodeNotImplemented:
        string = "NotImplemented";
        break;
    case kCodeBadGateway:
        string = "BadGateway";
        break;
    case kCodeServiceUnavailable:
        string = "ServiceUnavailable";
        break;
    case kCodeGatewayTimeout:
        string = "GatewayTimeout";
        break;
    case kCodeProxyNotSupported:
        string = "ProxyNotSupported";
        break;
    default:
        string = "Unknown";
        break;
    }

    return string;
}
#endif // OPENTHREAD_CONFIG_COAP_API_ENABLE

otError OptionIterator::Init(const Message *aMessage)
{
    otError err = OT_ERROR_NONE;

    /*
     * Check that:
     *   Length - Offset: the length of the payload
     * is greater than:
     *   Start position of options
     *
     * → Check options start before the message ends, or bail ::Init with
     *   OT_ERROR_PARSE as the reason.
     */
    VerifyOrExit(aMessage->GetLength() - aMessage->GetHelpData().mHeaderOffset >= aMessage->GetOptionStart(),
                 err = OT_ERROR_PARSE);

    mMessage = aMessage;
    GetFirstOption();

exit:
    return err;
}

const otCoapOption *OptionIterator::GetFirstOptionMatching(uint16_t aOption)
{
    const otCoapOption *rval = nullptr;

    for (const otCoapOption *option = GetFirstOption(); option != nullptr; option = GetNextOption())
    {
        if (option->mNumber == aOption)
        {
            // Found, stop searching
            rval = option;
            break;
        }
    }

    return rval;
}

const otCoapOption *OptionIterator::GetFirstOption(void)
{
    const otCoapOption *option  = nullptr;
    const Message &     message = GetMessage();

    ClearOption();

    mNextOptionOffset = message.GetHelpData().mHeaderOffset + message.GetOptionStart();

    if (mNextOptionOffset < message.GetLength())
    {
        option = GetNextOption();
    }

    return option;
}

const otCoapOption *OptionIterator::GetNextOptionMatching(uint16_t aOption)
{
    const otCoapOption *rval = nullptr;

    for (const otCoapOption *option = GetNextOption(); option != nullptr; option = GetNextOption())
    {
        if (option->mNumber == aOption)
        {
            // Found, stop searching
            rval = option;
            break;
        }
    }

    return rval;
}

const otCoapOption *OptionIterator::GetNextOption(void)
{
    otError        error = OT_ERROR_NONE;
    uint16_t       optionDelta;
    uint16_t       optionLength;
    uint8_t        buf[Message::kMaxOptionHeaderSize];
    uint8_t *      cur     = buf + 1;
    otCoapOption * rval    = nullptr;
    const Message &message = GetMessage();

    VerifyOrExit(mNextOptionOffset < message.GetLength(), error = OT_ERROR_NOT_FOUND);

    message.Read(mNextOptionOffset, sizeof(buf), buf);

    optionDelta  = buf[0] >> 4;
    optionLength = buf[0] & 0xf;
    mNextOptionOffset += sizeof(uint8_t);

    if (optionDelta < Message::kOption1ByteExtension)
    {
        // do nothing
    }
    else if (optionDelta == Message::kOption1ByteExtension)
    {
        optionDelta = Message::kOption1ByteExtensionOffset + cur[0];
        mNextOptionOffset += sizeof(uint8_t);
        cur++;
    }
    else if (optionDelta == Message::kOption2ByteExtension)
    {
        optionDelta = Message::kOption2ByteExtensionOffset + static_cast<uint16_t>((cur[0] << 8) | cur[1]);
        mNextOptionOffset += sizeof(uint16_t);
        cur += 2;
    }
    else
    {
        // RFC7252 (Section 3):
        // Reserved for payload marker.
        VerifyOrExit(optionLength == 0xf, error = OT_ERROR_PARSE);

        // The presence of a marker followed by a zero-length payload MUST be processed
        // as a message format error.
        VerifyOrExit(mNextOptionOffset < message.GetLength(), error = OT_ERROR_PARSE);

        ExitNow(error = OT_ERROR_NOT_FOUND);
    }

    if (optionLength < Message::kOption1ByteExtension)
    {
        // do nothing
    }
    else if (optionLength == Message::kOption1ByteExtension)
    {
        optionLength = Message::kOption1ByteExtensionOffset + cur[0];
        mNextOptionOffset += sizeof(uint8_t);
    }
    else if (optionLength == Message::kOption2ByteExtension)
    {
        optionLength = Message::kOption2ByteExtensionOffset + static_cast<uint16_t>((cur[0] << 8) | cur[1]);
        mNextOptionOffset += sizeof(uint16_t);
    }
    else
    {
        ExitNow(error = OT_ERROR_PARSE);
    }

    VerifyOrExit(optionLength <= message.GetLength() - mNextOptionOffset, error = OT_ERROR_PARSE);

    rval = &mOption;
    rval->mNumber += optionDelta;
    rval->mLength = optionLength;

    mNextOptionOffset += optionLength;

exit:
    if (error == OT_ERROR_PARSE)
    {
        mNextOptionOffset = 0;
    }

    return rval;
}

otError OptionIterator::GetOptionValue(uint64_t &aValue) const
{
    otError error = OT_ERROR_NONE;
    uint8_t value[sizeof(aValue)];

    VerifyOrExit(mOption.mLength <= sizeof(aValue), error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = GetOptionValue(value));

    aValue = 0;
    for (uint16_t pos = 0; pos < mOption.mLength; pos++)
    {
        aValue <<= 8;
        aValue |= value[pos];
    }

exit:
    return error;
}

otError OptionIterator::GetOptionValue(void *aValue) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mNextOptionOffset > 0, error = OT_ERROR_NOT_FOUND);

    VerifyOrExit(GetMessage().Read(mNextOptionOffset - mOption.mLength, mOption.mLength, aValue) == mOption.mLength,
                 error = OT_ERROR_PARSE);

exit:
    return error;
}

} // namespace Coap
} // namespace ot
