/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements the radio apis on posix platform.
 */

#include "platform-posix.h"

#include <string.h>

#include <openthread/logging.h>

#include "common/code_utils.hpp"
#include "common/new.hpp"
#include "lib/spinel/radio_spinel.hpp"
#include "posix/platform/radio.hpp"
#include "utils/parse_cmdline.hpp"

#if OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_UART
#include "hdlc_interface.hpp"

static ot::Spinel::RadioSpinel<ot::Posix::HdlcInterface> sRadioSpinel;
#elif OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_SPI
#include "spi_interface.hpp"

static ot::Spinel::RadioSpinel<ot::Posix::SpiInterface> sRadioSpinel;
#elif OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_VENDOR
#include "vendor_interface.hpp"

static ot::Spinel::RadioSpinel<ot::Posix::VendorInterface> sRadioSpinel;
#else
#error "OPENTHREAD_POSIX_CONFIG_RCP_BUS only allows OT_POSIX_RCP_BUS_UART, OT_POSIX_RCP_BUS_SPI and " \
    "OT_POSIX_RCP_BUS_VENDOR!"
#endif

#if OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE
#include "configuration.hpp"
static ot::Posix::Configuration sConfig;
#endif

namespace ot {
namespace Posix {

namespace {
alignas(alignof(ot::Posix::Radio)) char sRadioRaw[sizeof(ot::Posix::Radio)];

extern "C" void platformRadioInit(const char *aUrl)
{
    Radio &radio = *(new (&sRadioRaw) Radio(aUrl));

    radio.Init();
}
} // namespace

Radio::Radio(const char *aUrl)
    : mRadioUrl(aUrl)
{
    VerifyOrDie(mRadioUrl.GetPath() != nullptr, OT_EXIT_INVALID_ARGUMENTS);
}

void Radio::Init(void)
{
    bool        resetRadio             = (mRadioUrl.GetValue("no-reset") == nullptr);
    bool        restoreDataset         = (mRadioUrl.GetValue("ncp-dataset") != nullptr);
    bool        skipCompatibilityCheck = (mRadioUrl.GetValue("skip-rcp-compatibility-check") != nullptr);
    const char *parameterValue;
    const char *region;
#if OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
    const char *maxPowerTable;
#endif

#if OPENTHREAD_POSIX_VIRTUAL_TIME
    // The last argument must be the node id
    {
        const char *nodeId = nullptr;

        for (const char *arg = nullptr; (arg = mRadioUrl.GetValue("forkpty-arg", arg)) != nullptr; nodeId = arg)
        {
        }

        virtualTimeInit(static_cast<uint16_t>(atoi(nodeId)));
    }
#endif

    if (restoreDataset)
    {
        otLogCritPlat("The argument \"ncp-dataset\" is no longer supported");
        DieNow(OT_ERROR_FAILED);
    }

    SuccessOrDie(sRadioSpinel.GetSpinelInterface().Init(mRadioUrl));
    sRadioSpinel.Init(resetRadio, skipCompatibilityCheck);

    parameterValue = mRadioUrl.GetValue("fem-lnagain");
    if (parameterValue != nullptr)
    {
        long femLnaGain = strtol(parameterValue, nullptr, 0);

        VerifyOrDie(INT8_MIN <= femLnaGain && femLnaGain <= INT8_MAX, OT_EXIT_INVALID_ARGUMENTS);
        SuccessOrDie(sRadioSpinel.SetFemLnaGain(static_cast<int8_t>(femLnaGain)));
    }

    parameterValue = mRadioUrl.GetValue("cca-threshold");
    if (parameterValue != nullptr)
    {
        long ccaThreshold = strtol(parameterValue, nullptr, 0);

        VerifyOrDie(INT8_MIN <= ccaThreshold && ccaThreshold <= INT8_MAX, OT_EXIT_INVALID_ARGUMENTS);
        SuccessOrDie(sRadioSpinel.SetCcaEnergyDetectThreshold(static_cast<int8_t>(ccaThreshold)));
    }

    region = mRadioUrl.GetValue("region");
    if (region != nullptr)
    {
        uint16_t regionCode;

        VerifyOrDie(strnlen(region, 3) == 2, OT_EXIT_INVALID_ARGUMENTS);
        regionCode = static_cast<uint16_t>(static_cast<uint16_t>(region[0]) << 8) + static_cast<uint16_t>(region[1]);
        SuccessOrDie(otPlatRadioSetRegion(gInstance, regionCode));
    }

#if OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
    maxPowerTable = mRadioUrl.GetValue("max-power-table");
    if (maxPowerTable != nullptr)
    {
        constexpr int8_t kPowerDefault = 30; // Default power 1 watt (30 dBm).
        const char      *str           = nullptr;
        uint8_t          channel       = ot::Radio::kChannelMin;
        int8_t           power         = kPowerDefault;
        otError          error;

        for (str = strtok(const_cast<char *>(maxPowerTable), ","); str != nullptr && channel <= ot::Radio::kChannelMax;
             str = strtok(nullptr, ","))
        {
            power = static_cast<int8_t>(strtol(str, nullptr, 0));
            error = sRadioSpinel.SetChannelMaxTransmitPower(channel, power);
            if (error != OT_ERROR_NONE && error != OT_ERROR_NOT_IMPLEMENTED)
            {
                DieNow(OT_ERROR_FAILED);
            }
            else if (error == OT_ERROR_NOT_IMPLEMENTED)
            {
                otLogWarnPlat("The RCP doesn't support setting the max transmit power");
            }

            ++channel;
        }

        // Use the last power if omitted.
        while (channel <= ot::Radio::kChannelMax)
        {
            error = sRadioSpinel.SetChannelMaxTransmitPower(channel, power);
            if (error != OT_ERROR_NONE && error != OT_ERROR_NOT_IMPLEMENTED)
            {
                DieNow(OT_ERROR_FAILED);
            }
            else if (error == OT_ERROR_NOT_IMPLEMENTED)
            {
                otLogWarnPlat("The RCP doesn't support setting the max transmit power");
            }

            ++channel;
        }

        VerifyOrDie(str == nullptr, OT_EXIT_INVALID_ARGUMENTS);
    }
#endif // OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
    {
        const char *enableCoex = mRadioUrl.GetValue("enable-coex");
        if (enableCoex != nullptr)
        {
            SuccessOrDie(sRadioSpinel.SetCoexEnabled(enableCoex[0] != '0'));
        }
    }
#endif // OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
}

void *Radio::GetSpinelInstance(void) { return &sRadioSpinel; }

} // namespace Posix
} // namespace ot

