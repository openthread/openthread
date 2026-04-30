/*
 *    Copyright (c) 2023, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements the multipan radio platform callbacks into OpenThread and default/weak radio platform APIs.
 */

#include <openthread/instance.h>
#include <openthread/platform/multipan.h>

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "instance/instance.hpp"
#include "ncp/ncp_base.hpp"

using namespace ot;

//---------------------------------------------------------------------------------------------------------------------
// otPlatRadio callbacks

otInstance *otPlatMultipanIidToInstance(uint8_t aIid)
{
    Ncp::NcpBase *ncpBase = Ncp::NcpBase::GetNcpInstance();
    OT_ASSERT(ncpBase);

    return ncpBase->IidToInstance(aIid);
}

uint8_t otPlatMultipanInstanceToIid(otInstance *aInstance)
{
    Ncp::NcpBase *ncpBase = Ncp::NcpBase::GetNcpInstance();
    OT_ASSERT(ncpBase);

    return ncpBase->InstanceToIid(static_cast<ot::Instance *>(aInstance));
}

#if OPENTHREAD_RADIO && OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE

void otPlatMultipanSwitchoverDone(otInstance *aInstance, bool success)
{
    Ncp::NcpBase *ncpBase = Ncp::NcpBase::GetNcpInstance();
    OT_ASSERT(ncpBase);

    ncpBase->NotifySwitchoverDone(aInstance, success);

    return;
}

#else // OPENTHREAD_RADIO && OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE

// default implementation
OT_TOOL_WEAK void otPlatMultipanSwitchoverDone(otInstance *aInstance, bool success)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(success);
}

#endif // OPENTHREAD_RADIO && OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Default/weak implementation of multipan APIs

OT_TOOL_WEAK otError otPlatMultipanGetActiveInstance(otInstance **aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return kErrorNotImplemented;
}

OT_TOOL_WEAK otError otPlatMultipanSetActiveInstance(otInstance *aInstance, bool aCompletePending)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aCompletePending);

    return kErrorNotImplemented;
}
