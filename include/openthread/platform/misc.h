/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file includes platform abstractions for miscellaneous behaviors.
 */

#ifndef OT_PLATFORM_MISC_H
#define OT_PLATFORM_MISC_H 1

#include <stdint.h>

#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-misc
 *
 * @brief
 *   This module includes platform abstractions for miscellaneous behaviors.
 *
 * @{
 *
 */

/**
 * This function performs a software reset on the platform, if supported.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 */
void otPlatReset(otInstance *aInstance);

/**
 * Enumeration of possible reset reason codes.
 *
 * These are in the same order as the Spinel reset reason codes.
 *
 */
typedef enum
{
    OT_PLAT_RESET_REASON_POWER_ON       = 0,
    OT_PLAT_RESET_REASON_EXTERNAL       = 1,
    OT_PLAT_RESET_REASON_SOFTWARE       = 2,
    OT_PLAT_RESET_REASON_FAULT          = 3,
    OT_PLAT_RESET_REASON_CRASH          = 4,
    OT_PLAT_RESET_REASON_ASSERT         = 5,
    OT_PLAT_RESET_REASON_OTHER          = 6,
    OT_PLAT_RESET_REASON_UNKNOWN        = 7,
    OT_PLAT_RESET_REASON_WATCHDOG       = 8,

    OT_PLAT_RESET_REASON_COUNT,
} otPlatResetReason;

/**
 * This function returns the reason for the last platform reset.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 */
otPlatResetReason otPlatGetResetReason(otInstance *aInstance);

/**
 * This function provides a platform specific implementation for assert.
 *
 * @param[in] aFilename    The name of the file where the assert occurred.
 * @param[in] aLineNumber  The line number in the file where the assert occurred.
 *
 */
void otPlatAssertFail(const char *aFilename, int aLineNumber);

/**
 * This function performs a platform specific operation to wake the host MCU.
 * This is used only for NCP configurations.
 *
 */
void otPlatWakeHost(void);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OT_PLATFORM_MISC_H
