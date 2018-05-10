/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file implements the OpenThread Instance class.
 */

#define WPP_NAME "instance.tmh"

#include "instance.hpp"

#include <openthread/platform/misc.h>

#include "common/logging.hpp"
#include "common/new.hpp"
#include "thread/router_table.hpp"

namespace ot {

#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

// Define the raw storage used for OpenThread instance (in single-instance case).
static otDEFINE_ALIGNED_VAR(sInstanceRaw, sizeof(Instance), uint64_t);

#endif

Instance::Instance(void)
    : mTimerMilliScheduler(*this)
#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    , mTimerMicroScheduler(*this)
#endif
#if OPENTHREAD_MTD || OPENTHREAD_FTD
    , mActiveScanCallback(NULL)
    , mActiveScanCallbackContext(NULL)
    , mEnergyScanCallback(NULL)
    , mEnergyScanCallbackContext(NULL)
    , mNotifier(*this)
    , mSettings(*this)
    , mIp6(*this)
    , mThreadNetif(*this)
#if OPENTHREAD_ENABLE_APPLICATION_COAP
    , mApplicationCoap(*this)
#endif
#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
    , mChannelMonitor(*this)
#endif
#if OPENTHREAD_ENABLE_CHANNEL_MANAGER
    , mChannelManager(*this)
#endif
    , mMessagePool(*this)
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    , mLinkRaw(*this)
#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
    , mLogLevel(static_cast<otLogLevel>(OPENTHREAD_CONFIG_LOG_LEVEL))
#endif
    , mIsInitialized(false)
{
}

#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

Instance &Instance::InitSingle(void)
{
    Instance *instance = &Get();

    VerifyOrExit(instance->mIsInitialized == false);

    instance = new (&sInstanceRaw) Instance();

    instance->AfterInit();

exit:
    return *instance;
}

Instance &Instance::Get(void)
{
    void *instance = &sInstanceRaw;

    return *static_cast<Instance *>(instance);
}

#else // #if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

Instance *Instance::Init(void *aBuffer, size_t *aBufferSize)
{
    Instance *instance = NULL;

    VerifyOrExit(aBufferSize != NULL);

    // Make sure the input buffer is big enough
    VerifyOrExit(sizeof(Instance) <= *aBufferSize, *aBufferSize = sizeof(Instance));

    VerifyOrExit(aBuffer != NULL);

    instance = new (aBuffer) Instance();

    instance->AfterInit();

exit:
    return instance;
}

#endif // OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

void Instance::Reset(void)
{
    otPlatReset(this);
}

void Instance::AfterInit(void)
{
    mIsInitialized = true;
#if OPENTHREAD_MTD || OPENTHREAD_FTD

    // Restore datasets and network information

    GetSettings().Init();
    mThreadNetif.GetMle().Restore();

#if OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT

    if (otThreadGetAutoStart(this))
    {
        if (otIp6SetEnabled(this, true) == OT_ERROR_NONE)
        {
            // Only try to start Thread if we could bring up the interface
            if (otThreadSetEnabled(this, true) != OT_ERROR_NONE)
            {
                // Bring the interface down if Thread failed to start
                otIp6SetEnabled(this, false);
            }
        }
    }

#endif
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
}

#if OPENTHREAD_MTD || OPENTHREAD_FTD
void Instance::Finalize(void)
{
    VerifyOrExit(mIsInitialized == true);

    mIsInitialized = false;

    IgnoreReturnValue(otThreadSetEnabled(this, false));
    IgnoreReturnValue(otIp6SetEnabled(this, false));
    IgnoreReturnValue(otLinkSetEnabled(this, false));

exit:
    return;
}

void Instance::FactoryReset(void)
{
    GetSettings().Wipe();
    otPlatReset(this);
}

otError Instance::ErasePersistentInfo(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mThreadNetif.GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);
    GetSettings().Wipe();

exit:
    return error;
}

void Instance::RegisterActiveScanCallback(otHandleActiveScanResult aCallback, void *aContext)
{
    mActiveScanCallback        = aCallback;
    mActiveScanCallbackContext = aContext;
}

void Instance::InvokeActiveScanCallback(otActiveScanResult *aResult) const
{
    if (mActiveScanCallback != NULL)
    {
        mActiveScanCallback(aResult, mActiveScanCallbackContext);
    }
}

void Instance::RegisterEnergyScanCallback(otHandleEnergyScanResult aCallback, void *aContext)
{
    mEnergyScanCallback        = aCallback;
    mEnergyScanCallbackContext = aContext;
}

void Instance::InvokeEnergyScanCallback(otEnergyScanResult *aResult) const
{
    if (mEnergyScanCallback != NULL)
    {
        mEnergyScanCallback(aResult, mEnergyScanCallbackContext);
    }
}

// Specializations of the `Get<Type>()` method.

