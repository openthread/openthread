/*
 *  Copyright (c) 2017, The OpenThread Authors.
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

#include <stdio.h>
#include <string.h>

#include <openthread/logging.h>

#include "lib/spinel/logger.hpp"

#include "test_util.h"

static unsigned int sLogCount;
static otLogLevel   sLastLevel;
static char         sLastMessage[128];

extern "C" otLogLevel otLoggingGetLevel(void) { return OT_LOG_LEVEL_DEBG; }

extern "C" const char *otThreadErrorToString(otError aError)
{
    VerifyOrQuit(aError == OT_ERROR_FAILED);
    return "Failed";
}

extern "C" void otLogPlatArgs(otLogLevel aLevel, const char *aModule, const char *aFormat, va_list aArgs)
{
    VerifyOrQuit(strcmp(aModule, "TestSpinel") == 0);
    sLastLevel = aLevel;
    sLogCount++;
    vsnprintf(sLastMessage, sizeof(sLastMessage), aFormat, aArgs);
}

class TestLogger : public ot::Spinel::Logger
{
public:
    TestLogger(void)
        : Logger("TestSpinel")
    {
    }

    using Logger::LogCrit;
    using Logger::LogDebg;
    using Logger::LogIfFail;
    using Logger::LogInfo;
    using Logger::LogNote;
    using Logger::LogWarn;
};

static void VerifyLog(otLogLevel aLevel, const char *aMessage, unsigned int aCount)
{
    VerifyOrQuit(sLogCount == aCount);
    VerifyOrQuit(sLastLevel == aLevel);
    VerifyOrQuit(strcmp(sLastMessage, aMessage) == 0);
}

int main(void)
{
    TestLogger logger;

    logger.LogCrit("critical %d", 1);
    VerifyLog(OT_LOG_LEVEL_CRIT, "critical 1", 1);
    logger.LogWarn("warning %d", 2);
    VerifyLog(OT_LOG_LEVEL_WARN, "warning 2", 2);
    logger.LogNote("note %d", 3);
    VerifyLog(OT_LOG_LEVEL_NOTE, "note 3", 3);
    logger.LogInfo("info %d", 4);
    VerifyLog(OT_LOG_LEVEL_INFO, "info 4", 4);
    logger.LogDebg("debug %d", 5);
    VerifyLog(OT_LOG_LEVEL_DEBG, "debug 5", 5);

    logger.LogIfFail("operation", OT_ERROR_NONE);
    logger.LogIfFail("operation", OT_ERROR_NO_ACK);
    VerifyOrQuit(sLogCount == 5);
    logger.LogIfFail("operation", OT_ERROR_FAILED);
    VerifyLog(OT_LOG_LEVEL_WARN, "operation: Failed", 6);

    printf("All Spinel logger tests passed.\n");
    return 0;
}
