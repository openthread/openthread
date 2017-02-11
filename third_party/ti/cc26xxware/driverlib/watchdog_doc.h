/******************************************************************************
*  Filename:       watchdog_doc.h
*  Revised:        2016-03-30 13:03:59 +0200 (Wed, 30 Mar 2016)
*  Revision:       45971
*
*  Copyright (c) 2015 - 2016, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/
//! \addtogroup wdt_api
//! @{
//! \section sec_wdt Introduction
//!
//! The watchdog timer (WDT) is used to regain control when the system has failed due to unexpected
//! software behavior. The WDT can generate a non-maskable interrupt (NMI), a regular interrupt, or a
//! reset if the software fails to reload the watchdog before it times out.
//!
//! WDT has the following features:
//! - 32-bit down counter with a programmable load register.
//! - Programmable interrupt generation logic with interrupt masking and optional NMI function.
//! - Optional reset generation.
//! - Register protection from runaway software (lock).
//! - User-enabled stalling when the system CPU asserts the CPU Halt flag during debug.
//!
//! After the first time-out event, the 32-bit counter is re-loaded with the value of the Load register,
//! and the timer resumes counting down from that value.
//! If the timer counts down to its zero state again before the first time-out interrupt is cleared,
//! and the reset signal has been enabled, the WDT asserts its reset signal to the system. If the interrupt
//! is cleared before the 32-bit counter reaches its second time-out, the 32-bit counter is loaded with the
//! value in the Load register, and counting resumes from that value.
//!
//! If Load register is written with a new value while the WDT counter is counting, then the counter is
//! loaded with the new value and continues counting.
//! Writing to the Load register does not clear an active interrupt. An interrupt must be explicitly cleared
//! by clearing the interrupt.
//!
//! The WDT counter runs at system HF clock divided by 32; however, when in powerdown it runs at
//! LF clock (32 kHz) - if the LF clock to the MCU domain has been enabled.
//!
//! \section sec_wdt_api API
//!
//! The API functions can be grouped like this:
//!
//! Watchdog configuration:
//! - \ref WatchdogIntTypeSet()
//! - \ref WatchdogResetEnable()
//! - \ref WatchdogResetDisable()
//! - \ref WatchdogReloadSet()
//! - \ref WatchdogEnable()
//!
//! Status:
//! - \ref WatchdogRunning()
//! - \ref WatchdogValueGet()
//! - \ref WatchdogReloadGet()
//! - \ref WatchdogIntStatus()
//!
//! Interrupt configuration:
//! - \ref WatchdogIntEnable()
//! - \ref WatchdogIntClear()
//! - \ref WatchdogIntRegister()
//! - \ref WatchdogIntUnregister()
//!
//! Register protection:
//! - \ref WatchdogLock()
//! - \ref WatchdogLockState()
//! - \ref WatchdogUnlock()
//!
//! Stall configuration for debugging:
//! - \ref WatchdogStallDisable()
//! - \ref WatchdogStallEnable()
//!
//! @}
