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
 *  This file defines the OpenThread Operational Dataset API.
 */

#ifndef OPENTHREAD_DATASET_H_
#define OPENTHREAD_DATASET_H_

#include "openthread/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup dataset  Operational Dataset
 *
 * @brief
 *   This module includes functions for Operational Dataset configuration.
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
 * @retval kThreadError_None         Successfully retrieved the Active Operational Dataset.
 * @retval kThreadError_InvalidArgs  @p aDataset was NULL.
 *
 */
OTAPI ThreadError OTCALL otDatasetGetActive(otInstance *aInstance, otOperationalDataset *aDataset);

/**
 * This function sets the Active Operational Dataset.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aDataset  A pointer to the Active Operational Dataset.
 *
 * @retval kThreadError_None         Successfully set the Active Operational Dataset.
 * @retval kThreadError_NoBufs       Insufficient buffer space to set the Active Operational Datset.
 * @retval kThreadError_InvalidArgs  @p aDataset was NULL.
 *
 */
OTAPI ThreadError OTCALL otDatasetSetActive(otInstance *aInstance, const otOperationalDataset *aDataset);

/**
 * This function gets the Pending Operational Dataset.
 *
 * @param[in]   aInstance A pointer to an OpenThread instance.
 * @param[out]  aDataset  A pointer to where the Pending Operational Dataset will be placed.
 *
 * @retval kThreadError_None         Successfully retrieved the Pending Operational Dataset.
 * @retval kThreadError_InvalidArgs  @p aDataset was NULL.
 *
 */
OTAPI ThreadError OTCALL otDatasetGetPending(otInstance *aInstance, otOperationalDataset *aDataset);

/**
 * This function sets the Pending Operational Dataset.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aDataset  A pointer to the Pending Operational Dataset.
 *
 * @retval kThreadError_None         Successfully set the Pending Operational Dataset.
 * @retval kThreadError_NoBufs       Insufficient buffer space to set the Pending Operational Dataset.
 * @retval kThreadError_InvalidArgs  @p aDataset was NULL.
 *
 */
OTAPI ThreadError OTCALL otDatasetSetPending(otInstance *aInstance, const otOperationalDataset *aDataset);

/**
 * This function sends MGMT_ACTIVE_GET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aTlvTypes  A pointer to the TLV Types.
 * @param[in]  aLength    The length of TLV Types.
 * @param[in]  aAddress   A pointer to the IPv6 destination, if it is NULL, will use Leader ALOC as default.
 *
 * @retval kThreadError_None         Successfully send the meshcop dataset command.
 * @retval kThreadError_NoBufs       Insufficient buffer space to send.
 *
 */
OTAPI ThreadError OTCALL otDatasetSendMgmtActiveGet(otInstance *aInstance, const uint8_t *aTlvTypes, uint8_t aLength,
                                                    const otIp6Address *aAddress);

/**
 * This function sends MGMT_ACTIVE_SET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aDataset   A pointer to operational dataset.
 * @param[in]  aTlvs      A pointer to TLVs.
 * @param[in]  aLength    The length of TLVs.
 *
 * @retval kThreadError_None         Successfully send the meshcop dataset command.
 * @retval kThreadError_NoBufs       Insufficient buffer space to send.
 *
 */
OTAPI ThreadError OTCALL otDatasetSendMgmtActiveSet(otInstance *aInstance, const otOperationalDataset *aDataset,
                                                    const uint8_t *aTlvs, uint8_t aLength);

/**
 * This function sends MGMT_PENDING_GET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aTlvTypes  A pointer to the TLV Types.
 * @param[in]  aLength    The length of TLV Types.
 * @param[in]  aAddress   A pointer to the IPv6 destination, if it is NULL, will use Leader ALOC as default.
 *
 * @retval kThreadError_None         Successfully send the meshcop dataset command.
 * @retval kThreadError_NoBufs       Insufficient buffer space to send.
 *
 */
OTAPI ThreadError OTCALL otDatasetSendMgmtPendingGet(otInstance *aInstance, const uint8_t *aTlvTypes, uint8_t aLength,
                                                     const otIp6Address *aAddress);

/**
 * This function sends MGMT_PENDING_SET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aDataset   A pointer to operational dataset.
 * @param[in]  aTlvs      A pointer to TLVs.
 * @param[in]  aLength    The length of TLVs.
 *
 * @retval kThreadError_None         Successfully send the meshcop dataset command.
 * @retval kThreadError_NoBufs       Insufficient buffer space to send.
 *
 */
OTAPI ThreadError OTCALL otDatasetSendMgmtPendingSet(otInstance *aInstance, const otOperationalDataset *aDataset,
                                                     const uint8_t *aTlvs, uint8_t aLength);

/**
 * Get minimal delay timer.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval the value of minimal delay timer (in ms).
 *
 */
OTAPI uint32_t OTCALL otDatasetGetDelayTimerMinimal(otInstance *aInstance);

/**
 * Set minimal delay timer.
 *
 * @param[in]  aInstance           A pointer to an OpenThread instance.
 * @param[in]  aDelayTimerMinimal  The value of minimal delay timer (in ms).
 *
 * @retval  kThreadError_None         Successfully set minimal delay timer.
 * @retval  kThreadError_InvalidArgs  If @p aDelayTimerMinimal is not valid.
 *
 */
OTAPI ThreadError OTCALL otDatasetSetDelayTimerMinimal(otInstance *aInstance, uint32_t aDelayTimerMinimal);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_DATASET_H_
