/*****************************************************************************
 *
 * MODULE:             ALL MODULES
 *
 * DESCRIPTION:        The JENNIC standard header file defining extensions to
 *                     ANSI C standard required by the Jennic C coding standard.
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

#ifndef  JENDEFS_INCLUDED
#define  JENDEFS_INCLUDED

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <stdint.h>

#ifndef __KERNEL__
#include <stdarg.h>
#include <stdio.h>
#endif

#ifdef ECOS
#include <cyg/infra/cyg_type.h>
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/*--------------------------------------------------------------------------*/
/*          Compiler Constants for GCC compiler                             */
/*--------------------------------------------------------------------------*/

#if defined(__GNUC__)

/* The following 5 macros force the compiler to align the specified object  */
/* on a given byte boundary.                                                */
/* Example:                                                                 */
/*     int  iVar  ALIGN_(16);  Force iVar to be aligned on the next         */
/*                             16 byte boundary                             */

#define PACK      __attribute__ ((packed))        /* align to byte boundary  */
#define ALIGN_2   __attribute__ ((aligned (2)))   /* 16-bit boundary (2 byte)*/
#define ALIGN_4   __attribute__ ((aligned (4)))   /* 32-bit boundary (4 byte)*/
#define ALIGN_8   __attribute__ ((aligned (8)))   /* 64-bit boundary (8 byte)*/
#define ALIGN_(x) __attribute__ (((aligned (x)))) /* arbitrary alignment     */

/* force an unused variable not to be elided */
#define USED      __attribute__ ((used))

#if __GNUC__ < 5
#define INLINE    extern inline
#else
#if (defined JENNIC_CHIP_FAMILY_JN516x) || (defined JENNIC_CHIP_FAMILY_JN517x)
#pragma message ( "Unsupported version of GCC is being used!!" )
#endif
#define INLINE    inline
#endif

#define ALWAYS_INLINE __attribute__((always_inline))

#ifndef WEAK
#define WEAK      __attribute__ ((weak))
#endif
#define ALIAS(f)  __attribute__ ((weak, alias (#f)))

#elif defined(_MSC_VER)

#define PACK
#define USED
#define __attribute__(x)
#define ALWAYS_INLINE
#define INLINE __inline


#elif defined(__IAR_SYSTEMS_ICC__)
// if compiled for IAR, none of the above defines are required
#else
#error "Unsupported compiler"
#endif

/*--------------------------------------------------------------------------*/
/*          Alignment masks                                                 */
/*--------------------------------------------------------------------------*/

#define ALIGNMENT_MASK_4_BYTE     ((uint32)0x00000003)
#define ALIGNMENT_MASK_16_BYTE    ((uint32)0x0000000F)

/* Test for alignment on an arbitrary byte boundary - TRUE if aligned */

#define IS_ALIGNED(addr, mask) (((((uint32)addr) & mask) ? FALSE : TRUE))

/*--------------------------------------------------------------------------*/
/*          Boolean Constants                                               */
/*--------------------------------------------------------------------------*/

#if !defined FALSE && !defined TRUE
#define TRUE            (1)   /* page 207 K+R 2nd Edition */
#define FALSE           (0)
#endif /* !defined FALSE && #if !defined TRUE */

/*     Note that WINDOWS.H also defines TRUE and FALSE                 */

/*--------------------------------------------------------------------------*/
/*          Null pointers                                                   */
/*--------------------------------------------------------------------------*/

/* NULL data pointer */
#if !defined NULL
#define NULL                    ( (void *) 0 )
#endif

/* NULL routine pointer */
#if !defined RNULL
#define RNULL                   ((void *) 0 ())
#endif

/*--------------------------------------------------------------------------*/
/*          Macros for setting/clearing bits efficiently                    */
/*--------------------------------------------------------------------------*/

