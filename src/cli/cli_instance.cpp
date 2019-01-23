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
 *   This file implements the CLI interpreter Instance related functions.
 */

#include "openthread-core-config.h"

#include <stdio.h>
#include <stdlib.h>

#include "cli.hpp"
#include "cli_server.hpp"

#include "common/debug.hpp"
#include "utils/wrap_string.h"

namespace ot {

namespace Cli {

#ifdef OTDLL
void Interpreter::CacheInstances()
{
    if (mApiInstance)
    {
        otDeviceList *aDeviceList = otEnumerateDevices(mApiInstance);
        assert(aDeviceList);

        mInstancesLength = aDeviceList->aDevicesLength > MAX_CLI_OT_INSTANCES ? MAX_CLI_OT_INSTANCES
                                                                              : (uint8_t)aDeviceList->aDevicesLength;

        for (uint8_t i = 0; i < mInstancesLength; i++)
        {
            mInstances[i].mInterpreter = this;
            mInstances[i].mInstance = static_cast<Instance *>(otInstanceInit(mApiInstance, &aDeviceList->aDevices[i]));
            assert(mInstances[i].mInstance);
        }

        otFreeMemory(aDeviceList);

        if (mInstancesLength > 0)
        {
            mInstance = mInstances[0].mInstance;
        }
    }
}

#define GUID_FORMAT "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}"
#define GUID_ARG(guid)                                                                                             \
    guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], \
        guid.Data4[5], guid.Data4[6], guid.Data4[7]

void Interpreter::ProcessInstanceList(int argc, char *argv[])
{
    mServer->OutputFormat("%d instances found:\r\n", mInstancesLength);

    for (uint8_t i = 0; i < mInstancesLength; i++)
    {
        GUID     aDeviceGuid  = otGetDeviceGuid(mInstances[i].mInstance);
        uint32_t aCompartment = otGetCompartmentId(mInstances[i].mInstance);
        mServer->OutputFormat("[%d] " GUID_FORMAT " (Compartment %u)\r\n", i, GUID_ARG(aDeviceGuid), aCompartment);
    }
}

void Interpreter::ProcessInstance(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (argc == 0)
    {
        if (mInstance == NULL)
        {
            mServer->OutputFormat("No Instance Set\r\n");
        }
        else
        {
            GUID     aDeviceGuid  = otGetDeviceGuid(mInstance);
            uint32_t aCompartment = otGetCompartmentId(mInstance);
            mServer->OutputFormat("[%d] " GUID_FORMAT " (Compartment %u)\r\n", mInstanceIndex, GUID_ARG(aDeviceGuid),
                                  aCompartment);
        }
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        VerifyOrExit(value >= 0 && value < mInstancesLength, error = OT_ERROR_INVALID_ARGS);

        mInstanceIndex = (uint8_t)value;
        mInstance      = mInstances[mInstanceIndex].mInstance;
    }

exit:
    AppendResult(error);
}
#endif

} // namespace Cli
} // namespace ot
