/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file includes the SoftDevice platform-specific functions.
 *
 */

#ifndef PLATFORM_SOFTDEVICE_H_
#define PLATFORM_SOFTDEVICE_H_

#include <stdint.h>

/** @brief RAAL Softdevice default parameters. */
#define PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_LENGTH       6400
#define PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_ALLOC_ITERS  5
#define PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_SAFE_MARGIN  126
#define PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_TIMEOUT      6400
#define PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_MAX_LENGTH   120000000
#define PLATFORM_SOFTDEVICE_RAAL_DEFAULT_LF_CLK_ACCURACY_PPM   500

/** @brief RAAL Softdevice configuration parameters. */
typedef struct
{
    uint32_t timeslotLength;     /**< Timeslot length requested by the module in microseconds. */
    uint32_t timeslotTimeout;    /**< Longest acceptable delay until the start of the requested timeslot in microseconds. */
    uint32_t timeslotMaxLength;  /**< Maximum single timeslot length created by extension processing in microseconds. */
    uint16_t timeslotAllocIters; /**< Maximum number of iteration of dividing timeslot_length by factor of 2 performed by arbiter. */
    uint16_t timeslotSafeMargin; /**< Safe margin before timeslot is finished and nrf_raal_timeslot_ended should be called in microseconds. */
    uint16_t lfClkAccuracyPpm;   /**< Clock accuracy in ppm unit. */
} otSysSoftdeviceRaalConfigParams;

/**
 * Function for processing SoftDevice SoC events.
 *
 */
void otSysSoftdeviceSocEvtHandler(uint32_t aEvtId);

/**
 * Function used to set non-default parameters of Softdevice RAAL.
 *
 */
void otSysSoftdeviceRaalConfig(const otSysSoftdeviceRaalConfigParams *aConfig);

#endif  // PLATFORM_SOFTDEVICE_H_
