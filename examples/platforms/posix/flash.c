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
#include <platform/flash.h>

#include <common/code_utils.hpp>
#include "platform-posix.h"

static int sFlashFd;
uint32_t sEraseAddress;

enum
{
    FLASH_SIZE = 0x40000,
    FLASH_PAGE_SIZE = 0x800,
    FLASH_PAGE_NUM = 128,
};

ThreadError otPlatFlashInit(void)
{
    ThreadError error = kThreadError_None;

    sFlashFd = open("OT_Flash", O_RDWR | O_CREAT, 0666);
    VerifyOrExit(ftruncate(sFlashFd, 0) == 0, error = kThreadError_Failed);
    lseek(sFlashFd, 0, SEEK_SET);

    VerifyOrExit(sFlashFd >= 0, error = kThreadError_Failed);

    for (uint16_t index = 0; index < FLASH_PAGE_NUM; index++)
    {
        SuccessOrExit(error = otPlatFlashErasePage(index * FLASH_PAGE_SIZE));
    }

exit:
    return error;
}

uint32_t otPlatFlashGetSize(void)
{
    return FLASH_SIZE;
}

ThreadError otPlatFlashErasePage(uint32_t aAddress)
{
    ThreadError error = kThreadError_None;
    uint8_t buf = 0xff;

    VerifyOrExit(sFlashFd >= 0, error = kThreadError_Failed);
    VerifyOrExit(aAddress < FLASH_SIZE, error = kThreadError_InvalidArgs);

    for (uint16_t offset = 0; offset < FLASH_PAGE_SIZE; offset++)
    {
        VerifyOrExit(pwrite(sFlashFd, &buf, 1, (aAddress & (~(FLASH_PAGE_SIZE - 1))) + offset) == 1,
                     error = kThreadError_Failed);
    }

exit:
    return error;
}

ThreadError otPlatFlashStatusWait(uint32_t aTimeout)
{
    (void)aTimeout;
    return kThreadError_None;
}

uint32_t otPlatFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t ret = 0;

    VerifyOrExit(sFlashFd >= 0 && aAddress < FLASH_SIZE, ;);
    ret = (uint32_t)pwrite(sFlashFd, aData, aSize, aAddress);

exit:
    return ret;
}

uint32_t otPlatFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t ret = 0;

    VerifyOrExit(sFlashFd >= 0 && aAddress < FLASH_SIZE, ;);
    ret = (uint32_t)pread(sFlashFd, aData, aSize, aAddress);

exit:
    return ret;
}
