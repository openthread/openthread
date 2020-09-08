/*! *********************************************************************************
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
* All rights reserved.
*
* \file
*
* This is the source file for the linked lists part of the Utils package.
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "GenericList.h"
#include "fsl_os_abstraction.h"


/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */
/*! *********************************************************************************
* \brief     Initialises the list descriptor. 
*
* \param[in] list - List handle to init.
*            max - Maximum number of elements in list. 0 for unlimited.
*
* \return void.
*
* \pre
*
* \post
*
* \remarks
*
********************************************************************************** */
void ListInit(listHandle_t list, uint32_t max)
{
  list->head = NULL;
  list->tail = NULL;
  list->max = max;
  list->size = 0;
}

/*! *********************************************************************************
* \brief     Gets the list that contains the given element. 
*
* \param[in] element - Handle of the element.
*
* \return NULL if element is orphan.
*         Handle of the list the element is inserted into.
*
* \pre
*
* \post
*
* \remarks
*
********************************************************************************** */
listHandle_t ListGetList(listElementHandle_t element)
{
  return element->list;
}

/*! *********************************************************************************
* \brief     Links element to the tail of the list. 
*
* \param[in] list - ID of list to insert into.
*            element - element to add
*
* \return gListFull_c if list is full.
*         gListOk_c if insertion was successful.
*
* \pre
*
* \post
*
* \remarks
*
********************************************************************************** */
listStatus_t ListAddTail(listHandle_t list, listElementHandle_t element)
{
  OSA_InterruptDisable();

  if( (list->max != 0) && (list->max == list->size) )
  {
    OSA_InterruptEnable();
    return gListFull_c;
  }

  if(list->size == 0)  
  {
    list->head = element;
  }
  else
  {
    list->tail->next = element;
  }
  element->prev = list->tail;
  element->next = NULL;
  element->list = list;
  list->tail = element;
  list->size++;

  OSA_InterruptEnable();
  return gListOk_c;
}

/*! *********************************************************************************
* \brief     Links element to the head of the list. 
*
* \param[in] list - ID of list to insert into.
*            element - element to add
*
* \return gListFull_c if list is full.
*         gListOk_c if insertion was successful.
*
* \pre
*
* \post
*
* \remarks
*
********************************************************************************** */
listStatus_t ListAddHead(listHandle_t list, listElementHandle_t element)
{
  OSA_InterruptDisable();

  if( (list->max != 0) && (list->max == list->size) )
  {
    OSA_InterruptEnable();
    return gListFull_c;
  }
  
  if(list->size == 0)  
  {
    list->tail = element;
  }
  else
  {
    list->head->prev = element;
  }
  element->next = list->head;
  element->prev = NULL;
  element->list = list;
  list->head = element;
  list->size++;

  OSA_InterruptEnable();
  return gListOk_c;
}

/*! *********************************************************************************
* \brief     Unlinks element from the head of the list. 
*
* \param[in] list - ID of list to remove from.
*
* \return NULL if list is empty.
*         ID of removed element(pointer) if removal was successful.
*
* \pre
*
* \post
*
* \remarks
*
********************************************************************************** */
listElementHandle_t ListRemoveHead(listHandle_t list)
{
  listElementHandle_t element;
  
  OSA_InterruptDisable();

  if((NULL == list) || (list->size == 0))
  {
    OSA_InterruptEnable();
    return NULL; /*List is empty*/
  }
  
  element = list->head;
  list->size--;
  if(list->size == 0)  
  {
    list->tail = NULL;
  }
  else
  {
    element->next->prev = NULL;
  }
  list->head = element->next; /*Is NULL if element is head*/
  element->list = NULL;

  OSA_InterruptEnable();
  return element;
}

/*! *********************************************************************************
* \brief     Gets head element ID. 
*
* \param[in] list - ID of list.
*
* \return NULL if list is empty.
*         ID of head element if list is not empty.
*
* \pre
*
* \post
*
* \remarks
*
********************************************************************************** */
listElementHandle_t ListGetHead(listHandle_t list)
{
  return list->head;
}

/*! *********************************************************************************
* \brief     Gets next element ID. 
*
* \param[in] element - ID of the element.
*
* \return NULL if element is tail.
*         ID of next element if exists.
*
* \pre
*
* \post
*
* \remarks
*
********************************************************************************** */
listElementHandle_t ListGetNext(listElementHandle_t element)
{
  return element->next;
}

/*! *********************************************************************************
* \brief     Gets previous element ID. 
*
* \param[in] element - ID of the element.
*
* \return NULL if element is head.
*         ID of previous element if exists.
*
* \pre
*
* \post
*
* \remarks
*
********************************************************************************** */
listElementHandle_t ListGetPrev(listElementHandle_t element)
{
  return element->prev;
}

