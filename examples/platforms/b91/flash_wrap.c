/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <openthread/instance.h>
#include <openthread/platform/alarm-milli.h>

#include "platform-b91.h"
#include "utils/code_utils.h"

#define FLASH_BASE_ADDRESS 0x60000
#define FLASH_PAGE_SIZE 4096
#define FLASH_PAGE_NUM 2
#define FLASH_SWAP_SIZE (FLASH_PAGE_SIZE * (FLASH_PAGE_NUM / 2))

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
}

uint32_t otPlatFlashGetSwapSize(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return FLASH_SWAP_SIZE;
}

void otPlatFlashErase(otInstance *aInstance, uint32_t aSwapIndex)
{
    OT_UNUSED_VARIABLE(aInstance);

    Tl_printf("flash_erase_sector(%d)\n", (unsigned long)mapAddress(aSwapIndex, 0));
    flash_erase_sector((unsigned long)mapAddress(aSwapIndex, 0));
}

void otPlatFlashWrite(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    Tl_printf("flash_write_page(%d,%d,%d)\n", (unsigned long)mapAddress(aSwapIndex, aOffset), aSize,
              (unsigned char *)aData);
    flash_write_page((unsigned long)mapAddress(aSwapIndex, aOffset), aSize, (unsigned char *)aData);
}

void otPlatFlashRead(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    Tl_printf("flash_read_page(%d,%d,%d)\n", (unsigned long)mapAddress(aSwapIndex, aOffset), (unsigned long)aSize,
              (unsigned char *)aData);
    flash_read_page((unsigned long)mapAddress(aSwapIndex, aOffset), (unsigned long)aSize, (unsigned char *)aData);
}
