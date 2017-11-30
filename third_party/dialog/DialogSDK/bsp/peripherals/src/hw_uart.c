/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup UART
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file hw_uart.c
 *
 * @brief Implementation of the UART Low Level Driver.
 *
 * Copyright (c) 2016, Dialog Semiconductor
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 ****************************************************************************************
 */

#if dg_configUSE_HW_UART


#include <stdint.h>
#include <string.h>
#include <core_cm0.h>
#include <hw_uart.h>

#if (dg_configSYSTEMVIEW)
#  include "SEGGER_SYSVIEW_FreeRTOS.h"
#else
#  define SEGGER_SYSTEMVIEW_ISR_ENTER()
#  define SEGGER_SYSTEMVIEW_ISR_EXIT()
#endif

#if (dg_configUART1_SOFTWARE_FIFO_SIZE && dg_configUART_SOFTWARE_FIFO)
#if dg_configUART_RX_CIRCULAR_DMA && (dg_configUART1_RX_CIRCULAR_DMA_BUF_SIZE > 0)
#error UART1 can not be configured to use software FIFO and circular DMA FIFO at the same time
#endif
__RETAINED static uint8_t uart1_sw_fifo[dg_configUART1_SOFTWARE_FIFO_SIZE];
#else
#define uart1_sw_fifo   (NULL)
#endif

#if (dg_configUART2_SOFTWARE_FIFO_SIZE && dg_configUART_SOFTWARE_FIFO)
#if dg_configUART_RX_CIRCULAR_DMA && (dg_configUART2_RX_CIRCULAR_DMA_BUF_SIZE > 0)
#error UART2 can not be configured to use software FIFO and circular DMA FIFO at the same time
#endif
__RETAINED static uint8_t uart2_sw_fifo[dg_configUART2_SOFTWARE_FIFO_SIZE];
#else
#define uart2_sw_fifo   (NULL)
#endif

#if dg_configUART1_SOFTWARE_FIFO_SIZE > 255 || dg_configUART2_SOFTWARE_FIFO_SIZE > 255
typedef uint16_t fifo_size_t;
#else
typedef uint8_t fifo_size_t;
#endif

#if dg_configUART1_SOFTWARE_FIFO_SIZE > 256 || dg_configUART2_SOFTWARE_FIFO_SIZE > 256
typedef uint16_t fifo_ptr_t;
#else
typedef uint8_t fifo_ptr_t;
#endif

#if dg_configUART_RX_CIRCULAR_DMA

#if !HW_UART_USE_DMA_SUPPORT
#       error "dg_configUART_RX_CIRCULAR_DMA requires HW_UART_USE_DMA_SUPPORT to be enabled!"
#endif

#if dg_configUART1_RX_CIRCULAR_DMA_BUF_SIZE > 0
__RETAINED static uint8_t uart1_rx_dma_buf[dg_configUART1_RX_CIRCULAR_DMA_BUF_SIZE];
#else
#define uart1_rx_dma_buf (NULL)
#endif

#if dg_configUART2_RX_CIRCULAR_DMA_BUF_SIZE > 0
__RETAINED static uint8_t uart2_rx_dma_buf[dg_configUART2_RX_CIRCULAR_DMA_BUF_SIZE];
#else
#define uart2_rx_dma_buf (NULL)
#endif

#endif /* dg_configUART_RX_CIRCULAR_DMA */

typedef struct {
#ifdef HW_UART_ENABLE_USER_ISR
        hw_uart_interrupt_isr user_isr;
#endif
        const uint8_t       *tx_buffer;
        void                *tx_user_data;
        hw_uart_tx_callback tx_cb;
        uint16_t            tx_len;
        uint16_t            tx_ix;

        void                *rx_user_data;
        uint8_t             *rx_buffer;
        hw_uart_rx_callback rx_cb;
        uint16_t            rx_len;
        uint16_t            rx_ix;

        uint8_t             tx_fifo_on:1;
        uint8_t             rx_fifo_on:1;
        uint8_t             tx_fifo_level:2;
        uint8_t             rx_fifo_level:2;
#if dg_configUART_SOFTWARE_FIFO
        uint8_t            *rx_soft_fifo;
        fifo_size_t         rx_soft_fifo_size;
        fifo_ptr_t          rx_soft_fifo_rd_ptr;
        fifo_ptr_t          rx_soft_fifo_wr_ptr;
#endif
#if HW_UART_USE_DMA_SUPPORT
        uint8_t             use_dma:1;
        DMA_setup           tx_dma;
        DMA_setup           rx_dma;
#if dg_configUART_RX_CIRCULAR_DMA
        bool                rx_dma_active;
        uint8_t             *rx_dma_buf;
        uint16_t            rx_dma_buf_size;
        uint16_t            rx_dma_head;
#endif /* dg_configUART_RX_CIRCULAR_DMA */
#endif /* HW_UART_USE_DMA_SUPPORT */
} UART_Data;

__RETAINED_RW static UART_Data uart_data[2] = {
        {
#if dg_configUART_SOFTWARE_FIFO
                .rx_soft_fifo = uart1_sw_fifo,
                .rx_soft_fifo_size = dg_configUART1_SOFTWARE_FIFO_SIZE,
#endif
#if dg_configUART_RX_CIRCULAR_DMA
                .rx_dma_buf = uart1_rx_dma_buf,
                .rx_dma_buf_size = dg_configUART1_RX_CIRCULAR_DMA_BUF_SIZE,
#endif
        },
        {
#if dg_configUART_SOFTWARE_FIFO
                .rx_soft_fifo = uart2_sw_fifo,
                .rx_soft_fifo_size = dg_configUART2_SOFTWARE_FIFO_SIZE,
#endif
#if dg_configUART_RX_CIRCULAR_DMA
                .rx_dma_buf = uart2_rx_dma_buf,
                .rx_dma_buf_size = dg_configUART2_RX_CIRCULAR_DMA_BUF_SIZE,
#endif
        },
};

#define UART_INT(id) ((id) == HW_UART1 ? (UART_IRQn) : (UART2_IRQn))
#define UARTIX(id) ((id) == HW_UART1 ? 0 : 1)
#define UARTDATA(id) (&uart_data[UARTIX(id)])
#define UARTID(ud) ((ud) == uart_data ? HW_UART1 : HW_UART2)

#ifdef HW_UART_ENABLE_USER_ISR
void hw_uart_set_isr(HW_UART_ID uart, hw_uart_interrupt_isr isr)
{
        uart_data[UARTIX(uart)].user_isr = isr;
}
#endif

//===================== Read/Write functions ===================================

uint8_t hw_uart_read(HW_UART_ID uart)
{
        // Wait until received data are available
        while (hw_uart_read_buf_empty(uart));

        // Read element from the receive FIFO
        return UBA(uart)->UART2_RBR_THR_DLL_REG;
}

void hw_uart_write(HW_UART_ID uart, uint8_t data)
{
        // Wait if Transmit Holding Register is full
        while (hw_uart_write_buf_full(uart));

        // Write data to the transmit FIFO
        UBA(uart)->UART2_RBR_THR_DLL_REG = data;
}

void hw_uart_write_buffer(HW_UART_ID uart, const void *data, uint16_t len)
{
        const uint8_t *p = data;

        while (len > 0) {
                hw_uart_write(uart, *p++);
                len--;
        }
}

