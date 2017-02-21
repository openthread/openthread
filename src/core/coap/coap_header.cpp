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

#include <coap/coap_header.hpp>
#include <coap/coap_client.hpp>
#include <common/debug.hpp>
#include <common/code_utils.hpp>
#include <common/encoding.hpp>
#include <platform/random.h>

namespace Thread {
namespace Coap {

void Header::Init(void)
{
    mHeaderLength = kMinHeaderLength;
    mOptionLast = 0;
    mNextOptionOffset = 0;
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

ThreadError Header::FromMessage(const Message &aMessage, bool aCopiedMessage)
{
    ThreadError error = kThreadError_Parse;
    uint16_t offset = aMessage.GetOffset();
    uint16_t length = aMessage.GetLength() - aMessage.GetOffset();
    uint8_t tokenLength;
    bool firstOption = true;
    uint16_t optionDelta;
    uint16_t optionLength;

    if (aCopiedMessage)
    {
        length -= sizeof(RequestMetadata);
    }

    VerifyOrExit(length >= kTokenOffset, error = kThreadError_Parse);
    aMessage.Read(offset, kTokenOffset, mHeader.mBytes);
    mHeaderLength = kTokenOffset;
    offset += kTokenOffset;
    length -= kTokenOffset;

    VerifyOrExit(GetVersion() == 1, error = kThreadError_Parse);

    tokenLength = GetTokenLength();
    VerifyOrExit(tokenLength <= kMaxTokenLength && tokenLength <= length, error = kThreadError_Parse);
    aMessage.Read(offset, tokenLength, mHeader.mBytes + mHeaderLength);
    mHeaderLength += tokenLength;
    offset += tokenLength;
    length -= tokenLength;

    while (length > 0)
    {
        aMessage.Read(offset, kMaxOptionHeaderSize, mHeader.mBytes + mHeaderLength);

        if (mHeader.mBytes[mHeaderLength] == 0xff)
        {
            mHeaderLength += sizeof(uint8_t);
            ExitNow(error = kThreadError_None);
        }

        optionDelta = mHeader.mBytes[mHeaderLength] >> 4;
        optionLength = mHeader.mBytes[mHeaderLength] & 0xf;
        mHeaderLength += sizeof(uint8_t);
        offset += sizeof(uint8_t);
        length -= sizeof(uint8_t);

        if (optionDelta < kOption1ByteExtension)
        {
            // do nothing
        }
        else if (optionDelta == kOption1ByteExtension)
        {
            optionDelta = kOption1ByteExtensionOffset + mHeader.mBytes[mHeaderLength];
            mHeaderLength += sizeof(uint8_t);
            offset += sizeof(uint8_t);
            length -= sizeof(uint8_t);
        }
        else if (optionDelta == kOption2ByteExtension)
        {
            optionDelta = kOption2ByteExtensionOffset +
                          static_cast<uint16_t>((mHeader.mBytes[mHeaderLength] << 8) | mHeader.mBytes[mHeaderLength + 1]);
            mHeaderLength += sizeof(uint16_t);
            offset += sizeof(uint16_t);
            length -= sizeof(uint16_t);
        }
        else
        {
            ExitNow(error = kThreadError_Parse);
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
            length -= sizeof(uint8_t);
        }
        else if (optionLength == kOption2ByteExtension)
        {
            optionLength = kOption2ByteExtensionOffset +
                           static_cast<uint16_t>((mHeader.mBytes[mHeaderLength] << 8) | mHeader.mBytes[mHeaderLength + 1]);
            mHeaderLength += sizeof(uint16_t);
            offset += sizeof(uint16_t);
            length -= sizeof(uint16_t);
        }
        else
        {
            ExitNow(error = kThreadError_Parse);
        }

        if (firstOption)
        {
            mOption.mNumber = optionDelta;
            mOption.mLength = optionLength;
            mOption.mValue = mHeader.mBytes + mHeaderLength;
            mNextOptionOffset = mHeaderLength + optionLength;
            firstOption = false;
        }

        VerifyOrExit(optionLength <= length, error = kThreadError_Parse);
        aMessage.Read(offset, optionLength, mHeader.mBytes + mHeaderLength);
        mHeaderLength += static_cast<uint8_t>(optionLength);
        offset += optionLength;
        length -= optionLength;
    }

    if (length == 0)
    {
        // No payload present - return success.
        error = kThreadError_None;
    }

exit:
    return error;
}

ThreadError Header::AppendOption(const Option &aOption)
{
    ThreadError error = kThreadError_None;
    uint8_t *buf = mHeader.mBytes + mHeaderLength;
    uint8_t *cur = buf + 1;
    uint16_t optionDelta = aOption.mNumber - mOptionLast;
    uint16_t optionLength;

    // Assure that no option is inserted out of order.
    VerifyOrExit(aOption.mNumber >= mOptionLast, error = kThreadError_InvalidArgs);

    // Calculate the total option size and check the buffers.
    optionLength = 1 + aOption.mLength;
    optionLength += optionDelta < kOption1ByteExtensionOffset ? 0 :
                    (optionDelta < kOption2ByteExtensionOffset ? 1 : 2);
    optionLength += aOption.mLength < kOption1ByteExtensionOffset ? 0 :
                    (aOption.mLength < kOption2ByteExtensionOffset ? 1 : 2);
    VerifyOrExit(mHeaderLength + optionLength < kMaxHeaderLength, error = kThreadError_NoBufs);

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
        *cur++ = optionLength >> 8;
        *cur++ = optionLength & 0xff;
    }

    // Insert option value.
    memcpy(cur, aOption.mValue, aOption.mLength);
    cur += aOption.mLength;

    mHeaderLength += static_cast<uint8_t>(cur - buf);
    mOptionLast = aOption.mNumber;

exit:
    return error;
}

ThreadError Header::AppendObserveOption(uint32_t aObserve)
{
    Option coapOption;
    uint8_t buf[3];

    coapOption.mNumber = kCoapOptionObserve;

    if (aObserve <= 0xff) {
        buf[0] = aObserve;
        coapOption.mLength = 1;
    } else if (aObserve <= 0xffff) {
        buf[0] = aObserve >> 8;
        buf[1] = aObserve;
        coapOption.mLength = 2;
    } else {
        buf[0] = aObserve >> 16;
        buf[1] = aObserve >> 8;
        buf[2] = aObserve;
        coapOption.mLength = 3;
    }

    coapOption.mValue = buf;

    return AppendOption(coapOption);
}

ThreadError Header::AppendUriPathOptions(const char *aUriPath)
{
    ThreadError error = kThreadError_None;
    const char *cur = aUriPath;
    const char *end;
    Header::Option coapOption;

    coapOption.mNumber = kCoapOptionUriPath;

    while ((end = strchr(cur, '/')) != NULL)
    {
        coapOption.mLength = static_cast<uint16_t>(end - cur);
        coapOption.mValue = reinterpret_cast<const uint8_t *>(cur);
        SuccessOrExit(error = AppendOption(coapOption));
        cur = end + 1;
    }

    coapOption.mLength = static_cast<uint16_t>(strlen(cur));
    coapOption.mValue = reinterpret_cast<const uint8_t *>(cur);
    SuccessOrExit(error = AppendOption(coapOption));

exit:
    return error;
}

ThreadError Header::AppendContentFormatOption(MediaType aType)
{
    Option coapOption;
    uint8_t type = static_cast<uint8_t>(aType);

    coapOption.mNumber = kCoapOptionContentFormat;
    coapOption.mLength = 1;
    coapOption.mValue = &type;

    return AppendOption(coapOption);
}

ThreadError Header::AppendMaxAgeOption(uint32_t aMaxAge)
{
    Option coapOption;
    uint8_t buf[4];

    coapOption.mNumber = kCoapOptionMaxAge;

    if (aMaxAge <= 0xff) {
        buf[0] = aMaxAge;
        coapOption.mLength = 1;
    } else if (aMaxAge <= 0xffff) {
        buf[0] = aMaxAge >> 8;
        buf[1] = aMaxAge;
        coapOption.mLength = 2;
    } else if (aMaxAge <= 0xffffff) {
        buf[0] = aMaxAge >> 16;
        buf[1] = aMaxAge >> 8;
        buf[2] = aMaxAge;
        coapOption.mLength = 3;
    } else {
        buf[0] = aMaxAge >> 24;
        buf[1] = aMaxAge >> 16;
        buf[2] = aMaxAge >> 8;
        buf[3] = aMaxAge;
        coapOption.mLength = 4;
    }

    coapOption.mValue = buf;

    return AppendOption(coapOption);
}

ThreadError Header::AppendUriQueryOption(const char *aUriQuery)
{
    Option coapOption;

    coapOption.mNumber = kCoapOptionUriQuery;
    coapOption.mLength = strlen(aUriQuery);
    coapOption.mValue = (uint8_t *)aUriQuery;

    return AppendOption(coapOption);
}

const Header::Option *Header::GetCurrentOption(void) const
{
    return static_cast<const Header::Option *>(&mOption);
}

const Header::Option *Header::GetNextOption(void)
{
    Option *rval = NULL;
    uint16_t optionDelta;
    uint16_t optionLength;

    VerifyOrExit(mNextOptionOffset < mHeaderLength, ;);

    optionDelta = mHeader.mBytes[mNextOptionOffset] >> 4;
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
        optionDelta = kOption2ByteExtensionOffset +
                      static_cast<uint16_t>((mHeader.mBytes[mNextOptionOffset] << 8) | mHeader.mBytes[mNextOptionOffset + 1]);
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
        optionLength = kOption2ByteExtensionOffset +
                       static_cast<uint16_t>((mHeader.mBytes[mNextOptionOffset] << 8) | mHeader.mBytes[mNextOptionOffset + 1]);
        mNextOptionOffset += sizeof(uint16_t);
    }
    else
    {
        ExitNow();
    }

    mOption.mNumber += optionDelta;
    mOption.mLength = optionLength;
    mOption.mValue = mHeader.mBytes + mNextOptionOffset;
    mNextOptionOffset += optionLength;
    rval = static_cast<Header::Option *>(&mOption);

exit:
    return rval;
}

ThreadError Header::SetPayloadMarker(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mHeaderLength < kMaxHeaderLength, error = kThreadError_NoBufs);
    mHeader.mBytes[mHeaderLength++] = 0xff;

exit:
    return error;
}

void Header::SetToken(uint8_t aTokenLength)
{
    assert(aTokenLength <= kMaxTokenLength);

    uint8_t token[kMaxTokenLength] = { 0 };

    for (uint8_t i = 0; i < aTokenLength; i++)
    {
        token[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    SetToken(token, aTokenLength);
}

void Header::SetDefaultResponseHeader(const Header &aRequestHeader)
{
    Init(kCoapTypeAcknowledgment, kCoapResponseChanged);
    SetMessageId(aRequestHeader.GetMessageId());
    SetToken(aRequestHeader.GetToken(), aRequestHeader.GetTokenLength());
}

}  // namespace Coap
}  // namespace Thread
