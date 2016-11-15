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
 *   This file includes the OenThread API for jam detection feature.
 */

#ifndef OPENTHREAD_JAM_DETECTION_H_
#define OPENTHREAD_JAM_DETECTION_H_

#include <openthread-types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup jam-det  Jamming Detection
 *
 * @brief
 *   This module includes functions for signal jamming detection feature.
 *
 * @{
 *
 */

/**
 * This function pointer is called if signal jam detection is enabled and a jam is detected.
 *
 * @param[in]  aJamState Current jam state (`true` if jam is detected, `false` otherwise).
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otJamDetectionCallback)(bool aJamState, void *aContext);

/**
 * Set the Jam Detection RSSI Threshold (in dBm).
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aRssiThreshold  The RSSI threshold.
 *
 * @retval kThreadErrorNone    Successfully set the threshold.
 *
 */
ThreadError otSetJamDetectionRssiThreshold(otInstance *aInstance, int8_t aRssiThreshold);

/**
 * Get the Jam Detection RSSI Threshold (in dBm).
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Jam Detection RSSI Threshold.
 */
int8_t otGetJamDetectionRssiThreshold(otInstance *aInstance);

/**
 * Set the Jam Detection Detection Window (in seconds).
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aWindow              The Jam Detection window (valid range is 1 to 63)
 *
 * @retval kThreadErrorNone         Successfully set the window.
 * @retval kThreadErrorInvalidArgs  The given input parameter not within valid range (1-63)
 *
 */
ThreadError otSetJamDetectionWindow(otInstance *aInstance, uint8_t aWindow);

/**
 * Get the Jam Detection Detection Window (in seconds).
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 *
 * @returns The Jam Detection Window.
 */
uint8_t otGetJamDetectionWindow(otInstance *aInstance);

/**
 * Set the Jam Detection Busy Period (in seconds).
 *
 * The number of aggregate seconds within the detection window where the RSSI must be above
 * threshold to trigger detection.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aBusyPeriod          The Jam Detection busy period (should be non-zero and
                                    less than or equal to Jam Detection Window)
 *
 * @retval kThreadErrorNone         Successfully set the window.
 * @retval kThreadErrorInvalidArgs  The given input is not within the valid range.
 *
 */
ThreadError otSetJamDetectionBusyPeriod(otInstance *aInstance, uint8_t aBusyPeriod);

/**
 * Get the Jam Detection Busy Period (in seconds)
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 *
 * @returns The Jam Detection Busy Period
 */
uint8_t otGetJamDetectionBusyPeriod(otInstance *aInstance);

/**
 * Start the jamming detection.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aCallback            A pointer to a function called to notify of jamming state change.
 * @param[in]  aContext             A pointer to application-specific context.
 *
 * @retval kThreadErrorNone         Successfully started the jamming detection.
 * @retval kThreadErrorAlready      Jam detection has been started before.
 *
 */
ThreadError otStartJamDetection(otInstance *aInstance, otJamDetectionCallback aCallback, void *aContext);

/**
 * Stop the jamming detection.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 *
 * @retval kThreadErrorNone         Successfully stopped the jamming detection.
 * @retval kThreadErrorAlready      Jam detection is already stopped.
 *
 */
ThreadError otStopJamDetection(otInstance *aInstance);

/**
 * Get the Jam Detection Status (enabled/disabled)
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 *
 * @returns The Jam Detection status (true if enabled, false otherwise).
 */
bool otIsJamDetectionEnabled(otInstance *aInstance);

/**
 * Get the Jam Detection State
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 *
 * @returns The Jam Detection state (`true` jam is detected, `false' otherwise).
 */
bool otGetJamDetectionState(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_JAM_DETECTION_H_
