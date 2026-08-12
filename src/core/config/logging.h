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
 *   This file includes compile-time configurations for the logging service.
 */

#ifndef OT_CORE_CONFIG_LOGGING_H_
#define OT_CORE_CONFIG_LOGGING_H_

#include <openthread/platform/logging.h>

/**
 * @addtogroup config-logging
 *
 * @brief
 *   This module includes configuration variables for the Logging service.
 *
 * @{
 */

/**
 * @def OPENTHREAD_CONFIG_LOG_OUTPUT
 *
 * Selects if, and where the LOG output goes to.
 *
 * There are several options available
 * - @sa OPENTHREAD_CONFIG_LOG_OUTPUT_NONE
 * - @sa OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART
 * - @sa OPENTHREAD_CONFIG_LOG_OUTPUT_APP
 * - @sa OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
 * - and others
 *
 * Note:
 *
 * 1) Because the default is: OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
 *    The platform is expected to provide at least a stub for `otPlatLog()`.
 *
 * 2) This is effectively an ENUM so it can be if/else/endif at compile time.
 */
#ifndef OPENTHREAD_CONFIG_LOG_OUTPUT
#define OPENTHREAD_CONFIG_LOG_OUTPUT OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
#endif

/** Log output goes to the bit bucket (disabled) */
#define OPENTHREAD_CONFIG_LOG_OUTPUT_NONE 0
/** Log output goes to the debug uart - requires OPENTHREAD_CONFIG_ENABLE_DEBUG_UART to be enabled */
#define OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART 1
/** Log output goes to the "application" provided otPlatLog() in NCP and CLI code */
#define OPENTHREAD_CONFIG_LOG_OUTPUT_APP 2
/** Log output is handled by a platform defined function */
#define OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED 3

/**
 * @def OPENTHREAD_CONFIG_LOG_INSTANCE_AWARE_API_ENABLE
 *
 * Define to 1 to enable the instance-aware platform logging API.
 *
 * When this configuration is enabled, OpenThread logging will track the OpenThread instance (`otInstance`) from which
 * a log is generated. The core will use the `otPlatLogOutput()` platform API instead of `otPlatLog()`. The new
 * platform API provides the `otInstance` pointer along with the log as a fully formatted string, which is particularly
 * useful in multi-instance setups to distinguish logs from different OpenThread instances.
 */
#ifndef OPENTHREAD_CONFIG_LOG_INSTANCE_AWARE_API_ENABLE
#define OPENTHREAD_CONFIG_LOG_INSTANCE_AWARE_API_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL
 *
 * The log level (used at compile time). If `OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE` is set, this defines the most
 * verbose log level possible. See `OPENTHREAD_CONFIG_LOG_LEVEL_INIT` to set the initial log level.
 */
#ifndef OPENTHREAD_CONFIG_LOG_LEVEL
#define OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_CRIT
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
 *
 * Define as 1 to enable dynamic log level control.
 *
 * Note that the OPENTHREAD_CONFIG_LOG_LEVEL determines the log level at
 * compile time. The dynamic log level control (if enabled) only allows
 * decreasing the log level from the compile time value.
 */
#ifndef OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
#define OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL_INIT
 *
 * The initial log level used when OpenThread is initialized. See
 * `OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE`.
 */
#ifndef OPENTHREAD_CONFIG_LOG_LEVEL_INIT
#define OPENTHREAD_CONFIG_LOG_LEVEL_INIT OPENTHREAD_CONFIG_LOG_LEVEL
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PKT_DUMP
 *
 * Define to enable dump logs (of packets).
 */
#ifndef OPENTHREAD_CONFIG_LOG_PKT_DUMP
#define OPENTHREAD_CONFIG_LOG_PKT_DUMP 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_CLI
 *
 * Define to enable CLI logging and `otLogCli()` OT function.
 */
#ifndef OPENTHREAD_CONFIG_LOG_CLI
#define OPENTHREAD_CONFIG_LOG_CLI 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PLATFORM
 *
 * Define to enable platform logging and `otLog{Level}Plat()` OT functions.
 */
#ifndef OPENTHREAD_CONFIG_LOG_PLATFORM
#define OPENTHREAD_CONFIG_LOG_PLATFORM 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPEND_UPTIME
 *
 * Define as 1 to prepend the current uptime to all log messages.
 */
#ifndef OPENTHREAD_CONFIG_LOG_PREPEND_UPTIME
#define OPENTHREAD_CONFIG_LOG_PREPEND_UPTIME 0
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
 *
 * Define to prepend the log level to all log messages.
 */
#ifndef OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
#define OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_SUFFIX
 *
 * Define suffix to append at the end of logs.
 */
#ifndef OPENTHREAD_CONFIG_LOG_SUFFIX
#define OPENTHREAD_CONFIG_LOG_SUFFIX ""
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_SRC_DST_IP_ADDRESSES
 *
 * If defined as 1 when IPv6 message info is logged in mesh-forwarder, the source and destination IPv6 addresses of
 * messages are also included.
 */
#ifndef OPENTHREAD_CONFIG_LOG_SRC_DST_IP_ADDRESSES
#define OPENTHREAD_CONFIG_LOG_SRC_DST_IP_ADDRESSES 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_MAX_SIZE
 *
 * The maximum log string size (number of chars).
 */
