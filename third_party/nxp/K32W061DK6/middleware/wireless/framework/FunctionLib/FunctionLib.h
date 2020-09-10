/*! *********************************************************************************
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
* All rights reserved.
*
* \file
*
* This is the Function Lib module header file
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

#ifndef _FUNCTION_LIB_H_
#define _FUNCTION_LIB_H_

#include "EmbeddedTypes.h"
#include <setjmp.h>

/*! *********************************************************************************
*************************************************************************************
* Public macros
*************************************************************************************
********************************************************************************** */

#ifndef gUseToolchainMemFunc_d
#define gUseToolchainMemFunc_d 0
#endif

#ifndef gFLib_CheckBufferOverflow_d
#define gFLib_CheckBufferOverflow_d 0
#endif

#define FLib_MemSet16 FLib_MemSet


extern jmp_buf  *exception_buf;
#define TRY do {jmp_buf exc_buf;                  \
                jmp_buf *old_buf = exception_buf; \
                exception_buf = &exc_buf;         \
                switch (setjmp(exc_buf)) { case 0:

#define CATCH(x) break; case x:
#define YRT } exception_buf = old_buf; } while(0)

typedef enum {
    BUS_EXCEPTION = 1,
} EXCEPTION_ERROR_T;


/*! *********************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif

/*! *********************************************************************************
* \brief  Copy the content of one memory block to another. The amount of data to copy
*         must be specified in number of bytes.
*
* \param[out] pDst   Pointer to destination memory block
* \param[in] pSrc    Pointer to source memory block
* \param[in] cBytes  Number of bytes to copy
*
********************************************************************************** */
void FLib_MemCpy (void* pDst,
                  const void* pSrc,
                  uint32_t cBytes
                  );

void FLib_MemCpyAligned32bit (void* to_ptr,
                              const void* from_ptr,
                              register uint32_t number_of_bytes);

void FLib_MemCpyDir (void* pBuf1,
                     void* pBuf2,
                     bool_t dir,
                     uint32_t n);


/*! *********************************************************************************
* \brief  Copy bytes. The byte at index i from the source buffer is copied to index
*         ((n-1) - i) in the destination buffer (and vice versa).
*
* \param[out] pDst   Pointer to destination memory block
* \param[in] pSrc    Pointer to source memory block
* \param[in] cBytes  Number of bytes to copy
*
********************************************************************************** */
void FLib_MemCpyReverseOrder (void* pDst,
                              const void* pSrc,
                              uint32_t cBytes
                             );


/*! *********************************************************************************
* \brief  Compare two memory blocks. The number of bytes to compare must be specified.
*         If the blocks are equal byte by byte, the function returns TRUE,
*         and FALSE otherwise.
*
* \param[in] pData1  First memory block to compare
* \param[in] pData2  Second memory block to compare
* \param[in] cBytes  Number of bytes to compare
*
* \return  TRUE if memory areas are equal. FALSE othwerwise.
*
********************************************************************************** */
bool_t FLib_MemCmp (const void* pData1,
                    const void* pData2,
                    uint32_t cBytes
                   );

/*! *********************************************************************************
* \brief  Compare each byte of a memory block to a given value. The number of bytes to compare
          must be specified.
*         If all the bytes are equal to the given value, the function returns TRUE,
*         and FALSE otherwise.
*
* \param[in] pAddr   Location to be compared
* \param[in] val     Reference value
* \param[in] length  Number of bytes to compare
*
* \return  TRUE if all bytes match and FALSE otherwise.
*
********************************************************************************** */
bool_t FLib_MemCmpToVal(const void* pAddr,
                        uint8_t val,
                        uint32_t len
                        );

/*! *********************************************************************************
* \brief  Reset bytes in a memory block to a certain value. The value, and the number
*         of bytes to be set, are supplied as arguments.
*
* \param[in] pData   Pointer to memory block to reset
* \param[in] value   Value that memory block will be set to
* \param[in] cBytes  Number of bytes to set
*
********************************************************************************** */
 void FLib_MemSet (void* pData,
                   uint8_t value,
                   uint32_t cBytes
                  );


/*! *********************************************************************************
* \brief Copy bytes, possibly into the same overlapping memory as it is taken from
*
* \param[out] pDst   Pointer to destination memory block
* \param[in] pSrc    Pointer to source memory block
* \param[in] cBytes  Number of bytes to copy
*
********************************************************************************** */
void FLib_MemInPlaceCpy (void* pDst,
                         void* pSrc,
                         uint32_t cBytes
                        );


/*! *********************************************************************************
* \brief  Copies a 16bit value to an unaligned a memory block.
*
* \param[out] pDst   Pointer to destination memory block
* \param[in] val16   The 16-bit value to be copied
*
********************************************************************************** */
void FLib_MemCopy16Unaligned (void* pDst,
                              uint16_t val16
                             );


/*! *********************************************************************************
* \brief  Copies a 32bit value to an unaligned a memory block.
*
* \param[out] pDst   Pointer to destination memory block
* \param[in] val32   The 32-bit value to be copied
*
********************************************************************************** */
void FLib_MemCopy32Unaligned (void* pDst,
                              uint32_t val32
                             );


/*! *********************************************************************************
* \brief  Copies a 64bit value to an unaligned a memory block.
*
* \param[out] pDst   Pointer to destination memory block
* \param[in] val64   The 64-bit value to be copied
*
********************************************************************************** */
void FLib_MemCopy64Unaligned (void* pDst,
                              uint64_t val64
                             );


bool_t FLib_CopyFromFlash(void* pDst,
                        const void* pSrc,
                        uint32_t cBytes);

/*! *********************************************************************************
* \brief Add an offset to a pointer.
*
* \param[out] pPtr   Pointer to the pointer to be updated
* \param[in] offset  The offset(in bytes) to be added
*
********************************************************************************** */
void FLib_AddOffsetToPointer (void** pPtr,
                              uint32_t offset);

#define FLib_AddOffsetToPtr(pPtr,offset) FLib_AddOffsetToPointer((void**)(pPtr),(offset))


/*! *********************************************************************************
* \brief  This function returns the length of a NULL terminated string.
*
* \param[in]  str  A NULL terminated string
*
* \return  the size of string in bytes
*
********************************************************************************** */
uint32_t FLib_StrLen(const char *str);


/*! *********************************************************************************
* \brief  Compare two bytes.
*
* \param[in] pCmp1  pointer to the first 2-byte compare value
* \param[in] pCmp2  pointer to the second 2-byte compare value
*
* \return  TRUE if the two bytes are equal, and FALSE otherwise.
*
********************************************************************************** */
#define FLib_Cmp2Bytes(pCmp1, pCmp2) ((((uint8_t *)(pCmp1))[0] == ((uint8_t *)(pCmp2))[0]) && \
                                      (((uint8_t *)(pCmp1))[1] == ((uint8_t *)(pCmp2))[1]))


