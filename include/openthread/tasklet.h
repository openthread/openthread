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
 *   This file defines the OpenThread API for Tasklets.
 */

#ifndef OPENTHREAD_TASKLET_H_
#define OPENTHREAD_TASKLET_H_

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-tasklets
 *
 * @brief
 *   This module includes functions that control the Thread stack's execution.
 *
 * @{
 *
 */

#if OPENTHREAD_CONFIG_GENERIC_TASKLET_ENABLE
/**
 * Callback function set to be executed from the context of the OpenThread task
 *
 * @param[in]  aContext   A pointer to the application-specific context.
 *
 */
typedef void (*otTaskletCb)(void *aContext);

/**
 * Use the generic tasklet defined in the OpenThread instance to execute a callback function in
 * the context of the OpenThread task. This is useful for OpenThread modules that process data from
 * an external interface and want to execute the handling function in the context of OpenThread
 * task.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] callback  The callback function that executes from the context of the OpenThread task.
 * @param[in] context   A pointer to a context that will be used by the callback function.
 *
 * @retval OT_ERROR_NONE              Successfully allocated InternalContext.
 * @retval OT_ERROR_NO_BUFS           Insufficient space to store the InternalContext.
 * @retval OT_ERROR_INVALID_STATE     OpenThread Instance is not initialized.
 *
 */
otError otTaskletExecute(otInstance *aInstance, otTaskletCb callback, void *context);
#endif /*OPENTHREAD_CONFIG_GENERIC_TASKLET_ENABLE*/

/**
 * Run all queued OpenThread tasklets at the time this is called.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 */
void otTaskletsProcess(otInstance *aInstance);

/**
 * Indicates whether or not OpenThread has tasklets pending.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   If there are tasklets pending.
 * @retval FALSE  If there are no tasklets pending.
 *
 */
bool otTaskletsArePending(otInstance *aInstance);

/**
 * OpenThread calls this function when the tasklet queue transitions from empty to non-empty.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 */
extern void otTaskletsSignalPending(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_TASKLET_H_
