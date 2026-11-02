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
#include <openthread/thread_direct_listener.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/radio.h>

#include "test_platform.h"
#include "test_util.hpp"

#include "common/bit_utils.hpp"
#include "common/code_utils.hpp"
#include "common/num_utils.hpp"
#include "instance/instance.hpp"
#include "mac/sub_mac.hpp"
#include "thread_direct/td_listener.hpp"
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

//----------------------------------------------------------------------------------------------------------------------

#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE

static constexpr uint64_t kRadioTimeOffsets[] = {0, 1000, 1000000};

static uint64_t     sRadioTimeOffset    = 0;
static uint64_t     sNow                = 0;
static uint32_t     sMicroAlarmTime     = 0;
static bool         sMicroTimerStarted  = false;
static uint32_t     sMilliAlarmTime     = 0;
static bool         sMilliTimerStarted  = false;
static otRadioState sRadioState         = OT_RADIO_STATE_DISABLED;
static uint8_t      sChannel            = 0;
static uint32_t     sReceiveChangeCount = 0;
static uint32_t     sSleepChangeCount   = 0;

extern "C" {

uint64_t otPlatRadioGetNow(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sNow + sRadioTimeOffset;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);

    if ((sRadioState != OT_RADIO_STATE_RECEIVE) || (sChannel != aChannel))
    {
        sReceiveChangeCount++;
    }

    sRadioState = OT_RADIO_STATE_RECEIVE;
    sChannel    = aChannel;

    return OT_ERROR_NONE;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (sRadioState != OT_RADIO_STATE_SLEEP)
    {
        sSleepChangeCount++;
    }

    sRadioState = OT_RADIO_STATE_SLEEP;

    return OT_ERROR_NONE;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_RADIO_CAPS_NONE;
}

uint8_t otPlatRadioGetCslAccuracy(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return 0;
}

uint8_t otPlatRadioGetCslUncertainty(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return 0;
}

void otPlatAlarmMicroStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    sMicroTimerStarted = true;
    sMicroAlarmTime    = aT0 + aDt;
}

void otPlatAlarmMicroStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sMicroTimerStarted = false;
}

uint32_t otPlatAlarmMicroGetNow(void) { return static_cast<uint32_t>(sNow); }

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    sMilliTimerStarted = true;
    sMilliAlarmTime    = aT0 + aDt;
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sMilliTimerStarted = false;
}

uint32_t otPlatAlarmMilliGetNow(void) { return static_cast<uint32_t>(sNow / 1000); }

} // extern "C"

static void ResetPlatformState(uint64_t aRadioTimeOffset = 0)
{
    sRadioTimeOffset    = aRadioTimeOffset;
    sNow                = 0;
    sMicroAlarmTime     = 0;
    sMicroTimerStarted  = false;
    sMilliAlarmTime     = 0;
    sMilliTimerStarted  = false;
    sRadioState         = OT_RADIO_STATE_DISABLED;
    sChannel            = 0;
    sReceiveChangeCount = 0;
    sSleepChangeCount   = 0;
}

static void AdvanceTime(Instance &aInstance, uint32_t aDurationUs)
{
    uint64_t endTime = sNow + aDurationUs;

    while (sNow < endTime)
    {
        sNow++;

        while (sMicroTimerStarted && (static_cast<int32_t>(static_cast<uint32_t>(sNow) - sMicroAlarmTime) >= 0))
        {
            sMicroTimerStarted = false;
            otPlatAlarmMicroFired(&aInstance);
        }

        if ((sNow % 1000) == 0)
        {
            while (sMilliTimerStarted &&
                   (static_cast<int32_t>(static_cast<uint32_t>(sNow / 1000) - sMilliAlarmTime) >= 0))
            {
                sMilliTimerStarted = false;
                otPlatAlarmMilliFired(&aInstance);
            }
        }

        while (otTaskletsArePending(&aInstance))
        {
            otTaskletsProcess(&aInstance);
        }
    }
}

static void AdvanceTo(Instance &aInstance, uint64_t aTargetTimeUs)
{
    VerifyOrQuit(aTargetTimeUs >= sNow);
    AdvanceTime(aInstance, static_cast<uint32_t>(aTargetTimeUs - sNow));
}

