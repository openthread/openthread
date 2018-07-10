/******************************************************************************
*  Filename:       rf_patch_cpe_multi_protocol.h
*  Revised:        $Date: 2018-05-07 15:02:01 +0200 (ma, 07 mai 2018) $
*  Revision:       $Revision: 18438 $
*
*  Description: RF core patch for multi-protocol support (all available API command sets) in CC13x2 and CC26x2
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
#ifndef _RF_PATCH_CPE_MULTI_PROTOCOL_H
#define _RF_PATCH_CPE_MULTI_PROTOCOL_H

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


CPE_PATCH_TYPE patchImageMultiProtocol[] = {
   0x21004245,
   0x21004265,
   0x210042dd,
   0x21004385,
   0x2100442d,
   0x210045e3,
   0x21004633,
   0x210040bd,
   0x210040c9,
   0x210040e1,
   0x210046b7,
   0x21004105,
   0x21004125,
   0x21004785,
   0x21004159,
   0x2100417d,
   0x2100419f,
   0x2100418d,
   0x2100483d,
   0x21004857,
   0x210041d1,
   0x210041ed,
   0x210041fd,
   0x21004955,
   0x2100420d,
   0xb081b5ff,
   0x9d0a4803,
   0xb5f84700,
   0x48024684,
   0x47004613,
   0x00007f43,
   0x00005145,
   0x460cb5f7,
   0x47084900,
   0x0000681d,
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
   0x210049b8,
   0x21000340,
   0x6ac34807,
   0xf808f000,
   0x009b089b,
   0x6ac14804,
   0xd10007c9,
   0xbdf862c3,
   0xb5f84902,
   0x00004708,
   0x40045040,
   0x00029dd3,
   0x4801b510,
   0x00004700,
   0x000009df,
   0x460db5f8,
   0x4b0a4616,
   0x290c6d59,
   0x4b03d104,
   0x78192408,
   0x70194321,
   0x47084901,
   0x21000340,
   0x0000699d,
   0x46864810,
   0x21804801,
   0x47706041,
   0x40045000,
   0x2270480d,
   0x43916801,
   0x43112240,
   0x480b6001,
   0x480b4700,
   0x460c7801,
   0x43912208,
   0xf0007001,
   0x4807f803,
   0xbd107004,
   0xf7ffb510,
   0x4801ffe3,
   0x470038c8,
   0x000056cb,
   0x40043018,
   0x0000710f,
   0x210000b3,
   0x49044803,
   0x05c068c0,
   0x47880fc0,
   0x47084902,
   0x21000340,
   0x000087d1,
   0x000053cd,
   0xf0002000,
   0x4604fb41,
   0x47004800,
   0x0000545b,
   0xf0002004,
   0x4605fb39,
   0x47004800,
   0x0000533b,
   0xf811f000,
   0x296cb2e1,
   0x2804d00b,
   0x2806d001,
   0x4907d107,
   0x07c97809,
   0x7821d103,
   0xd4000709,
   0xb0032002,
   0xb5f0bdf0,
   0x4902b083,
   0x00004708,
   0x210000c8,
   0x0003071b,
   0x4905b672,
   0x22206808,
   0x600a4302,
   0x6ad24a03,
   0xb6626008,
   0x4770b250,
   0x40040000,
   0x40046040,
   0x4614b5f8,
   0x9b06461a,
   0x46139300,
   0xf7ff4622,
   0x9000ff09,
   0xd12d2800,
   0x6848495f,
   0xd0292800,
   0x00498809,
   0x43411b09,
   0x68c0485c,
   0x00640844,
   0x20187922,
   0xb2c61a80,
   0x04802001,
   0x0cc71808,
   0x46317965,
   0xf0004856,
   0x4855fb7b,
   0x19832201,
   0x408a1e69,
   0x21182000,
   0xe0071b8e,
   0x19090041,
   0x437988c9,
   0x40e91889,
   0x1c405419,
   0xdcf54286,
   0x4780484c,
   0xbdf89800,
   0xf7ffb570,
   0x4a46fed8,
   0x49492300,
   0x60534604,
   0x25136808,
   0x01ed8800,
   0x2e030b06,
   0x0760d00d,
   0x6808d43f,
   0x290c7bc1,
   0xdc0fd028,
   0xd0132904,
   0xd0142905,
   0xd10d290a,
   0xb2c0e01d,
   0xd0032806,
   0x8c006808,
   0xe02d8010,
   0xe02b8015,
   0xd018290f,
   0xd019291e,
   0xe0048015,
   0x01802013,
   0x4835e000,
   0x48308010,
   0x29c068c1,
   0x29d8d010,
   0x39ffd010,
   0xd1173939,
   0x20ffe00a,
   0xe7f130e7,
   0x309620ff,
   0x20ffe7ee,
   0xe7eb3045,
   0xe7e920a2,
   0xe000492a,
   0x6051492a,
   0x60c1492a,
   0x48232118,
   0xfb14f000,
   0x8013e000,
   0xbd704620,
   0x4604b5f8,
   0x4e1c481d,
   0x88003040,
   0x0a80460d,
   0xd00407c0,
   0x08644821,
   0x43447900,
   0x8830e025,
   0xd0222800,
   0x19000960,
   0xfe7af7ff,
   0x20014607,
   0x0240491b,
   0x46024788,
   0x1bc02005,
   0x40848831,
   0x18230848,
   0x62034817,
   0x21016241,
   0x430d61c1,
   0x60cd490a,
   0x07c969c1,
   0x6a80d1fc,
   0x60704910,
   0x39124610,
   0x46384788,
   0x2000bdf8,
   0x46206070,
   0xfe56f7ff,
   0x0000bdf8,
   0x210049b0,
   0x21000028,
   0x21000000,
   0x0000764d,
   0x21000108,
   0x000003cd,
   0x00063b91,
   0x0003fd29,
   0x000090fd,
   0x21000340,
   0x000040e5,
   0x40044100,
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
   0xfd0ef7ff,
   0xda072800,
   0x1ab900c2,
   0x7e493920,
   0x42112214,
   0x2000d100,
   0x4302bdf8,
   0x46384303,
   0xfcfef7ff,
   0xb570bdf8,
   0xfd1ef7ff,
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
   0xf8f4f000,
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
   0x210045a3,
   0x2100457f,
   0x21004553,
   0x21004501,
   0x210044db,
   0x2100449b,
   0x21004449,
   0x0000ffff,
   0x21000340,
   0x210049b8,
   0x000040e5,
   0x4c03b510,
   0xfce0f7ff,
   0x28006820,
   0xbd10d1fa,
   0x40041100,
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
   0x4c19b570,
   0x7ba14606,
   0xf820f000,
   0x7be14605,
   0xf0004630,
   0x4915f81b,
   0x78094604,
   0x070a2028,
   0x2d01d401,
   0x2038d100,
   0xd40106c9,
   0xd1012c01,
   0x43082140,
   0x4788490e,
   0xd0012dff,
   0x6145480d,
   0xd0012cff,
   0x61c4480c,
   0xbd704808,
   0xd0082900,
   0xd00629ff,
   0x070840c1,
   0x281c0ec0,
   0x2001d100,
   0x20ff4770,
   0x00004770,
   0x210000a8,
   0x21000340,
   0x000040e5,
   0x40045040,
   0x40046000,
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
#define _NWORD_PATCHIMAGE_MULTI_PROTOCOL 614

#define _NWORD_PATCHSYS_MULTI_PROTOCOL 0

#define _IRQ_PATCH_0 0x21004799


#ifndef _MULTI_PROTOCOL_SYSRAM_START
#define _MULTI_PROTOCOL_SYSRAM_START 0x20000000
#endif

#ifndef _MULTI_PROTOCOL_CPERAM_START
#define _MULTI_PROTOCOL_CPERAM_START 0x21000000
#endif

#define _MULTI_PROTOCOL_SYS_PATCH_FIXED_ADDR 0x20000000

#define _MULTI_PROTOCOL_PARSER_PATCH_TAB_OFFSET 0x0390
#define _MULTI_PROTOCOL_PATCH_TAB_OFFSET 0x0398
#define _MULTI_PROTOCOL_IRQPATCH_OFFSET 0x0434
#define _MULTI_PROTOCOL_PATCH_VEC_OFFSET 0x4024

#ifndef _MULTI_PROTOCOL_NO_PROG_STATE_VAR
static uint8_t bMultiProtocolPatchEntered = 0;
#endif

PATCH_FUN_SPEC void enterMultiProtocolCpePatch(void)
{
#if (_NWORD_PATCHIMAGE_MULTI_PROTOCOL > 0)
   uint32_t *pPatchVec = (uint32_t *) (_MULTI_PROTOCOL_CPERAM_START + _MULTI_PROTOCOL_PATCH_VEC_OFFSET);

   memcpy(pPatchVec, patchImageMultiProtocol, sizeof(patchImageMultiProtocol));
#endif
}

PATCH_FUN_SPEC void enterMultiProtocolSysPatch(void)
{
}

PATCH_FUN_SPEC void configureMultiProtocolPatch(void)
{
   uint8_t *pParserPatchTab = (uint8_t *) (_MULTI_PROTOCOL_CPERAM_START + _MULTI_PROTOCOL_PARSER_PATCH_TAB_OFFSET);
   uint8_t *pPatchTab = (uint8_t *) (_MULTI_PROTOCOL_CPERAM_START + _MULTI_PROTOCOL_PATCH_TAB_OFFSET);
   uint32_t *pIrqPatch = (uint32_t *) (_MULTI_PROTOCOL_CPERAM_START + _MULTI_PROTOCOL_IRQPATCH_OFFSET);


   pPatchTab[84] = 0;
   pPatchTab[142] = 1;
   pPatchTab[66] = 2;
   pPatchTab[102] = 3;
   pParserPatchTab[1] = 4;
   pPatchTab[1] = 5;
   pPatchTab[18] = 6;
   pPatchTab[112] = 7;
   pPatchTab[115] = 8;
   pPatchTab[22] = 9;
   pPatchTab[10] = 10;
   pPatchTab[36] = 11;
   pPatchTab[53] = 12;
   pPatchTab[28] = 13;
   pPatchTab[104] = 14;
   pPatchTab[75] = 15;
   pPatchTab[73] = 16;
   pPatchTab[117] = 17;
   pPatchTab[105] = 18;
   pPatchTab[106] = 19;
   pPatchTab[70] = 20;
   pPatchTab[71] = 21;
   pPatchTab[69] = 22;
   pParserPatchTab[0] = 23;
   pPatchTab[60] = 24;

   pIrqPatch[21] = _IRQ_PATCH_0;
}

PATCH_FUN_SPEC void applyMultiProtocolPatch(void)
{
#ifdef _MULTI_PROTOCOL_NO_PROG_STATE_VAR
   enterMultiProtocolSysPatch();
   enterMultiProtocolCpePatch();
#else
   if (!bMultiProtocolPatchEntered)
   {
      enterMultiProtocolSysPatch();
      enterMultiProtocolCpePatch();
      bMultiProtocolPatchEntered = 1;
   }
#endif
   configureMultiProtocolPatch();
}

PATCH_FUN_SPEC void refreshMultiProtocolPatch(void)
{
   configureMultiProtocolPatch();
}

#ifndef _MULTI_PROTOCOL_NO_PROG_STATE_VAR
PATCH_FUN_SPEC void cleanMultiProtocolPatch(void)
{
   bMultiProtocolPatchEntered = 0;
}
#endif

PATCH_FUN_SPEC void rf_patch_cpe_multi_protocol(void)
{
   applyMultiProtocolPatch();
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

#endif //  _RF_PATCH_CPE_MULTI_PROTOCOL_H

