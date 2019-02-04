/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

/* Memory Spaces Definitions */
MEMORY
{
  SYSRAM  (rxw) : ORIGIN = 0x20008000, LENGTH = 0x8000
  UCRAM  (rxw) : ORIGIN = 0x20010000, LENGTH = 0x8000
  FLASH (rx) : ORIGIN = 0x04000000, LENGTH = 0x80000
}

SECTIONS
{
    flash_end = 0x4000000 + 0x80000;
    .vpp   0x0400001C : { LONG(_ivt >> 8);  } > FLASH
    .mw    0x04000020 : { LONG(0x693A5C81); } > FLASH
    rts_start_offset = 0x200;
    rts_start_address = ORIGIN(FLASH) + rts_start_offset;
    .rt    rts_start_address : {. = ALIGN(4); *(.rt_flash);    } > FLASH
    .isr_vector : ALIGN(0x800) { _ivt = .; KEEP(*(.isr_vector)); } > FLASH

    __exidx_start = .;
    .text :       { . = ALIGN(4); *(.text)   *(.text.*);      } > FLASH
    .rodata :     { . = ALIGN(4); *(.rodata) *(.rodata.*);    } > FLASH
    __exidx_end = .;

    _slower_retain = ORIGIN(SYSRAM) ; /* Start retained memory in sleep */

    /* Fixed sections */
    .mw_crc 0x20008000 : { . += 0x4; } > SYSRAM
    .crc    0x20008004 : { . += 0x4; } > SYSRAM
    .ret_hw 0x20008008 : { . += 0x138; } > SYSRAM
    .ret_sw 0x20008140 : { . += 0x80; }  > SYSRAM

    /* Windowed sections */
    .ram_regmap :  { . += 0x140; } > SYSRAM
    .events :      { . += 0x10 ; } > SYSRAM
    .pbm_options : { . += 0x400 ; } > SYSRAM

    .lower_ram_retain_gpmicro_accessible (NOLOAD) :  { . = ALIGN(4); *(.lower_ram_retain_gpmicro_accessible)   *(.lower_ram_retain_gpmicro_accessible.*); } > SYSRAM
    .lower_ram_retain (NOLOAD) :  { . = ALIGN(4); *(.lower_ram_retain)   *(.lower_ram_retain.*); } > SYSRAM

    .bss :        { . = ALIGN(4); *(.bss) *(COMMON);} > SYSRAM

    _elower_retain = . ; /* End memory to be retained */
    .lower_ram_noretain (NOLOAD) :  { . = ALIGN(4); *(.lower_ram_noretain)   *(.lower_ram_noretain.*); } > SYSRAM
    _elower_ram = . ;

    . = ORIGIN(UCRAM);

    PROVIDE(_shigher_retain = .);
    .higher_ram_retain (NOLOAD) :  { . = ALIGN(4); *(.higher_ram_retain)   *(.higher_ram_retain.*); } > UCRAM
    PROVIDE(_ehigher_retain = .);
    .higher_ram_noretain (NOLOAD) :  { . = ALIGN(4); *(.higher_ram_noretain)   *(.higher_ram_noretain.*); } > UCRAM
    .data :       { . = ALIGN(4); *(.data)   *(.data.*);      } > UCRAM AT > FLASH
    .bss_uc :     { . = ALIGN(4); *(.bss.*) ;} > UCRAM
    _ehigher_ram = . ;

    /* remove the debugging information from the standard libraries */
    /DISCARD/ : {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    .gpNvm flash_end - 0x4000:
    {
        gpNvm_Start = . ;
        . = 0x4000 ;
        gpNvm_End = . ;
    } > FLASH

    _etext  = ADDR(.text) + SIZEOF(.text);
    _sidata = LOADADDR(.data);
    _sdata  = ADDR(.data);
    _edata  = ADDR(.data) + ALIGN(SIZEOF(.data), 4);
    _ldata  = _edata - _sdata;
    _sbss   = ADDR(.bss);
    _ebss   = ADDR(.bss)  + ALIGN(SIZEOF(.bss),  4);
    _lbss   = _ebss - _sbss;
    /* use UCRAM for stack */
    _sstack = _ehigher_ram;
    _estack = ORIGIN(UCRAM) + LENGTH(UCRAM);
    _sbss_uc = ADDR(.bss_uc);
    _ebss_uc = ADDR(.bss_uc)  + ALIGN(SIZEOF(.bss_uc),  4);
    _lbss_uc = _ebss_uc - _sbss_uc;

    __configured_stack_size = 0x1200;
    __stack_size     = _estack - _sstack;
    ASSERT(__stack_size >= __configured_stack_size, "STACK too small")

    _elower_ram = ADDR(.lower_ram_retain) + ALIGN(SIZEOF(.lower_ram_retain),  4);
    __lowerram_retain_size     = _elower_retain - _slower_retain;

    __higherram_retain_size     = SIZEOF(.higher_ram_retain);

    sw_retention_begin = ADDR(.ret_sw);
    sw_retention_end = ADDR(.ret_sw) + SIZEOF(.ret_sw);
    ram_regmap_begin   = ADDR(.ram_regmap);
    events_begin       = ADDR(.events);
    pbm_options_begin  = ADDR(.pbm_options);

    total_nr_of_events_as_an_address = SIZEOF(.events) / 16;
}

ENTRY(reset_handler)

