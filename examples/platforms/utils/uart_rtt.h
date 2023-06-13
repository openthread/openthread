#ifndef UTILS_UART_RTT_H
#define UTILS_UART_RTT_H

#include "openthread-core-config.h"
#include <openthread/config.h>

#include "logging_rtt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def UART_RTT_BUFFER_INDEX
 *
 * RTT buffer index used for the uart.
 */
#ifndef UART_RTT_BUFFER_INDEX
#define UART_RTT_BUFFER_INDEX 1
#endif

#if OPENTHREAD_UART_RTT_ENABLE                                                           \
    && (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)   \
    && (LOG_RTT_BUFFER_INDEX == UART_RTT_BUFFER_INDEX)
#error Log buffer index matches uart buffer index
#endif

/**
 * @def UART_RTT_BUFFER_NAME
 *
 * RTT name used for the uart. Only used if UART_RTT_BUFFER_INDEX is not 0.
 * Otherwise, the buffer name is fixed to "Terminal".
 */
#ifndef UART_RTT_BUFFER_NAME
#define UART_RTT_BUFFER_NAME "Terminal"
#endif

/**
 * @def UART_RTT_UP_BUFFER_SIZE
 *
 * RTT up buffer size used for the uart. Only used if UART_RTT_BUFFER_INDEX
 * is not 0. To configure buffer #0 size, check the BUFFER_SIZE_UP definition
 * in SEGGER_RTT_Conf.h
 */
#ifndef UART_RTT_UP_BUFFER_SIZE
#define UART_RTT_UP_BUFFER_SIZE 256
#endif

/**
 * @def UART_RTT_DOWN_BUFFER_SIZE
 *
 * RTT down buffer size used for the uart. Only used if UART_RTT_BUFFER_INDEX
 * is not 0. To configure buffer #0 size, check the BUFFER_SIZE_DOWN definition
 * in SEGGER_RTT_Conf.h
 */
#ifndef UART_RTT_DOWN_BUFFER_SIZE
#define UART_RTT_DOWN_BUFFER_SIZE 16
#endif

/**
 * @def UART_RTT_READ_BUFFER_SIZE
 *
 * Size of the temporary buffer used when reading from the RTT channel. It will be
 * locally allocated on the stack.
 */
#ifndef UART_RTT_READ_BUFFER_SIZE
#define UART_RTT_READ_BUFFER_SIZE 16
#endif

/**
 * Updates the rtt uart. Must be called frequently to process receive and send done.
 */
void utilsUartRttUpdate(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UTILS_UART_RTT_H
