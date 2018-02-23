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
 *   This file implements the CoAP header generation and parsing.
 */

#include "coap_header.hpp"

#include "coap/coap.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/random.hpp"

namespace ot {
namespace Coap {

void Header::Init(void)
{
    mHeaderLength      = kMinHeaderLength;
    mOptionLast        = 0;
    mFirstOptionOffset = 0;
    mNextOptionOffset  = 0;
    memset(&mOption, 0, sizeof(mOption));
    memset(&mHeader, 0, sizeof(mHeader));
    SetVersion(kVersion1);
}

void Header::Init(Type aType, Code aCode)
{
    Init();
    SetType(aType);
    SetCode(aCode);
}

otError Header::FromMessage(const Message &aMessage, uint16_t aMetadataSize)
{
    otError  error  = OT_ERROR_PARSE;
    uint16_t offset = aMessage.GetOffset();
    uint16_t length = aMessage.GetLength() - aMessage.GetOffset();
    uint8_t  tokenLength;
    bool     firstOption = true;
    uint16_t optionDelta;
    uint16_t optionLength;

    length -= aMetadataSize;

    Init();

    VerifyOrExit(kTokenOffset <= length);
    aMessage.Read(offset, kTokenOffset, mHeader.mBytes);
    mHeaderLength = kTokenOffset;
    offset += kTokenOffset;

    VerifyOrExit(GetVersion() == 1);

    tokenLength = GetTokenLength();
    VerifyOrExit(tokenLength <= kMaxTokenLength && (mHeaderLength + tokenLength) <= length);
    aMessage.Read(offset, tokenLength, mHeader.mBytes + mHeaderLength);
    mHeaderLength += tokenLength;
    offset += tokenLength;

    while (mHeaderLength < length)
    {
        VerifyOrExit(mHeaderLength + kMaxOptionHeaderSize <= kMaxHeaderLength);

        aMessage.Read(offset, kMaxOptionHeaderSize, mHeader.mBytes + mHeaderLength);

        if (mHeader.mBytes[mHeaderLength] == 0xff)
        {
            mHeaderLength += sizeof(uint8_t);
            // RFC7252: The presence of a marker followed by a zero-length payload MUST be processed
            // as a message format error.
            VerifyOrExit(mHeaderLength < length);
            ExitNow(error = OT_ERROR_NONE);
        }

        if (firstOption)
        {
            mFirstOptionOffset = mHeaderLength;
        }

        optionDelta  = mHeader.mBytes[mHeaderLength] >> 4;
        optionLength = mHeader.mBytes[mHeaderLength] & 0xf;
        mHeaderLength += sizeof(uint8_t);
        offset += sizeof(uint8_t);

        if (optionDelta < kOption1ByteExtension)
        {
            // do nothing
        }
        else if (optionDelta == kOption1ByteExtension)
        {
            optionDelta = kOption1ByteExtensionOffset + mHeader.mBytes[mHeaderLength];
            mHeaderLength += sizeof(uint8_t);
            offset += sizeof(uint8_t);
        }
        else if (optionDelta == kOption2ByteExtension)
        {
            optionDelta = kOption2ByteExtensionOffset + static_cast<uint16_t>((mHeader.mBytes[mHeaderLength] << 8) |
                                                                              (mHeader.mBytes[mHeaderLength + 1]));
            mHeaderLength += sizeof(uint16_t);
            offset += sizeof(uint16_t);
        }
        else
        {
            ExitNow(error = OT_ERROR_PARSE);
        }

        if (optionLength < kOption1ByteExtension)
        {
            // do nothing
        }
        else if (optionLength == kOption1ByteExtension)
        {
            optionLength = kOption1ByteExtensionOffset + mHeader.mBytes[mHeaderLength];
            mHeaderLength += sizeof(uint8_t);
            offset += sizeof(uint8_t);
        }
        else if (optionLength == kOption2ByteExtension)
        {
            optionLength = kOption2ByteExtensionOffset + static_cast<uint16_t>((mHeader.mBytes[mHeaderLength] << 8) |
                                                                               (mHeader.mBytes[mHeaderLength + 1]));
            mHeaderLength += sizeof(uint16_t);
            offset += sizeof(uint16_t);
        }
        else
        {
            ExitNow(error = OT_ERROR_PARSE);
        }

        if (firstOption)
        {
            mOption.mNumber   = optionDelta;
            mOption.mLength   = optionLength;
            mOption.mValue    = mHeader.mBytes + mHeaderLength;
            mNextOptionOffset = mHeaderLength + optionLength;
            firstOption       = false;
        }

        VerifyOrExit(mHeaderLength + optionLength <= kMaxHeaderLength);
        VerifyOrExit(mHeaderLength + optionLength <= length);

        aMessage.Read(offset, optionLength, mHeader.mBytes + mHeaderLength);
        mHeaderLength += static_cast<uint8_t>(optionLength);
        offset += optionLength;
    }

    if (mHeaderLength == length)
    {
        // No payload present - return success.
        error = OT_ERROR_NONE;
    }

exit:

    // In case any step failed, prevent access to corrupt Option
    if (error != OT_ERROR_NONE)
    {
        mFirstOptionOffset = 0;
    }

    return error;
}

otError Header::AppendOption(const Option &aOption)
{
    otError  error       = OT_ERROR_NONE;
    uint8_t *buf         = mHeader.mBytes + mHeaderLength;
    uint8_t *cur         = buf + 1;
    uint16_t optionDelta = aOption.mNumber - mOptionLast;
    uint16_t optionLength;

    // Assure that no option is inserted out of order.
    VerifyOrExit(aOption.mNumber >= mOptionLast, error = OT_ERROR_INVALID_ARGS);

    // Calculate the total option size and check the buffers.
    optionLength = 1 + aOption.mLength;
    optionLength += optionDelta < kOption1ByteExtensionOffset ? 0 : (optionDelta < kOption2ByteExtensionOffset ? 1 : 2);
    optionLength +=
        aOption.mLength < kOption1ByteExtensionOffset ? 0 : (aOption.mLength < kOption2ByteExtensionOffset ? 1 : 2);
    VerifyOrExit(mHeaderLength + optionLength < kMaxHeaderLength, error = OT_ERROR_NO_BUFS);

    // Insert option delta.
    if (optionDelta < kOption1ByteExtensionOffset)
    {
        *buf = (optionDelta << Option::kOptionDeltaOffset) & Option::kOptionDeltaMask;
    }
    else if (optionDelta < kOption2ByteExtensionOffset)
    {
        *buf |= kOption1ByteExtension << Option::kOptionDeltaOffset;
        *cur++ = (optionDelta - kOption1ByteExtensionOffset) & 0xff;
    }
    else
    {
        *buf |= kOption2ByteExtension << Option::kOptionDeltaOffset;
        optionDelta -= kOption2ByteExtensionOffset;
        *cur++ = optionDelta >> 8;
        *cur++ = optionDelta & 0xff;
    }

    // Insert option length.
    if (aOption.mLength < kOption1ByteExtensionOffset)
    {
        *buf |= aOption.mLength;
    }
    else if (aOption.mLength < kOption2ByteExtensionOffset)
    {
        *buf |= kOption1ByteExtension;
        *cur++ = (aOption.mLength - kOption1ByteExtensionOffset) & 0xff;
    }
    else
    {
        *buf |= kOption2ByteExtension;
        optionLength = aOption.mLength - kOption2ByteExtensionOffset;
        *cur++       = optionLength >> 8;
        *cur++       = optionLength & 0xff;
    }

    // Insert option value.
    memcpy(cur, aOption.mValue, aOption.mLength);
    cur += aOption.mLength;

    mHeaderLength += static_cast<uint8_t>(cur - buf);
    mOptionLast = aOption.mNumber;

exit:
    return error;
}

otError Header::AppendUintOption(uint16_t aNumber, uint32_t aValue)
{
    Option coapOption;

    aValue             = Encoding::BigEndian::HostSwap32(aValue);
    coapOption.mNumber = aNumber;
    coapOption.mLength = 4;
    coapOption.mValue  = reinterpret_cast<uint8_t *>(&aValue);

    // skip preceding zeros
    while (coapOption.mValue[0] == 0 && coapOption.mLength > 0)
    {
        coapOption.mValue++;
        coapOption.mLength--;
    }

    return AppendOption(coapOption);
}

otError Header::AppendObserveOption(uint32_t aObserve)
{
    return AppendUintOption(OT_COAP_OPTION_OBSERVE, aObserve & 0xFFFFFF);
}

otError Header::AppendUriPathOptions(const char *aUriPath)
{
    otError        error = OT_ERROR_NONE;
    const char *   cur   = aUriPath;
    const char *   end;
    Header::Option coapOption;

    coapOption.mNumber = OT_COAP_OPTION_URI_PATH;

    while ((end = strchr(cur, '/')) != NULL)
    {
        coapOption.mLength = static_cast<uint16_t>(end - cur);
        coapOption.mValue  = reinterpret_cast<const uint8_t *>(cur);
        SuccessOrExit(error = AppendOption(coapOption));
        cur = end + 1;
    }

    coapOption.mLength = static_cast<uint16_t>(strlen(cur));
    coapOption.mValue  = reinterpret_cast<const uint8_t *>(cur);
    SuccessOrExit(error = AppendOption(coapOption));

exit:
    return error;
}

otError Header::AppendContentFormatOption(otCoapOptionContentFormat aContentFormat)
{
    return AppendUintOption(OT_COAP_OPTION_CONTENT_FORMAT, static_cast<uint32_t>(aContentFormat));
}

otError Header::AppendMaxAgeOption(uint32_t aMaxAge)
{
    return AppendUintOption(OT_COAP_OPTION_MAX_AGE, aMaxAge);
}

otError Header::AppendUriQueryOption(const char *aUriQuery)
{
    Option coapOption;

    coapOption.mNumber = OT_COAP_OPTION_URI_QUERY;
    coapOption.mLength = static_cast<uint16_t>(strlen(aUriQuery));
    coapOption.mValue  = reinterpret_cast<const uint8_t *>(aUriQuery);

    return AppendOption(coapOption);
}

const Header::Option *Header::GetFirstOption(void)
{
    const Option *rval = NULL;

    VerifyOrExit(mFirstOptionOffset > 0);

    memset(&mOption, 0, sizeof(mOption));
    mNextOptionOffset = mFirstOptionOffset;

    rval = GetNextOption();

exit:
    return rval;
}

const Header::Option *Header::GetNextOption(void)
{
    Option * rval = NULL;
    uint16_t optionDelta;
    uint16_t optionLength;

    VerifyOrExit(mNextOptionOffset < mHeaderLength);

    optionDelta  = mHeader.mBytes[mNextOptionOffset] >> 4;
    optionLength = mHeader.mBytes[mNextOptionOffset] & 0xf;
    mNextOptionOffset += sizeof(uint8_t);

    if (optionDelta < kOption1ByteExtension)
    {
        // do nothing
    }
    else if (optionDelta == kOption1ByteExtension)
    {
        optionDelta = kOption1ByteExtensionOffset + mHeader.mBytes[mNextOptionOffset];
        mNextOptionOffset += sizeof(uint8_t);
    }
    else if (optionDelta == kOption2ByteExtension)
    {
        optionDelta = kOption2ByteExtensionOffset + static_cast<uint16_t>((mHeader.mBytes[mNextOptionOffset] << 8) |
                                                                          mHeader.mBytes[mNextOptionOffset + 1]);
        mNextOptionOffset += sizeof(uint16_t);
    }
    else
    {
        ExitNow();
    }

    if (optionLength < kOption1ByteExtension)
    {
        // do nothing
    }
    else if (optionLength == kOption1ByteExtension)
    {
        optionLength = kOption1ByteExtensionOffset + mHeader.mBytes[mNextOptionOffset];
        mNextOptionOffset += sizeof(uint8_t);
    }
    else if (optionLength == kOption2ByteExtension)
    {
        optionLength = kOption2ByteExtensionOffset + static_cast<uint16_t>((mHeader.mBytes[mNextOptionOffset] << 8) |
                                                                           mHeader.mBytes[mNextOptionOffset + 1]);
        mNextOptionOffset += sizeof(uint16_t);
    }
    else
    {
        ExitNow();
    }

    mOption.mNumber += optionDelta;
    mOption.mLength = optionLength;
    mOption.mValue  = mHeader.mBytes + mNextOptionOffset;
    mNextOptionOffset += optionLength;
    rval = static_cast<Header::Option *>(&mOption);

exit:
    return rval;
}

otError Header::SetPayloadMarker(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mHeaderLength < kMaxHeaderLength, error = OT_ERROR_NO_BUFS);
    mHeader.mBytes[mHeaderLength++] = 0xff;

exit:
    return error;
}

void Header::SetToken(uint8_t aTokenLength)
{
    uint8_t token[kMaxTokenLength] = {0};

    assert(aTokenLength <= kMaxTokenLength);

    Random::FillBuffer(token, aTokenLength);

    SetToken(token, aTokenLength);
}

void Header::SetDefaultResponseHeader(const Header &aRequestHeader)
{
    Init(OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CHANGED);
    SetMessageId(aRequestHeader.GetMessageId());
    SetToken(aRequestHeader.GetToken(), aRequestHeader.GetTokenLength());
}

} // namespace Coap
} // namespace ot
