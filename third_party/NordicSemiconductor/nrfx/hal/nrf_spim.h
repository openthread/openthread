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

#ifndef NRF_SPIM_H__
#define NRF_SPIM_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_spim_hal SPIM HAL
 * @{
 * @ingroup nrf_spim
 * @brief   Hardware access layer for managing the SPIM peripheral.
 */

/**
 * @brief This value can be used as a parameter for the @ref nrf_spim_pins_set
 *        function to specify that a given SPI signal (SCK, MOSI, or MISO)
 *        shall not be connected to a physical pin.
 */
#define NRF_SPIM_PIN_NOT_CONNECTED  0xFFFFFFFF

#if defined(SPIM_DCXCNT_DCXCNT_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief This value specified in the DCX line configuration causes this line
 *        to be set low during whole transmission (all transmitted bytes are
 *        marked as command bytes). Any lower value causes the DCX line to be
 *        switched from low to high after this number of bytes is transmitted
 *        (all remaining bytes are marked as data bytes).
 */
#define NRF_SPIM_DCX_CNT_ALL_CMD 0xF
#endif

#define NRF_SPIM_HW_CSN_PRESENT                        \
    (NRFX_CHECK(SPIM0_FEATURE_HARDWARE_CSN_PRESENT) || \
     NRFX_CHECK(SPIM1_FEATURE_HARDWARE_CSN_PRESENT) || \
     NRFX_CHECK(SPIM2_FEATURE_HARDWARE_CSN_PRESENT) || \
     NRFX_CHECK(SPIM3_FEATURE_HARDWARE_CSN_PRESENT))

/**
 * @brief SPIM tasks.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_SPIM_TASK_START   = offsetof(NRF_SPIM_Type, TASKS_START),   ///< Start SPI transaction.
    NRF_SPIM_TASK_STOP    = offsetof(NRF_SPIM_Type, TASKS_STOP),    ///< Stop SPI transaction.
    NRF_SPIM_TASK_SUSPEND = offsetof(NRF_SPIM_Type, TASKS_SUSPEND), ///< Suspend SPI transaction.
    NRF_SPIM_TASK_RESUME  = offsetof(NRF_SPIM_Type, TASKS_RESUME)   ///< Resume SPI transaction.
    /*lint -restore*/
} nrf_spim_task_t;

/**
 * @brief SPIM events.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_SPIM_EVENT_STOPPED = offsetof(NRF_SPIM_Type, EVENTS_STOPPED), ///< SPI transaction has stopped.
    NRF_SPIM_EVENT_ENDRX   = offsetof(NRF_SPIM_Type, EVENTS_ENDRX),   ///< End of RXD buffer reached.
    NRF_SPIM_EVENT_END     = offsetof(NRF_SPIM_Type, EVENTS_END),     ///< End of RXD buffer and TXD buffer reached.
    NRF_SPIM_EVENT_ENDTX   = offsetof(NRF_SPIM_Type, EVENTS_ENDTX),   ///< End of TXD buffer reached.
    NRF_SPIM_EVENT_STARTED = offsetof(NRF_SPIM_Type, EVENTS_STARTED)  ///< Transaction started.
    /*lint -restore*/
} nrf_spim_event_t;

/**
 * @brief SPIM shortcuts.
 */
typedef enum
{
    NRF_SPIM_SHORT_END_START_MASK = SPIM_SHORTS_END_START_Msk, ///< Shortcut between END event and START task.
    NRF_SPIM_ALL_SHORTS_MASK      = SPIM_SHORTS_END_START_Msk  ///< All SPIM shortcuts.
} nrf_spim_short_mask_t;

/**
 * @brief SPIM interrupts.
 */
typedef enum
{
    NRF_SPIM_INT_STOPPED_MASK = SPIM_INTENSET_STOPPED_Msk,  ///< Interrupt on STOPPED event.
    NRF_SPIM_INT_ENDRX_MASK   = SPIM_INTENSET_ENDRX_Msk,    ///< Interrupt on ENDRX event.
    NRF_SPIM_INT_END_MASK     = SPIM_INTENSET_END_Msk,      ///< Interrupt on END event.
    NRF_SPIM_INT_ENDTX_MASK   = SPIM_INTENSET_ENDTX_Msk,    ///< Interrupt on ENDTX event.
    NRF_SPIM_INT_STARTED_MASK = SPIM_INTENSET_STARTED_Msk,  ///< Interrupt on STARTED event.
    NRF_SPIM_ALL_INTS_MASK    = SPIM_INTENSET_STOPPED_Msk |
                                SPIM_INTENSET_ENDRX_Msk   |
                                SPIM_INTENSET_END_Msk     |
                                SPIM_INTENSET_ENDTX_Msk   |
                                SPIM_INTENSET_STARTED_Msk   ///< All SPIM interrupts.
} nrf_spim_int_mask_t;

