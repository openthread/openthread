/******************************************************************************
*  Filename:       interrupt_doc.h
*  Revised:        2016-03-30 13:03:59 +0200 (Wed, 30 Mar 2016)
*  Revision:       45971
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
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
//! \addtogroup interrupt_api
//! @{
//! \section sec_interrupt Introduction
//!
//! The interrupt controller API provides a set of functions for dealing with the
//! nested vectored interrupt controller (NVIC). Functions are provided to enable
//! and disable interrupts, register interrupt handlers, and set the priority of
//! interrupts.
//!
//! The event sources that trigger the interrupt lines in the NVIC are controlled by
//! the MCU event fabric. All event sources are statically connected to the NVIC interrupt lines
//! except one which is programmable. For more information about the MCU event fabric, see the
//! [MCU event fabric API](\ref event_api).
//!
//! The NVIC provides global interrupt masking, prioritization, and handler
//! dispatching. Devices within the \ti_device family support 34
//! interrupt lines and 8 priority levels from 0 to 7 with 0 being the highest priority.
//! Individual interrupt sources can
//! be masked, and the processor interrupt can be globally masked as well (without
//! affecting the individual source masks).
//!
//! The NVIC is tightly coupled with the System CPU. When the
//! processor responds to an interrupt, the NVIC supplies the address of the
//! function to handle the interrupt directly to the processor.
//!
//! Each interrupt source can be individually enabled and disabled through:
//! - \ref IntEnable()
//! - \ref IntDisable()
//!
//! The global processor interrupt can be enabled and disabled with the following functions:
//! - \ref IntMasterEnable()
//! - \ref IntMasterDisable()
//!
//! This does not affect the individual
//! interrupt enable states. Masking of the processor interrupt can be used as
//! a simple critical section (only an NMI can interrupt the processor while the
//! processor interrupt is disabled), although masking the processor interrupt can
//! have adverse effects on the interrupt response time.
//!
//! It is possible to access the NVIC to see if any interrupts are pending and manually
//! clear pending interrupts which have not yet been serviced or set a specific interrupt as
//! pending to be handled based on its priority. Pending interrupts are cleared automatically
//! when the interrupt is accepted and becomes active (being executed). The functions to read,
//! clear, and set pending interrupts are:
//! - \ref IntPendGet()
//! - \ref IntPendClear()
//! - \ref IntPendSet()
//!
//! The interrupt prioritization in the NVIC allows handling of higher priority interrupts
//! before lower priority interrupts, as well as allowing preemption of
//! lower priority interrupt handlers by higher priority interrupts.
//! The priority of each interrupt source can be set and examined using:
//! - \ref IntPrioritySet()
//! - \ref IntPriorityGet()
//!
//! Interrupts can be masked based on their priority such that interrupts with the same or lower
//! priority than the mask are effectively disabled. This can be configured with:
//! - \ref IntPriorityMaskSet()
//! - \ref IntPriorityMaskGet()
//!
//! Subprioritization is also possible; instead of having 3 bits of preemptable
//! prioritization (8 levels), the NVIC can be configured for 3 - M bits of
//! preemptable prioritization and M bits of subpriority. In this scheme, two
//! interrupts with the same preemptable prioritization but different subpriorities
//! do not cause a preemption; instead tail chaining is used to process
//! the two interrupts back-to-back.
//! If two interrupts with the same priority (and subpriority if so configured) are
//! asserted at the same time, the one with the lower interrupt number is
//! processed first. Subprioritization is handled by:
//! - \ref IntPriorityGroupingSet()
//! - \ref IntPriorityGroupingGet()
//!
//! Interrupt handlers can be configured in one of two ways; statically at compile
//! time or dynamically at run time. Static configuration of interrupt handlers is
//! accomplished by editing the interrupt handler table in the startup code of the application.
//! When statically configured, the interrupts must be explicitly
//! enabled in the NVIC through \ref IntEnable() before the processor can respond to the
//! interrupt (in addition to any interrupt enabling required within the peripheral).
//! Statically configuring the interrupt table provides the fastest
//! interrupt response time because the stacking operation (a write to SRAM) can be
//! performed in parallel with the interrupt handler table fetch (a read from
//! Flash), as well as the prefetch of the interrupt handler (assuming it is
//! also in Flash).
//!
//! Alternatively, interrupts can be configured at runtime using (or the corresponding in each individual module API):
//! - \ref IntRegister()
//! - \ref IntUnregister()
//!
//! Registering an interrupt handler is a simple matter of inserting the handler address into the
//! table. By default, the table is filled with pointers to an internal handler
//! that loops forever; it is an error for an interrupt to occur when there is no
//! interrupt handler registered to process it. Therefore, interrupt sources
//! should not be enabled before a handler has been registered, and interrupt
//! sources should be disabled before a handler is unregistered.
//! When using \ref IntRegister(), the interrupt
//! must also be enabled as before; when using the function in each individual
//! function API, \ref IntEnable() is called by the driver and does not need to be called by
//! the application.
//! Run-time configuration of interrupts adds a small latency to the interrupt
//! response time because the stacking operation (a write to SRAM) and the
//! interrupt handler table fetch (a read from SRAM) must be performed
//! sequentially.
//!
//! Run-time configuration of interrupt handlers requires that the interrupt
//! handler table is placed on a 1-kB boundary in SRAM (typically, this is
//! at the beginning of SRAM). Failure to do so results in an incorrect vector
//! address being fetched in response to an interrupt. The vector table is in a
//! section called \ti_code{vtable} and should be placed appropriately with a linker
//! script.
//!
//! @}
