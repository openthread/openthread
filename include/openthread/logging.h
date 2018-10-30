/*
 *  Copyright (c) 2016-2018, The OpenThread Authors.
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
 * @brief
 *   This file includes OpenThread logging related definitions.
 */

#ifndef OPENTHREAD_LOGGING_H_
#define OPENTHREAD_LOGGING_H_

#include <openthread/error.h>
#include <openthread/platform/logging.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-logging
 *
 * @brief
 *   This module includes OpenThread logging related definitions.
 *
 * @{
 *
 */

/**
 * This function returns the current log level.
 *
 * If dynamic log level feature `OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL` is enabled, this function returns the
 * currently set dynamic log level. Otherwise, this function returns the build-time configured log level.
 *
 * @returns The log level.
 *
 */
otLogLevel otLoggingGetLevel(void);

/**
 * This function sets the log level.
 *
 * @param[in]  aLogLevel               The log level.
 *
 * @retval OT_ERROR_NONE               The log level was changed successfully.
 * @retval OT_ERROR_DISABLED_FEATURE   The dynamic log level feature is not supported.
 *                                     (see `OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL` configuration option).
 *
 */
otError otLoggingSetLevel(otLogLevel aLogLevel);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_LOGGING_H_