void hw_uart_send(HW_UART_ID uart, const void *data, uint16_t len, hw_uart_tx_callback cb,
                                                                                void *user_data)
{
        UART_Data *ud = &uart_data[UARTIX(uart)];

        if (cb == NULL) {
                hw_uart_write_buffer(uart, data, len);
                ud->tx_ix = 0;
                ud->tx_len = 0;
                return;
        }
        ud->tx_buffer = data;
        ud->tx_user_data = user_data;
        ud->tx_len = len;
        ud->tx_ix = 0;
        ud->tx_cb = cb;

#if HW_UART_USE_DMA_SUPPORT
        if (ud->tx_dma.channel_number != HW_DMA_CHANNEL_INVALID && len > 1) {
                ud->tx_dma.src_address = (uint32) data;
                ud->tx_dma.length = len;
                // DMA requested
                hw_uart_clear_dma_request(uart);
                hw_dma_channel_initialization(&ud->tx_dma);
                hw_dma_channel_enable(ud->tx_dma.channel_number, HW_DMA_STATE_ENABLED);
                return;
        }
#endif
        // Interrupt driven
        NVIC_DisableIRQ(UART_INT(uart));
        // Enable transmit interrupts
        uint16_t ier_dlh_reg = UBA(uart)->UART2_IER_DLH_REG;
        ier_dlh_reg |= ((1 << UART_UART_IER_DLH_REG_ETBEI_dlh1_Pos) | (1 << UART_UART_IER_DLH_REG_PTIME_dlh7_Pos));
        UBA(uart)->UART2_IER_DLH_REG = ier_dlh_reg;

        NVIC_EnableIRQ(UART_INT(uart));
}

static inline void hw_uart_enable_rx_int(HW_UART_ID uart, bool enable)
{
        NVIC_DisableIRQ(UART_INT(uart));
        HW_UART_REG_SETF(uart, IER_DLH, ERBFI_dlh0, enable);
        NVIC_EnableIRQ(UART_INT(uart));
}

#if dg_configUART_SOFTWARE_FIFO

#define SOFTWARE_FIFO_PRESENT(ud) ((ud)->rx_soft_fifo != NULL)

/*
 * Copy bytes from software FIFO to user provided buffer.
 * This function allows software FIFO interrupts to read more data while copy takes place.
 *
 * Function returns true if all requested data is already in user buffer.
 */
static bool hw_uart_drain_rx(HW_UART_ID uart, UART_Data *ud, int len)
{
        /*
         * This function is called with RX interrupt disabled.
         *
         * Get current software FIFO pointers before enabling interrupt again.
         * ud->rx_len is still set to 0, so interrupt will not try to copy data to
         * application buffer till all data from software FIFO is drained to user buffer.
         */
        fifo_ptr_t rd_ptr = ud->rx_soft_fifo_rd_ptr;
        fifo_ptr_t wr_ptr = ud->rx_soft_fifo_wr_ptr;
        int idx = 0;

        /*
         * ud->rx_ix is 0, set ud->rx_len to 0 to prevent interrupt from using it
         * before all data from software FIFO is moved to application buffer.
         */
        ud->rx_len = 0;

        hw_uart_enable_rx_int(uart, true);

        while (idx < len) {

                if (wr_ptr == rd_ptr) {
                        /*
                         * No more data in software FIFO, at least from state before
                         * interrupt was enabled.
                         * Disable interrupt and update read pointer to reflect position
                         * of already copied data.
                         */
                        hw_uart_enable_rx_int(uart, false);
                        ud->rx_soft_fifo_rd_ptr = rd_ptr;

                        /*
                         * Check if write pointer moved, if so interrupt put some more data
                         * in buffer. Remember current write position and try again with
                         * interrupts enabled.
                         */
                        if (ud->rx_soft_fifo_wr_ptr != wr_ptr) {
                                wr_ptr = ud->rx_soft_fifo_wr_ptr;
                                hw_uart_enable_rx_int(uart, true);
                                continue;
                        }

                        /*
                         * All was copied from software FIFO.
                         * Setup user transaction ix and len so when interrupts are enabled
                         * again (not in this function) interrupt or DMA can continue.
                         */
                        ud->rx_ix = idx;
                        ud->rx_len = len;

                        return false;
                }

                /* Copy from software FIFO to user provided buffer */
                ud->rx_buffer[idx++] = ud->rx_soft_fifo[rd_ptr++];

                if (rd_ptr >= ud->rx_soft_fifo_size) {
                        rd_ptr = 0;
                }
        }

        /*
         * User buffer is filled, block interrupt so end of transmission callback will not be
         * called from interrupt.
         */
        hw_uart_enable_rx_int(uart, false);
        ud->rx_soft_fifo_rd_ptr = rd_ptr;
        ud->rx_len = len;
        ud->rx_ix = len;

        return true;
}

void hw_uart_read_buffer(HW_UART_ID uart, void *data, uint16_t len)
{
        UART_Data *ud = UARTDATA(uart);
        uint8_t *p = data;

        /*
         * Disable RX interrupt before draining software FIFO.
         */
        hw_uart_enable_rx_int(uart, false);
        if (SOFTWARE_FIFO_PRESENT(ud)) {
                /*
                 * Drain uses ud fields so setup them accordingly.
                 */
                ud->rx_buffer = data;

                hw_uart_drain_rx(uart, ud, len);
                len -= ud->rx_ix;
                p += ud->rx_ix;
        }
        /*
         * Read all remaining bytes with RX interrupt still disabled.
         */
        while (len > 0) {
                *p++ = hw_uart_read(uart);
                len--;
        }
        ud->rx_ix = 0;
        ud->rx_len = 0;
        hw_uart_enable_rx_int(uart, SOFTWARE_FIFO_PRESENT(ud));
}

void hw_uart_set_soft_fifo(HW_UART_ID uart, uint8_t *buf, uint8_t size)
{
        UART_Data *ud = UARTDATA(uart);

        hw_uart_enable_rx_int(uart, false);

        ud->rx_soft_fifo = buf;
        ud->rx_soft_fifo_size = size;
        ud->rx_soft_fifo_rd_ptr = 0;
        ud->rx_soft_fifo_wr_ptr = 0;

        hw_uart_enable_rx_int(uart, buf != NULL);
}

#else

#define SOFTWARE_FIFO_PRESENT(ud)  false

void hw_uart_read_buffer(HW_UART_ID uart, void *data, uint16_t len)
{
        uint8_t *p = data;

        while (len > 0) {
                *p++ = hw_uart_read(uart);
                len--;
        }
}
#endif

static void hw_uart_fire_callback(UART_Data *ud)
{
        hw_uart_rx_callback cb = ud->rx_cb;
        ud->rx_cb = NULL;
        /* Just finished receiving, disable RX interrupts unless software FIFO is enabled */
        hw_uart_enable_rx_int(UARTID(ud), SOFTWARE_FIFO_PRESENT(ud));
        if (cb) {
                cb(ud->rx_user_data, ud->rx_len);
        }
}

