/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup OTPC
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file hw_otpc.c
 *
 * @brief Implementation of the OTP Controller Low Level Driver.
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

#if dg_configUSE_HW_OTPC



#include <stdint.h>
#include "hw_otpc.h"


/*
 * Local variables
 */

/*
 * 1MHz: 1 cycle = 1us ==>
 *    25ns : 1us,   200ns : 1us,   500ns : 1us,   1us : 2us,     5us : 5us, 2us : 3us,     blanc : 1us
 *
 * 2MHz: 1 cycle = 500ns ==>
 *    25ns : 500ns, 200ns : 500ns, 500ns : 1us,   1us : 1.5us,   5us : 5us, 2us : 2.5us,   blanc : 500ns
 *
 * 3MHz: 1 cycle = 333ns ==>
 *    25ns : 333ns, 200ns : 333ns, 500ns : 666ns, 1us : 1.33us,  5us : 5us, 2us : 2.33us,  blanc : 333ns
 *
 * 4MHz: 1 cycle = 250ns ==>
 *    25ns : 250ns, 200ns : 250ns, 500ns : 750ns, 1us : 1.25ns,  5us : 5us, 2us : 2.25ns,  blanc : 250ns
 *
 * 6MHz: 1 cycle = 167ns ==>
 *    25ns : 167ns, 200ns : 333ns, 500ns : 667ns, 1us : 1.167ns, 5us : 5us, 2us : 2.167ns, blanc : 167ns
 *
 * 8MHz: 1 cycle = 125ns ==>
 *    25ns : 125ns, 200ns : 250ns, 500ns : 625ns, 1us : 1.125ns, 5us : 5us, 2us : 2.125ns, blanc : 125ns
 *
 * 12MHz: 1 cycle = 83.33ns ==>
 *    25ns : 83ns, 200ns : 250ns,  500ns : 583ns, 1us : 1.083ns, 5us : 5us, 2us : 2.083ns, blanc : 167ns
 *
 * 16MHz: 1 cycle = 62.5ns ==>
 *    25ns : 62ns, 200ns : 250ns,  500ns : 562ns, 1us : 1.062ns, 5us : 5us, 2us : 2.062ns, blanc : 125ns
 *
 * 24MHz: 1 cycle = 41.67ns ==>
 *    25ns : 41ns, 200ns : 208ns,  500ns : 541ns, 1us : 1.041ns, 5us : 5us, 2us : 2.041ns, blanc : 125ns
 *
 * 32MHz: 1 cycle = 31.25ns ==>
 *    25ns : 31ns, 200ns : 219ns,  500ns : 531ns, 1us : 1.031ns, 5us : 5us, 2us : 2.031ns, blanc : 125ns
 *
 * 48MHz: 1 cycle = 41.67ns ==>
 *    25ns : 41ns, 200ns : 208ns,  500ns : 521ns, 1us : 1.021ns, 5us : 5us, 2us : 2.021ns, blanc : 125ns
 *
 */
const uint32_t tim1[11] = {
        /* 1 MHz */
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_25NS_Pos) |
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_200NS_Pos) |
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_500NS_Pos) |
        (  1 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_1US_Pos) |
        (  4 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_PW_Pos) |
        (  2 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_CADX_Pos),
        /* 2MHz */
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_25NS_Pos) |
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_200NS_Pos) |
        (  1 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_500NS_Pos) |
        (  2 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_1US_Pos) |
        (  9 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_PW_Pos) |
        (  4 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_CADX_Pos),
        /* 3MHz */
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_25NS_Pos) |
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_200NS_Pos) |
        (  1 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_500NS_Pos) |
        (  3 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_1US_Pos) |
        ( 14 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_PW_Pos) |
        (  6 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_CADX_Pos),
        /* 4MHz */
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_25NS_Pos) |
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_200NS_Pos) |
        (  2 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_500NS_Pos) |
        (  4 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_1US_Pos) |
        ( 19 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_PW_Pos) |
        (  8 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_CADX_Pos),
        /* 6MHz */
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_25NS_Pos) |
        (  1 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_200NS_Pos) |
        (  3 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_500NS_Pos) |
        (  6 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_1US_Pos) |
        ( 29 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_PW_Pos) |
        ( 12 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_CADX_Pos),
        /* 8MHz */
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_25NS_Pos) |
        (  1 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_200NS_Pos) |
        (  4 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_500NS_Pos) |
        (  8 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_1US_Pos) |
        ( 39 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_PW_Pos) |
        ( 16 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_CADX_Pos),
        /* 12MHz */
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_25NS_Pos) |
        (  2 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_200NS_Pos) |
        (  6 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_500NS_Pos) |
        ( 12 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_1US_Pos) |
        ( 59 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_PW_Pos) |
        ( 24 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_CADX_Pos),
        /* 16MHz */
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_25NS_Pos) |
        (  3 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_200NS_Pos) |
        (  8 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_500NS_Pos) |
        ( 16 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_1US_Pos) |
        ( 79 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_PW_Pos) |
        ( 32 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_CADX_Pos),
        /* 24MHz */
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_25NS_Pos) |
        (  4 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_200NS_Pos) |
        ( 12 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_500NS_Pos) |
        ( 24 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_1US_Pos) |
        (119 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_PW_Pos) |
        ( 48 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_CADX_Pos),
        /* 32MHz */
        (  0 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_25NS_Pos) |
        (  6 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_200NS_Pos) |
        ( 16 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_500NS_Pos) |
        ( 32 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_1US_Pos) |
        (159 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_PW_Pos) |
        ( 64 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_CADX_Pos),
        /* 48MHz */
        (  1 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_25NS_Pos) |
        (  9 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_200NS_Pos) |
        ( 24 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_500NS_Pos) |
        ( 48 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_1US_Pos) |
        (239 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_PW_Pos) |
        ( 96 << OTPC_OTPC_TIM1_REG_OTPC_TIM1_CC_T_CADX_Pos)
};

static const uint8_t tim2[HW_OTPC_SYS_CLK_FREQ_48 + 1] =
                { 0, 0, 0, 0, 0, 0,  1,  1,  2,  3,  5 };


/*
 * Forward declarations
 */



/*
 * Inline helpers
 */

/**
 * Wait for programming to finish.
 */
__STATIC_INLINE void wait_for_prog_done(void)
{
        while (!(OTPC->OTPC_STAT_REG & HW_OTPC_REG_FIELD_MASK(STAT, PRDY)))
                ;
}

/**
 * Wait for AREAD or APROG to finish.
 */
__STATIC_INLINE void wait_for_auto_done(void)
{
        while (!(OTPC->OTPC_STAT_REG & HW_OTPC_REG_FIELD_MASK(STAT, ARDY)))
                ;
}

/**
 * Check for correctable or uncorrectable programming error.
 */
__STATIC_INLINE bool have_prog_error(void)
{
        return OTPC->OTPC_STAT_REG &
                (HW_OTPC_FIELD_VAL(STAT, PERR_UNC, 1) | HW_OTPC_FIELD_VAL(STAT, PERR_COR, 1));
}

/*
 * Assertion macros
 */

/*
 * Make sure that the OTP clock is enabled
 */
#define ASSERT_WARNING_OTP_CLK_ENABLED \
        ASSERT_WARNING(CRG_TOP->CLK_AMBA_REG & REG_MSK(CRG_TOP, CLK_AMBA_REG, OTP_ENABLE))

/*
 * Make sure that the OTPC is in given state
 */
#define ASSERT_WARNING_OTPC_MODE(s) \
        ASSERT_WARNING((OTPC->OTPC_MODE_REG & HW_OTPC_REG_FIELD_MASK(MODE, MODE)) \
                                == HW_OTPC_FIELD_VAL(MODE, MODE, s))

/*
 * Make sure that the cell address is valid
 */
#define ASSERT_CELL_OFFSET_VALID(off) \
        ASSERT_WARNING(off < 0x2000)

/*
 * Make sure val is non-zero and less than maximum
 */
#define ASSERT_WARNING_NONZERO_RANGE(val, maximum) \
        do { \
                ASSERT_WARNING(val); \
                ASSERT_WARNING((val) < (maximum)); \
        } while (0)


/*
 * Function definitions
 */

