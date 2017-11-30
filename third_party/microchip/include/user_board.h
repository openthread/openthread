/**
 * \file
 *
 * \brief User board definition template
 *
 */

 /* This file is intended to contain definitions and configuration details for
 * features and devices that are available on the board, e.g., frequency and
 * startup time for an external crystal, external memory devices, LED and USART
 * pins.
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#ifndef USER_BOARD_H
#define USER_BOARD_H

#include <conf_board.h>
#include <compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Resonator definitions */
#define BOARD_FREQ_SLCK_XTAL            (32768U)
#define BOARD_FREQ_SLCK_BYPASS          (32768U)
#define BOARD_FREQ_MAINCK_XTAL          0 /* Not Mounted */
#define BOARD_FREQ_MAINCK_BYPASS        0 /* Not Mounted */
#define BOARD_MCK                       CHIP_FREQ_CPU_MAX
#define BOARD_OSC_STARTUP_US            15625

/** UART interface definitions */
#define UART_SERCOM_MODULE              SERCOM2
#define UART_SERCOM_MUX_SETTING         USART_RX_3_TX_2_XCK_3
#define UART_SERCOM_PINMUX_PAD0         PINMUX_UNUSED
#define UART_SERCOM_PINMUX_PAD1         PINMUX_UNUSED
#define UART_SERCOM_PINMUX_PAD2         PINMUX_PA14C_SERCOM2_PAD2
#define UART_SERCOM_PINMUX_PAD3         PINMUX_PA15C_SERCOM2_PAD3
#define UART_SERCOM_DMAC_ID_TX          SERCOM2_DMAC_ID_TX
#define UART_SERCOM_DMAC_ID_RX          SERCOM2_DMAC_ID_RX

/** RF SPI interface definitions */
#define RF_SPI_MODULE                   SERCOM4
#define RF_SPI_SERCOM_MUX_SETTING       SPI_SIGNAL_MUX_SETTING_E
#define RF_SPI_SERCOM_PINMUX_PAD0       PINMUX_PC19F_SERCOM4_PAD0
#define RF_SPI_SERCOM_PINMUX_PAD1       PINMUX_PB31D_SERCOM5_PAD1
#define RF_SPI_SERCOM_PINMUX_PAD2       PINMUX_PB30F_SERCOM4_PAD2
#define RF_SPI_SERCOM_PINMUX_PAD3       PINMUX_PC18F_SERCOM4_PAD3

#define RF_IRQ_MODULE                   EIC
#define RF_IRQ_INPUT                    0
#define RF_IRQ_PIN                      PIN_PB00A_EIC_EXTINT0
#define RF_IRQ_MUX                      MUX_PB00A_EIC_EXTINT0
#define RF_IRQ_PINMUX                   PINMUX_PB00A_EIC_EXTINT0

/** 802.15.4 TRX Interface definitions */
#define AT86RFX_SPI                     SERCOM4
#define AT86RFX_RST_PIN                 PIN_PB15
#define AT86RFX_IRQ_PIN                 PIN_PB00
#define AT86RFX_SLP_PIN                 PIN_PA20
#define AT86RFX_SPI_CS                  PIN_PB31
#define AT86RFX_SPI_MOSI                PIN_PB30
#define AT86RFX_SPI_MISO                PIN_PC19
#define AT86RFX_SPI_SCK                 PIN_PC18
#define RFCTRL_CFG_ANT_DIV              4

#define AT86RFX_SPI_SERCOM_MUX_SETTING  RF_SPI_SERCOM_MUX_SETTING
#define AT86RFX_SPI_SERCOM_PINMUX_PAD0  RF_SPI_SERCOM_PINMUX_PAD0
#define AT86RFX_SPI_SERCOM_PINMUX_PAD1  PINMUX_UNUSED
#define AT86RFX_SPI_SERCOM_PINMUX_PAD2  RF_SPI_SERCOM_PINMUX_PAD2
#define AT86RFX_SPI_SERCOM_PINMUX_PAD3  RF_SPI_SERCOM_PINMUX_PAD3

#define AT86RFX_IRQ_CHAN                RF_IRQ_INPUT
#define AT86RFX_IRQ_PINMUX              RF_IRQ_PINMUX

/** Enables the transceiver main interrupt. */
#define ENABLE_TRX_IRQ()    \
        extint_chan_enable_callback(AT86RFX_IRQ_CHAN, EXTINT_CALLBACK_TYPE_DETECT)

/** Disables the transceiver main interrupt. */
#define DISABLE_TRX_IRQ()   \
        extint_chan_disable_callback(AT86RFX_IRQ_CHAN, EXTINT_CALLBACK_TYPE_DETECT)

/** Clears the transceiver main interrupt. */
#define CLEAR_TRX_IRQ()     \
        extint_chan_clear_detected(AT86RFX_IRQ_CHAN);

/** This macro saves the trx interrupt status and disables the trx interrupt. */
#define ENTER_TRX_REGION()   \
        { extint_chan_disable_callback(AT86RFX_IRQ_CHAN, EXTINT_CALLBACK_TYPE_DETECT)

/* This macro restores the transceiver interrupt status */
#define LEAVE_TRX_REGION()   \
        extint_chan_enable_callback(AT86RFX_IRQ_CHAN, EXTINT_CALLBACK_TYPE_DETECT); }

#ifdef __cplusplus
}
#endif

#endif // USER_BOARD_H
