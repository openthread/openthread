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

#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <openthread/types.h>
#include <openthread/platform/alarm.h>
#include <utils/flash.h>
#include <utils/code_utils.h>

#include "hal/nrf_nvmc.h"
#include "platform-nrf5.h"

extern uint32_t __flash_data_start;
extern uint32_t __flash_data_end;

#define FLASH_PAGE_ADDR_MASK 0xFFFFF000
#define FLASH_PAGE_SIZE      4096
#define FLASH_START_ADDR     ((uint32_t)&__flash_data_start)
#define FLASH_END_ADDR       ((uint32_t)&__flash_data_end)

static inline uint32_t mapAddress(uint32_t aAddress)
{
    return aAddress + FLASH_START_ADDR;
}

otError utilsFlashInit(void)
{
    // Just ensure that the start and end addresses are page-aligned.
    assert((FLASH_START_ADDR % FLASH_PAGE_SIZE) == 0);
    assert((FLASH_END_ADDR % FLASH_PAGE_SIZE) == 0);

    return OT_ERROR_NONE;
}

uint32_t utilsFlashGetSize(void)
{
    return FLASH_END_ADDR - FLASH_START_ADDR;
}

otError utilsFlashErasePage(uint32_t aAddress)
{
    otError error = OT_ERROR_NONE;
    otEXPECT_ACTION(aAddress < utilsFlashGetSize(), error = OT_ERROR_INVALID_ARGS);

    nrf_nvmc_page_erase(mapAddress(aAddress & FLASH_PAGE_ADDR_MASK));

exit:
    return error;
}

otError utilsFlashStatusWait(uint32_t aTimeout)
{
    otError error = OT_ERROR_BUSY;

    if (aTimeout == 0)
    {
        if (NRF_NVMC->READY == NVMC_READY_READY_Ready)
        {
            error = OT_ERROR_NONE;
        }
    }
    else
    {
        uint32_t startTime = otPlatAlarmGetNow();

        do
        {
            if (NRF_NVMC->READY == NVMC_READY_READY_Ready)
            {
                error = OT_ERROR_NONE;
                break;
            }
        }
        while (otPlatAlarmGetNow() - startTime < aTimeout);
    }

    return error;
}

uint32_t utilsFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t result = 0;
    otEXPECT(aData);
    otEXPECT(aAddress < utilsFlashGetSize());

    nrf_nvmc_write_bytes(mapAddress(aAddress), aData, aSize);
    result = aSize;

exit:
    return result;
}

uint32_t utilsFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t result = 0;
    otEXPECT(aData);
    otEXPECT(aAddress < utilsFlashGetSize());

    memcpy(aData, (uint8_t *)mapAddress(aAddress), aSize);
    result = aSize;

exit:
    return result;
}
