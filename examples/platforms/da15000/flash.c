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

#include "platform-da15000.h"

#include "qspi_automode.h"

#define FLASH_BUFFER_SIZE 0x2000
#define FLASH_SECTOR_SIZE 0x1000
#define FLASH_PAGE_SIZE 0x0100

/*
 * In case that user tries to write data to flash passing as source QSPI mapped flash address
 * data must be first copied to RAM as during write QSPI controller can't read flash.
 * To achieve this, small on stack buffer will be used to copy data to RAM first.
 * This define specifies how much stack can be used for this purpose.
 */
#define ON_STACK_BUFFER_SIZE 16

otError utilsFlashInit(void)
{
    return OT_ERROR_NONE;
}

uint32_t utilsFlashGetSize(void)
{
    return (uint32_t)FLASH_BUFFER_SIZE;
}

otError utilsFlashErasePage(uint32_t aAddress)
{
    uint32_t flash_offset = aAddress & ~(FLASH_SECTOR_SIZE - 1);

    qspi_automode_erase_flash_sector(flash_offset);

    CACHE->CACHE_CTRL1_REG = CACHE_CACHE_CTRL1_REG_CACHE_FLUSH_Msk;

    return OT_ERROR_NONE;
}

otError utilsFlashStatusWait(uint32_t aTimeout)
{
    OT_UNUSED_VARIABLE(aTimeout);

    return OT_ERROR_NONE;
}

static inline bool FlashQspiAddress(const void *aBuf)
{
    if (((uint32_t)aBuf >= MEMORY_QSPIF_BASE) && ((uint32_t)aBuf < MEMORY_QSPIF_END))
    {
        return true;
    }

    return ((uint32_t)aBuf >= MEMORY_REMAPPED_BASE) && ((uint32_t)aBuf < MEMORY_REMAPPED_END) &&
           (REG_GETF(CRG_TOP, SYS_CTRL_REG, REMAP_ADR0) == 2);
}

static size_t FlashWriteFromQspi(uint32_t aAddress, const uint8_t *aQspiBuf, size_t aSize)
{
    size_t  written;
    size_t  offset = 0;
    uint8_t buf[ON_STACK_BUFFER_SIZE];

    /*
     * qspi_automode_write_flash_page can't write data if source address points to QSPI mapped
     * memory. To write data from QSPI flash, it will be first copied to small on stack
     * buffer. This buffer can be accessed when QSPI controller is working in manual mode.
     */
    while (offset < aSize)
    {
        size_t chunk = sizeof(buf) > aSize - offset ? aSize - offset : sizeof(buf);
        memcpy(buf, aQspiBuf + offset, chunk);

        written = qspi_automode_write_flash_page(aAddress + offset, buf, chunk);
        offset += written;
    }

    return offset;
}

uint32_t utilsFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    size_t written;
    size_t offset         = 0;
    bool   buf_from_flash = FlashQspiAddress(aData);

    while (offset < aSize)
    {
        /*
         * If buf is QSPI Flash memory, use function that will copy source data to RAM
         * first.
         */
        if (buf_from_flash)
        {
            written = FlashWriteFromQspi(aAddress + offset, aData + offset, aSize - offset);
        }
        else
        {
            /*
             * Try write everything, lower driver will reduce this value to accommodate
             * page boundary and and maximum write size limitation
             */
            written = qspi_automode_write_flash_page(aAddress + offset, aData + offset, aSize - offset);
        }

        offset += written;
    }

    CACHE->CACHE_CTRL1_REG = CACHE_CACHE_CTRL1_REG_CACHE_FLUSH_Msk;

    return aSize;
}

uint32_t utilsFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    return qspi_automode_read(aAddress, aData, aSize);
    ;
}
