/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
#include <common/code_utils.hpp>
#include <common/encoding.hpp>

namespace Thread {
namespace Coap {

void Header::Init(void)
{
    mHeaderLength = kMinHeaderLength;
    mOptionLast = 0;
    mNextOptionOffset = 0;
    memset(&mOption, 0, sizeof(mOption));
    memset(mHeader, 0, sizeof(mHeader));
}

ThreadError Header::FromMessage(const Message &aMessage)
{
    ThreadError error = kThreadError_Parse;
    uint16_t offset = aMessage.GetOffset();
    uint16_t length = aMessage.GetLength() - aMessage.GetOffset();
    uint8_t tokenLength;
    bool firstOption = true;
    uint16_t optionDelta;
    uint16_t optionLength;

    VerifyOrExit(length >= kTokenOffset, error = kThreadError_Parse);
    aMessage.Read(offset, kTokenOffset, mHeader);
    mHeaderLength = kTokenOffset;
    offset += kTokenOffset;
    length -= kTokenOffset;

    VerifyOrExit(GetVersion() == 1, error = kThreadError_Parse);

    tokenLength = GetTokenLength();
    VerifyOrExit(tokenLength <= kMaxTokenLength && tokenLength < length, error = kThreadError_Parse);
    aMessage.Read(offset, tokenLength, mHeader + mHeaderLength);
    mHeaderLength += tokenLength;
    offset += tokenLength;
    length -= tokenLength;

    while (length > 0)
    {
        aMessage.Read(offset, 5, mHeader + mHeaderLength);

        if (mHeader[mHeaderLength] == 0xff)
        {
            mHeaderLength += sizeof(uint8_t);
            ExitNow(error = kThreadError_None);
        }

        optionDelta = mHeader[mHeaderLength] >> 4;
        optionLength = mHeader[mHeaderLength] & 0xf;
        mHeaderLength += sizeof(uint8_t);
        offset += sizeof(uint8_t);
        length -= sizeof(uint8_t);

        if (optionDelta < kOption1ByteExtension)
        {
            // do nothing
        }
        else if (optionDelta == kOption1ByteExtension)
        {
            optionDelta = kOption1ByteExtensionOffset + mHeader[mHeaderLength];
            mHeaderLength += sizeof(uint8_t);
            offset += sizeof(uint8_t);
            length -= sizeof(uint8_t);
        }
        else if (optionDelta == kOption2ByteExtension)
        {
            optionDelta = kOption2ByteExtensionOffset +
                          static_cast<uint16_t>((mHeader[mHeaderLength] << 8) | mHeader[mHeaderLength + 1]);
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
            optionLength = kOption1ByteExtensionOffset + mHeader[mHeaderLength];
            mHeaderLength += sizeof(uint8_t);
            offset += sizeof(uint8_t);
            length -= sizeof(uint8_t);
        }
        else if (optionLength == kOption2ByteExtension)
        {
            optionLength = kOption2ByteExtensionOffset +
                           static_cast<uint16_t>((mHeader[mHeaderLength] << 8) | mHeader[mHeaderLength + 1]);
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
            mOption.mValue = mHeader + mHeaderLength;
            mNextOptionOffset = mHeaderLength + optionLength;
            firstOption = false;
        }

        VerifyOrExit(optionLength <= length, error = kThreadError_Parse);
        aMessage.Read(offset, optionLength, mHeader + mHeaderLength);
        mHeaderLength += static_cast<uint8_t>(optionLength);
        offset += optionLength;
        length -= optionLength;
    }

exit:
    return error;
}

ThreadError Header::AppendOption(const Option &aOption)
{
    uint8_t *buf = mHeader + mHeaderLength;
    uint8_t *cur = buf + 1;
    uint16_t optionDelta = aOption.mNumber - mOptionLast;
    uint16_t optionLength;

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

    memcpy(cur, aOption.mValue, aOption.mLength);
    cur += aOption.mLength;

    mHeaderLength += static_cast<uint8_t>(cur - buf);
    mOptionLast = aOption.mNumber;

    return kThreadError_None;
}

ThreadError Header::AppendUriPathOptions(const char *aUriPath)
{
    const char *cur = aUriPath;
    const char *end;
    Header::Option coapOption;

    coapOption.mNumber = Option::kOptionUriPath;

    while ((end = strchr(cur, '/')) != NULL)
    {
        coapOption.mLength = static_cast<uint16_t>(end - cur);
        coapOption.mValue = reinterpret_cast<const uint8_t *>(cur);
        AppendOption(coapOption);
        cur = end + 1;
    }

    coapOption.mLength = static_cast<uint16_t>(strlen(cur));
    coapOption.mValue = reinterpret_cast<const uint8_t *>(cur);
    AppendOption(coapOption);

    return kThreadError_None;
}

ThreadError Header::AppendContentFormatOption(MediaType aType)
{
    Option coapOption;
    uint8_t type = static_cast<uint8_t>(aType);

    coapOption.mNumber = Option::kOptionContentFormat;
    coapOption.mLength = 1;
    coapOption.mValue = &type;
    AppendOption(coapOption);

    return kThreadError_None;
}

const Header::Option *Header::GetCurrentOption(void) const
{
    return &mOption;
}

const Header::Option *Header::GetNextOption(void)
{
    Option *rval = NULL;
    uint16_t optionDelta;
    uint16_t optionLength;

    VerifyOrExit(mNextOptionOffset < mHeaderLength, ;);

    optionDelta = mHeader[mNextOptionOffset] >> 4;
    optionLength = mHeader[mNextOptionOffset] & 0xf;
    mNextOptionOffset += sizeof(uint8_t);

    if (optionDelta < kOption1ByteExtension)
    {
        // do nothing
    }
    else if (optionDelta == kOption1ByteExtension)
    {
        optionDelta = kOption1ByteExtensionOffset + mHeader[mNextOptionOffset];
        mNextOptionOffset += sizeof(uint8_t);
    }
    else if (optionDelta == kOption2ByteExtension)
    {
        optionDelta = kOption2ByteExtensionOffset +
                      static_cast<uint16_t>((mHeader[mNextOptionOffset] << 8) | mHeader[mNextOptionOffset + 1]);
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
        optionLength = kOption1ByteExtensionOffset + mHeader[mNextOptionOffset];
        mNextOptionOffset += sizeof(uint8_t);
    }
    else if (optionLength == kOption2ByteExtension)
    {
        optionLength = kOption2ByteExtensionOffset +
                       static_cast<uint16_t>((mHeader[mNextOptionOffset] << 8) | mHeader[mNextOptionOffset + 1]);
        mNextOptionOffset += sizeof(uint16_t);
    }
    else
    {
        ExitNow();
    }

    mOption.mNumber += optionDelta;
    mOption.mLength = optionLength;
    mOption.mValue = mHeader + mNextOptionOffset;
    mNextOptionOffset += optionLength;
    rval = &mOption;

exit:
    return rval;
}

}  // namespace Coap
}  // namespace Thread
