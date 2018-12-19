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
     * This method returns a reference to the timer milli scheduler object.
     *
     * @returns A reference to the timer milli scheduler object.
     *
     */
    TimerMilliScheduler &GetTimerMilliScheduler(void) { return mTimerMilliScheduler; }

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    /**
     * This method returns a reference to the timer micro scheduler object.
     *
     * @returns A reference to the timer micro scheduler object.
     *
     */
    TimerMicroScheduler &GetTimerMicroScheduler(void) { return mTimerMicroScheduler; }
#endif

    /**
     * This method returns a reference to the tasklet scheduler object.
     *
     * @returns A reference to the tasklet scheduler object.
     *
     */
    TaskletScheduler &GetTaskletScheduler(void) { return mTaskletScheduler; }

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

    /**
     * This method returns a reference to the `Notifier` object.
     *
     * @returns A reference to the `Notifier` object.
     *
     */
    Notifier &GetNotifier(void) { return mNotifier; }

    /**
     * This method returns a reference to the `Settings` object.
     *
     * @returns A reference to the `Settings` object.
     *
     */
    Settings &GetSettings(void) { return mSettings; }

#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    /**
     * This method returns a reference to the Heap object.
     *
     * @returns A reference to the Heap object.
     *
     */
    Utils::Heap &GetHeap(void) { return mHeap; }
#endif

    /**
     * This method returns a reference to the Ip6 object.
     *
     * @returns A reference to the Ip6 object.
     *
     */
    Ip6::Ip6 &GetIp6(void) { return mIp6; }

    /**
     * This method returns a reference to the Thread Netif object.
     *
     * @returns A reference to the Thread Netif object.
     *
     */
    ThreadNetif &GetThreadNetif(void) { return mThreadNetif; }

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

#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
    /**
     * This method returns a reference to ChannelMonitor object.
     *
     * @returns A reference to the ChannelMonitor object.
     *
     */
    Utils::ChannelMonitor &GetChannelMonitor(void) { return mChannelMonitor; }
#endif

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER
    /**
     * This method returns a reference to ChannelManager object.
     *
     * @returns A reference to the ChannelManager object.
     *
     */
    Utils::ChannelManager &GetChannelManager(void) { return mChannelManager; }
#endif

#if OPENTHREAD_CONFIG_ENABLE_ANNOUNCE_SENDER
    /**
     * This method returns a reference to AnnounceSender object.
     *
     * @returns A reference to the AnnounceSender object.
     *
     */
    AnnounceSender &GetAnnounceSender(void) { return mAnnounceSender; }
#endif

    /**
     * This method returns a reference to message pool object.
     *
     * @returns A reference to the message pool object.
     *
     */
    MessagePool &GetMessagePool(void) { return mMessagePool; }

#if OPENTHREAD_ENABLE_VENDOR_EXTENSION
    /**
     * This method returns a reference to vendor extension object.
     *
     * @returns A reference to the vendor extension object.
     *
     */
    Extension::ExtensionBase &GetExtension(void) { return mExtension; }
#endif

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    /**
     * This method returns a reference to LinkRaw object.
     *
     * @returns A reference to the LinkRaw object.
     *
     */
    Mac::LinkRaw &GetLinkRaw(void) { return mLinkRaw; }
