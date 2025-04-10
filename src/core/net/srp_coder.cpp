/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "srp_coder.hpp"

#if OPENTHREAD_CONFIG_SRP_CODER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Srp {

const char Coder::kDefaultDomainName[] = "default.service.arpa";

const char *const Coder::kCommonlyUsedLabels[] = {
    "_udp",     // 0
    "_tcp",     // 1
    "_matter",  // 2
    "_matterc", // 3
    "_matterd", // 4
    "_hap",     // 5
};

RegisterLogModule("SrpCoder");

Coder::Coder(Instance &aInstance)
    : InstanceLocator(aInstance)
#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE
    , mDecodeCallback(nullptr)
#endif
{
}

bool Coder::IsEncoded(const Message &aMessage) const
{
    bool   isEncoded = false;
    Header header;

    SuccessOrExit(aMessage.Read(/* aOffset */ 0, header));
    VerifyOrExit(header.IsValid());
    isEncoded = true;

exit:
    return isEncoded;
}

bool Coder::IsEncoded(const uint8_t *aBuffer, uint16_t aLength) const
{
    bool isEncoded = false;

    VerifyOrExit(aLength > sizeof(Header));
    VerifyOrExit(reinterpret_cast<const Header *>(aBuffer)->IsValid());
    isEncoded = true;

exit:
    return isEncoded;
}

Error Coder::Decode(const Message &aCodedMsg, Message &aMessage)
{
    Error error = MsgDecoder(aCodedMsg, aMessage).Decode();

#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE
    if (mDecodeCallback != nullptr)
    {
        mDecodeCallback(aCodedMsg, aMessage, error);
    }
#endif

    return error;
}

Message *Coder::Decode(const uint8_t *aBuffer, uint16_t aLength, Error *aError)
{
    Error             error;
    OwnedPtr<Message> message;
    OwnedPtr<Message> codedMsg;

    VerifyOrExit(IsEncoded(aBuffer, aLength), error = kErrorInvalidArgs);

    codedMsg.Reset(Get<MessagePool>().Allocate(Message::kTypeOther));
    VerifyOrExit(codedMsg != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = codedMsg->AppendBytes(aBuffer, aLength));

    message.Reset(Get<MessagePool>().Allocate(Message::kTypeOther));
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    error = Decode(*codedMsg, *message);

exit:
    if (aError != nullptr)
    {
        *aError = error;
    }

    return (error == kErrorNone) ? message.Release() : nullptr;
}

Error Coder::AppendCompactUint(Message &aMessage, uint32_t aUint, uint8_t aFirstSegBitLength, uint8_t aFirstSegValue)
{
    // Encodes a given `aUint` value using the "Compact Integer"
    // format and appends the resulting bytes to `aMessage`.
    //
    // The number is encoded as one or more segments. Each segment is
    // a single byte (8 bits) long, except for the first segment
    // which can be shorter. The most significant bit (MSB) of each
    // segment acts as a "continuation bit" with `1` indicating more
    // segments follow, and `0` indicating this is the last segment.
    // The remaining bits(after the MSB) in each segment hold part of
    // the numerical value, arranged in big-endian order.
    //
    // The `aFirstSegBitLength` parameter specifies how many bits are
    // used in the first segment. If this is less than 8, then
    // `aFirstSegValue` provides the additional bits for the first
    // segment(the bits that are not part of the encoded first
    // segment itself).

    static constexpr uint8_t kMaxSegments = 6;

    uint8_t  segments[kMaxSegments];
    uint8_t *curSeg;

    curSeg = GetArrayEnd(segments);
    curSeg--;

    *curSeg = 0;

    while (true)
    {
        *curSeg |= static_cast<uint8_t>(aUint & kCompactUintValueMask);

        aUint >>= kCompactUintBitsPerByteSegement;

        if (aUint == 0)
        {
            break;
        }

        curSeg--;
        *curSeg = kCompactUintContinuationFlag;
    }

    if (aFirstSegBitLength != kBitsPerByte)
    {
        uint8_t contiunationFlag = static_cast<uint8_t>((1U << (aFirstSegBitLength - 1)));
        uint8_t valueMask        = (contiunationFlag - 1);
        uint8_t segValue         = (*curSeg & kCompactUintValueMask);

        if ((segValue & valueMask) == segValue)
        {
            // The segment can be shortened to fit in `aFirstSegBitLength`

            uint8_t firstSeg = aFirstSegValue;

            firstSeg |= segValue;

            if (*curSeg & kCompactUintContinuationFlag)
            {
                firstSeg |= contiunationFlag;
            }

            *curSeg = firstSeg;
        }
        else
        {
            curSeg--;
            *curSeg = (aFirstSegValue | contiunationFlag);
        }
    }

    return aMessage.AppendBytes(curSeg, static_cast<uint16_t>(GetArrayEnd(segments) - curSeg));
}

Error Coder::ReadCompactUint(const Message &aMessage, OffsetRange &aOffsetRange, uint32_t &aUint)
{
    return ReadCompactUint(aMessage, aOffsetRange, aUint, kBitsPerByte);
}

Error Coder::ReadCompactUint(const Message &aMessage,
                             OffsetRange   &aOffsetRange,
                             uint32_t      &aUint,
                             uint8_t        aFirstSegBitLength)
{
    enum SegmentType : uint8_t
    {
        kFirstSegement,
        kMiddleSegement,
        kLastSegement,
    };

    static constexpr uint32_t kMaxValueBeforeShift = (NumericLimits<uint32_t>::kMax >> kCompactUintBitsPerByteSegement);

    Error    error      = kErrorNone;
    uint32_t value      = 0;
    bool     isFirstSeg = true;

    uint8_t segment;
    uint8_t contiunationFlag;
    uint8_t valueMask;

    do
    {
        SuccessOrExit(error = aMessage.Read(aOffsetRange, segment));
        aOffsetRange.AdvanceOffset(sizeof(segment));

        if (isFirstSeg)
        {
            contiunationFlag = static_cast<uint8_t>(1U << (aFirstSegBitLength - 1));
            valueMask        = contiunationFlag - 1;
            isFirstSeg       = false;
        }
        else
        {
            contiunationFlag = kCompactUintContinuationFlag;
            valueMask        = kCompactUintValueMask;
        }

        VerifyOrExit(value <= kMaxValueBeforeShift, error = kErrorParse);

        value <<= kCompactUintBitsPerByteSegement;
        value += (segment & valueMask);

    } while (segment & contiunationFlag);

    aUint = value;

exit:
    return error;
}

