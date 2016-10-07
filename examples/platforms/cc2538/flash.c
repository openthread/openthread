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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <openthread-config.h>
#include <platform/alarm.h>
#include <platform/flash.h>

#include <common/code_utils.hpp>
#include "platform-cc2538.h"
#include "rom-utility.h"

#define FLASH_CTRL_FCTL_BUSY   0x00000080

enum
{
    FLASH_PAGE_SIZE = 0x800,
};

static ThreadError romStatusToThread(int32_t aStatus)
{
    ThreadError error = kThreadError_None;

    switch (aStatus)
    {
    case 0:
        error = kThreadError_None;
        break;

    case -1:
        error = kThreadError_Failed;
        break;

    case -2:
        error = kThreadError_InvalidArgs;
        break;

    default:
        error = kThreadError_Abort;
    }

    return error;
}

ThreadError otPlatFlashInit(void)
{
    return kThreadError_None;
}

uint32_t otPlatFlashGetSize(void)
{
    uint32_t reg = (HWREG(FLASH_CTRL_DIECFG0) & 0x00000070) >> 4;

    return reg ? (128 * reg) : 64;
}

ThreadError otPlatFlashErasePage(uint32_t aAddress)
{
    ThreadError error = kThreadError_None;
    int32_t status;
    uint32_t address;

    VerifyOrExit(aAddress < otPlatFlashGetSize() * 1024, error = kThreadError_InvalidArgs);

    address = FLASH_BASE + aAddress - (aAddress & (FLASH_PAGE_SIZE -1));
    status = ROM_PageErase(address, FLASH_PAGE_SIZE);
    error = romStatusToThread(status);

exit:
    return error;
}

ThreadError otPlatFlashStatusWait(uint32_t aTimeout)
{
    ThreadError error = kThreadError_None;
    uint32_t start = otPlatAlarmGetNow();
    uint32_t busy = 1;

    while (busy && ((otPlatAlarmGetNow() - start) < aTimeout))
    {
        busy = HWREG(FLASH_CTRL_FCTL) & FLASH_CTRL_FCTL_BUSY;
    }

    if (busy)
    {
        error = kThreadError_Busy;
    }

    return error;
}

uint32_t otPlatFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    int32_t status;
    uint32_t busy = 1;
    uint32_t *data;
    uint32_t size = 0;

    VerifyOrExit(((aAddress + aSize) < (otPlatFlashGetSize() * 1024)) &&
                 (!(aAddress & 3)) && (!(aSize & 3)), ;);

    data = (uint32_t *)(aData);

    while (size < aSize)
    {
        status = ROM_ProgramFlash(data, aAddress + FLASH_BASE, 4);

        while (busy)
        {
            busy = HWREG(FLASH_CTRL_FCTL) & FLASH_CTRL_FCTL_BUSY;
        }

        SuccessOrExit(romStatusToThread(status));

        size += 4;
        data++;
        aAddress += 4;
    }

exit:
    return size;
}

uint32_t otPlatFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t size = 0;

    VerifyOrExit((aAddress + aSize) < (otPlatFlashGetSize() * 1024), ;);

    while (size < (aSize / 4))
    {
        uint32_t reg = HWREG(aAddress + FLASH_BASE);
        uint8_t *byte = (uint8_t *)&reg;

        for (uint8_t index = 0; index < 4; index++, byte++, aData++)
        {
            *aData = *byte;
        }

        size += 1;
        aAddress += 4;
    }

    size *= 4;

    if (aSize % 4)
    {
        uint32_t reg = HWREG(aAddress + FLASH_BASE);
        uint8_t *byte = (uint8_t *)&reg;

        for (uint8_t index = 0; index < aSize % 4; index++, byte++, aData++)
        {
            *aData = *byte;
        }

        size += (aSize % 4);
    }

exit:
    return size;
}
