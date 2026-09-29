/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file implements the OpenThread Seeker API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_SEEKER_ENABLE

#include "instance/instance.hpp"

using namespace ot;

otError otSeekerStart(otInstance *aInstance, otSeekerScanEvaluator aScanEvaluator, void *aContext)
{
    return AsCoreType(aInstance).Get<MeshCoP::Seeker>().Start(aScanEvaluator, aContext);
}

void otSeekerStop(otInstance *aInstance) { AsCoreType(aInstance).Get<MeshCoP::Seeker>().Stop(); }

bool otSeekerIsRunning(otInstance *aInstance) { return AsCoreType(aInstance).Get<MeshCoP::Seeker>().IsRunning(); }

uint16_t otSeekerGetUdpPort(otInstance *aInstance) { return AsCoreType(aInstance).Get<MeshCoP::Seeker>().GetUdpPort(); }

otError otSeekerSetUdpPort(otInstance *aInstance, uint16_t aUdpPort)
{
    return AsCoreType(aInstance).Get<MeshCoP::Seeker>().SetUdpPort(aUdpPort);
}

otError otSeekerSetUpNextConnection(otInstance *aInstance, otSockAddr *aSockAddr)
{
    return AsCoreType(aInstance).Get<MeshCoP::Seeker>().SetUpNextConnection(AsCoreType(aSockAddr));
}

#endif // OPENTHREAD_CONFIG_SEEKER_ENABLE
