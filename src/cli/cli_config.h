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

/**
 * @file
 *   This file includes compile-time configurations for the CLI service.
 */

#ifndef CONFIG_CLI_H_
#define CONFIG_CLI_H_

#include "openthread-core-config.h"

#include <openthread/tcp.h>

#ifndef OPENTHREAD_POSIX
#if defined(__ANDROID__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__linux__) || defined(__NetBSD__) || \
    defined(__unix__)
#define OPENTHREAD_POSIX 1
#else
#define OPENTHREAD_POSIX 0
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH
 *
 * The maximum size of the CLI line in bytes including the null terminator.
 */
#ifndef OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH
#define OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH 384
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_BLE_SECURE_ENABLE
 *
 * Indicates whether TCAT should be enabled in the CLI tool.
 */
#ifndef OPENTHREAD_CONFIG_CLI_BLE_SECURE_ENABLE
#define OPENTHREAD_CONFIG_CLI_BLE_SECURE_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_TCP_ENABLE
 *
 * Indicates whether TCP should be enabled in the CLI tool.
 */
#ifndef OPENTHREAD_CONFIG_CLI_TCP_ENABLE
#define OPENTHREAD_CONFIG_CLI_TCP_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_TCP_DEFAULT_BENCHMARK_SIZE
 *
 * The number of bytes to transfer for the TCP benchmark in the CLI.
 */
#ifndef OPENTHREAD_CONFIG_CLI_TCP_DEFAULT_BENCHMARK_SIZE
#define OPENTHREAD_CONFIG_CLI_TCP_DEFAULT_BENCHMARK_SIZE (72 << 10)
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_TCP_RECEIVE_BUFFER_SIZE
 *
 * The size of memory used for the TCP receive buffer, in bytes.
 */
#ifndef OPENTHREAD_CONFIG_CLI_TCP_RECEIVE_BUFFER_SIZE
#define OPENTHREAD_CONFIG_CLI_TCP_RECEIVE_BUFFER_SIZE OT_TCP_RECEIVE_BUFFER_SIZE_FEW_HOPS
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_MAX_USER_CMD_ENTRIES
 *
 * The maximum number of user CLI command lists that can be registered by the interpreter.
 */
#ifndef OPENTHREAD_CONFIG_CLI_MAX_USER_CMD_ENTRIES
#define OPENTHREAD_CONFIG_CLI_MAX_USER_CMD_ENTRIES 1
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_VENDOR_COMMANDS_ENABLE
 *
 * Indicates whether or not an externally provided list of cli commands is defined.
 *
 * This is to be used only when `OPENTHREAD_CONFIG_CLI_MAX_USER_CMD_ENTRIES` is greater than 1.
 */
#ifndef OPENTHREAD_CONFIG_CLI_VENDOR_COMMANDS_ENABLE
#define OPENTHREAD_CONFIG_CLI_VENDOR_COMMANDS_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE
 *
 * Define as 1 for CLI to emit its command input string and the resulting output to the logs.
 *
 * By default this is enabled on any POSIX based platform (`OPENTHREAD_POSIX`) and only when CLI itself is not being
 * used for logging.
 */
#ifndef OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE
#define OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE \
    (OPENTHREAD_POSIX && (OPENTHREAD_CONFIG_LOG_OUTPUT != OPENTHREAD_CONFIG_LOG_OUTPUT_APP))
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_LEVEL
 *
 * Defines the log level to use when CLI emits its command input/output to the logs.
 *
 * This is used only when `OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE` is enabled.
 */
#ifndef OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_LEVEL
#define OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_LEVEL OT_LOG_LEVEL_DEBG
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_LOG_STRING_SIZE
 *
 * The log string buffer size (in bytes).
 *
 * This is only used when `OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE` is enabled.
 */
#ifndef OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_LOG_STRING_SIZE
#define OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_LOG_STRING_SIZE OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_PROMPT_ENABLE
 *
 * Enable CLI prompt.
 *
 * When enabled, the CLI will print prompt on the output after processing a command.
 * Otherwise, no prompt is added to the output.
 */
#ifndef OPENTHREAD_CONFIG_CLI_PROMPT_ENABLE
#define OPENTHREAD_CONFIG_CLI_PROMPT_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_TXT_RECORD_MAX_SIZE
 *
 * Specifies the max TXT record data length to use when performing DNS queries.
 *
 * If the service TXT record data length is greater than the specified value, it will be read partially (up to the given
 * size) and output as a sequence of raw hex bytes `[{hex-bytes}...]`
 */
#ifndef OPENTHREAD_CONFIG_CLI_TXT_RECORD_MAX_SIZE
#define OPENTHREAD_CONFIG_CLI_TXT_RECORD_MAX_SIZE 512
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_REGISTER_IP6_RECV_CALLBACK
 *
 * Define as 1 to have CLI register an IPv6 receive callback using `otIp6SetReceiveCallback()`.
 *
 * This is intended for testing only. Receive callback should be registered for the `otIp6GetBorderRoutingCounters()`
 * to count the messages being passed to the callback.
 */
#ifndef OPENTHREAD_CONFIG_CLI_REGISTER_IP6_RECV_CALLBACK
#define OPENTHREAD_CONFIG_CLI_REGISTER_IP6_RECV_CALLBACK 0
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_BLE_SECURE_ENABLE
 *
 * Define to 1 to enable BLE secure support.
 */
#ifndef OPENTHREAD_CONFIG_CLI_BLE_SECURE_ENABLE
#define OPENTHREAD_CONFIG_CLI_BLE_SECURE_ENABLE 0
#endif

#endif // CONFIG_CLI_H_
