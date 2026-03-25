/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
#include <stdlib.h>

#include <openthread/platform/logging.h>

#include "nexus_core.hpp"
#include "nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint16_t kTimestampStringSize = 40;

typedef String<kTimestampStringSize> TimestampString;

static TimestampString GetTimestamp(TimeMilli aNow)
{
    TimestampString string;

    UptimeToString(aNow.GetValue(), string, /* aIncludeMsec */ true);
    return string;
}

static TimestampString GetTimestamp(void) { return GetTimestamp(Core::Get().GetNow()); }

void Log(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    printf("%s ", GetTimestamp().AsCString());
    vprintf(aFormat, args);
    printf("\n");
    fflush(stdout);

    va_end(args);
}

//---------------------------------------------------------------------------------------------------------------------
// Logging

Logging::Logging(void)
    : mLogFile(nullptr)
{
}

Logging::~Logging(void)
{
    if (mLogFile != nullptr)
    {
        fclose(mLogFile);
    }
}

void Logging::Init(uint32_t aId)
{
    String<32> fileName;

    if (mLogFile != nullptr)
    {
        fclose(mLogFile);
    }

    fileName.Append("ot-logs%lu.log", ToUlong(aId));
    mLogFile = fopen(fileName.AsCString(), "wt");

    VerifyOrQuit(mLogFile != nullptr);

    fprintf(mLogFile, "OpenThread logs\r\n");
    fprintf(mLogFile, "- Platform: nexus\r\n");
    fprintf(mLogFile, "- Node ID : %lu\r\n", ToUlong(aId));
    fprintf(mLogFile, "\r\n");
}

void Logging::SaveLog(const char *aLogLine, TimeMilli aNow)
{
    VerifyOrExit(mLogFile != nullptr);
    fprintf(mLogFile, "%s %s\r\n", GetTimestamp(aNow).AsCString(), aLogLine);

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// otPlatLog

extern "C" {

void otPlatLogOutput(otInstance *aInstance, otLogLevel aLogLevel, const char *aLogLine)
{
    OT_UNUSED_VARIABLE(aLogLevel);

    TimeMilli now = Core::Get().GetNow();

    VerifyOrExit(aInstance != nullptr);

    printf("<%03u> %s %s\n", AsNode(aInstance).GetId(), GetTimestamp(now).AsCString(), aLogLine);
    fflush(stdout);

    AsNode(aInstance).mLogging.SaveLog(aLogLine, now);

exit:
    return;
}
}

} // namespace Nexus
} // namespace ot
