/*
* Copyright (c) 2014 - 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2019 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef BBC_AND_PHY_REGS_H
#define BBC_AND_PHY_REGS_H

#if defined __cplusplus
extern "C" {
#endif


/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include "jn518x.h"
#include "jn518x_zb_mac.h"
#include "jn518x_zb_modem.h"
#include "jn518x_rfp_modem.h"

/****************************************************************************/
/***        Macro/Type Definitions                                        ***/
/****************************************************************************/
#define vREG_BbcWrite(REG, VAL)     JN518X_ZBMAC->REG = (VAL)
#define u32REG_BbcRead(REG)         (JN518X_ZBMAC->REG)
#define vREG_PhyWrite(REG, VAL)     JN518X_ZBMODEM->REG = (VAL)
#define u32REG_PhyRead(REG)         (JN518X_ZBMODEM->REG)
#define vREG_XcvrPhyWrite(REG, VAL) JN518X_ZBMODEM->REG = (VAL)
#define u32REG_XcvrPhyRead(REG)     (JN518X_ZBMODEM->REG)

#define vREG_PhyReadModWrite32(eOffset, u32Mask, u32Data)              \
    vREG_PhyWrite(eOffset, (((u32Mask) & (u32Data))                    \
                           | (~(u32Mask) & (u32REG_PhyRead(eOffset)))))

#define vREG_BbcReadModWrite32(eOffset, u32Mask, u32Data)              \
    vREG_BbcWrite(eOffset, (((u32Mask) & (u32Data))                    \
                           | (~(u32Mask) & (u32REG_BbcRead(eOffset)))))

#ifndef BIT_W_1
#define BIT_W_1                                     0x00000001UL
#define BIT_W_2                                     0x00000003UL
#define BIT_W_3                                     0x00000007UL
#define BIT_W_4                                     0x0000000FUL
#define BIT_W_5                                     0x0000001FUL
#define BIT_W_6                                     0x0000003FUL
#define BIT_W_7                                     0x0000007FUL
#define BIT_W_8                                     0x000000FFUL
#define BIT_W_10                                    0x000003FFUL
#define BIT_W_12                                    0x00000FFFUL
#define BIT_W_15                                    0x00007FFFUL
#define BIT_W_16                                    0x0000FFFFUL
#define BIT_W_17                                    0x0001FFFFUL
#define BIT_W_18                                    0x0003FFFFUL
#define BIT_W_19                                    0x0007FFFFUL
#define BIT_W_20                                    0x000FFFFFUL
#define BIT_W_21                                    0x001FFFFFUL
#define BIT_W_25                                    0x01FFFFFFUL
#endif

/**** BBC IER/ISR ****/
#define REG_BBC_INT_TX_BIT      0
#define REG_BBC_INT_TX_MASK     ((uint32)(BIT_W_1 << REG_BBC_INT_TX_BIT))
#define REG_BBC_INT_RX_H_BIT    1
#define REG_BBC_INT_RX_H_MASK   ((uint32)(BIT_W_1 << REG_BBC_INT_RX_H_BIT))
#define REG_BBC_INT_RX_BIT      2
#define REG_BBC_INT_RX_MASK     ((uint32)(BIT_W_1 << REG_BBC_INT_RX_BIT))
#define REG_BBC_INT_M0_BIT      4
#define REG_BBC_INT_M0_MASK     ((uint32)(BIT_W_1 << REG_BBC_INT_M0_BIT))
#define REG_BBC_INT_M1_BIT      5
#define REG_BBC_INT_M1_MASK     ((uint32)(BIT_W_1 << REG_BBC_INT_M1_BIT))
#define REG_BBC_INT_M2_BIT      6
#define REG_BBC_INT_M2_MASK     ((uint32)(BIT_W_1 << REG_BBC_INT_M2_BIT))
#define REG_BBC_INT_M3_BIT      7
#define REG_BBC_INT_M3_MASK     ((uint32)(BIT_W_1 << REG_BBC_INT_M3_BIT))
#define REG_BBC_INT_T0_BIT      8
#define REG_BBC_INT_T0_MASK     ((uint32)(BIT_W_1 << REG_BBC_INT_T0_BIT))
#define REG_BBC_INT_T1_BIT      9
#define REG_BBC_INT_T1_MASK     ((uint32)(BIT_W_1 << REG_BBC_INT_T1_BIT))
#define REG_BBC_INT_T2_BIT      10
#define REG_BBC_INT_T2_MASK     ((uint32)(BIT_W_1 << REG_BBC_INT_T2_BIT))
#define REG_BBC_INT_T3_BIT      11
#define REG_BBC_INT_T3_MASK     ((uint32)(BIT_W_1 << REG_BBC_INT_T3_BIT))
#define REG_BBC_INT_TIMER_MASK  ((uint32)(BIT_W_6 << REG_BBC_INT_M0_BIT))

