/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes implementation of Thread Direct Command Frames.
 */

#include "td_commands.hpp"

#if OT_CONFIG_THREAD_DIRECT_ENABLE

#include "common/bit_utils.hpp"
#include "common/code_utils.hpp"
#include "common/num_utils.hpp"
#include "mac/mac_frame.hpp"
#include "radio/radio.hpp"

namespace ot {
namespace ThreadDirect {

//---------------------------------------------------------------------------------------------------------------------
// CommandUtils

Error CommandUtils::ParseCommandHeader(uint8_t aSubCmdId, FrameData &aPayloadData)
{
    Error   error = kErrorParse;
    uint8_t byte;

    SuccessOrExit(aPayloadData.ReadUint8(byte));
    VerifyOrExit(byte == Mac::Frame::kCmdThread);

    SuccessOrExit(aPayloadData.ReadUint8(byte));
    VerifyOrExit(byte == aSubCmdId);

    error = kErrorNone;

exit:
    return error;
}

Error CommandUtils::AppendCommandHeader(uint8_t aSubCmdId, FrameBuilder &aBuilder)
{
    Error error;

    SuccessOrExit(error = aBuilder.AppendUint8(Mac::Frame::kCmdThread));
    error = aBuilder.AppendUint8(aSubCmdId);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// WakeCommand

bool WakeCommand::Info::IsValid(void) const
{
    bool valid = false;

    if (mHasConnParams)
    {
        VerifyOrExit(IsValueInRange(mRetryInterval, kMinRetryInterval, kMaxRetryInterval));
        VerifyOrExit(mRetryCount <= kMaxRetryCount);
    }

    if (mHasChannel)
    {
        VerifyOrExit(Radio::IsChannelValid(mChannel));
    }

    valid = true;

exit:
    return valid;
}

Error WakeCommand::Info::ParseFrom(const FrameData &aPayloadData)
{
    Error     error;
    FrameData payloadData = aPayloadData;
    uint8_t   flags;

    Clear();

    SuccessOrExit(error = ParseCommandHeader(kSubCmdId, payloadData));

    if (payloadData.ReadUint8(flags) != kErrorNone)
    {
        // Empty payload indicates all flags are zero.
        ExitNow();
    }

    if (flags & kFlagRendezvousTime)
    {
        SuccessOrExit(error = payloadData.ReadUint8(mRendezvousTime));
        mHasRendezvousTime = true;
    }

    if (flags & kFlagConnParams)
    {
        uint8_t byte;

        SuccessOrExit(error = payloadData.ReadUint8(byte));
        mHasConnParams = true;
        mRetryInterval = ReadBits<uint8_t, kRetryIntervalMask>(byte);
        mRetryCount    = ReadBits<uint8_t, kRetryCountMask>(byte);
    }

    if (flags & kFlagChannel)
    {
        SuccessOrExit(error = payloadData.ReadUint8(mChannel));
        mHasChannel = true;
    }

    if (flags & kFlagActionSelect)
    {
        SuccessOrExit(error = payloadData.ReadUint8(mActionSelect));
        mHasActionSelect = true;
    }

exit:
    return error;
}

Error WakeCommand::Info::PrepareIn(FrameBuilder &aBuilder) const
{
    Error   error;
    uint8_t flags;

    SuccessOrExit(error = AppendCommandHeader(kSubCmdId, aBuilder));

    flags = 0;
    flags |= (mHasRendezvousTime ? kFlagRendezvousTime : 0);
    flags |= (mHasConnParams ? kFlagConnParams : 0);
    flags |= (mHasChannel ? kFlagChannel : 0);
    flags |= (mHasActionSelect ? kFlagActionSelect : 0);

    if (flags == 0)
    {
        ExitNow();
    }

    SuccessOrExit(error = aBuilder.AppendUint8(flags));

    if (mHasRendezvousTime)
    {
        SuccessOrExit(error = aBuilder.AppendUint8(mRendezvousTime));
    }

    if (mHasConnParams)
    {
        uint8_t byte = 0;

        WriteBits<uint8_t, kRetryIntervalMask>(byte, mRetryInterval);
        WriteBits<uint8_t, kRetryCountMask>(byte, mRetryCount);
        SuccessOrExit(error = aBuilder.AppendUint8(byte));
    }

    if (mHasChannel)
    {
        SuccessOrExit(error = aBuilder.AppendUint8(mChannel));
    }

    if (mHasActionSelect)
    {
        SuccessOrExit(error = aBuilder.AppendUint8(mActionSelect));
    }

exit:
    return error;
}

} // namespace ThreadDirect
} // namespace ot

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE
