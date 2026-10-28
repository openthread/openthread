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

#include <openthread/config.h>

#include "test_platform.h"
#include "test_util.hpp"

#include "common/bit_utils.hpp"
#include "common/code_utils.hpp"
#include "common/num_utils.hpp"
#include "thread_direct/td_types.hpp"

namespace ot {
namespace ThreadDirect {

#if OT_CONFIG_THREAD_DIRECT_ENABLE

using Window = RadioAvailability::Window;

static bool GetMaskBit(const uint8_t *aMask, uint16_t aBitIndex)
{
    return (aMask[aBitIndex / kBitsPerByte] & (0x80 >> (aBitIndex % kBitsPerByte))) != 0;
}

static void VerifyWindowProperties(const RadioAvailability &aAvail, Radio::Time64 aQueryTime, const Window &aWindow)
{
    VerifyOrQuit(aWindow.mStartTime >= aQueryTime);
    VerifyOrQuit(aWindow.mDuration > 0);
    VerifyOrQuit(aAvail.IsAvailableIn(aWindow));
    VerifyOrQuit(aAvail.IsAvailableAt(aWindow.mStartTime));

    if (aWindow.mStartTime > aQueryTime)
    {
        VerifyOrQuit(!aAvail.IsAvailableAt(aQueryTime));
        VerifyOrQuit(!aAvail.IsAvailableAt(aWindow.mStartTime - 1));
    }

    if (aWindow.mDuration != Window::kMaxDuration)
    {
        Window extendedWindow = aWindow;

        VerifyOrQuit(aAvail.IsAvailableAt(aWindow.GetEndTime() - 1));
        VerifyOrQuit(!aAvail.IsAvailableAt(aWindow.GetEndTime()));

        extendedWindow.mDuration += 1;
        VerifyOrQuit(!aAvail.IsAvailableIn(extendedWindow));
    }
}

void TestRadioAvailabilityInitAndUnspecified(void)
{
    static const uint8_t kFullBitmap[RadioAvailability::kMaxBitmapSize] = {
        0xa5, 0x5a, 0xf0, 0x0f, 0xcc, 0x33, 0xaa, 0x55, 0x12, 0x34, 0x56, 0x78,
    };

    RadioAvailability avail;
    Window            window;
    Window            saturatingWindow;

    VerifyOrQuit(SlotDurationToUsec(kSlotDuration625Usec) == 625);
    VerifyOrQuit(SlotDurationToUsec(kSlotDuration1250Usec) == 1250);

    // Default-constructed (unspecified) availability.
    VerifyOrQuit(!avail.IsSpecified());
    VerifyOrQuit(avail.GetBitLength() == 0);
    VerifyOrQuit(avail.GetStartTime() == 0);

    VerifyOrQuit(avail.IsAvailableAt(0));
    VerifyOrQuit(avail.IsAvailableAt(1));
    VerifyOrQuit(avail.IsAvailableAt(123456789ULL));
    VerifyOrQuit(avail.IsAvailableAt(Radio::kMaxTime64));

    VerifyOrQuit(avail.IsAvailableIn(Window{0, 0}));
    VerifyOrQuit(avail.IsAvailableIn(Window{0, 100000}));
    VerifyOrQuit(avail.IsAvailableIn(Window{500000, Window::kMaxDuration}));
    VerifyOrQuit(avail.IsAvailableIn(Window{Radio::kMaxTime64 - 10, 100}));

    SuccessOrQuit(avail.FindNextAvailableWindow(0, window));
    VerifyOrQuit(window.mStartTime == 0);
    VerifyOrQuit(window.mDuration == Window::kMaxDuration);

    SuccessOrQuit(avail.FindNextAvailableWindow(987654321ULL, window));
    VerifyOrQuit(window.mStartTime == 987654321ULL);
    VerifyOrQuit(window.mDuration == Window::kMaxDuration);

    SuccessOrQuit(avail.FindNextAvailableWindow(Radio::kMaxTime64, window));
    VerifyOrQuit(window.mStartTime == Radio::kMaxTime64);
    VerifyOrQuit(window.mDuration == Window::kMaxDuration);

    // `Window::GetEndTime()` normal addition and saturation.
    window = Window{1000, 2500};
    VerifyOrQuit(window.GetEndTime() == 3500);

    saturatingWindow = Window{Radio::kMaxTime64 - 10, 50};
    VerifyOrQuit(saturatingWindow.GetEndTime() == Radio::kMaxTime64);

    // Invalid bit lengths (> kMaxBitmapLength).
    VerifyOrQuit(avail.Init(1000, kSlotDuration625Usec, RadioAvailability::kMaxBitmapLength + 1, kFullBitmap) ==
                 kErrorInvalidArgs);
    VerifyOrQuit(!avail.IsSpecified());

    VerifyOrQuit(avail.Init(1000, kSlotDuration1250Usec, NumericLimits<uint8_t>::kMax, kFullBitmap) ==
                 kErrorInvalidArgs);
    VerifyOrQuit(!avail.IsSpecified());

    // Valid initialization at maximum bit length (96 bits = 12 bytes).
    SuccessOrQuit(avail.Init(42000, kSlotDuration1250Usec, RadioAvailability::kMaxBitmapLength, kFullBitmap));
    VerifyOrQuit(avail.IsSpecified());
    VerifyOrQuit(avail.GetStartTime() == 42000);
    VerifyOrQuit(avail.GetSlotDuration() == kSlotDuration1250Usec);
    VerifyOrQuit(avail.GetBitLength() == RadioAvailability::kMaxBitmapLength);
    VerifyOrQuit(memcmp(avail.GetBitmap(), kFullBitmap, sizeof(kFullBitmap)) == 0);

    // Re-initializing with bit length 0 (including with `nullptr` bitmap) marks it unspecified.
    SuccessOrQuit(avail.Init(1000, kSlotDuration625Usec, 0, nullptr));
    VerifyOrQuit(!avail.IsSpecified());
    VerifyOrQuit(avail.GetBitLength() == 0);
    VerifyOrQuit(avail.GetStartTime() == 0);
    VerifyOrQuit(avail.IsAvailableAt(500));

    // Re-initialize and then `Clear()`.
    SuccessOrQuit(avail.Init(1000, kSlotDuration625Usec, 8, kFullBitmap));
    VerifyOrQuit(avail.IsSpecified());
    avail.Clear();
    VerifyOrQuit(!avail.IsSpecified());
    VerifyOrQuit(avail.GetBitLength() == 0);
    VerifyOrQuit(avail.GetStartTime() == 0);

    printf("TestRadioAvailabilityInitAndUnspecified() passed\n");
}

void TestRadioAvailabilityIsAvailableAt(void)
{
    static const SlotDuration kSlotDurations[] = {kSlotDuration625Usec, kSlotDuration1250Usec};

    // 8-bit pattern 0xb2 = 0b10110010:
    // Slot 0: 1, Slot 1: 0, Slot 2: 1, Slot 3: 1, Slot 4: 0, Slot 5: 0, Slot 6: 1, Slot 7: 0
    static const uint8_t kBitmap8[]      = {0xb2};
    static const bool    kExpected8[8]   = {true, false, true, true, false, false, true, false};
    static const int32_t kCyclesToTest[] = {-10, -3, -2, -1, 0, 1, 2, 3, 10};

    // 16-bit pattern for testing non-byte-aligned bit lengths (1..13).
    static const uint8_t kPattern16[] = {0b10110100, 0b11010010};

    for (SlotDuration slotDuration : kSlotDurations)
    {
        uint32_t          slotDur   = SlotDurationToUsec(slotDuration);
        Radio::Time64     startTime = 1000000ULL;
        RadioAvailability avail;

        SuccessOrQuit(avail.Init(startTime, slotDuration, 8, kBitmap8));

        for (int32_t cycle : kCyclesToTest)
        {
            for (uint8_t slot = 0; slot < 8; slot++)
            {
                int64_t       slotOffset   = static_cast<int64_t>(cycle) * 8 + slot;
                Radio::Time64 slotStart    = static_cast<Radio::Time64>(static_cast<int64_t>(startTime) +
                                                                        slotOffset * static_cast<int64_t>(slotDur));
                bool          expected     = kExpected8[slot];
                bool          prevExpected = kExpected8[(slot + 7) % 8];
                bool          nextExpected = kExpected8[(slot + 1) % 8];

                VerifyOrQuit(avail.IsAvailableAt(slotStart - 1) == prevExpected);
                VerifyOrQuit(avail.IsAvailableAt(slotStart) == expected);
                VerifyOrQuit(avail.IsAvailableAt(slotStart + 1) == expected);
                VerifyOrQuit(avail.IsAvailableAt(slotStart + slotDur / 2) == expected);
                VerifyOrQuit(avail.IsAvailableAt(slotStart + slotDur - 1) == expected);
                VerifyOrQuit(avail.IsAvailableAt(slotStart + slotDur) == nextExpected);
            }
        }

        // Also test with `startTime == 0`.
        SuccessOrQuit(avail.Init(0, slotDuration, 8, kBitmap8));
        for (uint8_t cycle = 0; cycle < 4; cycle++)
        {
            for (uint8_t slot = 0; slot < 8; slot++)
            {
                Radio::Time64 slotStart = static_cast<Radio::Time64>(cycle * 8 + slot) * slotDur;

                VerifyOrQuit(avail.IsAvailableAt(slotStart) == kExpected8[slot]);
                VerifyOrQuit(avail.IsAvailableAt(slotStart + slotDur - 1) == kExpected8[slot]);
            }
        }

        // Test non-byte-aligned bit lengths (1 through 13).
        for (uint8_t bitLen = 1; bitLen <= 13; bitLen++)
        {
            SuccessOrQuit(avail.Init(startTime, slotDuration, bitLen, kPattern16));

            for (int32_t slotOffset = -3 * bitLen; slotOffset <= 3 * bitLen; slotOffset++)
            {
                int32_t       modIndex  = ((slotOffset % bitLen) + bitLen) % bitLen;
                bool          expected  = GetMaskBit(kPattern16, static_cast<uint8_t>(modIndex));
                Radio::Time64 slotStart = static_cast<Radio::Time64>(static_cast<int64_t>(startTime) +
                                                                     slotOffset * static_cast<int64_t>(slotDur));

                VerifyOrQuit(avail.IsAvailableAt(slotStart) == expected);
                VerifyOrQuit(avail.IsAvailableAt(slotStart + slotDur / 2) == expected);
                VerifyOrQuit(avail.IsAvailableAt(slotStart + slotDur - 1) == expected);
            }
        }
    }

    printf("TestRadioAvailabilityIsAvailableAt() passed\n");
}

void TestRadioAvailabilityIsAvailableIn(void)
{
    static const SlotDuration kSlotDurations[] = {kSlotDuration625Usec, kSlotDuration1250Usec};

    // 6-bit pattern: 0b110101 (with trailing bits set to 1 in `0xd7` to ensure trailing bits are ignored).
    // Slot 0: 1, Slot 1: 1, Slot 2: 0, Slot 3: 1, Slot 4: 0, Slot 5: 1.
    // Note that Slot 5 (1) wraps around into Slot 0 (1) and Slot 1 (1), creating a 3-slot available run!
    static const uint8_t kBitmap6[] = {0b11010111};
    static const uint8_t kAllOnes[] = {0xff, 0xff};

    for (SlotDuration slotDuration : kSlotDurations)
    {
        uint32_t          slotDur   = SlotDurationToUsec(slotDuration);
        Radio::Time64     startTime = 500000ULL;
        RadioAvailability avail;

        SuccessOrQuit(avail.Init(startTime, slotDuration, 6, kBitmap6));

        // 1. Zero-duration windows (`mDuration == 0`).
        VerifyOrQuit(avail.IsAvailableIn(Window{startTime, 0}));
        VerifyOrQuit(avail.IsAvailableIn(Window{startTime + 2 * slotDur - 1, 0}));
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime + 2 * slotDur, 0}));
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime + 2 * slotDur + 100, 0}));
        VerifyOrQuit(avail.IsAvailableIn(Window{startTime - 1, 0}));
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime - slotDur - 1, 0}));

        // 2. 1-microsecond windows at exact slot boundaries.
        VerifyOrQuit(avail.IsAvailableIn(Window{startTime, 1}));
        VerifyOrQuit(avail.IsAvailableIn(Window{startTime + 2 * slotDur - 1, 1}));
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime + 2 * slotDur - 1, 2}));
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime + 2 * slotDur, 1}));

        // 3. Single-slot window aligned with isolated available Slot 3 `[startTime + 3 * slotDur, startTime + 4 *
        // slotDur)`.
        VerifyOrQuit(avail.IsAvailableIn(Window{startTime + 3 * slotDur, slotDur}));
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime + 3 * slotDur, slotDur + 1}));
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime + 3 * slotDur - 1, slotDur}));
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime + 3 * slotDur - 1, 2}));

        // 4. Short windows (< slotDur) straddling a boundary between two slots.
        // Between Slot 0 (1) and Slot 1 (1): available.
        VerifyOrQuit(avail.IsAvailableIn(Window{startTime + slotDur - 10, 20}));
        // Between Slot 1 (1) and Slot 2 (0): unavailable.
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime + 2 * slotDur - 10, 20}));
        // Between Slot 2 (0) and Slot 3 (1): unavailable.
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime + 3 * slotDur - 10, 20}));
        // Between Slot 5 (1) and Slot 0 (1) across `startTime`: available.
        VerifyOrQuit(avail.IsAvailableIn(Window{startTime - 10, 20}));
        // Between Slot 4 (0) and Slot 5 (1) before `startTime`: unavailable.
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime - slotDur - 10, 20}));

        // 5. Multi-slot window spanning the 3-slot contiguous run `[Slot 5, Slot 0, Slot 1]` across `startTime`.
        VerifyOrQuit(avail.IsAvailableIn(Window{startTime - slotDur, 3 * slotDur}));
        VerifyOrQuit(avail.IsAvailableIn(Window{startTime - slotDur + 50, 3 * slotDur - 100}));
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime - slotDur - 1, 3 * slotDur + 1}));
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime - slotDur, 3 * slotDur + 1}));

        // 6. Same 3-slot wrap-around run in a later cycle `[startTime + 5 * slotDur, startTime + 8 * slotDur)`.
        VerifyOrQuit(avail.IsAvailableIn(Window{startTime + 5 * slotDur, 3 * slotDur}));
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime + 5 * slotDur - 1, 3 * slotDur + 1}));
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime + 5 * slotDur, 3 * slotDur + 1}));
        VerifyOrQuit(!avail.IsAvailableIn(Window{startTime, Window::kMaxDuration}));

        // 7. All-ones bitmap with `Window::kMaxDuration`.
        SuccessOrQuit(avail.Init(startTime, slotDuration, 11, kAllOnes));
        VerifyOrQuit(avail.IsAvailableIn(Window{startTime, Window::kMaxDuration}));
        VerifyOrQuit(avail.IsAvailableIn(Window{startTime - 12345, Window::kMaxDuration}));
    }

    printf("TestRadioAvailabilityIsAvailableIn() passed\n");
}