void hw_uart_receive(HW_UART_ID uart, void *data, uint16_t len, hw_uart_rx_callback cb,
                                                                                void *user_data)
{
        UART_Data *ud = UARTDATA(uart);

        if (cb == NULL) {
                hw_uart_read_buffer(uart, data, len);
                ud->rx_ix = 0;
                ud->rx_len = 0;
                return;
        }

        ud->rx_buffer = data;
        ud->rx_user_data = user_data;
        hw_uart_enable_rx_int(uart, false);
        ud->rx_len = len;
        ud->rx_ix = 0;
        ud->rx_cb = cb;
#if dg_configUART_SOFTWARE_FIFO
        if (hw_uart_drain_rx(uart, ud, len)) {
                hw_uart_fire_callback(ud);
                return;
        }
#endif

#if dg_configUART_RX_CIRCULAR_DMA
        if (ud->rx_dma_buf_size > 0) {
                ASSERT_ERROR(len < ud->rx_dma_buf_size);

                uint16_t new_int, cur_idx;
                bool data_ready = false;

                ASSERT_ERROR(ud->rx_dma_active == false);

                /* Calculate index of end of requested data (do not wrap it!) */
                new_int = ud->rx_dma_head + ud->rx_len - 1;

                /* Freeze DMA so it does not move pointers while we try to update them */
                hw_dma_freeze();

                cur_idx = hw_dma_transfered_bytes(ud->rx_dma.channel_number);

                /* cur_idx is lower than rx_head only if it wrapped-around - fix it */
                if (cur_idx < ud->rx_dma_head) {
                        cur_idx += ud->rx_dma_buf_size;
                }

                /*
                 * If DMA has not read past the calculated index, we can set it and just wait for an
                 * interrupt. In other case, data are ready immediately in buffer.
                 */
                if (cur_idx <= new_int) {
                        new_int %= ud->rx_dma_buf_size;
                        hw_dma_channel_update_int_ix(ud->rx_dma.channel_number, new_int);
                        ud->rx_dma_active = true;
                } else {
                        hw_dma_channel_update_int_ix(ud->rx_dma.channel_number, cur_idx - 1);
                        data_ready = true;
                }

                /* Unfreeze DMA now, it can start reading */
                hw_dma_unfreeze();

                /* Fire callback immediately if data already available in buffer */
                if (data_ready) {
                        hw_uart_fire_callback(ud);
                }

                return;
        }
#endif

#if HW_UART_USE_DMA_SUPPORT
        if (ud->rx_dma.channel_number != HW_DMA_CHANNEL_INVALID && (ud->rx_len - ud->rx_ix > 1)) {
                /* rx_ix could already be changed by hw_uart_drain_rx() */
                ud->rx_dma.dest_address = (uint32) data + ud->rx_ix;
                ud->rx_dma.length = ud->rx_len - ud->rx_ix;
                hw_uart_clear_dma_request(uart);
                /* Prepare and start DMA */
                hw_dma_channel_initialization(&ud->rx_dma);
                hw_dma_channel_enable(ud->rx_dma.channel_number, HW_DMA_STATE_ENABLED);
                return;
        }
#endif
        /* Interrupt driven */
        hw_uart_enable_rx_int(uart, true);
}

static void hw_uart_irq_stop_receive(HW_UART_ID uart)
{
        UART_Data *ud = &uart_data[UARTIX(uart)];

        // Disable RX interrupt
        hw_uart_enable_rx_int(uart, false);

        ud->rx_len = ud->rx_ix;

        hw_uart_fire_callback(ud);
}

#if dg_configUART_RX_CIRCULAR_DMA
static void hw_uart_copy_dma_rx_to_user_buffer(HW_UART_ID uart)
{
        UART_Data *ud = &uart_data[UARTIX(uart)];

        uint16_t cur_idx;
        uint16_t to_copy = 0;
        hw_uart_rx_callback cb;

        ud->rx_dma_active = false;
        cb = ud->rx_cb;

        if (cb) {
                ud->rx_cb = NULL;
                /*
                 * User callback was not fired, since rx_dma_active is false it will not
                 * fire even if requested number of bytes was received when abort was initiated.
                 */
                cur_idx = hw_dma_transfered_bytes(ud->rx_dma.channel_number);

                if (ud->rx_ix < ud->rx_len) {
                        if (cur_idx < ud->rx_dma_head) {
                                cur_idx += ud->rx_dma_buf_size;
                        }
                        to_copy = cur_idx - ud->rx_dma_head;
                        if (to_copy >= ud->rx_len - ud->rx_ix) {
                                to_copy = ud->rx_len - ud->rx_ix;
                        }
                }
        } else {
                /*
                 * Callback already fired, circular buffer holds enough data.
                 */
                to_copy = ud->rx_len - ud->rx_ix;
        }

        hw_uart_copy_rx_circular_dma_buffer(uart, ud->rx_buffer + ud->rx_ix, to_copy);
        ud->rx_ix += to_copy;
        ud->rx_len = ud->rx_ix;

        if (cb) {
                cb(ud->rx_user_data, ud->rx_len);
        }
}
#endif

uint16_t hw_uart_abort_receive(HW_UART_ID uart)
{
        UART_Data *ud = &uart_data[UARTIX(uart)];

#if HW_UART_USE_DMA_SUPPORT
        if (ud->rx_dma.channel_number != HW_DMA_CHANNEL_INVALID) {
#if dg_configUART_RX_CIRCULAR_DMA
                if (ud->rx_dma_buf_size > 0) {
                        hw_uart_copy_dma_rx_to_user_buffer(uart);
                        return ud->rx_ix;
                }
#else
                /* No DMA circular buffer, stop DMA */
                hw_dma_channel_stop(ud->rx_dma.channel_number);
#endif
        }
        else
#endif
                hw_uart_irq_stop_receive(uart);

        return ud->rx_ix;
}

uint16_t hw_uart_peek_received(HW_UART_ID uart)
{
        UART_Data *ud = &uart_data[UARTIX(uart)];

#if HW_UART_USE_DMA_SUPPORT
        if (ud->rx_dma.channel_number != HW_DMA_CHANNEL_INVALID)
                ud->rx_ix = hw_dma_transfered_bytes(ud->rx_dma.channel_number);
#endif
        return ud->rx_ix;
}

//============== Interrupt handling ============================================

static inline void hw_uart_tx_isr(HW_UART_ID uart)
{
        UART_Data *ud = UARTDATA(uart);

        while (ud->tx_ix < ud->tx_len) {
                if (ud->tx_fifo_on) {
                        if (!hw_uart_transmit_fifo_not_full(uart)) {
                                break;
                        }
                } else if (!hw_uart_thr_empty_getf(uart)) {
                        break;
                }
                hw_uart_txdata_setf(uart, ud->tx_buffer[ud->tx_ix++]);
        }
        // Everything sent?
        if (ud->tx_ix >= ud->tx_len) {
                hw_uart_tx_callback cb = ud->tx_cb;
                // Disable TX interrupts
                // They can be re-enabled in user callback
                uint16_t ier_dlh_reg = UBA(uart)->UART2_IER_DLH_REG;
                ier_dlh_reg &= ~((1 << UART_UART_IER_DLH_REG_ETBEI_dlh1_Pos) | (1 << UART_UART_IER_DLH_REG_PTIME_dlh7_Pos));
                UBA(uart)->UART2_IER_DLH_REG = ier_dlh_reg;
                ud->tx_cb = NULL;
                if (cb) {
                        cb(ud->tx_user_data, ud->tx_len);
                }
        }
}

static inline void hw_uart_rx_isr(HW_UART_ID uart)
{
        UART_Data *ud = UARTDATA(uart);

        if (SOFTWARE_FIFO_PRESENT(ud)) {
#if dg_configUART_SOFTWARE_FIFO
                for (;;) {
                        fifo_ptr_t wr_ptr = ud->rx_soft_fifo_wr_ptr + 1;
                        if (wr_ptr >= ud->rx_soft_fifo_size) {
                                wr_ptr = 0;
                        }
                        if (wr_ptr == ud->rx_soft_fifo_rd_ptr) {
                                /* Software FIFO full, disable interrupt since no one is reading */
                                hw_uart_enable_rx_int(uart, false);
                                return;
                        }
                        if (!hw_uart_is_data_ready(uart)) {
                                break;
                        }
                        ud->rx_soft_fifo[ud->rx_soft_fifo_wr_ptr] = hw_uart_rxdata_getf(uart);

                        /*
                         * Application read is in progress. Copy data from software FIFO to
                         * user provided buffer.
                         */
                        if (ud->rx_ix < ud->rx_len) {
                                ud->rx_buffer[ud->rx_ix++] = ud->rx_soft_fifo[ud->rx_soft_fifo_wr_ptr];
                                /*
                                 * When application read is in progress, rx_ix < rx_len
                                 * This interrupt is enabled only if all data was already copied from
                                 * FIFO to user buffer. It is safe to modify rx_soft_fifo_rd_ptr.
                                 */
                                ud->rx_soft_fifo_rd_ptr = wr_ptr;
                        }

                        ud->rx_soft_fifo_wr_ptr = wr_ptr;
                }
#endif
        } else {
                while (ud->rx_ix < ud->rx_len) {
                        if (hw_uart_is_data_ready(uart)) {
                                ud->rx_buffer[ud->rx_ix++] = hw_uart_rxdata_getf(uart);
                        } else {
                                break;
                        }
                }
        }

        // Everything read?
        if (ud->rx_len > 0 && ud->rx_ix >= ud->rx_len) {
                // Disable RX interrupts, fire callback if present
                hw_uart_irq_stop_receive(uart);
        }

}

static inline void hw_uart_rx_timeout_isr(HW_UART_ID uart)
{
        UART_Data *ud = UARTDATA(uart);

        hw_uart_rx_isr(uart);

        /*
         * Not everything was received yet, disable interrupt anyway since
         * some data was received.
         */
        if (ud->rx_ix > 0 && ud->rx_ix < ud->rx_len) {
                // Disable RX interrupts fire callback if present
                hw_uart_irq_stop_receive(uart);
        }
}

void UART_Interrupt_Handler(HW_UART_ID uart)
{
        HW_UART_INT int_id;

        for (;;) {
                int_id = hw_uart_get_interrupt_id(uart);
                switch (int_id) {
                case HW_UART_INT_TIMEOUT:
                        hw_uart_rx_timeout_isr(uart);
                        break;
                case HW_UART_INT_MODEM_STAT:
                        break;
                case HW_UART_INT_NO_INT_PEND:
                        return;
                        break;
                case HW_UART_INT_THR_EMPTY:
                        hw_uart_tx_isr(uart);
                        break;
                case HW_UART_INT_RECEIVED_AVAILABLE:
                        hw_uart_rx_isr(uart);
                        break;
                case HW_UART_INT_RECEIVE_LINE_STAT:
                        break;
                case HW_UART_INT_BUSY_DETECTED:
#ifdef CONFIG_UART_IGNORE_BUSY_DETECT
                        hw_uart_transmit_fifo_empty(uart);
#else
                        /*
                         * Stop here means that timing rules for access divisor latch were not
                         * followed. See description of register RBR_THR_DLL.
                         */
                        __BKPT(0);
#endif
                        break;
                }
        }
}

/**
 * \brief HW_UART1 Interrupt Handler
 *
 */
void UART_Handler(void)
{
        SEGGER_SYSTEMVIEW_ISR_ENTER();

#ifdef HW_UART_ENABLE_USER_ISR
        if (uart_data[UARTIX(HW_UART1)].user_isr) {
                uart_data[UARTIX(HW_UART1)].user_isr();
        } else {
#endif
                UART_Interrupt_Handler(HW_UART1);
#ifdef HW_UART_ENABLE_USER_ISR
        }
#endif

        SEGGER_SYSTEMVIEW_ISR_EXIT();
}

/**
 * \brief UART2 Interrupt Handler
 *
 */
void UART2_Handler(void)
{
        SEGGER_SYSTEMVIEW_ISR_ENTER();

#ifdef HW_UART_ENABLE_USER_ISR
        if (uart_data[UARTIX(HW_UART2)].user_isr) {
                uart_data[UARTIX(HW_UART2)].user_isr();
        } else {
#endif
                UART_Interrupt_Handler(HW_UART2);
#ifdef HW_UART_ENABLE_USER_ISR
        }
#endif

        SEGGER_SYSTEMVIEW_ISR_EXIT();
}

//==================== Configuration functions =================================

HW_UART_BAUDRATE hw_uart_baudrate_get(HW_UART_ID uart)
{
        uint32_t baud_rate;

        // Set Divisor Latch Access Bit in LCR register to access DLL & DLH registers
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 1);
        // Read baud rate low byte from DLL register
        baud_rate = (0xFF & UBA(uart)->UART2_RBR_THR_DLL_REG) << 8;
        // Read baud rate high byte from DLH register
        baud_rate += (0xFF & UBA(uart)->UART2_IER_DLH_REG) << 16;
        // Read baud rate fraction byte from DLF register
        baud_rate += 0xFF & UBA(uart)->UART2_DLF_REG;
        // Reset Divisor Latch Access Bit in Line Control Register
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 0);

        return (HW_UART_BAUDRATE) baud_rate;
}

void hw_uart_baudrate_set(HW_UART_ID uart, HW_UART_BAUDRATE baud_rate)
{
        // Set Divisor Latch Access Bit in LCR register to access DLL & DLH registers
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 1);
        // Set fraction byte of baud rate
        UBA(uart)->UART2_DLF_REG = 0xFF & baud_rate;
        // Set low byte of baud rate
        UBA(uart)->UART2_RBR_THR_DLL_REG = 0xFF & (baud_rate >> 8);
        // Set high byte of baud rare
        UBA(uart)->UART2_IER_DLH_REG = 0xFF & (baud_rate >> 16);
        // Reset Divisor Latch Access Bit in LCR register
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 0);
}

//=========================== FIFO control functions ===========================

uint8_t hw_uart_fifo_en_getf(HW_UART_ID uart)
{
        uint16_t fifo_enabled;

        /* Only UART2 has a FIFO */
        ASSERT_ERROR(uart == HW_UART2);

        fifo_enabled = (UBA(uart)->UART2_IIR_FCR_REG & 0x00C0); /* Bits[7:6] */

        switch (fifo_enabled) {
        case 0x00C0:
                return 1;
        case 0x0000:
                return 0;
        default:
                ASSERT_ERROR(0);
                return 255;     // To satisfy the compiler
        }
}

uint8_t hw_uart_tx_fifo_tr_lvl_getf(HW_UART_ID uart)
{
        /* Only UART2 has a FIFO */
        ASSERT_ERROR(uart == HW_UART2);

        return (UART2->UART2_STET_REG & HW_UART_REG_FIELD_MASK(2, STET, UART_SHADOW_TX_EMPTY_TRIGGER))
                >> HW_UART_REG_FIELD_POS(2, STET, UART_SHADOW_TX_EMPTY_TRIGGER);
}

