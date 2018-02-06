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

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <openthread/platform/alarm-milli.h>

#include "platform-cc2538.h"
#include "rom-utility.h"
#include "utils/code_utils.h"
#include "utils/flash.h"
#include "utils/wrap_string.h"

#define FLASH_CTRL_FCTL_BUSY 0x00000080

#if SETTINGS_CONFIG_PAGE_SIZE != 2048
#error FLASH page size is 2048 on this chip
#endif

#if SETTINGS_CONFIG_PAGE_NUM != 2
#error Linker script reserves 2 pages for settings.
#endif

/* The linker script creates this external symbol */
extern uint8_t _FLASH_settings_pageA[];

/* Convert a settings offset to the physical address within the flash settings pages */
static uint32_t flashPhysAddr(uint32_t settings_offset)
{
    uint32_t base;

    base = (uint32_t)(&_FLASH_settings_pageA[0]);
    base = base + settings_offset;
    return base;
}

static otError romStatusToThread(int32_t aStatus)
{
    otError error = OT_ERROR_NONE;

    switch (aStatus)
    {
    case 0:
        error = OT_ERROR_NONE;
        break;

    case -1:
        error = OT_ERROR_FAILED;
        break;

    case -2:
        error = OT_ERROR_INVALID_ARGS;
        break;

    default:
        error = OT_ERROR_ABORT;
    }

    return error;
}

otError utilsFlashInit(void)
{
    return OT_ERROR_NONE;
}

uint32_t utilsFlashGetSize(void)
{
    return (SETTINGS_CONFIG_PAGE_SIZE * SETTINGS_CONFIG_PAGE_NUM);
}

otError utilsFlashErasePage(uint32_t aAddress)
{
    otError  error = OT_ERROR_NONE;
    int32_t  status;
    uint32_t address;

    otEXPECT_ACTION(aAddress < utilsFlashGetSize(), error = OT_ERROR_INVALID_ARGS);

    address = aAddress - (aAddress & (SETTINGS_CONFIG_PAGE_SIZE - 1));
    address = flashPhysAddr(address);
    status  = ROM_PageErase(address, SETTINGS_CONFIG_PAGE_SIZE);
    error   = romStatusToThread(status);

exit:
    return error;
}

otError utilsFlashStatusWait(uint32_t aTimeout)
{
    otError  error = OT_ERROR_NONE;
    uint32_t start = otPlatAlarmMilliGetNow();
    uint32_t busy  = 1;

    while (busy && ((otPlatAlarmMilliGetNow() - start) < aTimeout))
    {
        busy = HWREG(FLASH_CTRL_FCTL) & FLASH_CTRL_FCTL_BUSY;
    }

    otEXPECT_ACTION(!busy, error = OT_ERROR_BUSY);

exit:
    return error;
}

uint32_t utilsFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    int32_t   status;
    uint32_t  busy = 1;
    uint32_t *data;
    uint32_t  size = 0;

    otEXPECT_ACTION(((aAddress + aSize) < utilsFlashGetSize()) && (!(aAddress & 3)) && (!(aSize & 3)), aSize = 0);

    data = (uint32_t *)(aData);

    while (size < aSize)
    {
        status = ROM_ProgramFlash(data, flashPhysAddr(aAddress), 4);

        while (busy)
        {
            busy = HWREG(FLASH_CTRL_FCTL) & FLASH_CTRL_FCTL_BUSY;
        }

        otEXPECT(romStatusToThread(status) == OT_ERROR_NONE);
        size += 4;
        data++;
        aAddress += 4;
    }

exit:
    return size;
}

uint32_t utilsFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t size = 0;

    otEXPECT((aAddress + aSize) < utilsFlashGetSize());

    while (size < aSize)
    {
        uint8_t *byte     = (uint8_t *)flashPhysAddr(aAddress);
        uint8_t  maxIndex = 4;

        if (size == (aSize - aSize % 4))
        {
            maxIndex = aSize % 4;
        }

        for (uint8_t index = 0; index < maxIndex; index++, byte++, aData++)
        {
            *aData = *byte;
        }

        size += maxIndex;
        aAddress += maxIndex;
    }

exit:
    return size;
}
