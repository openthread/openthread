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
 * (c) 2019-2020, Qorvo
 */

/* Memory Spaces Definitions */
MEMORY
{
    /* Last 32 bytes of UCRAM in use by ROM bootloader stack. */
    RAM  (rxw) : ORIGIN = 0x20008000, LENGTH = 0x17fe0
    FLASH (rx) : ORIGIN = 0x4000000, LENGTH = 0x80000
    ROM   (rx) : ORIGIN = 0x04800000, LENGTH = 0x8000
}

SECTIONS
{
    eFLASH = 0x4080000;

    sSYSRAM = 0x20008000;
    eSYSRAM = 0x20010000;
    sUCRAM  = 0x20010000;
    eUCRAM  = 0x20020000;

    /* Linkerscript only for contiguous SYS and UCRAM */
    ASSERT(eSYSRAM == sUCRAM, "Non contiguous SYS and UCRAM")

    /* 
     * User license is first - only for FLASH programs.
     *
     * BLE/MAC address entries (+0x10) in the user license skipped to
     * allow injection of custom values by programmer tool.
     */
    .native_user_license   0x4000000 + 0x10 : {
        sUserLicense = .;
        KEEP(*(.native_user_license))
        eUserLicense = .;
    } > FLASH
    ASSERT(sUserLicense < eUserLicense, "No User License")

    /* Reserve space for RT system in FLASH */
    rt_system_start_offset = 0x400;
    rt_system_start_address = ORIGIN(FLASH) + rt_system_start_offset;
    .rt_flash    rt_system_start_address : {. = ALIGN(4); *(.rt_flash);    } > FLASH
    ASSERT((rt_system_start_offset % 0x80 == 0), "RT system start should be aligned on 128 bytes")
    ASSERT((rt_system_start_address / 0x20000 == . / 0x20000 ), "RT system shall not cross 128kb boundary")
    .isr_vector : ALIGN(0x100) { KEEP(*(.isr_vector)); } > FLASH
    .text :   { . = ALIGN(4); *(.text)   *(.text.*);  } > FLASH
    .rodata : { . = ALIGN(4); *(.rodata) *(.rodata.*);} > FLASH
    .rom :    { . = ALIGN(4); *(.rom)   *(.rom.*);    } > ROM


    /* Start of memory to be retained in sleep */
    _sretain = 0x20008000;

    /* Fixed sections */
    .mw_crc 0x20008000 : { . += 0x4; } > RAM
    .crc    0x20008004 : { . += 0x4; } > RAM
    .ret_hw 0x20008008 : { . += 0x1B4; } > RAM
    .ret_sw 0x200081BC : { . += 0x7B; }  > RAM
    ASSERT(. >= 0x200081c0, "gpmicro areas must fall into retention areas")
    /*      0x20008370-0x2000837F : unused space */
    /* PUF RAM Area */
    .pkey   0x20008380 : { . += 0x80;  }  > RAM
    .pufr   0x20008400 : { . += 0x400; }  > RAM

    /* Windowed sections */
    /* ram_regmap, events, pbm options and pbm data are taken from buffers in .lower_ram_retain */

    .lower_ram_retain_gpmicro_accessible (NOLOAD) :  { . = ALIGN(4); *(.lower_ram_retain_gpmicro_accessible) *(.lower_ram_retain_gpmicro_accessible.*); } > RAM
    ASSERT (. < 0x20008000+0x4000, ".lower_ram_retain_gpmicro_accessible area outside range")
    .lower_ram_retain (NOLOAD) : { . = ALIGN(4); *(.lower_ram_retain) *(.lower_ram_retain.*); } > RAM

    _elower_ram = . ;

    /* Check regions are allocated in lower ram */
    ASSERT(_elower_ram < eSYSRAM, "SYSRAM ram full")

    .data : { . = ALIGN(4); *(.data) *(.data.*);          } > RAM AT > FLASH
    .bss :  { . = ALIGN(4); *(.bss)  *(.bss.*) *(COMMON); } > RAM

    /* End of memory to be retained */
    _eretain = . ;

    /* Scroll up to higher ram area for scratchpad variables */
    .higher_ram_noretain (NOLOAD) : {
        . = (_eretain > sUCRAM) ? ALIGN(4) : (sUCRAM - _eretain); 
        _shigher_ram_noretain = . ;
        *(.higher_ram_noretain) *(.higher_ram_noretain.*);
        _ehigher_ram_noretain = . ;
    } > RAM
    /* Check if properly allocated in UCRAM only if any variables required specific allocation. */
    ASSERT((_ehigher_ram_noretain - _shigher_ram_noretain) > 0 ? (_shigher_ram_noretain >= sUCRAM) : 1, "higher_ram_noretain not in higher ram")

    _eram = .;

   /* Remove the debugging information from the standard libraries */
    /DISCARD/ : {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    .gpNvm eFLASH - 0x4000:
    {
        gpNvm_Start = . ;
        . = 0x4000 ;
        gpNvm_End = . ;
    } > FLASH

    /* Linker Symbols */
    _native_user_license_vpp    = ADDR(.isr_vector) >> 8;
    _loaded_user_license_vpp    = ADDR(.isr_vector) >> 8;
    _etext  = ADDR(.text) + SIZEOF(.text);
    _sidata = LOADADDR(.data);
    _sdata  = ADDR(.data);
    _edata  = ADDR(.data) + ALIGN(SIZEOF(.data), 4);
    _ldata  = _edata - _sdata;
    _sbss   = ADDR(.bss);
    _ebss   = ADDR(.bss)  + ALIGN(SIZEOF(.bss),  4);
    _lbss   = _ebss - _sbss;

    /* using remaining RAM for stack */
    _sstack = _eram;
    _estack = ORIGIN(RAM) + LENGTH(RAM) - SIZEOF(.higher_ram_noretain);

    /* check minimum stack size is still available */
    min_stack_size = 0x800;
    stack_size     = _estack - _sstack;
    ASSERT(stack_size >= min_stack_size, "STACK too small")

    /* needed for ram retention configuration */
    __lowerram_retain_size     = _eretain < eSYSRAM ? _eretain - _sretain : eSYSRAM - _sretain;
    __higherram_retain_size    = _eretain < eSYSRAM ? 0 : _eretain - eSYSRAM;

    sw_retention_begin = ADDR(.ret_sw);
    sw_retention_end = ADDR(.ret_sw) + SIZEOF(.ret_sw);
    sw_retention_size = SIZEOF(.ret_sw);
}

ENTRY(reset_handler)
