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

/**
 * @file
 *   This file implements the OpenThread platform abstraction for UART communication.
 *
 */

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <utils/code_utils.h>
#include <utils/uart.h>
#include <openthread/platform/toolchain.h>

#include "openthread-system.h"

#include "platform-nrf5-transport.h"
#include <hal/nrf_gpio.h>
#include <hal/nrf_uarte.h>
#include <nrf_drv_clock.h>

#if (UART_AS_SERIAL_TRANSPORT == 1)

/**
 * UART enable flag.
 */
bool sUartEnabled = false;

/**
 * UART TX buffer variables.
 */
static const uint8_t *sTransmitBuffer = NULL;
static volatile bool  sTransmitDone   = 0;

/**
 * UART RX ring buffer variables.
 */
static uint8_t           sReceiveBuffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t sReceiveHead = 0;
static volatile uint16_t sReceiveTail = 0;

/**
 * Function for returning the next RX buffer index.
 *
 * @return Next RX buffer index.
 */
static __INLINE uint16_t getNextRxBufferIndex()
{
    return (sReceiveHead + 1) % UART_RX_BUFFER_SIZE;
}

/**
 * Function for checking if RX buffer is full.
 *
 * @retval true  RX buffer is full.
 * @retval false RX buffer is not full.
 */
static __INLINE bool isRxBufferFull()
{
    return getNextRxBufferIndex() == sReceiveTail;
}

/**
 * Function for checking if RX buffer is empty.
 *
 * @retval true  RX buffer is empty.
 * @retval false RX buffer is not empty.
 */
static __INLINE bool isRxBufferEmpty()
{
    uint16_t head = sReceiveHead;
    return (head == sReceiveTail);
}

/**
 * Function for notifying application about new bytes received.
 */
static void processReceive(void)
{
    // Set head position to not be changed during read procedure.
    uint16_t head = sReceiveHead;
    uint8_t *position;

    otEXPECT(isRxBufferEmpty() == false);

    // In case head roll back to the beginning of the buffer, notify about left
    // bytes from the end of the buffer.
    if (head < sReceiveTail)
    {
        position = &sReceiveBuffer[sReceiveTail];
        otPlatUartReceived(position, (UART_RX_BUFFER_SIZE - sReceiveTail));
        sReceiveTail = 0;
    }

    // Notify about received bytes.
    if (head > sReceiveTail)
    {
        position = &sReceiveBuffer[sReceiveTail];
        otPlatUartReceived(position, (head - sReceiveTail));
        sReceiveTail = head;
    }

    // In case interrupts were disabled due to the full buffer, re-enable it now.
    if (!nrf_uarte_int_enable_check(UART_INSTANCE, NRF_UARTE_INT_RXDRDY_MASK))
    {
        nrf_uarte_int_enable(UART_INSTANCE, NRF_UARTE_INT_RXDRDY_MASK | NRF_UARTE_INT_ERROR_MASK);
    }

exit:
    return;
}

otError otPlatUartFlush(void)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

/**
 * Function for notifying application about transmission being done.
 */
static void processTransmit(void)
{
    otEXPECT(sTransmitBuffer != NULL);

    if (sTransmitDone)
    {
        // Clear Transmition transaction and notify application.
        sTransmitBuffer = NULL;
        sTransmitDone   = false;
        otPlatUartSendDone();
    }

exit:
    return;
}

void nrf5UartProcess(void)
{
    processReceive();
    processTransmit();
}

void nrf5UartInit(void)
{
    // Intentionally empty.
}

void nrf5UartClearPendingData(void)
{
    // Intentionally empty.
}

void nrf5UartDeinit(void)
{
    if (sUartEnabled)
    {
        otPlatUartDisable();
    }
}

otError otPlatUartEnable(void)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sUartEnabled == false, error = OT_ERROR_ALREADY);

    // Set up TX and RX pins.
    nrf_gpio_pin_set(UART_PIN_TX);
    nrf_gpio_cfg_output(UART_PIN_TX);
    nrf_gpio_cfg_input(UART_PIN_RX, NRF_GPIO_PIN_NOPULL);
    nrf_uarte_txrx_pins_set(UART_INSTANCE, UART_PIN_TX, UART_PIN_RX);