Error Coder::AppendName(Message &aMessage, const char *aName, const OffsetRangeArray &aPrevLabelOffsetRanges)
{
    Error       error = kErrorNone;
    LabelBuffer label;
    uint16_t    nameLength = StringLength(aName, Dns::Name::kMaxNameSize);
    const char *nameEnd;
    const char *nameCur;

    VerifyOrExit(nameLength < Dns::Name::kMaxNameSize, error = kErrorInvalidArgs);

    nameCur = &aName[0];
    nameEnd = &aName[nameLength];

    while (nameCur < nameEnd)
    {
        const char *labelEnd;
        uint16_t    labelLength;

        labelEnd = StringFind(nameCur, Dns::Name::kLabelSeparatorChar);
        labelEnd = (labelEnd != nullptr) ? labelEnd : nameEnd;

        labelLength = static_cast<uint16_t>(labelEnd - nameCur);
        VerifyOrExit(labelLength <= Dns::Name::kMaxLabelLength, error = kErrorInvalidArgs);

        memcpy(label, nameCur, labelLength);
        label[labelLength] = kNullChar;

        nameCur += labelLength;

        if (*nameCur == Dns::Name::kLabelSeparatorChar)
        {
            nameCur++;
        }

        if (labelLength == 0)
        {
            VerifyOrExit(*nameCur == kNullChar, error = kErrorInvalidArgs);
            break;
        }

        SuccessOrExit(error = AppendLabel(aMessage, label, aPrevLabelOffsetRanges));
    }

    error = aMessage.Append<uint8_t>(0);

exit:
    return error;
}

Error Coder::AppendLabel(Message &aMessage, const char *aLabel, const OffsetRangeArray &aPrevLabelOffsetRanges)
{
    Error               error = kErrorNone;
    uint8_t             length;
    uint8_t             code;
    LabelGenerativeInfo genInfo;
    uint16_t            referOffset;

    if (CanEncodeAsCommonlyUsedLabel(aLabel, code))
    {
        error = aMessage.Append<uint8_t>(kLabelDispatchCommonlyUsed | code);
        ExitNow();
    }

    // Check if we can refer to a previously encoded label.

    for (OffsetRange offsetRange : aPrevLabelOffsetRanges)
    {
        while (!offsetRange.IsEmpty())
        {
            LabelBuffer prevLabel;

            referOffset = offsetRange.GetOffset();

            if (ReadLabel(aMessage, offsetRange, prevLabel) != kErrorNone)
            {
                break;
            }

            if (StringMatch(aLabel, prevLabel))
            {
                error = AppendCompactUint(aMessage, referOffset, kLabelDispatchOffsetFirstSegBitLength,
                                          kLabelDispatchReferOffset);
                ExitNow();
            }
        }
    }

    if (CanEncodeAsGenerativeLabel(aLabel, genInfo))
    {
        referOffset = 0;

        if (genInfo.mCode == kLabelGenerativeCharHexString)
        {
            // Check if we can find the same hex bytes earlier in the
            // message and use refer offset.

            for (OffsetRange offsetRange : aPrevLabelOffsetRanges)
            {
                for (; offsetRange.Contains(kHexValueSize); offsetRange.AdvanceOffset(sizeof(uint8_t)))
                {
                    if (aMessage.Compare(offsetRange.GetOffset(), genInfo.mFirstHexValue))
                    {
                        referOffset   = offsetRange.GetOffset();
                        genInfo.mCode = kLabelGenerativeCharHexStringOffset;
                        break;
                    }
                }

                if (genInfo.mCode == kLabelGenerativeCharHexStringOffset)
                {
                    break;
                }
            }
        }

        SuccessOrExit(error = aMessage.Append<uint8_t>(kLabelDispatchCommonlyUsed | kLabelDispatchGenerativeFlag |
                                                       genInfo.mCode));

        if ((genInfo.mCode == kLabelGenerativeCharHexString) || (genInfo.mCode == kLabelGenerativeCharHexStringOffset))
        {
            SuccessOrExit(error = aMessage.Append<uint8_t>(static_cast<uint8_t>(genInfo.mChar)));

            if (genInfo.mCode == kLabelGenerativeCharHexStringOffset)
            {
                error = AppendCompactUint(aMessage, referOffset);
                ExitNow();
            }
        }

        SuccessOrExit(error = aMessage.Append(genInfo.mFirstHexValue));

        if (genInfo.mCode == kLabelGenerativeTwoHexStrings)
        {
            SuccessOrExit(error = aMessage.Append(genInfo.mSecondHexValue));
        }

        ExitNow();
    }

    length = static_cast<uint8_t>(StringLength(aLabel, Dns::Name::kMaxLabelSize));
    VerifyOrExit(length <= Dns::Name::kMaxLabelLength, error = kErrorInvalidArgs);

    if (*aLabel == '_')
    {
        length--;
        aLabel++;
        code = kLabelDispatchService;
    }
    else
    {
        code = kLabelDispatchNormal;
    }

    SuccessOrExit(error = aMessage.Append<uint8_t>(code | length));
    SuccessOrExit(error = aMessage.AppendBytes(aLabel, length));

exit:
    return error;
}

Error Coder::ReadName(const Message &aMessage, OffsetRange &aOffsetRange, NameBuffer &aName)
{
    Error        error = kErrorNone;
    LabelBuffer  label;
    StringWriter writer(aName, sizeof(aName));

    while (true)
    {
        SuccessOrExit(error = ReadLabel(aMessage, aOffsetRange, label));

        if (label[0] == kNullChar)
        {
            break;
        }

        writer.Append("%s.", label);
    }

    if (aName[0] == kNullChar)
    {
        writer.Append(".");
    }

exit:
    return error;
}

