//------------------------------------------------------------------------------
// TI Confidential - NDA Restrictions
//
// Copyright (c) 2013 Texas Instruments, Inc.
//
//    This is an unpublished work created in the year stated above.
//    Texas Instruments owns all rights in and to this work and
//    intends to maintain and protect it as an unpublished copyright.
//    In the event of either inadvertent or deliberate publication,
//    the above stated date shall be treated as the year of first
//    publication. In the event of such publication, Texas Instruments
//    intends to enforce its rights in the work under the copyright
//    laws as a published work.
//
//------------------------------------------------------------------------------
///
/// \file            rf_prop_mailbox.h
/// \brief           Definitions for probrietary mode radio interface
///
/// \author          Low Power RF Wireless Business Unit
///                  Helge Coward (h.coward@ti.com)
///
/// \date            Thu Jan 31 11:49:17 CET 2013
///
//-----------------------------------------------------------------------------

#ifndef _PROP_MAILBOX_H
#define _PROP_MAILBOX_H

#include "rf_mailbox.h"

/// \name CPE interrupt definitions for proprietary mode
/// Interrupt masks for the CPE interrupt in RDBELL. These are new names for interrupts in mailbox.h,
/// used for compartibility with previous versions with separate interrupt numbers.
///@{
#define IRQN_PROP_RX_DATA_WRITTEN      IRQN_RX_DATA_WRITTEN
#define IRQN_PROP_N_RX_DATA_WRITTEN    IRQN_RX_N_DATA_WRITTEN
#define IRQN_PROP_TX_ENTRY_DONE        IRQN_TX_ENTRY_DONE

#define IRQN_PROP_RX_OK                IRQN_RX_OK
#define IRQN_PROP_RX_NOK               IRQN_RX_NOK
#define IRQN_PROP_RX_IGNORED           IRQN_RX_IGNORED
#define IRQN_PROP_RX_ABORTED           IRQN_RX_ABORTED
#define IRQN_PROP_RX_BUF_FULL          IRQN_RX_BUF_FULL
#define IRQN_PROP_RX_ENTRY_DONE        IRQN_RX_ENTRY_DONE

#define IRQ_PROP_RX_DATA_WRITTEN       (1U << IRQN_PROP_RX_DATA_WRITTEN)
#define IRQ_PROP_N_RX_DATA_WRITTEN     (1U << IRQN_PROP_N_RX_DATA_WRITTEN)
#define IRQ_PROP_TX_ENTRY_DONE         (1U << IRQN_PROP_TX_ENTRY_DONE)

#define IRQ_PROP_RX_OK                 (1U << IRQN_PROP_RX_OK)
#define IRQ_PROP_RX_NOK                (1U << IRQN_PROP_RX_NOK)
#define IRQ_PROP_RX_IGNORED            (1U << IRQN_PROP_RX_IGNORED)
#define IRQ_PROP_RX_ABORTED            (1U << IRQN_PROP_RX_ABORTED)
#define IRQ_PROP_RX_BUF_FULL           (1U << IRQN_PROP_RX_BUF_FULL)
#define IRQ_PROP_RX_ENTRY_DONE         (1U << IRQN_PROP_RX_ENTRY_DONE)
///@}



/// \name Radio operation status
/// Radio operation status format:
/// Bits 15:12: Protocol
///             0011: Proprietary
/// Bits 11:10: Type
///             00: Not finished
///             01: Done successfully
///             10: Done with error
/// Bits 9:0:   Identifier

/// \name Operation finished normally
///@{
#define PROP_DONE_OK            0x3400  ///< Operation ended normally
#define PROP_DONE_RXTIMEOUT     0x3401  ///< Operation stopped after end trigger while waiting for sync
#define PROP_DONE_BREAK         0x3402  ///< Rx stopped due to timeout in the middle of a packet
#define PROP_DONE_ENDED         0x3403  ///< Operation stopped after end trigger during reception
#define PROP_DONE_STOPPED       0x3404  ///< Operation stopped after stop command
#define PROP_DONE_ABORT         0x3405  ///< Operation aborted by abort command
#define PROP_DONE_RXERR         0x3406  ///< Operation ended after receiving packet with CRC error
#define PROP_DONE_IDLE          0x3407  ///< Carrier sense operation ended because of idle channel
#define PROP_DONE_BUSY          0x3408  ///< Carrier sense operation ended because of busy channel
#define PROP_DONE_IDLETIMEOUT   0x3409  ///< Carrier sense operation ended because of timeout with csConf.timeoutRes = 1
#define PROP_DONE_BUSYTIMEOUT   0x340A  ///< Carrier sense operation ended because of timeout with csConf.timeoutRes = 0

///@}
/// \name Operation finished with error
///@{
#define PROP_ERROR_PAR          0x3800  ///< Illegal parameter
#define PROP_ERROR_RXBUF        0x3801  ///< No available Rx buffer at the start of a packet
#define PROP_ERROR_RXFULL       0x3802  ///< Out of Rx buffer during reception in a partial read buffer
#define PROP_ERROR_NO_SETUP     0x3803  ///< Radio was not set up in proprietary mode
#define PROP_ERROR_NO_FS        0x3804  ///< Synth was not programmed when running Rx or Tx
#define PROP_ERROR_RXOVF        0x3805  ///< Rx overflow observed during operation
#define PROP_ERROR_TXUNF        0x3806  ///< Tx underflow observed during operation
///@}
///@}

#endif
