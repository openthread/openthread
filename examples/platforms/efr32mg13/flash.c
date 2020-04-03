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
 *   This file implements the OpenThread platform abstraction for the non-volatile
 *   storage for the EFR32 platform using the Silabs Nvm3 interface.
 */

#include <openthread/config.h>
#include <openthread/platform/settings.h>

#if !OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE

#include "nvm3.h"
#include "nvm3_hal_flash.h"
#include <string.h>
#include "common/code_utils.hpp"

// Two macros are provided to support the creation of the Silicon Labs NVM3 area and
// initialization data- NVM3_DEFINE_SECTION_STATIC_DATA() and NVM3_DEFINE_SECTION_INIT_DATA().
// A linker section called 'name'_section is defined by NVM3_DEFINE_SECTION_STATIC_DATA().
// The NVM3 area is placed at the top of the device FLASH section by the linker script
// file: e.g. efr32mgxx.ld
// An error is returned by nvm3_open() on alignment or size violation.

// Local version of SDK macro (avoids uninitialized var compile error).
#define OT_NVM3_DEFINE_SECTION_STATIC_DATA(name, nvmSize, cacheSize) \
    static nvm3_CacheEntry_t name##_cache[cacheSize];                \
    static uint8_t           name##_nvm[nvmSize] SL_ATTRIBUTE_SECTION(STRINGIZE(name##_section))

// Local version of SDK macro (allows OT to configure the maximum nvm3 object size and headroom).
#define OT_NVM3_DEFINE_SECTION_INIT_DATA(name, maxObjectSize, repackHeadroom) \
    static nvm3_Init_t name = {                                               \
        (nvm3_HalPtr_t)name##_nvm,                                            \
        sizeof(name##_nvm),                                                   \
        name##_cache,                                                         \
        sizeof(name##_cache) / sizeof(nvm3_CacheEntry_t),                     \
        maxObjectSize,                                                        \
        repackHeadroom,                                                       \
        &nvm3_halFlashHandle,                                                 \
    }

#define NUM_SETTINGS_OBJECTS 7 // == number of enumerated Settings key types (in settings.hpp).
#define NUM_INDEXED_SETTINGS \
    OPENTHREAD_CONFIG_MLE_MAX_CHILDREN // Indexed key types are only supported for kKeyChildInfo (=='child table').
#define NUM_USER_OBJECTS 16 // User nvm3 objects (nvm3 key range 0x0000 -> 0xDFFF is available for user data).

#define OT_NVM3_FLASH_NUM_PAGES 4               // Note- nvm3 requires minimum of 3 pages.
#define OT_NVM3_FLASH_PAGE_SIZE FLASH_PAGE_SIZE // Device flash page size (MG12/MG13=2K, MG21=8K)
#define OT_NVM3_FLASH_SIZE OT_NVM3_FLASH_NUM_PAGES *OT_NVM3_FLASH_PAGE_SIZE
#define OT_NVM3_MAX_NUM_OBJECTS NUM_SETTINGS_OBJECTS + NUM_INDEXED_SETTINGS + NUM_USER_OBJECTS
#define OT_NVM3_MAX_OBJECT_SIZE 256
#define OT_NVM3_REPACK_HEADROOM 64 // Threshold for automatic nvm3 flash repacking.

#define OT_NVM3_SETTINGS_KEY_PREFIX 0xE000 // (=> key range 0x0000-0xDFFF is available for OT User nvm3 objects).

#define ENUM_NVM3_KEY_LIST_SIZE 4 // List size used when enumerating nvm3 keys.

static nvm3_Handle_t handle = {NULL};

static otError          addSetting(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);
static nvm3_ObjectKey_t makeNvm3ObjKey(uint16_t otSettingsKey, int index);
static otError          mapNvm3Error(Ecode_t nvm3Res);

// Declare OT NVM3 data area and cache.
// OT data area should be enough for OT 'Settings' and whatwever User data is required.

OT_NVM3_DEFINE_SECTION_STATIC_DATA(otNvm3, OT_NVM3_FLASH_SIZE, OT_NVM3_MAX_NUM_OBJECTS);

OT_NVM3_DEFINE_SECTION_INIT_DATA(otNvm3, OT_NVM3_MAX_OBJECT_SIZE, OT_NVM3_REPACK_HEADROOM);

void otPlatSettingsInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError err;
    bool    needClose = false;

    err = mapNvm3Error(nvm3_open(&handle, &otNvm3));
    SuccessOrExit(err);
    needClose = true;

exit:
    if (needClose)
    {
        nvm3_close(&handle);
    }
}

void otPlatSettingsDeinit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    nvm3_close(&handle);
}