#define U8_CLR_BITS( P, B )   (( *(uint8 *)P ) &= ~( B ))
#define U8_SET_BITS( P, B )   (( *(uint8 *)P ) |=  ( B ))
#define U16_CLR_BITS( P, B )  (( *(uint16 *)P ) &= ~( B ))
#define U16_SET_BITS( P, B )  (( *(uint16 *)P ) |=  ( B ))
#define U32_CLR_BITS( P, B )  (( *(uint32 *)P ) &= ~( B ))
#define U32_SET_BITS( P, B )  (( *(uint32 *)P ) |=  ( B ))
#define U64_CLR_BITS( P, B )  (( *(uint64 *)P ) &= ~( B ))
#define U64_SET_BITS( P, B )  (( *(uint64 *)P ) |=  ( B ))

/*--------------------------------------------------------------------------*/
/*          Macros for obtaining maximum/minimum values                     */
/*--------------------------------------------------------------------------*/
#ifndef MAX
#define MAX(A,B)                ( ( ( A ) > ( B ) ) ? ( A ) : ( B ) )
#endif
#ifndef MIN
#define MIN(A,B)                ( ( ( A ) < ( B ) ) ? ( A ) : ( B ) )
#endif

/*--------------------------------------------------------------------------*/
/*          Number of bits in quantities                                    */
/*--------------------------------------------------------------------------*/

#define BITS_PER_U32            (32)
#define BITS_PER_U16            (16)
#define BITS_PER_U8             (8)
#define BITS_PER_NIBBLE         (4)

/*--------------------------------------------------------------------------*/
/*          Masking macros                                                  */
/*--------------------------------------------------------------------------*/

#define U8_LOW_NIBBLE_MASK      (0x0F)
#define U8_HIGH_NIBBLE_MASK     (0xF0)

#define U16_LOW_U8_MASK         (0x00FF)
#define U16_HIGH_U8_MASK        (0xFF00)

#define U32_LOWEST_U8_MASK      (0x000000FFL)
#define U32_LOW_U8_MASK         (0x0000FF00L)
#define U32_HIGH_U8_MASK        (0x00FF0000L)
#define U32_HIGHEST_U8_MASK     (0xFF000000L)

#define U32_LOWEST_U16_MASK     (0x0000FFFFL)
#define U32_HIGHEST_U16_MASK    (0xFFFF0000L)

#define U64_LOWEST_U32_MASK     (0x00000000FFFFFFFFLL)
#define U64_HIGHEST_U32_MASK    (0xFFFFFFFF00000000LL)

/*--------------------------------------------------------------------------*/
/*          Macros for extracting uint8s from a uint16                      */
/*--------------------------------------------------------------------------*/

/* NOTE: U16_UPPER_U8 is only safe for an unsigned U16 as >> fills with the sign bit for signed variables */
#define U16_UPPER_U8( x )      ((uint8) ((x) >> BITS_PER_U8))
#define U16_LOWER_U8( x )      ((uint8) (((x) & U16_LOW_U8_MASK)))

/*--------------------------------------------------------------------------*/
/*          Macros for extracting uint8s from a uint32                      */
/*--------------------------------------------------------------------------*/

#define U32_HIGHEST_U8( x ) ((uint8)(((x) & U32_HIGHEST_U8_MASK) >> (BITS_PER_U16 + BITS_PER_U8)))

#define U32_HIGH_U8( x ) ((uint8)( ( ( x ) & U32_HIGH_U8_MASK ) >> BITS_PER_U16 ))

#define U32_LOW_U8( x ) ((uint8)( ( ( x ) & U32_LOW_U8_MASK ) >> BITS_PER_U8 ))

#define U32_LOWEST_U8( x ) ((uint8)( ( ( x ) & U32_LOWEST_U8_MASK ) ))

/*--------------------------------------------------------------------------*/
/*          Macros for extracting uint16s from a uint32                     */
/*--------------------------------------------------------------------------*/

#define U32_UPPER_U16( x )     ((uint16)( ( ( x ) & U32_HIGHEST_U16_MASK ) >> ( BITS_PER_U16 ) ))
#define U32_LOWER_U16( x )     ((uint16)( ( ( x ) & U32_LOWEST_U16_MASK ) ))

