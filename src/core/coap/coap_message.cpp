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
    memset(&GetHelpData(), 0, sizeof(GetHelpData()));
    SetVersion(kVersion1);
    SetOffset(0);
    GetHelpData().mHeaderLength = kMinHeaderLength;
    SetLength(GetHelpData().mHeaderLength);
}

void Message::Init(Type aType, Code aCode)
{
    Init();
    SetType(aType);
    SetCode(aCode);
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

    Append(buf, static_cast<uint16_t>(cur - buf));
    Append(aValue, aLength);

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
    return AppendUintOption(OT_COAP_OPTION_OBSERVE, aObserve & 0xFFFFFF);
}

otError Message::AppendUriPathOptions(const char *aUriPath)
{
    otError     error = OT_ERROR_NONE;
    const char *cur   = aUriPath;
    const char *end;

    while ((end = strchr(cur, '/')) != NULL)
    {
        SuccessOrExit(error = AppendOption(OT_COAP_OPTION_URI_PATH, static_cast<uint16_t>(end - cur), cur));
        cur = end + 1;
    }

    SuccessOrExit(error = AppendStringOption(OT_COAP_OPTION_URI_PATH, cur));

exit:
    return error;
}

otError Message::AppendProxyUriOption(const char *aProxyUri)
{
    return AppendStringOption(OT_COAP_OPTION_PROXY_URI, aProxyUri);
}

otError Message::AppendContentFormatOption(otCoapOptionContentFormat aContentFormat)
{
    return AppendUintOption(OT_COAP_OPTION_CONTENT_FORMAT, static_cast<uint32_t>(aContentFormat));
}

otError Message::AppendMaxAgeOption(uint32_t aMaxAge)
{
    return AppendUintOption(OT_COAP_OPTION_MAX_AGE, aMaxAge);
}

otError Message::AppendUriQueryOption(const char *aUriQuery)
{
    return AppendStringOption(OT_COAP_OPTION_URI_QUERY, aUriQuery);
}

const otCoapOption *Message::GetFirstOption(void)
{
    const otCoapOption *option = NULL;

    memset(&GetHelpData().mOption, 0, sizeof(GetHelpData().mOption));

    VerifyOrExit(GetLength() - GetHelpData().mHeaderOffset >= GetOptionStart());

    GetHelpData().mNextOptionOffset = GetHelpData().mHeaderOffset + GetOptionStart();

    if (GetHelpData().mNextOptionOffset < GetLength())
    {
        option = GetNextOption();
    }

exit:
    return option;
}

const otCoapOption *Message::GetNextOption(void)
{
    otError       error = OT_ERROR_NONE;
    uint16_t      optionDelta;
    uint16_t      optionLength;
    uint8_t       buf[kMaxOptionHeaderSize];
    uint8_t *     cur  = buf + 1;
    otCoapOption *rval = NULL;

    VerifyOrExit(GetHelpData().mNextOptionOffset < GetLength(), error = OT_ERROR_NOT_FOUND);

    Read(GetHelpData().mNextOptionOffset, sizeof(buf), buf);

    optionDelta  = buf[0] >> 4;
    optionLength = buf[0] & 0xf;
    GetHelpData().mNextOptionOffset += sizeof(uint8_t);

    if (optionDelta < kOption1ByteExtension)
    {
        // do nothing
    }
    else if (optionDelta == kOption1ByteExtension)
    {
        optionDelta = kOption1ByteExtensionOffset + cur[0];
        GetHelpData().mNextOptionOffset += sizeof(uint8_t);
        cur++;
    }
    else if (optionDelta == kOption2ByteExtension)
    {
        optionDelta = kOption2ByteExtensionOffset + static_cast<uint16_t>((cur[0] << 8) | cur[1]);
        GetHelpData().mNextOptionOffset += sizeof(uint16_t);
        cur += 2;
    }
    else
    {
        VerifyOrExit(optionLength == 0xf, error = OT_ERROR_PARSE);
        ExitNow(error = OT_ERROR_NOT_FOUND);
    }

    if (optionLength < kOption1ByteExtension)
    {
        // do nothing
    }
    else if (optionLength == kOption1ByteExtension)
    {
        optionLength = kOption1ByteExtensionOffset + cur[0];
        GetHelpData().mNextOptionOffset += sizeof(uint8_t);
    }
    else if (optionLength == kOption2ByteExtension)
    {
        optionLength = kOption2ByteExtensionOffset + static_cast<uint16_t>((cur[0] << 8) | cur[1]);
        GetHelpData().mNextOptionOffset += sizeof(uint16_t);
    }
    else
    {
        ExitNow(error = OT_ERROR_PARSE);
    }

    VerifyOrExit(optionLength <= GetLength() - GetHelpData().mNextOptionOffset, error = OT_ERROR_PARSE);

    rval = &GetHelpData().mOption;
    rval->mNumber += optionDelta;
    rval->mLength = optionLength;

    GetHelpData().mNextOptionOffset += optionLength;

exit:
    if (error == OT_ERROR_PARSE)
    {
        GetHelpData().mNextOptionOffset = 0;
    }

    return rval;
}

otError Message::GetOptionValue(void *aValue) const
{
    otError             error  = OT_ERROR_NONE;
    const otCoapOption &option = GetHelpData().mOption;

    VerifyOrExit(GetHelpData().mNextOptionOffset > 0, error = OT_ERROR_NOT_FOUND);

    VerifyOrExit(Read(GetHelpData().mNextOptionOffset - option.mLength, option.mLength, aValue) == option.mLength,
                 error = OT_ERROR_PARSE);

exit:
    return error;
}

otError Message::SetPayloadMarker(void)
{
    otError error  = OT_ERROR_NONE;
    uint8_t marker = 0xff;

    VerifyOrExit(GetLength() < kMaxHeaderLength, error = OT_ERROR_NO_BUFS);
    Append(&marker, sizeof(marker));
    GetHelpData().mHeaderLength = GetLength();

exit:
    return error;
}

otError Message::ParseHeader(void)
{
    otError error = OT_ERROR_NONE;

    assert(mBuffer.mHead.mInfo.mReserved >=
           sizeof(GetHelpData()) +
               static_cast<size_t>((reinterpret_cast<uint8_t *>(&GetHelpData()) - mBuffer.mHead.mData)));

    memset(&GetHelpData(), 0, sizeof(GetHelpData()));

    GetHelpData().mHeaderOffset = GetOffset();
    Read(GetHelpData().mHeaderOffset, sizeof(GetHelpData().mHeader), &GetHelpData().mHeader);

    for (const otCoapOption *option = GetFirstOption(); option != NULL; option = GetNextOption())
    {
    }

    VerifyOrExit(GetHelpData().mNextOptionOffset > 0, error = OT_ERROR_PARSE);
    GetHelpData().mHeaderLength = GetHelpData().mNextOptionOffset - GetHelpData().mHeaderOffset;
    MoveOffset(GetHelpData().mHeaderLength);

exit:
    return error;
}

void Message::SetToken(const uint8_t *aToken, uint8_t aTokenLength)
{
    GetHelpData().mHeader.mVersionTypeToken = (GetHelpData().mHeader.mVersionTypeToken & ~kTokenLengthMask) |
                                              ((aTokenLength << kTokenLengthOffset) & kTokenLengthMask);
    memcpy(GetHelpData().mHeader.mToken, aToken, aTokenLength);
    GetHelpData().mHeaderLength += aTokenLength;
    SetLength(GetHelpData().mHeaderLength);
}

void Message::SetToken(uint8_t aTokenLength)
{
    uint8_t token[kMaxTokenLength] = {0};

    assert(aTokenLength <= sizeof(token));

    Random::FillBuffer(token, aTokenLength);

    SetToken(token, aTokenLength);
}

void Message::SetDefaultResponseHeader(const Message &aRequest)
{
    Init(OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CHANGED);
    SetMessageId(aRequest.GetMessageId());
    SetToken(aRequest.GetToken(), aRequest.GetTokenLength());
}

Message *Message::Clone(uint16_t aLength) const
{
    Message *message = static_cast<Message *>(ot::Message::Clone(aLength));

    VerifyOrExit(message != NULL);

    memcpy(&message->GetHelpData(), &GetHelpData(), sizeof(GetHelpData()));

exit:
    return message;
}

#if OPENTHREAD_ENABLE_APPLICATION_COAP
const char *Message::CodeToString(void) const
{
    const char *codeString;

    switch (GetCode())
    {
    case OT_COAP_CODE_INTERNAL_ERROR:
        codeString = "InternalError";
        break;
    case OT_COAP_CODE_METHOD_NOT_ALLOWED:
        codeString = "MethodNotAllowed";
        break;
    case OT_COAP_CODE_CONTENT:
        codeString = "Content";
        break;
    case OT_COAP_CODE_EMPTY:
        codeString = "Empty";
        break;
    case OT_COAP_CODE_GET:
        codeString = "Get";
        break;
    case OT_COAP_CODE_POST:
        codeString = "Post";
        break;
    case OT_COAP_CODE_PUT:
        codeString = "Put";
        break;
    case OT_COAP_CODE_DELETE:
        codeString = "Delete";
        break;
    case OT_COAP_CODE_NOT_FOUND:
        codeString = "NotFound";
        break;
    case OT_COAP_CODE_UNSUPPORTED_FORMAT:
        codeString = "UnsupportedFormat";
        break;
    case OT_COAP_CODE_RESPONSE_MIN:
        codeString = "ResponseMin";
        break;
    case OT_COAP_CODE_CREATED:
        codeString = "Created";
        break;
    case OT_COAP_CODE_DELETED:
        codeString = "Deleted";
        break;
    case OT_COAP_CODE_VALID:
        codeString = "Valid";
        break;
    case OT_COAP_CODE_CHANGED:
        codeString = "Changed";
        break;
    case OT_COAP_CODE_BAD_REQUEST:
        codeString = "BadRequest";
        break;
    case OT_COAP_CODE_UNAUTHORIZED:
        codeString = "Unauthorized";
        break;
    case OT_COAP_CODE_BAD_OPTION:
        codeString = "BadOption";
        break;
    case OT_COAP_CODE_FORBIDDEN:
        codeString = "Forbidden";
        break;
    case OT_COAP_CODE_NOT_ACCEPTABLE:
        codeString = "NotAcceptable";
        break;
    case OT_COAP_CODE_PRECONDITION_FAILED:
        codeString = "PreconditionFailed";
        break;
    case OT_COAP_CODE_REQUEST_TOO_LARGE:
        codeString = "RequestTooLarge";
        break;
    case OT_COAP_CODE_NOT_IMPLEMENTED:
        codeString = "NotImplemented";
        break;
    case OT_COAP_CODE_BAD_GATEWAY:
        codeString = "BadGateway";
        break;
    case OT_COAP_CODE_SERVICE_UNAVAILABLE:
        codeString = "ServiceUnavailable";
        break;
    case OT_COAP_CODE_GATEWAY_TIMEOUT:
        codeString = "GatewayTimeout";
        break;
    case OT_COAP_CODE_PROXY_NOT_SUPPORTED:
        codeString = "ProxyNotSupported";
        break;
    default:
        codeString = "Unknown";
        break;
    }

    return codeString;
}
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP

} // namespace Coap
} // namespace ot
