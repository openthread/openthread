/*! *********************************************************************************
* Copyright (c) 2013-2014, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
* All rights reserved.
*
* \file
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */
#if !defined(__FSL_OS_ABSTRACTION_BM_H__)
#define __FSL_OS_ABSTRACTION_BM_H__


/*!
 * @addtogroup os_abstraction_bm
 * @{
 */

/*******************************************************************************
 * Declarations
 ******************************************************************************/

/*! @brief Bare Metal does not use timer. */
#define FSL_OSA_BM_TIMER_NONE   0U

/*! @brief Bare Metal uses SYSTICK as timer. */
#define FSL_OSA_BM_TIMER_SYSTICK 1U

/*! @brief Configure what timer is used in Bare Metal. */
#ifndef FSL_OSA_BM_TIMER_CONFIG
#define FSL_OSA_BM_TIMER_CONFIG FSL_OSA_BM_TIMER_NONE
#endif

/*! @brief Type for an semaphore */
typedef struct Semaphore
{
    volatile bool_t    isWaiting;  /*!< Is any task waiting for a timeout on this object */
    volatile uint8_t   semCount;   /*!< The count value of the object                    */
    uint32_t           time_start; /*!< The time to start timeout                        */
    uint32_t           timeout;    /*!< Timeout to wait in milliseconds                  */
} semaphore_t;

/*! @brief Type for a mutex */
typedef struct Mutex
{
    volatile bool_t    isWaiting;  /*!< Is any task waiting for a timeout on this mutex */
    volatile bool_t    isLocked;   /*!< Is the object locked or not                     */
    uint32_t           time_start; /*!< The time to start timeout                       */
    uint32_t           timeout;    /*!< Timeout to wait in milliseconds                 */
} mutex_t;

/*! @brief Type for task parameter */
typedef void* task_param_t;
#define gIdleTaskPriority_c   ((task_priority_t) 0)
#define gInvalidTaskPriority_c ((task_priority_t) -1)

/*! @brief Type for a task handler, returned by the OSA_TaskCreate function */
typedef void (* task_t)(task_param_t param);
/*! @brief Task control block for bare metal. */
typedef struct TaskControlBlock
{
    osaTaskPtr_t p_func;                    /*!< Task's entry                           */
    bool_t haveToRun;                       /*!< Task was signaled                      */ 
    osaTaskPriority_t priority;             /*!< Task's priority                        */    
    osaTaskParam_t  param;                  /*!< Task's parameter                       */
    struct TaskControlBlock *next;          /*!< Pointer to next task control block     */
    struct TaskControlBlock *prev;          /*!< Pointer to previous task control block */
} task_control_block_t;

/*! @brief Type for a task pointer */
typedef task_control_block_t* task_handler_t;

/*! @brief Type for a task stack */
typedef uint32_t task_stack_t;
/*! @brief Type for an event flags group, bit 32 is reserved */
typedef uint32_t event_flags_t;


/*! @brief Type for an event object */
typedef struct Event
{
    volatile bool_t             isWaiting;   /*!< Is any task waiting for a timeout on this event  */
    uint32_t                    time_start;  /*!< The time to start timeout                        */
    uint32_t                    timeout;     /*!< Timeout to wait in milliseconds                  */
    volatile event_flags_t      flags;       /*!< The flags status                                 */
    bool_t                      autoClear;   /*!< Auto clear or manual clear                       */
    task_handler_t              waitingTask; /*!< Handler to the waiting task                      */
} event_t;

/*! @brief Type for a message queue */
typedef struct MsgQueue
{
    volatile bool_t        isWaiting;                    /*!< Is any task waiting for a timeout    */
    uint16_t               number;                       /*!< The number of messages in the queue  */
    uint16_t               max;                          /*!< The max number of queue messages     */
    uint16_t               head;                         /*!< Index of the next message to be read */
    uint16_t               tail;                         /*!< Index of the next place to write to  */
    uint32_t               queueMem[osNumberOfMessages]; /*!< Points to the queue memory           */
    uint32_t               time_start;                   /*!< The time to start timeout            */
    uint32_t               timeout;                      /*!< Timeout to wait in milliseconds      */
    task_handler_t         waitingTask;                  /*!< Handler to the waiting task          */
}msg_queue_t;

/*! @brief Type for a message queue handler */
typedef msg_queue_t*  msg_queue_handler_t;

/*! @brief Constant to pass as timeout value in order to wait indefinitely. */
#define OSA_WAIT_FOREVER  0xFFFFFFFFU

/*! @brief How many tasks can the bare metal support. */
#define TASK_MAX_NUM  7

/*! @brief OSA's time range in millisecond, OSA time wraps if exceeds this value. */
#define FSL_OSA_TIME_RANGE 0xFFFFFFFFU

/*! @brief The default interrupt handler installed in vector table. */
#define OSA_DEFAULT_INT_HANDLER  ((osa_int_handler_t)(&DefaultISR))

/*! @brief The default interrupt handler installed in vector table. */
extern void DefaultISR(void);

/*!
 * @name Thread management
 * @{
 */

/*!
 * @brief Defines a task.
 *
 * This macro defines resources for a task statically. Then, the OSA_TaskCreate 
 * creates the task based-on these resources.
 *
 * @param task The task function.
 * @param stackSize The stack size this task needs in bytes.
 */

#define PRIORITY_OSA_TO_RTOS(osa_prio)   (osa_prio)
#define PRIORITY_RTOS_TO_OSA(rtos_prio)  (rtos_prio)        

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*!
 * @brief Calls all task functions one time except for the current task.
 *
 * This function calls all other task functions one time. If current
 * task is waiting for an event triggered by other tasks, this function
 * could be used to trigger the event.
 *
 * @note There should be only one task calls this function, if more than
 * one task call this function, stack overflow may occurs. Be careful
 * to use this function.
 *
 */
void OSA_PollAllOtherTasks(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

/*! @}*/

#endif /* __FSL_OS_ABSTRACTION_BM_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/

