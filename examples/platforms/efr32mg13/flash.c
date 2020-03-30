/*
 *  Copyright (c) 2017, The OpenThread Authors.
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

#include <string.h>

#include <openthread/instance.h>

#include "em_msc.h"

#define FLASH_PAGE_NUM 4
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
