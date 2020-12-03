/*! *********************************************************************************
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
* All rights reserved.
*
* \file
*
* This module contains various common functions like copy and compare routines.
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

#include "FunctionLib.h"

#if gUseToolchainMemFunc_d
#include <string.h>
#endif

#if gFLib_CheckBufferOverflow_d
#include "MemManager.h"
#endif

/*! *********************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Private prototypes
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
* \brief  This function copies bytes from one buffer to another.
*         The buffers should not overlap.
*
* \param[in, out]  pDst Pointer to the destination buffer.
*
* \param[in]  pSrc Pointer to the source buffer.
*
* \param[in]  cBytes Number of bytes to copy.
*
* \post  The source and destination buffers must not overlap.
*
* \remarks
*
********************************************************************************** */
void FLib_MemCpy (void* pDst,
                  const void* pSrc,
                  uint32_t cBytes)
{
#if gFLib_CheckBufferOverflow_d && defined(MEM_TRACKING)
    (void)MEM_BufferCheck(pDst, cBytes);
#endif

#if gUseToolchainMemFunc_d
    memcpy(pDst, pSrc, cBytes);
#else
    while (cBytes)
    {
        *((uint8_t*)pDst) = *((uint8_t*)pSrc);
        pDst = ((uint8_t*)pDst)+1;
        pSrc = ((uint8_t*)pSrc)+1;
        cBytes--;
    }
#endif
}

/*! *********************************************************************************
* \brief  This function copies the specified number of bytes from the
*         source address to the destination address.  No attempt is made
*         to handle overlapping copies to prevent loss of data.
*         The copying is optimized to avoid alignment problems, and attempts
*         to copy 32bit numbers optimally.
*
* \param[in]  from_ptr Pointer to the source buffer.
*
* \param[in, out]  to_ptr Pointer to the destination buffer.
*
* \param[in]  number_of_bytes  Number of bytes to copy (32 bit value).
*
* \post
*
* \remarks
*
********************************************************************************** */
void FLib_MemCpyAligned32bit (void* to_ptr,
                              const void* from_ptr,
                              register uint32_t number_of_bytes)
{
    uint8_t*    from8_ptr = (uint8_t*)from_ptr;
    uint8_t*    to8_ptr = (uint8_t*)to_ptr;
    uint16_t*   from16_ptr = (uint16_t*)from_ptr;
    uint16_t*   to16_ptr = (uint16_t*)to_ptr;
    register    uint32_t* from32_ptr = (uint32_t*)from_ptr;
    register    uint32_t* to32_ptr = (uint32_t*)to_ptr;

    register    uint32_t loops;

#if gFLib_CheckBufferOverflow_d && defined(MEM_TRACKING)
    (void)MEM_BufferCheck(to_ptr, number_of_bytes);
#endif

    if (number_of_bytes > 3)
    {
        /* Try to align source on word */
        if ((uint32_t)from_ptr & 1)
        {
            from8_ptr = (uint8_t*)from_ptr;
            to8_ptr = (uint8_t*)to_ptr;

            *to8_ptr++ = *from8_ptr++;

            from_ptr = from8_ptr;
            to_ptr = to8_ptr;
            --number_of_bytes;
        }

        /* Try to align source on longword */
        if ((uint32_t)from_ptr & 2)
        {
            from16_ptr = (uint16_t*)from_ptr;
            to16_ptr = (uint16_t*)to_ptr;

            *to16_ptr++ = *from16_ptr++;

            from_ptr = from16_ptr;
            to_ptr = to16_ptr;
            number_of_bytes -= 2;
        }

        from32_ptr = (uint32_t*)from_ptr;
        to32_ptr = (uint32_t*)to_ptr;

        for (loops = number_of_bytes >> 2; loops != 0; loops--)
        {
            *to32_ptr++ = *from32_ptr++;
        }

        from_ptr = from32_ptr;
        to_ptr = to32_ptr;
    }

    /* Copy all remaining bytes */
    if (number_of_bytes & 2)
    {
        from16_ptr = (uint16_t*)from_ptr;
        to16_ptr = (uint16_t*)to_ptr;

        *to16_ptr++ = *from16_ptr++;

        from_ptr = from16_ptr;
        to_ptr = to16_ptr;
    }

    if (number_of_bytes & 1)
    {
        *(uint8_t*)to_ptr = *(uint8_t*)from_ptr;
    }
}


