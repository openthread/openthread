/******************************************************************************
*  Filename:       chipinfo.c
*  Revised:        2017-01-06 12:37:45 +0100 (Fri, 06 Jan 2017)
*  Revision:       48166
*
*  Description:    Collection of functions returning chip information.
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
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

#include "chipinfo.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  ChipInfo_GetSupportedProtocol_BV
    #define ChipInfo_GetSupportedProtocol_BV NOROM_ChipInfo_GetSupportedProtocol_BV
    #undef  ChipInfo_GetPackageType
    #define ChipInfo_GetPackageType         NOROM_ChipInfo_GetPackageType
    #undef  ChipInfo_GetChipType
    #define ChipInfo_GetChipType            NOROM_ChipInfo_GetChipType
    #undef  ChipInfo_GetChipFamily
    #define ChipInfo_GetChipFamily          NOROM_ChipInfo_GetChipFamily
    #undef  ChipInfo_GetHwRevision
    #define ChipInfo_GetHwRevision          NOROM_ChipInfo_GetHwRevision
    #undef  ThisLibraryIsFor_CC13x2_13x4_26x2_26x4_HaltIfViolated
    #define ThisLibraryIsFor_CC13x2_13x4_26x2_26x4_HaltIfViolated NOROM_ThisLibraryIsFor_CC13x2_13x4_26x2_26x4_HaltIfViolated
#endif

//*****************************************************************************
//
// ChipInfo_GetSupportedProtocol_BV()
//
//*****************************************************************************
ProtocolBitVector_t
ChipInfo_GetSupportedProtocol_BV( void )
{
   return ((ProtocolBitVector_t)( HWREG( PRCM_BASE + 0x1D4 ) & 0x0E ));
}

//*****************************************************************************
//
// ChipInfo_GetPackageType()
//
//*****************************************************************************
PackageType_t
ChipInfo_GetPackageType( void )
{
   PackageType_t packType = (PackageType_t)((
      HWREG( FCFG1_BASE + FCFG1_O_USER_ID     ) &
                          FCFG1_USER_ID_PKG_M ) >>
                          FCFG1_USER_ID_PKG_S ) ;

   if (( packType < PACKAGE_4x4    ) ||
       ( packType > PACKAGE_7x7_Q1 )    )
   {
      packType = PACKAGE_Unknown;
   }

   return ( packType );
}

//*****************************************************************************
//
// ChipInfo_GetChipFamily()
//
//*****************************************************************************
ChipFamily_t
ChipInfo_GetChipFamily( void )
{
   uint32_t       waferId                    ;
   ChipFamily_t   chipFam = FAMILY_Unknown   ;

   waferId = (( HWREG( FCFG1_BASE + FCFG1_O_ICEPICK_DEVICE_ID ) &
                                      FCFG1_ICEPICK_DEVICE_ID_WAFER_ID_M ) >>
                                      FCFG1_ICEPICK_DEVICE_ID_WAFER_ID_S ) ;

   if ( waferId == 0xBB41 ) {
      chipFam = FAMILY_CC13x2_13x4_26x2_26x4 ;
   }

   return ( chipFam );
}

//*****************************************************************************
//
// ChipInfo_GetChipType()
//
//*****************************************************************************
ChipType_t
ChipInfo_GetChipType( void )
{
   ChipType_t     chipType       = CHIP_TYPE_Unknown        ;

   // TBD
   // TBD Definition of ChipType for CC13x2, CC13x4, CC26x2 and CC26x4 in function ChipInfo_GetChipType() must be defined before RTM
   // TBD

   return ( chipType );
}

//*****************************************************************************
//
// ChipInfo_GetHwRevision()
//
//*****************************************************************************
HwRevision_t
ChipInfo_GetHwRevision( void )
{
   HwRevision_t   hwRev       = HWREV_Unknown                     ;
   uint32_t       fcfg1Rev    = ChipInfo_GetDeviceIdHwRevCode()   ;
   uint32_t       minorHwRev  = ChipInfo_GetMinorHwRev()          ;
   ChipFamily_t   chipFam     = ChipInfo_GetChipFamily()          ;

   if ( chipFam == FAMILY_CC13x2_13x4_26x2_26x4 ) {
      switch ( fcfg1Rev ) {
      case 0 : // CC13x2, CC13x4, CC26x2, CC26x4 - PG1.0 (or later)
      case 1 : // CC13x2, CC13x4, CC26x2, CC26x4 - PG1.01 (will show up as PG1.0 as long as minorHwRev=0)
         hwRev = (HwRevision_t)(((uint32_t)HWREV_1_0 ) + minorHwRev );
         break;
      }
   }

   return ( hwRev );
}

//*****************************************************************************
// ThisLibraryIsFor_CC13x2_13x4_26x2_26x4_HaltIfViolated()
//*****************************************************************************
void
ThisLibraryIsFor_CC13x2_13x4_26x2_26x4_HaltIfViolated( void )
{
   if ( ! ChipInfo_ChipFamilyIs_CC13x2_13x4_26x2_26x4() )
   {
      while(1)
      {
         // This driverlib version is for the CCxxx2/xxx4 chips.
         // Do nothing - stay here forever
      }
   }
}
