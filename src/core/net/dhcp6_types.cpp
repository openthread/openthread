/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements generating and processing of DHCPv6 options.
 */

#include "dhcp6_types.hpp"

#if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE || OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE

namespace ot {
namespace Dhcp6 {

//---------------------------------------------------------------------------------------------------------------------
// Option

Error Option::FindOption(const Message &aMessage, Code aCode, OffsetRange &aOptionOffsetRange)
{
    OffsetRange msgOffsetRange;

    msgOffsetRange.InitFromMessageOffsetToEnd(aMessage);

    return FindOption(aMessage, msgOffsetRange, aCode, aOptionOffsetRange);
}

Error Option::FindOption(const Message     &aMessage,
                         const OffsetRange &aMsgOffsetRange,
                         Code               aCode,
                         OffsetRange       &aOptionOffsetRange)

{
    Error       error;
    OffsetRange offsetRange;

    // Restrict `offsetRange to `aMessage.GetLength()`. This way we
    // know if option is within `offsetRange` it is also fully
    // contained within the `aMessage`.

    offsetRange.InitFromRange(aMsgOffsetRange.GetOffset(), Min(aMessage.GetLength(), aMsgOffsetRange.GetEndOffset()));

    while (!offsetRange.IsEmpty())
    {
        Option option;

        SuccessOrExit(error = aMessage.Read(offsetRange, option));

        VerifyOrExit(offsetRange.Contains(option.GetSize()), error = kErrorParse);

        if (option.GetCode() == aCode)
        {
            aOptionOffsetRange = offsetRange;
            aOptionOffsetRange.ShrinkLength(static_cast<uint16_t>(option.GetSize()));
            ExitNow();
        }

        offsetRange.AdvanceOffset(option.GetSize());
    }

    error = kErrorNotFound;

exit:
    return error;
}

void Option::UpdateOptionLengthInMessage(Message &aMessage, uint16_t aOffset)
{
    Option option;

    IgnoreError(aMessage.Read(aOffset, option));
    option.SetLength(aMessage.GetLength() - aOffset - sizeof(Option));
    aMessage.Write(aOffset, option);
}

Error Option::AppendOption(Message &aMessage, Code aCode, const void *aData, uint16_t aDataLength)
{
    Error  error;
    Option option;

    option.SetCode(aCode);
    option.SetLength(aDataLength);

    SuccessOrExit(error = aMessage.Append(option));
    VerifyOrExit(aDataLength != 0);
    error = aMessage.AppendBytes(aData, aDataLength);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// Option::Iterator

void Option::Iterator::Init(const Message &aMessage, Code aCode)
{
    OffsetRange msgOffsetRange;

    msgOffsetRange.InitFromMessageOffsetToEnd(aMessage);
    Init(aMessage, msgOffsetRange, aCode);
}

void Option::Iterator::Init(const Message &aMessage, const OffsetRange &aMsgOffsetRange, Code aCode)
{
    mMessage        = &aMessage;
    mCode           = aCode;
    mMsgOffsetRange = aMsgOffsetRange;
    mError          = kErrorNone;
    mIsDone         = false;
    Advance();
}

void Option::Iterator::Advance(void)
{
    VerifyOrExit(!mIsDone);

    if (mMessage == nullptr)
    {
        mError  = kErrorInvalidState;
        mIsDone = true;
        ExitNow();
    }

    mError = Option::FindOption(*mMessage, mMsgOffsetRange, mCode, mOptionOffsetRange);

    if (mError == kErrorNone)
    {
        // Update `mMsgOffsetRange` to start after the current option,
        // preparing the iterator for the next call to `Advance()`.

        mMsgOffsetRange.InitFromRange(mOptionOffsetRange.GetEndOffset(), mMsgOffsetRange.GetEndOffset());
        ExitNow();
    }

    mOptionOffsetRange.Clear();

    mError  = (mError == kErrorNotFound) ? kErrorNone : mError;
    mIsDone = true;

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// Eui64Duid

void Eui64Duid::Init(const Mac::ExtAddress &aExtAddress)
{
    SetType(Duid::kTypeLinkLayerAddress);
    SetHardwareType(Duid::kHardwareTypeEui64);
    mLinkLayerAddress = aExtAddress;
}

bool Eui64Duid::IsValid(void) const
{
    return (GetType() == Duid::kTypeLinkLayerAddress) && (GetHardwareType() == Duid::kHardwareTypeEui64);
}

//---------------------------------------------------------------------------------------------------------------------
// IdOption

Error IdOption::Read(Option::Code aCode, const Message &aMessage, OffsetRange &aDuidOffsetRange)
{
    Error       error;
    OffsetRange optionOffsetRange;

    SuccessOrExit(error = Option::FindOption(aMessage, aCode, optionOffsetRange));
    optionOffsetRange.AdvanceOffset(sizeof(Option));

    VerifyOrExit(optionOffsetRange.GetLength() >= Duid::kMinSize, error = kErrorParse);

    aDuidOffsetRange = optionOffsetRange;
    aDuidOffsetRange.ShrinkLength(Duid::kMaxSize);

exit:
    return error;
}

Error IdOption::ReadEui64(Option::Code aCode, const Message &aMessage, Mac::ExtAddress &aExtAddress)
{
    Error       error;
    OffsetRange duidOffsetRange;
    Eui64Duid   eui64Duid;

    SuccessOrExit(error = Read(aCode, aMessage, duidOffsetRange));
    SuccessOrExit(error = aMessage.Read(duidOffsetRange, eui64Duid));
    VerifyOrExit(eui64Duid.IsValid(), error = kErrorParse);

    aExtAddress = eui64Duid.GetLinkLayerAddress();

exit:
    return error;
}

Error IdOption::MatchesEui64(Option::Code aCode, const Message &aMessage, const Mac::ExtAddress &aExtAddress)
{
    Error           error;
    Mac::ExtAddress extAddress;

    SuccessOrExit(error = ReadEui64(aCode, aMessage, extAddress));
    VerifyOrExit(extAddress == aExtAddress, error = kErrorNotFound);

exit:
    return error;
}

Error IdOption::AppendEui64(Option::Code aCode, Message &aMessage, const Mac::ExtAddress &aExtAddress)
{
    Eui64Duid eui64Duid;

    eui64Duid.Init(aExtAddress);

    return Option::AppendOption(aMessage, aCode, &eui64Duid, sizeof(eui64Duid));
}

//---------------------------------------------------------------------------------------------------------------------
// StatusCodeOption

StatusCodeOption::Status StatusCodeOption::ReadStatusFrom(const Message &aMessage)
{
    OffsetRange msgOffsetRange;

    msgOffsetRange.InitFromMessageOffsetToEnd(aMessage);

    return ReadStatusFrom(aMessage, msgOffsetRange);
}

StatusCodeOption::Status StatusCodeOption::ReadStatusFrom(const Message &aMessage, const OffsetRange &aMsgOffsetRange)
{
    Status           status = kSuccess;
    StatusCodeOption statusOption;
    OffsetRange      optionOffsetRange;

    // Per RFC 8415, the absence of a Status Code option implies success
    SuccessOrExit(FindOption(aMessage, aMsgOffsetRange, kStatusCode, optionOffsetRange));

    SuccessOrExit(aMessage.Read(optionOffsetRange, statusOption));
    status = static_cast<Status>(statusOption.GetStatusCode());

exit:
    return status;
}

//---------------------------------------------------------------------------------------------------------------------
// RapidCommitOption

Error RapidCommitOption::FindIn(const Message &aMessage)
{
    OffsetRange optionOffsetRange;

    return Option::FindOption(aMessage, Option::kRapidCommit, optionOffsetRange);
}

Error RapidCommitOption::AppendTo(Message &aMessage)
{
    return Option::AppendOption(aMessage, Option::kRapidCommit, /* aData */ nullptr, 0);
}

} // namespace Dhcp6
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE || OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE
