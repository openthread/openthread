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
 *   This file implements the OpenThread channel manager APIs.
 */

#include "openthread-core-config.h"
#include "openthread/channel_manager.h"

#include "common/instance.hpp"
#include "utils/channel_manager.hpp"

using namespace ot;

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER && OPENTHREAD_FTD

otError otChannelManagerRequestChannelChange(otInstance *aInstance, uint8_t aChannel)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetChannelManager().RequestChannelChange(aChannel);
}

uint16_t otChannelManagerGetDelay(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetChannelManager().GetDelay();
}

otError otChannelManagerSetDelay(otInstance *aInstance, uint16_t aMinDelay)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetChannelManager().SetDelay(aMinDelay);
}

uint32_t otChannelManagerGetSupportedChannels(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetChannelManager().GetSupportedChannels();
}

void  otChannelManagerSetSupportedChannels(otInstance *aInstance, uint32_t aChannelMask)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetChannelManager().SetSupportedChannels(aChannelMask);
}

#endif  // OPENTHREAD_ENABLE_CHANNEL_MANAGER && OPENTHREAD_FTD
