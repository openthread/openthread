/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#ifndef OPENTHREADCONTEXT_H_
#define OPENTHREADCONTEXT_H_

#ifndef OPEN_THREAD_DRIVER
#include <stdint.h>
#include <stdbool.h>
#endif

#include <openthread-types.h>
#include <thread/thread_netif.hpp>
#include <net/ip6_mpl.hpp>
#include <net/ip6_routes.hpp>

#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#endif

/**
 * This type represents all the static / global variables used by OpenThread allocated in one place.
 */
typedef struct otContext
{
    //
    // Callbacks
    //
    Thread::Ip6::NetifCallback mNetifCallback;
    otReceiveIp6DatagramCallback mReceiveIp6DatagramCallback;
    void *mReceiveIp6DatagramCallbackContext;

    //
    // Variables
    //

    uint16_t mEphemeralPort;

    Thread::Ip6::IcmpHandler *mIcmpHandlers;
    bool mIsEchoEnabled;
    uint16_t mNextId;
    Thread::Ip6::IcmpEcho *mEchoClients;

    Thread::Ip6::Route *mRoutes;

    Thread::Ip6::Netif *mNetifListHead;
    int mNextInterfaceId;

    Thread::Mac::Mac *mMac;

    int mNumFreeBuffers;
    Thread::Buffer mBuffers[Thread::kNumBuffers];
    Thread::Buffer *mFreeBuffers;
    Thread::MessageList mAll;

    Thread::Timer *mTimerHead;
    Thread::Timer *mTimerTail;

    Thread::Tasklet *mTaskletHead;
    Thread::Tasklet *mTaskletTail;

    Thread::Ip6::UdpSocket *mUdpSockets;

    otCryptoContext mCryptoContext;

    Thread::LinkQualityInfo mNoiseFloorAverage;  // Store the noise floor average.

    bool mEnabled;
    Thread::ThreadNetif mThreadNetif;

    Thread::Ip6::Mpl mMpl;

    // Constructor
    otContext(void);

} otContext;

// Number of aligned bytes required for the context structure
const size_t cAlignedContextSize = otALIGNED_VAR_SIZE(sizeof(otContext), uint64_t) * sizeof(uint64_t);

// Number of bytes indicated in the public header file for the context structure
const size_t cPublicContextSize = OT_CONTEXT_SIZE;

// Ensure we are initializing the public definition of the size of the context structure correctly
C_ASSERT(cPublicContextSize >= cAlignedContextSize);

#endif  // OPENTHREADCONTEXT_H_
