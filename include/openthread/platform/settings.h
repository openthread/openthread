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
 *   This file includes platform abstractions for non-volatile storage of settings.
 */

#ifndef OT_PLATFORM_SETTINGS_H
#define OT_PLATFORM_SETTINGS_H 1

#include "openthread/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Performs any initialization for the settings subsystem, if necessary.
 *
 * @param[in]  aInstance
 *             The OpenThread instance structure.
 *
 */
void otPlatSettingsInit(otInstance *aInstance);

/// Begin atomic change set
/** This function is called at the start of a sequence of changes
 *  that should be made atomically.
 *
 *  This function, along with `otPlatSettingsCommitChange()` are designed
 *  to ensure atomicity of changes to multiple settings. Once
 *  this function has been called, any changes made to the settings
 *  store are only committed when `otPlatSettingsCommitChange()` is
 *  called.
 *
 *  The implementation of this function is optional. If not
 *  implemented, it should return kThreadError_None.
 *
 * @param[in]  aInstance
 *             The OpenThread instance structure.
 *
 *  @retval kThreadError_None    The settings commit lock has been set.
 *  @retval kThreadError_Already The commit lock is already set.
 *
 *  @sa otPlatSettingsCommitChange(), otPlatSettingsAbandonChange()
 */
ThreadError otPlatSettingsBeginChange(otInstance *aInstance);

/// Commit all settings changes since previous call to otPlatSettingsBeginChange()
/** This function is called at the end of a sequence of changes.
 *  The implementation of this function is optional. If not
 *  implemented, it should return kThreadError_NotImplemented.
 *
 *  @param[in]  aInstance
 *              The OpenThread instance structure.
 *
 *  @retval kThreadError_None
 *          The changes made since the last call to
 *          otPlatSettingsBeginChange() have been successfully
 *          committed.
 *  @retval kThreadError_InvalidState
 *          otPlatSettingsBeginChange() has not been called.
 *  @retval kThreadError_NotImplemented
 *          This function is not implemented on this platform.
 *
 *  @sa otPlatSettingsBeginChange(), otPlatSettingsAbandonChange()
 */
ThreadError otPlatSettingsCommitChange(otInstance *aInstance);

/// Abandon all settings changes since previous call to otPlatSettingsBeginChange()
/** This function may be called at the end of a sequence of changes.
 *  instead of otPlatSettingsCommitChange(). If implemented, it
 *  causes all changes made since otPlatSettingsBeginChange() to be
 *  rolled back and abandoned.
 *
 *  The implementation of this function is optional. If not
 *  implemented, it should return kThreadError_NotImplemented.
 *
 *  @param[in]  aInstance
 *              The OpenThread instance structure.
 *
 *  @retval kThreadError_None
 *          The changes made since the last call to
 *          otPlatSettingsBeginChange() have been successfully
 *          rolled back.
 *  @retval kThreadError_InvalidState
 *          otPlatSettingsBeginChange() has not been called.
 *  @retval kThreadError_NotImplemented
 *          This function is not implemented on this platform.
 *
 *  @sa otPlatSettingsBeginChange(), otPlatSettingsCommitChange()
 */
ThreadError otPlatSettingsAbandonChange(otInstance *aInstance);

