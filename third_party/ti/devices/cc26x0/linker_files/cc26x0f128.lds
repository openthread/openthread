/*
 @file      cc26x0f128.lds
 @brief     CC26x0F128 linker configuration file for GNU compiler.

 Revised    $Date: 2017-02-03 19:14:38 +0100 (Fri, 03 Feb 2017) $
 Revision   $Revision: 17632 $

 This file is auto-generated.

  Copyright (C) 2017 Texas Instruments Incorporated - http:www.ti.com/


  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* Entry Point */
ENTRY( ResetISR )

/* System memory map */
MEMORY
{
    /* Application is stored in and executes from internal flash */
    FLASH (RX) : ORIGIN = 0x0, LENGTH = 0x1FFA8
    /* Customer Configuration Area (CCFG) */
    FLASH_CCFG (RX) : ORIGIN = 0x1FFA8, LENGTH = 88
    /* Application uses internal RAM for data */
    SRAM (RWX) : ORIGIN = 0x20000000, LENGTH = 0x5000
    /* Application can use GPRAM region as RAM if cache is disabled in the CCFG
       (DEFAULT_CCFG_SIZE_AND_DIS_FLAGS.SET_CCFG_SIZE_AND_DIS_FLAGS_DIS_GPRAM = 0) */
    GPRAM (RWX) : ORIGIN = 0x11000000, LENGTH = 0x2000
}

/*. Highest address of the stack. Used in startup file .*/
_estack = ORIGIN(SRAM) + LENGTH(SRAM); /*end of SRAM .*/

/*. Generate a link error if heap and stack does not fit into RAM .*/
_Min_Heap_Size = 0;
_Min_Stack_Size = 0x100;


/* Section allocation in memory */
SECTIONS
{
    .text :
    {
        _text = .;
        KEEP(*(.vectors))
        *(.text*)
        *(.rodata*)
        *(.init)
        *(.fini*)
        *(.eh_frame*)
        _etext = .;
    } > FLASH = 0

   .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .data :
    {
        _data = .;
        *(vtable)
        *(.data*)
        _edata = .;
    } > SRAM AT > FLASH
    _ldata = LOADADDR(.data);

    .bss :
    {
        __bss_start__ = .;
        _bss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
        __bss_end__ = .;
    } > SRAM

    .ccfg :
    {
        KEEP(*(.ccfg));
    } > FLASH_CCFG

    /* User_heap_stack section, used to check that there is enough SRAM left */
    ._user_heap_stack :
    {
      . = ALIGN(4);
      end = .;
      . = . + _Min_Heap_Size;
      . = . + _Min_Stack_Size;
      . = ALIGN(4);
    } > SRAM

    .gpram :
    {
    } > GPRAM
}
