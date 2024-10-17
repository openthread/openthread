/*
 *  Copyright (c) 2023, The OpenThread Authors.
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

#ifndef OPENTHREAD_POSIX_DAEMON_CONFIG_H_
#define OPENTHREAD_POSIX_DAEMON_CONFIG_H_

/**
 * @file
 * @brief
 *   This file includes the POSIX daemon specific configurations.
 */

/**
 * @def OPENTHREAD_POSIX_CONFIG_DAEMON_SOCKET_BASENAME
 *
 * Define socket basename used by POSIX app daemon.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_DAEMON_SOCKET_BASENAME
#ifdef __linux__
#define OPENTHREAD_POSIX_CONFIG_DAEMON_SOCKET_BASENAME "/run/openthread-%s"
#else
#define OPENTHREAD_POSIX_CONFIG_DAEMON_SOCKET_BASENAME "/tmp/openthread-%s"
#endif
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
 *
 * Define to 1 to enable POSIX daemon.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
#define OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE 0
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_DAEMON_CLI_ENABLE
 *
 * Define to 1 to enable CLI for the posix daemon.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_DAEMON_CLI_ENABLE
#define OPENTHREAD_POSIX_CONFIG_DAEMON_CLI_ENABLE 1
#endif

#endif // OPENTHREAD_POSIX_DAEMON_CONFIG_H_
