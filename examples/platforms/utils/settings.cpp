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

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <openthread-core-config.h>

#include "openthread/types.h"

#include <common/code_utils.hpp>
#include <platform/settings.h>

#include "flash.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    kBlockAddBeginFlag = 0x1,
    kBlockAddCompleteFlag = 0x02,
    kBlockDeleteFlag = 0x04,
    kBlockIndex0Flag = 0x08,
};

enum
{
    kSettingsFlagSize = 4,
    kSettingsBlockDataSize = 255,

    kSettingsInSwap = 0xbe5cc5ef,
    kSettingsInUse = 0xbe5cc5ee,
    kSettingsNotUse = 0xbe5cc5ec,
};

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
#define SETTINGS_CONFIG_BASE_ADDRESS                 0x39000
#endif  // SETTINGS_CONFIG_BASE_ADDRESS

/**
 * @def SETTINGS_CONFIG_PAGE_SIZE
 *
 * The page size of settings.
 *
 */
#ifndef SETTINGS_CONFIG_PAGE_SIZE
#define SETTINGS_CONFIG_PAGE_SIZE                    0x800
#endif  // SETTINGS_CONFIG_PAGE_SIZE

/**
 * @def SETTINGS_CONFIG_PAGE_NUM
 *
 * The page number of settings.
 *
 */
#ifndef SETTINGS_CONFIG_PAGE_NUM
#define SETTINGS_CONFIG_PAGE_NUM                     2
#endif  // SETTINGS_CONFIG_PAGE_NUM

static uint32_t sSettingsBaseAddress;
static uint32_t sSettingsUsedSize;

static uint16_t getAlignLength(uint16_t length)
{
    return (length + 3) & 0xfffc;
}

static void setSettingsFlag(uint32_t aBase, uint32_t aFlag)
{
    utilsFlashWrite(aBase, reinterpret_cast<uint8_t *>(&aFlag), sizeof(aFlag));
}

