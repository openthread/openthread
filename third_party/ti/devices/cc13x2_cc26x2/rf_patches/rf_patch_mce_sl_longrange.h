/******************************************************************************
*  Filename:       rf_patch_mce_sl_longrange.h
*  Revised:        $Date: 2018-01-15 06:15:14 +0100 (ma, 15 jan 2018) $
*  Revision:       $Revision: 18170 $
*
*  Description: RF core patch for SimpleLink Long Range support in CC13x2 and CC26x2
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

#ifndef _RF_PATCH_MCE_SL_LONGRANGE_H
#define _RF_PATCH_MCE_SL_LONGRANGE_H

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

MCE_PATCH_TYPE patchSimplelinkLrmMce[228] = { 
   0x60466046,
   0x2fcf146f,
   0xdb3e0f9d,
   0x00007f7f,
   0x00020001,
   0x00000003,
   0x000c0003,
   0x00cc000f,
   0x003c00c3,
   0xcccc0033,
   0x33cccc33,
   0x0f003333,
   0x005f0f0f,
   0x005d0060,
   0x005b005e,
   0x0059005c,
   0x0003005a,
   0x001f0007,
   0x00000000,
   0x000f0400,
   0x03870000,
   0x40c00001,
   0x80000043,
   0x01000004,
   0x000006f0,
   0x0524091e,
   0x00050054,
   0x48200800,
   0x00000048,
   0x7f7f001f,
   0x8004004c,
   0x2e000000,
   0x00000000,
   0x00000000,
   0x10200000,
   0x65bb7223,
   0xa4e5a35d,
   0x73057303,
   0x73047203,
   0x72047306,
   0x72917391,
   0xffc0b008,
   0xa0089010,
   0x720e720d,
   0x7210720f,
   0x7100b0d0,
   0xa0d0b110,
   0x8162721b,
   0x39521020,
   0x00200670,
   0x11011630,
   0x6c011401,
   0x607e607d,
   0x60956095,
   0x607d607d,
   0x607d607d,
   0x60731220,
   0x72231210,
   0x73127311,
   0x73147313,
   0x001081b1,
   0xb07091b0,
   0x6072605a,
   0xc2b2c211,
   0x1820c4f0,
   0x6e236f13,
   0x16121611,
   0xc6326882,
   0x1820c9c0,
   0x1203408f,
   0x16126e23,
   0xb63c688c,
   0x91a0d000,
   0x9990c0c0,
   0xb1186072,
   0x409d8cb4,
   0xb0099334,
   0xb019b008,
   0x6594b018,
   0x94507860,
   0x81a7721e,
   0x39871076,
   0x99770677,
   0x16188988,
   0x9a17d030,
   0x64dc65c1,
   0x99c07850,
   0xb00764ef,
   0xb124b017,
   0xc205b0e4,
   0x657cc004,
   0x1c54658d,
   0x91c048ba,
   0x81811614,
   0x40b51e01,
   0x1c411671,
   0xa0e74cb5,
   0x798f797e,
   0x97af979e,
   0x658d1204,
   0x1c541614,
   0x91c040cd,
   0xa23560c7,
   0x65c1c040,
   0xa0e4a0e7,
   0x72027206,
   0x72037204,
   0x73057204,
   0x73917306,
   0x60727291,
   0x3181c061,
   0xcff3c002,
   0x16116e12,
   0x16116e12,
   0x6e13c7e0,
   0x6e121611,
   0x68e51611,
   0x6e12cff0,
   0x68eb1611,
   0xb0067000,
   0xb004b016,
   0xb002b014,
   0x7830b012,
   0x90509030,
   0x90407840,
   0xb2359060,
   0xc090b072,
   0xc60093f0,
   0x99303150,
   0x9910c060,
   0xb0f7b137,
   0xb10cb14c,
   0xb0d1b111,
   0x80b07100,
   0x45272270,
   0xa10cb073,
   0xb111a0f7,
   0xb127a0d1,
   0xba3eb0e7,
   0xb14c7100,
   0xb041b127,
   0xa044b061,
   0xa008a009,
   0xb1277100,
   0xb06db04d,
   0xb231b074,
   0xb1277100,
   0x80907000,
   0x41332210,
   0xa910b111,
   0x93308cb0,
   0x93f0c090,
   0xb008b009,
   0xb0e76104,
   0xa009a0f7,
   0xba39a008,
   0x93f0c0b0,
   0x31848af4,
   0x98d43d84,
   0x98e08990,
   0x6941c050,
   0xc40188f0,
   0x3d701410,
   0x31648334,
   0x18043d64,
   0x3520cff0,
   0x93340404,
   0x22c180c1,
   0xc060450e,
   0x80c17100,
   0x450e22c1,
   0x6952b127,
   0xb910b069,
   0xa0e7b127,
   0x12096104,
   0x6572120a,
   0x141a1409,
   0x416f1e17,
   0x14196572,
   0x1e37140a,
   0x6572416f,
   0x141a1409,
   0x14196572,
   0x3c89140a,
   0x70003c8a,
   0xb1277100,
   0xb1277100,
   0x318089a0,
   0x89b13980,
   0x70003981,
   0x109b655d,
   0x655d10ac,
   0x318d10bd,
   0x10cd14db,
   0x14dc318d,
   0x14a9318a,
   0x149c149b,
   0x97ac979b,
   0xa0e77000,
   0x7100b079,
   0xb0e7b124,
   0x700089d0,
   0x65c1c050,
   0xc0fcc19b,
   0xc07084a1,
   0x06321012,
   0x6f2314c2,
   0x41a22200,
   0x61a71034,
   0x00343183,
   0x6e746fb7,
   0x3921161b,
   0x84b1699a,
   0x1012c070,
   0x14c20632,
   0x22006f23,
   0x103441b3,
   0x318361b8,
   0x6fb70034,
   0x161b6e74,
   0x69ab3921,
   0x86307000,
   0x3151c801,
   0x96300410,
   0x9a007000,
   0x220089f0,
   0xb9e045c2,
   0x00007000
};

PATCH_FUN_SPEC void rf_patch_mce_sl_longrange(void)
{
#ifdef __PATCH_NO_UNROLLING
   uint32_t i;
   for (i = 0; i < 228; i++) {
      HWREG(RFC_MCERAM_BASE + 4 * i) = patchSimplelinkLrmMce[i];
   }
#else
   const uint32_t *pS = patchSimplelinkLrmMce;
   volatile unsigned long *pD = &HWREG(RFC_MCERAM_BASE);
   uint32_t t1, t2, t3, t4, t5, t6, t7, t8;
   uint32_t nIterations = 28;

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
   *pD++ = t1;
   *pD++ = t2;
   *pD++ = t3;
   *pD++ = t4;
#endif
}

#endif
