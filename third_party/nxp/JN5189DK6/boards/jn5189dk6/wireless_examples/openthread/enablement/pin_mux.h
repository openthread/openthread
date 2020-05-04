/*
* Copyright (c) 2014 - 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2019 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef _PIN_MUX_H_
#define _PIN_MUX_H_

#include "board.h"
#include "fsl_common.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/
       /*!
        * @brief configure all pins for this demo/example
        *
        */

#define IOCON_PIO_DIGITAL_EN 0x80u     /*!<@brief Enables digital function */
#define IOCON_PIO_ECS_DI 0x00u         /*!<@brief IO is an open drain cell */
#define IOCON_PIO_EGP_GPIO 0x08u       /*!<@brief GPIO mode */
#define IOCON_PIO_EHS_DI 0x00u         /*!<@brief High speed IO for GPIO mode, IIC not */
#define IOCON_PIO_FSEL_DI 0x00u        /*!<@brief IIC mode:Noise pulses below approximately 50ns are filtered out. GPIO mode:a 3ns filter */
#define IOCON_PIO_FUNC0 0x00u          /*!<@brief Selects pin function 2 */
#define IOCON_PIO_FUNC2 0x02u          /*!<@brief Selects pin function 2 */
#define IOCON_PIO_INPFILT_OFF 0x0100u  /*!<@brief Input filter disabled */
#define IOCON_PIO_INV_DI 0x00u         /*!<@brief Input function is not inverted */
#define IOCON_PIO_MODE_PULLUP 0x00u    /*!<@brief Selects pull-up function */
#define IOCON_PIO_OPENDRAIN_DI 0x00u   /*!<@brief Open drain is disabled */
#define IOCON_PIO_SLEW0_STANDARD 0x00u /*!<@brief Standard mode, output slew rate control is disabled */
#define IOCON_PIO_SLEW1_STANDARD 0x00u /*!<@brief Standard mode, output slew rate control is disabled */
#define IOCON_PIO_SSEL_DI 0x00u        /*!<@brief SSEL is disabled */
#define IOCON_PIO_IO_CLAMP_DI 0x00u    /*!<@brief IO_CLAMP disabled */

void BOARD_InitPins(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

#endif /* _PIN_MUX_H_  */
