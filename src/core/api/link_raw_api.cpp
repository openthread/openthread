/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements the OpenThread Link Raw API.
 */

#include "openthread-core-config.h"

#include <string.h>
#include <openthread/diag.h>
#include <openthread/platform/diag.h>

#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "common/random.hpp"
#include "mac/mac.hpp"
#include "mac/mac_frame.hpp"
#include "utils/parse_cmdline.hpp"

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

using namespace ot;

otError otLinkRawSetEnable(otInstance *aInstance, bool aEnabled)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().SetEnabled(aEnabled);
}

bool otLinkRawIsEnabled(otInstance *aInstance)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled();
}

otError otLinkRawSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().SetShortAddress(aShortAddress);
}

bool otLinkRawGetPromiscuous(otInstance *aInstance)
{
    return otPlatRadioGetPromiscuous(aInstance);
}

otError otLinkRawSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otLogInfoPlat("LinkRaw Promiscuous=%d", aEnable ? 1 : 0);

    otPlatRadioSetPromiscuous(aInstance, aEnable);

exit:
    return error;
}

otError otLinkRawSleep(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otLogInfoPlat("LinkRaw Sleep");

    error = otPlatRadioSleep(aInstance);

exit:
    return error;
}

otError otLinkRawReceive(otInstance *aInstance, otLinkRawReceiveDone aCallback)
{
    otLogInfoPlat("LinkRaw Recv");
    return static_cast<Instance *>(aInstance)->GetLinkRaw().Receive(aCallback);
}

otRadioFrame *otLinkRawGetTransmitBuffer(otInstance *aInstance)
{
    return &static_cast<Instance *>(aInstance)->GetLinkRaw().GetTransmitFrame();
}

otError otLinkRawTransmit(otInstance *aInstance, otLinkRawTransmitDone aCallback)
{
    otLogInfoPlat("LinkRaw Transmit");
    return static_cast<Instance *>(aInstance)->GetLinkRaw().Transmit(aCallback);
}

int8_t otLinkRawGetRssi(otInstance *aInstance)
{
    return otPlatRadioGetRssi(aInstance);
}

otRadioCaps otLinkRawGetCaps(otInstance *aInstance)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().GetCaps();
}

otError otLinkRawEnergyScan(otInstance *            aInstance,
                            uint8_t                 aScanChannel,
                            uint16_t                aScanDuration,
                            otLinkRawEnergyScanDone aCallback)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().EnergyScan(aScanChannel, aScanDuration, aCallback);
}

otError otLinkRawSrcMatchEnable(otInstance *aInstance, bool aEnable)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioEnableSrcMatch(aInstance, aEnable);

exit:
    return error;
}

otError otLinkRawSrcMatchAddShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    error = otPlatRadioAddSrcMatchShortEntry(aInstance, aShortAddress);

exit:
    return error;
}

otError otLinkRawSrcMatchAddExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    Mac::Address address;
    otError      error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    address.SetExtended(aExtAddress->m8, /* aReverse */ true);

    error = otPlatRadioAddSrcMatchExtEntry(aInstance, &address.GetExtended());

exit:
    return error;
}

otError otLinkRawSrcMatchClearShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    error = otPlatRadioClearSrcMatchShortEntry(aInstance, aShortAddress);

exit:
    return error;
}

otError otLinkRawSrcMatchClearExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    Mac::Address address;
    otError      error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    address.SetExtended(aExtAddress->m8, /* aReverse */ true);

    error = otPlatRadioClearSrcMatchExtEntry(aInstance, &address.GetExtended());

exit:
    return error;
}

otError otLinkRawSrcMatchClearShortEntries(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioClearSrcMatchShortEntries(aInstance);

exit:
    return error;
}

otError otLinkRawSrcMatchClearExtEntries(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioClearSrcMatchExtEntries(aInstance);

exit:
    return error;
}

#if OPENTHREAD_RADIO

