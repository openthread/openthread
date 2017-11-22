/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup RF
 * \{
 */

/**
 *****************************************************************************************
 *
 * @file hw_rf.c
 *
 * @brief Radio module (RF) Low Level Driver API definition.
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

#if dg_configUSE_HW_RF


#include "hw_rf.h"
#include "sdk_defs.h"
#include "hw_cpm.h"
#include "hw_gpio.h"

#if (dg_configSYSTEMVIEW)
#  include "SEGGER_SYSVIEW_FreeRTOS.h"
#else
#  define SEGGER_SYSTEMVIEW_ISR_ENTER()
#  define SEGGER_SYSTEMVIEW_ISR_EXIT()
#endif

#if dg_configUSE_HW_COEX == 1
#include "hw_coex.h"
#endif

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
#define KMODE_ALPHA_ACAD_PREF 0x0410
#define KMODE_ALPHA_AA_PREF 0x0416

#define DF1_DAC_CHECK_VALUE 32
#define DF1_DGAIN_THR0 16
#define DF1_DGAIN_THR1 24
#define DF1_DGAIN_THR2 32
#endif

/* User callbacks (defined as weak functions) definitions */
bool hw_rf_preoff_cb(void) __attribute__((weak));
void hw_rf_postconf_cb(void) __attribute__((weak));
void hw_rf_precalib_cb(void) __attribute__((weak));
void hw_rf_postcalib_cb(void) __attribute__((weak));
void hw_rf_apply_tcs_cb(void) __attribute__((weak));
uint64_t hw_rf_get_start_iff_time(void) __attribute__((weak));
bool hw_rf_check_iff_timeout(uint64_t start_time) __attribute__((weak));

#if dg_configFEM == FEM_SKY66112_11
#include "hw_fem_sky66112-11.h"
#endif

typedef enum rf_states_e {
        RF_ST_OFF = 0,
        RF_ST_ON,
        RF_ST_CONFIG,
        RF_ST_WAIT_NEXT1,
        RF_ST_WAIT_NEXT2
} rf_states_e;

#if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
typedef enum {
        IFF_MODE_CTRL_BLE = 0,
        IFF_MODE_CTRL_FTDF = 1,
        IFF_MODE_CTRL_COMBO = 2
} iff_mode_ctrl_e;

typedef enum {
        IFF_MODE_OVR_BLE = 1,
        IFF_MODE_OVR_FTDF = 2
} iff_mode_ovr_e;

typedef enum {
        MGC_MODE_CTRL_BLE_GAUSSDAC = 0,
        MGC_MODE_CTRL_BLE_TXDAC = 1,
        MGC_MODE_CTRL_FTDF = 2
} mgc_mode_ctrl_e;

#endif

/** RF status bitmap. */
typedef uint8_t rf_request_t;

/** Indicates whether FTDF MAC has turned RF on. */
#define RF_REQUEST_FTDF_ON                     (1 << 0)

/** Indicates whether BLE MAC has turned RF on. */
#define RF_REQUEST_BLE_ON                     (1 << 1)



static rf_request_t rf_request __RETAINED;
static rf_states_e rf_state __RETAINED;
hw_rf_tx_power_luts_t rf_tx_power_luts __RETAINED;

/* Variables used for holding previous reg values during gain calib */
static uint32_t rf_cntrl_timer14_reg_set_offset;
static uint32_t rf_cntrl_timer_7_reg_value;
static uint32_t rf_cal_ctrl_reg_value;
static uint32_t rf_mgain_ctrl_reg_value;
static uint32_t rf_mgain_ctrl2_reg_value;
static uint16_t rf_synth_ctrl2_ble_reg_value;
static uint16_t rf_synth_ctrl2_ftdf_reg_value;

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
static uint8_t kmoda_cal __RETAINED;
__RETAINED_RW static uint32_t gg_cal_modified = 128;
static uint32_t rf_enable_config14_ble_reg_value;
static uint32_t rf_enable_config15_ble_reg_value;
static uint32_t rf_enable_config14_ftdf_reg_value;
static uint32_t rf_enable_config15_ftdf_reg_value;
static uint16_t rf_bmcw_reg_value;
static uint16_t rf_vcocal_ctrl_reg_value;
#else
static uint16_t rf_enable_config14_reg_value;
static uint16_t rf_enable_config15_reg_value;
static uint16_t rf_overrule_reg_value;
static uint16_t rf_enable_config23_ble_reg_value;
static uint16_t rf_enable_config23_ftdf_reg_value;
static uint16_t rf_enable_config45_ble_reg_value;
static uint16_t rf_enable_config45_ftdf_reg_value;
#endif
bool hw_rf_system_init(void)
{
        bool calib_succeeded;

        rf_state = RF_ST_OFF;
        rf_request = 0;

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
        /* Initially set kmoda_cal to its preferred settings value */
        if ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A)
                        || ((dg_configUSE_AUTO_CHIP_DETECTION == 1)
                                && (CHIP_IS_AE))) {
                kmoda_cal = KMODE_ALPHA_ACAD_PREF & REG_MSK(PLLDIG, RF_KMOD_ALPHA_REG,
                        KMOD_ALPHA_BLE);
        }
#endif

#if dg_configCOEX_ENABLE_CONFIG == 1
        hw_coex_init();
#endif
        /* Apply tcs and preferred settings */
        hw_rf_apply_tcs_cb();
        hw_rf_set_recommended_settings();

        /* Perform calibration */
        calib_succeeded = hw_rf_calibration();

#ifndef CONFIG_USE_BLE
        /* Force disable the ble, making the ftdf the only requester */
        REG_SET_BIT(CRG_TOP, FORCE_SLEEP_REG, FORCE_BLE_SLEEP);
#endif
#ifndef CONFIG_USE_FTDF
        /* Force disable the ftdf, making the ble the only requester */
        REG_SET_BIT(CRG_TOP, FORCE_SLEEP_REG, FORCE_FTDF_SLEEP);
#endif

        /* System is ready to be used */
        return calib_succeeded;
}

/**
 * \brief Preferred settings 680 radio
 *
 */