//=========================== DMA control functions ============================

#if HW_UART_USE_DMA_SUPPORT

static void hw_uart_rx_dma_callback(void *user_data, uint16_t len)
{
        UART_Data *ud = user_data;
        hw_uart_rx_callback cb = ud->rx_cb;

        ud->rx_cb = NULL;
        ud->rx_ix += len;
        if (cb) {
                ud->rx_len = ud->rx_ix;
                hw_uart_enable_rx_int(UARTID(ud), SOFTWARE_FIFO_PRESENT(ud));
                cb(ud->rx_user_data, ud->rx_ix);
        }
}

static void hw_uart_tx_dma_callback(void *user_data, uint16_t len)
{
        UART_Data *ud = user_data;
        hw_uart_tx_callback cb = ud->tx_cb;

        ud->tx_cb = NULL;
        ud->tx_ix = len;
        if (cb) {
                cb(ud->tx_user_data, len);
        }
}

void hw_uart_set_dma_channels(HW_UART_ID uart, int8_t channel, HW_DMA_PRIO pri)
{
        UART_Data *ud = UARTDATA(uart);

        /* Only specific DMA channels (or -1 for no DMA) are allowed */
        ASSERT_ERROR(channel < 0 ||
                channel == HW_DMA_CHANNEL_0 ||
                channel == HW_DMA_CHANNEL_2 ||
                channel == HW_DMA_CHANNEL_4 ||
                channel == HW_DMA_CHANNEL_6 ||
                channel == HW_DMA_CHANNEL_INVALID);

        if (channel < 0) {
                ud->use_dma = 0;
                ud->rx_dma.channel_number = HW_DMA_CHANNEL_INVALID;
                ud->tx_dma.channel_number = HW_DMA_CHANNEL_INVALID;
        } else {
                ud->use_dma = 1;

                ud->rx_dma.channel_number = channel;
                ud->rx_dma.bus_width = HW_DMA_BW_BYTE;
                ud->rx_dma.irq_enable = HW_DMA_IRQ_STATE_ENABLED;
                ud->rx_dma.dma_req_mux = UARTIX(uart) == 0 ? HW_DMA_TRIG_UART_RXTX :
                                                                        HW_DMA_TRIG_UART2_RXTX;
                ud->rx_dma.irq_nr_of_trans = 0;
                ud->rx_dma.a_inc = HW_DMA_AINC_FALSE;
                ud->rx_dma.b_inc = HW_DMA_BINC_TRUE;
                ud->rx_dma.circular = HW_DMA_MODE_NORMAL;
                ud->rx_dma.dma_prio = pri;
                ud->rx_dma.dma_idle = HW_DMA_IDLE_INTERRUPTING_MODE; /* Not used by the HW in this case */
                ud->rx_dma.dma_init = HW_DMA_INIT_AX_BX_AY_BY;
                ud->rx_dma.dreq_mode = HW_DMA_DREQ_TRIGGERED;
                ud->rx_dma.src_address = (uint32_t) &UBA(uart)->UART2_RBR_THR_DLL_REG;
                ud->rx_dma.dest_address = 0;  // Change during transmission
                ud->rx_dma.length = 0; // Change during transmission
                ud->rx_dma.callback = hw_uart_rx_dma_callback;
                ud->rx_dma.user_data = ud;

                ud->tx_dma.channel_number = channel + 1;
                ud->tx_dma.bus_width = HW_DMA_BW_BYTE;
                ud->tx_dma.irq_enable = HW_DMA_IRQ_STATE_ENABLED;
                ud->tx_dma.dma_req_mux = UARTIX(uart) == 0 ? HW_DMA_TRIG_UART_RXTX :
                                                                        HW_DMA_TRIG_UART2_RXTX;
                ud->tx_dma.irq_nr_of_trans = 0;
                ud->tx_dma.a_inc = HW_DMA_AINC_TRUE;
                ud->tx_dma.b_inc = HW_DMA_BINC_FALSE;
                ud->tx_dma.circular = HW_DMA_MODE_NORMAL;
                ud->tx_dma.dma_prio = pri;
                ud->tx_dma.dma_idle = HW_DMA_IDLE_INTERRUPTING_MODE; /* Not used by the HW in this case */
                ud->tx_dma.dma_init = HW_DMA_INIT_AX_BX_AY_BY;
                ud->tx_dma.dreq_mode = HW_DMA_DREQ_TRIGGERED;
                ud->tx_dma.src_address = 0; // Change during transmission
                ud->tx_dma.dest_address = (uint32_t) &UBA(uart)->UART2_RBR_THR_DLL_REG;
                ud->tx_dma.length = 0; // Change during transmission
                ud->tx_dma.callback = hw_uart_tx_dma_callback;
                ud->tx_dma.user_data = ud;
        }
}

void hw_uart_set_dma_channels_ex(HW_UART_ID uart, int8_t tx_channel, int8_t rx_channel, HW_DMA_PRIO pri)
{
        UART_Data *ud = UARTDATA(uart);

        /* Only specific DMA channels are allowed (or HW_DMA_CHANNEL_INVALID for no DMA) */
        ASSERT_ERROR(tx_channel >= HW_DMA_CHANNEL_0 &&
                        tx_channel <= HW_DMA_CHANNEL_INVALID);

        /* Only specific DMA channels are allowed (or HW_DMA_CHANNEL_INVALID for no DMA) */
        ASSERT_ERROR(rx_channel >= HW_DMA_CHANNEL_0 &&
                        rx_channel <= HW_DMA_CHANNEL_INVALID);

        if (tx_channel == HW_DMA_CHANNEL_INVALID && rx_channel== HW_DMA_CHANNEL_INVALID) {
                ud->use_dma = 0;
                ud->rx_dma.channel_number = HW_DMA_CHANNEL_INVALID;
                ud->tx_dma.channel_number = HW_DMA_CHANNEL_INVALID;
        } else {
                if (tx_channel != HW_DMA_CHANNEL_INVALID && rx_channel!= HW_DMA_CHANNEL_INVALID) {//not both invalid
                        ASSERT_ERROR(tx_channel != rx_channel);//not equal
                        ASSERT_ERROR(tx_channel>>1 == rx_channel>>1);//on same pair
                }
                if (tx_channel != HW_DMA_CHANNEL_INVALID) {
                        ASSERT_ERROR(tx_channel & 1);//odd number
                }
                if (rx_channel != HW_DMA_CHANNEL_INVALID) {
                        ASSERT_ERROR((rx_channel & 1) == 0);//odd number
                }

                ud->use_dma = 1;

                ud->rx_dma.channel_number = rx_channel;
                ud->rx_dma.bus_width = HW_DMA_BW_BYTE;
                ud->rx_dma.irq_enable = HW_DMA_IRQ_STATE_ENABLED;
                ud->rx_dma.dma_req_mux = UARTIX(uart) == 0 ? HW_DMA_TRIG_UART_RXTX :
                                                                        HW_DMA_TRIG_UART2_RXTX;
                ud->rx_dma.irq_nr_of_trans = 0;
                ud->rx_dma.a_inc = HW_DMA_AINC_FALSE;
                ud->rx_dma.b_inc = HW_DMA_BINC_TRUE;
                ud->rx_dma.circular = HW_DMA_MODE_NORMAL;
                ud->rx_dma.dma_prio = pri;
                ud->rx_dma.dma_idle = HW_DMA_IDLE_INTERRUPTING_MODE; /* Not used by the HW in this case */
                ud->rx_dma.dma_init = HW_DMA_INIT_AX_BX_AY_BY;
                ud->rx_dma.dreq_mode = HW_DMA_DREQ_TRIGGERED;
                ud->rx_dma.src_address = (uint32_t) &UBA(uart)->UART2_RBR_THR_DLL_REG;
                ud->rx_dma.dest_address = 0;  // Change during transmission
                ud->rx_dma.length = 0; // Change during transmission
                ud->rx_dma.callback = hw_uart_rx_dma_callback;
                ud->rx_dma.user_data = ud;

                ud->tx_dma.channel_number = tx_channel;
                ud->tx_dma.bus_width = HW_DMA_BW_BYTE;
                ud->tx_dma.irq_enable = HW_DMA_IRQ_STATE_ENABLED;
                ud->tx_dma.dma_req_mux = UARTIX(uart) == 0 ? HW_DMA_TRIG_UART_RXTX :
                                                                        HW_DMA_TRIG_UART2_RXTX;
                ud->tx_dma.irq_nr_of_trans = 0;
                ud->tx_dma.a_inc = HW_DMA_AINC_TRUE;
                ud->tx_dma.b_inc = HW_DMA_BINC_FALSE;
                ud->tx_dma.circular = HW_DMA_MODE_NORMAL;
                ud->tx_dma.dma_prio = pri;
                ud->tx_dma.dma_idle = HW_DMA_IDLE_INTERRUPTING_MODE; /* Not used by the HW in this case */
                ud->tx_dma.dma_init = HW_DMA_INIT_AX_BX_AY_BY;
                ud->tx_dma.dreq_mode = HW_DMA_DREQ_TRIGGERED;
                ud->tx_dma.src_address = 0; // Change during transmission
                ud->tx_dma.dest_address = (uint32_t) &UBA(uart)->UART2_RBR_THR_DLL_REG;
                ud->tx_dma.length = 0; // Change during transmission
                ud->tx_dma.callback = hw_uart_tx_dma_callback;
                ud->tx_dma.user_data = ud;
        }
}

