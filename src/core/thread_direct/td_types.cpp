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
 *   This file includes implementation of Thread Direct.
 */

#include "td_types.hpp"

#include "instance/instance.hpp"

#if OT_CONFIG_THREAD_DIRECT_ENABLE

namespace ot {
namespace ThreadDirect {

//---------------------------------------------------------------------------------------------------------------------
// KeyId

KeyId::InfoString KeyId::ToString(void) const
{
    InfoString string;

    string.AppendHexBytes(GetBytes(), kSize);
    return string;
}

//---------------------------------------------------------------------------------------------------------------------
// Key

void Key::Init(Instance &aInstance, const KeyId &aId, uint32_t aFrameCounter)
{
    InstanceLocatorInit::Init(aInstance);

    mId = aId;
    SetFrameCounters(aFrameCounter);
}

void Key::SetFrameCounters(uint32_t aFrameCounter)
{
    mFrameCounter      = aFrameCounter;
    mSavedFrameCounter = aFrameCounter;
}

void Key::IncrementFrameCounter(void)
{
    if (mFrameCounter != NumericLimits<uint32_t>::kMax)
    {
        mFrameCounter++;
    }

    if (ShouldUpdateSavedFrameCounter())
    {
        Get<KeyManager>().PostStoreTask();
    }
}

bool Key::ShouldUpdateSavedFrameCounter(void) const
{
    return (mFrameCounter >= mSavedFrameCounter) && (mSavedFrameCounter != NumericLimits<uint32_t>::kMax);
}

uint32_t Key::UpdateSavedFrameCounter(void)
{
    mSavedFrameCounter = AddSafely(mFrameCounter, kStoredCounterAhead);

    return mSavedFrameCounter;
}

//---------------------------------------------------------------------------------------------------------------------

uint32_t SlotDurationToUsec(SlotDuration aSlotDuration)
{
    uint32_t dur = 625;

    switch (aSlotDuration)
    {
    case kSlotDuration625Usec:
        break;
    case kSlotDuration1250Usec:
        dur = 1250;
        break;
    }

    return dur;
}

//---------------------------------------------------------------------------------------------------------------------
// RadioAvailability

Error RadioAvailability::Init(Radio::Time64  aStartTime,
                              SlotDuration   aSlotDuration,
                              uint8_t        aBitLength,
                              const uint8_t *aBitmap)
{
    Error error = kErrorNone;

    VerifyOrExit(aBitLength <= kMaxBitmapLength, error = kErrorInvalidArgs);

    Clear();
    VerifyOrExit(aBitLength > 0);

    mStartTime    = aStartTime;
    mSlotDuration = aSlotDuration;
    mBitLength    = aBitLength;
    memcpy(mBitmap, aBitmap, BytesForBitSize(aBitLength));

exit:
    return error;
}

void RadioAvailability::DetermineSlotInfo(Radio::Time64 aTime, SlotInfo &aSlotInfo) const
{
    // Determines the bitmap slot index (`aSlotInfo.mIndex`) corresponding
    // to `aTime` and populates `aSlotInfo.mWindow` with the remaining time
    // window in that slot `[aTime, slotEndTime)`.
    //
    // Caller MUST ensure `IsSpecified()` is true (`mBitLength > 0`).

    bool     isBeforeStartTime = (aTime < mStartTime);
    uint32_t slotDurInUsec     = SlotDurationToUsec(mSlotDuration);
    uint64_t diff;
    uint32_t offsetInSlot;

    // Each slot is a half-open interval `[start, start + slotDuration)`.
    // When `aTime < mStartTime`, the slot immediately before `mStartTime`
    // covers `[mStartTime - slotDuration, mStartTime - 1]`, so we measure
    // backwards from `mStartTime - 1`.

    diff = isBeforeStartTime ? (mStartTime - 1 - aTime) : (aTime - mStartTime);

    aSlotInfo.mIndex             = static_cast<uint8_t>((diff / slotDurInUsec) % mBitLength);
    offsetInSlot                 = static_cast<uint32_t>(diff % slotDurInUsec);
    aSlotInfo.mWindow.mStartTime = aTime;

    if (isBeforeStartTime)
    {
        // Map the backward slot offset (`0` for the slot right before
        // `mStartTime`) to the corresponding bitmap index from the end.

        aSlotInfo.mIndex            = mBitLength - 1 - aSlotInfo.mIndex;
        aSlotInfo.mWindow.mDuration = offsetInSlot + 1;
    }
    else
    {
        aSlotInfo.mWindow.mDuration = slotDurInUsec - offsetInSlot;
    }
}

void RadioAvailability::AdvanceToNextSlot(SlotInfo &aSlotInfo) const
{
    aSlotInfo.mIndex             = (aSlotInfo.mIndex + 1) % mBitLength;
    aSlotInfo.mWindow.mStartTime = aSlotInfo.mWindow.GetEndTime();
    aSlotInfo.mWindow.mDuration  = SlotDurationToUsec(mSlotDuration);
}

bool RadioAvailability::IsAvailable(uint8_t aBitIndex) const
{
    uint8_t bitFlag = (0x80 >> (aBitIndex % kBitsPerByte));

    return (mBitmap[aBitIndex / kBitsPerByte] & bitFlag);
}

bool RadioAvailability::IsAvailableAt(Radio::Time64 aTime) const
{
    bool     isAvailable = true;
    SlotInfo slotInfo;

    VerifyOrExit(IsSpecified());

    DetermineSlotInfo(aTime, slotInfo);
    isAvailable = IsAvailable(slotInfo.mIndex);

exit:
    return isAvailable;
}

bool RadioAvailability::IsAvailableIn(const Window &aWindow) const
{
    bool          isAvailable = false;
    Radio::Time64 endTime     = aWindow.GetEndTime();
    SlotInfo      slotInfo;

    VerifyOrExit(IsSpecified(), isAvailable = true);

    DetermineSlotInfo(aWindow.mStartTime, slotInfo);
    VerifyOrExit(IsAvailable(slotInfo.mIndex));

    for (uint8_t count = 1; (count < mBitLength) && (slotInfo.mWindow.GetEndTime() < endTime); count++)
    {
        AdvanceToNextSlot(slotInfo);
        VerifyOrExit(IsAvailable(slotInfo.mIndex));
    }

    isAvailable = true;

exit:
    return isAvailable;
}

Error RadioAvailability::FindNextAvailableWindow(Radio::Time64 aTime, Window &aWindow) const
{
    Error    error = kErrorNone;
    SlotInfo slotInfo;

    if (!IsSpecified())
    {
        aWindow.mStartTime = aTime;
        aWindow.mDuration  = Window::kMaxDuration;
        ExitNow();
    }

    DetermineSlotInfo(aTime, slotInfo);

    if (!IsAvailable(slotInfo.mIndex))
    {
        error = kErrorNotFound;

        for (uint8_t count = 1; count < mBitLength; count++)
        {
            AdvanceToNextSlot(slotInfo);

            if (IsAvailable(slotInfo.mIndex))
            {
                error = kErrorNone;
                break;
            }
        }

        SuccessOrExit(error);
    }

    aWindow = slotInfo.mWindow;

    for (uint8_t count = 1; count < mBitLength; count++)
    {
        AdvanceToNextSlot(slotInfo);
        VerifyOrExit(IsAvailable(slotInfo.mIndex));
        aWindow.mDuration += slotInfo.mWindow.mDuration;
    }

    aWindow.mDuration = Window::kMaxDuration;

exit:
    return error;
}

} // namespace ThreadDirect
} // namespace ot

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE
