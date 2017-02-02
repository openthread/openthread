/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup OTPC
 * \{
 * \brief OTP Memory Controller
 */

/**
 ****************************************************************************************
 *
 * @file hw_otpc.h
 *
 * @brief Definition of API for the OTP Controller driver.
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

#ifndef HW_OTPC_H_
#define HW_OTPC_H_

#if dg_configUSE_HW_OTPC

#include <stdbool.h>
#include <stdint.h>
#include <sdk_defs.h>


/**
 * \brief Get the mask of a field of an OTPC register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to access
 *
 */
#define HW_OTPC_REG_FIELD_MASK(reg, field) \
        (OTPC_OTPC_##reg##_REG_##OTPC_##reg##_##field##_Msk)

/**
 * \brief Get the bit position of a field of an OTPC register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to access
 *
 */
#define HW_OTPC_REG_FIELD_POS(reg, field) \
        (OTPC_OTPC_##reg##_REG_##OTPC_##reg##_##field##_Pos)

/**
 * \brief Prepare (i.e. shift and mask) a value to be used for an OTPC register field.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to access
 * \param [in] val is the value to prepare
 *
 */
#define HW_OTPC_FIELD_VAL(reg, field, val) \
        (((val) << HW_OTPC_REG_FIELD_POS(reg, field)) & HW_OTPC_REG_FIELD_MASK(reg, field))

/**
 * \brief Get the value of a field of an OTPC register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to write
 *
 * \return the value of the register field
 *
 */
#define HW_OTPC_REG_GETF(reg, field) \
        ((OTPC->OTPC_##reg##_REG & (OTPC_OTPC_##reg##_REG_##OTPC_##reg##_##field##_Msk)) >> (OTPC_OTPC_##reg##_REG_##OTPC_##reg##_##field##_Pos))

/**
 * \brief Set the value of a field of an OTPC register.
 *
 * \param [in] reg is the register to access
 * \param [in] field is the register field to write
 * \param [in] new_val is the value to write
 *
 */
#define HW_OTPC_REG_SETF(reg, field, new_val) \
        OTPC->OTPC_##reg##_REG = ((OTPC->OTPC_##reg##_REG & ~(OTPC_OTPC_##reg##_REG_##OTPC_##reg##_##field##_Msk)) | \
        ((OTPC_OTPC_##reg##_REG_##OTPC_##reg##_##field##_Msk) & ((new_val) << (OTPC_OTPC_##reg##_REG_##OTPC_##reg##_##field##_Pos))))


/**
 * \brief OTP Controller mode
 */
typedef enum {
        HW_OTPC_MODE_STBY       = 0,
        HW_OTPC_MODE_MREAD      = 1,
        HW_OTPC_MODE_MPROG      = 2,
        HW_OTPC_MODE_AREAD      = 3,
        HW_OTPC_MODE_APROG      = 4,
        HW_OTPC_MODE_TBLANK     = 5,
        HW_OTPC_MODE_TDEC       = 6,
        HW_OTPC_MODE_TWR        = 7
} HW_OTPC_MODE;

#define MAX_RR_AVAIL            (8)

#define OTP_CLK_IS_16M          (0)
#define OTP_CLK_IS_32M          (1)
#define OTP_CLK_IS_48M          (2)


/**
 * \brief Word inside cell to program/read
 *
 * Cell contents in memory starts with low word (i.e. to program/read both words in cell at once,
 * HW_OTPC_WORD_LOW should be used for addressing).
 *
 */
typedef enum {
        HW_OTPC_WORD_LOW = 0,
        HW_OTPC_WORD_HIGH = 1
} HW_OTPC_WORD;


/**
 * \brief System clock frequency in MHz
 *
 */
typedef enum {
        HW_OTPC_SYS_CLK_FREQ_1 = 0,
        HW_OTPC_SYS_CLK_FREQ_2 = 1,
        HW_OTPC_SYS_CLK_FREQ_3 = 2,
        HW_OTPC_SYS_CLK_FREQ_4 = 3,
        HW_OTPC_SYS_CLK_FREQ_6 = 4,
        HW_OTPC_SYS_CLK_FREQ_8 = 5,
        HW_OTPC_SYS_CLK_FREQ_12 = 6,
        HW_OTPC_SYS_CLK_FREQ_16 = 7,
        HW_OTPC_SYS_CLK_FREQ_24 = 8,
        HW_OTPC_SYS_CLK_FREQ_32 = 9,
        HW_OTPC_SYS_CLK_FREQ_48 = 10
} HW_OTPC_SYS_CLK_FREQ;

/*
 * Reset values of OTPC registers
 */
#define OTPC_MODE_REG_RESET                  (0x00000000)
#define OTPC_PCTRL_REG_RESET                 (0x00000000)
#define OTPC_STAT_REG_RESET                  (0x00000051)
#define OTPC_AHBADR_REG_RESET                (0x07FC0000)
#define OTPC_CELADR_REG_RESET                (0x00000000)
#define OTPC_NWORDS_REG_RESET                (0x00000000)
#define OTPC_FFPRT_REG_RESET                 (0x00000000)
#define OTPC_FFRD_REG_RESET                  (0x00000000)
#define OTPC_PWORDL_REG_RESET                (0x00000000)
#define OTPC_PWORDH_REG_RESET                (0x00000000)
#define OTPC_TIM1_REG_RESET                  (0x1A104F20)
#define OTPC_TIM2_REG_RESET                  (0x00010000)

/**
 * \brief Convert system clock frequency expressed in MHz to equivalent HW_OTPC_SYS_CLK_FREQ value
 *
 * \sa HW_OTPC_SYS_CLK_FREQ
 */
__RETAINED_CODE HW_OTPC_SYS_CLK_FREQ hw_otpc_convert_sys_clk_mhz(uint32_t clk_freq);


/**
 * \brief Initialize the OTP Controller.
 *
 * \warning The AHB clock must be up to 48MHz! It is a responsibility of the caller to check this!
 *
 */
static inline void hw_otpc_init(void)
{
        GLOBAL_INT_DISABLE();
        if ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A)
                || ((dg_configUSE_AUTO_CHIP_DETECTION == 1) && (CHIP_IS_AE))) {
                /*
                 * Reset OTP controller
                 */
                OTPC->OTPC_MODE_REG = HW_OTPC_MODE_STBY;                // Set OTPC to standby
                REG_SET_BIT(CRG_TOP, SYS_CTRL_REG, OTPC_RESET_REQ);     // Reset the OTPC
                REG_CLR_BIT(CRG_TOP, SYS_CTRL_REG, OTPC_RESET_REQ);     // Get the OTPC out of Reset

                /*
                 * Initialize the POR registers
                 */
                OTPC->OTPC_NWORDS_REG = OTPC_NWORDS_REG_RESET;
                OTPC->OTPC_TIM1_REG = OTPC_TIM1_REG_RESET;
                OTPC->OTPC_TIM2_REG = OTPC_TIM2_REG_RESET;
        }

        /*
         * Enable OTPC clock
         */
        CRG_TOP->CLK_AMBA_REG |= 1 << REG_POS(CRG_TOP, CLK_AMBA_REG, OTP_ENABLE);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Close the OTP Controller.
 *
 */
static inline void hw_otpc_close(void)
{
        /*
         * Disable OTPC clock
         */
        GLOBAL_INT_DISABLE();
        CRG_TOP->CLK_AMBA_REG &= ~REG_MSK(CRG_TOP, CLK_AMBA_REG, OTP_ENABLE);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Check if the OTP Cotnroller is active.
 *
 * \return 1 if it is active, else 0.
 *
 */
static inline uint32_t hw_otpc_is_active(void) __attribute__((always_inline));

static inline uint32_t hw_otpc_is_active(void)
{
        /*
         * Check if the OTPC clock is enabled
         */
        return !!(CRG_TOP->CLK_AMBA_REG & REG_MSK(CRG_TOP, CLK_AMBA_REG, OTP_ENABLE));
}

/**
 * \brief Put the OTP Controller in STBY mode and turn-off clock.
 *
 */
void hw_otpc_disable(void);

/**
 * \brief Set the access speed of the OTP Controller based on the system clock.
 *
 * \details Switch from PLL to XTAL : call function after clock switch with high_clk_speed == false
 *          Switch from XTAL to PLL : call function before clock switch with high_clk_speed == true
 *
 * \param [in] clk_speed The frequency of the system clock: 1, 2, 3, 4, 6, 8, 12, 16, 24, 32 and 48.
 *
 * \warning The OTP clock must have been enabled (OTP_ENABLE == 1).
 *          (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 *
 */
__RETAINED_CODE void hw_otpc_set_speed(HW_OTPC_SYS_CLK_FREQ clk_speed);

/**
 * \brief Set or reset the power saving mode of the OTP Controller.
 *
 * \param [in] inactivity_period The number of HCLK cycles to remain powered while no access is made.
 *                               If set to 0, this feature is disabled.
 *
 * \warning The hw_otpc_init() and hw_otpc_set_speed() must have been called.
 *          (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 */
void hw_otpc_power_save(uint32_t inactivity_period);

/**
 * \brief Get the number of the Repair Records.
 *
 * \return The number of used Repair Records.
 *
 * \warning The hw_otpc_init() and hw_otpc_set_speed() must have been called. The OTPC mode will be reset.
 *          (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 */
uint32_t hw_otpc_num_of_rr(void);

/**
 * \brief Program manually an OTP entry.
 *
 * \param [in] cell_offset Cell offset to be programmed.
 *
 * \param [in] pword_l   The low 32-bits of the 64-bit word to be written.
 *
 * \param [in] pword_h   The high 32-bits of the 64-bit word to be written.
 *
 * \param [in] use_rr    Use a Repair Record if writing fails when true.
 *
 * \return true for success, false for failure
 *
 * \warning The hw_otpc_init() and the hw_otpc_set_speed() must have been called and the mode must
 *          be STBY. (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 *
 */
bool hw_otpc_manual_word_prog(uint32_t cell_offset, uint32_t pword_l, uint32_t pword_h, bool use_rr);

/**
 * \brief Program manually an OTP block.
 *
 * \param [in] p_data       The start address of the data to copy from.
 *
 * \param [in] cell_offset  Cell offset to start programming from.
 *
 * \param [in] cell_word    Word inside cell to start programming from.
 *
 * \param [in] num_of_words The actual number of the words to be written.
 *
 * \param [in] use_rr       true to use Repair Records if writing fails
 *
 * \return true for success, false for failure
 *
 * \warning The hw_otpc_init() and the hw_otpc_set_speed() must have been called and the mode must
 *          be STBY. (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 *
 */
bool hw_otpc_manual_prog(const uint32_t *p_data, uint32_t cell_offset, HW_OTPC_WORD cell_word,
                uint32_t num_of_words, bool use_rr);

/**
 * \brief Put the OTP in manual read mode.
 *
 * \details In manual read mode the OTP is memory mapped to the address MEMORY_OTP_BASE. So, any
 *          access to this memory space results into reading from the OTP. The caller must make sure
 *          that the function hw_otpc_manual_read_off() is called when reading from the OTP is
 *          finished.
 *
 * \param [in] spare_rows true to access the spare rows area, false to access normal memory cell
 *
 * \warning The hw_otpc_init() and the hw_otpc_set_speed() must have been called and the mode must
 *          be STBY. (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 *
 */
void hw_otpc_manual_read_on(bool spare_rows);

/**
 * \brief Take the OTP out of manual read mode and put it in STBY.
 *
 * \warning The hw_otpc_init() and the hw_otpc_set_speed() must have been called and the mode must
 *          be STBY. (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 *
 */
void hw_otpc_manual_read_off(void);

/**
 * \brief Program manually a Repair Record in the OTP's spare area.
 *
 * \param [in] cell_offset The address programmed with errors.
 * \param [in] pword_l   The low 32-bits of the 64-bit word to be written.
 * \param [in] pword_h   The high 32-bits of the 64-bit word to be written.
 * \return true for success, false for failure
 *
 * \warning The hw_otpc_init() and the hw_otpc_set_speed() must have been called and the mode must
 *          be STBY. (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 *
 */
bool hw_otpc_write_rr(uint32_t cell_offset, uint32_t pword_l, uint32_t pword_h);

/**
 * \brief Program OTP via DMA.
 *
 * \param [in] p_data    The start address of the data in RAM.
 *
 * \param [in] cell_offset Cell offset to be programmed.
 *
 * \param [in] cell_word   Word inside cell to be programmed.
 *
 * \param [in] num_of_words The actual number of the words to be written. The driver will write this
 *                          value minus 1 to the specific register.
 *
 * \param [in] spare_rows true to access the spare rows area, false to access normal memory cell
 *
 * \return true for success, false for failure (the programming should be retried or fixed!)
 *
 * \warning The hw_otpc_init() and the hw_otpc_set_speed() must have been called and the mode must
 *          be STBY. (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 *
 */
bool hw_otpc_dma_prog(const uint32_t *p_data, uint32_t cell_offset, HW_OTPC_WORD cell_word,
                uint32_t num_of_words, bool spare_rows);

/**
 * \brief Read OTP via DMA.
 *
 * \param [in] p_data    The address in RAM to write the data to.
 *
 * \param [in] cell_offset Cell offset to be programmed.
 *
 * \param [in] cell_word   Word inside cell to be programmed.
 *
 * \param [in] num_of_words The actual number of the words to be read. The driver will write this
 *                          value minus 1 to the specific register.
 *
 * \param [in] spare_rows true to access the spare rows area, false to access normal memory cell
 *
 * \warning The hw_otpc_init() and the hw_otpc_set_speed() must have been called and the mode must
 *          be STBY. (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 *
 */
void hw_otpc_dma_read(uint32_t *p_data, uint32_t cell_offset, HW_OTPC_WORD cell_word,
                uint32_t num_of_words, bool spare_rows);

/**
 * \brief Program OTP via FIFO (Auto mode).
 *
 * \param [in] p_data    The start address of the data in RAM.
 *
 * \param [in] cell_offset Cell offset to be programmed.
 *
 * \param [in] cell_word   Word inside cell to be programmed.
 *
 * \param [in] num_of_words The actual number of the words to be written. The driver will write this
 *                          value minus 1 to the specific register.
 *
 * \param [in] spare_rows true to access the spare rows area, false to access normal memory cell
 *
 * \return true for success, false for failure
 *
 * \warning The hw_otpc_init() and the hw_otpc_set_speed() must have been called and the mode must
 *          be STBY. (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 *
 */
bool hw_otpc_fifo_prog(const uint32_t *p_data, uint32_t cell_offset, HW_OTPC_WORD cell_word,
                uint32_t num_of_words, bool spare_rows);

/**
 * \brief Read OTP via FIFO.
 *
 * \param [in] p_data    The address in RAM to write the data to.
 *
 * \param [in] cell_offset Cell offset to be programmed.
 *
 * \param [in] cell_word   Word inside cell to be programmed.
 *
 * \param [in] num_of_words The actual number of the words to be read. The driver will write this
 *                          value minus 1 to the specific register.
 *
 * \param [in] spare_rows true to access the spare rows area, false to access normal memory cell
 *
 * \return true for success, false for failure
 *
 * \warning The hw_otpc_init() and the hw_otpc_set_speed() must have been called and the mode must
 *          be STBY. (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 *
 */
bool hw_otpc_fifo_read(uint32_t *p_data, uint32_t cell_offset, HW_OTPC_WORD cell_word,
                uint32_t num_of_words, bool spare_rows);

/**
 * \brief Prepare OTPC to execute an OTP copy after wakeup.
 *
 * \param [in] num_of_bytes The actual number of the bytes to be copied. The driver will write the
 *                          proper value to the specific register.
 *
 * \warning The hw_otpc_init() and the hw_otpc_set_speed() must have been called and the mode must
 *          be STBY. (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 *
 */
void hw_otpc_prepare(uint32_t num_of_bytes);

/**
 * \brief Cancel any previous preparations of the OTPC for executing an OTP copy after wakeup.
 *
 * \warning The hw_otpc_init() and the hw_otpc_set_speed() must have been called and the mode must
 *          be STBY. (Note: the hw_otpc_set_speed() must be called only once when the PLL is used or
 *          during each clock switch when both PLL and XTAL16 are used, since the register bits it
 *          modifies are retained.)
 *
 */
void hw_otpc_cancel_prepare(void);

/**
 * \brief Get cell memory address
 *
 * Returns mapped memory address for given cell, can be used for e.g. manual reading.
 *
 * \param [in] cell_offset cell offset
 *
 * \return cell memory address
 *
 * \sa hw_otpc_manual_read_on
 *
 */
static inline void *hw_otpc_cell_to_mem(uint32_t cell_offset)
{
        return (void *) (MEMORY_OTP_BASE + (cell_offset << 3)); // cell size is 8 bytes
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST FUNCTIONALITY
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief Execution of the BLANK test.
 *
 * \return The test result.
 *         <ul>
 *             <li> 0, if the test succeeded
 *             <li> 1, if the test 1 failed
 *         </ul>
 *
 */
int hw_otpc_blank(void);

/**
 * \brief Execution of the TDEC test.
 *
 * \return The test result.
 *         <ul>
 *             <li> 0, if the test succeeded
 *             <li> 1, if the test 1 failed
 *         </ul>
 *
 */
int hw_otpc_tdec(void);

/**
 * \brief Execution of the TWR test.
 *
 * \return The test result.
 *         <ul>
 *             <li> 0, if the test succeeded
 *             <li> 1, if the test 1 failed
 *         </ul>
 *
 */
int hw_otpc_twr(void);

#endif /* dg_configUSE_HW_OTPC */

#endif /* HW_OTPC_H_ */

/**
\}
\}
\}
*/
