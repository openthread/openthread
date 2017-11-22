/**
 * \addtogroup BSP
 * \{
 * \addtogroup SYSTEM
 * \{
 * \addtogroup TCS_HANDLER
 * 
 * \brief TCS Handler
 *
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file sys_tcs.h
 *
 * @brief TCS Handler header file.
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
 ****************************************************************************************
 */

#ifndef SYS_TCS_H_
#define SYS_TCS_H_

#include <stdint.h>
#include "sdk_defs.h"

typedef enum sys_tcs_area_type {
#ifdef CONFIG_USE_BLE
        tcs_ble,
#endif
#ifdef CONFIG_USE_FTDF
        tcs_ftdf,
#endif
        tcs_radio,
        tcs_charger,
        tcs_audio,
        tcs_system, /* Last area. Add any missing areas above this! */
} sys_tcs_area_t;

/**
 * \brief Flag to check whether the TCS is written or not.
 * \details true, the TCS is written (the BANDGAP_REG value exists in the TCS)
 *          false, the TCS is empty
 */
extern bool sys_tcs_is_calibrated_chip;

/**
 * \brief The XTAL16M settling time (in TCS).
 * \details XTAL32K, the settling time is expressed in clock cycles
 *          RCX, not applicable; the SDK hard-coded value is used
 *          If it is zero, the hard-coded value (dg_configXTAL16_SETTLE_TIME) is applied.
 */
extern uint16_t sys_tcs_xtal16m_settling_time;

/**
 * \brief Initialize the variables used from the TCS handling module.
 *
 */
void sys_tcs_init(void);

/**
 * \brief Store TCS <address, value> pair in the Global TCS array if it points to a register
 *        that is not in the AON power dowmain or is not retained, else it applies the vaule to
 *        the register.
 *
 * \param [in] address the address of the register
 * \param [in] value the value for this register
 *
 * \return true if the chip is calibrated (the BANDGAP setting has been applied), else false.
 *
 * \warning When this function is called, the RC16 must have been set as the system clock!
 *
 */
bool sys_tcs_store_pair(uint32_t address, uint32_t value);

/**
 * \brief Sort the registers of the registers in the Memory Map of the chip to a different "classes"
 *        in the TCS array.
 *
 */
void sys_tcs_sort_array(void);

/**
 * \brief Apply the <address, value> pairs located in a "class" of the TCS array. This is done only
 *        for calibrated chips!
 *
 */
void sys_tcs_apply(sys_tcs_area_t area);

#endif /* SYS_TCS_H_ */

/**
\}
\}
\}
*/
