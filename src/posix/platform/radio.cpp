/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements the radio driver that delegates all radio interactions with OpenThread core.
 */

#include "radio.h"

#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

#include "platform-posix.h"

#include "radio_bblink.hpp"
#include "radio_spinel.hpp"

static ot::PosixApp::RadioSpinel sRadioSpinel;
#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
static ot::PosixApp::RadioBackboneLink sRadioBackboneLink;
#endif

static uint8_t sTxWait    = 0;
static uint8_t sState     = OT_RADIO_STATE_DISABLED;
static bool    sTxStarted = false;

void platformOnRadioTxStarted(otInstance *aInstance, const otRadioFrame *aFrame)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFrame);

    if (!sTxStarted)
    {
        otPlatRadioTxStarted(aInstance, const_cast<otRadioFrame *>(aFrame));
        sTxStarted = true;
    }
}

void platformOnRadioTxDone(otInstance *aInstance, const otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError)
{
    assert(sTxWait > 0);

    --sTxWait;

    if (sTxWait == 0)
    {
        sTxStarted = false;
        otPlatRadioTxDone(aInstance, const_cast<otRadioFrame *>(aFrame), aAckFrame, aError);
    }
}

void platformOnRadioRxDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    otPlatRadioReceiveDone(aInstance, aFrame, aError);
}

static void TxInfoFromRadioInfo(const otRadioInfo *aRadioInfo, bool *aLink802154, bool *aLinkBackbone, bool *aMulticast)
{
    bool multicast = (aRadioInfo->mFields.m64[0] == -1ULL && aRadioInfo->mFields.m64[1] == -1ULL);

    if (aLink802154 != NULL)
    {
        *aLink802154 = multicast || (aRadioInfo->mFields.m64[0] == 0 && aRadioInfo->mFields.m64[1] == 0);
    }

    if (aLinkBackbone != NULL)
    {
        *aLinkBackbone = multicast || (aRadioInfo->mFields.m64[0] != 0 || aRadioInfo->mFields.m64[1] != 0);
    }

    if (aMulticast != NULL)
    {
        *aMulticast = multicast;
    }
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);

    SuccessOrDie(sRadioSpinel.GetIeeeEui64(aIeeeEui64));
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t panid)
{
    OT_UNUSED_VARIABLE(aInstance);

#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
    sRadioBackboneLink.SetPanId(panid);
#endif
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

#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
    sRadioBackboneLink.SetExtendedAddress(addr);
#endif
    SuccessOrDie(sRadioSpinel.SetExtendedAddress(addr));
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
    sRadioBackboneLink.SetShortAddress(aAddress);
#endif
    SuccessOrDie(sRadioSpinel.SetShortAddress(aAddress));
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    SuccessOrDie(sRadioSpinel.SetPromiscuous(aEnable));
    OT_UNUSED_VARIABLE(aInstance);
}

void platformRadioInit(const char *aRadioFile, const char *aRadioConfig, bool aReset, const char *aBackboneLink)
{
    OT_UNUSED_VARIABLE(aBackboneLink);
#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
    sRadioBackboneLink.Init(aBackboneLink);
#endif
    sRadioSpinel.Init(aRadioFile, aRadioConfig, aReset);
}

