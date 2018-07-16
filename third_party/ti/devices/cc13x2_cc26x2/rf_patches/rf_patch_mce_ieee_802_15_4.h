/******************************************************************************
*  Filename:       rf_patch_mce_ieee_802_15_4.h
*  Revised:        $Date: 2018-01-15 06:15:14 +0100 (ma, 15 jan 2018) $
*  Revision:       $Revision: 18170 $
*
*  Description: RF core patch for IEEE 802.15.4-2006 support in CC13x2 and CC26x2
*
*  Copyright (c) 2015-2017, Texas Instruments Incorporated
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
#ifndef _RF_PATCH_MCE_IEEE_802_15_4_H
#define _RF_PATCH_MCE_IEEE_802_15_4_H

#include <stdint.h>
#include "../inc/hw_types.h"

#ifndef MCE_PATCH_TYPE
#define MCE_PATCH_TYPE static const uint32_t
#endif

#ifndef PATCH_FUN_SPEC
#define PATCH_FUN_SPEC static inline
#endif

#ifndef RFC_MCERAM_BASE
#define RFC_MCERAM_BASE 0x21008000
#endif

#ifndef MCE_PATCH_MODE
#define MCE_PATCH_MODE 0
#endif

MCE_PATCH_TYPE patchIeee_802_15_4Mce[367] = { 
   0xf7036064,
   0x70399b3a,
   0x039bb3af,
   0x39b33af7,
   0x9b3aaf70,
   0xb3aff703,
   0x3af77039,
   0xaf70039b,
   0x08fcb9b3,
   0x8fc664c5,
   0xfc644c50,
   0xc64cc508,
   0x64c5508f,
   0x4c5008fc,
   0xc5088fc6,
   0x508ffc64,
   0x2fc0c64c,
   0x079c0fcf,
   0x7f7f01e0,
   0x0000dead,
   0x00000003,
   0x0000001f,
   0x80000000,
   0x0004000c,
   0x00011400,
   0x00000009,
   0x00008000,
   0x06700000,
   0x16aa002b,
   0x00000a11,
   0x00000b60,
   0x00404010,
   0x00060000,
   0x001e1e1e,
   0x00000000,
   0x00000003,
   0x0000001f,
   0x80000000,
   0x0004000c,
   0x000114c4,
   0x00000009,
   0x00008000,
   0x0f900000,
   0x121d0000,
   0x00000a11,
   0x00000b60,
   0x00404010,
   0x00060000,
   0x00202020,
   0x00000000,
   0x720d7223,
   0x720f720e,
   0x72637210,
   0x7203a35d,
   0x73057204,
   0x73917306,
   0xc7c07291,
   0xb0d09010,
   0xa0d07100,
   0x721bb110,
   0x10208162,
   0x06703952,
   0x16300020,
   0x14011101,
   0x60a16c01,
   0x60ca60b2,
   0x61466119,
   0x60a160a1,
   0x60a160a1,
   0x60ca60ad,
   0x61466119,
   0x60a160a1,
   0x60a160a1,
   0x60ca60b2,
   0x61466119,
   0x60a160a1,
   0x60a160a1,
   0x60ca60ad,
   0x61466119,
   0x60a160a1,
   0x60a460a1,
   0x60a51220,
   0x73111210,
   0x73137312,
   0x001081b1,
   0xb07091b0,
   0xc4616073,
   0xc460c2b2,
   0x60b61820,
   0xc2b2c281,
   0x1820c460,
   0x6e236f13,
   0x16121611,
   0xc63268b6,
   0x1820c9c0,
   0x120340c3,
   0x16126e23,
   0x7a4068c0,
   0xc1409c80,
   0x12209c90,
   0x60a49990,
   0xc1307291,
   0xb11391f0,
   0x81621213,
   0xd0303982,
   0x66d89a12,
   0x40e41e02,
   0x40e41e12,
   0x40dd1e22,
   0x00238992,
   0xb00560e4,
   0x1e208990,
   0x60e444e2,
   0xa005b270,
   0xb0e8b128,
   0xb1287100,
   0x9233a0e8,
   0x1e008c80,
   0x993040f2,
   0xb111b910,
   0x7100b0d1,
   0xb002b012,
   0xb013b633,
   0xb111b003,
   0x7291a0d1,
   0x7100b0d3,
   0xa0d3b113,
   0xc0301000,
   0xc0209910,
   0xb0d19930,
   0xb1117100,
   0x7291a0d1,
   0x39828162,
   0x490d1a22,
   0x726366d1,
   0xa002a003,
   0x73057263,
   0x73917306,
   0xa2307291,
   0x9010c7c0,
   0xb63c60a4,
   0x94c07810,
   0x782095b0,
   0x95c094d0,
   0xc0f18190,
   0xc0120410,
   0x14023110,
   0x94a06f20,
   0x16129590,
   0x94b06f20,
   0x824095a0,
   0x45352230,
   0x7100b0d5,
   0xa0d5b115,
   0x7291612d,
   0x91e0c030,
   0xb004b002,
   0x8160b006,
   0x41402280,
   0x61417a20,
   0x90507a10,
   0x7a309030,
   0x90409060,
   0x8460b235,
   0x84613180,
   0x39813181,
   0x94500010,
   0x06f58195,
   0x1217120e,
   0x12f01206,
   0xc1909440,
   0xb0119440,
   0xb0f9b139,
   0xb089a0fc,
   0x12041203,
   0x39828162,
   0x9a12d040,
   0x1e2266d8,
   0x1e324227,
   0x7100421d,
   0xa0d17291,
   0xa0f9b139,
   0xb0fcb13c,
   0xc600a444,
   0x99303140,
   0xb111b910,
   0x7100b0d1,
   0x7291a0d1,
   0x22108090,
   0xb13c46d5,
   0x1e7665be,
   0x81904194,
   0x06f03940,
   0x45461c0f,
   0x1c908c90,
   0x12054d46,
   0x12761202,
   0x9070c300,
   0xc050b230,
   0xc0c066d8,
   0x7a509440,
   0x619b9450,
   0x91cf1647,
   0x1e008180,
   0x1c70419b,
   0x710049a0,
   0x22008090,
   0x617b4476,
   0x7291a235,
   0x72047203,
   0x73067305,
   0xa004a002,
   0x7263a006,
   0x73067305,
   0x72917391,
   0x9010c7c0,
   0x39808160,
   0x4dbb1a30,
   0x22508000,
   0x122045b9,
   0x124061bc,
   0x89c061bc,
   0x60a49990,
   0x120a1209,
   0x120dc0cc,
   0x8ad8ba3e,
   0x49c91c89,
   0x4dcf1c8a,
   0x108961d6,
   0x18801200,
   0x10db100a,
   0x108a61d6,
   0x18801200,
   0x10db1009,
   0x61d6168b,
   0x1e8d161d,
   0x908c41db,
   0x10bf61c2,
   0x41e61e82,
   0x16121495,
   0x45e61e82,
   0x81b03125,
   0x91b50005,
   0x49e91e8b,
   0xc0701a8b,
   0x163018b0,
   0x14011101,
   0x908c6c01,
   0x908c908c,
   0x908c908c,
   0x908c908c,
   0xb082908c,
   0xba3e1000,
   0xb0838ad3,
   0x1000b083,
   0x8ad4ba3e,
   0x4a071e8f,
   0x4a0c1ca3,
   0x4a0e1ca4,
   0x1c937000,
   0x1c944e0c,
   0x70004e0e,
   0x62101a1e,
   0x6210161e,
   0x1ce0c040,
   0x10e04a17,
   0x4a1a1640,
   0xc00e7000,
   0x7000b085,
   0xb084c00e,
   0xa3a77000,
   0x66acb3a5,
   0x6a21cbe0,
   0xc7f066ac,
   0xb3a76a24,
   0x8460a3a5,
   0x31801640,
   0x16418461,
   0x39813181,
   0x94500010,
   0x94b07810,
   0x782095a0,
   0x959094a0,
   0xb0d1a0f9,
   0xb258b005,
   0xc0307291,
   0xb1309910,
   0xc290b133,
   0x80b09440,
   0x465e2230,
   0x465e2200,
   0x9930c040,
   0xb910a910,
   0xa0f0a0f3,
   0x7100b111,
   0xc2f0b064,
   0xb1309930,
   0xb111b133,
   0x80b07100,
   0x465c2230,
   0x465c2200,
   0x623f66ac,
   0x625e66ac,
   0xc0408248,
   0xa9109930,
   0x3988b910,
   0x9a18d060,
   0xb11166d8,
   0xc2f07100,
   0xb0649930,
   0xb133b130,
   0x7100b111,
   0x220080b0,
   0x22304676,
   0x627d4676,
   0x39818241,
   0x9a11d070,
   0x1c1866d8,
   0x66ac4a86,
   0x9930c040,
   0xb910a910,
   0x66d8c080,
   0xb0647100,
   0x9440c190,
   0xb139b0f9,
   0xa0f0a0f3,
   0x06f08190,
   0x3110c012,
   0x6f201402,
   0x959094a0,
   0x6f201612,
   0x95a094b0,
   0x9930c5f0,
   0x31808460,
   0x31818461,
   0x00103981,
   0xa9109450,
   0xb111b910,
   0x7100b133,
   0x229080b0,
   0x22304568,
   0x622746a1,
   0x39808160,
   0x4eb81a30,
   0x22508000,
   0xb00546b5,
   0xb27062b7,
   0x7000a005,
   0x22128232,
   0x261246c0,
   0x92322a22,
   0x62c499c2,
   0x26222a12,
   0x99c29232,
   0x96a49693,
   0x9a30c030,
   0x8a548a43,
   0x82400662,
   0x1c020660,
   0xb05346cb,
   0x89907000,
   0x99900a60,
   0xc0907000,
   0x615366d8,
   0x89f09a00,
   0x46d92200,
   0x7000b9e0
};

PATCH_FUN_SPEC void rf_patch_mce_ieee_802_15_4(void)
{
#ifdef __PATCH_NO_UNROLLING
   uint32_t i;
   for (i = 0; i < 367; i++) {
      HWREG(RFC_MCERAM_BASE + 4 * i) = patchIeee_802_15_4Mce[i];
   }
#else
   const uint32_t *pS = patchIeee_802_15_4Mce;
   volatile unsigned long *pD = &HWREG(RFC_MCERAM_BASE);
   uint32_t t1, t2, t3, t4, t5, t6, t7, t8;
   uint32_t nIterations = 45;

   do {
      t1 = *pS++;
      t2 = *pS++;
      t3 = *pS++;
      t4 = *pS++;
      t5 = *pS++;
      t6 = *pS++;
      t7 = *pS++;
      t8 = *pS++;
      *pD++ = t1;
      *pD++ = t2;
      *pD++ = t3;
      *pD++ = t4;
      *pD++ = t5;
      *pD++ = t6;
      *pD++ = t7;
      *pD++ = t8;
   } while (--nIterations);

   t1 = *pS++;
   t2 = *pS++;
   t3 = *pS++;
   t4 = *pS++;
   t5 = *pS++;
   t6 = *pS++;
   t7 = *pS++;
   *pD++ = t1;
   *pD++ = t2;
   *pD++ = t3;
   *pD++ = t4;
   *pD++ = t5;
   *pD++ = t6;
   *pD++ = t7;
#endif
}

#endif