otError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    // Searches through all matching nvm3 keys to find the one with the required
    // 'index', then reads the nvm3 data into the destination buffer.
    // (Repeatedly enumerates a list of matching keys from the nvm3 until the
    // required index is found).

    OT_UNUSED_VARIABLE(aInstance);

    otError  err;
    bool     needClose   = false;
    uint16_t valueLength = 0;

    err = mapNvm3Error(nvm3_open(&handle, &otNvm3));
    SuccessOrExit(err);
    needClose = true;

    nvm3_ObjectKey_t nvm3Key  = makeNvm3ObjKey(aKey, 0); // The base nvm3 key value.
    bool             idxFound = false;
    int              idx      = 0;
    err                       = OT_ERROR_NOT_FOUND;
    while ((idx <= NUM_INDEXED_SETTINGS) && (!idxFound))
    {
        // Get the next nvm3 key list.
        nvm3_ObjectKey_t keys[ENUM_NVM3_KEY_LIST_SIZE]; // List holds the next set of nvm3 keys.
        size_t           objCnt = nvm3_enumObjects(&handle, keys, ENUM_NVM3_KEY_LIST_SIZE, nvm3Key,
                                         makeNvm3ObjKey(aKey, NUM_INDEXED_SETTINGS));
        for (size_t i = 0; i < objCnt; ++i)
        {
            nvm3Key = keys[i];
            if (idx == aIndex)
            {
                uint32_t objType;
                size_t   objLen;
                err = mapNvm3Error(nvm3_getObjectInfo(&handle, nvm3Key, &objType, &objLen));
                if (err == OT_ERROR_NONE)
                {
                    valueLength = objLen;

                    // Only perform read if an input buffer was passed in.
                    if ((aValue != NULL) && (aValueLength != NULL))
                    {
                        // Read all nvm3 obj bytes into a tmp buffer, then copy the required
                        // number of bytes to the read destination buffer.
                        uint8_t *buf = malloc(valueLength);
                        err          = mapNvm3Error(nvm3_readData(&handle, nvm3Key, buf, valueLength));
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
    if (needClose)
    {
        nvm3_close(&handle);
    }

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

    otError err;
    bool    needClose = false;

    err = mapNvm3Error(nvm3_open(&handle, &otNvm3));
    SuccessOrExit(err);
    needClose = true;

    nvm3_ObjectKey_t nvm3Key  = makeNvm3ObjKey(aKey, 0); // The base nvm3 key value.
    bool             idxFound = false;
    int              idx      = 0;
    err                       = OT_ERROR_NOT_FOUND;
    while ((idx <= NUM_INDEXED_SETTINGS) && (!idxFound))
    {
        // Get the next nvm3 key list.
        nvm3_ObjectKey_t keys[ENUM_NVM3_KEY_LIST_SIZE]; // List holds the next set of nvm3 keys.
        size_t           objCnt = nvm3_enumObjects(&handle, keys, ENUM_NVM3_KEY_LIST_SIZE, nvm3Key,
                                         makeNvm3ObjKey(aKey, NUM_INDEXED_SETTINGS));
        for (size_t i = 0; i < objCnt; ++i)
        {
            nvm3Key = keys[i];
            if ((idx == aIndex) || (aIndex == -1))
            {
                uint32_t objType;
                size_t   objLen;
                err = mapNvm3Error(nvm3_getObjectInfo(&handle, nvm3Key, &objType, &objLen));
                if (err == OT_ERROR_NONE)
                {
                    // Delete the nvm3 object.
                    err = mapNvm3Error(nvm3_deleteObject(&handle, nvm3Key));
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
    if (needClose)
    {
        nvm3_close(&handle);
    }

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
    bool    needClose = false;

    err = mapNvm3Error(nvm3_open(&handle, &otNvm3));
    SuccessOrExit(err);
    needClose = true;

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
            err = mapNvm3Error(nvm3_getObjectInfo(&handle, nvm3Key, &objType, &objLen));
            if (err == OT_ERROR_NOT_FOUND)
            {
                // Use this index for the new nvm3 object.
                // Write the binary data to nvm3 (Creates nvm3 object if required).
                err = mapNvm3Error(nvm3_writeData(&handle, nvm3Key, aValue, aValueLength));
                break;
            }
            else if (err != OT_ERROR_NONE)
            {
                break;
            }
        }
    }

exit:
    if (needClose)
    {
        nvm3_close(&handle);
    }
    return err;
}

static nvm3_ObjectKey_t makeNvm3ObjKey(uint16_t otSettingsKey, int index)
{
    return (OT_NVM3_SETTINGS_KEY_PREFIX | (otSettingsKey << 8) | (index & 0xFF));
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

#endif // !OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
