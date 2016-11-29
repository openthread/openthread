/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
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
 */

/******************************************************************************/
/*                       Peripheral memory map                                */
/******************************************************************************/
_BOOTLOADER_SIZE = 0 ;
_QSPI_SIZE = 2048K ;   /* ld doesn't allow defines to be used for MEMORY */
_EXE_SIZE = 108K ;     /* In RAM config, reserve space for the executable... */
_TEXT_START = 0x140 ; 

MEMORY
{
  vector (rx) :        ORIGIN = 0x00000000,            LENGTH = 0x0C0 
  magic (r) :          ORIGIN = 0x000000C0,            LENGTH = 0x080   /* patch table */
  flash (rx) :         ORIGIN = 0x00000140,            LENGTH = 2048K
  rom (rx) :           ORIGIN = 0x07F00000,            LENGTH = 256K
  otp (rx) :           ORIGIN = 0x07F80000,            LENGTH = 256K
  ram (rwx) :          ORIGIN = 0x07FC0000+0x140,      LENGTH = 128K-0x140
  cacheram (rwx) :     ORIGIN = 0x07FE0000,            LENGTH = 16K
  qspi (rx) :          ORIGIN = 0x08000000,            LENGTH = 2048K
}

/**
 * GCC linker script for ARM Cortex-M microcontrollers.
 */

/*
ENTRY(_start)
*/
ENTRY(__gcc_program_start)

/*
 * Reserve memory for heap and stack. The linker will issue an error if there
 * is not enough memory.
 *
 * NOTE: The reserved heap and stack will be added to the bss column of the
 * binutils size command.
 */
_heap_size = 0x0;    /* required amount of heap  */
_stack_size = 0x0;   /* required amount of stack */

/*
 * The stack starts at the end of RAM and grows downwards. Full-descending
 * stack; decrement first, then store.
 */
__stack_start__ = ORIGIN(ram) + LENGTH(ram);
__stack_end__ = __stack_start__ - _stack_size;

EXTERN(__vector_table)


SECTIONS
{
    /* Reset and ISR vectors */
    .isr_vector :
    {
        __isr_vector_start__ = .;
	__cs3_interrupt_vector = __vector_table;
        KEEP(*(.isr_vector)) 
        ASSERT(. != __isr_vector_start__, "The .isr_vector section is empty");
    } >vector

    /* Special configuration section for some processors */
    .magic :
    {
        KEEP(*(.magic)) 
    } >magic

    /* Text section (code and read-only data) */
    .text :
    {
	. = _TEXT_START;
        . = ALIGN(4);
        _stext = .;
        *(.text*)   /* code */
        _etext = .;

	__rodata_start__ = .;
        *(.rodata*) /* read only data */

        /*
         * Note: No ARM/Thumb interworking, so no .glue_7 or .glue_7t sections
         */
        *(.ctors) 
        *(.dtors) 

        /* Static constructors and destructors */
        KEEP(*(.init))
        KEEP(*(.fini))
	
        . = ALIGN(4);
    } >flash


    /*
     * Stack unwinding and exception handling sections.
     *
     * ARM compilers emit object files with .ARM.extab and .ARM.exidx sections
     * when using C++ exceptions. Also, at least GCC emits those sections when
     * dividing large numbers (64-bit) in C. So we have to handle them.
     *
     * (ARM uses .ARM.extab and .ARM.exidx instead of the .eh_frame section
     * used on x86.)
     */
    .ARM.extab : /* exception unwinding information */
    {
        *(.ARM.extab*)
    } >flash
    .ARM.exidx : /* index entries for section unwinding */
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >flash


    /*
     * Initialized data section. This section is programmed into FLASH (LMA
     * address) and copied to RAM (VMA address) in startup code.
     */ 
     . = ALIGN(4);
     __rwdata_start__ = .;    /* LMA address is _sidata (in FLASH) */
    .data : AT(__rwdata_start__) 
    {
        . = ALIGN(4);
        _sdata = .; /* data section VMA address */
        *(text_retained)
        . = ALIGN(4);
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >ram


    /* Uninitialized data section (zeroed out by startup code) */
    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
	__end__ = .;
    } >ram


    /*
     * Reserve memory for heap and stack. The linker will issue an error if
     * there is not enough memory.
     */
    ._heap :
    {
        . = ALIGN(4);
        _sheap = .;
        . = . + _heap_size;
        . = ALIGN(4);
        _eheap = .;
    } >ram

    ._stack :
    {
        . = ALIGN(4);
        . = . + _stack_size;
        . = ALIGN(4);
    } >ram

}

/* Nice to have */
__isr_vector_size__ = SIZEOF(.isr_vector);
__text_size__ = SIZEOF(.text);
__data_size__ = SIZEOF(.data);
__bss_size__ = SIZEOF(.bss);

PROVIDE (end = .);