__RETAINED_CODE HW_OTPC_SYS_CLK_FREQ hw_otpc_convert_sys_clk_mhz(uint32_t clk_freq)
{
        HW_OTPC_SYS_CLK_FREQ f;

        /* Value to convert must be at most 48 MHz */
        ASSERT_WARNING(clk_freq <= 48);

        if (--clk_freq < 4) {
                /*
                 * cover 1, 2, 3, 4
                 */
                f = (HW_OTPC_SYS_CLK_FREQ)clk_freq;
        } else {
                clk_freq -= 5;
                /*
                 * remaining valid values:
                 *  -  0 (initially 6)
                 *  -  2 (initially 8)
                 *  -  6 (initially 12)
                 *  - 10 (initially 16)
                 *  - 18 (initially 24)
                 *  - 26 (initially 32)
                 *  - 42 (initially 48)
                 */

                /* no odd valid values any more */
                ASSERT_WARNING(!(clk_freq & 1));

                clk_freq >>= 1;
                /*
                 * remaining valid values:
                 *  -  0 (initially 6)
                 *  -  1 (initially 8)
                 *  -  3 (initially 12)
                 *  -  5 (initially 16)
                 *  -  9 (initially 24)
                 *  - 13 (initially 32)
                 *  - 21 (initially 48)
                 */
                if (clk_freq > 8) {
                        /*
                         * remaining valid values:
                         *  -  9 (initially 24)
                         *  - 13 (initially 32)
                         *  - 21 (initially 48)
                         */
                        if (clk_freq > 16) {
                                ASSERT_WARNING(clk_freq == 21);

                                f = HW_OTPC_SYS_CLK_FREQ_48;
                        } else {
                                clk_freq -= 8;
                                /*
                                 * remaining valid values:
                                 *  -  1 (initially 24)
                                 *  -  5 (initially 32)
                                 */
                                ASSERT_WARNING((clk_freq == 1) || (clk_freq == 5));

                                if (clk_freq < 4)
                                        f = (HW_OTPC_SYS_CLK_FREQ)HW_OTPC_SYS_CLK_FREQ_24;
                                else
                                        f = (HW_OTPC_SYS_CLK_FREQ)HW_OTPC_SYS_CLK_FREQ_32;
                        }
                } else {
                        /*
                         * remaining valid values:
                         *  -  0 (initially 6)
                         *  -  1 (initially 8)
                         *  -  3 (initially 12)
                         *  -  5 (initially 16)
                         */
                        if (clk_freq > 2) {
                                ASSERT_WARNING((clk_freq == 3) || (clk_freq == 5));

                                if (clk_freq > 4)
                                        f = (HW_OTPC_SYS_CLK_FREQ)HW_OTPC_SYS_CLK_FREQ_16;
                                else
                                        f = (HW_OTPC_SYS_CLK_FREQ)HW_OTPC_SYS_CLK_FREQ_12;
                        } else {
                                f = (HW_OTPC_SYS_CLK_FREQ)(HW_OTPC_SYS_CLK_FREQ_6 + clk_freq);
                        }
                }
        }

        return f;
}


void hw_otpc_disable(void)
{
        /*
         * Enable OTPC clock
         */
        hw_otpc_init();

        /*
         * set OTPC to stand-by mode
         */
        HW_OTPC_REG_SETF(MODE, MODE, HW_OTPC_MODE_STBY);

        /*
         * Disable OTPC clock
         */
        hw_otpc_close();
}


__RETAINED_CODE void hw_otpc_set_speed(HW_OTPC_SYS_CLK_FREQ clk_speed)
{
        ASSERT_WARNING_OTP_CLK_ENABLED;

        /*
         * Set access speed
         */
        ASSERT_WARNING(clk_speed <= HW_OTPC_SYS_CLK_FREQ_48);

        OTPC->OTPC_TIM1_REG = tim1[clk_speed];
        HW_OTPC_REG_SETF(TIM2, CC_T_BCHK, tim2[clk_speed]);
}


void hw_otpc_power_save(uint32_t inactivity_period)
{
        /* Only go to power save for an inactivity_period < 1024 */
        ASSERT_WARNING(inactivity_period < 1024);

        HW_OTPC_REG_SETF(TIM2, CC_STBY_THR, inactivity_period);
}


uint32_t hw_otpc_num_of_rr(void)
{
        unsigned int i;

        volatile uint32_t *ptr = (volatile uint32_t *)MEMORY_OTP_BASE;

        ASSERT_WARNING_OTP_CLK_ENABLED;

        // STBY mode
        HW_OTPC_REG_SETF(MODE, MODE, HW_OTPC_MODE_STBY);

        // The access will be performed in the spare rows
        HW_OTPC_REG_SETF(MODE, USE_SP_ROWS, 1);

        // MREAD mode
        HW_OTPC_REG_SETF(MODE, MODE, HW_OTPC_MODE_MREAD);

        // Read Records
        i = 0;
        while ((i < MAX_RR_AVAIL) && (ptr[0x9e - 4 * i] & 0x1)) {
                i++;
        }

        // STBY mode
        HW_OTPC_REG_SETF(MODE, MODE, HW_OTPC_MODE_STBY);

        // Select the normal memory array
        HW_OTPC_REG_SETF(MODE, USE_SP_ROWS, 0);

        return i;
}

static bool manual_prog_verify(uint32_t cell_offset, uint32_t pword_l, uint32_t pword_h)
{
        uint32_t val_l, val_h, old_mode = OTPC->OTPC_MODE_REG;
        uint32_t *addr = hw_otpc_cell_to_mem(cell_offset);

        /* we have to go through standby mode first */
        HW_OTPC_REG_SETF(MODE, MODE, HW_OTPC_MODE_STBY);
        __DMB();
        HW_OTPC_REG_SETF(MODE, ERR_RESP_DIS, 1);
        __DMB();
        HW_OTPC_REG_SETF(MODE, MODE, HW_OTPC_MODE_MREAD);

        if (OTPC->OTPC_STAT_REG & HW_OTPC_REG_FIELD_MASK(STAT, RERROR)) {
                HW_OTPC_REG_SETF(STAT, RERROR, 1); //This bit need to be cleared manually (can only happen if a previous read has not checked/cleared it)
        }
        /*
         * Read cell in manual mode, as 2 32-bit values (little-endian)
         */
        val_l = addr[0];
        val_h = addr[1];
        HW_OTPC_REG_SETF(MODE, MODE, HW_OTPC_MODE_STBY);
        __DMB();
        OTPC->OTPC_MODE_REG = old_mode;
        if (OTPC->OTPC_STAT_REG & HW_OTPC_REG_FIELD_MASK(STAT, RERROR)) {
                HW_OTPC_REG_SETF(STAT, RERROR, 1); //This bit need to be cleared manually
                return false;
        }
        return (pword_h == val_h) && (pword_l == val_l);
}

/*
 * Set to 1 to manually verify the programmed cell value, if the auto-verification fails.
 */
#define MANUAL_PROG_VERIFICATION        1

bool hw_otpc_manual_word_prog(uint32_t cell_offset, uint32_t pword_l, uint32_t pword_h, bool use_rr)
{
        int i;
        bool ret = true;

        ASSERT_WARNING_OTP_CLK_ENABLED;

        ASSERT_WARNING_OTPC_MODE(HW_OTPC_MODE_STBY);

        ASSERT_CELL_OFFSET_VALID(cell_offset);

        /*
         * Program the data regs
         */
        OTPC->OTPC_PWORDL_REG = pword_l;
        OTPC->OTPC_PWORDH_REG = pword_h;

        /*
         * Start programming
         */
        HW_OTPC_REG_SETF(MODE, MODE, HW_OTPC_MODE_MPROG);
        OTPC->OTPC_PCTRL_REG =
                HW_OTPC_FIELD_VAL(PCTRL, WADDR, cell_offset) |
                HW_OTPC_FIELD_VAL(PCTRL, PSTART, 1);
        wait_for_prog_done();

        /*
         * Check and retry up to 10 times
         */
        i = 0;
        while (have_prog_error()) {
                if (i == 10) {
                        break;
                }
                i = i + 1;

                OTPC->OTPC_PCTRL_REG =
                        HW_OTPC_FIELD_VAL(PCTRL, WADDR, cell_offset) |
                        HW_OTPC_FIELD_VAL(PCTRL, PSTART, 1) |
                        HW_OTPC_FIELD_VAL(PCTRL, PRETRY, 1);

                wait_for_prog_done();
        }

        if (i == 10) {
                ret = false;
                if (MANUAL_PROG_VERIFICATION)
                        ret = manual_prog_verify(cell_offset, pword_l, pword_h);

                if (use_rr && !ret) {
                        do {
                                // Reset state
                                HW_OTPC_REG_SETF(MODE, MODE, HW_OTPC_MODE_STBY);

                                // Abort if the writing was done in the spare area
                                if (OTPC->OTPC_MODE_REG & HW_OTPC_REG_FIELD_MASK(MODE, USE_SP_ROWS)) {
                                        break;
                                }

                                // Write the repair record to the spare area
                                if (!hw_otpc_write_rr(cell_offset, pword_l, pword_h)) {
                                        break;
                                }

                                HW_OTPC_REG_SETF(MODE, MODE, HW_OTPC_MODE_STBY);
                                // force reloading
                                HW_OTPC_REG_SETF(MODE, MODE, HW_OTPC_MODE_MREAD);

                                ret = true;
                                break;
                        } while (0);
                }
        }

        HW_OTPC_REG_SETF(MODE, MODE, HW_OTPC_MODE_STBY);

        return ret;
}

