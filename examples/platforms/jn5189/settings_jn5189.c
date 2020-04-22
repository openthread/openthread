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
 *   settings on JN5189 platform. It has been modified and optimized from the original
 *   Open Thread settings implementation to work with JN5189's flash particularities.
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

#include "utils/flash.h"

#define OT_FLASH_BLOCK_ADD_BEGIN_FLAG (1 << 0)
#define OT_FLASH_BLOCK_ADD_COMPLETE_FLAG (1 << 1)
#define OT_FLASH_BLOCK_DELETE_FLAG (1 << 2)
#define OT_FLASH_BLOCK_INDEX_0_FLAG (1 << 3)
/* Compact flag is used to indicate that the next settings block follows at the next 16 bytes
 * aligned address after the data portion of the current one. Otherwise the next block will be
 * placed in the next flash page */
#define OT_FLASH_BLOCK_COMPACT_FLAG (1 << 4)

#define OT_SETTINGS_FLAG_SIZE 16
#define OT_SETTINGS_BLOCK_DATA_SIZE 256

/* JN5189 erases the flash to value 0x00 */
#define FLASH_ERASE_VALUE 0x00
#define FLASH_ALIGN_SIZE 16
#define FLASH_BLOCK_PAD1_SIZE 10
#define FLASH_BLOCK_PAD2_SIZE 15

#define OT_SETTINGS_IN_USE 0xbe5cc5ee

/* Added padding to make settings block structure align to minimum flash write size of 16 bytes.
 * The delFlag field has an offset of 16 bytes from the beginning of the structure to allow a new
 * write on the field. */

OT_TOOL_PACKED_BEGIN
struct settingsBlock
{
    uint16_t key;
    uint16_t flag;
    uint16_t length;
    uint8_t  padding1[FLASH_BLOCK_PAD1_SIZE];
    uint8_t  delFlag;
    uint8_t  padding2[FLASH_BLOCK_PAD2_SIZE];
} OT_TOOL_PACKED_END;

/**
 * @def SETTINGS_CONFIG_BASE_ADDRESS
 *
 * The base address of settings.
 *
 */
#ifndef SETTINGS_CONFIG_BASE_ADDRESS
#define SETTINGS_CONFIG_BASE_ADDRESS 0
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

#if (SETTINGS_CONFIG_PAGE_NUM <= 1)
#error "Invalid value for `SETTINGS_CONFIG_PAGE_NUM` (should be >= 2)"
#endif

/**
 * @def FLASH_ERASE_VALUE
 *
 * The value a byte in flash takes after an erase operation.
 *
 */
#ifndef FLASH_ERASE_VALUE
#define FLASH_ERASE_VALUE 0xFF
#endif // FLASH_ERASE_VALUE

/* macros for setting/clearing bits in a flash byte depending on flash erase value */
#if (FLASH_ERASE_VALUE == 0xFF)
#define SET_FLASH_BLOCK_FLAG(aVar, aFlag) ((aVar) &= (~(aFlag)))
#define FLASH_BLOCK_FLAG_IS_SET(aVar, aFlag) (!((aVar) & (aFlag)))

#else
// FLASH_ERASE_VALUE = 0x00
#define SET_FLASH_BLOCK_FLAG(aVar, aFlag) ((aVar) |= (aFlag))
#define FLASH_BLOCK_FLAG_IS_SET(aVar, aFlag) ((aVar) & (aFlag))

#endif

static uint32_t sSettingsBaseAddress;
static uint32_t sSettingsUsedSize;
static uint32_t sSettingsPageNum;

/* linker file symbol for the number of flash sectors used for NVM */
extern uint32_t NV_STORAGE_MAX_SECTORS;
extern uint8_t  pageBuffer[];

/**
 * Calculates the aligned length of data for current settings block based on Compact flag
 *
 * @param[in] currentPos     Offset from the beginning of settings base address where the current
 *                           block is located
 * @param[in] blockFlag      Settings block flags value
 * @param[in] length         Length of current block data

 * @return uint16_t  Length of aligned data
 */
static uint16_t getAlignLength(uint16_t currentPos, uint8_t blockFlag, uint16_t length)
{
    uint16_t alignLen;
    /* in case the compact flag is set length is calculated based on real one aligned to
     * FLASH_ALIGN_SIZE(16 in this case) */
    if (FLASH_BLOCK_FLAG_IS_SET(blockFlag, OT_FLASH_BLOCK_COMPACT_FLAG))
    {
        alignLen = (length + 1) & 0xfffe;
    }
    else
    {
        /* if the block is not compacted the length will be calculated so that the next block starts
         * in the next flash page */
        uint16_t pageOffset = currentPos % SETTINGS_CONFIG_PAGE_SIZE;
        alignLen            = SETTINGS_CONFIG_PAGE_SIZE - pageOffset - sizeof(struct settingsBlock);
    }

    return alignLen;
}

static void setSettingsFlag(uint32_t aBase, uint32_t aFlag)
{
    utilsFlashWrite(aBase, (uint8_t *)&aFlag, sizeof(aFlag));
}

static void eraseSettings(uint32_t aBase)
{
    uint32_t address      = aBase;
    uint32_t settingsSize = SETTINGS_CONFIG_PAGE_SIZE * sSettingsPageNum / 2;

    while (address < (aBase + settingsSize))
    {
        utilsFlashErasePage(address);
        address += SETTINGS_CONFIG_PAGE_SIZE;
    }
}

static void initSettings(uint32_t aBase, uint32_t aFlag)
{
    eraseSettings(aBase);
    setSettingsFlag(aBase, aFlag);
}

