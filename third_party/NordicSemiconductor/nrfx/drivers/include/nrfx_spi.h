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

#ifndef NRFX_SPI_H__
#define NRFX_SPI_H__

#include <nrfx.h>
#include <hal/nrf_spi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_spi SPI driver
 * @{
 * @ingroup nrf_spi
 * @brief   SPI peripheral driver.
 */

/**
 * @brief SPI master driver instance data structure.
 */
typedef struct
{
    NRF_SPI_Type * p_reg;        ///< Pointer to a structure with SPI registers.
    uint8_t        drv_inst_idx; ///< Driver instance index.
} nrfx_spi_t;

enum {
#if NRFX_CHECK(NRFX_SPI0_ENABLED)
    NRFX_SPI0_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_SPI1_ENABLED)
    NRFX_SPI1_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_SPI2_ENABLED)
    NRFX_SPI2_INST_IDX,
#endif
    NRFX_SPI_ENABLED_COUNT
};

/**
 * @brief Macro for creating an SPI master driver instance.
 */
#define NRFX_SPI_INSTANCE(id)                               \
{                                                           \
    .p_reg        = NRFX_CONCAT_2(NRF_SPI, id),             \
    .drv_inst_idx = NRFX_CONCAT_3(NRFX_SPI, id, _INST_IDX), \
}

/**
 * @brief This value can be provided instead of a pin number for signals MOSI,
 *        MISO, and Slave Select to specify that the given signal is not used and
 *        therefore does not need to be connected to a pin.
 */
#define NRFX_SPI_PIN_NOT_USED  0xFF

/**
 * @brief SPI master driver instance configuration structure.
 */
typedef struct
{
    uint8_t sck_pin;                ///< SCK pin number.
    uint8_t mosi_pin;               ///< MOSI pin number (optional).
                                    /**< Set to @ref NRFX_SPI_PIN_NOT_USED
                                     *   if this signal is not needed. */
    uint8_t miso_pin;               ///< MISO pin number (optional).
                                    /**< Set to @ref NRFX_SPI_PIN_NOT_USED
                                     *   if this signal is not needed. */
    uint8_t ss_pin;                 ///< Slave Select pin number (optional).
                                    /**< Set to @ref NRFX_SPI_PIN_NOT_USED
                                     *   if this signal is not needed. The driver
                                     *   supports only active low for this signal.
                                     *   If the signal should be active high,
                                     *   it must be controlled externally. */
    uint8_t irq_priority;           ///< Interrupt priority.
    uint8_t orc;                    ///< Over-run character.
                                    /**< This character is used when all bytes from the TX buffer are sent,
                                         but the transfer continues due to RX. */
    nrf_spi_frequency_t frequency;  ///< SPI frequency.
    nrf_spi_mode_t      mode;       ///< SPI mode.
    nrf_spi_bit_order_t bit_order;  ///< SPI bit order.
} nrfx_spi_config_t;

/**
 * @brief SPI master instance default configuration.
 */
#define NRFX_SPI_DEFAULT_CONFIG                           \
{                                                         \
    .sck_pin      = NRFX_SPI_PIN_NOT_USED,                \
    .mosi_pin     = NRFX_SPI_PIN_NOT_USED,                \
    .miso_pin     = NRFX_SPI_PIN_NOT_USED,                \
    .ss_pin       = NRFX_SPI_PIN_NOT_USED,                \
    .irq_priority = NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY, \
    .orc          = 0xFF,                                 \
    .frequency    = NRF_SPI_FREQ_4M,                      \
    .mode         = NRF_SPI_MODE_0,                       \
    .bit_order    = NRF_SPI_BIT_ORDER_MSB_FIRST,          \
}

/**
 * @brief Single transfer descriptor structure.
 */
typedef struct
{
    uint8_t const * p_tx_buffer; ///< Pointer to TX buffer.
    size_t          tx_length;   ///< TX buffer length.
    uint8_t       * p_rx_buffer; ///< Pointer to RX buffer.
    size_t          rx_length;   ///< RX buffer length.
} nrfx_spi_xfer_desc_t;

/**
 * @brief Macro for setting up single transfer descriptor.
 *
 * This macro is for internal use only.
 */
