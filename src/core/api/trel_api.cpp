/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements the OpenThread TREL (Thread Radio Encapsulation Link) APIs for Thread Over Infrastructure.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#include <openthread/trel.h>

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/instance.hpp"

using namespace ot;

void otTrelEnable(otInstance *aInstance)
{
    AsCoreType(aInstance).Get<Trel::Interface>().Enable();
}

void otTrelDisable(otInstance *aInstance)
{
    AsCoreType(aInstance).Get<Trel::Interface>().Disable();
}

bool otTrelIsEnabled(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Trel::Interface>().IsEnabled();
}

void otTrelInitPeerIterator(otInstance *aInstance, otTrelPeerIterator *aIterator)
{
    AsCoreType(aInstance).Get<Trel::Interface>().InitIterator(*aIterator);
}

const otTrelPeer *otTrelGetNextPeer(otInstance *aInstance, otTrelPeerIterator *aIterator)
{
    return AsCoreType(aInstance).Get<Trel::Interface>().GetNextPeer(*aIterator);
}

void otTrelSetFilterEnabled(otInstance *aInstance, bool aEnable)
{
    AsCoreType(aInstance).Get<Trel::Interface>().SetFilterEnabled(aEnable);
}

bool otTrelIsFilterEnabled(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Trel::Interface>().IsFilterEnabled();
}

#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