/**** REG_BBC_TXMBEBT ****/
#define REG_BBC_TXMBEBT_MINBE_BIT     (0)
#define REG_BBC_TXMBEBT_MINBE_MASK    ((uint32)(BIT_W_4 << REG_BBC_TXMBEBT_MINBE_BIT))
#define REG_BBC_TXMBEBT_MAXBO_BIT     (4)
#define REG_BBC_TXMBEBT_MAXBO_MASK    ((uint32)(BIT_W_3 << REG_BBC_TXMBEBT_MAXBO_BIT))
#define REG_BBC_TXMBEBT_BLE_BIT       (7)
#define REG_BBC_TXMBEBT_MAXBE_BIT     (8)
#define REG_BBC_TXMBEBT_MAXBE_MASK    ((uint32)(BIT_W_4 << REG_BBC_TXMBEBT_MAXBE_BIT))
#define REG_BBC_TXMBEBT_CSMA_DLY_BIT  (12)
#define REG_BBC_TXMBEBT_CSMA_DLY_MASK ((uint32)(BIT_W_1 << REG_BBC_TXMBEBT_CSMA_DLY_BIT))
#define REG_BBC_TXMBEBT_DIR_DLY_BIT   (13)
#define REG_BBC_TXMBEBT_DIR_DLY_MASK  ((uint32)(BIT_W_4 << REG_BBC_TXMBEBT_DIR_DLY_BIT))

#define REG_BBC_TXMBEBT_FORMAT(dir_dly, min_be, ble, max_boffs, max_be) \
    (((min_be)     & BIT_W_4) | \
     (((ble)       & BIT_W_1) << REG_BBC_TXMBEBT_BLE_BIT)     | \
     (((max_be)    & BIT_W_4) << REG_BBC_TXMBEBT_MAXBE_BIT)   | \
     (((max_boffs) & BIT_W_3) << REG_BBC_TXMBEBT_MAXBO_BIT)   | \
     (((dir_dly)   & BIT_W_4) << REG_BBC_TXMBEBT_DIR_DLY_BIT)   \
    )

/**** REG_TXSTAT ****/

#define REG_BBC_TXSTAT_CCAE_BIT    0
#define REG_BBC_TXSTAT_CCAE_MASK   ((uint32)(BIT_W_1 << REG_BBC_TXSTAT_CCAE_BIT))
#define REG_BBC_TXSTAT_ACKE_BIT    1
#define REG_BBC_TXSTAT_ACKE_MASK   ((uint32)(BIT_W_1 << REG_BBC_TXSTAT_ACKE_BIT))
#define REG_BBC_TXSTAT_OOTE_BIT    2
#define REG_BBC_TXSTAT_OOTE_MASK   ((uint32)(BIT_W_1 << REG_BBC_TXSTAT_OOTE_BIT))
#define REG_BBC_TXSTAT_RXABT_BIT   3
#define REG_BBC_TXSTAT_RXABT_MASK  ((uint32)(BIT_W_1 << REG_BBC_TXSTAT_RXABT_BIT))
#define REG_BBC_TXSTAT_RXFP_BIT    4
#define REG_BBC_TXSTAT_RXFP_MASK   ((uint32)(BIT_W_1 << REG_BBC_TXSTAT_RXFP_BIT))
#define REG_BBC_TXSTAT_TXTO_BIT    5
#define REG_BBC_TXSTAT_TXTO_MASK   ((uint32)(BIT_W_1 << REG_BBC_TXSTAT_TXTO_BIT))
#define REG_BBC_TXSTAT_TXPCTO_BIT  6
#define REG_BBC_TXSTAT_TXPCTO_MASK ((uint32)(BIT_W_1 << REG_BBC_TXSTAT_TXPCTO_BIT))

