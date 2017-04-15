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
#include <malloc.h>

#include <openthread-config.h>
#include <utils/flash.h>
#include <utils/code_utils.h>

static int sFlashFd;
uint32_t sEraseAddress;

enum
{
    FLASH_SIZE = 0x40000,
    FLASH_PAGE_SIZE = 0x800,
    FLASH_PAGE_NUM = 128,
};

ThreadError utilsFlashInit(void)
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

    otEXPECT_ACTION(sFlashFd >= 0, error = kThreadError_Failed);

    if (create)
    {
        for (uint16_t index = 0; index < FLASH_PAGE_NUM; index++)
        {
            SuccessOrExit(error = utilsFlashErasePage(index * FLASH_PAGE_SIZE));
        }
    }

exit:
    return error;
}

uint32_t utilsFlashGetSize(void)
{
    return FLASH_SIZE;
}

ThreadError utilsFlashErasePage(uint32_t aAddress)
{
    ThreadError error = kThreadError_None;
    uint32_t address;
    uint8_t dummy_page[ FLASH_SIZE ];

    otEXPECT_ACTION(sFlashFd >= 0, error = kThreadError_Failed);
    otEXPECT_ACTION(aAddress < FLASH_SIZE, error = kThreadError_InvalidArgs);

    // Get start address of the flash page that includes aAddress
    address = aAddress & (~(uint32_t)(FLASH_PAGE_SIZE - 1));

    // Some notes:
    //
    // a)  Do not change this to a for(;;) loop that writes bytes
    //     This takes forever and causes timeouts to occur.
    //
    // The math:
    //     With pagesize = 8K pages, and n-pages=128
    //     This turns into 1 million calls to "pwrite()"
    //     Those 1million calls occur during startup.
    //     the "Thread Cert" tests, in some cases start
    //     over 30+ instances of the simulated app...
    //
    //     Tests actually run in parallel, in some cases there are
    //     70 to 80 instances of the CLI app starting up. Each one
    //     calling write - 1 million times, writing exactly 1 byte
    //
    //     That becomes super slow. Thus we write full pages
    //     On a powerful platform this is fast, very fast.
    //     On a virtual machine (ie: test-build-farm) it is slow
    //     Result is: The Thread Cert tests fail/timeout
    //
    // b)  Do not allocate memory for the page via malloc/free.
    //
    //     Reason: this causes false failures in some test modes
    //
    //     Specifically when enabling the address sanitizer ...
    //     And running thread certification tests *in*parallel*
    //     The memory requirements become huge.
    //
    // Background:
    //     The way "asan" works is this: When you free a page
    //     the "libasan" does not actually free the page instead
    //     libasan poisons the page but does not actually free the
    //     page, why? Because the goal to detect using the memory
    //     Thus ASAN is like a giant memory leak, it must hold onto
    //     the page so that it can detect "use-after-free"
    //
    //     Next (part 2)... the CERT tests, and run in parallel there
    //     are cases where 70 to 80 instances of the CLI app run
    //     in parallel - its a huge pseudo-memory leak times 70
    //     Depending upon your machine you need 8 to 16gig of ram
    //     Virtual test boxes are often configured with much less
    //
    // The solution is to use a full page on the stack
    // it could be a static, variable however, we are
    // running on Linux, we have plenty of RAM..

    // set the page to the erased state.
    memset((void *)(&dummy_page[0]), 0xff, FLASH_PAGE_SIZE);

    // Write the page
    ssize_t r;
    r =  pwrite(sFlashFd, &(dummy_page[0]), FLASH_PAGE_SIZE, address);
    VerifyOrExit(((int)r) == ((int)(FLASH_PAGE_SIZE)), error = kThreadError_Failed);


exit:
    return error;
}

ThreadError utilsFlashStatusWait(uint32_t aTimeout)
{
    (void)aTimeout;
    return kThreadError_None;
}

uint32_t utilsFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t ret = 0;
    uint32_t index = 0;
    uint8_t byte;

    otEXPECT(sFlashFd >= 0 && aAddress < FLASH_SIZE);

    for (index = 0; index < aSize; index++)
    {
        otEXPECT((ret = utilsFlashRead(aAddress + index, &byte, 1)) == 1);
        // Use bitwise AND to emulate the behavior of flash memory
        byte &= aData[index];
        otEXPECT((ret = (uint32_t)pwrite(sFlashFd, &byte, 1, aAddress + index)) == 1);
    }

exit:
    return index;
}

uint32_t utilsFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t ret = 0;

    otEXPECT(sFlashFd >= 0 && aAddress < FLASH_SIZE);
    ret = (uint32_t)pread(sFlashFd, aData, aSize, aAddress);

exit:
    return ret;
}
