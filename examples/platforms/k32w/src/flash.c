/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#include "openthread/platform/flash.h"
#include "fsl_device_registers.h"
#include "fsl_flash.h"
#include "openthread-core-config.h"
#include <utils/code_utils.h>
#include "openthread/platform/alarm-milli.h"

#define USE_MEM_COPY_FOR_READ 0

#define NUMBER_OF_INTEGERS 4
#define ONE_READ 1
#define BYTES_IN_ONE_READ (NUMBER_OF_INTEGERS * sizeof(uint32_t))
#define NORMAL_READ_MODE 0
#define ONE_PAGE 1
#define BYTES_ALINGMENT 16

uint8_t         pageBuffer[FLASH_PAGE_SIZE] __attribute__((aligned(4))) = {0};
static uint32_t sNvFlashStartAddr;
static uint32_t sNvFlashEndAddr;

static bool     mapToNvFlashAddress(uint32_t *aAddress);
static void     copyFromFlash(uint8_t *pDst, uint8_t *pSrc, uint32_t cBytes);
static uint32_t blankCheckAndErase(uint8_t *pageAddr);

void otPlatFlashInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    extern uint32_t __nv_storage_start_address;
    extern uint32_t __nv_storage_end_address;

    FLASH_Init(FLASH);

    sNvFlashStartAddr = (uint32_t)&__nv_storage_start_address;
    sNvFlashEndAddr   = (uint32_t)&__nv_storage_end_address;
}

otError utilsFlashErasePage(uint32_t aAddress)
{
    otError  error = OT_ERROR_INVALID_ARGS;
    status_t status;
    uint32_t address = aAddress;

    /* Map address to NV Flash space and check boundaries */
    if (mapToNvFlashAddress(&address))
    {
        /* If address is aligned to page size */
        if ((address % FLASH_PAGE_SIZE) == 0)
        {
            error = OT_ERROR_NONE;

            status = blankCheckAndErase((uint8_t *)address);
            otEXPECT_ACTION((status & FLASH_DONE), error = OT_ERROR_FAILED);
        }
    }

exit:
    return error;
}

void otPlatFlashWrite(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize)
{
    uint32_t result = 0;
    status_t status;
    uint32_t address = aOffset;
    uint32_t alignAddr;
    uint32_t bytes;

    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aSwapIndex);

    /* Map address to NV Flash space and check boundaries */
    if (mapToNvFlashAddress(&address))
    {
        /* Check to see if data is written outside NV Flash space */
        if ((address + aSize) <= sNvFlashEndAddr)
        {
            alignAddr = address - (address % FLASH_PAGE_SIZE);
            bytes     = (address - alignAddr);
            result    = aSize;

            if (bytes)
            {
                uint32_t unalignedBytes = FLASH_PAGE_SIZE - bytes;

                if (unalignedBytes > aSize)
                {
                    unalignedBytes = aSize;
                }

                copyFromFlash(pageBuffer, (void *)alignAddr, FLASH_PAGE_SIZE);
                memcpy(&pageBuffer[bytes], aData, unalignedBytes);

                status = blankCheckAndErase((uint8_t *)alignAddr);
                otEXPECT_ACTION((status & FLASH_DONE), result = 0);

                status = FLASH_Program(FLASH, (uint32_t *)alignAddr, (uint32_t *)pageBuffer, FLASH_PAGE_SIZE);
                otEXPECT_ACTION((status & FLASH_DONE), result = 0);

                address += unalignedBytes;
                /* if size is less than the distance to the end of program block
                   unalignedBytes has been shrunk , after size is decremented it will become 0 */
                aData += unalignedBytes;
                aSize -= unalignedBytes;
            }

            bytes = aSize & ~(FLASH_PAGE_SIZE - 1U);

            /* Now dest is on an aligned boundary */
            /* bytes is an integer number of program blocks (pages) */
            while (bytes)
            {
                status = blankCheckAndErase((uint8_t *)address);
                otEXPECT_ACTION((status & FLASH_DONE), result = 0);

                status = FLASH_Program(FLASH, (uint32_t *)address, (uint32_t *)aData, FLASH_PAGE_SIZE);
                otEXPECT_ACTION((status & FLASH_DONE), result = 0);

                address += FLASH_PAGE_SIZE;
                aData += FLASH_PAGE_SIZE;
                aSize -= FLASH_PAGE_SIZE;
                bytes -= FLASH_PAGE_SIZE;
            }

            /* dest is still aligned because we have increased it by a multiple of the program block (page) */
            if (aSize)
            {
                status = blankCheckAndErase((uint8_t *)address);
                otEXPECT_ACTION((status & FLASH_DONE), result = 0);

                status = FLASH_Program(FLASH, (uint32_t *)address, (uint32_t *)aData, aSize);
                otEXPECT_ACTION((status & FLASH_DONE), result = 0);
            }
        }
    }

exit:
    /* There are times when the result != 0.
     * Use this workaround until we replace the flash code with the Packet Data Manager.
     */
    if (result)
    {
        result = 0;
    }
}

void otPlatFlashRead(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize)
{
    uint32_t address = aOffset;

    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aSwapIndex);

    /* Map address to NV Flash space and check boundaries */
    if (mapToNvFlashAddress(&address))
    {
        /* Check to see if data is read outside NV Flash space */
        if ((address + aSize) <= sNvFlashEndAddr)
        {
            copyFromFlash(aData, (uint8_t *)address, aSize);
        }
    }
}

static bool mapToNvFlashAddress(uint32_t *aAddress)
{
    bool     status  = true;
    uint32_t address = *aAddress + sNvFlashStartAddr;

    if ((address < sNvFlashStartAddr) || (address > sNvFlashEndAddr))
    {
        status = false;
    }
    else
    {
        *aAddress = address;
    }

    return status;
}

uint32_t blankCheckAndErase(uint8_t *pageAddr)
{
    uint32_t status = FLASH_BlankCheck(FLASH, pageAddr, pageAddr + FLASH_PAGE_SIZE - 1);
    if (status & FLASH_FAIL)
    {
        status = FLASH_Erase(FLASH, pageAddr, (pageAddr + FLASH_PAGE_SIZE - 1));
    }
    else
    {
        status = FLASH_DONE;
    }

    return status;
}

static void copyFromFlash(uint8_t *pDst, uint8_t *pSrc, uint32_t cBytes)
{
#if !USE_MEM_COPY_FOR_READ

    uint32_t nbOfReads;
    uint32_t aligningOffset;
    uint32_t temp[NUMBER_OF_INTEGERS];
    uint32_t bytesLeft   = cBytes;
    uint32_t bytesToRead = 0;

    /* Flash driver reads 16 bytes in one run, so calculating the number of reads from Flash
       is needed */
    nbOfReads = cBytes / BYTES_IN_ONE_READ;
    if (cBytes % BYTES_IN_ONE_READ)
    {
        nbOfReads++;
    }

    /* calculate aligning offset -> the number of bytes from a 16 byte aligned address the
       read address is located */
    aligningOffset = (uint32_t)pSrc % BYTES_ALINGMENT;

    for (uint32_t i = 0; i < nbOfReads; i++)
    {
        /* Read from Flash */
        FLASH_Read(FLASH, (uint8_t *)(pSrc - aligningOffset + i * BYTES_IN_ONE_READ), NORMAL_READ_MODE,
                   (uint32_t *)temp);

        if (0 == i)
        {
            bytesToRead =
                (bytesLeft < BYTES_IN_ONE_READ - aligningOffset) ? bytesLeft : BYTES_IN_ONE_READ - aligningOffset;

            /* first read must take into account align offset */
            memcpy((void *)pDst, (void *)temp + aligningOffset, bytesToRead);
            bytesLeft -= bytesToRead;
            pDst += bytesToRead;
        }
        else
        {
            bytesToRead = (bytesLeft < BYTES_IN_ONE_READ) ? bytesLeft : BYTES_IN_ONE_READ;
            memcpy((void *)pDst, (void *)temp, bytesToRead);
            bytesLeft -= bytesToRead;
            pDst += bytesToRead;
        }
    }

#else
    while (cBytes)
    {
        *(pDst) = *(pSrc);
        pDst    = pDst + 1;
        pSrc    = pSrc + 1;
        cBytes--;
    }
#endif
}
