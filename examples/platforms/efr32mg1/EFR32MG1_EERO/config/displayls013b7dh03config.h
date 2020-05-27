/***************************************************************************//**
 * @file
 * @brief BRD4151A specific configuration for the display driver for
 *        the Sharp Memory LCD model LS013B7DH03.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#ifndef DISPLAY_LS013B7DH03_CONFIG_H
#define DISPLAY_LS013B7DH03_CONFIG_H

#include "displayconfigapp.h"

/* Display device name. */
#define SHARP_MEMLCD_DEVICE_NAME   "Sharp LS013B7DH03 #1"

/* LCD and SPI GPIO pin connections on the BRD4151A. */
#define LCD_PORT_SCLK             (gpioPortC)  /* EFM_DISP_SCLK on PC8 */
#define LCD_PIN_SCLK              (8)
#define LCD_PORT_SI               (gpioPortC)  /* EFM_DISP_MOSI on PC6 */
#define LCD_PIN_SI                (6)
#define LCD_PORT_SCS              (gpioPortD)  /* EFM_DISP_CS on PD14 */
#define LCD_PIN_SCS               (14)
#define LCD_PORT_EXTCOMIN         (gpioPortD)  /* EFM_DISP_COM on PD13 */
#define LCD_PIN_EXTCOMIN          (13)
#define LCD_PORT_DISP_PWR         (gpioPortD)  /* EFM_DISP_ENABLE on PD15 */
#define LCD_PIN_DISP_PWR          (15)

/* PRS settings for polarity inversion extcomin auto toggle.  */
#define LCD_AUTO_TOGGLE_PRS_CH    (4)  /* PRS channel 4.      */
#define LCD_AUTO_TOGGLE_PRS_ROUTELOC()  PRS->ROUTELOC1 = \
  ((PRS->ROUTELOC1 & ~_PRS_ROUTELOC1_CH4LOC_MASK) | PRS_ROUTELOC1_CH4LOC_LOC4)
#define LCD_AUTO_TOGGLE_PRS_ROUTEPEN    PRS_ROUTEPEN_CH4PEN

/*
 * Select how LCD polarity inversion should be handled:
 *
 * If POLARITY_INVERSION_EXTCOMIN is defined, the polarity inversion is armed
 * for every rising edge of the EXTCOMIN pin. The actual polarity inversion is
 * triggered at the next transision of SCS. This mode is recommended because it
 * causes less CPU and SPI load than the alternative mode, see below.
 * If POLARITY_INVERSION_EXTCOMIN is undefined, the polarity inversion is
 * toggled by sending an SPI command. This mode causes more CPU and SPI load
 * than using the EXTCOMIN pin mode.
 */
#define POLARITY_INVERSION_EXTCOMIN

/* Define POLARITY_INVERSION_EXTCOMIN_PAL_AUTO_TOGGLE if you want the PAL
 * (Platform Abstraction Layer interface) to automatically toggle the EXTCOMIN
 *  pin.
 * If the PAL_TIMER_REPEAT function is defined the EXTCOMIN toggling is handled
 * by a timer repeat system, therefore we must undefine
 * POLARITY_INVERSION_EXTCOMIN_PAL_AUTO_TOGGLE;
 */
#ifndef PAL_TIMER_REPEAT_FUNCTION
  #define POLARITY_INVERSION_EXTCOMIN_PAL_AUTO_TOGGLE
#endif

#endif /* DISPLAY_LS013B7DH03_CONFIG_H */
