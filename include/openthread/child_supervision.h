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
 * @brief
 *   This file includes the OpenThread API for child supervision feature.
 */

#ifndef OPENTHREAD_CHILD_SUPERVISION_H_
#define OPENTHREAD_CHILD_SUPERVISION_H_

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-child-supervision
 *
 * @brief
 *   This module includes functions for Child Supervision feature.
 *
 * @{
 *
 */

/**
 * Gets the Child Supervision interval (in seconds) on a child.
 *
 * Child Supervision feature provides a mechanism for parent to ensure that a message is sent to each sleepy child
 * within the supervision interval. If there is no transmission to the child within the supervision interval, OpenThread
 * enqueues and sends a Child Supervision Message to the child.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 *
 * @returns  The child supervision interval. Zero indicates that supervision is disabled.
 *
 */
uint16_t otChildSupervisionGetInterval(otInstance *aInstance);

/**
 * Sets the child supervision interval (in seconds) on the child.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aInterval       The supervision interval (in seconds). Zero to disable supervision.
 *
 */
void otChildSupervisionSetInterval(otInstance *aInstance, uint16_t aInterval);

/**
 * Gets the supervision check timeout interval (in seconds) on the child.
 *
 * If the device is a sleepy child and it does not hear from its parent within the specified check timeout, it initiates
 * the re-attach process (MLE Child Update Request/Response exchange with its parent).
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 *
 * @returns  The supervision check timeout. Zero indicates that supervision check on the child is disabled.
 *
 */
uint16_t otChildSupervisionGetCheckTimeout(otInstance *aInstance);

/**
 * Sets the supervision check timeout interval (in seconds) on the child.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aTimeout        The check timeout (in seconds). Zero to disable supervision check on the child.
 *
 */
void otChildSupervisionSetCheckTimeout(otInstance *aInstance, uint16_t aTimeout);

/**
 * Get the value of supervision check timeout failure counter.
 *
 * The counter tracks the number of supervision check failures on the child. It is incremented when the child does
 * not hear from its parent within the specified check timeout interval.
 *
 */
uint16_t otChildSupervisionGetCheckFailureCounter(otInstance *aInstance);

/**
 * Reset the supervision check timeout failure counter to zero.
 *
 */
void otChildSupervisionResetCheckFailureCounter(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_CHILD_SUPERVISION_H_