void platformRadioDeinit(void) { sRadioSpinel.Deinit(); }

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);
    SuccessOrDie(sRadioSpinel.GetIeeeEui64(aIeeeEui64));
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t panid)
{
    OT_UNUSED_VARIABLE(aInstance);
    SuccessOrDie(sRadioSpinel.SetPanId(panid));
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    otExtAddress addr;

    for (size_t i = 0; i < sizeof(addr); i++)
    {
        addr.m8[i] = aAddress->m8[sizeof(addr) - 1 - i];
    }

    SuccessOrDie(sRadioSpinel.SetExtendedAddress(addr));
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    SuccessOrDie(sRadioSpinel.SetShortAddress(aAddress));
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    SuccessOrDie(sRadioSpinel.SetPromiscuous(aEnable));
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.IsEnabled();
}

otError otPlatRadioEnable(otInstance *aInstance) { return sRadioSpinel.Enable(aInstance); }

otError otPlatRadioDisable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.Disable();
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.Sleep();
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error;

    SuccessOrExit(error = sRadioSpinel.Receive(aChannel));

exit:
    return error;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.Transmit(*aFrame);
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return &sRadioSpinel.GetTransmitFrame();
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetRssi();
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetRadioCaps();
}

const char *otPlatRadioGetVersionString(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetVersion();
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.IsPromiscuous();
}

