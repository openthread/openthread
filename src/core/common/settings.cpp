/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
    BLOCK_ADD_FLAG = 0x1,
    BLOCK_DELETE_FLAG = 0x2,
};

enum
{
    MAX_STAGE_ADD_NUM = 3,
    MAX_STAGE_DELETE_NUM = 3,
    MAX_STAGE_DATA_LEN = 32,
};

struct settingsBlock
{
    uint16_t key;
    uint8_t index;
    uint8_t flag;
    uint16_t length;
    uint16_t reserved;
};

struct stageAddSettingsBlock
{
    struct settingsBlock block;
    uint8_t data[MAX_STAGE_DATA_LEN];
};

struct stageDeleteSettingsBlock
{
    uint16_t key;
    int index;
};


static uint32_t sAddress;
static uint32_t sSettingsBlockFlag = 0xbe5cc5ef;
static bool sCommitLock = false;

static uint16_t sStageActionSeq;
static struct stageAddSettingsBlock sStageAddSettingsBlock[MAX_STAGE_ADD_NUM];
static uint8_t sStageAddSettingsNum;
static uint16_t sStageAddSettingsBufLength;
static struct stageDeleteSettingsBlock sStageDeleteSettingsBlock[MAX_STAGE_DELETE_NUM];
static uint8_t sStageDeleteSettingsNum;

void reorderSettingsBlock(void)
{
    uint8_t *ramBuffer = (uint8_t *)malloc(OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE);
    uint32_t ramBufferOffset = 0;

    uint8_t readPageIndex = 0;
    uint32_t readAddress;
    uint32_t readPageOffset = 0;

    uint8_t writePageIndex = 0;
    uint32_t writeAddress;
    uint32_t writePageOffset = 0;

    uint32_t length = 0;
    uint32_t writeLength;
    bool storeLeft = false;
    bool endOfData = false;

    while ((readPageIndex < static_cast<uint8_t>(OPENTHREAD_CONFIG_SETTINGS_SIZE / OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE)) &&
           !endOfData)
    {
        readPageOffset = (readPageIndex == 0) ? sizeof(sSettingsBlockFlag) : 0;
        readAddress = OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS +
                      OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE * readPageIndex + readPageOffset;

        // read the left bytes of one block sitting in 2 pages
        if (storeLeft)
        {
            otPlatFlashRead(readAddress, ramBuffer + ramBufferOffset, length);
            ramBufferOffset += length;
            storeLeft = false;
        }

        readPageOffset += length;
        readAddress += length;
        length = 0;

        // read flash to ram
        while (readPageOffset < OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE)
        {
            struct settingsBlock block;

            otPlatFlashRead(readAddress, (uint8_t *)&block, sizeof(block));

            if (!(block.flag & BLOCK_ADD_FLAG))
            {
                writeLength = block.length + sizeof(struct settingsBlock);

                if ((readPageOffset + writeLength) > OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE)
                {
                    writeLength -= (readPageOffset + writeLength - OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE);
                }

                length = block.length + sizeof(struct settingsBlock) - writeLength;

                if (block.flag & BLOCK_DELETE_FLAG)
                {
                    otPlatFlashRead(readAddress, ramBuffer + ramBufferOffset, writeLength);
                    ramBufferOffset += writeLength;

                    if (length)
                    {
                        storeLeft = true;
                        break;
                    }
                }

                readPageOffset += writeLength;
                readAddress += writeLength;
                writeLength = 0;
            }
            else if (block.flag == 0xff)
            {
                endOfData = true;
                break;
            }
            else
            {
                assert(false);
            }
        }

        otPlatFlashErasePage(OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + readPageIndex * OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE);
        otPlatFlashStatusWait(1000);

        // write ram back to flash
        if (readPageIndex == 0)
        {
            otPlatFlashWrite(OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS, (uint8_t *)&sSettingsBlockFlag,
                             sizeof(sSettingsBlockFlag));
            writePageOffset = sizeof(sSettingsBlockFlag);
        }

        writeAddress = OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + writePageIndex * OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE +
                       writePageOffset;

        writeLength = (OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE - writePageOffset) > ramBufferOffset ?
                      ramBufferOffset : (OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE - writePageOffset);
        otPlatFlashWrite(writeAddress, ramBuffer, writeLength);
        writePageOffset += writeLength;

        if (ramBufferOffset > writeLength)
        {
            writePageIndex++;
            writePageOffset = 0;
            writeAddress = OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS
                           + writePageIndex * OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE + writePageOffset;
            otPlatFlashWrite(writeAddress, ramBuffer + writeLength, ramBufferOffset - writeLength);
            writePageOffset += (ramBufferOffset - writeLength);
        }

        ramBufferOffset = 0;
        readPageIndex++;
    }

    free(ramBuffer);

    otPlatSettingsInit();
}

