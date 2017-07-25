/******************************************************************************
*  Filename:       rf_patch_cpe_ble_priv_1_2.h
*  Revised:        $Date: 2016-12-07 13:37:33 +0100 (on, 07 des 2016) $
*  Revision:       $Revision: 17556 $
*
*  Description:    RF Core patch file for CC26x0 Bluetooth Low Energy with privacy 1.2 support
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

#ifndef _RF_PATCH_CPE_BLE_PRIV_1_2_H
#define _RF_PATCH_CPE_BLE_PRIV_1_2_H

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


CPE_PATCH_TYPE patchImageBlePriv12[] = {
   0x210005c5,
   0x21000611,
   0x21000691,
   0x2100047d,
   0x4c17b5f0,
   0x18612140,
   0x280278c8,
   0x4809d005,
   0x60012100,
   0x47884908,
   0x6e25bdf0,
   0x60354e07,
   0x43280760,
   0x68276620,
   0x480e6024,
   0x60274780,
   0xbdf06035,
   0x4004112c,
   0x000065a5,
   0x40044028,
   0x4c07b510,
   0x29007da1,
   0x2101d105,
   0x024875a1,
   0x393e4904,
   0x68204788,
   0xd0002800,
   0xbd104780,
   0x21000254,
   0x0000398b,
   0x4905b510,
   0xb6724a05,
   0x280178c8,
   0x2001dc02,
   0x1d127048,
   0x4710b662,
   0x21000294,
   0x0000476d,
   0x4d53b5fe,
   0x462c4628,
   0x90003040,
   0x7e014627,
   0x78383760,
   0xd0022900,
   0xd10707c0,
   0x09c1e050,
   0x07c0d04e,
   0x7d20d14c,
   0xd5490640,
   0x31724629,
   0x20064a48,
   0x98004790,
   0x28007e00,
   0x7d20d007,
   0xd5010640,
   0xe0002003,
   0x26132001,
   0x6f68e008,
   0x28010f80,
   0x2006d002,
   0xe0014606,
   0x26072003,
   0x02312201,
   0x1a890412,
   0x02008a7a,
   0x43020412,
   0x35806f6b,
   0x68a89501,
   0x47a84d37,
   0x2e062201,
   0x2e07d002,
   0xe007d002,
   0xe00543c0,
   0x70797839,
   0x70394311,
   0x61089901,
   0xda012800,
   0x55022039,
   0x7e809800,
   0xd0022800,
   0x201e2106,
   0x6a61e002,
   0x201f1f89,
   0x6ca162a1,
   0x64e04788,
   0xbdfe2000,
   0x47804826,
   0x4822bdfe,
   0x78413060,
   0xd0022900,
   0x21007001,
   0x48217041,
   0x470038b0,
   0x4e1cb5f8,
   0x4635481f,
   0x7fec3540,
   0x09e14637,
   0x6db1d01a,
   0xd0172901,
   0x29007f69,
   0x07a1d002,
   0xe011d502,
   0xd10f07e1,
   0x06497d39,
   0x2103d50c,
   0x77e94321,
   0x6f314780,
   0x29010f89,
   0x2100d002,
   0x76793720,
   0xbdf877ec,
   0xbdf84780,
   0x31404909,
   0x28157508,
   0x281bd008,
   0x281dd008,
   0x490ad008,
   0x18400080,
   0x47706980,
   0x47704808,
   0x47704808,
   0x47704808,
   0x21000144,
   0x0000b8af,
   0x0000a001,
   0x0000be03,
   0x0000b98d,
   0x0000ccc0,
   0x21000579,
   0x21000563,
   0x2100049d,
   0x4e1ab5f8,
   0x6b314605,
   0x09cc4819,
   0x2d0001e4,
   0x4918d011,
   0x29027809,
   0x7b00d00f,
   0xb6724304,
   0x4f152001,
   0x47b80240,
   0x38204811,
   0x09c18800,
   0xd00407c9,
   0x7ac0e016,
   0x7b40e7f0,
   0x490fe7ee,
   0x61cc6334,
   0x07c00a40,
   0x2001d00c,
   0x6af10380,
   0xd0012d00,
   0xe0004301,
   0x46084381,
   0x490762f1,
   0x63483940,
   0x47b82000,
   0xbdf8b662,
   0x21000280,
   0x21000088,
   0x21000296,
   0x00003cdf,
   0x40044040,
   0x28004907,
   0x2004d000,
   0xb6724a06,
   0x07c97809,
   0x5810d001,
   0x2080e000,
   0xb240b662,
   0x00004770,
   0x2100026b,
   0x40046058,
};
#define _NWORD_PATCHIMAGE_BLE_PRIV_1_2 173

#define _NWORD_PATCHSYS_BLE_PRIV_1_2 0

#define _IRQ_PATCH_0 0x21000415
#define _IRQ_PATCH_1 0x21000455


#ifndef _BLE_PRIV_1_2_SYSRAM_START
#define _BLE_PRIV_1_2_SYSRAM_START 0x20000000
#endif

#ifndef _BLE_PRIV_1_2_CPERAM_START
#define _BLE_PRIV_1_2_CPERAM_START 0x21000000
#endif

#define _BLE_PRIV_1_2_SYS_PATCH_FIXED_ADDR 0x20000000

#define _BLE_PRIV_1_2_PARSER_PATCH_TAB_OFFSET 0x0334
#define _BLE_PRIV_1_2_PATCH_TAB_OFFSET 0x033C
#define _BLE_PRIV_1_2_IRQPATCH_OFFSET 0x03AC
#define _BLE_PRIV_1_2_PATCH_VEC_OFFSET 0x0404

PATCH_FUN_SPEC void enterBlePriv12CpePatch(void)
{
   uint32_t *pPatchVec = (uint32_t *) (_BLE_PRIV_1_2_CPERAM_START + _BLE_PRIV_1_2_PATCH_VEC_OFFSET);

#if (_NWORD_PATCHIMAGE_BLE_PRIV_1_2 > 0)
   memcpy(pPatchVec, patchImageBlePriv12, sizeof(patchImageBlePriv12));
#endif
}

PATCH_FUN_SPEC void enterBlePriv12SysPatch(void)
{
}

PATCH_FUN_SPEC void configureBlePriv12Patch(void)
{
   uint8_t *pPatchTab = (uint8_t *) (_BLE_PRIV_1_2_CPERAM_START + _BLE_PRIV_1_2_PATCH_TAB_OFFSET);
   uint32_t *pIrqPatch = (uint32_t *) (_BLE_PRIV_1_2_CPERAM_START + _BLE_PRIV_1_2_IRQPATCH_OFFSET);


   pPatchTab[1] = 0;
   pPatchTab[103] = 1;
   pPatchTab[60] = 2;
   pPatchTab[48] = 3;

   pIrqPatch[1] = _IRQ_PATCH_0;
   pIrqPatch[9] = _IRQ_PATCH_1;
}

PATCH_FUN_SPEC void applyBlePriv12Patch(void)
{
   enterBlePriv12SysPatch();
   enterBlePriv12CpePatch();
   configureBlePriv12Patch();
}

PATCH_FUN_SPEC void refreshBlePriv12Patch(void)
{
   enterBlePriv12CpePatch();
   configureBlePriv12Patch();
}

PATCH_FUN_SPEC void rf_patch_cpe_ble_priv_1_2(void)
{
   applyBlePriv12Patch();
}

#undef _IRQ_PATCH_0
#undef _IRQ_PATCH_1

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  _RF_PATCH_CPE_BLE_PRIV_1_2_H

