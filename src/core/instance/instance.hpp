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

#include <stdbool.h>
#include <stdint.h>

#include <openthread/heap.h>
#if OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
#include <openthread/platform/memory.h>
#endif

#include "common/array.hpp"
#include "common/as_core_type.hpp"
#include "common/error.hpp"
#include "common/log.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/random.hpp"
#include "common/tasklet.hpp"
#include "common/time_ticker.hpp"
#include "common/timer.hpp"
#include "common/uptime.hpp"
#include "diags/factory_diags.hpp"
#include "instance/extension.hpp"
#include "mac/link_raw.hpp"
#include "radio/radio.hpp"
#include "utils/otns.hpp"
#include "utils/power_calibration.hpp"

#if OPENTHREAD_FTD || OPENTHREAD_MTD
#include "backbone_router/backbone_tmf.hpp"
#include "backbone_router/bbr_leader.hpp"
#include "backbone_router/bbr_local.hpp"
#include "backbone_router/bbr_manager.hpp"
#include "border_router/routing_manager.hpp"
#include "coap/coap_secure.hpp"
#include "common/code_utils.hpp"
#include "common/notifier.hpp"
#include "common/settings.hpp"
#include "crypto/mbedtls.hpp"
#include "mac/mac.hpp"
#include "meshcop/border_agent.hpp"
#include "meshcop/commissioner.hpp"
#include "meshcop/dataset_manager.hpp"
#include "meshcop/dataset_updater.hpp"
#include "meshcop/extended_panid.hpp"
#include "meshcop/joiner.hpp"
#include "meshcop/joiner_router.hpp"
#include "meshcop/meshcop_leader.hpp"
#include "meshcop/network_name.hpp"
#include "net/dhcp6.hpp"
#include "net/dhcp6_client.hpp"
#include "net/dhcp6_server.hpp"
#include "net/dns_client.hpp"
#include "net/dns_dso.hpp"
#include "net/dnssd_server.hpp"
#include "net/ip6.hpp"
#include "net/ip6_filter.hpp"
#include "net/nat64_translator.hpp"
#include "net/nd_agent.hpp"
#include "net/netif.hpp"
#include "net/sntp_client.hpp"
#include "net/srp_client.hpp"
#include "net/srp_server.hpp"
#include "radio/ble_secure.hpp"
#include "thread/address_resolver.hpp"
#include "thread/announce_begin_server.hpp"
#include "thread/announce_sender.hpp"
#include "thread/anycast_locator.hpp"
#include "thread/child_supervision.hpp"
#include "thread/discover_scanner.hpp"
#include "thread/dua_manager.hpp"
#include "thread/energy_scan_server.hpp"
#include "thread/key_manager.hpp"
#include "thread/link_metrics.hpp"
#include "thread/link_quality.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle.hpp"
#include "thread/mle_router.hpp"
#include "thread/mlr_manager.hpp"
#include "thread/network_data_local.hpp"
#include "thread/network_data_notifier.hpp"
#include "thread/network_data_publisher.hpp"
#include "thread/network_data_service.hpp"
#include "thread/network_diagnostic.hpp"
#include "thread/panid_query_server.hpp"
#include "thread/radio_selector.hpp"
#include "thread/thread_netif.hpp"
#include "thread/time_sync_service.hpp"
#include "thread/tmf.hpp"
#include "utils/channel_manager.hpp"
#include "utils/channel_monitor.hpp"
#include "utils/heap.hpp"
#include "utils/history_tracker.hpp"
#include "utils/jam_detector.hpp"
#include "utils/link_metrics_manager.hpp"
#include "utils/mesh_diag.hpp"
#include "utils/ping_sender.hpp"
#include "utils/slaac_address.hpp"
#include "utils/srp_client_buffers.hpp"
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

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
 * Represents an opaque (and empty) type corresponding to an OpenThread instance object.
 *
 */
