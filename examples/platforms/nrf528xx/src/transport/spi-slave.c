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

/**
 * @file
 *   This file implements the OpenThread platform abstraction for SPIS communication.
 *
 */
#include <assert.h>

#include <utils/code_utils.h>
#include <openthread/platform/spi-slave.h>

#include "platform-nrf5-transport.h"
#include <hal/nrf_gpio.h>
#include <nrfx.h>
#include <nrfx_spis.h>

#include "openthread-system.h"

#if (SPIS_AS_SERIAL_TRANSPORT == 1)

/**
 *  SPI Slave transaction variables.
 */
static void *                                    sContext                = NULL;
static uint8_t *                                 sOutputBuf              = NULL;
static uint16_t                                  sOutputBufLen           = 0;
static uint8_t *                                 sInputBuf               = NULL;
static uint16_t                                  sInputBufLen            = 0;
static bool                                      sRequestTransactionFlag = false;
static bool                                      sFurtherProcessingFlag  = false;
static otPlatSpiSlaveTransactionProcessCallback  sProcessCallback        = NULL;
static otPlatSpiSlaveTransactionCompleteCallback sCompleteCallback       = NULL;
static const nrfx_spis_t                         sSpiSlaveInstance       = NRFX_SPIS_INSTANCE(SPIS_INSTANCE);

static void spisEventHandler(nrfx_spis_evt_t const *aEvent, void *aContext)
{
    OT_UNUSED_VARIABLE(aContext);

    switch (aEvent->evt_type)
    {
    case NRFX_SPIS_BUFFERS_SET_DONE:
        if (sRequestTransactionFlag)
        {
            // Host IRQ pin is active low.
            nrf_gpio_pin_clear(SPIS_PIN_HOST_IRQ);
        }
        break;

    case NRFX_SPIS_XFER_DONE:
        // Ensure Host IRQ pin is set.
        nrf_gpio_pin_set(SPIS_PIN_HOST_IRQ);

        // Execute application callback.
        if (sCompleteCallback(sContext, sOutputBuf, sOutputBufLen, sInputBuf, sInputBufLen, aEvent->rx_amount))
        {
            // Further processing is required.
            sFurtherProcessingFlag = true;

            otSysEventSignalPending();
        }
        break;

    default:
        assert(false);
        break;
    }
}

void nrf5SpiSlaveInit(void)
{
    // Intentionally empty.
}

void nrf5SpiSlaveDeinit(void)
{
    sOutputBuf              = NULL;
    sOutputBufLen           = 0;
    sInputBuf               = NULL;
    sInputBufLen            = 0;
    sRequestTransactionFlag = false;

    otPlatSpiSlaveDisable();
}

void nrf5SpiSlaveProcess(void)
{
    otEXPECT(sFurtherProcessingFlag == true);

    // Clear further processing flag.
    sFurtherProcessingFlag = false;

    // Perform any further processing if necessary.
    sProcessCallback(sContext);

exit:
    return;
}

otError otPlatSpiSlaveEnable(otPlatSpiSlaveTransactionCompleteCallback aCompleteCallback,
                             otPlatSpiSlaveTransactionProcessCallback  aProcessCallback,
                             void *                                    aContext)
{
    otError            result = OT_ERROR_NONE;
    nrfx_err_t         error  = NRFX_SUCCESS;
    nrfx_spis_config_t config = NRFX_SPIS_DEFAULT_CONFIG;

    assert(aCompleteCallback != NULL);
    assert(aProcessCallback != NULL);

    // Check if SPI Slave interface is already enabled.
    otEXPECT_ACTION(sCompleteCallback == NULL, error = OT_ERROR_ALREADY);

    config.csn_pin      = SPIS_PIN_CSN;
    config.miso_pin     = SPIS_PIN_MISO;
    config.mosi_pin     = SPIS_PIN_MOSI;
    config.sck_pin      = SPIS_PIN_SCK;
    config.mode         = SPIS_MODE;
    config.bit_order    = SPIS_BIT_ORDER;
    config.irq_priority = SPIS_IRQ_PRIORITY;

    error = nrfx_spis_init(&sSpiSlaveInstance, &config, spisEventHandler, NULL);
    assert(error == NRFX_SUCCESS);

    // Set up Host IRQ pin.
    nrf_gpio_pin_set(SPIS_PIN_HOST_IRQ);
    nrf_gpio_cfg_output(SPIS_PIN_HOST_IRQ);

    // Set proper callback and context.
    sProcessCallback  = aProcessCallback;
    sCompleteCallback = aCompleteCallback;
    sContext          = aContext;

exit:
    return result;
}

void otPlatSpiSlaveDisable(void)
{
    nrfx_spis_uninit(&sSpiSlaveInstance);
}

otError otPlatSpiSlavePrepareTransaction(uint8_t *aOutputBuf,
                                         uint16_t aOutputBufLen,
                                         uint8_t *aInputBuf,
                                         uint16_t aInputBufLen,
                                         bool     aRequestTransactionFlag)
{
    otError            result           = OT_ERROR_NONE;
    nrfx_err_t         error            = NRFX_SUCCESS;
    nrf_spis_semstat_t semaphore_status = nrf_spis_semaphore_status_get(sSpiSlaveInstance.p_reg);

    assert(sCompleteCallback != NULL);

    otEXPECT_ACTION(((semaphore_status != NRF_SPIS_SEMSTAT_SPIS) && (semaphore_status != NRF_SPIS_SEMSTAT_CPUPENDING)),
                    error = OT_ERROR_BUSY);

    if (aOutputBuf != NULL)
    {
        sOutputBuf    = aOutputBuf;
        sOutputBufLen = aOutputBufLen;
    }

    if (aInputBuf != NULL)
    {
        sInputBuf    = aInputBuf;
        sInputBufLen = aInputBufLen;
    }

    sRequestTransactionFlag = aRequestTransactionFlag;

    error = nrfx_spis_buffers_set(&sSpiSlaveInstance, sOutputBuf, sOutputBufLen, sInputBuf, sInputBufLen);
    assert(error == NRFX_SUCCESS);

exit:
    return result;
}

#endif // SPIS_AS_SERIAL_TRANSPORT == 1
