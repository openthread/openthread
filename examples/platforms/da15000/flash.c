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

#include <openthread/config.h>
#include <openthread/types.h>
#include <openthread/platform/alarm.h>

#include "hw_qspi.h"
#include "qspi_automode.h"

#define FLASH_SECTOR_SIZE             0x1000
#define FLASH_BUFFER_SIZE             0x2000

#define W25Q_READ_STATUS_REGISTER1   0x05

/* Erase/Write in progress */
#define W25Q_STATUS1_BUSY_BIT        0
#define W25Q_STATUS1_BUSY_MASK       (1 << W25Q_STATUS1_BUSY_BIT)

static uint32_t sWaitStatusTimeout;

QSPI_SECTION static  uint8_t qspi_get_erase_status(void)
{
    QSPIC->QSPIC_CHCKERASE_REG = 0;
    return HW_QSPIC_REG_GETF(ERASECTRL, ERS_STATE);
}

volatile bool DisableErase = false;

// Erase QSPI memory section as non-blocking function. Remember to check utilsFlashStatusWait before write or read!!
otError utilsFlashErasePage(uint32_t aAddress)
{

    uint32_t flash_offset = (aAddress) & ~(FLASH_SECTOR_SIZE - 1);

    if (DisableErase)
    {
        return OT_ERROR_NONE;
    }

    qspi_automode_init();
    qspi_automode_erase_flash_sector(flash_offset);

    CACHE->CACHE_CTRL1_REG = CACHE_CACHE_CTRL1_REG_CACHE_FLUSH_Msk;

    return OT_ERROR_NONE;
}


uint32_t utilsFlashGetSize(void)
{

    return (uint32_t)FLASH_BUFFER_SIZE;
}

otError utilsFlashStatusWait(uint32_t aTimeout)
{

    sWaitStatusTimeout = otPlatAlarmGetNow();

    while (qspi_get_erase_status()  && ((otPlatAlarmGetNow() - sWaitStatusTimeout) < aTimeout)) {}

    if (qspi_get_erase_status())
    {
        return OT_ERROR_BUSY;
    }
    else
    {
        return OT_ERROR_NONE;
    }
}

otError utilsFlashInit(void)
{
    qspi_automode_flash_power_up();

    if (utilsFlashStatusWait(1000) == OT_ERROR_BUSY)
    {
        return OT_ERROR_FAILED;
    }
    else
    {
        return OT_ERROR_NONE;
    }
}

uint32_t utilsFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{

    size_t offset = 0;
    size_t written;

    while (offset < aSize)
    {
        written = qspi_automode_write_flash_page(aAddress + offset, aData + offset,
                                                 aSize - offset);
        offset += written;
    }

    return offset;
}

uint32_t utilsFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    memcpy(aData, (void *)(MEMORY_QSPIF_BASE + aAddress), aSize);
    return aSize;
}
