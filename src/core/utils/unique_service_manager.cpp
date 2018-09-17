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
 *   This file implements unique service manager.
 */

#include "utils/unique_service_manager.hpp"
#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/owner-locator.hpp"
#include "common/random.hpp"
#include "net/netif.hpp"

#if OPENTHREAD_ENABLE_SERVICE

namespace ot {
namespace Utils {

UniqueServiceManager::UniqueServiceManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance, &UniqueServiceManager::HandleTimer, this)
    , mNotifierCallback(&UniqueServiceManager::HandleStateChanged, this)
{
    memset(mEntries, 0, sizeof(mEntries));
    aInstance.GetNotifier().RegisterCallback(mNotifierCallback);
}

otError UniqueServiceManager::RegisterService(const otServiceConfig * aConfig,
                                              otServiceUpdateCallback aServiceUpdateCallback,
                                              otServerCompareCallback aServerCompareCallback,
                                              void *                  aContext)
{
    otError         error = OT_ERROR_NONE;
    otServiceConfig config;
    bool            rlocIn;
    uint8_t         i;

    for (i = 0; i < OT_ARRAY_LENGTH(mEntries); i++)
    {
        if (mEntries[i].mServiceConfig == NULL)
        {
            break;
        }
    }

    VerifyOrExit(i < OT_ARRAY_LENGTH(mEntries), error = OT_ERROR_NO_BUFS);

    mEntries[i].mServiceConfig         = aConfig;
    mEntries[i].mServiceUpdateCallback = aServiceUpdateCallback;
    mEntries[i].mServerCompareCallback = aServerCompareCallback != NULL ? aServerCompareCallback : DefaultServerCompare;
    mEntries[i].mContext               = aContext;

    if (NetworkDataLeaderServiceLookup(i, config, rlocIn) == OT_ERROR_NONE)
    {
        mEntries[i].mServiceUpdateCallback(&config, mEntries[i].mContext);
    }
    else
    {
        AddNetworkDataLocalService(*mEntries[i].mServiceConfig);
        GetNetif().GetNetworkDataLocal().SendServerDataNotification();
    }

exit:
    return error;
}

otError UniqueServiceManager::UnregisterService(uint32_t       aEnterpriseNumber,
                                                const uint8_t *aServiceData,
                                                uint8_t        aServiceDataLength)
{
    otError         error = OT_ERROR_NONE;
    otServiceConfig config;
    bool            rlocIn;
    uint8_t         i;

    for (i = 0; i < OT_ARRAY_LENGTH(mEntries); i++)
    {
        if (mEntries[i].mServiceConfig != NULL && mEntries[i].mServiceConfig->mEnterpriseNumber == aEnterpriseNumber &&
            mEntries[i].mServiceConfig->mServiceDataLength == aServiceDataLength &&
            memcmp(mEntries[i].mServiceConfig->mServiceData, aServiceData, aServiceDataLength) == 0)
        {
            break;
        }
    }

    VerifyOrExit(i < OT_ARRAY_LENGTH(mEntries), error = OT_ERROR_NOT_FOUND);

    if (NetworkDataLeaderServiceLookup(i, config, rlocIn) == OT_ERROR_NONE && rlocIn)
    {
        RemoveNetworkDataLocalService(*mEntries[i].mServiceConfig);
        GetNetif().GetNetworkDataLocal().SendServerDataNotification();
    }

    memset(&mEntries[i], 0, sizeof(ServiceEntry));

exit:
    return error;
}

void UniqueServiceManager::HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags)
{
    aCallback.GetOwner<UniqueServiceManager>().HandleStateChanged(aFlags);
}

void UniqueServiceManager::HandleStateChanged(otChangedFlags aFlags)
{
    otServiceConfig config;
    bool            rlocIn;
    bool            startTimer = false;

    VerifyOrExit((aFlags & OT_CHANGED_THREAD_NETDATA) != 0);

    for (uint8_t i = 0; i < OT_ARRAY_LENGTH(mEntries); i++)
    {
        if (mEntries[i].mServiceConfig == NULL)
        {
            continue;
        }

        if (NetworkDataLeaderServiceLookup(i, config, rlocIn) == OT_ERROR_NONE)
        {
            // If the server is not selected as the primary server, it should unregister the service.
            if (rlocIn && config.mServerConfig.mRloc16 != GetNetif().GetMle().GetRloc16())
            {
                // MLE layer will send SVR_DATA.ntf when it detects the mismatch between NetworkDataLeader and
                // NetworkDataLocal in the function HandleStateChanged().
                RemoveNetworkDataLocalService(*mEntries[i].mServiceConfig);
            }

            mEntries[i].mServiceUpdateCallback(&config, mEntries[i].mContext);
        }
        else
        {
            // If there are no servers provide the service, server delays a random period, and then registers the
            // service.
            startTimer = true;
            mEntries[i].mServiceUpdateCallback(NULL, mEntries[i].mContext);
        }
    }

    if (startTimer && !mTimer.IsRunning())
    {
        mTimer.Start(Random::GetUint32InRange(1, TimerMilli::SecToMsec(kMaxRegisterDelay)));
    }

exit:
    return;
}

