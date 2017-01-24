/*********************************************************************
*               SEGGER MICROCONTROLLER GmbH & Co. KG                 *
*       Solutions for real time microcontroller applications         *
**********************************************************************
*                                                                    *
*       (c) 2014 - 2015  SEGGER Microcontroller GmbH & Co. KG        *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : SEGGER_RTT_Conf.h
Purpose : Implementation of SEGGER real-time transfer (RTT) which
          allows real-time communication on targets which support
          debugger memory accesses while the CPU is running.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef SEGGER_RTT_CONF_H
#define SEGGER_RTT_CONF_H

#include "compiler_abstraction.h"

#ifdef __ICCARM__
#include <intrinsics.h>
#endif

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define SEGGER_RTT_MAX_NUM_UP_BUFFERS             (2)     // Max. number of up-buffers (T->H) available on this target    (Default: 2)
#define SEGGER_RTT_MAX_NUM_DOWN_BUFFERS           (2)     // Max. number of down-buffers (H->T) available on this target  (Default: 2)

#define BUFFER_SIZE_UP                            (1024)  // Size of the buffer for terminal output of target, up to host (Default: 1k)
#define BUFFER_SIZE_DOWN                          (16)    // Size of the buffer for terminal input to target from host (Usually keyboard input) (Default: 16)

#define SEGGER_RTT_PRINTF_BUFFER_SIZE             (64u)    // Size of buffer for RTT printf to bulk-send chars via RTT     (Default: 64)

#define SEGGER_RTT_MODE_DEFAULT                   SEGGER_RTT_MODE_NO_BLOCK_SKIP // Mode for pre-initialized terminal channel (buffer 0)

//
// Target is not allowed to perform other RTT operations while string still has not been stored completely.
// Otherwise we would probably end up with a mixed string in the buffer.
// If using  RTT from within interrupts, multiple tasks or multi processors, define the SEGGER_RTT_LOCK() and SEGGER_RTT_UNLOCK() function here.
//
/*********************************************************************
*
*       RTT lock configuration for SEGGER Embedded Studio,
*       Rowley CrossStudio and GCC
*/
#if (defined __SES_ARM) || (defined __CROSSWORKS_ARM) || (defined __GNUC__)
#ifdef __ARM_ARCH_6M__
#define SEGGER_RTT_LOCK(SavedState)   {                                               \
                                          __ASM volatile ("mrs   %0, primask  \n\t"       \
                                                        "mov   r1, $1     \n\t"           \
                                                        "msr   primask, r1  \n\t"         \
                                                        : "=r" (SavedState)               \
                                                        :                                 \
                                                        : "r1"                            \
                                                        );                                \
                                          }

#define SEGGER_RTT_UNLOCK(SavedState) {                                               \
                                          __ASM volatile ("msr   primask, %0  \n\t"       \
                                                        :                                 \
                                                        : "r" (SavedState)                \
                                                        :                                 \
                                                        );                                \
                                          }

#elif (defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__))
#define SEGGER_RTT_LOCK(SavedState)   {                                               \
                                          __ASM volatile ("mrs   %0, basepri  \n\t"       \
                                                      "mov   r1, $128     \n\t"           \
                                                      "msr   basepri, r1  \n\t"           \
                                                      : "=r" (SavedState)                 \
                                                      :                                   \
                                                      : "r1"                              \
                                                      );                                  \
                                          }
#define SEGGER_RTT_UNLOCK(SavedState) {                                               \
                                          __ASM volatile ("msr   basepri, %0  \n\t"       \
                                                        :                                 \
                                                        : "r" (SavedState)                \
                                                        :                                 \
                                                        );                                \
                                          }
#else
#define SEGGER_RTT_LOCK(SavedState)   (void)(SavedState)
#define SEGGER_RTT_UNLOCK(SavedState) (void)(SavedState)
#endif
#endif

/*********************************************************************
*
*       RTT lock configuration for IAR EWARM
*/
#ifdef __ICCARM__
#if (defined (__ARM7M__) && (__CORE__ == __ARM7M__))
#define SEGGER_RTT_LOCK(SavedState)   {                                     \
                                          SavedState = __get_PRIMASK();         \
                                          __set_PRIMASK(1);                     \
                                          }

#define SEGGER_RTT_UNLOCK(SavedState) {                                     \
                                          __set_PRIMASK(SavedState);            \
                                          }
#elif (defined (__ARM7EM__) && (__CORE__ == __ARM7EM__))
#define SEGGER_RTT_LOCK(SavedState)   {                                     \
                                          SavedState = __get_BASEPRI();         \
                                          __set_BASEPRI(128);                   \
                                          }

#define SEGGER_RTT_UNLOCK(SavedState) {                                     \
                                          __set_BASEPRI(SavedState);            \
                                          }
#endif
#endif

/*********************************************************************
*
*       RTT lock configuration fallback
*/
#ifndef   SEGGER_RTT_LOCK
#define SEGGER_RTT_LOCK(SavedState)   (void)(SavedState)
#endif

#ifndef   SEGGER_RTT_UNLOCK
#define SEGGER_RTT_UNLOCK(SavedState) (void)(SavedState)
#endif

#endif
/*************************** End of file ****************************/