/*! *********************************************************************************
* \brief  Copy bytes from one buffer to another. The buffers should not overlap.
*         The function can copy in either direction. If 'dir' is TRUE, then the function
*         works like FLib_MemCpy(). If FALSE, the function swaps the buffer pointers
*         before copying.
*
* \param[in, out]  pBuf1 Pointer to the destination/source buffer.
*
* \param[in, out]  pBuf2 Pointer to the source/destination buffer.
*
* \param[in]  dir Direction to copy: pBuf2->pBuf1 if TRUE, pBuf1->pBuf2 if FALSE
*
* \param[in]  n Number of bytes to copy.
*
* \post  The source and destination buffers must not overlap.
*
* \remarks
*
********************************************************************************** */
void FLib_MemCpyDir (void* pBuf1,
                     void* pBuf2,
                     bool_t dir,
                     uint32_t n)
{
    if (dir)
    {
        FLib_MemCpy (pBuf1, pBuf2, n);
    }
    else
    {
        FLib_MemCpy (pBuf2, pBuf1, n);
    }
}


/*! *********************************************************************************
* \brief  The byte at index i from the source buffer is copied to index ((n-1) - i)
*         in the destination buffer (and vice versa).
*
* \param[in, out]  pDst Pointer to the destination buffer.
*
* \param[in]  pSrc Pointer to the source buffer.
*
* \param[in]  cBytes Number of bytes to copy.
*
* \post
*
* \remarks
*
********************************************************************************** */
void FLib_MemCpyReverseOrder (void* pDst,
                              const void* pSrc,
                              uint32_t cBytes)
{
#if gFLib_CheckBufferOverflow_d && defined(MEM_TRACKING)
    (void)MEM_BufferCheck(pDst, cBytes);
#endif
    if(cBytes)
    {
        pDst = (uint8_t*)pDst + (uint32_t)(cBytes-1);
        while (cBytes)
        {
            *((uint8_t*)pDst) = *((uint8_t*)pSrc);
            pDst = (uint8_t*)pDst-1;
            pSrc = (uint8_t*)pSrc+1;
            cBytes--;
        }
    }
}


/*! *********************************************************************************
* \brief  This function compares two buffers.
*
* \param[in]  pData1  First buffer to compare.
*
* \param[in]  pData2  Second buffer to compare.
*
* \param[in]  cBytes Number of bytes to compare.
*
* \return  This function return TRUE if the buffers are equal and FALSE otherwise.
*
* \post
*
* \remarks
*
********************************************************************************** */
bool_t FLib_MemCmp (const void* pData1,    /* IN: First memory block to compare */
                    const void* pData2,    /* IN: Second memory block to compare */
                    uint32_t cBytes  /* IN: Number of bytes to compare. */
                   )
{
    bool_t status = TRUE;
#if gUseToolchainMemFunc_d
    if( memcmp(pData1, pData2, cBytes) )
    {
        status = FALSE;
    }
#else
    while (cBytes)
    {
        if ( *((uint8_t *)pData1) != *((uint8_t *)pData2))
        {
            status = FALSE;
            break;
        }

        pData2 = (uint8_t* )pData2+1;
        pData1 = (uint8_t* )pData1+1;
        cBytes--;
    }
#endif
    return status;
}

