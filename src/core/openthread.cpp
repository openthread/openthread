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
 *   This file implements the top-level interface to the OpenThread stack.
 */

#define WPP_NAME "openthread.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "openthread/openthread.h"
#include "openthread/platform/settings.h"
#include "openthread/platform/radio.h"
#include "openthread/platform/random.h"
#include "openthread/platform/misc.h"

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/message.hpp>
#include <common/new.hpp>
#include <common/settings.hpp>
#include <common/timer.hpp>
#include <crypto/mbedtls.hpp>
#include <net/icmp6.hpp>
#include <net/ip6.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_uris.hpp>
#include <openthread-instance.h>
#include <coap/coap_server.hpp>

using namespace Thread;

#ifdef __cplusplus
extern "C" {
#endif

bool otIcmp6IsEchoEnabled(otInstance *aInstance)
{
    return aInstance->mIp6.mIcmp.IsEchoEnabled();
}

void otIcmp6SetEchoEnabled(otInstance *aInstance, bool aEnabled)
{
    aInstance->mIp6.mIcmp.SetEchoEnabled(aEnabled);
}

ThreadError otIcmp6RegisterHandler(otInstance *aInstance, otIcmp6Handler *aHandler)
{
    return aInstance->mIp6.mIcmp.RegisterHandler(*static_cast<Ip6::IcmpHandler *>(aHandler));
}

ThreadError otIcmp6SendEchoRequest(otInstance *aInstance, otMessage aMessage,
                                   const otMessageInfo *aMessageInfo, uint16_t aIdentifier)
{
    return aInstance->mIp6.mIcmp.SendEchoRequest(*static_cast<Message *>(aMessage),
                                                 *static_cast<const Ip6::MessageInfo *>(aMessageInfo),
                                                 aIdentifier);
}

#ifdef __cplusplus
}  // extern "C"
#endif
