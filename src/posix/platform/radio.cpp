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

#include "lib/spinel/radio_spinel.hpp"

#if OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_UART
#include "hdlc_interface.hpp"

#if OPENTHREAD_POSIX_VIRTUAL_TIME
static ot::Spinel::RadioSpinel<ot::Posix::HdlcInterface, VirtualTimeEvent> sRadioSpinel;
#else
static ot::Spinel::RadioSpinel<ot::Posix::HdlcInterface, RadioProcessContext> sRadioSpinel;
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME
#elif OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_SPI
#include "spi_interface.hpp"

static ot::Spinel::RadioSpinel<ot::Posix::SpiInterface, RadioProcessContext> sRadioSpinel;
#else
#error "OPENTHREAD_POSIX_CONFIG_RCP_BUS only allows OT_POSIX_RCP_BUS_UART and OT_POSIX_RCP_BUS_SPI!"
#endif

#if OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
#include "posix/platform/max_power_table.hpp"

static ot::Posix::MaxPowerTable sMaxPowerTable;
#endif

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

void platformRadioInit(otUrl *aRadioUrl)
{
    ot::Posix::RadioUrl &radioUrl       = *static_cast<ot::Posix::RadioUrl *>(aRadioUrl);
    bool                 resetRadio     = (radioUrl.GetValue("no-reset") == nullptr);
    bool                 restoreDataset = (radioUrl.GetValue("ncp-dataset") != nullptr);
#if OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
    uint8_t     channel       = ot::Radio::kChannelMin;
    int8_t      power         = ot::Posix::MaxPowerTable::kPowerDefault;
    const char *maxPowerTable = radioUrl.GetValue("max-power-table");

    if (maxPowerTable != nullptr)
    {
        const char *str = nullptr;

        for (str = strtok(const_cast<char *>(maxPowerTable), ","); str != nullptr && channel <= ot::Radio::kChannelMax;
             str = strtok(nullptr, ","))
        {
            power = static_cast<int8_t>(strtol(str, nullptr, 0));
            sMaxPowerTable.SetTransmitPower(channel++, power);
        }

        VerifyOrDie(str == nullptr, OT_EXIT_INVALID_ARGUMENTS);
    }

    // Use the last power if omitted.
    while (channel <= ot::Radio::kChannelMax)
    {
        sMaxPowerTable.SetTransmitPower(channel, power);
        ++channel;
    }
#endif

    SuccessOrDie(sRadioSpinel.GetSpinelInterface().Init(radioUrl));
    sRadioSpinel.Init(resetRadio, restoreDataset);
}

void platformRadioDeinit(void)
{
    sRadioSpinel.Deinit();
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.IsEnabled();
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    return sRadioSpinel.Enable(aInstance);
}

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

#if OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
    if (sRadioSpinel.GetChannel() != aChannel)
    {
        SuccessOrExit(error = otPlatRadioSetTransmitPower(aInstance, sMaxPowerTable.GetTransmitPower(aChannel)));
    }
#endif
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

void platformRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd, struct timeval *aTimeout)
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

        if (remain < static_cast<uint64_t>(aTimeout->tv_sec * US_PER_S + aTimeout->tv_usec))
        {
            aTimeout->tv_sec  = static_cast<time_t>(remain / US_PER_S);
            aTimeout->tv_usec = static_cast<suseconds_t>(remain % US_PER_S);
        }
    }
    else
    {
        aTimeout->tv_sec  = 0;
        aTimeout->tv_usec = 0;
    }

    sRadioSpinel.GetSpinelInterface().UpdateFdSet(*aReadFdSet, *aWriteFdSet, *aMaxFd, *aTimeout);

    if (sRadioSpinel.HasPendingFrame() || sRadioSpinel.IsTransmitDone())
    {
        aTimeout->tv_sec  = 0;
        aTimeout->tv_usec = 0;
    }
}

#if OPENTHREAD_POSIX_VIRTUAL_TIME
void virtualTimeRadioSpinelProcess(otInstance *aInstance, const struct VirtualTimeEvent *aEvent)
{
    OT_UNUSED_VARIABLE(aInstance);
    sRadioSpinel.Process(*aEvent);
}
#else
void platformRadioProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet)
{
    OT_UNUSED_VARIABLE(aInstance);
    RadioProcessContext context = {aReadFdSet, aWriteFdSet};

    sRadioSpinel.Process(context);
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
                          char *      aArgs[],
                          char *      aOutput,
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

bool otPlatDiagModeGet(void)
{
    return sRadioSpinel.IsDiagEnabled();
}

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

void otPlatDiagRadioReceived(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFrame);
    OT_UNUSED_VARIABLE(aError);
}

void otPlatDiagAlarmCallback(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}
#endif // OPENTHREAD_CONFIG_DIAG_ENABLE

uint32_t otPlatRadioGetSupportedChannelMask(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return
#if OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
        sMaxPowerTable.GetAllowedChannelMask() &
#endif
        sRadioSpinel.GetRadioChannelMask(false);
}

uint32_t otPlatRadioGetPreferredChannelMask(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return
#if OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
        sMaxPowerTable.GetAllowedChannelMask() &
#endif
        sRadioSpinel.GetRadioChannelMask(true);
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetState();
}

void otPlatRadioSetMacKey(otInstance *    aInstance,
                          uint8_t         aKeyIdMode,
                          uint8_t         aKeyId,
                          const otMacKey *aPrevKey,
                          const otMacKey *aCurrKey,
                          const otMacKey *aNextKey)
{
    SuccessOrDie(sRadioSpinel.SetMacKey(aKeyIdMode, aKeyId, *aPrevKey, *aCurrKey, *aNextKey));
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioSetMacFrameCounter(otInstance *aInstance, uint32_t aMacFrameCounter)
{
    SuccessOrDie(sRadioSpinel.SetMacFrameCounter(aMacFrameCounter));
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
