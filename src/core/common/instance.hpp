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
 *  This file defines OpenThread instance class.
 */

#ifndef INSTANCE_HPP_
#define INSTANCE_HPP_

#include "openthread-core-config.h"

#include "utils/wrap_stdbool.h"
#include "utils/wrap_stdint.h"

#include <openthread/error.h>
#include <openthread/platform/logging.h>

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
#include "common/message.hpp"
#include "mac/link_raw.hpp"
#endif
#if OPENTHREAD_FTD || OPENTHREAD_MTD
#include "coap/coap.hpp"
#include "common/code_utils.hpp"
#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
#include "crypto/mbedtls.hpp"
#include "utils/heap.hpp"
#endif
#include "common/notifier.hpp"
#include "common/settings.hpp"
#include "meshcop/border_agent.hpp"
#include "net/ip6.hpp"
#include "thread/announce_sender.hpp"
#include "thread/link_quality.hpp"
#include "thread/thread_netif.hpp"
#if OPENTHREAD_ENABLE_CHANNEL_MANAGER
#include "utils/channel_manager.hpp"
#endif
#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
#include "utils/channel_monitor.hpp"
#endif
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD
#if OPENTHREAD_ENABLE_VENDOR_EXTENSION
#include "common/extension.hpp"
#endif

/**
 * @addtogroup core-instance
 *
 * @brief
 *   This module includes definitions for OpenThread instance.
 *
 * @{
 *
 */

/**
 * This struct represents an opaque (and empty) type corresponding to an OpenThread instance object.
 *
 */
typedef struct otInstance
{
} otInstance;

namespace ot {

/**
 * This class represents an OpenThread instance.
 *
 * This class contains all the components used by OpenThread.
 *
 */
class Instance : public otInstance
{
public:
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    /**
      * This static method initializes the OpenThread instance.
      *
      * This function must be called before any other calls on OpenThread instance.
      *
      * @param[in]    aBuffer      The buffer for OpenThread to use for allocating the Instance.
      * @param[inout] aBufferSize  On input, the size of `aBuffer`. On output, if not enough space for `Instance`, the
                                   number of bytes required for `Instance`.
      *
      * @returns  A pointer to the new OpenThread instance.
      *
      */
    static Instance *Init(void *aBuffer, size_t *aBufferSize);

#else // OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

    /**
     * This static method initializes the single OpenThread instance.
     *
     * This method initializes OpenThread and prepares it for subsequent OpenThread API calls. This function must be
     * called before any other calls to OpenThread.
     *
     * @returns A reference to the single OpenThread instance.
     *
     */
    static Instance &InitSingle(void);

    /**
     * This static method returns a reference to the single OpenThread instance.
     *
     * @returns A reference to the single OpenThread instance.
     *
     */
    static Instance &Get(void);
#endif

    /**
     * This method indicates whether or not the instance is valid/initialized and not yet finalized.
     *
     * @returns TRUE if the instance is valid/initialized, FALSE otherwise.
     *
     */
    bool IsInitialized(void) const { return mIsInitialized; }

    /**
     * This method triggers a platform reset.
     *
     * The reset process ensures that all the OpenThread state/info (stored in volatile memory) is erased. Note that
     * this method does not erase any persistent state/info saved in non-volatile memory.
     *
     */
    void Reset(void);

    /**
     * This method returns the active log level.
     *
     * @returns The log level.
     *
     */
    otLogLevel GetLogLevel(void) const
#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
    {
        return mLogLevel;
    }
#else
    {
        return static_cast<otLogLevel>(OPENTHREAD_CONFIG_LOG_LEVEL);
    }
#endif

#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
    /**
     * This method sets the log level.
     *
     * @param[in] aLogLevel  A log level.
     *
     */
    void SetLogLevel(otLogLevel aLogLevel) { mLogLevel = aLogLevel; }
#endif

    /**
     * This method finalizes the OpenThread instance.
     *
     * This method should be called when OpenThread instance is no longer in use.
     *
     */
    void Finalize(void);

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    /**
     * This method deletes all the settings stored in non-volatile memory, and then triggers a platform reset.
     *
     */
    void FactoryReset(void);

