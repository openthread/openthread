/*
* Copyright (c) 2014 - 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2019 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef __ZB_MAC_IF_H__
#define __ZB_MAC_IF_H__

  typedef  struct {
        /* Address 0x000 */
        union {
           __IO uint32_t SCMR0;

        struct {
           __IO uint32_t  scmr0 : 32;
          } SCMR0_b;
        };
        /* Address 0x004 */
        union {
           __IO uint32_t SCMR1;

        struct {
           __IO uint32_t  scmr1 : 32;
          } SCMR1_b;
        };
        /* Address 0x008 */
        union {
           __IO uint32_t SCMR2;

        struct {
           __IO uint32_t  scmr2 : 32;
          } SCMR2_b;
        };
        /* Address 0x00C */
        union {
           __IO uint32_t SCMR3;

        struct {
           __IO uint32_t  scmr3 : 32;
          } SCMR3_b;
        };
        /* Address 0x010 */
        union {
           __IO uint32_t SCTR0;

        struct {
           __IO uint32_t  sctr0 : 32;
          } SCTR0_b;
        };
        /* Address 0x014 */
        union {
           __IO uint32_t SCTR1;

        struct {
           __IO uint32_t  sctr1 : 32;
          } SCTR1_b;
        };
        /* Addresses 0x018 - 0x01C */
        __O  uint32_t  RESERVED018[2];
        /* Address 0x020 */
        union {
           __IO uint32_t SCTCR;

        struct {
           __IO uint32_t  sctcr : 2;
           __IO uint32_t  reserved2 : 30;
          } SCTCR_b;
        };
        /* Address 0x024 */
        union {
           __IO uint32_t SCFRC;

        struct {
           __IO uint32_t  scfrc : 32;
          } SCFRC_b;
        };
        /* Address 0x028 */
        union {
           __I  uint32_t SCSSR;

        struct {
           __I  uint32_t  scssr : 32;
          } SCSSR_b;
        };
        /* Address 0x02C */
        union {
           __IO uint32_t SCESL;

        struct {
           __IO uint32_t  scesl : 32;
          } SCESL_b;
        };
        /* Address 0x030 */
        union {
           __IO uint32_t RXETST;

        struct {
           __IO uint32_t  rxetst : 32;
          } RXETST_b;
        };
        /* Address 0x034 */
        union {
           __I  uint32_t RXTSTP;

        struct {
           __I  uint32_t  rxtstp : 32;
          } RXTSTP_b;
        };
        /* Address 0x038 */
        union {
           __IO uint32_t TXTSTP;

        struct {
           __IO uint32_t  txtstp : 32;
          } TXTSTP_b;
        };
        /* Address 0x03C */
        union {
           __IO uint32_t IER;

        struct {
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
           __IO uint32_t  ier : 10;
           __IO uint32_t  reserved10 : 22;
#else
           __IO uint32_t  ier : 12;
           __IO uint32_t  reserved12 : 20;
#endif
          } IER_b;
        };
        /* Address 0x040 */
        union {
           __IO uint32_t ISR;

        struct {
           __IO uint32_t  txint : 1;
           __IO uint32_t  headerrxint : 1;
           __IO uint32_t  rxint : 1;
           __IO uint32_t  unused_3_3 : 1;
           __IO uint32_t  match : 4;
           __IO uint32_t  timeout : 2;
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
           __IO uint32_t  reserved10 : 22;
#else
           __IO uint32_t  rxaddnomatch : 1;
           __IO uint32_t  rxaddmatch : 1;
           __IO uint32_t  reserved12 : 20;
#endif
          } ISR_b;
        };
        /* Address 0x044 */
        union {
           __IO uint32_t SCTL;

        struct {
           __IO uint32_t  sctl : 7;
           __IO uint32_t  reserved7 : 25;
          } SCTL_b;
        };
        /* Address 0x048 */
        union {
           __IO uint32_t PHYTXTUNETIME;

        struct {
           __IO uint32_t  phytxtunetime : 8;
           __IO uint32_t  phyrx2txtime : 8;
           __IO uint32_t  reserved16 : 16;
          } PHYTXTUNETIME_b;
        };
        /* Address 0x04C */
        union {
           __IO uint32_t PHYRXTUNETIME;

        struct {
           __IO uint32_t  phyrxtunetime : 8;
           __IO uint32_t  reserved8 : 24;
          } PHYRXTUNETIME_b;
        };
        /* Address 0x050 */
        __O  uint32_t  RESERVED050[1];
        /* Address 0x054 */
        union {
           __IO uint32_t TURNAROUNDTIME;

        struct {
           __IO uint32_t  turnaroundtime : 8;
           __IO uint32_t  reserved8 : 24;
          } TURNAROUNDTIME_b;
        };
        /* Address 0x058 */
        union {
           __IO uint32_t BACKOFFTIME;

        struct {
           __IO uint32_t  backofftime : 8;
           __IO uint32_t  reserved8 : 24;
          } BACKOFFTIME_b;
        };
        /* Addresses 0x05C - 0x06C */
        __O  uint32_t  RESERVED05C[5];
        /* Address 0x070 */
        union {
           __IO uint32_t PRBS_SEED;

        struct {
           __IO uint32_t  prbs_seed : 15;
           __IO uint32_t  reserved15 : 17;
          } PRBS_SEED_b;
        };
        /* Addresses 0x074 - 0x078 */
        __O  uint32_t  RESERVED074[2];
        /* Address 0x07C */
        union {
           __IO uint32_t SM_STATE;

        struct {
           __IO uint32_t  sm_state : 7;
           __IO uint32_t  reserved7 : 25;
          } SM_STATE_b;
        };
        /* Address 0x080 */
        union {
           __IO uint32_t LIFSTURNAROUNDTIME;

        struct {
           __IO uint32_t  lifsturnaroundtime : 8;
           __IO uint32_t  reserved8 : 24;
          } LIFSTURNAROUNDTIME_b;
        };
        /* Address 0x084 */
        union {
           __IO uint32_t HDR_CTRL;

        struct {
           __IO uint32_t  hdr_ctrl : 2;
           __IO uint32_t  reserved2 : 30;
          } HDR_CTRL_b;
        };
        /* Address 0x088 */
        union {
           __IO uint32_t MISR;

        struct {
           __IO uint32_t  misr : 10;
           __IO uint32_t  reserved10 : 22;
          } MISR_b;
        };
        /* Addresses 0x08C - 0x0F8 */
        __O  uint32_t  RESERVED08C[28];
        /* Address 0x0FC */
        union {
           __I  uint32_t IDENTIFIER;

        struct {
           __I  uint32_t  identifier : 32;
          } IDENTIFIER_b;
        };
        /* Addresses 0x100 - 0x1BC */
        __O  uint32_t  RESERVED100[48];
        /* Address 0x1C0 */
        union {
           __IO uint32_t TXCTL;

        struct {
           __IO uint32_t  sch : 1;
           __IO uint32_t  ss : 1;
           __IO uint32_t  slotack : 1;
           __IO uint32_t  aa : 1;
           __IO uint32_t  mode : 2;
           __IO uint32_t  reserved6 : 26;
          } TXCTL_b;
        };
        /* Address 0x1C4 */
        union {
           __I  uint32_t TXSTAT;

        struct {
           __I  uint32_t  txstat : 5;
           __I  uint32_t  txto : 1;
           __I  uint32_t  txpcto : 1;
           __I  uint32_t  reserved7 : 25;
          } TXSTAT_b;
        };
        /* Address 0x1C8 */
        union {
           __IO uint32_t TXMBEBT;

        struct {
           __IO uint32_t  minbe : 4;
           __IO uint32_t  maxbo : 3;
           __IO uint32_t  ble : 1;
           __IO uint32_t  maxbe : 4;
           __IO uint32_t  frcdly : 1;
           __IO uint32_t  hide : 4;
           __IO uint32_t  reserved17 : 15;
          } TXMBEBT_b;
        };
        /* Address 0x1CC */
        union {
           __IO uint32_t TXCSMAC;

        struct {
           __IO uint32_t  txcsmac : 18;
           __IO uint32_t  reserved18 : 14;
          } TXCSMAC_b;
        };
        /* Address 0x1D0 */
        union {
           __IO uint32_t TXTRIES;

        struct {
           __IO uint32_t  txtries : 4;
           __IO uint32_t  reserved4 : 28;
          } TXTRIES_b;
        };
        /* Address 0x1D4 */
        union {
           __IO uint32_t TXPEND;

        struct {
           __IO uint32_t  txpend : 1;
           __IO uint32_t  reserved1 : 31;
          } TXPEND_b;
        };
        /* Address 0x1D8 */
        union {
           __IO uint32_t TXASC;

        struct {
           __IO uint32_t  txasc : 1;
           __IO uint32_t  reserved1 : 31;
          } TXASC_b;
        };
        /* Address 0x1DC */
        union {
           __IO uint32_t TXBUFAD;

        struct {
           __IO uint32_t  txbufad : 32;
          } TXBUFAD_b;
        };
        /* Address 0x1E0 */
        union {
           __IO uint32_t TXTO;

        struct {
           __IO uint32_t  tx_to_len : 7;
           __IO uint32_t  tx_to_ena : 1;
           __IO uint32_t  tx_topc_ena : 1;
           __IO uint32_t  reserved9 : 23;
          } TXTO_b;
        };
        /* Addresses 0x1E4 - 0x2BC */
        __O  uint32_t  RESERVED1E4[55];
        /* Address 0x2C0 */
        union {
           __IO uint32_t RXCTL;

        struct {
           __IO uint32_t  rxctl : 5;
           __IO uint32_t  reserved5 : 27;
          } RXCTL_b;
        };
        /* Address 0x2C4 */
        union {
           __I  uint32_t RXSTAT;

        struct {
           __I  uint32_t  fcs_error : 1;
           __I  uint32_t  abort : 1;
           __I  uint32_t  unused_2_2 : 1;
           __I  uint32_t  unused_3_3 : 1;
           __I  uint32_t  mod_in_packet : 1;
           __I  uint32_t  malformed : 1;
           __I  uint32_t  reserved6 : 26;
          } RXSTAT_b;
        };
        /* Address 0x2C8 */
        union {
           __IO uint32_t RXMPID;

        struct {
           __IO uint32_t  rxmpid : 18;
           __IO uint32_t  reserved18 : 14;
          } RXMPID_b;
        };
        /* Address 0x2CC */
        union {
           __IO uint32_t RXMSAD;

        struct {
           __IO uint32_t  rxmsad : 16;
           __IO uint32_t  reserved16 : 16;
          } RXMSAD_b;
        };
        /* Address 0x2D0 */
        union {
           __IO uint32_t RXMEADL;

        struct {
           __IO uint32_t  rxmeadl : 32;
          } RXMEADL_b;
        };
        /* Address 0x2D4 */
        union {
           __IO uint32_t RXMEADH;

        struct {
           __IO uint32_t  rxmeadh : 32;
          } RXMEADH_b;
        };
        /* Address 0x2D8 */
        __O  uint32_t  RESERVED2D8[1];
        /* Address 0x2DC */
        union {
           __IO uint32_t RXBUFAD;

        struct {
           __IO uint32_t  rxbufad : 32;
          } RXBUFAD_b;
        };
        /* Address 0x2E0 */
        union {
           __IO uint32_t RXPROM;

        struct {
           __IO uint32_t  rxprom : 2;
           __IO uint32_t  reserved2 : 30;
          } RXPROM_b;
        };
        /* Address 0x2E4 */
        union {
           __IO uint32_t RXMPID_SEC;

        struct {
           __IO uint32_t  rxmpid_sec : 19;
           __IO uint32_t  reserved19 : 13;
          } RXMPID_SEC_b;
        };
        /* Address 0x2E8 */
        union {
           __IO uint32_t RXMSAD_SEC;

        struct {
           __IO uint32_t  rxmsad_sec : 16;
           __IO uint32_t  reserved16 : 16;
          } RXMSAD_SEC_b;
        };
        /* Address 0x2EC */
        union {
           __IO uint32_t RXMEADL_SEC;

        struct {
           __IO uint32_t  rxmeadl_sec : 32;
          } RXMEADL_SEC_b;
        };
        /* Address 0x2F0 */
        union {
           __IO uint32_t RXMEADH_SEC;

        struct {
           __IO uint32_t  rxmeadh_sec : 32;
          } RXMEADH_SEC_b;
        };
        /* Addresses 0x2F4 - 0x2FC */
        __O  uint32_t  RESERVED2F4[3];
        /* Address 0x300 */
        union {
           __IO uint32_t DMA_ADDR;

        struct {
           __IO uint32_t  dma_addr : 32;
          } DMA_ADDR_b;
        };
        /* Address 0x304 */
        union {
           __I  uint32_t DMA_RD_DATA;

        struct {
           __I  uint32_t  dma_rd_data : 32;
          } DMA_RD_DATA_b;
        };
        /* Address 0x308 */
        union {
           __IO uint32_t DMA_WR_DATA;

        struct {
           __IO uint32_t  dma_wr_data : 32;
          } DMA_WR_DATA_b;
        };
        /* Address 0x30C */
        union {
           __IO uint32_t DMA_ADDR_OFFSET;

        struct {
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
           __IO uint32_t  reserved0 : 15;
           __IO uint32_t  dma_addr_offset : 17;
#else
           __IO uint32_t  reserved0 : 17;
           __IO uint32_t  dma_addr_offset : 15;
#endif
          } DMA_ADDR_OFFSET_b;
        };
        /* Addresses 0x310 - 0x3FC */
        __O  uint32_t  RESERVED310[60];
        /* Address 0x400 */
        union {
           __IO uint32_t DMA_FROM_ADC;

        struct {
           __IO uint32_t  activate : 1;
           __IO uint32_t  mode : 4;
#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
           __IO uint32_t  reserved5 : 27;
#else
           __IO uint32_t  safe : 1;
           __IO uint32_t  reserved6 : 26;
#endif
          } DMA_FROM_ADC_b;
        };
        /* Address 0x404 */
        union {
           __IO uint32_t PHY_MCTRL;

        struct {
           __IO uint32_t  bbc_rmi_req : 1;
           __IO uint32_t  reserved1 : 31;
          } PHY_MCTRL_b;
        };
        /* Address 0x408 */
        union {
           __I  uint32_t PHY_MSTATUS;

        struct {
           __I  uint32_t  bbc_rmi_ack : 1;
           __I  uint32_t  reserved1 : 31;
          } PHY_MSTATUS_b;
        };
        /* Addresses 0x40C - 0xFF0 */
        __O  uint32_t  RESERVED40C[762];
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
    } ZB_MAC_IF_Type;

