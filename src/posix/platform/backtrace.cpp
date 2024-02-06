/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file implements backtrace for posix.
 */

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/code_utils.hpp"
#include "common/logging.hpp"

#if OPENTHREAD_POSIX_CONFIG_BACKTRACE_ENABLE
#if OPENTHREAD_POSIX_CONFIG_ANDROID_ENABLE || defined(__GLIBC__)
#if OPENTHREAD_POSIX_CONFIG_ANDROID_ENABLE
#include <log/log.h>
#include <utils/CallStack.h>
#include <utils/Printer.h>

class LogPrinter : public android::Printer
{
public:
    virtual void printLine(const char *string) { otLogCritPlat("%s", string); }
};

static void dumpStack(void)
{
    LogPrinter         printer;
    android::CallStack callstack;

    callstack.update();
    callstack.print(printer);
}
#else // OPENTHREAD_POSIX_CONFIG_ANDROID_ENABLE
#include <cxxabi.h>
#include <execinfo.h>

static void demangleSymbol(int aIndex, const char *aSymbol)
{
    constexpr uint16_t kMaxNameSize = 256;
    int                status;
    char               program[kMaxNameSize + 1];
    char               mangledName[kMaxNameSize + 1];
    char              *demangledName = nullptr;
    char              *functionName  = nullptr;
    unsigned int       offset        = 0;
    unsigned int       address       = 0;

    // Try to demangle a C++ name
    if (sscanf(aSymbol, "%256[^(]%*[^_]%256[^)+]%*[^0x]%x%*[^0x]%x", program, mangledName, &offset, &address) == 4)
    {
        demangledName = abi::__cxa_demangle(mangledName, nullptr, nullptr, &status);
        functionName  = demangledName;
    }

    VerifyOrExit(demangledName == nullptr);

    // If failed to demangle a C++ name, try to get a regular C symbol
    if (sscanf(aSymbol, "%256[^(](%256[^)+]%*[^0x]%x%*[^0x]%x", program, mangledName, &offset, &address) == 4)
    {
        functionName = mangledName;
    }

exit:
    if (functionName != nullptr)
    {
        otLogCritPlat("#%2d: %s %s+0x%x [0x%x]\n", aIndex, program, functionName, offset, address);
    }
    else
    {
        otLogCritPlat("#%2d: %s\n", aIndex, aSymbol);
    }

    if (demangledName != nullptr)
    {
        free(demangledName);
    }
}

static void dumpStack(void)
{
    constexpr uint8_t kMaxDepth = 50;
    void             *stack[kMaxDepth];
    char            **symbols;
    int               depth;

    depth   = backtrace(stack, kMaxDepth);
    symbols = backtrace_symbols(stack, depth);
    VerifyOrExit(symbols != nullptr);

    for (int i = 0; i < depth; i++)
    {
        demangleSymbol(i, symbols[i]);
    }

    free(symbols);

exit:
    return;
}
#endif // OPENTHREAD_POSIX_CONFIG_ANDROID_ENABLE
static constexpr uint8_t kNumSignals           = 6;
static constexpr int     kSignals[kNumSignals] = {SIGABRT, SIGILL, SIGSEGV, SIGBUS, SIGTRAP, SIGFPE};
static struct sigaction  sSigActions[kNumSignals];

static void resetSignalActions(void)
{
    for (uint8_t i = 0; i < kNumSignals; i++)
    {
        sigaction(kSignals[i], &sSigActions[i], (struct sigaction *)nullptr);
    }
}

static void signalCritical(int sig, siginfo_t *info, void *ucontext)
{
    OT_UNUSED_VARIABLE(ucontext);
    OT_UNUSED_VARIABLE(info);

    otLogCritPlat("------------------ BEGINNING OF CRASH -------------");
    otLogCritPlat("*** FATAL ERROR: Caught signal: %d (%s)", sig, strsignal(sig));

    dumpStack();

    otLogCritPlat("------------------ END OF CRASH ------------------");

    resetSignalActions();
    raise(sig);
}

void platformBacktraceInit(void)
{
    struct sigaction sigact;

    memset(&sigact, 0, sizeof(struct sigaction));

    sigact.sa_sigaction = &signalCritical;
    sigact.sa_flags     = SA_RESTART | SA_SIGINFO | SA_NOCLDWAIT;

    for (uint8_t i = 0; i < kNumSignals; i++)
    {
        sigaction(kSignals[i], &sigact, &sSigActions[i]);
    }
}
#else  // OPENTHREAD_POSIX_CONFIG_ANDROID_ENABLE || defined(__GLIBC__)
void platformBacktraceInit(void) {}
#endif // OPENTHREAD_POSIX_CONFIG_ANDROID_ENABLE || defined(__GLIBC__)
#endif // OPENTHREAD_POSIX_CONFIG_BACKTRACE_ENABLE