    /**
     * This method erases all the OpenThread persistent info (network settings) stored in non-volatile memory.
     *
     * Erase is successful/allowed only if the device is in `disabled` state/role.
     *
     * @retval OT_ERROR_NONE           All persistent info/state was erased successfully.
     * @retval OT_ERROR_INVALID_STATE  Device is not in `disabled` state/role.
     *
     */
    otError ErasePersistentInfo(void);

    /**
     * This method registers the active scan callback.
     *
     * Subsequent calls to this method will overwrite the previous callback handler.
     *
     * @param[in]  aCallback   A pointer to the callback function pointer.
     * @param[in]  aContext    A pointer to application-specific context.
     *
     */
    void RegisterActiveScanCallback(otHandleActiveScanResult aCallback, void *aContext);

    /**
     * This method invokes the previously registered active scan callback with a given scan result.
     *
     * @param[in]  aResult     A pointer to active scan result.
     *
     */
    void InvokeActiveScanCallback(otActiveScanResult *aResult) const;

    /**
     * This method registers the energy scan callback.
     *
     * Subsequent calls to this method will overwrite the previous callback handler.
     *
     * @param[in]  aCallback   A pointer to the callback function pointer.
     * @param[in]  aContext    A pointer to application-specific context.
     *
     */
    void RegisterEnergyScanCallback(otHandleEnergyScanResult aCallback, void *aContext);

    /**
     * This method invokes the previously registered energy scan callback with a given result.
     *
     * @param[in]  aResult     A pointer to energy scan result.
     *
     */
    void InvokeEnergyScanCallback(otEnergyScanResult *aResult) const;

#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    /**
     * This method returns a reference to the Heap object.
     *
     * @returns A reference to the Heap object.
     *
     */
    Utils::Heap &GetHeap(void) { return mHeap; }
#endif

#if OPENTHREAD_ENABLE_APPLICATION_COAP
    /**
     * This method returns a reference to application COAP object.
     *
     * @returns A reference to the application COAP object.
     *
     */
    Coap::Coap &GetApplicationCoap(void) { return mApplicationCoap; }
#endif

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    /**
     * This method returns a reference to application COAP Secure object.
     *
     * @returns A reference to the application COAP Secure object.
     *
     */
    Coap::CoapSecure &GetApplicationCoapSecure(void) { return mApplicationCoapSecure; }
#endif

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

    /**
     * This template method returns a reference to a given `Type` object belonging to the OpenThread instance.
     *
     * For example, `Get<MeshForwarder>()` returns a reference to the `MeshForwarder` object of the instance.
     *
     * Note that any `Type` for which the `Get<Type>` is defined MUST be uniquely accessible from the OpenThread
     * `Instance` through the member variable property hierarchy.
     *
     * Specializations of the `Get<Type>()` method are defined in this file after the `Instance` class definition.
     *
     * @returns A reference to the `Type` object of the instance.
     *
     */
    template <typename Type> inline Type &Get(void);

private:
    Instance(void);
    void AfterInit(void);

    TimerMilliScheduler mTimerMilliScheduler;
#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    TimerMicroScheduler mTimerMicroScheduler;
#endif
    TaskletScheduler mTaskletScheduler;

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    otHandleActiveScanResult mActiveScanCallback;
    void *                   mActiveScanCallbackContext;
    otHandleEnergyScanResult mEnergyScanCallback;
    void *                   mEnergyScanCallbackContext;

    Notifier mNotifier;
    Settings mSettings;

#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
#if OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS
    Crypto::MbedTls mMbedTls;
#endif
    Utils::Heap mHeap;
#endif

    Ip6::Ip6    mIp6;
    ThreadNetif mThreadNetif;

#if OPENTHREAD_ENABLE_APPLICATION_COAP
    Coap::Coap mApplicationCoap;
#endif

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    Coap::CoapSecure mApplicationCoapSecure;
#endif

#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
    Utils::ChannelMonitor mChannelMonitor;
#endif

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER
    Utils::ChannelManager mChannelManager;
#endif

#if OPENTHREAD_CONFIG_ENABLE_ANNOUNCE_SENDER
    AnnounceSender mAnnounceSender;
#endif

