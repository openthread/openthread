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

#include "asf.h"

#define OT_FLASH_BASE_ADDRESS ((uint32_t)&__d_nv_mem_start)
#define OT_FLASH_PAGE_SIZE 0x100

/*
 * This value should not exceed:
 *     (((uint32_t)&__d_nv_mem_end - (uint32_t)&__d_nv_mem_start) / OT_FLASH_PAGE_SIZE)
 *
 * __d_nv_mem_start and __d_nv_mem_end is defined in linker script.
 * The size of NVRAM region is 4k. Page size is 256 bytes. Maximum OT_FLASH_PAGE_NUM
 * should be equal or less than 16.
 *
 */
#define OT_FLASH_PAGE_NUM 16

#define OT_FLASH_SWAP_SIZE (OT_FLASH_PAGE_SIZE * (OT_FLASH_PAGE_NUM / 2))

extern uint32_t __d_nv_mem_start;
extern uint32_t __d_nv_mem_end;

static uint32_t mapAddress(uint8_t aSwapIndex, uint32_t aOffset)
{
    uint32_t address = OT_FLASH_BASE_ADDRESS + aOffset;

    if (aSwapIndex)
    {
        address += OT_FLASH_SWAP_SIZE;
    }

    return address;
}

void otPlatFlashInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    struct nvm_config configNvm;
    enum status_code  status;

    nvm_get_config_defaults(&configNvm);

    configNvm.manual_page_write = false;
    while ((status = nvm_set_config(&configNvm)) == STATUS_BUSY)
    {
    }
}

uint32_t otPlatFlashGetSwapSize(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_FLASH_SWAP_SIZE;
}

void otPlatFlashErase(otInstance *aInstance, uint8_t aSwapIndex)
{
    OT_UNUSED_VARIABLE(aInstance);

    nvm_erase_row(mapAddress(aSwapIndex, 0));
    while (!nvm_is_ready())
    {
    }
}

void otPlatFlashWrite(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t address = mapAddress(aSwapIndex, aOffset);

    for (uint32_t i = 0; i < (aSize / sizeof(uint32_t)); i++)
    {
        *((volatile uint32_t *)address) = *((uint32_t *)aData);
        aData += sizeof(uint32_t);
        address += sizeof(uint32_t);
    }

    // check if write page command is required
    if ((address) & (NVMCTRL_PAGE_SIZE - 1))
    {
        nvm_execute_command(NVM_COMMAND_WRITE_PAGE, address & (~(NVMCTRL_PAGE_SIZE - 1)), 0);
    }
}

void otPlatFlashRead(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    memcpy(aData, (void *)mapAddress(aSwapIndex, aOffset), aSize);
}
