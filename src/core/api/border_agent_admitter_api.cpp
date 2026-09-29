/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements the OpenThread Border Agent Admitter API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLE

#include <openthread/border_agent_admitter.h>

#include "instance/instance.hpp"

using namespace ot;

void otBorderAdmitterSetEnabled(otInstance *aInstance, bool aEnable)
{
    AsCoreType(aInstance).Get<MeshCoP::BorderAgent::Admitter>().SetEnabled(aEnable);
}

bool otBorderAdmitterIsEnabled(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent::Admitter>().IsEnabled();
}

bool otBorderAdmitterIsPrimeAdmitter(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent::Admitter>().IsPrimeAdmitter();
}

bool otBorderAdmitterIsActiveCommissioner(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent::Admitter>().IsActiveCommissioner();
}

bool otBorderAdmitterIsPetitionRejected(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent::Admitter>().IsPetitionRejected();
}

uint16_t otBorderAdmitterGetJoinerUdpPort(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::BorderAgent::Admitter>().GetJoinerUdpPort();
}

void otBorderAdmitterSetJoinerUdpPort(otInstance *aInstance, uint16_t aUdpPort)
{
    AsCoreType(aInstance).Get<MeshCoP::BorderAgent::Admitter>().SetJoinerUdpPort(aUdpPort);
}

void otBorderAdmitterInitIterator(otInstance *aInstance, otBorderAdmitterIterator *aIterator)
{
    AsCoreType(aIterator).Init(AsCoreType(aInstance));
}

otError otBorderAdmitterGetNextEnrollerInfo(otBorderAdmitterIterator     *aIterator,
                                            otBorderAdmitterEnrollerInfo *aEnrollerInfo)
{
    AssertPointerIsNotNull(aEnrollerInfo);

    return AsCoreType(aIterator).GetNextEnrollerInfo(*aEnrollerInfo);
}

otError otBorderAdmitterGetNextJoinerInfo(otBorderAdmitterIterator *aIterator, otBorderAdmitterJoinerInfo *aJoinerInfo)
{
    AssertPointerIsNotNull(aJoinerInfo);

    return AsCoreType(aIterator).GetNextJoinerInfo(*aJoinerInfo);
}

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLE
