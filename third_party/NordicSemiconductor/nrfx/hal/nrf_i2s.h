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

#ifndef NRF_I2S_H__
#define NRF_I2S_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_i2s_hal I2S HAL
 * @{
 * @ingroup nrf_i2s
 * @brief   Hardware access layer for managing the Inter-IC Sound (I2S) peripheral.
 */

/**
 * @brief This value can be provided as a parameter for the @ref nrf_i2s_pins_set
 *        function call to specify that a given I2S signal (SDOUT, SDIN, or MCK)
 *        shall not be connected to a physical pin.
 */
#define NRF_I2S_PIN_NOT_CONNECTED  0xFFFFFFFF


/**
 * @brief I2S tasks.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_I2S_TASK_START = offsetof(NRF_I2S_Type, TASKS_START), ///< Starts continuous I2S transfer. Also starts the MCK generator if this is enabled.
    NRF_I2S_TASK_STOP  = offsetof(NRF_I2S_Type, TASKS_STOP)   ///< Stops I2S transfer. Also stops the MCK generator.
    /*lint -restore*/
} nrf_i2s_task_t;

/**
 * @brief I2S events.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_I2S_EVENT_RXPTRUPD = offsetof(NRF_I2S_Type, EVENTS_RXPTRUPD), ///< The RXD.PTR register has been copied to internal double-buffers.
    NRF_I2S_EVENT_TXPTRUPD = offsetof(NRF_I2S_Type, EVENTS_TXPTRUPD), ///< The TXD.PTR register has been copied to internal double-buffers.
    NRF_I2S_EVENT_STOPPED  = offsetof(NRF_I2S_Type, EVENTS_STOPPED)   ///< I2S transfer stopped.
    /*lint -restore*/
} nrf_i2s_event_t;

/**
 * @brief I2S interrupts.
 */
typedef enum
{
    NRF_I2S_INT_RXPTRUPD_MASK = I2S_INTENSET_RXPTRUPD_Msk, ///< Interrupt on RXPTRUPD event.
    NRF_I2S_INT_TXPTRUPD_MASK = I2S_INTENSET_TXPTRUPD_Msk, ///< Interrupt on TXPTRUPD event.
    NRF_I2S_INT_STOPPED_MASK  = I2S_INTENSET_STOPPED_Msk   ///< Interrupt on STOPPED event.
} nrf_i2s_int_mask_t;

/**
 * @brief I2S modes of operation.
 */
typedef enum
{
    NRF_I2S_MODE_MASTER = I2S_CONFIG_MODE_MODE_Master, ///< Master mode.
    NRF_I2S_MODE_SLAVE  = I2S_CONFIG_MODE_MODE_Slave   ///< Slave mode.
} nrf_i2s_mode_t;

/**
 * @brief I2S master clock generator settings.
 */
typedef enum
{
    NRF_I2S_MCK_DISABLED  = 0,                                       ///< MCK disabled.
    // [conversion to 'int' needed to prevent compilers from complaining
    //  that the provided value (0x80000000UL) is out of range of "int"]
    NRF_I2S_MCK_32MDIV2   = (int)I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV2, ///< 32 MHz / 2 = 16.0 MHz.
    NRF_I2S_MCK_32MDIV3   = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV3,      ///< 32 MHz / 3 = 10.6666667 MHz.
    NRF_I2S_MCK_32MDIV4   = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV4,      ///< 32 MHz / 4 = 8.0 MHz.
    NRF_I2S_MCK_32MDIV5   = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV5,      ///< 32 MHz / 5 = 6.4 MHz.
    NRF_I2S_MCK_32MDIV6   = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV6,      ///< 32 MHz / 6 = 5.3333333 MHz.
    NRF_I2S_MCK_32MDIV8   = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV8,      ///< 32 MHz / 8 = 4.0 MHz.
    NRF_I2S_MCK_32MDIV10  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV10,     ///< 32 MHz / 10 = 3.2 MHz.
    NRF_I2S_MCK_32MDIV11  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV11,     ///< 32 MHz / 11 = 2.9090909 MHz.
    NRF_I2S_MCK_32MDIV15  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV15,     ///< 32 MHz / 15 = 2.1333333 MHz.
    NRF_I2S_MCK_32MDIV16  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV16,     ///< 32 MHz / 16 = 2.0 MHz.
    NRF_I2S_MCK_32MDIV21  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV21,     ///< 32 MHz / 21 = 1.5238095 MHz.
    NRF_I2S_MCK_32MDIV23  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV23,     ///< 32 MHz / 23 = 1.3913043 MHz.
    NRF_I2S_MCK_32MDIV31  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV31,     ///< 32 MHz / 31 = 1.0322581 MHz.
    NRF_I2S_MCK_32MDIV42  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV42,     ///< 32 MHz / 42 = 0.7619048 MHz.
    NRF_I2S_MCK_32MDIV63  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV63,     ///< 32 MHz / 63 = 0.5079365 MHz.
    NRF_I2S_MCK_32MDIV125 = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV125     ///< 32 MHz / 125 = 0.256 MHz.
} nrf_i2s_mck_t;