void TestListenerConfigAndApi(void)
{
    Instance          *instance;
    Listener          *listener;
    Listener::Config   config;
    Listener::Config   invalidConfig;
    Listener::WakeInfo wakeInfo;

    ResetPlatformState();

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr);

    listener = &instance->Get<Listener>();

    // Verify `Config` defaults when cleared (`mChannel == 0`, `mInterval == 0`, `mDuration == 0`).
    config.Clear();
    VerifyOrQuit(config.GetChannel() == Listener::Config::kDefaultChannel);
    VerifyOrQuit(config.GetInterval() == Listener::Config::kDefaultInterval);
    VerifyOrQuit(config.GetDuration() == Listener::Config::kDefaultDuration);
    VerifyOrQuit(config.IsValid());

    // Verify `Config` getters with explicit values.
    config.mChannel  = 15;
    config.mInterval = 50000;
    config.mDuration = 4000;
    VerifyOrQuit(config.GetChannel() == 15);
    VerifyOrQuit(config.GetInterval() == 50000);
    VerifyOrQuit(config.GetDuration() == 4000);
    VerifyOrQuit(config.IsValid());

    // Verify valid channel boundaries (11 and 26).
    config.mChannel = 11;
    VerifyOrQuit(config.IsValid());
    config.mChannel = 26;
    VerifyOrQuit(config.IsValid());

    // Verify invalid channels (below 11 or above 26).
    invalidConfig          = config;
    invalidConfig.mChannel = 10;
    VerifyOrQuit(!invalidConfig.IsValid());
    VerifyOrQuit(listener->Start(invalidConfig) == kErrorInvalidArgs);

    invalidConfig.mChannel = 27;
    VerifyOrQuit(!invalidConfig.IsValid());
    VerifyOrQuit(listener->Start(invalidConfig) == kErrorInvalidArgs);

    // Verify interval vs duration validation (`GetInterval() >= GetDuration()`).
    config.Clear();
    config.mChannel  = 15;
    config.mDuration = 8000;
    config.mInterval = 8000;
    VerifyOrQuit(config.IsValid());

    invalidConfig           = config;
    invalidConfig.mInterval = 7999;
    VerifyOrQuit(!invalidConfig.IsValid());
    VerifyOrQuit(listener->Start(invalidConfig) == kErrorInvalidArgs);

    // Explicit interval smaller than default duration (`kDefaultDuration` is 8000).
    invalidConfig.Clear();
    invalidConfig.mInterval = Listener::Config::kDefaultDuration - 1;
    VerifyOrQuit(!invalidConfig.IsValid());
    VerifyOrQuit(listener->Start(invalidConfig) == kErrorInvalidArgs);

    // Explicit duration larger than default interval (`kDefaultInterval` is 1000000).
    invalidConfig.Clear();
    invalidConfig.mDuration = Listener::Config::kDefaultInterval + 1;
    VerifyOrQuit(!invalidConfig.IsValid());
    VerifyOrQuit(listener->Start(invalidConfig) == kErrorInvalidArgs);

    // Verify `TimerMicro::kMaxDelay` boundary on interval.
    config.Clear();
    config.mInterval = TimerMicro::kMaxDelay;
    VerifyOrQuit(config.IsValid());

    invalidConfig.Clear();
    invalidConfig.mInterval = TimerMicro::kMaxDelay + 1;
    VerifyOrQuit(!invalidConfig.IsValid());
    VerifyOrQuit(listener->Start(invalidConfig) == kErrorInvalidArgs);

    // Verify initial stopped state and public C API behavior.
    VerifyOrQuit(!listener->IsRunning());
    VerifyOrQuit(!otThreadDirectListenerIsRunning(instance));
    VerifyOrQuit(listener->GetWakeInfo(wakeInfo) == kErrorInvalidState);
    VerifyOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo) == OT_ERROR_INVALID_STATE);

    // Calling `Stop()` when already stopped is a safe no-op.
    otThreadDirectListenerStop(instance);
    VerifyOrQuit(!otThreadDirectListenerIsRunning(instance));

    // Start listener with valid configuration.
    config.Clear();
    config.mChannel  = 18;
    config.mInterval = 20000;
    config.mDuration = 2000;
    SuccessOrQuit(otThreadDirectListenerStart(instance, &config));
    VerifyOrQuit(listener->IsRunning());
    VerifyOrQuit(otThreadDirectListenerIsRunning(instance));

    // Starting again while already running must return `OT_ERROR_BUSY`.
    VerifyOrQuit(otThreadDirectListenerStart(instance, &config) == OT_ERROR_BUSY);
    VerifyOrQuit(otThreadDirectListenerStart(instance, &invalidConfig) == OT_ERROR_BUSY);

    // Querying `GetWakeInfo()` while running with no wake received returns `OT_ERROR_NOT_FOUND`.
    VerifyOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo) == OT_ERROR_NOT_FOUND);

    // Stop listener and verify state transitions back to stopped.
    otThreadDirectListenerStop(instance);
    VerifyOrQuit(!otThreadDirectListenerIsRunning(instance));
    VerifyOrQuit(otThreadDirectListenerGetWakeInfo(instance, &wakeInfo) == OT_ERROR_INVALID_STATE);

    // Restart with all-zero default config.
    config.Clear();
    SuccessOrQuit(otThreadDirectListenerStart(instance, &config));
    VerifyOrQuit(otThreadDirectListenerIsRunning(instance));

    otThreadDirectListenerStop(instance);
    VerifyOrQuit(!otThreadDirectListenerIsRunning(instance));

    testFreeInstance(instance);

    printf("TestListenerConfigAndApi() passed\n");
}

