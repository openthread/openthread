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

#include "rcp_caps_diag.hpp"

#if OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE
namespace ot {
namespace Posix {

#define SPINEL_ENTRY(aCategory, aCommand, aKey)                                      \
    {                                                                                \
        aCategory, aCommand, aKey, &RcpCapsDiag::HandleSpinelCommand<aCommand, aKey> \
    }

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_CCA_THRESHOLD>(void)
{
    int8_t ccaThreshold;

    return mRadioSpinel.GetCcaEnergyDetectThreshold(ccaThreshold);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_CHAN>(void)
{
    static constexpr uint8_t kPhyChannel = 22;

    return mRadioSpinel.Set(SPINEL_PROP_PHY_CHAN, SPINEL_DATATYPE_UINT8_S, kPhyChannel);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_CAPS>(void)
{
    static constexpr uint8_t kCapsBufferSize = 100;
    uint8_t                  capsBuffer[kCapsBufferSize];
    spinel_size_t            capsLength = sizeof(capsBuffer);

    return mRadioSpinel.Get(SPINEL_PROP_CAPS, SPINEL_DATATYPE_DATA_S, capsBuffer, &capsLength);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_RCP_ENH_ACK_PROBING>(void)
{
    uint16_t     shortAddress = 0x1122;
    otExtAddress extAddress   = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint8_t      flags        = SPINEL_THREAD_LINK_METRIC_PDU_COUNT | SPINEL_THREAD_LINK_METRIC_LQI |
                    SPINEL_THREAD_LINK_METRIC_LINK_MARGIN | SPINEL_THREAD_LINK_METRIC_RSSI;

    return mRadioSpinel.Set(SPINEL_PROP_RCP_ENH_ACK_PROBING,
                            SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_EUI64_S SPINEL_DATATYPE_UINT8_S, shortAddress,
                            extAddress.m8, flags);
}

const struct RcpCapsDiag::SpinelEntry RcpCapsDiag::sSpinelEntries[] = {
    //  Basic Spinel commands
    SPINEL_ENTRY(kCategoryBasic, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_CAPS),

    // Thread Version >= 1.1
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_CHAN),

    // Thread Version >= 1.2
    SPINEL_ENTRY(kCategoryThread1_2, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_RCP_ENH_ACK_PROBING),

    // Optional Spinel commands
    SPINEL_ENTRY(kCategoryOptional, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_CCA_THRESHOLD),
};

otError RcpCapsDiag::DiagProcess(char *aArgs[], uint8_t aArgsLength)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[1], "spinel") == 0)
    {
        ProcessSpinel();
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

void RcpCapsDiag::ProcessSpinel(void)
{
    for (uint8_t i = 0; i < kNumCategories; i++)
    {
        TestSpinelCommands(static_cast<Category>(i));
    }
}

void RcpCapsDiag::TestSpinelCommands(Category aCategory)
{
    otError error;

    Output("\r\n%s :\r\n", CategoryToString(aCategory));

    for (const SpinelEntry &entry : sSpinelEntries)
    {
        if (entry.mCategory != aCategory)
        {
            continue;
        }

        error = (this->*entry.mHandler)();
        OutputResult(entry, error);
    }
}

void RcpCapsDiag::SetDiagOutputCallback(otPlatDiagOutputCallback aCallback, void *aContext)
{
    mOutputCallback = aCallback;
    mOutputContext  = aContext;
}

void RcpCapsDiag::OutputResult(const SpinelEntry &aEntry, otError error)
{
    static constexpr uint8_t  kSpaceLength            = 1;
    static constexpr uint8_t  kMaxCommandStringLength = 20;
    static constexpr uint8_t  kMaxKeyStringLength     = 35;
    static constexpr uint16_t kMaxLength              = kMaxCommandStringLength + kMaxKeyStringLength + kSpaceLength;
    static const char         kPadding[]              = "----------------------------------------------------------";
    const char               *commandString           = spinel_command_to_cstr(aEntry.mCommand);
    const char               *keyString               = spinel_prop_key_to_cstr(aEntry.mKey);
    uint16_t actualLength  = static_cast<uint16_t>(strlen(commandString) + strlen(keyString) + kSpaceLength);
    uint16_t paddingOffset = (actualLength > kMaxLength) ? kMaxLength : actualLength;

    static_assert(kMaxLength < sizeof(kPadding), "Padding bytes are too short");

    Output("%.*s %.*s %s %s\r\n", kMaxCommandStringLength, commandString, kMaxKeyStringLength, keyString,
           &kPadding[paddingOffset], otThreadErrorToString(error));
}

void RcpCapsDiag::Output(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    if (mOutputCallback != nullptr)
    {
        mOutputCallback(aFormat, args, mOutputContext);
    }

    va_end(args);
}

const char *RcpCapsDiag::CategoryToString(Category aCategory)
{
    static const char *const kCategoryStrings[] = {
        "Basic",                 // (0) kCategoryBasic
        "Thread Version >= 1.1", // (1) kCategoryThread1_1
        "Thread Version >= 1.2", // (2) kCategoryThread1_2
        "Optional",              // (3) kCategoryOptional
    };

    static_assert(kCategoryBasic == 0, "kCategoryBasic value is incorrect");
    static_assert(kCategoryThread1_1 == 1, "kCategoryThread1_1 value is incorrect");
    static_assert(kCategoryThread1_2 == 2, "kCategoryThread1_2 value is incorrect");
    static_assert(kCategoryOptional == 3, "kCategoryOptional value is incorrect");

    return (aCategory < OT_ARRAY_LENGTH(kCategoryStrings)) ? kCategoryStrings[aCategory] : "invalid";
}

} // namespace Posix
} // namespace ot
#endif // OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE
