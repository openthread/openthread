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

Error KeyId::ReadFrom(const Mac::Frame::ParseInfo &aFrameInfo)
{
    static_assert(kSize == Mac::Frame::kKeySourceSizeMode2 + sizeof(uint8_t), "KeyId::kSize is invalid");

    Error error = kErrorParse;

    VerifyOrExit(aFrameInfo.mParsedSecurityHeader);
    VerifyOrExit(aFrameInfo.mIsSecurityEnabled);
    VerifyOrExit(aFrameInfo.mKeyIdMode == Mac::Frame::kKeyIdMode2);

    memcpy(m8, aFrameInfo.mKeySource.GetBytes(), Mac::Frame::kKeySourceSizeMode2);
    m8[Mac::Frame::kKeySourceSizeMode2] = aFrameInfo.mKeyIndex;

    error = kErrorNone;

exit:
    return error;
}

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
// SlotGrid

uint32_t SlotGrid::SlotsToUsec(uint16_t aNumSlots) const
{
    // `aNumSlots` is `uint16_t` (max 65535) and slot duration is at most
    // 1250 usec, so the product (~81.9M usec) fits safely in `uint32_t`.

    uint32_t dur = GetSlotDurationInUsec();

    dur *= aNumSlots;
    return dur;
}

void SlotGrid::DetermineSlotInfo(Radio::Time64 aTime, uint16_t aPeriod, SlotInfo &aSlotInfo) const
{
    // Determines the slot index within the time-slot grid (`aSlotInfo.mIndex`,
    // modulo `aPeriod`) corresponding to `aTime` and populates `aSlotInfo.mWindow`
    // with the remaining time window in that slot `[aTime, slotEndTime)`.
    //
    // Caller MUST ensure `aPeriod > 0`.

    bool     isBeforeStartTime = (aTime < mStartTime);
    uint32_t slotDurInUsec     = GetSlotDurationInUsec();
    uint64_t diff;
    uint32_t offsetInSlot;

    // Each slot is a half-open interval `[start, start + slotDuration)`.
    // When `aTime < mStartTime`, the slot immediately before `mStartTime`
    // covers `[mStartTime - slotDuration, mStartTime - 1]`, so we measure
    // backwards from `mStartTime - 1`.

    diff = isBeforeStartTime ? (mStartTime - 1 - aTime) : (aTime - mStartTime);

    aSlotInfo.mIndex = static_cast<uint16_t>((diff / slotDurInUsec) % aPeriod);
    offsetInSlot     = static_cast<uint32_t>(diff % slotDurInUsec);

    if (isBeforeStartTime)
    {
        // Map the backward slot offset (`0` for the slot right before
        // `mStartTime`) to the corresponding slot index from the end.

        aSlotInfo.mIndex = aPeriod - 1 - aSlotInfo.mIndex;
        aSlotInfo.mWindow.Init(aTime, offsetInSlot + 1);
    }
    else
    {
        aSlotInfo.mWindow.Init(aTime, slotDurInUsec - offsetInSlot);
    }
}

void SlotGrid::AdvanceToNextSlot(uint16_t aPeriod, SlotInfo &aSlotInfo) const
{
    aSlotInfo.mIndex++;
    aSlotInfo.mIndex %= aPeriod;

    aSlotInfo.mWindow.Init(aSlotInfo.mWindow.GetEndTime(), GetSlotDurationInUsec());
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

    SlotGrid::Init(aStartTime, aSlotDuration);
    mBitLength = aBitLength;
    memcpy(mBitmap, aBitmap, BytesForBitSize(aBitLength));

exit:
    return error;
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

    DetermineSlotInfo(aTime, mBitLength, slotInfo);
    isAvailable = IsAvailable(slotInfo.GetIndexAsUint8());

exit:
    return isAvailable;
}

bool RadioAvailability::IsAvailableIn(const TimeWindow &aWindow) const
{
    bool          isAvailable = false;
    Radio::Time64 endTime     = aWindow.GetEndTime();
    SlotInfo      slotInfo;

    VerifyOrExit(IsSpecified(), isAvailable = true);

    DetermineSlotInfo(aWindow.GetStartTime(), mBitLength, slotInfo);
    VerifyOrExit(IsAvailable(slotInfo.GetIndexAsUint8()));

    for (uint8_t count = 1; (count < mBitLength) && (slotInfo.mWindow.GetEndTime() < endTime); count++)
    {
        AdvanceToNextSlot(mBitLength, slotInfo);
        VerifyOrExit(IsAvailable(slotInfo.GetIndexAsUint8()));
    }

    isAvailable = true;

exit:
    return isAvailable;
}