#if defined(CPU_JN518X_REV)&&(CPU_JN518X_REV == 1)
  #define ZBMAC_SCMR0_SCMR0_Pos 0
  #define ZBMAC_SCMR0_SCMR0_Msk 0x0ffffffffUL
  #define ZBMAC_SCMR1_SCMR1_Pos 0
  #define ZBMAC_SCMR1_SCMR1_Msk 0x0ffffffffUL
  #define ZBMAC_SCMR2_SCMR2_Pos 0
  #define ZBMAC_SCMR2_SCMR2_Msk 0x0ffffffffUL
  #define ZBMAC_SCMR3_SCMR3_Pos 0
  #define ZBMAC_SCMR3_SCMR3_Msk 0x0ffffffffUL
  #define ZBMAC_SCTR0_SCTR0_Pos 0
  #define ZBMAC_SCTR0_SCTR0_Msk 0x0ffffffffUL
  #define ZBMAC_SCTR1_SCTR1_Pos 0
  #define ZBMAC_SCTR1_SCTR1_Msk 0x0ffffffffUL
  #define ZBMAC_SCTCR_SCTCR_Pos 0
  #define ZBMAC_SCTCR_SCTCR_Msk 0x03UL
  #define ZBMAC_SCFRC_SCFRC_Pos 0
  #define ZBMAC_SCFRC_SCFRC_Msk 0x0ffffffffUL
  #define ZBMAC_SCSSR_SCSSR_Pos 0
  #define ZBMAC_SCSSR_SCSSR_Msk 0x0ffffffffUL
  #define ZBMAC_SCESL_SCESL_Pos 0
  #define ZBMAC_SCESL_SCESL_Msk 0x0ffffffffUL
  #define ZBMAC_RXETST_RXETST_Pos 0
  #define ZBMAC_RXETST_RXETST_Msk 0x0ffffffffUL
  #define ZBMAC_RXTSTP_RXTSTP_Pos 0
  #define ZBMAC_RXTSTP_RXTSTP_Msk 0x0ffffffffUL
  #define ZBMAC_TXTSTP_TXTSTP_Pos 0
  #define ZBMAC_TXTSTP_TXTSTP_Msk 0x0ffffffffUL
  #define ZBMAC_IER_IER_Pos 0
  #define ZBMAC_IER_IER_Msk 0x03ffUL
  #define ZBMAC_ISR_TXINT_Pos 0
  #define ZBMAC_ISR_TXINT_Msk 0x01UL
  #define ZBMAC_ISR_HEADERRXINT_Pos 1
  #define ZBMAC_ISR_HEADERRXINT_Msk 0x02UL
  #define ZBMAC_ISR_RXINT_Pos 2
  #define ZBMAC_ISR_RXINT_Msk 0x04UL
  #define ZBMAC_ISR_UNUSED_3_3_Pos 3
  #define ZBMAC_ISR_UNUSED_3_3_Msk 0x08UL
  #define ZBMAC_ISR_MATCH_Pos 4
  #define ZBMAC_ISR_MATCH_Msk 0x0f0UL
  #define ZBMAC_ISR_TIMEOUT_Pos 8
  #define ZBMAC_ISR_TIMEOUT_Msk 0x0300UL
  #define ZBMAC_SCTL_SCTL_Pos 0
  #define ZBMAC_SCTL_SCTL_Msk 0x07fUL
  #define ZBMAC_PHYTXTUNETIME_PHYTXTUNETIME_Pos 0
  #define ZBMAC_PHYTXTUNETIME_PHYTXTUNETIME_Msk 0x0ffUL
  #define ZBMAC_PHYTXTUNETIME_PHYRX2TXTIME_Pos 8
  #define ZBMAC_PHYTXTUNETIME_PHYRX2TXTIME_Msk 0x0ff00UL
  #define ZBMAC_PHYRXTUNETIME_PHYRXTUNETIME_Pos 0
  #define ZBMAC_PHYRXTUNETIME_PHYRXTUNETIME_Msk 0x0ffUL
  #define ZBMAC_TURNAROUNDTIME_TURNAROUNDTIME_Pos 0
  #define ZBMAC_TURNAROUNDTIME_TURNAROUNDTIME_Msk 0x0ffUL
  #define ZBMAC_BACKOFFTIME_BACKOFFTIME_Pos 0
  #define ZBMAC_BACKOFFTIME_BACKOFFTIME_Msk 0x0ffUL
  #define ZBMAC_PRBS_SEED_PRBS_SEED_Pos 0
  #define ZBMAC_PRBS_SEED_PRBS_SEED_Msk 0x07fffUL
  #define ZBMAC_SM_STATE_SM_STATE_Pos 0
  #define ZBMAC_SM_STATE_SM_STATE_Msk 0x07fUL
  #define ZBMAC_LIFSTURNAROUNDTIME_LIFSTURNAROUNDTIME_Pos 0
  #define ZBMAC_LIFSTURNAROUNDTIME_LIFSTURNAROUNDTIME_Msk 0x0ffUL
  #define ZBMAC_HDR_CTRL_HDR_CTRL_Pos 0
  #define ZBMAC_HDR_CTRL_HDR_CTRL_Msk 0x03UL
  #define ZBMAC_MISR_MISR_Pos 0
  #define ZBMAC_MISR_MISR_Msk 0x03ffUL
  #define ZBMAC_IDENTIFIER_IDENTIFIER_Pos 0
  #define ZBMAC_IDENTIFIER_IDENTIFIER_Msk 0x0ffffffffUL
  #define ZBMAC_TXCTL_SCH_Pos 0
  #define ZBMAC_TXCTL_SCH_Msk 0x01UL
  #define ZBMAC_TXCTL_SS_Pos 1
  #define ZBMAC_TXCTL_SS_Msk 0x02UL
  #define ZBMAC_TXCTL_SLOTACK_Pos 2
  #define ZBMAC_TXCTL_SLOTACK_Msk 0x04UL
  #define ZBMAC_TXCTL_AA_Pos 3
  #define ZBMAC_TXCTL_AA_Msk 0x08UL
  #define ZBMAC_TXCTL_MODE_Pos 4
  #define ZBMAC_TXCTL_MODE_Msk 0x030UL
  #define ZBMAC_TXSTAT_TXSTAT_Pos 0
  #define ZBMAC_TXSTAT_TXSTAT_Msk 0x01fUL
  #define ZBMAC_TXSTAT_TXTO_Pos 5
  #define ZBMAC_TXSTAT_TXTO_Msk 0x020UL
  #define ZBMAC_TXSTAT_TXPCTO_Pos 6
  #define ZBMAC_TXSTAT_TXPCTO_Msk 0x040UL
  #define ZBMAC_TXMBEBT_MINBE_Pos 0
  #define ZBMAC_TXMBEBT_MINBE_Msk 0x0fUL
  #define ZBMAC_TXMBEBT_MAXBO_Pos 4
  #define ZBMAC_TXMBEBT_MAXBO_Msk 0x070UL
  #define ZBMAC_TXMBEBT_BLE_Pos 7
  #define ZBMAC_TXMBEBT_BLE_Msk 0x080UL
  #define ZBMAC_TXMBEBT_MAXBE_Pos 8
  #define ZBMAC_TXMBEBT_MAXBE_Msk 0x0f00UL
  #define ZBMAC_TXMBEBT_FRCDLY_Pos 12
  #define ZBMAC_TXMBEBT_FRCDLY_Msk 0x01000UL
  #define ZBMAC_TXMBEBT_HIDE_Pos 13
  #define ZBMAC_TXMBEBT_HIDE_Msk 0x01e000UL
  #define ZBMAC_TXCSMAC_TXCSMAC_Pos 0
  #define ZBMAC_TXCSMAC_TXCSMAC_Msk 0x03ffffUL
  #define ZBMAC_TXTRIES_TXTRIES_Pos 0
  #define ZBMAC_TXTRIES_TXTRIES_Msk 0x0fUL
  #define ZBMAC_TXPEND_TXPEND_Pos 0
  #define ZBMAC_TXPEND_TXPEND_Msk 0x01UL
  #define ZBMAC_TXASC_TXASC_Pos 0
  #define ZBMAC_TXASC_TXASC_Msk 0x01UL
  #define ZBMAC_TXBUFAD_TXBUFAD_Pos 0
  #define ZBMAC_TXBUFAD_TXBUFAD_Msk 0x0ffffffffUL
  #define ZBMAC_TXTO_TX_TO_LEN_Pos 0
  #define ZBMAC_TXTO_TX_TO_LEN_Msk 0x07fUL
  #define ZBMAC_TXTO_TX_TO_ENA_Pos 7
  #define ZBMAC_TXTO_TX_TO_ENA_Msk 0x080UL
  #define ZBMAC_TXTO_TX_TOPC_ENA_Pos 8
  #define ZBMAC_TXTO_TX_TOPC_ENA_Msk 0x0100UL
  #define ZBMAC_RXCTL_RXCTL_Pos 0
  #define ZBMAC_RXCTL_RXCTL_Msk 0x01fUL
  #define ZBMAC_RXSTAT_FCS_ERROR_Pos 0
  #define ZBMAC_RXSTAT_FCS_ERROR_Msk 0x01UL
  #define ZBMAC_RXSTAT_ABORT_Pos 1
  #define ZBMAC_RXSTAT_ABORT_Msk 0x02UL
  #define ZBMAC_RXSTAT_UNUSED_2_2_Pos 2
  #define ZBMAC_RXSTAT_UNUSED_2_2_Msk 0x04UL
  #define ZBMAC_RXSTAT_UNUSED_3_3_Pos 3
  #define ZBMAC_RXSTAT_UNUSED_3_3_Msk 0x08UL
  #define ZBMAC_RXSTAT_MOD_IN_PACKET_Pos 4
  #define ZBMAC_RXSTAT_MOD_IN_PACKET_Msk 0x010UL
  #define ZBMAC_RXSTAT_MALFORMED_Pos 5
  #define ZBMAC_RXSTAT_MALFORMED_Msk 0x020UL
  #define ZBMAC_RXMPID_RXMPID_Pos 0
  #define ZBMAC_RXMPID_RXMPID_Msk 0x03ffffUL
  #define ZBMAC_RXMSAD_RXMSAD_Pos 0
  #define ZBMAC_RXMSAD_RXMSAD_Msk 0x0ffffUL
  #define ZBMAC_RXMEADL_RXMEADL_Pos 0
  #define ZBMAC_RXMEADL_RXMEADL_Msk 0x0ffffffffUL
  #define ZBMAC_RXMEADH_RXMEADH_Pos 0
  #define ZBMAC_RXMEADH_RXMEADH_Msk 0x0ffffffffUL
  #define ZBMAC_RXBUFAD_RXBUFAD_Pos 0
  #define ZBMAC_RXBUFAD_RXBUFAD_Msk 0x0ffffffffUL
  #define ZBMAC_RXPROM_RXPROM_Pos 0
  #define ZBMAC_RXPROM_RXPROM_Msk 0x03UL
  #define ZBMAC_RXMPID_SEC_RXMPID_SEC_Pos 0
  #define ZBMAC_RXMPID_SEC_RXMPID_SEC_Msk 0x07ffffUL
  #define ZBMAC_RXMSAD_SEC_RXMSAD_SEC_Pos 0
  #define ZBMAC_RXMSAD_SEC_RXMSAD_SEC_Msk 0x0ffffUL
  #define ZBMAC_RXMEADL_SEC_RXMEADL_SEC_Pos 0
  #define ZBMAC_RXMEADL_SEC_RXMEADL_SEC_Msk 0x0ffffffffUL
  #define ZBMAC_RXMEADH_SEC_RXMEADH_SEC_Pos 0
  #define ZBMAC_RXMEADH_SEC_RXMEADH_SEC_Msk 0x0ffffffffUL
  #define ZBMAC_DMA_ADDR_DMA_ADDR_Pos 0
  #define ZBMAC_DMA_ADDR_DMA_ADDR_Msk 0x0ffffffffUL
  #define ZBMAC_DMA_RD_DATA_DMA_RD_DATA_Pos 0
  #define ZBMAC_DMA_RD_DATA_DMA_RD_DATA_Msk 0x0ffffffffUL
  #define ZBMAC_DMA_WR_DATA_DMA_WR_DATA_Pos 0
  #define ZBMAC_DMA_WR_DATA_DMA_WR_DATA_Msk 0x0ffffffffUL
  #define ZBMAC_DMA_ADDR_OFFSET_DMA_ADDR_OFFSET_Pos 15
  #define ZBMAC_DMA_ADDR_OFFSET_DMA_ADDR_OFFSET_Msk 0x0ffff8000UL
  #define ZBMAC_DMA_FROM_ADC_ACTIVATE_Pos 0
  #define ZBMAC_DMA_FROM_ADC_ACTIVATE_Msk 0x01UL
  #define ZBMAC_DMA_FROM_ADC_MODE_Pos 1
  #define ZBMAC_DMA_FROM_ADC_MODE_Msk 0x01eUL
  #define ZBMAC_PHY_MCTRL_BBC_RMI_REQ_Pos 0
  #define ZBMAC_PHY_MCTRL_BBC_RMI_REQ_Msk 0x01UL
  #define ZBMAC_PHY_MSTATUS_BBC_RMI_ACK_Pos 0
  #define ZBMAC_PHY_MSTATUS_BBC_RMI_ACK_Msk 0x01UL
  #define ZBMAC_POWERDOWN_SOFT_RESET_Pos 0
  #define ZBMAC_POWERDOWN_SOFT_RESET_Msk 0x01UL
  #define ZBMAC_POWERDOWN_FORCE_SOFTRESET_Pos 1
  #define ZBMAC_POWERDOWN_FORCE_SOFTRESET_Msk 0x02UL
  #define ZBMAC_POWERDOWN_ENABLE_FLAGS_WITH_CORE_BUS_Pos 2
  #define ZBMAC_POWERDOWN_ENABLE_FLAGS_WITH_CORE_BUS_Msk 0x04UL
  #define ZBMAC_POWERDOWN_POWERDOWN_Pos 31
  #define ZBMAC_POWERDOWN_POWERDOWN_Msk 0x080000000UL
  #define ZBMAC_MODULEID_APERTURE_Pos 0
  #define ZBMAC_MODULEID_APERTURE_Msk 0x0ffUL
  #define ZBMAC_MODULEID_MINOR_REVISION_Pos 8
  #define ZBMAC_MODULEID_MINOR_REVISION_Msk 0x0f00UL
  #define ZBMAC_MODULEID_MAJOR_REVISION_Pos 12
  #define ZBMAC_MODULEID_MAJOR_REVISION_Msk 0x0f000UL
  #define ZBMAC_MODULEID_IDENTIFIER_Pos 16
  #define ZBMAC_MODULEID_IDENTIFIER_Msk 0x0ffff0000UL