void platformRadioDeinit(void)
{
#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
    sRadioBackboneLink.Deinit();
#endif
    sRadioSpinel.Deinit();
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return
#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
        sRadioBackboneLink.IsEnabled() ||
#endif
        sRadioSpinel.IsEnabled();
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    otError error;

#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
    sRadioBackboneLink.Enable(aInstance);
#endif
    SuccessOrExit(error = sRadioSpinel.Enable(aInstance));

exit:
    if (error == OT_ERROR_NONE)
    {
        sState = OT_RADIO_STATE_SLEEP;
    }

    return error;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    otError error;

#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
    sRadioBackboneLink.Disable();
#endif
    SuccessOrExit(error = sRadioSpinel.Disable());

exit:
    if (error == OT_ERROR_NONE)
    {
        sState = OT_RADIO_STATE_DISABLED;
    }

    return error;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error;

#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
    SuccessOrExit(error = sRadioBackboneLink.Sleep());
#endif
    SuccessOrExit(error = sRadioSpinel.Sleep());

exit:
    if (error == OT_ERROR_NONE)
    {
        sState = OT_RADIO_STATE_SLEEP;
    }

    return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error;

#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
    SuccessOrExit(error = sRadioBackboneLink.Receive(aChannel));
#endif

    SuccessOrExit(error = sRadioSpinel.Receive(aChannel));

    sState  = OT_RADIO_STATE_RECEIVE;
    sTxWait = 0;

exit:
    return error;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    OT_UNUSED_VARIABLE(aInstance);
    bool    to802154;
    bool    toBackbone;
    bool    multicast;
    otError error = OT_ERROR_NONE;

    VerifyOrExit(sState == OT_RADIO_STATE_RECEIVE, error = OT_ERROR_INVALID_STATE);
    assert(sTxWait == 0);

    sTxStarted = false;
    TxInfoFromRadioInfo(&aFrame->mRadioInfo, &to802154, &toBackbone, &multicast);

    if (to802154)
    {
        if ((error = sRadioSpinel.Transmit(*aFrame)) == OT_ERROR_NONE)
        {
            ++sTxWait;
        }
    }

#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
    if (toBackbone)
    {
        otError err = error;

        if ((error = sRadioBackboneLink.Transmit(*aFrame)) == OT_ERROR_NONE)
        {
            ++sTxWait;
        }
        else // either succeeds is OK
        {
            error = err;
        }
    }
#endif

    if (error == OT_ERROR_NONE)
    {
        sState = OT_RADIO_STATE_TRANSMIT;
    }

exit:

    return error;
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
#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
    sRadioBackboneLink.UpdateFdSet(*aReadFdSet, *aWriteFdSet, *aMaxFd, *aTimeout);
#endif
    sRadioSpinel.UpdateFdSet(*aReadFdSet, *aWriteFdSet, *aMaxFd, *aTimeout);
}

void platformRadioProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet)
{
    OT_UNUSED_VARIABLE(aInstance);

#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
    sRadioBackboneLink.Process(*aReadFdSet, *aWriteFdSet);
#endif
    sRadioSpinel.Process(*aReadFdSet, *aWriteFdSet);
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    SuccessOrDie(sRadioSpinel.EnableSrcMatch(aEnable));
    OT_UNUSED_VARIABLE(aInstance);
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
    assert(aPower != NULL);
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetTransmitPower(*aPower);
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.SetTransmitPower(aPower);
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetReceiveSensitivity();
}

#if OPENTHREAD_CONFIG_DIAG_ENABLE
void otPlatDiagProcess(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    // deliver the platform specific diags commands to radio only ncp.
    OT_UNUSED_VARIABLE(aInstance);
    char  cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE] = {'\0'};
    char *cur                                              = cmd;
    char *end                                              = cmd + sizeof(cmd);

    for (int index = 0; index < argc; index++)
    {
        cur += snprintf(cur, static_cast<size_t>(end - cur), "%s ", argv[index]);
    }

    sRadioSpinel.PlatDiagProcess(cmd, aOutput, aOutputMaxLen);
}

void otPlatDiagModeSet(bool aMode)
{
    SuccessOrExit(sRadioSpinel.PlatDiagProcess(aMode ? "start" : "stop", NULL, 0));
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
    SuccessOrExit(sRadioSpinel.PlatDiagProcess(cmd, NULL, 0));

exit:
    return;
}

void otPlatDiagChannelSet(uint8_t aChannel)
{
    char cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "channel %d", aChannel);
    SuccessOrExit(sRadioSpinel.PlatDiagProcess(cmd, NULL, 0));

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
    return sRadioSpinel.GetRadioChannelMask(false);
}

uint32_t otPlatRadioGetPreferredChannelMask(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetRadioChannelMask(true);
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sRadioSpinel.GetCcaEnergyDetectThreshold(*aThreshold);
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sRadioSpinel.SetCcaEnergyDetectThreshold(aThreshold);
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

    VerifyOrExit(aCoexMetrics != NULL, error = OT_ERROR_INVALID_ARGS);

    error = sRadioSpinel.GetCoexMetrics(*aCoexMetrics);

exit:
    return error;
}
#endif

#if OPENTHREAD_POSIX_VIRTUAL_TIME
void platformSimRadioSpinelUpdate(struct timeval *aTimeout)
{
    sRadioSpinel.Update(*aTimeout);
}

void platformSimRadioSpinelProcess(otInstance *aInstance, const struct Event *aEvent)
{
    sRadioSpinel.Process(*aEvent);
    OT_UNUSED_VARIABLE(aInstance);
}
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME
