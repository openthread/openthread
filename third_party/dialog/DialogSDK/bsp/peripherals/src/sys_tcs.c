/**
\addtogroup BSP
\{
\addtogroup SYSTEM
\{
\addtogroup TCS_Handling
\{
*/

/**
 ****************************************************************************************
 *
 * @file sys_tcs.c
 *
 * @brief TCS HAndler
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

#include <stdint.h>
#include "hw_cpm.h"
#include "sys_tcs.h"
#include "hw_gpadc.h"

#define APPEND_TCS_LENGTH       (0)

#define RADIO_AREA_SIZE         (0x1000)
#define PERIPHERAL_AREA_SIZE    (0xC4A)

#define IS_IN_AREA(addr, base, size)    ((addr >= base) && (addr <= (base + size)))

#define ADC_SE_GAIN_ERROR(value)        ((value)  & 0x0000FFFF)
#define ADC_DIFF_GAIN_ERROR(value)      (((value) & 0xFFFF0000) >> 16)

/*
 * Global and / or retained variables
 */

uint32_t tcs_data[24 * 2 + APPEND_TCS_LENGTH] __RETAINED_UNINIT;
uint8_t tcs_length __RETAINED_UNINIT;

uint8_t area_offset[tcs_system + 1] __RETAINED;
uint8_t area_size_global[tcs_system + 1] __RETAINED;
uint16_t sys_tcs_xtal16m_settling_time __RETAINED;


const uint32_t *tcs_ptr __RETAINED;

bool sys_tcs_is_calibrated_chip __RETAINED_UNINIT;      // Initial value: false

/*
 * Local variables
 */

static const uint32_t uncalibrated_tcs_data[] = {
        /* BANDGAP_REG: (offset 0)
         *   [14]: BYPASS_COLD_BOOT_DISABLE = 0
         *   [13:10]: LDO_SLEEP_TRIM = 0x4
         *   [9:5]: BGR_ITRIM = 0x00
         *   [4:0]: BGR_TRIM = 0x13 (change until V12 is 1.2V)
         *
         *   Especially for this register, merge the trimmed and preferred settings.
         */
        (uint32_t)&CRG_TOP->BANDGAP_REG, 0x1013,

        /* CLK_FREQ_TRIM_REG: (offset 2)
         *   [10:8]: COARSE_ADJ = 0x4
         *   [7:0]: FINE_ADJ = 0x60
         */
        (uint32_t)&CRG_TOP->CLK_FREQ_TRIM_REG, 0x0460,

        /* CLK_16M_REG: (offset 4)
         *   [15]: RC16M_STARTUP_DISABLE = 0
         *   [14]: XTAL16_HPASS_FLT_EN = 0
         *   [13]: XTAL16_SPIKE_FLT_BYPASS = 0
         *   [12:10]: XTAL16_AMP_TRIM = 0x5
         *   [9]: XTAL16_EXT_CLK_ENABLE = 0
         *   [8]: XTAL16_MAX_CURRENT = 0
         *   [7:5]: XTAL16_CUR_SET = 0x5
         *   [4:1]: RC16M_TRIM = 0x9
         *   [0]: RC16M_ENABLE = 0
         *
         *   Apply the trimmed and a default preferred value. The correct one is applied
         *   later.
         */
        (uint32_t)&CRG_TOP->CLK_16M_REG, 0x14B2,

        /* CLK_32K_REG:  (offset 6)
         *   [15:13]: 0
         *   [12]: XTAL32K_DISABLE_AMPREG0 = 0
         *   [11:8]: RC32K_TRIM = 0x07
         *   [7]: RC32K_ENABLE = 0x1 (reset value)
         *   [6:3]: XTAL32K_CUR = 0x3 (reset value)
         *   [2:1]: XTAL32K_RBIAS = 0x2 (reset value)
         *   [0]: XTAL32K_ENABLE = 0
         */
        (uint32_t)&CRG_TOP->CLK_32K_REG, 0x79C,

        /* CHARGER_CTRL2_REG: (offset 8)
         *   [15:13]: 0
         *   [12:8]: CURRENT_OFFSET_TRIM = 0x0C
         *   [7:4]: CHARGER_VFLOAT_ADJ = 0x5
         *   [3:0]: CURRENT_GAIN_TRIM = 0xA
         */
        (uint32_t)&ANAMISC->CHARGER_CTRL2_REG, 0x0C5A,
};

