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

enum
{
    FLASH_SIZE = 0x100000,
    FLASH_BASE_ADDRESS = 0x200000,
    FLASH_PAGE_SIZE = 0x800,
    FLASH_PAGE_NUM = 512,
};

static ThreadError erasePage(uint32_t aOffset)
{
    ThreadError error = kThreadError_Failed;
    uint8_t buf = 0xff;
    uint16_t size;

    VerifyOrExit(sFlashFd >= 0, ;);

    for (size = 0; size < FLASH_PAGE_SIZE; size++)
    {
        VerifyOrExit(pwrite(sFlashFd, &buf, 1, aOffset + size) == 1, ;);
    }

    error = kThreadError_None;

exit:
    return error;
}

ThreadError otPlatFlashInit(void)
{
    ThreadError error = kThreadError_None;

    sFlashFd = open("OT_Flash", O_RDWR | O_CREAT, 0666);
    VerifyOrExit(ftruncate(sFlashFd, 0) == 0, error = kThreadError_Failed);
    lseek(sFlashFd, 0, SEEK_SET);

    VerifyOrExit(sFlashFd >= 0, error = kThreadError_Failed);

    SuccessOrExit(error = otPlatFlashErasePage(FLASH_BASE_ADDRESS, FLASH_SIZE));

exit:
    return error;
}

ThreadError otPlatFlashDisable(void)
{
    close(sFlashFd);

    return kThreadError_None;
}

uint32_t otPlatFlashGetBaseAddress(void)
{
    return FLASH_BASE_ADDRESS;
}

uint32_t otPlatFlashGetSize(void)
{
    return FLASH_SIZE;
}

uint32_t otPlatFlashGetPageSize(void)
{
    return FLASH_PAGE_SIZE;
}

ThreadError otPlatFlashErasePage(uint32_t aAddress, uint32_t aSize)
{
    ThreadError error = kThreadError_None;
    uint32_t offset = aAddress - FLASH_BASE_ADDRESS;
    uint16_t pageNum;
    uint16_t index;

    VerifyOrExit(aAddress >= FLASH_BASE_ADDRESS, error = kThreadError_InvalidArgs);
    VerifyOrExit(aAddress < (FLASH_BASE_ADDRESS + FLASH_SIZE), error = kThreadError_InvalidArgs);
    VerifyOrExit(!(aAddress & (FLASH_PAGE_SIZE - 1)), error = kThreadError_InvalidArgs);

    pageNum = (uint16_t)(aSize / FLASH_PAGE_SIZE);
    pageNum = (pageNum > FLASH_PAGE_NUM) ? FLASH_PAGE_NUM : pageNum;

    for (index = 0; index < pageNum; index++)
    {
        SuccessOrExit(error = erasePage(offset + FLASH_PAGE_SIZE * index));
    }

exit:
    return error;
}

uint32_t otPlatFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t offset = aAddress - FLASH_BASE_ADDRESS;
    uint32_t ret;

    VerifyOrExit(sFlashFd >= 0, ret = 0);

    ret = (uint32_t)pwrite(sFlashFd, aData, aSize, offset);

exit:
    return ret;
}

uint32_t otPlatFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t offset = aAddress - FLASH_BASE_ADDRESS;
    uint32_t ret;

    VerifyOrExit(sFlashFd >= 0, ret = 0);

    ret = (uint32_t)pread(sFlashFd, aData, aSize, offset);

exit:
    return ret;
}
