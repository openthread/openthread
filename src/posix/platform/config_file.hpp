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

#ifndef POSIX_PLATFORM_CONFIG_FILE_HPP_
#define POSIX_PLATFORM_CONFIG_FILE_HPP_

#include <assert.h>
#include <stdint.h>
#include <openthread/error.h>

namespace ot {
namespace Posix {

/**
 * This class manages configuration file.
 *
 */
class ConfigFile
{
public:
    /**
     * This method initializes the configuration file path.
     *
     * @param[in]  aFilePath  A pointer to the null-terminated file path.
     *
     */
    explicit ConfigFile(const char *aFilePath)
        : mFilePath(aFilePath)
    {
        assert(mFilePath != nullptr);
    }

    /**
     * This method gets a configuration from the configuration file.
     *
     * @param[in]      aKey          The key string associated with the requested configuration.
     * @param[in,out]  aIterator     A pointer to an iterator. MUST be initialized to 0 or the behavior is undefined.
     * @param[out]     aValue        A pointer to where the new value string of the configuration should be read from.
     *                               The @p aValue string will be terminated with `\0` if this method returns success.
     * @param[in]      aValueLength  The max length of the data pointed to by @p aValue.
     *
     * @retval OT_ERROR_NONE          The given configuration was found and fetched successfully.
     * @retval OT_ERROR_NOT_FOUND     The given key or iterator was not found in the configuration file.
     * @retval OT_ERROR_INVALID_ARGS  If @p aKey or @p aValue was NULL.
     *
     */
    otError Get(const char *aKey, int &aIterator, char *aValue, int aValueLength);

    /**
     * This method adds a configuration to the configuration file.
     *
     * @param[in]  aKey    The key string associated with the requested configuration.
     * @param[in]  aValue  A pointer to where the new value string of the configuration should be written.
     *
     * @retval OT_ERROR_NONE          The given key was found and removed successfully.
     * @retval OT_ERROR_INVALID_ARGS  If @p aKey or @p aValue was NULL.
     *
     */
    otError Add(const char *aKey, const char *aValue);

    /**
     * This function removes all configurations with the same key string from the configuration file.
     *
     * @param[in]  aKey  The key string associated with the requested configuration.
     *
     * @retval OT_ERROR_NONE          The given key was found and removed successfully.
     * @retval OT_ERROR_INVALID_ARGS  If @p aKey was NULL.
     *
     */
    otError Clear(const char *aKey);

private:
    const char *              kCommentDelimiter = "#";
    static constexpr uint16_t kLineMaxSize      = 512;
    static constexpr uint16_t kFileNameMaxSize  = 255;

    void RemoveGarbage(char *aString);

    const char *mFilePath;
};

} // namespace Posix
} // namespace ot

#endif // POSIX_PLATFORM_CONFIG_FILE_HPP_