/**** TXCTL ****/

#define REG_BBC_TXCTL_SCH_BIT   0
#define REG_BBC_TXCTL_SCH_MASK  ((uint32)(BIT_W_1 << REG_BBC_TXCTL_SCH_BIT))
#define REG_BBC_TXCTL_SS_BIT    1
#define REG_BBC_TXCTL_SS_MASK   ((uint32)(BIT_W_1 << REG_BBC_TXCTL_SS_BIT))
#define REG_BBC_TXCTL_SOVR_BIT  2
#define REG_BBC_TXCTL_SOVR_MASK ((uint32)(BIT_W_1 << REG_BBC_TXCTL_SOVR_BIT))
#define REG_BBC_TXCTL_AA_BIT    3
#define REG_BBC_TXCTL_AA_MASK   ((uint32)(BIT_W_1 << REG_BBC_TXCTL_AA_BIT))
#define REG_BBC_TXCTL_MODE_BIT  4
#define REG_BBC_TXCTL_MODE_MASK ((uint32)(BIT_W_2 << REG_BBC_TXCTL_MODE_BIT))

#define REG_BBC_TXCTL_VALUE(sched_basis, sched_ss, slot_override, auto_ack, mode) \
   (((sched_basis)    & BIT_W_1) | \
    (((sched_ss)      & BIT_W_1) << REG_BBC_TXCTL_SS_BIT) | \
    (((slot_override) & BIT_W_1) << REG_BBC_TXCTL_SOVR_BIT) | \
    (((auto_ack)      & BIT_W_1) << REG_BBC_TXCTL_AA_BIT) | \
    (((mode)          & BIT_W_2) << REG_BBC_TXCTL_MODE_BIT))

#define REG_BBC_TXCTL_SEND_AT(mode) REG_BBC_TXCTL_VALUE(1, 1, 0, 1, (mode))
#define REG_BBC_TXCTL_SEND_NOW(mode) REG_BBC_TXCTL_VALUE(0, 1, 0, 1, (mode))

/**** RXMPID ****/
#define REG_BBC_RXMPID_PAN_ID_BIT  0
#define REG_BBC_RXMPID_PAN_ID_MASK ((uint32)(BIT_W_16 << REG_BBC_RXMPID_PAN_ID_BIT))
#define REG_BBC_RXMPID_COORD_BIT   16
#define REG_BBC_RXMPID_COORD_MASK  ((uint32)(BIT_W_1 << REG_BBC_RXMPID_COORD_BIT))

/**** RXPROM ****/
#define REG_BBC_RXPROM_AM_BIT    0
#define REG_BBC_RXPROM_AM_MASK   ((uint32)(BIT_W_1 << REG_BBC_RXPROM_AM_BIT))
#define REG_BBC_RXPROM_FCSE_BIT  1
#define REG_BBC_RXPROM_FCSE_MASK ((uint32)(BIT_W_1 << REG_BBC_RXPROM_FCSE_BIT))
#define REG_BBC_RXPROM_AMAL_BIT  2
#define REG_BBC_RXPROM_AMAL_MASK ((uint32)(BIT_W_1 << REG_BBC_RXPROM_AMAL_BIT))

/**** REG_RXSTAT ****/
#define REG_BBC_RXSTAT_FCSE_BIT   0
#define REG_BBC_RXSTAT_FCSE_MASK  ((uint32)(BIT_W_1 << REG_BBC_RXSTAT_FCSE_BIT))
#define REG_BBC_RXSTAT_ABORT_BIT  1
#define REG_BBC_RXSTAT_ABORT_MASK ((uint32)(BIT_W_1 << REG_BBC_RXSTAT_ABORT_BIT))
#define REG_BBC_RXSTAT_INPKT_BIT  4
#define REG_BBC_RXSTAT_INPKT_MASK ((uint32)(BIT_W_1 << REG_BBC_RXSTAT_INPKT_BIT))
#define REG_BBC_RXSTAT_MAL_BIT    5
#define REG_BBC_RXSTAT_MAL_MASK   ((uint32)(BIT_W_1 << REG_BBC_RXSTAT_MAL_BIT))