#endif

    /**
     * This template method returns a reference to a given `Type` object belonging to the OpenThread instance.
     *
     * For example, `Get<MeshForwarder>()` returns a reference to the `MeshForwarder` object of the instance.
     *
     * Note that any `Type` for which the `Get<Type>` is defined MUST be uniquely accessible from the OpenThread
     * `Instance` through the member variable property hierarchy.
     *
     * Specializations of the `Get<Type>()` method are defined in the `instance.cpp`. The specializations are defined
     * for any class (type) which can use `GetOwner<Type>` method, i.e., any class that is an owner of a callback
     * providing object such as a `Timer`,`Tasklet`, or any sub-class of `OwnerLocator`.
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
    Crypto::MbedTls mMbedTls;
    Utils::Heap     mHeap;
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
    return GetNotifier();
}

template <> inline MeshForwarder &Instance::Get(void)
{
    return GetThreadNetif().GetMeshForwarder();
}

template <> inline Mle::Mle &Instance::Get(void)
{
    return GetThreadNetif().GetMle();
}

template <> inline Mle::MleRouter &Instance::Get(void)
{
    return GetThreadNetif().GetMle();
}

template <> inline ChildTable &Instance::Get(void)
{
    return GetThreadNetif().GetMle().GetChildTable();
}

template <> inline RouterTable &Instance::Get(void)
{
    return GetThreadNetif().GetMle().GetRouterTable();
}

template <> inline Ip6::Netif &Instance::Get(void)
{
    return GetThreadNetif();
}

template <> inline Ip6::Ip6 &Instance::Get(void)
{
    return GetIp6();
}

template <> inline Mac::Mac &Instance::Get(void)
{
    return GetThreadNetif().GetMac();
}

template <> inline Mac::SubMac &Instance::Get(void)
{
    return GetThreadNetif().GetMac().GetSubMac();
}

template <> inline KeyManager &Instance::Get(void)
{
    return GetThreadNetif().GetKeyManager();
}

#if OPENTHREAD_FTD
template <> inline AddressResolver &Instance::Get(void)
{
    return GetThreadNetif().GetAddressResolver();
}

template <> inline MeshCoP::Leader &Instance::Get(void)
{
    return GetThreadNetif().GetLeader();
}

template <> inline MeshCoP::JoinerRouter &Instance::Get(void)
{
    return GetThreadNetif().GetJoinerRouter();
}
#endif // OPENTHREAD_FTD

template <> inline AnnounceBeginServer &Instance::Get(void)
{
    return GetThreadNetif().GetAnnounceBeginServer();
}

template <> inline DataPollManager &Instance::Get(void)
{
    return GetThreadNetif().GetMeshForwarder().GetDataPollManager();
}

template <> inline EnergyScanServer &Instance::Get(void)
{
    return GetThreadNetif().GetEnergyScanServer();
}

template <> inline PanIdQueryServer &Instance::Get(void)
{
    return GetThreadNetif().GetPanIdQueryServer();
}

template <> inline NetworkData::Leader &Instance::Get(void)
{
    return GetThreadNetif().GetNetworkDataLeader();
}

template <> inline Ip6::Mpl &Instance::Get(void)
{
    return GetIp6().GetMpl();
}

template <> inline Coap::Coap &Instance::Get(void)
{
    return GetThreadNetif().GetCoap();
}

template <> inline MeshCoP::ActiveDataset &Instance::Get(void)
{
    return GetThreadNetif().GetActiveDataset();
}

template <> inline MeshCoP::PendingDataset &Instance::Get(void)
{
    return GetThreadNetif().GetPendingDataset();
}

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
template <> inline TimeSync &Instance::Get(void)
{
    return GetThreadNetif().GetTimeSync();
}
#endif

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
template <> inline MeshCoP::Commissioner &Instance::Get(void)
{
    return GetThreadNetif().GetCommissioner();
}
#endif

#if OPENTHREAD_ENABLE_JOINER
template <> inline MeshCoP::Joiner &Instance::Get(void)
{
    return GetThreadNetif().GetJoiner();
}
#endif

#if OPENTHREAD_ENABLE_DNS_CLIENT
template <> inline Dns::Client &Instance::Get(void)
{
    return GetThreadNetif().GetDnsClient();
}
#endif

#if OPENTHREAD_ENABLE_DTLS
template <> inline MeshCoP::Dtls &Instance::Get(void)
{
    return GetThreadNetif().GetDtls();
}

template <> inline Coap::CoapSecure &Instance::Get(void)
{
    return GetThreadNetif().GetCoapSecure();
}
#endif

#if OPENTHREAD_ENABLE_DHCP6_CLIENT
template <> inline Dhcp6::Dhcp6Client &Instance::Get(void)
{
    return GetThreadNetif().GetDhcp6Client();
}
#endif

#if OPENTHREAD_ENABLE_JAM_DETECTION
template <> inline Utils::JamDetector &Instance::Get(void)
{
    return GetThreadNetif().GetJamDetector();
}
#endif

#if OPENTHREAD_ENABLE_SNTP_CLIENT
template <> inline Sntp::Client &Instance::Get(void)
{
    return GetThreadNetif().GetSntpClient();
}
#endif

template <> inline Utils::ChildSupervisor &Instance::Get(void)
{
    return GetThreadNetif().GetChildSupervisor();
}

template <> inline Utils::SupervisionListener &Instance::Get(void)
{
    return GetThreadNetif().GetSupervisionListener();
}

#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
template <> inline Utils::ChannelMonitor &Instance::Get(void)
{
    return GetChannelMonitor();
}
#endif

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER
template <> inline Utils::ChannelManager &Instance::Get(void)
{
    return GetChannelManager();
}
#endif

#if OPENTHREAD_ENABLE_BORDER_AGENT
template <> inline MeshCoP::BorderAgent &Instance::Get(void)
{
    return GetThreadNetif().GetBorderAgent();
}
#endif

#if OPENTHREAD_CONFIG_ENABLE_ANNOUNCE_SENDER
template <> inline AnnounceSender &Instance::Get(void)
{
    return GetAnnounceSender();
}
#endif

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
template <> inline Mac::LinkRaw &Instance::Get(void)
{
    return GetLinkRaw();
}

#if OPENTHREAD_RADIO
template <> inline Mac::SubMac &Instance::Get(void)
{
    return GetLinkRaw().GetSubMac();
}
#endif

#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

template <> inline TaskletScheduler &Instance::Get(void)
{
    return GetTaskletScheduler();
}

#if OPENTHREAD_ENABLE_VENDOR_EXTENSION
template <> inline Extension::ExtensionBase &Instance::Get(void)
{
    return GetExtension();
}
#endif

/**
 * @}
 *
 */

} // namespace ot

#endif // INSTANCE_HPP_