void platformRadioUpdateFdSet(otSysMainloopContext *aContext)
{
    uint64_t now      = otPlatTimeGet();
    uint64_t deadline = sRadioSpinel.GetNextRadioTimeRecalcStart();

    if (sRadioSpinel.IsTransmitting())
    {
        uint64_t txRadioEndUs = sRadioSpinel.GetTxRadioEndUs();

        if (txRadioEndUs < deadline)
        {
            deadline = txRadioEndUs;
        }
    }

    if (now < deadline)
    {
        uint64_t remain = deadline - now;

        if (remain < (static_cast<uint64_t>(aContext->mTimeout.tv_sec) * US_PER_S +
                      static_cast<uint64_t>(aContext->mTimeout.tv_usec)))
        {
            aContext->mTimeout.tv_sec  = static_cast<time_t>(remain / US_PER_S);
            aContext->mTimeout.tv_usec = static_cast<suseconds_t>(remain % US_PER_S);
        }
    }
    else
    {
        aContext->mTimeout.tv_sec  = 0;
        aContext->mTimeout.tv_usec = 0;
    }

    sRadioSpinel.GetSpinelInterface().UpdateFdSet(aContext);

    if (sRadioSpinel.HasPendingFrame() || sRadioSpinel.IsTransmitDone())
    {
        aContext->mTimeout.tv_sec  = 0;
        aContext->mTimeout.tv_usec = 0;
    }
}

#if OPENTHREAD_POSIX_VIRTUAL_TIME
void virtualTimeRadioSpinelProcess(otInstance *aInstance, const struct VirtualTimeEvent *aEvent)
{
    OT_UNUSED_VARIABLE(aInstance);
    sRadioSpinel.Process(aEvent);
}
#else
void platformRadioProcess(otInstance *aInstance, const otSysMainloopContext *aContext)
{
    OT_UNUSED_VARIABLE(aInstance);

    sRadioSpinel.Process(aContext);
}
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    SuccessOrDie(sRadioSpinel.EnableSrcMatch(aEnable));
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.AddSrcMatchShortEntry(aShortAddress);
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    otExtAddress addr;

    for (size_t i = 0; i < sizeof(addr); i++)
    {
        addr.m8[i] = aExtAddress->m8[sizeof(addr) - 1 - i];
    }

    return sRadioSpinel.AddSrcMatchExtEntry(addr);
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.ClearSrcMatchShortEntry(aShortAddress);
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    otExtAddress addr;

    for (size_t i = 0; i < sizeof(addr); i++)
    {
        addr.m8[i] = aExtAddress->m8[sizeof(addr) - 1 - i];
    }

    return sRadioSpinel.ClearSrcMatchExtEntry(addr);
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    SuccessOrDie(sRadioSpinel.ClearSrcMatchShortEntries());
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    SuccessOrDie(sRadioSpinel.ClearSrcMatchExtEntries());
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.EnergyScan(aScanChannel, aScanDuration);
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    OT_UNUSED_VARIABLE(aInstance);
    assert(aPower != nullptr);
    return sRadioSpinel.GetTransmitPower(*aPower);
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.SetTransmitPower(aPower);
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);
    assert(aThreshold != nullptr);
    return sRadioSpinel.GetCcaEnergyDetectThreshold(*aThreshold);
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.SetCcaEnergyDetectThreshold(aThreshold);
}

otError otPlatRadioGetFemLnaGain(otInstance *aInstance, int8_t *aGain)
{
    OT_UNUSED_VARIABLE(aInstance);
    assert(aGain != nullptr);
    return sRadioSpinel.GetFemLnaGain(*aGain);
}

otError otPlatRadioSetFemLnaGain(otInstance *aInstance, int8_t aGain)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.SetFemLnaGain(aGain);
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetReceiveSensitivity();
}

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
otError otPlatRadioSetCoexEnabled(otInstance *aInstance, bool aEnabled)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.SetCoexEnabled(aEnabled);
}

bool otPlatRadioIsCoexEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.IsCoexEnabled();
}

otError otPlatRadioGetCoexMetrics(otInstance *aInstance, otRadioCoexMetrics *aCoexMetrics)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    VerifyOrExit(aCoexMetrics != nullptr, error = OT_ERROR_INVALID_ARGS);

    error = sRadioSpinel.GetCoexMetrics(*aCoexMetrics);

exit:
    return error;
}
#endif

