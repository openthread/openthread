/**************************************************************************//**
 * @file     system_ARMCM0.c
 * @brief    CMSIS Device System Source File for
 *           ARMCM0 Device Series
 * @version  V2.00
 * @date     18. August 2015
 ******************************************************************************/
/* Copyright (c) 2011 - 2015 ARM LIMITED

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/
 

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/errno.h>
#include "sdk_defs.h"
#include "interrupts.h"
#include "hw_cpm.h"
#include "hw_otpc.h"
#include "hw_qspi.h"
#include "hw_watchdog.h"
#include "sys_tcs.h"
#include "qspi_automode.h"

/*
 * This is just so that compilation doesn't break for RAM builds.
 * The code that uses it is optimized out in RAM builds.
 */
extern uint32_t NVMS_PARAM_PART_end;

/*
 * TCS Offsets
 */
#define OTP_HEADER_BASE_ADDR_IN_OTP     (0x7F8E9C0)

#define TCS_SECTION_OFFSET              (184) /* 0x7F8EA78 - 0x7F8E9C0 */
#define TCS_SECTION_LENGTH              (24)  /* 24 entries, 16 bytes each for OTP */

#ifndef __SYSTEM_CLOCK
# define __SYSTEM_CLOCK                 (16000000UL) /* 16 MHz */
#endif

#if (dg_configUSE_LP_CLK == LP_CLK_32000)
# define LP_CLK_FREQ                    (32000)

#elif (dg_configUSE_LP_CLK == LP_CLK_32768)
# define LP_CLK_FREQ                    (32768)

#elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
# define LP_CLK_FREQ                    (0)

#else
# error "dg_configUSE_LP_CLK is not defined or has an unknown value!"
#endif

/*
 * Linker symbols
 *
 * Note: if any of them is missing, please correct your linker script. Please refer to the linker
 * script of pxp_reporter.
 */
extern uint32_t __copy_table_start__;
extern uint32_t __copy_table_end__;
extern uint32_t __zero_table_start__;
extern uint32_t __zero_table_end__;
extern uint8_t end;
extern uint8_t __HeapLimit;

/*
 * Global variables
 */
__RETAINED_RW static uint8_t *heapend = &end;
__RETAINED_UNINIT __attribute__((unused)) uint32_t black_orca_chip_version;
uint32_t SystemCoreClock __RETAINED = __SYSTEM_CLOCK;   /*!< System Clock Frequency (Core Clock)*/
uint32_t SystemLPClock __RETAINED = LP_CLK_FREQ;        /*!< System Low Power Clock Frequency (LP Clock)*/

/**
 * @brief  Memory safe implementation of newlib's _sbrk().
 *
 */
__LTO_EXT
void *_sbrk(int incr)
{
        uint8_t *newheapstart;

        if (heapend + incr > &__HeapLimit) {
                /* Hitting this, means that the value of _HEAP_SIZE is too small.
                 * The value of incr is in stored_incr at this point. By checking the equation
                 * above, it is straightforward to determine the missing space.
                 */
                volatile int stored_incr __attribute__((unused));

                stored_incr = incr;
                ASSERT_ERROR(0);

                errno = ENOMEM;
                return (void *)-1;
        }

        newheapstart = heapend;
        heapend += incr;

        return newheapstart;
}

/**
 * Apply trim values from OTP.
 *
 * @brief Writes the trim values located in the OTP to the corresponding system registers.
 *
 * @param[out] tcs_array The valid <address, value> pairs are placed in this buffer.
 * @param[out] valid_entries The number of valid pairs.
 *
 * @return True if at least one trim value has been applied, else false.
 *
 */