    MessagePool mMessagePool;
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    Mac::LinkRaw mLinkRaw;
#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
    otLogLevel mLogLevel;
#endif
#if OPENTHREAD_ENABLE_VENDOR_EXTENSION
    Extension::ExtensionBase &mExtension;
#endif
    bool mIsInitialized;
};

// Specializations of the `Get<Type>()` method.

#if OPENTHREAD_MTD || OPENTHREAD_FTD
template <> inline Notifier &Instance::Get(void)
{
    return mNotifier;
}

template <> inline Settings &Instance::Get(void)
{
    return mSettings;
}

template <> inline MeshForwarder &Instance::Get(void)
{
    return mThreadNetif.mMeshForwarder;
}

template <> inline Mle::Mle &Instance::Get(void)
{
    return mThreadNetif.mMleRouter;
}

template <> inline Mle::MleRouter &Instance::Get(void)
{
    return mThreadNetif.mMleRouter;
}

template <> inline ChildTable &Instance::Get(void)
{
    return mThreadNetif.mMleRouter.mChildTable;
}

template <> inline RouterTable &Instance::Get(void)
{
    return mThreadNetif.mMleRouter.mRouterTable;
}

template <> inline Ip6::Netif &Instance::Get(void)
{
    return mThreadNetif;
}

template <> inline ThreadNetif &Instance::Get(void)
{
    return mThreadNetif;
}

template <> inline Ip6::Ip6 &Instance::Get(void)
{
    return mIp6;
}

template <> inline Mac::Mac &Instance::Get(void)
{
    return mThreadNetif.mMac;
}

template <> inline Mac::SubMac &Instance::Get(void)
{
    return mThreadNetif.mMac.mSubMac;
}

#if OPENTHREAD_ENABLE_MAC_FILTER
template <> inline Mac::Filter &Instance::Get(void)
{
    return mThreadNetif.mMac.mFilter;
}
#endif

template <> inline Lowpan::Lowpan &Instance::Get(void)
{
    return mThreadNetif.mLowpan;
}

template <> inline KeyManager &Instance::Get(void)
{
    return mThreadNetif.mKeyManager;
}

template <> inline Ip6::Filter &Instance::Get(void)
{
    return mThreadNetif.mIp6Filter;
}

#if OPENTHREAD_FTD
template <> inline SourceMatchController &Instance::Get(void)
{
    return mThreadNetif.mMeshForwarder.mSourceMatchController;
}

template <> inline AddressResolver &Instance::Get(void)
{
    return mThreadNetif.mAddressResolver;
}

template <> inline MeshCoP::Leader &Instance::Get(void)
{
    return mThreadNetif.mLeader;
}

template <> inline MeshCoP::JoinerRouter &Instance::Get(void)
{
    return mThreadNetif.mJoinerRouter;
}
#endif // OPENTHREAD_FTD

template <> inline AnnounceBeginServer &Instance::Get(void)
{
    return mThreadNetif.mAnnounceBegin;
}

template <> inline DataPollManager &Instance::Get(void)
{
    return mThreadNetif.mMeshForwarder.mDataPollManager;
}

template <> inline EnergyScanServer &Instance::Get(void)
{
    return mThreadNetif.mEnergyScan;
}

template <> inline PanIdQueryServer &Instance::Get(void)
{
    return mThreadNetif.mPanIdQuery;
}

#if OPENTHREAD_ENABLE_BORDER_ROUTER || OPENTHREAD_ENABLE_SERVICE
template <> inline NetworkData::Local &Instance::Get(void)
{
    return mThreadNetif.mNetworkDataLocal;
}
#endif

template <> inline NetworkData::Leader &Instance::Get(void)
{
    return mThreadNetif.mNetworkDataLeader;
}

template <> inline Ip6::Routes &Instance::Get(void)
{
    return mIp6.mRoutes;
}

template <> inline Ip6::Udp &Instance::Get(void)
{
    return mIp6.mUdp;
}

template <> inline Ip6::Icmp &Instance::Get(void)
{
    return mIp6.mIcmp;
}

template <> inline Ip6::Mpl &Instance::Get(void)
{
    return mIp6.mMpl;
}

template <> inline Coap::Coap &Instance::Get(void)
{
    return mThreadNetif.mCoap;
}

#if OPENTHREAD_ENABLE_DTLS
template <> inline Coap::CoapSecure &Instance::Get(void)
{
    return mThreadNetif.mCoapSecure;
}
#endif

template <> inline MeshCoP::ActiveDataset &Instance::Get(void)
{
    return mThreadNetif.mActiveDataset;
}

template <> inline MeshCoP::PendingDataset &Instance::Get(void)
{
    return mThreadNetif.mPendingDataset;
}

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
template <> inline TimeSync &Instance::Get(void)
{
    return mThreadNetif.mTimeSync;
}
#endif

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
template <> inline MeshCoP::Commissioner &Instance::Get(void)
{
    return mThreadNetif.mCommissioner;
}
#endif

#if OPENTHREAD_ENABLE_JOINER
template <> inline MeshCoP::Joiner &Instance::Get(void)
{
    return mThreadNetif.mJoiner;
}
#endif

#if OPENTHREAD_ENABLE_DNS_CLIENT
template <> inline Dns::Client &Instance::Get(void)
{
    return mThreadNetif.mDnsClient;
}
#endif

#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
template <> inline NetworkDiagnostic::NetworkDiagnostic &Instance::Get(void)
{
    return mThreadNetif.mNetworkDiagnostic;
}
#endif

#if OPENTHREAD_ENABLE_DHCP6_CLIENT
template <> inline Dhcp6::Dhcp6Client &Instance::Get(void)
{
    return mThreadNetif.mDhcp6Client;
}
#endif

#if OPENTHREAD_ENABLE_DHCP6_SERVER
template <> inline Dhcp6::Dhcp6Server &Instance::Get(void)
{
    return mThreadNetif.mDhcp6Server;
}
#endif

#if OPENTHREAD_CONFIG_ENABLE_SLAAC
template <> inline Utils::Slaac &Instance::Get(void)
{
    return mThreadNetif.mSlaac;
}
#endif

#if OPENTHREAD_ENABLE_JAM_DETECTION
template <> inline Utils::JamDetector &Instance::Get(void)
{
    return mThreadNetif.mJamDetector;
}
#endif

#if OPENTHREAD_ENABLE_SNTP_CLIENT
template <> inline Sntp::Client &Instance::Get(void)
{
    return mThreadNetif.mSntpClient;
}
#endif

template <> inline Utils::ChildSupervisor &Instance::Get(void)
{
    return mThreadNetif.mChildSupervisor;
}

template <> inline Utils::SupervisionListener &Instance::Get(void)
{
    return mThreadNetif.mSupervisionListener;
}

#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
template <> inline Utils::ChannelMonitor &Instance::Get(void)
{
    return mChannelMonitor;
}
#endif

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER
template <> inline Utils::ChannelManager &Instance::Get(void)
{
    return mChannelManager;
}
#endif

#if OPENTHREAD_ENABLE_BORDER_AGENT
template <> inline MeshCoP::BorderAgent &Instance::Get(void)
{
    return mThreadNetif.mBorderAgent;
}
#endif

#if OPENTHREAD_CONFIG_ENABLE_ANNOUNCE_SENDER
template <> inline AnnounceSender &Instance::Get(void)
{
    return mAnnounceSender;
}
#endif

template <> inline MessagePool &Instance::Get(void)
{
    return mMessagePool;
}

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
template <> inline Mac::LinkRaw &Instance::Get(void)
{
    return mLinkRaw;
}

#if OPENTHREAD_RADIO
template <> inline Mac::SubMac &Instance::Get(void)
{
    return mLinkRaw.mSubMac;
}
#endif

#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

template <> inline TaskletScheduler &Instance::Get(void)
{
    return mTaskletScheduler;
}

template <> inline TimerMilliScheduler &Instance::Get(void)
{
    return mTimerMilliScheduler;
}

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
template <> inline TimerMicroScheduler &Instance::Get(void)
{
    return mTimerMicroScheduler;
}
#endif

#if OPENTHREAD_ENABLE_VENDOR_EXTENSION
template <> inline Extension::ExtensionBase &Instance::Get(void)
{
    return mExtension;
}
#endif

/**
 * @}
 *
 */

} // namespace ot

#endif // INSTANCE_HPP_
