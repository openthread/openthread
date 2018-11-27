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
 *   This file implements the OpenThread platform abstraction for non-volatile storage of settings.
 *
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <openthread-core-config.h>

#include <openthread/instance.h>
#include <openthread/platform/settings.h>

#include "utils/code_utils.h"
#include "utils/wrap_string.h"

#include "flash.h"

#define OT_FLASH_BLOCK_ADD_BEGIN_FLAG (1 << 0)
#define OT_FLASH_BLOCK_ADD_COMPLETE_FLAG (1 << 1)
#define OT_FLASH_BLOCK_DELETE_FLAG (1 << 2)
#define OT_FLASH_BLOCK_INDEX_0_FLAG (1 << 3)

#define OT_SETTINGS_FLAG_SIZE 4
#define OT_SETTINGS_BLOCK_DATA_SIZE 255

#define OT_SETTINGS_IN_SWAP 0xbe5cc5ef
#define OT_SETTINGS_IN_USE 0xbe5cc5ee
#define OT_SETTINGS_NOT_USED 0xbe5cc5ec

OT_TOOL_PACKED_BEGIN
struct settingsBlock
{
    uint16_t key;
    uint16_t flag;
    uint16_t length;
    uint16_t reserved;
} OT_TOOL_PACKED_END;

/**
 * @def SETTINGS_CONFIG_BASE_ADDRESS
 *
 * The base address of settings.
 *
 */
#ifndef SETTINGS_CONFIG_BASE_ADDRESS
#define SETTINGS_CONFIG_BASE_ADDRESS 0x39000
#endif // SETTINGS_CONFIG_BASE_ADDRESS

/**
 * @def SETTINGS_CONFIG_PAGE_SIZE
 *
 * The page size of settings.
 *
 */
#ifndef SETTINGS_CONFIG_PAGE_SIZE
#define SETTINGS_CONFIG_PAGE_SIZE 0x800
#endif // SETTINGS_CONFIG_PAGE_SIZE

/**
 * @def SETTINGS_CONFIG_PAGE_NUM
 *
 * The page number of settings.
 *
 */
#ifndef SETTINGS_CONFIG_PAGE_NUM
#define SETTINGS_CONFIG_PAGE_NUM 2
#endif // SETTINGS_CONFIG_PAGE_NUM

static uint32_t sSettingsBaseAddress;
static uint32_t sSettingsUsedSize;

static uint16_t getAlignLength(uint16_t length)
{
    return (length + 3) & 0xfffc;
}

static void setSettingsFlag(uint32_t aBase, uint32_t aFlag)
{
    utilsFlashWrite(aBase, (uint8_t *)&aFlag, sizeof(aFlag));
}

static void initSettings(uint32_t aBase, uint32_t aFlag)
{
    uint32_t address      = aBase;
    uint32_t settingsSize = SETTINGS_CONFIG_PAGE_NUM > 1 ? SETTINGS_CONFIG_PAGE_SIZE * SETTINGS_CONFIG_PAGE_NUM / 2
                                                         : SETTINGS_CONFIG_PAGE_SIZE;

    while (address < (aBase + settingsSize))
    {
        utilsFlashErasePage(address);
        utilsFlashStatusWait(1000);
        address += SETTINGS_CONFIG_PAGE_SIZE;
    }

    setSettingsFlag(aBase, aFlag);
}