template <> Notifier &Instance::Get(void)
{
    return GetNotifier();
}

template <> MeshForwarder &Instance::Get(void)
{
    return GetThreadNetif().GetMeshForwarder();
}

template <> Mle::Mle &Instance::Get(void)
{
    return GetThreadNetif().GetMle();
}

template <> Mle::MleRouter &Instance::Get(void)
{
    return GetThreadNetif().GetMle();
}

template <> ChildTable &Instance::Get(void)
{
    return GetThreadNetif().GetMle().GetChildTable();
}

template <> RouterTable &Instance::Get(void)
{
    return GetThreadNetif().GetMle().GetRouterTable();
}

template <> Ip6::Netif &Instance::Get(void)
{
    return GetThreadNetif();
}

template <> Ip6::Ip6 &Instance::Get(void)
{
    return GetIp6();
}

template <> Mac::Mac &Instance::Get(void)
{
    return GetThreadNetif().GetMac();
}

template <> KeyManager &Instance::Get(void)
{
    return GetThreadNetif().GetKeyManager();
}

#if OPENTHREAD_FTD
template <> AddressResolver &Instance::Get(void)
{
    return GetThreadNetif().GetAddressResolver();
}

template <> MeshCoP::Leader &Instance::Get(void)
{
    return GetThreadNetif().GetLeader();
}

template <> MeshCoP::JoinerRouter &Instance::Get(void)
{
    return GetThreadNetif().GetJoinerRouter();
}
#endif // OPENTHREAD_FTD

template <> AnnounceBeginServer &Instance::Get(void)
{
    return GetThreadNetif().GetAnnounceBeginServer();
}

template <> DataPollManager &Instance::Get(void)
{
    return GetThreadNetif().GetMeshForwarder().GetDataPollManager();
}

template <> EnergyScanServer &Instance::Get(void)
{
    return GetThreadNetif().GetEnergyScanServer();
}

template <> PanIdQueryServer &Instance::Get(void)
{
    return GetThreadNetif().GetPanIdQueryServer();
}

template <> NetworkData::Leader &Instance::Get(void)
{
    return GetThreadNetif().GetNetworkDataLeader();
}

template <> Ip6::Mpl &Instance::Get(void)
{
    return GetIp6().GetMpl();
}

template <> Coap::Coap &Instance::Get(void)
{
    return GetThreadNetif().GetCoap();
}

template <> MeshCoP::ActiveDataset &Instance::Get(void)
{
    return GetThreadNetif().GetActiveDataset();
}

template <> MeshCoP::PendingDataset &Instance::Get(void)
{
    return GetThreadNetif().GetPendingDataset();
}

#if OPENTHREAD_ENABLE_APPLICATION_COAP
template <> Coap::ApplicationCoap &Instance::Get(void)
{
    return GetApplicationCoap();
}
#endif

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
template <> MeshCoP::Commissioner &Instance::Get(void)
{
    return GetThreadNetif().GetCommissioner();
}
#endif

#if OPENTHREAD_ENABLE_JOINER
template <> MeshCoP::Joiner &Instance::Get(void)
{
    return GetThreadNetif().GetJoiner();
}
#endif

#if OPENTHREAD_ENABLE_DNS_CLIENT
template <> Dns::Client &Instance::Get(void)
{
    return GetThreadNetif().GetDnsClient();
}
#endif

#if OPENTHREAD_ENABLE_DTLS
template <> MeshCoP::Dtls &Instance::Get(void)
{
    return GetThreadNetif().GetDtls();
}

template <> Coap::CoapSecure &Instance::Get(void)
{
    return GetThreadNetif().GetCoapSecure();
}
#endif

#if OPENTHREAD_ENABLE_DHCP6_CLIENT
template <> Dhcp6::Dhcp6Client &Instance::Get(void)
{
    return GetThreadNetif().GetDhcp6Client();
}
#endif

#if OPENTHREAD_ENABLE_JAM_DETECTION
template <> Utils::JamDetector &Instance::Get(void)
{
    return GetThreadNetif().GetJamDetector();
}
#endif

template <> Utils::ChildSupervisor &Instance::Get(void)
{
    return GetThreadNetif().GetChildSupervisor();
}

template <> Utils::SupervisionListener &Instance::Get(void)
{
    return GetThreadNetif().GetSupervisionListener();
}

#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
template <> Utils::ChannelMonitor &Instance::Get(void)
{
    return GetChannelMonitor();
}
#endif

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER
template <> Utils::ChannelManager &Instance::Get(void)
{
    return GetChannelManager();
}
#endif

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
template <> LinkRaw &Instance::Get(void)
{
    return GetLinkRaw();
}
#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

template <> TaskletScheduler &Instance::Get(void)
{
    return GetTaskletScheduler();
}

} // namespace ot
