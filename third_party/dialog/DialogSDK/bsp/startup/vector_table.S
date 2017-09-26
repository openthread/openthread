/**
 ****************************************************************************************
 *
 * @file vector_table.S
 *
 * @brief Interrupt Vector Table and Patch Table
 *
 * Copyright (C) 2015. Dialog Semiconductor, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <black.orca.support@diasemi.com> and contributors.
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

