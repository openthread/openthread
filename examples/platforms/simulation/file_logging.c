/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#include "file_logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/code_utils.hpp"

#include "utils/code_utils.h"

#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_FILE
#define LOG_BUFFER_SIZE 2048

FILE *sFile = NULL;

char     sLogBuffer[LOG_BUFFER_SIZE];
uint16_t sLogBufferPos = 0;

bool initLogFile(const char *aFileName)
{
    bool rval = true;

    sFile = fopen(aFileName, "w");
    otEXPECT_ACTION(sFile != NULL, rval = false);

    memset(sLogBuffer, 0, sizeof(sLogBuffer));
    sLogBufferPos = 0;

exit:
    return rval;
}

void flushFileLog(void)
{
    otEXPECT(sFile != NULL && sLogBufferPos > 0);

    fwrite(sLogBuffer, sizeof(char), sLogBufferPos, sFile);
    fflush(sFile);
    sLogBufferPos = 0;

exit:
    return;
}

void deinitLogFile(void)
{
    flushFileLog();

    fclose(sFile);
    sFile = NULL;
}

void writeFileLog(const char *aLogString, uint16_t aLen)
{
    uint16_t aPos = 0;

    while (aPos < aLen)
    {
        uint16_t aBytesWrite = OT_MIN(aLen - aPos, LOG_BUFFER_SIZE - sLogBufferPos);
        memcpy(sLogBuffer + sLogBufferPos, aLogString + aPos, aBytesWrite);

        aPos += aBytesWrite;
        sLogBufferPos += aBytesWrite;

        if (sLogBufferPos == LOG_BUFFER_SIZE)
        {
            flushFileLog();
        }
    }
}

#endif // OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_FILE
