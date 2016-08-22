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

static uint32_t sSettingsBlockFlag = 0xbe5cc5ef;

static uint16_t sStageActionSeq;
static struct stageAddSettingsBlock sStageAddSettingsBlock[MAX_STAGE_ADD_NUM];
static uint8_t sStageAddSettingsNum;
static uint16_t sStageAddSettingsBufLength;
static struct stageDeleteSettingsBlock sStageDeleteSettingsBlock[MAX_STAGE_DELETE_NUM];
static uint8_t sStageDeleteSettingsNum;

static uint32_t sBaseAddress;
static uint32_t sAddress;
static bool sCommitLock = false;

void reorderSettingsBlock(void)
{
    uint8_t *reorderBuffer = (uint8_t *)malloc(OPENTHREAD_CONFIG_SETTINGS_PAGE_SIZE);
    uint8_t *ramBuffer;
    uint32_t ramBufferOffset = 0;

    uint8_t readPageIndex = 0;
    uint32_t readAddress;
    uint32_t readPageOffset = 0;

    uint8_t writePageIndex = 0;
    uint32_t writeAddress;
    uint32_t writePageOffset = 0;

    uint8_t pageNum = static_cast<uint8_t>(OPENTHREAD_CONFIG_SETTINGS_SIZE / otPlatFlashGetPageSize());
    struct settingsBlock block;
    uint32_t length = 0;
    uint32_t writeLength;
    bool storeLeft = false;
    bool endOfData = false;

    ramBuffer = reorderBuffer;

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
        otPlatFlashWrite(writeAddress, reorderBuffer, writeLength);
        writePageOffset += writeLength;

        if (ramBufferOffset > writeLength)
        {
            writePageIndex++;
            writePageOffset = 0;
            writeAddress = otPlatFlashGetBaseAddress() + OPENTHREAD_CONFIG_SETTINGS_BASE_ADDRESS
                           + writePageIndex * otPlatFlashGetPageSize() + writePageOffset;
            otPlatFlashWrite(writeAddress, reorderBuffer + writeLength, ramBufferOffset - writeLength);
            writePageOffset += (ramBufferOffset - writeLength);
        }

        ramBuffer = reorderBuffer;
        ramBufferOffset = 0;
        readPageIndex++;
    }

    free(reorderBuffer);

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
    endAddress = sBaseAddress + OPENTHREAD_CONFIG_SETTINGS_SIZE - 1;

    assert(!(sAddress & otPlatFlashGetPageSize()));

    memset(sStageAddSettingsBlock, 0xff, sizeof(sStageAddSettingsBlock));
    sStageAddSettingsNum = 0;
    sStageAddSettingsBufLength = 0;

    memset(sStageDeleteSettingsBlock, 0, sizeof(sStageDeleteSettingsBlock));
    sStageDeleteSettingsNum = 0;

    sStageActionSeq = 0xffff;

    sCommitLock = false;

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
    uint32_t address = sBaseAddress + sizeof(sSettingsBlockFlag);
    uint32_t endAddress = sBaseAddress + OPENTHREAD_CONFIG_SETTINGS_SIZE - 1;
    struct stageAddSettingsBlock *stageAddBlock;
    struct stageDeleteSettingsBlock *stageDeleteBlock;
    struct settingsBlock block;

    VerifyOrExit(sCommitLock == true, error = kThreadError_InvalidState);

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
            otPlatFlashWrite(sAddress, (uint8_t *)&stageAddBlock->block, sizeof(struct settingsBlock));
            otPlatFlashWrite(sAddress + sizeof(struct settingsBlock), (uint8_t *)stageAddBlock->data, stageAddBlock->block.length);
            sAddress += (sizeof(struct settingsBlock) + stageAddBlock->block.length);
        }
        else
        {
            stageDeleteBlock = &sStageDeleteSettingsBlock[stageDeleteIndex++];
            address = sBaseAddress + sizeof(sSettingsBlockFlag);

            while (address < endAddress)
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
    uint32_t address = sBaseAddress + sizeof(sSettingsBlockFlag);
    uint32_t endAddress = sBaseAddress + OPENTHREAD_CONFIG_SETTINGS_SIZE - 1;
    struct settingsBlock block;

    while (address < endAddress)
    {
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
    uint32_t address = sBaseAddress + sizeof(sSettingsBlockFlag);
    uint32_t endAddress = sBaseAddress + OPENTHREAD_CONFIG_SETTINGS_SIZE - 1;
    struct settingsBlock block;
    
    while (address < endAddress)
    {
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
    uint32_t address = sBaseAddress + sizeof(sSettingsBlockFlag);
    uint32_t endAddress = sBaseAddress + OPENTHREAD_CONFIG_SETTINGS_SIZE - 1;

    VerifyOrExit((!sCommitLock) || (sCommitLock &&
                                    (sStageAddSettingsNum < MAX_STAGE_ADD_NUM)), error = kThreadError_NoBufs);

    stageBlock = &sStageAddSettingsBlock[sStageAddSettingsNum];
    stageBlock->block.key = aKey;

    while (address < endAddress)
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

    if (index >= 0)
    {
        stageBlock->block.index = static_cast<uint8_t>(index + 1);
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
        if ((sAddress + stageBlock->block.length + sizeof(struct settingsBlock)) > endAddress)
        {
            reorderSettingsBlock();
        }

        otPlatFlashWrite(sAddress, (uint8_t *)&stageBlock->block, sizeof(struct settingsBlock));
        otPlatFlashWrite(sAddress + sizeof(struct settingsBlock), (uint8_t *)stageBlock->data, stageBlock->block.length);
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
    struct stageDeleteSettingsBlock *stageBlock;
    uint32_t address = sBaseAddress + sizeof(sSettingsBlockFlag);
    uint32_t endAddress = sBaseAddress + OPENTHREAD_CONFIG_SETTINGS_SIZE - 1;
    struct settingsBlock block;

    VerifyOrExit((!sCommitLock) || (sCommitLock &&
                                    (sStageDeleteSettingsNum < MAX_STAGE_DELETE_NUM)), error = kThreadError_NoBufs);

    while (address < endAddress)
    {
        otPlatFlashRead(address, (uint8_t *)&block, sizeof(block));

        if (sCommitLock == false)
        {
            if (!(block.flag & BLOCK_ADD_FLAG) && (block.flag & BLOCK_DELETE_FLAG) &&
                 (block.key == aKey) && (block.index == aIndex || aIndex == -1))
            {
                error = kThreadError_None;

                block.flag &= (~BLOCK_DELETE_FLAG);
                otPlatFlashWrite(address, (uint8_t *)&block, sizeof(block));

                VerifyOrExit(aIndex == -1, ;);
            }
        }
        else
        {
            error = kThreadError_None;

            stageBlock = &sStageDeleteSettingsBlock[sStageDeleteSettingsNum];
            stageBlock->key = aKey;
            stageBlock->index = aIndex;
            sStageActionSeq &= ~(1 <<(sStageAddSettingsNum + sStageDeleteSettingsNum));
            sStageDeleteSettingsNum++;
            break;
        }

        address += (block.length + sizeof(struct settingsBlock));
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

    for (index = 0; index < 2; index++)
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

    for (index = 0; index < 2; index++)
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
