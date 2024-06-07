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

/**
 * @file
 *   This file contains definitions for the RCP capability diagnostics module.
 */

#ifndef OT_POSIX_PLATFORM_RCP_CAPS_DIAG_HPP_
#define OT_POSIX_PLATFORM_RCP_CAPS_DIAG_HPP_

#include "platform-posix.h"

#if OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE
#include "lib/spinel/radio_spinel.hpp"
#include "lib/spinel/spinel.h"

#if !OPENTHREAD_CONFIG_DIAG_ENABLE
#error "OPENTHREAD_CONFIG_DIAG_ENABLE is required for OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE"
#endif

namespace ot {
namespace Posix {

class RcpCapsDiag
{
public:
    /**
     * Constructor initializes the object.
     *
     * @param[in]  aRadioSpinel  A reference to the Spinel::RadioSpinel instance.
     *
     */
    explicit RcpCapsDiag(Spinel::RadioSpinel &aRadioSpinel)
        : mRadioSpinel(aRadioSpinel)
        , mOutputStart(nullptr)
        , mOutputEnd(nullptr)
    {
    }

    /**
     * Processes RCP capability diagnostics commands.
     *
     * @param[in]   aArgs           The arguments of diagnostics command line.
     * @param[in]   aArgsLength     The number of arguments in @p aArgs.
     * @param[out]  aOutput         The diagnostics execution result.
     * @param[in]   aOutputMaxLen   The output buffer size.
     *
     * @retval  OT_ERROR_INVALID_ARGS       The command is supported but invalid arguments provided.
     * @retval  OT_ERROR_NONE               The command is successfully processed.
     * @retval  OT_ERROR_INVALID_COMMAND    The command is not valid or not supported.
     *
     */
    otError DiagProcess(char *aArgs[], uint8_t aArgsLength, char *aOutput, size_t aOutputMaxLen);

private:
    template <uint32_t aCommand, spinel_prop_key_t aKey> otError HandleSpinelCommand(void);
    typedef otError (RcpCapsDiag::*SpinelCommandHandler)(void);

    enum Category : uint8_t
    {
        kCategoryBasic,
        kCategoryThread1_1,
        kCategoryThread1_2,
        kCategoryUtils,
        kNumCategories,
    };

    struct SpinelEntry
    {
        Category                          mCategory;
        uint32_t                          mCommand;
        spinel_prop_key_t                 mKey;
        RcpCapsDiag::SpinelCommandHandler mHandler;
    };

    void ProcessSpinel(void);
    void TestSpinelCommands(Category aCategory);
    void OutputResult(const SpinelEntry &aEntry, otError error);
    void Output(const char *aFormat, ...);

    const char *CategoryToString(Category aCategory);

    static const struct SpinelEntry sSpinelEntries[];

    Spinel::RadioSpinel &mRadioSpinel;
    char                *mOutputStart;
    char                *mOutputEnd;
};

} // namespace Posix
} // namespace ot
#endif // OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE
#endif // OT_POSIX_PLATFORM_RCP_CAPS_DIAG_HPP_