/*! *********************************************************************************
* \brief     Unlinks an element from its list. 
*
* \param[in] element - ID of the element to remove.
*
* \return gOrphanElement_c if element is not part of any list.
*         gListOk_c if removal was successful.
*
* \pre
*
* \post
*
* \remarks
*
********************************************************************************** */
listStatus_t ListRemoveElement(listElementHandle_t element)
{
  if(element->list == NULL)
  {
    return gOrphanElement_c; /*Element was previusly removed or never added*/
  }
  
  OSA_InterruptDisable();

  if(element->prev == NULL) /*Element is head or solo*/
  {
    element->list->head = element->next; /*is null if solo*/
  }
  if(element->next == NULL) /*Element is tail or solo*/
  {
    element->list->tail = element->prev; /*is null if solo*/
  }  
  if(element->prev != NULL) /*Element is not head*/
  {
    element->prev->next = element->next;
  }
  if(element->next != NULL) /*Element is not tail*/
  {
    element->next->prev = element->prev;
  }
  element->list->size--;
  element->list = NULL;

  OSA_InterruptEnable();
  return gListOk_c;  
}

/*! *********************************************************************************
* \brief     Links an element in the previous position relative to a given member 
*            of a list. 
*
* \param[in] element - ID of a member of a list.
*            newElement - new element to insert before the given member.
*
* \return gOrphanElement_c if element is not part of any list.
*         gListFull_c if list is full.
*         gListOk_c if insertion was successful.
*
* \pre
*
* \post
*
* \remarks
*
********************************************************************************** */
listStatus_t ListAddPrevElement(listElementHandle_t element, listElementHandle_t newElement)
{
  if(element->list == NULL)
  {
    return gOrphanElement_c; /*Element was previusly removed or never added*/
  }
  OSA_InterruptDisable();

  if( (element->list->max != 0) && (element->list->max == element->list->size) )
  {
    OSA_InterruptEnable();
    return gListFull_c;
  }
  
  if(element->prev == NULL) /*Element is list head*/
  {
    element->list->head = newElement;
  }
  else
  {
    element->prev->next = newElement;
  }
  newElement->list = element->list;
  element->list->size++;
  newElement->next = element;
  newElement->prev = element->prev;
  element->prev = newElement;

  OSA_InterruptEnable();
  return gListOk_c;
}

/*! *********************************************************************************
* \brief     Gets the current size of a list. 
*
* \param[in] list - ID of the list.
*
* \return Current size of the list.
*
* \pre
*
* \post
*
* \remarks
*
********************************************************************************** */
uint32_t ListGetSize(listHandle_t list)
{
  return list->size;
}

/*! *********************************************************************************
* \brief     Gets the number of free places in the list. 
*
* \param[in] list - ID of the list.
*
* \return Available size of the list.
*
* \pre
*
* \post
*
* \remarks
*
********************************************************************************** */
uint32_t ListGetAvailable(listHandle_t list)
{
  return (list->max - list->size);
}

