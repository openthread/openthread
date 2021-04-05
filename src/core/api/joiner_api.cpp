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
 *   This file implements the OpenThread Joiner API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_JOINER_ENABLE

#include <openthread/joiner.h>

#include "common/instance.hpp"
#include "common/locator-getters.hpp"

using namespace ot;

otError otJoinerStart(otInstance *     aInstance,
                      const char *     aPskd,
                      const char *     aProvisioningUrl,
                      const char *     aVendorName,
                      const char *     aVendorModel,
                      const char *     aVendorSwVersion,
                      const char *     aVendorData,
                      otJoinerCallback aCallback,
                      void *           aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Joiner>().Start(aPskd, aProvisioningUrl, aVendorName, aVendorModel, aVendorSwVersion,
                                                 aVendorData, aCallback, aContext);
}

void otJoinerStop(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<MeshCoP::Joiner>().Stop();
}

otJoinerState otJoinerGetState(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return static_cast<otJoinerState>(instance.Get<MeshCoP::Joiner>().GetState());
}

const otExtAddress *otJoinerGetId(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<MeshCoP::Joiner>().GetId();
}

otError otJoinerSetDiscerner(otInstance *aInstance, otJoinerDiscerner *aDiscerner)
{
    Error            error  = kErrorNone;
    MeshCoP::Joiner &joiner = static_cast<Instance *>(aInstance)->Get<MeshCoP::Joiner>();

    if (aDiscerner != nullptr)
    {
        error = joiner.SetDiscerner(*static_cast<const MeshCoP::JoinerDiscerner *>(aDiscerner));
    }
    else
    {
        error = joiner.ClearDiscerner();
    }

    return error;
}

const otJoinerDiscerner *otJoinerGetDiscerner(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Joiner>().GetDiscerner();
}

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE
