/******************************************************************************
*  Filename:       standalone_rom_crypto.h
*  Revised:        $Date: 2015-03-19 15:37:11 +0100 (to, 19 mar 2015) $
*  Revision:       $Revision: 43051 $
*
*  Description:    Provide a simple initialization the common ROM-RAM system
*                  needed prior to using the ROM Encryption functions
*                  (ref. <driverlib/RomEncryption.h>)
*
*  Copyright (c) 2015, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
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
*
******************************************************************************/

#include <stdint.h>
#include <inc/hw_chip_def.h>

// Note: Contents of the HelpFuncTable[] differ between the CC26xx and CC13xx chip family
uint32_t HelpFuncTable[] = {
   0         , // [0] => offset 0x00
   0         , // [1] => offset 0x04
   0         , // [2] => offset 0x08
   0x100046f7, // [3] => offset 0x0C
   0         , // [4] => offset 0x10
   0           // [5] => offset 0x14
};


uint32_t Rom2RomPatchTable[] = {
   0x10016975, // [32] => offset 0x80
   0x10016979, // [33] => offset 0x84
   0x10016985, // [34] => offset 0x88
   0x10016a99, // [35] => offset 0x8C
   0x10016af5, // [36] => offset 0x90
   0x10016b79, // [37] => offset 0x94
   0x10016c29, // [38] => offset 0x98
   0x10016c45, // [39] => offset 0x9C
   0x10016c79, // [40] => offset 0xA0
   0x10016d05, // [41] => offset 0xA4
   0x10016d29, // [42] => offset 0xA8
   0x10016da1, // [43] => offset 0xAC
   0x10016dc9, // [44] => offset 0xB0
   0x10016e8d, // [45] => offset 0xB4
   0x10016ea1, // [46] => offset 0xB8
   0x10016ed5, // [47] => offset 0xBC
   0x10017365, // [48] => offset 0xC0
   0x100174fd, // [49] => offset 0xC4
   0x1001767d, // [50] => offset 0xC8
   0x10017839, // [51] => offset 0xCC
   0x10017895, // [52] => offset 0xD0
   0x100178f5, // [53] => offset 0xD4
   0x10017771, // [54] => offset 0xD8
   0x100175ed, // [55] => offset 0xDC
   0x10018b09, // [56] => offset 0xE0
   0x10018a99, // [57] => offset 0xE4
   0x10018ac5, // [58] => offset 0xE8
   0x10018a19, // [59] => offset 0xEC
   0x10018a35  // [60] => offset 0xF0
};

uint32_t commonROMScratchArea[ 53 ] @0x20004F2C;

extern void StandaloneRomCryptoInit( void )
{
   for ( int i = 0 ; i < ( sizeof( commonROMScratchArea ) / sizeof( uint32_t )) ; i++ ) {
      commonROMScratchArea[ i ] = 0;
   }
   commonROMScratchArea[ 3 ] = ((uint32_t)HelpFuncTable);
   commonROMScratchArea[ 4 ] = ((uint32_t)Rom2RomPatchTable) - ( 32 * sizeof( uint32_t ));
}
