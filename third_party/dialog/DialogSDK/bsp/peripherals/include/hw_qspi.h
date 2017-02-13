/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup QSPI
 * \{
 * \brief QSPI Flash Memory Controller
 */

/**
 *****************************************************************************************
 *
 * @file hw_qspi.h
 *
 * @brief Definition of API for the QSPI Low Level Driver.
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

#ifndef HW_QSPI_H_
#define HW_QSPI_H_

#if dg_configUSE_HW_QSPI

#include <stdbool.h>
#include <stdint.h>
#include <sdk_defs.h>


/**
 * \brief Get the mask of a field of an QSPIC register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to access
 *
 */
#define HW_QSPIC_REG_FIELD_MASK(reg, field) \
        (QSPIC_QSPIC_##reg##_REG_##QSPIC_##field##_Msk)

/**
 * \brief Get the bit position of a field of an QSPIC register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to access
 *
 */
#define HW_QSPIC_REG_FIELD_POS(reg, field) \
        (QSPIC_QSPIC_##reg##_REG_##QSPIC_##field##_Pos)

/**
 * \brief Get the value of a field of an QSPIC register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to write
 *
 * \return the value of the register field
 *
 */
#define HW_QSPIC_REG_GETF(reg, field) \
        ((QSPIC->QSPIC_##reg##_REG & (QSPIC_QSPIC_##reg##_REG_##QSPIC_##field##_Msk)) >> (QSPIC_QSPIC_##reg##_REG_##QSPIC_##field##_Pos))

/**
 * \brief Set the value of a field of an QSPIC register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to write
 * \param [in] new_val is the value to write
 *
 */
#define HW_QSPIC_REG_SETF(reg, field, new_val) \
        QSPIC->QSPIC_##reg##_REG = ((QSPIC->QSPIC_##reg##_REG & ~(QSPIC_QSPIC_##reg##_REG_##QSPIC_##field##_Msk)) | \
        ((QSPIC_QSPIC_##reg##_REG_##QSPIC_##field##_Msk) & ((new_val) << (QSPIC_QSPIC_##reg##_REG_##QSPIC_##field##_Pos))))


extern uint8_t dummy_num[];

/*
 * ENUMERATION DEFINITIONS
 *****************************************************************************************
 */

/**
 * \brief Bus mode
 *
 */
typedef enum {
        HW_QSPI_BUS_MODE_SINGLE,   /**< Bus mode in single mode */
        HW_QSPI_BUS_MODE_DUAL,     /**< Bus mode in dual mode */
        HW_QSPI_BUS_MODE_QUAD      /**< Bus mode in quad mode */
} HW_QSPI_BUS_MODE;

/**
 * \brief Flash memory address size
 *
 */
typedef enum {
        HW_QSPI_ADDR_SIZE_24,      /**< QSPI flash memory uses 24 bits address */
        HW_QSPI_ADDR_SIZE_32       /**< QSPI flash memory uses 32 bits address */
} HW_QSPI_ADDR_SIZE;

/**
 * \brief Idle clock state
 *
 */
typedef enum {
        HW_QSPI_POL_LOW = 0,        /**< SPI clock will be low at idle */
        HW_QSPI_POL_HIGH = 1        /**< SPI clock will be high at idle */
} HW_QSPI_POL;

/**
 * \brief Type of QSPI_CLK edge for sampling received data
 *
 */
typedef enum {
        HW_QSPI_SAMPLING_EDGE_POSITIVE = 0,        /**< Sample the received data with the positive edge of the QSPI_SCK */
        HW_QSPI_SAMPLING_EDGE_NEGATIVE = 1         /**< Sample the received data with the negative edge of the QSPI_SCK */
} HW_QSPI_SAMPLING_EDGE;

/**
 * \brief Selected data size of a wrapping burst
 *
 */
typedef enum {
        HW_QSPI_WRAP_SIZE_8BITS = 0,       /**< Byte access (8-bits) */
        HW_QSPI_WRAP_SIZE_16BITS = 1,      /**< Half word access (8-bits) */
        HW_QSPI_WRAP_SIZE_32BITS = 2,      /**< Word access (8-bits) */
} HW_QSPI_WRAP_SIZE;

/**
 * \brief Selected data length of a wrapping burst
 *
 */
typedef enum {
        HW_QSPI_WRAP_LEN_4BEAT = 0,        /**< 4 beat wrapping burst */
        HW_QSPI_WRAP_LEN_8BEAT = 1,        /**< 8 beat wrapping burst */
        HW_QSPI_WRAP_LEN_16BEAT = 2,       /**< 16 beat wrapping burst */
} HW_QSPI_WRAP_LEN;

/**
 * \brief Size of Burst Break Sequence
 *
 */
typedef enum {
        HW_QSPI_BREAK_SEQ_SIZE_1B = 0,     /**< One byte */
        HW_QSPI_BREAK_SEQ_SIZE_2B = 1      /**< Two bytes */
} HW_QSPI_BREAK_SEQ_SIZE;

/**
 * \brief QSPI pads slew rate control
 *
 */
typedef enum {
        HW_QSPI_SLEW_RATE_0,       /**< xx V/ns (weak) */
        HW_QSPI_SLEW_RATE_1,       /**< xx V/ns */
        HW_QSPI_SLEW_RATE_2,       /**< xx V/ns */
        HW_QSPI_SLEW_RATE_3        /**< xx V/ns (strong) */
} HW_QSPI_SLEW_RATE;

/**
 * \brief QSPI pads drive current
 *
 */
typedef enum {
        HW_QSPI_DRIVE_CURRENT_4,   /**< 4 mA */
        HW_QSPI_DRIVE_CURRENT_8,   /**< 8 mA */
        HW_QSPI_DRIVE_CURRENT_12,  /**< 12 mA */
        HW_QSPI_DRIVE_CURRENT_16,  /**< 16 mA */
} HW_QSPI_DRIVE_CURRENT;

/**
 * \brief QSPI clock divider setting
 *
 */
typedef enum {
        HW_QSPI_DIV_1 = 0,      /**< divide by 1 */
        HW_QSPI_DIV_2 = 1,      /**< divide by 2 */
        HW_QSPI_DIV_4 = 2,      /**< divide by 4 */
        HW_QSPI_DIV_8 = 3       /**< divide by 8 */
} HW_QSPI_DIV;

/**
 * \brief QSPI configuration
 *
 */
typedef struct {
        HW_QSPI_ADDR_SIZE address_size;
        HW_QSPI_POL idle_clock;
        HW_QSPI_SAMPLING_EDGE sampling_edge;
} qspi_config;

/**
 * \brief Enable CS on QSPI bus
 *
 * Use this in manual mode.
 *
 */
static inline void hw_qspi_cs_enable(void) __attribute__((always_inline));

static inline void hw_qspi_cs_enable(void)
{
        QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_EN_CS_Msk;
}

/**
 * \brief Disable CS on QSPI bus
 *
 * Use this in manual mode.
 *
 */
static inline void hw_qspi_cs_disable(void) __attribute__((always_inline));

static inline void hw_qspi_cs_disable(void)
{
        QSPIC->QSPIC_CTRLBUS_REG = QSPIC_QSPIC_CTRLBUS_REG_QSPIC_DIS_CS_Msk;
}

/**
 * \brief Select QSPI bus mode
 *
 * Use this in manual mode.
 *
 * \note Selecting QUAD mode will automatically configure pins IO2 and IO3 to input.
 *
 * \param [in] mode selects single/dual/quad mode for QSPI bus
 *
 */
void hw_qspi_set_bus_mode(HW_QSPI_BUS_MODE mode);

/**
 * \brief Check is SPI Bus is busy
 *
 * \return 0 - SPI Bus is idle,
 *         1 - SPI Bus is active. ReadData, WriteData or DummyData activity is in progress
 *
 */
static inline uint8_t hw_qspi_is_busy(void) __attribute__((always_inline));

static inline uint8_t hw_qspi_is_busy(void)
{
        return (uint8_t) QSPIC->QSPIC_STATUS_REG;
}

/**
 * \brief Generate 32 bits read transfer on QSPI bus
 *
 * The data is transferred using the selected mode of the SPI bus (SPI, Dual
 * SPI, Quad SPI).
 *
 * \return data read during read transfer
 *
 */
static inline uint32_t hw_qspi_read32(void) __attribute__((always_inline));

static inline uint32_t hw_qspi_read32(void)
{
        return QSPIC->QSPIC_READDATA_REG;
}

/**
 * \brief Generate 16 bits read transfer on QSPI bus
 *
 * The data is transferred using the selected mode of the SPI bus (SPI, Dual
 * SPI, Quad SPI).
 *
 * \return data read during read transfer
 *
 */
static inline uint16_t hw_qspi_read16(void) __attribute__((always_inline));

static inline uint16_t hw_qspi_read16(void)
{
        return *((volatile uint16_t *) &(QSPIC->QSPIC_READDATA_REG));
}

/**
 * \brief Generate 8 bits read transfer on QSPI bus
 *
 * The data is transferred using the selected mode of the SPI bus (SPI, Dual
 * SPI, Quad SPI).
 *
 * \return data read during read transfer
 *
 */
static inline uint8_t hw_qspi_read8(void) __attribute__((always_inline));

static inline uint8_t hw_qspi_read8(void)
{
        return *((volatile uint8_t *) &(QSPIC->QSPIC_READDATA_REG));
}

/**
 * \brief Generate 32 bits write transfer on QSPI bus
 *
 * The data is transferred using the selected mode of the SPI bus (SPI, Dual
 * SPI, Quad SPI).
 *
 * param [in] data data to transfer
 *
 */
static inline void hw_qspi_write32(uint32_t data) __attribute__((always_inline));

static inline void hw_qspi_write32(uint32_t data)
{
        QSPIC->QSPIC_WRITEDATA_REG = data;
}

/**
 * \brief Generate 16 bits write transfer on QSPI bus
 *
 * The data is transferred using the selected mode of the SPI bus (SPI, Dual
 * SPI, Quad SPI).
 *
 * param [in] data data to transfer
 *
 */
static inline void hw_qspi_write16(uint16_t data) __attribute__((always_inline));

static inline void hw_qspi_write16(uint16_t data)
{
        *((volatile uint16_t *) &(QSPIC->QSPIC_WRITEDATA_REG)) = data;
}

/**
 * \brief Generate 8 bits write transfer on QSPI bus
 *
 * The data is transferred using the selected mode of the SPI bus (SPI, Dual
 * SPI, Quad SPI).
 *
 * param [in] data data to transfer
 *
 */
static inline void hw_qspi_write8(uint8_t data) __attribute__((always_inline));

static inline void hw_qspi_write8(uint8_t data)
{
        *((volatile uint8_t *) &(QSPIC->QSPIC_WRITEDATA_REG)) = data;
}

/**
 * \brief Generate a clock pulses to the SPI bus for 32 bits transfer
 *
 * During this activity in the SPI bus, the QSPI_IOx data pads are in hi-z state.
 * Number of pulses depends on selected mode of the SPI bus (SPI, Dual SPI, Quad SPI).
 *
 */
static inline void hw_qspi_dummy32(void) __attribute__((always_inline));

static inline void hw_qspi_dummy32(void)
{
        QSPIC->QSPIC_DUMMYDATA_REG = 0;
}

/**
 * \brief Generate a clock pulses to the SPI bus for 16 bits transfer
 *
 * During this activity in the SPI bus, the QSPI_IOx data pads are in hi-z state.
 * Number of pulses depends on selected mode of the SPI bus (SPI, Dual SPI, Quad SPI).
 *
 */
static inline void hw_qspi_dummy16(void) __attribute__((always_inline));

static inline void hw_qspi_dummy16(void)
{
        *((volatile uint16_t *) &(QSPIC->QSPIC_DUMMYDATA_REG)) = 0;
}

/**
 * \brief Generate a clock pulses to the SPI bus for 8 bits transfer
 *
 * During this activity in the SPI bus, the QSPI_IOx data pads are in hi-z state.
 * Number of pulses depends on selected mode of the SPI bus (SPI, Dual SPI, Quad SPI).
 *
 */
static inline void hw_qspi_dummy8(void) __attribute__((always_inline));

static inline void hw_qspi_dummy8(void)
{
        *((volatile uint8_t *) &(QSPIC->QSPIC_DUMMYDATA_REG)) = 0;
}

/**
 * \brief Specifies address with that flash memory uses
 *
 * The controller uses 32 of 24 bits for address during Auto mode transfer.
 *
 * \param [in] size selects 32 or 24 address size
 *
 */
static inline void hw_qspi_set_address_size(HW_QSPI_ADDR_SIZE size) __attribute__((always_inline));

static inline void hw_qspi_set_address_size(HW_QSPI_ADDR_SIZE size)
{
        HW_QSPIC_REG_SETF(CTRLMODE, USE_32BA, (size == HW_QSPI_ADDR_SIZE_32 ? 1 : 0));
}

/**
 * \brief Get address size that flash memory uses
 *
 * The controller uses 32 of 24 bits for address during Auto mode transfer.
 *
 * \return currently selected address size
 *
 * \sa hw_qspi_set_address_size
 *
 */
static inline HW_QSPI_ADDR_SIZE hw_qspi_get_address_size(void) __attribute__((always_inline));

static inline HW_QSPI_ADDR_SIZE hw_qspi_get_address_size(void)
{
        return (HW_QSPI_ADDR_SIZE) HW_QSPIC_REG_GETF(CTRLMODE, USE_32BA);
}

/**
 * \brief Controls translation of burst accesses form AMBA bus to the QSPI bus
 *
 * \param [in] force 0 - controller translates a burst access on the AMBA bus
 *                       to a burst access on the QSPI bus. That results to a minimum
 *                       command/address phases, but the QSPI_CS is low for as
 *                       long as the access occures,
 *                   1 - controller will split a burst access on the AMBA bus
 *                       into single accesses on the qspi bus. This results to a separate
 *                       read command to the FLASH memory foreach data
 *                       required. A 4-beat word incremental AMBA access will be
 *                       split into 4 different sequences of reading (command/address/
 *                       extra clock/read data). QSPI_CS will be only high while a qspi
 *                       access occures. This results to lower power dissipation with
 *                       respect to QSPIC_FORCENSEQ_EN=0 at cost of performance
 *
 */
static inline void hw_qspi_force_nseq(uint8_t force) __attribute__((always_inline));

static inline void hw_qspi_force_nseq(uint8_t force)
{
        HW_QSPIC_REG_SETF(CTRLMODE, FORCENSEQ_EN, force);
}

/**
 * \brief Enable or disable auto mode on QSPI
 *
 * \note Selecting auto mode when any command previously configured has chosen
 * QUAD mode for any phase will automatically configure pins IO2 and IO3 to input.
 *
 * \param [in] automode true auto mode selected,
 *                      false manual mode selected
 *
 * \sa hw_qspi_set_read_instruction
 * \sa hw_qspi_set_read_status_instruction
 * \sa hw_qspi_set_suspend_resume_instructions
 * \sa hw_qspi_set_write_enable_instruction
 * \sa hw_qspi_set_extra_byte
 * \sa hw_qspi_set_erase_instruction
 * \sa hw_qspi_set_break_sequence
 *
 */
void hw_qspi_set_automode(bool automode);

/**
 * \brief Read automode state
 *
 * \return true auto mode is selected,
 *         false manual mode is selected
 *
 */
static inline bool hw_qspi_get_automode(void) __attribute__((always_inline));

static inline bool hw_qspi_get_automode(void)
{
        return HW_QSPIC_REG_GETF(CTRLMODE, AUTO_MD);
}

/**
 * \brief Get read pipe clock delay
 *
 * \return read pipe clock delay relative to the falling edge of QSPI_SCK
 *
 */
static inline uint8_t hw_qspi_get_read_pipe_clock_delay(void) __attribute__((always_inline));

static inline uint8_t hw_qspi_get_read_pipe_clock_delay(void)
{
        return HW_QSPIC_REG_GETF(CTRLMODE, PCLK_MD);
}

/**
 * \brief Set read pipe clock delay
 *
 * \param [in] delay read pipe clock delay relative to the falling edge of QSPI_SCK
 *                   value should be in range 0..7
 *
 */
static inline void hw_qspi_set_read_pipe_clock_delay(uint8_t delay) __attribute__((always_inline));

static inline void hw_qspi_set_read_pipe_clock_delay(uint8_t delay)
{
        HW_QSPIC_REG_SETF(CTRLMODE, PCLK_MD, delay);
}

/**
 * \brief Check if read pipe is enabled
 *
 * \return 1 if read pipe is enabled, 0 otherwise
 *
 */
static inline uint8_t hw_qspi_is_read_pipe_clock_enabled(void) __attribute__((always_inline));

static inline uint8_t hw_qspi_is_read_pipe_clock_enabled(void)
{
        return HW_QSPIC_REG_GETF(CTRLMODE, RPIPE_EN);
}

/**
 * \brief Enable read pipe
 *
 * \param [in] enable 1 enable read pipe,
 *                    0 disable read pipe
 *
 */
static inline void hw_qspi_enable_readpipe(uint8_t enable) __attribute__((always_inline));

static inline void hw_qspi_enable_readpipe(uint8_t enable)
{
        HW_QSPIC_REG_SETF(CTRLMODE, RPIPE_EN, enable);
}

/**
 * \brief Get read sampling edge
 *
 * \return type of QSPI_CLK edge for sampling received data
 *
 */
static inline HW_QSPI_SAMPLING_EDGE hw_qspi_get_read_sampling_edge(void) __attribute__((always_inline));

static inline HW_QSPI_SAMPLING_EDGE hw_qspi_get_read_sampling_edge(void)
{
        return HW_QSPIC_REG_GETF(CTRLMODE, RXD_NEG);
}

/**
 * \brief Set read sampling edge
 *
 * \param [in] edge sets whether read samples or taken on rising or falling edge
 *                  of QSPI_SCK
 *
 */
static inline void hw_qspi_set_read_sampling_edge(HW_QSPI_SAMPLING_EDGE edge) __attribute__((always_inline));

static inline void hw_qspi_set_read_sampling_edge(HW_QSPI_SAMPLING_EDGE edge)
{
        HW_QSPIC_REG_SETF(CTRLMODE, RXD_NEG, edge);
}

/**
 * \brief Check if hready signal is used
 *
 * \return 0 - when wait states are not added via hready signal during access to
 *             QSPIC_WRITEDATA, QSPIC_READDATA and QSPIC_DUMMYDATA registers,
 *         1 - when wait states are added via hready signal during access to
 *             QSPIC_WRITEDATA, QSPIC_READDATA and QSPIC_DUMMYDATA registers.
 *             In this case read read QSPI_STATUS register to check the end of
 *             activity at the SPI bus
 *
 */
static inline uint8_t hw_qspi_is_hready_enabled(void) __attribute__((always_inline));

static inline uint8_t hw_qspi_is_hready_enabled(void)
{
        return HW_QSPIC_REG_GETF(CTRLMODE, HRDY_MD);
}

/**
 * \brief Enable/disable adding wait states during register access
 *
 * \param [in] enable 0 - wait states will be added in hready signal during access to
 *                        QSPIC_WRITEDATA, QSPIC_READDATA and QSPIC_DUMMYDATA registers
 *
 */
static inline void hw_qspi_enable_hready(uint8_t enable) __attribute__((always_inline));

static inline void hw_qspi_enable_hready(uint8_t enable)
{
        HW_QSPIC_REG_SETF(CTRLMODE, HRDY_MD, enable);
}

/**
 * \brief Get clock mode
 *
 * \return 0 - SPI mode 0 is used for QSPI_CLK, the QSPI_SCK is low when QSPI_CS is high (idle),
 *         1 - SPI mode 3 is used for QSPI_CLK, the QSPI_SCK is high when QSPI_CS is high (idle)
 *
 */
static inline uint8_t hw_qspi_get_clock_mode(void) __attribute__((always_inline));

static inline uint8_t hw_qspi_get_clock_mode(void)
{
        return HW_QSPIC_REG_GETF(CTRLMODE, CLK_MD);
}

/**
 * \brief Set clock mode
 *
 * \param [in] mode HW_SPI_POL_LOW - SPI mode 0 is used for QSPI_CLK,
 *                      The QSPI_SCK is low when QSPI_CS is high (idle),
 *                  HW_SPI_POL_HIGH - SPI mode 3 is used for QSPI_CLK,
 *                      The QSPI_SCK is high when QSPI_CS is high (idle)
 *
 */
static inline void hw_qspi_set_clock_mode(HW_QSPI_POL mode) __attribute__((always_inline));

static inline void hw_qspi_set_clock_mode(HW_QSPI_POL mode)
{
        HW_QSPIC_REG_SETF(CTRLMODE, CLK_MD, mode);
}

/**
 * \brief Set IO2 direction
 *
 * \param [in] output QSPI_IO2 output enable. Use this only in SPI or Dual SPI
 *                    mode to control /WP signal. When the Auto Mode is selected
 *                    and the QUAD SPI is used, set this to false.
 *                    false: The QSPI_IO2 pad is input
 *                    true: The QSPI_IO2 pad is output
 *
 */
#if (dg_configFLASH_POWER_DOWN == 1)
__attribute__((section("text_retained")))
#endif
static inline void hw_qspi_set_io2_output(bool output) __attribute__((always_inline));

static inline void hw_qspi_set_io2_output(bool output)
{
        HW_QSPIC_REG_SETF(CTRLMODE, IO2_OEN, output);
}

/**
 * \brief Set IO3 direction
 *
 * \param [in] output Use this only in SPI or Dual SPI mode to control /HOLD signal.
 *                    When the Auto Mode is selected and the QUAD SPI is used,
 *                    set this to false.
 *                    false: The QSPI_IO3 pad is input,
 *                    true: The QSPI_IO3 pad is output
 *
 */
#if (dg_configFLASH_POWER_DOWN == 1)
__attribute__((section("text_retained")))
#endif
static inline void hw_qspi_set_io3_output(bool output) __attribute__((always_inline));

static inline void hw_qspi_set_io3_output(bool output)
{
        HW_QSPIC_REG_SETF(CTRLMODE, IO3_OEN, output);
}

/**
 * \brief Set state for IO2 when it's configured as output
 *
 * \param [in] val QSPI_IO2 value to set (0 or 1)
 *
 * \sa hw_qspi_set_io2_direction
 *
 */
static inline void hw_qspi_set_io2(uint8_t val) __attribute__((always_inline));

static inline void hw_qspi_set_io2(uint8_t val)
{
        HW_QSPIC_REG_SETF(CTRLMODE, IO2_DAT, val);
}

/**
 * \brief Set state for IO3 when it's configured as output
 *
 * \param [in] val QSPI_IO3 value to set (0 or 1)
 *
 * \sa hw_qspi_set_io3_direction
 *
 */
static inline void hw_qspi_set_io3(uint8_t val) __attribute__((always_inline));

static inline void hw_qspi_set_io3(uint8_t val)
{
        HW_QSPIC_REG_SETF(CTRLMODE, IO3_DAT, val);
}

/**
 * \brief Read state of IO2 when not in QUAD mode
 *
 * \return value of IO2 pin (0 or 1)
 *
 */
static inline uint8_t hw_qspi_get_io2(void) __attribute__((always_inline));

static inline uint8_t hw_qspi_get_io2(void)
{
        return HW_QSPIC_REG_GETF(CTRLMODE, IO2_DAT);
}

/**
 * \brief Read state of IO3 when not in QUAD mode
 *
 * \return value of IO3 pin (0 or 1)
 *
 */
static inline uint8_t hw_qspi_get_io3(void) __attribute__((always_inline));

static inline uint8_t hw_qspi_get_io3(void)
{
        return HW_QSPIC_REG_GETF(CTRLMODE, IO3_DAT);
}

/**
 * \brief Set read instructions for QSPI flash
 *
 * This function sets up instruction to be sent to flash memory when data is requested
 * on AHB bus.
 *
 * \param [in] inst instruction for Incremental Burst or Single read access.
 *                  This value is the selected instruction at the cases of incremental
 *                  burst or single read access. Also this value is used when a wrapping
 *                  burst is not supported.
 * \param [in] send_once 0 - transmit instruction at any burst access,
 *                       1 - transmit instruction only in the first access after the
 *                       selection of Auto Mode.
 * \param [in] dummy_count number of dummy bytes to send 0..4
 * \param [in] inst_phase describes the mode of the SPI bus during the instruction phase
 * \param [in] addr_phase describes the mode of the SPI bus during the address phase
 * \param [in] dummy_phase describes the mode of the SPI bus during the dummy phase
 * \param [in] data_phase describes the mode of the SPI bus during the data phase
 *
 * \sa hw_qspi_set_extra_byte
 * \sa hw_qspi_set_wrapping_burst_instruction
 *
 */
static inline void hw_qspi_set_read_instruction(uint8_t inst,
                                                uint8_t send_once,
                                                uint8_t dummy_count,
                                                HW_QSPI_BUS_MODE inst_phase,
                                                HW_QSPI_BUS_MODE addr_phase,
                                                HW_QSPI_BUS_MODE dummy_phase,
                                                HW_QSPI_BUS_MODE data_phase)
                                                __attribute((always_inline));

static inline void hw_qspi_set_read_instruction(uint8_t inst,
                                                uint8_t send_once,
                                                uint8_t dummy_count,
                                                HW_QSPI_BUS_MODE inst_phase,
                                                HW_QSPI_BUS_MODE addr_phase,
                                                HW_QSPI_BUS_MODE dummy_phase,
                                                HW_QSPI_BUS_MODE data_phase)
{
        QSPIC->QSPIC_BURSTCMDA_REG =
                BITS32(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_INST, inst) |
                BITS32(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_INST_TX_MD, inst_phase) |
                BITS32(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_ADR_TX_MD, addr_phase) |
                BITS32(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_DMY_TX_MD, dummy_phase);

        QSPIC->QSPIC_BURSTCMDB_REG =
                BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_DAT_RX_MD, data_phase) |
                BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_INST_MD, send_once) |
                BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_DMY_FORCE, (dummy_count == 3 ? 1 : 0)) |
                BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_DMY_NUM, dummy_num[dummy_count]);
}

/**
 * \brief Set wrapping burst read instructions for QSPI flash
 *
 * Calling this function will set up wrapping burst instruction to use.
 * Use this feature only when the serial FLASH memory supports a special
 * instruction for wrapping burst access.
 *
 * \param [in] inst instruction for Wrapping Burst
 * \param [in] len describes the selected length of a wrapping burst
 * \param [in] size describes the selected data size of a wrapping burst
 *
 */
void hw_qspi_set_wrapping_burst_instruction(uint8_t inst, HW_QSPI_WRAP_LEN len,
                                                                        HW_QSPI_WRAP_SIZE size);

/**
 * \brief Set extra byte to use in read instruction
 *
 * \param [in] extra_byte the value of an extra byte which will be transferred after
 *                        address. This byte is used for telling memory if it should stay
 *                        in continuous read mode or wait for normal instruction after CS
 *                        goes inactive
 * \param [in] bus_mode describes the mode of the SPI bus during the extra byte phase.
 * \param [in] half_disable_out 1 - disable (hi-z) output during the transmission
 *                              of bits [3:0] of extra byte
 *
 * \sa hw_qspi_set_read_instruction
 *
 */
static inline void hw_qspi_set_extra_byte(uint8_t extra_byte, HW_QSPI_BUS_MODE bus_mode,
                        uint8_t half_disable_out) __attribute__((always_inline));

static inline void hw_qspi_set_extra_byte(uint8_t extra_byte, HW_QSPI_BUS_MODE bus_mode,
                                                                        uint8_t half_disable_out)
{
        QSPIC->QSPIC_BURSTCMDA_REG =
                (QSPIC->QSPIC_BURSTCMDA_REG &
                        ~(REG_MSK(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_EXT_BYTE) |
                                REG_MSK(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_EXT_TX_MD))) |
                BITS32(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_EXT_BYTE, extra_byte) |
                BITS32(QSPIC, QSPIC_BURSTCMDA_REG, QSPIC_EXT_TX_MD, bus_mode);

        QSPIC->QSPIC_BURSTCMDB_REG =
                (QSPIC->QSPIC_BURSTCMDB_REG &
                        ~(REG_MSK(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_EXT_BYTE_EN) |
                                REG_MSK(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_EXT_HF_DS))) |
                BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_EXT_BYTE_EN, 1) |
                BITS32(QSPIC, QSPIC_BURSTCMDB_REG, QSPIC_EXT_HF_DS, half_disable_out ? 1 : 0);
}

/**
 * \brief Set dummy byte count
 *
 * Number of dummy bytes to send when read instruction is executed.
 *
 * \param [in] count number of dummy bytes to send 0..4
 *
 */
void hw_qspi_set_dummy_bytes_count(uint8_t count);

/**
 * \brief Set number of clock when CS stays high
 *
 * \param [in] clock_count between the transmission of two different instructions to the
 *                         flash memory, the qspi bus stays in idle state (QSPI_CS high)
 *                         for at least this number of spi clock cycles. See the QSPIC_ERS_CS_HI
 *                         register for an exception
 *
 */
static inline void hw_qspi_set_min_cs_high(uint8_t clock_count) __attribute__((always_inline));

static inline void hw_qspi_set_min_cs_high(uint8_t clock_count)
{
        HW_QSPIC_REG_SETF(BURSTCMDB, CS_HIGH_MIN, clock_count);
}

/**
 * \brief Set up erase instructions
 *
 * Instruction will be send after call to hw_qspi_erase_block.
 *
 * \param [in] inst code value of the erase instruction
 * \param [in] inst_phase mode of the QSPI Bus during the instruction phase of the erase instruction
 * \param [in] addr_phase the mode of the QSPI Bus during the address phase of the erase instruction
 * \param [in] hclk_cycles the controller must stay without flash memory reading
 *                         requests for this number of AMBA AHB hclk cycles, before
 *                         performs the command of erase or resume 15 - 0
 * \param [in] cs_hi_cycles after the execution of instructions: write enable, erase, erase
 *                          suspend and erase resume, the QSPI_CS remains high for at
 *                          least this number of qspi bus clock cycles
 *
 * \sa hw_qspi_erase_block
 *
 */
static inline void hw_qspi_set_erase_instruction(uint8_t inst, HW_QSPI_BUS_MODE inst_phase,
                        HW_QSPI_BUS_MODE addr_phase, uint8_t hclk_cycles, uint8_t cs_hi_cycles)
                        __attribute__((always_inline));

static inline void hw_qspi_set_erase_instruction(uint8_t inst, HW_QSPI_BUS_MODE inst_phase,
                        HW_QSPI_BUS_MODE addr_phase, uint8_t hclk_cycles, uint8_t cs_hi_cycles)
{
        HW_QSPIC_REG_SETF(ERASECMDA, ERS_INST, inst);
        QSPIC->QSPIC_ERASECMDB_REG =
                (QSPIC->QSPIC_ERASECMDB_REG &
                        ~(REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_ERS_TX_MD) |
                                REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_EAD_TX_MD) |
                                REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_ERSRES_HLD) |
                                REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_ERS_CS_HI))) |
                                BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_ERS_TX_MD, inst_phase) |
                                BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_EAD_TX_MD, addr_phase) |
                                BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_ERSRES_HLD, hclk_cycles) |
                                BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_ERS_CS_HI, cs_hi_cycles);
}

