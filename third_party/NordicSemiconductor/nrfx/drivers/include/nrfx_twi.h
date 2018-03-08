/**
 * Copyright (c) 2015 - 2018, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#ifndef NRFX_TWI_H__
#define NRFX_TWI_H__

#include <nrfx.h>
#include <hal/nrf_twi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_twi TWI driver
 * @{
 * @ingroup nrf_twi
 * @brief   TWI peripheral driver.
 */

/**
 * @brief Structure for the TWI master driver instance.
 */
typedef struct
{
    NRF_TWI_Type * p_twi;        ///< Pointer to a structure with TWI registers.
    uint8_t        drv_inst_idx; ///< Driver instance index.
} nrfx_twi_t;

/**
 * @brief Macro for creating a TWI master driver instance.
 */
#define NRFX_TWI_INSTANCE(id)                               \
{                                                           \
    .p_twi        = NRFX_CONCAT_2(NRF_TWI, id),             \
    .drv_inst_idx = NRFX_CONCAT_3(NRFX_TWI, id, _INST_IDX), \
}

enum {
#if NRFX_CHECK(NRFX_TWI0_ENABLED)
    NRFX_TWI0_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_TWI1_ENABLED)
    NRFX_TWI1_INST_IDX,
#endif
    NRFX_TWI_ENABLED_COUNT
};

/**
 * @brief Structure for the TWI master driver instance configuration.
 */
typedef struct
{
    uint32_t            scl;                 ///< SCL pin number.
    uint32_t            sda;                 ///< SDA pin number.
    nrf_twi_frequency_t frequency;           ///< TWI frequency.
    uint8_t             interrupt_priority;  ///< Interrupt priority.
    bool                hold_bus_uninit;     ///< Hold pull up state on gpio pins after uninit.
} nrfx_twi_config_t;

/**
 * @brief TWI master driver instance default configuration.
 */
#define NRFX_TWI_DEFAULT_CONFIG                                                   \
{                                                                                 \
    .frequency          = (nrf_twi_frequency_t)NRFX_TWI_DEFAULT_CONFIG_FREQUENCY, \
    .scl                = 31,                                                     \
    .sda                = 31,                                                     \
    .interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY,                   \
    .hold_bus_uninit    = NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT,                \
}

#define NRFX_TWI_FLAG_NO_XFER_EVT_HANDLER (1UL << 2) /**< Interrupt after each transfer is suppressed, and the event handler is not called. */
#define NRFX_TWI_FLAG_TX_NO_STOP          (1UL << 5) /**< Flag indicating that the TX transfer will not end with a stop condition. */

/**
 * @brief TWI master driver event types.
 */
typedef enum
{
    NRFX_TWI_EVT_DONE,         ///< Transfer completed event.
    NRFX_TWI_EVT_ADDRESS_NACK, ///< Error event: NACK received after sending the address.
    NRFX_TWI_EVT_DATA_NACK     ///< Error event: NACK received after sending a data byte.
} nrfx_twi_evt_type_t;

/**
 * @brief TWI master driver transfer types.
 */
typedef enum
{
    NRFX_TWI_XFER_TX,          ///< TX transfer.
    NRFX_TWI_XFER_RX,          ///< RX transfer.
    NRFX_TWI_XFER_TXRX,        ///< TX transfer followed by RX transfer with repeated start.
    NRFX_TWI_XFER_TXTX         ///< TX transfer followed by TX transfer with repeated start.
} nrfx_twi_xfer_type_t;

/**
 * @brief Structure for a TWI transfer descriptor.
 */
typedef struct
{
    nrfx_twi_xfer_type_t    type;             ///< Type of transfer.
    uint8_t                 address;          ///< Slave address.
    size_t                  primary_length;   ///< Number of bytes transferred.
    size_t                  secondary_length; ///< Number of bytes transferred.
    uint8_t *               p_primary_buf;    ///< Pointer to transferred data.
    uint8_t *               p_secondary_buf;  ///< Pointer to transferred data.
} nrfx_twi_xfer_desc_t;


/**@brief Macro for setting the TX transfer descriptor. */
#define NRFX_TWI_XFER_DESC_TX(addr, p_data, length) \
    {                                               \
        .type = NRFX_TWI_XFER_TX,                   \
        .address = addr,                            \
        .primary_length = length,                   \
        .p_primary_buf  = p_data,                   \
    }

