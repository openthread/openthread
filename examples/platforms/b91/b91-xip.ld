/********************************************************************************************************
 * @file	flash_boot.link
 *
 * @brief	This is the link file for B91
 *
 * @author	S.P
 * @date	2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *          
 *          Redistribution and use in source and binary forms, with or without
 *          modification, are permitted provided that the following conditions are met:
 *          
 *              1. Redistributions of source code must retain the above copyright
 *              notice, this list of conditions and the following disclaimer.
 *          
 *              2. Unless for usage inside a TELINK integrated circuit, redistributions 
 *              in binary form must reproduce the above copyright notice, this list of 
 *              conditions and the following disclaimer in the documentation and/or other
 *              materials provided with the distribution.
 *          
 *              3. Neither the name of TELINK, nor the names of its contributors may be 
 *              used to endorse or promote products derived from this software without 
 *              specific prior written permission.
 *          
 *              4. This software, with or without modification, must only be used with a
 *              TELINK integrated circuit. All other usages are subject to written permission
 *              from TELINK and different commercial license may apply.
 *
 *              5. Licensee shall be solely responsible for any claim to the extent arising out of or 
 *              relating to such deletion(s), modification(s) or alteration(s).
 *         
 *          THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *          ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *          WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *          DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
 *          DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *          (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *          LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *          ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *          (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *          SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *         
 *******************************************************************************************************/
 
