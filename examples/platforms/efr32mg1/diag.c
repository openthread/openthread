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
 *   This file implements the OpenThread platform abstraction for the diagnostics.
 *
 */

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <openthread/cli.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/config.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/toolchain.h>

#include "platform-efr32.h"
#include <common/logging.hpp>
#include <utils/code_utils.h>
#include "btl_interface.h"
#include "rail.h"
#include "rail_config.h"
#include "rail_types.h"
#include "retargetserial.h"

#if OPENTHREAD_CONFIG_DIAG_ENABLE

struct PlatformDiagCommand
{
    const char *mName;
    otError (*mCommand)(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen);
};

struct PlatformDiagMessage
{
    const char mMessageDescriptor[11];
    uint8_t    mChannel;
    int16_t    mID;
    uint32_t   mCnt;
};

static otError parseLong(char *aArgs, long *aValue)
{
    char *endptr;
    *aValue = strtol(aArgs, &endptr, 0);
    return (*endptr == '\0') ? OT_ERROR_NONE : OT_ERROR_PARSE;
}

static void appendErrorResult(otError aError, char *aOutput, size_t aOutputMaxLen)
{
    if (aError != OT_ERROR_NONE)
    {
        snprintf(aOutput, aOutputMaxLen, "failed\r\nstatus %#x\r\n", aError);
    }
}

/**
 * Diagnostics mode variables.
 *
 */
static bool sDiagMode = false;

void otPlatDiagModeSet(bool aMode)
{
    sDiagMode = aMode;
}

static void setTimer(RAIL_MultiTimer_t * timer,
                     uint32_t time,
                     RAIL_MultiTimerCallback_t cb)
{
  if (!RAIL_IsMultiTimerRunning(timer)) {
    RAIL_SetMultiTimer(timer,
                       time,
                       RAIL_TIME_DELAY,
                       cb,
                       NULL);
  }
}

static RAIL_MultiTimer_t bltimer;

static void timerCb(RAIL_MultiTimer_t *tmr,
                    RAIL_Time_t expectedTimeOfEvent,
                    void *cbArg)
{
	bootloader_init();
        bootloader_rebootAndInstall();
}

static otError processLaunchGeckoBootloader(otInstance *aInstance,
                           uint8_t     aArgsLength,
                           char *      aArgs[],
                           char *      aOutput,
                           size_t      aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (aArgsLength == 0)
    {
         setTimer(&bltimer, 50, &timerCb);
    }
    else
    {
        long value;

        error = parseLong(aArgs[0], &value);
        otEXPECT(error == OT_ERROR_NONE);
        otEXPECT_ACTION(value >= 0, error = OT_ERROR_INVALID_ARGS);
        snprintf(aOutput, aOutputMaxLen, "invalid command\r\n");
    }

exit:
    appendErrorResult(error, aOutput, aOutputMaxLen);
    return error;
}

static otError processEeroVersion(otInstance *aInstance,
                           uint8_t     aArgsLength,
                           char *      aArgs[],
                           char *      aOutput,
                           size_t      aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (aArgsLength == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "v5.0.0.0.0\r\n");
    }
    else
    {
        long value;

        error = parseLong(aArgs[0], &value);
        otEXPECT(error == OT_ERROR_NONE);
        otEXPECT_ACTION(value >= 0, error = OT_ERROR_INVALID_ARGS);
        snprintf(aOutput, aOutputMaxLen, "invalid command\r\n");
    }

exit:
    appendErrorResult(error, aOutput, aOutputMaxLen);
    return error;
}


const struct PlatformDiagCommand sCommands[] = {
    {"launchbootloader", &processLaunchGeckoBootloader},
    {"eeroversion", &processEeroVersion},
};

otError otPlatDiagProcess(otInstance *aInstance,
                          uint8_t     aArgsLength,
                          char *      aArgs[],
                          char *      aOutput,
                          size_t      aOutputMaxLen)
{
    otError error = OT_ERROR_INVALID_COMMAND;
    size_t  i;

    for (i = 0; i < otARRAY_LENGTH(sCommands); i++)
    {
        if (strcmp(aArgs[0], sCommands[i].mName) == 0)
        {
            error = sCommands[i].mCommand(aInstance, aArgsLength - 1, aArgsLength > 1 ? &aArgs[1] : NULL, aOutput,
                                          aOutputMaxLen);
            break;
        }
    }

    return error;
}

bool otPlatDiagModeGet()
{
    return sDiagMode;
}

void otPlatDiagChannelSet(uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aChannel);
}

void otPlatDiagTxPowerSet(int8_t aTxPower)
{
    OT_UNUSED_VARIABLE(aTxPower);
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

#endif // #if OPENTHREAD_CONFIG_DIAG_ENABLE
