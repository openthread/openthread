/*! *********************************************************************************
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
* All rights reserved.
*
* \file
*
* This is the source file for the Memory Manager.
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */


/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "EmbeddedTypes.h"
#include "fsl_os_abstraction.h"
#include "Panic.h"
#include "MemManager.h"
#include "FunctionLib.h"

/*! *********************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
********************************************************************************** */

#define _block_size_  {
#define _number_of_blocks_  ,
#define _pool_id_(a) , a ,
#define _eol_  },


poolInfo_t poolInfo[] =
{
  PoolsDetails_c
  {0, 0, 0} /*termination tag*/
};

#undef _block_size_
#undef _number_of_blocks_
#undef _eol_
#undef _pool_id_

#define _block_size_ (sizeof(listHeader_t)+
#define _number_of_blocks_ ) *
#define _eol_  +
#define _pool_id_(a)

#define heapSize_c (PoolsDetails_c 0)

/* Heap */
uint32_t memHeap[heapSize_c/sizeof(uint32_t)];
const uint32_t heapSize = heapSize_c;

#undef _block_size_
#undef _number_of_blocks_
#undef _eol_
#undef _pool_id_

#define _block_size_ 0 *
#define _number_of_blocks_ + 0 *
#define _eol_  + 1 +
#define _pool_id_(a)

#define poolCount (PoolsDetails_c 0)

/* Memory pool info and anchors. */
pools_t memPools[poolCount];
#ifdef MEM_STATISTICS
pools_t  memPoolsSnapShot[poolCount];
#endif

#undef _block_size_
#undef _number_of_blocks_
#undef _eol_
#undef _pool_id_

#ifdef MEM_TRACKING

#ifndef NUM_OF_TRACK_PTR
#define NUM_OF_TRACK_PTR 1
#endif

#define _block_size_ 0*
#define _number_of_blocks_ +
#define _eol_  +
#define _pool_id_(a)

#define mTotalNoOfMsgs_d (PoolsDetails_c 0)
static const uint16_t mTotalNoOfMsgs_c = mTotalNoOfMsgs_d;
blockTracking_t memTrack[mTotalNoOfMsgs_d];

#undef _block_size_
#undef _number_of_blocks_
#undef _eol_
#undef _pool_id_

#endif /*MEM_TRACKING*/

/* Free messages counter. Not used by module. */
uint16_t gFreeMessagesCount;
#ifdef MEM_STATISTICS
uint16_t gFreeMessagesCountMin = 0xFFFF;
uint16_t gTotalFragmentWaste = 0;
uint16_t gMaxTotalFragmentWaste = 0;
#endif

