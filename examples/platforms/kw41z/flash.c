/*
 *  Copyright (c) 2017, The OpenThread Authors.
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

#include "fsl_device_registers.h"
#include "fsl_flash.h"
#include <stdint.h>
#include <utils/code_utils.h>
#include <utils/flash.h>
#include "openthread/platform/alarm-milli.h"

static flash_config_t sFlashConfig;

otError utilsFlashInit(void)
{
    otError error = OT_ERROR_NONE;

    if (FLASH_Init(&sFlashConfig) != kStatus_FLASH_Success)
    {
        error = OT_ERROR_FAILED;
    }

    return error;
}

uint32_t utilsFlashGetSize(void)
{
    return FSL_FEATURE_FLASH_PFLASH_BLOCK_SIZE;
}

otError utilsFlashErasePage(uint32_t aAddress)
{
    otError  error;
    status_t status;

    status = FLASH_Erase(&sFlashConfig, aAddress, FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE, kFLASH_ApiEraseKey);

    if (status == kStatus_FLASH_Success)
    {
        error = OT_ERROR_NONE;
    }
    else if (status == kStatus_FLASH_AlignmentError)
    {
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        error = OT_ERROR_FAILED;
    }

    return error;
}

otError utilsFlashStatusWait(uint32_t aTimeout)
{
    otError  error = OT_ERROR_BUSY;
    uint32_t start = otPlatAlarmMilliGetNow();

    do
    {
        if (FTFA->FSTAT & FTFA_FSTAT_CCIF_MASK)
        {
            error = OT_ERROR_NONE;
            break;
        }
    } while (aTimeout && ((otPlatAlarmMilliGetNow() - start) < aTimeout));

    return error;
}

uint32_t utilsFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    if (FLASH_Program(&sFlashConfig, aAddress, (uint32_t *)aData, aSize) != kStatus_FLASH_Success)
    {
        aSize = 0;
    }

    return aSize;
}

uint32_t utilsFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    memcpy(aData, (uint8_t *)aAddress, aSize);

    return aSize;
}
