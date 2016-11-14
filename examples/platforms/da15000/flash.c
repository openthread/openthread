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


#include <openthread-types.h>
#include "hw_qspi.h"
#include "qspi_automode.h"
#include "platform/alarm.h"
#include "openthread-config.h"

#define FLASH_SECTOR_SIZE       0x1000
#define FLASH_PAGE_SIZE         0x0100
#define FLASH_BUFFER_SIZE       0x2000          // 8kB
#define FLASH_OFFSET            0x7B000         // Flash memory size is 512kB (0x7D000) and starts from 0x8000000. Offset point to 500kB position

#define W25Q_READ_STATUS_REGISTER1   0x05

/* Erase/Write in progress */
#define W25Q_STATUS1_BUSY_BIT        0
#define W25Q_STATUS1_BUSY_MASK       (1 << W25Q_STATUS1_BUSY_BIT)

static uint32_t wait_status_timeout;

#define QSPI_SECTION __attribute__ ((section ("text_retained")))

QSPI_SECTION static inline uint8_t qspi_get_erase_status(void)
{
    QSPIC->QSPIC_CHCKERASE_REG = 0;
    return HW_QSPIC_REG_GETF(ERASECTRL, ERS_STATE);
}

// Erase QSPI memory section as non-blocking function. Remember to check utilsFlashStatusWait before write or read!!
ThreadError utilsFlashErasePage(uint32_t aAddress)
{

    uint32_t flash_offset = (aAddress + FLASH_OFFSET) & ~(FLASH_SECTOR_SIZE - 1);

    if (aAddress > FLASH_BUFFER_SIZE)
    {
        return kThreadError_InvalidArgs;
    }

    /* Setup erase block page */
    HW_QSPIC_REG_SETF(ERASECTRL, ERS_ADDR, (flash_offset >>= 4));
    /* Fire erase */
    HW_QSPIC_REG_SETF(ERASECTRL, ERASE_EN, 1);

    CACHE->CACHE_CTRL1_REG = CACHE_CACHE_CTRL1_REG_CACHE_FLUSH_Msk;

    return kThreadError_None;
}


uint32_t utilsFlashGetSize(void)
{

    return (uint32_t)FLASH_BUFFER_SIZE;
}

ThreadError utilsFlashStatusWait(uint32_t aTimeout)
{

    wait_status_timeout = otPlatAlarmGetNow();

    while (qspi_get_erase_status()  && ((otPlatAlarmGetNow() - wait_status_timeout) < aTimeout));

    if (qspi_get_erase_status())
    {
        return kThreadError_Busy;
    }
    else
    {
        return kThreadError_None;
    }

}

ThreadError utilsFlashInit(void)
{

    qspi_automode_init();
    qspi_automode_flash_power_up();

    utilsFlashErasePage(0);
    utilsFlashErasePage(0x1000);

    if (utilsFlashStatusWait(1000) == kThreadError_Busy)
    {
        return kThreadError_Failed;
    }
    else
    {
        return kThreadError_None;
    }
}

uint32_t utilsFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{

    uint32_t flash_offset = (aAddress + FLASH_OFFSET);
    size_t offset = 0;
    size_t written;

    while (flash_offset < (aAddress + FLASH_OFFSET) + aSize)
    {
        qspi_automode_erase_flash_sector(flash_offset);
        flash_offset += FLASH_SECTOR_SIZE;
    }

    flash_offset = (aAddress + FLASH_OFFSET);

    while (offset < aSize)
    {
        written = qspi_automode_write_flash_page(flash_offset + offset, aData + offset,
                                                 aSize - offset);
        offset += written;
    }

    return flash_offset;
}

uint32_t utilsFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{

    size_t written = 0;
    size_t offset = 0;
    uint32_t flash_offset = (aAddress + FLASH_OFFSET);

    memcpy(aData, (void *)(MEMORY_QSPIF_BASE + flash_offset), aSize);

    return aSize;
}
