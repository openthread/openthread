/*
 *  Copyright (c) 2019, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     noqtice, this list of conditions and the following disclaimer.
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
 *   This file implements the OpenThread platform abstraction for non-volatile storage of settings.
 *
 */

#include <openthread-core-config.h>

#include <stdbool.h>
#include "openthread/platform/settings.h"

#include <utils/code_utils.h>

#include "settings_qorvo.h"

#if !OPENTHREAD_SETTINGS_RAM

/*****************************************************************************
 *                    Public Function Definitions
 *****************************************************************************/

// settings API
void otPlatSettingsInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    qorvoSettingsInit();
}

void otPlatSettingsDeinit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

otError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    otError error = OT_ERROR_NOT_FOUND;
    OT_UNUSED_VARIABLE(aInstance);

    /* we only support multiple entries for the ChildInfo */
    /* Note: removed the assert, since the settings types are protected now */
    // assert((aIndex == 0) || (aKey == ot::Settings::kKeyChildInfo));

    error = qorvoSettingsGet(aKey, aIndex, aValue, aValueLength);

    if (error == OT_ERROR_NOT_FOUND)
    {
        if (aValue != NULL)
        {
            *aValueLength = 0;
        }
    }

    return error;
}

static otError PlatformSettingsAdd(otInstance *   aInstance,
                                   uint16_t       aKey,
                                   bool           aIndex0,
                                   const uint8_t *aValue,
                                   uint16_t       aValueLength)
{
    otError error = OT_ERROR_NONE;
    OT_UNUSED_VARIABLE(aInstance);

    error = qorvoSettingsAdd(aKey, aIndex0, aValue, aValueLength);

    return error;
}

otError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    return PlatformSettingsAdd(aInstance, aKey, true, aValue, aValueLength);
}

otError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    return PlatformSettingsAdd(aInstance, aKey, false, aValue, aValueLength);
}

otError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
    otError error = OT_ERROR_NOT_FOUND;
    OT_UNUSED_VARIABLE(aInstance);

    if (otPlatSettingsGet(aInstance, aKey, 0, NULL, NULL) == OT_ERROR_NONE)
    {
        qorvoSettingsDelete(aKey, aIndex);
        error = OT_ERROR_NONE;
    }

    return error;
}

void otPlatSettingsWipe(otInstance *aInstance)
{
    qorvoSettingsWipe();
    otPlatSettingsInit(aInstance);
}

#endif /* OPENTHREAD_SETTINGS_RAM */