typedef struct otInstance
{
} otInstance;

namespace ot {

/**
 * Represents an OpenThread instance.
 *
 * Contains all the components used by OpenThread.
 *
 */
class Instance : public otInstance, private NonCopyable
{
public:
    /**
     * Represents the message buffer information (number of messages/buffers in all OT stack message queues).
     *
     */
    class BufferInfo : public otBufferInfo, public Clearable<BufferInfo>
    {
    };

#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    /**
      * Initializes the OpenThread instance.
      *
      * Must be called before any other calls on OpenThread instance.
      *
      * @param[in]     aBuffer      The buffer for OpenThread to use for allocating the Instance.
      * @param[in,out] aBufferSize  On input, the size of `aBuffer`. On output, if not enough space for `Instance`, the
                                    number of bytes required for `Instance`.
      *
      * @returns  A pointer to the new OpenThread instance.
      *
      */
    static Instance *Init(void *aBuffer, size_t *aBufferSize);

#if OPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE
    /**
     * This static method initializes the OpenThread instance.
     *
     * This method utilizes static buffer to initialize the OpenThread instance.
     * This function must be called before any other calls on OpenThread instance.
     *
     * @param[in] aIdx The index of the OpenThread instance to initialize.
     *
     * @returns  A pointer to the new OpenThread instance.
     *
     */
    static Instance *InitMultiple(uint8_t aIdx);
#endif

#else // OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE

    /**
     * Initializes the single OpenThread instance.
     *
     * Initializes OpenThread and prepares it for subsequent OpenThread API calls. This function must be
     * called before any other calls to OpenThread.
     *
     * @returns A reference to the single OpenThread instance.
     *
     */
    static Instance &InitSingle(void);

    /**
     * Returns a reference to the single OpenThread instance.
     *
     * @returns A reference to the single OpenThread instance.
     *
     */
    static Instance &Get(void);
#endif

    /**
     * Gets the instance identifier.
     *
     * The instance identifier is set to a random value when the instance is constructed, and then its value will not
     * change after initialization.
     *
     * @returns The instance identifier.
     *
     */
    uint32_t GetId(void) const { return mId; }

    /**
     * Indicates whether or not the instance is valid/initialized and not yet finalized.
     *
     * @returns TRUE if the instance is valid/initialized, FALSE otherwise.
     *
     */
    bool IsInitialized(void) const { return mIsInitialized; }

    /**
     * Triggers a platform reset.
     *
     * The reset process ensures that all the OpenThread state/info (stored in volatile memory) is erased. Note that
     * this method does not erase any persistent state/info saved in non-volatile memory.
     *
     */
    void Reset(void);

#if OPENTHREAD_CONFIG_PLATFORM_BOOTLOADER_MODE_ENABLE
    /**
     * Triggers a platform reset to bootloader mode, if supported.
     *
     * @retval kErrorNone        Reset to bootloader successfully.
     * @retval kErrorBusy        Failed due to another operation is ongoing.
     * @retval kErrorNotCapable  Not capable of resetting to bootloader.
     *
     */
    Error ResetToBootloader(void);
#endif

#if OPENTHREAD_RADIO
    /**
     * Resets the internal states of the radio.
     *
     */
    void ResetRadioStack(void);
#endif

    /**
     * Returns the active log level.
     *
     * @returns The log level.
     *
     */
    static LogLevel GetLogLevel(void)
#if OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
    {
        return sLogLevel;
    }
#else
    {
        return static_cast<LogLevel>(OPENTHREAD_CONFIG_LOG_LEVEL);
    }
#endif

#if OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
    /**
     * Sets the log level.
     *
     * @param[in] aLogLevel  A log level.
     *
     */
    static void SetLogLevel(LogLevel aLogLevel);
#endif

    /**
     * Finalizes the OpenThread instance.
     *
     * Should be called when OpenThread instance is no longer in use.
     *
     */
    void Finalize(void);

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    /**
     * Deletes all the settings stored in non-volatile memory, and then triggers a platform reset.
     *
     */
    void FactoryReset(void);

