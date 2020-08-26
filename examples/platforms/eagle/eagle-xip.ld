
ENTRY(_RESET_ENTRY)
SECTIONS
{
	NDS_SAG_LMA_FLASH = 0x20000000 ;
	
	. = 0x20000000;
	
	PROVIDE (BIN_BEGIN = .);
	
	.vectors 	: { KEEP(*(.vectors )) }
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
	PROVIDE (_RAMCODE_VMA_START = .);
	PROVIDE (_RAMCODE_LMA_START = ALIGN(LOADADDR (.fini_array) + SIZEOF (.fini_array),8));
	.ram_code	: AT(ALIGN(LOADADDR (.fini_array) + SIZEOF (.fini_array),8)) 
	  	{ KEEP(*(.ram_code )) }
	PROVIDE (_RAMCODE_VMA_END = ALIGN(.,8)); 
	
	. = ALIGN(LOADADDR (.ram_code) + SIZEOF (.ram_code),8);
	.text 		: AT(ALIGN(LOADADDR (.ram_code) + SIZEOF (.ram_code),8)) 
		{ *(.text .stub .text.* .gnu.linkonce.t.* ) KEEP(*(.text.*personality* )) *(.gnu.warning ) . = ALIGN(4); }
	
	.rodata 	: AT(ALIGN(LOADADDR (.text) + SIZEOF (.text), ALIGNOF(.rodata))) 
		{ *(.rodata .rodata.* .gnu.linkonce.r.* )}
		
	.eh_frame_hdr 	: AT(ALIGN(LOADADDR (.rodata) + SIZEOF (.rodata), ALIGNOF(.eh_frame_hdr)))
		{ *(.eh_frame_hdr ) }
	. = ALIGN(0x20);
	.eh_frame 	: AT(ALIGN(LOADADDR (.eh_frame_hdr) + SIZEOF (.eh_frame_hdr),  ALIGNOF(.eh_frame)))
		{ KEEP(*(.eh_frame )) }
		
	.exec.itable 	: AT(ALIGN(LOADADDR (.eh_frame) + SIZEOF (.eh_frame), ALIGNOF(.exec.itable)))
		{ KEEP(*(.exec.itable)) }
    
	.ctors 	: AT(ALIGN(LOADADDR (.exec.itable) + SIZEOF (.exec.itable), ALIGNOF(.ctors)))
		{ KEEP(*crtbegin*.o(.ctors)) KEEP(*(EXCLUDE_FILE (*crtend*.o) .ctors)) KEEP(*(SORT(.ctors.* ))) KEEP(*(.ctors )) }
	.dtors 	: AT(ALIGN(LOADADDR (.ctors) + SIZEOF (.ctors), ALIGNOF(.dtors)))
		{ KEEP(*crtbegin*.o(.dtors)) KEEP(*(EXCLUDE_FILE (*crtend*.o) .dtors)) KEEP(*(SORT(.dtors.* ))) KEEP(*(.dtors )) }
				
	. = 0x00080000;
	PROVIDE (_DATA_VMA_START = .);
	PROVIDE (_DATA_LMA_START = ALIGN(LOADADDR (.dtors) + SIZEOF (.dtors),  ALIGNOF(.sdata)));
	PROVIDE( __global_pointer$ = . + (4K / 2) );
		.sdata 	: AT(ALIGN(LOADADDR (.dtors) + SIZEOF (.dtors), ALIGNOF(.sdata)))
		{ *(.srodata.cst16 ) *(.srodata.cst8 ) *(.srodata.cst4 ) *(.srodata.cst2 ) *(.srodata .srodata.* ) *(.sdata .sdata.* .gnu.linkonce.s.* ) *(.sdata2 .sdata2.* .gnu.linkonce.s.* ) }

	.data 	: AT(ALIGN(LOADADDR (.sdata) + SIZEOF (.sdata), ALIGNOF(.data)))
		{ *(.data .data.* .gnu.linkonce.d.* ) KEEP(*(.gnu.linkonce.d.*personality* )) SORT(CONSTRUCTORS) }
	PROVIDE (_DATA_VMA_END = .);
	
	PROVIDE (BIN_SIZE = LOADADDR(.data) + SIZEOF(.data) - BIN_BEGIN);
	
	. = ALIGN(8);
	PROVIDE (_BSS_VMA_START = .);
	.sbss 	(NOLOAD)	: { *(.dynsbss ) *(.sbss .sbss.* .gnu.linkonce.sb.* ) *(.scommon .scommon.* ) }
	.bss 	(NOLOAD)	: { *(.dynbss ) *(.bss .bss.* .gnu.linkonce.b.* ) *(COMMON ) . = ALIGN(8); }
	PROVIDE (_BSS_VMA_END = .);
    . = ALIGN(8);
	_end = .;
	PROVIDE (end = .);
			
	PROVIDE (_STACK_TOP = 0x00a0000);
	PROVIDE (FLASH_SIZE = 0x0080000);
}

ASSERT((BIN_SIZE)<= FLASH_SIZE, "BIN FILE OVERFLOW");