/**
 * \brief Set up write enable instruction
 *
 * Instruction setup by this function will be executed before erase.
 *
 * \param [in] write_enable code value of the write enable instruction
 * \param [in] inst_phase mode of the QSPI Bus during the instruction phase
 *
 * \sa hw_qspi_erase_block
 * \sa hw_qspi_set_erase_instruction
 *
 */
static inline void hw_qspi_set_write_enable_instruction(uint8_t write_enable,
                                                        HW_QSPI_BUS_MODE inst_phase)
                                                __attribute((always_inline));

static inline void hw_qspi_set_write_enable_instruction(uint8_t write_enable,
                                                        HW_QSPI_BUS_MODE inst_phase)
{
        HW_QSPIC_REG_SETF(ERASECMDA, WEN_INST, write_enable);
        HW_QSPIC_REG_SETF(ERASECMDB, WEN_TX_MD, inst_phase);
}

/**
 * \brief Set up erase suspend/resume instructions
 *
 * \param [in] erase_suspend_inst code value of the erase suspend instruction.
 * \param [in] suspend_inst_phase mode of the QSPI Bus during the instruction phase
 *                                of the suspend instruction
 * \param [in] erase_resume_inst code value of the erase resume instruction.
 * \param [in] resume_inst_phase mode of the QSPI Bus during the instruction phase
 *                               of the resume instruction
 * \param [in] minimum_delay defines the minimum time distance between the instruction
 *                           erase suspend and the previous instruction erase resume.
 *                           This delay will be also applied after the Erase command.
 *                           0 - don't wait. The controller starts the erase suspend procedure
 *                               immediately,
 *                           1..63 - controller waits at least this number of 288kHz clock cycles
 *                                   before the suspension of erasing. Time starts counting after
 *                                   the end of the previous erase resume
 *
 */
