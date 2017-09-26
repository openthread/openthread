/* File: startup_ARMCM0.S
 * Purpose: startup file for Cortex-M0 devices. Should use with
 *   GCC for ARM Embedded Processors
 * Version: V2.0
 * Date: 16 August 2013
 */

/* Copyright (c) 2011 - 2013 ARM LIMITED

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/
	.syntax	unified
	.arch	armv6-m
	.section text_reset
	.thumb
	.thumb_func
	.align	1
	.globl	Reset_Handler
	.type	Reset_Handler, %function
Reset_Handler:
#if (dg_configSKIP_MAGIC_CHECK_AT_START == 0)
/* If the Magic Word {0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF, 0xDEAD10CC} is found at 0x7fd0000 then the
 * execution will block for a while in order to give time to a debugger to attach. */
        ldr     r4, =0x7fd0000
        ldmia   r4!, {r0-r3}
        ldr     r4, =0xDEADBEEF
        cmp     r0, r4
        bne     .code_starts
        cmp     r1, r4
        bne     .code_starts
        cmp     r2, r4
        bne     .code_starts
        ldr     r4, =0xDEAD10CC
        cmp     r3, r4
        bne     .code_starts
        /* Enable debugger */
        ldr     r4, =0x50000012
        ldrh    r5, [r4]
        mov     r6, r5
        movs    r1, #128
        orrs    r5, r5, r1
        strh    r5, [r4]
        /* Block for 2'' */
        ldr     r0, =0x150000
.delay_loop:
        subs    r0, #1
        bgt     .delay_loop
        /* Make sure that this will happen only once! */
        ldr     r4, =0x7fd0000
        movs    r0, #0
        str     r0, [r4]
        /* Restore debugger setting */
        ldr     r4, =0x50000012
        strh    r6, [r4]

.code_starts:
#endif

/*  RAM shuffling configuration should be determined from the image in the QSPI Flash or OTP
 *  and must be applied at the beginning of the Reset Vector. The interrupt vector table must
 *  be copied to the RAM base after the shuffling is done.
 *  NOTE: Even though there is an OTP field specifically for the shuffling configuration
 *        we must bypass it here since the shuffling might change between different
 *        QSPI FLASH image versions. For OTP images RAM shuffling configuration will never change,
 * 		  but we also apply it to ignore the OTP field in any case. */
        ldr     r4, =0x50000012
        movs    r0, #0
        ldrh    r0, [r4]
        movs    r1, #0x18
        bics    r0, r0, r1
        movs    r1, #dg_configSHUFFLING_MODE<<3
        orrs    r0, r0, r1
        /* Update SYS_CTRL_REG. */
        strh    r0, [r4]

        /* Copy ISR VT from from QSPI Flash/OTP to RAM. This must be done
         * after applying shuffling mode. */
# if (dg_configIMAGE_FLASH_OFFSET == 0)
        ldr     r1, =0x8000008
# else
        ldr     r1, =(0x8000000 + dg_configIMAGE_FLASH_OFFSET)
# endif
        ldr     r2, =0x7FC0000
        ldr     r3, =0x7FC00C0
        /* Block should be a multiple of 16 to traverse correctly L_loopIV */
        subs    r3, r2
        ble     .L_loopIV_done

.L_loopIV:
        ldmia   r1!, {r4-r7}
        stmia   r2!, {r4-r7}
        subs    r3, #16
        bgt     .L_loopIV

.L_loopIV_done:

        bl      SystemInitPre
	bl	SystemInit

/*  Firstly it copies data from read only memory to RAM. There are two schemes
 *  to copy. One can copy more than one sections. Another can only copy
 *  one section.  The former scheme needs more instructions and read-only
 *  data to implement than the latter.
 *  Macro __STARTUP_COPY_MULTIPLE is used to choose between two schemes.  */
#define __STARTUP_COPY_MULTIPLE