void hw_rf_set_recommended_settings(void)
{

        // Preferred Settings File for DCTMON
        // Device             : DA14680AA
        // Package            : All packages, no dependency on package.
        // Last change date   : June 18, 2015 - 18:00:48
        // Last change item   : Register: RF_KMOD_ALPHA_REG, Field: KMOD_ALPHA_FTDF, Value: 0x10
        // File date          : June 18, 2015 - 19:16:16

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
        if (((dg_configUSE_AUTO_CHIP_DETECTION == 1) && (CHIP_IS_AE))
                || (BLACK_ORCA_TARGET_IC == BLACK_ORCA_IC_VERSION(A, E))) {
                REG_SET_MASKED(DEM, RF_AFC_CTRL_REG,                    0x0030, 0x00F5);
                REG_SET_MASKED(DEM, RF_AGC_CTRL2_REG,                   0x003F, 0x0049);

                // MP
                REG_SET_MASKED(DEM, RF_AGC_CTRL1_REG,                   0x007F, 0x950A);
                REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_10_REG,       0xFF00, 0x182E);
                REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_11_REG,       0xFF00, 0x1830);
                RFCU_POWER->RF_CNTRL_TIMER_12_REG =                             0x3C;
                RFCU_POWER->RF_CNTRL_TIMER_13_REG =                             0x163C;
                RFCU_POWER->RF_CNTRL_TIMER_15_REG =                             0x183C;
                RFCU_POWER->RF_CNTRL_TIMER_16_REG =                             0x2207;
                RFCU_POWER->RF_CNTRL_TIMER_17_REG =                             0x410;
                RFCU_POWER->RF_CNTRL_TIMER_18_REG =                             0x218;
                RFCU_POWER->RF_CNTRL_TIMER_19_REG =                             0x218;
                REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_1_REG,        0xFF00, 0x1E00);
                RFCU_POWER->RF_CNTRL_TIMER_20_REG =                             0x508;
                REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_21_REG,       0x00FF, 0x44);
                REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_22_REG,       0x00FF, 0x40);
                REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_23_REG,       0x00FF, 0x52);
                REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_2_REG,        0xFF00, 0x1B08);
                REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_3_REG,        0xFF00, 0x1A10);
                REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_5_REG,        0xFF00, 0x1818);
                REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_7_REG,        0xFF00, 0x1818);
                REG_SET_MASKED(RFCU, RF_CP_CTRL_BLE_REG,                0x0F0F, 0x7575);
                REG_SET_MASKED(RFCU, RF_CP_CTRL_FTDF_REG,               0x0F0F, 0x7575);
                REG_SET_MASKED(DEM, RF_DC_OFFSET_CTRL2_REG,             0x0202, 0x01D0);
                REG_SET_MASKED(DEM, RF_DC_OFFSET_CTRL3_REG,             0x00FF, 0xDCE4);
                REG_SET_MASKED(RFCU, RF_DIV_IQ_TX_REG,                  0x00FF, 0x00A1);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG11_BLE_REG,  0x001F, 0x0054);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG11_FTDF_REG, 0x001F, 0x0054);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG12_BLE_REG,  0x001F, 0x0071);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG12_FTDF_REG, 0x001F, 0x0071);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG15_BLE_REG,  0x03E0, 0x01E0);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG15_FTDF_REG, 0x03E0, 0x01E0);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG19_BLE_REG,  0x001F, 0x0054);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG19_FTDF_REG, 0x001F, 0x0054);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG20_BLE_REG,  0x001F, 0x0071);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG20_FTDF_REG, 0x001F, 0x0071);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG21_BLE_REG,  0x001F, 0x0071);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG21_FTDF_REG, 0x001F, 0x0071);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG22_BLE_REG,  0x001F, 0x0071);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG22_FTDF_REG, 0x001F, 0x0071);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG24_FTDF_REG, 0x03E0, 0x01A0);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG25_FTDF_REG, 0x03E0, 0x0060);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG27_BLE_REG,  0x001F, 0x0071);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG27_FTDF_REG, 0x001F, 0x0071);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG28_BLE_REG,  0x001F, 0x00B2);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG28_FTDF_REG, 0x001F, 0x00B2);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG29_BLE_REG,  0x001F, 0x00B2);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG29_FTDF_REG, 0x001F, 0x00B2);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG33_BLE_REG,  0x001F, 0x0071);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG33_FTDF_REG, 0x001F, 0x0071);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG34_BLE_REG,  0x001F, 0x00F3);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG34_FTDF_REG, 0x001F, 0x00F3);
                RFCU_POWER->RF_ENABLE_CONFIG42_BLE_REG =                        0x210;
                RFCU_POWER->RF_ENABLE_CONFIG42_FTDF_REG =                       0x210;
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG46_BLE_REG,  0x001F, 0x0015);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG46_FTDF_REG, 0x001F, 0x0015);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG47_BLE_REG,  0x001F, 0x0016);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG47_FTDF_REG, 0x001F, 0x0016);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG48_BLE_REG,  0x001F, 0x0017);
                REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG48_FTDF_REG, 0x001F, 0x0017);
                REG_SET_MASKED(DEM, RF_FTDF_CTRL1_REG,                  0xC000, 0x87C0);
                REG_SET_MASKED(DEM, RF_FTDF_CTRL2_REG,                  0x0700, 0x6810);
                REG_SET_MASKED(DEM, RF_FTDF_CTRL5_REG,                  0x1FFF, 0x4708);
                REG_SET_MASKED(DEM, RF_FTDF_LOOP_GAIN_DS_REG,           0x00FF, 0x0060);
                REG_SET_MASKED(DEM, RF_FTDF_LOOP_GAIN_PD_REG,           0x00FF, 0x0060);
                PLLDIG->RF_KMOD_ALPHA_REG =                                     KMODE_ALPHA_ACAD_PREF;
                REG_SET_MASKED(RFCU, RF_LF_CTRL_REG,                    0x001F, 0x00C0);
                REG_SET_MASKED(RFCU, RF_LF_RES_CTRL_BLE_REG,            0x0F0F, 0x7474);
                REG_SET_MASKED(RFCU, RF_LF_RES_CTRL_FTDF_REG,           0x0F0F, 0x7474);


                PLLDIG->RF_MGAIN_CTRL2_REG =                                    0x0006;
                REG_SET_MASKED(PLLDIG, RF_MGAIN_CTRL_BLE_REG,           0x1C00, 0x1403);


                REG_SET_MASKED(RFCU, RF_MIXER_CTRL1_BLE_REG,            0x000F, 0x0031);
                REG_SET_MASKED(RFCU, RF_MIXER_CTRL1_FTDF_REG,           0x000F, 0x0031);
                REG_SET_MASKED(RFCU, RF_MIXER_CTRL2_REG,                0x001F, 0x0000);
                REG_SET_MASKED(PLLDIG, RF_MSKMOD_CTRL1_REG,             0x0003, 0x0003);
                REG_SET_MASKED(RFCU, RF_REF_OSC_BLE_REG,                0x7FC0, 0x302C);
                REG_SET_MASKED(RFCU, RF_REF_OSC_FTDF_REG,               0x7FC0, 0x302C);
                REG_SET_MASKED(DEM, RF_RSSI_COMP_CTRL_REG,              0xF000, 0x9777);

                REG_SET_MASKED(RFCU, RF_SPARE1_FTDF_REG,                0x4800, 0x4000);
                REG_SET_MASKED(PLLDIG, RF_SYNTH_CTRL2_BLE_REG,          0x14C0, 0x108B);
                REG_SET_MASKED(PLLDIG, RF_SYNTH_CTRL2_FTDF_REG,         0x00C0, 0x009B);
                REG_SET_MASKED(RFCU, RF_TX_PWR_LUT_1_REG,               0x003F, 0x003B);
                REG_SET_MASKED(RFCU, RF_TX_PWR_LUT_2_REG,               0x003F, 0x0037);
                RFCU->RF_TX_PWR_LUT_3_REG =                                     0x01F6;
                REG_SET_MASKED(RFCU, RF_TX_PWR_LUT_4_REG,               0x003F, 0x0036);
                PLLDIG->RF_VCO_CALCAP_BIT14_REG =                               0xD59D;

                /* FTDF specific */
                REG_SETF(DEM, RF_FTDF_CTRL5_REG, RSSITH, 1800);
        }

        REG_SETF(PLLDIG, RF_KMOD_ALPHA_REG, KMOD_ALPHA_BLE, kmoda_cal);
        REG_SETF(PLLDIG, RF_MGAIN_CTRL_BLE_REG, GAUSS_GAIN_WR, gg_cal_modified);
        REG_SET_BIT(PLLDIG, RF_MGAIN_CTRL_BLE_REG, GAUSS_GAIN_SEL);
#else
        REG_SET_MASKED(DEM, RF_AFC_CTRL_REG,                    0x0330, 0x01F5);
        REG_SET_MASKED(DEM, RF_AGC_CTRL1_REG,                   0x007F, 0x950A);
        REG_SET_MASKED(DEM, RF_AGC_CTRL2_REG,                   0x003F, 0x0049);
        REG_SET_MASKED(DEM, RF_CCA_RSSITH_REG,                  0xE000, 0xE708);

        RFCU_POWER->RF_CNTRL_TIMER_10_REG =  0x0A42;
        RFCU_POWER->RF_CNTRL_TIMER_11_REG =  0x0A44;
        REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_12_REG,       0x00FF, 0x0050);
        RFCU_POWER->RF_CNTRL_TIMER_13_REG =  0x0850;
        RFCU_POWER->RF_CNTRL_TIMER_14_REG =  0x1858;
        RFCU_POWER->RF_CNTRL_TIMER_15_REG =  0x0A50;
        REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_16_REG, 0xFF00, 0x1207);
        REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_1_REG, 0xFF00, 0x0F00);
        REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_21_REG, 0x00FF, 0x0044);
        REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_22_REG, 0x00FF, 0x0040);
        REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_23_REG, 0x00FF, 0x0052);
        REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_2_REG, 0xFF00, 0x0D08);
        REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_3_REG, 0xFF00, 0x0C10);
        REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_5_REG, 0xFF00, 0x0A18);
        REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_7_REG, 0xFF00, 0x0A18);
        RFCU->RF_CP_CTRL_BLE_REG = 0x3535;
        REG_SET_MASKED(RFCU, RF_CP_CTRL_FTDF_REG, 0x0F0F, 0x7575);
        REG_SET_MASKED(DEM, RF_DC_OFFSET_CTRL2_REG, 0x0402, 0x05D0);
        REG_SET_MASKED(RFCU, RF_DIV_IQ_TX_REG, 0x00FF, 0x00A1);
        REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG16_REG, 0x001F, 0x0014);
        REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG23_BLE_REG, 0x03E0, 0x0000);
        RFCU_POWER->RF_ENABLE_CONFIG42_REG = 0x0210;
        REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG45_BLE_REG, 0x03E0, 0x0060);
        REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG46_BLE_REG, 0x001F, 0x0015);
        REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG46_FTDF_REG, 0x001F, 0x0015);
        REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG47_BLE_REG, 0x001F, 0x0016);
        REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG47_FTDF_REG, 0x001F, 0x0016);
        REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG48_BLE_REG, 0x001F, 0x0017);
        REG_SET_MASKED(RFCU_POWER, RF_ENABLE_CONFIG48_FTDF_REG, 0x001F, 0x0017);
        REG_SET_MASKED(DEM, RF_FTDF_CTRL1_REG,                  0xCC00, 0x4BC0);
        REG_SET_MASKED(DEM, RF_FTDF_CTRL4_REG,                  0x0500, 0xC6A7);
        REG_SET_MASKED(DEM, RF_FTDF_SIGDET_CTRL_REG,            0x1FFF, 0x0BC3);
        REG_SET_MASKED(PLLDIG, RF_KMOD_ALPHA_BLE_REG, 0x0FC0, 0x030C);
        PLLDIG->RF_KMOD_ALPHA_FTDF_REG = 0x000D;
        RFCU->RF_LF_RES_CTRL_BLE_REG = 0x3434;
        REG_SET_MASKED(RFCU, RF_LF_CTRL_REG,                    0x0040, 0x0080);
        REG_SET_MASKED(RFCU, RF_LF_RES_CTRL_FTDF_REG, 0x0F0F, 0x7474);
        RFCU->RF_LO_IQ_TRIM_REG = 0x0001;
        PLLDIG->RF_MGAIN_CTRL3_REG = 0x0050;
        REG_SET_MASKED(PLLDIG, RF_MGAIN_CTRL_FTDF_REG, 0xFF00, 0x5000);
        REG_SET_MASKED(RFCU, RF_MIXER_CTRL1_BLE_REG, 0x000F, 0x0031);
        REG_SET_MASKED(RFCU, RF_MIXER_CTRL1_FTDF_REG, 0x000F, 0x0031);
        REG_SET_MASKED(RFCU, RF_OVERRULE_REG, 0x3C00, 0x5800);
        REG_SET_MASKED(RFCU, RF_REF_OSC_FTDF_REG, 0x7FFF, 0x31EB);
        REG_SET_MASKED(DEM, RF_RSSI_COMP_CTRL_REG, 0xF000, 0x9777);
        REG_SET_MASKED(RFCU, RF_SPARE1_BLE_REG, 0x0038, 0x0018);
        REG_SET_MASKED(RFCU, RF_SPARE1_FTDF_REG, 0x0008, 0x0008);
        REG_SET_MASKED(PLLDIG, RF_SYNTH_CTRL2_BLE_REG, 0x20C0, 0x304B);
        REG_SET_MASKED(PLLDIG, RF_SYNTH_CTRL2_FTDF_REG, 0x0040, 0x004B);
        RFCU->RF_TX_PWR_LUT_5_REG = 0x09FF;
        REG_SET_MASKED(RFCU, RF_TXDAC_CTRL_REG, 0x0040, 0x0000);
