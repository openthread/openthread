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
 *  This file defines the OpenThread Operational Dataset API (for both FTD and MTD).
 */

#ifndef OPENTHREAD_DATASET_H_
#define OPENTHREAD_DATASET_H_

#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-thread-general
 *
 * @{
 *
 */

/**
 * This function indicates whether a valid network is present in the Active Operational Dataset or not.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns TRUE if a valid network is present in the Active Operational Dataset, FALSE otherwise.
 *
 */
OTAPI bool OTCALL otDatasetIsCommissioned(otInstance *aInstance);

/**
 * This function gets the Active Operational Dataset.
 *
 * @param[in]   aInstance A pointer to an OpenThread instance.
 * @param[out]  aDataset  A pointer to where the Active Operational Dataset will be placed.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved the Active Operational Dataset.
 * @retval OT_ERROR_INVALID_ARGS  @p aDataset was NULL.
 *
 */
OTAPI otError OTCALL otDatasetGetActive(otInstance *aInstance, otOperationalDataset *aDataset);

/**
 * This function gets the Pending Operational Dataset.
 *
 * @param[in]   aInstance A pointer to an OpenThread instance.
 * @param[out]  aDataset  A pointer to where the Pending Operational Dataset will be placed.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved the Pending Operational Dataset.
 * @retval OT_ERROR_INVALID_ARGS  @p aDataset was NULL.
 *
 */
OTAPI otError OTCALL otDatasetGetPending(otInstance *aInstance, otOperationalDataset *aDataset);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_DATASET_H_
