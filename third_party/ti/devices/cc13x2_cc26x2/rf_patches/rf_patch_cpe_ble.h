/******************************************************************************
*  Filename:       rf_patch_cpe_ble.h
*  Revised:        $Date: 2018-05-07 15:02:01 +0200 (ma, 07 mai 2018) $
*  Revision:       $Revision: 18438 $
*
*  Description: RF core patch for Bluetooth low energy version 4.0, 4.1, and 4.2 support ("BLE" API command set) in CC13x2 and CC26x2
*
*  Copyright (c) 2015-2018, Texas Instruments Incorporated
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
#ifndef _RF_PATCH_CPE_BLE_H
#define _RF_PATCH_CPE_BLE_H

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <string.h>

#ifndef CPE_PATCH_TYPE
#define CPE_PATCH_TYPE static const uint32_t
#endif

#ifndef SYS_PATCH_TYPE
#define SYS_PATCH_TYPE static const uint32_t
#endif

#ifndef PATCH_FUN_SPEC
#define PATCH_FUN_SPEC static inline
#endif

#ifndef _APPLY_PATCH_TAB
#define _APPLY_PATCH_TAB
#endif


CPE_PATCH_TYPE patchImageBle[] = {
   0x210040c9,
   0x21004141,
   0x21004073,
   0x21004061,
   0x21004209,
   0x2100421d,
   0x21004237,
   0x210040ad,
   0xb081b5ff,
   0x9d0a4803,
   0xb5f84700,
   0x48024684,
   0x47004613,
   0x00007f43,
   0x00005145,
   0x2270480b,
   0x43916801,
   0x43112240,
   0x48096001,
   0x48094700,
   0x460c7801,
   0x43912208,
   0xf0007001,
   0x4805f803,
   0xbd107004,
   0x4804b510,
   0x00004700,
   0x40043018,
   0x0000710f,
   0x210000b3,
   0x00005603,
   0x4801b510,
   0x00004700,
   0x000009df,
   0x49044803,
   0x05c068c0,
   0x47880fc0,
   0x47084902,
   0x21000340,
   0x000087d1,
   0x000053cd,
   0x4614b5f8,
   0x9b06461a,
   0x46139300,
   0xf7ff4622,
   0x9000ffb5,
   0xd12d2800,
   0x6848492e,
   0xd0292800,
   0x00498809,
   0x43411b09,
   0x68c0482b,
   0x00640844,
   0x20187922,
   0xb2c61a80,
   0x04802001,
   0x0cc71808,
   0x46317965,
   0xf0004825,
   0x4824f8a1,
   0x19832201,
   0x408a1e69,
   0x21182000,
   0xe0071b8e,
   0x19090041,
   0x437988c9,
   0x40e91889,
   0x1c405419,
   0xdcf54286,
   0x4780481b,
   0xbdf89800,
   0xf7ffb510,
   0x4604ff84,
   0x20004914,
   0x60484a17,
   0x88006810,
   0x28030b00,
   0x0760d01d,
   0x6810d41b,
   0x28007bc0,
   0x2802d001,
   0x2013d115,
   0x4a0c01c0,
   0x68d08008,
   0xd00428c0,
   0xd00428d8,
   0x383938ff,
   0x480bd109,
   0x480be000,
   0x480b6048,
   0x211860d0,
   0xf0004804,
   0x4620f85f,
   0x0000bd10,
   0x21004260,
   0x21000028,
   0x21000000,
   0x0000764d,
   0x21000108,
   0x00063b91,
   0x0003fd29,
   0x000090fd,
   0x480eb570,
   0x4c0e6ac0,
   0x0f000700,
   0x28012502,
   0x2804d005,
   0x280cd00b,
   0x280dd001,
   0x6960d106,
   0x616043a8,
   0x49072001,
   0x60080280,
   0x4906bd70,
   0x47882001,
   0x43286960,
   0xbd706160,
   0x40046000,
   0x40041100,
   0xe000e180,
   0x00007d05,
   0x4c03b510,
   0xff48f7ff,
   0x28006820,
   0xbd10d1fa,
   0x40041100,
   0x780a490b,
   0xd1042aff,
   0x7ad24a0a,
   0x0f120712,
   0x4908700a,
   0x75883140,
   0x49054770,
   0x29ff7809,
   0x0900d005,
   0x43080100,
   0x31404902,
   0x47707588,
   0x210002a5,
   0x40086200,
   0x4801b403,
   0xbd019001,
   0x000089dd,
   0x00000000,
   0x00000000,
};
#define _NWORD_PATCHIMAGE_BLE 145

#define _NWORD_PATCHSYS_BLE 0

#define _IRQ_PATCH_0 0x210041bd


#ifndef _BLE_SYSRAM_START
#define _BLE_SYSRAM_START 0x20000000
#endif

#ifndef _BLE_CPERAM_START
#define _BLE_CPERAM_START 0x21000000
#endif

#define _BLE_SYS_PATCH_FIXED_ADDR 0x20000000

#define _BLE_PARSER_PATCH_TAB_OFFSET 0x0390
#define _BLE_PATCH_TAB_OFFSET 0x0398
#define _BLE_IRQPATCH_OFFSET 0x0434
#define _BLE_PATCH_VEC_OFFSET 0x4024

#ifndef _BLE_NO_PROG_STATE_VAR
static uint8_t bBlePatchEntered = 0;
#endif

PATCH_FUN_SPEC void enterBleCpePatch(void)
{
#if (_NWORD_PATCHIMAGE_BLE > 0)
   uint32_t *pPatchVec = (uint32_t *) (_BLE_CPERAM_START + _BLE_PATCH_VEC_OFFSET);

   memcpy(pPatchVec, patchImageBle, sizeof(patchImageBle));
#endif
}

PATCH_FUN_SPEC void enterBleSysPatch(void)
{
}

PATCH_FUN_SPEC void configureBlePatch(void)
{
   uint8_t *pPatchTab = (uint8_t *) (_BLE_CPERAM_START + _BLE_PATCH_TAB_OFFSET);
   uint32_t *pIrqPatch = (uint32_t *) (_BLE_CPERAM_START + _BLE_IRQPATCH_OFFSET);


   pPatchTab[142] = 0;
   pPatchTab[66] = 1;
   pPatchTab[73] = 2;
   pPatchTab[117] = 3;
   pPatchTab[28] = 4;
   pPatchTab[105] = 5;
   pPatchTab[106] = 6;
   pPatchTab[70] = 7;

   pIrqPatch[21] = _IRQ_PATCH_0;
}

PATCH_FUN_SPEC void applyBlePatch(void)
{
#ifdef _BLE_NO_PROG_STATE_VAR
   enterBleSysPatch();
   enterBleCpePatch();
#else
   if (!bBlePatchEntered)
   {
      enterBleSysPatch();
      enterBleCpePatch();
      bBlePatchEntered = 1;
   }
#endif
   configureBlePatch();
}

PATCH_FUN_SPEC void refreshBlePatch(void)
{
   configureBlePatch();
}

#ifndef _BLE_NO_PROG_STATE_VAR
PATCH_FUN_SPEC void cleanBlePatch(void)
{
   bBlePatchEntered = 0;
}
#endif

PATCH_FUN_SPEC void rf_patch_cpe_ble(void)
{
   applyBlePatch();
}

#undef _IRQ_PATCH_0

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  _RF_PATCH_CPE_BLE_H

