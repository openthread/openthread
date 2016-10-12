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

#include <openthread-types.h>
#include <openthread-core-config.h>

#include <common/code_utils.hpp>
#include <platform/settings.h>
#include <platform/flash.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    kBlockAddBeginFlag = 0x1,
    kBlockAddCompleteFlag = 0x02,
    kBlockDeleteFlag = 0x04,
};

enum
{
    kSettingsFlagSize = 4,
    kSettingsBlockDataSize = 32,

    kSettingsInSwap = 0xbe5cc5ef,
    kSettingsInUse = 0xbe5cc5ee,
    kSettingsNotUse = 0xbe5cc5ec,
};

OT_TOOL_PACKED_BEGIN
struct settingsBlock
{
    uint16_t key;
    uint8_t index;
    uint8_t flag;
    uint16_t length;
    uint16_t reserved;
} OT_TOOL_PACKED_END;

static uint32_t sBase;
static uint32_t sUsedSize;

static void setSettingsFlag(uint32_t aBase, uint32_t aFlag)
{
    otPlatFlashWrite(aBase, reinterpret_cast<uint8_t *>(&aFlag), sizeof(aFlag));
}

static void initSettings(uint32_t aBase, uint32_t aFlag)
{
    uint32_t address = aBase;
    uint32_t settingsSize = OPENTHREAD_CONFIG_SETTINGS_PAGE_NUM > 1 ?
                            OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE * OPENTHREAD_CONFIG_SETTINGS_PAGE_NUM / 2 :
                            OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE;

    while (address < (aBase + settingsSize))
    {
        otPlatFlashErasePage(address);
        otPlatFlashStatusWait(1000);
        address += OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE;
    }

    setSettingsFlag(aBase, aFlag);
}

static uint32_t swapSettingsBlock(void)
{
    uint32_t oldBase = sBase;
    uint32_t swapAddress = sBase;
    uint32_t usedSize = sUsedSize;
    uint32_t settingsSize = OPENTHREAD_CONFIG_SETTINGS_PAGE_NUM > 1 ?
                            OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE * OPENTHREAD_CONFIG_SETTINGS_PAGE_NUM / 2 :
                            OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE;

    VerifyOrExit(OPENTHREAD_CONFIG_SETTINGS_PAGE_NUM > 1, ;);

    sBase = (swapAddress == OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS) ? (swapAddress + settingsSize) :
            OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS;

    initSettings(sBase, kSettingsInSwap);
    sUsedSize = kSettingsFlagSize;
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

        otPlatFlashRead(swapAddress, reinterpret_cast<uint8_t *>(&addBlock.block), sizeof(struct settingsBlock));
        swapAddress += sizeof(struct settingsBlock);

        if (!(addBlock.block.flag & kBlockAddCompleteFlag) && (addBlock.block.flag & kBlockDeleteFlag))
        {
            uint32_t address = swapAddress + addBlock.block.length;

            while (address < (oldBase + usedSize))
            {
                struct settingsBlock block;

                otPlatFlashRead(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));

                if (!(block.flag & kBlockAddCompleteFlag) && (block.flag & kBlockDeleteFlag) &&
                    (block.key == addBlock.block.key) && (block.index <= addBlock.block.index))
                {
                    valid = false;
                    break;
                }

                address += (block.length + sizeof(struct settingsBlock));
            }

            if (valid)
            {
                otPlatFlashRead(swapAddress, addBlock.data, addBlock.block.length);
                otPlatFlashWrite(sBase + sUsedSize, reinterpret_cast<uint8_t *>(&addBlock),
                                 addBlock.block.length + sizeof(struct settingsBlock));
                sUsedSize += (sizeof(struct settingsBlock) + addBlock.block.length);
            }
        }
        else if (addBlock.block.flag == 0xff)
        {
            break;
        }

        swapAddress += addBlock.block.length;
    }

    setSettingsFlag(sBase, kSettingsInUse);
    setSettingsFlag(oldBase, kSettingsNotUse);

exit:
    return settingsSize - sUsedSize;
}

