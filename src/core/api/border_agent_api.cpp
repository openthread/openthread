/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements the OpenThread Border Agent API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

#include <openthread/border_agent.h>

#include "common/as_core_type.hpp"
#include "instance/instance.hpp"

using namespace ot;

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
otError otBorderAgentGetId(otInstance *aInstance, otBorderAgentId *aId)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().GetId(AsCoreType(aId));
}

otError otBorderAgentSetId(otInstance *aInstance, const otBorderAgentId *aId)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().SetId(AsCoreType(aId));
}
#endif

otBorderAgentState otBorderAgentGetState(otInstance *aInstance)
{
    otBorderAgentState state = OT_BORDER_AGENT_STATE_STOPPED;

    switch (AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().GetState())
    {
    case MeshCoP::BorderAgent::kStateStopped:
        break;
    case MeshCoP::BorderAgent::kStateStarted:
        state = OT_BORDER_AGENT_STATE_STARTED;
        break;
    case MeshCoP::BorderAgent::kStateConnected:
    case MeshCoP::BorderAgent::kStateAccepted:
        state = OT_BORDER_AGENT_STATE_ACTIVE;
        break;
    }

    return state;
}

uint16_t otBorderAgentGetUdpPort(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().GetUdpPort();
}

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

otError otBorderAgentSetEphemeralKey(otInstance *aInstance,
                                     const char *aKeyString,
                                     uint32_t    aTimeout,
                                     uint16_t    aUdpPort)
{
    AssertPointerIsNotNull(aKeyString);

    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().SetEphemeralKey(aKeyString, aTimeout, aUdpPort);
}

void otBorderAgentClearEphemeralKey(otInstance *aInstance)
{
    AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().ClearEphemeralKey();
}

bool otBorderAgentIsEphemeralKeyActive(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().IsEphemeralKeyActive();
}

void otBorderAgentSetEphemeralKeyCallback(otInstance                       *aInstance,
                                          otBorderAgentEphemeralKeyCallback aCallback,
                                          void                             *aContext)
{
    AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().SetEphemeralKeyCallback(aCallback, aContext);
}

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

const otBorderAgentCounters *otBorderAgentGetCounters(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().GetCounters();
}

void otBorderAgentDisconnect(otInstance *aInstance) { AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().Disconnect(); }

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
