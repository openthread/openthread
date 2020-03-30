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

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <openthread/platform/alarm-milli.h>

#include <utils/code_utils.h>

#include "platform-nrf5.h"
#include "softdevice.h"

#define FLASH_PAGE_SIZE 4096
#define FLASH_MAX_RETRY 5

typedef enum
{
    FLASH_STATE_IDLE,
    FLASH_STATE_WAITING_FOR_IDLE,
    FLASH_STATE_PENDING,
    FLASH_STATE_COMPLETE_SUCCESS,
    FLASH_STATE_COMPLETE_FAILED
} SdFlashState;

static volatile SdFlashState sState;

void nrf5SdSocFlashProcess(uint32_t aEvtId)
{
    switch (aEvtId)
    {
    case NRF_EVT_FLASH_OPERATION_SUCCESS:
        switch (sState)
        {
        case FLASH_STATE_PENDING:
            sState = FLASH_STATE_COMPLETE_SUCCESS;
            break;

        case FLASH_STATE_WAITING_FOR_IDLE:
            sState = FLASH_STATE_IDLE;
            break;

        default:
            break;
        }

        break;

    case NRF_EVT_FLASH_OPERATION_ERROR:
        switch (sState)
        {
        case FLASH_STATE_PENDING:
            sState = FLASH_STATE_COMPLETE_FAILED;
            break;

        case FLASH_STATE_WAITING_FOR_IDLE:
            sState = FLASH_STATE_IDLE;
            break;

        default:
            break;
        }

        break;

    default:
        break;
    }
}

static void waitInState(SdFlashState state)
{
    do
    {
        nrf_sdh_evts_poll();

    } while (sState == state);
}

static otError sdFlashSingleWrite(uint32_t aAddress, const uint8_t *aData, uint32_t aSize)
{
    uint32_t retval;
    uint32_t maxRetry = FLASH_MAX_RETRY;

    do
    {
        nrf_sdh_suspend();

        do
        {
            sState = FLASH_STATE_PENDING;

            retval = sd_flash_write((uint32_t *)aAddress, (uint32_t *)aData, aSize);

            if (retval == NRF_SUCCESS)
            {
                break;
            }
            else if (retval == NRF_ERROR_BUSY)
            {
                sState = FLASH_STATE_WAITING_FOR_IDLE;
            }
            else
            {
                assert(false);
            }

            waitInState(FLASH_STATE_WAITING_FOR_IDLE);

        } while (retval == NRF_ERROR_BUSY);

        waitInState(FLASH_STATE_PENDING);

        if (sState != FLASH_STATE_COMPLETE_SUCCESS)
        {
            retval = NRF_ERROR_INTERNAL;
        }

        sState = FLASH_STATE_IDLE;

        nrf_sdh_resume();

    } while (retval != NRF_SUCCESS && maxRetry--);

    return nrf5SdErrorToOtError(retval);
}

otError nrf5FlashPageErase(uint32_t aAddress)
{
    uint32_t retval;
    uint32_t maxRetry = FLASH_MAX_RETRY;

    do
    {
        nrf_sdh_suspend();

        do
        {
            sState = FLASH_STATE_PENDING;

            retval = sd_flash_page_erase(aAddress / FLASH_PAGE_SIZE);

            if (retval == NRF_SUCCESS)
            {
                break;
            }
            else if (retval == NRF_ERROR_BUSY)
            {
                sState = FLASH_STATE_WAITING_FOR_IDLE;
            }
            else
            {
                assert(false);
            }

            waitInState(FLASH_STATE_WAITING_FOR_IDLE);

        } while (retval == NRF_ERROR_BUSY);

        waitInState(FLASH_STATE_PENDING);

        if (sState != FLASH_STATE_COMPLETE_SUCCESS)
        {
            retval = NRF_ERROR_INTERNAL;
        }

        sState = FLASH_STATE_IDLE;

        nrf_sdh_resume();

    } while (retval != NRF_SUCCESS && maxRetry--);

    return nrf5SdErrorToOtError(retval);
}

bool nrf5FlashIsBusy(void)
{
    return sState != FLASH_STATE_IDLE;
}

otError nrf5FlashWrite(uint32_t aAddress, const uint8_t *aData, uint32_t aSize)
{
    otError  error     = OT_ERROR_NONE;
    uint32_t remainder = (aAddress % sizeof(uint32_t));
    uint32_t blockSize;
    uint32_t blockValue;

    otEXPECT_ACTION(sState == FLASH_STATE_IDLE, error = OT_ERROR_BUSY);

    // Check if @p aAddress is aligned to full word size. If not, make additional
    // flash write at the beginning.
    if (remainder)
    {
        blockSize  = MIN((sizeof(uint32_t) - remainder), aSize);
        blockValue = 0xffffffff;

        memcpy((uint8_t *)&blockValue + remainder, aData, blockSize);

        error = sdFlashSingleWrite(aAddress - remainder, (uint8_t *)&blockValue, sizeof(blockValue) / sizeof(uint32_t));

        otEXPECT(error == OT_ERROR_NONE);

        aAddress += blockSize;
        aData += blockSize;
        aSize -= blockSize;
    }

    otEXPECT(aSize);

    // Store the middle block of data.
    remainder = aSize % sizeof(uint32_t);
    blockSize = aSize - remainder;

    error = sdFlashSingleWrite(aAddress, aData, blockSize / sizeof(uint32_t));

    otEXPECT(error == OT_ERROR_NONE);

    aAddress += blockSize;
    aData += blockSize;
    aSize -= blockSize;

    // Store any additional bytes that didn't fit into middle block.
    if (remainder)
    {
        blockValue = 0xffffffff;

        memcpy((uint8_t *)&blockValue, aData, remainder);

        error = sdFlashSingleWrite(aAddress, (uint8_t *)&blockValue, sizeof(blockValue) / sizeof(uint32_t));

        otEXPECT(error == OT_ERROR_NONE);
    }

exit:
    return error;
}