static ThreadError addSetting(uint16_t aKey, uint8_t aIndex, const uint8_t *aValue, int aValueLength)
{
    ThreadError error = kThreadError_None;
    OT_TOOL_PACKED_BEGIN
    struct addSettingsBlock
    {
        struct settingsBlock block;
        uint8_t data[kSettingsBlockDataSize];
    } OT_TOOL_PACKED_END addBlock;
    uint32_t settingsSize = OPENTHREAD_CONFIG_SETTINGS_PAGE_NUM > 1 ?
                            OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE * OPENTHREAD_CONFIG_SETTINGS_PAGE_NUM / 2 :
                            OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE;

    addBlock.block.flag = 0xff;
    addBlock.block.key = aKey;
    addBlock.block.index = aIndex;
    addBlock.block.flag &= (~kBlockAddBeginFlag);
    addBlock.block.length = static_cast<uint16_t>(aValueLength);

    if ((sUsedSize + addBlock.block.length + sizeof(struct settingsBlock)) >= settingsSize)
    {
        VerifyOrExit(swapSettingsBlock() >= (addBlock.block.length + sizeof(struct settingsBlock)),
                     error = kThreadError_NoBufs);
    }

    otPlatFlashWrite(sBase + sUsedSize, reinterpret_cast<uint8_t *>(&addBlock.block),
                     sizeof(struct settingsBlock));

    memcpy(addBlock.data, aValue, addBlock.block.length);

    // padding
    while (addBlock.block.length & 3)
    {
        addBlock.data[addBlock.block.length++] = 0xff;
    }

    otPlatFlashWrite(sBase + sUsedSize + sizeof(struct settingsBlock),
                     reinterpret_cast<uint8_t *>(addBlock.data), addBlock.block.length);

    addBlock.block.flag &= (~kBlockAddCompleteFlag);
    otPlatFlashWrite(sBase + sUsedSize, reinterpret_cast<uint8_t *>(&addBlock.block),
                     sizeof(struct settingsBlock));
    sUsedSize += (sizeof(struct settingsBlock) + addBlock.block.length);

exit:
    return error;
}

// settings API
void otPlatSettingsInit(otInstance *aInstance)
{
    uint8_t index;
    uint32_t settingsSize = OPENTHREAD_CONFIG_SETTINGS_PAGE_NUM > 1 ?
                            OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE * OPENTHREAD_CONFIG_SETTINGS_PAGE_NUM / 2 :
                            OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE;
    (void)aInstance;

    sBase = OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS;

    for (index = 0; index < 2; index++)
    {
        uint32_t blockFlag;

        sBase += settingsSize * index;
        otPlatFlashRead(sBase, reinterpret_cast<uint8_t *>(&blockFlag), sizeof(blockFlag));

        if (blockFlag == kSettingsInUse)
        {
            break;
        }
    }

    if (index == 2)
    {
        initSettings(sBase, kSettingsInUse);
    }

    sUsedSize = kSettingsFlagSize;

    while (sUsedSize < settingsSize)
    {
        struct settingsBlock block;

        otPlatFlashRead(sBase + sUsedSize, reinterpret_cast<uint8_t *>(&block), sizeof(block));

        if (!(block.flag & kBlockAddBeginFlag))
        {
            sUsedSize += (block.length + sizeof(struct settingsBlock));
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

ThreadError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, int *aValueLength)
{
    ThreadError error = kThreadError_NotFound;
    uint32_t address = sBase + kSettingsFlagSize;
    (void)aInstance;

    while (address < (sBase + sUsedSize))
    {
        struct settingsBlock block;

        otPlatFlashRead(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));

        if (block.key == aKey && block.index == static_cast<uint8_t>(aIndex) &&
            (!(block.flag & kBlockAddCompleteFlag) && (block.flag & kBlockDeleteFlag)))
        {
            error = kThreadError_None;

            if (aValueLength)
            {
                *aValueLength = block.length;
            }

            if (aValue)
            {
                VerifyOrExit(aValueLength, error = kThreadError_InvalidArgs);
                otPlatFlashRead(address + sizeof(struct settingsBlock), aValue, block.length);
            }
        }

        address += (block.length + sizeof(struct settingsBlock));
    }

exit:
    return error;
}

ThreadError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, int aValueLength)
{
    (void)aInstance;

    return addSetting(aKey, 0, aValue, aValueLength);
}

ThreadError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, int aValueLength)
{
    int index = -1;
    uint32_t address = sBase + kSettingsFlagSize;
    (void)aInstance;

    while (address < (sBase + sUsedSize))
    {
        struct settingsBlock block;

        otPlatFlashRead(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));

        if (!(block.flag & kBlockAddCompleteFlag) && (block.flag & kBlockDeleteFlag) &&
            (block.key == aKey))
        {
            index = block.index;
        }
        else if (block.flag == 0xff)
        {
            break;
        }

        address += (block.length + sizeof(struct settingsBlock));
    }

    return addSetting(aKey, static_cast<uint8_t>(index + 1), aValue, aValueLength);
}

ThreadError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
    ThreadError error = kThreadError_NotFound;
    uint32_t address = sBase + kSettingsFlagSize;
    (void)aInstance;

    while (address < (sBase + sUsedSize))
    {
        struct settingsBlock block;

        otPlatFlashRead(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));

        if (!(block.flag & kBlockAddCompleteFlag) && (block.flag & kBlockDeleteFlag) &&
            (block.key == aKey) && (block.index == aIndex || aIndex == -1))
        {
            error = kThreadError_None;

            block.flag &= (~kBlockDeleteFlag);
            otPlatFlashWrite(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));
        }

        address += (block.length + sizeof(struct settingsBlock));
    }

    return error;
}

void otPlatSettingsWipe(otInstance *aInstance)
{
    (void)aInstance;

    initSettings(sBase, kSettingsInUse);
    otPlatSettingsInit(aInstance);
}

#ifdef __cplusplus
};
#endif