/**
 * @brief SPI master data rates.
 */
typedef enum
{
    NRF_SPIM_FREQ_125K = SPIM_FREQUENCY_FREQUENCY_K125,    ///< 125 kbps.
    NRF_SPIM_FREQ_250K = SPIM_FREQUENCY_FREQUENCY_K250,    ///< 250 kbps.
    NRF_SPIM_FREQ_500K = SPIM_FREQUENCY_FREQUENCY_K500,    ///< 500 kbps.
    NRF_SPIM_FREQ_1M   = SPIM_FREQUENCY_FREQUENCY_M1,      ///< 1 Mbps.
    NRF_SPIM_FREQ_2M   = SPIM_FREQUENCY_FREQUENCY_M2,      ///< 2 Mbps.
    NRF_SPIM_FREQ_4M   = SPIM_FREQUENCY_FREQUENCY_M4,      ///< 4 Mbps.
    // [conversion to 'int' needed to prevent compilers from complaining
    //  that the provided value (0x80000000UL) is out of range of "int"]
    NRF_SPIM_FREQ_8M   = (int)SPIM_FREQUENCY_FREQUENCY_M8, ///< 8 Mbps.
#if defined(SPIM_FREQUENCY_FREQUENCY_M16) || defined(__NRFX_DOXYGEN__)
    NRF_SPIM_FREQ_16M  = SPIM_FREQUENCY_FREQUENCY_M16,     ///< 16 Mbps.
#endif
#if defined(SPIM_FREQUENCY_FREQUENCY_M32) || defined(__NRFX_DOXYGEN__)
    NRF_SPIM_FREQ_32M  = SPIM_FREQUENCY_FREQUENCY_M32      ///< 32 Mbps.
#endif
} nrf_spim_frequency_t;

/**
 * @brief SPI modes.
 */
typedef enum
{
    NRF_SPIM_MODE_0, ///< SCK active high, sample on leading edge of clock.
    NRF_SPIM_MODE_1, ///< SCK active high, sample on trailing edge of clock.
    NRF_SPIM_MODE_2, ///< SCK active low, sample on leading edge of clock.
    NRF_SPIM_MODE_3  ///< SCK active low, sample on trailing edge of clock.
} nrf_spim_mode_t;

/**
 * @brief SPI bit orders.
 */
typedef enum
{
    NRF_SPIM_BIT_ORDER_MSB_FIRST = SPIM_CONFIG_ORDER_MsbFirst, ///< Most significant bit shifted out first.
    NRF_SPIM_BIT_ORDER_LSB_FIRST = SPIM_CONFIG_ORDER_LsbFirst  ///< Least significant bit shifted out first.
} nrf_spim_bit_order_t;

#if (NRF_SPIM_HW_CSN_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief SPI CSN pin polarity.
 */
typedef enum
{
    NRF_SPIM_CSN_POL_LOW  = SPIM_CSNPOL_CSNPOL_LOW, ///< Active low (idle state high).
    NRF_SPIM_CSN_POL_HIGH = SPIM_CSNPOL_CSNPOL_HIGH ///< Active high (idle state low).
} nrf_spim_csn_pol_t;
#endif // (NRF_SPIM_HW_CSN_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for activating a specific SPIM task.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] spim_task Task to activate.
 */
__STATIC_INLINE void nrf_spim_task_trigger(NRF_SPIM_Type * p_reg,
                                           nrf_spim_task_t spim_task);

/**
 * @brief Function for getting the address of a specific SPIM task register.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] spim_task Requested task.
 *
 * @return Address of the specified task register.
 */
__STATIC_INLINE uint32_t nrf_spim_task_address_get(NRF_SPIM_Type * p_reg,
                                                   nrf_spim_task_t spim_task);

/**
 * @brief Function for clearing a specific SPIM event.
 *
 * @param[in] p_reg      Pointer to the peripheral registers structure.
 * @param[in] spim_event Event to clear.
 */
