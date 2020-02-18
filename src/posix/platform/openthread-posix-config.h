/*
 *  Copyright (c) 2019, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef OPENTHREAD_PLATFORM_CONFIG_H_
#define OPENTHREAD_PLATFORM_CONFIG_H_

#include "openthread-core-config.h"

/**
 * @file
 * @brief
 *   This file includes the POSIX platform-specific configurations.
 */

/**
 * @def OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE
 *
 * Define as 1 to enable PTY device support in POSIX app.
 *
 */
#ifndef OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE
#define OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE 1
#endif

/**
 * @def OPENTHREAD_POSIX_APP_SOCKET_BASENAME
 *
 * Define socket basename used by POSIX app daemon.
 *
 */
#ifndef OPENTHREAD_POSIX_APP_SOCKET_BASENAME
#define OPENTHREAD_POSIX_APP_SOCKET_BASENAME "/tmp/openthread"
#endif

/**
 * @def OPENTHREAD_POSIX_VIRTUAL_TIME
 *
 * This setting configures whether to use virtual time.
 *
 */
#ifndef OPENTHREAD_POSIX_VIRTUAL_TIME
#define OPENTHREAD_POSIX_VIRTUAL_TIME 0
#endif

/**
 * @def OPENTHREAD_POSIX_RCP_UART_ENABLE
 *
 * Define as 1 to enable UART interface to RCP.
 *
 */
#ifndef OPENTHREAD_POSIX_RCP_UART_ENABLE
#define OPENTHREAD_POSIX_RCP_UART_ENABLE 0
#endif

/**
 * @def OPENTHREAD_POSIX_RCP_SPI_ENABLE
 *
 * Define as 1 to enable SPI interface to RCP.
 *
 */
#ifndef OPENTHREAD_POSIX_RCP_SPI_ENABLE
#define OPENTHREAD_POSIX_RCP_SPI_ENABLE 0
#endif

#endif // OPENTHREAD_PLATFORM_CONFIG_H_