static bool apply_trim_values_from_otp(uint32_t *tcs_array, uint32_t *valid_entries)
{
        uint32_t address;
        uint32_t inverted_address;
        uint32_t value;
        uint32_t inverted_value;
        uint32_t *p;
        int i;
        int index = 0;
        int vdd = 0;
        int retries = 0;
        bool forward_reading = true;
        bool res = false;

        p = (uint32_t *)(OTP_HEADER_BASE_ADDR_IN_OTP + TCS_SECTION_OFFSET);

        for (i = 0; i < TCS_SECTION_LENGTH; i++) {
                do {
                        address = *p;
                        p++;
                        inverted_address = *p;
                        p++;
                        value = *p;
                        p++;
                        inverted_value = *p;
                        p++;

                        if ((address == 0) && (value == 0)) {
                                break;
                        }

                        // Check validity
                        if ((address != ~inverted_address) || (value != ~inverted_value)) {
                                // Change LDO core voltage level and retry
                                vdd++;
                                vdd &= 0x3;
                                REG_SETF(CRG_TOP, LDO_CTRL1_REG, LDO_CORE_SETVDD, vdd);

                                // Wait for the voltage to settle...
                                SysTick->CTRL = 0;
                                SysTick->LOAD = 500;    // 500 * (62.5 * 4) = 125usec
                                SysTick->VAL = 0;
                                SysTick->CTRL = 0x5;    // Start using system clock
                                while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0) {}

                                // Adjust the read pointer
                                p -= 4;
                        }

                        retries++;
                        if (retries == 32) {
                                // Unrecoverable problem! Assert in development mode
                                ASSERT_WARNING(0);

                                // Unrecoverable problem! Issue a HW reset.
                                hw_cpm_reset_system();
                        }
                } while ((address != ~inverted_address) || (value != ~inverted_value));

                retries = 0;

                // Read the complete TCS area but skip empty entries.
                if ((address == 0) && (value == 0)) {
                        if ((BLACK_ORCA_TARGET_IC >= BLACK_ORCA_IC_VERSION(A, E))
                                || ((dg_configUSE_AUTO_CHIP_DETECTION == 1)
                                        && (CHIP_IS_AE || CHIP_IS_BB))) {
                                if (!forward_reading) {
                                        break;
                                }

                                forward_reading = false;
                                p = (uint32_t *)(OTP_HEADER_BASE_ADDR_IN_OTP + TCS_SECTION_OFFSET);
                                p += (TCS_SECTION_LENGTH - 1) * 4;
                        }
                        else {
                                (void)forward_reading;
                        }

                        continue;
                }

                if (!forward_reading) {
                        p -= 8;
                }

                sys_tcs_store_pair(address, value);

                tcs_array[(index * 2) + 0] = address;
                tcs_array[(index * 2) + 1] = value;

                *valid_entries = index + 1;
                index++;

                res = true;
        }

        return res;
}

/**
 * Check whether the code has been compiled for a chip version that is compatible with the chip
 * it runs on.
 */
static bool is_compatible_chip_version(void)
{
        const uint32_t ver = black_orca_get_chip_version();

        /* Oldest supported version is AE. */
        if ((ver < BLACK_ORCA_IC_VERSION(A, E)) && (dg_configUSE_AUTO_CHIP_DETECTION == 0)) {
                return false;
        }

        if ((ver == BLACK_ORCA_TARGET_IC) || (dg_configUSE_AUTO_CHIP_DETECTION == 1)) {
                return true;
        }

        return false;
}

#ifdef OS_BAREMETAL

static volatile bool nortos_xtal16m_settled = false;

void XTAL16RDY_Handler(void)
{
        nortos_xtal16m_settled = true;
}

/* carry out clock initialization sequence */
static void nortos_clk_setup(void)
{
        // Setup DIVN
        if (dg_configEXT_CRYSTAL_FREQ == EXT_CRYSTAL_IS_16M) {
                hw_cpm_set_divn(false);                 // External crystal is 16MHz
        }
        else {
                hw_cpm_set_divn(true);                  // External crystal is 32MHz
        }
        hw_cpm_enable_rc32k();
        hw_cpm_lp_set_rc32k();

        NVIC_ClearPendingIRQ(XTAL16RDY_IRQn);
        nortos_xtal16m_settled = false;
        NVIC_EnableIRQ(XTAL16RDY_IRQn);                 // Activate XTAL16 Ready IRQ

        hw_cpm_set_xtal16m_settling_time(dg_configXTAL16_SETTLE_TIME_RC32K);
        hw_cpm_enable_xtal16m();                        // Enable XTAL16M
        hw_watchdog_unfreeze();                         // Start watchdog

        while(!hw_cpm_is_xtal16m_started());            // Block until XTAL16M starts

        /* Wait for XTAL16M to settle */
        while(!nortos_xtal16m_settled);

        hw_watchdog_freeze();                           // Stop watchdog
        hw_cpm_set_sysclk(SYS_CLK_IS_XTAL16M);
}

#endif  /* OS_BAREMETAL */

