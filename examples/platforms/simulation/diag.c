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

#include "platform-simulation.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <openthread/config.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

#include "utils/code_utils.h"

#if OPENTHREAD_CONFIG_DIAG_ENABLE

/**
 * Diagnostics mode variables.
 *
 */
static bool sDiagMode = false;

enum
{
    SIM_GPIO = 0,
};

static otGpioMode sGpioMode  = OT_GPIO_MODE_INPUT;
static bool       sGpioValue = false;
static uint8_t    sRawPowerSetting[OPENTHREAD_CONFIG_POWER_CALIBRATION_RAW_POWER_SETTING_SIZE];
static uint16_t   sRawPowerSettingLength = 0;

void otPlatDiagModeSet(bool aMode) { sDiagMode = aMode; }

bool otPlatDiagModeGet(void) { return sDiagMode; }

void otPlatDiagChannelSet(uint8_t aChannel) { OT_UNUSED_VARIABLE(aChannel); }

void otPlatDiagTxPowerSet(int8_t aTxPower) { OT_UNUSED_VARIABLE(aTxPower); }

void otPlatDiagRadioReceived(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFrame);
    OT_UNUSED_VARIABLE(aError);
}

void otPlatDiagAlarmCallback(otInstance *aInstance) { OT_UNUSED_VARIABLE(aInstance); }

otError otPlatDiagGpioSet(uint32_t aGpio, bool aValue)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aGpio == SIM_GPIO, error = OT_ERROR_INVALID_ARGS);
    sGpioValue = aValue;

exit:
    return error;
}

otError otPlatDiagGpioGet(uint32_t aGpio, bool *aValue)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((aGpio == SIM_GPIO) && (aValue != NULL), error = OT_ERROR_INVALID_ARGS);
    *aValue = sGpioValue;

exit:
    return error;
}

otError otPlatDiagGpioSetMode(uint32_t aGpio, otGpioMode aMode)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aGpio == SIM_GPIO, error = OT_ERROR_INVALID_ARGS);
    sGpioMode = aMode;

exit:
    return error;
}

otError otPlatDiagGpioGetMode(uint32_t aGpio, otGpioMode *aMode)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((aGpio == SIM_GPIO) && (aMode != NULL), error = OT_ERROR_INVALID_ARGS);
    *aMode = sGpioMode;

exit:
    return error;
}

otError otPlatDiagRadioSetRawPowerSetting(otInstance    *aInstance,
                                          const uint8_t *aRawPowerSetting,
                                          uint16_t       aRawPowerSettingLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((aRawPowerSetting != NULL) && (aRawPowerSettingLength <= sizeof(sRawPowerSetting)),
                    error = OT_ERROR_INVALID_ARGS);
    memcpy(sRawPowerSetting, aRawPowerSetting, aRawPowerSettingLength);
    sRawPowerSettingLength = aRawPowerSettingLength;

exit:
    return error;
}

otError otPlatDiagRadioGetRawPowerSetting(otInstance *aInstance,
                                          uint8_t    *aRawPowerSetting,
                                          uint16_t   *aRawPowerSettingLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((aRawPowerSetting != NULL) && (aRawPowerSettingLength != NULL), error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION((sRawPowerSettingLength != 0), error = OT_ERROR_NOT_FOUND);
    otEXPECT_ACTION((sRawPowerSettingLength <= *aRawPowerSettingLength), error = OT_ERROR_INVALID_ARGS);

    memcpy(aRawPowerSetting, sRawPowerSetting, sRawPowerSettingLength);
    *aRawPowerSettingLength = sRawPowerSettingLength;

exit:
    return error;
}

otError otPlatDiagRadioRawPowerSettingEnable(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);

    return OT_ERROR_NONE;
}

otError otPlatDiagRadioTransmitCarrier(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);

    return OT_ERROR_NONE;
}

otError otPlatDiagRadioTransmitStream(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);

    return OT_ERROR_NONE;
}
#endif // OPENTHREAD_CONFIG_DIAG_ENABLE