/*! *********************************************************************************
* \brief  This function compares each octet of a given location to a value.
*
* \param [in]     pAddr      location to be compared
*
* \param [in]     val        reference value
*
* \param [in]     len        length of location to be compared
*
* \return  This function return TRUE if all octests match and FALSE otherwise.
*
* \post
*
* \remarks
*
********************************************************************************** */
bool_t FLib_MemCmpToVal
(
    const void* pAddr,
    uint8_t val,
    uint32_t len
)
{
    while(len)
    {
        len--;

        if(((uint8_t *)pAddr)[len] != val)
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*! *********************************************************************************
* \brief  This function resets all bytes in a specified buffer to a set value.
*
* \param[in,out]  pDst  Address of the buffer to set.
*
* \param[in]  value  Set value.
*
* \param[in]  cBytes Number of bytes to set in the buffer (maximum 255 bytes).
*
* \post
*
* \remarks
*
********************************************************************************** */
void FLib_MemSet (void* pData,
                  uint8_t value,
                  uint32_t cBytes)
{
#if gFLib_CheckBufferOverflow_d && defined(MEM_TRACKING)
    (void)MEM_BufferCheck(pData, cBytes);
#endif
#if gUseToolchainMemFunc_d
    memset(pData, value, cBytes);
#else
    while (cBytes)
    {
        ((uint8_t* )pData)[--cBytes] = value;
    }
#endif
}


/*! *********************************************************************************
* \brief  This function copies a buffer,
*         possibly into the same overlapping memory as it is taken from
*
* \param[in, out]  pDst Pointer to the destination buffer.
*
* \param[in]  pSrc Pointer to the source buffer.
*
* \param[in]  cBytes Number of bytes to copy.
*
* \post
*
* \remarks
*
********************************************************************************** */
void FLib_MemInPlaceCpy (void* pDst,
                         void* pSrc,
                         uint32_t cBytes)
{
#if gFLib_CheckBufferOverflow_d && defined(MEM_TRACKING)
    (void)MEM_BufferCheck(pDst, cBytes);
#endif
    if (pDst != pSrc)
    {
        /* Do nothing if copying to same position */
        if (pDst < pSrc)
        {
            /* If dst is before src in memory copy forward */
            while (cBytes)
            {
                *((uint8_t*)pDst) = *((uint8_t*)pSrc);
                pDst = ((uint8_t*)pDst)+1;
                pSrc = ((uint8_t*)pSrc)+1;
                cBytes--;
            }
        }
        else
        {
            /* If dst is after src in memory copy backward */
            while(cBytes)
            {
                cBytes--;
                ((uint8_t* )pDst)[cBytes] = ((uint8_t* )pSrc)[cBytes];
            }
        }
    }
}

/*! *********************************************************************************
* \brief This function copies a 16bit value to an unaligned a memory block.
*
* \param[in, out]  pDst Pointer to the destination memory block.
*
* \param[in]  val16 The value to be copied.
*
********************************************************************************** */
void FLib_MemCopy16Unaligned (void* pDst,
                              uint16_t val16)
{
    uint8_t* pData = (uint8_t*)pDst;

    *pData++ = (uint8_t)(val16);
    *pData =   (uint8_t)(val16 >> 8);

    return;
}


/*! *********************************************************************************
* \brief This function copies a 32bit value to an unaligned a memory block.
*
* \param[in, out]  pDst Pointer to the destination memory block.
*
* \param[in]  val32 The value to be copied.
*
********************************************************************************** */
void FLib_MemCopy32Unaligned (void* pDst,
                              uint32_t val32)
{
    uint8_t* pData = (uint8_t*)pDst;

    *pData++ = (uint8_t)(val32);
    *pData++ = (uint8_t)(val32 >> 8);
    *pData++ = (uint8_t)(val32 >> 16);
    *pData++ = (uint8_t)(val32 >> 24);

    return;
}


/*! *********************************************************************************
* \brief This function copies a 64bit value to an unaligned a memory block.
*
* \param[in, out]  pDst Pointer to the destination memory block.
*
* \param[in]  val64 The value to be copied.
*
********************************************************************************** */
void FLib_MemCopy64Unaligned (void* pDst,
                              uint64_t val64)
{
    uint8_t* pData = (uint8_t*)pDst;

    *pData++ = (uint8_t)(val64);
    *pData++ = (uint8_t)(val64 >> 8);
    *pData++ = (uint8_t)(val64 >> 16);
    *pData++ = (uint8_t)(val64 >> 24);
    *pData++ = (uint8_t)(val64 >> 32);
    *pData++ = (uint8_t)(val64 >> 40);
    *pData++ = (uint8_t)(val64 >> 48);
    *pData = (uint8_t)(val64 >> 56);

    return;
}


/*! *********************************************************************************
* \brief  This function adds an offset to a pointer.
*
* \param[in,out]  pPtr  Pointer to the pointer to add the offset to
*
* \param[in]  offset  Offset to add to the specified pointer.
*
* \post
*
* \remarks
*
********************************************************************************** */
void FLib_AddOffsetToPointer (void** pPtr,
                              uint32_t offset)
{
    (*pPtr) = ((uint8_t* )*pPtr) + offset;
}


/*! *********************************************************************************
* \brief  This function returns the length of a NULL terminated string.
*
* \param[in]  str  A NULL terminated string
*
* \return  the size of string in bytes
*
********************************************************************************** */
uint32_t FLib_StrLen(const char *str)
{
#if gUseToolchainMemFunc_d
     return strlen(str);
#else
    register uint32_t len=0;

    while(*str != '\0')
    {
        str++;
        len++;
    }

    return len;
#endif

}

#ifdef CPU_JN518X
/*! *********************************************************************************
* \brief  This function copies bytes from FLASH to ram.
*
* \param[in, out]  pDst Pointer to the destination buffer.
*
* \param[in]  pSrc Pointer to the source buffer.
*
* \param[in]  cBytes Number of bytes to copy.
*
* \remarks
*
********************************************************************************** */
bool_t FLib_CopyFromFlash(void* pDst,
                        const void* pSrc,
                        uint32_t cBytes)
{
    bool_t raise_error = FALSE;

    TRY
    {
        FLib_MemCpy(pDst, pSrc, cBytes);
    }
    CATCH (BUS_EXCEPTION)
    {
        raise_error = TRUE;
    }
    YRT;

    return raise_error;
}
#else
bool_t FLib_CopyFromFlash(void* pDst,
                          const void* pSrc,
                          uint32_t cBytes)
{
    FLib_MemCpy(pDst, pSrc, cBytes);
    return false;
}

#endif