#if OPENTHREAD_CONFIG_DIAG_ENABLE
otError otPlatDiagProcess(otInstance *aInstance,
                          uint8_t     aArgsLength,
                          char       *aArgs[],
                          char       *aOutput,
                          size_t      aOutputMaxLen)
{
    // deliver the platform specific diags commands to radio only ncp.
    OT_UNUSED_VARIABLE(aInstance);
    char  cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE] = {'\0'};
    char *cur                                              = cmd;
    char *end                                              = cmd + sizeof(cmd);

    for (uint8_t index = 0; (index < aArgsLength) && (cur < end); index++)
    {
        cur += snprintf(cur, static_cast<size_t>(end - cur), "%s ", aArgs[index]);
    }

    return sRadioSpinel.PlatDiagProcess(cmd, aOutput, aOutputMaxLen);
}

void otPlatDiagModeSet(bool aMode)
{
    SuccessOrExit(sRadioSpinel.PlatDiagProcess(aMode ? "start" : "stop", nullptr, 0));
    sRadioSpinel.SetDiagEnabled(aMode);

exit:
    return;
}

bool otPlatDiagModeGet(void) { return sRadioSpinel.IsDiagEnabled(); }

void otPlatDiagTxPowerSet(int8_t aTxPower)
{
    char cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "power %d", aTxPower);
    SuccessOrExit(sRadioSpinel.PlatDiagProcess(cmd, nullptr, 0));

exit:
    return;
}

void otPlatDiagChannelSet(uint8_t aChannel)
{
    char cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "channel %d", aChannel);
    SuccessOrExit(sRadioSpinel.PlatDiagProcess(cmd, nullptr, 0));

exit:
    return;
}

otError otPlatDiagGpioSet(uint32_t aGpio, bool aValue)
{
    otError error;
    char    cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "gpio set %d %d", aGpio, aValue);
    SuccessOrExit(error = sRadioSpinel.PlatDiagProcess(cmd, nullptr, 0));

exit:
    return error;
}

otError otPlatDiagGpioGet(uint32_t aGpio, bool *aValue)
{
    otError error;
    char    cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];
    char    output[OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE];
    char   *str;

    snprintf(cmd, sizeof(cmd), "gpio get %d", aGpio);
    SuccessOrExit(error = sRadioSpinel.PlatDiagProcess(cmd, output, sizeof(output)));
    VerifyOrExit((str = strtok(output, "\r")) != nullptr, error = OT_ERROR_FAILED);
    *aValue = static_cast<bool>(atoi(str));

exit:
    return error;
}

otError otPlatDiagGpioSetMode(uint32_t aGpio, otGpioMode aMode)
{
    otError error;
    char    cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "gpio mode %d %s", aGpio, aMode == OT_GPIO_MODE_INPUT ? "in" : "out");
    SuccessOrExit(error = sRadioSpinel.PlatDiagProcess(cmd, nullptr, 0));

exit:
    return error;
}

otError otPlatDiagGpioGetMode(uint32_t aGpio, otGpioMode *aMode)
{
    otError error;
    char    cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];
    char    output[OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE];
    char   *str;

    snprintf(cmd, sizeof(cmd), "gpio mode %d", aGpio);
    SuccessOrExit(error = sRadioSpinel.PlatDiagProcess(cmd, output, sizeof(output)));
    VerifyOrExit((str = strtok(output, "\r")) != nullptr, error = OT_ERROR_FAILED);

    if (strcmp(str, "in") == 0)
    {
        *aMode = OT_GPIO_MODE_INPUT;
    }
    else if (strcmp(str, "out") == 0)
    {
        *aMode = OT_GPIO_MODE_OUTPUT;
    }
    else
    {
        error = OT_ERROR_FAILED;
    }

exit:
    return error;
}