void TestRadioAvailabilityFindNextAvailableWindow(void)
{
    static const SlotDuration kSlotDurations[] = {kSlotDuration625Usec, kSlotDuration1250Usec};

    for (SlotDuration slotDuration : kSlotDurations)
    {
        uint32_t          slotDur   = SlotDurationToUsec(slotDuration);
        Radio::Time64     startTime = 2000000ULL;
        RadioAvailability avail;
        Window            window;

        // 1. All-zeros schedules (including non-byte-aligned with trailing 1s in unused bits) -> `kErrorNotFound`.
        {
            static const uint8_t kZeroBytes[RadioAvailability::kMaxBitmapSize] = {0};
            static const uint8_t kOneBitZeroWithTrailingOnes[]                 = {0x7f};
            static const uint8_t kFiveBitsZeroWithTrailingOnes[]               = {0x07};
            static const uint8_t kThirteenBitsZeroWithTrailingOnes[]           = {0x00, 0x07};

            SuccessOrQuit(avail.Init(startTime, slotDuration, 8, kZeroBytes));
            VerifyOrQuit(avail.FindNextAvailableWindow(startTime, window) == kErrorNotFound);
            VerifyOrQuit(avail.FindNextAvailableWindow(startTime - 5000, window) == kErrorNotFound);

            SuccessOrQuit(avail.Init(startTime, slotDuration, RadioAvailability::kMaxBitmapLength, kZeroBytes));
            VerifyOrQuit(avail.FindNextAvailableWindow(startTime, window) == kErrorNotFound);

            SuccessOrQuit(avail.Init(startTime, slotDuration, 1, kOneBitZeroWithTrailingOnes));
            VerifyOrQuit(avail.FindNextAvailableWindow(startTime, window) == kErrorNotFound);
            VerifyOrQuit(avail.FindNextAvailableWindow(startTime - 1, window) == kErrorNotFound);

            SuccessOrQuit(avail.Init(startTime, slotDuration, 5, kFiveBitsZeroWithTrailingOnes));
            VerifyOrQuit(avail.FindNextAvailableWindow(startTime, window) == kErrorNotFound);

            SuccessOrQuit(avail.Init(startTime, slotDuration, 13, kThirteenBitsZeroWithTrailingOnes));
            VerifyOrQuit(avail.FindNextAvailableWindow(startTime, window) == kErrorNotFound);
        }

        // 2. All-ones schedules (1 bit, 5 bits with trailing 0s, and 96 bits) -> `Window::kMaxDuration`.
        {
            static const uint8_t kOneBitOne[]                                = {0x80};
            static const uint8_t kFiveBitsOneWithTrailingZeros[]             = {0xf8};
            static const uint8_t kAllOnes[RadioAvailability::kMaxBitmapSize] = {
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            };

            SuccessOrQuit(avail.Init(startTime, slotDuration, 1, kOneBitOne));
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime - 1234, window));
            VerifyOrQuit(window.mStartTime == startTime - 1234);
            VerifyOrQuit(window.mDuration == Window::kMaxDuration);
            VerifyWindowProperties(avail, startTime - 1234, window);

            SuccessOrQuit(avail.Init(startTime, slotDuration, 5, kFiveBitsOneWithTrailingZeros));
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime + 333, window));
            VerifyOrQuit(window.mStartTime == startTime + 333);
            VerifyOrQuit(window.mDuration == Window::kMaxDuration);
            VerifyWindowProperties(avail, startTime + 333, window);

            SuccessOrQuit(avail.Init(startTime, slotDuration, RadioAvailability::kMaxBitmapLength, kAllOnes));
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime, window));
            VerifyOrQuit(window.mStartTime == startTime);
            VerifyOrQuit(window.mDuration == Window::kMaxDuration);
        }

        // 3. 1-Available / 1-Unavailable alternating (`mBitLength = 2`, `0b10` = `0x80`).
        {
            static const uint8_t kAlternating[] = {0x80};

            SuccessOrQuit(avail.Init(startTime, slotDuration, 2, kAlternating));

            // At start of available Slot 0.
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime, window));
            VerifyOrQuit(window.mStartTime == startTime);
            VerifyOrQuit(window.mDuration == slotDur);
            VerifyWindowProperties(avail, startTime, window);

            // Partway through Slot 0.
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime + 100, window));
            VerifyOrQuit(window.mStartTime == startTime + 100);
            VerifyOrQuit(window.mDuration == slotDur - 100);
            VerifyWindowProperties(avail, startTime + 100, window);

            // At start of unavailable Slot 1 -> jumps to next Slot 0 (`startTime + 2 * slotDur`).
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime + slotDur, window));
            VerifyOrQuit(window.mStartTime == startTime + 2 * slotDur);
            VerifyOrQuit(window.mDuration == slotDur);
            VerifyWindowProperties(avail, startTime + slotDur, window);

            // 1 us before `startTime` (which is the end of unavailable Slot 1 in previous cycle).
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime - 1, window));
            VerifyOrQuit(window.mStartTime == startTime);
            VerifyOrQuit(window.mDuration == slotDur);
            VerifyWindowProperties(avail, startTime - 1, window);
        }

        // 4. 6-bit pattern `0b110101` (`0xd4`):
        // Slots: 0:1, 1:1, 2:0, 3:1, 4:0, 5:1 (Slots 5, 0, 1 form a 3-slot wrap-around available window).
        {
            static const uint8_t kBitmap6[] = {0xd4};

            SuccessOrQuit(avail.Init(startTime, slotDuration, 6, kBitmap6));

            // At `startTime` (Slot 0): covers Slots 0 and 1 (`2 * slotDur`).
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime, window));
            VerifyOrQuit(window.mStartTime == startTime);
            VerifyOrQuit(window.mDuration == 2 * slotDur);
            VerifyWindowProperties(avail, startTime, window);

            // Mid-Slot 0 (`startTime + 50`): covers remainder of Slot 0 + Slot 1.
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime + 50, window));
            VerifyOrQuit(window.mStartTime == startTime + 50);
            VerifyOrQuit(window.mDuration == 2 * slotDur - 50);
            VerifyWindowProperties(avail, startTime + 50, window);

            // Last microsecond of Slot 1 (`startTime + 2 * slotDur - 1`): duration is 1 us.
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime + 2 * slotDur - 1, window));
            VerifyOrQuit(window.mStartTime == startTime + 2 * slotDur - 1);
            VerifyOrQuit(window.mDuration == 1);
            VerifyWindowProperties(avail, startTime + 2 * slotDur - 1, window);

            // Start of unavailable Slot 2 (`startTime + 2 * slotDur`): next available is Slot 3 (`1 * slotDur`).
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime + 2 * slotDur, window));
            VerifyOrQuit(window.mStartTime == startTime + 3 * slotDur);
            VerifyOrQuit(window.mDuration == slotDur);
            VerifyWindowProperties(avail, startTime + 2 * slotDur, window);

            // Start of Slot 5 (`startTime + 5 * slotDur`): wraps around through Slots 5, 0, 1 (`3 * slotDur`).
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime + 5 * slotDur, window));
            VerifyOrQuit(window.mStartTime == startTime + 5 * slotDur);
            VerifyOrQuit(window.mDuration == 3 * slotDur);
            VerifyWindowProperties(avail, startTime + 5 * slotDur, window);

            // Before `startTime` at start of Slot 5 (`startTime - slotDur`): crosses `startTime` (`3 * slotDur`).
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime - slotDur, window));
            VerifyOrQuit(window.mStartTime == startTime - slotDur);
            VerifyOrQuit(window.mDuration == 3 * slotDur);
            VerifyWindowProperties(avail, startTime - slotDur, window);

            // Before `startTime` at `startTime - 1` (last us of Slot 5): `1 + 2 * slotDur`.
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime - 1, window));
            VerifyOrQuit(window.mStartTime == startTime - 1);
            VerifyOrQuit(window.mDuration == 2 * slotDur + 1);
            VerifyWindowProperties(avail, startTime - 1, window);

            // Before `startTime` in unavailable Slot 4 (`startTime - 2 * slotDur + 10`):
            // next window starts at `startTime - slotDur` with duration `3 * slotDur`.
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime - 2 * slotDur + 10, window));
            VerifyOrQuit(window.mStartTime == startTime - slotDur);
            VerifyOrQuit(window.mDuration == 3 * slotDur);
            VerifyWindowProperties(avail, startTime - 2 * slotDur + 10, window);
        }

        // 5. 7-bit pattern `0b0011000` (`0x30` with trailing 1s `0x31`):
        // Slots: 0:0, 1:0, 2:1, 3:1, 4:0, 5:0, 6:0.
        // Unavailable run wraps around from Slot 4, 5, 6 into Slot 0, 1 (5 consecutive unavailable slots).
        {
            static const uint8_t kBitmap7[] = {0b00110001};

            SuccessOrQuit(avail.Init(startTime, slotDuration, 7, kBitmap7));

            // Query in Slot 4 (`startTime + 4 * slotDur + 123`):
            // Must skip Slots 4, 5, 6, 0, 1 and land at Slot 2 of the next cycle (`startTime + 9 * slotDur`).
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime + 4 * slotDur + 123, window));
            VerifyOrQuit(window.mStartTime == startTime + 9 * slotDur);
            VerifyOrQuit(window.mDuration == 2 * slotDur);
            VerifyWindowProperties(avail, startTime + 4 * slotDur + 123, window);

            // Query before `startTime` in Slot 4 of previous cycle (`startTime - 3 * slotDur + 50`):
            // Must skip Slots 4, 5, 6 (before `startTime`) and Slots 0, 1 (after `startTime`),
            // landing at `startTime + 2 * slotDur` with duration `2 * slotDur`.
            SuccessOrQuit(avail.FindNextAvailableWindow(startTime - 3 * slotDur + 50, window));
            VerifyOrQuit(window.mStartTime == startTime + 2 * slotDur);
            VerifyOrQuit(window.mDuration == 2 * slotDur);
            VerifyWindowProperties(avail, startTime - 3 * slotDur + 50, window);

            // Step through 10 consecutive windows starting from `startTime - 10 * slotDur` by advancing
            // `queryTime = window.GetEndTime()`.
            {
                Radio::Time64 queryTime = startTime - 10 * slotDur;

                for (uint8_t iter = 0; iter < 10; iter++)
                {
                    SuccessOrQuit(avail.FindNextAvailableWindow(queryTime, window));
                    VerifyWindowProperties(avail, queryTime, window);
                    VerifyOrQuit(window.mDuration == 2 * slotDur);
                    queryTime = window.GetEndTime();
                }
            }
        }
    }

    printf("TestRadioAvailabilityFindNextAvailableWindow() passed\n");
}

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE

} // namespace ThreadDirect
} // namespace ot

int main(void)
{
#if OT_CONFIG_THREAD_DIRECT_ENABLE
    ot::ThreadDirect::TestRadioAvailabilityInitAndUnspecified();
    ot::ThreadDirect::TestRadioAvailabilityIsAvailableAt();
    ot::ThreadDirect::TestRadioAvailabilityIsAvailableIn();
    ot::ThreadDirect::TestRadioAvailabilityFindNextAvailableWindow();

    printf("All tests passed\n");
#else
    printf("Thread Direct is not enabled\n");
#endif

    return 0;
}
