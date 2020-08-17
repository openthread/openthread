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
 *   This file implements the OpenThread platform abstraction for the non-volatile storage.
 */

#include <openthread-core-config.h>
#include <openthread/config.h>

#if OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE // Use OT NV system

#include "em_msc.h"
#include <string.h>
#include <openthread/instance.h>

#define FLASH_PAGE_NUM 2
#define FLASH_DATA_END_ADDR (FLASH_BASE + FLASH_SIZE)
#define FLASH_DATA_START_ADDR (FLASH_DATA_END_ADDR - (FLASH_PAGE_SIZE * FLASH_PAGE_NUM))
#define FLASH_SWAP_PAGE_NUM (FLASH_PAGE_NUM / 2)
#define FLASH_SWAP_SIZE (FLASH_PAGE_SIZE * FLASH_SWAP_PAGE_NUM)

static inline uint32_t mapAddress(uint8_t aSwapIndex, uint32_t aOffset)
{
    uint32_t address;

    address = FLASH_DATA_START_ADDR + aOffset;

    if (aSwapIndex)
    {
        address += FLASH_SWAP_SIZE;
    }

    return address;
}

void otPlatFlashInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

uint32_t otPlatFlashGetSwapSize(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return FLASH_SWAP_SIZE;
}

void otPlatFlashErase(otInstance *aInstance, uint8_t aSwapIndex)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t address = mapAddress(aSwapIndex, 0);

    for (uint32_t n = 0; n < FLASH_SWAP_PAGE_NUM; n++, address += FLASH_PAGE_SIZE)
    {
        MSC_ErasePage((uint32_t *)address);
    }
}

void otPlatFlashWrite(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    MSC_WriteWord((uint32_t *)mapAddress(aSwapIndex, aOffset), aData, aSize);
}

void otPlatFlashRead(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    memcpy(aData, (const uint8_t *)mapAddress(aSwapIndex, aOffset), aSize);
}

#else // Defaults to Silabs nvm3 system

#include "nvm3.h"
#include "nvm3_default.h"
#include <string.h>
#include <openthread/platform/settings.h>
#include "common/code_utils.hpp"
#include "common/logging.hpp"

#define NVM3KEY_DOMAIN_OPENTHREAD 0x20000U
#define NUM_INDEXED_SETTINGS \
    OPENTHREAD_CONFIG_MLE_MAX_CHILDREN // Indexed key types are only supported for kKeyChildInfo (=='child table').
#define ENUM_NVM3_KEY_LIST_SIZE 4      // List size used when enumerating nvm3 keys.

static otError          addSetting(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);
static nvm3_ObjectKey_t makeNvm3ObjKey(uint16_t otSettingsKey, int index);
static otError          mapNvm3Error(Ecode_t nvm3Res);

void otPlatSettingsInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (mapNvm3Error(nvm3_open(nvm3_defaultHandle, nvm3_defaultInit)) != OT_ERROR_NONE)
    {
        otLogDebgPlat("Error initializing nvm3 instance");
    }
}

void otPlatSettingsDeinit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    nvm3_close(nvm3_defaultHandle);
}

otError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    // Searches through all matching nvm3 keys to find the one with the required
    // 'index', then reads the nvm3 data into the destination buffer.
    // (Repeatedly enumerates a list of matching keys from the nvm3 until the
    // required index is found).

    OT_UNUSED_VARIABLE(aInstance);

    otError  err;
    uint16_t valueLength = 0;

    nvm3_ObjectKey_t nvm3Key  = makeNvm3ObjKey(aKey, 0); // The base nvm3 key value.
    bool             idxFound = false;
    int              idx      = 0;
    err                       = OT_ERROR_NOT_FOUND;
    while ((idx <= NUM_INDEXED_SETTINGS) && (!idxFound))
    {
        // Get the next nvm3 key list.
        nvm3_ObjectKey_t keys[ENUM_NVM3_KEY_LIST_SIZE]; // List holds the next set of nvm3 keys.
        size_t           objCnt = nvm3_enumObjects(nvm3_defaultHandle, keys, ENUM_NVM3_KEY_LIST_SIZE, nvm3Key,
                                         makeNvm3ObjKey(aKey, NUM_INDEXED_SETTINGS));
        for (size_t i = 0; i < objCnt; ++i)
        {
            nvm3Key = keys[i];
            if (idx == aIndex)
            {
                uint32_t objType;
                size_t   objLen;
                err = mapNvm3Error(nvm3_getObjectInfo(nvm3_defaultHandle, nvm3Key, &objType, &objLen));
                if (err == OT_ERROR_NONE)
                {
                    valueLength = objLen;

                    // Only perform read if an input buffer was passed in.
                    if ((aValue != NULL) && (aValueLength != NULL))
                    {
                        // Read all nvm3 obj bytes into a tmp buffer, then copy the required
                        // number of bytes to the read destination buffer.
                        uint8_t *buf = malloc(valueLength);
                        err          = mapNvm3Error(nvm3_readData(nvm3_defaultHandle, nvm3Key, buf, valueLength));
                        if (err == OT_ERROR_NONE)
                        {
                            memcpy(aValue, buf, (valueLength < *aValueLength) ? valueLength : *aValueLength);
                        }
                        free(buf);
                        SuccessOrExit(err);
                    }
                }
                idxFound = true;
                break;
            }
            ++idx;
        }
        if (objCnt < ENUM_NVM3_KEY_LIST_SIZE)
        {
            // Stop searching (there are no more matching nvm3 objects).
            break;
        }
        ++nvm3Key; // Inc starting value for next nvm3 key list enumeration.
    }