__STATIC_INLINE void nrf_spim_event_clear(NRF_SPIM_Type * p_reg,
                                          nrf_spim_event_t spim_event);

/**
 * @brief Function for checking the state of a specific SPIM event.
 *
 * @param[in] p_reg      Pointer to the peripheral registers structure.
 * @param[in] spim_event Event to check.
 *
 * @retval true  If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_spim_event_check(NRF_SPIM_Type * p_reg,
                                          nrf_spim_event_t spim_event);

/**
 * @brief Function for getting the address of a specific SPIM event register.
 *
 * @param[in] p_reg      Pointer to the peripheral registers structure.
 * @param[in] spim_event Requested event.
 *
 * @return Address of the specified event register.
 */
__STATIC_INLINE uint32_t nrf_spim_event_address_get(NRF_SPIM_Type  * p_reg,
                                                    nrf_spim_event_t spim_event);
/**
 * @brief Function for enabling specified shortcuts.
 *
 * @param[in] p_reg            Pointer to the peripheral registers structure.
 * @param[in] spim_shorts_mask Shortcuts to enable.
 */
__STATIC_INLINE void nrf_spim_shorts_enable(NRF_SPIM_Type * p_reg,
                                            uint32_t spim_shorts_mask);

/**
 * @brief Function for disabling specified shortcuts.
 *
 * @param[in] p_reg            Pointer to the peripheral registers structure.
 * @param[in] spim_shorts_mask Shortcuts to disable.
 */
__STATIC_INLINE void nrf_spim_shorts_disable(NRF_SPIM_Type * p_reg,
                                             uint32_t spim_shorts_mask);

/**
 * @brief Function for getting shorts setting.
 *
 * @param[in] p_reg           Pointer to the peripheral registers structure.
 */