#endif
}



#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
/**
 * \brief Modulation gain calibration
 *
 * \param[in] bool, true for BLE, false for FTDF
 * \param[in] bool, true to enable RFCAL IRQ, false to poll
 *
 */
static void hw_rf_modulation_gain_calibration_start(bool mode_ble)
{

        uint32_t calcap_mid;  //  MP-12-02-2016

        // Store all the registers that will be changed in this function
        rf_cntrl_timer14_reg_set_offset = REG_GETF(RFCU_POWER, RF_CNTRL_TIMER_14_REG,
                        SET_OFFSET);
        rf_cntrl_timer_7_reg_value = RFCU_POWER->RF_CNTRL_TIMER_7_REG;
        rf_enable_config14_ble_reg_value = RFCU_POWER->RF_ENABLE_CONFIG14_BLE_REG;
        rf_enable_config15_ble_reg_value = RFCU_POWER->RF_ENABLE_CONFIG15_BLE_REG;
        rf_enable_config14_ftdf_reg_value = RFCU_POWER->RF_ENABLE_CONFIG14_FTDF_REG;
        rf_enable_config15_ftdf_reg_value = RFCU_POWER->RF_ENABLE_CONFIG15_FTDF_REG;
        rf_cal_ctrl_reg_value = RFCU->RF_CAL_CTRL_REG;
        rf_mgain_ctrl_reg_value = PLLDIG->RF_MGAIN_CTRL_BLE_REG;
        rf_mgain_ctrl2_reg_value = PLLDIG->RF_MGAIN_CTRL2_REG;
        rf_synth_ctrl2_ble_reg_value = PLLDIG->RF_SYNTH_CTRL2_BLE_REG;
        rf_synth_ctrl2_ftdf_reg_value = PLLDIG->RF_SYNTH_CTRL2_FTDF_REG;
        rf_bmcw_reg_value = PLLDIG->RF_BMCW_REG;
        rf_vcocal_ctrl_reg_value = PLLDIG->RF_VCOCAL_CTRL_REG;

        // move PLLCLOSED_EN to after PLL is in lock
        REG_SETF(RFCU_POWER, RF_CNTRL_TIMER_7_REG, SET_OFFSET,
                        rf_cntrl_timer14_reg_set_offset + 150);

        // Disable PA and PA ramp.
        RFCU_POWER->RF_ENABLE_CONFIG14_BLE_REG = 0x0000;
        RFCU_POWER->RF_ENABLE_CONFIG15_BLE_REG = 0x0000;

        RFCU_POWER->RF_ENABLE_CONFIG14_FTDF_REG = 0x0000;
        RFCU_POWER->RF_ENABLE_CONFIG15_FTDF_REG = 0x0000;

        // Disable end-of-packet detection (not needed for the MGC)
        REG_SET_BIT(PLLDIG, RF_SYNTH_CTRL2_BLE_REG, EO_PACKET_DIS);

        //Set modulator inversion
//        REG_CLR_BIT(PLLDIG, RF_MSKMOD_CTRL1_REG, INV_SDM_POL);        // Normal SDM polarity
//        REG_SET_BIT(PLLDIG, RF_MSKMOD_CTRL1_REG, INV_DAC_POL);      // Inverted DAC polarity

        //Make sure that normal operation is selected.
        REG_CLR_BIT(PLLDIG, RF_MGAIN_CTRL_BLE_REG, GAUSS_GAIN_SEL);

        // Settings for modulation gain calibration
        REG_SET_BIT(RFCU, RF_MGC_CTRL_REG, MGC_GAIN_SET);
        REG_SETF(PLLDIG, RF_MGAIN_CTRL_BLE_REG, MGAIN_AVER, 0x2);

        // invert the comparator value if mode bit is set to true.
        if (!mode_ble) {
                REG_SETF(RFCU, RF_OVERRULE_REG, RF_MODE_OVR, 0x2);   // Set radio in FTDF mode
                REG_SET_BIT(PLLDIG, RF_MGAIN_CTRL_BLE_REG, MGAIN_CMP_INV);
                // Set length dependent on mode , JH-09-11-2015
                REG_SETF(PLLDIG, RF_MGAIN_CTRL2_REG, MGAIN_TRANSMIT_LENGTH, 12);
                // Need to set ModIndex value to 0x3F.
                REG_SETF(PLLDIG, RF_SYNTH_CTRL2_FTDF_REG, FTDF_MODINDEX, 0x3F);
        } else {
                REG_SETF(RFCU, RF_OVERRULE_REG, RF_MODE_OVR, 0x1);   // Set radio in BLE mode
                REG_SET_BIT(PLLDIG, RF_MGAIN_CTRL_BLE_REG, MGAIN_CMP_INV);
                // Set length dependent on mode , JH-09-11-2015
                REG_SETF(PLLDIG, RF_MGAIN_CTRL2_REG, MGAIN_TRANSMIT_LENGTH, 8);
                // Set modindex to 266kHz to improve df2/df1 , JH-09-17-2015
                REG_SETF(PLLDIG, RF_SYNTH_CTRL2_BLE_REG, MODINDEX, 0x2);

                // Start with a VCO coarse cal to find the calcap for mid channel
                REG_SETF(PLLDIG, RF_BMCW_REG, CN_SEL, 1);
                REG_SETF(PLLDIG, RF_BMCW_REG, CN_WR, 18);

                // Disable all calibrations except the VCO coarse cal.
                RFCU->RF_CAL_CTRL_REG = 0x001C;

                // Clear eo-cal interrupt, such that new calibration can be stared
                RFCU->RF_IRQ_CTRL_REG = 0x0000;

                // Wait until the VCO coarse calibration can be started
                while ((RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, EO_CAL))) {}

                // Start the VCO coarse calibration
                REG_SET_BIT(RFCU, RF_CAL_CTRL_REG, SO_CAL);

                // Wait until the VCO coarse calibration has started
                while (!(RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL))) {}

                // Wait until the VCO coarse calibration is completed
                while ((RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL))) {}

                // Clear eo_cal interrupt, so that new calibration can be started
                RFCU->RF_IRQ_CTRL_REG = 0x0000;

                calcap_mid = REG_GETF(PLLDIG, RF_SYNTH_RESULT_BLE_REG, VCO_FREQTRIM_RD);
                REG_SETF(PLLDIG, RF_VCOCAL_CTRL_REG,  VCO_FREQTRIM_WR,
                        calcap_mid - REG_GETF(PLLDIG, RF_VCOCAL_CTRL_REG, VCO_FREQTRIM_SEL));

                REG_SETF(PLLDIG, RF_VCOCAL_CTRL_REG,  VCO_FREQTRIM_SEL, 2); // manual calcap

        }

        // Disable all calibrations except the modulation gain cal.
        RFCU->RF_CAL_CTRL_REG = 0x0038;

        // Clear eo-cal interrupt, such that new calibration can be started
        RFCU->RF_IRQ_CTRL_REG = 0x0000;

        // Wait until next calibration can be started
        while (RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, EO_CAL)) {}

        NVIC_ClearPendingIRQ(RFCAL_IRQn);

        // Start the modulation gain calibration
        REG_SET_BIT(RFCU, RF_CAL_CTRL_REG, SO_CAL);

        // Wait until the modulation gain calibration has started
        while (!(RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL))) {}
}
#else /* dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_B */
static void hw_rf_modulation_gain_calibration_generic_start(mgc_mode_ctrl_e mode_ctrl,
        int16_t average, int16_t mgain_mode, int16_t force, int16_t length)
{

        // Store all the registers that will be changed in this function
        rf_cntrl_timer14_reg_set_offset = REG_GETF(RFCU_POWER, RF_CNTRL_TIMER_14_REG,
                        SET_OFFSET);
        rf_cntrl_timer_7_reg_value = RFCU_POWER->RF_CNTRL_TIMER_7_REG;
        rf_enable_config14_reg_value = RFCU_POWER->RF_ENABLE_CONFIG14_REG;
        rf_enable_config15_reg_value = RFCU_POWER->RF_ENABLE_CONFIG15_REG;
        rf_cal_ctrl_reg_value = RFCU->RF_CAL_CTRL_REG;
        rf_mgain_ctrl_reg_value = PLLDIG->RF_MGAIN_CTRL_BLE_REG;
        rf_mgain_ctrl2_reg_value = PLLDIG->RF_MGAIN_CTRL2_REG;
        rf_synth_ctrl2_ble_reg_value = PLLDIG->RF_SYNTH_CTRL2_BLE_REG;
        rf_synth_ctrl2_ftdf_reg_value = PLLDIG->RF_SYNTH_CTRL2_FTDF_REG;
        rf_overrule_reg_value = RFCU->RF_OVERRULE_REG;
        rf_enable_config23_ble_reg_value = RFCU_POWER->RF_ENABLE_CONFIG23_BLE_REG;
        rf_enable_config23_ftdf_reg_value = RFCU_POWER->RF_ENABLE_CONFIG23_FTDF_REG;
        rf_enable_config45_ble_reg_value = RFCU_POWER->RF_ENABLE_CONFIG45_BLE_REG;
        rf_enable_config45_ftdf_reg_value = RFCU_POWER->RF_ENABLE_CONFIG45_FTDF_REG;

        // move PLLCLOSED_EN to after PLL is in lock
        REG_SETF(RFCU_POWER, RF_CNTRL_TIMER_7_REG, SET_OFFSET,
                        rf_cntrl_timer14_reg_set_offset + 150);

        // Disable PA and PA ramp.
        RFCU_POWER->RF_ENABLE_CONFIG14_REG = 0x0000;
        RFCU_POWER->RF_ENABLE_CONFIG15_REG = 0x0000;

        // Disable end-of-packet detection (not needed for the MGC)
        REG_SET_BIT(PLLDIG, RF_SYNTH_CTRL2_BLE_REG, EO_PACKET_DIS);

        // Set Gain in Analog comparator to 1
        REG_SET_BIT(RFCU, RF_MGC_CTRL_REG, MGC_GAIN_SET);


        /* Set mode specific settings for BLE mode with GaussDAC */
        if (mode_ctrl == MGC_MODE_CTRL_BLE_GAUSSDAC) {
                /* Set radio in BLE mode */
                REG_SETF(RFCU, RF_OVERRULE_REG, RF_MODE_OVR, 0x1);
                /* Inversion of analog comparator signal */
                REG_SET_BIT(PLLDIG, RF_MGAIN_CTRL_BLE_REG, MGAIN_CMP_INV);

                if (force != -1) {
                        /* Disable Gauss Gain override */
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL_BLE_REG, GAUSS_GAIN_SEL, force & 0x1);
                }

                if (average != -1) {
                        /* Set number of averaging */
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL_BLE_REG, MGAIN_AVER, average & 0x3);
                }

                /* Setting number of zeros and ones */
                if (length == -1) {
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL2_REG, MGAIN_TRANSMIT_LENGTH, 8);
                } else {
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL2_REG, MGAIN_TRANSMIT_LENGTH,
                                length & 0x7f);
                }

                /* Set modindex to 266kHz */
                REG_SETF(PLLDIG, RF_SYNTH_CTRL2_BLE_REG, MODINDEX, 0x2);
                /* Select the GaussDac in BLE mode */
                REG_SETF(PLLDIG, RF_SYNTH_CTRL2_BLE_REG, BLE_DAC_SEL, 0x0);

                if (mgain_mode != -1) {
                        /* Use the new algorithm */
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL2_REG, MGAIN_MODE_SEL, mgain_mode & 0x1);
                }

                /* Disable TXDAC in BLE mode */
                REG_SETF(RFCU_POWER, RF_ENABLE_CONFIG45_BLE_REG, txdac_en_ble_dcf_tx, 0x0);
                /* Enable GaussDAC in BLE mode */
                REG_SETF(RFCU_POWER, RF_ENABLE_CONFIG23_BLE_REG, gauss_en_ble_dcf_tx, 0x3);
                /* Disable TX-DAC output selection */
                REG_SETF(RFCU, RF_OVERRULE_REG, TXDAC_SEL, 0x0);
                /* Enable GaussDAC output selection switch */
                REG_SETF(RFCU, RF_OVERRULE_REG, GAUSS_DAC_SEL, 0x0);
        }

        /* Set mode specific settings for BLE mode with TXDAC */
        if (mode_ctrl == MGC_MODE_CTRL_BLE_TXDAC) {
                /* Set radio in BLE mode */
                REG_SETF(RFCU, RF_OVERRULE_REG, RF_MODE_OVR, 0x1);
                /* Inversion of analog comparator signal */
                REG_SET_BIT(PLLDIG, RF_MGAIN_CTRL_BLE_REG, MGAIN_CMP_INV);

                if (force != -1) {
                        /* Disable Gauss Gain override */
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL_BLE_REG, GAUSS_GAIN_SEL, force & 0x1);
                }

                if (average != -1) {
                        /* Set number of averaging */
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL_BLE_REG, MGAIN_AVER, average & 0x3);
                }

                /* Setting number of zeros and ones */
                if (length == -1) {
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL2_REG, MGAIN_TRANSMIT_LENGTH, 8);
                } else {
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL2_REG, MGAIN_TRANSMIT_LENGTH,
                                length & 0x7f);
                }

                /* Set modindex to 250kHz */
                REG_SETF(PLLDIG, RF_SYNTH_CTRL2_BLE_REG, MODINDEX, 0x0);
                /* Select the TXDAC in BLE mode */
                REG_SETF(PLLDIG, RF_SYNTH_CTRL2_BLE_REG, BLE_DAC_SEL, 0x1);

                if (mgain_mode != -1) {
                        /* Use the new algorithm */
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL2_REG, MGAIN_MODE_SEL, mgain_mode & 0x1);
                }

                /* Enable TXDAC in BLE mode */
                REG_SETF(RFCU_POWER, RF_ENABLE_CONFIG45_BLE_REG, txdac_en_ble_dcf_tx, 0x3);
                /* Disable GaussDAC in BLE mode */
                REG_SETF(RFCU_POWER, RF_ENABLE_CONFIG23_BLE_REG, gauss_en_ble_dcf_tx, 0x0);
                /* Select TX-DAC output selection */
                REG_SETF(RFCU, RF_OVERRULE_REG, TXDAC_SEL, 0x2);
                /* Unselect GaussDAC output selection switch */
                REG_SETF(RFCU, RF_OVERRULE_REG, GAUSS_DAC_SEL, 0x1);
        }

        /* Set mode specific settings for FTDF mode */
        if (mode_ctrl == MGC_MODE_CTRL_FTDF) {
                /* Set radio in FTDF mode */
                REG_SETF(RFCU, RF_OVERRULE_REG, RF_MODE_OVR, 0x2);
                /* Inversion of analog comparator signal */
                REG_SET_BIT(PLLDIG, RF_MGAIN_CTRL_BLE_REG, MGAIN_CMP_INV);

                if (force != -1) {
                        /* Disable Gauss Gain override */
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL_BLE_REG, GAUSS_GAIN_SEL, force & 0x1);
                }

                if (average != -1) {
                        /* Set number of averaging */
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL_BLE_REG, MGAIN_AVER, average & 0x3);
                }

                /* Setting number of zeros and ones */
                if (length == -1) {
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL2_REG, MGAIN_TRANSMIT_LENGTH, 16);
                } else {
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL2_REG, MGAIN_TRANSMIT_LENGTH,
                                length & 0x7f);
                }

                if (mgain_mode != -1) {
                        /* Use the new algorithm */
                        REG_SETF(PLLDIG, RF_MGAIN_CTRL2_REG, MGAIN_MODE_SEL, mgain_mode & 0x1);
                }

                /* Enable TXDAC in FTDF mode */
                REG_SETF(RFCU_POWER, RF_ENABLE_CONFIG45_FTDF_REG, txdac_en_ftdf_dcf_tx, 0x3);
                /* Disable GaussDAC in FTDF mode */
                REG_SETF(RFCU_POWER, RF_ENABLE_CONFIG23_FTDF_REG, gauss_en_ftdf_dcf_tx, 0x0);
                /* Select TX-DAC output selection */
                REG_SETF(RFCU, RF_OVERRULE_REG, TXDAC_SEL, 0x0);
                /* Unselect GaussDAC output selection switch */
                REG_SETF(RFCU, RF_OVERRULE_REG, GAUSS_DAC_SEL, 0x0);
        }

        // Disable all calibrations except the modulation gain cal.
        RFCU->RF_CAL_CTRL_REG = 0x0038;

        // Clear eo-cal interrupt, such that new calibration can be started
        RFCU->RF_IRQ_CTRL_REG = 0x0000;

        // Wait until next calibration can be started
        while (RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, EO_CAL)) {}

        NVIC_ClearPendingIRQ(RFCAL_IRQn);

        // Start the modulation gain calibration
        REG_SET_BIT(RFCU, RF_CAL_CTRL_REG, SO_CAL);

        // Wait until the modulation gain calibration has started
        while (!(RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL))) {}
}

static inline void hw_rf_modulation_gain_calibration_start(bool mode_ble)
{
        if (mode_ble) {
                hw_rf_modulation_gain_calibration_generic_start(MGC_MODE_CTRL_BLE_TXDAC, 2, 1, 0, 8);
        } else {
                hw_rf_modulation_gain_calibration_generic_start(MGC_MODE_CTRL_FTDF, 2, 1, 0, 8);
        }
}
#endif

static void hw_rf_modulation_gain_calibration_end(void)
{
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
        uint32_t  ch_cal,gg_cal,gg_ch_prod,kmoda_max, gg0, gg_th1, gg_th2;  //  MP-12-02-2016
        uint32_t kmoda_base;
        uint32_t Y;
        uint32_t tmp;
        uint32_t dgain_kill_second_jump;
        uint32_t dgain;

        if ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A)
                        || ((dg_configUSE_AUTO_CHIP_DETECTION == 1)
                                && (CHIP_IS_AE))) {
                kmoda_base = KMODE_ALPHA_ACAD_PREF & REG_MSK(PLLDIG, RF_KMOD_ALPHA_REG,
                        KMOD_ALPHA_BLE);
        }

        // Clear eo_cal interrupt, so that new calibration can be started
        RFCU->RF_IRQ_CTRL_REG = 0x0000;

        ch_cal = REG_GETF(PLLDIG, RF_SYNTH_RESULT2_BLE_REG, CN_CAL_RD);
        gg_cal = REG_GETF(PLLDIG, RF_SYNTH_RESULT_BLE_REG,  GAUSS_GAIN_CAL_RD);

        gg0    = gg_cal + ((gg_cal*kmoda_base*ch_cal+1024)>>11);
        gg_th1 = (gg_cal&0xE0) + DF1_DAC_CHECK_VALUE; // gg_th1 = (gg_cal&0xE0)+32;
        gg_th2 = (gg_th1 + DF1_DAC_CHECK_VALUE);      // gg_th2 = (gg_th1+32);

        if (gg0 >= gg_th2) { //so 2 jumps are on the left of the cal. channel
                /*
                Formula (MP):
                Y = 1 + ((kmoda_base*ch_cal)/2048);
                dgain_kill_second_jump = gg_cal - (gg_th2-1)/Y;

                Improve the resolution,
                Y = 2048* + (kmoda_base*ch_cal);
                dgain_kill_second_jump = gg_cal - (gg_th2-1)*2048/Y;
                 */

                Y = 2048 + (kmoda_base*ch_cal);
                tmp = (((gg_th2-1)<<11)/Y);

                if (gg_cal > tmp) {//check to prevent negative value for dgain_kill_second_jump
                        dgain_kill_second_jump = gg_cal - tmp;

                        if (dgain_kill_second_jump <= DF1_DGAIN_THR1) {
                                dgain = DF1_DGAIN_THR1;
                        } else if (dgain_kill_second_jump >= DF1_DGAIN_THR2) {
                                dgain = DF1_DGAIN_THR2;  // Should never end up here � but better to be safe than sorry. dgain_thr2 is default 16
                        } else {
                                dgain = dgain_kill_second_jump;
                        }
                } else {
                        dgain = 0; // End up here because something went wrong with the calculation of tmp!
                }

        } else if (gg0 >= gg_th1) {
                dgain = DF1_DGAIN_THR1;
        } else {
                dgain = DF1_DGAIN_THR0;
        }

        if (gg_cal>dgain) {
                gg_cal_modified = gg_cal-dgain;
        } else {
                gg_cal_modified = gg_cal;
        }

        gg_ch_prod = (uint32_t) (gg_cal_modified * ch_cal);

        if (gg_ch_prod == 0) { // Avoid HardFault due to division by 0 (not likely)
                kmoda_cal = kmoda_base;
        } else {

                kmoda_max = (255*2048 - 2048 * gg_cal_modified) / gg_ch_prod ;

                if (kmoda_max >= kmoda_base) {
                        kmoda_cal = kmoda_base;
                } else {
                        kmoda_cal = kmoda_max;
                }
        }

        // This should be done both here (to take effect after calib) AND after preferred settings
        REG_SETF(PLLDIG, RF_KMOD_ALPHA_REG, KMOD_ALPHA_BLE, kmoda_cal);

        //Restore the values of the registers that were changed in the function
        //REG_SET_BIT(PLLDIG, RF_MSKMOD_CTRL1_REG, INV_SDM_POL);  // Inverted SDM polarity
        //REG_CLR_BIT(PLLDIG, RF_MSKMOD_CTRL1_REG, INV_DAC_POL);  // Normal DAC polarity
        RFCU_POWER->RF_CNTRL_TIMER_7_REG = rf_cntrl_timer_7_reg_value;
        RFCU->RF_CAL_CTRL_REG = rf_cal_ctrl_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG14_BLE_REG = rf_enable_config14_ble_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG14_FTDF_REG = rf_enable_config14_ftdf_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG15_BLE_REG = rf_enable_config15_ble_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG15_FTDF_REG = rf_enable_config15_ftdf_reg_value;

        PLLDIG->RF_MGAIN_CTRL_BLE_REG = rf_mgain_ctrl_reg_value;
        PLLDIG->RF_MGAIN_CTRL2_REG = rf_mgain_ctrl2_reg_value;
        PLLDIG->RF_SYNTH_CTRL2_BLE_REG = rf_synth_ctrl2_ble_reg_value;
        PLLDIG->RF_SYNTH_CTRL2_FTDF_REG = rf_synth_ctrl2_ftdf_reg_value;

        PLLDIG->RF_BMCW_REG = rf_bmcw_reg_value; // MP-12-02-2016
        PLLDIG->RF_VCOCAL_CTRL_REG = rf_vcocal_ctrl_reg_value; // MP-12-02-2016

        REG_SETF(PLLDIG, RF_MGAIN_CTRL_BLE_REG, GAUSS_GAIN_WR, gg_cal_modified);
        REG_SET_BIT(PLLDIG, RF_MGAIN_CTRL_BLE_REG, GAUSS_GAIN_SEL);

        // Disable overrule mode
        RFCU->RF_OVERRULE_REG &= ~REG_MSK(RFCU, RF_OVERRULE_REG, RF_MODE_OVR);
#else
        // Clear eo_cal interrupt, so that new calibration can be started
        RFCU->RF_IRQ_CTRL_REG = 0x0000;

        //Restore the values of the registers that were changed in the function
        RFCU_POWER->RF_CNTRL_TIMER_7_REG = rf_cntrl_timer_7_reg_value;
        RFCU->RF_CAL_CTRL_REG = rf_cal_ctrl_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG14_REG = rf_enable_config14_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG15_REG = rf_enable_config15_reg_value;
        PLLDIG->RF_MGAIN_CTRL_BLE_REG = rf_mgain_ctrl_reg_value;
        PLLDIG->RF_MGAIN_CTRL2_REG = rf_mgain_ctrl2_reg_value;
        PLLDIG->RF_SYNTH_CTRL2_BLE_REG = rf_synth_ctrl2_ble_reg_value;
        PLLDIG->RF_SYNTH_CTRL2_FTDF_REG = rf_synth_ctrl2_ftdf_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG23_BLE_REG = rf_enable_config23_ble_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG23_FTDF_REG = rf_enable_config23_ftdf_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG45_BLE_REG = rf_enable_config45_ble_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG45_FTDF_REG = rf_enable_config45_ftdf_reg_value;

        // Disable overrule mode
        RFCU->RF_OVERRULE_REG = rf_overrule_reg_value;

#endif
}

/**
 * \brief Modulation gain calibration - Wrapper function
 *
 * Used on boot, and generally whenever the interrupt-based approach is not
 * needed
 *
 * \param[in] bool, true for FTDF, false for BLE
 *
 */
void hw_rf_modulation_gain_calibration(bool mode_ble)
{
        /* Start the gain calibration */
        hw_rf_modulation_gain_calibration_start(mode_ble);

        // Wait until the modulation gain calibration is completed
        while (RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL)) {}

        hw_rf_modulation_gain_calibration_end();
}


void hw_rf_dc_offset_calibration(void)
{
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
        // Save the values of registers that will be changed in the function
        uint32_t RF_ENABLE_CONFIG0_BLE_REG_value = RFCU_POWER->RF_ENABLE_CONFIG0_BLE_REG;
        uint32_t RF_ENABLE_CONFIG1_BLE_REG_value = RFCU_POWER->RF_ENABLE_CONFIG1_BLE_REG;
        uint32_t RF_ENABLE_CONFIG2_BLE_REG_value = RFCU_POWER->RF_ENABLE_CONFIG2_BLE_REG;
        uint32_t RF_ENABLE_CONFIG46_BLE_REG_value = RFCU_POWER->RF_ENABLE_CONFIG46_BLE_REG;

        uint32_t RF_DC_OFFSET_CTRL2_REG_value = DEM->RF_DC_OFFSET_CTRL2_REG;
        uint32_t rf_cal_ctrl_reg_value_local = RFCU->RF_CAL_CTRL_REG;
        uint32_t rf_overrule_reg_value_local = RFCU->RF_OVERRULE_REG;

        // Required setting for the DC-offset calibration
        RFCU_POWER->RF_ENABLE_CONFIG0_BLE_REG = 0x0000;   // Disable LNA_LDO
        RFCU_POWER->RF_ENABLE_CONFIG1_BLE_REG = 0x0000;   // Disable LNA_CORE
        RFCU_POWER->RF_ENABLE_CONFIG2_BLE_REG = 0x0000;   // Disable LNA_CGM
        RFCU_POWER->RF_ENABLE_CONFIG46_BLE_REG = 0x0000;             // Disable the DCF triggered Partial DCOC

        // DCNGAIN = 3, DCNSTEP = 5, DCPARCAL_EN = 0, DCOFFSET_SEL = 0
        DEM->RF_DC_OFFSET_CTRL2_REG = 0x01D0;
        // Disable all calibrations except the DC-offset cal.
        RFCU->RF_CAL_CTRL_REG = 0x002C;

        REG_SETF(RFCU, RF_OVERRULE_REG, RF_MODE_OVR, 0x1);   // Overrule RF_MODE <- BLE_MODE

        // Clear eo-cal interrupt, so that new calibration can be stared
        RFCU->RF_IRQ_CTRL_REG = 0x0000;

        // Wait until the DC-offset calibration can be started
        while (RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, EO_CAL)) {}

        // Start the modulation gain calibration
        REG_SET_BIT(RFCU, RF_CAL_CTRL_REG, SO_CAL);

        // Wait until the modulation gain calibration has started
        while (!(RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL))) {}

        // Wait until the modulation gain calibration is completed
        while (RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL)) {}

        // Clear eo-cal interrupt, so that new calibration can be started
        RFCU->RF_IRQ_CTRL_REG = 0x0000;

        // Restore the values of registers that were changed in the function
        RFCU_POWER->RF_ENABLE_CONFIG0_BLE_REG = RF_ENABLE_CONFIG0_BLE_REG_value;
        RFCU_POWER->RF_ENABLE_CONFIG1_BLE_REG = RF_ENABLE_CONFIG1_BLE_REG_value;
        RFCU_POWER->RF_ENABLE_CONFIG2_BLE_REG = RF_ENABLE_CONFIG2_BLE_REG_value;
        RFCU_POWER->RF_ENABLE_CONFIG46_BLE_REG = RF_ENABLE_CONFIG46_BLE_REG_value;

        DEM->RF_DC_OFFSET_CTRL2_REG = RF_DC_OFFSET_CTRL2_REG_value;
        RFCU->RF_CAL_CTRL_REG = rf_cal_ctrl_reg_value_local;
        RFCU->RF_OVERRULE_REG = rf_overrule_reg_value_local;