/**
 * @brief I2S MCK/LRCK ratios.
 */
typedef enum
{
    NRF_I2S_RATIO_32X  = I2S_CONFIG_RATIO_RATIO_32X,  ///< LRCK = MCK / 32.
    NRF_I2S_RATIO_48X  = I2S_CONFIG_RATIO_RATIO_48X,  ///< LRCK = MCK / 48.
    NRF_I2S_RATIO_64X  = I2S_CONFIG_RATIO_RATIO_64X,  ///< LRCK = MCK / 64.
    NRF_I2S_RATIO_96X  = I2S_CONFIG_RATIO_RATIO_96X,  ///< LRCK = MCK / 96.
    NRF_I2S_RATIO_128X = I2S_CONFIG_RATIO_RATIO_128X, ///< LRCK = MCK / 128.
    NRF_I2S_RATIO_192X = I2S_CONFIG_RATIO_RATIO_192X, ///< LRCK = MCK / 192.
    NRF_I2S_RATIO_256X = I2S_CONFIG_RATIO_RATIO_256X, ///< LRCK = MCK / 256.
    NRF_I2S_RATIO_384X = I2S_CONFIG_RATIO_RATIO_384X, ///< LRCK = MCK / 384.
    NRF_I2S_RATIO_512X = I2S_CONFIG_RATIO_RATIO_512X  ///< LRCK = MCK / 512.
} nrf_i2s_ratio_t;

/**
 * @brief I2S sample widths.
 */
typedef enum
{
    NRF_I2S_SWIDTH_8BIT  = I2S_CONFIG_SWIDTH_SWIDTH_8Bit,  ///< 8 bit.
    NRF_I2S_SWIDTH_16BIT = I2S_CONFIG_SWIDTH_SWIDTH_16Bit, ///< 16 bit.
    NRF_I2S_SWIDTH_24BIT = I2S_CONFIG_SWIDTH_SWIDTH_24Bit  ///< 24 bit.
} nrf_i2s_swidth_t;

/**
 * @brief I2S alignments of sample within a frame.
 */
typedef enum
{
    NRF_I2S_ALIGN_LEFT  = I2S_CONFIG_ALIGN_ALIGN_Left, ///< Left-aligned.
    NRF_I2S_ALIGN_RIGHT = I2S_CONFIG_ALIGN_ALIGN_Right ///< Right-aligned.
} nrf_i2s_align_t;

/**
 * @brief I2S frame formats.
 */
typedef enum
{
    NRF_I2S_FORMAT_I2S     = I2S_CONFIG_FORMAT_FORMAT_I2S,    ///< Original I2S format.
    NRF_I2S_FORMAT_ALIGNED = I2S_CONFIG_FORMAT_FORMAT_Aligned ///< Alternate (left- or right-aligned) format.
} nrf_i2s_format_t;

/**
 * @brief I2S enabled channels.
 */
typedef enum
{
    NRF_I2S_CHANNELS_STEREO = I2S_CONFIG_CHANNELS_CHANNELS_Stereo, ///< Stereo.
    NRF_I2S_CHANNELS_LEFT   = I2S_CONFIG_CHANNELS_CHANNELS_Left,   ///< Left only.
    NRF_I2S_CHANNELS_RIGHT  = I2S_CONFIG_CHANNELS_CHANNELS_Right   ///< Right only.
} nrf_i2s_channels_t;


/**
 * @brief Function for activating a specific I2S task.
 *
 * @param[in] p_i2s I2S instance.
 * @param[in] task  Task to activate.
 */
__STATIC_INLINE void nrf_i2s_task_trigger(NRF_I2S_Type * p_i2s,
                                          nrf_i2s_task_t task);

