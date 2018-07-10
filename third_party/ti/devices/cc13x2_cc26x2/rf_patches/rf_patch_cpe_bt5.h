/******************************************************************************
*  Filename:       rf_patch_cpe_bt5.h
*  Revised:        $Date: 2018-05-07 15:02:01 +0200 (ma, 07 mai 2018) $
*  Revision:       $Revision: 18438 $
*
*  Description: RF core patch for Bluetooth 5 support ("BLE" and "BLE5" API command sets) in CC13x2 and CC26x2
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
#ifndef _RF_PATCH_CPE_BT5_H
#define _RF_PATCH_CPE_BT5_H

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


CPE_PATCH_TYPE patchImageBt5[] = {
   0x21004161,
   0x210041d9,
   0x21004255,
   0x2100440b,
   0x2100445b,
   0x21004091,
   0x2100409d,
   0x210040b5,
   0x210044df,
   0x210040d9,
   0x2100410b,
   0x210040f9,
   0x21004651,
   0x21004665,
   0x2100467f,
   0x21004145,
   0x21004701,
   0xb081b5ff,
   0x9d0a4803,
   0xb5f84700,
   0x48024684,
   0x47004613,
   0x00007f43,
   0x00005145,
   0x461db570,
   0x47204c00,
   0x00022281,
   0x0a804670,
   0x288d4a05,
   0x4710d004,
   0x32e44a03,
   0x90042001,
   0x20004902,
   0x47107008,
   0x00006f69,
   0x210001f8,
   0x88084903,
   0x46714a03,
   0xd0004290,
   0x47081c89,
   0x210001a6,
   0x00001404,
   0xb083b5f3,
   0x47004800,
   0x00020b17,
   0x68024805,
   0xd3030852,
   0x60da4b04,
   0x60022200,
   0x1d004670,
   0x00004700,
   0x21004764,
   0x21000340,
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
   0x9000ff7b,
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
   0x4824fad3,
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
   0x4604ff4a,
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
   0x4620fa91,
   0x0000bd10,
   0x2100475c,
   0x21000028,
   0x21000000,
   0x0000764d,
   0x21000108,
   0x00063b91,
   0x0003fd29,
   0x000090fd,
   0x4abb8801,
   0x29031889,
   0x48bad806,
   0x8501217c,
   0x624149b9,
   0x47702001,
   0x470849b8,
   0xb43048b8,
   0x31404601,
   0x2a027c0a,
   0x6802d116,
   0x754a79d2,
   0x68936802,
   0x32804602,
   0x7d486093,
   0xd00b2802,
   0xd0092800,
   0x2c061ec4,
   0x49aad809,
   0x18400080,
   0x6b803840,
   0x60901818,
   0xbc3048aa,
   0x20034700,
   0x80c802c0,
   0x72082002,
   0x2003bc30,
   0xb5704770,
   0x460448a3,
   0x7da53440,
   0xd0122d01,
   0x06497d01,
   0x21800fca,
   0x7c21540a,
   0xd10a2900,
   0x78403060,
   0x07c00880,
   0x7ce0d002,
   0xd5020700,
   0x75a02001,
   0x2000e000,
   0x489873a0,
   0x75a54780,
   0xb570bd70,
   0x48964c93,
   0x35504625,
   0x28024780,
   0x3440d109,
   0x4a9388e1,
   0xd1044291,
   0x06897ce9,
   0x1d91d401,
   0xbd7080e1,
   0x498ab570,
   0x4608890a,
   0xb2d23050,
   0xd1072a28,
   0x68d2680a,
   0x7a936142,
   0x02127ad2,
   0x8302189a,
   0x4a877803,
   0xd0102b00,
   0x2b017983,
   0x7c03d10d,
   0x07db095b,
   0x7d09d009,
   0x74c14d82,
   0x20207f6c,
   0x77684320,
   0x776c4790,
   0x21ffbd70,
   0x479074c1,
   0xb510bd70,
   0x4c75487c,
   0x28024780,
   0x4621d10e,
   0x88ca3140,
   0x429a4b75,
   0x7ccad108,
   0x07d20952,
   0x7d22d004,
   0xd4010692,
   0x80ca1d9a,
   0xb570bd10,
   0x4972486a,
   0x7cc03040,
   0x07c00940,
   0x4d6dd007,
   0x8b2c486f,
   0x83284320,
   0x832c4788,
   0x4788bd70,
   0xb570bd70,
   0x496b4c61,
   0x36404626,
   0x00a87935,
   0x6b401840,
   0x2d0a4780,
   0xd0104621,
   0x780a3154,
   0x07db0993,
   0x73b2d004,
   0x2303780a,
   0x700a431a,
   0xb2ca8921,
   0xd3012a2b,
   0x81213928,
   0x3153bd70,
   0x4952e7ed,
   0x71083140,
   0xd01a2825,
   0x280adc08,
   0x280bd011,
   0x2818d011,
   0x281ed011,
   0xe014d106,
   0xd010282a,
   0xd006283c,
   0xd010283d,
   0x00804951,
   0x6b401840,
   0x48504770,
   0x48504770,
   0x48504770,
   0x48504770,
   0x48504770,
   0x48504770,
   0x48504770,
   0xb5f84770,
   0x4c3d4607,
   0x5d00204e,
   0x07ee0985,
   0x2e0025fb,
   0x7d26d017,
   0x0f240734,
   0xd0032c05,
   0x42202401,
   0xe012d002,
   0xe7fa2402,
   0xd00509c0,
   0xd5030670,
   0x0f806848,
   0xd0082801,
   0x005b085b,
   0x00520852,
   0x2800e003,
   0x402bd001,
   0x2b06402a,
   0x2010d003,
   0xd0102b02,
   0x2302e010,
   0x4638402a,
   0xfde4f7ff,
   0xda072800,
   0x1ab900c2,
   0x7e493920,
   0x42112214,
   0x2000d100,
   0x4302bdf8,
   0x46384303,
   0xfdd4f7ff,
   0xb570bdf8,
   0xfdf4f7ff,
   0xd12b0005,
   0x481a4917,
   0x30406809,
   0x4b2a8982,
   0xd123429a,
   0x210169cc,
   0x22000709,
   0xd20c428c,
   0x794b4926,
   0xd506075b,
   0x4a2568c8,
   0x1c400040,
   0x60cc6010,
   0x8182e012,
   0x8182e010,
   0x49212075,
   0x478800c0,
   0x46204606,
   0xf8b6f000,
   0xd0012800,
   0x02ed2503,
   0xb2b0491b,
   0x47883912,
   0xbd704628,
   0xffffe7d5,
   0x21000108,
   0x00020619,
   0x00020625,
   0x21000160,
   0x000245a5,
   0x00022cfd,
   0x00023b49,
   0x00001404,
   0x000238fd,
   0x210000a8,
   0x000224fb,
   0x00021d4f,
   0x00002020,
   0x00024fc0,
   0x210043cb,
   0x210043a7,
   0x2100437b,
   0x21004329,
   0x21004303,
   0x210042c3,
   0x21004271,
   0x0000ffff,
   0x21000340,
   0x21004764,
   0x000040e5,
   0x4d20b5f8,
   0x882c4820,
   0x4e206ac0,
   0x20010701,
   0x02400f09,
   0x0b222702,
   0xd0202908,
   0x2901dc0d,
   0x2904d00f,
   0x491ad114,
   0xd01207d2,
   0x43824622,
   0x2001802a,
   0x802c4788,
   0x290ce00d,
   0x290dd001,
   0x6970d106,
   0x617043b8,
   0x49122001,
   0x60080280,
   0x2001bdf8,
   0x69704788,
   0x61704338,
   0x0a61bdf8,
   0xd0fb07c9,
   0xd0f907d1,
   0x47a04c0b,
   0x2201490b,
   0x03926a48,
   0x62484310,
   0x63484909,
   0x47a02000,
   0x0000bdf8,
   0x21000068,
   0x40046000,
   0x40041100,
   0x00007d05,
   0xe000e180,
   0x000045b7,
   0x210002c0,
   0x40044000,
   0x4c03b510,
   0xfd70f7ff,
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
   0x4e26b5f1,
   0x79742000,
   0x60f068f7,
   0x46254824,
   0x05c08800,
   0x2004d502,
   0x71744304,
   0x4b21221d,
   0x98002100,
   0x28004798,
   0x68f1d109,
   0x29001f03,
   0x9900d004,
   0xb2ca6809,
   0xd0032a2b,
   0x60f74618,
   0xbdf87175,
   0x0f890589,
   0xd1022902,
   0x400c21fb,
   0x21047174,
   0x400d400c,
   0xd0f242ac,
   0x03094a12,
   0xbdf86091,
   0x4604b570,
   0x28158800,
   0x490fd003,
   0x47884620,
   0x2075bd70,
   0x00c0490d,
   0x46054788,
   0xf7ff6860,
   0x2800ffbd,
   0x2487d001,
   0x2401e000,
   0x46284907,
   0x4788311c,
   0xbd704620,
   0x21000340,
   0x21000284,
   0x00004a99,
   0x40041100,
   0x0000270f,
   0x000045c7,
   0x4801b403,
   0xbd019001,
   0x000089dd,
   0x00000000,
   0x00000000,
   0x00000000,
};
#define _NWORD_PATCHIMAGE_BT5 465

#define _NWORD_PATCHSYS_BT5 0

#define _IRQ_PATCH_0 0x210045ad


#ifndef _BT5_SYSRAM_START
#define _BT5_SYSRAM_START 0x20000000
#endif

#ifndef _BT5_CPERAM_START
#define _BT5_CPERAM_START 0x21000000
#endif

#define _BT5_SYS_PATCH_FIXED_ADDR 0x20000000

#define _BT5_PARSER_PATCH_TAB_OFFSET 0x0390
#define _BT5_PATCH_TAB_OFFSET 0x0398
#define _BT5_IRQPATCH_OFFSET 0x0434
#define _BT5_PATCH_VEC_OFFSET 0x4024

#ifndef _BT5_NO_PROG_STATE_VAR
static uint8_t bBt5PatchEntered = 0;
#endif

PATCH_FUN_SPEC void enterBt5CpePatch(void)
{
#if (_NWORD_PATCHIMAGE_BT5 > 0)
   uint32_t *pPatchVec = (uint32_t *) (_BT5_CPERAM_START + _BT5_PATCH_VEC_OFFSET);

   memcpy(pPatchVec, patchImageBt5, sizeof(patchImageBt5));
#endif
}

PATCH_FUN_SPEC void enterBt5SysPatch(void)
{
}

PATCH_FUN_SPEC void configureBt5Patch(void)
{
   uint8_t *pParserPatchTab = (uint8_t *) (_BT5_CPERAM_START + _BT5_PARSER_PATCH_TAB_OFFSET);
   uint8_t *pPatchTab = (uint8_t *) (_BT5_CPERAM_START + _BT5_PATCH_TAB_OFFSET);
   uint32_t *pIrqPatch = (uint32_t *) (_BT5_CPERAM_START + _BT5_IRQPATCH_OFFSET);


   pPatchTab[142] = 0;
   pPatchTab[66] = 1;
   pParserPatchTab[1] = 2;
   pPatchTab[1] = 3;
   pPatchTab[18] = 4;
   pPatchTab[112] = 5;
   pPatchTab[115] = 6;
   pPatchTab[22] = 7;
   pPatchTab[10] = 8;
   pPatchTab[36] = 9;
   pPatchTab[73] = 10;
   pPatchTab[117] = 11;
   pPatchTab[28] = 12;
   pPatchTab[105] = 13;
   pPatchTab[106] = 14;
   pPatchTab[70] = 15;
   pParserPatchTab[0] = 16;

   pIrqPatch[21] = _IRQ_PATCH_0;
}

PATCH_FUN_SPEC void applyBt5Patch(void)
{
#ifdef _BT5_NO_PROG_STATE_VAR
   enterBt5SysPatch();
   enterBt5CpePatch();
#else
   if (!bBt5PatchEntered)
   {
      enterBt5SysPatch();
      enterBt5CpePatch();
      bBt5PatchEntered = 1;
   }
#endif
   configureBt5Patch();
}

PATCH_FUN_SPEC void refreshBt5Patch(void)
{
   configureBt5Patch();
}

#ifndef _BT5_NO_PROG_STATE_VAR
PATCH_FUN_SPEC void cleanBt5Patch(void)
{
   bBt5PatchEntered = 0;
}
#endif

PATCH_FUN_SPEC void rf_patch_cpe_bt5(void)
{
   applyBt5Patch();
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

#endif //  _RF_PATCH_CPE_BT5_H

