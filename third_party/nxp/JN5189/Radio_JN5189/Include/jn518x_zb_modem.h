/*
* Copyright (c) 2014 - 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2019 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef __ZB_MODEM_IF_H__
#define __ZB_MODEM_IF_H__

  typedef  struct {
        /* Address 0x000 */
        union {
           __IO uint32_t MCCA_CTRL;

        struct {
           __IO uint32_t  rx_cca_mode : 2;
           __IO uint32_t  rx_cca_thresh : 10;
           __IO uint32_t  rx_cca_or : 1;
           __IO uint32_t  reserved13 : 19;
          } MCCA_CTRL_b;
        };
        /* Address 0x004 */
        union {
           __IO uint32_t MTX_CTRL;

        struct {
           __IO uint32_t  tx_test_en : 1;
           __IO uint32_t  tx_test_mode : 1;
           __IO uint32_t  tx_pol : 1;
           __IO uint32_t  tx_inframe_sync : 1;
           __IO uint32_t  reserved4 : 28;
          } MTX_CTRL_b;
        };
        /* Address 0x008 */
        union {
           __IO uint32_t MTX_TST_SEQ;

        struct {
           __IO uint32_t  tx_test_seq : 32;
          } MTX_TST_SEQ_b;
        };
        /* Address 0x00C */
        union {
           __IO uint32_t MRX_CTRL;

        struct {
           __IO uint32_t  rx_cor_lo : 6;
           __IO uint32_t  rx_cor_hi : 6;
           __IO uint32_t  rx_sync_mode : 2;
           __IO uint32_t  rx_pol : 2;
           __IO uint32_t  rx_pre_num : 3;
           __IO uint32_t  rx_early_eop : 2;
           __IO uint32_t  rx_agc_rst_en : 1;
           __IO uint32_t  rx_agc_rst_thr : 7;
           __IO uint32_t  rx_rssi_avg_en : 1;
           __IO uint32_t  rx_rssi_pulse_mode : 1;
           __IO uint32_t  reserved31 : 1;
          } MRX_CTRL_b;
        };
        /* Address 0x010 */
        union {
           __I  uint32_t MSTAT;

        struct {
           __I  uint32_t  rx_pre_state : 2;
           __I  uint32_t  rx_pre_cnt : 3;
           __I  uint32_t  rx_ad_ant : 1;
           __I  uint32_t  rx_rssi_avg : 10;
           __I  uint32_t  rx_sqi : 8;
           __I  uint32_t  rx_ccas : 1;
           __I  uint32_t  rx_ad_switch : 1;
           __I  uint32_t  reserved26 : 6;
          } MSTAT_b;
        };
        /* Address 0x014 */
        union {
           __IO uint32_t RXFE_CONFIG;

        struct {
           __IO uint32_t  selfi : 4;
           __IO uint32_t  specinv : 1;
           __IO uint32_t  bypass_lpf : 1;
           __IO uint32_t  bypass_adj : 1;
           __IO uint32_t  selfe4demod : 1;
           __IO uint32_t  selfe4agc : 1;
           __IO uint32_t  loopback : 1;
           __IO uint32_t  bitinv : 1;
           __IO uint32_t  xcorr_mode : 1;
           __IO uint32_t  reserved12 : 20;
          } RXFE_CONFIG_b;
        };
        /* Address 0x018 */
        union {
           __IO uint32_t PHY_MCTRL;

        struct {
           __IO uint32_t  reserved0 : 1;
           __IO uint32_t  miom : 1;
           __IO uint32_t  mphyon : 1;
           __IO uint32_t  mphydir : 1;
           __IO uint32_t  mccat : 1;
           __IO uint32_t  medt : 1;
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
    /* ES1 if explicitly configured */
	           __IO uint32_t  reserved6 : 26;
#else
    /* ES2 default */
           __IO uint32_t  aedt : 1;
           __IO uint32_t  reserved7 : 25;
#endif
          } PHY_MCTRL_b;
        };
        /* Address 0x01C */
        union {
           __IO uint32_t PHY_CHAN;

        struct {
           __IO uint32_t  chan : 5;
           __IO uint32_t  unused_6_5 : 2;
           __IO uint32_t  man_freq_en : 1;
           __IO uint32_t  man_freq : 8;
           __IO uint32_t  reserved16 : 16;
          } PHY_CHAN_b;
        };
        /* Address 0x020 */
        union {
           __IO uint32_t PHY_PWR;

        struct {
           __IO uint32_t  pwr : 8;
           __IO uint32_t  reserved8 : 24;
          } PHY_PWR_b;
        };
        /* Address 0x024 */
        union {
           __I  uint32_t RX_PP;

        struct {
           __I  uint32_t  rx_cpp : 10;
           __I  uint32_t  rx_ppp : 10;
           __I  uint32_t  rx_ipp : 10;
           __I  uint32_t  reserved30 : 2;
          } RX_PP_b;
        };
        /* Address 0x028 */
        union {
           __IO uint32_t ANT_DIV_P;

        struct {
           __IO uint32_t  sw_toogle : 1;
           __IO uint32_t  reserved1 : 31;
          } ANT_DIV_P_b;
        };
        /* Address 0x02C */
        union {
           __IO uint32_t ANT_DIV;

        struct {
           __IO uint32_t  ad_rssi_thr : 10;
           __IO uint32_t  rx_ad_en : 1;
           __IO uint32_t  tx_ad_en : 1;
           __IO uint32_t  rx_ad_timer : 4;
           __IO uint32_t  reserved16 : 16;
          } ANT_DIV_b;
        };
        /* Addresses 0x030 - 0xFDC */
        __O  uint32_t  RESERVED030[1004];
        /* Address 0xFE0 */
        union {
           __I  uint32_t ZB_EVENTS_STATUS;

        struct {
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
           __I  uint32_t  status : 13;
           __I  uint32_t  reserved13 : 19;
#else
           __I  uint32_t  status : 15;
           __I  uint32_t  reserved15 : 17;
#endif
          } ZB_EVENTS_STATUS_b;
        };
        /* Address 0xFE4 */
        union {
           __IO uint32_t ZB_EVENTS_ENABLE;

        struct {
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
           __IO uint32_t  enable : 13;
           __IO uint32_t  reserved13 : 19;
#else
           __IO uint32_t  enable : 15;
           __IO uint32_t  reserved15 : 17;
#endif
          } ZB_EVENTS_ENABLE_b;
        };
        /* Address 0xFE8 */
        union {
           __IO uint32_t ZB_EVENTS_CLEAR;

        struct {
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
           __IO uint32_t  clear : 13;
           __IO uint32_t  reserved13 : 19;
#else
           __IO uint32_t  clear : 15;
           __IO uint32_t  reserved15 : 17;
#endif
          } ZB_EVENTS_CLEAR_b;
        };
        /* Address 0xFEC */
        union {
           __IO uint32_t ZB_EVENTS_SET;

        struct {
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
           __IO uint32_t  set : 13;
           __IO uint32_t  reserved13 : 19;
#else
           __IO uint32_t  set : 15;
           __IO uint32_t  reserved15 : 17;
#endif
          } ZB_EVENTS_SET_b;
        };
        /* Address 0xFF0 */
        __O  uint32_t  RESERVEDFF0[1];
        /* Address 0xFF4 */
        union {
           __IO uint32_t POWERDOWN;

        struct {
           __IO uint32_t  soft_reset : 1;
           __IO uint32_t  force_softreset : 1;
           __IO uint32_t  enable_flags_with_core_bus : 1;
           __IO uint32_t  reserved3 : 28;
           __IO uint32_t  powerdown : 1;
          } POWERDOWN_b;
        };
        /* Address 0xFF8 */
        __O  uint32_t  RESERVEDFF8[1];
        /* Address 0xFFC */
        union {
           __I  uint32_t MODULEID;

        struct {
           __I  uint32_t  aperture : 8;
           __I  uint32_t  minor_revision : 4;
           __I  uint32_t  major_revision : 4;
           __I  uint32_t  identifier : 16;
          } MODULEID_b;
        };
    } ZB_MODEM_IF_Type;

  #define ZB_MODEM_MCCA_CTRL_RX_CCA_MODE_Pos 0
  #define ZB_MODEM_MCCA_CTRL_RX_CCA_MODE_Msk 0x03UL
  #define ZB_MODEM_MCCA_CTRL_RX_CCA_THRESH_Pos 2
  #define ZB_MODEM_MCCA_CTRL_RX_CCA_THRESH_Msk 0x0ffcUL
  #define ZB_MODEM_MCCA_CTRL_RX_CCA_OR_Pos 12
  #define ZB_MODEM_MCCA_CTRL_RX_CCA_OR_Msk 0x01000UL
  #define ZB_MODEM_MTX_CTRL_TX_TEST_EN_Pos 0
  #define ZB_MODEM_MTX_CTRL_TX_TEST_EN_Msk 0x01UL
  #define ZB_MODEM_MTX_CTRL_TX_TEST_MODE_Pos 1
  #define ZB_MODEM_MTX_CTRL_TX_TEST_MODE_Msk 0x02UL
  #define ZB_MODEM_MTX_CTRL_TX_POL_Pos 2
  #define ZB_MODEM_MTX_CTRL_TX_POL_Msk 0x04UL
  #define ZB_MODEM_MTX_CTRL_TX_INFRAME_SYNC_Pos 3
  #define ZB_MODEM_MTX_CTRL_TX_INFRAME_SYNC_Msk 0x08UL
  #define ZB_MODEM_MTX_TST_SEQ_TX_TEST_SEQ_Pos 0
  #define ZB_MODEM_MTX_TST_SEQ_TX_TEST_SEQ_Msk 0x0ffffffffUL
  #define ZB_MODEM_MRX_CTRL_RX_COR_LO_Pos 0
  #define ZB_MODEM_MRX_CTRL_RX_COR_LO_Msk 0x03fUL
  #define ZB_MODEM_MRX_CTRL_RX_COR_HI_Pos 6
  #define ZB_MODEM_MRX_CTRL_RX_COR_HI_Msk 0x0fc0UL
  #define ZB_MODEM_MRX_CTRL_RX_SYNC_MODE_Pos 12
  #define ZB_MODEM_MRX_CTRL_RX_SYNC_MODE_Msk 0x03000UL
  #define ZB_MODEM_MRX_CTRL_RX_POL_Pos 14
  #define ZB_MODEM_MRX_CTRL_RX_POL_Msk 0x0c000UL
  #define ZB_MODEM_MRX_CTRL_RX_PRE_NUM_Pos 16
  #define ZB_MODEM_MRX_CTRL_RX_PRE_NUM_Msk 0x070000UL
  #define ZB_MODEM_MRX_CTRL_RX_EARLY_EOP_Pos 19
  #define ZB_MODEM_MRX_CTRL_RX_EARLY_EOP_Msk 0x0180000UL
  #define ZB_MODEM_MRX_CTRL_RX_AGC_RST_EN_Pos 21
  #define ZB_MODEM_MRX_CTRL_RX_AGC_RST_EN_Msk 0x0200000UL
  #define ZB_MODEM_MRX_CTRL_RX_AGC_RST_THR_Pos 22
  #define ZB_MODEM_MRX_CTRL_RX_AGC_RST_THR_Msk 0x01fc00000UL
  #define ZB_MODEM_MRX_CTRL_RX_RSSI_AVG_EN_Pos 29
  #define ZB_MODEM_MRX_CTRL_RX_RSSI_AVG_EN_Msk 0x020000000UL
  #define ZB_MODEM_MRX_CTRL_RX_RSSI_PULSE_MODE_Pos 30
  #define ZB_MODEM_MRX_CTRL_RX_RSSI_PULSE_MODE_Msk 0x040000000UL
  #define ZB_MODEM_MSTAT_RX_PRE_STATE_Pos 0
  #define ZB_MODEM_MSTAT_RX_PRE_STATE_Msk 0x03UL
  #define ZB_MODEM_MSTAT_RX_PRE_CNT_Pos 2
  #define ZB_MODEM_MSTAT_RX_PRE_CNT_Msk 0x01cUL
  #define ZB_MODEM_MSTAT_RX_AD_ANT_Pos 5
  #define ZB_MODEM_MSTAT_RX_AD_ANT_Msk 0x020UL
  #define ZB_MODEM_MSTAT_RX_RSSI_AVG_Pos 6
  #define ZB_MODEM_MSTAT_RX_RSSI_AVG_Msk 0x0ffc0UL
  #define ZB_MODEM_MSTAT_RX_SQI_Pos 16
  #define ZB_MODEM_MSTAT_RX_SQI_Msk 0x0ff0000UL
  #define ZB_MODEM_MSTAT_RX_CCAS_Pos 24
  #define ZB_MODEM_MSTAT_RX_CCAS_Msk 0x01000000UL
  #define ZB_MODEM_MSTAT_RX_AD_SWITCH_Pos 25
  #define ZB_MODEM_MSTAT_RX_AD_SWITCH_Msk 0x02000000UL
  #define ZB_MODEM_RXFE_CONFIG_SELFI_Pos 0
  #define ZB_MODEM_RXFE_CONFIG_SELFI_Msk 0x0fUL
  #define ZB_MODEM_RXFE_CONFIG_SPECINV_Pos 4
  #define ZB_MODEM_RXFE_CONFIG_SPECINV_Msk 0x010UL
  #define ZB_MODEM_RXFE_CONFIG_BYPASS_LPF_Pos 5
  #define ZB_MODEM_RXFE_CONFIG_BYPASS_LPF_Msk 0x020UL
  #define ZB_MODEM_RXFE_CONFIG_BYPASS_ADJ_Pos 6
  #define ZB_MODEM_RXFE_CONFIG_BYPASS_ADJ_Msk 0x040UL
  #define ZB_MODEM_RXFE_CONFIG_SELFE4DEMOD_Pos 7
  #define ZB_MODEM_RXFE_CONFIG_SELFE4DEMOD_Msk 0x080UL
  #define ZB_MODEM_RXFE_CONFIG_SELFE4AGC_Pos 8
  #define ZB_MODEM_RXFE_CONFIG_SELFE4AGC_Msk 0x0100UL
  #define ZB_MODEM_RXFE_CONFIG_LOOPBACK_Pos 9
  #define ZB_MODEM_RXFE_CONFIG_LOOPBACK_Msk 0x0200UL
  #define ZB_MODEM_RXFE_CONFIG_BITINV_Pos 10
  #define ZB_MODEM_RXFE_CONFIG_BITINV_Msk 0x0400UL
  #define ZB_MODEM_RXFE_CONFIG_XCORR_MODE_Pos 11
  #define ZB_MODEM_RXFE_CONFIG_XCORR_MODE_Msk 0x0800UL
  #define ZB_MODEM_PHY_MCTRL_MIOM_Pos 1
  #define ZB_MODEM_PHY_MCTRL_MIOM_Msk 0x02UL
  #define ZB_MODEM_PHY_MCTRL_MPHYON_Pos 2
  #define ZB_MODEM_PHY_MCTRL_MPHYON_Msk 0x04UL
  #define ZB_MODEM_PHY_MCTRL_MPHYDIR_Pos 3
  #define ZB_MODEM_PHY_MCTRL_MPHYDIR_Msk 0x08UL
  #define ZB_MODEM_PHY_MCTRL_MCCAT_Pos 4
  #define ZB_MODEM_PHY_MCTRL_MCCAT_Msk 0x010UL
  #define ZB_MODEM_PHY_MCTRL_MEDT_Pos 5
  #define ZB_MODEM_PHY_MCTRL_MEDT_Msk 0x020UL
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
#else
  #define ZB_MODEM_PHY_MCTRL_AEDT_Pos 6
  #define ZB_MODEM_PHY_MCTRL_AEDT_Msk 0x040UL
