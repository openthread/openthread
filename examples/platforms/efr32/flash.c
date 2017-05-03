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

#include <openthread-config.h>
#include <openthread/platform/alarm.h>

#include "utils/code_utils.h"
#include "utils/flash.h"

#include "em_msc.h"

#define FLASH_DATA_USED_PAGES   10
#define FLASH_DATA_END_ADDR     (FLASH_BASE + FLASH_SIZE)
#define FLASH_DATA_START_ADDR   (FLASH_DATA_END_ADDR - (FLASH_PAGE_SIZE * FLASH_DATA_USED_PAGES))

static inline uint32_t mapAddress(uint32_t aAddress)
{
    return aAddress + FLASH_DATA_START_ADDR;
}

static ThreadError returnTypeConvert(int32_t aStatus)
{
    ThreadError error = kThreadError_None;

    switch (aStatus)
    {
    case mscReturnOk:
        error = kThreadError_None;
        break;

    case mscReturnInvalidAddr:
    case mscReturnUnaligned:
        error = kThreadError_InvalidArgs;
        break;

    default:
        error = kThreadError_Failed;
    }

    return error;
}

ThreadError utilsFlashInit(void)
{
    MSC_Init();
    return kThreadError_None;
}

uint32_t utilsFlashGetSize(void)
{
    return FLASH_DATA_END_ADDR - FLASH_DATA_START_ADDR;
}

ThreadError utilsFlashErasePage(uint32_t aAddress)
{
    int32_t status;

    status = MSC_ErasePage((uint32_t *)mapAddress(aAddress));

    return returnTypeConvert(status);
}

ThreadError utilsFlashStatusWait(uint32_t aTimeout)
{
    ThreadError error = kThreadError_Busy;
    uint32_t start = otPlatAlarmGetNow();

    do
    {
        if (MSC->STATUS & MSC_STATUS_WDATAREADY)
        {
            error = kThreadError_None;
            break;
        }
    }
    while (aTimeout && ((otPlatAlarmGetNow() - start) < aTimeout));

    return error;
}

uint32_t utilsFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t rval = aSize;
    int32_t status;

    otEXPECT_ACTION(aData, rval = 0);
    otEXPECT_ACTION(((aAddress + aSize) < utilsFlashGetSize()) &&
                    (!(aAddress & 3)) && (!(aSize & 3)), rval = 0);

    status = MSC_WriteWord((uint32_t *)mapAddress(aAddress), aData, aSize);
    otEXPECT_ACTION(returnTypeConvert(status) == kThreadError_None, rval = 0);

exit:
    return rval;
}

uint32_t utilsFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t rval = aSize;
    uint32_t pAddress = mapAddress(aAddress);
    uint8_t *byte = aData;

    otEXPECT_ACTION(aData, rval = 0);
    otEXPECT_ACTION((aAddress + aSize) < utilsFlashGetSize(), rval = 0);

    while (aSize--)
    {
        *byte++ = (*(uint8_t *)(pAddress++));
    }

exit:
    return rval;
}
