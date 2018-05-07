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

#include <openthread/types.h>
#include <openthread/platform/logging.h>

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
#include "api/link_raw.hpp"
#include "common/message.hpp"
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
#include "net/ip6.hpp"
#include "thread/link_quality.hpp"
#include "thread/thread_netif.hpp"
#if OPENTHREAD_ENABLE_CHANNEL_MANAGER
#include "utils/channel_manager.hpp"
#endif
#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
#include "utils/channel_monitor.hpp"
#endif
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

#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
    /**
     * This method returns the current dynamic log level.
     *
     * @returns the currently set dynamic log level.
     *
     */
    otLogLevel GetDynamicLogLevel(void) const { return mLogLevel; }

    /**
     * This method sets the dynamic log level.
     *
     * @param[in]  aLogLevel The dynamic log level.
     *
     */
    void SetDynamicLogLevel(otLogLevel aLogLevel) { mLogLevel = aLogLevel; }
#endif

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    /**
     * This method finalizes the OpenThread instance.
     *
     * This method should be called when OpenThread instance is no longer in use.
     *
     */
    void Finalize(void);

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
    Coap::ApplicationCoap &GetApplicationCoap(void) { return mApplicationCoap; }
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

    /**
     * This method returns a reference to message pool object.
     *
     * @returns A reference to the message pool object.
     *
     */
    MessagePool &GetMessagePool(void) { return mMessagePool; }
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

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
    template <typename Type> Type &Get(void);

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    /**
     * This method returns a reference to LinkRaw object.
     *
     * @returns A reference to the LinkRaw object.
     *
     */
    LinkRaw &GetLinkRaw(void) { return mLinkRaw; }
#endif

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
    Coap::ApplicationCoap mApplicationCoap;
#endif

#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
    Utils::ChannelMonitor mChannelMonitor;
#endif

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER
    Utils::ChannelManager mChannelManager;
#endif

    MessagePool mMessagePool;
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    LinkRaw mLinkRaw;
#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
    otLogLevel mLogLevel;
#endif
    bool mIsInitialized;
};

/**
 * @}
 *
 */

} // namespace ot

#endif // INSTANCE_HPP_
