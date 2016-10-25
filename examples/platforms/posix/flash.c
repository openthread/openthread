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

#include "platform-posix.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include <openthread-config.h>
#include <platform/flash.h>

#include <common/code_utils.hpp>

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
    char fileName[20];
    struct stat st;
    bool create = false;
    struct timeval tv;

    gettimeofday(&tv, NULL);

    memset(&st, 0, sizeof(st));

    if (stat("tmp", &st) == -1)
    {
        mkdir("tmp", 0777);
    }

    snprintf(fileName, sizeof(fileName), "tmp/%d_%d.flash", NODE_ID, (uint32_t)tv.tv_usec);

    if (access(fileName, 0))
    {
        create = true;
    }

    sFlashFd = open(fileName, O_RDWR | O_CREAT, 0666);
    lseek(sFlashFd, 0, SEEK_SET);

    VerifyOrExit(sFlashFd >= 0, error = kThreadError_Failed);

    if (create)
    {
        for (uint16_t index = 0; index < FLASH_PAGE_NUM; index++)
        {
            SuccessOrExit(error = otPlatFlashErasePage(index * FLASH_PAGE_SIZE));
        }
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
    uint32_t address;

    VerifyOrExit(sFlashFd >= 0, error = kThreadError_Failed);
    VerifyOrExit(aAddress < FLASH_SIZE, error = kThreadError_InvalidArgs);

    // Get start address of the flash page that includes aAddress
    address = aAddress & (~(uint32_t)(FLASH_PAGE_SIZE - 1));

    for (uint16_t offset = 0; offset < FLASH_PAGE_SIZE; offset++)
    {
        VerifyOrExit(pwrite(sFlashFd, &buf, 1, address + offset) == 1, error = kThreadError_Failed);
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
    uint32_t index = 0;
    uint8_t byte;

    VerifyOrExit(sFlashFd >= 0 && aAddress < FLASH_SIZE, ;);

    for (index = 0; index < aSize; index++)
    {
        VerifyOrExit((ret = otPlatFlashRead(aAddress + index, &byte, 1)) == 1, ;);
        // Use bitwise AND to emulate the behavior of flash memory
        byte &= aData[index];
        VerifyOrExit((ret = (uint32_t)pwrite(sFlashFd, &byte, 1, aAddress + index)) == 1, ;);
    }

exit:
    return index;
}

uint32_t otPlatFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t ret = 0;

    VerifyOrExit(sFlashFd >= 0 && aAddress < FLASH_SIZE, ;);
    ret = (uint32_t)pread(sFlashFd, aData, aSize, aAddress);

exit:
    return ret;
}