static inline void hw_qspi_set_suspend_resume_instructions(uint8_t erase_suspend_inst,
                                                                HW_QSPI_BUS_MODE suspend_inst_phase,
                                                                uint8_t erase_resume_inst,
                                                                HW_QSPI_BUS_MODE resume_inst_phase,
                                                                uint8_t minimum_delay)
                                                        __attribute((always_inline));

static inline void hw_qspi_set_suspend_resume_instructions(uint8_t erase_suspend_inst,
                                                                HW_QSPI_BUS_MODE suspend_inst_phase,
                                                                uint8_t erase_resume_inst,
                                                                HW_QSPI_BUS_MODE resume_inst_phase,
                                                                uint8_t minimum_delay)
{
        QSPIC->QSPIC_ERASECMDA_REG =
                (QSPIC->QSPIC_ERASECMDA_REG &
                        ~(REG_MSK(QSPIC, QSPIC_ERASECMDA_REG, QSPIC_SUS_INST) |
                                REG_MSK(QSPIC, QSPIC_ERASECMDA_REG, QSPIC_RES_INST))) |
                BITS32(QSPIC, QSPIC_ERASECMDA_REG, QSPIC_SUS_INST, erase_suspend_inst) |
                BITS32(QSPIC, QSPIC_ERASECMDA_REG, QSPIC_RES_INST, erase_resume_inst);
        QSPIC->QSPIC_ERASECMDB_REG =
                (QSPIC->QSPIC_ERASECMDB_REG &
                        ~(REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_SUS_TX_MD) |
                                REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_RES_TX_MD) |
                                REG_MSK(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_RESSUS_DLY))) |
                BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_SUS_TX_MD, suspend_inst_phase) |
                BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_RES_TX_MD, resume_inst_phase) |
                BITS32(QSPIC, QSPIC_ERASECMDB_REG, QSPIC_RESSUS_DLY, minimum_delay);
}