/// Fetches the value of a setting
/** This function fetches the value of the setting identified
 *  by aKey and write it to the memory pointed to by aValue.
 *  It then writes the length to the integer pointed to by
 *  aValueLength. The initial value of aValueLength is the
 *  maximum number of bytes to be written to aValue.
 *
 *  This function can be used to check for the existence of
 *  a key without fetching the value by setting aValue and
 *  aValueLength to NULL. You can also check the length of
 *  the setting without fetching it by setting only aValue
 *  to NULL.
 *
 *  Note that the underlying storage implementation is not
 *  required to maintain the order of settings with multiple
 *  values. The order of such values MAY change after ANY
 *  write operation to the store.
 *
 *  @param[in]     aInstance
 *                 The OpenThread instance structure.
 *  @param[in]     aKey
 *                 The key associated with the requested setting.
 *  @param[in]     aIndex
 *                 The index of the specific item to get.
 *  @param[out]    aValue
 *                 A pointer to where the value of the setting
 *                 should be written. May be set to NULL if just
 *                 testing for the presence or length of a setting.
 *  @param[in/out] aValueLength
 *                 A pointer to the length of the value. When
 *                 called, this pointer should point to an
 *                 integer containing the maximum value size that
 *                 can be written to aValue. At return, the actual
 *                 length of the setting is written. This may be
 *                 set to NULL if performing a presence check.
 *
 *  @retval kThreadError_None
 *          The given setting was found and fetched successfully.
 *  @retval kThreadError_NotFound
 *          The given setting was not found in the setting store.
 *  @retval kThreadError_NotImplemented
 *          This function is not implemented on this platform.
 */
ThreadError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue,
                              uint16_t *aValueLength);

/// Sets or replaces the value of a setting
/** This function sets or replaces the value of a setting
 *  identified by aKey. If there was more than one
 *  value previously associated with aKey, then they are
 *  all deleted and replaced with this single entry.
 *
 *  Calling this function successfully may cause unrelated
 *  settings with multiple values to be reordered.
 *
 *  @param[in]  aInstance
 *              The OpenThread instance structure.
 *  @param[in]  aKey
 *              The key associated with the setting to change.
 *  @param[out] aValue
 *              A pointer to where the new value of the setting
 *              should be read from. MUST NOT be NULL if aValueLength
 *              is non-zero.
 *  @param[in]  aValueLength
 *              The length of the data pointed to by aValue.
 *              May be zero.
 *
 *  @retval kThreadError_None
 *          The given setting was changed or staged.
 *  @retval kThreadError_NotImplemented
 *          This function is not implemented on this platform.
 */
ThreadError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);

/// Adds a value to a setting
/** This function adds the value to a setting
 *  identified by aKey, without replacing any existing
 *  values.
 *
 *  Note that the underlying implementation is not required
 *  to maintain the order of the items associated with a
 *  specific key. The added value may be added to the end,
 *  the beginning, or even somewhere in the middle. The order
 *  of any pre-existing values may also change.
 *
 *  Calling this function successfully may cause unrelated
 *  settings with multiple values to be reordered.
 *
 * @param[in]     aInstance
 *                The OpenThread instance structure.
 * @param[in]     aKey
 *                The key associated with the setting to change.
 * @param[out]    aValue
 *                A pointer to where the new value of the setting
 *                should be read from. MUST NOT be NULL if aValueLength
 *                is non-zero.
 * @param[in/out] aValueLength
 *                The length of the data pointed to by aValue.
 *                May be zero.
 *
 * @retval kThreadError_None
 *         The given setting was added or staged to be added.
 * @retval kThreadError_NotImplemented
 *         This function is not implemented on this platform.
 */
ThreadError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);

/// Removes a setting from the setting store
/** This function deletes a specific value from the
 *  setting identified by aKey from the settings store.
 *
 *  Note that the underlying implementation is not required
 *  to maintain the order of the items associated with a
 *  specific key.
 *
 *  @param[in] aInstance
 *             The OpenThread instance structure.
 *  @param[in] aKey
 *             The key associated with the requested setting.
 *  @param[in] aIndex
 *             The index of the value to be removed. If set to
 *             -1, all values for this aKey will be removed.
 *
 *  @retval kThreadError_None
 *          The given key and index was found and removed successfully.
 *  @retval kThreadError_NotFound
 *          The given key or index  was not found in the setting store.
 *  @retval kThreadError_NotImplemented
 *          This function is not implemented on this platform.
 */
ThreadError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex);

/// Removes all settings from the setting store
/** This function deletes all settings from the settings
 *  store, resetting it to its initial factory state.
 *
 *  @param[in] aInstance
 *             The OpenThread instance structure.
 */
void otPlatSettingsWipe(otInstance *aInstance);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OT_PLATFORM_SETTINGS_H
