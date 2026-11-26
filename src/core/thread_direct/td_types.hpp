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
 *   This file includes definitions for Thread Direct related types.
 */

#ifndef OT_CORE_THREAD_DIRECT_TD_TYPES_HPP_
#define OT_CORE_THREAD_DIRECT_TD_TYPES_HPP_

#include "openthread-core-config.h"

#include <openthread/thread_direct.h>
#include <openthread/thread_direct_listener.h>
#include <openthread/platform/radio.h>

#include "common/as_core_type.hpp"
#include "common/bit_utils.hpp"
#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/error.hpp"
#include "common/heap_allocatable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/num_utils.hpp"
#include "common/numeric_limits.hpp"
#include "mac/mac_frame.hpp"
#include "mac/mac_types.hpp"

//----------------------------------------------------------------------------------------------------------------------

#if OT_CONFIG_THREAD_DIRECT_ENABLE

struct otThreadDirectKey
{
};

struct otThreadDirectLink
{
};

namespace ot {

class Settings;

namespace ThreadDirect {

/**
 * Type alias for a cryptographic key reference (`otCryptoKeyRef`).
 */
typedef Mac::KeyRef KeyRef;

/**
 * Type alias for a literal 16-byte MAC key.
 */
typedef Mac::Key LiteralKey;

/**
 * Type alias for Thread Direct key material (literal key or key reference).
 */
typedef Mac::KeyMaterial KeyMaterial;

/**
 * Represents a Thread Direct Key ID.
 */
OT_TOOL_PACKED_BEGIN
class KeyId : public otThreadDirectKeyId, public Equatable<KeyId>
{
public:
    static constexpr uint16_t kSize = OT_THREAD_DIRECT_KEY_ID_SIZE; ///< Size of a Thread Direct Key ID in bytes.

    static constexpr uint16_t kInfoStringSize = 2 * kSize + 1; ///< Max chars for the info string (`ToString()`).

    /**
     * Defines the fixed-length `String` object returned from `ToString()`.
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * Gets the Key ID bytes.
     *
     * @returns A pointer to the Key ID byte array of `kSize` bytes.
     */
    const uint8_t *GetBytes(void) const { return m8; }

    /**
     * Reads and populates the Key ID from a given parsed MAC frame header.
     *
     * @param[in] aFrameInfo  The parsed MAC frame info to read the Key ID from.
     *
     * @retval kErrorNone   Successfully read the Key ID from @p aFrameInfo.
     * @retval kErrorParse  The frame does not have a valid Mode 2 security header.
     */
    Error ReadFrom(const Mac::Frame::ParseInfo &aFrameInfo);