/**@brief Macro for setting the RX transfer descriptor. */
#define NRFX_TWI_XFER_DESC_RX(addr, p_data, length) \
    {                                               \
        .type = NRFX_TWI_XFER_RX,                   \
        .address = addr,                            \
        .primary_length = length,                   \
        .p_primary_buf  = p_data,                   \
    }

/**@brief Macro for setting the TXRX transfer descriptor. */
#define NRFX_TWI_XFER_DESC_TXRX(addr, p_tx, tx_len, p_rx, rx_len)   \
    {                                                               \
        .type = NRFX_TWI_XFER_TXRX,                                 \
        .address = addr,                                            \
        .primary_length   = tx_len,                                 \
        .secondary_length = rx_len,                                 \
        .p_primary_buf    = p_tx,                                   \
        .p_secondary_buf  = p_rx,                                   \
    }

/**@brief Macro for setting the TXTX transfer descriptor. */
#define NRFX_TWI_XFER_DESC_TXTX(addr, p_tx, tx_len, p_tx2, tx_len2) \
    {                                                               \
        .type = NRFX_TWI_XFER_TXTX,                                 \
        .address = addr,                                            \
        .primary_length   = tx_len,                                 \
        .secondary_length = tx_len2,                                \
        .p_primary_buf    = p_tx,                                   \
        .p_secondary_buf  = p_tx2,                                  \
    }

/**
 * @brief Structure for a TWI event.
 */
typedef struct
{
    nrfx_twi_evt_type_t  type;      ///< Event type.
    nrfx_twi_xfer_desc_t xfer_desc; ///< Transfer details.
} nrfx_twi_evt_t;

/**
 * @brief TWI event handler prototype.
 */
typedef void (* nrfx_twi_evt_handler_t)(nrfx_twi_evt_t const * p_event,
                                        void *                 p_context);

/**
 * @brief Function for initializing the TWI driver instance.
 *
 * @param[in] p_instance      Pointer to the driver instance structure.
 * @param[in] p_config        Pointer to the structure with initial configuration.
 * @param[in] event_handler   Event handler provided by the user. If NULL, blocking mode is enabled.
 * @param[in] p_context       Context passed to event handler.
 *
 * @retval NRFX_SUCCESS             If initialization was successful.
 * @retval NRFX_ERROR_INVALID_STATE If the driver is in invalid state.
 * @retval NRFX_ERROR_BUSY          If some other peripheral with the same
 *                                  instance ID is already in use. This is
 *                                  possible only if @ref nrfx_prs module
 *                                  is enabled.
 */
nrfx_err_t nrfx_twi_init(nrfx_twi_t const *        p_instance,
                         nrfx_twi_config_t const * p_config,
                         nrfx_twi_evt_handler_t    event_handler,
                         void *                    p_context);

/**
 * @brief Function for uninitializing the TWI instance.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_twi_uninit(nrfx_twi_t const * p_instance);

/**
 * @brief Function for enabling the TWI instance.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_twi_enable(nrfx_twi_t const * p_instance);

/**
 * @brief Function for disabling the TWI instance.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_twi_disable(nrfx_twi_t const * p_instance);

/**
 * @brief Function for sending data to a TWI slave.
 *
 * The transmission will be stopped when an error occurs. If a transfer is ongoing,
 * the function returns the error code @ref NRFX_ERROR_BUSY.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] address    Address of a specific slave device (only 7 LSB).
 * @param[in] p_data     Pointer to a transmit buffer.
 * @param[in] length     Number of bytes to send.
 * @param[in] no_stop    If set, the stop condition is not generated on the bus
 *                       after the transfer has completed successfully (allowing
 *                       for a repeated start in the next transfer).
 *
 * @retval NRFX_SUCCESS                  If the procedure was successful.
 * @retval NRFX_ERROR_BUSY               If the driver is not ready for a new transfer.
 * @retval NRFX_ERROR_INTERNAL           If an error was detected by hardware.
 * @retval NRFX_ERROR_DRV_TWI_ERR_ANACK  If NACK received after sending the address in polling mode.
 * @retval NRFX_ERROR_DRV_TWI_ERR_DNACK  If NACK received after sending a data byte in polling mode.
 */
nrfx_err_t nrfx_twi_tx(nrfx_twi_t const * p_instance,
                       uint8_t            address,
                       uint8_t const *    p_data,
                       size_t             length,
                       bool               no_stop);