Error RadioAvailability::FindNextAvailableWindow(Radio::Time64 aTime, TimeWindow &aWindow) const
{
    Error    error = kErrorNone;
    SlotInfo slotInfo;

    if (!IsSpecified())
    {
        aWindow.Init(aTime, TimeWindow::kMaxDuration);
        ExitNow();
    }

    DetermineSlotInfo(aTime, mBitLength, slotInfo);

    if (!IsAvailable(slotInfo.GetIndexAsUint8()))
    {
        error = kErrorNotFound;

        for (uint8_t count = 1; count < mBitLength; count++)
        {
            AdvanceToNextSlot(mBitLength, slotInfo);

            if (IsAvailable(slotInfo.GetIndexAsUint8()))
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
        AdvanceToNextSlot(mBitLength, slotInfo);
        VerifyOrExit(IsAvailable(slotInfo.GetIndexAsUint8()));
        aWindow.ExtendDuration(slotInfo.mWindow.GetDuration());
    }

    aWindow.SetUnbounded();

exit:
    return error;
}

RadioAvailability::CompareResult RadioAvailability::Compare(const RadioAvailability &aOther) const
{
    CompareResult result = kMatch;
    SlotInfo      slotInfo;

    if (!IsSpecified() || !aOther.IsSpecified())
    {
        VerifyOrExit(IsSpecified() == aOther.IsSpecified(), result = kMismatchUnspecified);
        ExitNow();
    }

    VerifyOrExit(mSlotDuration == aOther.mSlotDuration, result = kMismatchSlotDuration);
    VerifyOrExit(mBitLength == aOther.mBitLength, result = kMismatchPeriod);

    DetermineSlotInfo(aOther.mStartTime, mBitLength, slotInfo);
    VerifyOrExit(slotInfo.mWindow.GetDuration() == GetSlotDurationInUsec(), result = kMismatchAlignment);

    for (uint8_t i = 0; i < mBitLength; i++)
    {
        VerifyOrExit(IsAvailable(slotInfo.GetIndexAsUint8()) == aOther.IsAvailable(i), result = kMismatchPattern);
        slotInfo.mIndex = (slotInfo.mIndex + 1) % mBitLength;
    }

exit:
    return result;
}

void RadioAvailability::RebaseStartTime(Radio::Time64 aTime)
{
    SlotInfo slotInfo;

    VerifyOrExit(IsSpecified());

    DetermineSlotInfo(aTime, mBitLength, slotInfo);

    if (slotInfo.mWindow.GetDuration() < GetSlotDurationInUsec())
    {
        AdvanceToNextSlot(mBitLength, slotInfo);
    }

    mStartTime = slotInfo.mWindow.GetStartTime();

    if (slotInfo.mIndex != 0)
    {
        RotateBitmask(mBitmap, mBitLength, mBitLength - slotInfo.GetIndexAsUint8());
    }

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// ListenSchedule

Error ListenSchedule::Init(Radio::Time64 aStartTime,
                           SlotDuration  aSlotDuration,
                           uint16_t      aPeriod,
                           uint16_t      aPhase,
                           uint8_t       aMinDuration,
                           uint8_t       aExtendedDuration)
{
    Error error = kErrorNone;

    if (aPeriod == 0)
    {
        Clear();
        ExitNow();
    }

    VerifyOrExit(IsValueInRange<uint8_t>(aMinDuration, 1, kMaxMinDuration), error = kErrorInvalidArgs);
    VerifyOrExit(aExtendedDuration <= kMaxExtendedDuration, error = kErrorInvalidArgs);

    SlotGrid::Init(aStartTime, aSlotDuration);
    mPeriod           = aPeriod;
    mPhase            = aPhase % aPeriod;
    mMinDuration      = aMinDuration;
    mExtendedDuration = aExtendedDuration;

exit:
    return error;
}

uint32_t ListenSchedule::DetermineListenDuration(void) const
{
    return (mExtendedDuration == 0) ? TimeWindow::kMaxDuration : SlotsToUsec(mMinDuration + mExtendedDuration);
}

void ListenSchedule::FindNextListenWindow(Radio::Time64 aTime, TimeWindow &aWindow) const
{
    SlotInfo      slotInfo;
    uint16_t      slotsUntilPhase;
    Radio::Time64 startTime;

    if (!IsSpecified())
    {
        aWindow.Init(aTime, TimeWindow::kMaxDuration);
        ExitNow();
    }

    DetermineSlotInfo(aTime, mPeriod, slotInfo);

    if (slotInfo.mWindow.GetDuration() < GetSlotDurationInUsec())
    {
        AdvanceToNextSlot(mPeriod, slotInfo);
    }

    slotsUntilPhase = (mPhase >= slotInfo.mIndex) ? (mPhase - slotInfo.mIndex) : (mPeriod - slotInfo.mIndex + mPhase);
    startTime       = slotInfo.mWindow.GetStartTime() + SlotsToUsec(slotsUntilPhase);

    aWindow.Init(startTime, DetermineListenDuration());

exit:
    return;
}

} // namespace ThreadDirect
} // namespace ot

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE
