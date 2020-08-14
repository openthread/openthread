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
#include <stdint.h>

#include <openthread/instance.h>

#include <openthread/platform/alarm-milli.h>



#include "utils/code_utils.h"
//#include "utils/flash.h"
//#include "utils/wrap_string.h"
#include "platform-eagle.h"

// clang-format off
//#define FLASH_DATA_END_ADDR     (FLASH_BASE + FLASH_SIZE)
//#define FLASH_DATA_START_ADDR   (FLASH_DATA_END_ADDR - (FLASH_PAGE_SIZE * SETTINGS_CONFIG_PAGE_NUM))
// clang-format on

#if 0

static inline uint32_t mapAddress(uint32_t aAddress)
{
    //return aAddress + SETTINGS_CONFIG_BASE_ADDRESS;
    return aAddress;
}



/**
 * Perform any initialization for flash driver.
 *
 * @retval ::OT_ERROR_NONE    Initialize flash driver success.
 * @retval ::OT_ERROR_FAILED  Initialize flash driver fail.
 */
otError utilsFlashInit(void)
{
	return OT_ERROR_NONE;
}

/**
 * Get the size of flash that can be read/write by the caller.
 * The usable flash size is always the multiple of flash page size.
 *
 * @returns The size of the flash.
 */
uint32_t utilsFlashGetSize(void)
{
	return (SETTINGS_CONFIG_PAGE_SIZE * SETTINGS_CONFIG_PAGE_NUM);
}

/**
 * Erase one flash page that include the input address.
 * This is a non-blocking function. It can work with utilsFlashStatusWait to check when erase is done.
 *
 * The flash address starts from 0, and this function maps the input address to the physical address of flash for
 * erasing. 0 is always mapped to the beginning of one flash page. The input address should never be mapped to the
 * firmware space or any other protected flash space.
 *
 * @param[in]  aAddress  The start address of the flash to erase.
 *
 * @retval OT_ERROR_NONE           Erase flash operation is started.
 * @retval OT_ERROR_FAILED         Erase flash operation is not started.
 * @retval OT_ERROR_INVALID_ARGS    aAddress is out of range of flash or not aligned.
 */
otError utilsFlashErasePage(uint32_t aAddress)
{
    //Tl_printf("erase page , address:%x\n",aAddress);
    aAddress = aAddress&0xfffff000;
    flash_start_erase_sector((unsigned long)mapAddress(aAddress));

    return OT_ERROR_NONE;
}

/**
 * Check whether flash is ready or busy.
 *
 * @param[in]  aTimeout  The interval in milliseconds waiting for the flash operation to be done and become ready again.
 *                       zero indicates that it is a polling function, and returns current status of flash immediately.
 *                       non-zero indicates that it is blocking there until the operation is done and become ready, or
 * timeout expires.
 *
 * @retval OT_ERROR_NONE           Flash is ready for any operation.
 * @retval OT_ERROR_BUSY           Flash is busy.
 */
otError utilsFlashStatusWait(uint32_t aTimeout)
{
	otError error = OT_ERROR_BUSY;

    if (aTimeout == 0)
    {
        if (!flash_is_busy_standalone())
        {
            error = OT_ERROR_NONE;
        }
    }
    else
    {
        uint32_t startTime = otPlatAlarmMilliGetNow();

        do
        {
            if (!flash_is_busy_standalone())
            {
                error = OT_ERROR_NONE;
                break;
            }
        } while (otPlatAlarmMilliGetNow() - startTime < aTimeout);
    }
    if(error == OT_ERROR_BUSY)
    {
        //Tl_printf("flash busy!\n");
    }else
    {
        //Tl_printf("flash free!\n");
    }
    return error;
}

