/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include <openthread/platform/entropy.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/misc.h>

#include "nexus_core.hpp"
#include "nexus_node.hpp"

namespace ot {
namespace Nexus {

static void LogVarArgs(Node *aActiveNode, const char *aFormat, va_list aArgs);

extern "C" {

//---------------------------------------------------------------------------------------------------------------------
// otTasklets

void otTaskletsSignalPending(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    Core::Get().MarkPendingAction();
}

//---------------------------------------------------------------------------------------------------------------------
// otPlatLog

void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);

    va_list args;

    va_start(args, aFormat);
    LogVarArgs(Core::Get().GetActiveNode(), aFormat, args);
    va_end(args);
}

//---------------------------------------------------------------------------------------------------------------------
// Heap allocation APIs

void *otPlatCAlloc(size_t aNum, size_t aSize)
{
    void *ptr = calloc(aNum, aSize);
    return ptr;
}

void otPlatFree(void *aPtr) { free(aPtr); }

//---------------------------------------------------------------------------------------------------------------------
// Entropy

otError otPlatEntropyGet(uint8_t *aOutput, uint16_t aOutputLength)
{
    Error  error = OT_ERROR_NONE;
    FILE  *file  = nullptr;
    size_t readLength;

    file = fopen("/dev/urandom", "rb");
    VerifyOrExit(file != nullptr, error = kErrorFailed);

    readLength = fread(aOutput, 1, aOutputLength, file);

    if (readLength != aOutputLength)
    {
        error = kErrorFailed;
    }

    fclose(file);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// Misc

otError           otPlatDiagProcess(otInstance *, uint8_t, char *[]) { return kErrorNotImplemented; }
void              otPlatDiagModeSet(bool) {}
bool              otPlatDiagModeGet() { return false; }
void              otPlatDiagChannelSet(uint8_t) {}
void              otPlatDiagTxPowerSet(int8_t) {}
void              otPlatDiagRadioReceived(otInstance *, otRadioFrame *, otError) {}
void              otPlatDiagAlarmCallback(otInstance *) {}
void              otPlatUartSendDone(void) {}
void              otPlatUartReceived(const uint8_t *, uint16_t) {}
void              otPlatReset(otInstance *) {}
otError           otPlatResetToBootloader(otInstance *) { return kErrorNotImplemented; }
otPlatResetReason otPlatGetResetReason(otInstance *) { return OT_PLAT_RESET_REASON_POWER_ON; }
void              otPlatWakeHost(void) {}

} // extern "C"

//---------------------------------------------------------------------------------------------------------------------
// Log related function

void Log(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    LogVarArgs(nullptr, aFormat, args);
    va_end(args);
}

static void LogVarArgs(Node *aActiveNode, const char *aFormat, va_list aArgs)
{
    uint32_t now = Core::Get().GetNow().GetValue();

    printf("%02u:%02u:%02u.%03u ", now / 3600000, (now / 60000) % 60, (now / 1000) % 60, now % 1000);

    if (aActiveNode != nullptr)
    {
        printf("%03u ", aActiveNode->GetInstance().GetId());
    }

    vprintf(aFormat, aArgs);
    printf("\n");
}

} // namespace Nexus
} // namespace ot
