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

#include <openthread/joiner.h>

#include "common/instance.hpp"

using namespace ot;

otError otJoinerStart(otInstance *     aInstance,
                      const char *     aPSKd,
                      const char *     aProvisioningUrl,
                      const char *     aVendorName,
                      const char *     aVendorModel,
                      const char *     aVendorSwVersion,
                      const char *     aVendorData,
                      otJoinerCallback aCallback,
                      void *           aContext)
{
    otError error = OT_ERROR_DISABLED_FEATURE;
#if OPENTHREAD_ENABLE_JOINER
    Instance &instance = *static_cast<Instance *>(aInstance);

    error = instance.GetThreadNetif().GetJoiner().Start(aPSKd, aProvisioningUrl, aVendorName, aVendorModel,
                                                        aVendorSwVersion, aVendorData, aCallback, aContext);
#else
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPSKd);
    OT_UNUSED_VARIABLE(aProvisioningUrl);
    OT_UNUSED_VARIABLE(aVendorName);
    OT_UNUSED_VARIABLE(aVendorModel);
    OT_UNUSED_VARIABLE(aVendorSwVersion);
    OT_UNUSED_VARIABLE(aVendorData);
    OT_UNUSED_VARIABLE(aCallback);
    OT_UNUSED_VARIABLE(aContext);
#endif

    return error;
}

otError otJoinerStop(otInstance *aInstance)
{
    otError error = OT_ERROR_DISABLED_FEATURE;

#if OPENTHREAD_ENABLE_JOINER
    Instance &instance = *static_cast<Instance *>(aInstance);

    error = instance.GetThreadNetif().GetJoiner().Stop();
#else
    OT_UNUSED_VARIABLE(aInstance);
#endif

    return error;
}

otJoinerState otJoinerGetState(otInstance *aInstance)
{
    otJoinerState state = OT_JOINER_STATE_IDLE;

#if OPENTHREAD_ENABLE_JOINER
    Instance &instance = *static_cast<Instance *>(aInstance);

    state = instance.GetThreadNetif().GetJoiner().GetState();
#else
    OT_UNUSED_VARIABLE(aInstance);
#endif

    return state;
}

otError otJoinerGetId(otInstance *aInstance, otExtAddress *aJoinerId)
{
    otError error = OT_ERROR_DISABLED_FEATURE;

#if OPENTHREAD_ENABLE_JOINER
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetThreadNetif().GetJoiner().GetJoinerId(*static_cast<Mac::ExtAddress *>(aJoinerId));
    error = OT_ERROR_NONE;
#else
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aJoinerId);
#endif

    return error;
}