ENTRY(_RESET_ENTRY)
SECTIONS
{
	NDS_SAG_LMA_FLASH = 0x20000000 ;
	. = 0x20000000;	
	PROVIDE (BIN_BEGIN = .);
	.vectors 		: { KEEP(*(.vectors )) }
    . = ALIGN(8);
	PROVIDE (__preinit_array_start = .);
	.preinit_array 	: AT(ALIGN(ALIGN(LOADADDR (.vectors) + SIZEOF (.vectors), ALIGNOF(.preinit_array)), 8))
		{ KEEP(*(.preinit_array )) }
	PROVIDE (__preinit_array_end = .);
	PROVIDE (__init_array_start = .);
	.init_array 	: AT(ALIGN(LOADADDR (.preinit_array) + SIZEOF (.preinit_array), ALIGNOF(.init_array)))
		{ KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.* ))) KEEP(*(.init_array )) }
	PROVIDE (__init_array_end = .);
	PROVIDE (__fini_array_start = .);
	.fini_array 	: AT(ALIGN(LOADADDR (.init_array) + SIZEOF (.init_array), ALIGNOF(.fini_array)))
		{ KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.* ))) KEEP(*(.fini_array )) }
	PROVIDE (__fini_array_end = .);
	. = 0x00000000;
	.retention_reset	: AT( ALIGN(LOADADDR (.fini_array) + SIZEOF (.fini_array),8)) 
	  	{ KEEP(*(.retention_reset )) }
	PROVIDE (_RETENTION_RESET_VMA_START = ADDR(.retention_reset));
	PROVIDE (_RETENTION_RESET_LMA_START = LOADADDR(.retention_reset));
	PROVIDE (_RETENTION_RESET_VMA_END = .); 
	/* By default,the aes_data section can only be used in the first 64K of IRAM, */
	/* please do not change the position of this section,unless you know the correct way to use */ 
	. = ALIGN(8);
	PROVIDE (_AES_VMA_START = .);
	.aes_data 	(NOLOAD)	: { KEEP(*(.aes_data )) }
	PROVIDE (_AES_VMA_END = .);
	. = ALIGN(8);
	.retention_data : AT( ALIGN(LOADADDR (.retention_reset) + SIZEOF (.retention_reset),8)) 
		{ KEEP(*(.retention_data )) }
	PROVIDE (_RETENTION_DATA_VMA_START = ADDR(.retention_data));
	PROVIDE (_RETENTION_DATA_LMA_START = LOADADDR(.retention_data));
	PROVIDE (_RETENTION_DATA_VMA_END = .); 
	
	. = ALIGN(8);
	.ram_code	: AT( ALIGN(LOADADDR (.retention_data) + SIZEOF (.retention_data),8)) 
	  	{ KEEP(*(.ram_code )) }
	PROVIDE (_RAMCODE_VMA_END = .);
	PROVIDE (_RAMCODE_VMA_START = ADDR(.ram_code));
	PROVIDE (_RAMCODE_LMA_START = LOADADDR(.ram_code));
	PROVIDE (_RAMCODE_SIZE = SIZEOF (.ram_code)); 
	
	. = ALIGN(LOADADDR (.ram_code) + SIZEOF (.ram_code), 8);
	.text 			: AT(ALIGN(LOADADDR (.ram_code) + SIZEOF (.ram_code), 8)) 
		{ *(.text .stub .text.* .gnu.linkonce.t.* ) KEEP(*(.text.*personality* )) *(.gnu.warning )  }
	.rodata 		: AT(ALIGN(LOADADDR (.text) + SIZEOF (.text), ALIGNOF(.rodata))) 
		{ *(.rodata .rodata.* .gnu.linkonce.r.* )}
	.eh_frame_hdr 	: AT(ALIGN(LOADADDR (.rodata) + SIZEOF (.rodata), ALIGNOF(.eh_frame_hdr)))
		{ *(.eh_frame_hdr ) }
	. = ALIGN(0x20);
	.eh_frame 	: AT(ALIGN(LOADADDR (.eh_frame_hdr) + SIZEOF (.eh_frame_hdr), 32))
		{ KEEP(*(.eh_frame )) }
		
	.exec.itable 	: AT(ALIGN(LOADADDR (.eh_frame) + SIZEOF (.eh_frame), ALIGNOF(.exec.itable)))
		{ KEEP(*(.exec.itable)) }
    .ctors 	: AT(ALIGN(LOADADDR (.exec.itable) + SIZEOF (.exec.itable), ALIGNOF(.ctors)))
		{ KEEP(*crtbegin*.o(.ctors)) KEEP(*(EXCLUDE_FILE (*crtend*.o) .ctors)) KEEP(*(SORT(.ctors.* ))) KEEP(*(.ctors )) }
	.dtors 	: AT(ALIGN(LOADADDR (.ctors) + SIZEOF (.ctors), ALIGNOF(.dtors)))
		{ KEEP(*crtbegin*.o(.dtors)) KEEP(*(EXCLUDE_FILE (*crtend*.o) .dtors)) KEEP(*(SORT(.dtors.* ))) KEEP(*(.dtors )) }
				
	. = 0x00080000;
	PROVIDE( __global_pointer$ = . + (4K / 2) );
	.data	: AT(ALIGN(LOADADDR (.dtors) + SIZEOF (.dtors),  ALIGNOF(.data)))
		{ *(.data .data.* .gnu.linkonce.d.* ) KEEP(*(.gnu.linkonce.d.*personality* )) SORT(CONSTRUCTORS) 
		  *(.srodata.cst16 ) *(.srodata.cst8 ) *(.srodata.cst4 ) *(.srodata.cst2 ) *(.srodata .srodata.* ) *(.sdata .sdata.* .gnu.linkonce.s.* ) *(.sdata2 .sdata2.* .gnu.linkonce.s.* )
		}
    PROVIDE (_DATA_VMA_END = .);	
    PROVIDE (_DATA_VMA_START = ADDR(.data));
	PROVIDE (_DATA_LMA_START = LOADADDR(.data));
	PROVIDE (BIN_SIZE = LOADADDR(.data) + SIZEOF(.data) - BIN_BEGIN);
	
	. = ALIGN(8);
	PROVIDE (_BSS_VMA_START = .);
	.sbss 	(NOLOAD)	: { *(.dynsbss ) *(.sbss .sbss.* .gnu.linkonce.sb.* ) *(.scommon .scommon.* ) }
	.bss 	(NOLOAD)	: { *(.dynbss ) *(.bss .bss.* .gnu.linkonce.b.* ) *(COMMON ) . = ALIGN(8); }
	PROVIDE (_BSS_VMA_END = .);
	
	. = ALIGN(8);
	/* end is the starting address of the heap, the heap grows upward */
	_end = .;
	PROVIDE (end = .);		
	PROVIDE (_STACK_TOP = 0x00a0000);
	PROVIDE (FLASH_SIZE = 0x0100000);
}

ASSERT((BIN_SIZE)<= FLASH_SIZE, "BIN FILE OVERFLOW");