/*--------------------------------------------------------------------------*/
/*          Macros for extracting uint32s from a uint64                     */
/*--------------------------------------------------------------------------*/

#define U64_UPPER_U32( x )     ((uint32)( ( ( x ) & U64_HIGHEST_U32_MASK ) >> ( BITS_PER_U32 ) ))
#define U64_LOWER_U32( x )     ((uint32)( ( ( x ) & U64_LOWEST_U32_MASK ) ))

/*--------------------------------------------------------------------------*/
/*         Macros for assembling byte sequences into various word sizes     */
/*--------------------------------------------------------------------------*/

/* B0 - LSB, B3 - MSB */

#ifdef HOST_PROCESSOR_BIG_ENDIAN
/* BIG ENDIAN DEFINITIONS */
#define BYTE_ORDER_32(B3, B2, B1, B0)   ((B0) + (B1<<8) + (B2<<16) + (B3<<24))

#define BYTE_ORDER_24(B2, B1, B0)       ((B0) + (B1<<8) + (B2<<16))

#define BYTE_ORDER_16(B1, B0)           (uint16)(((B0) + (B1<<8)))

#define BYTE_ORDER_8(B0)                (B0)

#define BYTE_ORDER_4(B0)                (B0)
#else
/* LITTLE ENDIAN DEFINITIONS */
#define BYTE_ORDER_32(B3, B2, B1, B0)   ((B3) + (B2<<8) + (B1<<16) + (B0<<24))

#define BYTE_ORDER_24(B2, B1, B0)       ((B2) + (B1<<8) + (B0<<16))

#define BYTE_ORDER_16(B1, B0)           (uint16)(((B1) + (B0<<8)))

#define BYTE_ORDER_8(B0)                (B0)

#define BYTE_ORDER_4(B0)                (B0)
#endif

/*--------------------------------------------------------------------------*/
/*          Storage classes                                                 */
/*--------------------------------------------------------------------------*/

#if !defined PUBLIC
#define PUBLIC
#endif

#if !defined MODULE
#define MODULE
#endif

#if !defined PRIVATE
#define PRIVATE static
#endif

#if !defined BANKED
#define BANKED
#endif

/*--------------------------------------------------------------------------*/
/* Useful macro for variables that are not currently referenced             */
/* Prevents compiler warnings and should not produce any code               */
/*--------------------------------------------------------------------------*/

#define VARIABLE_INTENTIONALLY_NOT_REFERENCED(x) (x=x);
#define CONST_POINTER_INTENTIONALLY_NOT_REFERENCED(p) (*p);
#define CONST_VARIABLE_INTENTIONALLY_NOT_REFERENCED(x) (x);

/*--------------------------------------------------------------------------*/
/* Offset of field m in a struct s                                          */
/*--------------------------------------------------------------------------*/

#if !defined(__GNUC__) && !defined(ECOS) && !defined(offsetof)
#define offsetof(type, tag)     ( (int)&( (type *)0 )->tag )
#endif

/*--------------------------------------------------------------------------*/
/*          Diagnostics                                                     */
/*--------------------------------------------------------------------------*/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

    #ifndef __cplusplus /* microsoft specific */
        #ifndef bool /* Seems to need this in for certain M$ builds*/
            typedef int                     bool;     /* boolean type */
        #endif
    #endif

    typedef uint8_t                 BOOL_T;     /* boolean type nothing to do with C++ */
    typedef uint8_t                 bool_t;     /* boolean type nothing to do with C++ */

    typedef int8_t                  int8;
    typedef int16_t                 int16;
    typedef int32_t                 int32;
    typedef int64_t                 int64;
    typedef uint8_t                 uint8;
    typedef uint16_t                uint16;
    typedef uint32_t                uint32;
    typedef uint64_t                uint64;

    typedef char *                  string;

    typedef volatile uint8          u8Register;
    typedef volatile uint16         u16Register;
    typedef volatile uint32         u32Register;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#endif  /* JENDEFS_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/