Error Coder::ReadLabel(const Message &aMessage, OffsetRange &aOffsetRange, LabelBuffer &aLabel)
{
    Error       error = kErrorNone;
    char       *label = &aLabel[0];
    uint8_t     dispatch;
    uint8_t     length;
    OffsetRange referOffsetRange;

    SuccessOrExit(error = aMessage.Read(aOffsetRange, dispatch));

    switch (dispatch & kLabelDispatchTypeMask)
    {
    case kLabelDispatchService:
        *label++ = '_';
        OT_FALL_THROUGH;

    case kLabelDispatchNormal:
        length = (dispatch & kLabelDispatchLengthMask);
        VerifyOrExit(label + length < GetArrayEnd(aLabel), error = kErrorParse);
        aOffsetRange.AdvanceOffset(sizeof(dispatch));
        SuccessOrExit(error = aMessage.Read(aOffsetRange, label, length));
        aOffsetRange.AdvanceOffset(length);
        label[length] = kNullChar;
        break;

    case kLabelDispatchReferOffset:
        SuccessOrExit(
            error = ReadReferOffset(aMessage, aOffsetRange, referOffsetRange, kLabelDispatchOffsetFirstSegBitLength));

        error = ReadLabel(aMessage, referOffsetRange, aLabel);
        break;

    case kLabelDispatchCommonlyUsed:
    {
        uint8_t      code = (dispatch & kLabelDispatchCodeMask);
        StringWriter labelWriter(aLabel, sizeof(aLabel));

        aOffsetRange.AdvanceOffset(sizeof(dispatch));

        if (dispatch & kLabelDispatchGenerativeFlag)
        {
            char prefixChar;

            switch (code)
            {
            case kLabelGenerativeHexString:
                break;

            case kLabelGenerativeTwoHexStrings:
                SuccessOrExit(error = ReadAndAppendHexValueToLabel(aMessage, aOffsetRange, labelWriter));
                labelWriter.Append("-");
                break;

            case kLabelGenerativeCharHexString:
            case kLabelGenerativeCharHexStringOffset:
                SuccessOrExit(error = aMessage.Read(aOffsetRange, prefixChar));
                aOffsetRange.AdvanceOffset(sizeof(prefixChar));
                labelWriter.Append("_%c", prefixChar);
                break;

            default:
                ExitNow(error = kErrorParse);
            }

            if (code == kLabelGenerativeCharHexStringOffset)
            {
                SuccessOrExit(ReadReferOffset(aMessage, aOffsetRange, referOffsetRange));
                error = ReadAndAppendHexValueToLabel(aMessage, referOffsetRange, labelWriter);
                ExitNow();
            }

            // In all other cases, we need to read a `HexValue` from
            // message and add to the end of label.

            error = ReadAndAppendHexValueToLabel(aMessage, aOffsetRange, labelWriter);
        }
        else
        {
            VerifyOrExit(code < GetArrayLength(kCommonlyUsedLabels), error = kErrorParse);
            SuccessOrExit(error = StringCopy(aLabel, kCommonlyUsedLabels[code]));
        }

        break;
    }

    default:
        error = kErrorParse;
        break;
    }

exit:
    return error;
}

Error Coder::ReadAndAppendHexValueToLabel(const Message &aMessage,
                                          OffsetRange   &aOffsetRange,
                                          StringWriter  &aLabelWriter)
{
    // Reads 64-bit hex raw byte value from the `aMessage` at given
    // `aOffsetRange`. Appends the hexadecimal string representation
    // of the read value to the given label string. Updates
    // `aOffsetRange` to skip over the read bytes.

    Error    error;
    HexValue hexValue;

    SuccessOrExit(error = aMessage.Read(aOffsetRange, hexValue));
    aOffsetRange.AdvanceOffset(sizeof(hexValue));

    aLabelWriter.AppendHexBytesUppercase(hexValue, sizeof(hexValue));

exit:
    return error;
}

Error Coder::ReadReferOffset(const Message &aMessage,
                             OffsetRange   &aOffsetRange,
                             OffsetRange   &aReferOffsetRange,
                             uint8_t        aFirstSegBitLength)
{
    Error    error;
    uint16_t startOffset;
    uint32_t referOffset;

    startOffset = aOffsetRange.GetOffset();
    SuccessOrExit(error = ReadCompactUint(aMessage, aOffsetRange, referOffset, aFirstSegBitLength));
    VerifyOrExit(referOffset < startOffset, error = kErrorParse);

    aReferOffsetRange.InitFromRange(static_cast<uint16_t>(referOffset), aMessage.GetLength());

exit:
    return error;
}

bool Coder::CanEncodeAsCommonlyUsedLabel(const char *aLabel, uint8_t &aCode)
{
    bool canEncode = false;

    aCode = 0;

    for (const char *commonLable : kCommonlyUsedLabels)
    {
        if (StringMatch(aLabel, commonLable))
        {
            canEncode = true;
            break;
        }

        aCode++;
    }

    return canEncode;
}

bool Coder::CanEncodeAsGenerativeLabel(const char *aLabel, LabelGenerativeInfo &aInfo)
{
    bool        canEncode = false;
    const char *cur       = aLabel;

    if (*cur == '_')
    {
        cur++;
        VerifyOrExit(*cur != kNullChar);
        VerifyOrExit(!IsDigit(*cur));
        aInfo.mChar = *cur;
        cur++;

        SuccessOrExit(ReadHexValue(cur, aInfo.mFirstHexValue));
        VerifyOrExit(*cur == kNullChar);
        aInfo.mCode = kLabelGenerativeCharHexString;
        canEncode   = true;
        ExitNow();
    }

    SuccessOrExit(ReadHexValue(cur, aInfo.mFirstHexValue));

    if (*cur == kNullChar)
    {
        aInfo.mCode = kLabelGenerativeHexString;
        canEncode   = true;
        ExitNow();
    }

    VerifyOrExit(*cur == '-');
    cur++;

    SuccessOrExit(ReadHexValue(cur, aInfo.mSecondHexValue));
    VerifyOrExit(*cur == kNullChar);

    aInfo.mCode = kLabelGenerativeTwoHexStrings;
    canEncode   = true;

exit:
    return canEncode;
}

Error Coder::ReadHexValue(const char *&aLabel, HexValue &aHexValue)
{
    Error error = kErrorParse;

    for (uint8_t &byte : aHexValue)
    {
        uint8_t digit;

        SuccessOrExit(error = ReadHexDigit(aLabel, digit));
        byte = static_cast<uint8_t>(digit << 4);

        SuccessOrExit(error = ReadHexDigit(aLabel, digit));
        byte += digit;
    }

exit:
    return error;
}

Error Coder::ReadHexDigit(const char *&aLabel, uint8_t &aDigit)
{
    Error error = kErrorParse;

    VerifyOrExit(!IsLowercase(*aLabel));
    SuccessOrExit(error = ParseHexDigit(*aLabel, aDigit));
    aLabel++;

exit:
    return error;
}

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Coder::MsgEncoder

