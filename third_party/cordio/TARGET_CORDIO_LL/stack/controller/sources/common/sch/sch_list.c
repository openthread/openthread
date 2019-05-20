/* Copyright (c) 2009-2019 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*************************************************************************************************/
/*!
 *  \brief Operation list maintenance implementation file.
 */
/*************************************************************************************************/

#include "sch_int.h"
#include "wsf_assert.h"
#include "wsf_cs.h"
#include "wsf_math.h"
#include "wsf_trace.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief      Total BOD time including setup delay. */
#define SCH_TOTAL_DUR(p)        (p->minDurUsec + BbGetSchSetupDelayUs())

/*! \brief      Time immediately after the given BOD. */
#define SCH_END_TIME(p)         (p->due + BB_US_TO_BB_TICKS(SCH_TOTAL_DUR(p)))

/*! \brief      Is BOD[a] due time before BOD[b] due time (rt = reference time). */
#define SCH_IS_DUE_BEFORE(a, b, rt)     ((((a)->due) - (rt)) < (((b)->due) - (rt)))

/*! \brief      Is BOD[a] completion time before BOD[b] due time (rt = reference time). */
#define SCH_IS_DONE_BEFORE(a, b, rt)    ((SCH_END_TIME(a) - (rt)) <= (((b)->due) - (rt)))

/*! \brief      Is BOD[a] due time after BOD[b] completion time. */
#define SCH_IS_DUE_AFTER(a, b, rt)      SCH_IS_DONE_BEFORE(b, a, rt)

#ifndef SCH_TRACE_ENABLE
/*! \brief      Enable scheduler trace. */
#define SCH_TRACE_ENABLE  FALSE
#endif

#if SCH_TRACE_ENABLE
#define SCH_TRACE_INFO0(msg)                        WSF_TRACE0("SCH", "INFO", msg)
#define SCH_TRACE_INFO1(msg, var1)                  WSF_TRACE1("SCH", "INFO", msg, var1)
#define SCH_TRACE_WARN0(msg)                        WSF_TRACE0("SCH", "WARN", msg)
#define SCH_TRACE_WARN1(msg, var1)                  WSF_TRACE1("SCH", "WARN", msg, var1)
#else
/*! \brief      Information trace with 0 parameters. */
#define SCH_TRACE_INFO0(msg)
/*! \brief      Information trace with 1 parameters. */
#define SCH_TRACE_INFO1(msg, var1)
/*! \brief      Warning trace with 0 parameters. */
#define SCH_TRACE_WARN0(msg)
/*! \brief      Warning trace with 1 parameters. */
#define SCH_TRACE_WARN1(msg, var1)
#endif

#ifndef SCH_CHECK_LIST_INTEGRITY
/*! \brief      Check list requirements upon insertions and removals. */
#define SCH_CHECK_LIST_INTEGRITY                    FALSE
#endif

#if (SCH_CHECK_LIST_INTEGRITY)
/*************************************************************************************************/
/*!
 *  \brief      Ensure BOD is not already inserted in the list.
 *
 *  \param      pBod    Target BOD.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static inline void SchCheckIsNotInserted(BbOpDesc_t *pBod)
{
  BbOpDesc_t *pCur = schCb.pHead;

  while (pCur != NULL)
  {
    WSF_ASSERT(pCur != pBod);
    pCur = pCur->pNext;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Ensure BOD is already inserted in the list.
 *
 *  \param      pBod    Target BOD.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static inline void SchCheckIsInserted(BbOpDesc_t *pBod)
{
  BbOpDesc_t *pCur = schCb.pHead;

  while (pCur != NULL)
  {
    if (pCur == pBod)
    {
      /* pBod found in the list. */
      return;
    }
    pCur = pCur->pNext;
  }
  WSF_ASSERT(pCur != NULL);
}
#endif

/*************************************************************************************************/
/*!
 *  \brief      Is BOD due within next BOD if not already started.
 *
 *  \param      pBod    Target BOD.
 *
 *  \return     TRUE if BOD is cancelable, FALSE otherwise.
 */
