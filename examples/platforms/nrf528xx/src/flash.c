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

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "platform-nrf5.h"

/**
 * @def PLATFORM_FLASH_PAGE_NUM
 *
 * Number of flash pages to use for OpenThread's non-volatile settings.
 *
 * @note This define applies only for MDK-ARM Keil toolchain configuration.
 *
 */
#ifndef PLATFORM_FLASH_PAGE_NUM
#define PLATFORM_FLASH_PAGE_NUM 4
#endif

#define FLASH_PAGE_ADDR_MASK 0xFFFFF000
#define FLASH_PAGE_SIZE 4096

static uint32_t sFlashDataStart;
static uint32_t sFlashDataEnd;
static uint32_t sSwapSize;

static inline uint32_t mapAddress(uint8_t aSwapIndex, uint32_t aOffset)
{
    uint32_t address;

    address = sFlashDataStart + aOffset;

    if (aSwapIndex)
    {
        address += sSwapSize;
    }

    return address;
}

void otPlatFlashInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

#if defined(__CC_ARM)
    // Temporary solution for Keil compiler.
    uint32_t const bootloaderAddr = NRF_UICR->NRFFW[0];
    uint32_t const pageSize       = NRF_FICR->CODEPAGESIZE;
    uint32_t const codeSize       = NRF_FICR->CODESIZE;

    if (bootloaderAddr != 0xFFFFFFFF)
    {
        sFlashDataEnd = bootloaderAddr;
    }
    else
    {
        sFlashDataEnd = pageSize * codeSize;
    }

    sFlashDataStart = sFlashDataEnd - (pageSize * PLATFORM_FLASH_PAGE_NUM);

#elif defined(__GNUC__) || defined(__ICCARM__)
    extern uint32_t __start_ot_flash_data;
    extern uint32_t __stop_ot_flash_data;

    sFlashDataStart = (uint32_t)&__start_ot_flash_data;
    sFlashDataEnd   = (uint32_t)&__stop_ot_flash_data;

#endif

    // Just ensure that the start and end addresses are page-aligned.
    assert((sFlashDataStart % FLASH_PAGE_SIZE) == 0);
    assert((sFlashDataEnd % FLASH_PAGE_SIZE) == 0);

    sSwapSize = ((sFlashDataEnd - sFlashDataStart) / FLASH_PAGE_SIZE / 2) * FLASH_PAGE_SIZE;
    assert(sSwapSize > 0);
}

uint32_t otPlatFlashGetSwapSize(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sSwapSize;
}

void otPlatFlashErase(otInstance *aInstance, uint8_t aSwapIndex)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error;

    for (uint32_t offset = 0; offset < sSwapSize; offset += FLASH_PAGE_SIZE)
    {
        error = nrf5FlashPageErase(mapAddress(aSwapIndex, offset));
        assert(error == OT_ERROR_NONE);

        while (nrf5FlashIsBusy())
        {
        }
    }
}

void otPlatFlashWrite(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error;

    error = nrf5FlashWrite(mapAddress(aSwapIndex, aOffset), aData, aSize);
    assert(error == OT_ERROR_NONE);
}

void otPlatFlashRead(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    memcpy(aData, (uint8_t *)mapAddress(aSwapIndex, aOffset), aSize);
}