/*! *********************************************************************************
* \brief     Creates, tests and deletes a list. Any error that occurs will trap the 
*            CPU in a while(1) loop.
*
* \param[in] void.
*
* \return gListOk_c.
*
* \pre
*
* \post
*
* \remarks
*
********************************************************************************** */
/* To be removed or rewritten to remove MemManager dependency. */
#if 0
listStatus_t ListTest()
{
  listHandle_t list;
  listElementHandle_t element, newElement;
  uint32_t i,freeBlocks;
  const uint32_t max = 10;
  
  freeBlocks = MEM_GetAvailableFwkBlocks(0);
  /*create list*/
  list = ListCreate(max); 
  LIST_ASSERT(list != NULL);
  
  /*add elements*/
  for(i=0; i<max; i++)
  {
    element = (listElementHandle_t)MEM_BufferFwkAlloc(sizeof(listElement_t));
    LIST_ASSERT(element != NULL);
    LIST_ASSERT(ListAddHead(list, element) == gListOk_c);
    LIST_ASSERT(list->head == element)
    ListRemoveHead(list);
    LIST_ASSERT(ListAddTail(list, element) == gListOk_c);
    LIST_ASSERT(list->tail == element);
    if(ListGetSize(list) == 1)
    {
      LIST_ASSERT(list->head == element);
    }
    else
    {
      LIST_ASSERT(list->head != element);
    }
  }
  LIST_ASSERT(ListGetSize(list) == max);
  
  /*add one more element*/
  element = (listElementHandle_t)MEM_BufferFwkAlloc(sizeof(listElement_t));
  LIST_ASSERT(element != NULL);
  LIST_ASSERT(ListAddTail(list, element) == gListFull_c);
  list->max = 0;
  LIST_ASSERT(ListAddTail(list, element) == gListOk_c);
  LIST_ASSERT(ListGetSize(list) == max+1);
  /*remove the extra element*/
  element = ListRemoveHead(list);
  LIST_ASSERT(element != NULL);
  LIST_ASSERT(ListGetSize(list) == max);
  LIST_ASSERT(MEM_BufferFree(element) == MEM_SUCCESS_c);
  list->max = max;
  
  /*parse elements*/
  element = ListGetHead(list);
  LIST_ASSERT(element != NULL);
  for(i=0; i<(max-1); i++)
  {
    element = ListGetNext(element);
    LIST_ASSERT(element != NULL);
  }
  LIST_ASSERT(element == list->tail);
  LIST_ASSERT(ListGetNext(element) == NULL);
  
  /*Reverse parse elements*/
  for(i=0; i<(max-1); i++)
  {
    element = ListGetPrev(element);
    LIST_ASSERT(element != NULL);
  }
  LIST_ASSERT(element == list->head);
  LIST_ASSERT(ListGetPrev(element) == NULL);
  
  /*Add prev*/
  element = ListGetHead(list);
  LIST_ASSERT(element != NULL);
  newElement = (listElementHandle_t)MEM_BufferFwkAlloc(sizeof(listElement_t));
  LIST_ASSERT(newElement != NULL);
  LIST_ASSERT(ListAddPrevElement(element, newElement) == gListFull_c);
  LIST_ASSERT(ListGetHead(list) == element);
  list->max = 0;
  LIST_ASSERT(ListAddPrevElement(element, newElement) == gListOk_c);
  LIST_ASSERT(ListGetHead(list) == newElement);
  newElement = (listElementHandle_t)MEM_BufferFwkAlloc(sizeof(listElement_t));
  LIST_ASSERT(newElement != NULL);
  element = list->head->next->next;
  LIST_ASSERT(ListAddPrevElement(element, newElement) == gListOk_c); 
  LIST_ASSERT(list->head->next->next == newElement);
  newElement = (listElementHandle_t)MEM_BufferFwkAlloc(sizeof(listElement_t));
  LIST_ASSERT(newElement != NULL);
  element = list->tail;
  LIST_ASSERT(ListAddPrevElement(element, newElement) == gListOk_c); 
  LIST_ASSERT(list->tail->prev == newElement);
  newElement = (listElementHandle_t)MEM_BufferFwkAlloc(sizeof(listElement_t));
  LIST_ASSERT(newElement != NULL);
  element = (listElementHandle_t)MEM_BufferFwkAlloc(sizeof(listElement_t));
  LIST_ASSERT(element != NULL);
  element->list = NULL;
  LIST_ASSERT(ListAddPrevElement(element, newElement) == gOrphanElement_c); 
  MEM_BufferFree(newElement);
  MEM_BufferFree(element);
  LIST_ASSERT(ListGetSize(list) == max+3);
  
  /*Remove element*/
  element = ListGetHead(list);
  LIST_ASSERT(element == list->head);
  LIST_ASSERT(ListRemoveElement(element) == gListOk_c);
  LIST_ASSERT(list->head != element);
  LIST_ASSERT(ListRemoveElement(element) == gOrphanElement_c);
  MEM_BufferFree(element);
  element = ListGetHead(list)->next->next;
  LIST_ASSERT(ListRemoveElement(element) == gListOk_c);
  MEM_BufferFree(element);
  element = list->tail;
  LIST_ASSERT(ListRemoveElement(element) == gListOk_c);
  LIST_ASSERT(list->tail != element);
  MEM_BufferFree(element);
  LIST_ASSERT(ListGetSize(list) == max);
  list->max = max;
  
  for(i=0; i<(max-1); i++)
  {
    element = ListRemoveHead(list);
    LIST_ASSERT(element != NULL);
    MEM_BufferFree(element);
  }
  element = ListGetHead(list);
  LIST_ASSERT(element != NULL);
  LIST_ASSERT(ListRemoveElement(element) == gListOk_c);
  LIST_ASSERT(list->head == NULL);
  LIST_ASSERT(list->tail == NULL);
  LIST_ASSERT(element->list == NULL);
  MEM_BufferFree(element);
  
  /*List is empty here.*/
  LIST_ASSERT(ListGetSize(list) == 0);
  element = ListRemoveHead(list);
  LIST_ASSERT(element == NULL);
  element = ListGetHead(list);
  LIST_ASSERT(element == NULL);
  
  MEM_BufferFree(list);
  /*Did we produce a memory leak?*/
  LIST_ASSERT(freeBlocks == MEM_GetAvailableFwkBlocks(0));
  
  return gListOk_c;
}
#else
listStatus_t ListTest(void)
{
  return gListOk_c;
}
#endif

