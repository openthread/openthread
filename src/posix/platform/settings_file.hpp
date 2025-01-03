/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#ifndef OT_POSIX_PLATFORM_SETTINGS_FILE_HPP_
#define OT_POSIX_PLATFORM_SETTINGS_FILE_HPP_

#include "openthread-posix-config.h"
#include "platform-posix.h"

namespace ot {
namespace Posix {

class SettingsFile
{
public:
    SettingsFile(void)
        : mSettingsFd(-1)
    {
    }

    /**
     * Performs the initialization for the settings file.
     *
     * @param[in]  aSettingsFileBaseName    A pointer to the base name of the settings file.
     *
     * @retval OT_ERROR_NONE    The given settings file was initialized successfully.
     * @retval OT_ERROR_PARSE   The key-value format could not be parsed (invalid format).
     */
    otError Init(const char *aSettingsFileBaseName);

    /**
     * Performs the de-initialization for the settings file.
     */
    void Deinit(void);

    /**
     * Gets a setting from the settings file.
     *
     * @param[in]      aKey          The key associated with the requested setting.
     * @param[in]      aIndex        The index of the specific item to get.
     * @param[out]     aValue        A pointer to where the value of the setting should be written.
     * @param[in,out]  aValueLength  A pointer to the length of the value.
     *
     * @retval OT_ERROR_NONE        The given setting was found and fetched successfully.
     * @retval OT_ERROR_NOT_FOUND   The given key or index was not found in the setting store.
     */
    otError Get(uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength);

    /**
     * Sets a setting in the settings file.
     *
     * @param[in]  aKey          The key associated with the requested setting.
     * @param[in]  aValue        A pointer to where the new value of the setting should be read from.
     * @param[in]  aValueLength  The length of the data pointed to by aValue.
     */
    void Set(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);

    /**
     * Adds a setting to the settings file.
     *
     * @param[in]  aKey          The key associated with the requested setting.
     * @param[in]  aValue        A pointer to where the new value of the setting should be read from.
     * @param[in]  aValueLength  The length of the data pointed to by aValue.
     */
    void Add(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);

    /**
     * Removes a setting from the settings file.
     *
     * @param[in]  aKey       The key associated with the requested setting.
     * @param[in]  aIndex     The index of the value to be removed. If set to -1, all values for this aKey will be
     *                        removed.
     *
     * @retval OT_ERROR_NONE        The given key and index was found and removed successfully.
     * @retval OT_ERROR_NOT_FOUND   The given key or index was not found in the setting store.
     */
    otError Delete(uint16_t aKey, int aIndex);

    /**
     * Deletes all settings from the setting file.
     */
    void Wipe(void);

private:
    static const size_t kMaxFileDirectorySize   = sizeof(OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH);
    static const size_t kSlashLength            = 1;
    static const size_t kMaxFileBaseNameSize    = 64;
    static const size_t kMaxFileExtensionLength = 5; ///< The length of `.Swap` or `.data`.
    static const size_t kMaxFilePathSize =
        kMaxFileDirectorySize + kSlashLength + kMaxFileBaseNameSize + kMaxFileExtensionLength;

    otError Delete(uint16_t aKey, int aIndex, int *aSwapFd);
    void    GetSettingsFilePath(char aFileName[kMaxFilePathSize], bool aSwap);
    int     SwapOpen(void);
    void    SwapWrite(int aFd, uint16_t aLength);
    void    SwapPersist(int aFd);
    void    SwapDiscard(int aFd);

    char mSettingFileBaseName[kMaxFileBaseNameSize];
    int  mSettingsFd;
};

} // namespace Posix
} // namespace ot

#endif // OT_POSIX_PLATFORM_SETTINGS_FILE_HPP_