otError otPlatDiagRadioGetPowerSettings(otInstance *aInstance,
                                        uint8_t     aChannel,
                                        int16_t    *aTargetPower,
                                        int16_t    *aActualPower,
                                        uint8_t    *aRawPowerSetting,
                                        uint16_t   *aRawPowerSettingLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    static constexpr uint16_t kRawPowerStringSize = OPENTHREAD_CONFIG_POWER_CALIBRATION_RAW_POWER_SETTING_SIZE * 2 + 1;
    static constexpr uint16_t kFmtStringSize      = 100;

    otError error;
    char    cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];
    char    output[OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE];
    int     targetPower;
    int     actualPower;
    char    rawPowerSetting[kRawPowerStringSize];
    char    fmt[kFmtStringSize];

    assert((aTargetPower != nullptr) && (aActualPower != nullptr) && (aRawPowerSetting != nullptr) &&
           (aRawPowerSettingLength != nullptr));

    snprintf(cmd, sizeof(cmd), "powersettings %d", aChannel);
    SuccessOrExit(error = sRadioSpinel.PlatDiagProcess(cmd, output, sizeof(output)));
    snprintf(fmt, sizeof(fmt), "TargetPower(0.01dBm): %%d\r\nActualPower(0.01dBm): %%d\r\nRawPowerSetting: %%%us\r\n",
             kRawPowerStringSize);
    VerifyOrExit(sscanf(output, fmt, &targetPower, &actualPower, rawPowerSetting) == 3, error = OT_ERROR_FAILED);
    SuccessOrExit(
        error = ot::Utils::CmdLineParser::ParseAsHexString(rawPowerSetting, *aRawPowerSettingLength, aRawPowerSetting));
    *aTargetPower = static_cast<int16_t>(targetPower);
    *aActualPower = static_cast<int16_t>(actualPower);

exit:
    return error;
}

otError otPlatDiagRadioSetRawPowerSetting(otInstance    *aInstance,
                                          const uint8_t *aRawPowerSetting,
                                          uint16_t       aRawPowerSettingLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error;
    char    cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];
    int     nbytes;

    assert(aRawPowerSetting != nullptr);

    nbytes = snprintf(cmd, sizeof(cmd), "rawpowersetting ");

    for (uint16_t i = 0; i < aRawPowerSettingLength; i++)
    {
        nbytes += snprintf(cmd + nbytes, sizeof(cmd) - static_cast<size_t>(nbytes), "%02x", aRawPowerSetting[i]);
        VerifyOrExit(nbytes < static_cast<int>(sizeof(cmd)), error = OT_ERROR_INVALID_ARGS);
    }

    SuccessOrExit(error = sRadioSpinel.PlatDiagProcess(cmd, nullptr, 0));

exit:
    return error;
}

otError otPlatDiagRadioGetRawPowerSetting(otInstance *aInstance,
                                          uint8_t    *aRawPowerSetting,
                                          uint16_t   *aRawPowerSettingLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    otError error;
    char    cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];
    char    output[OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE];
    char   *str;

    assert((aRawPowerSetting != nullptr) && (aRawPowerSettingLength != nullptr));

    snprintf(cmd, sizeof(cmd), "rawpowersetting");
    SuccessOrExit(error = sRadioSpinel.PlatDiagProcess(cmd, output, sizeof(output)));
    VerifyOrExit((str = strtok(output, "\r")) != nullptr, error = OT_ERROR_FAILED);
    SuccessOrExit(error = ot::Utils::CmdLineParser::ParseAsHexString(str, *aRawPowerSettingLength, aRawPowerSetting));

exit:
    return error;
}

otError otPlatDiagRadioRawPowerSettingEnable(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error;
    char    cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "rawpowersetting %s", aEnable ? "enable" : "disable");
    SuccessOrExit(error = sRadioSpinel.PlatDiagProcess(cmd, nullptr, 0));

exit:
    return error;
}

otError otPlatDiagRadioTransmitCarrier(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error;
    char    cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "cw %s", aEnable ? "start" : "stop");
    SuccessOrExit(error = sRadioSpinel.PlatDiagProcess(cmd, nullptr, 0));

exit:
    return error;
}

otError otPlatDiagRadioTransmitStream(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    char cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "stream %s", aEnable ? "start" : "stop");
    return sRadioSpinel.PlatDiagProcess(cmd, nullptr, 0);
}

void otPlatDiagRadioReceived(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFrame);
    OT_UNUSED_VARIABLE(aError);
}

void otPlatDiagAlarmCallback(otInstance *aInstance) { OT_UNUSED_VARIABLE(aInstance); }
#endif // OPENTHREAD_CONFIG_DIAG_ENABLE

uint32_t otPlatRadioGetSupportedChannelMask(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
#if OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE
    return sConfig.GetSupportedChannelMask();
#else
    return sRadioSpinel.GetRadioChannelMask(false);
#endif
}

uint32_t otPlatRadioGetPreferredChannelMask(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
#if OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE
    return sConfig.GetPreferredChannelMask();
#else
    return sRadioSpinel.GetRadioChannelMask(true);
#endif
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetState();
}

