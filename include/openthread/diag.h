/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *  This file defines the top-level functions for the OpenThread diagnostics library.
 */

#ifndef OPENTHREAD_DIAG_H_
#define OPENTHREAD_DIAG_H_

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-factory-diagnostics
 *
 * @brief
 *   This module includes functions that control the Thread stack's execution.
 *
 * @{
 *
 */

/**
 * Initialize the diagnostics module.
 *
 * @param[in]  aInstance  A pointer to the OpenThread instance.
 *
 */
void otDiagInit(otInstance *aInstance);

/**
 * This function processes a factory diagnostics command line.
 *
 * @param[in]   aArgCount       The argument counter of diagnostics command line.
 * @param[in]   aArgVector      The argument vector of diagnostics command line.
 * @param[out]  aOutput         The diagnostics execution result.
 * @param[in]   aOutputMaxLen   The output buffer size.
 *
 */
void otDiagProcessCmd(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);

/**
 * This function processes a factory diagnostics command line.
 *
 * @param[in]   aString         A NULL-terminated input string.
 * @param[out]  aOutput         The diagnostics execution result.
 * @param[in]   aOutputMaxLen   The output buffer size.
 *
 */
void otDiagProcessCmdLine(const char *aString, char *aOutput, size_t aOutputMaxLen);

/**
 * This function indicates whether or not the factory diagnostics mode is enabled.
 *
 * @returns TRUE if factory diagnostics mode is enabled, FALSE otherwise.
 *
 */
bool otDiagIsEnabled(void);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_DIAG_H_
