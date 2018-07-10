/******************************************************************************
*  Filename:       rf_patch_mce_slr.h
*  Revised:        $Date: 2017-12-01 13:47:22 +0100 (fr, 01 des 2017) $
*  Revision:       $Revision: 18103 $
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

#ifndef _RF_PATCH_MCE_SLR_H
#define _RF_PATCH_MCE_SLR_H

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

MCE_PATCH_TYPE patchSlrMce[254] = { 
   0x60456045,
   0x2fcf146f,
   0xdb3e0f9d,
   0x3c337f7f,
   0xcccc333c,
   0x00003cc3,
   0x00020001,
   0x00000003,
   0x000c0003,
   0x00cc000f,
   0x003c00c3,
   0xcccc0033,
   0x33cccc33,
   0x0f003333,
   0x00000f0f,
   0x00070003,
   0x0000001f,
   0x04000000,
   0x0000000f,
   0x00010387,
   0x004348c0,
   0x00048000,
   0x06f00080,
   0x091e0000,
   0x04500524,
   0x08000005,
   0x00484820,
   0x001f0000,
   0x004c7f7f,
   0x00009000,
   0x00007f7f,
   0x00000000,
   0x00000000,
   0x00000000,
   0x72230000,
   0x73037263,
   0x72037305,
   0x73067304,
   0x73917204,
   0xc7c07291,
   0x00018001,
   0x90109001,
   0x90010801,
   0x720e720d,
   0x7210720f,
   0x7100b0d0,
   0xa0d0b110,
   0x8162721b,
   0x39521020,
   0x00200670,
   0x11011630,
   0x6c011401,
   0x6077606c,
   0x610b608b,
   0x72231210,
   0x73127311,
   0x73147313,
   0x001081b1,
   0xb07091b0,
   0xc1e1605a,
   0xc4e0c2b2,
   0x6f131820,
   0x16116e23,
   0x687b1612,
   0xc9c0c632,
   0x40881820,
   0x6e231203,
   0x68851612,
   0x91a0d000,
   0x7311606c,
   0xc0007312,
   0xc00991f0,
   0x398081a0,
   0x16100670,
   0x449a1e10,
   0xc008c0b7,
   0x60a9c01e,
   0x44a01e20,
   0xc018c0f7,
   0x60a9c03e,
   0x44a61e40,
   0xc038c137,
   0x60a9c07e,
   0xc078c177,
   0xc030c0fe,
   0xb0e865f6,
   0xb1287100,
   0xb230a0e8,
   0xb003b630,
   0xb002b013,
   0x8176b012,
   0x06f63946,
   0xb0e01616,
   0xc13cc004,
   0x92c0c070,
   0xc0707821,
   0x06321012,
   0x6f2314c2,
   0x71009643,
   0x3921b120,
   0x161468c0,
   0x44be1c64,
   0x0bf17821,
   0x1012c070,
   0x14c20632,
   0x96436f23,
   0xb1207100,
   0x68cf3921,
   0xc6d5c4f4,
   0x81dfc066,
   0x821064f4,
   0x40e12210,
   0xc07060db,
   0xc00f1a10,
   0x68e364f4,
   0xc030a0e1,
   0x99309910,
   0xb0d1b111,
   0xb1117100,
   0x7291a0d1,
   0xa002a003,
   0x606c7223,
   0x316f061f,
   0x109100f9,
   0x99710441,
   0x061a898a,
   0x04511091,
   0x898b9971,
   0x311b061b,
   0x391914ba,
   0x6fa3147a,
   0x92ce9643,
   0xb1207100,
   0xb1187000,
   0x410f8cb4,
   0xc1f09334,
   0x78609440,
   0x721e9450,
   0x107681a7,
   0x06773987,
   0x89889977,
   0xd0401618,
   0x65f69a17,
   0x95907870,
   0x95a07880,
   0x95b07890,
   0x95c078a0,
   0x78506557,
   0x656a99c0,
   0xb007b017,
   0xb0e4b124,
   0xc004c205,
   0x65ef65de,
   0x49351c54,
   0x161491c0,
   0x1e018181,
   0x16714130,
   0x4d301c41,
   0x79bea0e7,
   0x979e79cf,
   0x120497af,
   0x161465ef,
   0x41481c54,
   0x614291c0,
   0xc050a235,
   0xa0e765f6,
   0x7206a0e4,
   0x72047202,
   0x72047203,
   0x73067305,
   0x72917391,
   0xc061606c,
   0xc0023181,
   0x6e12cff3,
   0x6e121611,
   0xc7e01611,
   0x16116e13,
   0x16116e12,
   0xcff06960,
   0x16116e12,
   0x70006966,
   0xb016b006,
   0xb014b004,
   0xb012b002,
   0x90307830,
   0x78409050,
   0x90609040,
   0xb072b235,
   0x93f0c090,
   0xb0f7b137,
   0xb0f6b136,
   0xb0737100,
   0xa0f7b127,
   0x80b0a0f7,
   0x459f2270,
   0xa0f7a0f6,
   0xba3eb0e7,
   0xb1367100,
   0xb041b127,
   0xc0f0b061,
   0xba3f93f0,
   0x31148b14,
   0xa0449704,
   0xb1277100,
   0xb06db04d,
   0xb231b074,
   0xb1277100,
   0xb0e77000,
   0x8af4ba39,
   0x3d843184,
   0xc05098d4,
   0xc05098e0,
   0x88f069a8,
   0x1410c201,
   0x83343d60,
   0x3d643164,
   0xcff01804,
   0x04043520,
   0xc0b09334,
   0xb06993f0,
   0x9a14d060,
   0x710065f6,
   0xa0e7b127,
   0x1209617a,
   0x65d4120a,
   0x141a1409,
   0x41d11e17,
   0x141965d4,
   0x1e37140a,
   0x65d441d1,
   0x141a1409,
   0x141965d4,
   0x3c89140a,
   0x70003c8a,
   0xb1277100,
   0xb1277100,
   0x318089a0,
   0x89b13980,
   0x70003981,
   0x109b65bf,
   0x65bf10ac,
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
   0x89f09a00,
   0x45f72200,
   0x7000b9e0
};

PATCH_FUN_SPEC void rf_patch_mce_slr(void)
{
#ifdef __PATCH_NO_UNROLLING
   uint32_t i;
   for (i = 0; i < 254; i++) {
      HWREG(RFC_MCERAM_BASE + 4 * i) = patchSlrMce[i];
   }
#else
   const uint32_t *pS = patchSlrMce;
   volatile unsigned long *pD = &HWREG(RFC_MCERAM_BASE);
   uint32_t t1, t2, t3, t4, t5, t6, t7, t8;
   uint32_t nIterations = 31;

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
   *pD++ = t1;
   *pD++ = t2;
   *pD++ = t3;
   *pD++ = t4;
   *pD++ = t5;
   *pD++ = t6;
#endif
}

#endif