#define UNCALIBRATED_DATA_NON_RETAINED_OFFSET   (8)

#ifdef CONFIG_USE_BLE
#define UNCALIBRATED_DATA_BLE_SIZE              (0)
#define UNCALIBRATED_DATA_BLE_OFFSET            (0)
#endif

#ifdef CONFIG_USE_FTDF
#define UNCALIBRATED_DATA_FTDF_SIZE             (0)
#define UNCALIBRATED_DATA_FTDF_OFFSET           (0)
#endif

#define UNCALIBRATED_DATA_RADIO_SIZE            (0)
#define UNCALIBRATED_DATA_RADIO_OFFSET          (0)

#define UNCALIBRATED_DATA_CHARGER_SIZE          (1 * 2)
#define UNCALIBRATED_DATA_CHARGER_OFFSET        (8)

#define UNCALIBRATED_DATA_AUDIO_SIZE            (0)
#define UNCALIBRATED_DATA_AUDIO_OFFSET          (0)

#define UNCALIBRATED_DATA_SYSTEM_SIZE           (0)
#define UNCALIBRATED_DATA_SYSTEM_OFFSET         (0)


/*
 * Forward declarations
 */



/*
 * Function definitions
 */

/**
 * \brief Apply a TCS <address, value> pair.
 *
 * \param [in] address the address of the register
 * \param [in] value the value for this register
 *
 */
static void apply_pair(uint32_t address, uint32_t value)
{
        if (address & 0x2) {
                *(volatile uint16_t *)address = value;
        }
        else {
                *(volatile uint32_t *)address = value;
        }
}

/**
 * \brief Apply a TCS <address, value> entry of the TCS array.
 *
 * \param [in] address the address of the register
 * \param [in] value the value for this register
 *
 */
static void apply_entry(uint8_t index)
{
        uint32_t address = tcs_ptr[index];
        uint32_t value = tcs_ptr[index + 1];

        if (address & 0x2) {
                *(volatile uint16_t *)address = value;
        }
        else {
                *(volatile uint32_t *)address = value;
        }
}

/**
 * \brief Store TCS <address, value> pair in the Global TCS array.
 *
 * \param [in] address the address of the register
 * \param [in] value the value for this register
 *
 */
static void store_in_array(uint32_t address, uint32_t value)
{
        tcs_data[tcs_length] = address;
        tcs_length++;
        tcs_data[tcs_length] = value;
        tcs_length++;
}

/**
 * \brief Swaps the contents of two entries in the TCS array.
 *
 * \param [in] address the address of the register
 * \param [in] value the value for this register
 *
 * \warning When this function is called, the RC16 must have been set as the system clock!
 *
 */
static void swap_entries(uint32_t first_pos, uint32_t second_pos)
{
        uint32_t address_stored;
        uint32_t value_stored;

        address_stored = tcs_data[second_pos];
        value_stored = tcs_data[second_pos + 1];
        tcs_data[second_pos] = tcs_data[first_pos];
        tcs_data[second_pos + 1] = tcs_data[first_pos + 1];
        tcs_data[first_pos] = address_stored;
        tcs_data[first_pos + 1] = value_stored;
}

/**
 * \brief Sort the registers of a specific Memory Map area of the chip to a continuous region in the
 *        TCS array.
 *
 * \param [in] area_base the base address of the Memory Map area
 * \param [in] area_size the size of the Memory Map region in bytes
 * \param [in] array_ptr the current position in the TCS array to start reading from (all previous
 *        positions have been sorted)
 * \param [out] ptr the offset of the beginning of the sorted region in the TCS array
 * \param [out] size the size of the sorted region in bytes
 *
 * \return The updated position in the TCS array to start reading next time
 */