/**
 * @brief Function for getting the address of a specific I2S task register.
 *
 * @param[in] p_i2s I2S instance.
 * @param[in] task  Requested task.
 *
 * @return Address of the specified task register.
 */
__STATIC_INLINE uint32_t nrf_i2s_task_address_get(NRF_I2S_Type const * p_i2s,
                                                  nrf_i2s_task_t task);

/**
 * @brief Function for clearing a specific I2S event.
 *
 * @param[in] p_i2s I2S instance.
 * @param[in] event Event to clear.
 */
__STATIC_INLINE void nrf_i2s_event_clear(NRF_I2S_Type * p_i2s,
                                         nrf_i2s_event_t event);

/**
 * @brief Function for checking the state of a specific I2S event.
 *
 * @param[in] p_i2s I2S instance.
 * @param[in] event Event to check.
 *
 * @retval true  If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_i2s_event_check(NRF_I2S_Type const * p_i2s,
                                         nrf_i2s_event_t event);

/**
 * @brief Function for getting the address of a specific I2S event register.
 *
 * @param[in] p_i2s I2S instance.
 * @param[in] event Requested event.
 *
 * @return Address of the specified event register.
 */
__STATIC_INLINE uint32_t nrf_i2s_event_address_get(NRF_I2S_Type const * p_i2s,
                                                   nrf_i2s_event_t event);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_i2s I2S instance.
 * @param[in] mask  Interrupts to enable.
 */
__STATIC_INLINE void nrf_i2s_int_enable(NRF_I2S_Type * p_i2s, uint32_t mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_i2s I2S instance.
 * @param[in] mask  Interrupts to disable.
 */
__STATIC_INLINE void nrf_i2s_int_disable(NRF_I2S_Type * p_i2s, uint32_t mask);

/**
 * @brief Function for retrieving the state of a given interrupt.
 *
 * @param[in] p_i2s   I2S instance.
 * @param[in] i2s_int Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_i2s_int_enable_check(NRF_I2S_Type const * p_i2s,
                                              nrf_i2s_int_mask_t i2s_int);

/**
 * @brief Function for enabling the I2S peripheral.
 *
 * @param[in] p_i2s I2S instance.
 */
__STATIC_INLINE void nrf_i2s_enable(NRF_I2S_Type * p_i2s);

/**
 * @brief Function for disabling the I2S peripheral.
 *
 * @param[in] p_i2s I2S instance.
 */
__STATIC_INLINE void nrf_i2s_disable(NRF_I2S_Type * p_i2s);

/**
 * @brief Function for configuring I2S pins.
 *
 * Usage of the SDOUT, SDIN, and MCK signals is optional.
 * If a given signal is not needed, pass the @ref NRF_I2S_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_i2s     I2S instance.
 * @param[in] sck_pin   SCK pin number.
 * @param[in] lrck_pin  LRCK pin number.
 * @param[in] mck_pin   MCK pin number.
 * @param[in] sdout_pin SDOUT pin number.
 * @param[in] sdin_pin  SDIN pin number.
 */
__STATIC_INLINE void nrf_i2s_pins_set(NRF_I2S_Type * p_i2s,
                                      uint32_t sck_pin,
                                      uint32_t lrck_pin,
                                      uint32_t mck_pin,
                                      uint32_t sdout_pin,
                                      uint32_t sdin_pin);

/**
 * @brief Function for setting the I2S peripheral configuration.
 *
 * @param[in] p_i2s        I2S instance.
 * @param[in] mode         Mode of operation (master or slave).
 * @param[in] format       I2S frame format.
 * @param[in] alignment    Alignment of sample within a frame.
 * @param[in] sample_width Sample width.
 * @param[in] channels     Enabled channels.
 * @param[in] mck_setup    Master clock generator setup.
 * @param[in] ratio        MCK/LRCK ratio.
 *
 * @retval true  If the configuration has been set successfully.
 * @retval false If the requested configuration is not allowed.
 */
__STATIC_INLINE bool nrf_i2s_configure(NRF_I2S_Type * p_i2s,
                                       nrf_i2s_mode_t     mode,
                                       nrf_i2s_format_t   format,
                                       nrf_i2s_align_t    alignment,
                                       nrf_i2s_swidth_t   sample_width,
                                       nrf_i2s_channels_t channels,
                                       nrf_i2s_mck_t      mck_setup,
                                       nrf_i2s_ratio_t    ratio);

/**
 * @brief Function for setting up the I2S transfer.
 *
 * This function sets up the RX and TX buffers and enables reception and/or
 * transmission accordingly. If the transfer in a given direction is not
 * required, pass NULL instead of the pointer to the corresponding buffer.
 *
 * @param[in] p_i2s       I2S instance.
 * @param[in] size        Size of the buffers (in 32-bit words).
 * @param[in] p_rx_buffer Pointer to the receive buffer.
 *                        Pass NULL to disable reception.
 * @param[in] p_tx_buffer Pointer to the transmit buffer.
 *                        Pass NULL to disable transmission.
 */
__STATIC_INLINE void nrf_i2s_transfer_set(NRF_I2S_Type * p_i2s,
                                          uint16_t         size,
                                          uint32_t *       p_rx_buffer,
                                          uint32_t const * p_tx_buffer);

/**
 * @brief Function for setting the pointer to the receive buffer.
 *
 * @note The size of the buffer can be set only by calling
 *       @ref nrf_i2s_transfer_set.
 *
 * @param[in] p_i2s    I2S instance.
 * @param[in] p_buffer Pointer to the receive buffer.
 */
__STATIC_INLINE void nrf_i2s_rx_buffer_set(NRF_I2S_Type * p_i2s,
                                           uint32_t * p_buffer);

/**
 * @brief Function for getting the pointer to the receive buffer.
 *
 * @param[in] p_i2s I2S instance.
 *
 * @return Pointer to the receive buffer.
 */
__STATIC_INLINE uint32_t * nrf_i2s_rx_buffer_get(NRF_I2S_Type const * p_i2s);

/**
 * @brief Function for setting the pointer to the transmit buffer.
 *
 * @note The size of the buffer can be set only by calling
 *       @ref nrf_i2s_transfer_set.
 *
 * @param[in] p_i2s    I2S instance.
 * @param[in] p_buffer Pointer to the transmit buffer.
 */
__STATIC_INLINE void nrf_i2s_tx_buffer_set(NRF_I2S_Type * p_i2s,
                                           uint32_t const * p_buffer);

/**
 * @brief Function for getting the pointer to the transmit buffer.
 *
 * @param[in] p_i2s I2S instance.
 *
 * @return Pointer to the transmit buffer.
 */
__STATIC_INLINE uint32_t * nrf_i2s_tx_buffer_get(NRF_I2S_Type const * p_i2s);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_i2s_task_trigger(NRF_I2S_Type * p_i2s,
                                          nrf_i2s_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_i2s + (uint32_t)task)) = 0x1UL;
}

