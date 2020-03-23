/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_WTIMER_H_
#define _FSL_WTIMER_H_

#include "fsl_common.h"

/*!
 * @addtogroup wtimer
 * @{
 */

/*! @file */

/**
 * Wake timers provide wakeup capabilities in sleep modes where 32KHz clock is kept active.
 * Wake timer 0 is a 48bit based counter while wake timer 1 is 32bit based counter.
 * A special API functions WTIMER_StartTimerLarge(0 and WTIMER_StartTimerLarge() are provided to access the 48bit
 * counter. The Wake timer 1 is to be used bu the PWRM framework. It shall not be used by the Application directly. API
 * provides the capability to enable and disable interrupts. The application shall implement the Wake timer ISR on its
 * side. The wake timer ISR prototypes are : void WAKE_UP_TIMER0_IRQHandler(void); and void
 * WAKE_UP_TIMER1_IRQHandler(void); The Application shall correctly the 32KHz source amoung the FRO32 or Crystal 32KHz
 * using CLOCK_EnableClock() API in fsl_clock.h The APi provides the capability to calibrate the 32KHz clock versus a
 * high reference clock (32MHz crystal).
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_WTIMER_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0 */
/*@}*/

typedef enum
{
    WTIMER_TIMER0_ID = 0,
    WTIMER_TIMER1_ID = 1,
} WTIMER_timer_id_t;

typedef enum
{
    WTIMER_STATUS_NOT_RUNNING = 0,
    WTIMER_STATUS_RUNNING     = 1,
    WTIMER_STATUS_EXPIRED     = 2,
} WTIMER_status_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Enable the clocks to the peripheral (functional clock and AHB clock)
 *
 * @note This function does not reset the wake timer peripheral. Wake timer reset is done in PWRM_vColdStart() from the
 * PWRM framework module if integrated If PWRM framework module is integrated, WTIMER_Init() is called in PWRM_vInit()
 * for power modes with Oscillator ON.
 *
 */
void WTIMER_Init(void);

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Disable the clocks to the peripheral (functional clock and AHB clock)
 *
 * @note This function does not reset the wake timer peripheral.
 *
 */
void WTIMER_DeInit(void);

/*!
 * @brief Enable the selected Timer interrupts.
 * The application shall implement the Wake timer ISR
 *
 * @param timer_id   Wtimer Id
 */
void WTIMER_EnableInterrupts(WTIMER_timer_id_t timer_id);

#ifdef NOT_IMPLEMENTED_YET
/*!
 * @brief Disable the selected Timer interrupts.
 * Interrupts are disabled by default. The API shall be called if the interrupt was enabled before.
 *
 * @param timer_id   Wtimer Id
 */
void WTIMER_DisableInterrupts(WTIMER_timer_id_t timer_id);
#endif

/*!
 * @brief Gets the Timer status flags.
 *
 * @param timer_id   Wtimer Id
 *
 * @return The status flags.
 */
WTIMER_status_t WTIMER_GetStatusFlags(WTIMER_timer_id_t timer_id);

/*!
 * @brief Clears the Timer status flags if expired and clear the pendng interrupt if active
 * it needs to be called in ISR
 *
 * @param timer_id   Wtimer Id
 */
void WTIMER_ClearStatusFlags(WTIMER_timer_id_t timer_id);

/*!
 * @brief Starts the Timer counter.
 * The function performs:
 * -stop the timer if running, clear the status and interrupt flag if set (WTIMER_ClearStatusFlags())
 * -set the counter value
 * -start the timer
 *
 * @param timer_id   Wtimer Id
 * @param count      number of 32KHz clock periods before expiration
 */
void WTIMER_StartTimer(WTIMER_timer_id_t timer_id, uint32_t count);

/*!
 * @brief Stops the Timer counter.
 *
 * @param timer_id   Wtimer Id
 */
void WTIMER_StopTimer(WTIMER_timer_id_t timer_id);

/*!
 * @brief Calibrate the 32KHz clock to be used by the wake timer versus the 32MHz crystal clock source
 * The Applicaton shall switches OFF the 32MHz clock if no longer used by the chip using CLOCK_DisableClock() in
 * fsl_clock.h
 *
 * @return  32KHz clock frequency (number of 32KHz clock in one sec) - expect to have 32768
 */
uint32_t WTIMER_CalibrateTimer(void);

/*!
 * @brief Read the LSB counter of the wake timer
 * This API is unsafe. If the counter has just been started, the counter value may not be
 * up to date until the next 32KHz clock edge.
 * Use WTIMER_ReadTimerSafe() instead
 *
 * @param timer_id   Wtimer Id
 * @return  counter value - number of ticks before expiration if running
 */
uint32_t WTIMER_ReadTimer(WTIMER_timer_id_t timer_id);

/*!
 * @brief Read the LSB counter of the wake timer
 * API checks the next counter update (next 32KHz clock edge) so the value is uptodate
 * Important note : The counter shall be running otherwise, the API gets locked and never return
 *
 * @param timer_id   Wtimer Id
 * @return  32KHz clock frequency (number of 32KHz clock in one sec) - expect to have 32768
 */
uint32_t WTIMER_ReadTimerSafe(WTIMER_timer_id_t timer_id);

#ifdef NOT_IMPLEMENTED_YET
/*!
 * @brief Starts the Timer counter.
 *
 * @param timer_id   Wtimer Id
 * @param count      number of 32KHz clock periods before expiration
 */
void WTIMER_StartTimerLarge(WTIMER_timer_id_t timer_id, uint64_t count);

/*!
 * @brief Read the LSB + MSB counter of the 48bit wake timer, Read the LSB counter for 32bit wale timer
 *
 *
 * @param timer_id   Wtimer Id
 * @return wake timer counter
 */
uint64_t WTIMER_ReadTimerLarge(WTIMER_timer_id_t timer_id);
#endif

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_WTIMER_H_ */
