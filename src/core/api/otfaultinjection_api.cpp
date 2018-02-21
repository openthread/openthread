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

/**
 * @file
 *   This file implements the OpenThread fault injection API.
 */
#include <openthread/config.h>

#if OPENTHREAD_ENABLE_FAULT_INJECTION

#include <openthread/otfaultinjection.h>

#include <common/otfaultinjection.hpp>

using namespace ot;

static nl::FaultInjection::GetManagerFn sFaultMgrTable[] = {ot::FaultInjection::GetManager};

otError otFIFailAtFault(otFaultId aId, uint32_t aNumCallsToSkip, uint32_t aNumCallsToFail)
{
    int     retval;
    otError otRetval;

    nl::FaultInjection::Manager &mgr = ot::FaultInjection::GetManager();

    retval = mgr.FailAtFault(aId, aNumCallsToSkip, aNumCallsToFail);

    switch (retval)
    {
    case 0:
        otRetval = OT_ERROR_NONE;
        break;

    case EINVAL:
        otRetval = OT_ERROR_INVALID_ARGS;
        break;

    default:
        otRetval = OT_ERROR_FAILED;
        break;
    }

    return otRetval;
}

const char *otFIGetManagerName(void)
{
    nl::FaultInjection::Manager &mgr = ot::FaultInjection::GetManager();

    return mgr.GetName();
}

const char *otFIGetFaultName(otFaultId aId)
{
    nl::FaultInjection::Manager &mgr = ot::FaultInjection::GetManager();

    if (aId >= mgr.GetNumFaults())
    {
        return NULL;
    }

    return mgr.GetFaultNames()[aId];
}

int otFIGetFaultCounterValue(otFaultId aId, uint32_t *aValue)
{
    nl::FaultInjection::Manager &mgr = ot::FaultInjection::GetManager();

    if (aId >= mgr.GetNumFaults())
    {
        return EINVAL;
    }

    *aValue = mgr.GetFaultRecords()[aId].mNumTimesChecked;

    return 0;
}

bool otFIParseFaultInjectionStr(char *aStr)
{
    return nl::FaultInjection::ParseFaultInjectionStr(aStr, sFaultMgrTable,
                                                      sizeof(sFaultMgrTable) / sizeof(sFaultMgrTable[0]));
}

void otFIResetCounters(void)
{
    nl::FaultInjection::Manager &mgr = ot::FaultInjection::GetManager();

    mgr.ResetFaultCounters();
}

void otFIResetConfiguration(void)
{
    nl::FaultInjection::Manager &mgr = ot::FaultInjection::GetManager();

    mgr.ResetFaultConfigurations();
}
#endif // OPENTHREAD_ENABLE_FAULT_INJECTION
