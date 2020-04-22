/*
* Copyright (c) 2014 - 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2019 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "fsl_common.h"
#include "fsl_iocon.h"
#include "fsl_gpio.h"
#include "pin_mux.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

#ifndef BOARD_USECLKINSRC
#define BOARD_USECLKINSRC (0)
#endif

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Local Prototypes
 ****************************************************************************/
/*****************************************************************************
* Private functions
****************************************************************************/
static void ConfigureConsolePort(void)
{
    /* UART0 RX/TX pins */
    IOCON_PinMuxSet(IOCON, 0, 8, IOCON_MODE_INACT | IOCON_FUNC2 | IOCON_DIGITAL_EN);
    IOCON_PinMuxSet(IOCON, 0, 9, IOCON_MODE_INACT | IOCON_FUNC2 | IOCON_DIGITAL_EN);
}

static void ConfigureDebugPort(void)
{
    /* SWD SWCLK/SWDIO pins */
    IOCON_PinMuxSet(IOCON, 0, 12, IOCON_FUNC2 | IOCON_MODE_INACT | IOCON_DIGITAL_EN);
    IOCON_PinMuxSet(IOCON, 0, 13, IOCON_FUNC2 | IOCON_MODE_INACT | IOCON_DIGITAL_EN);
#ifdef ENABLE_DEBUG_PORT_SWO
    /* SWD SWO pin (optional) */
    IOCON_PinMuxSet(IOCON, 0, 14, IOCON_FUNC5 | IOCON_MODE_INACT | IOCON_DIGITAL_EN);
    SYSCON->TRACECLKDIV = 0; /* Clear HALT bit */
#endif
}

static void ConfigureDongleLEDs(void)
{
    const uint32_t port0_pin4_config = (/* Pin is configured as PIO0_4 */
                                        IOCON_PIO_FUNC0 |
                                        /* Selects pull-up function */
                                        IOCON_PIO_MODE_PULLUP |
                                        /* Standard mode, output slew rate control is disabled */
                                        IOCON_PIO_SLEW0_STANDARD |
                                        /* Input function is not inverted */
                                        IOCON_PIO_INV_DI |
                                        /* Enables digital function */
                                        IOCON_PIO_DIGITAL_EN |
                                        /* Input filter disabled */
                                        IOCON_PIO_INPFILT_OFF |
                                        /* Standard mode, output slew rate control is disabled */
                                        IOCON_PIO_SLEW1_STANDARD |
                                        /* Open drain is disabled */
                                        IOCON_PIO_OPENDRAIN_DI |
                                        /* SSEL is disabled */
                                        IOCON_PIO_SSEL_DI);
    /* PORT0 PIN4 (coords: 7) is configured as PIO0_4 */
    IOCON_PinMuxSet(IOCON, 0U, 4U, port0_pin4_config);

    const uint32_t port0_pin10_config = (/* Pin is configured as PIO0_10 */
                                         IOCON_PIO_FUNC0 |
                                         /* GPIO mode */
                                         IOCON_PIO_EGP_GPIO |
                                         /* IO is an open drain cell */
                                         IOCON_PIO_ECS_DI |
                                         /* High speed IO for GPIO mode, IIC not */
                                         IOCON_PIO_EHS_DI |
                                         /* Input function is not inverted */
                                         IOCON_PIO_INV_DI |
                                         /* Enables digital function */
                                         IOCON_PIO_DIGITAL_EN |
                                         /* Input filter disabled */
                                         IOCON_PIO_INPFILT_OFF |
                                         /* IIC mode:Noise pulses below approximately 50ns are filtered out. GPIO mode:a 3ns filter */
                                         IOCON_PIO_FSEL_DI |
                                         /* Open drain is disabled */
                                         IOCON_PIO_OPENDRAIN_DI |
                                         /* IO_CLAMP disabled */
                                         IOCON_PIO_IO_CLAMP_DI);
    /* PORT0 PIN10 (coords: 13) is configured as PIO0_10 */
    IOCON_PinMuxSet(IOCON, 0U, 10U, port0_pin10_config);
}

/*******************************************************************************
 * Code
 ******************************************************************************/
void BOARD_InitPins(void)
{
    /* Define the init structure for the output LED pin*/
    gpio_pin_config_t led_config = {
        kGPIO_DigitalOutput,
        0,
    };

    /* Enable IOCON clock */
    CLOCK_EnableClock(kCLOCK_Iocon);
    CLOCK_EnableClock(kCLOCK_InputMux);

    /* Console signals */
    ConfigureConsolePort();

    /* Debugger signals */
    ConfigureDebugPort();

    ConfigureDongleLEDs();

    /* IOCON clock left on, this is needed if CLKIN is used. */
    /* Initialize GPIO */
    CLOCK_EnableClock(kCLOCK_Gpio0);
    RESET_PeripheralReset(kGPIO0_RST_SHIFT_RSTn);

    /* Init output LED GPIO. */
    GPIO_PortInit(BOARD_LED_USB_DONGLE_GPIO, BOARD_LED_USB_DONGLE_GPIO_PORT);
    GPIO_PinInit(BOARD_LED_USB_DONGLE_GPIO, BOARD_LED_USB_DONGLE_GPIO_PORT, BOARD_LED_USB_DONGLE1_GPIO_PIN, &led_config);
    GPIO_PinInit(BOARD_LED_USB_DONGLE_GPIO, BOARD_LED_USB_DONGLE_GPIO_PORT, BOARD_LED_USB_DONGLE2_GPIO_PIN, &led_config);

    GPIO_PortToggle(BOARD_LED_USB_DONGLE_GPIO, BOARD_LED_USB_DONGLE_GPIO_PORT, 1u << BOARD_LED_USB_DONGLE1_GPIO_PIN);
}


void BOARD_LedDongleToggle(void)
{
    GPIO_PortToggle(BOARD_LED_USB_DONGLE_GPIO, BOARD_LED_USB_DONGLE_GPIO_PORT, 1u << BOARD_LED_USB_DONGLE1_GPIO_PIN);
    GPIO_PortToggle(BOARD_LED_USB_DONGLE_GPIO, BOARD_LED_USB_DONGLE_GPIO_PORT, 1u << BOARD_LED_USB_DONGLE2_GPIO_PIN);
}
