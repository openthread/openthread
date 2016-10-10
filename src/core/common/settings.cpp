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
    kBlockAddFlag = 0x1,
    kBlockDeleteFlag = 0x02,
};

enum
{
    kMaxStageAddNum = 3,
    kMaxStageDeleteNum = 3,
    kMaxStageDataLen = 32,
};

enum
{
    kSettingsFlagSize = 4,
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

OT_TOOL_PACKED_BEGIN
struct stageAddSettingsBlock
{
    struct settingsBlock block;
    uint8_t data[kMaxStageDataLen];
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
struct stageDeleteSettingsBlock
{
    uint16_t key;
    int index;
} OT_TOOL_PACKED_END;

static uint32_t sBase;
static uint32_t sSettingsSize;
static uint32_t sUsedSize;
static bool sCommitLock = false;
static bool sSwapDone = false;

static uint16_t sStageActionSeq;
static struct stageAddSettingsBlock sStageAddSettingsBlock[kMaxStageAddNum];
static uint8_t sStageAddSettingsNum;
static uint16_t sStageAddSettingsBufLength;
static struct stageDeleteSettingsBlock sStageDeleteSettingsBlock[kMaxStageDeleteNum];
static uint8_t sStageDeleteSettingsNum;

static void setSettingsFlag(uint32_t aBase, uint32_t aFlag)
{
    otPlatFlashWrite(aBase, reinterpret_cast<uint8_t *>(&aFlag), sizeof(aFlag));
}

static void initSettings(uint32_t aBase, uint32_t aFlag)
{
    uint32_t address = aBase;

    while (address < (aBase + sSettingsSize))
    {
        otPlatFlashErasePage(address);
        otPlatFlashStatusWait(1000);
        address += OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE;
    }

    setSettingsFlag(aBase, aFlag);
}

static void initCommitChanges(bool aLock)
{
    sCommitLock = aLock;

    memset(sStageAddSettingsBlock, 0xff, sizeof(sStageAddSettingsBlock));
    sStageAddSettingsNum = 0;
    sStageAddSettingsBufLength = 0;

    memset(sStageDeleteSettingsBlock, 0, sizeof(sStageDeleteSettingsBlock));
    sStageDeleteSettingsNum = 0;

    sStageActionSeq = 0xffff;
}

static uint32_t swapSettingsBlock(void)
{
    uint32_t oldBase = sBase;
    uint32_t swapAddress = sBase;

    VerifyOrExit((sSwapDone == false) && (OPENTHREAD_CONFIG_SETTINGS_PAGE_NUM > 1), ;);

    sBase = (swapAddress == OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS) ? (swapAddress + sSettingsSize) :
            OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS;

    initSettings(sBase, kSettingsInSwap);
    sUsedSize = kSettingsFlagSize;
    swapAddress += kSettingsFlagSize;

    while (swapAddress < (oldBase + sSettingsSize))
    {
        struct stageAddSettingsBlock block;

        otPlatFlashRead(swapAddress, reinterpret_cast<uint8_t *>(&block.block), sizeof(struct settingsBlock));
        swapAddress += sizeof(struct settingsBlock);

        if (!(block.block.flag & kBlockAddFlag) && (block.block.flag & kBlockDeleteFlag))
        {
            otPlatFlashRead(swapAddress, block.data, block.block.length);
            otPlatFlashWrite(sBase + sUsedSize, reinterpret_cast<uint8_t *>(&block),
                             block.block.length + sizeof(struct settingsBlock));
            sUsedSize += (sizeof(struct settingsBlock) + block.block.length);
        }
        else if (block.block.flag == 0xff)
        {
            break;
        }

        swapAddress += block.block.length;
    }

    setSettingsFlag(sBase, kSettingsInUse);
    setSettingsFlag(oldBase, kSettingsNotUse);
    sSwapDone = true;

exit:
    return sSettingsSize - sUsedSize;
}

// settings API
void otPlatSettingsInit(void)
{
    uint8_t index;

    sSettingsSize = OPENTHREAD_CONFIG_SETTINGS_PAGE_NUM > 1 ?
                    OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE * OPENTHREAD_CONFIG_SETTINGS_PAGE_NUM / 2 :
                    OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE;

    initCommitChanges(false);

    sBase = OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS;

    for (index = 0; index < 2; index++)
    {
        uint32_t blockFlag;

        sBase += sSettingsSize * index;
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

    while (sUsedSize < sSettingsSize)
    {
        struct settingsBlock block;

        otPlatFlashRead(sBase + sUsedSize, reinterpret_cast<uint8_t *>(&block), sizeof(block));

        if (!(block.flag & kBlockAddFlag))
        {
            sUsedSize += (block.length + sizeof(struct settingsBlock));
        }
        else
        {
            break;
        }
    }
}

ThreadError otPlatSettingsBeginChange(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(sCommitLock == false, error = kThreadError_Already);
    initCommitChanges(true);

exit:
    return error;
}

ThreadError otPlatSettingsCommitChange(void)
{
    ThreadError error = kThreadError_None;
    uint8_t stageAddIndex;
    uint8_t stageDeleteIndex;

    VerifyOrExit(sCommitLock == true, error = kThreadError_InvalidState);

    if ((sUsedSize + sStageAddSettingsBufLength) >= sSettingsSize)
    {
        VerifyOrExit(swapSettingsBlock() >= sStageAddSettingsBufLength, error = kThreadError_NoBufs);
    }

    stageAddIndex = 0;
    stageDeleteIndex = 0;

    while ((stageAddIndex + stageDeleteIndex) < (sStageAddSettingsNum + sStageDeleteSettingsNum))
    {
        if (sStageActionSeq & (1 << (stageAddIndex + stageDeleteIndex)))
        {
            struct stageAddSettingsBlock *stageAddBlock = &sStageAddSettingsBlock[stageAddIndex++];

            otPlatFlashWrite(sBase + sUsedSize, reinterpret_cast<uint8_t *>(stageAddBlock),
                             sizeof(struct settingsBlock) + stageAddBlock->block.length);
            sUsedSize += (sizeof(struct settingsBlock) + stageAddBlock->block.length);
        }
        else
        {
            uint32_t address = sBase + kSettingsFlagSize;
            struct stageDeleteSettingsBlock *stageDeleteBlock = &sStageDeleteSettingsBlock[stageDeleteIndex++];
            struct settingsBlock block;

            while (address < (sBase + sSettingsSize))
            {
                otPlatFlashRead(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));

                if (!(block.flag & kBlockAddFlag) && (block.flag & kBlockDeleteFlag) &&
                    (block.key == stageDeleteBlock->key) &&
                    (block.index == static_cast<uint8_t>(stageDeleteBlock->index) || stageDeleteBlock->index == -1))
                {
                    block.flag &= (~kBlockDeleteFlag);
                    otPlatFlashWrite(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));
                    sSwapDone = false;
                    VerifyOrExit(stageDeleteBlock->index == -1, ;);
                }

                address += (block.length + sizeof(struct settingsBlock));
            }
        }
    }

exit:
    initCommitChanges(false);
    return error;
}

ThreadError otPlatSettingsAbandonChange(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(sCommitLock == true, error = kThreadError_InvalidState);
    initCommitChanges(false);

exit:
    return error;
}

ThreadError otPlatSettingsGet(uint16_t aKey, int aIndex, uint8_t *aValue, int *aValueLength)
{
    ThreadError error = kThreadError_NotFound;
    uint32_t address = sBase + kSettingsFlagSize;

    while (address < (sBase + sSettingsSize))
    {
        struct settingsBlock block;

        otPlatFlashRead(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));

        if (block.key == aKey && block.index == static_cast<uint8_t>(aIndex) &&
            (!(block.flag & kBlockAddFlag) && (block.flag & kBlockDeleteFlag)))
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

            break;
        }