bool hw_otpc_manual_prog(const uint32_t *p_data, uint32_t cell_offset, HW_OTPC_WORD cell_word,
                uint32_t num_of_words, bool use_rr)
{
        uint32_t w, c, ncells, off, val;
        bool ret = true;
        const uint32_t* addr;

        ASSERT_WARNING_OTP_CLK_ENABLED;

        ASSERT_WARNING_OTPC_MODE(HW_OTPC_MODE_STBY);

        ASSERT_CELL_OFFSET_VALID(cell_offset);

        if (num_of_words == 0)
                return true;  // early exit

        /* index in p_data[] */
        w = 0;
        off = cell_offset;
        if (cell_word == HW_OTPC_WORD_HIGH) {
                /*
                 * read low 32-bit word so that we program the same value
                 */
                addr = hw_otpc_cell_to_mem(cell_offset);

                hw_otpc_manual_read_on(use_rr);
                /* little-endian */
                val = addr[0];
                hw_otpc_manual_read_off();

                if (!hw_otpc_manual_word_prog(off++, val, p_data[w++], use_rr))
                        return false;

                ncells = (num_of_words - 1) >> 1;
        } else {
                ncells = num_of_words >> 1;
        }

        for (c = 0; c < ncells; c++, off++, w += 2) {
                if (!hw_otpc_manual_word_prog(off, p_data[w], p_data[w + 1], use_rr))
                        return false;
        }

        if (w < num_of_words) {
                /*
                 * read high 32-bit word so that we program the same value
                 */
                addr = hw_otpc_cell_to_mem(off);

                hw_otpc_manual_read_on(use_rr);
                /* little-endian */
                val = addr[1];
                hw_otpc_manual_read_off();

                if (!hw_otpc_manual_word_prog(off, p_data[w++], val, use_rr))
                        return false;
        }

        ASSERT_WARNING(w == num_of_words);

        return ret;
}

bool hw_otpc_write_rr(uint32_t cell_addr, uint32_t pword_l, uint32_t pword_h)
{
        uint32_t repair_cnt;
        bool ret = false;

        ASSERT_WARNING_OTP_CLK_ENABLED;

        ASSERT_WARNING_OTPC_MODE(HW_OTPC_MODE_STBY);

        // Get the number of used Repair Records
        repair_cnt = hw_otpc_num_of_rr();

        // Abort if all repair records are being used
        if (repair_cnt < MAX_RR_AVAIL) {
                do {
                        // The access will be performed in the spare rows
                        HW_OTPC_REG_SETF(MODE, USE_SP_ROWS, 1);

                        // Write the data to the spare area
                        if (!hw_otpc_manual_word_prog(0x4F - repair_cnt - 1,
                                                pword_l, pword_h, false)) {
                                break;
                        }

                        // Write the header of the repair record to the spare area
                        if (!hw_otpc_manual_word_prog(0x4F - repair_cnt,
                                                1 | (cell_addr << 1), 0x00000000, false)) {
                                break;
                        }

                        ret = true;
                } while (0);

                // Return to the normal memory array
                HW_OTPC_REG_SETF(MODE, USE_SP_ROWS, 0);

                if (ret) {
                        /*
                         * Request the reloading of the repair records at the next
                         * enabling of the OTP cell
                         */
                        HW_OTPC_REG_SETF(MODE, RLD_RR_REQ, 1);
                }
        }

        return ret;
}


void hw_otpc_manual_read_on(bool spare_rows)
{
        ASSERT_WARNING_OTP_CLK_ENABLED;

        ASSERT_WARNING_OTPC_MODE(HW_OTPC_MODE_STBY);

        /*
         * Place the OTPC in manual read mode
         */
        if (spare_rows) {
                OTPC->OTPC_MODE_REG =
                        HW_OTPC_FIELD_VAL(MODE, MODE, HW_OTPC_MODE_MREAD) |
                        HW_OTPC_FIELD_VAL(MODE, USE_SP_ROWS, 1);
        }
        else {
                OTPC->OTPC_MODE_REG = HW_OTPC_MODE_MREAD;
        }
}


void hw_otpc_manual_read_off(void)
{
        ASSERT_WARNING_OTP_CLK_ENABLED;

        ASSERT_WARNING_OTPC_MODE(HW_OTPC_MODE_MREAD);

        /*
         * Place the OTPC in STBY mode
         */
        OTPC->OTPC_MODE_REG = HW_OTPC_MODE_STBY;
}


bool hw_otpc_dma_prog(const uint32_t *p_data, uint32_t cell_offset, HW_OTPC_WORD cell_word,
                uint32_t num_of_words, bool spare_rows)
{
        ASSERT_WARNING_OTP_CLK_ENABLED;

        ASSERT_WARNING_OTPC_MODE(HW_OTPC_MODE_STBY);

        ASSERT_CELL_OFFSET_VALID(cell_offset);

        /*
         * Make sure that the size is valid
         */
        ASSERT_WARNING(num_of_words);
        ASSERT_WARNING(num_of_words < 16384);

        /*
         * Set up DMA
         */
        unsigned int remap_type = REG_GETF(CRG_TOP, SYS_CTRL_REG, REMAP_ADR0);

        if (IS_SYSRAM_ADDRESS(p_data) ||
            (IS_REMAPPED_ADDRESS(p_data) && (remap_type == 0x3))) {
                OTPC->OTPC_AHBADR_REG = DA15000_phy_addr((uint32_t)p_data);
#if dg_configEXEC_MODE != MODE_IS_CACHED
        } else if (IS_CACHERAM_ADDRESS(p_data)) {
                OTPC->OTPC_AHBADR_REG = DA15000_phy_addr((uint32_t)p_data);
#endif
        } else {
                /*
                 * Destination address can only reside in RAM or Cache RAM, but in case of remapped
                 * address, REMAP_ADR0 cannot be 0x6 (Cache Data RAM)
                 */
                ASSERT_WARNING(0);
        }

        OTPC->OTPC_CELADR_REG = (cell_offset << 1) | (cell_word == HW_OTPC_WORD_HIGH);
        OTPC->OTPC_NWORDS_REG = num_of_words-1;

        /*
         * Start DMA programming
         */
        if (spare_rows) {
                OTPC->OTPC_MODE_REG =
                        HW_OTPC_FIELD_VAL(MODE, MODE, HW_OTPC_MODE_APROG) |
                        HW_OTPC_FIELD_VAL(MODE, USE_DMA, 1) |
                        HW_OTPC_FIELD_VAL(MODE, USE_SP_ROWS, 1);
        }
        else {
                OTPC->OTPC_MODE_REG =
                        HW_OTPC_FIELD_VAL(MODE, MODE, HW_OTPC_MODE_APROG) |
                        HW_OTPC_FIELD_VAL(MODE, USE_DMA, 1);
        }

        wait_for_auto_done();

        /*
         * Check result
         */
        if (OTPC->OTPC_STAT_REG & HW_OTPC_REG_FIELD_MASK(STAT, PERR_UNC)) {
                return false;
        }

        return true;
}


