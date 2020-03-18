/*
* Copyright (c) 2014 - 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2019 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef _BOARD_H_
#define _BOARD_H_

#include "fsl_device_registers.h"
#include "fsl_common.h"
#include "clock_config.h"
#include "fsl_clock.h"
#include "fsl_power.h"
#include "fsl_gpio.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief The board name */
#define BOARD_NAME "DK6"

/* The UART to use for debug messages. */
#define BOARD_DEBUG_UART_TYPE       DEBUG_CONSOLE_DEVICE_TYPE_FLEXCOMM
#define BOARD_DEBUG_UART_BAUDRATE   115200U
#define BOARD_DEBUG_UART_BASEADDR   (uint32_t) USART0
#define BOARD_DEBUG_UART_CLK_FREQ   CLOCK_GetFreq(kCLOCK_Fro32M)
#define BOARD_UART_IRQ              LPUART0_IRQn
#define BOARD_UART_IRQ_HANDLER      LPUART0_IRQHandler
#define BOARD_DEBUG_UART_CLK_ATTACH kOSC32M_to_USART_CLK

/* There are 2 red LEDs on DK6 board: PIO0 and PIO3 */
#define BOARD_LED_RED_GPIO        GPIO
#define BOARD_LED_RED_GPIO_PORT   0U
#define BOARD_LED_RED_GPIO_PIN    0U
#define BOARD_LED_GREEN_GPIO      GPIO
#define BOARD_LED_GREEN_GPIO_PORT 0U
#define BOARD_LED_GREEN_GPIO_PIN  5U
#define BOARD_LED_BLUE_GPIO       GPIO
#define BOARD_LED_BLUE_GPIO_PORT  0U
#define BOARD_LED_BLUE_GPIO_PIN   3U

/* Board led color mapping */
#define LOGIC_LED_ON  0U
#define LOGIC_LED_OFF 1U

#define LED_RED_INIT(output)                                                          \
    GPIO_PinInit(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PORT, BOARD_LED_RED_GPIO_PIN, \
                 &(gpio_pin_config_t){kGPIO_DigitalOutput, (output)}) /*!< Enable target LED_RED */
#define LED_RED_ON()                                                  \
    GPIO_ClearPinsOutput(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PORT, \
                         1U << BOARD_LED_RED_GPIO_PIN) /*!< Turn on target LED_RED */
#define LED_RED_OFF()                                               \
    GPIO_SetPinsOutput(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PORT, \
                       1U << BOARD_LED_RED_GPIO_PIN) /*!< Turn off target LED_RED */
#define LED_RED_TOGGLE()                                               \
    GPIO_TogglePinsOutput(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PORT, \
                          1U << BOARD_LED_RED_GPIO_PIN) /*!< Toggle on target LED_RED */

#define LED_GREEN_INIT(output)                                                              \
    GPIO_PinInit(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PORT, BOARD_LED_GREEN_GPIO_PIN, \
                 &(gpio_pin_config_t){kGPIO_DigitalOutput, (output)}) /*!< Enable target LED_GREEN */
#define LED_GREEN_ON()                                                    \
    GPIO_ClearPinsOutput(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PORT, \
                         1U << BOARD_LED_GREEN_GPIO_PIN) /*!< Turn on target LED_GREEN */
#define LED_GREEN_OFF()                                                 \
    GPIO_SetPinsOutput(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PORT, \
                       1U << BOARD_LED_GREEN_GPIO_PIN) /*!< Turn off target LED_GREEN */
#define LED_GREEN_TOGGLE()                                                 \
    GPIO_TogglePinsOutput(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PORT, \
                          1U << BOARD_LED_GREEN_GPIO_PIN) /*!< Toggle on target LED_GREEN */

#define LED_BLUE_INIT(output)                                                            \
    GPIO_PinInit(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_GPIO_PORT, BOARD_LED_BLUE_GPIO_PIN, \
                 &(gpio_pin_config_t){kGPIO_DigitalOutput, (output)}) /*!< Enable target LED_BLUE */
#define LED_BLUE_ON()                                                   \
    GPIO_ClearPinsOutput(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_GPIO_PORT, \
                         1U << BOARD_LED_BLUE_GPIO_PIN) /*!< Turn on target LED_BLUE */
#define LED_BLUE_OFF()                                                \
    GPIO_SetPinsOutput(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_GPIO_PORT, \
                       1U << BOARD_LED_BLUE_GPIO_PIN) /*!< Turn off target LED_BLUE */
#define LED_BLUE_TOGGLE()                                                \
    GPIO_TogglePinsOutput(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_GPIO_PORT, \
                          1U << BOARD_LED_BLUE_GPIO_PIN) /*!< Toggle on target LED_BLUE */

/* There are 2 red LEDs on USB Dongle: PIO4 and PIO10 */
#define BOARD_LED_USB_DONGLE_GPIO GPIO
#define BOARD_LED_USB_DONGLE_GPIO_PORT 0U
#define BOARD_LED_USB_DONGLE1_GPIO_PIN 4U
#define BOARD_LED_USB_DONGLE2_GPIO_PIN 10U

#define BOARD_SW1_GPIO GPIO
#define BOARD_SW1_GPIO_PORT 0U
#define BOARD_SW1_GPIO_PIN  1U
#define BOARD_SW1_NAME "SW1"
#define BOARD_SW3_IRQ PIN_INT0_IRQn
#define BOARD_SW3_IRQ_HANDLER PIN_INT0_IRQHandler

#define BOARD_SW2_GPIO GPIO
#define BOARD_SW2_GPIO_PORT 0U
#define BOARD_SW2_GPIO_PIN  5U
#define BOARD_SW2_NAME "SW2"
#define BOARD_SW3_IRQ PIN_INT0_IRQn
#define BOARD_SW3_IRQ_HANDLER PIN_INT0_IRQHandler

/* Capacitance values for 32MHz and 32kHz crystals; board-specific. Value is
   pF x 100. For example, 6pF becomes 600, 1.2pF becomes 120 */
#define CLOCK_32MfXtalIecLoadpF_x100    (600) /* 6.0pF */
#define CLOCK_32MfXtalPPcbParCappF_x100 (20)  /* 0.2pF */
#define CLOCK_32MfXtalNPcbParCappF_x100 (40)  /* 0.4pF */
#define CLOCK_32kfXtalIecLoadpF_x100    (600) /* 6.0pF */
#define CLOCK_32kfXtalPPcbParCappF_x100 (40)  /* 0.4pF */
#define CLOCK_32kfXtalNPcbParCappF_x100 (40)  /* 0.4pF */

/* Capacitance variation for 32MHz crystal across temperature
   ----------------------------------------------------------

   TCXO_32M_MODE_EN should be 1 to indicate that temperature-compensated 32MHz
   XO is supported and required. If so, HW_32M_LOAD_VS_TEMP_MIN,
   _MAX, _STEP must be defined here and CLOCK_ai32MXtalIecLoadPfVsTemp_x1000
   must be defined in board.c.

   Values are used as follows:
   CLOCK_ai32MXtalIecLoadPfVsTemp_x1000 is an array of crystal load capacitance
   values across temp, with each value being at a specific temp. First value is
   for temp given by HW_32M_LOAD_VS_TEMP_MIN, next value is for
   temp given by HW_32M_LOAD_VS_TEMP_MIN + _STEP, next value is
   for temp given by HW_32M_LOAD_VS_TEMP_MIN + _ STEP x 2, etc.
   Final value is for temp given by HW_32M_LOAD_VS_TEMP_MAX. It is
   important for HW_32M_LOAD_VS_TEMP_x defines and the table to be
   matched to one another */
#define TCXO_32M_MODE_EN                     (1)

/* Values below are for NDK NX2016SA 32MHz EXS00A-CS11213-6(IEC) */

/* Temperature related to element 0 of CLOCK_ai32MXtalIecLoadPfVsTemp_x1000 */
#define HW_32M_LOAD_VS_TEMP_MIN  (-40)

/* Temperature related to final element of CLOCK_ai32MXtalIecLoadPfVsTemp_x1000 */
#define HW_32M_LOAD_VS_TEMP_MAX  (130)

/* Temperature step between elements of CLOCK_ai32MXtalIecLoadPfVsTemp_x1000 */
#define HW_32M_LOAD_VS_TEMP_STEP (5)

#define HW_32M_LOAD_VS_TEMP_SIZE ((HW_32M_LOAD_VS_TEMP_MAX \
                                   - HW_32M_LOAD_VS_TEMP_MIN) \
                                  / HW_32M_LOAD_VS_TEMP_STEP + 1U)

/* Table of load capacitance versus temperature for 32MHz crystal. Values are
   for temperatures from -40 to +130 in steps of 5 */
extern int32_t CLOCK_ai32MXtalIecLoadPfVsTemp_x1000[HW_32M_LOAD_VS_TEMP_SIZE];

/* Capacitance variation for 32kHz crystal across temperature
   ----------------------------------------------------------

   TCXO_32k_MODE_EN should be 1 to indicate that temperature-compensated 32kHz
   XO is supported and required. If so, HW_32k_LOAD_VS_TEMP_MIN,
   _MAX, _STEP must be defined here and CLOCK_ai32kXtalIecLoadPfVsTemp_x1000
   must be defined in board.c.

   Values are used as follows:
   CLOCK_ai32kXtalIecLoadPfVsTemp_x1000 is an array of crystal load capacitance
   values across temp, with each value being at a specific temp. First value is
   for temp given by HW_32k_LOAD_VS_TEMP_MIN, next value is for
   temp given by HW_32k_LOAD_VS_TEMP_MIN + _STEP, next value is
   for temp given by HW_32k_LOAD_VS_TEMP_MIN + _ STEP x 2, etc.
   Final value is for temp given by HW_32k_LOAD_VS_TEMP_MAX. It is
   important for HW_32k_LOAD_VS_TEMP_x defines and the table to be
   matched to one another */
#define TCXO_32k_MODE_EN                     (0) /* Disabled because table is
                                                    *not* correct: values below
                                                    are just for example */

/* Temperature related to element 0 of CLOCK_ai32kXtalIecLoadPfVsTemp_x1000 */
#define HW_32k_LOAD_VS_TEMP_MIN  (-20)

/* Temperature related to final element of CLOCK_ai32kXtalIecLoadPfVsTemp_x1000 */
#define HW_32k_LOAD_VS_TEMP_MAX  (100)

/* Temperature step between elements of CLOCK_ai32kXtalIecLoadPfVsTemp_x1000 */
#define HW_32k_LOAD_VS_TEMP_STEP (20)

#define HW_32k_LOAD_VS_TEMP_SIZE ((HW_32k_LOAD_VS_TEMP_MAX \
                                   - HW_32k_LOAD_VS_TEMP_MIN) \
                                  / HW_32k_LOAD_VS_TEMP_STEP + 1U)

/* Table of load capacitance versus temperature for 32kHz crystal. Values are
   for temperatures from -20 to +100 in steps of 20 */
extern int32_t CLOCK_ai32kXtalIecLoadPfVsTemp_x1000[HW_32k_LOAD_VS_TEMP_SIZE];

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * API
 ******************************************************************************/
status_t BOARD_InitDebugConsole(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _BOARD_H_ */