        address += (block.length + sizeof(struct settingsBlock));
    }

exit:
    return error;
}

ThreadError otPlatSettingsSet(uint16_t aKey, const uint8_t *aValue, int aValueLength)
{
    ThreadError error = kThreadError_None;
    uint32_t address = sBase + kSettingsFlagSize;

    while (address < (sBase + sSettingsSize))
    {
        struct settingsBlock block;

        otPlatFlashRead(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));

        if ((!(block.flag & kBlockAddFlag) && block.flag & kBlockDeleteFlag) && (block.key == aKey))
        {
            SuccessOrExit(error = otPlatSettingsDelete(aKey, -1));
            break;
        }

        address += (block.length + sizeof(struct settingsBlock));
    }

    error = otPlatSettingsAdd(aKey, aValue, aValueLength);

exit:
    return error;
}

ThreadError otPlatSettingsAdd(uint16_t aKey, const uint8_t *aValue, int aValueLength)
{
    ThreadError error = kThreadError_None;
    struct stageAddSettingsBlock *stageBlock;
    int index = -1;
    uint32_t address = sBase + kSettingsFlagSize;

    VerifyOrExit((!sCommitLock) || (sCommitLock &&
                                    (sStageAddSettingsNum < kMaxStageAddNum)), error = kThreadError_NoBufs);

    stageBlock = &sStageAddSettingsBlock[sStageAddSettingsNum];
    stageBlock->block.key = aKey;

    while (address < (sBase + sSettingsSize))
    {
        struct settingsBlock block;

        otPlatFlashRead(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));

        if (!(block.flag & kBlockAddFlag) && (block.flag & kBlockDeleteFlag) && (block.key == aKey) && (block.index > index))
        {
            index = block.index;
        }
        else if (block.flag == 0xff)
        {
            break;
        }

        address += (block.length + sizeof(struct settingsBlock));
    }

    stageBlock->block.index = static_cast<uint8_t>(index + 1);

    if (sCommitLock)
    {
        for (index = 0; index < sStageAddSettingsNum; index++)
        {
            if ((sStageAddSettingsBlock[index].block.key == aKey) &&
                (sStageAddSettingsBlock[index].block.index >= stageBlock->block.index))
            {
                stageBlock->block.index = sStageAddSettingsBlock[index].block.index + 1;
            }
        }
    }

    stageBlock->block.flag &= (~kBlockAddFlag);
    stageBlock->block.length = static_cast<uint16_t>(aValueLength);
    memcpy(stageBlock->data, aValue, stageBlock->block.length);

    // padding
    while (stageBlock->block.length & 3)
    {
        stageBlock->data[stageBlock->block.length++] = 0xff;
    }

    if (!sCommitLock)
    {
        if ((sUsedSize + stageBlock->block.length + sizeof(struct settingsBlock)) >= sSettingsSize)
        {
            VerifyOrExit(swapSettingsBlock() >= (stageBlock->block.length + sizeof(struct settingsBlock)),
                         error = kThreadError_NoBufs);
        }

        otPlatFlashWrite(sBase + sUsedSize, reinterpret_cast<uint8_t *>(stageBlock),
                         sizeof(struct settingsBlock) + stageBlock->block.length);
        sUsedSize += (sizeof(struct settingsBlock) + stageBlock->block.length);
        stageBlock->block.flag = 0xff;
    }
    else
    {
        sStageAddSettingsNum++;
        sStageAddSettingsBufLength += (stageBlock->block.length + sizeof(struct settingsBlock));
    }

