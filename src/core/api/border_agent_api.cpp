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

void otBorderAgentSetEnabled(otInstance *aInstance, bool aEnabled)
{
    AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().SetEnabled(aEnabled);
}

bool otBorderAgentIsEnabled(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().IsEnabled();
}

bool otBorderAgentIsActive(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().IsRunning();
}

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

uint16_t otBorderAgentGetUdpPort(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().GetUdpPort();
}

void otBorderAgentInitSessionIterator(otInstance *aInstance, otBorderAgentSessionIterator *aIterator)
{
    AsCoreType(aIterator).Init(AsCoreType(aInstance));
}

otError otBorderAgentGetNextSessionInfo(otBorderAgentSessionIterator *aIterator, otBorderAgentSessionInfo *aSessionInfo)
{
    AssertPointerIsNotNull(aSessionInfo);

    return AsCoreType(aIterator).GetNextSessionInfo(*aSessionInfo);
}

void otBorderAgentSetMeshCoPServiceChangedCallback(otInstance                                *aInstance,
                                                   otBorderAgentMeshCoPServiceChangedCallback aCallback,
                                                   void                                      *aContext)
{
    AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().SetServiceChangedCallback(aCallback, aContext);
}

otError otBorderAgentGetMeshCoPServiceTxtData(otInstance *aInstance, otBorderAgentMeshCoPServiceTxtData *aTxtData)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().PrepareServiceTxtData(*aTxtData);
}

const otBorderAgentCounters *otBorderAgentGetCounters(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<MeshCoP::BorderAgent>().GetCounters();
}

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

otBorderAgentEphemeralKeyState otBorderAgentEphemeralKeyGetState(otInstance *aInstance)
{
    return MapEnum(AsCoreType(aInstance).Get<MeshCoP::BorderAgent::EphemeralKeyManager>().GetState());
}

void otBorderAgentEphemeralKeySetEnabled(otInstance *aInstance, bool aEnabled)
{
    AsCoreType(aInstance).Get<MeshCoP::BorderAgent::EphemeralKeyManager>().SetEnabled(aEnabled);
}

otError otBorderAgentEphemeralKeyStart(otInstance *aInstance,
                                       const char *aKeyString,
                                       uint32_t    aTimeout,
                                       uint16_t    aUdpPort)
{
    AssertPointerIsNotNull(aKeyString);

    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent::EphemeralKeyManager>().Start(aKeyString, aTimeout, aUdpPort);
}

void otBorderAgentEphemeralKeyStop(otInstance *aInstance)
{
    AsCoreType(aInstance).Get<MeshCoP::BorderAgent::EphemeralKeyManager>().Stop();
}

uint16_t otBorderAgentEphemeralKeyGetUdpPort(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent::EphemeralKeyManager>().GetUdpPort();
}

void otBorderAgentEphemeralKeySetCallback(otInstance                       *aInstance,
                                          otBorderAgentEphemeralKeyCallback aCallback,
                                          void                             *aContext)
{
    AsCoreType(aInstance).Get<MeshCoP::BorderAgent::EphemeralKeyManager>().SetCallback(aCallback, aContext);
}

const char *otBorderAgentEphemeralKeyStateToString(otBorderAgentEphemeralKeyState aState)
{
    OT_ASSERT(aState <= OT_BORDER_AGENT_STATE_ACCEPTED);

    return MeshCoP::BorderAgent::EphemeralKeyManager::StateToString(MapEnum(aState));
}

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