/*************************************************************************************************/
static inline bool_t SchEnoughTimeToCancel(BbOpDesc_t *pBod)
{
  bool_t result = FALSE;

  const uint32_t curTime = BbDrvGetCurrentTime();
  const uint32_t delta = pBod->due - curTime;

  if ((delta >= BB_US_TO_BB_TICKS(BbGetSchSetupDelayUs())) &&   /* sufficient time to cancel */
      (delta < SCH_MAX_SPAN))                                   /* due time has not passed */
  {
    result = TRUE;
  }

  return result;
}

/*************************************************************************************************/
/*!
 *  \brief      Get start reference time.
 *
 *  \return     Start reference time.
 *
 *  Returns the earliest time as a time base for calculations.
 */
/*************************************************************************************************/
static inline uint32_t SchGetStartRefTime(void)
{
  if (schCb.pHead)
  {
    return schCb.pHead->due - SCH_MAX_SPAN;
  }
  else
  {
    return BbDrvGetCurrentTime();
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Cancel current head operation.
 *
 *  \return     TRUE if successful, FALSE otherwise.
 */
/*************************************************************************************************/
static inline bool_t schCancelHead(void)
{
  bool_t result = FALSE;

  if (schCb.state == SCH_STATE_EXEC)
  {
    if ((result = SchEnoughTimeToCancel(schCb.pHead)) == TRUE)
    {
      BbCancelBod();

      schCb.state = SCH_STATE_IDLE;
    }
  }
  else
  {
    /* If not the current BOD then cancel is unconditional. */
    result = TRUE;
  }

  return result;
}

/*************************************************************************************************/
/*!
 *  \brief      Insert item into an empty list.
 *
 *  \param      pItem   Item to insert.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static inline void schInsertToEmptyList(BbOpDesc_t *pItem)
{
  WSF_ASSERT(pItem);
  WSF_ASSERT(schCb.pHead == NULL);
  WSF_ASSERT(schCb.pTail == NULL);

  schCb.pHead = pItem;
  schCb.pTail = pItem;

  pItem->pPrev = NULL;
  pItem->pNext = NULL;

  SCH_TRACE_INFO1("++| schInsertToEmptyList |++ pBod=0x%08x", (uint32_t)pItem);
  SCH_TRACE_INFO1("++|                      |++     .due=%u", pItem->due);
  SCH_TRACE_INFO1("++|                      |++     .minDurUsec=%u", pItem->minDurUsec);
  SCH_TRACE_INFO1("++|                      |++     .maxDurUsec=%u", pItem->maxDurUsec);
}

/*************************************************************************************************/
/*!
 *  \brief      Insert item before the given target position.
 *
 *  \param      pItem   Item to insert.
 *  \param      pTgt    Target position.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static inline void schInsertBefore(BbOpDesc_t *pItem, BbOpDesc_t *pTgt)
{
  WSF_ASSERT(pTgt && pItem);

  pItem->pNext = pTgt;
  pItem->pPrev = pTgt->pPrev;
  pTgt->pPrev = pItem;

  if (pItem->pPrev)
  {
    pItem->pPrev->pNext = pItem;
  }
  else
  {
    schCb.pHead = pItem;
  }

  SCH_TRACE_INFO1("++| schInsertBefore      |++ pBod=0x%08x", (uint32_t)pItem);
  SCH_TRACE_INFO1("++|                      |++     .due=%u", pItem->due);
  SCH_TRACE_INFO1("++|                      |++     .minDurUsec=%u", pItem->minDurUsec);
  SCH_TRACE_INFO1("++|                      |++     .maxDurUsec=%u", pItem->maxDurUsec);
}

/*************************************************************************************************/
/*!
 *  \brief      Insert item after the given target position.
 *
 *  \param      pItem   Item to insert.
 *  \param      pTgt    Target position.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static inline void schInsertAfter(BbOpDesc_t *pItem, BbOpDesc_t *pTgt)
{
  WSF_ASSERT(pTgt && pItem);

  pItem->pPrev = pTgt;
  pItem->pNext = pTgt->pNext;
  pTgt->pNext = pItem;

  if (pItem->pNext)
  {
    pItem->pNext->pPrev = pItem;
  }
  else
  {
    schCb.pTail = pItem;
  }

  SCH_TRACE_INFO1("++| schInsertAfter       |++ pBod=0x%08x", (uint32_t)pItem);
  SCH_TRACE_INFO1("++|                      |++     .due=%u", pItem->due);
  SCH_TRACE_INFO1("++|                      |++     .minDurUsec=%u", pItem->minDurUsec);
  SCH_TRACE_INFO1("++|                      |++     .maxDurUsec=%u", pItem->maxDurUsec);
}

/*************************************************************************************************/
/*!
 *  \brief      Remove head item from BOD list.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void schRemoveHead(void)
{
  WSF_ASSERT(schCb.pHead);

  schCb.pHead = schCb.pHead->pNext;

  if (schCb.pHead)
  {
    schCb.pHead->pPrev = NULL;
  }
  else
  {
    /* Now empty list */
    schCb.pTail = NULL;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Remove non-head item from BOD list.
 *
 *  \param      pBod      Element to remove.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void schRemoveMiddle(BbOpDesc_t *pBod)
{
  if (schCb.pTail == pBod)
  {
    /* Last element */
    schCb.pTail = schCb.pTail->pPrev;

    if (schCb.pTail)
    {
      schCb.pTail->pNext = NULL;
    }
    else
    {
      /* Now empty list */
      schCb.pHead = NULL;
    }
  }
  else
  {
    /* Linkage is intact. */
    WSF_ASSERT(pBod->pPrev && pBod->pNext);

    /* Middle element */
    pBod->pPrev->pNext = pBod->pNext;
    pBod->pNext->pPrev = pBod->pPrev;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Remove item from list for conflict.
 *
 *  \param      pBod    Element to remove.
 *
 *  \return     Return TRUE if removed successfully, FALSE if item not in the list or could not be
 *              removed.
 *
 *  Remove item from list.
 */
/*************************************************************************************************/
static bool_t schRemoveForConflict(BbOpDesc_t *pBod)
{
  if (schCb.pHead == NULL)
  {
    return FALSE;
  }

  bool_t result = FALSE;

  if (schCb.pHead == pBod)
  {
    if (schCb.state == SCH_STATE_EXEC)
    {
      if ((result = SchEnoughTimeToCancel(pBod)) == TRUE)
      {
        BbCancelBod();
        schRemoveHead();
        result = TRUE;
      }
    }
    else
    {
      schRemoveHead();
      result = TRUE;
    }
  }
  else
  {
    schRemoveMiddle(pBod);
    result = TRUE;
  }

  return result;
}

/*************************************************************************************************/
/*!
 *  \brief      Check whether the conflict between BODs is resolvable.
 *
 *  \param      pItem           Item to insert.
 *  \param      pTgt            Target position.
 *  \param      conflictCback   Conflict action callback.
 *
 *  \return     TRUE if conflict is resolvable, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t SchIsBodResolvable(BbOpDesc_t *pItem, BbOpDesc_t *pTgt, BbConflictAct_t conflictCback)
{
  bool_t result = FALSE;

  if (pItem->reschPolicy < pTgt->reschPolicy)
  {
    result = TRUE;
  }
  else if ((pItem->reschPolicy == pTgt->reschPolicy) &&
           conflictCback)
  {
    if (conflictCback(pItem, pTgt) == pItem)
    {
      result = TRUE;
    }
  }
  else
  {
    /* pItem is lower priority; no insertion. */
    LL_TRACE_WARN2("!!! Scheduling conflict: existing policy=%u prioritized over incoming policy=%u", pTgt->reschPolicy, pItem->reschPolicy);
  }

  return result;
}

/*************************************************************************************************/
/*!
 *  \brief      Resolve conflict between BODs.
 *
 *  \param      pItem   Item to insert.
 *  \param      pTgt    Target position.
 *
 *  \note       Resolve the conflict between the pItem and pTgt. It is possible that
 *              there is a conflict between the pItem and BODs after pTgt. But these conflict
 *              resolutions will be delayed until the BODs are executed.
 *
 *  \return     TRUE if \a pItem is inserted, FALSE for no change.
 */
/*************************************************************************************************/
static bool_t SchResolveConflict(BbOpDesc_t *pItem, BbOpDesc_t *pTgt)
{
  bool_t result = FALSE;

  BbOpDesc_t *pNext = pTgt->pNext;
  BbOpDesc_t *pPrev = pTgt->pPrev;

  if ((result = schRemoveForConflict(pTgt)) == TRUE)
  {
    if (pNext)
    {
      schInsertBefore(pItem, pNext);
    }
    else if (pPrev)
    {
      schInsertAfter(pItem, pPrev);
    }
    else
    {
      /* Insert at head. */
      schInsertToEmptyList(pItem);
    }

    if (pTgt->abortCback)
    {
      pTgt->abortCback(pTgt);
    }
  }
  else
  {
    LL_TRACE_WARN0("!!! Could not remove existing BOD");
  }

  return result;
}

/*************************************************************************************************/
/*!
 *  \brief      Remove background BOD.
 *
 *  \return     None.
 *
 *  \note       Removes the background BOD, usually unconditionally. It will call the BOD's cancel
 *              callback.
 */
/*************************************************************************************************/
void SchRemoveBackground(void)
{
  if ((schCb.background.pBod != NULL) &&
      schCb.background.active)
  {
    BbCancelBod();
    schCb.background.active = FALSE;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Insert BOD into background slot.
 *
 *  \param      pBod    Element to insert.
 *
 *  \return     None.
 *
 *  Insert this BOD in the background slot.
 */
/*************************************************************************************************/
void SchInsertBackground(BbOpDesc_t *pBod)
{
  /* Only set if there is no current background or activity in the list */
  if ((schCb.background.pBod == NULL) &&
      !schCb.background.active)
  {
    /* Mark the BOD as background */
    schCb.background.pBod = pBod;
    if (schCb.pHead == NULL)
    {
      /* Only start it if there is no other active BOD */
      BbExecuteBod(pBod);
      schCb.background.active = TRUE;
    }
  }
  SCH_TRACE_INFO1("++| schInsertBackground  |++ pBod=0x%08x", (uint32_t)pBod);
  SCH_TRACE_INFO1("++|                      |++     .active=%u", (uint32_t)schCb.background.active);
}

/*************************************************************************************************/
/*!
 *  \brief      Check whether the conflicts between pItem and BODs starting at pTgt are
 *              resolvable.
 *
 *  \param      pItem           Item to insert.
 *  \param      pTgt            Target position.
 *  \param      conflictCback   Conflict action callback
 *
 *  \return     TRUE if conflict is resolvable, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t SchIsConflictResolvable(BbOpDesc_t *pItem, BbOpDesc_t *pTgt, BbConflictAct_t conflictCback)
{
  bool_t result;
  const uint32_t startRef = SchGetStartRefTime();
  BbOpDesc_t *pCur = pTgt;

  WSF_ASSERT(pTgt && pItem);

  while (TRUE)
  {
    if ((pCur->pNext == NULL) ||                                /* pCur is the tail. */
        (SCH_IS_DONE_BEFORE(pItem, pCur->pNext, startRef)))     /* Only conflict with pCur. */
    {
      /* Only 1 conflicting BOD. */
      result = SchIsBodResolvable(pItem, pCur, conflictCback);
      break;
    }
    else
    {
      /* Check all conflicting BODs. */
      if ((result = SchIsBodResolvable(pItem, pCur, conflictCback)) == FALSE)
      {
        break;
      }
    }
    /* Traverse to the next BOD. */
    pCur = pCur->pNext;
  }

  return result;
}

/*************************************************************************************************/
/*!
 *  \brief      Insert item into BOD list at the next available opportunity.
 *
 *  \param      pBod    Element to insert.
 *
 *  \return     None.
 *
 *  Insert this BOD in the active BOD list where its duration can be accommodated.
 */
/*************************************************************************************************/
void SchInsertNextAvailable(BbOpDesc_t *pBod)
{
#if (SCH_CHECK_LIST_INTEGRITY)
  SchCheckIsNotInserted(pBod);
#endif

  SchRemoveBackground();

  const uint32_t startRef = SchGetStartRefTime();

  pBod->due = BbDrvGetCurrentTime() + BB_US_TO_BB_TICKS(BbGetSchSetupDelayUs());

  if (schCb.pHead == NULL)
  {
    schInsertToEmptyList(pBod);
  }
  else if (SCH_IS_DONE_BEFORE(pBod, schCb.pHead, startRef) &&
           schCancelHead())
  {
    /* Insert at head */
    WSF_ASSERT(pBod != schCb.pHead);
    schInsertBefore(pBod, schCb.pHead);
  }
  else
  {
    BbOpDesc_t *pCur = schCb.pHead;

    while (TRUE)
    {
      WSF_ASSERT(pBod != pCur);

      /* Only update due time when pCur ends in the future. */
      if (SCH_END_TIME(pCur) > pBod->due)
      {
        pBod->due = SCH_END_TIME(pCur);
      }

      if ((pCur->pNext == NULL) ||                          /* insert at tail */
          SCH_IS_DONE_BEFORE(pBod, pCur->pNext, startRef))
      {
        schInsertAfter(pBod, pCur);
        break;
      }

      pCur = pCur->pNext;
    }
  }

  schLoadNext();
}

/*************************************************************************************************/
/*!
 *  \brief      Insert item into BOD list sorted by due time.
 *
 *  \param      pBod            Element to insert.
 *  \param      conflictCback   Conflict callback.
 *
 *  \return     Return TRUE if inserted successfully, FALSE otherwise.
 *
 *  Insert BOD in the active list at the specified due time.
 */
/*************************************************************************************************/
bool_t SchInsertAtDueTime(BbOpDesc_t *pBod, BbConflictAct_t conflictCback)
{
  bool_t result = FALSE;

#if (SCH_CHECK_LIST_INTEGRITY)
  SchCheckIsNotInserted(pBod);
#endif

  SchRemoveBackground();

  const uint32_t startRef = SchGetStartRefTime();

  if (!schDueTimeInFuture(pBod))
  {
    return FALSE;
  }

  if (schCb.pHead == NULL)
  {
    /* No conflict when list is empty. */
    WSF_ASSERT(pBod != schCb.pHead);
    schInsertToEmptyList(pBod);
    result = TRUE;
  }
  else
  {
    /* List is not empty. */
    BbOpDesc_t *pCur = schCb.pHead;

    while (TRUE)
    {
      WSF_ASSERT(pBod != pCur);

      if (SCH_IS_DUE_BEFORE(pBod, pCur, startRef))          /* BOD is due before pCur. */
      {
        if ((SCH_IS_DONE_BEFORE(pBod, pCur, startRef)) ||                               /* BOD is due and done before pCur, insert before. */
            ((result = SchIsConflictResolvable(pBod, pCur, conflictCback)) == TRUE))    /* Check priority if due before but done after pCur. */
        {
          schInsertBefore(pBod, pCur);
          result = TRUE;
        }
        break;
      }
      else if (!SCH_IS_DONE_BEFORE(pCur, pBod, startRef))   /* BOD is due during pCur, check priority and resolve BOD. */
      {
        if ((result = SchIsConflictResolvable(pBod, pCur, conflictCback)) == TRUE)
        {
          /* Resolve conflict here otherwise delay conflict resolution when BOD is executed. */
          result = SchResolveConflict(pBod, pCur);
        }
        break;
      }
      else if (pCur->pNext == NULL)                         /* BOD is due after pCur and pCur is tail, insert after. */
      {
        schInsertAfter(pBod, pCur);
        result = TRUE;
        break;
      }

      /* Traverse to the next BOD. */
      pCur = pCur->pNext;
    }
  }

  if (result && (pBod == schCb.pHead))
  {
    result = schTryLoadHead();
  }

  return result;
}

/*************************************************************************************************/
/*!
 *  \brief      Insert item into BOD list at the earliest possible opportunity.
 *
 *  \param      pBod    Element to insert.
 *  \param      min     Minimum operation interval from BOD due time.
 *  \param      max     Maximum operation interval from BOD due time.
 *
 *  \return     Return TRUE if inserted successfully, FALSE otherwise.
 *
 *  Insert this BOD in the active BOD list where its duration can be accommodated at the earliest
 *  possible due time.
 */
/*************************************************************************************************/
bool_t SchInsertEarlyAsPossible(BbOpDesc_t *pBod, uint32_t min, uint32_t max)
{
  WSF_ASSERT(min <= max);

#if (SCH_CHECK_LIST_INTEGRITY)
  SchCheckIsNotInserted(pBod);
#endif

  SchRemoveBackground();

  bool_t result = FALSE;

  const uint32_t startRef = SchGetStartRefTime();
  const uint32_t dueOrigin = pBod->due;

  /* Try inserting at minimum interval. */
  pBod->due += min;

  if (schDueTimeInFuture(pBod))
  {
    if (schCb.pHead == NULL)
    {
      schInsertToEmptyList(pBod);
      result = TRUE;
    }
    else if (SCH_IS_DUE_BEFORE (pBod, schCb.pHead, startRef) &&
             SCH_IS_DONE_BEFORE(pBod, schCb.pHead, startRef) &&
             schCancelHead())
    {
      /* Insert at head */
      WSF_ASSERT(pBod != schCb.pHead);
      schInsertBefore(pBod, schCb.pHead);
      result = TRUE;
    }
    else if (SCH_IS_DONE_BEFORE(schCb.pTail, pBod, startRef))
    {
      /* Insert at tail */
      WSF_ASSERT(pBod != schCb.pTail);
      schInsertAfter(pBod, schCb.pTail);
      result = TRUE;
    }
    else
    {
      BbOpDesc_t *pCur = schCb.pHead;

      while (pCur)
      {
        WSF_ASSERT(pBod != pCur);

        /* Only update due time when pCur ends in the future. */
        if (SCH_END_TIME(pCur) > pBod->due)
        {
          pBod->due = SCH_END_TIME(pCur);
        }
        uint32_t nextAvailInter = pBod->due - dueOrigin;

        if ((nextAvailInter >= min) &&
            (nextAvailInter <= max))
        {
          if (pCur->pNext == NULL)
          {
            /* Insert at tail */
            schInsertAfter(pBod, pCur);
            result = TRUE;
            break;
          }
          else if (SCH_IS_DONE_BEFORE(pBod, pCur->pNext, startRef))
          {
            /* Insert middle. */
            schInsertBefore(pBod, pCur->pNext);
            result = TRUE;
            break;
          }
        }

        pCur = pCur->pNext;
      }
    }
  }

  if (result && (pBod == schCb.pHead))
  {
    result = schTryLoadHead();
  }

  if (!result)
  {
    /* Reset due time origin. */
    pBod->due = dueOrigin;
  }

  return result;
}

/*************************************************************************************************/
/*!
 *  \brief      Insert item into BOD list at the latest possible opportunity.
 *
 *  \param      pBod    Element to insert.
 *  \param      min     Minimum operation interval from BOD due time.
 *  \param      max     Maximum operation interval from BOD due time.
 *
 *  \return     Return TRUE if inserted successfully, FALSE otherwise.
 *
 *  Insert this BOD in the active BOD list where its duration can be accommodated at the latest
 *  possible due time.
 */
/*************************************************************************************************/
bool_t SchInsertLateAsPossible(BbOpDesc_t *pBod, uint32_t min, uint32_t max)
{
  WSF_ASSERT(min <= max);

  bool_t result = FALSE;

#if (SCH_CHECK_LIST_INTEGRITY)
  SchCheckIsNotInserted(pBod);
#endif

  SchRemoveBackground();

  const uint32_t startRef = SchGetStartRefTime();
  const uint32_t dueOrigin = pBod->due;

  /* Try inserting at maximum interval. */
  pBod->due = dueOrigin + max;

  if (schCb.pTail == NULL)
  {
    if (schDueTimeInFuture(pBod))
    {
      schInsertToEmptyList(pBod);
      result = TRUE;
    }
  }
  else if (SCH_IS_DUE_AFTER(pBod, schCb.pTail, startRef))
  {
    if (schDueTimeInFuture(pBod))
    {
      /* Insert at tail. */
      WSF_ASSERT(pBod != schCb.pTail);
      schInsertAfter(pBod, schCb.pTail);
      result = TRUE;
    }
  }
  else
  {
    BbOpDesc_t *pCur = schCb.pTail;

    while (pCur)
    {
      WSF_ASSERT(pBod != pCur);

      if (pCur->pPrev == NULL)
      {
        pBod->due = pCur->due - BB_US_TO_BB_TICKS(SCH_TOTAL_DUR(pBod));

        if (!schDueTimeInFuture(pBod))
        {
          break;
        }

        uint32_t nextAvailInter = pBod->due - dueOrigin;

        if ((nextAvailInter >= min) &&
            (nextAvailInter <= max) &&
            schCancelHead())
        {
          /* Insert at head. */
          schInsertBefore(pBod, pCur);
          result = TRUE;
          break;
        }
      }
      else
      {
        pBod->due = SCH_END_TIME(pCur->pPrev);

        if (!schDueTimeInFuture(pBod))
        {
          break;
        }

        uint32_t nextAvailInter = pBod->due - dueOrigin;

        if ((nextAvailInter >= min) &&
            (nextAvailInter <= max) &&
            SCH_IS_DONE_BEFORE(pBod, pCur, startRef))
        {
          /* Insert middle. */
          schInsertBefore(pBod, pCur);
          result = TRUE;
          break;
        }
      }

      pCur = pCur->pPrev;
    }
  }

  if (result && (pBod == schCb.pHead))
  {
    result = schTryLoadHead();
  }

  if (!result)
  {
    /* Reset due time origin. */
    pBod->due = dueOrigin;
  }

  return result;
}

/*************************************************************************************************/
/*!
 *  \brief      Remove BOD from list.
 *
 *  \param      pBod    Element to remove.
 *
 *  \return     Return TRUE if removed successfully, FALSE if item not in the list.
 *
 *  Remove item from list.
 */
/*************************************************************************************************/
bool_t SchRemove(BbOpDesc_t *pBod)
{
  WSF_ASSERT(pBod);

  if (schCb.pHead == NULL)
  {
    return FALSE;
  }

  bool_t result = FALSE;

  if (schCb.pHead == pBod)
  {
    if (schCb.state == SCH_STATE_EXEC)
    {
      if ((result = SchEnoughTimeToCancel(pBod)) == TRUE)
      {
        BbCancelBod();

        /* Call callback after removing from list. */
        schRemoveHead();
        schCb.state = SCH_STATE_LOAD;
        if (pBod->abortCback)
        {
          pBod->abortCback(pBod);
        }
        schCb.state = SCH_STATE_IDLE;

        result = TRUE;
      }
      else
      {
        BbSetBodTerminateFlag();
      }
    }
    else
    {
      /* Call callback after removing from list. */
      schRemoveHead();
      if (pBod->abortCback)
      {
        pBod->abortCback(pBod);
      }

      result = TRUE;
    }

    if (result)
    {
      schLoadNext();    /* TODO prevent recursion */
    }
  }
  else
  {
#if (SCH_CHECK_LIST_INTEGRITY)
    SchCheckIsInserted(pBod);
#endif

    /* Call callback after removing from list. */
    schRemoveMiddle(pBod);
    if (pBod->abortCback)
    {
      pBod->abortCback(pBod);
    }

    result = TRUE;
  }

  if (result)
  {
    SCH_TRACE_INFO1("--| SchRemove            |-- pBod=0x%08x", (uint32_t)pBod);
    SCH_TRACE_INFO1("--|                      |--     .due=%u", pBod->due);
  }

  return result;
}

/*************************************************************************************************/
/*!
 *  \brief      Reload BOD.
 *
 *  \param      pBod    Element to reload.
 *
 *  \return     None.
 *
 *  Only if the head operation, cancel the operation and re-start it.
 */
/*************************************************************************************************/
void SchReload(BbOpDesc_t *pBod)
{
  if ((schCb.pHead == pBod) &&
      schCancelHead())
  {
    schLoadNext();
  }
}