static uint32_t swapSettingsBlock(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t oldBase      = sSettingsBaseAddress;
    uint32_t swapAddress  = oldBase;
    uint32_t usedSize     = sSettingsUsedSize;
    uint8_t  pageNum      = SETTINGS_CONFIG_PAGE_NUM;
    uint32_t settingsSize = pageNum > 1 ? SETTINGS_CONFIG_PAGE_SIZE * pageNum / 2 : SETTINGS_CONFIG_PAGE_SIZE;

    otEXPECT(pageNum > 1);

    sSettingsBaseAddress =
        (swapAddress == SETTINGS_CONFIG_BASE_ADDRESS) ? (swapAddress + settingsSize) : SETTINGS_CONFIG_BASE_ADDRESS;

    initSettings(sSettingsBaseAddress, (uint32_t)OT_SETTINGS_IN_SWAP);
    sSettingsUsedSize = OT_SETTINGS_FLAG_SIZE;
    swapAddress += OT_SETTINGS_FLAG_SIZE;

    while (swapAddress < (oldBase + usedSize))
    {
        OT_TOOL_PACKED_BEGIN
        struct addSettingsBlock
        {
            struct settingsBlock block;
            uint8_t              data[OT_SETTINGS_BLOCK_DATA_SIZE];
        } OT_TOOL_PACKED_END addBlock;
        bool                 valid = true;

        utilsFlashRead(swapAddress, (uint8_t *)(&addBlock.block), sizeof(struct settingsBlock));
        swapAddress += sizeof(struct settingsBlock);

        if (!(addBlock.block.flag & OT_FLASH_BLOCK_ADD_COMPLETE_FLAG) &&
            (addBlock.block.flag & OT_FLASH_BLOCK_DELETE_FLAG))
        {
            uint32_t address = swapAddress + getAlignLength(addBlock.block.length);

            while (address < (oldBase + usedSize))
            {
                struct settingsBlock block;

                utilsFlashRead(address, (uint8_t *)(&block), sizeof(block));

                if (!(block.flag & OT_FLASH_BLOCK_ADD_COMPLETE_FLAG) && (block.flag & OT_FLASH_BLOCK_DELETE_FLAG) &&
                    !(block.flag & OT_FLASH_BLOCK_INDEX_0_FLAG) && (block.key == addBlock.block.key))
                {
                    valid = false;
                    break;
                }

                address += (getAlignLength(block.length) + sizeof(struct settingsBlock));
            }

            if (valid)
            {
                utilsFlashRead(swapAddress, addBlock.data, getAlignLength(addBlock.block.length));
                utilsFlashWrite(sSettingsBaseAddress + sSettingsUsedSize, (uint8_t *)(&addBlock),
                                getAlignLength(addBlock.block.length) + sizeof(struct settingsBlock));
                sSettingsUsedSize += (sizeof(struct settingsBlock) + getAlignLength(addBlock.block.length));
            }
        }
        else if (addBlock.block.flag == 0xff)
        {
            break;
        }

        swapAddress += getAlignLength(addBlock.block.length);
    }

    setSettingsFlag(sSettingsBaseAddress, (uint32_t)OT_SETTINGS_IN_USE);
    setSettingsFlag(oldBase, (uint32_t)OT_SETTINGS_NOT_USED);

exit:
    return settingsSize - sSettingsUsedSize;
}

static otError addSetting(otInstance *   aInstance,
                          uint16_t       aKey,
                          bool           aIndex0,
                          const uint8_t *aValue,
                          uint16_t       aValueLength)
{
    otError error = OT_ERROR_NONE;
    OT_TOOL_PACKED_BEGIN
    struct addSettingsBlock
    {
        struct settingsBlock block;
        uint8_t              data[OT_SETTINGS_BLOCK_DATA_SIZE];
    } OT_TOOL_PACKED_END addBlock;
    uint32_t settingsSize = SETTINGS_CONFIG_PAGE_NUM > 1 ? SETTINGS_CONFIG_PAGE_SIZE * SETTINGS_CONFIG_PAGE_NUM / 2
                                                         : SETTINGS_CONFIG_PAGE_SIZE;

    addBlock.block.flag = 0xff;
    addBlock.block.key  = aKey;

    if (aIndex0)
    {
        addBlock.block.flag &= (~OT_FLASH_BLOCK_INDEX_0_FLAG);
    }

    addBlock.block.flag &= (~OT_FLASH_BLOCK_ADD_BEGIN_FLAG);
    addBlock.block.length = aValueLength;

    if ((sSettingsUsedSize + getAlignLength(addBlock.block.length) + sizeof(struct settingsBlock)) >= settingsSize)
    {
        otEXPECT_ACTION(swapSettingsBlock(aInstance) >=
                            (getAlignLength(addBlock.block.length) + sizeof(struct settingsBlock)),
                        error = OT_ERROR_NO_BUFS);
    }

    utilsFlashWrite(sSettingsBaseAddress + sSettingsUsedSize, (uint8_t *)(&addBlock.block),
                    sizeof(struct settingsBlock));

    memset(addBlock.data, 0xff, OT_SETTINGS_BLOCK_DATA_SIZE);
    memcpy(addBlock.data, aValue, addBlock.block.length);

    utilsFlashWrite(sSettingsBaseAddress + sSettingsUsedSize + sizeof(struct settingsBlock), (uint8_t *)(addBlock.data),
                    getAlignLength(addBlock.block.length));

    addBlock.block.flag &= (~OT_FLASH_BLOCK_ADD_COMPLETE_FLAG);
    utilsFlashWrite(sSettingsBaseAddress + sSettingsUsedSize, (uint8_t *)(&addBlock.block),
                    sizeof(struct settingsBlock));
    sSettingsUsedSize += (sizeof(struct settingsBlock) + getAlignLength(addBlock.block.length));

exit:
    return error;
}