/**** RXCTL ****/

#define REG_BBC_RXCTL_SCH_BIT   0
#define REG_BBC_RXCTL_SCH_MASK  ((uint32)(BIT_W_1 << REG_BBC_RXCTL_SCH_BIT))
#define REG_BBC_RXCTL_SS_BIT    1
#define REG_BBC_RXCTL_SS_MASK   ((uint32)(BIT_W_1 << REG_BBC_RXCTL_SS_BIT))
#define REG_BBC_RXCTL_ICAP_BIT  2
#define REG_BBC_RXCTL_ICAP_MASK ((uint32)(BIT_W_1 << REG_BBC_RXCTL_ICAP_BIT))
#define REG_BBC_RXCTL_AA_BIT    3
#define REG_BBC_RXCTL_AA_MASK   ((uint32)(BIT_W_1 << REG_BBC_RXCTL_AA_BIT))
#define REG_BBC_RXCTL_PRSP_BIT  4
#define REG_BBC_RXCTL_PRSP_MASK ((uint32)(BIT_W_1 << REG_BBC_RXCTL_PRSP_BIT))

#define REG_BBC_RXCTL_FORMAT(sched_basis, sched_ss, in_cap, auto_ack) \
   (((sched_basis) & BIT_W_1) | \
    (((sched_ss)   & BIT_W_1) << REG_BBC_RXCTL_SS_BIT) | \
    (((in_cap)     & BIT_W_1) << REG_BBC_RXCTL_ICAP_BIT) | \
    (((auto_ack)   & BIT_W_1) << REG_BBC_RXCTL_AA_BIT))

/**** SM_STATE ****/
#define REG_BBC_SM_STATE_SUP_BIT   0
#define REG_BBC_SM_STATE_SUP_MASK  ((uint32)(BIT_W_4 << REG_BBC_SM_STATE_SUP_BIT))
#define REG_BBC_SM_STATE_CSMA_BIT  4
#define REG_BBC_SM_STATE_CSMA_MASK ((uint32)(BIT_W_3 << REG_BBC_SM_STATE_CSMA_BIT))
#define REG_BBC_SM_STATE_ISA_BIT   8
#define REG_BBC_SM_STATE_ISA_MASK  ((uint32)(BIT_W_5 << REG_BBC_SM_STATE_ISA_BIT))

/**** SCTCR ****/

#define REG_BBC_SCTCR_E0_BIT    0
#define REG_BBC_SCTCR_E0_MASK   ((uint32)(BIT_W_1 << REG_BBC_SCTCR_E0_BIT))
#define REG_BBC_SCTCR_E1_BIT    1
#define REG_BBC_SCTCR_E1_MASK   ((uint32)(BIT_W_1 << REG_BBC_SCTCR_E1_BIT))

/**** SCTL ****/
#define REG_BBC_SCTL_USE_BIT    0
#define REG_BBC_SCTL_USE_MASK   ((uint32)(BIT_W_1 << REG_BBC_SCTL_USE_BIT))
#define REG_BBC_SCTL_SNAP_BIT   1
#define REG_BBC_SCTL_SNAP_MASK  ((uint32)(BIT_W_1 << REG_BBC_SCTL_SNAP_BIT))
#define REG_BBC_SCTL_CO_BIT     2
#define REG_BBC_SCTL_CO_MASK    ((uint32)(BIT_W_1 << REG_BBC_SCTL_CO_BIT))
// bit name changed
#define REG_BBC_SCTL_CE_BIT     2
#define REG_BBC_SCTL_CE_MASK    ((uint32)(BIT_W_1 << REG_BBC_SCTL_CE_BIT))
#define REG_BBC_SCTL_PHYON_BIT  3
#define REG_BBC_SCTL_PHYON_MASK ((uint32)(BIT_W_1 << REG_BBC_SCTL_PHYON_BIT))

/**** RXFCTL/TXFCTL ****/

/* Operations on Frame header */
#define REG_BBC_FCTL_TYPE_BIT           0
#define REG_BBC_FCTL_TYPE_MASK          ((uint32)(BIT_W_3/* << REG_BBC_FCTL_TYPE_BIT*/)) /* Optimised bit 0 */

#define REG_BBC_FCTL_SEC_BIT            3
#define REG_BBC_FCTL_SEC_MASK           ((uint32)(BIT_W_1 << REG_BBC_FCTL_SEC_BIT))

#define REG_BBC_FCTL_FP_BIT             4
#define REG_BBC_FCTL_FP_MASK            ((uint32)(BIT_W_1 << REG_BBC_FCTL_FP_BIT))

#define REG_BBC_FCTL_ACK_BIT            5
#define REG_BBC_FCTL_ACK_MASK           ((uint32)(BIT_W_1 << REG_BBC_FCTL_ACK_BIT))

#define REG_BBC_FCTL_IP_BIT             6
#define REG_BBC_FCTL_IP_MASK            ((uint32)(BIT_W_1 << REG_BBC_FCTL_IP_BIT))

#define REG_BBC_FCTL_DAM_BIT            10
#define REG_BBC_FCTL_DAM_MASK           ((uint32)(BIT_W_2 << REG_BBC_FCTL_DAM_BIT))
#define REG_BBC_FCTL_DAM(x)             (((x) & REG_BBC_FCTL_DAM_MASK) >> REG_BBC_FCTL_DAM_BIT)

#define REG_BBC_FCTL_SAM_BIT            14
#define REG_BBC_FCTL_SAM_MASK           ((uint32)(BIT_W_2 << REG_BBC_FCTL_SAM_BIT))
#define REG_BBC_FCTL_SAM(x)             (((x) & REG_BBC_FCTL_SAM_MASK) >> REG_BBC_FCTL_SAM_BIT)

/* Frame types */
#define REG_BBC_FCTL_TYPE_BEACON        0
#define REG_BBC_FCTL_TYPE_DATA          1
#define REG_BBC_FCTL_TYPE_ACK           2
#define REG_BBC_FCTL_TYPE_CMD           3

/* Addr modes */
#define REG_BBC_FCTL_AM_NONE            0
#define REG_BBC_FCTL_AM_RSVD            1
#define REG_BBC_FCTL_AM_SHORT           2
#define REG_BBC_FCTL_AM_EXT             3


#define REG_BBC_FCTL_FORMAT(type, sec, fp, ack, ip, dam, sam) \
    (((type) & BIT_W_3) | \
     (((sec) & BIT_W_1) << REG_BBC_FCTL_SEC_BIT) | \
     (((fp) & BIT_W_1) << REG_BBC_FCTL_FP_BIT) | \
     (((ack) & BIT_W_1) << REG_BBC_FCTL_ACK_BIT) | \
     (((ip) & BIT_W_1) << REG_BBC_FCTL_IP_BIT) | \
     (((dam) & BIT_W_2) << REG_BBC_FCTL_DAM_BIT) | \
     (((sam) & BIT_W_2) << REG_BBC_FCTL_SAM_BIT))


/**** MCCA_CTRL ****/
#define REG_PHY_MCCA_CCAM_BIT           0
#define REG_PHY_MCCA_CCAM_MASK          ((uint32)(BIT_W_2 << REG_PHY_MCCA_CCAM_BIT))
#define REG_PHY_MCCA_CCA_ED_THR_BIT     2
#define REG_PHY_MCCA_CCA_ED_THR_MASK    ((uint32)(BIT_W_10 << REG_PHY_MCCA_CCA_ED_THR_BIT))

/**** MSTAT ****/
#define REG_PHY_MSTAT_ED_BIT           6
#define REG_PHY_MSTAT_ED_MASK          ((uint32)(BIT_W_10 << REG_PHY_MSTAT_ED_BIT))
#define REG_PHY_MSTAT_SQI_BIT          16
#define REG_PHY_MSTAT_SQI_MASK         ((uint32)(BIT_W_8 << REG_PHY_MSTAT_SQI_BIT))
#define REG_PHY_MSTAT_MCCAS_BIT        24
#define REG_PHY_MSTAT_MCCAS_MASK       ((uint32)(BIT_W_1 << REG_PHY_MSTAT_MCCAS_BIT))

