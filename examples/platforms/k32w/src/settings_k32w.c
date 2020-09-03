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
 *   This file implements the OpenThread platform abstraction for non-volatile storage of
 *   settings on K32W platform. It has been modified and optimized from the original
 *   Open Thread settings implementation to work with K32W's flash particularities.
 *
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <openthread-core-config.h>

#include <openthread/instance.h>
#include <openthread/platform/settings.h>

#include <string.h>
#include "utils/code_utils.h"

#include "EmbeddedTypes.h"
#include "PDM.h"

#define pdmBufferSize 512
#define NVM_START_ID 0x4F00

/* WARNING - the defines below must be in sync with OT NVM datasets from Settings.hpp */
#define NVM_MAX_ID 7

static uint8_t sPdmBuffer[pdmBufferSize] __attribute__((aligned(4))) = {0};

static otError addSetting(otInstance *   aInstance,
                          uint16_t       aKey,
                          bool           aIndex0,
                          const uint8_t *aValue,
                          uint16_t       aValueLength);

static otError addSetting(otInstance *   aInstance,
                          uint16_t       aKey,
                          bool           aIndex0,
                          const uint8_t *aValue,
                          uint16_t       aValueLength)

{
    otError      error = OT_ERROR_NONE;
    PDM_teStatus pdmStatus;
    uint16_t     bytesRead;

    otEXPECT_ACTION((pdmBufferSize > aValueLength + sizeof(uint16_t)), error = OT_ERROR_NO_BUFS);

    if (aIndex0)
    {
        /* save the lenght of the first record element at the start of the record so we can know
           if the record contains multiple entries of a size or just a single entry */
        memcpy(sPdmBuffer, (uint8_t *)&aValueLength, sizeof(uint16_t));
        memcpy(sPdmBuffer + sizeof(uint16_t), (uint8_t *)aValue, aValueLength);

        pdmStatus = PDM_eSaveRecordData(aKey + NVM_START_ID, sPdmBuffer, aValueLength + sizeof(uint16_t));
        otEXPECT_ACTION((PDM_E_STATUS_OK == pdmStatus), error = OT_ERROR_NO_BUFS);
    }
    else
    {
        pdmStatus = PDM_eReadDataFromRecord(aKey + NVM_START_ID, sPdmBuffer, pdmBufferSize, &bytesRead);
        otEXPECT_ACTION((PDM_E_STATUS_OK == pdmStatus), error = OT_ERROR_NOT_FOUND);
        otEXPECT_ACTION((pdmBufferSize > aValueLength + bytesRead), error = OT_ERROR_NO_BUFS);

        memcpy(sPdmBuffer + bytesRead, (uint8_t *)aValue, aValueLength);

        pdmStatus = PDM_eSaveRecordData(aKey + NVM_START_ID, sPdmBuffer, aValueLength + bytesRead);
        otEXPECT_ACTION((PDM_E_STATUS_OK == pdmStatus), error = OT_ERROR_NO_BUFS);
    }

exit:
    return error;
}

// settings API
void otPlatSettingsInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    (void)PDM_Init();
}

void otPlatSettingsDeinit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

otError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError      error = OT_ERROR_NONE;
    PDM_teStatus pdmStatus;
    uint16_t     bytesRead = 0;
    uint16_t     offset    = 0;

    // only perform read if an input buffer was passed in
    if (aValue != NULL && aValueLength != NULL)
    {
        offset    = aIndex * (*aValueLength);
        pdmStatus = PDM_eReadPartialDataFromExistingRecord(aKey + NVM_START_ID, offset + sizeof(uint16_t), aValue,
                                                           *aValueLength, &bytesRead);

        otEXPECT_ACTION((PDM_E_STATUS_OK == pdmStatus), error = OT_ERROR_NOT_FOUND);
        *aValueLength = bytesRead;
    }
    else
    {
        if (false == PDM_bDoesDataExist(aKey + NVM_START_ID, &bytesRead))
        {
            error = OT_ERROR_NOT_FOUND;
        }
        else if (aValueLength != NULL)
        {
            *aValueLength = bytesRead;
        }
    }

exit:
    return error;
}

otError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    return addSetting(aInstance, aKey, true, aValue, aValueLength);
}

otError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    uint16_t length;
    bool     index0;

    index0 = (otPlatSettingsGet(aInstance, aKey, 0, NULL, &length) == OT_ERROR_NOT_FOUND ? true : false);
    return addSetting(aInstance, aKey, index0, aValue, aValueLength);
}

otError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError      error = OT_ERROR_NONE;
    PDM_teStatus pdmStatus;
    uint16_t     bytesRead     = 0;
    uint16_t     recordElmSize = 0;

    pdmStatus = PDM_eReadDataFromRecord(aKey + NVM_START_ID, sPdmBuffer, pdmBufferSize, &bytesRead);
    otEXPECT_ACTION((PDM_E_STATUS_OK == pdmStatus), error = OT_ERROR_NOT_FOUND);

    recordElmSize = *((uint16_t *)sPdmBuffer);

    /* Determine if record contains multiple entries or just one */
    if ((-1 == aIndex) || (recordElmSize == bytesRead - sizeof(recordElmSize)))
    {
        PDM_vDeleteDataRecord(aKey + NVM_START_ID);
    }
    else if (recordElmSize < bytesRead)
    {
        uint8_t *pMovePtrDst = sPdmBuffer + sizeof(recordElmSize) + (recordElmSize * aIndex);
        uint8_t *pMovePtrSrc = pMovePtrDst + recordElmSize;

        if (pMovePtrSrc < sPdmBuffer + bytesRead)
        {
            memcpy(pMovePtrDst, pMovePtrSrc, recordElmSize);
        }
        pdmStatus = PDM_eSaveRecordData(aKey + NVM_START_ID, sPdmBuffer, bytesRead - recordElmSize);
        otEXPECT_ACTION((PDM_E_STATUS_OK == pdmStatus), error = OT_ERROR_NOT_FOUND);
    }

exit:
    return error;
}

void otPlatSettingsWipe(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    for (uint32_t i = 0; i <= NVM_MAX_ID; i++)
    {
        PDM_vDeleteDataRecord(NVM_START_ID + i);
    }
}