void otPlatRadioSetMacKey(otInstance             *aInstance,
                          uint8_t                 aKeyIdMode,
                          uint8_t                 aKeyId,
                          const otMacKeyMaterial *aPrevKey,
                          const otMacKeyMaterial *aCurrKey,
                          const otMacKeyMaterial *aNextKey,
                          otRadioKeyType          aKeyType)
{
    SuccessOrDie(sRadioSpinel.SetMacKey(aKeyIdMode, aKeyId, aPrevKey, aCurrKey, aNextKey));
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aKeyType);
}

void otPlatRadioSetMacFrameCounter(otInstance *aInstance, uint32_t aMacFrameCounter)
{
    SuccessOrDie(sRadioSpinel.SetMacFrameCounter(aMacFrameCounter, /* aSetIfLarger */ false));
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioSetMacFrameCounterIfLarger(otInstance *aInstance, uint32_t aMacFrameCounter)
{
    SuccessOrDie(sRadioSpinel.SetMacFrameCounter(aMacFrameCounter, /* aSetIfLarger */ true));
    OT_UNUSED_VARIABLE(aInstance);
}

uint64_t otPlatRadioGetNow(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetNow();
}

uint32_t otPlatRadioGetBusSpeed(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetBusSpeed();
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
uint8_t otPlatRadioGetCslAccuracy(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sRadioSpinel.GetCslAccuracy();
}
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
uint8_t otPlatRadioGetCslUncertainty(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sRadioSpinel.GetCslUncertainty();
}
#endif

otError otPlatRadioSetChannelMaxTransmitPower(otInstance *aInstance, uint8_t aChannel, int8_t aMaxPower)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.SetChannelMaxTransmitPower(aChannel, aMaxPower);
}

#if OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE
otError otPlatRadioAddCalibratedPower(otInstance    *aInstance,
                                      uint8_t        aChannel,
                                      int16_t        aActualPower,
                                      const uint8_t *aRawPowerSetting,
                                      uint16_t       aRawPowerSettingLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.AddCalibratedPower(aChannel, aActualPower, aRawPowerSetting, aRawPowerSettingLength);
}

otError otPlatRadioClearCalibratedPowers(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.ClearCalibratedPowers();
}

otError otPlatRadioSetChannelTargetPower(otInstance *aInstance, uint8_t aChannel, int16_t aTargetPower)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.SetChannelTargetPower(aChannel, aTargetPower);
}
#endif

otError otPlatRadioSetRegion(otInstance *aInstance, uint16_t aRegionCode)
{
    OT_UNUSED_VARIABLE(aInstance);
#if OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE
    return sConfig.SetRegion(aRegionCode);
#else
    return sRadioSpinel.SetRadioRegion(aRegionCode);
#endif
}

otError otPlatRadioGetRegion(otInstance *aInstance, uint16_t *aRegionCode)
{
    OT_UNUSED_VARIABLE(aInstance);
#if OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE
    *aRegionCode = sConfig.GetRegion();
    return OT_ERROR_NONE;
#else
    return sRadioSpinel.GetRadioRegion(aRegionCode);
#endif
}

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
otError otPlatRadioConfigureEnhAckProbing(otInstance          *aInstance,
                                          otLinkMetrics        aLinkMetrics,
                                          const otShortAddress aShortAddress,
                                          const otExtAddress  *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sRadioSpinel.ConfigureEnhAckProbing(aLinkMetrics, aShortAddress, *aExtAddress);
}
#endif

otError otPlatRadioReceiveAt(otInstance *aInstance, uint8_t aChannel, uint32_t aStart, uint32_t aDuration)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aChannel);
    OT_UNUSED_VARIABLE(aStart);
    OT_UNUSED_VARIABLE(aDuration);
    return OT_ERROR_NOT_IMPLEMENTED;
}

const otRadioSpinelMetrics *otSysGetRadioSpinelMetrics(void) { return sRadioSpinel.GetRadioSpinelMetrics(); }

const otRcpInterfaceMetrics *otSysGetRcpInterfaceMetrics(void)
{
    return sRadioSpinel.GetSpinelInterface().GetRcpInterfaceMetrics();
}