static __RETAINED_CODE void configure_cache(void)
{
        bool flush = false;

        GLOBAL_INT_DISABLE();

        if (dg_configCACHEABLE_QSPI_AREA_LEN != -1) {
                uint32_t cache_len;

                /* dg_configCACHEABLE_QSPI_AREA_LEN must be 64KB-aligned */
                ASSERT_WARNING((dg_configCACHEABLE_QSPI_AREA_LEN & 0xFFFF) == 0);
                /*
                 * dg_configCACHEABLE_QSPI_AREA_LEN shouldn't set any bits that do not fit in
                 * CACHE_CTRL2_REG.CACHE_LEN (9 bits wide) after shifting out the lower 16 bits
                 */
                ASSERT_WARNING((dg_configCACHEABLE_QSPI_AREA_LEN & 0x1FF0000)
                                == dg_configCACHEABLE_QSPI_AREA_LEN);
                /*
                 * set cacheable area
                 *
                 * setting CACHE_CTRL2_REG.CACHE_LEN to N, actually sets the size of the cacheable
                 * area to (N + 1) * 64KB
                 * special cases:
                 *  N == 0 --> no caching
                 *  N == 1 --> 128KB are cached, i.e. no way to cache only 64KB
                 */
                cache_len = dg_configCACHEABLE_QSPI_AREA_LEN >> 16;
                /* cannot cache only 64KB! */
                ASSERT_WARNING(cache_len != 1);
                if (cache_len > 1) {
                        cache_len--;
                }
                REG_SETF(CACHE, CACHE_CTRL2_REG, CACHE_LEN, cache_len);
        }

        if (dg_configCACHE_ASSOCIATIVITY != CACHE_ASSOC_AS_IS) {
                if (CACHE->CACHE_ASSOCCFG_REG != dg_configCACHE_ASSOCIATIVITY) {
                        /* override the set associativity setting */
                        CACHE->CACHE_ASSOCCFG_REG = dg_configCACHE_ASSOCIATIVITY;
                        flush = true;
                }
        }

        if (dg_configCACHE_LINESZ != CACHE_LINESZ_AS_IS) {
                if (CACHE->CACHE_LNSIZECFG_REG != dg_configCACHE_LINESZ) {
                        /* override the cache line setting */
                        CACHE->CACHE_LNSIZECFG_REG = dg_configCACHE_LINESZ;
                        flush = true;
                }
        }

        if (flush && (REG_GETF(CACHE, CACHE_CTRL2_REG, CACHE_LEN) > 0)) {
                /* flush cache */
                REG_SET_BIT(CACHE, CACHE_CTRL1_REG, CACHE_FLUSH);
        }

        GLOBAL_INT_RESTORE();
}

/**
 * Basic system setup
 *
 * @brief  Setup the AMBA clocks. Ensure proper alignment of copy and zero table entries.
 */
void SystemInitPre(void) __attribute__((section("text_reset")));

void SystemInitPre(void)
{
        /*
         * Enable debugger.
         */
        if (dg_configENABLE_DEBUGGER) {
                ENABLE_DEBUGGER;
        }

        /*
         * Bandgap has already been set by the bootloader.
         * Use fast clocks from now on.
         */
        hw_cpm_set_hclk_div(0);
        hw_cpm_set_pclk_div(0);

        /*
         * Ensure 16-byte alignment for all elements of each entry in the Copy Table. This is
         * a requirement imposed by the fast start-up code! If any of the assertions below hits,
         * please correct your linker script file accordingly!
         */
        if (dg_configIMAGE_SETUP == DEVELOPMENT_MODE) {
                uint32_t *p;

                for (p = &__copy_table_start__; p < &__copy_table_end__; p += 3) {
                        ASSERT_WARNING_UNINIT( (p[0] & 0xF) == 0 );     // from
                        ASSERT_WARNING_UNINIT( (p[1] & 0xF) == 0 );     // to
                        ASSERT_WARNING_UNINIT( (p[2] & 0xF) == 0 );     // size
                }
        }

        /*
         * Ensure 32-byte alignment for all elements of each entry in the Data Table. This is
         * a requirement imposed by the fast start-up code! If any of the assertions below hits,
         * please correct your linker script file accordingly!
         */
        if (dg_configIMAGE_SETUP == DEVELOPMENT_MODE) {
                uint32_t *p;

                for (p = &__zero_table_start__; p < &__zero_table_end__; p += 2) {
                        ASSERT_WARNING_UNINIT( (p[0] & 0x1F) == 0 );    // start at
                        ASSERT_WARNING_UNINIT( (p[1] & 0x1F) == 0 );    // size
                }
        }
}

/**
 * Initialize the system
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System.
 */