/**
 * \brief Set status command
 *
 * This command will be send by QSPI controller when it needs to check status of flash memory.
 * This command is also send indirectly when hw_qspi_get_erase_status is called.
 *
 * \param [in] inst instruction for read status
 * \param [in] inst_phase mode of the QSPI Bus during the instruction phase of the read
 *                        status instruction
 * \param [in] receive_phase mode of the QSPI Bus during the receive status phase of
 *                           the read status instruction
 * \param [in] busy_pos it describes which bit from the bits of status represents
 *                      the Busy bit (7 - 0)
 * \param [in] busy_val defines the value of the Busy bit which means that the flash is busy
 *                      0 - flash is busy when the Busy bit is equal to 0,
 *                      1 - flash is busy when the Busy bit is equal to 1
 * \param [in] read_delay defines the minimum time distance between the instruction
 *                        read status register and previous instructions like erase or
 *                        resume. The same delay will be also applied after the Erase command.
 *                        0 - don't wait. The controller starts to read the Flash memory
 *                            status register immediately.
 *                        1..63 - controller waits at least the number of QSPI_CLK
 *                                cycles and afterwards it starts to read the Flash memory status
 *                                register. Time starts counting after the end of the previous
 *                                erase/resume.
 * \param [in] sts_delay defines the register to count the delay to wait for the FLASH Status
 *                       Register read after an erase or erase/ resume command.
 *                       0 - delay is controlled by the \p read_dealy (QSPIC_RESSTS_DLY)
 *                           which counts on the qspi clock,
 *                       1 - delay is controlled by the \p minimum_delay value passed to
 *                           hw_qspi_set_suspend_resume_instructions (QSPIC_RESSUS_DLY) which
 *                           counts on the 288 kHz clock
 *
 * \sa hw_qspi_set_suspend_resume_instructions
 * \sa hw_qspi_get_erase_status
 *
 */