#if dg_configUART_RX_CIRCULAR_DMA

static void hw_uart_rx_circular_dma_callback(void *user_data, uint16_t len)
{
        UART_Data *ud = user_data;
        hw_uart_rx_callback cb = ud->rx_cb;

        if (!ud->rx_dma_active) {
                return;
        }

        ud->rx_cb = NULL;
        ud->rx_dma_active = false;

        if (cb) {
                cb(ud->rx_user_data, ud->rx_len);
        }
}

void hw_uart_enable_rx_circular_dma(HW_UART_ID uart)
{
        UART_Data *ud = UARTDATA(uart);

        ASSERT_ERROR(ud->rx_dma_buf_size > 0);

        hw_dma_channel_enable(ud->rx_dma.channel_number, HW_DMA_STATE_DISABLED);

        /* Need to reconfigure few things for circular operation... */
        ud->rx_dma.circular = HW_DMA_MODE_CIRCULAR;
        ud->rx_dma.dest_address = (uint32_t) ud->rx_dma_buf;
        ud->rx_dma.length = ud->rx_dma_buf_size;
        ud->rx_dma.callback = hw_uart_rx_circular_dma_callback;
        ud->rx_dma.user_data = ud;

        /* Reset DMA buffer read pointer */
        ud->rx_dma_head = 0;

        /* Start DMA now since it should be always-running */
        hw_uart_clear_dma_request(uart);
        hw_dma_channel_initialization(&ud->rx_dma);
        hw_dma_channel_enable(ud->rx_dma.channel_number, HW_DMA_STATE_ENABLED);
}

void hw_uart_copy_rx_circular_dma_buffer(HW_UART_ID uart, uint8_t *buf, uint16_t len)
{
        UART_Data *ud = UARTDATA(uart);

        ASSERT_ERROR(len < ud->rx_dma_buf_size);

        if (ud->rx_dma_head + len <= ud->rx_dma_buf_size) {
                memcpy(buf, &ud->rx_dma_buf[ud->rx_dma_head], len);
        } else {
                uint16_t chunk_len = ud->rx_dma_buf_size - ud->rx_dma_head;
                memcpy(buf, &ud->rx_dma_buf[ud->rx_dma_head], chunk_len);
                memcpy(buf + chunk_len, &ud->rx_dma_buf[0], len - chunk_len);
        }

        /* This should be protected so ISR does not try to read rx_dma_head while we update it */
        GLOBAL_INT_DISABLE();
        ud->rx_dma_head = (ud->rx_dma_head + len) % ud->rx_dma_buf_size;
        GLOBAL_INT_RESTORE();
}

#endif /* dg_configUART_RX_CIRCULAR_DMA */

#endif /* HW_UART_USE_DMA_SUPPORT */

//=========================== Line control functions ============================
void hw_uart_init_ex(HW_UART_ID uart, const uart_config_ex *uart_init)
{
        UART_Data *ud = UARTDATA(uart);

        /*
         * Read UART_USR_REG to clear any pending busy interrupt.
         */
        hw_uart_transmit_fifo_empty(uart);


        if (uart == HW_UART1) {
                // there is no FIFO for UART0 as yet
                ud->rx_fifo_on = 0;
                ud->tx_fifo_on = 0;
                hw_uart_disable_fifo(uart);
        } else {
                if (uart_init->use_fifo) {
                        ud->rx_fifo_on = 1;
                        ud->tx_fifo_on = 1;
                        hw_uart_enable_fifo(uart);
                        ud->rx_fifo_level = uart_init->rx_fifo_tr_lvl;
                        hw_uart_rx_fifo_tr_lvl_setf(uart, uart_init->rx_fifo_tr_lvl);
                        ud->tx_fifo_level = uart_init->tx_fifo_tr_lvl;
                        hw_uart_tx_fifo_tr_lvl_setf(uart, uart_init->tx_fifo_tr_lvl);
                } else {
                        ud->rx_fifo_on = 0;
                        ud->tx_fifo_on = 0;
                        hw_uart_disable_fifo(uart);
                }
        }
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_PER, CLK_PER_REG, UART_ENABLE);
        GLOBAL_INT_RESTORE();

        // Set Divisor Latch Access Bit in LCR register to access DLL & DLH registers
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 1);
        // Set fraction byte of baud rate
        UBA(uart)->UART2_DLF_REG = 0xFF & uart_init->baud_rate;
        // Set low byte of baud rate
        UBA(uart)->UART2_RBR_THR_DLL_REG = 0xFF & (uart_init->baud_rate >> 8);
        // Set high byte of baud rate
        UBA(uart)->UART2_IER_DLH_REG = 0xFF & (uart_init->baud_rate >> 16);
        // Reset Divisor Latch Access Bit in LCR register
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 0);
        // Set Parity
        UBA(uart)->UART2_LCR_REG = (uart_init->parity) << 3;
        // Set Data Bits
        HW_UART_REG_SETF(uart, LCR, UART_DLS, uart_init->data);
        // Set Stop Bits
        HW_UART_REG_SETF(uart, LCR, UART_STOP, uart_init->stop);
        // Set Auto flow control
        HW_UART_REG_SETF(uart, MCR, UART_AFCE, uart_init->auto_flow_control);
        HW_UART_REG_SETF(uart, MCR, UART_RTS, uart_init->auto_flow_control);
        ud->tx_cb = NULL;
        ud->rx_cb = NULL;
        ud->rx_len = 0;
        ud->tx_len = 0;
        ud->use_dma = 0;
        ud->rx_dma.channel_number = HW_DMA_CHANNEL_INVALID;
        ud->tx_dma.channel_number = HW_DMA_CHANNEL_INVALID;
