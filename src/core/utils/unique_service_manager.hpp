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
 *   This file includes definitions for unique service manager.
 */

#ifndef UNIQUE_SERVICE_MANAGER_HPP_
#define UNIQUE_SERVICE_MANAGER_HPP_

#include <openthread/error.h>
#include <openthread/server.h>

#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "thread/network_data_local.hpp"

namespace ot {
namespace Utils {

/**
 * This class controls the service registeration process to ensure that only one server provides the service.
 *
 */
class UniqueServiceManager : public InstanceLocator
{
public:
    /**
     * This constructor initializes the Unique Service Manager object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit UniqueServiceManager(Instance &aInstance);

    /**
     * This method registers a unique service to the Leader.
     *
     * If the Leader Network Data doesn't contains this service, the server will send SVR_DATA.ntf to the Leader to
     * register the service. Otherwise, the server uses the existing service.
     *
     * @param[in]  aConfig                A pointer to the service configuration.
     * @param[in]  aServiceUpdateCallback A pointer to a function that is called when the server changes.
     * @param[in]  aServerCompareCallback A pointer to a function for comparing servers.
     * @param[in]  aContext               A pointer to a user context.
     *
     * @retval OT_ERROR_NONE     Successfully added and registered the service configuration.
     * @retval OT_ERROR_NO_BUFS  Insufficient space to add the service configuration.
     *
     */
    otError RegisterService(const otServiceConfig * aConfig,
                            otServiceUpdateCallback aServiceUpdateCallback,
                            otServerCompareCallback aServerCompareCallback,
                            void *                  aContext);

    /**
     * This method unregisters a unique service to the Leader.
     *
     * If the server provides the service in the Leader Network Data, it sends a SVR_DATA.ntf to the Leader to
     * unregister the service.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number of the service to be deleted.
     * @param[in]  aServiceData       A pointer to the service data.
     * @param[in]  aServiceDataLength The length of @p aServiceData in bytes.
     *
     * @retval OT_ERROR_NONE       Successfully removed and unregistered the Service configuration.
     * @retval OT_ERROR_NOT_FOUND  Could not find the Service configuration.
     *
     */
    otError UnregisterService(uint32_t aEnterpriseNumber, const uint8_t *aServiceData, uint8_t aServiceDataLength);

private:
    enum
    {
        kNumEntries       = 2,  ///< The number of the service entries.
        kMaxRegisterDelay = 30, ///< Maximum delay before registering the service (seconds).
    };

    struct ServiceEntry
    {
        const otServiceConfig * mServiceConfig;
        otServerCompareCallback mServerCompareCallback;
        otServiceUpdateCallback mServiceUpdateCallback;
        void *                  mContext;
    };

    static void HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags);
    void        HandleStateChanged(otChangedFlags aFlags);
    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    otError NetworkDataLeaderServiceLookup(uint8_t aEntryIndex, otServiceConfig &aConfig, bool &aRlocIn);
    otError AddNetworkDataLocalService(const otServiceConfig &aConfig);
    otError RemoveNetworkDataLocalService(const otServiceConfig &aConfig);

    static bool DefaultServerCompare(const otServerConfig *aServerA, const otServerConfig *aServerB, void *aContext);

    TimerMilli         mTimer;
    ServiceEntry       mEntries[kNumEntries];
    Notifier::Callback mNotifierCallback;
};

} // namespace Utils
} // namespace ot

#endif // UNIQUE_SERVICE_MANAGER_HPP_