exit:
    return error;
}

ThreadError otPlatSettingsDelete(uint16_t aKey, int aIndex)
{
    ThreadError error = kThreadError_NotFound;
    uint32_t address = sBase + kSettingsFlagSize;

    VerifyOrExit((!sCommitLock) || (sCommitLock &&
                                    (sStageDeleteSettingsNum < kMaxStageDeleteNum)), error = kThreadError_NoBufs);

    while (address < (sBase + sSettingsSize))
    {
        struct settingsBlock block;

        otPlatFlashRead(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));

        if (!(block.flag & kBlockAddFlag) && (block.flag & kBlockDeleteFlag) &&
            (block.key == aKey) && (block.index == aIndex || aIndex == -1))
        {
            error = kThreadError_None;

            if (sCommitLock == false)
            {
                block.flag &= (~kBlockDeleteFlag);
                otPlatFlashWrite(address, reinterpret_cast<uint8_t *>(&block), sizeof(block));
                sSwapDone = false;
                VerifyOrExit(aIndex == -1, ;);
            }
            else
            {
                struct stageDeleteSettingsBlock *stageBlock = &sStageDeleteSettingsBlock[sStageDeleteSettingsNum];

                stageBlock->key = aKey;
                stageBlock->index = aIndex;
                sStageActionSeq &= ~(1 << (sStageAddSettingsNum + sStageDeleteSettingsNum));
                sStageDeleteSettingsNum++;
                ExitNow();
            }
        }

        address += (block.length + sizeof(struct settingsBlock));
    }

exit:
    return error;
}

void otPlatSettingsWipe(void)
{
    initSettings(sBase, kSettingsInUse);
    otPlatSettingsInit();
}

#ifdef __cplusplus
};
#endif
