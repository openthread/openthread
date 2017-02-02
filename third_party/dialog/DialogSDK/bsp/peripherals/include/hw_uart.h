/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup UART
 * \{
 * \brief UART Controller
 */

/**
 *****************************************************************************************
 *
 * @file hw_uart.h
 *
 * @brief Definition of API for the UART Low Level Driver.
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
 *****************************************************************************************
 */

#ifndef HW_UART_H_
#define HW_UART_H_

#if dg_configUSE_HW_UART

#include <stdbool.h>
#include <stdint.h>
#include <sdk_defs.h>
#include "hw_dma.h"

#ifndef HW_UART_USE_DMA_SUPPORT
#  define HW_UART_USE_DMA_SUPPORT	1
#endif

#define UBA(id)         ((UART2_Type *)id)
#define HW_UART1        ((uint16_t *)UART_BASE)
#define HW_UART2        ((uint16_t *)UART2_BASE)
typedef uint16_t * HW_UART_ID;


/**
 * \brief Get the mask of a field of a UART register.
 *
 * \param [in] instance identifies the UART instance to use (empty: UART, 2: UART2)
 * \param [in] reg is the register to access
 * \param [in] field is the register field to access
 *
 * \note The stripped register name should be provided, e.g.:
 *        - to get UART_UART_USR_REG_UART_BUSY_Msk, use
 *              HW_UART_REG_FIELD_MASK(, USR, UART_BUSY)
 *        - to get UART2_UART2_SFE_REG_UART_SHADOW_FIFO_ENABLE_Msk, use
 *              HW_UART_REG_FIELD_MASK(2, SFE, UART_SHADOW_FIFO_ENABLE)
 *
 */
#define HW_UART_REG_FIELD_MASK(instance, reg, field) \
        (UART##instance##_UART##instance##_##reg##_REG_##field##_Msk)

/**
 * \brief Get the bit position of a field of a UART register.
 *
 * \param [in] instance identifies the UART instance to use (empty: UART, 2: UART2)
 * \param [in] reg is the register to access
 * \param [in] field is the register field to access
 *
 * \note The stripped register name should be provided, e.g.:
 *        - to get UART_UART_USR_REG_UART_BUSY_Pos, use
 *              HW_UART_REG_FIELD_POS(, USR, UART_BUSY)
 *        - to get UART2_UART2_SFE_REG_UART_SHADOW_FIFO_ENABLE_Pos, use
 *              HW_UART_REG_FIELD_POS(2, SFE, UART_SHADOW_FIFO_ENABLE)
 *
 */
#define HW_UART_REG_FIELD_POS(instance, reg, field) \
        (UART##instance##_UART##instance##_##reg##_REG_##field##_Pos)

/**
 * \brief Get the value of a field of a UART register.
 *
 * \param [in] id identifies UART to use
 * \param [in] reg is the register to access
 * \param [in] field is the register field to write
 *
 * \return the value of the register field
 *
 */
#define HW_UART_REG_GETF(id, reg, field) \
                ((UBA(id)->UART2_##reg##_REG & (UART2_UART2_##reg##_REG_##field##_Msk)) >> (UART2_UART2_##reg##_REG_##field##_Pos))

/**
 * \brief Set the value of a field of a UART register.
 *
 * \param [in] id identifies UART to use
 * \param [in] reg is the register to access
 * \param [in] field is the register field to write
 * \param [in] new_val is the value to write
 *
 */
#define HW_UART_REG_SETF(id, reg, field, new_val) \
                UBA(id)->UART2_##reg##_REG = ((UBA(id)->UART2_##reg##_REG & ~(UART2_UART2_##reg##_REG_##field##_Msk)) | \
                ((UART2_UART2_##reg##_REG_##field##_Msk) & ((new_val) << (UART2_UART2_##reg##_REG_##field##_Pos))))



/*
 * ENUMERATION DEFINITIONS
 *****************************************************************************************
 */

/**
 * \brief Interrupt Identification codes
 *
 */
typedef enum {
        HW_UART_INT_MODEM_STAT         = 0,
        HW_UART_INT_NO_INT_PEND        = 1,
        HW_UART_INT_THR_EMPTY          = 2,
        HW_UART_INT_RECEIVED_AVAILABLE = 4,
        HW_UART_INT_RECEIVE_LINE_STAT  = 6,
        HW_UART_INT_BUSY_DETECTED      = 7,
        HW_UART_INT_TIMEOUT            = 12,
} HW_UART_INT;

/**
 * \brief Baud rates dividers
 *
 * The defined values comprise the values of 3 registers: DLH, DLL, DLF.
 * The encoding of the values for each register is:
 *
 * +--------+--------+--------+--------+
 * | unused |   DLH  |   DLL  |   DLF  |
 * +--------+--------+--------+--------+
 *
 */
typedef enum {
        HW_UART_BAUDRATE_1000000   = 0x00000100,
        HW_UART_BAUDRATE_500000    = 0x00000200,
        HW_UART_BAUDRATE_230400    = 0x00000405,
        HW_UART_BAUDRATE_115200    = 0x0000080b,
        HW_UART_BAUDRATE_57600     = 0x00001106,
        HW_UART_BAUDRATE_38400     = 0x00001a01,
        HW_UART_BAUDRATE_28800     = 0x0000220c,
        HW_UART_BAUDRATE_19200     = 0x00003401,
        HW_UART_BAUDRATE_14400     = 0x00004507,
        HW_UART_BAUDRATE_9600      = 0x00006803,
        HW_UART_BAUDRATE_4800      = 0x0000d005,
} HW_UART_BAUDRATE;

/**
 * \brief Character format
 *
 */
typedef enum {
        HW_UART_DATABITS_5        = 0,
        HW_UART_DATABITS_6        = 1,
        HW_UART_DATABITS_7        = 2,
        HW_UART_DATABITS_8        = 3,
} HW_UART_DATABITS;

/**
 * \brief Parity
 *
 */
typedef enum {
        HW_UART_PARITY_NONE     = 0,
        HW_UART_PARITY_ODD      = 1,
        HW_UART_PARITY_EVEN     = 3,
} HW_UART_PARITY;

/**
 * \brief Stop bits
 *
 */
typedef enum {
        HW_UART_STOPBITS_1 = 0,
        /**< The number of stop bits is 1.5 if a character format with 5 bit is chosen */
        HW_UART_STOPBITS_2 = 1,         /**< Stop bit 2 */
} HW_UART_STOPBITS;

/**
 * \brief UART configuration structure definition
 *
 */
typedef struct {
        // Baud rate divisor
        HW_UART_BAUDRATE        baud_rate;
        HW_UART_DATABITS        data:2;
        HW_UART_PARITY          parity:2;
        HW_UART_STOPBITS        stop:1;
        uint8_t                 auto_flow_control:1;
        uint8_t                 use_dma:1;
        uint8_t                 use_fifo:1;

        HW_DMA_CHANNEL          tx_dma_channel:4;
        HW_DMA_CHANNEL          rx_dma_channel:4;
} uart_config;

/**
 * \brief UART configuration structure definition (extended version)
 *
 */
typedef struct {
        // Baud rate divisor
        HW_UART_BAUDRATE        baud_rate;
        HW_UART_DATABITS        data:2;
        HW_UART_PARITY          parity:2;
        HW_UART_STOPBITS        stop:1;
        uint8_t                 auto_flow_control:1;
        uint8_t                 use_dma:1;
        uint8_t                 use_fifo:1;
        uint8_t                 tx_fifo_tr_lvl:2;
        uint8_t                 rx_fifo_tr_lvl:2;

        HW_DMA_CHANNEL          tx_dma_channel:4;
        HW_DMA_CHANNEL          rx_dma_channel:4;
} uart_config_ex;

#ifdef HW_UART_ENABLE_USER_ISR
/**
 * \brief User defined interrupt function
 *
 * User code does not need to handle interrupts. Interrupts are handled by the driver.
 * If for some reason the user need to handle interrupts differently it is possible to
 * set a different ISR via hw_uart_set_isr(uart_id, user_isr).
 * In that case, the user-defined ISR must handle all UART interrupts as required.
 *
 * \sa hw_uart_set_isr
 *
 */
typedef void (*hw_uart_interrupt_isr)(void);

/**
 * \brief Setup user defined interrupt function for uart
 *
 * \param [in] uart identifies UART
 * \param [in] isr user defined interrupt handler
 *
 */
void hw_uart_set_isr(HW_UART_ID uart, hw_uart_interrupt_isr isr);
#endif

typedef void (*hw_uart_tx_callback)(void *user_data, uint16_t written);
typedef void (*hw_uart_rx_callback)(void *user_data, uint16_t read);

//======================= Status functions ====================================

/**
 * \brief Check if a serial transfer is inprogress
 *
 * \return true or false
 */
static inline bool hw_uart_is_busy(HW_UART_ID uart)
{
        return HW_UART_REG_GETF(uart, USR, UART_BUSY);
}

//===================== Read/Write functions ===================================

/**
 * \brief Read one byte from UART
 *
 * This is blocking function to read single byte from UART.
 * UART must be enabled before.
 *
 * \param [in] uart identifies UART to use
 *
 * \return the received byte
 *
 */
uint8_t hw_uart_read(HW_UART_ID uart);

/**
 * \brief Write one byte to UART
 *
 * Function wait till output FIFO has empty space according to
 * threshold level, and puts byte to be transmitted.
 *
 * \param [in] uart identifies UART to use
 * \param [in] data byte to be written
 *
 */
void hw_uart_write(HW_UART_ID uart, uint8_t data);

/**
 * \brief Stop asynchronous read from UART
 *
 * If there is outstanding read on given UART it will be stopped.
 * Callback will be fired with data already present in buffer.
 * Callback may be called from interrupt in the meantime but will be called
 * only once.
 *
 * \param [in] uart identifies UART to use
 *
 * \return number of bytes actually received
 *
 */
uint16_t hw_uart_abort_receive(HW_UART_ID uart);

/**
 * \brief Get number of bytes currently received by asynchronous read
 *
 * If there is outstanding read on given UART function returns number of
 * byte currently received.
 *
 * \param [in] uart identifies UART to use
 *
 */
uint16_t hw_uart_peek_received(HW_UART_ID uart);

/**
 * \brief Read receive buffer register
 *
 * \param [in] uart identifies UART to use
 *
 * \return the read byte
 *
 */
static inline uint8_t hw_uart_rxdata_getf(HW_UART_ID uart)
{
        // Read element from the receive FIFO
        return UBA(uart)->UART2_RBR_THR_DLL_REG;
}

/**
 * \brief Write byte to the transmit holding register
 *
 * \param [in] uart identifies UART to use
 * \param [in] data byte to be written
 *
 */
static inline void hw_uart_txdata_setf(HW_UART_ID uart, uint8_t data)
{
        // Write data to the transmit FIFO
        UBA(uart)->UART2_RBR_THR_DLL_REG = data;
}

/**
 * \brief Writes number of bytes to UART synchronously
 *
 * This function finishes when all data is put in FIFO. It does not wait for data to be transmitted
 * out from UART. Call \sa hw_uart_is_tx_fifo_empty() to wait for transmission to finish.
 * This function does not use interrupts nor DMA.
 *
 * \param [in] uart identifies UART to use
 * \param [in] data data to be written
 * \param [in] len length of to be written
 *
 */
void hw_uart_write_buffer(HW_UART_ID uart, const void *data, uint16_t len);

/**
 * \brief Writes number of bytes to UART
 *
 * \param [in] uart identifies UART to use
 * \param [in] data data to be written
 * \param [in] len length of to be written
 * \param [in] cb function to call from interrupt when transmission ends
 * \param [in] user_data parameter passed to cb function
 *
 */
void hw_uart_send(HW_UART_ID uart, const void *data, uint16_t len, hw_uart_tx_callback cb,
                                                                                void *user_data);

/**
 * \brief Read number of bytes from UART synchronously
 *
 * This function waits to receive \p len bytes from UART.
 * This function does not use interrupts not DMA, it just waits for all data to arive.
 *
 * \param [in] uart identifies UART to use
 * \param [out] data buffer for incoming data
 * \param [in] len number of bytes to read
 *
 */
void hw_uart_read_buffer(HW_UART_ID uart, void *data, uint16_t len);

/**
 * \brief Read number of bytes from UART
 *
 * Function initializes UART for receiving data.
 * When \p len data is received or timeout occurs user provided callback will be fired with
 * number of bytes read so far.
 *
 * \param [in] uart identifies UART to use
 * \param [out] data buffer for incoming data
 * \param [in] len max number of bytes to read
 * \param [in] cb function to call from interrupt when data is received
 * \param [in] user_data parameter passed to cb function
 *
 */
void hw_uart_receive(HW_UART_ID uart, void *data, uint16_t len, hw_uart_rx_callback cb,
                                                                                void *user_data);

#if HW_UART_USE_DMA_SUPPORT

/**
 * \brief Assign DMA channel to UART rx and tx
 *
 * Function specifies DMA channel to use for specified UART rx and tx FIFO.
 * It will setup channel only if there is no transmission in progress.
 *
 * \param [in] uart identifies UART to use
 * \param [in] channel id of DMA channel to use for rx,
 *                     tx will use next channel,
 *                     -1 if DMA should not be used
 * \param [in] pri priority DMA_PRIO_0 (lowest),
 *                          .....
 *                          DMA_PRIO_7 (highest)
 *
 */
void hw_uart_set_dma_channels(HW_UART_ID uart, int8_t channel, HW_DMA_PRIO pri);

/**
 * \brief Assign DMA channel to UART rx and tx
 *
 * Function specifies DMA channel to use for specified UART rx and tx FIFO.
 * It will setup channel only if there is no transmission in progress.
 *
 * \param [in] uart identifies UART to use
 * \param [in] tx_channel id of DMA channel to use for tx:
 * 					HW_DMA_CHANNEL_0..HW_DMA_CHANNEL_INVALID
 * \param [in] rx_channel id of DMA channel to use for rx:
 * 					HW_DMA_CHANNEL_0..HW_DMA_CHANNEL_INVALID
 * \param [in] pri priority DMA_PRIO_0 (lowest),
 *                          .....
 *                          DMA_PRIO_7 (highest)
 *
 */
void hw_uart_set_dma_channels_ex(HW_UART_ID uart, int8_t tx_channel, int8_t rx_channel, HW_DMA_PRIO pri);
#endif

//============== Interrupt handling ============================================

/**
 * \brief Set the Received Data Available interrupt
 *
 * \param [in] uart identifies UART to use
 * \param [in] recdataavail should be 0 to disable and 1 to enable
 *
 */
static inline void hw_uart_rec_data_int_set(HW_UART_ID uart, uint8_t recdataavail)
{
        // Set ERBFI bit in Interrupt Enable Register
        HW_UART_REG_SETF(uart, IER_DLH, ERBFI_dlh0, recdataavail);
}
/**
 * \brief Set the Transmit Holding Register empty interrupt
 *
 * \param [in] uart identifies UART to use
 * \param [in] txempty should be 0 to disable and 1 to enable
 *
 */
static inline void hw_uart_tx_empty_int_set(HW_UART_ID uart, uint8_t txempty)
{
        // Set ETBEI bit in Interrupt Enable Register
        HW_UART_REG_SETF(uart, IER_DLH, ETBEI_dlh1, txempty);
}

/**
 * \brief Set the Line Status interrupt
 *
 * \param [in] uart identifies UART to use
 * \param [in] linestat should be 0 to disable and 1 to enable
 *
 */
static inline void hw_uart_linestat_int_set(HW_UART_ID uart, uint8_t linestat)
{
        // Set ELSI bit in Interrupt Enable Register
        HW_UART_REG_SETF(uart, IER_DLH, ELSI_dhl2, linestat);
}

/**
 * \brief Set the Programmable THRE interrupt
 *
 * \param [in] uart identifies UART to use
 * \param [in] pthre should be 0 to disable and 1 to enable
 *
 */
static inline void hw_uart_pthre_int_set(HW_UART_ID uart, uint8_t pthre)
{
        // Set PTIME bit in Interrupt Enable Register
        HW_UART_REG_SETF(uart, IER_DLH, PTIME_dlh7, pthre);
}

/**
 * \brief Get the Interrupt ID
 *
 * \param [in] uart identifies UART to use
 *
 * \return interrupt type
 *
 */
static inline HW_UART_INT hw_uart_get_interrupt_id(HW_UART_ID uart)
{
        return (HW_UART_INT) (UBA(uart)->UART2_IIR_FCR_REG & 0xF);
}

/**
 * \brief Write Scratch pad register
 *
 * \param [in] uart identifies UART to use
 * \param [in] value the value to be written
 *
 * /warning Reserved when retarget is used else it is free to use.
 *
 */
static inline void hw_uart_write_scr(HW_UART_ID uart, uint8_t value)
{
        UBA(uart)->UART2_SCR_REG = value;
}

/**
 * \brief Read Scratch pad register
 *
 * \param [in] uart identifies UART to use
 *
 * \return register value
 *
 * \warning Reserved when retarget is used else it is free to use.
 *
 */
static inline uint8_t hw_uart_read_scr(HW_UART_ID uart)
{
        return UBA(uart)->UART2_SCR_REG;
}

//==================== Configuration functions =================================

/**
 * \brief Get the baud rate setting
 *
 * \param [in] uart identifies UART to use
 *
 * \return baud rate of specified uart
 *
 */
HW_UART_BAUDRATE hw_uart_baudrate_get(HW_UART_ID uart);

/**
 * \brief Set the baud rate
 *
 * \param [in] uart identifies UART to use
 * \param [in] baud_rate uart baud rate
 *
 */
void hw_uart_baudrate_set(HW_UART_ID uart, HW_UART_BAUDRATE baud_rate);

//=========================== FIFO control functions ===========================

/**
 * \brief Check if there is data available for read
 *
 * \param [in] uart identifies UART to use
 *
 * \return true if there is data to receive
 *
 */
static inline bool hw_uart_is_data_ready(HW_UART_ID uart)
{
        return (UBA(uart)->UART2_LSR_REG & 1) != 0;
}

/**
 * \brief Get the FIFO mode setting
 *
 * \param [in] uart identifies UART to use
 *
 * \return true if FIFO is enabled (both transmitter and receiver)
 *
 */
static inline bool hw_uart_is_fifo_enabled(HW_UART_ID uart)
{
        /* Only UART2 has a FIFO */
        ASSERT_ERROR(uart == HW_UART2);

        return UART2->UART2_SFE_REG != 0;
}

/**
 * \brief Disable both FIFOs
 *
 * \param [in] uart identifies UART to use
 *
 */
static inline void hw_uart_disable_fifo(HW_UART_ID uart)
{
        uint16_t iir_fcr_reg = UBA(uart)->UART2_IIR_FCR_REG;
        iir_fcr_reg &= 0xfffe;
        UBA(uart)->UART2_IIR_FCR_REG = iir_fcr_reg;
}

/**
 * \brief Enable both FIFOs
 *
 * Thresholds should be set before for predictable results.
 *
 * \param [in] uart identifies UART to use
 *
 */
static inline void hw_uart_enable_fifo(HW_UART_ID uart)
{
        /* Only UART2 has a FIFO */
        ASSERT_ERROR(uart == HW_UART2);

        UART2->UART2_SFE_REG = 1 << HW_UART_REG_FIELD_POS(2, SFE, UART_SHADOW_FIFO_ENABLE);
}

/**
 * \brief Check if receive FIFO is not empty
 *
 * \param [in] uart identifies UART to use
 *
 * \return true if FIFO is not empty
 *
 */
static inline bool hw_uart_receive_fifo_not_empty(HW_UART_ID uart)
{
        /* Only UART2 has a FIFO */
        ASSERT_ERROR(uart == HW_UART2);
        return HW_UART_REG_GETF(uart, USR, UART_RFNE) != 0;
}

/**
 * \brief Check if transmit FIFO is not full
 *
 * \param [in] uart identifies UART to use
 *
 * \return true if FIFO is full
 *
 */
static inline bool hw_uart_transmit_fifo_not_full(HW_UART_ID uart)
{
        /* Only UART2 has a FIFO */
        ASSERT_ERROR(uart == HW_UART2);
        return HW_UART_REG_GETF(uart, USR, UART_TFNF) != 0;
}

/**
 * \brief Check if transmit FIFO is empty
 *
 * \param [in] uart identifies UART to use
 *
 * \return true if FIFO empty
 *
 */
static inline bool hw_uart_transmit_fifo_empty(HW_UART_ID uart)
{
        return HW_UART_REG_GETF(uart, USR, UART_TFE) != 0;
}


/**
 * \brief Read number of byte in receive FIFO
 *
 * \param [in] uart identifies UART to use
 *
 * \return number of byte in receive FIFO
 *
 */
static inline uint16_t hw_uart_receive_fifo_count(HW_UART_ID uart)
{
        /* Only UART2 has a FIFO */
        ASSERT_ERROR(uart == HW_UART2);

        return UART2->UART2_RFL_REG;
}

/**
 * \brief Read number of byte in transmit FIFO
 *
 * \param [in] uart identifies UART to use
 *
 * \return number of byte in transmit FIFO
 *
 */
static inline uint16_t hw_uart_transmit_fifo_count(HW_UART_ID uart)
{
        /* Only UART2 has a FIFO */
        ASSERT_ERROR(uart == HW_UART2);

        return UART2->UART2_TFL_REG;
}

/**
 * \brief Enable loopback
 *
 * \param [in] uart identifies UART to use
 *
 */
static inline void hw_uart_enable_loopback(HW_UART_ID uart)
{
        HW_UART_REG_SETF(uart, MCR, UART_LB, 1);
}

/**
 * \brief Disable loopback
 *
 * \param [in] uart identifies UART to use
 *
 */
static inline void hw_uart_disable_loopback(HW_UART_ID uart)
{
        HW_UART_REG_SETF(uart, MCR, UART_LB, 0);
}

/**
 * \brief Get the FIFO mode setting
 *
 * \param [in] uart identifies UART to use
 *
 * \return the FIFO mode that has been set in the FIFO Control Register:
 *         0 = FIFO mode disabled,
 *         1 = FIFO mode enabled
 *
 */
uint8_t hw_uart_fifo_en_getf(HW_UART_ID uart);

/**
 * \brief Enable or disable the UART FIFO mode
 *
 * \param [in] uart identifies UART to use
 * \param [in] en should be 0 to disable and 1 to enable FIFO mode
 *
 */
static inline void hw_uart_fifo_en_setf(HW_UART_ID uart, uint8_t en)
{
        /* Only UART2 has a FIFO */
        ASSERT_ERROR(uart == HW_UART2);
        // Write FIFO Enable (FIFOE) bit in FIFO Control Register
        uint16_t iir_fcr_reg = UBA(uart)->UART2_IIR_FCR_REG;
        iir_fcr_reg &= ~0x1;
        iir_fcr_reg |= (en & 0x1);
        UBA(uart)->UART2_IIR_FCR_REG = iir_fcr_reg;
}

/**
 * \brief Get the receive FIFO trigger level at which the
 *        Received Data Available Interrupt is generated
 *
 * \param [in] uart identifies UART to use
 *
 * \return the receive FIFO trigger level:
 *         0 = 1 character in the FIFO,
 *         1 = FIFO 1/4 full,
 *         2 = FIFO 1/2 full,
 *         3 = FIFO 2 less than full
 *
 */
static inline uint8_t hw_uart_rx_fifo_tr_lvl_getf(HW_UART_ID uart)
{
        /* Only UART2 has a FIFO */
        ASSERT_ERROR(uart == HW_UART2);
        return UART2->UART2_SRT_REG & UART2_UART2_SRT_REG_UART_SHADOW_RCVR_TRIGGER_Msk;
}

/**
 * \brief Set the receive FIFO trigger level at which the
 *        Received Data Available Interrupt is generated
 *
 * \param [in] uart identifies UART to use
 * \param [in] tr_lvl The receive FIFO trigger level:
 *                    0 = 1 character in the FIFO,
 *                    1 = FIFO 1/4 full,
 *                    2 = FIFO 1/2 full,
 *                    3 = FIFO 2 less than full
 *
 */
static inline void hw_uart_rx_fifo_tr_lvl_setf(HW_UART_ID uart, uint8_t tr_lvl)
{
        /* Only UART2 has a FIFO */
        ASSERT_ERROR(uart == HW_UART2);

        UART2->UART2_SRT_REG = tr_lvl;
}

/**
 * \brief Get the transmit FIFO trigger level at which the
 *        Transmit Holding Register Empty (THRE) Interrupt is generated
 *
 * \param [in] uart identifies UART to use
 *
 * \return the transmit FIFO trigger level:
 *         0 = FIFO empty,
 *         1 = 2 characters in the FIFO,
 *         2 = FIFO 1/4 full,
 *         3 = FIFO 1/2 full
 *
 */
uint8_t hw_uart_tx_fifo_tr_lvl_getf(HW_UART_ID uart);

/**
 * \brief Set the transmit FIFO trigger level at which the
 *        Transmit Holding Register Empty (THRE) Interrupt is generated
 *
 * \param [in] uart identifies UART to use
 * \param [in] tr_lvl the transmit FIFO trigger level:
 *                    0 = FIFO empty,
 *                    1 = 2 characters in the FIFO,
 *                    2 = FIFO 1/4 full,
 *                    3 = FIFO 1/2 full
 *
 */
static inline void hw_uart_tx_fifo_tr_lvl_setf(HW_UART_ID uart, uint8_t tr_lvl)
{
        /* Only UART2 has a FIFO */
        ASSERT_ERROR(uart == HW_UART2);

        UART2->UART2_STET_REG = tr_lvl;
}

/**
 * \brief Reset UART transmit FIFO
 *
 * \param [in] uart identifies UART to use
 *
 */
static inline void hw_uart_tx_fifo_flush(HW_UART_ID uart)
{
        HW_UART_REG_SETF(uart, SRR, UART_XFR, 1);
}

/**
 * \brief Reset UART receive FIFO
 *
 * \param [in] uart identifies UART to use
 *
 */
static inline void hw_uart_rx_fifo_flush(HW_UART_ID uart)
{
        HW_UART_REG_SETF(uart, SRR, UART_RFR, 1);
}

/**
 * \brief Check whether reading buffer is empty
 *
 * Works for both when Rx FIFO is enabled or not.
 *
 * \param [in] uart identifies UART to use
 *
 * \return true if there is no data available for reading
 *
 */
static inline bool hw_uart_read_buf_empty(HW_UART_ID uart)
{
        return !HW_UART_REG_GETF(uart, LSR, UART_DR);
}

/**
 * \brief Check whether writing buffer is full
 *
 * Works for both when Tx FIFO is enabled or not.
 *
 * \param [in] uart identifies UART to use
 *
 * \return true if transmit buffer is full
 *
 */
static inline bool hw_uart_write_buf_full(HW_UART_ID uart)
{
        return !HW_UART_REG_GETF(uart, LSR, UART_THRE);
}

#if dg_configUART_SOFTWARE_FIFO
/**
 * \brief Set buffer that will be use for incoming data when there is no read in progress
 *
 * \param [in] uart identifies UART to use
 * \param [in] buf buffer to use as software FIFO
 * \param [in] size size of \p buf
 *
 */
void hw_uart_set_soft_fifo(HW_UART_ID uart, uint8_t *buf, uint8_t size);
#endif

//=========================== DMA control functions ============================

/**
 * \brief Set UART DMA mode
 *
 * \param [in] uart identifies UART to use
 * \param [in] dma_mode DMA mode 0 or 1
 *
 */
static inline void hw_uart_dma_mode_setf(HW_UART_ID uart, uint8_t dma_mode)
{
        /* Only UART2 has the SDMAM register */
        ASSERT_ERROR(uart == HW_UART2);

        UART2->UART2_SDMAM_REG = ((dma_mode & 1) << UART2_UART2_SDMAM_REG_UART_SHADOW_DMA_MODE_Pos)
                                        << UART2_UART2_SDMAM_REG_UART_SHADOW_DMA_MODE_Msk;
}

/**
 * \brief Clear DMA request
 *
 * \param [in] uart identifies UART to use
 *
 */
static inline void hw_uart_clear_dma_request(HW_UART_ID uart)
{
        UBA(uart)->UART2_DMASA_REG = 1;
}

//=========================== Line control functions ============================

/**
 * \brief Set UART line settings
 *
 * This function initialize UART registers with given configuration.
 * It also initializes all internal software variables for buffered transmissions.
 *
 * \param [in] uart identifies UART to use
 * \param [in] cfg pointer to the UART configuration structure
 *
 */
void hw_uart_init(HW_UART_ID uart, const uart_config *cfg);

/**
 * \brief Set UART line settings
 *
 * This function initialize UART registers with given configuration,
 * including tx and rx fifo trigger levels
 * It also initializes all internal software variables for buffered transmissions.
 *
 * \param [in] uart identifies UART to use
 * \param [in] cfg pointer to the UART configuration structure
 *
 */
void hw_uart_init_ex(HW_UART_ID uart, const uart_config_ex *cfg);

/**
 * \brief Re-initialize UART registers
 *
 * Call this function with configuration that should be re-applied, this function should be
 * called after platform exits power sleep mode.
 * This function is similar to uart_init, but it does not initialize software variables
 * used for transmission control it just re-applies hardware configuration.
 * It will turn on interrupts if transmission was in progress.
 *
 * \param [in] uart identifies UART to use
 * \param [in] cfg pointer to the UART configuration structure
 *
 */
void hw_uart_reinit(HW_UART_ID uart, const uart_config *cfg);

/**
 * \brief Re-initialize UART registers
 *
 * Call this function with configuration that should be re-applied, this function should be
 * called after platform exits power sleep mode.
 * This function is similar to uart_init_ex, but it does not initialize software variables
 * used for transmission control it just re-applies hardware configuration.
 * It will turn on interrupts if transmission was in progress.
 *
 * \param [in] uart identifies UART to use
 * \param [in] cfg pointer to the UART configuration structure
 *
 */
void hw_uart_reinit_ex(HW_UART_ID uart, const uart_config_ex *cfg);

/**
 * \brief Get UART line settings
 *
 * \param [in] uart identifies UART to use
 * \param [in] cfg pointer to the UART configuration structure
 */
void hw_uart_cfg_get(HW_UART_ID uart, uart_config *cfg);

//=========================== Modem control functions ==========================

/**
 * \brief Get the IrDA SIR mode setting
 *
 * \param [in] uart identifies UART to use
 *
 * \return IrDA SIR mode:
 *         0 = IrDA SIR mode disabled,
 *         1 = IrDA SIR mode enabled
 *
 */
uint8_t hw_uart_sire_getf(HW_UART_ID uart);

/**
 * \brief Set the IrDA SIR mode
 *
 * \param [in] uart identifies UART to use
 * \param [in] sire 0 = disable IrDA SIR mode,
 *                  1 = enable IrDA SIR mode
 *
 */
void hw_uart_sire_setf(HW_UART_ID uart, uint8_t sire);

/**
 * \brief Get the Auto Flow Control Enable (AFCE) setting
 *
 * \param [in] uart identifies UART to use
 *
 * \return Auto Flow Control Enable (AFCE):
 *         0 = Auto Flow Control Mode disabled,
 *         1 = Auto Flow Control Mode enabled
 *
 */
uint8_t hw_uart_afce_getf(HW_UART_ID uart);

/**
 * \brief Enable or disable Auto Flow Control
 *
 * \param [in] uart identifies UART to use
 * \param [in] afce Auto Flow Control Enable (AFCE):
 *                  0 = disable Auto Flow Control Mode,
 *                  1 = enable Auto Flow Control Mode
 *
 */
void hw_uart_afce_setf(HW_UART_ID uart, uint8_t afce);

/**
 * \brief Get UART diagnostic mode status
 *
 * \param [in] uart identifies UART to use
 *
 * \return the value of the loop back bit
 *
 */
uint8_t hw_uart_loopback_getf(HW_UART_ID uart);

/**
 * \brief Set UART in diagnostic mode
 *
 * \param [in] uart identifies UART to use
 * \param [in] lb loop back bit value
 *
 */
void hw_uart_loopback_setf(HW_UART_ID uart, uint8_t lb);

/**
 * \brief Get RTS output value
 *
 * \param [in] uart identifies UART to use
 *
 * \return RTS output value
 *
 */
uint8_t hw_uart_rts_getf(HW_UART_ID uart);

/**
 * \brief Set RTS output value
 *
 * \param [in] uart identifies UART to use
 * \param [in] rtsn Value for the RTS output (asserted low)
 *
 */
void hw_uart_rts_setf(HW_UART_ID uart, uint8_t rtsn);

//=========================== Line status functions ============================

/**
 * \brief Get the value of the Receiver FIFO Error bit
 *
 * \param [in] uart identifies UART to use
 *
 * \return receiver FIFO Error bit value:
 *         0 = no error in RX FIFO,
 *         1 = error in RX FIFO
 *
 */
uint8_t hw_uart_rx_fifo_err_getf(HW_UART_ID uart);

/**
 * \brief Get the value of the Transmitter Empty bit
 *
 * \param [in] uart identifies UART to use
 *
 * \return 1 if transmitter FIFO is empty
 *
 */
uint8_t hw_uart_is_tx_fifo_empty(HW_UART_ID uart);

/**
 * \brief Get the value of the Transmit Holding Register Empty bit
 *
 * \param [in] uart identifies UART to use
 *
 * \return the Transmit Holding Register Empty bit value
 *
 */
uint8_t hw_uart_thr_empty_getf(HW_UART_ID uart);

/**
 * \brief Get the value of the Break Interrupt bit
 *
 * \param [in] uart identifies UART to use
 *
 * \return break interrupt bit value: This is used to indicate the detection of a
 *         break sequence on the serial input data
 *
 */
uint8_t hw_uart_break_int_getf(HW_UART_ID uart);

/**
 * \brief Get the value of the Framing Error bit
 *
 * \param [in] uart identifies UART to use
 *
 * \return framing error bit value:
 *         0 = no framing error,
 *         1 = framing error
 *
 *
 */
uint8_t hw_uart_frame_err_getf(HW_UART_ID uart);

/**
 * \brief Get the value of the Parity Error bit
 *
 * \param [in] uart identifies UART to use
 *
 * \return parity error bit value:
 *         0 = no parity error,
 *         1 = parity error
 *
 */
uint8_t hw_uart_parity_err_getf(HW_UART_ID uart);

/**
 * \brief Get the value of the Overrun Error bit
 *
 * \param [in] uart identifies UART to use
 *
 * \return overrun error bit value:
 *         0 = no overrun error,
 *         1 = overrun error
 *
 */
uint8_t hw_uart_overrun_err_getf(HW_UART_ID uart);

//=========================== Modem status functions ===========================

/**
 * \brief Get CTS input status
 *
 * \param [in] uart identifies UART to use
 *
 * \return status of CTS input:
 *         0 = CTSn input is de-asserted (logic 1),
 *         1 = CTSn input is asserted (logic 0),
 *         In loopback mode, CTS is the same as RTS
 *
 */
uint8_t hw_uart_cts_getf(HW_UART_ID uart);

/**
 * \brief Get Delta CTS
 *
 * \param [in] uart identifies UART to use
 *
 * \return DCTS:
 *         0 = No change on CTS since last read of Modem Control Register,
 *         1 = Change on CTS since last read of Modem Control Register,
 *         Note that calling this function will clear the DCTS bit,
 *         In loopback mode, DCTS reflects changes on RTS
 *
 */
uint8_t hw_uart_delta_cts_getf(HW_UART_ID uart);

/**
 * \brief Check if buffered write is in progress
 *
 * \param [in] uart identifies UART to use
 *
 * \return true if there is TX transmission in progress
 *
 */
bool hw_uart_tx_in_progress(HW_UART_ID uart);

/**
 * \brief Check if buffered read is in progress
 *
 * \param [in] uart identifies UART to use
 *
 * \return true if there is RX transmission in progress
 *
 */
bool hw_uart_rx_in_progress(HW_UART_ID uart);

#if dg_configUART_RX_CIRCULAR_DMA

/**
 * \brief Enable circular RX DMA for UART
 *
 * This reconfigures RX DMA channel to use circular buffer for incoming data and enabled it
 * immediately. Incoming data will be read by DMA to intermediate buffer. Subsequent calls to
 * hw_uart_receive() will fire callback as soon as enough data are available in buffer to be read,
 * but the actual read from intermediate buffer has to be done using hw_uart_copy_rx_circular_dma_buffer().
 *
 * Recommended way to move between DMA buffer and user buffer is to call hw_uart_abort_receive()
 * that will copy the amount specified on read or the actual number of received bytes if
 * hw_uart_abort_receive() was called before requested the amount was received.
 *
 * \note
 * This can be only called for an UART which is configured using \p dg_configUARTx_RX_CIRCULAR_DMA_BUF_SIZE.
 *
 * \param [in] uart identifies UART to use
 *
 */
void hw_uart_enable_rx_circular_dma(HW_UART_ID uart);

/**
 * \brief Copy data from circular RX DMA buffer to user buffer
 *
 * \param [in] uart identifies UART to use
 * \param [in] buf buffer to copy data to
 * \param [in] len length of data to copy to buffer
 *
 */
void hw_uart_copy_rx_circular_dma_buffer(HW_UART_ID uart, uint8_t *buf, uint16_t len);

#endif /* dg_configUART_RX_CIRCULAR_DMA */

#endif /* dg_configUSE_HW_UART */

#endif /* HW_UART_H_ */

/**
 * \}
 * \}
 * \}
 */