static void initSettings(uint32_t aBase, uint32_t aFlag)
{
    uint32_t address = aBase;
    uint32_t settingsSize = SETTINGS_CONFIG_PAGE_NUM > 1 ?
                            SETTINGS_CONFIG_PAGE_SIZE * SETTINGS_CONFIG_PAGE_NUM / 2 :
                            SETTINGS_CONFIG_PAGE_SIZE;

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
    uint32_t oldBase = sSettingsBaseAddress;
    uint32_t swapAddress = oldBase;
    uint32_t usedSize = sSettingsUsedSize;
    uint8_t pageNum = SETTINGS_CONFIG_PAGE_NUM;
    uint32_t settingsSize = pageNum > 1 ? SETTINGS_CONFIG_PAGE_SIZE * pageNum / 2 :
                            SETTINGS_CONFIG_PAGE_SIZE;

    (void)aInstance;

    VerifyOrExit(pageNum > 1, ;);

    sSettingsBaseAddress = (swapAddress == SETTINGS_CONFIG_BASE_ADDRESS) ?
                           (swapAddress + settingsSize) :
                           SETTINGS_CONFIG_BASE_ADDRESS;

    initSettings(sSettingsBaseAddress, static_cast<uint32_t>(kSettingsInSwap));
    sSettingsUsedSize = kSettingsFlagSize;
    swapAddress += kSettingsFlagSize;

    while (swapAddress < (oldBase + usedSize))
    {
        OT_TOOL_PACKED_BEGIN
        struct addSettingsBlock
        {
            struct settingsBlock block;
            uint8_t data[kSettingsBlockDataSize];
        } OT_TOOL_PACKED_END addBlock;
        bool valid = true;

        utilsFlashRead(swapAddress, reinterpret_cast<uint8_t *>(&addBlock.block), sizeof(struct settingsBlock));
        swapAddress += sizeof(struct settingsBlock);

        if (!(addBlock.block.flag & kBlockAddCompleteFlag) && (addBlock.block.flag & kBlockDeleteFlag))
        {
            uint32_t address = swapAddress + getAlignLength(addBlock.block.length);

            while (address < (oldBase + usedSize))
            {
                struct settingsBlock block;

                utilsFlashRead(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));

                if (!(block.flag & kBlockAddCompleteFlag) && (block.flag & kBlockDeleteFlag) &&
                    !(block.flag & kBlockIndex0Flag) && (block.key == addBlock.block.key))
                {
                    valid = false;
                    break;
                }

                address += (getAlignLength(block.length) + sizeof(struct settingsBlock));
            }

            if (valid)
            {
                utilsFlashRead(swapAddress, addBlock.data, getAlignLength(addBlock.block.length));
                utilsFlashWrite(sSettingsBaseAddress + sSettingsUsedSize,
                                reinterpret_cast<uint8_t *>(&addBlock),
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

    setSettingsFlag(sSettingsBaseAddress, static_cast<uint32_t>(kSettingsInUse));
    setSettingsFlag(oldBase, static_cast<uint32_t>(kSettingsNotUse));

exit:
    return settingsSize - sSettingsUsedSize;
}

static ThreadError addSetting(otInstance *aInstance, uint16_t aKey, bool aIndex0, const uint8_t *aValue,
                              uint16_t aValueLength)
{
    ThreadError error = kThreadError_None;
    OT_TOOL_PACKED_BEGIN
    struct addSettingsBlock
    {
        struct settingsBlock block;
        uint8_t data[kSettingsBlockDataSize];
    } OT_TOOL_PACKED_END addBlock;
    uint32_t settingsSize = SETTINGS_CONFIG_PAGE_NUM > 1 ?
                            SETTINGS_CONFIG_PAGE_SIZE * SETTINGS_CONFIG_PAGE_NUM / 2 :
                            SETTINGS_CONFIG_PAGE_SIZE;

    addBlock.block.flag = 0xff;
    addBlock.block.key = aKey;

    if (aIndex0)
    {
        addBlock.block.flag &= (~kBlockIndex0Flag);
    }

    addBlock.block.flag &= (~kBlockAddBeginFlag);
    addBlock.block.length = aValueLength;

    if ((sSettingsUsedSize + getAlignLength(addBlock.block.length) + sizeof(struct settingsBlock)) >=
        settingsSize)
    {
        VerifyOrExit(swapSettingsBlock(aInstance) >= (getAlignLength(addBlock.block.length) + sizeof(struct settingsBlock)),
                     error = kThreadError_NoBufs);
    }

    utilsFlashWrite(sSettingsBaseAddress + sSettingsUsedSize,
                    reinterpret_cast<uint8_t *>(&addBlock.block),
                    sizeof(struct settingsBlock));

    memset(addBlock.data, 0xff, kSettingsBlockDataSize);
    memcpy(addBlock.data, aValue, addBlock.block.length);

    utilsFlashWrite(sSettingsBaseAddress + sSettingsUsedSize + sizeof(struct settingsBlock),
                    reinterpret_cast<uint8_t *>(addBlock.data), getAlignLength(addBlock.block.length));

    addBlock.block.flag &= (~kBlockAddCompleteFlag);
    utilsFlashWrite(sSettingsBaseAddress + sSettingsUsedSize,
                    reinterpret_cast<uint8_t *>(&addBlock.block),
                    sizeof(struct settingsBlock));
    sSettingsUsedSize += (sizeof(struct settingsBlock) + getAlignLength(addBlock.block.length));

exit:
    return error;
}

// settings API
void otPlatSettingsInit(otInstance *aInstance)
{
    uint8_t index;
    uint32_t settingsSize = SETTINGS_CONFIG_PAGE_NUM > 1 ?
                            SETTINGS_CONFIG_PAGE_SIZE * SETTINGS_CONFIG_PAGE_NUM / 2 :
                            SETTINGS_CONFIG_PAGE_SIZE;

    (void)aInstance;

    sSettingsBaseAddress = SETTINGS_CONFIG_BASE_ADDRESS;

    utilsFlashInit();

    for (index = 0; index < 2; index++)
    {
        uint32_t blockFlag;

        sSettingsBaseAddress += settingsSize * index;
        utilsFlashRead(sSettingsBaseAddress, reinterpret_cast<uint8_t *>(&blockFlag), sizeof(blockFlag));

        if (blockFlag == kSettingsInUse)
        {
            break;
        }
    }

    if (index == 2)
    {
        initSettings(sSettingsBaseAddress, static_cast<uint32_t>(kSettingsInUse));
    }

    sSettingsUsedSize = kSettingsFlagSize;

    while (sSettingsUsedSize < settingsSize)
    {
        struct settingsBlock block;

        utilsFlashRead(sSettingsBaseAddress + sSettingsUsedSize,
                       reinterpret_cast<uint8_t *>(&block), sizeof(block));

        if (!(block.flag & kBlockAddBeginFlag))
        {
            sSettingsUsedSize += (getAlignLength(block.length) + sizeof(struct settingsBlock));
        }
        else
        {
            break;
        }
    }
}

ThreadError otPlatSettingsBeginChange(otInstance *aInstance)
{
    (void)aInstance;
    return kThreadError_None;
}

ThreadError otPlatSettingsCommitChange(otInstance *aInstance)
{
    (void)aInstance;
    return kThreadError_None;
}

ThreadError otPlatSettingsAbandonChange(otInstance *aInstance)
{
    (void)aInstance;
    return kThreadError_None;
}

ThreadError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    ThreadError error = kThreadError_NotFound;
    uint32_t address = sSettingsBaseAddress + kSettingsFlagSize;
    uint16_t valueLength = 0;
    int index = 0;

    (void)aInstance;

    while (address < (sSettingsBaseAddress + sSettingsUsedSize))
    {
        struct settingsBlock block;

        utilsFlashRead(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));

        if (block.key == aKey)
        {
            if (!(block.flag & kBlockIndex0Flag))
            {
                index = 0;
            }

            if (!(block.flag & kBlockAddCompleteFlag) && (block.flag & kBlockDeleteFlag))
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

                    valueLength = readLength;
                    error = kThreadError_None;
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

ThreadError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    return addSetting(aInstance, aKey, true, aValue, aValueLength);
}

ThreadError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    uint16_t length;
    bool index0;

    index0 = (otPlatSettingsGet(aInstance, aKey, 0, NULL, &length) == kThreadError_NotFound ? true : false);
    return addSetting(aInstance, aKey, index0, aValue, aValueLength);
}

ThreadError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
    ThreadError error = kThreadError_NotFound;
    uint32_t address = sSettingsBaseAddress + kSettingsFlagSize;
    int index = 0;

    (void)aInstance;

    while (address < (sSettingsBaseAddress + sSettingsUsedSize))
    {
        struct settingsBlock block;

        utilsFlashRead(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));

        if (block.key == aKey)
        {
            if (!(block.flag & kBlockIndex0Flag))
            {
                index = 0;
            }

            if (!(block.flag & kBlockAddCompleteFlag) && (block.flag & kBlockDeleteFlag))
            {
                if (aIndex == index || aIndex == -1)
                {
                    error = kThreadError_None;
                    block.flag &= (~kBlockDeleteFlag);
                    utilsFlashWrite(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));
                }

                if (index == 1 && aIndex == 0)
                {
                    block.flag &= (~kBlockIndex0Flag);
                    utilsFlashWrite(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));
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
    initSettings(sSettingsBaseAddress, static_cast<uint32_t>(kSettingsInUse));
    otPlatSettingsInit(aInstance);
}

#ifdef __cplusplus
};
#endif