static inline void hw_qspi_set_read_status_instruction(uint8_t inst,
                                                        HW_QSPI_BUS_MODE inst_phase,
                                                        HW_QSPI_BUS_MODE receive_phase,
                                                        uint8_t busy_pos,
                                                        uint8_t busy_val,
                                                        uint8_t read_delay,
                                                        uint8_t sts_delay)
                                                __attribute((always_inline));

static inline void hw_qspi_set_read_status_instruction(uint8_t inst,
                                                        HW_QSPI_BUS_MODE inst_phase,
                                                        HW_QSPI_BUS_MODE receive_phase,
                                                        uint8_t busy_pos,
                                                        uint8_t busy_val,
                                                        uint8_t read_delay,
                                                        uint8_t sts_delay)
{
        QSPIC->QSPIC_STATUSCMD_REG =
                BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_BUSY_VAL, busy_val) |
                BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_BUSY_POS , busy_pos) |
                BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_RSTAT_RX_MD, receive_phase) |
                BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_RSTAT_TX_MD, inst_phase) |
                BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_RSTAT_INST, inst) |
                BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_STSDLY_SEL, sts_delay) |
                BITS32(QSPIC, QSPIC_STATUSCMD_REG, QSPIC_RESSTS_DLY, read_delay);
}

/**
 * \brief Erase block/sector of flash memory
 *
 * For this function to work, erase instructions must be set up
 * with hw_qspi_set_erase_instruction.
 * User should call hw_qspi_get_erase_status to check if block erased.
 *
 * \note If user doesn't call hw_qspi_get_erase_status that returns status 0
 * QSPI controller will not be able to switch to manual mode.
 *
 * \param [in] addr memory address of block/sector thats is requested to be erased
 *                  for 24 bits addressing, bits [23:12] determine the block/sector
 *                  address bits. Bits [11:0] are ignored by the controller.
 *                  For 32 bits addressing, bits [31:12] determine the block/sectors
 *                  address bits. Bits [11:0] are ignored by the controller
 *
 * \sa hw_qspi_set_erase_instruction
 * \sa hw_qspi_get_erase_status
 * \sa hw_qspi_set_automode
 *
 */