// settings API
void otPlatSettingsInit(void)
{
    sAddress = OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS;

    memset(sStageAddSettingsBlock, 0xff, sizeof(sStageAddSettingsBlock));
    sStageAddSettingsNum = 0;
    sStageAddSettingsBufLength = 0;

    memset(sStageDeleteSettingsBlock, 0, sizeof(sStageDeleteSettingsBlock));
    sStageDeleteSettingsNum = 0;

    sStageActionSeq = 0xffff;

    sCommitLock = false;

    while (sAddress < OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + OPENTHREAD_CONFIG_SETTINGS_SIZE - 1)
    {
        struct settingsBlock block;

        if (sAddress == OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS)
        {
            uint32_t blockFlag;

            otPlatFlashRead(sAddress, (uint8_t *)&blockFlag, sizeof(blockFlag));

            if (blockFlag != sSettingsBlockFlag)
            {
                otPlatSettingsWipe();
                break;
            }

            sAddress += sizeof(sSettingsBlockFlag);
        }

        otPlatFlashRead(sAddress, (uint8_t *)&block, sizeof(block));

        if (!(block.flag & BLOCK_ADD_FLAG))
        {
            sAddress += (block.length + sizeof(struct settingsBlock));
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

    memset(sStageAddSettingsBlock, 0xff, sizeof(sStageAddSettingsBlock));
    sStageAddSettingsNum = 0;
    sStageAddSettingsBufLength = 0;

    memset(sStageDeleteSettingsBlock, 0, sizeof(sStageDeleteSettingsBlock));
    sStageDeleteSettingsNum = 0;

    sStageActionSeq = 0xffff;

    sCommitLock = true;

exit:
    return error;
}

ThreadError otPlatSettingsCommitChange(void)
{
    ThreadError error = kThreadError_None;
    uint8_t stageAddIndex;
    uint8_t stageDeleteIndex;
    uint32_t address = OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + sizeof(sSettingsBlockFlag);
    struct stageAddSettingsBlock *stageAddBlock;
    struct stageDeleteSettingsBlock *stageDeleteBlock;
    struct settingsBlock block;

    VerifyOrExit(sCommitLock == true, error = kThreadError_InvalidState);

    if ((sAddress + sStageAddSettingsBufLength) >= (OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS +
                                                    OPENTHREAD_CONFIG_SETTINGS_SIZE))
    {
        reorderSettingsBlock();
    }

    VerifyOrExit((sAddress + sStageAddSettingsBufLength) < (OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS +
                                                            OPENTHREAD_CONFIG_SETTINGS_SIZE),
                 error = kThreadError_NoBufs);

    stageAddIndex = 0;
    stageDeleteIndex = 0;

    while ((stageAddIndex + stageDeleteIndex) < (sStageAddSettingsNum + sStageDeleteSettingsNum))
    {
        if (sStageActionSeq & (1 << (stageAddIndex + stageDeleteIndex)))
        {
            stageAddBlock = &sStageAddSettingsBlock[stageAddIndex++];
            otPlatFlashWrite(sAddress, (uint8_t *)&stageAddBlock->block, sizeof(struct settingsBlock));
            otPlatFlashWrite(sAddress + sizeof(struct settingsBlock), (uint8_t *)stageAddBlock->data, stageAddBlock->block.length);
            sAddress += (sizeof(struct settingsBlock) + stageAddBlock->block.length);
        }
        else
        {
            stageDeleteBlock = &sStageDeleteSettingsBlock[stageDeleteIndex++];
            address = OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + sizeof(sSettingsBlockFlag);

            while (address < (OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + OPENTHREAD_CONFIG_SETTINGS_SIZE))
            {
                otPlatFlashRead(address, (uint8_t *)&block, sizeof(block));

                if (!(block.flag & BLOCK_ADD_FLAG) && (block.flag & BLOCK_DELETE_FLAG) &&
                    (block.key == stageDeleteBlock->key) &&
                    (block.index == static_cast<uint8_t>(stageDeleteBlock->index) || stageDeleteBlock->index == -1))
                {
                    block.flag &= (~BLOCK_DELETE_FLAG);
                    otPlatFlashWrite(address, (uint8_t *)&block, sizeof(block));

                    if (stageDeleteBlock->index != -1)
                    {
                        break;
                    }
                }

                address += (block.length + sizeof(struct settingsBlock));
            }
        }
    }

exit:
    sCommitLock = false;

    memset(sStageAddSettingsBlock, 0xff, sizeof(sStageAddSettingsBlock));
    sStageAddSettingsNum = 0;
    sStageAddSettingsBufLength = 0;

    memset(sStageDeleteSettingsBlock, 0, sizeof(sStageDeleteSettingsBlock));
    sStageDeleteSettingsNum = 0;

    sStageActionSeq = 0xffff;

    return error;
}

ThreadError otPlatSettingsAbandonChange(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(sCommitLock == true, error = kThreadError_InvalidState);

    sCommitLock = false;
    memset(sStageAddSettingsBlock, 0xff, sizeof(sStageAddSettingsBlock));
    sStageAddSettingsNum = 0;
    sStageAddSettingsBufLength = 0;

    memset(sStageDeleteSettingsBlock, 0, sizeof(sStageDeleteSettingsBlock));
    sStageDeleteSettingsNum = 0;

    sStageActionSeq = 0xffff;

exit:
    return error;
}

ThreadError otPlatSettingsGet(uint16_t aKey, int aIndex, uint8_t *aValue, int *aValueLength)
{
    ThreadError error = kThreadError_NotFound;
    uint32_t address = OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + sizeof(sSettingsBlockFlag);

    while (address < (OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + OPENTHREAD_CONFIG_SETTINGS_SIZE))
    {
        struct settingsBlock block;

        otPlatFlashRead(address, (uint8_t *)&block, sizeof(block));

        if (block.key == aKey && block.index == static_cast<uint8_t>(aIndex) &&
            (!(block.flag & BLOCK_ADD_FLAG) && (block.flag & BLOCK_DELETE_FLAG)))
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
    uint32_t address = OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + sizeof(sSettingsBlockFlag);

    while (address < (OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + OPENTHREAD_CONFIG_SETTINGS_SIZE))
    {
        struct settingsBlock block;

        otPlatFlashRead(address, (uint8_t *)&block, sizeof(block));

        if ((!(block.flag & BLOCK_ADD_FLAG) && block.flag & BLOCK_DELETE_FLAG) && (block.key == aKey))
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
    struct settingsBlock block;
    int index = -1;
    uint32_t address = OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + sizeof(sSettingsBlockFlag);

    VerifyOrExit((!sCommitLock) || (sCommitLock &&
                                    (sStageAddSettingsNum < MAX_STAGE_ADD_NUM)), error = kThreadError_NoBufs);

    stageBlock = &sStageAddSettingsBlock[sStageAddSettingsNum];
    stageBlock->block.key = aKey;

    while (address < (OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + OPENTHREAD_CONFIG_SETTINGS_SIZE))
    {
        otPlatFlashRead(address, (uint8_t *)&block, sizeof(block));

        if (!(block.flag & BLOCK_ADD_FLAG) && (block.flag & BLOCK_DELETE_FLAG) && (block.key == aKey) && (block.index > index))
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

    stageBlock->block.flag &= (~BLOCK_ADD_FLAG);
    stageBlock->block.length = static_cast<uint16_t>(aValueLength);
    memcpy(stageBlock->data, aValue, stageBlock->block.length);

    // padding
    while (stageBlock->block.length & 3)
    {
        stageBlock->data[stageBlock->block.length++] = 0xff;
    }

    if (!sCommitLock)
    {
        if ((sAddress + stageBlock->block.length + sizeof(struct settingsBlock)) >=
            (OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + OPENTHREAD_CONFIG_SETTINGS_SIZE))
        {
            reorderSettingsBlock();
        }

        VerifyOrExit((sAddress + stageBlock->block.length + sizeof(struct settingsBlock)) <
                     (OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + OPENTHREAD_CONFIG_SETTINGS_SIZE), error = kThreadError_NoBufs);

        otPlatFlashWrite(sAddress, (uint8_t *)&stageBlock->block, sizeof(struct settingsBlock));
        otPlatFlashWrite(sAddress + sizeof(struct settingsBlock), (uint8_t *)stageBlock->data, stageBlock->block.length);
        sAddress += (stageBlock->block.length + sizeof(struct settingsBlock));
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
    uint32_t address = OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + sizeof(sSettingsBlockFlag);

    VerifyOrExit((!sCommitLock) || (sCommitLock &&
                                    (sStageDeleteSettingsNum < MAX_STAGE_DELETE_NUM)), error = kThreadError_NoBufs);

    while (address < OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + OPENTHREAD_CONFIG_SETTINGS_SIZE)
    {
        struct settingsBlock block;

        otPlatFlashRead(address, (uint8_t *)&block, sizeof(block));

        if (!(block.flag & BLOCK_ADD_FLAG) && (block.flag & BLOCK_DELETE_FLAG) &&
            (block.key == aKey) && (block.index == aIndex || aIndex == -1))
        {
            error = kThreadError_None;

            if (sCommitLock == false)
            {
                block.flag &= (~BLOCK_DELETE_FLAG);
                otPlatFlashWrite(address, (uint8_t *)&block, sizeof(block));
                VerifyOrExit(aIndex == -1, ;);
            }
            else
            {
                struct stageDeleteSettingsBlock *stageBlock = &sStageDeleteSettingsBlock[sStageDeleteSettingsNum];

                stageBlock->key = aKey;
                stageBlock->index = aIndex;
                sStageActionSeq &= ~(1 << (sStageAddSettingsNum + sStageDeleteSettingsNum));
                sStageDeleteSettingsNum++;
                break;
            }
        }

        address += (block.length + sizeof(struct settingsBlock));
    }

exit:
    return error;
}

void otPlatSettingsWipe(void)
{
    uint32_t address = OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS;

    while (address < OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS + OPENTHREAD_CONFIG_SETTINGS_SIZE)
    {
        otPlatFlashErasePage(address);
        otPlatFlashStatusWait(1000);
        address += OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE;
    }

    otPlatFlashWrite(OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS, (uint8_t *)&sSettingsBlockFlag, sizeof(sSettingsBlockFlag));
    otPlatSettingsInit();
}

#ifdef __cplusplus
};
#endif