Coder::MsgEncoder::MsgEncoder(void)
    : mCurrLabelsOffsetRange(nullptr)
{
}

Error Coder::MsgEncoder::AllocateMessage(Ip6::Udp::Socket &aSocket)
{
    mMessage.Reset(aSocket.NewMessage());

    return (mMessage != nullptr) ? kErrorNone : kErrorNoBufs;
}

void Coder::MsgEncoder::Init(void)
{
    VerifyOrExit(HasMessage());
    IgnoreError(mMessage->SetLength(0));

    mSavedLabelOffsetRanges.Clear();
    mSavedTxtDataOffsets.Clear();

exit:
    return;
}

Error Coder::MsgEncoder::EncodeHeaderBlock(uint16_t    aMessageId,
                                           const char *aDomainName,
                                           uint32_t    aDefaultTtl,
                                           const char *aHostName)
{
    Error   error = kErrorNone;
    uint8_t dispatch;
    Header  header;

    VerifyOrExit(HasMessage());

    dispatch = kHeaderDispatchCode;
    dispatch |= (!Dns::Name::IsSameDomain(aDomainName, kDefaultDomainName) ? kHeaderDispatchZoneFlag : 0);
    dispatch |= ((aDefaultTtl != kDefaultTtl) ? kHeaderDispatchTtlFlag : 0);

    header.SetMessageId(aMessageId);
    header.SetDispatch(dispatch);

    SuccessOrExit(error = mMessage->Append(header));

    if (dispatch & kHeaderDispatchZoneFlag)
    {
        SuccessOrExit(error = EncodeName(aDomainName));
    }

    if (dispatch & kHeaderDispatchTtlFlag)
    {
        SuccessOrExit(error = EncodeCompactUint(aDefaultTtl));
    }

    SaveLabelsOffsetRange();
    SuccessOrExit(error = EncodeName(aHostName));

exit:
    return error;
}

Error Coder::MsgEncoder::EncodeServiceBlock(const ClientService &aService, bool aRemoving)
{
    Error                  error   = kErrorNone;
    const Client::Service &service = AsCoreType(&aService);
    uint8_t                dispatch;
    bool                   hasTxtData = false;

    VerifyOrExit(HasMessage());

    dispatch = aRemoving ? kDispatchRemoveServiceType : kDispatchAddServiceType;

    if (!aRemoving)
    {
        hasTxtData = (service.GetNumTxtEntries() != 0 && service.GetTxtEntries() != nullptr);

        dispatch |= (service.HasSubType() ? kAddServiceDispatchSubTypeFlag : 0);
        dispatch |= ((service.GetPriority() != 0) ? kAddServiceDispatchPriorityFlag : 0);
        dispatch |= ((service.GetWeight() != 0) ? kAddServiceDispatchWeightFlag : 0);
        dispatch |= (hasTxtData ? kAddServiceDispatchTxtDataFlag : 0);
    }

    SuccessOrExit(error = mMessage->Append<uint8_t>(dispatch));

    SaveLabelsOffsetRange();
    SuccessOrExit(error = EncodeLabel(service.GetInstanceName()));
    SuccessOrExit(error = EncodeName(service.GetName()));

    if (!aRemoving && service.HasSubType())
    {
        for (uint16_t index = 0; true; index++)
        {
            const char *subTypeLabel = service.GetSubTypeLabelAt(index);

            if (subTypeLabel == nullptr)
            {
                break;
            }

            SuccessOrExit(error = EncodeLabel(subTypeLabel));
        }

        SuccessOrExit(error = mMessage->Append<uint8_t>(0));
    }

    VerifyOrExit(!aRemoving);

    SuccessOrExit(error = EncodeCompactUint(service.GetPort()));

    if (service.GetPriority() != 0)
    {
        SuccessOrExit(error = EncodeCompactUint(service.GetPriority()));
    }

    if (service.GetWeight() != 0)
    {
        SuccessOrExit(error = EncodeCompactUint(service.GetWeight()));
    }

    if (hasTxtData)
    {
        OwnedPtr<Message> txtData(mMessage->Get<MessagePool>().Allocate(Message::kTypeOther));
        uint16_t          txtDataLength;
        uint16_t          referOffset = 0;

        VerifyOrExit(txtData != nullptr, error = kErrorNoBufs);

        SuccessOrExit(error =
                          Dns::TxtEntry::AppendEntries(service.GetTxtEntries(), service.GetNumTxtEntries(), *txtData));
        txtDataLength = txtData->GetLength();

        // Check if the same TXT data bytes were previously
        // encoded in the message.

        for (uint16_t offset : mSavedTxtDataOffsets)
        {
            OffsetRange offsetRange;
            uint32_t    prevTxtDataLength;

            offsetRange.InitFromRange(offset, mMessage->GetLength());

            SuccessOrExit(error = ReadCompactUint(*mMessage, offsetRange, prevTxtDataLength,
                                                  kTxtDataDispatchSizeSegmentBitLength));

            if ((prevTxtDataLength == txtDataLength) &&
                mMessage->CompareBytes(offsetRange.GetOffset(), *txtData, 0, txtDataLength))
            {
                referOffset = offset;
                break;
            }
        }

        if (referOffset != 0)
        {
            SuccessOrExit(error = EncodeCompactUint(referOffset, kTxtDataDispatchSizeSegmentBitLength,
                                                    kTxtDataDispatchReferFlag));
        }
        else
        {
            IgnoreError(mSavedTxtDataOffsets.PushBack(mMessage->GetLength()));

            SuccessOrExit(error = EncodeCompactUint(txtDataLength, kTxtDataDispatchSizeSegmentBitLength, 0));
            SuccessOrExit(error = mMessage->AppendBytesFromMessage(*txtData, 0, txtDataLength));
        }
    }

exit:
    return error;
}

Error Coder::MsgEncoder::EncodeHostDispatch(bool aHasAnyAddress)
{
    Error   error = kErrorNone;
    uint8_t dispatch;

    VerifyOrExit(HasMessage());

    dispatch = kDispatchHostType | kHostDispatchKeyFlag;
    dispatch |= (aHasAnyAddress ? kHostDispatchAddrListFlag : 0);

    error = mMessage->Append(dispatch);

exit:
    return error;
}