/*  Multiple sections scheme.
 *
 *  Between symbol address __copy_table_start__ and __copy_table_end__,
 *  there are array of triplets, each of which specify:
 *    offset 0: LMA of start of a section to copy from
 *    offset 4: VMA of start of a section to copy to
 *    offset 8: size of the section to copy. Must be multiply of 4
 *
 *  All addresses must be aligned to 4 bytes boundary.
 */
        ldr     r4, =__copy_table_start__
        ldr     r0, =__copy_table_end__

.L_loop0:
        cmp     r4, r0
        bge     .L_loop0_done
        ldr     r1, [r4]
        ldr     r2, [r4, #4]
        ldr     r3, [r4, #8]
        push    {r4}

.L_loop0_0:
        subs    r3, #16
        blt     .L_loop0_0_done
        ldmia   r1!, {r4-r7}
        stmia   r2!, {r4-r7}
        b       .L_loop0_0

.L_loop0_0_done:
        pop     {r4}
        adds    r4, #12
        b       .L_loop0

.L_loop0_done:

/*  This part of work usually is done in C library startup code. Otherwise,
 *  define this macro to enable it in this startup.
 *
 *  There are two schemes too. One can clear multiple BSS sections. Another
 *  can only clear one section. The former is more size expensive than the
 *  latter.
 *
 *  Define macro __STARTUP_CLEAR_BSS_MULTIPLE to choose the former.
 *  Otherwise efine macro __STARTUP_CLEAR_BSS to choose the later.
 */
#define __STARTUP_CLEAR_BSS_MULTIPLE

/*  Multiple sections scheme.
 *
 *  Between symbol address __copy_table_start__ and __copy_table_end__,
 *  there are array of tuples specifying:
 *    offset 0: Start of a BSS section
 *    offset 4: Size of this BSS section. Must be multiply of 4
 */
        ldr     r3, =__zero_table_start__
        ldr     r0, =__zero_table_end__
        movs    r4, 0
        movs    r5, r4
        mov     r6, r4
        mov     r7, r4

.L_loop2:
        cmp     r3, r0
        bge     .L_loop2_done
        ldr     r1, [r3]
        ldr     r2, [r3, #4]

.L_loop2_0:
        subs    r2, #32 // Requires 32-byte alignment!
        blt     .L_loop2_0_done
        stmia   r1!, {r4-r7}
        stmia   r1!, {r4-r7}
        b       .L_loop2_0
.L_loop2_0_done:

        adds    r3, #8
        b       .L_loop2
.L_loop2_done:

        /* Copy the address of the retained NMI and HardFault handlers to the vector table. */
        ldr     r0, =0x7FC0008
        ldr     r1, =NMI_Handler
        str     r1, [r0, #0]
        ldr     r1, =HardFault_Handler
        str     r1, [r0, #4]

        bl SystemInitPost

	bl	main

	.pool
	.size	Reset_Handler, . - Reset_Handler

        .text
	.align	1
	.thumb_func
	.weak	Default_Handler
	.type	Default_Handler, %function
Default_Handler:
        /*
         * enable debugger:
         * CRG_TOP->SYS_CTRL_REG_b.DEBUGGER_ENABLE = 1;
         */
        movs    r1, #0x50
        lsls    r1, #24
        ldrh    r2, [r1, #0x12]
        movs    r3, #0x80
        orrs    r2, r2, r3
        strh    r2, [r1, #0x12]
	b	.
	.size	Default_Handler, . - Default_Handler

/*    Macro to define default handlers. Default handler
 *    will be weak symbol and just dead loops. They can be
 *    overwritten by other handlers */
	.macro	def_irq_handler	handler_name
	.weak	\handler_name
	.set	\handler_name, Default_Handler
	.endm

	.thumb
	.thumb_func
	.align	1
	.weak	SVC_Handler
	.type	SVC_Handler, %function
SVC_Handler:
	b	.
	.size	SVC_Handler, . - SVC_Handler

	def_irq_handler	PendSV_Handler
	def_irq_handler	SysTick_Handler
	def_irq_handler BLE_WAKEUP_LP_Handler
	def_irq_handler BLE_GEN_Handler
	def_irq_handler FTDF_WAKEUP_Handler
	def_irq_handler FTDF_GEN_Handler
	def_irq_handler RFCAL_Handler
	def_irq_handler COEX_Handler
	def_irq_handler CRYPTO_Handler
	def_irq_handler MRM_Handler
	def_irq_handler UART_Handler
	def_irq_handler UART2_Handler
	def_irq_handler I2C_Handler
	def_irq_handler I2C2_Handler
	def_irq_handler SPI_Handler
	def_irq_handler SPI2_Handler
	def_irq_handler ADC_Handler
	def_irq_handler KEYBRD_Handler
	def_irq_handler IRGEN_Handler
	def_irq_handler WKUP_GPIO_Handler
	def_irq_handler SWTIM0_Handler
	def_irq_handler SWTIM1_Handler
	def_irq_handler QUADEC_Handler
	def_irq_handler USB_Handler
	def_irq_handler PCM_Handler
	def_irq_handler SRC_IN_Handler
	def_irq_handler SRC_OUT_Handler
	def_irq_handler VBUS_Handler
	def_irq_handler DMA_Handler
	def_irq_handler RF_DIAG_Handler
	def_irq_handler TRNG_Handler
	def_irq_handler DCDC_Handler
	def_irq_handler XTAL16RDY_Handler
	def_irq_handler RESERVED31_Handler

        .section text_retained
        .align  1
        .thumb
        .thumb_func
        .globl  NMI_Handler
        .type   NMI_Handler, %function
NMI_Handler:
        ldr     r1, =NMI_HandlerC
        movs    r0, #4
        mov     r2, lr
        tst     r0, r2
        beq     NMI_stacking_using_MSP
        mrs     r0, psp
        b       stack_check
NMI_stacking_using_MSP:
        mrs     r0, msp
        b       stack_check
        .size   NMI_Handler, . - NMI_Handler

        .align  1
        .thumb
        .thumb_func
        .globl  HardFault_Handler
        .type   HardFault_Handler, %function
HardFault_Handler:
        ldr     r1, =HardFault_HandlerC
        movs    r0, #4
        mov     r2, lr
        tst     r0, r2
        beq     HF_stacking_using_MSP
        mrs     r0, psp
        b       stack_check
HF_stacking_using_MSP:
        mrs     r0, msp
stack_check:
        /* 0x7FC0000 is start of RAM */
        ldr     r2, =0x7FC0000
        cmp     r0, r2
        blt     Wrong_SP
        /* Check SYS_CTRL_REG:CACHERAM_MUX */
        ldr     r2, =0x50000012
        ldrh    r2, [r2]
        movs    r3, #1
        lsls    r3, r3, #10
        ands    r2, r2, r3
        bne     cache_is_on
        /* 0x7FE4000 marks end of RAM (assuming that the CACHE is used as RAM) */
        ldr     r2, =0x7FE4000-32
        b       check_end_of_ram
cache_is_on:
        /* 0x7FE0000 marks end of RAM */
        ldr     r2, =0x7FE0000-32
check_end_of_ram:
        cmp     r0, r2
        bgt     Wrong_SP
        bx      r1
Wrong_SP:
        /* Freeze WDog at all times */
        ldr     r0, =0x50003300
        ldr     r1, =8
        strh    r1, [r0, #0]
        /* Enable debugger at all times */
        ldr     r4, =0x50000012
        ldrh    r0, [r4, #0]
        movs    r1, #0x80
        orrs    r0, r0, r1
        /* Update SYS_CTRL_REG */
        strh    r0, [r4, #0]

        /* Wait for the WDog to hit or a debug session to start */
        b       .
        .size   HardFault_Handler, . - HardFault_Handler

	.end

