/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#include <utils/code_utils.h>
#include <openthread/platform/spi-slave.h>

#include <hal/nrf_gpio.h>
#include <hal/nrf_spis.h>
#include <platform-nrf5.h>

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

void nrf5SpiSlaveInit(void)
{
    /* Intentionally empty. */
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

    /* Clear further processing flag. */
    sFurtherProcessingFlag = false;

    /* Perform any further processing if necessary. */
    sProcessCallback(sContext);

exit:
    return;
}

otError otPlatSpiSlaveEnable(otPlatSpiSlaveTransactionCompleteCallback aCompleteCallback,
                             otPlatSpiSlaveTransactionProcessCallback  aProcessCallback,
                             void *                                    aContext)
{
    otError error = OT_ERROR_NONE;

    /* Check if SPI Slave interface is already enabled. */
    otEXPECT_ACTION(sCompleteCallback == NULL, error = OT_ERROR_ALREADY);

    /* Set up MISO/MOSI/SCK/CSN and Host IRQ. */
    nrf_gpio_cfg_input(SPIS_PIN_MISO, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(SPIS_PIN_MOSI, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(SPIS_PIN_SCK, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(SPIS_PIN_CSN, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_pin_set(SPIS_PIN_HOST_IRQ);
    nrf_gpio_cfg_output(SPIS_PIN_HOST_IRQ);

    nrf_spis_pins_set(SPIS_INSTANCE, SPIS_PIN_SCK, SPIS_PIN_MOSI, SPIS_PIN_MISO, SPIS_PIN_CSN);

    /* Set buffer pointers. */
    nrf_spis_rx_buffer_set(SPIS_INSTANCE, NULL, 0);
    nrf_spis_tx_buffer_set(SPIS_INSTANCE, NULL, 0);

    /* Configure SPIS Mode and Bit order. */
    nrf_spis_configure(SPIS_INSTANCE, SPIS_MODE, SPIS_BIT_ORDER);

    /* Use 0xFF character to indicate that there is no transmit buffer set up or overflow occured
       as described in API documentation. */
    nrf_spis_def_set(SPIS_INSTANCE, 0xFF);
    nrf_spis_orc_set(SPIS_INSTANCE, 0xFF);

    /* Clear SPIS specific events. */
    nrf_spis_event_clear(SPIS_INSTANCE, NRF_SPIS_EVENT_END);
    nrf_spis_event_clear(SPIS_INSTANCE, NRF_SPIS_EVENT_ACQUIRED);

    /* Enable interrupts for ACQUIRED and END events. */
    nrf_spis_int_enable(SPIS_INSTANCE, (NRF_SPIS_INT_ACQUIRED_MASK | NRF_SPIS_INT_END_MASK));

    /* Configure NVIC to handle SPIS interrupts. */
    NVIC_SetPriority(SPIS_IRQN, SPIS_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(SPIS_IRQN);
    NVIC_EnableIRQ(SPIS_IRQN);

    /* Enable SPI slave device. */
    nrf_spis_enable(SPIS_INSTANCE);

    /* Set proper callback and context. */
    sProcessCallback  = aProcessCallback;
    sCompleteCallback = aCompleteCallback;
    sContext          = aContext;

exit:
    return error;
}

void otPlatSpiSlaveDisable(void)
{
    /* Disable interrupts for ACQUIRED and END events. */
    nrf_spis_int_disable(SPIS_INSTANCE, (NRF_SPIS_INT_ACQUIRED_MASK | NRF_SPIS_INT_END_MASK));

    /* Disable NVIC interrupt. */
    NVIC_DisableIRQ(SPIS_IRQN);

    /* Disable SPIS instance. */
    nrf_spis_disable(SPIS_INSTANCE);
}

otError otPlatSpiSlavePrepareTransaction(uint8_t *aOutputBuf,
                                         uint16_t aOutputBufLen,
                                         uint8_t *aInputBuf,
                                         uint16_t aInputBufLen,
                                         bool     aRequestTransactionFlag)
{
    otError            error            = OT_ERROR_NONE;
    nrf_spis_semstat_t semaphore_status = nrf_spis_semaphore_status_get(SPIS_INSTANCE);

    otEXPECT_ACTION(sCompleteCallback != NULL, error = OT_ERROR_INVALID_STATE);

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

    /* Trigger acquired event. */
    nrf_spis_task_trigger(SPIS_INSTANCE, NRF_SPIS_TASK_ACQUIRE);

exit:
    return error;
}

/**
 * Interrupt handler of SPIS peripherial.
 */
void SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler(void)
{
    /* Check for SPI semaphore acquired event. */
    if (nrf_spis_event_check(SPIS_INSTANCE, NRF_SPIS_EVENT_ACQUIRED))
    {
        nrf_spis_event_clear(SPIS_INSTANCE, NRF_SPIS_EVENT_ACQUIRED);

        /* Set actual TX/RX buffers. */
        nrf_spis_tx_buffer_set(SPIS_INSTANCE, sOutputBuf, sOutputBufLen);
        nrf_spis_rx_buffer_set(SPIS_INSTANCE, sInputBuf, sInputBufLen);

        if (sRequestTransactionFlag)
        {
            /* Interrupt pin is active low. */
            nrf_gpio_pin_clear(SPIS_PIN_HOST_IRQ);
        }

        /* Trigger RELEASE event immediately. */
        nrf_spis_task_trigger(SPIS_INSTANCE, NRF_SPIS_TASK_RELEASE);
    }

    /* Check for SPI transaction complete event. */
    if (nrf_spis_event_check(SPIS_INSTANCE, NRF_SPIS_EVENT_END))
    {
        nrf_spis_event_clear(SPIS_INSTANCE, NRF_SPIS_EVENT_END);

        if (sRequestTransactionFlag)
        {
            nrf_gpio_pin_set(SPIS_PIN_HOST_IRQ);
        }

        /* Discard all transactions until buffers are updated. */
        nrf_spis_tx_buffer_set(SPIS_INSTANCE, sOutputBuf, 0);
        nrf_spis_rx_buffer_set(SPIS_INSTANCE, sInputBuf, 0);

        /* Execute application callback. */
        if (sCompleteCallback(sContext, sOutputBuf, sOutputBufLen, sInputBuf, sInputBufLen,
                              nrf_spis_rx_amount_get(SPIS_INSTANCE)))
        {
            /* Further processing is required. */
            sFurtherProcessingFlag = true;
        }
    }
}