Error Coder::MsgEncoder::EncodeHostAddress(const Ip6::Address &aAddress, bool aHasMore)
{
    Error           error = kErrorNone;
    uint8_t         dispatch;
    Lowpan::Context context;

    VerifyOrExit(HasMessage());

    dispatch = (aHasMore ? kAddrDispatchMoreFlag : 0);

    if ((mMessage->Get<NetworkData::Leader>().GetContext(aAddress, context) == kErrorNone) && context.mCompressFlag)
    {
        dispatch |= kAddrDispatchContextFlag;
        dispatch |= (context.mContextId & kAddrDispatchContextIdMask);
        SuccessOrExit(mMessage->Append(dispatch));
        SuccessOrExit(mMessage->Append(aAddress.GetIid()));
    }
    else
    {
        SuccessOrExit(mMessage->Append(dispatch));
        SuccessOrExit(mMessage->Append(aAddress));
    }

exit:
    return error;
}

Error Coder::MsgEncoder::EncodeHostKey(const Crypto::Ecdsa::P256::PublicKey &aKey)
{
    Error error = kErrorNone;

    VerifyOrExit(HasMessage());
    error = mMessage->Append(aKey);

exit:
    return error;
}

Error Coder::MsgEncoder::EncodeFooterBlock(uint32_t                              aLease,
                                           uint32_t                              aKeyLease,
                                           const Crypto::Ecdsa::P256::Signature &aSignature)
{
    Error   error = kErrorNone;
    uint8_t dispatch;

    VerifyOrExit(HasMessage());

    dispatch = kDispatchFooterType | kFooterDispatchSign64;
    dispatch |= ((aLease != kDefaultLease) ? kFooterDispatchLeaseFlag : 0);
    dispatch |= ((aKeyLease != kDefaultKeyLease) ? kFooterDispatchKeyLeaseFlag : 0);

    SuccessOrExit(error = mMessage->Append(dispatch));

    if (aLease != kDefaultLease)
    {
        SuccessOrExit(error = EncodeCompactUint(aLease));
    }

    if (aKeyLease != kDefaultKeyLease)
    {
        SuccessOrExit(error = EncodeCompactUint(aKeyLease));
    }

    SuccessOrExit(error = mMessage->Append(aSignature));

exit:
    return error;
}

Error Coder::MsgEncoder::SendMessage(Ip6::Udp::Socket &aSocket)
{
    Error error;

    VerifyOrExit(mMessage != nullptr, error = kErrorNotFound);
    SuccessOrExit(error = aSocket.SendTo(*mMessage, Ip6::MessageInfo()));

    // Ownership of the message is transferred to the socket upon a
    // successful `SendTo()` call.

    mMessage.Release();

exit:
    return error;
}

Error Coder::MsgEncoder::EncodeCompactUint(uint32_t aUint)
{
    return AppendCompactUint(*mMessage, aUint, kBitsPerByte, 0);
}

Error Coder::MsgEncoder::EncodeCompactUint(uint32_t aUint, uint8_t aFirstSegBitLength, uint8_t aFirstSegValue)
{
    return AppendCompactUint(*mMessage, aUint, aFirstSegBitLength, aFirstSegValue);
}

void Coder::MsgEncoder::SaveLabelsOffsetRange(void)
{
    // We track the offset ranges where labels/names are encoded in
    // the message. This allows us to identify duplicate labels and
    // use reference labels instead, optimizing the message size.
    //
    // This method allocates a new entry in `mSavedLabelOffsetRanges`
    // array and initializes it as an empty range, starting from the
    // current message offset. `EncodeName()` and `EncodeLabel()`
    // will later extend this range as labels are encoded and added
    // to the message.

    mCurrLabelsOffsetRange = mSavedLabelOffsetRanges.PushBack();

    if (mCurrLabelsOffsetRange != nullptr)
    {
        mCurrLabelsOffsetRange->Init(mMessage->GetLength(), 0);
    }
}

void Coder::MsgEncoder::ExtendedCurrLabelsOffsetRange(void)
{
    // Extend `mCurrLabelsOffsetRange` to the current position in
    // the message (end of the message).

    VerifyOrExit(mCurrLabelsOffsetRange != nullptr);
    mCurrLabelsOffsetRange->InitFromRange(mCurrLabelsOffsetRange->GetOffset(), mMessage->GetLength());

exit:
    return;
}

Error Coder::MsgEncoder::EncodeName(const char *aName)
{
    Error error;

    SuccessOrExit(error = AppendName(*mMessage, aName, mSavedLabelOffsetRanges));
    ExtendedCurrLabelsOffsetRange();

exit:
    return error;
}