void hw_otpc_dma_read(uint32_t *p_data, uint32_t cell_offset, HW_OTPC_WORD cell_word,
                uint32_t num_of_words, bool spare_rows)
{
        ASSERT_WARNING_OTP_CLK_ENABLED;

        ASSERT_WARNING_OTPC_MODE(HW_OTPC_MODE_STBY);

        ASSERT_CELL_OFFSET_VALID(cell_offset);

        /*
         * Make sure that the size is valid
         */
        ASSERT_WARNING(num_of_words);
        ASSERT_WARNING(num_of_words < 16384);

        /*
         * Set up DMA
         */
        unsigned int remap_type = REG_GETF(CRG_TOP, SYS_CTRL_REG, REMAP_ADR0);

        if (IS_SYSRAM_ADDRESS(p_data) ||
            (IS_REMAPPED_ADDRESS(p_data) && (remap_type == 0x3))) {
                OTPC->OTPC_AHBADR_REG = DA15000_phy_addr((uint32_t)p_data);
#if dg_configEXEC_MODE != MODE_IS_CACHED
        } else if (IS_CACHERAM_ADDRESS(p_data)) {
                OTPC->OTPC_AHBADR_REG = DA15000_phy_addr((uint32_t)p_data);
#endif
        } else {
                /*
                 * Destination address can only reside in RAM or Cache RAM, but in case of remapped
                 * address, REMAP_ADR0 cannot be 0x6 (Cache Data RAM)
                 */
                ASSERT_WARNING(0);
        }

        OTPC->OTPC_CELADR_REG = (cell_offset << 1) | (cell_word == HW_OTPC_WORD_HIGH);
        OTPC->OTPC_NWORDS_REG = num_of_words-1;

        /*
         * Start DMA programming
         */
        if (spare_rows) {
                OTPC->OTPC_MODE_REG =
                        HW_OTPC_FIELD_VAL(MODE, MODE, HW_OTPC_MODE_AREAD) |
                        HW_OTPC_FIELD_VAL(MODE, USE_DMA, 1) |
                        HW_OTPC_FIELD_VAL(MODE, USE_SP_ROWS, 1);
        }
        else {
                OTPC->OTPC_MODE_REG =
                        HW_OTPC_FIELD_VAL(MODE, MODE, HW_OTPC_MODE_AREAD) |
                        HW_OTPC_FIELD_VAL(MODE, USE_DMA, 1);
        }

        wait_for_auto_done();
}


bool hw_otpc_fifo_prog(const uint32_t *p_data, uint32_t cell_offset, HW_OTPC_WORD cell_word,
                uint32_t num_of_words, bool spare_rows)
{
        unsigned int i;

        ASSERT_WARNING_OTP_CLK_ENABLED;

        ASSERT_WARNING_OTPC_MODE(HW_OTPC_MODE_STBY);

        ASSERT_CELL_OFFSET_VALID(cell_offset);

        ASSERT_WARNING_NONZERO_RANGE(num_of_words, 16384);

        /*
         * Set up FIFO
         */
        OTPC->OTPC_CELADR_REG = (cell_offset << 1) | (cell_word == HW_OTPC_WORD_HIGH);
        OTPC->OTPC_NWORDS_REG = num_of_words-1;

        /*
         * Perform programming via FIFO
         */
        if (spare_rows) {
                OTPC->OTPC_MODE_REG =
                        HW_OTPC_FIELD_VAL(MODE, MODE, HW_OTPC_MODE_APROG) |
                        HW_OTPC_FIELD_VAL(MODE, USE_SP_ROWS, 1);
        }
        else {
                OTPC->OTPC_MODE_REG = HW_OTPC_FIELD_VAL(MODE, MODE, HW_OTPC_MODE_APROG);
        }

        for (i = 0; i < num_of_words; i++) {
                while (HW_OTPC_REG_GETF(STAT, FWORDS) == 8)
                        ;
                OTPC->OTPC_FFPRT_REG = p_data[i]; // Write FIFO data
        }

        /*
         * Wait for completion
         */
        wait_for_auto_done();

        /*
         * Check result
         */
        if (OTPC->OTPC_STAT_REG & HW_OTPC_REG_FIELD_MASK(STAT, PERR_UNC)) {
                return false;
        }

        return true;
}


