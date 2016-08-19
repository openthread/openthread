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
    MAX_KEY_VALUE = 128,
    MAX_BLOCKS_NUM = 256,
    MAX_STAGE_ADD_NUM = 16,
    MAX_STAGE_DELETE_NUM = 16,
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

struct settingsBlockMgmt
{
    uint32_t cur;
    struct settingsBlockMgmt *prev;
    struct settingsBlockMgmt *next;
};

struct settingsList
{
    struct settingsBlockMgmt *head;
    struct settingsBlockMgmt *tail;
};

struct stageAddSettingsBlock
{
    struct settingsBlock block;
    uint8_t data[MAX_STAGE_DATA_LEN];
};

struct stageDeleteSettingsBlock
{
    bool deleteList;
    struct settingsBlockMgmt *blockMgmt;
};

static uint32_t sBaseAddress;

static uint32_t sSettingsBlockFlag = 0xbe5cc5ef;

static struct settingsList sSettingsList[MAX_KEY_VALUE];
static struct settingsBlockMgmt sSettingsBlockMgmt[MAX_BLOCKS_NUM];
static uint16_t sSettingsBlockMgmtNum;

static uint32_t sStageActionSeq;
static struct stageAddSettingsBlock sStageAddSettingsBlock[MAX_STAGE_ADD_NUM];
static uint8_t sStageAddSettingsNum;
static uint16_t sStageAddSettingsBufLength;
static struct stageDeleteSettingsBlock sStageDeleteSettingsBlock[MAX_STAGE_DELETE_NUM];
static uint8_t sStageDeleteSettingsNum;

static uint32_t sAddress;
static bool sCommitLock = false;

// settings block management structure in ram
static void enqueueSettingsBlockMgmt(uint16_t aKey, uint32_t aBlock)
{
    uint16_t index;
    uint32_t endAddress = sBaseAddress + OPENTHREAD_CONFIG_SETTINGS_SIZE - 1;
    struct settingsList *list;
    struct settingsBlockMgmt *blockMgmt;

    assert((aBlock >= sBaseAddress) && (aBlock <= endAddress));

    for (index = 0; index < MAX_BLOCKS_NUM; index++)
    {
        if (sSettingsBlockMgmt[index].cur == 0)
        {
            break;
        }
    }

    assert(index < MAX_BLOCKS_NUM);

    list = &sSettingsList[aKey];
    blockMgmt = &sSettingsBlockMgmt[index];

    blockMgmt->cur = aBlock;
    blockMgmt->prev = list->tail;
    blockMgmt->next = NULL;

    if (list->head == NULL)
    {
        list->head = blockMgmt;
        list->tail = blockMgmt;
    }
    else
    {
        list->tail->next = blockMgmt;
        list->tail = blockMgmt;
    }

    sSettingsBlockMgmtNum++;
}

static struct settingsBlockMgmt *dequeueSettingsBlockMgmt(struct settingsBlockMgmt *aBlockMgmt)
{
    struct settingsList *list;
    struct settingsBlockMgmt *rval;
    struct settingsBlock block;

    assert(aBlockMgmt && aBlockMgmt->cur);

    otPlatFlashRead(aBlockMgmt->cur, (uint8_t *)&block, sizeof(block));
    list = &sSettingsList[block.key];
    aBlockMgmt->cur = 0;

    if (aBlockMgmt->prev)
    {
        aBlockMgmt->prev->next = aBlockMgmt->next;
    }
    else
    {
        list->head = aBlockMgmt->next;
    }

    if (aBlockMgmt->next)
    {
        aBlockMgmt->next->prev = aBlockMgmt->prev;
    }
    else
    {
        list->tail = aBlockMgmt->prev;
    }

    rval = aBlockMgmt->next;
    aBlockMgmt->prev = NULL;
    aBlockMgmt->next = NULL;

    sSettingsBlockMgmtNum--;

    return rval;
}