Error Coder::MsgEncoder::EncodeLabel(const char *aLabel)
{
    Error error;

    SuccessOrExit(error = AppendLabel(*mMessage, aLabel, mSavedLabelOffsetRanges));
    ExtendedCurrLabelsOffsetRange();

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Coder::MsgDecoder

Coder::MsgDecoder::MsgDecoder(const Message &aCodedMsg, Message &aMessage)
    : mCodedMsg(aCodedMsg)
    , mMessage(aMessage)
{
}

Error Coder::MsgDecoder::Decode(void)
{
    Error   error = kErrorNone;
    uint8_t dispatch;

    IgnoreError(mMessage.SetLength(0));
    mOffsetRange.InitFromMessageFullLength(mCodedMsg);
    mDefaultTtl        = kDefaultTtl;
    mUpdateRecordCount = 0;
    mAddnlRecordCount  = 0;
    mDomainNameOffset  = kUnknownOffset;
    mHostNameOffset    = kUnknownOffset;

    SuccessOrExit(error = DecodeHeaderBlock());

    while (true)
    {
        uint8_t type;

        SuccessOrExit(error = mCodedMsg.Read(mOffsetRange, dispatch));
        mOffsetRange.AdvanceOffset(sizeof(dispatch));

        type = (dispatch & kDispatchTypeMask);

        if ((type == kDispatchAddServiceType) || (type == kDispatchRemoveServiceType))
        {
            SuccessOrExit(error = DecodeServiceBlock(dispatch));
        }
        else
        {
            break;
        }
    }

    SuccessOrExit(error = DecodeHostBlock(dispatch));
    SuccessOrExit(error = DecodeFooterBlock());

    UpdateHeaderRecordCounts();

exit:
    return error;
}

Error Coder::MsgDecoder::DecodeHeaderBlock(void)
{
    Error             error = kErrorNone;
    uint8_t           dispatch;
    Header            header;
    Dns::UpdateHeader updateHeader;
    const char       *domainName;
    NameBuffer        domainNameBuffer;

    // Read and decode header from coded message

    SuccessOrExit(error = mCodedMsg.Read(mOffsetRange, header));
    mOffsetRange.AdvanceOffset(sizeof(Header));

    VerifyOrExit(header.IsValid(), error = kErrorInvalidArgs);

    dispatch = header.GetDispatch();

    updateHeader.SetMessageId(header.GetMessageId());
    updateHeader.SetZoneRecordCount(1);
    SuccessOrExit(error = mMessage.Append(updateHeader));

    // Prepare Zone section

    mDomainNameOffset = mMessage.GetLength();

    if (dispatch & kHeaderDispatchZoneFlag)
    {
        SuccessOrExit(error = DecodeName(domainNameBuffer));
        domainName = &domainNameBuffer[0];
    }
    else
    {
        domainName = kDefaultDomainName;
    }

    SuccessOrExit(error = Dns::Name::AppendName(domainName, mMessage));
    SuccessOrExit(error = mMessage.Append(Dns::Zone()));

    // Read default TTL and host name from coded message

    if (dispatch & kHeaderDispatchTtlFlag)
    {
        SuccessOrExit(error = DecodeUint32(mDefaultTtl));
    }
    else
    {
        mDefaultTtl = kDefaultTtl;
    }

    SuccessOrExit(error = DecodeName(mHostName));

exit:
    return error;
}

Error Coder::MsgDecoder::DecodeServiceBlock(uint8_t aDispatch)
{
    // Dispatch type is already checked to be `Add` or `Remove`
    // service type.

    Error               error    = kErrorNone;
    bool                removing = ((aDispatch & kDispatchTypeMask) == kDispatchRemoveServiceType);
    uint32_t            ptrTtl   = mDefaultTtl;
    uint32_t            srvTtl   = mDefaultTtl;
    LabelBuffer         label;
    NameBuffer          serviceName;
    Dns::ResourceRecord rr;
    Dns::SrvRecord      srv;
    uint16_t            serviceNameOffset;
    uint16_t            instanceNameOffset;
    uint16_t            rrOffset;
    uint16_t            u16;

    //------------------------------------------------------------------
    // Decode the TTL fields if not elided

    if (!removing)
    {
        if (aDispatch & kAddServiceDispatchPtrTtlFlag)
        {
            SuccessOrExit(error = DecodeUint32(ptrTtl));
        }

        if (aDispatch & kAddServiceDispatchSrvTxtTtlFlag)
        {
            SuccessOrExit(error = DecodeUint32(srvTtl));
        }
    }

    //------------------------------------------------------------------
    // Decode service instance label and service name

    SuccessOrExit(error = DecodeLabel(label));
    SuccessOrExit(error = DecodeName(serviceName));

    //------------------------------------------------------------------
    // Append PTR record

    // "service name labels" + (pointer to) domain name.
    serviceNameOffset = mMessage.GetLength();
    SuccessOrExit(error = Dns::Name::AppendMultipleLabels(serviceName, mMessage));
    SuccessOrExit(error = Dns::Name::AppendPointerLabel(mDomainNameOffset, mMessage));

    // On remove, we use "Delete an RR from an RRSet" where class is set
    // to NONE and TTL to zero (RFC 2136 - section 2.5.4).

    rr.Init(Dns::ResourceRecord::kTypePtr, removing ? Dns::PtrRecord::kClassNone : Dns::PtrRecord::kClassInternet);
    rr.SetTtl(removing ? 0 : ptrTtl);
    rrOffset = mMessage.GetLength();
    SuccessOrExit(error = mMessage.Append(rr));

    // "Instance label" + (pointer to) service name.
    instanceNameOffset = mMessage.GetLength();
    SuccessOrExit(error = Dns::Name::AppendLabel(label, mMessage));
    SuccessOrExit(error = Dns::Name::AppendPointerLabel(serviceNameOffset, mMessage));

    UpdateRecordLengthInMessage(rr, rrOffset);
    mUpdateRecordCount++;

    //------------------------------------------------------------------
    // Decode sub-type labels and append sub-type PTR records

    if (!removing && (aDispatch & kAddServiceDispatchSubTypeFlag))
    {
        uint16_t subServiceNameOffset = kUnknownOffset;

        while (true)
        {
            SuccessOrExit(error = DecodeLabel(label));

            if (label[0] == kNullChar)
            {
                break;
            }

            // subtype label + "_sub" label + (pointer to) service name.

            SuccessOrExit(error = Dns::Name::AppendLabel(label, mMessage));

            if (subServiceNameOffset == kUnknownOffset)
            {
                subServiceNameOffset = mMessage.GetLength();
                SuccessOrExit(error = Dns::Name::AppendLabel("_sub", mMessage));
                SuccessOrExit(error = Dns::Name::AppendPointerLabel(serviceNameOffset, mMessage));
            }
            else
            {
                SuccessOrExit(error = Dns::Name::AppendPointerLabel(subServiceNameOffset, mMessage));
            }

            // `rr` is already initialized as PTR.
            rrOffset = mMessage.GetLength();
            SuccessOrExit(error = mMessage.Append(rr));

            SuccessOrExit(error = Dns::Name::AppendPointerLabel(instanceNameOffset, mMessage));
            UpdateRecordLengthInMessage(rr, rrOffset);
            mUpdateRecordCount++;
        }
    }

    //------------------------------------------------------------------
    // Append "Delete all RRsets from a name" for Instance Name.
    // (Service Description Instruction)

    SuccessOrExit(error = Dns::Name::AppendPointerLabel(instanceNameOffset, mMessage));
    SuccessOrExit(error = AppendDeleteAllRrsets());
    mUpdateRecordCount++;

    VerifyOrExit(!removing);

    //------------------------------------------------------------------
    // Decode SRV info and append it.

    SuccessOrExit(error = Dns::Name::AppendPointerLabel(instanceNameOffset, mMessage));

    srv.Init();
    srv.SetTtl(srvTtl);

    SuccessOrExit(error = DecodeUint16(u16));
    srv.SetPort(u16);

    if (aDispatch & kAddServiceDispatchPriorityFlag)
    {
        SuccessOrExit(error = DecodeUint16(u16));
        srv.SetPriority(u16);
    }
    else
    {
        srv.SetPriority(0);
    }

    if (aDispatch & kAddServiceDispatchWeightFlag)
    {
        SuccessOrExit(error = DecodeUint16(u16));
        srv.SetWeight(u16);
    }
    else
    {
        srv.SetWeight(0);
    }

    rrOffset = mMessage.GetLength();
    SuccessOrExit(error = mMessage.Append(srv));
    SuccessOrExit(error = AppendHostName());
    UpdateRecordLengthInMessage(srv, rrOffset);
    mUpdateRecordCount++;

    //------------------------------------------------------------------
    // Decode TXT data info and append it.

    SuccessOrExit(error = Dns::Name::AppendPointerLabel(instanceNameOffset, mMessage));
    rr.Init(Dns::ResourceRecord::kTypeTxt);
    rrOffset = mMessage.GetLength();
    SuccessOrExit(error = mMessage.Append(rr));

    if (aDispatch & kAddServiceDispatchTxtDataFlag)
    {
        uint8_t     txtDataDispatch;
        OffsetRange txtDataOffsetRange;

        SuccessOrExit(error = mCodedMsg.Read(mOffsetRange, txtDataDispatch));
        SuccessOrExit(error = DecodeUint16(u16, kTxtDataDispatchSizeSegmentBitLength));

        if (txtDataDispatch & kTxtDataDispatchReferFlag)
        {
            uint32_t txtDataLen;

            txtDataOffsetRange.InitFromRange(u16, mCodedMsg.GetLength());
            SuccessOrExit(error = ReadCompactUint(mCodedMsg, txtDataOffsetRange, txtDataLen,
                                                  kTxtDataDispatchSizeSegmentBitLength));
            VerifyOrExit(txtDataLen <= mCodedMsg.GetLength(), error = kErrorParse);
            txtDataOffsetRange.ShrinkLength(static_cast<uint16_t>(txtDataLen));
        }
        else
        {
            txtDataOffsetRange = mOffsetRange;
            txtDataOffsetRange.ShrinkLength(u16);

            mOffsetRange.AdvanceOffset(u16);
        }

        SuccessOrExit(error = mMessage.AppendBytesFromMessage(mCodedMsg, txtDataOffsetRange));
    }
    else
    {
        SuccessOrExit(error = mMessage.Append<uint8_t>(0));
    }

    UpdateRecordLengthInMessage(rr, rrOffset);
    mUpdateRecordCount++;

exit:
    return error;
}

Error Coder::MsgDecoder::DecodeHostBlock(uint8_t aDispatch)
{
    Error    error   = kErrorNone;
    uint32_t addrTtl = mDefaultTtl;
    uint32_t keyTtl  = mDefaultTtl;

    VerifyOrExit((aDispatch & kDispatchTypeMask) == kDispatchHostType, error = kErrorParse);

    // Host Description Instruction

    // "Delete all RRsets from a name" for Host Name.

    SuccessOrExit(error = AppendHostName());
    SuccessOrExit(error = AppendDeleteAllRrsets());
    mUpdateRecordCount++;

    if (aDispatch & kHostDispatchAddrTtlFlag)
    {
        SuccessOrExit(error = DecodeUint32(addrTtl));
    }

    if (aDispatch & kHostDispatchAddrListFlag)
    {
        uint8_t             addrDisptach;
        Ip6::Address        ip6Address;
        Dns::ResourceRecord rr;

        do
        {
            SuccessOrExit(error = mCodedMsg.Read(mOffsetRange, addrDisptach));
            mOffsetRange.AdvanceOffset(sizeof(addrDisptach));

            if (addrDisptach & kAddrDispatchContextFlag)
            {
                uint8_t         contextId = (addrDisptach & kAddrDispatchContextIdMask);
                Lowpan::Context context;

                SuccessOrExit(error = mCodedMsg.Read(mOffsetRange, ip6Address.GetIid()));
                mOffsetRange.AdvanceOffset(sizeof(Ip6::InterfaceIdentifier));

                if (mMessage.Get<NetworkData::Leader>().GetContext(contextId, context) != kErrorNone)
                {
                    LogWarn("Failed to get lowpan context for %u", contextId);
                    ExitNow(error = kErrorParse);
                }

                ip6Address.SetPrefix(context.mPrefix);
            }
            else
            {
                SuccessOrExit(error = mCodedMsg.Read(mOffsetRange, ip6Address));
                mOffsetRange.AdvanceOffset(sizeof(Ip6::Address));
            }

            // Append AAAA record

            rr.Init(Dns::ResourceRecord::kTypeAaaa);
            rr.SetTtl(addrTtl);
            rr.SetLength(sizeof(Ip6::Address));

            SuccessOrExit(error = AppendHostName());
            SuccessOrExit(error = mMessage.Append(rr));
            SuccessOrExit(error = mMessage.Append(ip6Address));
            mUpdateRecordCount++;

        } while (addrDisptach & kAddrDispatchMoreFlag);
    }

    // Decode key related info and append KEY record

    if (aDispatch & kHostDispatchKeyTtlFlag)
    {
        SuccessOrExit(error = DecodeUint32(keyTtl));
    }

    if (aDispatch & kHostDispatchKeyFlag)
    {
        OffsetRange    keyOffsetRange = mOffsetRange;
        Dns::KeyRecord key;

        VerifyOrExit(keyOffsetRange.Contains(kEcdsaKeySize), error = kErrorParse);
        keyOffsetRange.ShrinkLength(kEcdsaKeySize);
        mOffsetRange.AdvanceOffset(kEcdsaKeySize);

        key.Init();
        key.SetTtl(keyTtl);
        key.SetFlags(Dns::KeyRecord::kAuthConfidPermitted, Dns::KeyRecord::kOwnerNonZone,
                     Dns::KeyRecord::kSignatoryFlagGeneral);
        key.SetProtocol(Dns::KeyRecord::kProtocolDnsSec);
        key.SetAlgorithm(Dns::KeyRecord::kAlgorithmEcdsaP256Sha256);
        key.SetLength(sizeof(Dns::KeyRecord) - sizeof(Dns::ResourceRecord) + kEcdsaKeySize);

        SuccessOrExit(error = AppendHostName());
        SuccessOrExit(error = mMessage.Append(key));
        SuccessOrExit(error = mMessage.AppendBytesFromMessage(mCodedMsg, keyOffsetRange));
        mUpdateRecordCount++;
    }

exit:
    return error;
}

Error Coder::MsgDecoder::DecodeFooterBlock(void)
{
    Error            error = kErrorNone;
    uint8_t          dispatch;
    uint32_t         lease    = kDefaultLease;
    uint32_t         keyLease = kDefaultKeyLease;
    uint16_t         optionSize;
    uint16_t         rrOffset;
    Dns::OptRecord   optRecord;
    Dns::LeaseOption leaseOption;
    Dns::SigRecord   sig;
    OffsetRange      sigOffsetRange;

    SuccessOrExit(error = mCodedMsg.Read(mOffsetRange, dispatch));
    mOffsetRange.AdvanceOffset(sizeof(dispatch));

    VerifyOrExit((dispatch & kDispatchTypeMask) == kDispatchFooterType, error = kErrorParse);

    if (dispatch & kFooterDispatchLeaseFlag)
    {
        SuccessOrExit(error = DecodeUint32(lease));
    }

    if (dispatch & kFooterDispatchKeyLeaseFlag)
    {
        SuccessOrExit(error = DecodeUint32(keyLease));
    }

    // Append OPT record with Lease Option

    // Empty (root domain) as OPT RR name
    SuccessOrExit(error = Dns::Name::AppendTerminator(mMessage));

    leaseOption.InitAsLongVariant(lease, keyLease);
    optionSize = static_cast<uint16_t>(leaseOption.GetSize());

    optRecord.Init();
    optRecord.SetUdpPayloadSize(kUdpPayloadSize);
    optRecord.SetDnsSecurityFlag();
    optRecord.SetLength(optionSize);

    SuccessOrExit(error = mMessage.Append(optRecord));
    SuccessOrExit(error = mMessage.AppendBytes(&leaseOption, optionSize));
    mAddnlRecordCount++;

    // Decode and append signature record

    switch (dispatch & kFooterDispatchSignMask)
    {
    case kFooterDispatchSign64:
        sigOffsetRange = mOffsetRange;
        VerifyOrExit(sigOffsetRange.Contains(kEcdsaSignatureSize), error = kErrorParse);
        sigOffsetRange.ShrinkLength(kEcdsaSignatureSize);
        mOffsetRange.AdvanceOffset(kEcdsaSignatureSize);
        break;

    case kFooterDispatchSignShort:
    default:
        error = kErrorParse;
        OT_FALL_THROUGH;

    case kFooterDispatchSignElided:
        ExitNow();
    }

    sig.Clear();
    sig.Init(Dns::ResourceRecord::kClassAny);
    sig.SetAlgorithm(Dns::KeyRecord::kAlgorithmEcdsaP256Sha256);

    // SIG(0) uses owner name of root (single zero byte).
    SuccessOrExit(error = Dns::Name::AppendTerminator(mMessage));

    rrOffset = mMessage.GetLength();
    SuccessOrExit(error = mMessage.Append(sig));
    SuccessOrExit(error = AppendHostName());
    SuccessOrExit(error = mMessage.AppendBytesFromMessage(mCodedMsg, sigOffsetRange));
    UpdateRecordLengthInMessage(sig, rrOffset);
    mAddnlRecordCount++;

exit:
    return error;
}

Error Coder::MsgDecoder::DecodeLabel(LabelBuffer &aLabel) { return ReadLabel(mCodedMsg, mOffsetRange, aLabel); }

Error Coder::MsgDecoder::DecodeName(NameBuffer &aName) { return ReadName(mCodedMsg, mOffsetRange, aName); }

Error Coder::MsgDecoder::DecodeUint32(uint32_t &aUint32) { return ReadCompactUint(mCodedMsg, mOffsetRange, aUint32); }

Error Coder::MsgDecoder::DecodeUint16(uint16_t &aUint16, uint8_t aFirstSegBitLength)
{
    Error    error;
    uint32_t u32;

    SuccessOrExit(error = ReadCompactUint(mCodedMsg, mOffsetRange, u32, aFirstSegBitLength));
    VerifyOrExit(u32 <= NumericLimits<uint16_t>::kMax, error = kErrorParse);
    aUint16 = static_cast<uint16_t>(u32);

exit:
    return error;
}

void Coder::MsgDecoder::UpdateHeaderRecordCounts(void)
{
    static constexpr uint16_t kHeaderOffset = 0;

    Dns::UpdateHeader updateHeader;

    IgnoreError(mMessage.Read(kHeaderOffset, updateHeader));

    updateHeader.SetUpdateRecordCount(mUpdateRecordCount);
    updateHeader.SetAdditionalRecordCount(mAddnlRecordCount);

    mMessage.Write(kHeaderOffset, updateHeader);
}

void Coder::MsgDecoder::UpdateRecordLengthInMessage(Dns::ResourceRecord &aRecord, uint16_t aOffset)
{
    // Determines the records DATA length and updates it in `mMessage`.
    // Should be called immediately after all the fields in the
    // record are appended to the message. `aOffset` gives the offset
    // in the message to the start of the record.

    aRecord.SetLength(mMessage.GetLength() - aOffset - sizeof(Dns::ResourceRecord));
    mMessage.Write(aOffset, aRecord);
}

Error Coder::MsgDecoder::AppendDeleteAllRrsets(void)
{
    // "Delete all RRsets from a name" (RFC 2136 - 2.5.3)
    // Name should be already appended in the message.

    Dns::ResourceRecord rr;

    rr.Init(Dns::ResourceRecord::kTypeAny, Dns::ResourceRecord::kClassAny);
    rr.SetTtl(0);
    rr.SetLength(0);

    return mMessage.Append(rr);
}

Error Coder::MsgDecoder::AppendHostName(void)
{
    Error error;

    // If host name was previously added in the message, add it
    // compressed as pointer to the previous one. Otherwise,
    // append it and remember the offset.

    if (mHostNameOffset != kUnknownOffset)
    {
        ExitNow(error = Dns::Name::AppendPointerLabel(mHostNameOffset, mMessage));
    }

    mHostNameOffset = mMessage.GetLength();
    SuccessOrExit(error = Dns::Name::AppendMultipleLabels(mHostName, mMessage));
    error = Dns::Name::AppendPointerLabel(mDomainNameOffset, mMessage);

exit:
    return error;
}

} // namespace Srp
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_CODER_ENABLE
