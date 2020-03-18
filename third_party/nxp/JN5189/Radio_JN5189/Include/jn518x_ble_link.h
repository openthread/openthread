/*
* Copyright (c) 2014 - 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2019 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef __BLE_LINK_IF_H__
#define __BLE_LINK_IF_H__

  typedef  struct {
        /* Address 0x000 */
        union {
           __IO uint32_t RW_BLE_RWBLECNTL;

        struct {
           __IO uint32_t  rw_ble_rwblecntl : 32;
          } RW_BLE_RWBLECNTL_b;
        };
        /* Address 0x004 */
        union {
           __IO uint32_t RW_BLE_VERSION;

        struct {
           __IO uint32_t  rw_ble_version : 32;
          } RW_BLE_VERSION_b;
        };
        /* Address 0x008 */
        union {
           __IO uint32_t RW_BLE_RWBLECONF;

        struct {
           __IO uint32_t  rw_ble_rwbleconf : 32;
          } RW_BLE_RWBLECONF_b;
        };
        /* Address 0x00C */
        union {
           __IO uint32_t RW_BLE_INTCNTL;

        struct {
           __IO uint32_t  rw_ble_intcntl : 32;
          } RW_BLE_INTCNTL_b;
        };
        /* Address 0x010 */
        union {
           __IO uint32_t RW_BLE_INTSTAT;

        struct {
           __IO uint32_t  rw_ble_intstat : 32;
          } RW_BLE_INTSTAT_b;
        };
        /* Address 0x014 */
        union {
           __IO uint32_t RW_BLE_INTRAWSTAT;

        struct {
           __IO uint32_t  rw_ble_intrawstat : 32;
          } RW_BLE_INTRAWSTAT_b;
        };
        /* Address 0x018 */
        union {
           __IO uint32_t RW_BLE_INTACK;

        struct {
           __IO uint32_t  rw_ble_intack : 32;
          } RW_BLE_INTACK_b;
        };
        /* Address 0x01C */
        union {
           __IO uint32_t RW_BLE_BASETIMECNT;

        struct {
           __IO uint32_t  rw_ble_basetimecnt : 32;
          } RW_BLE_BASETIMECNT_b;
        };
        /* Address 0x020 */
        union {
           __IO uint32_t RW_BLE_FINETIMECNT;

        struct {
           __IO uint32_t  rw_ble_finetimecnt : 32;
          } RW_BLE_FINETIMECNT_b;
        };
        /* Address 0x024 */
        union {
           __IO uint32_t RW_BLE_BDADDRL;

        struct {
           __IO uint32_t  rw_ble_bdaddrl : 32;
          } RW_BLE_BDADDRL_b;
        };
        /* Address 0x028 */
        union {
           __IO uint32_t RW_BLE_BDADDRU;

        struct {
           __IO uint32_t  rw_ble_bdaddru : 32;
          } RW_BLE_BDADDRU_b;
        };
        /* Address 0x02C */
        union {
           __IO uint32_t RW_BLE_ET_CURRENTRXDESCPTR;

        struct {
           __IO uint32_t  rw_ble_et_currentrxdescptr : 32;
          } RW_BLE_ET_CURRENTRXDESCPTR_b;
        };
        /* Address 0x030 */
        union {
           __IO uint32_t RW_BLE_DEEPSLCNTL;

        struct {
           __IO uint32_t  rw_ble_deepslcntl : 32;
          } RW_BLE_DEEPSLCNTL_b;
        };
        /* Address 0x034 */
        union {
           __IO uint32_t RW_BLE_DEEPSLWKUP;

        struct {
           __IO uint32_t  rw_ble_deepslwkup : 32;
          } RW_BLE_DEEPSLWKUP_b;
        };
        /* Address 0x038 */
        union {
           __IO uint32_t RW_BLE_DEEPSLSTAT;

        struct {
           __IO uint32_t  rw_ble_deepslstat : 32;
          } RW_BLE_DEEPSLSTAT_b;
        };
        /* Address 0x03C */
        union {
           __IO uint32_t RW_BLE_ENBPRESET;

        struct {
           __IO uint32_t  rw_ble_enbpreset : 32;
          } RW_BLE_ENBPRESET_b;
        };
        /* Address 0x040 */
        union {
           __IO uint32_t RW_BLE_FINECNTCORR;

        struct {
           __IO uint32_t  rw_ble_finecntcorr : 32;
          } RW_BLE_FINECNTCORR_b;
        };
        /* Address 0x044 */
        union {
           __IO uint32_t RW_BLE_BASETIMECNTCORR;

        struct {
           __IO uint32_t  rw_ble_basetimecntcorr : 32;
          } RW_BLE_BASETIMECNTCORR_b;
        };
        /* Addresses 0x048 - 0x04C */
        __O  uint32_t  RESERVED048[2];
        /* Address 0x050 */
        union {
           __IO uint32_t RW_BLE_DIAGCNTL;

        struct {
           __IO uint32_t  rw_ble_diagcntl : 32;
          } RW_BLE_DIAGCNTL_b;
        };
        /* Address 0x054 */
        union {
           __IO uint32_t RW_BLE_DIAGSTAT;

        struct {
           __IO uint32_t  rw_ble_diagstat : 32;
          } RW_BLE_DIAGSTAT_b;
        };
        /* Address 0x058 */
        union {
           __IO uint32_t RW_BLE_DEBUGADDMAX;

        struct {
           __IO uint32_t  rw_ble_debugaddmax : 32;
          } RW_BLE_DEBUGADDMAX_b;
        };
        /* Address 0x05C */
        union {
           __IO uint32_t RW_BLE_DEBUGADDMIN;

        struct {
           __IO uint32_t  rw_ble_debugaddmin : 32;
          } RW_BLE_DEBUGADDMIN_b;
        };
        /* Address 0x060 */
        union {
           __IO uint32_t RW_BLE_ERRORTYPESTAT;

        struct {
           __IO uint32_t  rw_ble_errortypestat : 32;
          } RW_BLE_ERRORTYPESTAT_b;
        };
        /* Address 0x064 */
        union {
           __IO uint32_t RW_BLE_SWPROFILING;

        struct {
           __IO uint32_t  rw_ble_swprofiling : 32;
          } RW_BLE_SWPROFILING_b;
        };
        /* Addresses 0x068 - 0x06C */
        __O  uint32_t  RESERVED068[2];
        /* Address 0x070 */
        union {
           __IO uint32_t RW_BLE_RADIOCNTL0;

        struct {
           __IO uint32_t  rw_ble_radiocntl0 : 32;
          } RW_BLE_RADIOCNTL0_b;
        };
        /* Address 0x074 */
        union {
           __IO uint32_t RW_BLE_RADIOCNTL1;

        struct {
           __IO uint32_t  rw_ble_radiocntl1 : 32;
          } RW_BLE_RADIOCNTL1_b;
        };
        /* Address 0x078 */
        union {
           __IO uint32_t RW_BLE_RADIOCNTL2;

        struct {
           __IO uint32_t  rw_ble_radiocntl2 : 32;
          } RW_BLE_RADIOCNTL2_b;
        };
        /* Address 0x07C */
        union {
           __IO uint32_t RW_BLE_RADIOCNTL3;

        struct {
           __IO uint32_t  rw_ble_radiocntl3 : 32;
          } RW_BLE_RADIOCNTL3_b;
        };
        /* Address 0x080 */
        union {
           __IO uint32_t RW_BLE_RADIOPWRUPDN;

        struct {
           __IO uint32_t  rw_ble_radiopwrupdn : 32;
          } RW_BLE_RADIOPWRUPDN_b;
        };
        /* Address 0x084 */
        union {
           __IO uint32_t RW_BLE_SPIPTRCNTL0;

        struct {
           __IO uint32_t  rw_ble_spiptrcntl0 : 32;
          } RW_BLE_SPIPTRCNTL0_b;
        };
        /* Address 0x088 */
        union {
           __IO uint32_t RW_BLE_SPIPTRCNTL1;

        struct {
           __IO uint32_t  rw_ble_spiptrcntl1 : 32;
          } RW_BLE_SPIPTRCNTL1_b;
        };
        /* Address 0x08C */
        union {
           __IO uint32_t RW_BLE_SPIPTRCNTL2;

        struct {
           __IO uint32_t  rw_ble_spiptrcntl2 : 32;
          } RW_BLE_SPIPTRCNTL2_b;
        };
        /* Address 0x090 */
        union {
           __IO uint32_t RW_BLE_ADVCHMAP;

        struct {
           __IO uint32_t  rw_ble_advchmap : 32;
          } RW_BLE_ADVCHMAP_b;
        };
        /* Addresses 0x094 - 0x09C */
        __O  uint32_t  RESERVED094[3];
        /* Address 0x0A0 */
        union {
           __IO uint32_t RW_BLE_ADVTIM;

        struct {
           __IO uint32_t  rw_ble_advtim : 32;
          } RW_BLE_ADVTIM_b;
        };
        /* Address 0x0A4 */
        union {
           __IO uint32_t RW_BLE_ACTSCANSTAT;

        struct {
           __IO uint32_t  rw_ble_actscanstat : 32;
          } RW_BLE_ACTSCANSTAT_b;
        };
        /* Addresses 0x0A8 - 0x0AC */
        __O  uint32_t  RESERVED0A8[2];
        /* Address 0x0B0 */
        union {
           __IO uint32_t RW_BLE_WLPUBADDPTR;

        struct {
           __IO uint32_t  rw_ble_wlpubaddptr : 32;
          } RW_BLE_WLPUBADDPTR_b;
        };
        /* Address 0x0B4 */
        union {
           __IO uint32_t RW_BLE_WLPRIVADDPTR;

        struct {
           __IO uint32_t  rw_ble_wlprivaddptr : 32;
          } RW_BLE_WLPRIVADDPTR_b;
        };
        /* Address 0x0B8 */
        union {
           __IO uint32_t RW_BLE_WLNBDEV;

        struct {
           __IO uint32_t  rw_ble_wlnbdev : 32;
          } RW_BLE_WLNBDEV_b;
        };
        /* Address 0x0BC */
        __O  uint32_t  RESERVED0BC[1];
        /* Address 0x0C0 */
        union {
           __IO uint32_t RW_BLE_AESCNTL;

        struct {
           __IO uint32_t  rw_ble_aescntl : 32;
          } RW_BLE_AESCNTL_b;
        };
        /* Address 0x0C4 */
        union {
           __IO uint32_t RW_BLE_AESKEY31_0;

        struct {
           __IO uint32_t  rw_ble_aeskey31_0 : 32;
          } RW_BLE_AESKEY31_0_b;
        };
        /* Address 0x0C8 */
        union {
           __IO uint32_t RW_BLE_AESKEY63_32;

        struct {
           __IO uint32_t  rw_ble_aeskey63_32 : 32;
          } RW_BLE_AESKEY63_32_b;
        };
        /* Address 0x0CC */
        union {
           __IO uint32_t RW_BLE_AESKEY95_64;

        struct {
           __IO uint32_t  rw_ble_aeskey95_64 : 32;
          } RW_BLE_AESKEY95_64_b;
        };
        /* Address 0x0D0 */
        union {
           __IO uint32_t RW_BLE_AESKEY127_96;

        struct {
           __IO uint32_t  rw_ble_aeskey127_96 : 32;
          } RW_BLE_AESKEY127_96_b;
        };
        /* Address 0x0D4 */
        union {
           __IO uint32_t RW_BLE_AESPTR;

        struct {
           __IO uint32_t  rw_ble_aesptr : 32;
          } RW_BLE_AESPTR_b;
        };
        /* Address 0x0D8 */
        union {
           __IO uint32_t RW_BLE_TXMICVAL;

        struct {
           __IO uint32_t  rw_ble_txmicval : 32;
          } RW_BLE_TXMICVAL_b;
        };
        /* Address 0x0DC */
        union {
           __IO uint32_t RW_BLE_RXMICVAL;

        struct {
           __IO uint32_t  rw_ble_rxmicval : 32;
          } RW_BLE_RXMICVAL_b;
        };
        /* Address 0x0E0 */
        union {
           __IO uint32_t RW_BLE_RFTESTCNTL;

        struct {
           __IO uint32_t  rw_ble_rftestcntl : 32;
          } RW_BLE_RFTESTCNTL_b;
        };
        /* Address 0x0E4 */
        union {
           __IO uint32_t RW_BLE_RFTESTTXSTAT;

        struct {
           __IO uint32_t  rw_ble_rftesttxstat : 32;
          } RW_BLE_RFTESTTXSTAT_b;
        };
        /* Address 0x0E8 */
        union {
           __IO uint32_t RW_BLE_RFTESTRXSTAT;

        struct {
           __IO uint32_t  rw_ble_rftestrxstat : 32;
          } RW_BLE_RFTESTRXSTAT_b;
        };
        /* Address 0x0EC */
        __O  uint32_t  RESERVED0EC[1];
        /* Address 0x0F0 */
        union {
           __IO uint32_t RW_BLE_TIMGENCNTL;

        struct {
           __IO uint32_t  rw_ble_timgencntl : 32;
          } RW_BLE_TIMGENCNTL_b;
        };
        /* Address 0x0F4 */
        union {
           __IO uint32_t RW_BLE_GROSSTIMTGT;

        struct {
           __IO uint32_t  rw_ble_grosstimtgt : 32;
          } RW_BLE_GROSSTIMTGT_b;
        };
        /* Address 0x0F8 */
        union {
           __IO uint32_t RW_BLE_FINETIMTGT;

        struct {
           __IO uint32_t  rw_ble_finetimtgt : 32;
          } RW_BLE_FINETIMTGT_b;
        };
        /* Address 0x0FC */
        __O  uint32_t  RESERVED0FC[1];
        /* Address 0x100 */
        union {
           __IO uint32_t RW_BLE_COEXIFCNTL0;

        struct {
           __IO uint32_t  rw_ble_coexifcntl0 : 32;
          } RW_BLE_COEXIFCNTL0_b;
        };
        /* Address 0x104 */
        union {
           __IO uint32_t RW_BLE_COEXIFCNTL1;

        struct {
           __IO uint32_t  rw_ble_coexifcntl1 : 32;
          } RW_BLE_COEXIFCNTL1_b;
        };
        /* Address 0x108 */
        union {
           __IO uint32_t RW_BLE_COEXIFCNTL2;

        struct {
           __IO uint32_t  rw_ble_coexifcntl2 : 32;
          } RW_BLE_COEXIFCNTL2_b;
        };
        /* Address 0x10C */
        union {
           __IO uint32_t RW_BLE_BLEMPRIO0;

        struct {
           __IO uint32_t  rw_ble_blemprio0 : 32;
          } RW_BLE_BLEMPRIO0_b;
        };
        /* Address 0x110 */
        union {
           __IO uint32_t RW_BLE_BLEMPRIO1;

        struct {
           __IO uint32_t  rw_ble_blemprio1 : 32;
          } RW_BLE_BLEMPRIO1_b;
        };
        /* Addresses 0x114 - 0x11C */
        __O  uint32_t  RESERVED114[3];
        /* Address 0x120 */
        union {
           __IO uint32_t RW_BLE_RALPTR;

        struct {
           __IO uint32_t  rw_ble_ralptr : 32;
          } RW_BLE_RALPTR_b;
        };
        /* Address 0x124 */
        union {
           __IO uint32_t RW_BLE_RALNBDEV;

        struct {
           __IO uint32_t  rw_ble_ralnbdev : 32;
          } RW_BLE_RALNBDEV_b;
        };
        /* Address 0x128 */
        union {
           __IO uint32_t RW_BLE_RAL_LOCAL_RND;

        struct {
           __IO uint32_t  rw_ble_ral_local_rnd : 32;
          } RW_BLE_RAL_LOCAL_RND_b;
        };
        /* Address 0x12C */
        union {
           __IO uint32_t RW_BLE_RAL_PEER_RND;

        struct {
           __IO uint32_t  rw_ble_ral_peer_rnd : 32;
          } RW_BLE_RAL_PEER_RND_b;
        };
        /* Address 0x130 */
        union {
           __IO uint32_t RW_BLE_ISOCHANCNTL0;

        struct {
           __IO uint32_t  rw_ble_isochancntl0 : 32;
          } RW_BLE_ISOCHANCNTL0_b;
        };
        /* Address 0x134 */
        union {
           __IO uint32_t RW_BLE_ISOMUTECNTL0;

        struct {
           __IO uint32_t  rw_ble_isomutecntl0 : 32;
          } RW_BLE_ISOMUTECNTL0_b;
        };
        /* Address 0x138 */
        union {
           __IO uint32_t RW_BLE_ISOCURRENTTXPTR0;

        struct {
           __IO uint32_t  rw_ble_isocurrenttxptr0 : 32;
          } RW_BLE_ISOCURRENTTXPTR0_b;
        };
        /* Address 0x13C */
        union {
           __IO uint32_t RW_BLE_ISOCURRENTRXPTR0;

        struct {
           __IO uint32_t  rw_ble_isocurrentrxptr0 : 32;
          } RW_BLE_ISOCURRENTRXPTR0_b;
        };
        /* Address 0x140 */
        union {
           __IO uint32_t RW_BLE_ISOTRCNL0;

        struct {
           __IO uint32_t  rw_ble_isotrcnl0 : 32;
          } RW_BLE_ISOTRCNL0_b;
        };
        /* Address 0x144 */
        union {
           __IO uint32_t RW_BLE_ISOEVTCNTLOFFSETL0;

        struct {
           __IO uint32_t  rw_ble_isoevtcntloffsetl0 : 32;
          } RW_BLE_ISOEVTCNTLOFFSETL0_b;
        };
        /* Address 0x148 */
        union {
           __IO uint32_t RW_BLE_ISOEVTCNTLOFFSETU0;

        struct {
           __IO uint32_t  rw_ble_isoevtcntloffsetu0 : 32;
          } RW_BLE_ISOEVTCNTLOFFSETU0_b;
        };
        /* Address 0x14C */
        __O  uint32_t  RESERVED14C[1];
        /* Address 0x150 */
        union {
           __IO uint32_t RW_BLE_ISOCHANCNTL1;

        struct {
           __IO uint32_t  rw_ble_isochancntl1 : 32;
          } RW_BLE_ISOCHANCNTL1_b;
        };
        /* Address 0x154 */
        union {
           __IO uint32_t RW_BLE_ISOMUTECNTL1;

        struct {
           __IO uint32_t  rw_ble_isomutecntl1 : 32;
          } RW_BLE_ISOMUTECNTL1_b;
        };
        /* Address 0x158 */
        union {
           __IO uint32_t RW_BLE_ISOCURRENTTXPTR1;

        struct {
           __IO uint32_t  rw_ble_isocurrenttxptr1 : 32;
          } RW_BLE_ISOCURRENTTXPTR1_b;
        };
        /* Address 0x15C */
        union {
           __IO uint32_t RW_BLE_ISOCURRENTRXPTR1;

        struct {
           __IO uint32_t  rw_ble_isocurrentrxptr1 : 32;
          } RW_BLE_ISOCURRENTRXPTR1_b;
        };
        /* Address 0x160 */
        union {
           __IO uint32_t RW_BLE_ISOTRCNL1;

        struct {
           __IO uint32_t  rw_ble_isotrcnl1 : 32;
          } RW_BLE_ISOTRCNL1_b;
        };
        /* Address 0x164 */
        union {
           __IO uint32_t RW_BLE_ISOEVTCNTLOFFSETL1;

        struct {
           __IO uint32_t  rw_ble_isoevtcntloffsetl1 : 32;
          } RW_BLE_ISOEVTCNTLOFFSETL1_b;
        };
        /* Address 0x168 */
        union {
           __IO uint32_t RW_BLE_ISOEVTCNTLOFFSETU1;

        struct {
           __IO uint32_t  rw_ble_isoevtcntloffsetu1 : 32;
          } RW_BLE_ISOEVTCNTLOFFSETU1_b;
        };
        /* Address 0x16C */
        __O  uint32_t  RESERVED16C[1];
        /* Address 0x170 */
        union {
           __IO uint32_t RW_BLE_ISOCHANCNTL2;

        struct {
           __IO uint32_t  rw_ble_isochancntl2 : 32;
          } RW_BLE_ISOCHANCNTL2_b;
        };
        /* Address 0x174 */
        union {
           __IO uint32_t RW_BLE_ISOMUTECNTL2;

        struct {
           __IO uint32_t  rw_ble_isomutecntl2 : 32;
          } RW_BLE_ISOMUTECNTL2_b;
        };
        /* Address 0x178 */
        union {
           __IO uint32_t RW_BLE_ISOCURRENTTXPTR2;

        struct {
           __IO uint32_t  rw_ble_isocurrenttxptr2 : 32;
          } RW_BLE_ISOCURRENTTXPTR2_b;
        };
        /* Address 0x17C */
        union {
           __IO uint32_t RW_BLE_ISOCURRENTRXPTR2;

        struct {
           __IO uint32_t  rw_ble_isocurrentrxptr2 : 32;
          } RW_BLE_ISOCURRENTRXPTR2_b;
        };
        /* Address 0x180 */
        union {
           __IO uint32_t RW_BLE_ISOTRCNL2;

        struct {
           __IO uint32_t  rw_ble_isotrcnl2 : 32;
          } RW_BLE_ISOTRCNL2_b;
        };
        /* Address 0x184 */
        union {
           __IO uint32_t RW_BLE_ISOEVTCNTLOFFSETL2;

        struct {
           __IO uint32_t  rw_ble_isoevtcntloffsetl2 : 32;
          } RW_BLE_ISOEVTCNTLOFFSETL2_b;
        };
        /* Address 0x188 */
        union {
           __IO uint32_t RW_BLE_ISOEVTCNTLOFFSETU2;

        struct {
           __IO uint32_t  rw_ble_isoevtcntloffsetu2 : 32;
          } RW_BLE_ISOEVTCNTLOFFSETU2_b;
        };
        /* Address 0x18C */
        __O  uint32_t  RESERVED18C[1];
        /* Address 0x190 */
        union {
           __IO uint32_t RW_BLE_BLEPRIOSCHARB;

        struct {
           __IO uint32_t  rw_ble_bleprioscharb : 32;
          } RW_BLE_BLEPRIOSCHARB_b;
        };
        /* Addresses 0x194 - 0xFF0 */
        __O  uint32_t  RESERVED194[920];
    } BLE_LINK_IF_Type;

  #define BLELNK_RW_BLE_RWBLECNTL_RW_BLE_RWBLECNTL_Pos 0
  #define BLELNK_RW_BLE_RWBLECNTL_RW_BLE_RWBLECNTL_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_VERSION_RW_BLE_VERSION_Pos 0
  #define BLELNK_RW_BLE_VERSION_RW_BLE_VERSION_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RWBLECONF_RW_BLE_RWBLECONF_Pos 0
  #define BLELNK_RW_BLE_RWBLECONF_RW_BLE_RWBLECONF_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_INTCNTL_RW_BLE_INTCNTL_Pos 0
  #define BLELNK_RW_BLE_INTCNTL_RW_BLE_INTCNTL_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_INTSTAT_RW_BLE_INTSTAT_Pos 0
  #define BLELNK_RW_BLE_INTSTAT_RW_BLE_INTSTAT_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_INTRAWSTAT_RW_BLE_INTRAWSTAT_Pos 0
  #define BLELNK_RW_BLE_INTRAWSTAT_RW_BLE_INTRAWSTAT_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_INTACK_RW_BLE_INTACK_Pos 0
  #define BLELNK_RW_BLE_INTACK_RW_BLE_INTACK_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_BASETIMECNT_RW_BLE_BASETIMECNT_Pos 0
  #define BLELNK_RW_BLE_BASETIMECNT_RW_BLE_BASETIMECNT_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_FINETIMECNT_RW_BLE_FINETIMECNT_Pos 0
  #define BLELNK_RW_BLE_FINETIMECNT_RW_BLE_FINETIMECNT_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_BDADDRL_RW_BLE_BDADDRL_Pos 0
  #define BLELNK_RW_BLE_BDADDRL_RW_BLE_BDADDRL_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_BDADDRU_RW_BLE_BDADDRU_Pos 0
  #define BLELNK_RW_BLE_BDADDRU_RW_BLE_BDADDRU_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ET_CURRENTRXDESCPTR_RW_BLE_ET_CURRENTRXDESCPTR_Pos 0
  #define BLELNK_RW_BLE_ET_CURRENTRXDESCPTR_RW_BLE_ET_CURRENTRXDESCPTR_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_DEEPSLCNTL_RW_BLE_DEEPSLCNTL_Pos 0
  #define BLELNK_RW_BLE_DEEPSLCNTL_RW_BLE_DEEPSLCNTL_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_DEEPSLWKUP_RW_BLE_DEEPSLWKUP_Pos 0
  #define BLELNK_RW_BLE_DEEPSLWKUP_RW_BLE_DEEPSLWKUP_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_DEEPSLSTAT_RW_BLE_DEEPSLSTAT_Pos 0
  #define BLELNK_RW_BLE_DEEPSLSTAT_RW_BLE_DEEPSLSTAT_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ENBPRESET_RW_BLE_ENBPRESET_Pos 0
  #define BLELNK_RW_BLE_ENBPRESET_RW_BLE_ENBPRESET_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_FINECNTCORR_RW_BLE_FINECNTCORR_Pos 0
  #define BLELNK_RW_BLE_FINECNTCORR_RW_BLE_FINECNTCORR_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_BASETIMECNTCORR_RW_BLE_BASETIMECNTCORR_Pos 0
  #define BLELNK_RW_BLE_BASETIMECNTCORR_RW_BLE_BASETIMECNTCORR_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_DIAGCNTL_RW_BLE_DIAGCNTL_Pos 0
  #define BLELNK_RW_BLE_DIAGCNTL_RW_BLE_DIAGCNTL_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_DIAGSTAT_RW_BLE_DIAGSTAT_Pos 0
  #define BLELNK_RW_BLE_DIAGSTAT_RW_BLE_DIAGSTAT_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_DEBUGADDMAX_RW_BLE_DEBUGADDMAX_Pos 0
  #define BLELNK_RW_BLE_DEBUGADDMAX_RW_BLE_DEBUGADDMAX_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_DEBUGADDMIN_RW_BLE_DEBUGADDMIN_Pos 0
  #define BLELNK_RW_BLE_DEBUGADDMIN_RW_BLE_DEBUGADDMIN_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ERRORTYPESTAT_RW_BLE_ERRORTYPESTAT_Pos 0
  #define BLELNK_RW_BLE_ERRORTYPESTAT_RW_BLE_ERRORTYPESTAT_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_SWPROFILING_RW_BLE_SWPROFILING_Pos 0
  #define BLELNK_RW_BLE_SWPROFILING_RW_BLE_SWPROFILING_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RADIOCNTL0_RW_BLE_RADIOCNTL0_Pos 0
  #define BLELNK_RW_BLE_RADIOCNTL0_RW_BLE_RADIOCNTL0_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RADIOCNTL1_RW_BLE_RADIOCNTL1_Pos 0
  #define BLELNK_RW_BLE_RADIOCNTL1_RW_BLE_RADIOCNTL1_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RADIOCNTL2_RW_BLE_RADIOCNTL2_Pos 0
  #define BLELNK_RW_BLE_RADIOCNTL2_RW_BLE_RADIOCNTL2_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RADIOCNTL3_RW_BLE_RADIOCNTL3_Pos 0
  #define BLELNK_RW_BLE_RADIOCNTL3_RW_BLE_RADIOCNTL3_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RADIOPWRUPDN_RW_BLE_RADIOPWRUPDN_Pos 0
  #define BLELNK_RW_BLE_RADIOPWRUPDN_RW_BLE_RADIOPWRUPDN_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_SPIPTRCNTL0_RW_BLE_SPIPTRCNTL0_Pos 0
  #define BLELNK_RW_BLE_SPIPTRCNTL0_RW_BLE_SPIPTRCNTL0_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_SPIPTRCNTL1_RW_BLE_SPIPTRCNTL1_Pos 0
  #define BLELNK_RW_BLE_SPIPTRCNTL1_RW_BLE_SPIPTRCNTL1_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_SPIPTRCNTL2_RW_BLE_SPIPTRCNTL2_Pos 0
  #define BLELNK_RW_BLE_SPIPTRCNTL2_RW_BLE_SPIPTRCNTL2_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ADVCHMAP_RW_BLE_ADVCHMAP_Pos 0
  #define BLELNK_RW_BLE_ADVCHMAP_RW_BLE_ADVCHMAP_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ADVTIM_RW_BLE_ADVTIM_Pos 0
  #define BLELNK_RW_BLE_ADVTIM_RW_BLE_ADVTIM_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ACTSCANSTAT_RW_BLE_ACTSCANSTAT_Pos 0
  #define BLELNK_RW_BLE_ACTSCANSTAT_RW_BLE_ACTSCANSTAT_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_WLPUBADDPTR_RW_BLE_WLPUBADDPTR_Pos 0
  #define BLELNK_RW_BLE_WLPUBADDPTR_RW_BLE_WLPUBADDPTR_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_WLPRIVADDPTR_RW_BLE_WLPRIVADDPTR_Pos 0
  #define BLELNK_RW_BLE_WLPRIVADDPTR_RW_BLE_WLPRIVADDPTR_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_WLNBDEV_RW_BLE_WLNBDEV_Pos 0
  #define BLELNK_RW_BLE_WLNBDEV_RW_BLE_WLNBDEV_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_AESCNTL_RW_BLE_AESCNTL_Pos 0
  #define BLELNK_RW_BLE_AESCNTL_RW_BLE_AESCNTL_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_AESKEY31_0_RW_BLE_AESKEY31_0_Pos 0
  #define BLELNK_RW_BLE_AESKEY31_0_RW_BLE_AESKEY31_0_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_AESKEY63_32_RW_BLE_AESKEY63_32_Pos 0
  #define BLELNK_RW_BLE_AESKEY63_32_RW_BLE_AESKEY63_32_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_AESKEY95_64_RW_BLE_AESKEY95_64_Pos 0
  #define BLELNK_RW_BLE_AESKEY95_64_RW_BLE_AESKEY95_64_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_AESKEY127_96_RW_BLE_AESKEY127_96_Pos 0
  #define BLELNK_RW_BLE_AESKEY127_96_RW_BLE_AESKEY127_96_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_AESPTR_RW_BLE_AESPTR_Pos 0
  #define BLELNK_RW_BLE_AESPTR_RW_BLE_AESPTR_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_TXMICVAL_RW_BLE_TXMICVAL_Pos 0
  #define BLELNK_RW_BLE_TXMICVAL_RW_BLE_TXMICVAL_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RXMICVAL_RW_BLE_RXMICVAL_Pos 0
  #define BLELNK_RW_BLE_RXMICVAL_RW_BLE_RXMICVAL_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RFTESTCNTL_RW_BLE_RFTESTCNTL_Pos 0
  #define BLELNK_RW_BLE_RFTESTCNTL_RW_BLE_RFTESTCNTL_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RFTESTTXSTAT_RW_BLE_RFTESTTXSTAT_Pos 0
  #define BLELNK_RW_BLE_RFTESTTXSTAT_RW_BLE_RFTESTTXSTAT_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RFTESTRXSTAT_RW_BLE_RFTESTRXSTAT_Pos 0
  #define BLELNK_RW_BLE_RFTESTRXSTAT_RW_BLE_RFTESTRXSTAT_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_TIMGENCNTL_RW_BLE_TIMGENCNTL_Pos 0
  #define BLELNK_RW_BLE_TIMGENCNTL_RW_BLE_TIMGENCNTL_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_GROSSTIMTGT_RW_BLE_GROSSTIMTGT_Pos 0
  #define BLELNK_RW_BLE_GROSSTIMTGT_RW_BLE_GROSSTIMTGT_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_FINETIMTGT_RW_BLE_FINETIMTGT_Pos 0
  #define BLELNK_RW_BLE_FINETIMTGT_RW_BLE_FINETIMTGT_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_COEXIFCNTL0_RW_BLE_COEXIFCNTL0_Pos 0
  #define BLELNK_RW_BLE_COEXIFCNTL0_RW_BLE_COEXIFCNTL0_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_COEXIFCNTL1_RW_BLE_COEXIFCNTL1_Pos 0
  #define BLELNK_RW_BLE_COEXIFCNTL1_RW_BLE_COEXIFCNTL1_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_COEXIFCNTL2_RW_BLE_COEXIFCNTL2_Pos 0
  #define BLELNK_RW_BLE_COEXIFCNTL2_RW_BLE_COEXIFCNTL2_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_BLEMPRIO0_RW_BLE_BLEMPRIO0_Pos 0
  #define BLELNK_RW_BLE_BLEMPRIO0_RW_BLE_BLEMPRIO0_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_BLEMPRIO1_RW_BLE_BLEMPRIO1_Pos 0
  #define BLELNK_RW_BLE_BLEMPRIO1_RW_BLE_BLEMPRIO1_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RALPTR_RW_BLE_RALPTR_Pos 0
  #define BLELNK_RW_BLE_RALPTR_RW_BLE_RALPTR_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RALNBDEV_RW_BLE_RALNBDEV_Pos 0
  #define BLELNK_RW_BLE_RALNBDEV_RW_BLE_RALNBDEV_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RAL_LOCAL_RND_RW_BLE_RAL_LOCAL_RND_Pos 0
  #define BLELNK_RW_BLE_RAL_LOCAL_RND_RW_BLE_RAL_LOCAL_RND_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_RAL_PEER_RND_RW_BLE_RAL_PEER_RND_Pos 0
  #define BLELNK_RW_BLE_RAL_PEER_RND_RW_BLE_RAL_PEER_RND_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOCHANCNTL0_RW_BLE_ISOCHANCNTL0_Pos 0
  #define BLELNK_RW_BLE_ISOCHANCNTL0_RW_BLE_ISOCHANCNTL0_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOMUTECNTL0_RW_BLE_ISOMUTECNTL0_Pos 0
  #define BLELNK_RW_BLE_ISOMUTECNTL0_RW_BLE_ISOMUTECNTL0_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOCURRENTTXPTR0_RW_BLE_ISOCURRENTTXPTR0_Pos 0
  #define BLELNK_RW_BLE_ISOCURRENTTXPTR0_RW_BLE_ISOCURRENTTXPTR0_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOCURRENTRXPTR0_RW_BLE_ISOCURRENTRXPTR0_Pos 0
  #define BLELNK_RW_BLE_ISOCURRENTRXPTR0_RW_BLE_ISOCURRENTRXPTR0_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOTRCNL0_RW_BLE_ISOTRCNL0_Pos 0
  #define BLELNK_RW_BLE_ISOTRCNL0_RW_BLE_ISOTRCNL0_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOEVTCNTLOFFSETL0_RW_BLE_ISOEVTCNTLOFFSETL0_Pos 0
  #define BLELNK_RW_BLE_ISOEVTCNTLOFFSETL0_RW_BLE_ISOEVTCNTLOFFSETL0_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOEVTCNTLOFFSETU0_RW_BLE_ISOEVTCNTLOFFSETU0_Pos 0
  #define BLELNK_RW_BLE_ISOEVTCNTLOFFSETU0_RW_BLE_ISOEVTCNTLOFFSETU0_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOCHANCNTL1_RW_BLE_ISOCHANCNTL1_Pos 0
  #define BLELNK_RW_BLE_ISOCHANCNTL1_RW_BLE_ISOCHANCNTL1_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOMUTECNTL1_RW_BLE_ISOMUTECNTL1_Pos 0
  #define BLELNK_RW_BLE_ISOMUTECNTL1_RW_BLE_ISOMUTECNTL1_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOCURRENTTXPTR1_RW_BLE_ISOCURRENTTXPTR1_Pos 0
  #define BLELNK_RW_BLE_ISOCURRENTTXPTR1_RW_BLE_ISOCURRENTTXPTR1_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOCURRENTRXPTR1_RW_BLE_ISOCURRENTRXPTR1_Pos 0
  #define BLELNK_RW_BLE_ISOCURRENTRXPTR1_RW_BLE_ISOCURRENTRXPTR1_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOTRCNL1_RW_BLE_ISOTRCNL1_Pos 0
  #define BLELNK_RW_BLE_ISOTRCNL1_RW_BLE_ISOTRCNL1_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOEVTCNTLOFFSETL1_RW_BLE_ISOEVTCNTLOFFSETL1_Pos 0
  #define BLELNK_RW_BLE_ISOEVTCNTLOFFSETL1_RW_BLE_ISOEVTCNTLOFFSETL1_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOEVTCNTLOFFSETU1_RW_BLE_ISOEVTCNTLOFFSETU1_Pos 0
  #define BLELNK_RW_BLE_ISOEVTCNTLOFFSETU1_RW_BLE_ISOEVTCNTLOFFSETU1_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOCHANCNTL2_RW_BLE_ISOCHANCNTL2_Pos 0
  #define BLELNK_RW_BLE_ISOCHANCNTL2_RW_BLE_ISOCHANCNTL2_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOMUTECNTL2_RW_BLE_ISOMUTECNTL2_Pos 0
  #define BLELNK_RW_BLE_ISOMUTECNTL2_RW_BLE_ISOMUTECNTL2_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOCURRENTTXPTR2_RW_BLE_ISOCURRENTTXPTR2_Pos 0
  #define BLELNK_RW_BLE_ISOCURRENTTXPTR2_RW_BLE_ISOCURRENTTXPTR2_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOCURRENTRXPTR2_RW_BLE_ISOCURRENTRXPTR2_Pos 0
  #define BLELNK_RW_BLE_ISOCURRENTRXPTR2_RW_BLE_ISOCURRENTRXPTR2_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOTRCNL2_RW_BLE_ISOTRCNL2_Pos 0
  #define BLELNK_RW_BLE_ISOTRCNL2_RW_BLE_ISOTRCNL2_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOEVTCNTLOFFSETL2_RW_BLE_ISOEVTCNTLOFFSETL2_Pos 0
  #define BLELNK_RW_BLE_ISOEVTCNTLOFFSETL2_RW_BLE_ISOEVTCNTLOFFSETL2_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_ISOEVTCNTLOFFSETU2_RW_BLE_ISOEVTCNTLOFFSETU2_Pos 0
  #define BLELNK_RW_BLE_ISOEVTCNTLOFFSETU2_RW_BLE_ISOEVTCNTLOFFSETU2_Msk 0x0ffffffffUL
  #define BLELNK_RW_BLE_BLEPRIOSCHARB_RW_BLE_BLEPRIOSCHARB_Pos 0
  #define BLELNK_RW_BLE_BLEPRIOSCHARB_RW_BLE_BLEPRIOSCHARB_Msk 0x0ffffffffUL

#endif // __BLE_LINK_IF_H__