/**
 * @brief Function for reading data from a TWI slave.
 *
 * The transmission will be stopped when an error occurs. If a transfer is ongoing,
 * the function returns the error code @ref NRFX_ERROR_BUSY.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] address    Address of a specific slave device (only 7 LSB).
 * @param[in] p_data     Pointer to a receive buffer.
 * @param[in] length     Number of bytes to be received.
 *
 * @retval NRFX_SUCCESS                    If the procedure was successful.
 * @retval NRFX_ERROR_BUSY                 If the driver is not ready for a new transfer.
 * @retval NRFX_ERROR_INTERNAL             If an error was detected by hardware.
 * @retval NRFX_ERROR_DRV_TWI_ERR_OVERRUN  If the unread data was replaced by new data
 * @retval NRFX_ERROR_DRV_TWI_ERR_ANACK    If NACK received after sending the address in polling mode.
 * @retval NRFX_ERROR_DRV_TWI_ERR_DNACK    If NACK received after sending a data byte in polling mode.
 */
nrfx_err_t nrfx_twi_rx(nrfx_twi_t const * p_instance,
                       uint8_t            address,
                       uint8_t *          p_data,
                       size_t             length);

/**
 * @brief Function for preparing a TWI transfer.
 *
 * The following transfer types can be configured (@ref nrfx_twi_xfer_desc_t::type):
 * - @ref NRFX_TWI_XFER_TXRX<span></span>: Write operation followed by a read operation (without STOP condition in between).
 * - @ref NRFX_TWI_XFER_TXTX<span></span>: Write operation followed by a write operation (without STOP condition in between).
 * - @ref NRFX_TWI_XFER_TX<span></span>:   Write operation (with or without STOP condition).
 * - @ref NRFX_TWI_XFER_RX<span></span>:   Read operation  (with STOP condition).
 *
 * @note TXRX and TXTX transfers are supported only in non-blocking mode.
 *
 * Additional options are provided using the flags parameter:
 * - @ref NRFX_TWI_FLAG_NO_XFER_EVT_HANDLER<span></span>: No user event handler after transfer completion. In most cases, this also means no interrupt at the end of the transfer.
 * - @ref NRFX_TWI_FLAG_TX_NO_STOP<span></span>: No stop condition after TX transfer.
 *
 * @note
 * Some flag combinations are invalid:
 * - @ref NRFX_TWI_FLAG_TX_NO_STOP with @ref nrfx_twi_xfer_desc_t::type different than @ref NRFX_TWI_XFER_TX
 *
 * @param[in] p_instance        Pointer to the driver instance structure.
 * @param[in] p_xfer_desc       Pointer to the transfer descriptor.
 * @param[in] flags             Transfer options (0 for default settings).
 *
 * @retval NRFX_SUCCESS                    If the procedure was successful.
 * @retval NRFX_ERROR_BUSY                 If the driver is not ready for a new transfer.
 * @retval NRFX_ERROR_NOT_SUPPORTED        If the provided parameters are not supported.
 * @retval NRFX_ERROR_INTERNAL             If an error was detected by hardware.
 * @retval NRFX_ERROR_DRV_TWI_ERR_OVERRUN  If the unread data was replaced by new data (TXRX and RX)
 * @retval NRFX_ERROR_DRV_TWI_ERR_ANACK    If NACK received after sending the address.
 * @retval NRFX_ERROR_DRV_TWI_ERR_DNACK    If NACK received after sending a data byte.
 */
nrfx_err_t nrfx_twi_xfer(nrfx_twi_t           const * p_instance,
                         nrfx_twi_xfer_desc_t const * p_xfer_desc,
                         uint32_t                     flags);

/**
 * @brief Function for checking the TWI driver state.
 *
 * @param[in] p_instance TWI instance.
 *
 * @retval true  If the TWI driver is currently busy performing a transfer.
 * @retval false If the TWI driver is ready for a new transfer.
 */
bool nrfx_twi_is_busy(nrfx_twi_t const * p_instance);

/**
 * @brief Function for getting the transferred data count.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @return     Data count.
 */
size_t nrfx_twi_data_count_get(nrfx_twi_t const * const p_instance);

/**
 * @brief Function for returning the address of a STOPPED TWI event.
 *
 * A STOPPED event can be used to detect the end of a transfer if the @ref NRFX_TWI_FLAG_NO_XFER_EVT_HANDLER
 * option is used.
 *
 * @param[in]  p_instance Pointer to the driver instance structure.
 *
 * @return     STOPPED event address.
 */
uint32_t nrfx_twi_stopped_event_get(nrfx_twi_t const * p_instance);

void nrfx_twi_0_irq_handler(void);
void nrfx_twi_1_irq_handler(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_TWI_H__