#define NRFX_SPI_SINGLE_XFER(p_tx, tx_len, p_rx, rx_len)  \
    {                                                     \
    .p_tx_buffer = (uint8_t const *)(p_tx),               \
    .tx_length = (tx_len),                                \
    .p_rx_buffer = (p_rx),                                \
    .rx_length = (rx_len),                                \
    }

/**
 * @brief Macro for setting duplex TX RX transfer.
 */
#define NRFX_SPI_XFER_TRX(p_tx_buf, tx_length, p_rx_buf, rx_length) \
        NRFX_SPI_SINGLE_XFER(p_tx_buf, tx_length, p_rx_buf, rx_length)

/**
 * @brief Macro for setting TX transfer.
 */
#define NRFX_SPI_XFER_TX(p_buf, length) \
        NRFX_SPI_SINGLE_XFER(p_buf, length, NULL, 0)

/**
 * @brief Macro for setting RX transfer.
 */
#define NRFX_SPI_XFER_RX(p_buf, length) \
        NRFX_SPI_SINGLE_XFER(NULL, 0, p_buf, length)

/**
 * @brief SPI master driver event types, passed to the handler routine provided
 *        during initialization.
 */
typedef enum
{
    NRFX_SPI_EVENT_DONE, ///< Transfer done.
} nrfx_spi_evt_type_t;

typedef struct
{
    nrfx_spi_evt_type_t  type;      ///< Event type.
    nrfx_spi_xfer_desc_t xfer_desc; ///< Transfer details.
} nrfx_spi_evt_t;

/**
 * @brief SPI master driver event handler type.
 */
typedef void (* nrfx_spi_evt_handler_t)(nrfx_spi_evt_t const * p_event,
                                        void *                 p_context);

/**
 * @brief Function for initializing the SPI master driver instance.
 *
 * This function configures and enables the specified peripheral.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] p_config   Pointer to the structure with initial configuration.
 *
 * @param     handler    Event handler provided by the user. If NULL, transfers
 *                       will be performed in blocking mode.
 * @param     p_context  Context passed to event handler.
 *
 * @retval NRFX_SUCCESS             If initialization was successful.
 * @retval NRFX_ERROR_INVALID_STATE If the driver was already initialized.
 * @retval NRFX_ERROR_BUSY          If some other peripheral with the same
 *                                  instance ID is already in use. This is
 *                                  possible only if @ref nrfx_prs module
 *                                  is enabled.
 */
nrfx_err_t nrfx_spi_init(nrfx_spi_t const * const  p_instance,
                         nrfx_spi_config_t const * p_config,
                         nrfx_spi_evt_handler_t    handler,
                         void *                    p_context);

/**
 * @brief Function for uninitializing the SPI master driver instance.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void       nrfx_spi_uninit(nrfx_spi_t const * const p_instance);

/**
 * @brief Function for starting the SPI data transfer.
 *
 * If an event handler was provided in the @ref nrfx_spi_init call, this function
 * returns immediately and the handler is called when the transfer is done.
 * Otherwise, the transfer is performed in blocking mode, which means that this function
 * returns when the transfer is finished.
 *
 * @param p_instance  Pointer to the driver instance structure.
 * @param p_xfer_desc Pointer to the transfer descriptor.
 * @param flags       Transfer options (0 for default settings).
 *                    Currently, no additional flags are available.
 *
 * @retval NRFX_SUCCESS             If the procedure was successful.
 * @retval NRFX_ERROR_BUSY          If the driver is not ready for a new transfer.
 * @retval NRFX_ERROR_NOT_SUPPORTED If the provided parameters are not supported.
 */
nrfx_err_t nrfx_spi_xfer(nrfx_spi_t const * const     p_instance,
                         nrfx_spi_xfer_desc_t const * p_xfer_desc,
                         uint32_t                     flags);

/**
 * @brief Function for aborting ongoing transfer.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_spi_abort(nrfx_spi_t const * p_instance);


void nrfx_spi_0_irq_handler(void);
void nrfx_spi_1_irq_handler(void);
void nrfx_spi_2_irq_handler(void);


/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_SPI_H__