exit:
    if (aValueLength != NULL)
    {
        *aValueLength = valueLength; // always return actual nvm3 object length.
    }

    return err;
}

otError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError err;

    // Delete all nvm3 objects matching the input key (i.e. the 'setting indexes' of the key).
    err = otPlatSettingsDelete(aInstance, aKey, -1);
    if ((err == OT_ERROR_NONE) || (err == OT_ERROR_NOT_FOUND))
    {
        // Add new setting object (i.e. 'index0' of the key).
        err = addSetting(aKey, aValue, aValueLength);
        SuccessOrExit(err);
    }

exit:
    return err;
}

otError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    return addSetting(aKey, aValue, aValueLength);
}

otError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
    // Searches through all matching nvm3 keys to find the one with the required
    // 'index' (or index = -1 to delete all), then deletes the nvm3 object.
    // (Repeatedly enumerates a list of matching keys from the nvm3 until the
    // required index is found).

    OT_UNUSED_VARIABLE(aInstance);

    otError          err;
    nvm3_ObjectKey_t nvm3Key  = makeNvm3ObjKey(aKey, 0); // The base nvm3 key value.
    bool             idxFound = false;
    int              idx      = 0;
    err                       = OT_ERROR_NOT_FOUND;
    while ((idx <= NUM_INDEXED_SETTINGS) && (!idxFound))
    {
        // Get the next nvm3 key list.
        nvm3_ObjectKey_t keys[ENUM_NVM3_KEY_LIST_SIZE]; // List holds the next set of nvm3 keys.
        size_t           objCnt = nvm3_enumObjects(nvm3_defaultHandle, keys, ENUM_NVM3_KEY_LIST_SIZE, nvm3Key,
                                         makeNvm3ObjKey(aKey, NUM_INDEXED_SETTINGS));
        for (size_t i = 0; i < objCnt; ++i)
        {
            nvm3Key = keys[i];
            if ((idx == aIndex) || (aIndex == -1))
            {
                uint32_t objType;
                size_t   objLen;
                err = mapNvm3Error(nvm3_getObjectInfo(nvm3_defaultHandle, nvm3Key, &objType, &objLen));
                if (err == OT_ERROR_NONE)
                {
                    // Delete the nvm3 object.
                    err = mapNvm3Error(nvm3_deleteObject(nvm3_defaultHandle, nvm3Key));
                    SuccessOrExit(err);
                }
                if (aIndex != -1)
                {
                    idxFound = true;
                    break;
                }
            }
            ++idx;
        }
        if (objCnt < ENUM_NVM3_KEY_LIST_SIZE)
        {
            // Stop searching (there are no more matching nvm3 objects).
            break;
        }
        ++nvm3Key; // Inc starting value for next nvm3 key list enumeration.
    }

exit:
    return err;
}

void otPlatSettingsWipe(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    // Delete nvm3 objects for all OT Settings keys (and any of their associated 'indexes').
    // Note- any OT User nvm3 objects in the OT nvm3 area are NOT be erased.
    for (uint16_t aKey = 0; aKey < 8; ++aKey)
    {
        otPlatSettingsDelete(NULL, aKey, -1);
    }
}

// Local functions..

static otError addSetting(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    // Helper function- writes input buffer data to a NEW nvm3 object.
    // nvm3 object is created at the first available Key + index.

    otError err;

    if ((aValueLength == 0) || (aValue == NULL))
    {
        err = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        for (int idx = 0; idx <= NUM_INDEXED_SETTINGS; ++idx)
        {
            nvm3_ObjectKey_t nvm3Key;
            nvm3Key = makeNvm3ObjKey(aKey, idx);

            uint32_t objType;
            size_t   objLen;
            err = mapNvm3Error(nvm3_getObjectInfo(nvm3_defaultHandle, nvm3Key, &objType, &objLen));
            if (err == OT_ERROR_NOT_FOUND)
            {
                // Use this index for the new nvm3 object.
                // Write the binary data to nvm3 (Creates nvm3 object if required).
                err = mapNvm3Error(nvm3_writeData(nvm3_defaultHandle, nvm3Key, aValue, aValueLength));
                break;
            }
            else if (err != OT_ERROR_NONE)
            {
                break;
            }
        }
    }

    return err;
}

static nvm3_ObjectKey_t makeNvm3ObjKey(uint16_t otSettingsKey, int index)
{
    return (NVM3KEY_DOMAIN_OPENTHREAD | (otSettingsKey << 8) | (index & 0xFF));
}

static otError mapNvm3Error(Ecode_t nvm3Res)
{
    otError err;

    switch (nvm3Res)
    {
    case ECODE_NVM3_OK:
        err = OT_ERROR_NONE;
        break;

    case ECODE_NVM3_ERR_KEY_NOT_FOUND:
        err = OT_ERROR_NOT_FOUND;
        break;

    default:
        err = OT_ERROR_FAILED;
        break;
    }

    return err;
}

#endif // OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