    /**
     * Converts the Key ID to a hex string.
     *
     * @returns An `InfoString` representing the Key ID in hex format.
     */
    InfoString ToString(void) const;

} OT_TOOL_PACKED_END;

class KeyManager;

/**
 * Represents a registered Thread Direct key entry.
 */
class Key : public otThreadDirectKey,
            public InstanceLocatorInit,
            public LinkedListEntry<Key>,
#if OPENTHREAD_CONFIG_TD_KEY_LIST_USE_HEAP_ENABLE
            public Heap::Allocatable<Key>,
#endif
            private NonCopyable
{
    friend class KeyManager;
    friend class LinkedListEntry<Key>;
    friend class LinkedList<Key>;
    friend class ot::Settings;

public:
    /**
     * Gets the key material associated with the Thread Direct key.
     *
     * @returns A reference to the `KeyMaterial`.
     */
    const KeyMaterial &GetKeyMaterial(void) const { return mKeyMaterial; }

    /**
     * Gets the Key ID associated with the Thread Direct key.
     *
     * @returns A reference to the `KeyId`.
     */
    const KeyId &GetId(void) const { return mId; }

    /**
     * Indicates whether the Thread Direct key has a given Key ID.
     *
     * @param[in] aId  The Key ID to check against.
     *
     * @retval TRUE   If the key's ID matches @p aId.
     * @retval FALSE  If the key's ID does not match @p aId.
     */
    bool HasId(const KeyId &aId) const { return mId == aId; }

    /**
     * Gets the current MAC security frame counter associated with the Thread Direct key.
     *
     * @returns The current MAC security frame counter.
     */
    uint32_t GetFrameCounter(void) const { return mFrameCounter; }

    /**
     * Increments the MAC security frame counter associated with the Thread Direct key.
     *
     * If the incremented counter reaches or exceeds the stored counter in non-volatile settings, a tasklet is posted
     * to update the persisted frame counter in settings.
     */
    void IncrementFrameCounter(void);

private:
    static constexpr uint32_t kStoredCounterAhead = OPENTHREAD_CONFIG_TD_FRAME_COUNTER_AHEAD;

    void Init(Instance &aInstance, const KeyId &aId, uint32_t aFrameCounter);
    void SetKeyMaterial(const KeyMaterial &aKeyMaterial) { mKeyMaterial = aKeyMaterial; }
#if !OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    void SetKeyMaterial(const LiteralKey &aLiteralKey) { mKeyMaterial.SetKey(aLiteralKey); }
#else
    void SetKeyMaterial(KeyRef aKeyRef) { mKeyMaterial.SetKeyRef(aKeyRef); }
    void SetKeyRefToInvalid(void) { mKeyMaterial.SetKeyRefToInvalid(); }
    bool Matches(KeyRef aKeyRef) const { return mKeyMaterial.GetKeyRef() == aKeyRef; }
#endif
    bool Matches(const KeyMaterial &aKeyMaterial, const KeyId &aId) const
    {
        return (mKeyMaterial == aKeyMaterial) && HasId(aId);
    }

    void     SetFrameCounters(uint32_t aFrameCounter);
    uint32_t GetSavedFrameCounter(void) const { return mSavedFrameCounter; }
    bool     ShouldUpdateSavedFrameCounter(void) const;
    uint32_t UpdateSavedFrameCounter(void);

    Key        *mNext;
    KeyMaterial mKeyMaterial;
    KeyId       mId;
    uint32_t    mFrameCounter;
    uint32_t    mSavedFrameCounter;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
 * Defines the slot durations for a time-slot grid (`otRadioAvailabilitySlotDuration`).
 */
enum SlotDuration : uint8_t
{
    kSlotDuration625Usec  = OT_RADIO_AVAILABILITY_SLOT_DURATION_625_US,  ///< 625 microseconds.
    kSlotDuration1250Usec = OT_RADIO_AVAILABILITY_SLOT_DURATION_1250_US, ///< 1250 microseconds (1.25 msec).
};

/**
 * Converts a `SlotDuration` enumerator to its duration in microseconds.
 *
 * @param[in] aSlotDuration  The slot duration enumerator.
 *
 * @returns The slot duration in microseconds.
 */
uint32_t SlotDurationToUsec(SlotDuration aSlotDuration);

class RadioAvailability;

/**
 * Represents a time window (start time and duration).
 */
class TimeWindow : public Clearable<TimeWindow>
{
    friend class RadioAvailability;

public:
    static constexpr uint32_t kMaxDuration = NumericLimits<uint32_t>::kMax; ///< Maximum (unbounded) duration.

    /**
     * Initializes the `TimeWindow` as cleared (`GetStartTime() == 0`, `GetDuration() == 0`).
     */
    TimeWindow(void) { Clear(); }

    /**
     * Initializes the `TimeWindow` with a given start time and duration.
     *
     * @param[in] aStartTime  The start time in microseconds.
     * @param[in] aDuration   The duration in microseconds (`kMaxDuration` indicates unbounded).
     */
    void Init(Radio::Time64 aStartTime, uint32_t aDuration)
    {
        mStartTime = aStartTime;
        mDuration  = aDuration;
    }

    /**
     * Gets the start time of the window.
     *
     * @returns The start time of the window in microseconds.
     */
    Radio::Time64 GetStartTime(void) const { return mStartTime; }

    /**
     * Gets the duration of the window.
     *
     * @returns The duration of the window in microseconds (`kMaxDuration` if unbounded).
     */
    uint32_t GetDuration(void) const { return mDuration; }

    /**
     * Indicates whether the window is unbounded (`GetDuration() == kMaxDuration`).
     *
     * @retval TRUE   If the window is unbounded.
     * @retval FALSE  If the window has a bounded duration.
     */
    bool IsUnbounded(void) const { return mDuration == kMaxDuration; }

    /**
     * Gets the end time of the window (`GetStartTime() + GetDuration()`).
     *
     * @returns The end time of the window in microseconds.
     */
    Radio::Time64 GetEndTime(void) const { return AddSafely<uint64_t>(mStartTime, mDuration); }

private:
    void ExtendDuration(uint32_t aDuration) { mDuration = AddSafely<uint32_t>(mDuration, aDuration); }
    void SetUnbounded(void) { mDuration = kMaxDuration; }

    Radio::Time64 mStartTime;
    uint32_t      mDuration;
};

/**
 * Represents a time-slot grid defined by a start (anchor) time and a `SlotDuration`.
 */
class SlotGrid
{
public:
    /**
     * Gets the start (anchor) time of the slot grid.
     *
     * @returns The start time in microseconds.
     */
    Radio::Time64 GetStartTime(void) const { return mStartTime; }

    /**
     * Gets the slot duration of the slot grid.
     *
     * @returns The slot duration enumerator.
     */
    SlotDuration GetSlotDuration(void) const { return mSlotDuration; }

    /**
     * Gets the slot duration in microseconds.
     *
     * @returns The slot duration in microseconds.
     */
    uint32_t GetSlotDurationInUsec(void) const { return SlotDurationToUsec(mSlotDuration); }

    /**
     * Converts a number of slots to duration in microseconds using this grid's slot duration.
     *
     * @param[in] aNumSlots  The number of slots.
     *
     * @returns The duration in microseconds.
     */
    uint32_t SlotsToUsec(uint16_t aNumSlots) const;

protected:
    struct SlotInfo
    {
        uint8_t GetIndexAsUint8(void) const { return static_cast<uint8_t>(mIndex); }

        uint16_t   mIndex;
        TimeWindow mWindow;
    };

    SlotGrid(void) = default;

    void Init(Radio::Time64 aStartTime, SlotDuration aSlotDuration)
    {
        mStartTime    = aStartTime;
        mSlotDuration = aSlotDuration;
    }

    void DetermineSlotInfo(Radio::Time64 aTime, uint16_t aPeriod, SlotInfo &aSlotInfo) const;
    void AdvanceToNextSlot(uint16_t aPeriod, SlotInfo &aSlotInfo) const;

    Radio::Time64 mStartTime;
    SlotDuration  mSlotDuration;
};

/**
 * Represents a periodic radio availability schedule bitmap.
 */
class RadioAvailability : public SlotGrid, public Clearable<RadioAvailability>
{
public:
    static constexpr uint8_t kMaxBitmapSize = OT_RADIO_AVAILABILITY_MAX_BITMAP_SIZE; ///< Max bitmap size in bytes.

    static constexpr uint8_t kMaxBitmapLength = kMaxBitmapSize * kBitsPerByte; ///< Max bitmap length in bits.

    /**
     * Represents the outcome of comparing two `RadioAvailability` schedules (see `Compare()`).
     */
    enum CompareResult : uint8_t
    {
        kMismatchUnspecified,  ///< One schedule is unspecified (always available) and the other is specified.
        kMismatchSlotDuration, ///< Schedules use different slot durations.
        kMismatchPeriod,       ///< Schedules have different periods (bitmap lengths).
        kMismatchAlignment,    ///< Slot times are not aligned (start time diff is not a multiple of slot duration).
        kMismatchPattern,      ///< Schedules are aligned with the same period, but their shifted bit patterns differ.
        kMatch,                ///< Schedules fully match (same effective availability over time).
    };

    /**
     * Initializes the `RadioAvailability` as cleared (not specified / always available).
     */
    RadioAvailability(void) { Clear(); }

    /**
     * Initializes the `RadioAvailability` schedule.
     *
     * @param[in] aStartTime     The start time of the schedule.
     * @param[in] aSlotDuration  The duration of each slot.
     * @param[in] aBitLength     The number of slots (bits) in the schedule (`0` indicates always available).
     * @param[in] aBitmap        A pointer to the MSB-first availability bitmap.
     *
     * @retval kErrorNone         Successfully initialized the schedule.
     * @retval kErrorInvalidArgs  @p aBitLength exceeds `kMaxBitmapLength`.
     */
    Error Init(Radio::Time64 aStartTime, SlotDuration aSlotDuration, uint8_t aBitLength, const uint8_t *aBitmap);

    /**
     * Indicates whether a periodic availability schedule is specified (`GetBitLength() != 0`).
     *
     * When not specified (`GetBitLength() == 0`), the radio is considered always available.
     *
     * @retval TRUE   If a schedule is specified.
     * @retval FALSE  If no schedule is specified (always available).
     */
    bool IsSpecified(void) const { return mBitLength != 0; }

    /**
     * Gets the length of the bitmap in bits (number of slots in the periodic schedule).
     *
     * @returns The bitmap length in bits (`0` if not specified).
     */
    uint8_t GetBitLength(void) const { return mBitLength; }

    /**
     * Gets the availability bitmap buffer.
     *
     * @returns A pointer to the availability bitmap byte array.
     */
    const uint8_t *GetBitmap(void) const { return mBitmap; }

    /**
     * Indicates whether the radio is available at a given time.
     *
     * @param[in] aTime  The radio time in microseconds to check.
     *
     * @retval TRUE   If the radio is available at @p aTime.
     * @retval FALSE  If the radio is not available at @p aTime.
     */
    bool IsAvailableAt(Radio::Time64 aTime) const;

    /**
     * Indicates whether the radio is continuously available throughout a given time window.
     *
     * @param[in] aWindow  The time window to check.
     *
     * @retval TRUE   If the radio is available throughout @p aWindow.
     * @retval FALSE  If the radio is unavailable during any part of @p aWindow.
     */
    bool IsAvailableIn(const TimeWindow &aWindow) const;

    /**
     * Finds the next contiguous available time window starting on or after a given time.
     *
     * If @p aTime falls within an available slot, @p aWindow starts at @p aTime and extends through the end of that
     * contiguous run of available slots. Otherwise, @p aWindow starts at the beginning of the next available slot.
     * If the radio is always available (either `!IsSpecified()` or all slots in the bitmap are available),
     * @p aWindow is marked unbounded (`aWindow.IsUnbounded()` is `true`).
     *
     * @param[in]  aTime    The radio time from which to start searching.
     * @param[out] aWindow  A reference to a `TimeWindow` to output the found available window.
     *
     * @retval kErrorNone      Successfully found an available window and populated @p aWindow.
     * @retval kErrorNotFound  No available slot exists in the schedule.
     */
    Error FindNextAvailableWindow(Radio::Time64 aTime, TimeWindow &aWindow) const;

    /**
     * Compares this `RadioAvailability` schedule with another schedule.
     *
     * If both schedules are unspecified (`!IsSpecified()`), they are considered a match and `kMatch` is returned.
     *
     * @param[in] aOther  The other `RadioAvailability` schedule to compare against.
     *
     * @returns A `CompareResult` indicating the comparison outcome.
     */
    CompareResult Compare(const RadioAvailability &aOther) const;

    /**
     * Rebases the schedule's start time and rotates its bitmap so the new start time is the closest slot boundary
     * at or after @p aTime.
     *
     * Does nothing if the schedule is unspecified (`!IsSpecified()`).
     *
     * @param[in] aTime  The reference radio time in microseconds.
     */
    void RebaseStartTime(Radio::Time64 aTime);

private:
    bool IsAvailable(uint8_t aBitIndex) const;

    uint8_t mBitLength;
    uint8_t mBitmap[kMaxBitmapSize];
};

/**
 * Represents a Scheduled Listen Window (SLW) schedule.
 */
class ListenSchedule : public SlotGrid, public Clearable<ListenSchedule>
{
public:
    static constexpr uint8_t kMaxMinDuration      = 16; ///< Max minimum duration in units of slot duration (`1..16`).
    static constexpr uint8_t kMaxExtendedDuration = 15; ///< Max extended duration in units of slot duration (`0..15`).

    /**
     * Initializes the `ListenSchedule` as cleared (not specified / continuous rx-on-when-idle).
     */
    ListenSchedule(void) { Clear(); }

    /**
     * Initializes the `ListenSchedule`.
     *
     * If @p aPeriod is `0`, all other parameters are ignored and the schedule is cleared (indicating continuous
     * rx-on-when-idle mode). If an error is returned, the existing `ListenSchedule` remains unchanged.
     *
     * @param[in] aStartTime         The start (anchor) time of the schedule.
     * @param[in] aSlotDuration      The duration of each slot.
     * @param[in] aPeriod            The schedule period in units of @p aSlotDuration (`0` for rx-on-when-idle).
     * @param[in] aPhase             The phase in units of @p aSlotDuration (reduced modulo @p aPeriod).
     * @param[in] aMinDuration       The minimum listen duration in units of @p aSlotDuration (`1..kMaxMinDuration`).
     * @param[in] aExtendedDuration  The extended listen duration in units of @p aSlotDuration
     *                               (`0..kMaxExtendedDuration`).
     *
     * @retval kErrorNone         Successfully initialized the schedule.
     * @retval kErrorInvalidArgs  One or more parameters are out of valid range.
     */
    Error Init(Radio::Time64 aStartTime,
               SlotDuration  aSlotDuration,
               uint16_t      aPeriod,
               uint16_t      aPhase,
               uint8_t       aMinDuration,
               uint8_t       aExtendedDuration);

    /**
     * Indicates whether a discrete listen schedule is specified (`GetPeriod() != 0`).
     *
     * When not specified (`GetPeriod() == 0`), the device is in continuous rx-on-when-idle mode.
     *
     * @retval TRUE   If a listen schedule is specified.
     * @retval FALSE  If no listen schedule is specified (continuous rx-on-when-idle).
     */
    bool IsSpecified(void) const { return mPeriod != 0; }

    /**
     * Gets the schedule period in units of the slot duration (`GetSlotDuration()`).
     *
     * @returns The period in units of the slot duration (`0` if not specified).
     */
    uint16_t GetPeriod(void) const { return mPeriod; }

    /**
     * Gets the schedule phase in units of the slot duration (`GetSlotDuration()`), specifying the offset from
     * `GetStartTime()` to the start of the active RX window.
     *
     * @returns The phase in units of the slot duration.
     */
    uint16_t GetPhase(void) const { return mPhase; }

    /**
     * Gets the minimum listen window duration in units of the slot duration (`GetSlotDuration()`).
     *
     * @returns The minimum listen window duration in units of the slot duration (`1..16`, or `0` if not specified).
     */
    uint8_t GetMinDuration(void) const { return mMinDuration; }

    /**
     * Gets the extended listen window duration in units of the slot duration (`GetSlotDuration()`).
     *
     * @returns The extended listen window duration in units of the slot duration (`0..15`, where `0` indicates
     *          unconstrained).
     */
    uint8_t GetExtendedDuration(void) const { return mExtendedDuration; }

    /**
     * Determines the listen window duration in microseconds.
     *
     * If `GetExtendedDuration()` is `0` (including when `!IsSpecified()`), the listen window duration is
     * unconstrained and `TimeWindow::kMaxDuration` is returned. Otherwise, returns the combined minimum and extended
     * duration in microseconds.
     *
     * @returns The listen window duration in microseconds (`TimeWindow::kMaxDuration` if unconstrained).
     */
    uint32_t DetermineListenDuration(void) const;

    /**
     * Finds the next Scheduled Listen Window starting on or after a given time.
     *
     * If `!IsSpecified()`, @p aWindow starts at @p aTime and is marked unbounded (`aWindow.IsUnbounded()` is `true`).
     * Otherwise, @p aWindow starts at the next listen window boundary on or after @p aTime, with duration equal to
     * `DetermineListenDuration()`.
     *
     * @param[in]  aTime    The radio time from which to start searching.
     * @param[out] aWindow  A reference to a `TimeWindow` to output the found listen window.
     */
    void FindNextListenWindow(Radio::Time64 aTime, TimeWindow &aWindow) const;

private:
    uint16_t mPeriod;
    uint16_t mPhase;
    uint8_t  mMinDuration;
    uint8_t  mExtendedDuration;
};

} // namespace ThreadDirect

DefineCoreType(otThreadDirectKeyId, ThreadDirect::KeyId);
DefineCoreType(otThreadDirectKey, ThreadDirect::Key);
DefineMapEnum(otRadioAvailabilitySlotDuration, ThreadDirect::SlotDuration);

} // namespace ot

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE

#endif // OT_CORE_THREAD_DIRECT_TD_TYPES_HPP_
