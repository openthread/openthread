/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_IOCON_H_
#define _FSL_IOCON_H_

#include "fsl_common.h"
/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.jn_iocon"
#endif

/*!
 * @addtogroup jn_iocon
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief IOCON driver version 2.0.0. */
#define LPC_IOCON_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/**
 * @brief Array of IOCON pin definitions passed to IOCON_SetPinMuxing() must be in this format
 */
typedef struct _iocon_group
{
    uint32_t port : 8;      /* Pin port */
    uint32_t pin : 8;       /* Pin number */
    uint32_t modefunc : 16; /* Function and mode */
} iocon_group_t;

/**
 * @brief IOCON function and mode selection definitions
 * @note See the User Manual for specific modes and functions supported by the various pins.
 */
#define IOCON_FUNC0 IOCON_PIO_FUNC(0) /*!< Selects pin function 0 */
#define IOCON_FUNC1 IOCON_PIO_FUNC(1) /*!< Selects pin function 1 */
#define IOCON_FUNC2 IOCON_PIO_FUNC(2) /*!< Selects pin function 2 */
#define IOCON_FUNC3 IOCON_PIO_FUNC(3) /*!< Selects pin function 3 */
#define IOCON_FUNC4 IOCON_PIO_FUNC(4) /*!< Selects pin function 4 */
#define IOCON_FUNC5 IOCON_PIO_FUNC(5) /*!< Selects pin function 5 */
#define IOCON_FUNC6 IOCON_PIO_FUNC(6) /*!< Selects pin function 6 */
#define IOCON_FUNC7 IOCON_PIO_FUNC(7) /*!< Selects pin function 7 */

#define IOCON_MODE_PULLUP IOCON_PIO_MODE(0)   /*!< Selects pull-up function */
#define IOCON_MODE_REPEATER IOCON_PIO_MODE(1) /*!< Selects pin repeater function */
#define IOCON_MODE_INACT IOCON_PIO_MODE(2)    /*!< No addition pin function */
#define IOCON_MODE_PULLDOWN IOCON_PIO_MODE(3) /*!< Selects pull-down function */

#define IOCON_HYS_EN (0x1 << 5)            /*!< Enables hysteresis ??*/
#define IOCON_GPIO_MODE IOCON_PIO_EGP(1)   /*!< GPIO Mode */
#define IOCON_I2C_SLEW IOCON_PIO_SLEW0(1)  /*!< I2C Slew Rate Control */

#define IOCON_INV_EN IOCON_PIO_INVERT(1) /*!< Enables invert function on input */

#define IOCON_ANALOG_EN IOCON_PIO_DIGIMODE(0)  /*!< Enables analog function by setting 0 to bit 7 */
#define IOCON_DIGITAL_EN IOCON_PIO_DIGIMODE(1) /*!< Enables digital function by setting 1 to bit 7(default) */

#define IOCON_STDI2C_EN IOCON_PIO_FILTEROFF(1) /*!< I2C standard mode/fast-mode */

#define IOCON_INPFILT_OFF IOCON_PIO_FILTEROFF(1) /*!< Input filter Off for GPIO pins */
#define IOCON_INPFILT_ON IOCON_PIO_FILTEROFF(0)  /*!< Input filter On for GPIO pins */

#define IOCON_SLEW1_OFF IOCON_PIO_SLEW1(0) /*!< Driver Slew Rate Control */
#define IOCON_SLEW1_ON IOCON_PIO_SLEW1(1)  /*!< Driver Slew Rate Control */

#define IOCON_FASTI2C_EN (IOCON_INPFILT_ON | IOCON_SLEW1_ON) /*!< I2C Fast-mode Plus and high-speed slave */

#define IOCON_OPENDRAIN_EN IOCON_PIO_OD(1) /*!< Enables open-drain function */

#define IOCON_S_MODE_0CLK IOCON_PIO_SSEL(0) /*!< Bypass input filter */
#define IOCON_S_MODE_1CLK IOCON_PIO_SSEL(1) /*!< Input pulses shorter than 1 filter clock are rejected */
#define IOCON_S_MODE_2CLK IOCON_PIO_SSEL(2) /*!< Input pulses shorter than 2 filter clock2 are rejected */
#define IOCON_S_MODE_3CLK IOCON_PIO_SSEL(3) /*!< Input pulses shorter than 3 filter clock2 are rejected */

/* Set IO clamping to the DIO : freeze the IO state.
 * Requires SYSCON->RETENTIONCTRL.IOCLAMPING=1 . Automatically set in powerdown */
#define IOCON_IO_CLAMPING_NORMAL_MFIO (1 << 11)
#define IOCON_IO_CLAMPING_COMBO_MFIO_I2C (1 << 12) /* Use this flag for PIO11 and PIO12 only */

#define IOCON_PIO_DBG_FUNC_MASK        (0xF000U)
#define IOCON_PIO_DBG_FUNC_SHIFT       (12U)
#define IOCON_PIO_DBG_FUNC(x)          (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_DBG_FUNC_SHIFT)) & IOCON_PIO_DBG_FUNC_MASK)
#define IOCON_PIO_DBG_MODE_MASK        (0x10000U)
#define IOCON_PIO_DBG_MODE_SHIFT       (16U)
#define IOCON_PIO_DBG_MODE(x)          (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_DBG_MODE_SHIFT)) & IOCON_PIO_DBG_MODE_MASK)