static uint32_t swapSettingsBlock(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t oldBase        = sSettingsBaseAddress;
    uint32_t swapAddress    = oldBase;
    uint32_t usedSize       = sSettingsUsedSize;
    uint32_t settingsSize   = SETTINGS_CONFIG_PAGE_SIZE * sSettingsPageNum / 2;
    bool     pageBufferUsed = true;
    uint8_t  tempFlag;

    /* New settings base address */
    sSettingsBaseAddress =
        (swapAddress == SETTINGS_CONFIG_BASE_ADDRESS) ? (swapAddress + settingsSize) : SETTINGS_CONFIG_BASE_ADDRESS;

    /* Erase new settings */
    eraseSettings(sSettingsBaseAddress);
    /* Erase the page buffer used to accumulate data that will be written to a flash page once it is
     * full */
    memset(pageBuffer, FLASH_ERASE_VALUE, SETTINGS_CONFIG_PAGE_SIZE);

    *((uint32_t *)pageBuffer) = OT_SETTINGS_IN_USE;
    sSettingsUsedSize         = OT_SETTINGS_FLAG_SIZE;
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
        tempFlag = addBlock.block.flag;

        if ((FLASH_BLOCK_FLAG_IS_SET(addBlock.block.flag, OT_FLASH_BLOCK_ADD_COMPLETE_FLAG)) &&
            (0 == FLASH_BLOCK_FLAG_IS_SET(addBlock.block.delFlag, OT_FLASH_BLOCK_DELETE_FLAG)))

        {
            uint32_t address = swapAddress + getAlignLength(swapAddress - sizeof(struct settingsBlock),
                                                            addBlock.block.flag, addBlock.block.length);

            while (address < (oldBase + usedSize))
            {
                struct settingsBlock block;

                utilsFlashRead(address, (uint8_t *)(&block), sizeof(block));

                if ((FLASH_BLOCK_FLAG_IS_SET(block.flag, OT_FLASH_BLOCK_ADD_COMPLETE_FLAG)) &&
                    (0 == FLASH_BLOCK_FLAG_IS_SET(block.delFlag, OT_FLASH_BLOCK_DELETE_FLAG)) &&
                    (FLASH_BLOCK_FLAG_IS_SET(block.flag, OT_FLASH_BLOCK_INDEX_0_FLAG)) &&
                    (block.key == addBlock.block.key))

                {
                    valid = false;
                    break;
                }

                address += (getAlignLength(address, block.flag, block.length) + sizeof(struct settingsBlock));
            }

            if (valid)
            {
                /* Calculate the size of data(block + settings data) that will be written to the
                 * page buffer */
                uint32_t writeSize = sizeof(struct settingsBlock);

                if (!FLASH_BLOCK_FLAG_IS_SET(addBlock.block.flag, OT_FLASH_BLOCK_COMPACT_FLAG))
                {
                    /* Once a swap is initiated every settings block is compacted in the new flash
                     * region */
                    SET_FLASH_BLOCK_FLAG(addBlock.block.flag, OT_FLASH_BLOCK_COMPACT_FLAG);
                }

                /* current pos parameter doesn't count in this case */
                writeSize += getAlignLength(0, OT_FLASH_BLOCK_COMPACT_FLAG, addBlock.block.length);

                utilsFlashRead(swapAddress, addBlock.data, addBlock.block.length);
                /* contents fits in current page - we can copy it to page buffer until there is
                 * enough data to program a page */
                if ((sSettingsUsedSize % SETTINGS_CONFIG_PAGE_SIZE) + writeSize <= SETTINGS_CONFIG_PAGE_SIZE)
                {
                    pageBufferUsed = false;
                    memcpy(pageBuffer + (sSettingsUsedSize % SETTINGS_CONFIG_PAGE_SIZE), (uint8_t *)(&addBlock),
                           writeSize);
                }
                else
                {
                    /* Page buffer is full and can be written to a flash page */
                    pageBufferUsed = true;

                    uint32_t remPageSize = SETTINGS_CONFIG_PAGE_SIZE - (sSettingsUsedSize % SETTINGS_CONFIG_PAGE_SIZE);
                    memcpy(pageBuffer + (sSettingsUsedSize % SETTINGS_CONFIG_PAGE_SIZE), (uint8_t *)(&addBlock),
                           remPageSize);

                    /* calculate page address that we are going to write */
                    uint32_t alignAddress = sSettingsBaseAddress + sSettingsUsedSize;
                    alignAddress          = alignAddress - (alignAddress % SETTINGS_CONFIG_PAGE_SIZE);
                    utilsFlashWrite(alignAddress, pageBuffer, SETTINGS_CONFIG_PAGE_SIZE);

                    /* After the page buffer is erased copy what dind't fit the previous page */
                    memset(pageBuffer, FLASH_ERASE_VALUE, SETTINGS_CONFIG_PAGE_SIZE);
                    memcpy(pageBuffer, (uint8_t *)(&addBlock) + remPageSize, writeSize - remPageSize);
                }

                sSettingsUsedSize += writeSize;
            }
        }
        else if (addBlock.block.flag == FLASH_ERASE_VALUE)
        {
            break;
        }
        swapAddress += getAlignLength(swapAddress - sizeof(struct settingsBlock), tempFlag, addBlock.block.length);
    }

    if (false == pageBufferUsed)
    {
        /* If the page buffer has been used and it's not full write to flash at the end */
        uint32_t alignAddr = sSettingsBaseAddress + sSettingsUsedSize;
        alignAddr          = alignAddr - (alignAddr % SETTINGS_CONFIG_PAGE_SIZE);
        utilsFlashWrite(alignAddr, pageBuffer, SETTINGS_CONFIG_PAGE_SIZE);
    }
    /* Clear the old settings zone */
    eraseSettings(oldBase);

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
    uint32_t             settingsSize = SETTINGS_CONFIG_PAGE_SIZE * sSettingsPageNum / 2;

    /* Add all the settings flags once and optimize for one write to flash */
    addBlock.block.flag    = FLASH_ERASE_VALUE;
    addBlock.block.delFlag = FLASH_ERASE_VALUE;
    addBlock.block.key     = aKey;
    memset(addBlock.block.padding1, FLASH_ERASE_VALUE, FLASH_BLOCK_PAD1_SIZE);
    memset(addBlock.block.padding2, FLASH_ERASE_VALUE, FLASH_BLOCK_PAD2_SIZE);

    if (aIndex0)
    {
        SET_FLASH_BLOCK_FLAG(addBlock.block.flag, OT_FLASH_BLOCK_INDEX_0_FLAG);
    }

    SET_FLASH_BLOCK_FLAG(addBlock.block.flag, OT_FLASH_BLOCK_ADD_BEGIN_FLAG);
    addBlock.block.length = aValueLength;

    if ((sSettingsUsedSize + getAlignLength(sSettingsUsedSize, addBlock.block.flag, addBlock.block.length) +
         sizeof(struct settingsBlock)) >= settingsSize)
    {
        otEXPECT_ACTION(swapSettingsBlock(aInstance) >=
                            (getAlignLength(sSettingsUsedSize, addBlock.block.flag, addBlock.block.length) +
                             sizeof(struct settingsBlock)),
                        error = OT_ERROR_NO_BUFS);
    }

    memset(addBlock.data, FLASH_ERASE_VALUE, OT_SETTINGS_BLOCK_DATA_SIZE);
    memcpy(addBlock.data, aValue, addBlock.block.length);

    SET_FLASH_BLOCK_FLAG(addBlock.block.flag, OT_FLASH_BLOCK_ADD_COMPLETE_FLAG);
    utilsFlashWrite(sSettingsBaseAddress + sSettingsUsedSize, (uint8_t *)(&addBlock.block),
                    sizeof(struct settingsBlock) + addBlock.block.length);
    /* The next settings block will be written to the next flash page to optimize the number of
     * writes made to a page */
    sSettingsUsedSize +=
        (sizeof(struct settingsBlock) + getAlignLength(sSettingsUsedSize, addBlock.block.flag, addBlock.block.length));

