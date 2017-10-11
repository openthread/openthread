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
 *   This file implements OpenThread's fault injection Manager
 */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "common/otfaultinjection.hpp"
#include "common/logging.hpp"
#include "common/locator.hpp"

#if OPENTHREAD_ENABLE_FAULT_INJECTION

#define otLogCritFI(aInstance, aFormat, ...) otLogCrit(&(aInstance), OT_LOG_REGION_PLATFORM, aFormat, ## __VA_ARGS__)


namespace ot {
namespace FaultInjection {

static nl::FaultInjection::Record sFaultRecordArray[kFault_NumFaultIds];
static class nl::FaultInjection::Manager sOTFaultInMgr;
static const char *sManagerName = "OpenThread";
static const char *sFaultNames[] =
{
    "AllocBuffer",
    "DropRadioRx",
};

static void PostInjectionCallbackFn(nl::FaultInjection::Manager *aManager,
                                    nl::FaultInjection::Identifier aId,
                                    nl::FaultInjection::Record *aFaultRecord);

static nl::FaultInjection::GlobalContext sFaultInjectionGlobalContext =
{
    {
        NULL, // reboot callback
        PostInjectionCallbackFn
    }
};

static void PostInjectionCallbackFn(nl::FaultInjection::Manager *aManager,
                                    nl::FaultInjection::Identifier aId,
                                    nl::FaultInjection::Record *aFaultRecord)
{
    static char tmpstr[300];
    size_t charAvailable = sizeof(tmpstr);
    size_t charWritten = 0;
    int retval = 0;
    uint16_t numargs = aFaultRecord->mNumArguments;

    tmpstr[0] = 0;

    retval = snprintf(tmpstr, charAvailable,
                      "Injecting fault %s_%s, instance: %" PRIu32 "; %s",
                      aManager->GetName(), aManager->GetFaultNames()[aId],
                      aFaultRecord->mNumTimesChecked, aFaultRecord->mReboot ? "reboot" : "");

    if (retval < 0)
    {
        goto error_exit;
    }

    charWritten += retval;

    if (numargs)
    {
        uint16_t i;

        if (charWritten < charAvailable)
        {
            retval = snprintf(tmpstr + charWritten, charAvailable - charWritten,
                              " with %" PRIu16 " args:", numargs);

            if (retval < 0)
            {
                goto error_exit;
            }

            charWritten += retval;
        }

        for (i = 0; i < numargs; i++)
        {
            if (charWritten < charAvailable)
            {
                retval = snprintf(tmpstr + charWritten, charAvailable - charWritten,
                                  " %" PRId32, aFaultRecord->mArguments[i]);

                if (retval < 0)
                {
                    goto error_exit;
                }

                charWritten += retval;
            }
        }
    }

    otLogCritFI(ot::GetInstance(), "%s", tmpstr);

    if (charWritten >= charAvailable)
    {
        otLogCritFI(ot::GetInstance(), "String overflow!");
    }

    return;

error_exit:
    otLogCritFI(ot::GetInstance(), "snprintf error!");
}



/**
 * Get the singleton FaultInjection::Manager for OpenThread faults
 */
nl::FaultInjection::Manager &GetManager(void)
{
    if (0 == sOTFaultInMgr.GetNumFaults())
    {
        nl::FaultInjection::SetGlobalContext(&sFaultInjectionGlobalContext);

        sOTFaultInMgr.Init(kFault_NumFaultIds,
                           sFaultRecordArray,
                           sManagerName,
                           sFaultNames);
    }

    return sOTFaultInMgr;
}

} // namespace FaultInjection
} // namespace ot


#endif // OPENTHREAD_ENABLE_FAULT_INJECTION
