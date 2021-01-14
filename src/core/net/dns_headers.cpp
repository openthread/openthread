/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements generating and processing of DNS headers and helper functions/methods.
 */

#include "dns_headers.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/random.hpp"
#include "common/string.hpp"

namespace ot {
namespace Dns {

using ot::Encoding::BigEndian::HostSwap16;

otError Header::SetRandomMessageId(void)
{
    return Random::Crypto::FillBuffer(reinterpret_cast<uint8_t *>(&mMessageId), sizeof(mMessageId));
}

otError Header::ResponseCodeToError(Response aResponse)
{
    otError error = OT_ERROR_FAILED;

    switch (aResponse)
    {
    case kResponseSuccess:
        error = OT_ERROR_NONE;
        break;

    case kResponseFormatError:   // Server unable to interpret request due to format error.
    case kResponseBadName:       // Bad name.
    case kResponseBadTruncation: // Bad truncation.
    case kResponseNotZone:       // A name is not in the zone.
        error = OT_ERROR_PARSE;
        break;

    case kResponseServerFailure: // Server encountered an internal failure.
        error = OT_ERROR_FAILED;
        break;

    case kResponseNameError:       // Name that ought to exist, does not exists.
    case kResponseRecordNotExists: // Some RRset that ought to exist, does not exist.
        error = OT_ERROR_NOT_FOUND;
        break;

    case kResponseNotImplemented: // Server does not support the query type (OpCode).
        error = OT_ERROR_NOT_IMPLEMENTED;
        break;

    case kResponseBadAlg: // Bad algorithm.
        error = OT_ERROR_NOT_CAPABLE;
        break;

    case kResponseNameExists:   // Some name that ought not to exist, does exist.
    case kResponseRecordExists: // Some RRset that ought not to exist, does exist.
        error = OT_ERROR_DUPLICATED;
        break;

    case kResponseRefused: // Server refused to perform operation for policy or security reasons.
    case kResponseNotAuth: // Service is not authoritative for zone.
        error = OT_ERROR_SECURITY;
        break;

    default:
        break;
    }

    return error;
}

otError Name::AppendLabel(const char *aLabel, Message &aMessage)
{
    return AppendLabel(aLabel, static_cast<uint8_t>(StringLength(aLabel, kMaxLabelLength + 1)), aMessage);
}

otError Name::AppendLabel(const char *aLabel, uint8_t aLabelLength, Message &aMessage)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit((0 < aLabelLength) && (aLabelLength <= kMaxLabelLength), error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = aMessage.Append(aLabelLength));
    error = aMessage.AppendBytes(aLabel, aLabelLength);

exit:
    return error;
}

otError Name::AppendMultipleLabels(const char *aLabels, Message &aMessage)
{
    otError  error           = OT_ERROR_NONE;
    uint16_t index           = 0;
    uint16_t labelStartIndex = 0;
    char     ch;

    VerifyOrExit(aLabels != nullptr);

    do
    {
        VerifyOrExit(index < kMaxLength, error = OT_ERROR_INVALID_ARGS);

        ch = aLabels[index];

        if ((ch == kNullChar) || (ch == kLabelSeperatorChar))
        {
            uint8_t labelLength = static_cast<uint8_t>(index - labelStartIndex);

            if (labelLength == 0)
            {
                // Empty label (e.g., consecutive dots) is invalid, but we
                // allow for two cases: (1) where `aLabels` ends with a dot
                // (`labelLength` is zero but we are at end of `aLabels` string
                // and `ch` is null char. (2) if `aLabels` is just "." (we
                // see a dot at index 0, and index 1 is null char).

                error = ((ch == kNullChar) || ((index == 0) && (aLabels[1] == kNullChar))) ? OT_ERROR_NONE
                                                                                           : OT_ERROR_INVALID_ARGS;
                ExitNow();
            }

            SuccessOrExit(error = AppendLabel(&aLabels[labelStartIndex], labelLength, aMessage));

            labelStartIndex = index + 1;
        }

        index++;

    } while (ch != kNullChar);

exit:
    return error;
}

otError Name::AppendTerminator(Message &aMessage)
{
    uint8_t terminator = 0;

    return aMessage.Append(terminator);
}

otError Name::AppendPointerLabel(uint16_t aOffset, Message &aMessage)
{
    // A pointer label takes the form of a two byte sequence as a
    // `uint16_t` value. The first two bits are ones. This allows a
    // pointer to be distinguished from a text label, since the text
    // label must begin with two zero bits (note that labels are
    // restricted to 63 octets or less). The next 14-bits specify
    // an offset value relative to start of DNS header.

    uint16_t value;

    OT_ASSERT(aOffset < kPointerLabelTypeUint16);

    value = HostSwap16(aOffset | kPointerLabelTypeUint16);

    return aMessage.Append(value);
}

otError Name::AppendName(const char *aName, Message &aMessage)
{
    otError error;

    SuccessOrExit(error = AppendMultipleLabels(aName, aMessage));
    error = AppendTerminator(aMessage);

exit:
    return error;
}

otError Name::ParseName(const Message &aMessage, uint16_t &aOffset)
{
    otError       error;
    LabelIterator iterator(aMessage, aOffset);

    while (true)
    {
        error = iterator.GetNextLabel();

        VerifyOrExit((error == OT_ERROR_NONE) || (error == OT_ERROR_NOT_FOUND));

        if (iterator.IsEndOffsetSet())
        {
            aOffset = iterator.mNameEndOffset;
            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

otError Name::ReadLabel(const Message &aMessage,
                        uint16_t &     aOffset,
                        uint16_t       aHeaderOffset,
                        char *         aLabelBuffer,
                        uint8_t &      aLabelLength)
{
    otError       error;
    LabelIterator iterator(aMessage, aOffset, aHeaderOffset);

    SuccessOrExit(error = iterator.GetNextLabel());
    SuccessOrExit(error = iterator.ReadLabel(aLabelBuffer, aLabelLength, /* aAllowDotCharInLabel */ true));
    aOffset = iterator.mNextLabelOffset;

exit:
    return error;
}

otError Name::ReadName(const Message &aMessage,
                       uint16_t &     aOffset,
                       uint16_t       aHeaderOffset,
                       char *         aNameBuffer,
                       uint16_t       aNameBufferSize)
{
    otError       error;
    LabelIterator iterator(aMessage, aOffset, aHeaderOffset);
    bool          firstLabel = true;
    uint8_t       labelLength;

    while (true)
    {
        error = iterator.GetNextLabel();

        switch (error)
        {
        case OT_ERROR_NONE:

            if (!firstLabel)
            {
                *aNameBuffer++ = kLabelSeperatorChar;
                aNameBufferSize--;

                // No need to check if we have reached end of the name buffer
                // here since `iterator.ReadLabel()` would verify it.
            }

            labelLength = static_cast<uint8_t>(OT_MIN(kMaxLabelLength + 1, aNameBufferSize));
            SuccessOrExit(error = iterator.ReadLabel(aNameBuffer, labelLength, /* aAllowDotCharInLabel */ false));
            aNameBuffer += labelLength;
            aNameBufferSize -= labelLength;
            firstLabel = false;
            break;

        case OT_ERROR_NOT_FOUND:
            // We reach the end of name successfully. Always add a terminating dot
            // at the end.
            *aNameBuffer++ = kLabelSeperatorChar;
            aNameBufferSize--;
            VerifyOrExit(aNameBufferSize >= sizeof(uint8_t), error = OT_ERROR_NO_BUFS);
            *aNameBuffer = kNullChar;
            aOffset      = iterator.mNameEndOffset;
            error        = OT_ERROR_NONE;

            // Fall through

        default:
            ExitNow();
        }
    }

exit:
    return error;
}

otError Name::LabelIterator::GetNextLabel(void)
{
    otError error;

    while (true)
    {
        uint8_t labelLength;
        uint8_t labelType;

        SuccessOrExit(error = mMessage.Read(mNextLabelOffset, labelLength));

        labelType = labelLength & kLabelTypeMask;

        if (labelType == kTextLabelType)
        {
            if (labelLength == 0)
            {
                // Zero label length indicates end of a name.

                if (!IsEndOffsetSet())
                {
                    mNameEndOffset = mNextLabelOffset + sizeof(uint8_t);
                }

                ExitNow(error = OT_ERROR_NOT_FOUND);
            }

            mLabelStartOffset = mNextLabelOffset + sizeof(uint8_t);
            mLabelLength      = labelLength;
            mNextLabelOffset  = mLabelStartOffset + labelLength;
            ExitNow();
        }
        else if (labelType == kPointerLabelType)
        {
            // A pointer label takes the form of a two byte sequence as a
            // `uint16_t` value. The first two bits are ones. The next 14 bits
            // specify an offset value from the start of the DNS header.

            uint16_t pointerValue;

            SuccessOrExit(error = mMessage.Read(mNextLabelOffset, pointerValue));

            if (!IsEndOffsetSet())
            {
                mNameEndOffset = mNextLabelOffset + sizeof(uint16_t);
            }

            mNextLabelOffset = mHeaderOffset + (HostSwap16(pointerValue) & kPointerLabelOffsetMask);

            // Go back through the `while(true)` loop to get the next label.
        }
        else
        {
            ExitNow(error = OT_ERROR_PARSE);
        }
    }

exit:
    return error;
}

otError Name::LabelIterator::ReadLabel(char *aLabelBuffer, uint8_t &aLabelLength, bool aAllowDotCharInLabel) const
{
    otError error;

    VerifyOrExit(mLabelLength < aLabelLength, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = mMessage.Read(mLabelStartOffset, aLabelBuffer, mLabelLength));
    aLabelBuffer[mLabelLength] = kNullChar;
    aLabelLength               = mLabelLength;

    if (!aAllowDotCharInLabel)
    {
        VerifyOrExit(StringFind(aLabelBuffer, kLabelSeperatorChar) == nullptr, error = OT_ERROR_PARSE);
    }

exit:
    return error;
}

} // namespace Dns
} // namespace ot