#define IOCON_CFG(dbg_func) (IOCON_PIO_FUNC(0) | IOCON_MODE_PULLDOWN |\
                             IOCON_DIGITAL_EN | IOCON_INPFILT_OFF | \
                             IOCON_PIO_DBG_FUNC(dbg_func) | IOCON_PIO_DBG_MODE(1))

#define IOCON_PIO_I2C_EGP_SHIFT        (3U)
#define IOCON_PIO_I2C_EGP_MASK         (1 << IOCON_PIO_I2C_EGP_SHIFT)
#define IOCON_PIO_I2C_ECS_SHIFT        (4U)
#define IOCON_PIO_I2C_ECS_MASK         (1 << IOCON_PIO_I2C_ECS_SHIFT)
#define IOCON_PIO_I2C_EHS_SHIFT        (5U)
#define IOCON_PIO_I2C_EHS_MASK         (1 << IOCON_PIO_I2C_EHS_SHIFT)
#define IOCON_PIO_I2C_FSEL_SHIFT       (9U)
#define IOCON_PIO_I2C_FSEL_MASK        (1 << IOCON_PIO_I2C_FSEL_SHIFT)
#define IOCON_PIO_I2C_CLAMP_SHIFT      (12U)
#define IOCON_PIO_I2C_CLAMP_MASK       (1 << IOCON_PIO_I2C_CLAMP_SHIFT)
#define IOCON_PIO_I2C_DBG_FUNC_SHIFT   (13U)
#define IOCON_PIO_I2C_DBG_FUNC_MASK    (0xf << IOCON_PIO_I2C_DBG_FUNC_SHIFT)
#define IOCON_PIO_I2C_DBG_FUNC(x)      (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_I2C_DBG_FUNC_SHIFT)) & IOCON_PIO_I2C_DBG_FUNC_MASK)
#define IOCON_PIO_I2C_DBG_MODE_SHIFT   (17U)
#define IOCON_PIO_I2C_DBG_MODE_MASK    (1 << IOCON_PIO_I2C_DBG_MODE_SHIFT)
#define IOCON_PIO_I2C_DBG_MODE(x)      (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_I2C_DBG_MODE_SHIFT)) & IOCON_PIO_I2C_DBG_MODE_MASK)


#define IOCON_I2C_CFG(dbg_func) (IOCON_PIO_FUNC(0) |\
                                 IOCON_PIO_I2C_EGP_MASK |\
                                 IOCON_PIO_I2C_ECS_MASK |\
                                 IOCON_DIGITAL_EN |\
                                 IOCON_INPFILT_OFF |\
                                 IOCON_PIO_I2C_DBG_FUNC(dbg_func)|\
                                 IOCON_PIO_I2C_DBG_MODE(1))

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief   Sets I/O Control pin mux
 * @param   base        : The base of IOCON peripheral on the chip
 * @param   port        : GPIO port to mux
 * @param   pin         : GPIO pin to mux
 * @param   modefunc    : OR'ed values of type IOCON_*
 * @return  Nothing
 */
__STATIC_INLINE void IOCON_PinMuxSet(IOCON_Type *base, uint8_t port, uint8_t pin, uint32_t modefunc)
{
    base->PIO[port][pin] = modefunc;
}

/**
 * @brief   Set all I/O Control pin muxing
 * @param   base        : The base of IOCON peripheral on the chip
 * @param   pinArray    : Pointer to array of pin mux selections
 * @param   arrayLength : Number of entries in pinArray
 * @return  Nothing
 */
__STATIC_INLINE void IOCON_SetPinMuxing(IOCON_Type *base, const iocon_group_t *pinArray, uint32_t arrayLength)
{
    uint32_t i;

    for (i = 0; i < arrayLength; i++)
    {
        IOCON_PinMuxSet(base, pinArray[i].port, pinArray[i].pin, pinArray[i].modefunc);
    }
}

/**
 * @brief   Sets I/O Control pin mux pull select
 * @param   base        : The base of IOCON peripheral on the chip
 * @param   port        : GPIO port to mux
 * @param   pin         : GPIO pin to mux
 * @param   pull_select : OR'ed values of type IOCON_*
 * @return  Nothing
 */
__STATIC_INLINE void IOCON_PullSet(IOCON_Type *base, uint8_t port, uint8_t pin, uint8_t pull_select)
{
    uint32_t reg = base->PIO[port][pin];
    reg &= ~IOCON_PIO_MODE_MASK;
    reg |= IOCON_PIO_MODE(pull_select);
    base->PIO[port][pin] = reg;
}

/**
 * @brief   Sets I/O Control pin mux pull select
 * @param   base        : The base of IOCON peripheral on the chip
 * @param   port        : GPIO port to mux
 * @param   pin         : GPIO pin to mux
 * @param   func        : Pinmux function
 * @return  Nothing
 */
__STATIC_INLINE void IOCON_FuncSet(IOCON_Type *base, uint8_t port, uint8_t pin, uint8_t func)
{
    uint32_t reg = base->PIO[port][pin];
    reg &= ~IOCON_PIO_FUNC_MASK;
    reg |= IOCON_PIO_FUNC(func);
    base->PIO[port][pin] = reg;
}
/* @} */

#if defined(__cplusplus)
}
#endif

#endif /* _FSL_IOCON_H_ */
