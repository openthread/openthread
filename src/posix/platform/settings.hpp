/*
 *  Copyright (c) 2022, The OpenThread Authors.
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

#ifndef POSIX_PLATFORM_SETTINGS_HPP_
#define POSIX_PLATFORM_SETTINGS_HPP_

namespace ot {
namespace Posix {

/**
 * This function gets a setting from the persisted file.
 *
 * @param[in]      aInstance     The OpenThread instance structure.
 * @param[in]      aKey          The key associated with the requested setting.
 * @param[in]      aIndex        The index of the specific item to get.
 * @param[out]     aValue        A pointer to where the value of the setting should be written.
 * @param[in,out]  aValueLength  A pointer to the length of the value.
 *
 * @retval OT_ERROR_NONE        The given setting was found and fetched successfully.
 * @retval OT_ERROR_NOT_FOUND   The given key or index was not found in the setting store.
 *
 */
otError PlatformSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength);

/**
 * This function sets a setting in the persisted file.
 *
 * @param[in]  aInstance     The OpenThread instance structure.
 * @param[in]  aKey          The key associated with the requested setting.
 * @param[in]  aValue        A pointer to where the new value of the setting should be read from.
 * @param[in]  aValueLength  The length of the data pointed to by aValue.
 *
 */
void PlatformSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);

/**
 * This function adds a setting to the persisted file.
 *
 * @param[in]  aInstance     The OpenThread instance structure.
 * @param[in]  aKey          The key associated with the requested setting.
 * @param[in]  aValue        A pointer to where the new value of the setting should be read from.
 * @param[in]  aValueLength  The length of the data pointed to by aValue.
 *
 */
void PlatformSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);

/**
 * This function removes a setting either from swap file or persisted file.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in]  aKey       The key associated with the requested setting.
 * @param[in]  aIndex     The index of the value to be removed. If set to -1, all values for this aKey will be removed.
 * @param[out] aSwapFd    A optional pointer to receive file descriptor of the generated swap file descriptor.
 *
 * @note
 *   If @p aSwapFd is null, operate deleting on the setting file.
 *   If @p aSwapFd is not null, operate on the swap file, and aSwapFd will point to the swap file descriptor.
 *
 * @retval OT_ERROR_NONE        The given key and index was found and removed successfully.
 * @retval OT_ERROR_NOT_FOUND   The given key or index was not found in the setting store.
 *
 */
otError PlatformSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex, int *aSwapFd);

/**
 * This function gets the sensitive keys that should be stored in the secure area.
 *
 * @param[in]   aInstance    The OpenThread instance structure.
 * @param[out]  aKeys        A pointer to where the pointer to the array containing sensitive keys should be written.
 * @param[out]  aKeysLength  A pointer to where the count of sensitive keys should be written.
 *
 */
void PlatformSettingsGetSensitiveKeys(otInstance *aInstance, const uint16_t **aKeys, uint16_t *aKeysLength);

} // namespace Posix
} // namespace ot

#endif // POSIX_PLATFORM_SETTINGS_HPP_
