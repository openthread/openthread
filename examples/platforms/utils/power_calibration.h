/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file provides utils functions to simplify the implementation of power calibration interfaces.
 *
 */

#ifndef UTILS_POWER_CALIBRATION_H
#define UTILS_POWER_CALIBRATION_H

#include "openthread-core-config.h"

#include <openthread/error.h>
#include <openthread/platform/radio.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def PLATFORM_UTILS_CONFIG_NUM_CALIBRATED_POWERS
 *
 * The number of calibrated powers in the power calibration table.
 *
 */
#ifndef PLATFORM_UTILS_CONFIG_NUM_CALIBRATED_POWERS
#define PLATFORM_UTILS_CONFIG_NUM_CALIBRATED_POWERS 6
#endif

/**
 * Get the raw power setting for the given channel.
 *
 * Platform radio layer should parse the raw power setting based on the radio layer defined format and set the
 * parameters of each radio hardware module.
 *
 * @param[in]   aChannel          The radio channel.
 * @param[out]  aRawPowerSetting  A pointer to the raw power setting.
 *
 * @retval  OT_ERROR_NONE          Successfully got the target power.
 * @retval  OT_ERROR_INVALID_ARGS  The @p aChannel is invalid or @p aRawPowerSetting is NULL.
 * @retval  OT_ERROR_NOT_FOUND     The raw power setting for the @p aChannel was not found.
 *
 */
otError otUtilsPowerCalibrationGetRawPowerSetting(uint8_t aChannel, otRawPowerSetting *aRawPowerSetting);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UTILS_POWER_CALIBRATION_H