#else /* REV_B */
        // Save the values of registers that will be changed in the function
        uint32_t RF_ENABLE_CONFIG0_REG_value = RFCU_POWER->RF_ENABLE_CONFIG0_REG;
        uint32_t RF_ENABLE_CONFIG1_REG_value = RFCU_POWER->RF_ENABLE_CONFIG1_REG;
        uint32_t RF_ENABLE_CONFIG2_REG_value = RFCU_POWER->RF_ENABLE_CONFIG2_REG;

        uint32_t RF_DC_OFFSET_CTRL2_REG_value = DEM->RF_DC_OFFSET_CTRL2_REG;
        uint32_t rf_cal_ctrl_reg_value_local = RFCU->RF_CAL_CTRL_REG;
        uint32_t rf_overrule_reg_value_local = RFCU->RF_OVERRULE_REG;

        uint32_t rf_enable_config46_ble_reg_value  = RFCU_POWER->RF_ENABLE_CONFIG46_BLE_REG;
        uint32_t rf_enable_config46_ftdf_reg_value = RFCU_POWER->RF_ENABLE_CONFIG46_FTDF_REG;

        // Required setting for the DC-offset calibration
        RFCU_POWER->RF_ENABLE_CONFIG0_REG = 0x0000;   // Disable LNA_LDO
        RFCU_POWER->RF_ENABLE_CONFIG1_REG = 0x0000;   // Disable LNA_CORE
        RFCU_POWER->RF_ENABLE_CONFIG2_REG = 0x0000;   // Disable LNA_CGM

        // DCNGAIN = 3, DCNSTEP = 5, DCPARCAL_EN = 0, DCOFFSET_SEL = 0
        DEM->RF_DC_OFFSET_CTRL2_REG = 0x01D0;
        // Disable all calibrations except the DC-offset cal.
        RFCU->RF_CAL_CTRL_REG = 0x002C;

        RFCU_POWER->RF_ENABLE_CONFIG46_BLE_REG  = 0x0000;
        RFCU_POWER->RF_ENABLE_CONFIG46_FTDF_REG = 0x0000;

        REG_SETF(RFCU, RF_OVERRULE_REG, RF_MODE_OVR, 0x1);   // Overrule RF_MODE <- BLE_MODE

        // Clear eo-cal interrupt, so that new calibration can be stared
        RFCU->RF_IRQ_CTRL_REG = 0x0000;

        // Wait until the DC-offset calibration can be started
        while (RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, EO_CAL)) {}

        // Start the modulation gain calibration
        REG_SET_BIT(RFCU, RF_CAL_CTRL_REG, SO_CAL);

        // Wait until the modulation gain calibration has started
        while (!(RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL))) {}

        // Wait until the modulation gain calibration is completed
        while (RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL)) {}

        // Clear eo-cal interrupt, so that new calibration can be started
        RFCU->RF_IRQ_CTRL_REG = 0x0000;

        // Restore the values of registers that were changed in the function
        RFCU_POWER->RF_ENABLE_CONFIG0_REG = RF_ENABLE_CONFIG0_REG_value;
        RFCU_POWER->RF_ENABLE_CONFIG1_REG = RF_ENABLE_CONFIG1_REG_value;
        RFCU_POWER->RF_ENABLE_CONFIG2_REG = RF_ENABLE_CONFIG2_REG_value;

        DEM->RF_DC_OFFSET_CTRL2_REG = RF_DC_OFFSET_CTRL2_REG_value;
        RFCU->RF_CAL_CTRL_REG = rf_cal_ctrl_reg_value_local;
        RFCU->RF_OVERRULE_REG = rf_overrule_reg_value_local;
        RFCU_POWER->RF_ENABLE_CONFIG46_BLE_REG  = rf_enable_config46_ble_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG46_FTDF_REG = rf_enable_config46_ftdf_reg_value;

#endif
}



#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
bool hw_rf_iff_calibration(void)
{
        uint64_t start_time;

        uint16_t calcap_result;

        // Save the values of registers that will be changed in the function
        uint32_t rf_cal_ctrl_reg_value_local = RFCU->RF_CAL_CTRL_REG;

        // Configure the reference oscillator for the calibration
        // 21-04-2015 :  JH  -  Removed to use preferred settings instead
        //uint16_t reg = RFCU->RF_REF_OSC_BLE_REG;
        //REG_SET_FIELD(RFCU, RF_REF_OSC_BLE_REG, CNT_CLK, reg, 0xA6);
        //REG_SET_FIELD(RFCU, RF_REF_OSC_BLE_REG, CNT_RO, reg, 0x2C);
        //RFCU->RF_REF_OSC_BLE_REG = reg;
        //reg = RFCU->RF_REF_OSC_FTDF_REG;
        //REG_SET_FIELD(RFCU, RF_REF_OSC_FTDF_REG, CNT_CLK, reg, 0xA6);
        //REG_SET_FIELD(RFCU, RF_REF_OSC_FTDF_REG, CNT_RO, reg, 0x2C);
        //RFCU->RF_REF_OSC_FTDF_REG = reg;

        // Save the values of registers that will be changed in the function
        uint32_t RF_ENABLE_CONFIG0_BLE_REG_value = RFCU_POWER->RF_ENABLE_CONFIG0_BLE_REG;
        uint32_t RF_ENABLE_CONFIG1_BLE_REG_value = RFCU_POWER->RF_ENABLE_CONFIG1_BLE_REG;
        uint32_t RF_ENABLE_CONFIG2_BLE_REG_value = RFCU_POWER->RF_ENABLE_CONFIG2_BLE_REG;
        uint32_t RF_ENABLE_CONFIG46_BLE_REG_value = RFCU_POWER->RF_ENABLE_CONFIG46_BLE_REG;

        // Required setting for the DC-offset calibration
        RFCU_POWER->RF_ENABLE_CONFIG0_BLE_REG = 0x0000;   // Disable LNA_LDO
        RFCU_POWER->RF_ENABLE_CONFIG1_BLE_REG = 0x0000;   // Disable LNA_CORE
        RFCU_POWER->RF_ENABLE_CONFIG2_BLE_REG = 0x0000;   // Disable LNA_CGM
        RFCU_POWER->RF_ENABLE_CONFIG46_BLE_REG = 0x0000;             // Disable the DCF triggered Partial DCOC

        // Disable all calibrations except the Iff cal.
        RFCU->RF_CAL_CTRL_REG = 0x0034;
        // Select the IF calcap to use the FSM value
        REG_SET_BIT(RFCU, RF_IFF_CTRL1_REG, IF_SELECT_FSM);

        // Clear eo-cal interrupt, so that new calibration can be stared
        RFCU->RF_IRQ_CTRL_REG = 0x0000;


        start_time = hw_rf_get_start_iff_time();

        // Wait until the IFF calibration can be started
        while (RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, EO_CAL)) {
                if (hw_rf_check_iff_timeout(start_time)) {
                        /* IFF lockup detected. Abort */
                        return false;
                }
        }

        /*
         * New code by SR 3/23/2016
         * (Disable partial calibration before IFF calibrations starts)
         *
         * Save current content of RF_DC_OFFSET_CTRl2_REG and set DCPARCAL_EN = 0
         *
         */
        uint16_t rf_dc_offset_ctrl2_reg = DEM->RF_DC_OFFSET_CTRL2_REG; //GetWord16(RF_DC_OFFSET_CTRL2_REG);
        REG_CLR_BIT(DEM, RF_DC_OFFSET_CTRL2_REG, DCPARCAL_EN);

         // Start the IFF calibration
        REG_SET_BIT(RFCU, RF_CAL_CTRL_REG, SO_CAL);

        // Wait until the IFF calibration has started
        while (!(RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL))) {
                if (hw_rf_check_iff_timeout(start_time)) {
                        /* IFF lockup detected. Abort */
                        return false;
                }
        }

        // Wait until the IFF calibration is completed
        while (RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL)) {
                if (hw_rf_check_iff_timeout(start_time)) {
                        /* IFF lockup detected. Abort */
                        return false;
                }
        }

        // Clear eo-cal interrupt, so that new calibration can be started
        RFCU->RF_IRQ_CTRL_REG = 0x0000;

        // Store the result to all IFF CC registers
        calcap_result = RFCU->RF_IFF_CAL_CAP_STAT_REG;
        RFCU->RF_IFF_CC_BLE_SET1_REG = calcap_result;
        RFCU->RF_IFF_CC_BLE_SET2_REG = calcap_result;
        RFCU->RF_IFF_CC_FTDF_SET1_REG = calcap_result;
        RFCU->RF_IFF_CC_FTDF_SET2_REG = calcap_result;

        // Restore the values of registers that were changed in the function
        RFCU->RF_CAL_CTRL_REG = rf_cal_ctrl_reg_value_local;
        // De-select the IF calcap to use the FSM value
        REG_CLR_BIT(RFCU, RF_IFF_CTRL1_REG, IF_SELECT_FSM);


        /*
         * New code by SR 3/23/2016
         * (Disable partial calibration before IFF calibrations starts)
         *
         * Restore RF_DC_OFFSET_CTRl2_REG
         *
         */
        DEM->RF_DC_OFFSET_CTRL2_REG = rf_dc_offset_ctrl2_reg;

        // Restore the values of registers that were changed in the function
        RFCU_POWER->RF_ENABLE_CONFIG0_BLE_REG = RF_ENABLE_CONFIG0_BLE_REG_value;
        RFCU_POWER->RF_ENABLE_CONFIG1_BLE_REG = RF_ENABLE_CONFIG1_BLE_REG_value;
        RFCU_POWER->RF_ENABLE_CONFIG2_BLE_REG = RF_ENABLE_CONFIG2_BLE_REG_value;
        RFCU_POWER->RF_ENABLE_CONFIG46_BLE_REG = RF_ENABLE_CONFIG46_BLE_REG_value;
        return true;
}
#else /* dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_B */
static bool iff_calib_internal(iff_mode_ovr_e rf_mode_ovr, iff_mode_ctrl_e mode_ctrl)
{
        uint16_t calcap_result;
        uint64_t start_time;

        // Set mode
        REG_SETF(RFCU, RF_OVERRULE_REG, RF_MODE_OVR, rf_mode_ovr);

        // Disable all calibrations except the Iff cal.
        RFCU->RF_CAL_CTRL_REG = 0x0034;

        // Select the IF calcap to use the FSM value
        REG_SET_BIT(RFCU, RF_IFF_CTRL1_REG, IF_SELECT_FSM);

        // Clear eo-cal interrupt, so that new calibration can be stared
        RFCU->RF_IRQ_CTRL_REG = 0x0000;

        start_time = hw_rf_get_start_iff_time();

        // Wait until the IFF calibration can be started
        while (RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, EO_CAL)) {
                if (hw_rf_check_iff_timeout(start_time)) {
                        /* IFF lockup detected. Abort */
                        return false;
                }
        }

        // Start the IFF calibration
        REG_SET_BIT(RFCU, RF_CAL_CTRL_REG, SO_CAL);

        // Wait until the IFF calibration has started
        while (!(RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL))) {
                if (hw_rf_check_iff_timeout(start_time)) {
                        /* IFF lockup detected. Abort */
                        return false;
                }
        }

        // Wait until the IFF calibration is completed
        while (RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL)) {
                if (hw_rf_check_iff_timeout(start_time)) {
                        /* IFF lockup detected. Abort */
                        return false;
                }
        }

        // Clear eo-cal interrupt, so that new calibration can be started
        RFCU->RF_IRQ_CTRL_REG = 0x0000;

        // Store the result to all IFF CC BLE registers
        calcap_result = RFCU->RF_IFF_CAL_CAP_STAT_REG;
        if (mode_ctrl != IFF_MODE_CTRL_COMBO || rf_mode_ovr == IFF_MODE_OVR_BLE) {
                RFCU->RF_IFF_CC_BLE_SET1_REG = calcap_result;
                RFCU->RF_IFF_CC_BLE_SET2_REG = calcap_result;
        }

        switch (mode_ctrl) {
        case IFF_MODE_CTRL_BLE: // BLE
                RFCU->RF_IFF_CC_FTDF_SET1_REG = calcap_result;
                RFCU->RF_IFF_CC_FTDF_SET2_REG = calcap_result;
                break;
        case IFF_MODE_CTRL_FTDF: // FTDF
                RFCU->RF_IFF_CC_FTDF_SET1_REG = calcap_result;
                RFCU->RF_IFF_CC_FTDF_SET2_REG = calcap_result;
                break;
        case IFF_MODE_CTRL_COMBO: // COMBO
                if (rf_mode_ovr == IFF_MODE_OVR_FTDF) {
                        RFCU->RF_IFF_CC_FTDF_SET1_REG = calcap_result;
                        RFCU->RF_IFF_CC_FTDF_SET2_REG = calcap_result;
                }
                break;
        }
        return true;
}