/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
* \brief   This function initializes the message module private variables.
*          Must be called at boot time, or if device is reset.
*
* \param[in] none
*
* \return MEM_SUCCESS_c if initialization is successful. (It's always successful).
*
********************************************************************************** */
memStatus_t MEM_Init(void)
{
  poolInfo_t *pPoolInfo = poolInfo; /* IN: Memory layout information */
  pools_t *pPools = memPools;/* OUT: Will be initialized with requested memory pools. */
  uint8_t *pHeap = (uint8_t *)memHeap;/* IN: Memory heap.*/

  uint16_t poolN;
#ifdef MEM_TRACKING
  uint16_t memTrackIndex = 0;
#endif /*MEM_TRACKING*/

  gFreeMessagesCount = 0;

  for(;;)
  {
    poolN = pPoolInfo->poolSize;
    ListInit((listHandle_t)&pPools->anchor, poolN);
#ifdef MEM_STATISTICS
    pPools->poolStatistics.numBlocks = 0;
    pPools->poolStatistics.allocatedBlocks = 0;
    pPools->poolStatistics.allocatedBlocksPeak = 0;
    pPools->poolStatistics.allocationFailures = 0;
    pPools->poolStatistics.freeFailures = 0;
#ifdef MEM_TRACKING
    pPools->poolStatistics.poolFragmentWaste = 0;
    pPools->poolStatistics.poolFragmentWastePeak = 0;
    pPools->poolStatistics.poolFragmentMinWaste = 0xFFFF;
    pPools->poolStatistics.poolFragmentMaxWaste = 0;
#endif /*MEM_TRACKING*/
#endif /*MEM_STATISTICS*/

    while(poolN)
    {
      /* Add block to list of free memory. */
      ListAddTail((listHandle_t)&pPools->anchor, (listElementHandle_t)&((listHeader_t *)pHeap)->link);
      ((listHeader_t *)pHeap)->pParentPool = pPools;
#ifdef MEM_STATISTICS
      pPools->poolStatistics.numBlocks++;
#endif /*MEM_STATISTICS*/

      pPools->numBlocks++;
      gFreeMessagesCount++;
#ifdef MEM_TRACKING
      memTrack[memTrackIndex].blockAddr = (void *)(pHeap + sizeof(listHeader_t));
      memTrack[memTrackIndex].blockSize = pPoolInfo->blockSize;
      memTrack[memTrackIndex].fragmentWaste = 0;
      memTrack[memTrackIndex].allocAddr = NULL;
      memTrack[memTrackIndex].allocCounter = 0;
      memTrack[memTrackIndex].allocStatus = MEM_TRACKING_FREE_c;
      memTrack[memTrackIndex].freeAddr = NULL;
      memTrack[memTrackIndex].freeCounter = 0;
      memTrackIndex++;
#endif /*MEM_TRACKING*/

        /* Add block size (without list header)*/
      pHeap += pPoolInfo->blockSize + sizeof(listHeader_t);
      poolN--;
    }

    pPools->blockSize = pPoolInfo->blockSize;
    pPools->poolId = pPoolInfo->poolId;
    pPools->nextBlockSize = (pPoolInfo+1)->blockSize;
    if(pPools->nextBlockSize == 0)
    {
      break;
    }

    pPools++;
    pPoolInfo++;
  }

  return MEM_SUCCESS_c;
}

/*! *********************************************************************************
* \brief    This function returns the number of available blocks greater or
*           equal to the given size.
*
* \param[in] size - Size of blocks to check for availability.
*
* \return Number of available blocks greater or equal to the given size.
*
* \pre Memory manager must be previously initialized.
*
********************************************************************************** */
uint32_t MEM_GetAvailableBlocks
(
uint32_t size
)
{
    pools_t *pPools = memPools;
    uint32_t pTotalCount = 0;

    for(;;)
    {
        if(size <= pPools->blockSize)
        {
            pTotalCount += ListGetSize((listHandle_t)&pPools->anchor);
        }

        if(pPools->nextBlockSize == 0)
        {
            break;
        }

        pPools++;
    }

    return  pTotalCount;
}

/*! *********************************************************************************
* \brief     Allocate a block from the memory pools. The function uses the
*            numBytes argument to look up a pool with adequate block sizes.
* \param[in] numBytes - Size of buffer to allocate.
*
* \return Pointer to the allocated buffer, NULL if failed.
*
* \pre Memory manager must be previously initialized.
*
********************************************************************************** */

/*! *********************************************************************************
* \brief     Allocate a block from the memory pools. The function uses the
*            numBytes argument to look up a pool with adequate block sizes.
* \param[in] numBytes - Size of buffer to allocate.
* \param[in] poolId - The ID of the pool where to search for a free buffer.
* \param[in] pCaller - pointer to the caller function (Debug purpose)
*
* \return Pointer to the allocated buffer, NULL if failed.
*
* \pre Memory manager must be previously initialized.
*
********************************************************************************** */
void* MEM_BufferAllocWithId
(
uint32_t numBytes,
uint8_t  poolId,
void *pCaller
)
{
#ifdef MEM_TRACKING
    /* Save the Link Register */
    volatile uint32_t savedLR = (uint32_t) __get_LR();
#endif
#if defined(MEM_TRACKING) || defined(MEM_DEBUG_OUT_OF_MEMORY)
    uint16_t requestedSize = numBytes;
#endif /*MEM_TRACKING*/
#ifdef MEM_STATISTICS
    bool_t allocFailure = FALSE;
#endif

    pools_t *pPools = memPools;
    listHeader_t *pBlock;

    OSA_InterruptDisable();

    while(numBytes)
    {
        if( (numBytes <= pPools->blockSize) && (poolId == pPools->poolId) )
        {
            pBlock = (listHeader_t *)ListRemoveHead((listHandle_t)&pPools->anchor);

            if(NULL != pBlock)
            {
                pBlock++;
                gFreeMessagesCount--;
                pPools->allocatedBlocks++;

#ifdef MEM_STATISTICS
                if(gFreeMessagesCount < gFreeMessagesCountMin)
                {
                    gFreeMessagesCountMin = gFreeMessagesCount;
                }

                pPools->poolStatistics.allocatedBlocks++;
                if ( pPools->poolStatistics.allocatedBlocks > pPools->poolStatistics.allocatedBlocksPeak )
                {
                    pPools->poolStatistics.allocatedBlocksPeak = pPools->poolStatistics.allocatedBlocks;
                }
                MEM_ASSERT(pPools->poolStatistics.allocatedBlocks <= pPools->poolStatistics.numBlocks);
#endif /*MEM_STATISTICS*/

#ifdef MEM_TRACKING
                MEM_Track(pBlock, MEM_TRACKING_ALLOC_c, savedLR, requestedSize, pCaller);
#endif /*MEM_TRACKING*/
                OSA_InterruptEnable();
                return pBlock;
            }
            else
            {
#ifdef MEM_STATISTICS
                if(!allocFailure)
                {
                    pPools->poolStatistics.allocationFailures++;
                    allocFailure = TRUE;
                }
#endif /*MEM_STATISTICS*/
                if(numBytes > pPools->nextBlockSize)
                {
                    break;
                }
                /* No more blocks of that size, try next size. */
                numBytes = pPools->nextBlockSize;
            }
        }
        /* Try next pool*/
        if(pPools->nextBlockSize)
        {
            pPools++;
        }
        else
        {
            numBytes = 0;
        }
    }

#ifdef MEM_DEBUG_OUT_OF_MEMORY
    if (requestedSize)
    {
        panic( 0, (uint32_t)MEM_BufferAllocWithId, 0, 0);
    }
#endif

    OSA_InterruptEnable();
    return NULL;
}

/*! *********************************************************************************
* \brief     Deallocate a memory block by putting it in the corresponding pool
*            of free blocks.
*
* \param[in] buffer - Pointer to buffer to deallocate.
*
* \return MEM_SUCCESS_c if deallocation was successful, MEM_FREE_ERROR_c if not.
*
* \pre Memory manager must be previously initialized.
*
* \remarks Never deallocate the same buffer twice.
*
********************************************************************************** */
#if (defined MULTICORE_MEM_MANAGER) && ((defined MULTICORE_HOST) || (defined MULTICORE_BLACKBOX))
#if defined MULTICORE_HOST
memStatus_t MEM_BufferFreeOnMaster
#elif defined MULTICORE_BLACKBOX
memStatus_t MEM_BufferFreeOnSlave
#endif /* MULTICORE_HOST */
(
    uint8_t *pBuff
)
{
#ifdef MEM_TRACKING
    /* Save the Link Register */
    volatile uint32_t savedLR = (uint32_t) __get_LR();
#endif /*MEM_TRACKING*/
    listHeader_t *pHeader;

    if( pBuff == NULL )
    {
        return MEM_FREE_ERROR_c;
    }

    pHeader = (listHeader_t *)pBuff-1;

    if( ((uint8_t*)pHeader < (uint8_t*)memHeap) || ((uint8_t*)pHeader > ((uint8_t*)memHeap + sizeof(memHeap))) )
    {
#ifdef MEM_DEBUG_INVALID_POINTERS
        panic( 0, (uint32_t)MEM_BufferFree, 0, 0);
#endif
        return MEM_FREE_ERROR_c;
    }
    else
    {
        return MEM_BufferFree(pBuff);
    }
}
#endif /* (defined MULTICORE_MEM_MANAGER) && ((defined MULTICORE_HOST) || (defined MULTICORE_BLACKBOX)) */

memStatus_t MEM_BufferFree
(
void* buffer /* IN: Block of memory to free*/
)
{
#ifdef MEM_TRACKING
    /* Save the Link Register */
    volatile uint32_t savedLR = (uint32_t) __get_LR();
#endif /*MEM_TRACKING*/
    listHeader_t *pHeader;
    pools_t *pParentPool;
    pools_t *pool;

    if( buffer == NULL )
    {
        return MEM_FREE_ERROR_c;
    }

    pHeader = (listHeader_t *)buffer-1;

    if( ((uint8_t*)pHeader < (uint8_t*)memHeap) || ((uint8_t*)pHeader > ((uint8_t*)memHeap + sizeof(memHeap))) )
    {
#if (defined MULTICORE_MEM_MANAGER) && (defined MULTICORE_BLACKBOX)
        return MEM_BufferFreeOnMaster(buffer);
#elif (defined MULTICORE_MEM_MANAGER) && (defined MULTICORE_HOST)
        return MEM_BufferFreeOnSlave(buffer);
#else
#ifdef MEM_DEBUG_INVALID_POINTERS
        panic( 0, (uint32_t)MEM_BufferFree, 0, 0);
#endif
        return MEM_FREE_ERROR_c;
#endif
    }

    OSA_InterruptDisable();

    pParentPool = (pools_t *)pHeader->pParentPool;
    pool = memPools;

    for(;;)
    {
        if (pParentPool == pool)
        {
            break;
        }
        if(pool->nextBlockSize == 0)
        {
            /* The parent pool was not found! This means that the memory buffer is corrupt or
            that the MEM_BufferFree() function was called with an invalid parameter */
#ifdef MEM_STATISTICS
            pParentPool->poolStatistics.freeFailures++;
#endif /*MEM_STATISTICS*/
            OSA_InterruptEnable();
#ifdef MEM_DEBUG_INVALID_POINTERS
            panic( 0, (uint32_t)MEM_BufferFree, 0, 0);
#endif
            return MEM_FREE_ERROR_c;
        }
        pool++;
    }

    if( pHeader->link.list != NULL )
    {
        /* The memory buffer appears to be enqueued in a linked list.
        This list may be the free memory buffers pool, or another list. */
#ifdef MEM_STATISTICS
        pParentPool->poolStatistics.freeFailures++;
#endif /*MEM_STATISTICS*/
        OSA_InterruptEnable();
#ifdef MEM_DEBUG_INVALID_POINTERS
        panic( 0, (uint32_t)MEM_BufferFree, 0, 0);
#endif
        return MEM_FREE_ERROR_c;
    }

    gFreeMessagesCount++;

    ListAddTail((listHandle_t)&pParentPool->anchor, (listElementHandle_t)&pHeader->link);
    pParentPool->allocatedBlocks--;

#ifdef MEM_STATISTICS
    MEM_ASSERT(pParentPool->poolStatistics.allocatedBlocks > 0);
    pParentPool->poolStatistics.allocatedBlocks--;
#endif /*MEM_STATISTICS*/

#ifdef MEM_TRACKING
    MEM_Track(buffer, MEM_TRACKING_FREE_c, savedLR, 0, NULL);
#endif /*MEM_TRACKING*/
    OSA_InterruptEnable();
    return MEM_SUCCESS_c;
}

/*! *********************************************************************************
* \brief     Determines the size of a memory block
*
* \param[in] buffer - Pointer to buffer.
*
* \return size of memory block
*
* \pre Memory manager must be previously initialized.
*
********************************************************************************** */
uint16_t MEM_BufferGetSize
(
void* buffer /* IN: Block of memory to free*/
)
{
    if( buffer )
    {
        return ((pools_t *)((listHeader_t *)buffer-1)->pParentPool)->blockSize;
    }

    return 0;
}

/*! *********************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
********************************************************************************** */
/*! *********************************************************************************
* \brief     This function updates the tracking array element corresponding to the given
*            block.
*
* \param[in] block - Pointer to the block.
* \param[in] alloc - Indicates whether an allocation or free operation was performed
* \param[in] address - Address where MEM_BufferAlloc or MEM_BufferFree was called
* \param[in] requestedSize - Indicates the requested buffer size  passed to MEM_BufferAlloc.
*                            Has no use if a free operation was performed.
*
* \return Returns TRUE if correct allocation or dealocation was performed, FALSE if a
*         buffer was allocated or freed twice.
*
********************************************************************************** */
#ifdef MEM_TRACKING
uint8_t MEM_Track(listHeader_t *block, memTrackingStatus_t alloc, uint32_t address, uint16_t requestedSize, void *pCaller)
{
  uint16_t i;
  blockTracking_t *pTrack = NULL;
#ifdef MEM_STATISTICS
  poolStat_t * poolStatistics = (poolStat_t *)&((pools_t *)( (listElementHandle_t)(block-1)->pParentPool ))->poolStatistics;
#endif

  for( i=0; i<mTotalNoOfMsgs_c; i++ )
  {
      if( block == memTrack[i].blockAddr )
      {
          pTrack = &memTrack[i];
          break;
      }
  }

  if( !pTrack || pTrack->allocStatus == alloc)
  {
#ifdef MEM_DEBUG
      panic( 0, (uint32_t)MEM_Track, 0, 0);
#endif
      return FALSE;
  }

  pTrack->allocStatus = alloc;
  pTrack->pCaller = (void*)((uint32_t)pCaller & 0x7FFFFFFF);

  if(alloc == MEM_TRACKING_ALLOC_c)
  {
    pTrack->fragmentWaste = pTrack->blockSize - requestedSize;
    pTrack->allocCounter++;
    pTrack->allocAddr = (void *)address;
    if( (uint32_t)pCaller & 0x80000000 )
    {
        pTrack->timeStamp = 0xFFFFFFFF;
    }
    else
    {
        pTrack->timeStamp = MEM_GetTimeStamp();
    }
#ifdef MEM_STATISTICS
    gTotalFragmentWaste += pTrack->fragmentWaste;
    if(gTotalFragmentWaste > gMaxTotalFragmentWaste)
    {
        gMaxTotalFragmentWaste = gTotalFragmentWaste;
        FLib_MemCpy(&memPoolsSnapShot[0], &memPools[0], sizeof(memPools));
    }

    poolStatistics->poolFragmentWaste += pTrack->fragmentWaste;
    if(poolStatistics->poolFragmentWaste > poolStatistics->poolFragmentWastePeak)
      poolStatistics->poolFragmentWastePeak = poolStatistics->poolFragmentWaste;
    if(pTrack->fragmentWaste < poolStatistics->poolFragmentMinWaste)
    {
      poolStatistics->poolFragmentMinWaste = pTrack->fragmentWaste;
    }
    if(pTrack->fragmentWaste > poolStatistics->poolFragmentMaxWaste)
    {
      poolStatistics->poolFragmentMaxWaste = pTrack->fragmentWaste;
    }

    if(poolStatistics->allocatedBlocks == poolStatistics->numBlocks)
    {
          //__asm("BKPT #0\n") ; /* cause the debugger to stop */
    }
#endif /*MEM_STATISTICS*/
  }
  else
  {
#ifdef MEM_STATISTICS
    poolStatistics->poolFragmentWaste -= pTrack->fragmentWaste;
    gTotalFragmentWaste -= pTrack->fragmentWaste;
#endif /*MEM_STATISTICS*/

    pTrack->fragmentWaste = 0;
    pTrack->freeCounter++;
    pTrack->freeAddr = (void *)address;
    pTrack->timeStamp = 0;
  }

  return TRUE;
}

/*! *********************************************************************************
* \brief     This function checks for buffer overflow when copying multiple bytes
*
* \param[in] p    - pointer to destination.
* \param[in] size - number of bytes to copy
*
* \return 1 if overflow detected, else 0
*
********************************************************************************** */
uint8_t MEM_BufferCheck(uint8_t *p, uint32_t size)
{
    poolInfo_t *pPollInfo = poolInfo;
    uint8_t* memAddr = (uint8_t *)memHeap;
    uint32_t poolBytes, blockBytes, i;

    if( (p < (uint8_t*)memHeap) || (p > ((uint8_t*)memHeap + sizeof(memHeap))) )
    {
        return 0;
    }

    while( pPollInfo->blockSize )
    {
        blockBytes = pPollInfo->blockSize + sizeof(listHeader_t);
        poolBytes  = blockBytes * pPollInfo->poolSize;

        /* Find the correct message pool */
        if( (p >= memAddr) && (p < memAddr + poolBytes) )
        {
            /* Check if the size to copy is greather then the size of the current block */
            if( size > pPollInfo->blockSize )
            {
#ifdef MEM_DEBUG
                panic(0,0,0,0);
#endif
                return 1;
            }

            /* Find the correct memory block */
            for( i=0; i<pPollInfo->poolSize; i++ )
            {
                if( (p >= memAddr) && (p < memAddr + blockBytes) )
                {
                    if( p + size > memAddr + blockBytes )
                    {
#ifdef MEM_DEBUG
                        panic(0,0,0,0);
#endif
                        return 1;
                    }
                    else
                    {
                        return 0;
                    }
                }

                memAddr += blockBytes;
            }
        }

        /* check next pool */
        memAddr += poolBytes;
        pPollInfo++;
    }

    return 0;
}
#endif /*MEM_TRACKING*/


/*! *********************************************************************************
* \brief     This function checks if the buffers are allocated for more than the
*            specified duration
*
********************************************************************************** */
#ifdef MEM_TRACKING
void MEM_CheckIfMemBuffersAreFreed(void)
{
    uint32_t t;
    uint16_t i, j = 0;
    uint8_t trackCount = 0;
    pools_t *pParentPool;
    volatile blockTracking_t *pTrack;
    static volatile blockTracking_t *mpTrackTbl[NUM_OF_TRACK_PTR];
    static uint32_t lastTimestamp = 0;
    uint32_t currentTime = MEM_GetTimeStamp();

    if( (currentTime - lastTimestamp) >= MEM_CheckMemBufferInterval_c )
    {
        lastTimestamp = currentTime;
        j = 0;

        for( i = 0; i < mTotalNoOfMsgs_c; i++ )
        {
            pTrack = &memTrack[i];

            /* Validate the pParent first */
            pParentPool = (((listHeader_t *)(pTrack->blockAddr))-1)->pParentPool;
            if(pParentPool != &memPools[j])
            {
                if(j < NumberOfElements(memPools))
                {
                    j++;
                    if(pParentPool != &memPools[j])
                    {
                        panic(0,0,0,0);
                    }
                }
                else
                {
                    panic(0,0,0,0);
                }
            }

            /* Check if it should be freed  */
            OSA_InterruptDisable();
            if((pTrack->timeStamp != 0xffffffff ) &&
               (pTrack->allocStatus == MEM_TRACKING_ALLOC_c) &&
               (currentTime > pTrack->timeStamp))
            {
                t = currentTime - pTrack->timeStamp;
                if( t > MEM_CheckMemBufferThreshold_c )
                {
                    mpTrackTbl[trackCount++] = pTrack;
                    if(trackCount == NUM_OF_TRACK_PTR)
                    {
                        (void)mpTrackTbl; /* remove compiler warnings */
                        panic(0,0,0,0);
                        break;
                    }
                }
            }
            OSA_InterruptEnable();
        } /* end for */
    }
}
#endif /*MEM_TRACKING*/


/*! *********************************************************************************
* \brief     Get time-stamp for memory operation: alloc/free.
*
* \return dymmy time-stamp
*
********************************************************************************** */
#ifdef MEM_TRACKING
#if defined(__IAR_SYSTEMS_ICC__)
__weak uint32_t MEM_GetTimeStamp(void)
#elif defined(__GNUC__)
__attribute__((weak)) uint32_t MEM_GetTimeStamp(void)
#endif
{
    return 0xFFFFFFFF;
}
#endif /* MEM_TRACKING */


/*! ************************************************************************************************
\brief  MEM Manager calloc alternative implementation.

\param  [in]    len                     Number of blocks
\param  [in]    size                    Size of one block

\return         void*                   Pointer to the allocated buffer or NULL in case of error
************************************************************************************************* */
void* MEM_CallocAlt
(
    size_t len,
    size_t val
)
{
    void *pData = MEM_BufferAllocForever (len* val, 0);
    if (pData)
    {
        FLib_MemSet (pData, 0, len* val);
    }

    return pData;
}

/*! ************************************************************************************************
\brief  MEM Manager free alternative implementation.

\param  [in]    pData                   Pointer to the allocated buffer

\return         void
************************************************************************************************* */
void MEM_FreeAlt
(
    void* pData
)
{
    MEM_BufferFree (pData);
}