#if HW_UART_USE_DMA_SUPPORT
        if (uart_init->use_dma) {
                hw_uart_set_dma_channels_ex(uart, uart_init->tx_dma_channel, uart_init->rx_dma_channel, HW_DMA_PRIO_2);
        }
#endif
}

void hw_uart_reinit_ex(HW_UART_ID uart, const uart_config_ex *uart_init)
{
        UART_Data *ud = UARTDATA(uart);

        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_PER, CLK_PER_REG, UART_ENABLE);
        GLOBAL_INT_RESTORE();

        /*
         * Read UART_USR_REG to clear any pending busy interrupt.
         */
        hw_uart_transmit_fifo_empty(uart);

        if (uart == HW_UART2) {
                if (uart_init->use_fifo) {
                        hw_uart_enable_fifo(uart);
                        hw_uart_rx_fifo_tr_lvl_setf(uart, uart_init->rx_fifo_tr_lvl);
                        hw_uart_tx_fifo_tr_lvl_setf(uart, uart_init->tx_fifo_tr_lvl);
                } else {
                        hw_uart_disable_fifo(uart);
                }
        }

        // Set Divisor Latch Access Bit in LCR register to access DLL & DLH registers
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 1);
        // Set fraction byte of baud rate
        UBA(uart)->UART2_DLF_REG = 0xFF & uart_init->baud_rate;
        // Set low byte of baud rate
        UBA(uart)->UART2_RBR_THR_DLL_REG = 0xFF & (uart_init->baud_rate >> 8);
        // Set high byte of baud rare
        UBA(uart)->UART2_IER_DLH_REG = 0xFF & (uart_init->baud_rate >> 16);
        // Reset Divisor Latch Access Bit in LCR register
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 0);
        // Set Parity
        UBA(uart)->UART2_LCR_REG = (uart_init->parity) << 3;
        // Set Data Bits
        HW_UART_REG_SETF(uart, LCR, UART_DLS, uart_init->data);
        // Set Stop Bits
        HW_UART_REG_SETF(uart, LCR, UART_STOP, uart_init->stop);
        // Set Auto flow control
        HW_UART_REG_SETF(uart, MCR, UART_AFCE, uart_init->auto_flow_control);
        HW_UART_REG_SETF(uart, MCR, UART_RTS, uart_init->auto_flow_control);

        if (ud->rx_cb && ud->rx_len != ud->rx_ix) {
                if (ud->rx_len > 1 && uart_init->use_dma && uart_init->rx_dma_channel != HW_DMA_CHANNEL_INVALID) {
                } else {
                        // Interrupt driven
                        hw_uart_enable_rx_int(uart, true);
                }
        }
}

void hw_uart_init(HW_UART_ID uart, const uart_config *uart_init)
{
        UART_Data *ud = UARTDATA(uart);

        /*
         * Read UART_USR_REG to clear any pending busy interrupt.
         */
        hw_uart_transmit_fifo_empty(uart);


        if (uart == HW_UART1) {
                // there is no FIFO for UART0 as yet
                ud->rx_fifo_on = 0;
                ud->tx_fifo_on = 0;
                hw_uart_disable_fifo(uart);
        } else {
                if (uart_init->use_fifo) {
                        ud->rx_fifo_on = 1;
                        ud->tx_fifo_on = 1;
                        hw_uart_enable_fifo(uart);
                        hw_uart_rx_fifo_tr_lvl_setf(uart, 0);
                        hw_uart_tx_fifo_tr_lvl_setf(uart, 0);
                } else {
                        ud->rx_fifo_on = 0;
                        ud->tx_fifo_on = 0;
                        hw_uart_disable_fifo(uart);
                }
        }
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_PER, CLK_PER_REG, UART_ENABLE);
        GLOBAL_INT_RESTORE();

        // Set Divisor Latch Access Bit in LCR register to access DLL & DLH registers
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 1);
        // Set fraction byte of baud rate
        UBA(uart)->UART2_DLF_REG = 0xFF & uart_init->baud_rate;
        // Set low byte of baud rate
        UBA(uart)->UART2_RBR_THR_DLL_REG = 0xFF & (uart_init->baud_rate >> 8);
        // Set high byte of baud rate
        UBA(uart)->UART2_IER_DLH_REG = 0xFF & (uart_init->baud_rate >> 16);
        // Reset Divisor Latch Access Bit in LCR register
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 0);
        // Set Parity
        UBA(uart)->UART2_LCR_REG = (uart_init->parity) << 3;
        // Set Data Bits
        HW_UART_REG_SETF(uart, LCR, UART_DLS, uart_init->data);
        // Set Stop Bits
        HW_UART_REG_SETF(uart, LCR, UART_STOP, uart_init->stop);
        // Set Auto flow control
        HW_UART_REG_SETF(uart, MCR, UART_AFCE, uart_init->auto_flow_control);
        HW_UART_REG_SETF(uart, MCR, UART_RTS, uart_init->auto_flow_control);
        ud->tx_cb = NULL;
        ud->rx_cb = NULL;
        ud->rx_len = 0;
        ud->tx_len = 0;
        ud->use_dma = 0;
        ud->rx_dma.channel_number = HW_DMA_CHANNEL_INVALID;
        ud->tx_dma.channel_number = HW_DMA_CHANNEL_INVALID;
#if HW_UART_USE_DMA_SUPPORT
        if (uart_init->use_dma) {
                hw_uart_set_dma_channels_ex(uart, uart_init->tx_dma_channel, uart_init->rx_dma_channel, HW_DMA_PRIO_2);
        }
#endif
}

void hw_uart_reinit(HW_UART_ID uart, const uart_config *uart_init)
{
        UART_Data *ud = UARTDATA(uart);

        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_PER, CLK_PER_REG, UART_ENABLE);
        GLOBAL_INT_RESTORE();

        /*
         * Read UART_USR_REG to clear any pending busy interrupt.
         */
        hw_uart_transmit_fifo_empty(uart);

        if (uart == HW_UART2) {
                if (uart_init->use_fifo) {
                        hw_uart_enable_fifo(uart);
                        hw_uart_rx_fifo_tr_lvl_setf(uart, 0);
                        hw_uart_tx_fifo_tr_lvl_setf(uart, 0);
                } else {
                        hw_uart_disable_fifo(uart);
                }
        }

        // Set Divisor Latch Access Bit in LCR register to access DLL & DLH registers
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 1);
        // Set fraction byte of baud rate
        UBA(uart)->UART2_DLF_REG = 0xFF & uart_init->baud_rate;
        // Set low byte of baud rate
        UBA(uart)->UART2_RBR_THR_DLL_REG = 0xFF & (uart_init->baud_rate >> 8);
        // Set high byte of baud rare
        UBA(uart)->UART2_IER_DLH_REG = 0xFF & (uart_init->baud_rate >> 16);
        // Reset Divisor Latch Access Bit in LCR register
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 0);
        // Set Parity
        UBA(uart)->UART2_LCR_REG = (uart_init->parity) << 3;
        // Set Data Bits
        HW_UART_REG_SETF(uart, LCR, UART_DLS, uart_init->data);
        // Set Stop Bits
        HW_UART_REG_SETF(uart, LCR, UART_STOP, uart_init->stop);
        // Set Auto flow control
        HW_UART_REG_SETF(uart, MCR, UART_AFCE, uart_init->auto_flow_control);
        HW_UART_REG_SETF(uart, MCR, UART_RTS, uart_init->auto_flow_control);

        if (ud->rx_cb && ud->rx_len != ud->rx_ix) {
                if (ud->rx_len > 1 && uart_init->use_dma && uart_init->rx_dma_channel != HW_DMA_CHANNEL_INVALID) {
                } else {
                        // Interrupt driven
                        hw_uart_enable_rx_int(uart, true);
                }
        }
}

