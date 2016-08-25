/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file includes platform abstractions for miscelaneous behaviors.
 */

#ifndef OT_PLATFORM_MISC_H
#define OT_PLATFORM_MISC_H 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
    kPlatResetReason_PowerOn        = 0,
    kPlatResetReason_External       = 1,
    kPlatResetReason_Software       = 2,
    kPlatResetReason_Fault          = 3,
    kPlatResetReason_Crash          = 4,
    kPlatResetReason_Assert         = 5,
    kPlatResetReason_Other          = 6,
    kPlatResetReason_Unknown        = 7,
    kPlatResetReason_Watchdog       = 8,

    kPlatResetReason_Count,
} otPlatResetReason;

/**
 * This function returns the reason for the last platform reset.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 */
otPlatResetReason otPlatGetResetReason(otInstance *aInstance);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OT_PLATFORM_MISC_H