#if (UART_HWFC_ENABLED == 1)
    // Set up CTS and RTS pins.
    nrf_gpio_cfg_input(UART_PIN_CTS, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_pin_set(UART_PIN_RTS);
    nrf_gpio_cfg_output(UART_PIN_RTS);
    nrf_uarte_hwfc_pins_set(UART_INSTANCE, UART_PIN_RTS, UART_PIN_CTS);

    nrf_uarte_configure(UART_INSTANCE, UART_PARITY, NRF_UARTE_HWFC_ENABLED);
#else
    nrf_uarte_configure(UART_INSTANCE, UART_PARITY, NRF_UARTE_HWFC_DISABLED);
#endif

    // Configure baudrate.
    nrf_uarte_baudrate_set(UART_INSTANCE, UART_BAUDRATE);

    // Clear UART specific events.
    nrf_uarte_event_clear(UART_INSTANCE, NRF_UARTE_EVENT_ENDTX);
    nrf_uarte_event_clear(UART_INSTANCE, NRF_UARTE_EVENT_ERROR);
    nrf_uarte_event_clear(UART_INSTANCE, NRF_UARTE_EVENT_RXDRDY);

    // Enable interrupts for TX.
    nrf_uarte_int_enable(UART_INSTANCE, NRF_UARTE_INT_ENDTX_MASK);

    // Enable interrupts for RX.
    nrf_uarte_int_enable(UART_INSTANCE, NRF_UARTE_INT_RXDRDY_MASK | NRF_UARTE_INT_ERROR_MASK);

    // Configure NVIC to handle UART interrupts.
    NVIC_SetPriority(UART_IRQN, UART_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(UART_IRQN);
    NVIC_EnableIRQ(UART_IRQN);

    // Start HFCLK
    nrf_drv_clock_hfclk_request(NULL);

    while (!nrf_drv_clock_hfclk_is_running())
    {
    }

    // Enable UART instance, and start RX on it.
    nrf_uarte_enable(UART_INSTANCE);
    nrf_uarte_task_trigger(UART_INSTANCE, NRF_UARTE_TASK_STARTRX);

    sUartEnabled = true;

exit:
    return error;
}

otError otPlatUartDisable(void)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sUartEnabled == true, error = OT_ERROR_ALREADY);

    // Disable NVIC interrupt.
    NVIC_DisableIRQ(UART_IRQN);
    NVIC_ClearPendingIRQ(UART_IRQN);
    NVIC_SetPriority(UART_IRQN, 0);

    // Disable interrupts for TX.
    nrf_uarte_int_disable(UART_INSTANCE, NRF_UARTE_INT_ENDTX_MASK);

    // Disable interrupts for RX.
    nrf_uarte_int_disable(UART_INSTANCE, NRF_UARTE_INT_RXDRDY_MASK | NRF_UARTE_INT_ERROR_MASK);

    // Disable UART instance.
    nrf_uarte_disable(UART_INSTANCE);

    // Release HF clock.
    nrf_drv_clock_hfclk_release();

    sUartEnabled = false;

exit:
    return error;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sTransmitBuffer == NULL, error = OT_ERROR_BUSY);

    // Set up transmit buffer.
    sTransmitBuffer = aBuf;

    // Initiate transmission process.
    nrf_uarte_event_clear(UART_INSTANCE, NRF_UARTE_EVENT_ENDTX);
    nrf_uarte_tx_buffer_set(UART_INSTANCE, sTransmitBuffer, aBufLength);
    nrf_uarte_task_trigger(UART_INSTANCE, NRF_UARTE_TASK_STARTTX);

exit:
    return error;
}

/**
 * Interrupt handler of UART0 peripherial.
 */
void UARTE0_UART0_IRQHandler(void)
{
    // Check if any error has been detected.
    if (nrf_uarte_int_enable_check(UART_INSTANCE, NRF_UARTE_INT_ERROR_MASK) &&
        nrf_uarte_event_check(UART_INSTANCE, NRF_UARTE_EVENT_ERROR))
    {
        nrf_uarte_event_clear(UART_INSTANCE, NRF_UARTE_EVENT_ERROR);
    }
    else if (nrf_uarte_int_enable_check(UART_INSTANCE, NRF_UARTE_INT_RXDRDY_MASK) &&
             nrf_uarte_event_check(UART_INSTANCE, NRF_UARTE_EVENT_RXDRDY))
    {
        // Clear RXDRDY event.
        nrf_uarte_event_clear(UART_INSTANCE, NRF_UARTE_EVENT_RXDRDY);

        // Read byte from the UART buffer.
        uint8_t byte = nrf_uart_rxd_get((NRF_UART_Type *)UART_INSTANCE);

        assert(!isRxBufferFull());

        sReceiveBuffer[sReceiveHead] = byte;
        sReceiveHead                 = getNextRxBufferIndex();

        // Check if we are able to process next RX bytes.
        if (isRxBufferFull())
        {
            // Disable interrupts for RX.
            nrf_uarte_int_disable(UART_INSTANCE, NRF_UARTE_INT_RXDRDY_MASK | NRF_UARTE_INT_ERROR_MASK);
        }

        otSysEventSignalPending();
    }

    if (nrf_uarte_event_check(UART_INSTANCE, NRF_UARTE_EVENT_ENDTX))
    {
        // Clear ENDTX event.
        nrf_uarte_event_clear(UART_INSTANCE, NRF_UARTE_EVENT_ENDTX);

        sTransmitDone = true;

        nrf_uarte_task_trigger(UART_INSTANCE, NRF_UARTE_TASK_STOPTX);

        otSysEventSignalPending();
    }
}

#endif // UART_AS_SERIAL_TRANSPORT == 1

/**
 * The UART driver weak functions definition.
 *
 */
OT_TOOL_WEAK void otPlatUartSendDone(void)
{
}

OT_TOOL_WEAK void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
    OT_UNUSED_VARIABLE(aBuf);
    OT_UNUSED_VARIABLE(aBufLength);
}
