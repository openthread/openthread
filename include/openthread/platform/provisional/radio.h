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
 *   This file defines the radio interface for OpenThread.
 */

#ifndef OPENTHREAD_PLATFORM_PROVISIONAL_RADIO_H_
#define OPENTHREAD_PLATFORM_PROVISIONAL_RADIO_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-radio
 *
 * @brief
 *   This module includes the platform abstraction for radio communication.
 *
 * @{
 */
enum
{
    OT_RADIO_ECSL_SLOT_SIZE = 1250, ///< The size of the enhanced CSL slot, in unit of microseconds.
};

/**
 * Set the enhanced CSL period.
 *
 * @param[in]  aInstance     The OpenThread instance structure.
 * @param[in]  aCslPeriod    Enhanced CSL period, 0 for disabling eCSL, in unit of 1250 microseconds.
 *
 * @retval  OT_ERROR_NOT_IMPLEMENTED Radio driver doesn't support eCSL.
 * @retval  OT_ERROR_FAILED          Other platform specific errors.
 * @retval  OT_ERROR_NONE            Successfully enabled or disabled eCSL.
 */
otError otPlatRadioSetEnhancedCslPeriod(otInstance *aInstance, uint32_t aCslPeriod);

/**
 * Set enhanced CSL sample time in radio driver.
 *
 * @param[in]  aInstance         The OpenThread instance structure.
 * @param[in]  aCslSampleTime    The next sample time, in microseconds. It is
 *                               the time when the first symbol of the MHR of
 *                               the frame is expected.
 */
void otPlatRadioSetEnhancedCslSampleTime(otInstance *aInstance, uint32_t aCslSampleTime);

/**
 * Add an extended address to the enhanced CSL peer address match table.
 *
 * @note Platforms should use eCSL peer addresses to include SCA IE when generating enhanced acks.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[in]  aExtAddr    The extended address of the eCSL peer to be added stored in little-endian byte order.
 *
 * @retval OT_ERROR_NONE      Successfully added extended address to the eCSL peer address match table.
 * @retval OT_ERROR_NO_BUFS   No available entry in the eCSL peer address match table.
 */
otError otPlatRadioAddEnhancedCslPeerAddress(otInstance *aInstance, const otExtAddress *aExtAddr);

/**
 * Remove an extended address from the enhanced CSL peer address match table.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[in]  aExtAddr    The extended address of the eCSL peer to be removed.
 *
 * @retval OT_ERROR_NONE        Successfully removed extended address from the eCSL peer address match table.
 * @retval OT_ERROR_NO_ADDRESS  The extended address is not in eCSL peer address  match table.
 */
otError otPlatRadioClearEnhancedCslPeerAddress(otInstance *aInstance, const otExtAddress *aExtAddr);

/**
 * Clear all extended addresses in the enhanced CSL peer address match table.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 */
void otPlatRadioClearEnhancedCslPeerAddresses(otInstance *aInstance);

/**
 * @}
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_PROVISIONAL_RADIO_H_