    /**
     * Erases all the OpenThread persistent info (network settings) stored in non-volatile memory.
     *
     * Erase is successful/allowed only if the device is in `disabled` state/role.
     *
     * @retval kErrorNone          All persistent info/state was erased successfully.
     * @retval kErrorInvalidState  Device is not in `disabled` state/role.
     *
     */
    Error ErasePersistentInfo(void);

#if !OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
    /**
     * Returns a reference to the Heap object.
     *
     * @returns A reference to the Heap object.
     *
     */
    static Utils::Heap &GetHeap(void);
#endif

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
    /**
     * Returns a reference to application COAP object.
     *
     * @returns A reference to the application COAP object.
     *
     */
    Coap::Coap &GetApplicationCoap(void) { return mApplicationCoap; }
#endif

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
    /**
     * Returns a reference to application COAP Secure object.
     *
     * @returns A reference to the application COAP Secure object.
     *
     */
    Coap::CoapSecure &GetApplicationCoapSecure(void) { return mApplicationCoapSecure; }
#endif

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    /**
     * Enables/disables the "DNS name compressions" mode.
     *
     * By default DNS name compression is enabled. When disabled, DNS names are appended as full and never compressed.
     * This is applicable to OpenThread's DNS and SRP client/server modules.
     *
     * This is intended for testing only and available under a `REFERENCE_DEVICE` config.
     *
     * @param[in] aEnabled   TRUE to enable the "DNS name compression" mode, FALSE to disable.
     *
     */
    static void SetDnsNameCompressionEnabled(bool aEnabled) { sDnsNameCompressionEnabled = aEnabled; }

    /**
     * Indicates whether the "DNS name compression" mode is enabled or not.
     *
     * @returns TRUE if the "DNS name compressions" mode is enabled, FALSE otherwise.
     *
     */
    static bool IsDnsNameCompressionEnabled(void) { return sDnsNameCompressionEnabled; }
#endif

    /**
     * Retrieves the the Message Buffer information.
     *
     * @param[out]  aInfo  A `BufferInfo` where information is written.
     *
     */
    void GetBufferInfo(BufferInfo &aInfo);

    /**
     * Resets the Message Buffer information counter tracking maximum number buffers in use at the same
     * time.
     *
     * Resets `mMaxUsedBuffers` in `BufferInfo`.
     *
     */
    void ResetBufferInfo(void);

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

