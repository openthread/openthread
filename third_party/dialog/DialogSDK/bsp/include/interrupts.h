/**
 * \addtogroup BSP
 * \{
 * \addtogroup SYSTEM
 * \{
 * \addtogroup INTERRUPTS
 * 
 * \brief Interrupt priority configuration
 *
 * \{
 */

/**
 *****************************************************************************************
 *
 * @file interrupts.h
 *
 * @brief Interrupt priority configuration
 *
 * Copyright (c) 2016, Dialog Semiconductor
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software without 
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *****************************************************************************************
 */

#ifndef INTERRUPTS_H_
#define INTERRUPTS_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Setup interrupt priorities
 *
 * When CPU is reset all interrupts have some priority setup.
 * Reset     -3
 * NMI       -2
 * HardFault -1
 * All other interrupts have configurable priority that is set to 0.
 * If some interrupts should have priority other then default, this function should be called.
 * Argument \p prios can specify only those interrupts that need to have value other than default.
 * For memory efficiency table with priorities for each interrupt consist of interrupt priority
 * tag PRIORITY_x followed by interrupts that should have this priority, interrupts names are from
 * enum IRQn_Type.
 *
 * \note If interrupt priorities do not need to be changed dynamically at runtime, best way to
 * specify static configuration is to create table named __dialog_interrupt_priorities that will
 * be used automatically at startup.
 *
 * Most convenient way to prepare such table is to use macros like in example below:
 *
 * \code{.c}
 * INTERRUPT_PRIORITY_CONFIG_START(__dialog_interrupt_priorities)
 *      PRIORITY_0, // Start interrupts with priority 0 (highest)
 *               SVCall_IRQn,
 *               PendSV_IRQn,
 *               SysTick_IRQn,
 *      PRIORITY_1, // Start interrupts with priority 1
 *               BLE_WAKEUP_LP_IRQn,
 *               BLE_GEN_IRQn,
 *               FTDF_WAKEUP_IRQn,
 *               FTDF_GEN_IRQn,
 *      PRIORITY_2,
 *               SRC_IN_IRQn,
 *               SRC_OUT_IRQn,
 *      PRIORITY_3,
 *               UART_IRQn,
 *               UART2_IRQn,
 * INTERRUPT_PRIORITY_CONFIG_END
 * \endcode
 *
 * Table __dialog_interrupt_priorities can now be used to call this function.
 * Table can specify all interrupts or only those that need to be changed.
 *
 * \param [in] prios table with interrupts and priorities to setup
 *
 */
void set_interrupt_priorities(const int8_t prios[]);

/**
 * \brief Check whether running in interrupt context.
 *
 * \return true if the CPU is serving an interrupt.
 */
static inline bool in_interrupt(void)
{
        return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}


/**
 * \brief Default interrupt priorities table.
 *
 */
extern const int8_t __dialog_interrupt_priorities[];


#if (dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A) && (dg_configUSE_AUTO_CHIP_DETECTION != 1)
        #define LAST_IRQn PLL_LOCK_IRQn
#else
        #define LAST_IRQn RESERVED31_IRQn
#endif

/*
 * Following macros allow easy way to build table with interrupt priorities.
 * See example in set_interrupt_priorities function description.
 */
#define INTERRUPT_PRIORITY_CONFIG_START(name) const int8_t name[] = {
#define PRIORITY_0      (LAST_IRQn + 1)
#define PRIORITY_1      (LAST_IRQn + 2)
#define PRIORITY_2      (LAST_IRQn + 3)
#define PRIORITY_3      (LAST_IRQn + 4)
#define PRIORITY_TABLE_END (LAST_IRQn + 5)
#define INTERRUPT_PRIORITY_CONFIG_END PRIORITY_TABLE_END };

#ifdef __cplusplus
}
#endif

#endif /* INTERRUPTS_H_ */

/**
 * \}
 * \}
 * \}
 */