static bool hw_rf_iff_calibration(iff_mode_ctrl_e mode_ctrl)
{
        bool ret = false;

        //////////////////////////////////////////////////////////////
        /// Capacitance calibration for different modes
        /// mode_ctrl=0 : Calibration in BLE mode, use offsetted values in FTDF
        /// mode_ctrl=1 : Calibration in FTDF mode, use offsetted values in BLE
        /// mode_ctrl=2 : Calibration done in BLE mode and FTDF mode
        //////////////////////////////////////////////////////////////


        // Save the values of registers that will be changed in the function
        uint16_t rf_cal_ctrl_reg_value_local = RFCU->RF_CAL_CTRL_REG;
        uint16_t rf_overrule_reg_value_local = RFCU->RF_OVERRULE_REG;
        uint16_t rf_enable_config0_reg_value = RFCU_POWER->RF_ENABLE_CONFIG0_REG;
        uint16_t rf_enable_config1_reg_value = RFCU_POWER->RF_ENABLE_CONFIG1_REG;
        uint16_t rf_enable_config2_reg_value = RFCU_POWER->RF_ENABLE_CONFIG2_REG;
        uint16_t rf_enable_config46_ble_reg_value = RFCU_POWER->RF_ENABLE_CONFIG46_BLE_REG;
        uint16_t rf_enable_config46_ftdf_reg_value = RFCU_POWER->RF_ENABLE_CONFIG46_FTDF_REG;
        uint16_t rf_dc_offset_ctrl2_reg_value = DEM->RF_DC_OFFSET_CTRL2_REG;

        // Required setting for the DC-offset calibration
        RFCU_POWER->RF_ENABLE_CONFIG0_REG = 0x0000;   // Disable LNA_LDO
        RFCU_POWER->RF_ENABLE_CONFIG1_REG = 0x0000;   // Disable LNA_CORE
        RFCU_POWER->RF_ENABLE_CONFIG2_REG = 0x0000;   // Disable LNA_CGM
        RFCU_POWER->RF_ENABLE_CONFIG46_BLE_REG = 0x0000;             // Disable the DCF triggered Partial DCOC
        RFCU_POWER->RF_ENABLE_CONFIG46_FTDF_REG = 0x0000;             // Disable the DCF triggered Partial DCOC
        REG_SETF(DEM, RF_DC_OFFSET_CTRL2_REG, DCPARCAL_EN, 0x0);


        switch (mode_ctrl) {
        case IFF_MODE_CTRL_BLE: // BLE
                ret = iff_calib_internal(IFF_MODE_OVR_BLE, IFF_MODE_CTRL_BLE);
                break;
        case IFF_MODE_CTRL_FTDF: // FTDF
                ret = iff_calib_internal(IFF_MODE_OVR_FTDF, IFF_MODE_CTRL_FTDF);
                break;
        case IFF_MODE_CTRL_COMBO: // COMBO
                ret = iff_calib_internal(IFF_MODE_OVR_BLE, IFF_MODE_CTRL_COMBO);
                if (ret)
                        ret = iff_calib_internal(IFF_MODE_OVR_FTDF, IFF_MODE_CTRL_COMBO);
                break;
        default:
                /* should never reach this point */
                ASSERT_WARNING(0);
        }

        if (!ret)
                return false;

        // Restore the values of registers that were changed in the function
        RFCU->RF_CAL_CTRL_REG = rf_cal_ctrl_reg_value_local;
        RFCU->RF_OVERRULE_REG = rf_overrule_reg_value_local;
        RFCU_POWER->RF_ENABLE_CONFIG0_REG = rf_enable_config0_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG1_REG = rf_enable_config1_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG2_REG = rf_enable_config2_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG46_BLE_REG = rf_enable_config46_ble_reg_value;
        RFCU_POWER->RF_ENABLE_CONFIG46_FTDF_REG = rf_enable_config46_ftdf_reg_value;
        DEM->RF_DC_OFFSET_CTRL2_REG = rf_dc_offset_ctrl2_reg_value;

        // De-select the IF calcap to use the FSM value
        REG_CLR_BIT(RFCU, RF_IFF_CTRL1_REG, IF_SELECT_FSM);

        return true;
}
#endif

bool hw_rf_calibration(void)
{
        bool iff_result;

        GLOBAL_INT_DISABLE();
        hw_rf_dc_offset_calibration();
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
        iff_result = hw_rf_iff_calibration();
#else
#if defined(CONFIG_USE_FTDF) && defined(CONFIG_USE_BLE)
        iff_result = hw_rf_iff_calibration(0x2);
#elif defined(CONFIG_USE_BLE)
        iff_result = hw_rf_iff_calibration(0x0);
#elif defined(CONFIG_USE_FTDF)
        iff_result = hw_rf_iff_calibration(0x1);
#endif
#endif
        if (iff_result) {
#ifdef CONFIG_USE_FTDF
                hw_rf_modulation_gain_calibration(0);
#endif
#ifdef CONFIG_USE_BLE
                hw_rf_modulation_gain_calibration(1);
#endif
        }
        GLOBAL_INT_RESTORE();

        return iff_result;
}

void hw_rf_request_on(bool mode_ble)
{
        /* First RF requester. Switch it on */
        if (rf_request == 0) {
                if (rf_state == RF_ST_OFF) {
                        hw_rf_poweron();
                        rf_state = RF_ST_ON;
#if dg_configCOEX_ENABLE_CONFIG == 1
                        hw_coex_apply_config();
#endif
                }
        }

        if (mode_ble) {
                rf_request |= RF_REQUEST_BLE_ON;
        } else {
                rf_request |= RF_REQUEST_FTDF_ON;
        }

}