void SystemInit(void)
{
        /*
         * Make sure we are running on a chip version that the code has been built for.
         */
        ASSERT_WARNING_UNINIT(is_compatible_chip_version());

        /*
         * Detect chip version (optionally).
         */
        if (dg_configUSE_AUTO_CHIP_DETECTION == 1) {
                black_orca_chip_version = black_orca_get_chip_version();

                if (!CHIP_IS_AE && !CHIP_IS_BB) {
                        // Oldest supported version is AE.
                        ASSERT_WARNING_UNINIT(0);
                }
        }

        /*
         * Initialize UNINIT variables.
         */
        sys_tcs_init();

        /*
         * BOD protection
         */
        if (dg_configUSE_BOD == 1) {
                /* BOD has already been enabled at this point but it must be reconfigured */
                hw_cpm_configure_bod_protection();
        } else {
                hw_cpm_deactivate_bod_protection();
        }

        /*
         * Apply default priorities to interrupts.
         */
        set_interrupt_priorities(__dialog_interrupt_priorities);

        /*
         * If we are executing from RAM, make sure that code written in Flash won't have left the
         * system in "unknown" state.
         */
        if (dg_configCODE_LOCATION == NON_VOLATILE_IS_NONE) {
                GLOBAL_INT_DISABLE();
                CRG_TOP->PMU_CTRL_REG |= 0xE;
                CRG_TOP->PMU_CTRL_REG &= ~1;
                GLOBAL_INT_RESTORE();
        }

        /*
         * Switch to RC16.
         */
        hw_cpm_set_sysclk(SYS_CLK_IS_RC16);             // Use RC16
        hw_cpm_disable_xtal16m();                       // Disable XTAL16M

        /*
         * Set highest access speed for QSPI Flash.
         */
        if (dg_configFLASH_CONNECTED_TO != FLASH_IS_NOT_CONNECTED) {
                hw_qspi_set_div(HW_QSPI_DIV_1);
        }
}

/**
 * Process the TCS
 *
 * @brief  Process the TCS section.
 */
void SystemInitPost(void)
{
        SystemCoreClock = __SYSTEM_CLOCK;
        SystemLPClock = LP_CLK_FREQ;

        uint32_t tcs_pairs[TCS_SECTION_LENGTH * 2];
        uint32_t valid_tcs_pairs = 0;

        hw_cpm_start_ldos();                            // Make sure all LDOs are as expected
        hw_cpm_reset_radio_vdd();                       // Set Radio voltage to 1.40V

        /*
         * Initialize Flash
         */
        if (dg_configFLASH_CONNECTED_TO != FLASH_IS_NOT_CONNECTED) {
                qspi_automode_init();                   // The bootloader may have left the Flash in wrong mode...
        }

        /*
         * Retrieve trim values from OTP.
         */
        hw_otpc_init();         // Start clock.
        hw_otpc_disable();      // Make sure it is in standby mode.
        hw_otpc_init();         // Restart clock.
        hw_otpc_manual_read_on(false);

        /*
         * Apply trim values from OTP.
         */
        apply_trim_values_from_otp(tcs_pairs, &valid_tcs_pairs);

        if (dg_configIMAGE_SETUP == PRODUCTION_MODE) {
                ASSERT_ERROR(sys_tcs_is_calibrated_chip);       // Uncalibrated chip!!!
        }

        hw_otpc_manual_read_off();
        hw_otpc_disable();

        if (dg_configFLASH_CONNECTED_TO != FLASH_IS_NOT_CONNECTED) {
                hw_cpm_enable_qspi_init();              // Enable QSPI init after wakeup

                hw_qspi_set_read_pipe_clock_delay(6);   // Set read pipe clock delay to 6 (Last review date: Feb 15, 2016 - 12:25:47)
        }

        /*
         * All trim values have been loaded from TCS or defaults are used.
         */
        sys_tcs_sort_array();
        sys_tcs_apply(tcs_system);
        hw_cpm_set_preferred_values();

#ifdef OS_BAREMETAL
        /* perform clock initialization here, as there is no CPM to do it later for us */
        nortos_clk_setup();
#endif

        configure_cache();
}

uint32_t DA15000_phy_addr(uint32_t addr)
{
        static const uint32 remap[] = {
                MEMORY_ROM_BASE,
                MEMORY_OTP_BASE,
                MEMORY_QSPIF_BASE,
                MEMORY_SYSRAM_BASE,
                MEMORY_QSPIF_BASE,
                MEMORY_OTP_BASE,
                MEMORY_CACHERAM_BASE,
                0
        };

        if (addr >= MEMORY_REMAPPED_END)
                return addr;

        return addr + remap[REG_GETF(CRG_TOP, SYS_CTRL_REG, REMAP_ADR0)];
}

__extension__ typedef int __guard __attribute__((mode(__DI__)));
int __cxa_guard_acquire(__guard *g) { return !*(char *)(g); }
void __cxa_guard_release(__guard *g) { *(char *)g = 1; }
void __cxa_guard_abort(__guard *g) { (void)g; }
void __cxa_pure_virtual(void) { while (1); }