void hw_qspi_erase_block(uint32_t addr);

/**
 * \brief Get erase status
 *
 * \return progress of sector/block erasing
 *         0 = no erase,
 *         1 = pending erase request,
 *         2 = erase procedure is running,
 *         3 = suspended erase procedure,
 *         4 = finishing the erase procedure
 *
 */
static inline uint8_t hw_qspi_get_erase_status(void) __attribute__((always_inline));

static inline uint8_t hw_qspi_get_erase_status(void)
{
        QSPIC->QSPIC_CHCKERASE_REG = 0;
        return HW_QSPIC_REG_GETF(ERASECTRL, ERS_STATE);
}

/**
 * \brief Set burst break sequence
 *
 * \param [in] sequence value will be transmitted as the burst break sequence
 * \param [in] mode mode of the QSPI Bus during the transmission of the burst break sequence
 * \param [in] size size of Burst Break Sequence
 * \param [in] dis_out disable output during the transmission of the second half (sequence[3:0]).
 *                     Setting this bit is only useful if size = 1.
 *                     0 - controller drives the QSPI bus during the transmission
 *                         of the sequence[3:0],
 *                     1 - controller leaves the QSPI bus in Hi-Z during the transmission
 *                         of the sequence[3:0]
 *
 */
static inline void hw_qspi_set_break_sequence(uint16_t sequence, HW_QSPI_BUS_MODE mode,
                                                HW_QSPI_BREAK_SEQ_SIZE size, uint8_t dis_out)
                                                __attribute__((always_inline));