bool hw_otpc_fifo_read(uint32_t *p_data, uint32_t cell_offset, HW_OTPC_WORD cell_word,
                uint32_t num_of_words, bool spare_rows)
{
        unsigned int i;

        ASSERT_WARNING_OTP_CLK_ENABLED;

        ASSERT_WARNING_OTPC_MODE(HW_OTPC_MODE_STBY);

        ASSERT_CELL_OFFSET_VALID(cell_offset);

        ASSERT_WARNING_NONZERO_RANGE(num_of_words, 16384);

        if (OTPC->OTPC_STAT_REG & HW_OTPC_REG_FIELD_MASK(STAT, RERROR)) {
                HW_OTPC_REG_SETF(STAT, RERROR, 1); //This bit need to be cleared manually (can only happen if a previous read has not checked/cleared it)
        }

        /*
         * Set up FIFO
         */
        OTPC->OTPC_CELADR_REG = (cell_offset << 1) | (cell_word == HW_OTPC_WORD_HIGH);
        OTPC->OTPC_NWORDS_REG = num_of_words-1;

        /*
         * Perform reading via FIFO
         */
        if (spare_rows) {
                OTPC->OTPC_MODE_REG =
                        HW_OTPC_FIELD_VAL(MODE, MODE, HW_OTPC_MODE_AREAD) |
                        HW_OTPC_FIELD_VAL(MODE, USE_SP_ROWS, 1);
        }
        else {
                OTPC->OTPC_MODE_REG = HW_OTPC_MODE_AREAD;
        }

        for (i = 0; i < num_of_words; i++) {
                while (HW_OTPC_REG_GETF(STAT, FWORDS) == 0)
                        ;
                p_data[i] = OTPC->OTPC_FFPRT_REG;
        }

        /*
         * Wait for completion
         */
        wait_for_auto_done();

        /*
         * Check result
         */
        if (OTPC->OTPC_STAT_REG & HW_OTPC_REG_FIELD_MASK(STAT, RERROR)) {
                HW_OTPC_REG_SETF(STAT, RERROR, 1); //This bit need to be cleared manually
                return false;
        }

        return true;
}


void hw_otpc_prepare(uint32_t num_of_bytes)
{
        ASSERT_WARNING_OTP_CLK_ENABLED;

        ASSERT_WARNING_OTPC_MODE(HW_OTPC_MODE_STBY);

        ASSERT_WARNING_NONZERO_RANGE(num_of_bytes, 65536);

        /*
         * Set up image size
         */
        OTPC->OTPC_NWORDS_REG = ((num_of_bytes + 3) >> 2) - 1;

        /*
         * Enable OTP_COPY
         */
        GLOBAL_INT_DISABLE();
        CRG_TOP->SYS_CTRL_REG |= 1 << REG_POS(CRG_TOP, SYS_CTRL_REG, OTP_COPY);
        GLOBAL_INT_RESTORE();
}


void hw_otpc_cancel_prepare(void)
{
        ASSERT_WARNING_OTP_CLK_ENABLED;

        ASSERT_WARNING_OTPC_MODE(HW_OTPC_MODE_STBY);

        /*
         * Disable OTP_COPY
         */
        GLOBAL_INT_DISABLE();
        CRG_TOP->SYS_CTRL_REG &= ~REG_MSK(CRG_TOP, SYS_CTRL_REG, OTP_COPY);
        GLOBAL_INT_RESTORE();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST FUNCTIONALITY
////////////////////////////////////////////////////////////////////////////////////////////////////


/**
 * \brief Execution of the test.
 *
 * \return The test result.
 *         <ul>
 *             <li> 0, if the test succeeded
 *             <li> 1, if the test 1 failed
 *         </ul>
 *
 */
static int hw_otpc_core_test(int mode)
{
        ASSERT_WARNING_OTPC_MODE(HW_OTPC_MODE_STBY);

        /*
         * Put OTPC in the proper test mode
         */
        HW_OTPC_REG_SETF(MODE, MODE, mode);

        /*
         * Wait end of test
         */
        while (!(OTPC->OTPC_STAT_REG & HW_OTPC_REG_FIELD_MASK(STAT, TRDY)))
                ;


        /*
         * Check result
         */
        if (OTPC->OTPC_STAT_REG & HW_OTPC_REG_FIELD_MASK(STAT, TERROR)) {
                return 1;
        }

        return 0;
}


int hw_otpc_blank(void)
{
        return hw_otpc_core_test(HW_OTPC_MODE_TBLANK);
}


int hw_otpc_tdec(void)
{
        return hw_otpc_core_test(HW_OTPC_MODE_TDEC);
}


int hw_otpc_twr(void)
{
        return hw_otpc_core_test(HW_OTPC_MODE_TWR);
}


#endif /* dg_configUSE_HW_OTPC */
/**
 * \}
 * \}
 * \}
 */