#endif
  #define ZB_MODEM_PHY_CHAN_CHAN_Pos 0
  #define ZB_MODEM_PHY_CHAN_CHAN_Msk 0x01fUL
  #define ZB_MODEM_PHY_CHAN_UNUSED_6_5_Pos 5
  #define ZB_MODEM_PHY_CHAN_UNUSED_6_5_Msk 0x060UL
  #define ZB_MODEM_PHY_CHAN_MAN_FREQ_EN_Pos 7
  #define ZB_MODEM_PHY_CHAN_MAN_FREQ_EN_Msk 0x080UL
  #define ZB_MODEM_PHY_CHAN_MAN_FREQ_Pos 8
  #define ZB_MODEM_PHY_CHAN_MAN_FREQ_Msk 0x0ff00UL
  #define ZB_MODEM_PHY_PWR_PWR_Pos 0
  #define ZB_MODEM_PHY_PWR_PWR_Msk 0x0ffUL
  #define ZB_MODEM_RX_PP_RX_CPP_Pos 0
  #define ZB_MODEM_RX_PP_RX_CPP_Msk 0x03ffUL
  #define ZB_MODEM_RX_PP_RX_PPP_Pos 10
  #define ZB_MODEM_RX_PP_RX_PPP_Msk 0x0ffc00UL
  #define ZB_MODEM_RX_PP_RX_IPP_Pos 20
  #define ZB_MODEM_RX_PP_RX_IPP_Msk 0x03ff00000UL
  #define ZB_MODEM_ANT_DIV_P_SW_TOOGLE_Pos 0
  #define ZB_MODEM_ANT_DIV_P_SW_TOOGLE_Msk 0x01UL
  #define ZB_MODEM_ANT_DIV_AD_RSSI_THR_Pos 0
  #define ZB_MODEM_ANT_DIV_AD_RSSI_THR_Msk 0x03ffUL
  #define ZB_MODEM_ANT_DIV_RX_AD_EN_Pos 10
  #define ZB_MODEM_ANT_DIV_RX_AD_EN_Msk 0x0400UL
  #define ZB_MODEM_ANT_DIV_TX_AD_EN_Pos 11
  #define ZB_MODEM_ANT_DIV_TX_AD_EN_Msk 0x0800UL
  #define ZB_MODEM_ANT_DIV_RX_AD_TIMER_Pos 12
  #define ZB_MODEM_ANT_DIV_RX_AD_TIMER_Msk 0x0f000UL
  #define ZB_MODEM_ZB_EVENTS_STATUS_STATUS_Pos 0
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
  #define ZB_MODEM_ZB_EVENTS_STATUS_STATUS_Msk 0x01fffUL
#else
  #define ZB_MODEM_ZB_EVENTS_STATUS_STATUS_Msk 0x07fffUL
#endif
  #define ZB_MODEM_ZB_EVENTS_ENABLE_ENABLE_Pos 0
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
  #define ZB_MODEM_ZB_EVENTS_ENABLE_ENABLE_Msk 0x01fffUL
#else
  #define ZB_MODEM_ZB_EVENTS_ENABLE_ENABLE_Msk 0x07fffUL
#endif
  #define ZB_MODEM_ZB_EVENTS_CLEAR_CLEAR_Pos 0
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
  #define ZB_MODEM_ZB_EVENTS_CLEAR_CLEAR_Msk 0x01fffUL
#else
  #define ZB_MODEM_ZB_EVENTS_CLEAR_CLEAR_Msk 0x07fffUL
#endif
  #define ZB_MODEM_ZB_EVENTS_SET_SET_Pos 0
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
  #define ZB_MODEM_ZB_EVENTS_SET_SET_Msk 0x01fffUL
#else
  #define ZB_MODEM_ZB_EVENTS_SET_SET_Msk 0x07fffUL
#endif
  #define ZB_MODEM_POWERDOWN_SOFT_RESET_Pos 0
  #define ZB_MODEM_POWERDOWN_SOFT_RESET_Msk 0x01UL
  #define ZB_MODEM_POWERDOWN_FORCE_SOFTRESET_Pos 1
  #define ZB_MODEM_POWERDOWN_FORCE_SOFTRESET_Msk 0x02UL
  #define ZB_MODEM_POWERDOWN_ENABLE_FLAGS_WITH_CORE_BUS_Pos 2
  #define ZB_MODEM_POWERDOWN_ENABLE_FLAGS_WITH_CORE_BUS_Msk 0x04UL
  #define ZB_MODEM_POWERDOWN_POWERDOWN_Pos 31
  #define ZB_MODEM_POWERDOWN_POWERDOWN_Msk 0x080000000UL
  #define ZB_MODEM_MODULEID_APERTURE_Pos 0
  #define ZB_MODEM_MODULEID_APERTURE_Msk 0x0ffUL
  #define ZB_MODEM_MODULEID_MINOR_REVISION_Pos 8
  #define ZB_MODEM_MODULEID_MINOR_REVISION_Msk 0x0f00UL
  #define ZB_MODEM_MODULEID_MAJOR_REVISION_Pos 12
  #define ZB_MODEM_MODULEID_MAJOR_REVISION_Msk 0x0f000UL
  #define ZB_MODEM_MODULEID_IDENTIFIER_Pos 16
  #define ZB_MODEM_MODULEID_IDENTIFIER_Msk 0x0ffff0000UL

#endif // __ZB_MODEM_IF_H__