static inline void hw_qspi_set_break_sequence(uint16_t sequence, HW_QSPI_BUS_MODE mode,
                                                HW_QSPI_BREAK_SEQ_SIZE size, uint8_t dis_out)
{
        QSPIC->QSPIC_BURSTBRK_REG =
                BITS32(QSPIC, QSPIC_BURSTBRK_REG, QSPIC_SEC_HF_DS, dis_out) |
                BITS32(QSPIC, QSPIC_BURSTBRK_REG, QSPIC_BRK_SZ, size) |
                BITS32(QSPIC, QSPIC_BURSTBRK_REG, QSPIC_BRK_TX_MD, mode) |
                BITS32(QSPIC, QSPIC_BURSTBRK_REG, QSPIC_BRK_EN, 1) |
                BITS32(QSPIC, QSPIC_BURSTBRK_REG, QSPIC_BRK_WRD, sequence);
}

/**
 * \brief Disable burst break sequence
 *
 */
static inline void hw_qspi_disable_burst_break_sequence(void) __attribute__((always_inline));

static inline void hw_qspi_disable_burst_break_sequence(void)
{
        HW_QSPIC_REG_SETF(BURSTBRK, BRK_EN, 0);
}

/**
 * \brief Set configuration of QSPI pads
 *
 * \param [in] rate QSPI pads slew rate control
 * \param [in] current QSPI pads drive current
 *
 */