__STATIC_INLINE uint32_t nrf_i2s_task_address_get(NRF_I2S_Type const * p_i2s,
                                                  nrf_i2s_task_t task)
{
    return ((uint32_t)p_i2s + (uint32_t)task);
}

__STATIC_INLINE void nrf_i2s_event_clear(NRF_I2S_Type * p_i2s,
                                         nrf_i2s_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_i2s + (uint32_t)event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_i2s + (uint32_t)event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_i2s_event_check(NRF_I2S_Type const * p_i2s,
                                         nrf_i2s_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_i2s + (uint32_t)event);
}

__STATIC_INLINE uint32_t nrf_i2s_event_address_get(NRF_I2S_Type const * p_i2s,
                                                   nrf_i2s_event_t event)
{
    return ((uint32_t)p_i2s + (uint32_t)event);
}

__STATIC_INLINE void nrf_i2s_int_enable(NRF_I2S_Type * p_i2s, uint32_t mask)
{
    p_i2s->INTENSET = mask;
}

__STATIC_INLINE void nrf_i2s_int_disable(NRF_I2S_Type * p_i2s, uint32_t mask)
{
    p_i2s->INTENCLR = mask;
}

__STATIC_INLINE bool nrf_i2s_int_enable_check(NRF_I2S_Type const * p_i2s,
                                              nrf_i2s_int_mask_t i2s_int)
{
    return (bool)(p_i2s->INTENSET & i2s_int);
}

