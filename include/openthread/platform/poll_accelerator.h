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
 *   This file defines the data poll accelerator interface for OpenThread.
 */

#ifndef OPENTHREAD_PLATFORM_POLL_ACCELERATOR_H_
#define OPENTHREAD_PLATFORM_POLL_ACCELERATOR_H_

#include <stdint.h>
#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-poll-accelerator
 *
 * @brief
 *   This module includes the platform abstraction for Poll Accelerator.
 *
 * @{
 *
 */

/**
 * Poll Accelerator configuration structure.
 */
typedef struct otPollAcceleratorConfig
{
    uint8_t  mCsmaMinBe;           ///< CSMA/CA minimum backoff exponent
    uint8_t  mCsmaMaxBe;           ///< CSMA/CA maximum backoff exponent
    uint32_t mStartTime;           ///< Start time in microseconds
    uint32_t mPollPeriod;          ///< Poll period in milliseconds
    uint32_t mWaitForDataDuration; ///< Wait for data duration in milliseconds
    uint32_t mMaxIterations;       ///< Maximum number of poll iterations (0 = unlimited)
} otPollAcceleratorConfig;

/**
 * Start the platform-level Poll Accelerator.
 *
 * This function initiates the hardware-accelerated polling mechanism.
 * The platform implementation should:
 * - Configure CSMA/CA parameters
 * - Set up periodic data request transmission
 * - Handle ACK reception and frame pending detection
 * - Call `otPlatPollAcceleratorDone()` when complete or interrupted
 *
 * @param[in] aInstance  A pointer to the OpenThread instance.
 * @param[in] aFrame     A pointer to the data request frame to be sent.
 * @param[in] aConfig    A pointer to the Poll Accelerator configuration.
 *
 * @retval OT_ERROR_NONE          Successfully started.
 * @retval OT_ERROR_INVALID_STATE Poll Accelerator is already running.
 *
 */
otError otPlatPollAcceleratorStart(otInstance *aInstance, otRadioFrame *aFrame, const otPollAcceleratorConfig *aConfig);

/**
 * Stop the platform-level Poll Accelerator.
 *
 * This function requests the hardware-accelerated polling mechanism to stop.
 *
 * @param[in] aInstance  A pointer to the OpenThread instance.
 *
 * @retval OT_ERROR_NONE           Request to stop was successful.
 * @retval OT_ERROR_INVALID_STATE  Poll Accelerator is not running.
 *
 */
otError otPlatPollAcceleratorStop(otInstance *aInstance);

/**
 * Platform callback for Poll Accelerator completion.
 *
 * The platform MUST invoke this callback when the Poll Accelerator operation terminates.
 *
 * Termination conditions:
 * - Maximum iterations reached
 * - Data received (ACK with FP=1 followed by data frame)
 * - Timeout (no data after ACK with FP=1)
 * - Transmission/reception error
 * - Interrupted by otPlatPollAcceleratorStop()
 *
 * @param[in] aInstance        A pointer to the OpenThread instance.
 * @param[in] aIterationsDone  Number of poll iterations completed.
 * @param[in] aPrevAckFrame    A pointer to the ACK from iteration N-1, or NULL.
 * @param[in] aTxFrame         A pointer to the last transmitted Data Request frame.
 * @param[in] aAckFrame        A pointer to the ACK from iteration N, or NULL if no ACK received.
 * @param[in] aTxError         Transmission error status (OT_ERROR_NONE on success).
 * @param[in] aRxFrame         A pointer to received data frame, or NULL if no data.
 * @param[in] aRxError         Reception error status (OT_ERROR_NONE on success).
 *
 */
extern void otPlatPollAcceleratorDone(otInstance   *aInstance,
                                      uint32_t      aIterationsDone,
                                      otRadioFrame *aPrevAckFrame,
                                      otRadioFrame *aTxFrame,
                                      otRadioFrame *aAckFrame,
                                      otError       aTxError,
                                      otRadioFrame *aRxFrame,
                                      otError       aRxError);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_POLL_ACCELERATOR_H_
