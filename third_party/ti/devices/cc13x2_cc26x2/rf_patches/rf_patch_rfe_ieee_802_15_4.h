/******************************************************************************
*  Filename:       rf_patch_rfe_ieee_802_15_4.h
*  Revised:        $Date: 2018-05-07 15:02:01 +0200 (ma, 07 mai 2018) $
*  Revision:       $Revision: 18438 $
*
*  Description: RF core patch for IEEE 802.15.4-2006 support in CC13x2 and CC26x2. Contains fix to correct RSSIMAXVAL calculation.
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
// It should only be included from ONE source file to avoid duplicated constant arrays


#ifndef _RF_PATCH_RFE_IEEE_802_15_4_H
#define _RF_PATCH_RFE_IEEE_802_15_4_H

#include <stdint.h>
#include "../inc/hw_types.h"

#ifndef RFE_PATCH_TYPE
#define RFE_PATCH_TYPE static const uint32_t
#endif

#ifndef PATCH_FUN_SPEC
#define PATCH_FUN_SPEC static inline
#endif

#ifndef RFC_RFERAM_BASE
#define RFC_RFERAM_BASE 0x2100C000
#endif

#ifndef RFE_PATCH_MODE
#define RFE_PATCH_MODE 0
#endif

RFE_PATCH_TYPE patchIeee_802_15_4Rfe[331] = { 
   0x163f614a,
   0x07ff07f7,
   0x000f0046,
   0x00040000,
   0x003f002e,
   0x40004030,
   0x40034001,
   0x400f4007,
   0x40cf404f,
   0x43cf41cf,
   0x4fcf47cf,
   0x2fcf3fcf,
   0x0fcf1fcf,
   0x9100c050,
   0xc0707000,
   0x70009100,
   0x00213182,
   0xb1109131,
   0x81017000,
   0xa100b101,
   0x91323182,
   0x9101b110,
   0x81411011,
   0x402d2241,
   0x700006f1,
   0x9150c050,
   0xc0707000,
   0x70009150,
   0x00213182,
   0xb1609181,
   0x10257000,
   0x9100c050,
   0xc0a0c3f4,
   0x6f031420,
   0x04411031,
   0x22f082a0,
   0x2651404a,
   0x3182c022,
   0x91310021,
   0x3963b110,
   0x04411031,
   0x3182c082,
   0x91310021,
   0x3963b110,
   0xc0a21031,
   0x00213182,
   0xb1109131,
   0x31151050,
   0x92551405,
   0x641d7000,
   0x1031c2b2,
   0x31610631,
   0x642002c1,
   0x1031c112,
   0x06713921,
   0x02e13151,
   0x70006420,
   0x82b1641a,
   0x39813181,
   0x6420c0e2,
   0xc111641d,
   0x6420c122,
   0x687dc470,
   0xc0c2c111,
   0x64966420,
   0x700064a9,
   0x82b16432,
   0x39813181,
   0x6438c182,
   0xc1116435,
   0x6438c0a2,
   0x688fc470,
   0xc162c331,
   0x64966438,
   0x700064a9,
   0xb054b050,
   0x80407100,
   0x44a32240,
   0x40962200,
   0x8081b060,
   0x44961e11,
   0xa0547000,
   0x80f0b064,
   0x40962200,
   0x12407000,
   0xb03290b0,
   0x395382a3,
   0x64633953,
   0x68b1c2f0,
   0xc1f18080,
   0xc1510410,
   0x40bd1c10,
   0xc221641d,
   0x6420c0c2,
   0x643560c1,
   0xc162c441,
   0xce306438,
   0x128068c2,
   0xb03290b0,
   0x641d7000,
   0xc0c2c201,
   0x80a06420,
   0x39403180,
   0xc10168ce,
   0x6420c0c2,
   0xc122c101,
   0x82a36420,
   0x12c06463,
   0xb03290b0,
   0x64357000,
   0xc162c401,
   0x80a06438,
   0x39403180,
   0xc30168e2,
   0x6438c162,
   0xc0a2c101,
   0x82a36438,
   0x12c06463,
   0xb03290b0,
   0x641d7000,
   0xc081c272,
   0xc1226420,
   0x6420c111,
   0xc111c002,
   0xc0626420,
   0x6420c331,
   0xc111c362,
   0xc3026420,
   0x6420c111,
   0x395382a3,
   0xc3e26463,
   0x22116425,
   0xc2424105,
   0x6420c881,
   0xc111c252,
   0xc2726420,
   0x6420cee1,
   0xc881c202,
   0xc2026420,
   0x6420c801,
   0x6919c170,
   0x641d7000,
   0xc801c242,
   0xc2526420,
   0x6420c011,
   0xc0e1c272,
   0xc0026420,
   0x6420c101,
   0xc301c062,
   0xc1226420,
   0x6420c101,
   0xc101c362,
   0xc3026420,
   0x6420c101,
   0x646382a3,
   0x80817000,
   0x41451e11,
   0xb054b050,
   0x80407100,
   0x41462240,
   0xb064a054,
   0x220180f1,
   0x7000453a,
   0x413a2200,
   0x6137b060,
   0x72057306,
   0x720e720b,
   0x7100b050,
   0xb0608081,
   0x8092a050,
   0x9341ef80,
   0x668f9352,
   0x456e2241,
   0xc1f18080,
   0x16300410,
   0x14011101,
   0x617f6c01,
   0x617f617f,
   0x617f617f,
   0x6180617f,
   0x61846182,
   0x617f6186,
   0x6272626f,
   0x0402c0f0,
   0x2a413132,
   0x16321412,
   0x14211101,
   0x61a46c01,
   0x61a46188,
   0x618e618a,
   0x617f6192,
   0x6196617f,
   0x61966472,
   0x619664c7,
   0x619664ef,
   0x6196651b,
   0x619665ba,
   0x65376472,
   0x619664c7,
   0x65ba64ef,
   0x6196651b,
   0x65376484,
   0x619664db,
   0xdf708082,
   0x668f9342,
   0x90b01210,
   0x1220619f,
   0x730690b0,
   0x12107205,
   0x614e9030,
   0x91a07840,
   0x31107850,
   0x14107851,
   0x78709250,
   0x78513140,
   0x31400010,
   0x00107861,
   0x78809260,
   0x78909270,
   0xc0f092b0,
   0x619690a0,
   0xa054b0e3,
   0x225080f0,
   0x804045c3,
   0x46642200,
   0xa0e361ba,
   0x668fcf60,
   0x10e9826e,
   0x394e06f9,
   0x06fa10ea,
   0x394e10ac,
   0x10c206fe,
   0x827d643d,
   0x80f310cb,
   0x41d82213,
   0x61db6675,
   0x41db2223,
   0x78104682,
   0x662f91e0,
   0xb053b013,
   0xb050b063,
   0xb064b054,
   0x80f3b003,
   0x41ba2253,
   0x80417100,
   0x46642201,
   0x41f82241,
   0x80f3b064,
   0x41f42213,
   0x61e56675,
   0x41e52223,
   0x61e56682,
   0x8200b063,
   0x822f9210,
   0x318080e0,
   0x318f3980,
   0x90e000f0,
   0x80f3822f,
   0x46622203,
   0x22408090,
   0x10f34662,
   0x4e1418d3,
   0x16130bf3,
   0x4a621ce3,
   0x82339213,
   0x6219143b,
   0x4a621ce3,
   0x82339213,
   0x1cab183b,
   0x1c9b4e2b,
   0x1cbc4a2d,
   0x1cbc4262,
   0xa0e0b0e0,
   0x10b210bc,
   0x663f643d,
   0xb063663f,
   0xb0637100,
   0x10ab61e5,
   0x109b621d,
   0x7827621d,
   0x18707830,
   0xc0011a10,
   0x16176e71,
   0x78276a34,
   0xc0061208,
   0x91b0c800,
   0x10007000,
   0x10f01000,
   0x18108251,
   0x6d716d71,
   0x14061816,
   0x16176e70,
   0x1c177831,
   0x7827464c,
   0x1e881618,
   0x1060465f,
   0x81a13d30,
   0x80f11810,
   0x425f2251,
   0x81c191b0,
   0x3d813181,
   0x4a5d1c10,
   0xb03191c0,
   0x70001278,
   0x10001000,
   0x61e5663f,
   0x8251a0e3,
   0x318281b2,
   0xef503d82,
   0x93529341,
   0xa003668f,
   0x80a27000,
   0x6196643d,
   0x7100b050,
   0xc0506196,
   0xc0029100,
   0x3182c551,
   0x91310021,
   0xb0e1b110,
   0xcf40a0e2,
   0x7000668f,
   0x9100c050,
   0xca91c002,
   0x00213182,
   0xb1109131,
   0xb0e2a0e1,
   0x668fcf30,
   0x93307000,
   0x22008320,
   0xb3104690,
   0x00007000
};

PATCH_FUN_SPEC void rf_patch_rfe_ieee_802_15_4(void)
{
#ifdef __PATCH_NO_UNROLLING
   uint32_t i;
   for (i = 0; i < 331; i++) {
      HWREG(RFC_RFERAM_BASE + 4 * i) = patchIeee_802_15_4Rfe[i];
   }
#else
   const uint32_t *pS = patchIeee_802_15_4Rfe;
   volatile unsigned long *pD = &HWREG(RFC_RFERAM_BASE);
   uint32_t t1, t2, t3, t4, t5, t6, t7, t8;
   uint32_t nIterations = 41;

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
   *pD++ = t1;
   *pD++ = t2;
   *pD++ = t3;
#endif
}

#endif