/*! *********************************************************************************
* \brief  Returns the maximum value of arguments a and b.
*
* \return  The maximum value of arguments a and b
*
* \remarks
*   The primitive should must be implemented as a macro, as it should be possible to
*   evaluate the result on compile time if the arguments are constants.
*
********************************************************************************** */
#define FLib_GetMax(a,b)    (((a) > (b)) ? (a) : (b))


/*! *********************************************************************************
* \brief  Returns the minimum value of arguments a and b.
*
* \return  The minimum value of arguments a and b
*
* \remarks
*   The primitive should must be implemented as a macro, as it should be possible to
*   evaluate the result on compile time if the arguments are constants.
*
********************************************************************************** */
#define FLib_GetMin(a,b)    (((a) < (b)) ? (a) : (b))

#ifdef __GNUC__
#define _CLZ(x) __builtin_clz(x)
#endif
#if defined (__IAR_SYSTEMS_ICC__)
#define _CLZ(x) __CLZ(x)
#endif
#define _BSR(x) (31 - _CLZ(x))

static inline uint32_t Flib_Log2(uint32_t x)
{
  uint32_t msb = _BSR(x);
  /* if not a power of 2, round to next so that x remains <= (1<<msb) */
  if (x > (1<< msb))  msb++;
  return msb;
}


#define LOG_1(n) (((n) >= 2) ? 1 : 0)
#define LOG_2(n) (((n) >= 1<<2) ? (2 + LOG_1((n)>>2)) : LOG_1(n))
#define LOG_4(n) (((n) >= 1<<4) ? (4 + LOG_2((n)>>4)) : LOG_2(n))
#define LOG_8(n) (((n) >= 1<<8) ? (8 + LOG_4((n)>>8)) : LOG_4(n))
#define LOG(n)   (((n) >= 1<<16) ? (16 + LOG_8((n)>>16)) : LOG_8(n))




#define MASK_LOG(log) ((1<<(log))-1)
#define ROUND_FLOOR(x, log)  (((x)  >> (log)) << (log))
#define ROUND_CEIL(x, log)  ((((x) + MASK_LOG(log) ) >> (log)) << (log))
#define IS_POW_OF_TWO(x) (((x) & ((x)-1)) == 0)



#ifdef __cplusplus
}
#endif

#endif /* _FUNCTION_LIB_H_ */