void UniqueServiceManager::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<UniqueServiceManager>().HandleTimer();
}

void UniqueServiceManager::HandleTimer(void)
{
    bool sendNotification = false;

    for (uint8_t i = 0; i < OT_ARRAY_LENGTH(mEntries); i++)
    {
        if (mEntries[i].mServiceConfig == NULL)
        {
            continue;
        }

        if (!GetNetif().GetNetworkDataLeader().ContainsService(mEntries[i].mServiceConfig->mEnterpriseNumber,
                                                               mEntries[i].mServiceConfig->mServiceData,
                                                               mEntries[i].mServiceConfig->mServiceDataLength))
        {
            AddNetworkDataLocalService(*mEntries[i].mServiceConfig);
            sendNotification = true;
        }
    }

    if (sendNotification)
    {
        GetNetif().GetNetworkDataLocal().SendServerDataNotification();
    }
}

otError UniqueServiceManager::NetworkDataLeaderServiceLookup(uint8_t          aEntryIndex,
                                                             otServiceConfig &aConfig,
                                                             bool &           aRlocIn)
{
    otError               error    = OT_ERROR_NONE;
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    ThreadNetif &         netif    = GetNetif();
    ServiceEntry *        entry    = &mEntries[aEntryIndex];
    otServiceConfig       config;
    bool                  serviceExistsInNetworkDataLeader = false;

    aRlocIn = false;

    while (netif.GetNetworkDataLeader().GetNextService(&iterator, &config) == OT_ERROR_NONE)
    {
        if (entry->mServiceConfig->mEnterpriseNumber != config.mEnterpriseNumber ||
            entry->mServiceConfig->mServiceDataLength != config.mServiceDataLength ||
            memcmp(entry->mServiceConfig->mServiceData, config.mServiceData, config.mServiceDataLength) != 0)
        {
            continue;
        }

        if (!serviceExistsInNetworkDataLeader)
        {
            serviceExistsInNetworkDataLeader = true;
            aConfig                          = config;
        }
        else if (!entry->mServerCompareCallback(&aConfig.mServerConfig, &config.mServerConfig, entry->mContext))
        {
            aConfig = config;
        }

        if (config.mServerConfig.mRloc16 == GetNetif().GetMle().GetRloc16())
        {
            aRlocIn = true;
        }
    }

    VerifyOrExit(serviceExistsInNetworkDataLeader, error = OT_ERROR_NOT_FOUND);

exit:
    return error;
}

otError UniqueServiceManager::AddNetworkDataLocalService(const otServiceConfig &aConfig)
{
    return GetNetif().GetNetworkDataLocal().AddService(
        aConfig.mEnterpriseNumber, aConfig.mServiceData, aConfig.mServiceDataLength, aConfig.mServerConfig.mStable,
        aConfig.mServerConfig.mServerData, aConfig.mServerConfig.mServerDataLength);
}

otError UniqueServiceManager::RemoveNetworkDataLocalService(const otServiceConfig &aConfig)
{
    return GetNetif().GetNetworkDataLocal().RemoveService(aConfig.mEnterpriseNumber, aConfig.mServiceData,
                                                          aConfig.mServiceDataLength);
}

bool UniqueServiceManager::DefaultServerCompare(const otServerConfig *aServerA,
                                                const otServerConfig *aServerB,
                                                void *                aContext)
{
    bool rval = false;

    if ((Mle::Mle::IsActiveRouter(aServerA->mRloc16) && Mle::Mle::IsActiveRouter(aServerB->mRloc16)) ||
        (!Mle::Mle::IsActiveRouter(aServerA->mRloc16) && !Mle::Mle::IsActiveRouter(aServerB->mRloc16)))
    {
        if (aServerA->mRloc16 <= aServerB->mRloc16)
        {
            rval = true;
        }
        else
        {
            rval = false;
        }
    }
    else if (Mle::Mle::IsActiveRouter(aServerA->mRloc16))
    {
        rval = true;
    }
    else if (Mle::Mle::IsActiveRouter(aServerA->mRloc16))
    {
        rval = false;
    }

    OT_UNUSED_VARIABLE(aContext);

    return rval;
}
} // namespace Utils
} // namespace ot

#endif // OPENTHREAD_ENABLE_SERVICE
