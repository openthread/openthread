/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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

#define _XOPEN_SOURCE 500

#include "platform_qorvo.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openthread/config.h>

#include "utils/code_utils.h"
#include "utils/flash.h"

static int sFlashFd;
uint32_t   sEraseAddress;

#define FLASH_SIZE 0x40000
#define FLASH_PAGE_SIZE 0x800
#define FLASH_PAGE_NUM 128

otError utilsFlashInit(void)
{
    otError     error = OT_ERROR_NONE;
    char        fileName[20];
    struct stat st;
    bool        create = false;

    memset(&st, 0, sizeof(st));

    if (stat("tmp", &st) == -1)
    {
        mkdir("tmp", 0777);
    }

    snprintf(fileName, sizeof(fileName), "tmp/node.flash");

    if (access(fileName, 0))
    {
        create = true;
    }

    sFlashFd = open(fileName, O_RDWR | O_CREAT, 0666);
    lseek(sFlashFd, 0, SEEK_SET);

    otEXPECT_ACTION(sFlashFd >= 0, error = OT_ERROR_FAILED);

    if (create)
    {
        for (uint16_t index = 0; index < FLASH_PAGE_NUM; index++)
        {
            error = utilsFlashErasePage(index * FLASH_PAGE_SIZE);
            otEXPECT(error == OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

uint32_t utilsFlashGetSize(void)
{
    return FLASH_SIZE;
}

otError utilsFlashErasePage(uint32_t aAddress)
{
    otError  error = OT_ERROR_NONE;
    uint32_t address;
    uint8_t  dummyPage[FLASH_SIZE];
    ssize_t  r;

    otEXPECT_ACTION(sFlashFd >= 0, error = OT_ERROR_FAILED);
    otEXPECT_ACTION(aAddress < FLASH_SIZE, error = OT_ERROR_INVALID_ARGS);

    // Get start address of the flash page that includes aAddress
    address = aAddress & (~(uint32_t)(FLASH_PAGE_SIZE - 1));

    // set the page to the erased state.
    memset((void *)(&dummyPage[0]), 0xff, FLASH_PAGE_SIZE);

    // Write the page
    r = pwrite(sFlashFd, &(dummyPage[0]), FLASH_PAGE_SIZE, (off_t)address);
    otEXPECT_ACTION((r) == ((FLASH_PAGE_SIZE)), error = OT_ERROR_FAILED);

exit:
    return error;
}

otError utilsFlashStatusWait(uint32_t aTimeout)
{
    OT_UNUSED_VARIABLE(aTimeout);

    return OT_ERROR_NONE;
}

uint32_t utilsFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t ret   = 0;
    uint32_t index = 0;
    uint8_t  byte;

    otEXPECT(sFlashFd >= 0 && aAddress < FLASH_SIZE);

    for (index = 0; index < aSize; index++)
    {
        ret = utilsFlashRead(aAddress + index, &byte, 1);
        otEXPECT(ret == 1);
        // Use bitwise AND to emulate the behavior of flash memory
        byte &= aData[index];
        ret = (uint32_t)pwrite(sFlashFd, &byte, 1, (off_t)(aAddress + index));
        otEXPECT(ret == 1);
    }

exit:
    return index;
}

uint32_t utilsFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t ret = 0;

    otEXPECT(sFlashFd >= 0 && aAddress < FLASH_SIZE);
    ret = (uint32_t)pread(sFlashFd, aData, aSize, (off_t)aAddress);

exit:
    return ret;
}