void hw_rf_request_off(bool mode_ble)
{
        if (mode_ble) {
                rf_request &= ~RF_REQUEST_BLE_ON;
        } else {
                rf_request &= ~RF_REQUEST_FTDF_ON;
        }

        /* If not already off or during recalibration... */
        if (rf_state == RF_ST_ON || rf_state == RF_ST_CONFIG) {
                /* If not RF requesters remain, switch it off */
                if (rf_request == 0) {
                        /* If user pre-off cb returns true, don't shutdown */
                        if (hw_rf_preoff_cb() == false) {
                                hw_rf_poweroff();
                                rf_state = RF_ST_OFF;
                        }
                }
        }
}


bool hw_rf_start_calibration()
{
        bool iff_result;
        if (rf_state == RF_ST_CONFIG) {
                /* Call pre-calibration user cb */
                hw_rf_precalib_cb();

                /* force RADIO_BUSY to BLE to 0 */
                REG_SETF(COEX, COEX_CTRL_REG, SEL_BLE_RADIO_BUSY, 1);
                /* Ignore BLE/FTDF TX/RX_EN */
                REG_SET_BIT(COEX, COEX_CTRL_REG, IGNORE_BLE);
                REG_SET_BIT(COEX, COEX_CTRL_REG, IGNORE_FTDF);

                /* Wait until DCFs have settled */
                while (REG_GETF(COEX, COEX_STAT_REG, COEX_RADIO_BUSY));

                /* Perform DC offset calibration */
                hw_rf_dc_offset_calibration();

                /* Perform IFF calibration */
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
                iff_result = hw_rf_iff_calibration();
#else /* BLACK_ORCA_IC_REV_B */
#if defined(CONFIG_USE_FTDF) && defined(CONFIG_USE_BLE)
                iff_result = hw_rf_iff_calibration(0x2);
#elif defined(CONFIG_USE_BLE)
                iff_result = hw_rf_iff_calibration(0x0);
#elif defined(CONFIG_USE_FTDF)
                iff_result = hw_rf_iff_calibration(0x1);
#endif
#endif

                if (iff_result == false) {
                        return false;
                }

                NVIC_EnableIRQ(RFCAL_IRQn);

#if defined(CONFIG_USE_FTDF) && defined(CONFIG_USE_BLE)
                /* Start the gain calibration, on ftdf mode */
                hw_rf_modulation_gain_calibration_start(0);
                rf_state = RF_ST_WAIT_NEXT1;
#elif defined(CONFIG_USE_FTDF)
                /* Start the gain calibration, on ftdf mode */
                hw_rf_modulation_gain_calibration_start(0);
                rf_state = RF_ST_WAIT_NEXT2;
#elif defined(CONFIG_USE_BLE)
                /* Start the gain calibration, on ble mode */
                hw_rf_modulation_gain_calibration_start(1);
                rf_state = RF_ST_WAIT_NEXT2;
#endif

        }

        return true;
}

void RFCAL_Handler(void)
{
        SEGGER_SYSTEMVIEW_ISR_ENTER();

        uint32_t iser;

        /* Interrupts must be disabled. However, some callbacks actually
         * use FreeRTOS critical sections. To overcome this problem. use
         * ISER/ICER to enable/disable interrupts
         */
        GLOBAL_INT_DISABLE();
        iser = NVIC->ISER[0];;
        NVIC->ICER[0] = iser;
        GLOBAL_INT_RESTORE();

        switch (rf_state) {
        case RF_ST_WAIT_NEXT1:
                /* FTDF gain calib (boot) completed. Proceed with ble gain
                 * calib
                 */
                while (RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL)) {}
                hw_rf_modulation_gain_calibration_end();

                /* Make sure RFCAL IRQ will be enabled when interrupts are enabled */
                iser |= ((uint32_t)(1 << RFCAL_IRQn));

                /* Start the gain calibration, on ble mode */
                hw_rf_modulation_gain_calibration_start(1);

                rf_state = RF_ST_WAIT_NEXT2;
                break;
        case RF_ST_WAIT_NEXT2:
                /* BLE (or ftdf) gain calib (boot) completed.
                 */
                while (RFCU->RF_CAL_CTRL_REG & REG_MSK(RFCU, RF_CAL_CTRL_REG, SO_CAL)) {}
                hw_rf_modulation_gain_calibration_end();

                /* Make sure RFCAL IRQ will be disabled when interrupts are enabled */
                iser &= ~((uint32_t)(1 << RFCAL_IRQn));

                /* Restore state to normal BLE/FTDF MAC operation */
                REG_CLR_BIT(COEX, COEX_CTRL_REG, IGNORE_BLE);
                REG_CLR_BIT(COEX, COEX_CTRL_REG, IGNORE_FTDF);
                REG_SETF(COEX, COEX_CTRL_REG, SEL_BLE_RADIO_BUSY, 0);

                /* Calibration complete. */
                /* Call post-calibration user cb */
                hw_rf_postcalib_cb();

                /* Check if RF is still needed on,
                 * otherwise, switch it off
                 */
                if (rf_request == 0) {
                        hw_rf_poweroff();
                        rf_state = RF_ST_OFF;
                } else {
                        rf_state = RF_ST_CONFIG;
                        hw_rf_postconf_cb();
                }
                break;
        default:
                break;
        }

        /* Re-enable interrupts */
        NVIC->ISER[0] = iser;

        SEGGER_SYSTEMVIEW_ISR_EXIT();
}

void hw_rf_request_recommended_settings(void)
{
        if (rf_state == RF_ST_ON) {
                hw_rf_apply_tcs_cb();
                hw_rf_set_recommended_settings();

                /* Initialize TX Power LUTs */
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
                hw_rf_set_tx_power(rf_tx_power_luts.tx_power_ble);
#else
#ifdef CONFIG_USE_BLE
                hw_rf_set_tx_power_ble(rf_tx_power_luts.tx_power_ble);
#endif
#ifdef CONFIG_USE_FTDF
                hw_rf_set_tx_power_ftdf(rf_tx_power_luts.tx_power_ftdf);
#endif
#endif

                rf_state = RF_ST_CONFIG;
                hw_rf_postconf_cb();
        }
}

bool __attribute__((weak)) hw_rf_preoff_cb(void)
{
        return false;
}

void hw_rf_start_continuous_wave(uint8_t mode, uint8_t ch)
{
        REG_SETF(RFCU,RF_OVERRULE_REG, TX_EN_OVR, 1);   // disable TX overrule
        REG_SETF(RFCU,RF_OVERRULE_REG, TX_EN_OVR, 0);   // NORMAL TX

        REG_SETF(PLLDIG,RF_BMCW_REG, CN_SEL, 1);
        REG_SETF(PLLDIG,RF_BMCW_REG, CN_WR, ch);

        REG_SETF(RFCU,RF_OVERRULE_REG, RF_MODE_OVR, mode);
        REG_SETF(RFCU,RF_OVERRULE_REG, TX_EN_OVR, 2);   // OVERRULE TX ENABLED
}

void hw_rf_start_continuous_wave_rx(uint8_t mode, uint8_t ch)
{
        REG_SETF(RFCU,RF_OVERRULE_REG, RX_EN_OVR, 1);   // disable RX overrule
        REG_SETF(RFCU,RF_OVERRULE_REG, RX_EN_OVR, 0);   // NORMAL RX

        REG_SETF(PLLDIG,RF_BMCW_REG, CN_SEL, 1);
        REG_SETF(PLLDIG,RF_BMCW_REG, CN_WR, ch);

        REG_SETF(RFCU,RF_OVERRULE_REG, RF_MODE_OVR, mode);  // BLE MODE
        REG_SETF(RFCU,RF_OVERRULE_REG, RX_EN_OVR, 2);   // OVERRULE RX ENABLED
}

void hw_rf_stop_continuous_wave(void)
{
        REG_SETF(PLLDIG,RF_BMCW_REG, CN_SEL, 0);
        REG_SETF(PLLDIG,RF_BMCW_REG, CN_WR,  0);

        REG_SETF(RFCU,RF_OVERRULE_REG, TX_EN_OVR, 1);   // disable TX overrule
        REG_SETF(RFCU,RF_OVERRULE_REG, TX_EN_OVR, 0);   // NORMAL TX
        REG_SETF(RFCU,RF_OVERRULE_REG, RX_EN_OVR, 1);   // disable RX overrule
        REG_SETF(RFCU,RF_OVERRULE_REG, RX_EN_OVR, 0);   // NORMAL RX
        REG_SETF(RFCU,RF_OVERRULE_REG, RF_MODE_OVR, 0); // NORMAL RF mode, so disable BLE mode
}

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
void hw_rf_set_tx_power(HW_RF_PWR_LUT_SETTING lut)
{
        RFCU->RF_TX_PWR_REG = lut;
        rf_tx_power_luts.tx_power_ble = lut;
}
#else

#ifdef CONFIG_USE_BLE
void hw_rf_set_tx_power_ble(HW_RF_PWR_LUT_SETTING lut)
{
        RFCU->RF_TX_PWR_BLE_REG = lut;
        rf_tx_power_luts.tx_power_ble = lut;
}
#endif

#ifdef CONFIG_USE_FTDF
void hw_rf_set_tx_power_ftdf(HW_RF_PWR_LUT_SETTING lut)
{
        RFCU->RF_TX_PWR_FTDF_REG = lut;
        rf_tx_power_luts.tx_power_ftdf = lut;
}
#endif

#endif /* dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A */
#endif /* dg_configUSE_HW_RF */
/**
 * \}
 * \}
 * \}
 */
