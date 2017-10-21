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
 * @brief
 *  This file defines the structure of the variables required for all instances of OpenThread API.
 */

#ifndef OPENTHREADINSTANCE_H_
#define OPENTHREADINSTANCE_H_

#include "openthread-core-config.h"

#include "utils/wrap_stdint.h"
#include "utils/wrap_stdbool.h"

#include <openthread/types.h>
#include <openthread/platform/logging.h>

#include "openthread-single-instance.h"
#if OPENTHREAD_ENABLE_RAW_LINK_API
#include "api/link_raw.hpp"
#endif
#include "coap/coap.hpp"
#include "crypto/heap.hpp"
#include "crypto/mbedtls.hpp"
#include "net/ip6.hpp"
#include "thread/thread_netif.hpp"

/**
 * This type represents all the static / global variables used by OpenThread allocated in one place.
 */
typedef struct otInstance
{
    //
    // Callbacks
    //

    ot::Ip6::NetifCallback mNetifCallback[OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS];

    otIp6ReceiveCallback mReceiveIp6DatagramCallback;
    void *mReceiveIp6DatagramCallbackContext;

    otHandleActiveScanResult mActiveScanCallback;
    void *mActiveScanCallbackContext;

    otHandleEnergyScanResult mEnergyScanCallback;
    void *mEnergyScanCallbackContext;

    //
    // State
    //

    ot::TaskletScheduler mTaskletScheduler;
    ot::TimerMilliScheduler mTimerMilliScheduler;
#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    ot::TimerMicroScheduler mTimerMicroScheduler;
#endif

#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    ot::Crypto::MbedTls mMbedTls;
    ot::Crypto::Heap    mMbedTlsHeap;
#endif
    ot::Ip6::Ip6 mIp6;
    ot::ThreadNetif mThreadNetif;

#if OPENTHREAD_ENABLE_RAW_LINK_API
    ot::LinkRaw mLinkRaw;
#endif // OPENTHREAD_ENABLE_RAW_LINK_API

#if OPENTHREAD_ENABLE_APPLICATION_COAP
    ot::Coap::ApplicationCoap mApplicationCoap;
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP

#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
    otLogLevel mLogLevel;
#endif // OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
    ot::MessagePool mMessagePool;

    // Constructor
    otInstance(void);

} otInstance;

#endif  // OPENTHREADINSTANCE_H_
