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
#include "common/logging.hpp"
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
    return AppendLabel(aLabel, static_cast<uint8_t>(StringLength(aLabel, kMaxLabelSize)), aMessage);
}

otError Name::AppendLabel(const char *aLabel, uint8_t aLength, Message &aMessage)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit((0 < aLength) && (aLength <= kMaxLabelLength), error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = aMessage.Append(aLength));
    error = aMessage.AppendBytes(aLabel, aLength);

exit:
    return error;
}

otError Name::AppendMultipleLabels(const char *aLabels, Message &aMessage)
{
    return AppendMultipleLabels(aLabels, kMaxNameLength, aMessage);
}

otError Name::AppendMultipleLabels(const char *aLabels, uint8_t aLength, Message &aMessage)
{
    otError  error           = OT_ERROR_NONE;
    uint16_t index           = 0;
    uint16_t labelStartIndex = 0;
    char     ch;

    VerifyOrExit(aLabels != nullptr);

    do
    {
        ch = index < aLength ? aLabels[index] : static_cast<char>(kNullChar);

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

            labelLength = static_cast<uint8_t>(OT_MIN(static_cast<uint8_t>(kMaxLabelSize), aNameBufferSize));
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

otError Name::CompareName(const Message &aMessage, uint16_t &aOffset, const Name &aName)
{
    return aName.IsFromCString()
               ? CompareName(aMessage, aOffset, aName.mString)
               : (aName.IsFromMessage() ? CompareName(aMessage, aOffset, *aName.mMessage, aName.mOffset)
                                        : ParseName(aMessage, aOffset));
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

bool Name::IsSubDomainOf(const char *aName, const char *aDomain)
{
    bool     match        = false;
    uint16_t nameLength   = StringLength(aName, kMaxNameLength);
    uint16_t domainLength = StringLength(aDomain, kMaxNameLength);

    if (nameLength > 0 && aName[nameLength - 1] == kLabelSeperatorChar)
    {
        --nameLength;
    }

    if (domainLength > 0 && aDomain[domainLength - 1] == kLabelSeperatorChar)
    {
        --domainLength;
    }

    VerifyOrExit(nameLength >= domainLength);
    aName += nameLength - domainLength;

    if (nameLength > domainLength)
    {
        VerifyOrExit(aName[-1] == kLabelSeperatorChar);
    }
    VerifyOrExit(memcmp(aName, aDomain, domainLength) == 0);

    match = true;

exit:
    return match;
}

otError ResourceRecord::ParseRecords(const Message &aMessage, uint16_t &aOffset, uint16_t aNumRecords)
{
    otError error = OT_ERROR_NONE;

    while (aNumRecords > 0)
    {
        ResourceRecord record;

        SuccessOrExit(error = Name::ParseName(aMessage, aOffset));
        SuccessOrExit(error = record.ReadFrom(aMessage, aOffset));
        aOffset += static_cast<uint16_t>(record.GetSize());
        aNumRecords--;
    }

exit:
    return error;
}

otError ResourceRecord::FindRecord(const Message &aMessage, uint16_t &aOffset, uint16_t &aNumRecords, const Name &aName)
{
    otError error;

    while (aNumRecords > 0)
    {
        bool           matches = true;
        ResourceRecord record;

        error = Name::CompareName(aMessage, aOffset, aName);

        switch (error)
        {
        case OT_ERROR_NONE:
            break;
        case OT_ERROR_NOT_FOUND:
            matches = false;
            break;
        default:
            ExitNow();
        }

        SuccessOrExit(error = record.ReadFrom(aMessage, aOffset));
        aNumRecords--;
        VerifyOrExit(!matches);
        aOffset += static_cast<uint16_t>(record.GetSize());
    }

    error = OT_ERROR_NOT_FOUND;

exit:
    return error;
}

otError ResourceRecord::FindRecord(const Message & aMessage,
                                   uint16_t &      aOffset,
                                   uint16_t        aNumRecords,
                                   uint16_t        aIndex,
                                   const Name &    aName,
                                   uint16_t        aType,
                                   ResourceRecord &aRecord,
                                   uint16_t        aMinRecordSize)
{
    // This static method searches in `aMessage` starting from `aOffset`
    // up to maximum of `aNumRecords`, for the `(aIndex+1)`th
    // occurrence of a resource record of type `aType` with record name
    // matching `aName`. It also verifies that the record size is larger
    // than `aMinRecordSize`. If found, `aMinRecordSize` bytes from the
    // record are read and copied into `aRecord`. In this case `aOffset`
    // is updated to point to the last record byte read from the message
    // (so that the caller can read any remaining fields in the record
    // data).

    otError  error;
    uint16_t offset = aOffset;
    uint16_t recordOffset;

    while (aNumRecords > 0)
    {
        SuccessOrExit(error = FindRecord(aMessage, offset, aNumRecords, aName));

        // Save the offset to start of `ResourceRecord` fields.
        recordOffset = offset;

        error = ReadRecord(aMessage, offset, aType, aRecord, aMinRecordSize);

        if (error == OT_ERROR_NOT_FOUND)
        {
            // `ReadRecord()` already updates the `offset` to skip
            // over a non-matching record.
            continue;
        }

        SuccessOrExit(error);

        if (aIndex == 0)
        {
            aOffset = offset;
            ExitNow();
        }

        aIndex--;

        // Skip over the record.
        offset = static_cast<uint16_t>(recordOffset + aRecord.GetSize());
    }

    error = OT_ERROR_NOT_FOUND;

exit:
    return error;
}

otError ResourceRecord::ReadRecord(const Message & aMessage,
                                   uint16_t &      aOffset,
                                   uint16_t        aType,
                                   ResourceRecord &aRecord,
                                   uint16_t        aMinRecordSize)
{
    // This static method tries to read a matching resource record of a
    // given type and a minimum record size from a message. The `aType`
    // value of `kTypeAny` matches any type.  If the record in the
    // message does not match, it skips over the record. Please see
    // `ReadRecord<RecordType>()` for more details.

    otError        error;
    ResourceRecord record;

    SuccessOrExit(error = record.ReadFrom(aMessage, aOffset));

    if (((aType == kTypeAny) || (record.GetType() == aType)) && (record.GetSize() >= aMinRecordSize))
    {
        IgnoreError(aMessage.Read(aOffset, &aRecord, aMinRecordSize));
        aOffset += aMinRecordSize;
    }
    else
    {
        // Skip over the entire record.
        aOffset += static_cast<uint16_t>(record.GetSize());
        error = OT_ERROR_NOT_FOUND;
    }

exit:
    return error;
}

otError ResourceRecord::ReadName(const Message &aMessage,
                                 uint16_t &     aOffset,
                                 uint16_t       aStartOffset,
                                 char *         aNameBuffer,
                                 uint16_t       aNameBufferSize,
                                 bool           aSkipRecord) const
{
    // This protected method parses and reads a name field in a record
    // from a message. It is intended only for sub-classes of
    // `ResourceRecord`.
    //
    // On input `aOffset` gives the offset in `aMessage` to the start of
    // name field. `aStartOffset` gives the offset to the start of the
    // `ResourceRecord`. `aSkipRecord` indicates whether to skip over
    // the entire resource record or just the read name. On exit, when
    // successfully read, `aOffset` is updated to either point after the
    // end of record or after the the name field.
    //
    // When read successfully, this method returns `OT_ERROR_NONE`. On a
    // parse error (invalid format) returns `OT_ERROR_PARSE`. If the
    // name does not fit in the given name buffer it returns
    // `OT_ERROR_NO_BUFS`

    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = Name::ReadName(aMessage, aOffset, aNameBuffer, aNameBufferSize));
    VerifyOrExit(aOffset <= aStartOffset + GetSize(), error = OT_ERROR_PARSE);

    VerifyOrExit(aSkipRecord);
    aOffset = aStartOffset;
    error   = SkipRecord(aMessage, aOffset);

exit:
    return error;
}

otError ResourceRecord::SkipRecord(const Message &aMessage, uint16_t &aOffset) const
{
    // This protected method parses and skips over a resource record
    // in a message.
    //
    // On input `aOffset` gives the offset in `aMessage` to the start of
    // the `ResourceRecord`. On exit, when successfully parsed, `aOffset`
    // is updated to point to byte after the entire record.

    otError error;

    SuccessOrExit(error = CheckRecord(aMessage, aOffset));
    aOffset += static_cast<uint16_t>(GetSize());

exit:
    return error;
}

otError ResourceRecord::CheckRecord(const Message &aMessage, uint16_t aOffset) const
{
    // This method checks that the entire record (including record data)
    // is present in `aMessage` at `aOffset` (pointing to the start of
    // the `ResourceRecord` in `aMessage`).

    return (aOffset + GetSize() <= aMessage.GetLength()) ? OT_ERROR_NONE : OT_ERROR_PARSE;
}

otError ResourceRecord::ReadFrom(const Message &aMessage, uint16_t aOffset)
{
    // This method reads the `ResourceRecord` from `aMessage` at
    // `aOffset`. It verifies that the entire record (including record
    // data) is present in the message.

    otError error;

    SuccessOrExit(error = aMessage.Read(aOffset, *this));
    error = CheckRecord(aMessage, aOffset);

exit:
    return error;
}

otError TxtEntry::AppendTo(Message &aMessage) const
{
    otError error = OT_ERROR_NONE;
    uint8_t length;

    if (mKey == nullptr)
    {
        VerifyOrExit(mValue != nullptr);
        error = aMessage.AppendBytes(mValue, mValueLength);
        ExitNow();
    }

    length = mKeyLength;

    VerifyOrExit(length <= kMaxKeyLength, error = OT_ERROR_INVALID_ARGS);

    if (mValue == nullptr)
    {
        // Treat as a boolean attribute and encoded as "key" (with no `=`).
        SuccessOrExit(error = aMessage.Append(length));
        error = aMessage.AppendBytes(mKey, length);
        ExitNow();
    }

    // Treat as key/value and encode as "key=value", value may be empty.

    VerifyOrExit(mValueLength + length + sizeof(char) <= kMaxKeyValueEncodedSize, error = OT_ERROR_INVALID_ARGS);

    length += static_cast<uint8_t>(mValueLength + sizeof(char));

    SuccessOrExit(error = aMessage.Append(length));
    SuccessOrExit(error = aMessage.AppendBytes(mKey, length));
    SuccessOrExit(error = aMessage.Append<char>(kKeyValueSeparator));
    error = aMessage.AppendBytes(mValue, mValueLength);

exit:
    return error;
}

otError TxtEntry::AppendEntries(const TxtEntry *aEntries, uint8_t aNumEntries, Message &aMessage)
{
    otError  error       = OT_ERROR_NONE;
    uint16_t startOffset = aMessage.GetLength();

    for (uint8_t index = 0; index < aNumEntries; index++)
    {
        SuccessOrExit(error = aEntries[index].AppendTo(aMessage));
    }

    if (aMessage.GetLength() == startOffset)
    {
        error = aMessage.Append<uint8_t>(0);
    }

exit:
    return error;
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

otError PtrRecord::ReadPtrName(const Message &aMessage,
                               uint16_t &     aOffset,
                               char *         aLabelBuffer,
                               uint8_t        aLabelBufferSize,
                               char *         aNameBuffer,
                               uint16_t       aNameBufferSize) const
{
    otError  error       = OT_ERROR_NONE;
    uint16_t startOffset = aOffset - sizeof(PtrRecord); // start of `PtrRecord`.

    // Verify that the name is within the record data length.
    SuccessOrExit(error = Name::ParseName(aMessage, aOffset));
    VerifyOrExit(aOffset <= startOffset + GetSize(), error = OT_ERROR_PARSE);

    aOffset = startOffset + sizeof(PtrRecord);
    SuccessOrExit(error = Name::ReadLabel(aMessage, aOffset, aLabelBuffer, aLabelBufferSize));

    if (aNameBuffer != nullptr)
    {
        SuccessOrExit(error = Name::ReadName(aMessage, aOffset, aNameBuffer, aNameBufferSize));
    }

    aOffset = startOffset;
    error   = SkipRecord(aMessage, aOffset);

exit:
    return error;
}

otError TxtRecord::ReadTxtData(const Message &aMessage,
                               uint16_t &     aOffset,
                               uint8_t *      aTxtBuffer,
                               uint16_t &     aTxtBufferSize) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(GetLength() <= aTxtBufferSize, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = aMessage.Read(aOffset, aTxtBuffer, GetLength()));
    VerifyOrExit(VerifyTxtData(aTxtBuffer, GetLength()), error = OT_ERROR_PARSE);
    aTxtBufferSize = GetLength();
    aOffset += GetLength();

exit:
    return error;
}

bool TxtRecord::VerifyTxtData(const uint8_t *aTxtData, uint16_t aTxtLength)
{
    bool    valid          = false;
    uint8_t curEntryLength = 0;

    // Per RFC 1035, TXT-DATA MUST have one or more <character-string>s.
    VerifyOrExit(aTxtLength > 0);

    for (uint16_t i = 0; i < aTxtLength; ++i)
    {
        if (curEntryLength == 0)
        {
            curEntryLength = aTxtData[i];
        }
        else
        {
            --curEntryLength;
        }
    }

    valid = (curEntryLength == 0);

exit:
    return valid;
}

otError TxtRecord::GetNextTxtEntry(const uint8_t *aTxtData,
                                   uint16_t       aTxtLength,
                                   TxtIterator &  aIterator,
                                   TxtEntry &     aTxtEntry)
{
    otError error = OT_ERROR_NONE;

    for (uint16_t i = aIterator; i < aTxtLength;)
    {
        uint8_t length = aTxtData[i++];

        OT_ASSERT(i + length <= aTxtLength);
        aTxtEntry.mKey         = reinterpret_cast<const char *>(aTxtData + i);
        aTxtEntry.mKeyLength   = length;
        aTxtEntry.mValue       = nullptr;
        aTxtEntry.mValueLength = 0;

        for (uint8_t j = 0; j < length; ++j)
        {
            if (aTxtData[i + j] == TxtEntry::kKeyValueSeparator)
            {
                aTxtEntry.mKeyLength   = j;
                aTxtEntry.mValue       = aTxtData + i + j + 1;
                aTxtEntry.mValueLength = length - j - 1;
                break;
            }
        }

        i += length;

        // Per RFC 6763, a TXT entry with empty key MUST be silently ignored.
        if (aTxtEntry.mKeyLength == 0)
        {
            continue;
        }

        aIterator = i;
        ExitNow();
    }

    error = OT_ERROR_NOT_FOUND;

exit:
    return error;
}

} // namespace Dns
} // namespace ot
