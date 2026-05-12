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
 *  This file defines the OpenThread Border Router Multi-AIL Detection API.
 */

#ifndef OPENTHREAD_MULTI_AIL_DETECTION_H_
#define OPENTHREAD_MULTI_AIL_DETECTION_H_

#include <stdbool.h>

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-multi-ail-detection
 *
 * @brief
 *  This module includes functions for the OpenThread Multi-AIL Detection feature.
 *
 * @{
 *
 * All the functions in this module require both the `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` and
 * `OPENTHREAD_CONFIG_BORDER_ROUTING_MULTI_AIL_DETECTION_ENABLE` to be enabled.
 */

/**
 * A callback function pointer called when the multi-AIL detection state changes.
 *
 * This callback function is invoked by the OpenThread stack whenever the detector determines a change in whether
 * Border Routers on the Thread mesh might be connected to different Adjacent Infrastructure Links (AILs).
 *
 * See `otBorderRoutingIsMultiAilDetected()` for more details.
 *
 * @param[in] aDetected   `TRUE` if multiple AILs are now detected, `FALSE` otherwise.
 * @param[in] aContext    A pointer to arbitrary context information provided when the callback was registered
 *                        using `otBorderRoutingSetMultiAilCallback()`.
 */
typedef void (*otBorderRoutingMultiAilCallback)(bool aDetected, void *aContext);

/**
 * Enables or disables the Multi-AIL Detector.
 *
 * If `OPENTHREAD_CONFIG_BORDER_ROUTING_MULTI_AIL_DETECTION_AUTO_ENABLE_MODE` is enabled, the detector is enabled
 * by default and starts running when the infra-if network is initialized and becomes active (running).
 *
 * @param[in] aInstance  A pointer to the OpenThread instance.
 * @param[in] aEnable    TRUE to enable the detector, FALSE to disable.
 */
void otBorderRoutingSetMultiAilDetectionEnabled(otInstance *aInstance, bool aEnable);

/**
 * Checks if the Multi-AIL Detector is enabled.
 *
 * @param[in] aInstance  A pointer to the OpenThread instance.
 *
 * @retval TRUE   If the detector is enabled.
 * @retval FALSE  If the detector is disabled.
 */
bool otBorderRoutingIsMultiAilDetectionEnabled(otInstance *aInstance);

/**
 * Checks if the Multi-AIL Detector is running.
 *
 * The detector runs when it is enabled and the infrastructure interface is also active.
 *
 * @param[in] aInstance  A pointer to the OpenThread instance.
 *
 * @retval TRUE   If the detector is running.
 * @retval FALSE  If the detector is not running.
 */
bool otBorderRoutingIsMultiAilDetectionRunning(otInstance *aInstance);

/**
 * Gets the current detected state regarding multiple Adjacent Infrastructure Links (AILs).
 *
 * It returns whether the detector currently believes that Border Routers (BRs) on the Thread mesh may be connected to
 * different AILs.
 *
 * The detection mechanism operates as follows: The detector monitors the number of peer BRs listed in the
 * Thread Network Data (see `otBorderRoutingCountPeerBrs()`) and compares this count with the number of peer BRs
 * discovered by processing received Router Advertisement (RA) messages on its connected AIL. If the count derived from
 * Network Data consistently exceeds the count derived from RAs for a detection duration of 10 minutes, it concludes
 * that BRs are likely connected to different AILs. To clear state a shorter window of 1 minute is used.
 *
 * The detection window of 10 minutes helps to avoid false positives due to transient changes. The detector uses
 * 200 seconds for reachability checks of peer BRs (sending Neighbor Solicitation). Stale Network Data entries are
 * also expected to age out within a few minutes. So a 10-minute detection time accommodates both cases.
 *
 * While generally effective, this detection mechanism may get less reliable in scenarios with a large number of
 * BRs, particularly exceeding ten. This is related to the "Network Data Publisher" mechanism, where BRs might refrain
 * from publishing their external route information in the Network Data to conserve its limited size, potentially
 * skewing the Network Data BR count.
 *
 * @param[in] aInstance  A pointer to the OpenThread instance.
 *
 * @retval TRUE   Has detected that BRs are likely connected to multiple AILs.
 * @retval FALSE  Has not detected (or no longer detects) that BRs are connected to multiple AILs.
 */
bool otBorderRoutingIsMultiAilDetected(otInstance *aInstance);

/**
 * Sets a callback function to be notified of changes in the multi-AIL detection state.
 *
 * Subsequent calls to this function will overwrite the previous callback setting. Using `NULL` for @p aCallback will
 * disable the callback.
 *
 * @param[in] aInstance  A pointer to the OpenThread instance.
 * @param[in] aCallback  A pointer to the function (`otBorderRoutingMultiAilCallback`) to be called
 *                       upon state changes, or `NULL` to unregister a previously set callback.
 * @param[in] aContext   A pointer to application-specific context that will be passed back
 *                       in the `aCallback` function. This can be `NULL` if no context is needed.
 */
void otBorderRoutingSetMultiAilCallback(otInstance                     *aInstance,
                                        otBorderRoutingMultiAilCallback aCallback,
                                        void                           *aContext);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_MULTI_AIL_DETECTION_H_
