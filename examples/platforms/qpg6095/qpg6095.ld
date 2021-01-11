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

/*
 * (c) 2015, GreenPeak Technologies
 * Simple linker script.
 */





/* Memory Spaces Definitions */
MEMORY
{
  /* Ram origin start 0x1800 after real begin: this is (with some margin) the location of the windows.
     We can move this back, but this means we need to init the windows ourselves */
  SYSRAM  (rxw) : ORIGIN = 0x20008000, LENGTH = 0xA000 /* reserve 1KB for bootloader */
  UCRAM  (rxw) : ORIGIN = 0x20012000, LENGTH = 0x6000
  FLASH (rx) : ORIGIN = 0x04000000, LENGTH = 0x80000
}

SECTIONS
{
    flash_end = 0x4000000 + 0x80000;

/*
 * User license is first - only for FLASH programs.
 */
    .vpp   0x4000000 + 28 : { LONG(_ivt >> 8);  } > FLASH
    .mw    0x4000000 + 0x20 : { LONG(0x693A5C81); } > FLASH
    /* Reserve space for RT system in FLASH */
    rt_system_start_offset = 0x200;
    rt_system_start_address = ORIGIN(FLASH) + rt_system_start_offset;
    .rt_flash    rt_system_start_address : {. = ALIGN(4); *(.rt_flash);    } > FLASH
    ASSERT((rt_system_start_offset % 0x20 == 0), "RT system start should be aligned on 32 bytes")
    ASSERT((rt_system_start_address / 0x20000 == . / 0x20000 ), "RT system shall not cross 128kb boundary")

/* FIXME: temp offset to allow flash bootloader to reside in lower memory */
    .isr_vector : ALIGN(0x800) { _ivt = .; KEEP(*(.isr_vector)); } > FLASH


    .text :   { . = ALIGN(4); *(.text)   *(.text.*);  } > FLASH
    .rodata : { . = ALIGN(4); *(.rodata) *(.rodata.*);} > FLASH

    /* Start retained memory in sleep */
    _slower_retain = 0x20008000;

    /* Fixed sections */
    .mw_crc 0x20008000 : { . += 0x4; } > SYSRAM
    .crc    0x20008004 : { . += 0x4; } > SYSRAM
    .ret_hw 0x20008008 : { . += 0x1B4; } > SYSRAM
    .ret_sw 0x200081BC : { . += 0x7B; }  > SYSRAM
    ASSERT(. >= 0x200081c0, "gpmicro areas must fall into retention areas")

    /* .events and .pbm_options too big for this space */

    /* MAC_filt and BLE_mgr RAM register map - fixed size: 0x140 */
    .ram_regmap 0x20008240 : ALIGN(4) { . += 0x140; } > SYSRAM 
    /* .ram_regmap :  { . += 0x140; } > SYSRAM /* don't waste the last bytes of the 512-byte window;  */
    /* Windowed sections */
    .events :      { . += 0x10; } > SYSRAM /* 1 events (16 bytes) */
    .pbm_options : { . += 0x400; } > SYSRAM /* 32 PBM options (32 bytes) */
    /* pbm data is taken from buffers in lower_ram_retain */

    /* Data for first 16 KB region */
    .lower_ram_retain_gpmicro_accessible (NOLOAD) :  { . = ALIGN(4); *(.lower_ram_retain_gpmicro_accessible) *(.lower_ram_retain_gpmicro_accessible.*); } > SYSRAM
    ASSERT (. < 0x2000c000, ".lower_ram_retain_gpmicro_accessible outside of gpMicro address range")
    .lower_ram_retain (NOLOAD) : { . = ALIGN(4); *(.lower_ram_retain) *(.lower_ram_retain.*); } > SYSRAM

    .bss_uc :     { . = ALIGN(4); *(.bss.*)  *(.bss);} > SYSRAM

    _elower_retain = . ; /* End memory to be retained */
    .lower_ram_noretain (NOLOAD) : { . = ALIGN(4); *(.lower_ram_noretain) *(.lower_ram_noretain.*); } > SYSRAM
    _elower_ram = . ;

    /* make sure this goes into ucram */
    . = ORIGIN(UCRAM);

    PROVIDE(_shigher_retain = .);
    .higher_ram_retain (NOLOAD) : { . = ALIGN(4); *(.higher_ram_retain) *(.higher_ram_retain.*); } > UCRAM
    .bss :        { . = ALIGN(4); *(COMMON);} > UCRAM
    .data :       { . = ALIGN(4); *(.data)   *(.data.*);      } > UCRAM AT > FLASH
    PROVIDE(_ehigher_retain = .);
    .higher_ram_noretain (NOLOAD) :  { . = ALIGN(4); *(.higher_ram_noretain)   *(.higher_ram_noretain.*); } > UCRAM
    _ehigher_ram = . ;

    /* remove the debugging information from the standard libraries */
    /DISCARD/ : {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    .gpNvm flash_end - 0x1800:
    {
        gpNvm_Start = . ;
        . = 0x1800 ;
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
    _sbss_uc = ADDR(.bss_uc);
    _ebss_uc = ADDR(.bss_uc)  + ALIGN(SIZEOF(.bss_uc),  4);
    _lbss_uc = _ebss_uc - _sbss_uc;

    /* using remaining UCRAM for stack */
    _sstack = _ehigher_ram;
    _estack = ORIGIN(UCRAM) + LENGTH(UCRAM);

    /* check minimum stack size is still available */
    min_stack_size = 0x2000;
    stack_size     = _estack - _sstack;
    ASSERT(stack_size >= min_stack_size, "STACK too small")

    /* fixme: needs to be calculated basedl on size of event block */
    /* total_nr_of_events_as_an_address = SIZEOF(.events) / 16;*/
    total_nr_of_events_as_an_address = 16;

    /* needed for ram retention configuration */
    __lowerram_retain_size     = _elower_retain - _slower_retain;
    __higherram_retain_size     = SIZEOF(.higher_ram_retain);

    sw_retention_begin = ADDR(.ret_sw);
    sw_retention_end = ADDR(.ret_sw) + SIZEOF(.ret_sw);
    sw_retention_size = SIZEOF(.ret_sw);

    /* needed for .windowed regions configuration */
    ram_regmap_begin   = ADDR(.ram_regmap);
    events_begin       = ADDR(.events);
    pbm_options_begin  = ADDR(.pbm_options);

    /* check .windowed regions' offset from RAM start is divisible by 4 */
    ASSERT( ((ram_regmap_begin - 0x20008000) % 4) == 0, ".ram_regmap offset not divisible by 4")
    ASSERT( ((events_begin - 0x20008000) % 4) == 0, ".events offset not divisible by 4")
    ASSERT( ((pbm_options_begin - 0x20008000) % 4) == 0, ".pbm_options offset not divisible by 4")
}

ENTRY(reset_handler)