static int sys_tcs_sort_area(uint32_t area_base, uint32_t area_size, uint32_t array_ptr, uint8_t *ptr, uint8_t *size)
{
        uint32_t i = array_ptr;
        uint32_t sort_start;
        uint32_t sort_end;

        if (array_ptr == tcs_length) {
                return array_ptr;       // Nothing else is left in the array or tcs_length is zero
        }

        sort_start = array_ptr;
        sort_end = array_ptr;

        /* Make sure that the values in TCS don't overflow the allocated array */
        ASSERT_WARNING(tcs_length <= sizeof(tcs_data)/sizeof(tcs_data[0]));

        while (i < (tcs_length - 1)) {
                if (IS_IN_AREA(tcs_data[i], area_base, area_size)) {
                        if (i == array_ptr) {
                                if (sort_start == array_ptr) {
                                        sort_start = array_ptr;
                                        sort_end = sort_start + 2;
                                }
                        }
                        else {
                                // Copy it to the proper place.
                                if (i != sort_end) {
                                        swap_entries(sort_end, i);
                                }
                                sort_end += 2;
                        }
                }
                i += 2;
        }

        *ptr = sort_start;
        *size = sort_end - sort_start;

        return sort_end;
}

void sys_tcs_init(void)
{
        tcs_length = 0;
        sys_tcs_is_calibrated_chip = false;
        hw_cpm_bod_enabled_in_tcs = 0;
}

bool sys_tcs_store_pair(uint32_t address, uint32_t value)
{
        // If it stops here then tcs_length must become uint16_t.
        ASSERT_WARNING( (256 * sizeof(tcs_length)) >= (24 * 2 + APPEND_TCS_LENGTH) );

        // Check if the address is in Always-ON or of a retained register and apply it.
        if (IS_IN_AREA(address, CRG_TOP_BASE, sizeof(CRG_TOP_Type))
                        || IS_IN_AREA(address, TIMER1_BASE, sizeof(TIMER1_Type))
                        || IS_IN_AREA(address, WAKEUP_BASE, sizeof(WAKEUP_Type))
                        || IS_IN_AREA(address, DCDC_BASE, sizeof(DCDC_Type))
                        || IS_IN_AREA(address, QSPIC_BASE, sizeof(QSPIC_Type))
                        || IS_IN_AREA(address, CACHE_BASE, sizeof(CACHE_Type))
                        || IS_IN_AREA(address, OTPC_BASE, sizeof(OTPC_Type))) {
                if (address == (uint32_t)&CRG_TOP->BANDGAP_REG) {
                        sys_tcs_is_calibrated_chip = true;
                } else if (address == (uint32_t)&CRG_TOP->CLK_16M_REG) {
                        value |= CRG_TOP_CLK_16M_REG_RC16M_ENABLE_Msk; // Make sure that RC16 is enabled!
                } else if (address == (uint32_t)&CRG_TOP->XTALRDY_CTRL_REG) {
                        sys_tcs_xtal16m_settling_time = value;
                } else if (address == (uint32_t)&CRG_TOP->BOD_CTRL2_REG) {
                        hw_cpm_bod_enabled_in_tcs = value;
                  /*
                   * Read the ADC Gain Error. Given that the Chip Configuration Section
                   * (CCS) of OTP is out of space, the values are stored in the TCS section
                   * using as address the read-only register SYS_STAT_REG.
                   * The 16 LSB of the 32bit data-value is the single ended gain error.
                   * The 16 MSB of the 32bit data-value is the differential gain error.
                   * Both gain error values should be interpreted as signed 16bit integers.
                   */
                } else if (dg_configUSE_ADC_GAIN_ERROR_CORRECTION == 1) {
                        if (address == (uint32_t)&CRG_TOP->SYS_STAT_REG) {
                                hw_gpadc_store_se_gain_error(ADC_SE_GAIN_ERROR(value));
                                hw_gpadc_store_diff_gain_error(ADC_DIFF_GAIN_ERROR(value));
                        }
                }
                apply_pair(address, value);
        } else {
                store_in_array(address, value);
        }

        return sys_tcs_is_calibrated_chip;
}

