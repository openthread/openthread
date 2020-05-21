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
 * @file logging.c
 * Platform abstraction for the logging
 *
 */

#include "platform-k32w.h"
#include <openthread-core-config.h>
#include <utils/code_utils.h>
#include <openthread/config.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/toolchain.h>
#include <openthread/platform/uart.h>

#include "stdio.h"
#include "string.h"

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED) || \
    (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL)

/* defines */
#define TX_BUFFER_SIZE 256 /* Length of the send buffer */
#define EOL_CHARS "\r\n"   /* End of Line Characters */
#define EOL_CHARS_LEN 2    /* Length of EOL */

/* static functions */
static void K32WLogOutput(const char *aFormat, va_list ap);

/* static variables */
static char sTxBuffer[TX_BUFFER_SIZE + 1]; /* Transmit Buffer */

OT_TOOL_WEAK void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);
    va_list ap;

    va_start(ap, aFormat);
    K32WLogOutput(aFormat, ap);
    va_end(ap);
}

/**
 * Write Blocking data
 *
 * @param[in]    aFormat*     A pointer to the format string
 * @param[in]    ap           Variable List Argument
 *
 */
static void K32WLogOutput(const char *aFormat, va_list ap)
{
    int len = 0;

    len = vsnprintf(sTxBuffer, TX_BUFFER_SIZE - EOL_CHARS_LEN, aFormat, ap);
    otEXPECT(len >= 0);
    memcpy(sTxBuffer + len, EOL_CHARS, EOL_CHARS_LEN);
    len += EOL_CHARS_LEN;
    K32WWriteBlocking((const uint8_t *)sTxBuffer, len);

exit:
    return;
}

#endif
