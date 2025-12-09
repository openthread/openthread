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
 * @brief This file defines the OpenThread MeshCoP Steering Data APIs.
 *
 */

#ifndef OPENTHREAD_STEERING_DATA_H_
#define OPENTHREAD_STEERING_DATA_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/error.h>
#include <openthread/joiner.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-steering-data
 *
 * @brief
 *   This module includes helper functions for the MeshCoP Steering Data.
 *
 * @{
 *
 * All the functions in this header require `OPENTHREAD_CONFIG_MESHCOP_STEERING_DATA_API_ENABLE` to be enabled.
 */

#define OT_STEERING_DATA_MIN_LENGTH 1 ///< Min Steering Data length (bytes)

#define OT_STEERING_DATA_MAX_LENGTH 16 ///< Max Steering Data length (bytes)

/**
 * Represents the steering data.
 */
typedef struct otSteeringData
{
    uint8_t mLength;                         ///< Length of Steering Data (bytes).
    uint8_t m8[OT_STEERING_DATA_MAX_LENGTH]; ///< Byte values.
} otSteeringData;

/**
 * Initializes the Steering Data.
 *
 * @param[out] aSteeringData  The Steering Data to initialize.
 * @param[in]  aLength        The length of the Steering Data in bytes.
 *
 * @retval OT_ERROR_NONE          Successfully initialized the Steering Data.
 * @retval OT_ERROR_INVALID_ARGS  The @p aLength is invalid.
 */
otError otSteeringDataInit(otSteeringData *aSteeringData, uint8_t aLength);

/**
 * Checks whether the Steering Data has a valid length.
 *
 * @param[in] aSteeringData  The Steering Data to check.
 *
 * @retval TRUE   If the Steering Data's length is valid.
 * @retval FALSE  If the Steering Data's length is not valid.
 */
bool otSteeringDataIsValid(const otSteeringData *aSteeringData);

/**
 * Sets the Steering Data to permit all joiners.
 *
 * @param[out] aSteeringData  The Steering Data to update.
 */
void otSteeringDataSetToPermitAllJoiners(otSteeringData *aSteeringData);

/**
 * Updates the Steering Data's bloom filter with a Joiner ID.
 *
 * @param[out] aSteeringData  The Steering Data to update.
 * @param[in]  aJoinerId      The Joiner ID to add.
 *
 * @retval OT_ERROR_NONE          Successfully updated the Steering Data.
 * @retval OT_ERROR_INVALID_ARGS  The Steering Data is not valid (incorrect length).
 */
otError otSteeringDataUpdateWithJoinerId(otSteeringData *aSteeringData, const otExtAddress *aJoinerId);

/**
 * Updates the Steering Data's bloom filter with a Joiner Discerner.
 *
 * @param[out] aSteeringData  The Steering Data to update
 * @param[in]  aDiscerner     The Joiner Discerner to add.
 *
 * @retval OT_ERROR_NONE          Successfully updated the Steering Data.
 * @retval OT_ERROR_INVALID_ARGS  The Steering Data is not valid (incorrect length).
 */
otError otSteeringDataUpdateWithDiscerner(otSteeringData *aSteeringData, const otJoinerDiscerner *aDiscerner);

/**
 * Merges two Steering Data bloom filters.
 *
 * The @p aOtherSteeringData must have a length that is a divisor of the @p aSteeringData length.
 *
 * @param[out] aSteeringData       The Steering Data to merge into.
 * @param[in]  aOtherSteeringData  The other Steering Data to merge from.
 *
 * @retval OT_ERROR_NONE          Successfully merged the Steering Data.
 * @retval OT_ERROR_INVALID_ARGS  The Steering Data lengths are not valid or they cannot be merged.
 */
otError otSteeringDataMerge(otSteeringData *aSteeringData, const otSteeringData *aOtherSteeringData);

/**
 * Checks if the Steering Data permits all joiners.
 *
 * @param[in] aSteeringData  The Steering Data to check.
 *
 * @retval TRUE   If the Steering Data permits all joiners.
 * @retval FALSE  If the Steering Data does not permit all joiners.
 */
bool otSteeringDataPermitsAllJoiners(const otSteeringData *aSteeringData);

/**
 * Checks if the Steering Data is empty.
 *
 * @param[in] aSteeringData  The Steering Data to check.
 *
 * @retval TRUE   If the Steering Data is empty.
 * @retval FALSE  If the Steering Data is not empty.
 */
bool otSteeringDataIsEmpty(const otSteeringData *aSteeringData);

/**
 * Checks if the Steering Data contains a Joiner ID.
 *
 * @param[in] aSteeringData  The Steering Data to check.
 * @param[in] aJoinerId      The Joiner ID.
 *
 * @retval TRUE   If the Steering Data contains the Joiner ID.
 * @retval FALSE  If the Steering Data does not contain the Joiner ID.
 */
bool otSteeringDataContainsJoinerId(const otSteeringData *aSteeringData, const otExtAddress *aJoinerId);

/**
 * Checks if the Steering Data contains a Joiner Discerner.
 *
 * @param[in] aSteeringData  The Steering Data to check.
 * @param[in] aDiscerner     The Joiner Discerner.
 *
 * @retval TRUE   If the Steering Data contains the Joiner Discerner.
 * @retval FALSE  If the Steering Data does not contain the Joiner Discerner.
 */
bool otSteeringDataContainsDiscerner(const otSteeringData *aSteeringData, const otJoinerDiscerner *aDiscerner);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_STEERING_DATA_H_