void hw_uart_cfg_get(HW_UART_ID uart, uart_config *uart_cfg)
{
#if HW_UART_USE_DMA_SUPPORT
        UART_Data *ud = UARTDATA(uart);
#endif

        // Set Divisor Latch Access Bit in LCR register to access DLL & DLH registers
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 1);
        // Read baud rate low byte from DLL register
        uart_cfg->baud_rate = (0xFF & UBA(uart)->UART2_RBR_THR_DLL_REG) << 8;
        // Read baud rate high byte from DLH register
        uart_cfg->baud_rate += (0xFF & UBA(uart)->UART2_IER_DLH_REG) << 16;
        // Read baud rate fraction byte from DLF register
        uart_cfg->baud_rate += 0xFF & UBA(uart)->UART2_DLF_REG;
        // Reset Divisor Latch Access Bit in Line Control Register
        HW_UART_REG_SETF(uart, LCR, UART_DLAB, 0);

        // Fill-in the rest of the configuration settings
        uart_cfg->data = HW_UART_REG_GETF(uart, LCR, UART_DLS);
        uart_cfg->parity = UBA(uart)->UART2_LCR_REG;
        uart_cfg->parity &= ((1 << UART_UART_LCR_REG_UART_EPS_Pos) | (1 << UART_UART_LCR_REG_UART_PEN_Pos));
        uart_cfg->parity = uart_cfg->parity >> UART_UART_LCR_REG_UART_PEN_Pos;
        uart_cfg->stop = HW_UART_REG_GETF(uart, LCR, UART_STOP);
#if HW_UART_USE_DMA_SUPPORT
        uart_cfg->tx_dma_channel = ud->tx_dma.channel_number;
        uart_cfg->rx_dma_channel = ud->rx_dma.channel_number;
        uart_cfg->use_dma = ud->use_dma;
#endif
        uart_cfg->auto_flow_control = hw_uart_afce_getf(uart);
}

//=========================== Modem control functions ==========================

uint8_t hw_uart_sire_getf(HW_UART_ID uart)
{
        // Get the value of the SIRE bit from the Modem Control Register
        return (uint8_t) HW_UART_REG_GETF(uart, MCR, UART_SIRE);
}

void hw_uart_sire_setf(HW_UART_ID uart, uint8_t sire)
{
        // Set the value of the SIRE bit in the Modem Control Register
        HW_UART_REG_SETF(uart, MCR, UART_SIRE,  sire);
}

uint8_t hw_uart_afce_getf(HW_UART_ID uart)
{
        // Get the value of the AFCE bit from the Modem Control Register
        return 0xFF & HW_UART_REG_GETF(uart, MCR, UART_AFCE);
}

void hw_uart_afce_setf(HW_UART_ID uart, uint8_t afce)
{
        // Set the value of the AFCE bit in the Modem Control Register
        HW_UART_REG_SETF(uart, MCR, UART_AFCE, afce);
}

uint8_t hw_uart_loopback_getf(HW_UART_ID uart)
{
        // Get the value of the loop back (LB) bit from the Modem Control Register
        return (uint8_t) HW_UART_REG_GETF(uart, MCR, UART_LB);
}

void hw_uart_loopback_setf(HW_UART_ID uart, uint8_t lb)
{
        // Set the value of the loop back (LB) bit in the Modem Control Register
        HW_UART_REG_SETF(uart, MCR, UART_LB, lb);
}

uint8_t hw_uart_rts_getf(HW_UART_ID uart)
{
        // Get the value of the RTS bit from the Modem Control Register
        return 0xFF & HW_UART_REG_GETF(uart, MCR, UART_RTS);
}

void hw_uart_rts_setf(HW_UART_ID uart, uint8_t rtsn)
{
        // Set the value of the RTS bit in the Modem Control Register
        HW_UART_REG_SETF(uart, MCR, UART_RTS, rtsn);
}

//=========================== Line status functions ============================

uint8_t hw_uart_rx_fifo_err_getf(HW_UART_ID uart)
{
        /* Only UART2 has a FIFO */
        ASSERT_ERROR(uart == HW_UART2);
        // Get Receiver FIFO Error bit
        return (uint8_t) HW_UART_REG_GETF(uart, LSR, UART_RFE);
}

uint8_t hw_uart_is_tx_fifo_empty(HW_UART_ID uart)
{
        // Get Transmitter Empty bit from Line Status Register
        return HW_UART_REG_GETF(uart, LSR, UART_TEMT) != 0;
}

uint8_t hw_uart_thr_empty_getf(HW_UART_ID uart)
{
        // Get Transmit Holding Register Empty bit value from Line Status Register
        return HW_UART_REG_GETF(uart, LSR, UART_THRE);
}

uint8_t hw_uart_break_int_getf(HW_UART_ID uart)
{
        // Get Break Interrupt bit value from Line Status Register
        return HW_UART_REG_GETF(uart, LSR, UART_BI);
}

uint8_t hw_uart_frame_err_getf(HW_UART_ID uart)
{
        // Get Framing Error bit value from Line Status Register
        return HW_UART_REG_GETF(uart, LSR, UART_FE);
}

uint8_t hw_uart_parity_err_getf(HW_UART_ID uart)
{
        // Get Parity Error bit value from Line Status Register
        return HW_UART_REG_GETF(uart, LSR, UART_PE);
}

uint8_t hw_uart_overrun_err_getf(HW_UART_ID uart)
{
        // Get Overrun Error bit value from Line Status Register
        return HW_UART_REG_GETF(uart, LSR, UART_OE);
}

//=========================== Modem status functions ===========================

uint8_t hw_uart_cts_getf(HW_UART_ID uart)
{
        // Get CTS bit from Modem Control Register
        return (uint8_t) HW_UART_REG_GETF(uart, MSR, UART_CTS);
}

uint8_t hw_uart_delta_cts_getf(HW_UART_ID uart)
{
        // Get the DCTS bit value from the Modem Control Register
        return (uint8_t) HW_UART_REG_GETF(uart, MSR, UART_DCTS);
}

bool hw_uart_tx_in_progress(HW_UART_ID uart)
{
        UART_Data *ud = UARTDATA(uart);
        return ud->tx_cb != NULL;
}

bool hw_uart_rx_in_progress(HW_UART_ID uart)
{
        UART_Data *ud = UARTDATA(uart);
        return ud->rx_cb != NULL;
}

#endif /* dg_configUSE_HW_UART */
/**
 * \}
 * \}
 * \}
 */
