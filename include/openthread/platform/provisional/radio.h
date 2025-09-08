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
 * @brief
 *   This file defines the provisional radio interface for OpenThread.
 */

#ifndef OPENTHREAD_PLATFORM_PROVISIONAL_RADIO_H_
#define OPENTHREAD_PLATFORM_PROVISIONAL_RADIO_H_

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-provisional-radio
 *
 * @brief
 *   This module includes the provisional platform abstraction for radio communication.
 *
 * @{
 */

enum
{
    OT_RADIO_SLOT_TIME = 1250, ///< Radio slot duration time in unit of microseconds.
};

/**
 * Defines constants about radio slot type.
 */
typedef enum
{
    OT_SLOT_TYPE_NOT_ALLOWED = 0, ///< The slot will be occupied by other radios, and Thread is not allowed to use it.
    OT_SLOT_TYPE_MOSTLY_NOT_ALLOWED =
        1, ///< The slot is likely to be occupied by other radios, and Thread should try to not use it.
    OT_SLOT_TYPE_MAYBE_ALLOWED = 2, ///< The slot has a low probability of being occupied by other radios, and
                                    ///< Thread can use it when Thread can't find free slots.
    OT_SLOT_TYPE_ALLOWED = 3,       ///< The slot is free, and Thread can use these slots directly.
} otSlotType;

/**
 * @struct otSlotEntry
 *
 * Represents the radio slot entry.
 */
OT_TOOL_PACKED_BEGIN
struct otSlotEntry
{
    otSlotType mSlotType : 2; ///< The radio slot type.
    uint8_t    mNumSlots : 6; ///< The number of slots with the same radio type.
} OT_TOOL_PACKED_END;

/**
 * Represents the radio slot entry.
 */
typedef struct otSlotEntry otSlotEntry;

/**
 * Notifies the OpenThread that the radio availability map has updated.
 *
 * When Thread uses the same radio chip with BT or 2.4GHz WiFi, the radio driver has the ability to know when BT or
 * WiFi will occupy the radio. To reduce the interference between Thread and BT or WiFi, the radio driver calls this
 * method to notify OpenThread when the radio is available for Thread. OpenThread will try its best to avoid using the
 * not allowed radio slots.
 *
 * @note: The radio availability map is a periodic map.
 *
 * @param[in]  aInstance    The OpenThread instance structure.
 * @param[in]  aTimestamp   The time of the local radio clock in microseconds when the radio availability map starts.
 * @param[in]  aSlotEntries A pointer to radio slot entries.
 * @param[in]  aNumEntries  The number of entries pointed by the @p aSlotEntries. Value 0 indicates that the radio is
 *                          always available for Thread.
 */
extern void otPlatRadioAvailMapUpdated(otInstance        *aInstance,
                                       uint64_t           aTimestamp,
                                       const otSlotEntry *aSlotEntries,
                                       uint8_t            aNumEntries);

/**
 * @}
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_PROVISIONAL_RADIO_H_