/**
 * Write flash. The write operation only clears bits, but never set bits.
 *
 * The flash address starts from 0, and this function maps the input address to the physical address of flash for
 * writing. 0 is always mapped to the beginning of one flash page. The input address should never be mapped to the
 * firmware space or any other protected flash space.
 *
 * @param[in]  aAddress  The start address of the flash to write.
 * @param[in]  aData     The pointer of the data to write.
 * @param[in]  aSize     The size of the data to write.
 *
 * @returns The actual size of octets write to flash.
 *          It is expected the same as aSize, and may be less than aSize.
 *          0 indicates that something wrong happens when writing.
 */
uint32_t utilsFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
	uint32_t rval = aSize;

    //Tl_printf("flash write , address:%x , size:%d\n",aAddress,aSize);

    otEXPECT_ACTION(aData, rval = 0);
    //otEXPECT_ACTION(((aAddress + aSize) < utilsFlashGetSize()) && (!(aAddress & 3)) && (!(aSize & 3)), rval = 0);

    flash_write_page((unsigned long)mapAddress(aAddress), aSize , aData );
    

exit:
    return rval;
}

/**
 * Read flash.
 *
 * The flash address starts from 0, and this function maps the input address to the physical address of flash for
 * reading. 0 is always mapped to the beginning of one flash page. The input address should never be mapped to the
 * firmware space or any other protected flash space.
 *
 * @param[in]   aAddress  The start address of the flash to read.
 * @param[Out]  aData     The pointer of buffer for reading.
 * @param[in]   aSize     The size of the data to read.
 *
 * @returns The actual size of octets read to buffer.
 *          It is expected the same as aSize, and may be less than aSize.
 *          0 indicates that something wrong happens when reading.
 */
uint32_t utilsFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
	uint32_t rval     = aSize;
    uint32_t pAddress = mapAddress(aAddress);
    uint8_t *byte     = aData;

    //Tl_printf("flash read , address:%x , size:%d\n",aAddress,aSize);

    otEXPECT_ACTION(aData, rval = 0);
    //otEXPECT_ACTION((aAddress + aSize) < utilsFlashGetSize(), rval = 0);

    //while (aSize--)
    //{
    //    *byte++ = (*(uint8_t *)(pAddress++));
    //}
    flash_read_page((unsigned long) pAddress, (unsigned long) aSize, byte);

exit:
    return rval;
}

#else
#define FLASH_BASE_ADDRESS 0x60000
#define FLASH_PAGE_SIZE 4096
#define FLASH_PAGE_NUM 2
#define FLASH_SWAP_SIZE (FLASH_PAGE_SIZE * (FLASH_PAGE_NUM / 2))



static uint32_t mapAddress(uint8_t aSwapIndex, uint32_t aOffset)
{
    uint32_t address = FLASH_BASE_ADDRESS + aOffset;

    if (aSwapIndex)
    {
        address += FLASH_SWAP_SIZE;
    }

    return address;
}

void otPlatFlashInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

}

uint32_t otPlatFlashGetSwapSize(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return FLASH_SWAP_SIZE;
}

void otPlatFlashErase(otInstance *aInstance, uint32_t aSwapIndex)
{
    OT_UNUSED_VARIABLE(aInstance);

    Tl_printf("flash_erase_sector(%d)\n",(unsigned long)mapAddress(aSwapIndex, 0));
    flash_erase_sector((unsigned long)mapAddress(aSwapIndex, 0));
}

void otPlatFlashWrite(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    Tl_printf("flash_write_page(%d,%d,%d)\n",(unsigned long)mapAddress(aSwapIndex, aOffset), aSize , (unsigned char *)aData);
    flash_write_page((unsigned long)mapAddress(aSwapIndex, aOffset), aSize , (unsigned char *)aData );
}

void otPlatFlashRead(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    Tl_printf("flash_read_page(%d,%d,%d)\n",(unsigned long) mapAddress(aSwapIndex, aOffset), (unsigned long) aSize, (unsigned char *)aData);
    flash_read_page((unsigned long) mapAddress(aSwapIndex, aOffset), (unsigned long) aSize, (unsigned char *)aData);
}

#endif