void reorderSettingsBlock(void)
{
    uint8_t reorderBuffer[OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE];
    uint8_t *ramBufferStart;
    uint8_t *ramBuffer;
    uint32_t ramBufferOffset = 0;

    uint8_t readPageIndex = 0;
    uint32_t readAddress;
    uint32_t readPageOffset = 0;

    uint8_t writePageIndex = 0;
    uint32_t writeAddress;
    uint32_t writePageOffset = 0;

    uint8_t pageNum;
    struct settingsBlock block;
    uint32_t length;
    uint32_t writeLength;
    bool storeLeft = false;
    bool endOfData = false;

    ramBufferStart = reorderBuffer;
    ramBuffer = ramBufferStart;
    pageNum = static_cast<uint8_t>(OPENTHREAD_CONFIG_SETTINGS_SIZE / otPlatFlashGetPageSize());
    length = 0;

    while ((readPageIndex < pageNum) && !endOfData)
    {
        readAddress = sBaseAddress + otPlatFlashGetPageSize() * readPageIndex;
        readPageOffset = (readPageIndex == 0) ? sizeof(sSettingsBlockFlag) : 0;
        readAddress += readPageOffset;

        readPageOffset += length;

        // read the left bytes of one block sitting in 2 pages
        if (storeLeft)
        {
            otPlatFlashRead(readAddress, ramBuffer, length);
            ramBuffer += length;
            ramBufferOffset += length;
        }

        readAddress += length;
        length = 0;
        storeLeft = false;

        // read flash to ram
        while (readPageOffset < otPlatFlashGetPageSize())
        {
            otPlatFlashRead(readAddress, (uint8_t *)&block, sizeof(block));

            if (!(block.flag & BLOCK_ADD_FLAG))
            {
                writeLength = block.length + sizeof(struct settingsBlock);

                if ((readPageOffset + writeLength) > otPlatFlashGetPageSize())
                {
                    writeLength -= (readPageOffset + writeLength - otPlatFlashGetPageSize());
                }

                readPageOffset += writeLength;
                length = block.length + sizeof(struct settingsBlock) - writeLength;

                if (block.flag & BLOCK_DELETE_FLAG)
                {
                    otPlatFlashRead(readAddress, ramBuffer, writeLength);
                    ramBuffer += writeLength;
                    ramBufferOffset += writeLength;
                    readAddress += writeLength;
                    writeLength = 0;

                    if (length)
                    {
                        storeLeft = true;
                        break;
                    }
                }
                else
                {
                    readAddress += writeLength;
                    writeLength = 0;
                }
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

        otPlatFlashErasePage(sBaseAddress + otPlatFlashGetPageSize() * readPageIndex, otPlatFlashGetPageSize());

        if (readPageIndex == 0)
        {
            otPlatFlashWrite(sBaseAddress, (uint8_t *)&sSettingsBlockFlag, sizeof(sSettingsBlockFlag));
            writePageOffset = sizeof(sSettingsBlockFlag);
        }

        // write ram back to flash
        writeAddress = sBaseAddress + writePageIndex * otPlatFlashGetPageSize() + writePageOffset;

        writeLength = (otPlatFlashGetPageSize() - writePageOffset) > ramBufferOffset ? ramBufferOffset :
                      (otPlatFlashGetPageSize() - writePageOffset);
        otPlatFlashWrite(writeAddress, ramBufferStart, writeLength);
        writePageOffset += writeLength;

        if (ramBufferOffset > writeLength)
        {
            writePageIndex++;
            writePageOffset = 0;
            writeAddress = otPlatFlashGetBaseAddress() + OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS
                           + writePageIndex * otPlatFlashGetPageSize() + writePageOffset;
            otPlatFlashWrite(writeAddress, ramBufferStart + writeLength, ramBufferOffset - writeLength);
            writePageOffset += (ramBufferOffset - writeLength);
        }

        ramBuffer = ramBufferStart;
        ramBufferOffset = 0;
        readPageIndex++;
    }

    otPlatSettingsInit();
}

// settings API
void otPlatSettingsInit(void)
{
    uint32_t endAddress;
    struct settingsBlock block;
    uint32_t blockFlag;

    sBaseAddress = otPlatFlashGetBaseAddress() + OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS;
    sAddress = sBaseAddress;
    endAddress = sAddress + OPENTHREAD_CONFIG_SETTINGS_SIZE - 1;

    assert(!(sAddress & otPlatFlashGetPageSize()));

    memset(sSettingsList, 0, sizeof(sSettingsList));
    memset(sSettingsBlockMgmt, 0, sizeof(sSettingsBlockMgmt));
    sSettingsBlockMgmtNum = 0;

    memset(sStageAddSettingsBlock, 0xff, sizeof(sStageAddSettingsBlock));
    sStageAddSettingsNum = 0;
    sStageAddSettingsBufLength = 0;

    memset(sStageDeleteSettingsBlock, 0, sizeof(sStageDeleteSettingsBlock));
    sStageDeleteSettingsNum = 0;

    sStageActionSeq = 0xffffffff;

    sCommitLock = false;

    // initialize settings mangement data structure
    while (sAddress < endAddress)
    {
        if (sAddress == sBaseAddress)
        {
            otPlatFlashRead(sAddress, (uint8_t *)&blockFlag, sizeof(blockFlag));

            if (blockFlag != sSettingsBlockFlag)
            {
                break;
            }

            sAddress += sizeof(sSettingsBlockFlag);
        }

        otPlatFlashRead(sAddress, (uint8_t *)&block, sizeof(block));

        if (!(blockFlag & BLOCK_DELETE_FLAG))
        {
            sAddress += (block.length + sizeof(struct settingsBlock));
        }
        else if (!(block.flag & BLOCK_ADD_FLAG))
        {
            assert(block.key <= MAX_KEY_VALUE);
            assert(sSettingsBlockMgmtNum <= MAX_BLOCKS_NUM);

            enqueueSettingsBlockMgmt(block.key, sAddress);
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

    sCommitLock = true;

exit:
    return error;
}

ThreadError otPlatSettingsCommitChange(void)
{
    ThreadError error = kThreadError_None;
    uint8_t stageAddIndex;
    uint8_t stageDeleteIndex;
    uint16_t length;
    uint32_t endAddress;
    bool deleteList;
    struct stageAddSettingsBlock *stageAddBlock;
    struct stageDeleteSettingsBlock *stageDeleteBlock;
    struct settingsBlockMgmt *blockMgmt;
    struct settingsBlock block;

    VerifyOrExit(sCommitLock == true, error = kThreadError_InvalidState);

    endAddress = sBaseAddress + OPENTHREAD_CONFIG_SETTINGS_SIZE - 1;

    if ((sAddress + sStageAddSettingsBufLength) >= endAddress)
    {
        reorderSettingsBlock();
    }

    VerifyOrExit((sAddress + sStageAddSettingsBufLength) < endAddress, error = kThreadError_NoBufs);

    stageAddIndex = 0;
    stageDeleteIndex = 0;

    while ((stageAddIndex + stageDeleteIndex) < (sStageAddSettingsNum + sStageDeleteSettingsNum))
    {
        if (sStageActionSeq & (1 << (stageAddIndex + stageDeleteIndex)))
        {
            stageAddBlock = &sStageAddSettingsBlock[stageAddIndex++];
            length = sizeof(struct settingsBlock) + stageAddBlock->block.length;
            otPlatFlashWrite(sAddress, (uint8_t *)&stageAddBlock->block, sizeof(struct settingsBlock));
            otPlatFlashWrite(sAddress + sizeof(struct settingsBlock), (uint8_t *)stageAddBlock->data, stageAddBlock->block.length);
            enqueueSettingsBlockMgmt(stageAddBlock->block.key, sAddress);
            sAddress += length;
        }
        else
        {
            stageDeleteBlock = &sStageDeleteSettingsBlock[stageDeleteIndex++];
            blockMgmt = stageDeleteBlock->blockMgmt;
            deleteList = stageDeleteBlock->deleteList;

            while (blockMgmt)
            {
                otPlatFlashRead(blockMgmt->cur, (uint8_t *)&block, sizeof(block));
                block.flag &= (~BLOCK_DELETE_FLAG);
                otPlatFlashWrite(blockMgmt->cur, (uint8_t *)&block, sizeof(block));
                blockMgmt = dequeueSettingsBlockMgmt(blockMgmt);

                if (!deleteList)
                {
                    break;
                }
            }
        }
    }

exit:
    sCommitLock = false;

    memset(sStageAddSettingsBlock, 0, sizeof(sStageAddSettingsBlock));
    sStageAddSettingsNum = 0;
    sStageAddSettingsBufLength = 0;

    memset(sStageDeleteSettingsBlock, 0, sizeof(sStageDeleteSettingsBlock));
    sStageDeleteSettingsNum = 0;

    sStageActionSeq = 0xffffffff;

    return error;
}

ThreadError otPlatSettingsAbandonChange(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(sCommitLock == true, error = kThreadError_InvalidState);

    sCommitLock = false;
    memset(sStageAddSettingsBlock, 0, sizeof(sStageAddSettingsBlock));
    sStageAddSettingsNum = 0;
    sStageAddSettingsBufLength = 0;

    memset(sStageDeleteSettingsBlock, 0, sizeof(sStageDeleteSettingsBlock));
    sStageDeleteSettingsNum = 0;

    sStageActionSeq = 0xffffffff;

exit:
    return error;
}

ThreadError otPlatSettingsGet(uint16_t aKey, int aIndex, uint8_t *aValue, int *aValueLength)
{
    ThreadError error = kThreadError_NotFound;
    struct settingsList *list;
    struct settingsBlockMgmt *blockMgmt;
    uint32_t address;
    struct settingsBlock block;

    VerifyOrExit(sSettingsList[aKey].head, error = kThreadError_NotFound);

    list = &sSettingsList[aKey];
    blockMgmt = list->head;

    while (blockMgmt)
    {
        otPlatFlashRead(blockMgmt->cur, (uint8_t *)&block, sizeof(block));

        if (block.index == aIndex)
        {
            if (!(block.flag & BLOCK_ADD_FLAG) && (block.flag & BLOCK_DELETE_FLAG))
            {
                error = kThreadError_None;

                if (aValueLength)
                {
                    *aValueLength = block.length;
                }

                if (aValue)
                {
                    VerifyOrExit(aValueLength, error = kThreadError_InvalidArgs);
                    address = blockMgmt->cur + sizeof(struct settingsBlock);
                    otPlatFlashRead(address, aValue, block.length);
                }
            }

            break;
        }

        blockMgmt = blockMgmt->next;
    }

exit:
    return error;
}

ThreadError otPlatSettingsSet(uint16_t aKey, const uint8_t *aValue, int aValueLength)
{
    ThreadError error = kThreadError_None;
    struct settingsList *list;

    list = &sSettingsList[aKey];

    if (list->head == NULL)
    {
        error = otPlatSettingsAdd(aKey, aValue, aValueLength);
    }
    else
    {
        SuccessOrExit(error = otPlatSettingsDelete(aKey, -1));
        error = otPlatSettingsAdd(aKey, aValue, aValueLength);
    }

exit:
    return error;
}

ThreadError otPlatSettingsAdd(uint16_t aKey, const uint8_t *aValue, int aValueLength)
{
    ThreadError error = kThreadError_None;
    struct settingsList *list;
    struct stageAddSettingsBlock *stageBlock;
    struct settingsBlock block;
    uint8_t index;
    uint32_t endAddress;

    VerifyOrExit((!sCommitLock) || (sCommitLock &&
                                    (sStageAddSettingsNum < MAX_STAGE_ADD_NUM)), error = kThreadError_NoBufs);

    list = &sSettingsList[aKey];
    stageBlock = &sStageAddSettingsBlock[sStageAddSettingsNum];
    stageBlock->block.key = aKey;

    if (list->tail)
    {
        otPlatFlashRead(list->tail->cur, (uint8_t *)&block, sizeof(block));
        stageBlock->block.index = block.index + 1;
    }
    else
    {
        stageBlock->block.index = 0;
    }

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
        endAddress = sBaseAddress + OPENTHREAD_CONFIG_SETTINGS_SIZE - 1;

        if ((sAddress + stageBlock->block.length + sizeof(struct settingsBlock)) > endAddress)
        {
            reorderSettingsBlock();
        }

        otPlatFlashWrite(sAddress, (uint8_t *)&stageBlock->block, sizeof(struct settingsBlock));
        otPlatFlashWrite(sAddress + sizeof(struct settingsBlock), (uint8_t *)stageBlock->data, stageBlock->block.length);
        enqueueSettingsBlockMgmt(aKey, sAddress);
        sAddress += (stageBlock->block.length + sizeof(struct settingsBlock));
        stageBlock->block.flag = 0xff;
    }
    else
    {
        sStageAddSettingsBufLength += (stageBlock->block.length + sizeof(struct settingsBlock));
        sStageAddSettingsNum++;
    }

exit:
    return error;
}

ThreadError otPlatSettingsDelete(uint16_t aKey, int aIndex)
{
    ThreadError error = kThreadError_NotFound;
    struct settingsBlockMgmt *blockMgmt;
    struct stageDeleteSettingsBlock *stageBlock;
    struct settingsBlock block;

    VerifyOrExit(sSettingsList[aKey].head, error = kThreadError_NotFound);
    VerifyOrExit((!sCommitLock) || (sCommitLock &&
                                    (sStageDeleteSettingsNum < MAX_STAGE_DELETE_NUM)), error = kThreadError_NoBufs);

    blockMgmt = sSettingsList[aKey].head;

    while (blockMgmt)
    {
        otPlatFlashRead(blockMgmt->cur, (uint8_t *)&block, sizeof(block));

        if (aIndex == -1 || block.index == aIndex)
        {
            if (!(block.flag & BLOCK_ADD_FLAG) && (block.flag & BLOCK_DELETE_FLAG))
            {
                error = kThreadError_None;

                if (sCommitLock)
                {
                    stageBlock = &sStageDeleteSettingsBlock[sStageDeleteSettingsNum];
                    stageBlock->blockMgmt = blockMgmt;
                    stageBlock->deleteList = (aIndex == -1) ? true : false;
                    sStageActionSeq &= ~(1 << (sStageAddSettingsNum + sStageDeleteSettingsNum));
                    sStageDeleteSettingsNum++;
                    break;
                }
                else
                {
                    block.flag &= (~BLOCK_DELETE_FLAG);
                    otPlatFlashWrite(blockMgmt->cur, (uint8_t *)&block, sizeof(block));
                    blockMgmt = dequeueSettingsBlockMgmt(blockMgmt);

                    if (aIndex != -1)
                    {
                        break;
                    }
                }
            }
            else if (aIndex == -1)
            {
                blockMgmt = blockMgmt->next;
            }
            else
            {
                break;
            }
        }
        else
        {
            blockMgmt = blockMgmt->next;
        }
    }

exit:
    return error;
}

void otPlatSettingsWipe(void)
{
    otPlatFlashErasePage(sBaseAddress, OPENTHREAD_CONFIG_SETTINGS_SIZE);
    otPlatFlashWrite(sBaseAddress, (uint8_t *)&sSettingsBlockFlag, sizeof(sSettingsBlockFlag));
    otPlatSettingsInit();
}

// test functions
#if ENABLE_SETTINGS_API_TEST
int testSettingsApi(void)
{
    int rval = 0;
    ThreadError error = kThreadError_None;
    uint16_t key;
    uint8_t index;
    uint8_t writeBuffer[MAX_STAGE_DATA_LEN];
    int writeBufferLength;
    uint8_t readBuffer[MAX_STAGE_DATA_LEN];
    int readBufferLength = 32;

    otPlatFlashInit();
    otPlatSettingsInit();

    // wipe settings block flash area
    otPlatSettingsWipe();

    // prepare for add setting blocks
    for (index = 0; index < MAX_STAGE_DATA_LEN - 1; index++)
    {
        writeBuffer[index] = index;
    }

    writeBufferLength = index;

    // add setting blocks
    for (key = 7; key < 15; key++)
    {
        for (index = 0; index < 10; index++)
        {
            writeBuffer[0] = index;
            error = otPlatSettingsAdd(key, writeBuffer, (int)writeBufferLength);
            // -1: otPlatSettingsAdd error
            VerifyOrExit(error == kThreadError_None, rval = -1);
        }
    }

    for (key = 7; key < 15; key++)
    {
        for (index = 0; index < 10; index++)
        {
            error = otPlatSettingsGet(key, index, readBuffer, &readBufferLength);
            // -2: otPlatSettingsGet error to get setting block
            VerifyOrExit(error == kThreadError_None, rval = -2);
            // -3: otPlatSettingsAdd and otPlatSettingsGet not match
            VerifyOrExit(readBuffer[0] == index, rval = -3);
            VerifyOrExit(!memcmp(readBuffer + 1, writeBuffer + 1, static_cast<uint16_t>(writeBufferLength - 1)), rval = -3);
        }
    }

    // delete all setting blocks of one key
    key = 8;
    error = otPlatSettingsDelete(key, -1);
    // -4: otPlatSettingDelete error to delete all setting blocks of one key
    VerifyOrExit(error == kThreadError_None, rval = -4);

    for (index = 0; index < 10; index++)
    {
        error = otPlatSettingsGet(key, index, readBuffer, &readBufferLength);
        // -5: otPlatSettingsDelete error to delete all setting blocks of one key
        VerifyOrExit(error == kThreadError_NotFound, rval = -5);
    }

    // set one setting block
    key = 8;
    error = otPlatSettingsSet(key, writeBuffer, (int)writeBufferLength);
    // -6: otPlatSettingsSet error to set a new setting block
    VerifyOrExit(error == kThreadError_None, rval = -6);
    error = otPlatSettingsGet(key, 0, readBuffer, &readBufferLength);
    // -7: otPlatSettingsGet error to get after otPlatSettingsSet
    VerifyOrExit(error == kThreadError_None, rval = -7);
    // -8: otPlatSettingsGet and otPlatSettingsSet not match
    VerifyOrExit(!memcmp(readBuffer, writeBuffer, static_cast<uint16_t>(writeBufferLength)), rval = -8);

    // set one setting block
    key = 8;
    error = otPlatSettingsSet(key, writeBuffer, (int)writeBufferLength);
    // -9: otPlatSettingsSet error to set an existing setting block
    VerifyOrExit(error == kThreadError_None, rval = -9);
    error = otPlatSettingsGet(key, 0, readBuffer, &readBufferLength);
    // -10: otPlatSettingsGet error to get after otPlatSettingsSet for an existing setting block
    VerifyOrExit(error == kThreadError_None, rval = -10);
    // -11: otPlatSettingsGet and otPlatSettingsSet not match for an existing setting block
    VerifyOrExit(!memcmp(readBuffer, writeBuffer, static_cast<uint16_t>(writeBufferLength)), rval = -10);

    // commit
    otPlatSettingsBeginChange();
    key = 15;

    for (index = 0; index < 3; index++)
    {
        writeBuffer[0] = index;
        error = otPlatSettingsAdd(key, writeBuffer, (int)writeBufferLength);

        // -12: otPlatSettingsAdd error in commit
        VerifyOrExit(error == kThreadError_None, rval = -12);
    }

    key = 13;
    writeBuffer[0] = 10;
    error = otPlatSettingsSet(key, writeBuffer, (int)writeBufferLength);
    // -13: set one new setting block error in commit
    VerifyOrExit(error == kThreadError_None, rval = -13);

    key = 7;
    error = otPlatSettingsDelete(key, 1);
    // -14: otPlatSettingsDelete error in commit
    VerifyOrExit(error == kThreadError_None, rval = -14);

    error = otPlatSettingsCommitChange();
    // -15: otPlatSettingsCommitChange error
    VerifyOrExit(error == kThreadError_None, rval = -15);

    key = 15;

    for (index = 0; index < 3; index++)
    {
        error = otPlatSettingsGet(key, index, readBuffer, &readBufferLength);
        // -16: otPlatSettingsGet error in commit
        VerifyOrExit(error == kThreadError_None, rval = -16);
        // -17: otPlatSettingsAdd and otPlatSettingsGet not match in commit
        VerifyOrExit(readBuffer[0] == index, rval = -17);
        VerifyOrExit(!memcmp(readBuffer + 1, writeBuffer + 1, static_cast<uint16_t>(writeBufferLength - 1)), rval = -17);
    }

    key = 13;
    index = 10;
    error = otPlatSettingsGet(key, index, readBuffer, &readBufferLength);
    // -18: otPlatSettingsGet error in commit
    VerifyOrExit(error == kThreadError_None, rval = -18);
    // -19: otPlatSettingsSet and otPlatSettingsGet not match in commit
    VerifyOrExit(readBuffer[0] == index, rval = -19);
    VerifyOrExit(!memcmp(readBuffer + 1, writeBuffer + 1, static_cast<uint16_t>(writeBufferLength - 1)), rval = -19);

    // reordering
    reorderSettingsBlock();

    key = 7;

    for (index = 0; index < 10; index++)
    {
        error = otPlatSettingsGet(key, index, readBuffer, &readBufferLength);

        // -16: otPlatSettingsGet error in commit
        if (index == 1)
        {
            VerifyOrExit(error == kThreadError_NotFound, rval = -20);
        }
        else
        {
            VerifyOrExit(error == kThreadError_None, rval = -21);
        }

        // -22: otPlatSettingsGet not match after reordering
        VerifyOrExit(!memcmp(readBuffer + 1, writeBuffer + 1, static_cast<uint16_t>(writeBufferLength - 1)), rval = -22);
    }

    key = 8;

    for (index = 0; index < 10; index++)
    {
        error = otPlatSettingsGet(key, index, readBuffer, &readBufferLength);

        // -16: otPlatSettingsGet error in commit
        if (index == 0)
        {
            VerifyOrExit(error == kThreadError_None, rval = -23);
        }
        else
        {
            VerifyOrExit(error == kThreadError_NotFound, rval = -24);
        }

        // -22: otPlatSettingsGet not match after reordering
        VerifyOrExit(!memcmp(readBuffer + 1, writeBuffer + 1, static_cast<uint16_t>(writeBufferLength - 1)), rval = -25);
    }

    for (key = 9; key < 13; key++)
    {
        for (index = 0; index < 10; index++)
        {
            error = otPlatSettingsGet(key, index, readBuffer, &readBufferLength);
            // -26: otPlatSettingsGet error to get setting block after reordering
            VerifyOrExit(error == kThreadError_None, rval = -26);
            // -27: otPlatSettingsAdd and otPlatSettingsGet not match after reordering
            VerifyOrExit(readBuffer[0] == index, rval = -27);
            VerifyOrExit(!memcmp(readBuffer + 1, writeBuffer + 1, static_cast<uint16_t>(writeBufferLength - 1)), rval = -27);
        }
    }

exit:
    otPlatFlashDisable();

    return rval;
}
#endif

#ifdef __cplusplus
};
#endif
