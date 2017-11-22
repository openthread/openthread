/**
 ****************************************************************************************
 *
 * @file vector_table.S
 *
 * @brief Interrupt Vector Table and Patch Table
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

	.syntax	unified
	.arch	armv6-m

	.section .stack
	.align	3
	.equ	Stack_Size, __STACK_SIZE
	.globl	__StackTop
	.globl	__StackLimit
__StackLimit:
	.space	Stack_Size
	.size	__StackLimit, . - __StackLimit
__StackTop:
	.size	__StackTop, . - __StackTop

	.section .heap
	.align	3
	.equ	Heap_Size, __HEAP_SIZE
	.globl	__HeapBase
	.globl	__HeapLimit
__HeapBase:
	.if	Heap_Size
	.space	Heap_Size
	.endif
	.size	__HeapBase, . - __HeapBase
__HeapLimit:
	.size	__HeapLimit, . - __HeapLimit

	.section .isr_vector, "a"
	.align 2
	.globl	__isr_vector
__isr_vector:
	.long	__StackTop            /* Top of Stack */
	.long	Reset_Handler         /* Reset Handler */
	.long	Default_Handler       /* replaced by NMI_Handler after the ISR is copied to retRAM*/
	.long	Default_Handler       /* replaced by HardFault_Handler after the ISR is copied to retRAM */
	.long	0                     /* Reserved */
	.long	0                     /* Reserved */
	.long	0                     /* Reserved */
	.long	0                     /* Reserved */
	.long	0                     /* Reserved */
	.long	0                     /* Reserved */
	.long	0                     /* Reserved */
	.long	SVC_Handler           /* SVCall Handler */
	.long	0                     /* Reserved */
	.long	0                     /* Reserved */
	.long	PendSV_Handler        /* PendSV Handler */
	.long	SysTick_Handler       /* SysTick Handler */

	/* External interrupts */
	.long BLE_WAKEUP_LP_Handler  /*  0 */
	.long BLE_GEN_Handler        /*  1 */
	.long FTDF_WAKEUP_Handler    /*  2 */
	.long FTDF_GEN_Handler       /*  3 */
	.long RFCAL_Handler          /*  4 */
	.long COEX_Handler           /*  5 */
	.long CRYPTO_Handler         /*  6 */
	.long MRM_Handler            /*  7 */
	.long UART_Handler           /*  8 */
	.long UART2_Handler          /*  9 */
	.long I2C_Handler            /* 10 */
	.long I2C2_Handler           /* 11 */
	.long SPI_Handler            /* 12 */
	.long SPI2_Handler           /* 13 */
	.long ADC_Handler            /* 14 */
	.long KEYBRD_Handler         /* 15 */
	.long IRGEN_Handler          /* 16 */
	.long WKUP_GPIO_Handler      /* 17 */
	.long SWTIM0_Handler         /* 18 */
	.long SWTIM1_Handler         /* 19 */
	.long QUADEC_Handler         /* 20 */
	.long USB_Handler            /* 21 */
	.long PCM_Handler            /* 22 */
	.long SRC_IN_Handler         /* 23 */
	.long SRC_OUT_Handler        /* 24 */
	.long VBUS_Handler           /* 25 */
	.long DMA_Handler            /* 26 */
	.long RF_DIAG_Handler        /* 27 */
	.long TRNG_Handler           /* 28 */
	.long DCDC_Handler           /* 29 */
	.long XTAL16RDY_Handler      /* 30 */
	.long RESERVED31_Handler     /* 31 */

	.size	__isr_vector, . - __isr_vector

	/*
	 * Patch Table
	 * Notice: it crosses the 0x100 boundary
	 *         in cached mode, the first entries will be copied to
	 *         (and accessed from) RAM and others will only lie on
	 *         QSPI flash or OTP.
	 *
	 * In C declare: extern uint32_t __patch_table_start[];
	 */
	.section .patch_table, "a"
	.align 4
	.globl	__patch_table_start
__patch_table_start:
        /*
         * 28 32-bit entries, initialized to "pointer to Default_Patch_Code"
         *
         * NOTE: we assume that SYS_CTRL_REG.REMAP_INTVECT == 1
         */
        .long Default_Patch_Code_Handler        /*  0, in RAM */
        .long Default_Patch_Code_Handler        /*  1, in RAM */
        .long Default_Patch_Code_Handler        /*  2, in RAM */
        .long Default_Patch_Code_Handler        /*  3, in RAM */
        .long Default_Patch_Code_Handler        /*  4, in RAM */
        .long Default_Patch_Code_Handler        /*  5, in RAM */
        .long Default_Patch_Code_Handler        /*  6, in RAM */
        .long Default_Patch_Code_Handler        /*  7, in RAM */
        .long Default_Patch_Code_Handler        /*  8, in RAM */
        .long Default_Patch_Code_Handler        /*  9, in RAM */
        .long Default_Patch_Code_Handler        /* 10, in RAM */
        .long Default_Patch_Code_Handler        /* 11, in RAM */
        /*
         * Chip version: AD
         * Entries 12, 13, 14 and 15 are unusable and skipped in the QSPI image,
         * to account for the overhead of the QSPI header (16 bytes).
         *
         * Chip version: AA, AC, AE
         * Entries 14 and 15 are unusable and skipped in the QSPI image,
         * to account for the overhead of the QSPI header (8 bytes).
         *
         * The mkimage tool is responsible for stripping the proper entries from the QSPI image.
         */
        .long Default_Patch_Code_Handler        /* 12 */
        .long Default_Patch_Code_Handler        /* 13 */
        .long 0xDEADBEEF                        /* 14 */
        .long 0xDEADBEEF                        /* 15 */
	.size	__patch_table_start, . - __patch_table_start

__patch_table_end:

	.section .default_patch_code_handler_section, "a"
	.globl Default_Patch_Code_Handler
	.align	1
	.thumb
	.thumb_func
	.type	Default_Patch_Code_Handler, %function
Default_Patch_Code_Handler:
	bkpt	#0
	bkpt	#0