#else
  #define ZB_MAC_SCMR0_SCMR0_Pos 0
  #define ZB_MAC_SCMR0_SCMR0_Msk 0x0ffffffffUL
  #define ZB_MAC_SCMR1_SCMR1_Pos 0
  #define ZB_MAC_SCMR1_SCMR1_Msk 0x0ffffffffUL
  #define ZB_MAC_SCMR2_SCMR2_Pos 0
  #define ZB_MAC_SCMR2_SCMR2_Msk 0x0ffffffffUL
  #define ZB_MAC_SCMR3_SCMR3_Pos 0
  #define ZB_MAC_SCMR3_SCMR3_Msk 0x0ffffffffUL
  #define ZB_MAC_SCTR0_SCTR0_Pos 0
  #define ZB_MAC_SCTR0_SCTR0_Msk 0x0ffffffffUL
  #define ZB_MAC_SCTR1_SCTR1_Pos 0
  #define ZB_MAC_SCTR1_SCTR1_Msk 0x0ffffffffUL
  #define ZB_MAC_SCTCR_SCTCR_Pos 0
  #define ZB_MAC_SCTCR_SCTCR_Msk 0x03UL
  #define ZB_MAC_SCFRC_SCFRC_Pos 0
  #define ZB_MAC_SCFRC_SCFRC_Msk 0x0ffffffffUL
  #define ZB_MAC_SCSSR_SCSSR_Pos 0
  #define ZB_MAC_SCSSR_SCSSR_Msk 0x0ffffffffUL
  #define ZB_MAC_SCESL_SCESL_Pos 0
  #define ZB_MAC_SCESL_SCESL_Msk 0x0ffffffffUL
  #define ZB_MAC_RXETST_RXETST_Pos 0
  #define ZB_MAC_RXETST_RXETST_Msk 0x0ffffffffUL
  #define ZB_MAC_RXTSTP_RXTSTP_Pos 0
  #define ZB_MAC_RXTSTP_RXTSTP_Msk 0x0ffffffffUL
  #define ZB_MAC_TXTSTP_TXTSTP_Pos 0
  #define ZB_MAC_TXTSTP_TXTSTP_Msk 0x0ffffffffUL
  #define ZB_MAC_IER_IER_Pos 0
  #define ZB_MAC_IER_IER_Msk 0x0fffUL
  #define ZB_MAC_ISR_TXINT_Pos 0
  #define ZB_MAC_ISR_TXINT_Msk 0x01UL
  #define ZB_MAC_ISR_HEADERRXINT_Pos 1
  #define ZB_MAC_ISR_HEADERRXINT_Msk 0x02UL
  #define ZB_MAC_ISR_RXINT_Pos 2
  #define ZB_MAC_ISR_RXINT_Msk 0x04UL
  #define ZB_MAC_ISR_UNUSED_3_3_Pos 3
  #define ZB_MAC_ISR_UNUSED_3_3_Msk 0x08UL
  #define ZB_MAC_ISR_MATCH_Pos 4
  #define ZB_MAC_ISR_MATCH_Msk 0x0f0UL
  #define ZB_MAC_ISR_TIMEOUT_Pos 8
  #define ZB_MAC_ISR_TIMEOUT_Msk 0x0300UL
  #define ZB_MAC_ISR_RXNOMATCH_Pos 10
  #define ZB_MAC_ISR_RXNOMATCH_Msk 0x0400UL
  #define ZB_MAC_ISR_RXMATCH_Pos 11
  #define ZB_MAC_ISR_RXMATCH_Msk 0x0800UL
  #define ZB_MAC_SCTL_SCTL_Pos 0
  #define ZB_MAC_SCTL_SCTL_Msk 0x07fUL
  #define ZB_MAC_PHYTXTUNETIME_PHYTXTUNETIME_Pos 0
  #define ZB_MAC_PHYTXTUNETIME_PHYTXTUNETIME_Msk 0x0ffUL
  #define ZB_MAC_PHYTXTUNETIME_PHYRX2TXTIME_Pos 8
  #define ZB_MAC_PHYTXTUNETIME_PHYRX2TXTIME_Msk 0x0ff00UL
  #define ZB_MAC_PHYRXTUNETIME_PHYRXTUNETIME_Pos 0
  #define ZB_MAC_PHYRXTUNETIME_PHYRXTUNETIME_Msk 0x0ffUL
  #define ZB_MAC_TURNAROUNDTIME_TURNAROUNDTIME_Pos 0
  #define ZB_MAC_TURNAROUNDTIME_TURNAROUNDTIME_Msk 0x0ffUL
  #define ZB_MAC_BACKOFFTIME_BACKOFFTIME_Pos 0
  #define ZB_MAC_BACKOFFTIME_BACKOFFTIME_Msk 0x0ffUL
  #define ZB_MAC_PRBS_SEED_PRBS_SEED_Pos 0
  #define ZB_MAC_PRBS_SEED_PRBS_SEED_Msk 0x07fffUL
  #define ZB_MAC_SM_STATE_SM_STATE_Pos 0
  #define ZB_MAC_SM_STATE_SM_STATE_Msk 0x07fUL
  #define ZB_MAC_LIFSTURNAROUNDTIME_LIFSTURNAROUNDTIME_Pos 0
  #define ZB_MAC_LIFSTURNAROUNDTIME_LIFSTURNAROUNDTIME_Msk 0x0ffUL
  #define ZB_MAC_HDR_CTRL_HDR_CTRL_Pos 0
  #define ZB_MAC_HDR_CTRL_HDR_CTRL_Msk 0x03UL
  #define ZB_MAC_MISR_MISR_Pos 0
  #define ZB_MAC_MISR_MISR_Msk 0x03ffUL
  #define ZB_MAC_IDENTIFIER_IDENTIFIER_Pos 0
  #define ZB_MAC_IDENTIFIER_IDENTIFIER_Msk 0x0ffffffffUL
  #define ZB_MAC_TXCTL_SCH_Pos 0
  #define ZB_MAC_TXCTL_SCH_Msk 0x01UL
  #define ZB_MAC_TXCTL_SS_Pos 1
  #define ZB_MAC_TXCTL_SS_Msk 0x02UL
  #define ZB_MAC_TXCTL_SLOTACK_Pos 2
  #define ZB_MAC_TXCTL_SLOTACK_Msk 0x04UL
  #define ZB_MAC_TXCTL_AA_Pos 3
  #define ZB_MAC_TXCTL_AA_Msk 0x08UL
  #define ZB_MAC_TXCTL_MODE_Pos 4
  #define ZB_MAC_TXCTL_MODE_Msk 0x030UL
  #define ZB_MAC_TXSTAT_TXSTAT_Pos 0
  #define ZB_MAC_TXSTAT_TXSTAT_Msk 0x01fUL
  #define ZB_MAC_TXSTAT_TXTO_Pos 5
  #define ZB_MAC_TXSTAT_TXTO_Msk 0x020UL
  #define ZB_MAC_TXSTAT_TXPCTO_Pos 6
  #define ZB_MAC_TXSTAT_TXPCTO_Msk 0x040UL
  #define ZB_MAC_TXMBEBT_MINBE_Pos 0
  #define ZB_MAC_TXMBEBT_MINBE_Msk 0x0fUL
  #define ZB_MAC_TXMBEBT_MAXBO_Pos 4
  #define ZB_MAC_TXMBEBT_MAXBO_Msk 0x070UL
  #define ZB_MAC_TXMBEBT_BLE_Pos 7
  #define ZB_MAC_TXMBEBT_BLE_Msk 0x080UL
  #define ZB_MAC_TXMBEBT_MAXBE_Pos 8
  #define ZB_MAC_TXMBEBT_MAXBE_Msk 0x0f00UL
  #define ZB_MAC_TXMBEBT_FRCDLY_Pos 12
  #define ZB_MAC_TXMBEBT_FRCDLY_Msk 0x01000UL
  #define ZB_MAC_TXMBEBT_HIDE_Pos 13
  #define ZB_MAC_TXMBEBT_HIDE_Msk 0x01e000UL
  #define ZB_MAC_TXCSMAC_TXCSMAC_Pos 0
  #define ZB_MAC_TXCSMAC_TXCSMAC_Msk 0x03ffffUL
  #define ZB_MAC_TXTRIES_TXTRIES_Pos 0
  #define ZB_MAC_TXTRIES_TXTRIES_Msk 0x0fUL
  #define ZB_MAC_TXPEND_TXPEND_Pos 0
  #define ZB_MAC_TXPEND_TXPEND_Msk 0x01UL
  #define ZB_MAC_TXASC_TXASC_Pos 0
  #define ZB_MAC_TXASC_TXASC_Msk 0x01UL
  #define ZB_MAC_TXBUFAD_TXBUFAD_Pos 0
  #define ZB_MAC_TXBUFAD_TXBUFAD_Msk 0x0ffffffffUL
  #define ZB_MAC_TXTO_TX_TO_LEN_Pos 0
  #define ZB_MAC_TXTO_TX_TO_LEN_Msk 0x07fUL
  #define ZB_MAC_TXTO_TX_TO_ENA_Pos 7
  #define ZB_MAC_TXTO_TX_TO_ENA_Msk 0x080UL
  #define ZB_MAC_TXTO_TX_TOPC_ENA_Pos 8
  #define ZB_MAC_TXTO_TX_TOPC_ENA_Msk 0x0100UL
  #define ZB_MAC_RXCTL_RXCTL_Pos 0
  #define ZB_MAC_RXCTL_RXCTL_Msk 0x01fUL
  #define ZB_MAC_RXSTAT_FCS_ERROR_Pos 0
  #define ZB_MAC_RXSTAT_FCS_ERROR_Msk 0x01UL
  #define ZB_MAC_RXSTAT_ABORT_Pos 1
  #define ZB_MAC_RXSTAT_ABORT_Msk 0x02UL
  #define ZB_MAC_RXSTAT_UNUSED_2_2_Pos 2
  #define ZB_MAC_RXSTAT_UNUSED_2_2_Msk 0x04UL
  #define ZB_MAC_RXSTAT_UNUSED_3_3_Pos 3
  #define ZB_MAC_RXSTAT_UNUSED_3_3_Msk 0x08UL
  #define ZB_MAC_RXSTAT_MOD_IN_PACKET_Pos 4
  #define ZB_MAC_RXSTAT_MOD_IN_PACKET_Msk 0x010UL
  #define ZB_MAC_RXSTAT_MALFORMED_Pos 5
  #define ZB_MAC_RXSTAT_MALFORMED_Msk 0x020UL
  #define ZB_MAC_RXMPID_RXMPID_Pos 0
  #define ZB_MAC_RXMPID_RXMPID_Msk 0x03ffffUL
  #define ZB_MAC_RXMSAD_RXMSAD_Pos 0
  #define ZB_MAC_RXMSAD_RXMSAD_Msk 0x0ffffUL
  #define ZB_MAC_RXMEADL_RXMEADL_Pos 0
  #define ZB_MAC_RXMEADL_RXMEADL_Msk 0x0ffffffffUL
  #define ZB_MAC_RXMEADH_RXMEADH_Pos 0
  #define ZB_MAC_RXMEADH_RXMEADH_Msk 0x0ffffffffUL
  #define ZB_MAC_RXBUFAD_RXBUFAD_Pos 0
  #define ZB_MAC_RXBUFAD_RXBUFAD_Msk 0x0ffffffffUL
  #define ZB_MAC_RXPROM_RXPROM_Pos 0
  #define ZB_MAC_RXPROM_RXPROM_Msk 0x03UL
  #define ZB_MAC_RXMPID_SEC_RXMPID_SEC_Pos 0
  #define ZB_MAC_RXMPID_SEC_RXMPID_SEC_Msk 0x07ffffUL
  #define ZB_MAC_RXMSAD_SEC_RXMSAD_SEC_Pos 0
  #define ZB_MAC_RXMSAD_SEC_RXMSAD_SEC_Msk 0x0ffffUL
  #define ZB_MAC_RXMEADL_SEC_RXMEADL_SEC_Pos 0
  #define ZB_MAC_RXMEADL_SEC_RXMEADL_SEC_Msk 0x0ffffffffUL
  #define ZB_MAC_RXMEADH_SEC_RXMEADH_SEC_Pos 0
  #define ZB_MAC_RXMEADH_SEC_RXMEADH_SEC_Msk 0x0ffffffffUL
  #define ZB_MAC_DMA_ADDR_DMA_ADDR_Pos 0
  #define ZB_MAC_DMA_ADDR_DMA_ADDR_Msk 0x0ffffffffUL
  #define ZB_MAC_DMA_RD_DATA_DMA_RD_DATA_Pos 0
  #define ZB_MAC_DMA_RD_DATA_DMA_RD_DATA_Msk 0x0ffffffffUL
  #define ZB_MAC_DMA_WR_DATA_DMA_WR_DATA_Pos 0
  #define ZB_MAC_DMA_WR_DATA_DMA_WR_DATA_Msk 0x0ffffffffUL
  #define ZB_MAC_DMA_ADDR_OFFSET_DMA_ADDR_OFFSET_Pos 17
  #define ZB_MAC_DMA_ADDR_OFFSET_DMA_ADDR_OFFSET_Msk 0x0fffe0000UL
  #define ZB_MAC_DMA_FROM_ADC_ACTIVATE_Pos 0
  #define ZB_MAC_DMA_FROM_ADC_ACTIVATE_Msk 0x01UL
  #define ZB_MAC_DMA_FROM_ADC_MODE_Pos 1
  #define ZB_MAC_DMA_FROM_ADC_MODE_Msk 0x01eUL
  #define ZB_MAC_DMA_FROM_ADC_SAFE_Pos 5
  #define ZB_MAC_DMA_FROM_ADC_SAFE_Msk 0x020UL
  #define ZB_MAC_PHY_MCTRL_BBC_RMI_REQ_Pos 0
  #define ZB_MAC_PHY_MCTRL_BBC_RMI_REQ_Msk 0x01UL
  #define ZB_MAC_PHY_MSTATUS_BBC_RMI_ACK_Pos 0
  #define ZB_MAC_PHY_MSTATUS_BBC_RMI_ACK_Msk 0x01UL
  #define ZB_MAC_POWERDOWN_SOFT_RESET_Pos 0
  #define ZB_MAC_POWERDOWN_SOFT_RESET_Msk 0x01UL
  #define ZB_MAC_POWERDOWN_FORCE_SOFTRESET_Pos 1
  #define ZB_MAC_POWERDOWN_FORCE_SOFTRESET_Msk 0x02UL
  #define ZB_MAC_POWERDOWN_ENABLE_FLAGS_WITH_CORE_BUS_Pos 2
  #define ZB_MAC_POWERDOWN_ENABLE_FLAGS_WITH_CORE_BUS_Msk 0x04UL
  #define ZB_MAC_POWERDOWN_POWERDOWN_Pos 31
  #define ZB_MAC_POWERDOWN_POWERDOWN_Msk 0x080000000UL
  #define ZB_MAC_MODULEID_APERTURE_Pos 0
  #define ZB_MAC_MODULEID_APERTURE_Msk 0x0ffUL
  #define ZB_MAC_MODULEID_MINOR_REVISION_Pos 8
  #define ZB_MAC_MODULEID_MINOR_REVISION_Msk 0x0f00UL
  #define ZB_MAC_MODULEID_MAJOR_REVISION_Pos 12
  #define ZB_MAC_MODULEID_MAJOR_REVISION_Msk 0x0f000UL
  #define ZB_MAC_MODULEID_IDENTIFIER_Pos 16
  #define ZB_MAC_MODULEID_IDENTIFIER_Msk 0x0ffff0000UL
#endif

#endif // __ZB_MAC_IF_H__
