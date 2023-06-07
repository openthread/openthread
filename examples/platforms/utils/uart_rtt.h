#ifndef UTILS_UART_RTT_H
#define UTILS_UART_RTT_H

#include "openthread-core-config.h"
#include <openthread/config.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def UART_RTT_BUFFER_INDEX
 *
 * RTT buffer index used for the uart.
 */
#ifndef UART_RTT_BUFFER_INDEX
#define UART_RTT_BUFFER_INDEX 0
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
 * Updates the rtt uart. Must be called frequently to process receive and send done.
 */
void utilsUartRttUpdate(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UTILS_UART_RTT_H