otDeviceRole otThreadGetDeviceRole(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_DEVICE_ROLE_DISABLED;
}

uint8_t otLinkGetChannel(otInstance *aInstance)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().GetChannel();
}

otError otLinkSetChannel(otInstance *aInstance, uint8_t aChannel)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().SetChannel(aChannel);
}

otPanId otLinkGetPanId(otInstance *aInstance)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().GetPanId();
}

otError otLinkSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().SetPanId(aPanId);
}

const otExtAddress *otLinkGetExtendedAddress(otInstance *aInstance)
{
    return &static_cast<Instance *>(aInstance)->GetLinkRaw().GetExtAddress();
}

otError otLinkSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().SetExtAddress(
        *static_cast<const Mac::ExtAddress *>(aExtAddress));
}

uint16_t otLinkGetShortAddress(otInstance *aInstance)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().GetShortAddress();
}

#if OPENTHREAD_ENABLE_DIAG
static otInstance *sDiagInstance;

void otDiagInit(otInstance *aInstance)
{
    sDiagInstance = aInstance;
}

void otDiagProcessCmdLine(const char *aString, char *aOutput, size_t aOutputMaxLen)
{
    enum
    {
        kMaxArgs          = OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX,
        kMaxCommandBuffer = OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE,
    };

    otError error = OT_ERROR_NONE;
    char    buffer[kMaxCommandBuffer];
    char *  argVector[kMaxArgs];
    uint8_t argCount = 0;

    VerifyOrExit(strnlen(aString, kMaxCommandBuffer) < kMaxCommandBuffer, error = OT_ERROR_NO_BUFS);

    strcpy(buffer, aString);
    SuccessOrExit(error = Utils::CmdLineParser::ParseCmd(buffer, argCount, argVector, kMaxArgs));
    VerifyOrExit(argCount >= 1, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argVector[0], "power") == 0)
    {
        char * endptr;
        int8_t power;

        VerifyOrExit(argCount == 2, error = OT_ERROR_INVALID_ARGS);
        power = static_cast<int8_t>(strtol(argVector[1], &endptr, 0));
        VerifyOrExit(*endptr == '\0', error = OT_ERROR_INVALID_ARGS);

        otPlatDiagTxPowerSet(power);
    }
    else if (strcmp(argVector[0], "channel") == 0)
    {
        char *  endptr;
        uint8_t channel;

        VerifyOrExit(argCount == 2, error = OT_ERROR_INVALID_ARGS);
        channel = static_cast<uint8_t>(strtol(argVector[1], &endptr, 0));
        VerifyOrExit(*endptr == '\0', error = OT_ERROR_INVALID_ARGS);

        otPlatDiagChannelSet(channel);
    }
    else if (strcmp(argVector[0], "start") == 0)
    {
        otPlatDiagModeSet(true);
    }
    else if (strcmp(argVector[0], "stop") == 0)
    {
        otPlatDiagModeSet(false);
    }
    else
    {
        otPlatDiagProcess(sDiagInstance, argCount, argVector, aOutput, aOutputMaxLen);
    }

exit:
    switch (error)
    {
    case OT_ERROR_NONE:
        break;

    default:
        snprintf(aOutput, aOutputMaxLen, "failed: invalid command: %s\r\n", otThreadErrorToString(error));
        break;
    }
}

extern "C" void otPlatDiagAlarmFired(otInstance *aInstance)
{
    otPlatDiagAlarmCallback(aInstance);
}

extern "C" void otPlatDiagRadioTransmitDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    // notify OpenThread Diags module on host side
    otPlatRadioTxDone(aInstance, aFrame, NULL, aError);
}

extern "C" void otPlatDiagRadioReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    // notify OpenThread Diags module on host side
    otPlatRadioReceiveDone(aInstance, aFrame, aError);
}
#endif // OPENTHREAD_ENABLE_DIAG

#endif // OPENTHREAD_RADIO

#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
