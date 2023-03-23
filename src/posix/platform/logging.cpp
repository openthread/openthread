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

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <assert.h>
#include <stdarg.h>
#include <syslog.h>

#if OPENTHREAD_POSIX_CONFIG_LOG_OUTPUT_STDOUT_PRINTF
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#endif

#include <openthread/platform/logging.h>

#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
OT_TOOL_WEAK void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogRegion);

    va_list args;

    switch (aLogLevel)
    {
    case OT_LOG_LEVEL_NONE:
        aLogLevel = LOG_ALERT;
        break;
    case OT_LOG_LEVEL_CRIT:
        aLogLevel = LOG_CRIT;
        break;
    case OT_LOG_LEVEL_WARN:
        aLogLevel = LOG_WARNING;
        break;
    case OT_LOG_LEVEL_NOTE:
        aLogLevel = LOG_NOTICE;
        break;
    case OT_LOG_LEVEL_INFO:
        aLogLevel = LOG_INFO;
        break;
    case OT_LOG_LEVEL_DEBG:
        aLogLevel = LOG_DEBUG;
        break;
    default:
        assert(false);
        aLogLevel = LOG_DEBUG;
        break;
    }

    va_start(args, aFormat);
#if OPENTHREAD_POSIX_CONFIG_LOG_OUTPUT_STDOUT_PRINTF
    {
        const uint16_t kBufferSize = 4096;
        char           buffer[kBufferSize];
        OT_UNUSED_VARIABLE(aLogLevel);
        if ((vsnprintf(buffer, sizeof(buffer), aFormat, args) > 0))
        {
            struct timeval tv;
            time_t         nowtime;
            struct tm     *nowtm;
            char           tmbuf[64], buf[64];
            int            ret;
            gettimeofday(&tv, nullptr);
            nowtime = tv.tv_sec;
            nowtm   = localtime(&nowtime);
            strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);

#if defined(__APPLE__)
            ret = snprintf(buf, sizeof buf, "%s.%06d", tmbuf, tv.tv_usec);
#else
            ret = snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tv.tv_usec);
#endif
            if (ret < 0)
            {
#if defined(__APPLE__)
                printf("[%s][%d] openthread: %s\n", buf, getpid(), "failed to make log");
#else
                printf("[%s][%d:%d] openthread: %s\n", buf, getpid(), gettid(), "failed to make log");
#endif
            }
            else
            {
#if defined(__APPLE__)
                printf("[%s][%d] openthread: %s\n", buf, getpid(), buffer);
#else
                printf("[%s][%d:%d] openthread: %s\n", buf, getpid(), gettid(), buffer);
#endif
            }
        }
    }
#else
    vsyslog(aLogLevel, aFormat, args);
#endif // OPENTHREAD_POSIX_CONFIG_LOG_OUTPUT_STDOUT_PRINTF
    va_end(args);
}
#endif // OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