/**** PHY_MCTRL ****/
#define REG_PHY_MCTRL_MIOM_BIT          1
#define REG_PHY_MCTRL_MIOM_MASK         ((uint32)(BIT_W_1 << REG_PHY_MCTRL_MIOM_BIT))
#define REG_PHY_MCTRL_MPHYON_BIT        2
#define REG_PHY_MCTRL_MPHYON_MASK       ((uint32)(BIT_W_1 << REG_PHY_MCTRL_MPHYON_BIT))
#define REG_PHY_MCTRL_MPHYTX_BIT        3
#define REG_PHY_MCTRL_MPHYTX_MASK       ((uint32)(BIT_W_1 << REG_PHY_MCTRL_MPHYTX_BIT))
#define REG_PHY_MCTRL_MCCAT_BIT         4
#define REG_PHY_MCTRL_MCCAT_MASK        ((uint32)(BIT_W_1 << REG_PHY_MCTRL_MCCAT_BIT))
#define REG_PHY_MCTRL_MEDT_BIT          5
#define REG_PHY_MCTRL_MEDT_MASK         ((uint32)(BIT_W_1 << REG_PHY_MCTRL_MEDT_BIT))

/**** PHY_PWR ****/
#define REG_PHY_PWR_BIT                 0
#define REG_PHY_PWR_MASK                ((uint32)(BIT_W_1 << REG_PHY_PWR_BIT))

/**** PHY IER/ISR ****/
#define REG_PHY_INT_ED_BIT               3
#define REG_PHY_INT_ED_MASK              ((uint32)(BIT_W_1 << REG_PHY_INT_ED_BIT))
#define REG_PHY_INT_CCA_BIT              4
#define REG_PHY_INT_CCA_MASK             ((uint32)(BIT_W_1 << REG_PHY_INT_CCA_BIT))

/* Register equivalents, mapping from old names to new ones */
#define REG_BBC_ISR             ISR
#define REG_BBC_IER             IER
#define REG_BBC_RXCTL           RXCTL
#define REG_BBC_SM_STATE        SM_STATE
#define REG_BBC_MISR            MISR
#define REG_BBC_SCFRC           SCFRC
#define REG_BBC_SCESL           SCESL
#define REG_BBC_RXMPID          RXMPID
#define REG_BBC_RXMSAD          RXMSAD
#define REG_BBC_RXMEADL         RXMEADL
#define REG_BBC_RXMEADH         RXMEADH
#define REG_BBC_RXETST          RXETST
#define REG_BBC_TXCTL           TXCTL
#define REG_BBC_SCTL            SCTL
#define REG_BBC_RXBUFAD         RXBUFAD
#define REG_BBC_RXPROM          RXPROM
#define REG_BBC_RXSTAT          RXSTAT
#define REG_BBC_RXTSTP          RXTSTP
#define REG_BBC_TXRETRY         TXTRIES
#define REG_BBC_TXMBEBT         TXMBEBT
#define REG_BBC_TXTSTP          TXTSTP
#define REG_BBC_TXCSMAC         TXCSMAC
#define REG_BBC_TXBUFAD         TXBUFAD
#define REG_BBC_TXSTAT          TXSTAT
#define REG_BBC_TXASC           TXASC
#define REG_BBC_PRBSS           PRBS_SEED
#define REG_BBC_SCMR0           SCMR0
#define REG_BBC_SCTR1           SCTR1
#define REG_BBC_SCTCR           SCTCR
#define REG_BBC_TXPEND          TXPEND
#define REG_BBC_PRTT            PHYRXTUNETIME
#define REG_BBC_TAT             TURNAROUNDTIME
#define REG_BBC_LIFS_TURNAROUND LIFSTURNAROUNDTIME
#define REG_BBC_SCTR0           SCTR0

#define REG_PHY_CHAN            PHY_CHAN
#define REG_PHY_MCTRL           PHY_MCTRL
#define REG_PHY_MSTAT0          MSTAT
#define REG_PHY_MCCA            MCCA_CTRL

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif /* #ifndef BBC_AND_PHY_REGS_H */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