__STATIC_INLINE uint32_t nrf_spim_shorts_get(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg        Pointer to the peripheral registers structure.
 * @param[in] spim_int_mask Interrupts to enable.
 */
__STATIC_INLINE void nrf_spim_int_enable(NRF_SPIM_Type * p_reg,
                                         uint32_t spim_int_mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg        Pointer to the peripheral registers structure.
 * @param[in] spim_int_mask Interrupts to disable.
 */
__STATIC_INLINE void nrf_spim_int_disable(NRF_SPIM_Type * p_reg,
                                          uint32_t spim_int_mask);

/**
 * @brief Function for retrieving the state of a given interrupt.
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param[in] spim_int Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_spim_int_enable_check(NRF_SPIM_Type * p_reg,
                                               nrf_spim_int_mask_t spim_int);

/**
 * @brief Function for enabling the SPIM peripheral.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_enable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for disabling the SPIM peripheral.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_disable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for configuring SPIM pins.
 *
 * If a given signal is not needed, pass the @ref NRF_SPIM_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_reg    Pointer to the peripheral registers structure.
 * @param[in] sck_pin  SCK pin number.
 * @param[in] mosi_pin MOSI pin number.
 * @param[in] miso_pin MISO pin number.
 */
__STATIC_INLINE void nrf_spim_pins_set(NRF_SPIM_Type * p_reg,
                                       uint32_t sck_pin,
                                       uint32_t mosi_pin,
                                       uint32_t miso_pin);

#if (NRF_SPIM_HW_CSN_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for configuring the SPIM hardware CSN pin.
 *
 * If this signal is not needed, pass the @ref NRF_SPIM_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_reg    Pointer to the peripheral registers structure.
 * @param[in] pin      CSN pin number.
 * @param[in] polarity CSN pin polarity.
 * @param[in] duration Minimum duration between the edge of CSN and the edge of SCK
 *                     and minimum duration of CSN must stay unselected between transactions.
 *                     The value is specified in number of 64 MHz clock cycles (15.625 ns).
 */
__STATIC_INLINE void nrf_spim_csn_configure(NRF_SPIM_Type * p_reg,
                                            uint32_t pin,
                                            nrf_spim_csn_pol_t polarity,
                                            uint32_t duration);
#endif // (NRF_SPIM_HW_CSN_PRESENT) || defined(__NRFX_DOXYGEN__)

#if defined(SPIM_PSELDCX_CONNECT_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for configuring the SPIM DCX pin.
 *
 * If this signal is not needed, pass the @ref NRF_SPIM_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param[in] dcx_pin DCX pin number.
 */
__STATIC_INLINE void nrf_spim_dcx_pin_set(NRF_SPIM_Type * p_reg,
                                          uint32_t dcx_pin);

/**
 * @brief Function for configuring the number of command bytes.
 *
 * Maximum value available for dividing the transmitted bytes into command
 * bytes and data bytes is @ref NRF_SPIM_DCX_CNT_ALL_CMD - 1.
 * The @ref NRF_SPIM_DCX_CNT_ALL_CMD value passed as the @c count parameter
 * causes all transmitted bytes to be marked as command bytes.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] count Number of command bytes preceding the data bytes.
 */
__STATIC_INLINE void nrf_spim_dcx_cnt_set(NRF_SPIM_Type * p_reg,
                                          uint32_t count);
#endif // defined(SPIM_PSELDCX_CONNECT_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(SPIM_IFTIMING_RXDELAY_RXDELAY_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for configuring the extended SPIM interface.
 * @param p_reg   Pointer to the peripheral registers structure.
 * @param rxdelay Sample delay for input serial data on MISO,
 *                specified in 64 MHz clock cycles (15.625 ns) from the sampling edge of SCK.
 */
__STATIC_INLINE void nrf_spim_iftiming_set(NRF_SPIM_Type * p_reg,
                                           uint32_t rxdelay);
#endif // defined(SPIM_IFTIMING_RXDELAY_RXDELAY_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(SPIM_STALLSTAT_RX_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for clearing stall status for RX EasyDMA RAM accesses.
 *
 * @param p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_stallstat_rx_clear(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for getting stall status for RX EasyDMA RAM accesses.
 *
 * @param p_reg Pointer to the peripheral registers structure.
 *
 * @return Stall status of RX EasyDMA RAM accesses.
 */
__STATIC_INLINE bool nrf_spim_stallstat_rx_get(NRF_SPIM_Type * p_reg);
#endif // defined(SPIM_STALLSTAT_RX_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(SPIM_STALLSTAT_TX_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for clearing stall status for TX EasyDMA RAM accesses.
 *
 * @param p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_stallstat_tx_clear(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for getting stall status for TX EasyDMA RAM accesses.
 *
 * @param p_reg Pointer to the peripheral registers structure.
 *
 * @return Stall status of TX EasyDMA RAM accesses.
 */
__STATIC_INLINE bool nrf_spim_stallstat_tx_get(NRF_SPIM_Type * p_reg);
#endif // defined(SPIM_STALLSTAT_TX_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for setting the SPI master data rate.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] frequency SPI frequency.
 */
__STATIC_INLINE void nrf_spim_frequency_set(NRF_SPIM_Type * p_reg,
                                            nrf_spim_frequency_t frequency);

/**
 * @brief Function for setting the transmit buffer.
 *
 * @param[in]  p_reg   Pointer to the peripheral registers structure.
 * @param[in]  p_buffer Pointer to the buffer with data to send.
 * @param[in]  length   Maximum number of data bytes to transmit.
 */
__STATIC_INLINE void nrf_spim_tx_buffer_set(NRF_SPIM_Type * p_reg,
                                            uint8_t const * p_buffer,
                                            size_t          length);

/**
 * @brief Function for setting the receive buffer.
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param[in] p_buffer Pointer to the buffer for received data.
 * @param[in] length   Maximum number of data bytes to receive.
 */
__STATIC_INLINE void nrf_spim_rx_buffer_set(NRF_SPIM_Type * p_reg,
                                            uint8_t * p_buffer,
                                            size_t    length);

/**
 * @brief Function for setting the SPI configuration.
 *
 * @param[in] p_reg        Pointer to the peripheral registers structure.
 * @param[in] spi_mode      SPI mode.
 * @param[in] spi_bit_order SPI bit order.
 */
__STATIC_INLINE void nrf_spim_configure(NRF_SPIM_Type * p_reg,
                                        nrf_spim_mode_t spi_mode,
                                        nrf_spim_bit_order_t spi_bit_order);

/**
 * @brief Function for setting the over-read character.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] orc    Over-read character that is clocked out in case of
 *                   an over-read of the TXD buffer.
 */
__STATIC_INLINE void nrf_spim_orc_set(NRF_SPIM_Type * p_reg,
                                      uint8_t orc);

/**
 * @brief Function for enabling the TX list feature.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_tx_list_enable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for disabling the TX list feature.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_tx_list_disable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for enabling the RX list feature.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_rx_list_enable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for disabling the RX list feature.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_spim_rx_list_disable(NRF_SPIM_Type * p_reg);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_spim_task_trigger(NRF_SPIM_Type * p_reg,
                                           nrf_spim_task_t spim_task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spim_task)) = 0x1UL;
}

__STATIC_INLINE uint32_t nrf_spim_task_address_get(NRF_SPIM_Type * p_reg,
                                                   nrf_spim_task_t spim_task)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)spim_task);
}

__STATIC_INLINE void nrf_spim_event_clear(NRF_SPIM_Type * p_reg,
                                          nrf_spim_event_t spim_event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spim_event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spim_event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_spim_event_check(NRF_SPIM_Type * p_reg,
                                          nrf_spim_event_t spim_event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)spim_event);
}

__STATIC_INLINE uint32_t nrf_spim_event_address_get(NRF_SPIM_Type * p_reg,
                                                    nrf_spim_event_t spim_event)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)spim_event);
}

__STATIC_INLINE void nrf_spim_shorts_enable(NRF_SPIM_Type * p_reg,
                                            uint32_t spim_shorts_mask)
{
    p_reg->SHORTS |= spim_shorts_mask;
}

__STATIC_INLINE void nrf_spim_shorts_disable(NRF_SPIM_Type * p_reg,
                                             uint32_t spim_shorts_mask)
{
    p_reg->SHORTS &= ~(spim_shorts_mask);
}

__STATIC_INLINE uint32_t nrf_spim_shorts_get(NRF_SPIM_Type * p_reg)
{
    return p_reg->SHORTS;
}

__STATIC_INLINE void nrf_spim_int_enable(NRF_SPIM_Type * p_reg,
                                         uint32_t spim_int_mask)
{
    p_reg->INTENSET = spim_int_mask;
}

__STATIC_INLINE void nrf_spim_int_disable(NRF_SPIM_Type * p_reg,
                                          uint32_t spim_int_mask)
{
    p_reg->INTENCLR = spim_int_mask;
}

__STATIC_INLINE bool nrf_spim_int_enable_check(NRF_SPIM_Type * p_reg,
                                               nrf_spim_int_mask_t spim_int)
{
    return (bool)(p_reg->INTENSET & spim_int);
}

__STATIC_INLINE void nrf_spim_enable(NRF_SPIM_Type * p_reg)
{
    p_reg->ENABLE = (SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_spim_disable(NRF_SPIM_Type * p_reg)
{
    p_reg->ENABLE = (SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_spim_pins_set(NRF_SPIM_Type * p_reg,
                                       uint32_t sck_pin,
                                       uint32_t mosi_pin,
                                       uint32_t miso_pin)
{
    p_reg->PSEL.SCK  = sck_pin;
    p_reg->PSEL.MOSI = mosi_pin;
    p_reg->PSEL.MISO = miso_pin;
}

#if (NRF_SPIM_HW_CSN_PRESENT)
__STATIC_INLINE void nrf_spim_csn_configure(NRF_SPIM_Type * p_reg,
                                            uint32_t pin,
                                            nrf_spim_csn_pol_t polarity,
                                            uint32_t duration)
{
    p_reg->PSEL.CSN = pin;
    p_reg->CSNPOL = polarity;
    p_reg->IFTIMING.CSNDUR = duration;
}
#endif // defined(NRF_SPIM_HW_CSN_PRESENT)

#if defined(SPIM_PSELDCX_CONNECT_Msk)
__STATIC_INLINE void nrf_spim_dcx_pin_set(NRF_SPIM_Type * p_reg,
                                          uint32_t dcx_pin)
{
    p_reg->PSELDCX = dcx_pin;
}

__STATIC_INLINE void nrf_spim_dcx_cnt_set(NRF_SPIM_Type * p_reg,
                                          uint32_t dcx_cnt)
{
    p_reg->DCXCNT = dcx_cnt;
}
#endif // defined(SPIM_PSELDCX_CONNECT_Msk)

#if defined(SPIM_IFTIMING_RXDELAY_RXDELAY_Msk)
__STATIC_INLINE void nrf_spim_iftiming_set(NRF_SPIM_Type * p_reg,
                                           uint32_t rxdelay)
{
    p_reg->IFTIMING.RXDELAY = rxdelay;
}
#endif // defined(SPIM_IFTIMING_RXDELAY_RXDELAY_Msk)

#if defined(SPIM_STALLSTAT_RX_Msk)
__STATIC_INLINE void nrf_spim_stallstat_rx_clear(NRF_SPIM_Type * p_reg)
{
    p_reg->STALLSTAT &= ~(SPIM_STALLSTAT_RX_Msk);
}

__STATIC_INLINE bool nrf_spim_stallstat_rx_get(NRF_SPIM_Type * p_reg)
{
    return (p_reg->STALLSTAT & SPIM_STALLSTAT_RX_Msk) != 0;
}
#endif // defined(SPIM_STALLSTAT_RX_Msk)

#if defined(SPIM_STALLSTAT_TX_Msk)
__STATIC_INLINE void nrf_spim_stallstat_tx_clear(NRF_SPIM_Type * p_reg)
{
    p_reg->STALLSTAT &= ~(SPIM_STALLSTAT_TX_Msk);
}

__STATIC_INLINE bool nrf_spim_stallstat_tx_get(NRF_SPIM_Type * p_reg)
{
    return (p_reg->STALLSTAT & SPIM_STALLSTAT_TX_Msk) != 0;
}
#endif // defined(SPIM_STALLSTAT_TX_Msk)

__STATIC_INLINE void nrf_spim_frequency_set(NRF_SPIM_Type * p_reg,
                                            nrf_spim_frequency_t frequency)
{
    p_reg->FREQUENCY = frequency;
}

__STATIC_INLINE void nrf_spim_tx_buffer_set(NRF_SPIM_Type * p_reg,
                                            uint8_t const * p_buffer,
                                            size_t          length)
{
    p_reg->TXD.PTR    = (uint32_t)p_buffer;
    p_reg->TXD.MAXCNT = length;
}

__STATIC_INLINE void nrf_spim_rx_buffer_set(NRF_SPIM_Type * p_reg,
                                            uint8_t * p_buffer,
                                            size_t    length)
{
    p_reg->RXD.PTR    = (uint32_t)p_buffer;
    p_reg->RXD.MAXCNT = length;
}

__STATIC_INLINE void nrf_spim_configure(NRF_SPIM_Type * p_reg,
                                        nrf_spim_mode_t spi_mode,
                                        nrf_spim_bit_order_t spi_bit_order)
{
    uint32_t config = (spi_bit_order == NRF_SPIM_BIT_ORDER_MSB_FIRST ?
        SPIM_CONFIG_ORDER_MsbFirst : SPIM_CONFIG_ORDER_LsbFirst);
    switch (spi_mode)
    {
    default:
    case NRF_SPIM_MODE_0:
        config |= (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos) |
                  (SPIM_CONFIG_CPHA_Leading    << SPIM_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIM_MODE_1:
        config |= (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos) |
                  (SPIM_CONFIG_CPHA_Trailing   << SPIM_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIM_MODE_2:
        config |= (SPIM_CONFIG_CPOL_ActiveLow  << SPIM_CONFIG_CPOL_Pos) |
                  (SPIM_CONFIG_CPHA_Leading    << SPIM_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIM_MODE_3:
        config |= (SPIM_CONFIG_CPOL_ActiveLow  << SPIM_CONFIG_CPOL_Pos) |
                  (SPIM_CONFIG_CPHA_Trailing   << SPIM_CONFIG_CPHA_Pos);
        break;
    }
    p_reg->CONFIG = config;
}

__STATIC_INLINE void nrf_spim_orc_set(NRF_SPIM_Type * p_reg,
                                      uint8_t orc)
{
    p_reg->ORC = orc;
}


__STATIC_INLINE void nrf_spim_tx_list_enable(NRF_SPIM_Type * p_reg)
{
    p_reg->TXD.LIST = 1;
}

__STATIC_INLINE void nrf_spim_tx_list_disable(NRF_SPIM_Type * p_reg)
{
    p_reg->TXD.LIST = 0;
}

__STATIC_INLINE void nrf_spim_rx_list_enable(NRF_SPIM_Type * p_reg)
{
    p_reg->RXD.LIST = 1;
}

__STATIC_INLINE void nrf_spim_rx_list_disable(NRF_SPIM_Type * p_reg)
{
    p_reg->RXD.LIST = 0;
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_SPIM_H__