void TestListenerPeriodicWindows(void)
{
    static constexpr uint8_t  kChannel  = 17;
    static constexpr uint32_t kDuration = 1500;
    static constexpr uint32_t kInterval = 6000;
    static constexpr uint32_t kPeriods  = 4;

    for (uint64_t timeOffset : kRadioTimeOffsets)
    {
        Instance        *instance;
        Mac::SubMac     *subMac;
        Listener        *listener;
        Listener::Config config;

        ResetPlatformState(timeOffset);

        instance = static_cast<Instance *>(testInitInstance());
        VerifyOrQuit(instance != nullptr);

        subMac   = &instance->Get<Mac::SubMac>();
        listener = &instance->Get<Listener>();

        SuccessOrQuit(subMac->Enable());
        SuccessOrQuit(subMac->Sleep());
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        sReceiveChangeCount = 0;
        sSleepChangeCount   = 0;

        AdvanceTime(*instance, 1234);

        // 1. Start listener and verify periodic receive and sleep windows across multiple intervals.
        config.Clear();
        config.mChannel  = kChannel;
        config.mDuration = kDuration;
        config.mInterval = kInterval;

        SuccessOrQuit(listener->Start(config));

        // First receive window starts immediately at `now`.
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kChannel);
        VerifyOrQuit(sReceiveChangeCount == 1);
        VerifyOrQuit(sSleepChangeCount == 0);

        for (uint32_t period = 0; period < kPeriods; period++)
        {
            // Stay in receive for `kDuration - 1` microseconds.
            AdvanceTime(*instance, kDuration - 1);
            VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
            VerifyOrQuit(sChannel == kChannel);
            VerifyOrQuit(sReceiveChangeCount == period + 1);
            VerifyOrQuit(sSleepChangeCount == period);

            // At `kDuration`, the window ends and the radio transitions to sleep.
            AdvanceTime(*instance, 1);
            VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);
            VerifyOrQuit(sReceiveChangeCount == period + 1);
            VerifyOrQuit(sSleepChangeCount == period + 1);

            // Stay in sleep for `(kInterval - kDuration) - 1` microseconds.
            AdvanceTime(*instance, (kInterval - kDuration) - 1);
            VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);
            VerifyOrQuit(sReceiveChangeCount == period + 1);
            VerifyOrQuit(sSleepChangeCount == period + 1);

            // At `kInterval`, the next periodic window starts on `kChannel`.
            AdvanceTime(*instance, 1);
            VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
            VerifyOrQuit(sChannel == kChannel);
            VerifyOrQuit(sReceiveChangeCount == period + 2);
            VerifyOrQuit(sSleepChangeCount == period + 1);
        }

        // 2. Stop listener while in the middle of an active receive window.
        AdvanceTime(*instance, 500);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);

        listener->Stop();
        VerifyOrQuit(!listener->IsRunning());

        // The already-active timed receive window finishes its remaining duration (`kDuration - 500`)
        // and then sleeps, with no further periodic windows scheduled afterward.
        AdvanceTime(*instance, (kDuration - 500) - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);

        AdvanceTime(*instance, 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);
        VerifyOrQuit(sReceiveChangeCount == kPeriods + 1);
        VerifyOrQuit(sSleepChangeCount == kPeriods + 1);

        AdvanceTime(*instance, kInterval * 3);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);
        VerifyOrQuit(sReceiveChangeCount == kPeriods + 1);
        VerifyOrQuit(sSleepChangeCount == kPeriods + 1);

        // 3. Restart with a different configuration and stop during the sleep phase between windows.
        sReceiveChangeCount = 0;
        sSleepChangeCount   = 0;

        config.Clear();
        config.mChannel  = 22;
        config.mDuration = 1000;
        config.mInterval = 5000;

        SuccessOrQuit(listener->Start(config));
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == 22);
        VerifyOrQuit(sReceiveChangeCount == 1);

        AdvanceTime(*instance, 1000);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);
        VerifyOrQuit(sSleepChangeCount == 1);

        // Advance partway through the sleep phase and stop the listener.
        AdvanceTime(*instance, 1500);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        listener->Stop();
        VerifyOrQuit(!listener->IsRunning());

        // Verify no further windows start across multiple intervals.
        AdvanceTime(*instance, 15000);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);
        VerifyOrQuit(sReceiveChangeCount == 1);
        VerifyOrQuit(sSleepChangeCount == 1);

        // 4. Start with default configuration (`mChannel = 0`, `mDuration = 0`, `mInterval = 0`).
        sReceiveChangeCount = 0;
        sSleepChangeCount   = 0;

        config.Clear();
        SuccessOrQuit(listener->Start(config));
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == Listener::Config::kDefaultChannel);
        VerifyOrQuit(sReceiveChangeCount == 1);

        AdvanceTime(*instance, Listener::Config::kDefaultDuration - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == Listener::Config::kDefaultChannel);

        AdvanceTime(*instance, 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);
        VerifyOrQuit(sSleepChangeCount == 1);

        AdvanceTime(*instance, (Listener::Config::kDefaultInterval - Listener::Config::kDefaultDuration) - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);
        VerifyOrQuit(sReceiveChangeCount == 1);

        AdvanceTime(*instance, 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == Listener::Config::kDefaultChannel);
        VerifyOrQuit(sReceiveChangeCount == 2);

        listener->Stop();

        // 5. Start with `mDuration == mInterval` (100% duty cycle) and verify back-to-back receive windows.
        AdvanceTime(*instance, Listener::Config::kDefaultDuration);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        config.Clear();
        config.mChannel  = 19;
        config.mDuration = 4000;
        config.mInterval = 4000;
        SuccessOrQuit(listener->Start(config));
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == 19);

        for (uint32_t period = 0; period < kPeriods; period++)
        {
            AdvanceTime(*instance, 4000 - 1);
            VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
            VerifyOrQuit(sChannel == 19);

            AdvanceTime(*instance, 1);
            VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
            VerifyOrQuit(sChannel == 19);
        }

        listener->Stop();
        AdvanceTime(*instance, 4000);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        // 6. Toggle `subMac->Disable()` and `subMac->Enable()` across multiple elapsed intervals while listener runs.
        config.Clear();
        config.mChannel  = 21;
        config.mDuration = 1000;
        config.mInterval = 5000;
        SuccessOrQuit(listener->Start(config));
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == 21);

        // Finish first window `[T0, T0 + 1000)` and advance 1000 us into sleep (`T0 + 2000`).
        AdvanceTime(*instance, 2000);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        // Disable `SubMac` and advance 11000 us (`T0 + 13000`), skipping windows at `T0 + 5000` and `T0 + 10000`.
        SuccessOrQuit(subMac->Disable());
        sReceiveChangeCount = 0;

        AdvanceTime(*instance, 11000);
        VerifyOrQuit(sReceiveChangeCount == 0);

        // Re-enable `SubMac` at `T0 + 13000`; next window `[T0 + 15000, T0 + 16000)` is 2000 us away.
        SuccessOrQuit(subMac->Enable());
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);
        VerifyOrQuit(sReceiveChangeCount == 0);

        AdvanceTime(*instance, 2000 - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        AdvanceTime(*instance, 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == 21);
        VerifyOrQuit(sReceiveChangeCount == 1);

        AdvanceTime(*instance, 1000 - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);

        AdvanceTime(*instance, 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        listener->Stop();

        testFreeInstance(instance);
    }

    printf("TestListenerPeriodicWindows() passed\n");
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

void TestListenerAndCslScheduling(void)
{
    static constexpr uint8_t  kCslChannel          = 15;
    static constexpr uint8_t  kTdChannel           = 20;
    static constexpr uint16_t kCslPeriodTenSymbols = 250;
    static constexpr uint32_t kCslPeriodUs         = Mac::CslPeriodToUsec(kCslPeriodTenSymbols); // 40000 us
    static constexpr uint32_t kCslAhead            = OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD;
    static constexpr uint32_t kCslAfter            = OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER;
    static constexpr uint32_t kCslDuration         = kCslAhead + kCslAfter;
    static constexpr uint64_t kCslSyncTime         = 10000;

    for (uint64_t timeOffset : kRadioTimeOffsets)
    {
        Instance        *instance;
        Mac::SubMac     *subMac;
        Listener        *listener;
        Mac::CslAccuracy cslAccuracy;
        Mac::ExtAddress  extAddr;
        Listener::Config config;
        uint64_t         cslStart;
        uint64_t         cslEnd;
        uint64_t         tdStart;
        uint64_t         tdEnd;
        uint32_t         rxCountBefore;

        ResetPlatformState(timeOffset);

        instance = static_cast<Instance *>(testInitInstance());
        VerifyOrQuit(instance != nullptr);

        subMac   = &instance->Get<Mac::SubMac>();
        listener = &instance->Get<Listener>();

        SuccessOrQuit(subMac->Enable());
        SuccessOrQuit(subMac->Sleep());

        // Set parent clock accuracy and uncertainty to 0 so each CSL window around sample time
        // `T_k = kCslSyncTime + k * kCslPeriodUs` is `[T_k - kCslAhead, T_k + kCslAfter)`.
        cslAccuracy.SetClockAccuracy(0);
        cslAccuracy.SetUncertainty(0);
        subMac->SetCslParentAccuracy(cslAccuracy);
        extAddr.Clear();

        AdvanceTo(*instance, kCslSyncTime);
        subMac->SetCslParams(kCslPeriodTenSymbols, kCslChannel, Mac::kShortAddrBroadcast, extAddr);

        // CSL Window 0 (`[kCslSyncTime - kCslAhead, kCslSyncTime + kCslAfter)`) is immediately active.
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);

        AdvanceTo(*instance, kCslSyncTime + kCslAfter - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);

        AdvanceTo(*instance, kCslSyncTime + kCslAfter);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        //--------------------------------------------------------------------------------------------------------------
        // Scenario 1: Non-overlapping TD and CSL windows (TD window before CSL Window 1, and TD window after it).
        // CSL Window 1 is `[cslStart, cslEnd)` around `kCslSyncTime + 1 * kCslPeriodUs` (50000 us).

        cslStart = kCslSyncTime + 1 * kCslPeriodUs - kCslAhead;
        cslEnd   = cslStart + kCslDuration;

        tdStart = 20000;
        tdEnd   = tdStart + 4000;

        AdvanceTo(*instance, tdStart);

        config.Clear();
        config.mChannel  = kTdChannel;
        config.mDuration = 4000;
        config.mInterval = 40000;
        SuccessOrQuit(listener->Start(config));

        // TD window `[20000, 24000)` is active on `kTdChannel`.
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        AdvanceTo(*instance, tdEnd - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        AdvanceTo(*instance, tdEnd);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        // CSL Window 1 `[cslStart, cslEnd)` runs on `kCslChannel`.
        AdvanceTo(*instance, cslStart - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        AdvanceTo(*instance, cslStart);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);

        AdvanceTo(*instance, cslEnd - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);

        AdvanceTo(*instance, cslEnd);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        // Second TD window `[60000, 64000)` runs after CSL Window 1 on `kTdChannel`.
        tdStart = 60000;
        tdEnd   = tdStart + 4000;

        AdvanceTo(*instance, tdStart - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        AdvanceTo(*instance, tdStart);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        AdvanceTo(*instance, tdEnd - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        AdvanceTo(*instance, tdEnd);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        listener->Stop();

        //--------------------------------------------------------------------------------------------------------------
        // Scenario 2: TD window starts before CSL Window 2 and ends inside it (CSL preempts TD at `cslStart`).
        // CSL Window 2 is around `kCslSyncTime + 2 * kCslPeriodUs` (90000 us).

        cslStart = kCslSyncTime + 2 * kCslPeriodUs - kCslAhead;
        cslEnd   = cslStart + kCslDuration;

        tdStart = cslStart - 2000;
        tdEnd   = cslStart + 2000;

        AdvanceTo(*instance, tdStart);

        config.Clear();
        config.mChannel  = kTdChannel;
        config.mDuration = static_cast<uint32_t>(tdEnd - tdStart);
        config.mInterval = 200000;
        SuccessOrQuit(listener->Start(config));

        // Radio receives on `kTdChannel` during `[tdStart, cslStart)`.
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        AdvanceTo(*instance, cslStart - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        // At `cslStart`, higher-priority CSL preempts TD and switches the radio to `kCslChannel`.
        AdvanceTo(*instance, cslStart);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);

        // Radio remains on `kCslChannel` past `tdEnd` until `cslEnd`.
        AdvanceTo(*instance, tdEnd);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);

        AdvanceTo(*instance, cslEnd - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);

        AdvanceTo(*instance, cslEnd);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        listener->Stop();

        //--------------------------------------------------------------------------------------------------------------
        // Scenario 3: TD window starts before CSL Window 3 and ends after it (TD runs, CSL preempts, then TD resumes).
        // CSL Window 3 is around `kCslSyncTime + 3 * kCslPeriodUs` (130000 us).

        cslStart = kCslSyncTime + 3 * kCslPeriodUs - kCslAhead;
        cslEnd   = cslStart + kCslDuration;

        tdStart = cslStart - 2000;
        tdEnd   = cslEnd + 2500;

        AdvanceTo(*instance, tdStart);

        config.Clear();
        config.mChannel  = kTdChannel;
        config.mDuration = static_cast<uint32_t>(tdEnd - tdStart);
        config.mInterval = 200000;
        SuccessOrQuit(listener->Start(config));

        // Phase 1 (`[tdStart, cslStart)`): radio receives on `kTdChannel`.
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        AdvanceTo(*instance, cslStart - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        // Phase 2 (`[cslStart, cslEnd)`): CSL preempts TD on `kCslChannel`.
        AdvanceTo(*instance, cslStart);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);

        AdvanceTo(*instance, cslEnd - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);

        // Phase 3 (`[cslEnd, tdEnd)`): CSL ends and TD resumes on `kTdChannel` for its remaining tail.
        AdvanceTo(*instance, cslEnd);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        AdvanceTo(*instance, tdEnd - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        AdvanceTo(*instance, tdEnd);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        listener->Stop();

        //--------------------------------------------------------------------------------------------------------------
        // Scenario 4: CSL Window 4 starts before TD window, and TD window ends after CSL Window 4
        // (CSL blocks TD start; TD runs its remaining tail after CSL ends).
        // CSL Window 4 is around `kCslSyncTime + 4 * kCslPeriodUs` (170000 us).
        // Start TD one interval (20000 us) before `cslStart + 2000` so its second window lands at
        // `[cslStart + 2000, cslEnd + 3000)`.

        cslStart = kCslSyncTime + 4 * kCslPeriodUs - kCslAhead;
        cslEnd   = cslStart + kCslDuration;

        tdStart = (cslStart + 2000) - 20000;
        tdEnd   = cslEnd + 3000;

        AdvanceTo(*instance, tdStart);

        config.Clear();
        config.mChannel  = kTdChannel;
        config.mDuration = (kCslDuration - 2000) + 3000;
        config.mInterval = 20000;
        SuccessOrQuit(listener->Start(config));

        // First TD window `[tdStart, tdStart + config.mDuration)` runs before CSL Window 4.
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        AdvanceTo(*instance, tdStart + config.mDuration);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        // Second TD window is `[cslStart + 2000, cslEnd + 3000)`.
        // CSL Window 4 starts at `cslStart` on `kCslChannel` and is not interrupted at `cslStart + 2000`.
        AdvanceTo(*instance, cslStart - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        AdvanceTo(*instance, cslStart);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);
        rxCountBefore = sReceiveChangeCount;

        AdvanceTo(*instance, cslStart + 2000);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);
        VerifyOrQuit(sReceiveChangeCount == rxCountBefore);

        AdvanceTo(*instance, cslEnd - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);
        VerifyOrQuit(sReceiveChangeCount == rxCountBefore);

        // At `cslEnd`, CSL finishes and TD runs its remaining tail `[cslEnd, tdEnd)` on `kTdChannel`.
        AdvanceTo(*instance, cslEnd);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        AdvanceTo(*instance, tdEnd - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        AdvanceTo(*instance, tdEnd);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        listener->Stop();

        //--------------------------------------------------------------------------------------------------------------
        // Scenario 5: CSL Window 5 completely encloses TD window (CSL eclipses TD completely).
        // CSL Window 5 is around `kCslSyncTime + 5 * kCslPeriodUs` (210000 us).
        // Start TD one interval (20000 us) before `cslStart + 1000` with duration 2000 us so its second window
        // `[cslStart + 1000, cslStart + 3000)` lies strictly inside `[cslStart, cslEnd)`.

        cslStart = kCslSyncTime + 5 * kCslPeriodUs - kCslAhead;
        cslEnd   = cslStart + kCslDuration;

        tdStart = (cslStart + 1000) - 20000;

        AdvanceTo(*instance, tdStart);

        config.Clear();
        config.mChannel  = kTdChannel;
        config.mDuration = 2000;
        config.mInterval = 20000;
        SuccessOrQuit(listener->Start(config));

        AdvanceTo(*instance, tdStart + config.mDuration);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        AdvanceTo(*instance, cslStart);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);
        rxCountBefore = sReceiveChangeCount;

        // Throughout `[cslStart, cslEnd)`, the radio remains on `kCslChannel` without switching to `kTdChannel`.
        AdvanceTo(*instance, cslStart + 1000);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);
        VerifyOrQuit(sReceiveChangeCount == rxCountBefore);

        AdvanceTo(*instance, cslStart + 3000);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);
        VerifyOrQuit(sReceiveChangeCount == rxCountBefore);

        AdvanceTo(*instance, cslEnd - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);
        VerifyOrQuit(sReceiveChangeCount == rxCountBefore);

        // At `cslEnd`, both CSL and the enclosed TD window have ended, so the radio sleeps.
        AdvanceTo(*instance, cslEnd);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        listener->Stop();

        //--------------------------------------------------------------------------------------------------------------
        // Scenario 6: TD window and CSL Window 6 start at the exact same timestamp `cslStart`
        // (priority tie-breaking selects CSL first, then TD runs its remaining tail after `cslEnd`).
        // CSL Window 6 is around `kCslSyncTime + 6 * kCslPeriodUs` (250000 us).

        cslStart = kCslSyncTime + 6 * kCslPeriodUs - kCslAhead;
        cslEnd   = cslStart + kCslDuration;

        tdStart = cslStart - 20000;
        tdEnd   = cslEnd + 2000;

        AdvanceTo(*instance, tdStart);

        config.Clear();
        config.mChannel  = kTdChannel;
        config.mDuration = kCslDuration + 2000;
        config.mInterval = 20000;
        SuccessOrQuit(listener->Start(config));

        AdvanceTo(*instance, tdStart + config.mDuration);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        // Both CSL Window 6 and the second TD window start at `cslStart`. Higher-priority CSL wins.
        AdvanceTo(*instance, cslStart - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        AdvanceTo(*instance, cslStart);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);

        AdvanceTo(*instance, cslEnd - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kCslChannel);

        // At `cslEnd`, CSL ends and TD runs its remaining 2000 us tail `[cslEnd, tdEnd)` on `kTdChannel`.
        AdvanceTo(*instance, cslEnd);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        AdvanceTo(*instance, tdEnd - 1);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_RECEIVE);
        VerifyOrQuit(sChannel == kTdChannel);

        AdvanceTo(*instance, tdEnd);
        VerifyOrQuit(sRadioState == OT_RADIO_STATE_SLEEP);

        listener->Stop();
        subMac->SetCslParams(0, kCslChannel, Mac::kShortAddrBroadcast, extAddr);

        testFreeInstance(instance);
    }

    printf("TestListenerAndCslScheduling() passed\n");
}

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#endif // OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE

} // namespace ThreadDirect
} // namespace ot

int main(void)
{
#if OT_CONFIG_THREAD_DIRECT_ENABLE
    ot::ThreadDirect::TestRadioAvailabilityInitAndUnspecified();
    ot::ThreadDirect::TestRadioAvailabilityIsAvailableAt();
    ot::ThreadDirect::TestRadioAvailabilityIsAvailableIn();
    ot::ThreadDirect::TestRadioAvailabilityFindNextAvailableWindow();

#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
    ot::ThreadDirect::TestListenerConfigAndApi();
    ot::ThreadDirect::TestListenerPeriodicWindows();
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    ot::ThreadDirect::TestListenerAndCslScheduling();
#endif
#endif

    printf("All tests passed\n");
#else
    printf("Thread Direct is not enabled\n");
#endif

    return 0;
}