void hw_qspi_set_pads(HW_QSPI_SLEW_RATE rate, HW_QSPI_DRIVE_CURRENT current);

/**
 * \brief QSPI controller initialization
 *
 * This function will enable QSPI controller in manual mode.
 *
 * \note This function doesn't change QSPI_DIV
 *
 */
void hw_qspi_init(const qspi_config *cfg);

/**
 * \brief Enable QSPI controller clock
 *
 */
static inline void hw_qspi_enable_clock(void) __attribute__((always_inline));

static inline void hw_qspi_enable_clock(void)
{
        GLOBAL_INT_DISABLE();
        CRG_TOP->CLK_AMBA_REG |= 1 << REG_POS(CRG_TOP, CLK_AMBA_REG, QSPI_ENABLE);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Disable QSPI controller clock
 *
 */
static inline void hw_qspi_disable_clock(void) __attribute__((always_inline));

static inline void hw_qspi_disable_clock(void)
{
        GLOBAL_INT_DISABLE();
        CRG_TOP->CLK_AMBA_REG &= ~REG_MSK(CRG_TOP, CLK_AMBA_REG, QSPI_ENABLE);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Set the QSPI clock divider
 *
 * \param [in] div QSPI clock divider
 *
 * \sa HW_QSPI_DIV
 */
static inline void hw_qspi_set_div(HW_QSPI_DIV div) __attribute__((always_inline));

static inline void hw_qspi_set_div(HW_QSPI_DIV div)
{
        GLOBAL_INT_DISABLE();
        REG_SETF(CRG_TOP, CLK_AMBA_REG, QSPI_DIV, div);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Get the QSPI clock divider
 *
 * \return the QSPI clock divider setting
 *
 * \sa HW_QSPI_DIV
 */
static inline HW_QSPI_DIV hw_qspi_get_div(void) __attribute__((always_inline));

static inline HW_QSPI_DIV hw_qspi_get_div(void)
{
        return REG_GETF(CRG_TOP, CLK_AMBA_REG, QSPI_DIV);
}

#endif /* dg_configUSE_HW_QSPI */

#endif /* HW_QSPI_H_ */

/**
 * \}
 * \}
 * \}
 */