exit:
    return error;
}

// settings API
void otPlatSettingsInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint8_t              index;
    struct settingsBlock block;

    /* exported symbol from linker file */
    sSettingsPageNum      = (uint32_t)&NV_STORAGE_MAX_SECTORS;
    uint32_t settingsSize = SETTINGS_CONFIG_PAGE_SIZE * sSettingsPageNum / 2;

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
        utilsFlashRead(sSettingsBaseAddress + sSettingsUsedSize, (uint8_t *)(&block), sizeof(block));

        if (FLASH_BLOCK_FLAG_IS_SET(block.flag, OT_FLASH_BLOCK_ADD_BEGIN_FLAG))
        {
            sSettingsUsedSize +=
                (getAlignLength(sSettingsUsedSize, block.flag, block.length) + sizeof(struct settingsBlock));
        }
        else
        {
            break;
        }
    }
}

void otPlatSettingsDeinit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
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
            if (FLASH_BLOCK_FLAG_IS_SET(block.flag, OT_FLASH_BLOCK_INDEX_0_FLAG))
            {
                index = 0;
            }

            if ((FLASH_BLOCK_FLAG_IS_SET(block.flag, OT_FLASH_BLOCK_ADD_COMPLETE_FLAG)) &&
                (0 == FLASH_BLOCK_FLAG_IS_SET(block.delFlag, OT_FLASH_BLOCK_DELETE_FLAG)))
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

        address += (getAlignLength(address, block.flag, block.length) + sizeof(struct settingsBlock));
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
            if (FLASH_BLOCK_FLAG_IS_SET(block.flag, OT_FLASH_BLOCK_INDEX_0_FLAG))
            {
                index = 0;
            }

            if ((FLASH_BLOCK_FLAG_IS_SET(block.flag, OT_FLASH_BLOCK_ADD_COMPLETE_FLAG)) &&
                (0 == FLASH_BLOCK_FLAG_IS_SET(block.delFlag, OT_FLASH_BLOCK_DELETE_FLAG)))
            {
                bool flashWrite = false;

                if (aIndex == index || aIndex == -1)
                {
                    error = OT_ERROR_NONE;
                    SET_FLASH_BLOCK_FLAG(block.delFlag, OT_FLASH_BLOCK_DELETE_FLAG);
                    flashWrite = true;
                }

                if (index == 1 && aIndex == 0)
                {
                    SET_FLASH_BLOCK_FLAG(block.flag, OT_FLASH_BLOCK_INDEX_0_FLAG);
                    flashWrite = true;
                }

                if (flashWrite)
                {
                    utilsFlashWrite(address, (uint8_t *)(&block), sizeof(block));
                }

                index++;
            }
        }

        address += (getAlignLength(address, block.flag, block.length) + sizeof(struct settingsBlock));
    }

    return error;
}

void otPlatSettingsWipe(otInstance *aInstance)
{
    uint32_t address = SETTINGS_CONFIG_BASE_ADDRESS;

    /* Clears all the flash pages during a factory reset */
    for (uint32_t i = 0; i < sSettingsPageNum; i++)
    {
        /* This function protects against erasing an already erased page so we can just erase all
         * pages to have and have all the settings storage clean */
        utilsFlashErasePage(address);
        address += SETTINGS_CONFIG_PAGE_SIZE;
    }

    /* Each time a factory reset is invoked start the settings zone in the alternate region to
     * maximize wear leveling */
    if (SETTINGS_CONFIG_BASE_ADDRESS == sSettingsBaseAddress)
    {
        sSettingsBaseAddress = SETTINGS_CONFIG_BASE_ADDRESS + (SETTINGS_CONFIG_PAGE_SIZE * sSettingsPageNum / 2);
    }
    else
    {
        sSettingsBaseAddress = SETTINGS_CONFIG_BASE_ADDRESS;
    }
    setSettingsFlag(sSettingsBaseAddress, OT_SETTINGS_IN_USE);

    otPlatSettingsInit(aInstance);
}