__STATIC_INLINE void nrf_i2s_enable(NRF_I2S_Type * p_i2s)
{
    p_i2s->ENABLE = (I2S_ENABLE_ENABLE_Enabled << I2S_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_i2s_disable(NRF_I2S_Type * p_i2s)
{
    p_i2s->ENABLE = (I2S_ENABLE_ENABLE_Disabled << I2S_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_i2s_pins_set(NRF_I2S_Type * p_i2s,
                                      uint32_t sck_pin,
                                      uint32_t lrck_pin,
                                      uint32_t mck_pin,
                                      uint32_t sdout_pin,
                                      uint32_t sdin_pin)
{
    p_i2s->PSEL.SCK   = sck_pin;
    p_i2s->PSEL.LRCK  = lrck_pin;
    p_i2s->PSEL.MCK   = mck_pin;
    p_i2s->PSEL.SDOUT = sdout_pin;
    p_i2s->PSEL.SDIN  = sdin_pin;
}

__STATIC_INLINE bool nrf_i2s_configure(NRF_I2S_Type * p_i2s,
                                       nrf_i2s_mode_t     mode,
                                       nrf_i2s_format_t   format,
                                       nrf_i2s_align_t    alignment,
                                       nrf_i2s_swidth_t   sample_width,
                                       nrf_i2s_channels_t channels,
                                       nrf_i2s_mck_t      mck_setup,
                                       nrf_i2s_ratio_t    ratio)
{
    if (mode == NRF_I2S_MODE_MASTER)
    {
        // The MCK/LRCK ratio shall be a multiple of 2 * sample width.
        if (((sample_width == NRF_I2S_SWIDTH_16BIT) &&
                 (ratio == NRF_I2S_RATIO_48X))
            ||
            ((sample_width == NRF_I2S_SWIDTH_24BIT) &&
                ((ratio == NRF_I2S_RATIO_32X)  ||
                 (ratio == NRF_I2S_RATIO_64X)  ||
                 (ratio == NRF_I2S_RATIO_128X) ||
                 (ratio == NRF_I2S_RATIO_256X) ||
                 (ratio == NRF_I2S_RATIO_512X))))
        {
            return false;
        }
    }

    p_i2s->CONFIG.MODE     = mode;
    p_i2s->CONFIG.FORMAT   = format;
    p_i2s->CONFIG.ALIGN    = alignment;
    p_i2s->CONFIG.SWIDTH   = sample_width;
    p_i2s->CONFIG.CHANNELS = channels;
    p_i2s->CONFIG.RATIO    = ratio;

    if (mck_setup == NRF_I2S_MCK_DISABLED)
    {
        p_i2s->CONFIG.MCKEN =
            (I2S_CONFIG_MCKEN_MCKEN_Disabled << I2S_CONFIG_MCKEN_MCKEN_Pos);
    }
    else
    {
        p_i2s->CONFIG.MCKFREQ = mck_setup;
        p_i2s->CONFIG.MCKEN =
            (I2S_CONFIG_MCKEN_MCKEN_Enabled << I2S_CONFIG_MCKEN_MCKEN_Pos);
    }

    return true;
}

__STATIC_INLINE void nrf_i2s_transfer_set(NRF_I2S_Type * p_i2s,
                                          uint16_t         size,
                                          uint32_t *       p_buffer_rx,
                                          uint32_t const * p_buffer_tx)
{
    p_i2s->RXTXD.MAXCNT = size;

    nrf_i2s_rx_buffer_set(p_i2s, p_buffer_rx);
    p_i2s->CONFIG.RXEN = (p_buffer_rx != NULL) ? 1 : 0;

    nrf_i2s_tx_buffer_set(p_i2s, p_buffer_tx);
    p_i2s->CONFIG.TXEN = (p_buffer_tx != NULL) ? 1 : 0;
}

__STATIC_INLINE void nrf_i2s_rx_buffer_set(NRF_I2S_Type * p_i2s,
                                           uint32_t * p_buffer)
{
    p_i2s->RXD.PTR = (uint32_t)p_buffer;
}

__STATIC_INLINE uint32_t * nrf_i2s_rx_buffer_get(NRF_I2S_Type const * p_i2s)
{
    return (uint32_t *)(p_i2s->RXD.PTR);
}

__STATIC_INLINE void nrf_i2s_tx_buffer_set(NRF_I2S_Type * p_i2s,
                                           uint32_t const * p_buffer)
{
    p_i2s->TXD.PTR = (uint32_t)p_buffer;
}

__STATIC_INLINE uint32_t * nrf_i2s_tx_buffer_get(NRF_I2S_Type const * p_i2s)
{
    return (uint32_t *)(p_i2s->TXD.PTR);
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_I2S_H__
