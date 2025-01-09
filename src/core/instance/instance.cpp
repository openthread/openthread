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

#include "instance.hpp"

#include <openthread/platform/misc.h>

#include "common/new.hpp"
#include "utils/heap.hpp"

namespace ot {

#if !OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE

// Define the raw storage used for OpenThread instance (in single-instance case).
OT_DEFINE_ALIGNED_VAR(gInstanceRaw, sizeof(Instance), uint64_t);

#endif

#if OPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE

#define INSTANCE_SIZE_ALIGNED OT_ALIGNED_VAR_SIZE(sizeof(ot::Instance), uint64_t)
#define MULTI_INSTANCE_SIZE (OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM * INSTANCE_SIZE_ALIGNED)

// Define the raw storage used for OpenThread instance (in multi-instance case).
static uint64_t gMultiInstanceRaw[MULTI_INSTANCE_SIZE];

#endif

#if OPENTHREAD_MTD || OPENTHREAD_FTD
#if !OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
OT_DEFINE_ALIGNED_VAR(sHeapRaw, sizeof(Utils::Heap), uint64_t);
Utils::Heap *Instance::sHeap{nullptr};
#endif
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
bool Instance::sDnsNameCompressionEnabled = true;
#endif
#endif

#if OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
LogLevel Instance::sLogLevel = static_cast<LogLevel>(OPENTHREAD_CONFIG_LOG_LEVEL_INIT);
#endif

Instance::Instance(void)
    : mTimerMilliScheduler(*this)
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    , mTimerMicroScheduler(*this)
#endif
    , mRadio(*this)
#if OPENTHREAD_CONFIG_UPTIME_ENABLE
    , mUptime(*this)
#endif
#if OPENTHREAD_MTD || OPENTHREAD_FTD
    , mNotifier(*this)
    , mTimeTicker(*this)
    , mSettings(*this)
    , mSettingsDriver(*this)
    , mMessagePool(*this)
#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE || OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    // DNS-SD (mDNS) platform is initialized early to
    // allow other modules to use it.
    , mDnssd(*this)
#endif
    , mIp6(*this)
    , mThreadNetif(*this)
    , mTmfAgent(*this)
#if OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE
    , mDhcp6Client(*this)
#endif
#if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE
    , mDhcp6Server(*this)
#endif
#if OPENTHREAD_CONFIG_NEIGHBOR_DISCOVERY_AGENT_ENABLE
    , mNeighborDiscoveryAgent(*this)
#endif
#if OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
    , mSlaac(*this)
#endif
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    , mDnsClient(*this)
#endif
#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
    , mSrpClient(*this)
#endif
#if OPENTHREAD_CONFIG_SRP_CLIENT_BUFFERS_ENABLE
    , mSrpClientBuffers(*this)
#endif
#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE
    , mDnssdServer(*this)
#endif
#if OPENTHREAD_CONFIG_DNS_DSO_ENABLE
    , mDnsDso(*this)
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    , mMdnsCore(*this)
#endif
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    , mSntpClient(*this)
#endif
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    , mBackboneRouterLocal(*this)
#endif
    , mActiveDataset(*this)
    , mPendingDataset(*this)
    , mExtendedPanIdManager(*this)
    , mNetworkNameManager(*this)
    , mIp6Filter(*this)
    , mKeyManager(*this)
    , mLowpan(*this)
    , mMac(*this)
    , mMeshForwarder(*this)
    , mMleRouter(*this)
    , mDiscoverScanner(*this)
    , mAddressResolver(*this)
#if OPENTHREAD_CONFIG_MULTI_RADIO
    , mRadioSelector(*this)
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    , mNetworkDataLocal(*this)
#endif
    , mNetworkDataLeader(*this)
#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    , mNetworkDataNotifier(*this)
#endif
#if OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE
    , mNetworkDataPublisher(*this)
#endif
    , mNetworkDataServiceManager(*this)
    , mNetworkDiagnosticServer(*this)
#if OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE
    , mNetworkDiagnosticClient(*this)
#endif
#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
    , mBorderAgent(*this)
#endif
#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
    , mCommissioner(*this)
#endif
#if OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
    , mTmfSecureAgent(*this)
#endif
#if OPENTHREAD_CONFIG_JOINER_ENABLE
    , mJoiner(*this)
#endif
#if OPENTHREAD_CONFIG_JAM_DETECTION_ENABLE
    , mJamDetector(*this)
#endif
#if OPENTHREAD_FTD
    , mJoinerRouter(*this)
    , mLeader(*this)
#endif
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    , mBackboneRouterLeader(*this)
#endif
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    , mBackboneRouterManager(*this)
#endif
#if OPENTHREAD_CONFIG_MLR_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE)
    , mMlrManager(*this)
#endif

#if OPENTHREAD_CONFIG_DUA_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE)
    , mDuaManager(*this)
#endif
#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    , mSrpServer(*this)
#if OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE
    , mSrpAdvertisingProxy(*this)
#endif
#endif
#if OPENTHREAD_FTD
    , mChildSupervisor(*this)
#endif
    , mSupervisionListener(*this)
    , mAnnounceBegin(*this)
    , mPanIdQuery(*this)
    , mEnergyScan(*this)
#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE
    , mAnycastLocator(*this)
#endif
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    , mTimeSync(*this)
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    , mInitiator(*this)
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    , mSubject(*this)
#endif
#if OPENTHREAD_CONFIG_COAP_API_ENABLE
    , mApplicationCoap(*this)
#endif
#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
    , mApplicationCoapSecure(*this, kWithLinkSecurity)
#endif
#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
    , mApplicationBleSecure(*this)
#endif
#if OPENTHREAD_CONFIG_PING_SENDER_ENABLE
    , mPingSender(*this)
#endif
#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
    , mChannelMonitor(*this)
#endif
#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE && \
    (OPENTHREAD_FTD || OPENTHREAD_CONFIG_CHANNEL_MANAGER_CSL_CHANNEL_SELECT_ENABLE)
    , mChannelManager(*this)
#endif
#if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD
    , mMeshDiag(*this)
#endif
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    , mHistoryTracker(*this)
#endif
#if OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE
    , mLinkMetricsManager(*this)
#endif
#if (OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE || OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE) && OPENTHREAD_FTD
    , mDatasetUpdater(*this)
#endif
#if OPENTHREAD_CONFIG_ANNOUNCE_SENDER_ENABLE
    , mAnnounceSender(*this)
#endif
#if OPENTHREAD_CONFIG_OTNS_ENABLE
    , mOtns(*this)
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    , mRoutingManager(*this)
#endif
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    , mNat64Translator(*this)
#endif
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
#if OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    , mLinkRaw(*this)
#endif
#if OPENTHREAD_ENABLE_VENDOR_EXTENSION
    , mExtension(Extension::ExtensionBase::Init(*this))
#endif
#if OPENTHREAD_CONFIG_DIAG_ENABLE
    , mDiags(*this)
#endif
#if OPENTHREAD_CONFIG_POWER_CALIBRATION_ENABLE && OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE
    , mPowerCalibration(*this)
#endif
    , mIsInitialized(false)
    , mId(Random::NonCrypto::GetUint32())
{
}

#if (OPENTHREAD_MTD || OPENTHREAD_FTD) && !OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
Utils::Heap &Instance::GetHeap(void)
{
    if (nullptr == sHeap)
    {
        sHeap = new (&sHeapRaw) Utils::Heap();
    }

    return *sHeap;
}
#endif

#if !OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE

Instance &Instance::InitSingle(void)
{
    Instance *instance = &Get();

    VerifyOrExit(!instance->mIsInitialized);

    instance = new (&gInstanceRaw) Instance();

    instance->AfterInit();

exit:
    return *instance;
}

Instance &Instance::Get(void)
{
    void *instance = &gInstanceRaw;

    return *static_cast<Instance *>(instance);
}

#else // #if !OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
#if OPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE

Instance *Instance::InitMultiple(uint8_t aIdx)
{
    size_t    bufferSize;
    uint64_t *instanceBuffer = gMultiInstanceRaw + aIdx * INSTANCE_SIZE_ALIGNED;
    Instance *instance       = reinterpret_cast<Instance *>(instanceBuffer);

    VerifyOrExit(aIdx < OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM);
    VerifyOrExit(!instance->mIsInitialized);

    bufferSize = (&gMultiInstanceRaw[MULTI_INSTANCE_SIZE] - instanceBuffer) * sizeof(uint64_t);
    instance   = Instance::Init(instanceBuffer, &bufferSize);

exit:
    return instance;
}

Instance &Instance::Get(uint8_t aIdx)
{
    void *instance = gMultiInstanceRaw + aIdx * INSTANCE_SIZE_ALIGNED;
    return *static_cast<Instance *>(instance);
}

uint8_t Instance::GetIdx(Instance *aInstance)
{
    return static_cast<uint8_t>((reinterpret_cast<uint64_t *>(aInstance) - gMultiInstanceRaw) / INSTANCE_SIZE_ALIGNED);
}

#endif // #if OPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE

Instance *Instance::Init(void *aBuffer, size_t *aBufferSize)
{
    Instance *instance = nullptr;

    VerifyOrExit(aBufferSize != nullptr);

    // Make sure the input buffer is big enough
    VerifyOrExit(sizeof(Instance) <= *aBufferSize, *aBufferSize = sizeof(Instance));

    VerifyOrExit(aBuffer != nullptr);

    instance = new (aBuffer) Instance();

    instance->AfterInit();

exit:
    return instance;
}

#endif // OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE

void Instance::Reset(void) { otPlatReset(this); }

#if OPENTHREAD_CONFIG_PLATFORM_BOOTLOADER_MODE_ENABLE
Error Instance::ResetToBootloader(void) { return otPlatResetToBootloader(this); }
#endif

#if OPENTHREAD_RADIO
void Instance::ResetRadioStack(void)
{
    mRadio.Init();
    mLinkRaw.Init();
}
#endif

void Instance::AfterInit(void)
{
    mIsInitialized = true;
#if OPENTHREAD_MTD || OPENTHREAD_FTD

    // Restore datasets and network information

    Get<Settings>().Init();
    Get<Mle::MleRouter>().Restore();

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    Get<Trel::Link>().AfterInit();
#endif

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_VENDOR_EXTENSION
    Get<Extension::ExtensionBase>().SignalInstanceInit();
#endif
}

void Instance::Finalize(void)
{
    VerifyOrExit(mIsInitialized);

    mIsInitialized = false;

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    IgnoreError(otThreadSetEnabled(this, false));
    IgnoreError(otIp6SetEnabled(this, false));
    IgnoreError(otLinkSetEnabled(this, false));

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    Get<KeyManager>().DestroyTemporaryKeys();
#endif

    Get<Settings>().Deinit();
#endif

    IgnoreError(Get<Mac::SubMac>().Disable());

#if !OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE

    /**
     * Object was created on buffer, so instead of deleting
     * the object we call destructor explicitly.
     */
    this->~Instance();

#endif // !OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE

exit:
    return;
}

#if OPENTHREAD_MTD || OPENTHREAD_FTD

void Instance::FactoryReset(void)
{
    Get<Settings>().Wipe();
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    Get<KeyManager>().DestroyTemporaryKeys();
    Get<KeyManager>().DestroyPersistentKeys();
#endif
    otPlatReset(this);
}

Error Instance::ErasePersistentInfo(void)
{
    Error error = kErrorNone;

    VerifyOrExit(Get<Mle::MleRouter>().IsDisabled(), error = kErrorInvalidState);
    Get<Settings>().Wipe();
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    Get<KeyManager>().DestroyTemporaryKeys();
    Get<KeyManager>().DestroyPersistentKeys();
#endif

exit:
    return error;
}

void Instance::GetBufferInfo(BufferInfo &aInfo)
{
    aInfo.Clear();

    aInfo.mTotalBuffers   = Get<MessagePool>().GetTotalBufferCount();
    aInfo.mFreeBuffers    = Get<MessagePool>().GetFreeBufferCount();
    aInfo.mMaxUsedBuffers = Get<MessagePool>().GetMaxUsedBufferCount();

    Get<MeshForwarder>().GetSendQueue().GetInfo(aInfo.m6loSendQueue);
    Get<MeshForwarder>().GetReassemblyQueue().GetInfo(aInfo.m6loReassemblyQueue);
    Get<Ip6::Ip6>().GetSendQueue().GetInfo(aInfo.mIp6Queue);

#if OPENTHREAD_FTD
    Get<Ip6::Mpl>().GetBufferedMessageSet().GetInfo(aInfo.mMplQueue);
#endif

    Get<Mle::MleRouter>().GetMessageQueue().GetInfo(aInfo.mMleQueue);

    Get<Tmf::Agent>().GetRequestMessages().GetInfo(aInfo.mCoapQueue);
    Get<Tmf::Agent>().GetCachedResponses().GetInfo(aInfo.mCoapQueue);

#if OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
    Get<Tmf::SecureAgent>().GetRequestMessages().GetInfo(aInfo.mCoapSecureQueue);
    Get<Tmf::SecureAgent>().GetCachedResponses().GetInfo(aInfo.mCoapSecureQueue);
#endif

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
    GetApplicationCoap().GetRequestMessages().GetInfo(aInfo.mApplicationCoapQueue);
    GetApplicationCoap().GetCachedResponses().GetInfo(aInfo.mApplicationCoapQueue);
#endif
}

void Instance::ResetBufferInfo(void) { Get<MessagePool>().ResetMaxUsedBufferCount(); }

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE

void Instance::SetLogLevel(LogLevel aLogLevel)
{
    if (aLogLevel != sLogLevel)
    {
        sLogLevel = aLogLevel;
        otPlatLogHandleLevelChanged(sLogLevel);
    }
}

extern "C" OT_TOOL_WEAK void otPlatLogHandleLevelChanged(otLogLevel aLogLevel) { OT_UNUSED_VARIABLE(aLogLevel); }

#endif

} // namespace ot
