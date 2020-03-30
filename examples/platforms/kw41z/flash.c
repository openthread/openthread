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

#include <stdint.h>

#include <openthread/instance.h>

#include "fsl_device_registers.h"
#include "fsl_flash.h"

#define FLASH_BASE_ADDRESS 0x40000
#define FLASH_PAGE_SIZE 0x800
#define FLASH_PAGE_NUM 2
#define FLASH_SWAP_SIZE (FLASH_PAGE_SIZE * (FLASH_PAGE_NUM / 2))

static flash_config_t sFlashConfig;

static uint32_t mapAddress(uint8_t aSwapIndex, uint32_t aOffset)
{
    uint32_t address = FLASH_BASE_ADDRESS + aOffset;

    if (aSwapIndex)
    {
        address += FLASH_SWAP_SIZE;
    }

    return address;
}

void otPlatFlashInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    FLASH_Init(&sFlashConfig);
}

uint32_t otPlatFlashGetSwapSize(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return FLASH_SWAP_SIZE;
}

void otPlatFlashErase(otInstance *aInstance, uint32_t aSwapIndex)
{
    OT_UNUSED_VARIABLE(aInstance);

    FLASH_Erase(&sFlashConfig, mapAddress(aSwapIndex, 0), FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE,
                kFLASH_ApiEraseKey);
    while ((FTFA->FSTAT & FTFA_FSTAT_CCIF_MASK) == 0)
    {
    }
}

void otPlatFlashWrite(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    FLASH_Program(&sFlashConfig, mapAddress(aSwapIndex, aOffset), (uint32_t *)aData, aSize);
}

void otPlatFlashRead(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    memcpy(aData, (void *)mapAddress(aSwapIndex, aOffset), aSize);
}
