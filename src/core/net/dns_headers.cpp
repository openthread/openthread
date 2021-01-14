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

            VerifyOrExit(index + 1 < kMaxEncodedLength, error = OT_ERROR_INVALID_ARGS);
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

        switch (error)
        {
        case OT_ERROR_NONE:
            break;

        case OT_ERROR_NOT_FOUND:
            // We reached the end of name successfully.
            aOffset = iterator.mNameEndOffset;
            error   = OT_ERROR_NONE;

            OT_FALL_THROUGH;

        default:
            ExitNow();
        }
    }

exit:
    return error;
}

otError Name::ReadLabel(const Message &aMessage, uint16_t &aOffset, char *aLabelBuffer, uint8_t &aLabelLength)
{
    otError       error;
    LabelIterator iterator(aMessage, aOffset);

    SuccessOrExit(error = iterator.GetNextLabel());
    SuccessOrExit(error = iterator.ReadLabel(aLabelBuffer, aLabelLength, /* aAllowDotCharInLabel */ true));
    aOffset = iterator.mNextLabelOffset;

exit:
    return error;
}

otError Name::ReadName(const Message &aMessage, uint16_t &aOffset, char *aNameBuffer, uint16_t aNameBufferSize)
{
    otError       error;
    LabelIterator iterator(aMessage, aOffset);
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

            OT_FALL_THROUGH;

        default:
            ExitNow();
        }
    }

exit:
    return error;
}

otError Name::CompareLabel(const Message &aMessage, uint16_t &aOffset, const char *aLabel)
{
    otError       error;
    LabelIterator iterator(aMessage, aOffset);

    SuccessOrExit(error = iterator.GetNextLabel());
    VerifyOrExit(iterator.CompareLabel(aLabel, /* aIsSingleLabel */ true), error = OT_ERROR_NOT_FOUND);
    aOffset = iterator.mNextLabelOffset;

exit:
    return error;
}

otError Name::CompareName(const Message &aMessage, uint16_t &aOffset, const char *aName)
{
    otError       error;
    LabelIterator iterator(aMessage, aOffset);
    bool          matches = true;

    if (*aName == kLabelSeperatorChar)
    {
        aName++;
        VerifyOrExit(*aName == kNullChar, error = OT_ERROR_INVALID_ARGS);
    }

    while (true)
    {
        error = iterator.GetNextLabel();

        switch (error)
        {
        case OT_ERROR_NONE:
            if (matches && !iterator.CompareLabel(aName, /* aIsSingleLabel */ false))
            {
                matches = false;
            }

            break;

        case OT_ERROR_NOT_FOUND:
            // We reached the end of the name in `aMessage`. We check if
            // all the previous labels matched so far, and we are also
            // at the end of `aName` string (see null char), then we
            // return `OT_ERROR_NONE` indicating a successful comparison
            // (full match). Otherwise we return `OT_ERROR_NOT_FOUND` to
            // indicate failed comparison.

            if (matches && (*aName == kNullChar))
            {
                error = OT_ERROR_NONE;
            }

            aOffset = iterator.mNameEndOffset;

            OT_FALL_THROUGH;

        default:
            ExitNow();
        }
    }

exit:
    return error;
}

otError Name::CompareName(const Message &aMessage, uint16_t &aOffset, const Message &aMessage2, uint16_t aOffset2)
{
    otError       error;
    LabelIterator iterator(aMessage, aOffset);
    LabelIterator iterator2(aMessage2, aOffset2);
    bool          matches = true;

    while (true)
    {
        error = iterator.GetNextLabel();

        switch (error)
        {
        case OT_ERROR_NONE:
            // If all the previous labels matched so far, then verify
            // that we can get the next label on `iterator2` and that it
            // matches the label from `iterator`.
            if (matches && (iterator2.GetNextLabel() != OT_ERROR_NONE || !iterator.CompareLabel(iterator2)))
            {
                matches = false;
            }

            break;

        case OT_ERROR_NOT_FOUND:
            // We reached the end of the name in `aMessage`. We check
            // that `iterator2` is also at its end, and if all previous
            // labels matched we return `OT_ERROR_NONE`.

            if (matches && (iterator2.GetNextLabel() == OT_ERROR_NOT_FOUND))
            {
                error = OT_ERROR_NONE;
            }

            aOffset = iterator.mNameEndOffset;

            OT_FALL_THROUGH;

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

            // `mMessage.GetOffset()` must point to the start of the
            // DNS header.
            mNextLabelOffset = mMessage.GetOffset() + (HostSwap16(pointerValue) & kPointerLabelOffsetMask);

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

bool Name::LabelIterator::CompareLabel(const char *&aName, bool aIsSingleLabel) const
{
    // This method compares the current label in the iterator with the
    // `aName` string. `aIsSingleLabel` indicates whether `aName` is a
    // single label, or a sequence of labels separated by dot '.' char.
    // If the label matches `aName`, then `aName` pointer is moved
    // forward to the start of the next label (skipping over the `.`
    // char). This method returns `true` when the labels match, `false`
    // otherwise.

    bool matches = false;

    VerifyOrExit(StringLength(aName, mLabelLength) == mLabelLength);
    matches = mMessage.CompareBytes(mLabelStartOffset, aName, mLabelLength);

    VerifyOrExit(matches);

    aName += mLabelLength;

    // If `aName` is a single label, we should be also at the end of the
    // `aName` string. Otherwise, we should see either null or dot '.'
    // character (in case `aName` contains multiple labels).

    matches = (*aName == kNullChar);

    if (!aIsSingleLabel && (*aName == kLabelSeperatorChar))
    {
        matches = true;
        aName++;
    }

exit:
    return matches;
}

bool Name::LabelIterator::CompareLabel(const LabelIterator &aOtherIterator) const
{
    // This method compares the current label in the iterator with the
    // label from another iterator.

    return (mLabelLength == aOtherIterator.mLabelLength) &&
           mMessage.CompareBytes(mLabelStartOffset, aOtherIterator.mMessage, aOtherIterator.mLabelStartOffset,
                                 mLabelLength);
}

bool AaaaRecord::IsValid(void) const
{
    return GetType() == Dns::ResourceRecord::kTypeAaaa && GetSize() == sizeof(*this);
}

bool KeyRecord::IsValid(void) const
{
    return GetType() == Dns::ResourceRecord::kTypeKey;
}

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
void Ecdsa256KeyRecord::Init(void)
{
    KeyRecord::Init();
    SetAlgorithm(kAlgorithmEcdsaP256Sha256);
}

bool Ecdsa256KeyRecord::IsValid(void) const
{
    return KeyRecord::IsValid() && GetLength() == sizeof(*this) - sizeof(ResourceRecord) &&
           GetAlgorithm() == kAlgorithmEcdsaP256Sha256;
}
#endif

bool SigRecord::IsValid(void) const
{
    return GetType() == Dns::ResourceRecord::kTypeSig && GetLength() >= sizeof(*this) - sizeof(ResourceRecord);
}

bool LeaseOption::IsValid(void) const
{
    return GetLeaseInterval() <= GetKeyLeaseInterval();
}

} // namespace Dns
} // namespace ot