// settings API
void otPlatSettingsInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint8_t  index;
    uint32_t settingsSize = SETTINGS_CONFIG_PAGE_NUM > 1 ? SETTINGS_CONFIG_PAGE_SIZE * SETTINGS_CONFIG_PAGE_NUM / 2
                                                         : SETTINGS_CONFIG_PAGE_SIZE;

    sSettingsBaseAddress = SETTINGS_CONFIG_BASE_ADDRESS;

    utilsFlashInit();

    for (index = 0; index < 2; index++)
    {
        uint32_t blockFlag;

        sSettingsBaseAddress += settingsSize * index;
        utilsFlashRead(sSettingsBaseAddress, (uint8_t *)(&blockFlag), sizeof(blockFlag));

        if (blockFlag == OT_SETTINGS_IN_USE)
        {
            break;
        }
    }

    if (index == 2)
    {
        initSettings(sSettingsBaseAddress, (uint32_t)OT_SETTINGS_IN_USE);
    }

    sSettingsUsedSize = OT_SETTINGS_FLAG_SIZE;

    while (sSettingsUsedSize < settingsSize)
    {
        struct settingsBlock block;

        utilsFlashRead(sSettingsBaseAddress + sSettingsUsedSize, (uint8_t *)(&block), sizeof(block));

        if (!(block.flag & OT_FLASH_BLOCK_ADD_BEGIN_FLAG))
        {
            sSettingsUsedSize += (getAlignLength(block.length) + sizeof(struct settingsBlock));
        }
        else
        {
            break;
        }
    }
}

otError otPlatSettingsBeginChange(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_ERROR_NONE;
}

otError otPlatSettingsCommitChange(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_ERROR_NONE;
}

otError otPlatSettingsAbandonChange(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_ERROR_NONE;
}

otError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError  error       = OT_ERROR_NOT_FOUND;
    uint32_t address     = sSettingsBaseAddress + OT_SETTINGS_FLAG_SIZE;
    uint16_t valueLength = 0;
    int      index       = 0;

    while (address < (sSettingsBaseAddress + sSettingsUsedSize))
    {
        struct settingsBlock block;

        utilsFlashRead(address, (uint8_t *)(&block), sizeof(block));

        if (block.key == aKey)
        {
            if (!(block.flag & OT_FLASH_BLOCK_INDEX_0_FLAG))
            {
                index = 0;
            }

            if (!(block.flag & OT_FLASH_BLOCK_ADD_COMPLETE_FLAG) && (block.flag & OT_FLASH_BLOCK_DELETE_FLAG))
            {
                if (index == aIndex)
                {
                    uint16_t readLength = block.length;

                    // only perform read if an input buffer was passed in
                    if (aValue != NULL && aValueLength != NULL)
                    {
                        // adjust read length if input buffer length is smaller
                        if (readLength > *aValueLength)
                        {
                            readLength = *aValueLength;
                        }

                        utilsFlashRead(address + sizeof(struct settingsBlock), aValue, readLength);
                    }

                    valueLength = block.length;
                    error       = OT_ERROR_NONE;
                }

                index++;
            }
        }

        address += (getAlignLength(block.length) + sizeof(struct settingsBlock));
    }

    if (aValueLength != NULL)
    {
        *aValueLength = valueLength;
    }

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

    otError  error   = OT_ERROR_NOT_FOUND;
    uint32_t address = sSettingsBaseAddress + OT_SETTINGS_FLAG_SIZE;
    int      index   = 0;

    while (address < (sSettingsBaseAddress + sSettingsUsedSize))
    {
        struct settingsBlock block;

        utilsFlashRead(address, (uint8_t *)(&block), sizeof(block));

        if (block.key == aKey)
        {
            if (!(block.flag & OT_FLASH_BLOCK_INDEX_0_FLAG))
            {
                index = 0;
            }

            if (!(block.flag & OT_FLASH_BLOCK_ADD_COMPLETE_FLAG) && (block.flag & OT_FLASH_BLOCK_DELETE_FLAG))
            {
                if (aIndex == index || aIndex == -1)
                {
                    error = OT_ERROR_NONE;
                    block.flag &= (~OT_FLASH_BLOCK_DELETE_FLAG);
                    utilsFlashWrite(address, (uint8_t *)(&block), sizeof(block));
                }

                if (index == 1 && aIndex == 0)
                {
                    block.flag &= (~OT_FLASH_BLOCK_INDEX_0_FLAG);
                    utilsFlashWrite(address, (uint8_t *)(&block), sizeof(block));
                }

                index++;
            }
        }

        address += (getAlignLength(block.length) + sizeof(struct settingsBlock));
    }

    return error;
}

void otPlatSettingsWipe(otInstance *aInstance)
{
    initSettings(sSettingsBaseAddress, (uint32_t)OT_SETTINGS_IN_USE);
    otPlatSettingsInit(aInstance);
}
