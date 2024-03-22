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

#ifndef SPINEL_LOGGER_HPP_
#define SPINEL_LOGGER_HPP_

#include "openthread-core-config.h"

#include <openthread/error.h>
#include <openthread/platform/toolchain.h>

#include "ncp/ncp_config.h"

namespace ot {
namespace Spinel {

class Logger
{
protected:
    explicit Logger(const char *aModuleName);

    void LogIfFail(const char *aText, otError aError);

    void LogCrit(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(2, 3);
    void LogWarn(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(2, 3);
    void LogNote(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(2, 3);
    void LogInfo(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(2, 3);
    void LogDebg(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(2, 3);

    uint32_t Snprintf(char *aDest, uint32_t aSize, const char *aFormat, ...);
    void     LogSpinelFrame(const uint8_t *aFrame, uint16_t aLength, bool aTx);

    enum
    {
        kChannelMaskBufferSize = 32, ///< Max buffer size used to store `SPINEL_PROP_PHY_CHAN_SUPPORTED` value.
    };

    const char *mModuleName;
};

} // namespace Spinel
} // namespace ot

#endif // SPINEL_LOG_HPP_