#ifndef OPENTHREAD_CONFIG_LOG_MAX_SIZE
#define OPENTHREAD_CONFIG_LOG_MAX_SIZE 150
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_OFFLOADING_ENABLE
 *
 * Define to 1 to have the `LogCrit()`/`LogWarn()`/`LogNote()`/`LogInfo()`/`LogDebg()`/`LogAt()` logging macros
 * (and the `Log{Level}OnError()` family) expand directly to platform-provided logging macros, instead of going
 * through `Logger::LogAtLevel()` / `Logger::LogInModule()` / `Logger::LogOnError()`.
 *
 * This allows the platform to package a log message (format string and arguments, including the module name)
 * directly at the original call site, e.g. to avoid an extra text-rendering step and to let read-only/rodata
 * string arguments be handled as pointers instead of being copied.
 *
 * This also applies to the `DumpCrit()`/`DumpWarn()`/`DumpNote()`/`DumpInfo()`/`DumpDebg()` macros, which are
 * likewise expanded to platform-provided macros instead of going through `Logger::Dump()`.
 *
 * `RegisterLogModule()` is also affected: in addition to defining `kLogModuleName`, it invokes
 * `OT_LOG_PLATFORM_MODULE_REGISTER(aName)`, so the platform can register its own equivalent of a "log
 * module" for the calling file (e.g. to get native, per-module log filtering).
 *
 * The public `otLog{Level}Plat()` / `otDump{Level}Plat()` API (declared in `<openthread/logging.h>` and used by
 * platform code, e.g. `otLogCritPlat()`, to route its own logs through OpenThread's formatting) is likewise
 * affected: it expands to the same `OT_LOG_PLATFORM_{LEVEL}()` / `OT_LOG_PLATFORM_DUMP_{LEVEL}()` macros, using
 * `"Platform"` as the module name, instead of calling into `Logger`. `otLogPlat()`/`otLogPlatArgs()`/`otLogCli()`
 * are NOT affected and always go through `Logger`, since they carry a runtime module name/log level that isn't a
 * fixed part of the macro name.
 *
 * When enabled, the platform MUST provide the header file named by
 * `OPENTHREAD_CONFIG_LOG_OFFLOADING_HEADER_FILE` (including quotes or angle brackets, since no default is
 * provided by OpenThread core), defining the following macros for each log level (`CRIT`, `WARN`, `NOTE`,
 * `INFO`, `DEBG`):
 *
 *   - `OT_LOG_PLATFORM_MODULE_REGISTER(aName)`
 *   - `OT_LOG_PLATFORM_{LEVEL}(aModuleName, aFormat, ...)`
 *   - `OT_LOG_PLATFORM_{LEVEL}_ON_ERROR(aModuleName, aError, aFormat, ...)`
 *   - `OT_LOG_PLATFORM_LOG_AT(aModuleName, aLogLevel, aFormat, ...)`
 *   - `OT_LOG_PLATFORM_DUMP_{LEVEL}(aModuleName, aText, aData, aDataLength)`
 *
 * `LogAt()` uses `OT_LOG_PLATFORM_LOG_AT()`, which is passed the (only known at run time) `ot::LogLevel`
 * value as an argument rather than having a level baked into its name, since the platform then has to
 * dispatch to the right underlying macro itself (e.g. via a runtime `switch`).
 *
 * @note `LogAlways()`, `LogCert()`, and `DumpAlways()`/`DumpCert()` are unaffected and continue to go
 * through `Logger`, since they are not tied to any of the levels above (they always log, regardless of
 * the configured log level).
 */
#ifndef OPENTHREAD_CONFIG_LOG_OFFLOADING_ENABLE
#define OPENTHREAD_CONFIG_LOG_OFFLOADING_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL_OVERRIDE_ENABLE
 *
 * Define to 1 to enable the log level override feature and its associated APIs.
 *
 * This feature is used when `OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE` is also enabled.
 *
 * When enabled, new mechanism is added to allow temporary override of the current log level (e.g., to increase the
 * level to capture more detailed information) and subsequently restore it to the original user-specified level.
 */
#ifndef OPENTHREAD_CONFIG_LOG_LEVEL_OVERRIDE_ENABLE
#define OPENTHREAD_CONFIG_LOG_LEVEL_OVERRIDE_ENABLE 0
#endif

#if OPENTHREAD_CONFIG_LOG_LEVEL_OVERRIDE_ENABLE && !OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
#error "OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE is required for OPENTHREAD_CONFIG_LOG_LEVEL_OVERRIDE_ENABLE"
#endif

#if OPENTHREAD_CONFIG_LOG_LEVEL_INIT > OPENTHREAD_CONFIG_LOG_LEVEL
#error "OPENTHREAD_CONFIG_LOG_LEVEL_INIT must not be more verbose than OPENTHREAD_CONFIG_LOG_LEVEL"
#endif

#if OPENTHREAD_CONFIG_LOG_OFFLOADING_ENABLE && !defined(OPENTHREAD_CONFIG_LOG_OFFLOADING_HEADER_FILE)
#error \
    "OPENTHREAD_CONFIG_LOG_OFFLOADING_HEADER_FILE must be defined when OPENTHREAD_CONFIG_LOG_OFFLOADING_ENABLE is set"
#endif

#if OPENTHREAD_CONFIG_LOG_OFFLOADING_ENABLE && OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
#error "OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE has no effect on the `Log{Level}()`/`LogAt()`/`Dump{Level}()` " \
       "macros (i.e. on the vast majority of OpenThread's internal log statements) when " \
       "OPENTHREAD_CONFIG_LOG_OFFLOADING_ENABLE is set, since those macros then bypass `Logger` (and its runtime " \
       "log-level check) entirely and expand directly to platform-provided macros. Disable one of the two options."
#endif

/**
 * @}
 */

#endif // OT_CORE_CONFIG_LOGGING_H_