    /**
     * Returns a reference to a given `Type` object belonging to the OpenThread instance.
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

    // Order of variables (their initialization in `Instance`)
    // is important.
    //
    // Tasklet and Timer Schedulers are first to ensure other
    // objects/classes can use them from their constructors.

    Tasklet::Scheduler    mTaskletScheduler;
    TimerMilli::Scheduler mTimerMilliScheduler;
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    TimerMicro::Scheduler mTimerMicroScheduler;
#endif

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    // Random::Manager is initialized before other objects. Note that it
    // requires MbedTls which itself may use Heap.
#if !OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
    static Utils::Heap *sHeap;
#endif
    Crypto::MbedTls mMbedTls;
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

    Random::Manager mRandomManager;

    // Radio is initialized before other member variables
    // (particularly, SubMac and Mac) to allow them to use its methods
    // from their constructor.
    Radio mRadio;

#if OPENTHREAD_CONFIG_UPTIME_ENABLE
    Uptime mUptime;
#endif

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    // Notifier, TimeTicker, Settings, and MessagePool are initialized
    // before other member variables since other classes/objects from
    // their constructor may use them.
    Notifier       mNotifier;
    TimeTicker     mTimeTicker;
    Settings       mSettings;
    SettingsDriver mSettingsDriver;
    MessagePool    mMessagePool;

    Ip6::Ip6    mIp6;
    ThreadNetif mThreadNetif;
    Tmf::Agent  mTmfAgent;

#if OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE
    Dhcp6::Client mDhcp6Client;
#endif

#if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE
    Dhcp6::Server mDhcp6Server;
#endif

#if OPENTHREAD_CONFIG_NEIGHBOR_DISCOVERY_AGENT_ENABLE
    NeighborDiscovery::Agent mNeighborDiscoveryAgent;
#endif

#if OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
    Utils::Slaac mSlaac;
#endif

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    Dns::Client mDnsClient;
#endif

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
    Srp::Client mSrpClient;
#endif

#if OPENTHREAD_CONFIG_SRP_CLIENT_BUFFERS_ENABLE
    Utils::SrpClientBuffers mSrpClientBuffers;
#endif

#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE
    Dns::ServiceDiscovery::Server mDnssdServer;
#endif

#if OPENTHREAD_CONFIG_DNS_DSO_ENABLE
    Dns::Dso mDnsDso;
#endif

#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    Sntp::Client mSntpClient;
#endif

    MeshCoP::ActiveDatasetManager  mActiveDataset;
    MeshCoP::PendingDatasetManager mPendingDataset;
    MeshCoP::ExtendedPanIdManager  mExtendedPanIdManager;
    MeshCoP::NetworkNameManager    mNetworkNameManager;
    Ip6::Filter                    mIp6Filter;
    KeyManager                     mKeyManager;
    Lowpan::Lowpan                 mLowpan;
    Mac::Mac                       mMac;
    MeshForwarder                  mMeshForwarder;
    Mle::MleRouter                 mMleRouter;
    Mle::DiscoverScanner           mDiscoverScanner;
    AddressResolver                mAddressResolver;

#if OPENTHREAD_CONFIG_MULTI_RADIO
    RadioSelector mRadioSelector;
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    NetworkData::Local mNetworkDataLocal;
#endif

    NetworkData::Leader mNetworkDataLeader;

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    NetworkData::Notifier mNetworkDataNotifier;
#endif

#if OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE
    NetworkData::Publisher mNetworkDataPublisher;
#endif

    NetworkData::Service::Manager mNetworkDataServiceManager;

    NetworkDiagnostic::Server mNetworkDiagnosticServer;
#if OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE
    NetworkDiagnostic::Client mNetworkDiagnosticClient;
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
    MeshCoP::BorderAgent mBorderAgent;
#endif

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
    MeshCoP::Commissioner mCommissioner;
#endif

#if OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
    Tmf::SecureAgent mTmfSecureAgent;
#endif

#if OPENTHREAD_CONFIG_JOINER_ENABLE
    MeshCoP::Joiner mJoiner;
#endif

#if OPENTHREAD_CONFIG_JAM_DETECTION_ENABLE
    Utils::JamDetector mJamDetector;
#endif

#if OPENTHREAD_FTD
    MeshCoP::JoinerRouter mJoinerRouter;
    MeshCoP::Leader       mLeader;
#endif

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    BackboneRouter::Leader mBackboneRouterLeader;
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    BackboneRouter::Local   mBackboneRouterLocal;
    BackboneRouter::Manager mBackboneRouterManager;
#endif

#if OPENTHREAD_CONFIG_MLR_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE)
    MlrManager mMlrManager;
#endif

#if OPENTHREAD_CONFIG_DUA_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE)
    DuaManager mDuaManager;
#endif

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    Srp::Server mSrpServer;
#endif

#if OPENTHREAD_FTD
    ChildSupervisor mChildSupervisor;
#endif
    SupervisionListener mSupervisionListener;

    AnnounceBeginServer mAnnounceBegin;
    PanIdQueryServer    mPanIdQuery;
    EnergyScanServer    mEnergyScan;

#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE
    AnycastLocator mAnycastLocator;
#endif

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    TimeSync mTimeSync;
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    LinkMetrics::Initiator mInitiator;
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    LinkMetrics::Subject mSubject;
#endif

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
    Coap::Coap mApplicationCoap;
#endif

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
    Coap::CoapSecure mApplicationCoapSecure;
#endif

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
    Ble::BleSecure mApplicationBleSecure;
#endif

#if OPENTHREAD_CONFIG_PING_SENDER_ENABLE
    Utils::PingSender mPingSender;
#endif

#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
    Utils::ChannelMonitor mChannelMonitor;
#endif

#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE && OPENTHREAD_FTD
    Utils::ChannelManager mChannelManager;
#endif

#if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD
    Utils::MeshDiag mMeshDiag;
#endif

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    Utils::HistoryTracker mHistoryTracker;
#endif

#if OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE
    Utils::LinkMetricsManager mLinkMetricsManager;
#endif

#if (OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE || OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE) && OPENTHREAD_FTD
    MeshCoP::DatasetUpdater mDatasetUpdater;
#endif

#if OPENTHREAD_CONFIG_ANNOUNCE_SENDER_ENABLE
    AnnounceSender mAnnounceSender;
#endif

#if OPENTHREAD_CONFIG_OTNS_ENABLE
    Utils::Otns mOtns;
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    BorderRouter::RoutingManager mRoutingManager;
#endif

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    Nat64::Translator mNat64Translator;
#endif

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

#if OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    Mac::LinkRaw mLinkRaw;
#endif

#if OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
    static LogLevel sLogLevel;
#endif

#if OPENTHREAD_ENABLE_VENDOR_EXTENSION
    Extension::ExtensionBase &mExtension;
#endif

#if OPENTHREAD_CONFIG_DIAG_ENABLE
    FactoryDiags::Diags mDiags;
#endif
#if OPENTHREAD_CONFIG_POWER_CALIBRATION_ENABLE && OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE
    Utils::PowerCalibration mPowerCalibration;
#endif

    bool mIsInitialized;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)
    static bool sDnsNameCompressionEnabled;
#endif

    uint32_t mId;
};

DefineCoreType(otInstance, Instance);
DefineCoreType(otBufferInfo, Instance::BufferInfo);

// Specializations of the `Get<Type>()` method.

template <> inline Instance &Instance::Get(void) { return *this; }

template <> inline Radio &Instance::Get(void) { return mRadio; }

template <> inline Radio::Callbacks &Instance::Get(void) { return mRadio.mCallbacks; }

#if OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)
template <> inline RadioStatistics &Instance::Get(void) { return mRadio.mRadioStatistics; }
#endif

#if OPENTHREAD_CONFIG_UPTIME_ENABLE
template <> inline Uptime &Instance::Get(void) { return mUptime; }
#endif

#if OPENTHREAD_MTD || OPENTHREAD_FTD
template <> inline Notifier &Instance::Get(void) { return mNotifier; }

template <> inline TimeTicker &Instance::Get(void) { return mTimeTicker; }

template <> inline Settings &Instance::Get(void) { return mSettings; }

template <> inline SettingsDriver &Instance::Get(void) { return mSettingsDriver; }

template <> inline MeshForwarder &Instance::Get(void) { return mMeshForwarder; }

#if OPENTHREAD_CONFIG_MULTI_RADIO
template <> inline RadioSelector &Instance::Get(void) { return mRadioSelector; }
#endif

template <> inline Mle::Mle &Instance::Get(void) { return mMleRouter; }

#if OPENTHREAD_FTD
template <> inline Mle::MleRouter &Instance::Get(void) { return mMleRouter; }
#endif

template <> inline Mle::DiscoverScanner &Instance::Get(void) { return mDiscoverScanner; }

template <> inline NeighborTable &Instance::Get(void) { return mMleRouter.mNeighborTable; }

#if OPENTHREAD_FTD
template <> inline ChildTable &Instance::Get(void) { return mMleRouter.mChildTable; }

template <> inline RouterTable &Instance::Get(void) { return mMleRouter.mRouterTable; }
#endif

template <> inline Ip6::Netif &Instance::Get(void) { return mThreadNetif; }

template <> inline ThreadNetif &Instance::Get(void) { return mThreadNetif; }

template <> inline Ip6::Ip6 &Instance::Get(void) { return mIp6; }

template <> inline Mac::Mac &Instance::Get(void) { return mMac; }

template <> inline Mac::SubMac &Instance::Get(void) { return mMac.mLinks.mSubMac; }

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
template <> inline Trel::Link &Instance::Get(void) { return mMac.mLinks.mTrel; }

template <> inline Trel::Interface &Instance::Get(void) { return mMac.mLinks.mTrel.mInterface; }
#endif

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
template <> inline Mac::Filter &Instance::Get(void) { return mMac.mFilter; }
#endif

template <> inline Lowpan::Lowpan &Instance::Get(void) { return mLowpan; }

template <> inline KeyManager &Instance::Get(void) { return mKeyManager; }

template <> inline Ip6::Filter &Instance::Get(void) { return mIp6Filter; }

template <> inline AddressResolver &Instance::Get(void) { return mAddressResolver; }

#if OPENTHREAD_FTD

template <> inline IndirectSender &Instance::Get(void) { return mMeshForwarder.mIndirectSender; }

template <> inline SourceMatchController &Instance::Get(void)
{
    return mMeshForwarder.mIndirectSender.mSourceMatchController;
}

template <> inline DataPollHandler &Instance::Get(void) { return mMeshForwarder.mIndirectSender.mDataPollHandler; }

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
template <> inline CslTxScheduler &Instance::Get(void) { return mMeshForwarder.mIndirectSender.mCslTxScheduler; }
#endif

template <> inline MeshCoP::Leader &Instance::Get(void) { return mLeader; }

template <> inline MeshCoP::JoinerRouter &Instance::Get(void) { return mJoinerRouter; }
#endif // OPENTHREAD_FTD

template <> inline AnnounceBeginServer &Instance::Get(void) { return mAnnounceBegin; }

template <> inline DataPollSender &Instance::Get(void) { return mMeshForwarder.mDataPollSender; }

template <> inline EnergyScanServer &Instance::Get(void) { return mEnergyScan; }

template <> inline PanIdQueryServer &Instance::Get(void) { return mPanIdQuery; }

#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE
template <> inline AnycastLocator &Instance::Get(void) { return mAnycastLocator; }
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
template <> inline NetworkData::Local &Instance::Get(void) { return mNetworkDataLocal; }
#endif

template <> inline NetworkData::Leader &Instance::Get(void) { return mNetworkDataLeader; }

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
template <> inline NetworkData::Notifier &Instance::Get(void) { return mNetworkDataNotifier; }
#endif

#if OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE
template <> inline NetworkData::Publisher &Instance::Get(void) { return mNetworkDataPublisher; }
#endif

template <> inline NetworkData::Service::Manager &Instance::Get(void) { return mNetworkDataServiceManager; }

#if OPENTHREAD_CONFIG_TCP_ENABLE
template <> inline Ip6::Tcp &Instance::Get(void) { return mIp6.mTcp; }
#endif

template <> inline Ip6::Udp &Instance::Get(void) { return mIp6.mUdp; }

template <> inline Ip6::Icmp &Instance::Get(void) { return mIp6.mIcmp; }

template <> inline Ip6::Mpl &Instance::Get(void) { return mIp6.mMpl; }

template <> inline Tmf::Agent &Instance::Get(void) { return mTmfAgent; }

#if OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
template <> inline Tmf::SecureAgent &Instance::Get(void) { return mTmfSecureAgent; }
#endif

template <> inline MeshCoP::ExtendedPanIdManager &Instance::Get(void) { return mExtendedPanIdManager; }

template <> inline MeshCoP::NetworkNameManager &Instance::Get(void) { return mNetworkNameManager; }

template <> inline MeshCoP::ActiveDatasetManager &Instance::Get(void) { return mActiveDataset; }

template <> inline MeshCoP::PendingDatasetManager &Instance::Get(void) { return mPendingDataset; }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
template <> inline TimeSync &Instance::Get(void) { return mTimeSync; }
#endif

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
template <> inline MeshCoP::Commissioner &Instance::Get(void) { return mCommissioner; }

template <> inline AnnounceBeginClient &Instance::Get(void) { return mCommissioner.GetAnnounceBeginClient(); }

template <> inline EnergyScanClient &Instance::Get(void) { return mCommissioner.GetEnergyScanClient(); }

template <> inline PanIdQueryClient &Instance::Get(void) { return mCommissioner.GetPanIdQueryClient(); }
#endif

#if OPENTHREAD_CONFIG_JOINER_ENABLE
template <> inline MeshCoP::Joiner &Instance::Get(void) { return mJoiner; }
#endif

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
template <> inline Dns::Client &Instance::Get(void) { return mDnsClient; }
#endif

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
template <> inline Srp::Client &Instance::Get(void) { return mSrpClient; }
#endif

#if OPENTHREAD_CONFIG_SRP_CLIENT_BUFFERS_ENABLE
template <> inline Utils::SrpClientBuffers &Instance::Get(void) { return mSrpClientBuffers; }
#endif

#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE
template <> inline Dns::ServiceDiscovery::Server &Instance::Get(void) { return mDnssdServer; }
#endif

#if OPENTHREAD_CONFIG_DNS_DSO_ENABLE
template <> inline Dns::Dso &Instance::Get(void) { return mDnsDso; }
#endif

template <> inline NetworkDiagnostic::Server &Instance::Get(void) { return mNetworkDiagnosticServer; }

#if OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE
template <> inline NetworkDiagnostic::Client &Instance::Get(void) { return mNetworkDiagnosticClient; }
#endif

#if OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE
template <> inline Dhcp6::Client &Instance::Get(void) { return mDhcp6Client; }
#endif

#if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE
template <> inline Dhcp6::Server &Instance::Get(void) { return mDhcp6Server; }
#endif

#if OPENTHREAD_CONFIG_NEIGHBOR_DISCOVERY_AGENT_ENABLE
template <> inline NeighborDiscovery::Agent &Instance::Get(void) { return mNeighborDiscoveryAgent; }
#endif

#if OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
template <> inline Utils::Slaac &Instance::Get(void) { return mSlaac; }
#endif

#if OPENTHREAD_CONFIG_JAM_DETECTION_ENABLE
template <> inline Utils::JamDetector &Instance::Get(void) { return mJamDetector; }
#endif

#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
template <> inline Sntp::Client &Instance::Get(void) { return mSntpClient; }
#endif

#if OPENTHREAD_FTD
template <> inline ChildSupervisor &Instance::Get(void) { return mChildSupervisor; }
#endif
template <> inline SupervisionListener &Instance::Get(void) { return mSupervisionListener; }

#if OPENTHREAD_CONFIG_PING_SENDER_ENABLE
template <> inline Utils::PingSender &Instance::Get(void) { return mPingSender; }
#endif

#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
template <> inline Utils::ChannelMonitor &Instance::Get(void) { return mChannelMonitor; }
#endif

#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE && OPENTHREAD_FTD
template <> inline Utils::ChannelManager &Instance::Get(void) { return mChannelManager; }
#endif

#if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD
template <> inline Utils::MeshDiag &Instance::Get(void) { return mMeshDiag; }
#endif

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
template <> inline Utils::HistoryTracker &Instance::Get(void) { return mHistoryTracker; }
#endif

#if OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE
template <> inline Utils::LinkMetricsManager &Instance::Get(void) { return mLinkMetricsManager; }
#endif

#if (OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE || OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE) && OPENTHREAD_FTD
template <> inline MeshCoP::DatasetUpdater &Instance::Get(void) { return mDatasetUpdater; }
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
template <> inline MeshCoP::BorderAgent &Instance::Get(void) { return mBorderAgent; }
#endif

#if OPENTHREAD_CONFIG_ANNOUNCE_SENDER_ENABLE
template <> inline AnnounceSender &Instance::Get(void) { return mAnnounceSender; }
#endif

template <> inline MessagePool &Instance::Get(void) { return mMessagePool; }

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

template <> inline BackboneRouter::Leader &Instance::Get(void) { return mBackboneRouterLeader; }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
template <> inline BackboneRouter::Local   &Instance::Get(void) { return mBackboneRouterLocal; }
template <> inline BackboneRouter::Manager &Instance::Get(void) { return mBackboneRouterManager; }

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
template <> inline BackboneRouter::MulticastListenersTable &Instance::Get(void)
{
    return mBackboneRouterManager.GetMulticastListenersTable();
}
#endif

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
template <> inline BackboneRouter::NdProxyTable &Instance::Get(void)
{
    return mBackboneRouterManager.GetNdProxyTable();
}
#endif

template <> inline BackboneRouter::BackboneTmfAgent &Instance::Get(void)
{
    return mBackboneRouterManager.GetBackboneTmfAgent();
}
#endif

#if OPENTHREAD_CONFIG_MLR_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE)
template <> inline MlrManager &Instance::Get(void) { return mMlrManager; }
#endif

#if OPENTHREAD_CONFIG_DUA_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE)
template <> inline DuaManager &Instance::Get(void) { return mDuaManager; }
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
template <> inline LinkMetrics::Initiator &Instance::Get(void) { return mInitiator; }
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
template <> inline LinkMetrics::Subject &Instance::Get(void) { return mSubject; }
#endif

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

#if OPENTHREAD_CONFIG_OTNS_ENABLE
template <> inline Utils::Otns &Instance::Get(void) { return mOtns; }
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
template <> inline BorderRouter::RoutingManager &Instance::Get(void) { return mRoutingManager; }

template <> inline BorderRouter::InfraIf &Instance::Get(void) { return mRoutingManager.mInfraIf; }
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
template <> inline Nat64::Translator &Instance::Get(void) { return mNat64Translator; }
#endif

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
template <> inline Srp::Server &Instance::Get(void) { return mSrpServer; }
#endif

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
template <> inline Ble::BleSecure &Instance::Get(void) { return mApplicationBleSecure; }
#endif

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

#if OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE
template <> inline Mac::LinkRaw &Instance::Get(void) { return mLinkRaw; }

#if OPENTHREAD_RADIO
template <> inline Mac::SubMac &Instance::Get(void) { return mLinkRaw.mSubMac; }
#endif

#endif // OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE

template <> inline Tasklet::Scheduler &Instance::Get(void) { return mTaskletScheduler; }

template <> inline TimerMilli::Scheduler &Instance::Get(void) { return mTimerMilliScheduler; }

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
template <> inline TimerMicro::Scheduler &Instance::Get(void) { return mTimerMicroScheduler; }
#endif

#if OPENTHREAD_ENABLE_VENDOR_EXTENSION
template <> inline Extension::ExtensionBase &Instance::Get(void) { return mExtension; }
#endif

#if OPENTHREAD_CONFIG_DIAG_ENABLE
template <> inline FactoryDiags::Diags &Instance::Get(void) { return mDiags; }
#endif

#if OPENTHREAD_CONFIG_POWER_CALIBRATION_ENABLE && OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE
template <> inline Utils::PowerCalibration &Instance::Get(void) { return mPowerCalibration; }
#endif

/**
 * @}
 *
 */

} // namespace ot

#endif // INSTANCE_HPP_