void sys_tcs_sort_array(void)
{
        int entry_ptr = 0;
        int i;

        /*
         * Apply a default value to CLK_FREQ_TRIM_REG, if not set in TCS.
         *   [10:8]: COARSE_ADJ = 0x4
         *   [7:0]: FINE_ADJ = 0x60
         */
        if (CRG_TOP->CLK_FREQ_TRIM_REG == 0) {
                CRG_TOP->CLK_FREQ_TRIM_REG = 0x0460;
        }

        if (sys_tcs_is_calibrated_chip) {
#ifdef CONFIG_USE_BLE
                entry_ptr = sys_tcs_sort_area(BLE_BASE, sizeof(BLE_Type), entry_ptr,
                                &area_offset[tcs_ble], &area_size_global[tcs_ble]);
#endif

#ifdef CONFIG_USE_FTDF
                entry_ptr = sys_tcs_sort_area(FTDF_BASE, sizeof(FTDF_Type), entry_ptr,
                                &area_offset[tcs_ftdf], &area_size_global[tcs_ftdf]);
#endif

                entry_ptr = sys_tcs_sort_area(RFCU_BASE, RADIO_AREA_SIZE, entry_ptr,
                                &area_offset[tcs_radio], &area_size_global[tcs_radio]);

                entry_ptr = sys_tcs_sort_area((uint32_t)&ANAMISC->CHARGER_CTRL2_REG, 1, entry_ptr,
                                &area_offset[tcs_charger], &area_size_global[tcs_charger]);

                entry_ptr = sys_tcs_sort_area(APU_BASE, sizeof(APU_Type), entry_ptr,
                                &area_offset[tcs_audio], &area_size_global[tcs_audio]);

                // The rest should be System. Assert if not!
                for (i = entry_ptr; i < tcs_length; i += 2) {
                        if (IS_IN_AREA(tcs_data[i], UART_BASE, PERIPHERAL_AREA_SIZE)) {
                                ASSERT_WARNING(0);
                        }

                        if (IS_IN_AREA(tcs_data[i], CRG_TOP_BASE, 0x6100)) {
                                // It is ok (most probably).
                        }
                        else {
                                ASSERT_WARNING(0);
                        }
                }
                area_offset[tcs_system] = entry_ptr;
                area_size_global[tcs_system] = tcs_length - entry_ptr;

                tcs_ptr = tcs_data;
        }
        else {
                tcs_ptr = uncalibrated_tcs_data;

                for (i = 0; i < UNCALIBRATED_DATA_NON_RETAINED_OFFSET; i += 2) {
                        apply_pair(tcs_ptr[i], tcs_ptr[i + 1]);
                }

#ifdef CONFIG_USE_BLE
                if (UNCALIBRATED_DATA_BLE_SIZE > 0) {
                        area_offset[tcs_ble] = UNCALIBRATED_DATA_BLE_OFFSET;
                        area_size_global[tcs_ble] = UNCALIBRATED_DATA_BLE_SIZE;
                }
#endif

#ifdef CONFIG_USE_FTDF
                if (UNCALIBRATED_DATA_FTDF_SIZE > 0) {
                        area_offset[tcs_ftdf] = UNCALIBRATED_DATA_FTDF_OFFSET;
                        area_size_global[tcs_ftdf] = UNCALIBRATED_DATA_FTDF_SIZE;
                }
#endif

                if (UNCALIBRATED_DATA_RADIO_SIZE > 0) {
                        area_offset[tcs_radio] = UNCALIBRATED_DATA_RADIO_OFFSET;
                        area_size_global[tcs_radio] = UNCALIBRATED_DATA_RADIO_SIZE;
                }

                if (UNCALIBRATED_DATA_CHARGER_SIZE > 0) {
                        area_offset[tcs_charger] = UNCALIBRATED_DATA_CHARGER_OFFSET;
                        area_size_global[tcs_charger] = UNCALIBRATED_DATA_CHARGER_SIZE;
                }

                if (UNCALIBRATED_DATA_AUDIO_SIZE > 0) {
                        area_offset[tcs_audio] = UNCALIBRATED_DATA_AUDIO_OFFSET;
                        area_size_global[tcs_audio] = UNCALIBRATED_DATA_AUDIO_SIZE;
                }

                if (UNCALIBRATED_DATA_SYSTEM_SIZE > 0) {
                        area_offset[tcs_system] = UNCALIBRATED_DATA_SYSTEM_OFFSET;
                        area_size_global[tcs_system] = UNCALIBRATED_DATA_SYSTEM_SIZE;
                }
        }
}

void sys_tcs_apply(sys_tcs_area_t area)
{
        int i;

        for (i = area_offset[area]; i < (area_offset[area] + area_size_global[area]); i += 2) {
                apply_entry(i);
        }
}

/**
\}
\}
\}
*/
