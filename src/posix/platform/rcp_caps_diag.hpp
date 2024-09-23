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
#include <openthread/platform/diag.h>

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
     */
    explicit RcpCapsDiag(Spinel::RadioSpinel &aRadioSpinel)
        : mRadioSpinel(aRadioSpinel)
        , mOutputCallback(nullptr)
        , mOutputContext(nullptr)
        , mDiagOutput(nullptr)
        , mDiagOutputLength(0)
    {
    }

    /**
     * Processes RCP capability diagnostics commands.
     *
     * @param[in]   aArgs           The arguments of diagnostics command line.
     * @param[in]   aArgsLength     The number of arguments in @p aArgs.
     *
     * @retval  OT_ERROR_INVALID_ARGS       The command is supported but invalid arguments provided.
     * @retval  OT_ERROR_NONE               The command is successfully processed.
     * @retval  OT_ERROR_INVALID_COMMAND    The command is not valid or not supported.
     */
    otError DiagProcess(char *aArgs[], uint8_t aArgsLength);

    /**
     * Sets the diag output callback.
     *
     * @param[in]  aCallback   A pointer to a function that is called on outputting diag messages.
     * @param[in]  aContext    A user context pointer.
     */
    void SetDiagOutputCallback(otPlatDiagOutputCallback aCallback, void *aContext);

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

    static constexpr uint16_t kMaxNumChildren = 512;

    void ProcessSpinel(void);
    void ProcessSpinelSpeed(void);
    void ProcessCapabilityFlags(void);
    void ProcessSrcMatchTable(void);
    void TestSpinelCommands(Category aCategory);
    void TestRadioCapbilityFlags(void);
    void OutputRadioCapFlags(Category aCategory, uint32_t aRadioCaps, const uint32_t *aFlags, uint16_t aNumbFlags);
    void TestSpinelCapbilityFlags(void);
    void OutputSpinelCapFlags(Category        aCategory,
                              const uint8_t  *aCapsData,
                              spinel_size_t   aCapsLength,
                              const uint32_t *aFlags,
                              uint16_t        aNumbFlags);
    bool IsSpinelCapabilitySupported(const uint8_t *aCapsData, spinel_size_t aCapsLength, uint32_t aCapability);
    void OutputExtendedSrcMatchTableSize(void);
    void OutputShortSrcMatchTableSize(void);

    static void HandleDiagOutput(const char *aFormat, va_list aArguments, void *aContext);
    void        HandleDiagOutput(const char *aFormat, va_list aArguments);

    void OutputFormat(const char *aName, const char *aValue);
    void OutputFormat(const char *aName, uint32_t aValue);
    void OutputResult(const SpinelEntry &aEntry, otError error);
    void Output(const char *aFormat, ...);

    static const char *SupportToString(bool aSupport);
    static const char *RadioCapbilityToString(uint32_t aCapability);
    static const char *CategoryToString(Category aCategory);

    static const struct SpinelEntry sSpinelEntries[];

    Spinel::RadioSpinel     &mRadioSpinel;
    otPlatDiagOutputCallback mOutputCallback;
    void                    *mOutputContext;
    char                    *mDiagOutput;
    uint16_t                 mDiagOutputLength;
};

} // namespace Posix
} // namespace ot
#endif // OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE
#endif // OT_POSIX_PLATFORM_RCP_CAPS_DIAG_HPP_
