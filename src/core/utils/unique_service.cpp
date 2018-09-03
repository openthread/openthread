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
 *   This file implements unique service.
 */

#include "utils/unique_service.hpp"
#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/owner-locator.hpp"
#include "common/random.hpp"
#include "net/netif.hpp"

#if OPENTHREAD_ENABLE_SERVICE

namespace ot {
namespace Utils {

UniqueService::UniqueService(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance, &UniqueService::HandleTimer, this)
    , mNetworkData(aInstance)
    , mNotifierCallback(&UniqueService::HandleStateChanged, this)
{
    mNetworkData.Clear();
    aInstance.GetNotifier().RegisterCallback(mNotifierCallback);
}

otError UniqueService::AddService(uint32_t                aEnterpriseNumber,
                                  const uint8_t *         aServiceData,
                                  uint8_t                 aServiceDataLength,
                                  bool                    aServerStable,
                                  const uint8_t *         aServerData,
                                  uint8_t                 aServerDataLength,
                                  otServerCompareCallback aCallback)
{
    otError         error;
    otServiceConfig config;

    VerifyOrExit(aServiceData != NULL && aServerData != NULL, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aServiceDataLength <= OT_SERVICE_DATA_MAX_SIZE, error = OT_ERROR_NO_BUFS);
    VerifyOrExit(aServerDataLength + sizeof(ServiceMetadata) <= OT_SERVER_DATA_MAX_SIZE, error = OT_ERROR_NO_BUFS);

    memset(&config, 0, sizeof(config));
    config.mEnterpriseNumber  = aEnterpriseNumber;
    config.mServiceDataLength = aServiceDataLength;
    memcpy(config.mServiceData, aServiceData, aServiceDataLength);

    config.mServerConfig.mStable           = aServerStable;
    config.mServerConfig.mServerDataLength = aServerDataLength + sizeof(ServiceMetadata);
    memcpy(config.mServerConfig.mServerData, aServerData, aServerDataLength);

    if (aCallback != NULL)
    {
        GetServiceMetadata(config)->SetServerCompareCallback(aCallback);
    }
    else
    {
        GetServiceMetadata(config)->SetServerCompareCallback(DefaultServerCompare);
    }

    if (!NetworkDataLeaderContainsService(config))
    {
        GetServiceMetadata(config)->SetState(ServiceMetadata::kStateRegisterService);
        GetServiceMetadata(config)->SetTimeout(Random::GetUint8InRange(1, kMaxRegisterServiceDelay));
    }

    SuccessOrExit(error = AddNetworkDataUniqueService(config));

    if (GetServiceMetadata(config)->GetTimeout() != 0 && !mTimer.IsRunning())
    {
        mTimer.Start(kStateUpdatePeriod);
    }

exit:
    return error;
}

otError UniqueService::RemoveService(uint32_t       aEnterpriseNumber,
                                     const uint8_t *aServiceData,
                                     uint8_t        aServiceDataLength)
{
    otError         error = OT_ERROR_NONE;
    otServiceConfig config;

    VerifyOrExit(aServiceData != NULL, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = FindNetworkDataUniqueService(aEnterpriseNumber, aServiceData, aServiceDataLength, config));

    GetServiceMetadata(config)->SetState(ServiceMetadata::kStateDeleteService);
    GetServiceMetadata(config)->SetTimeout(kMaxUnregisterServiceDelay);
    SuccessOrExit(error = UpdateNetworkDataUniqueService(config));

    if (!mTimer.IsRunning())
    {
        mTimer.Start(kStateUpdatePeriod);
    }

exit:
    return error;
}

otError UniqueService::GetNextService(otNetworkDataIterator *aIterator, otServiceConfig *aConfig)
{
    return GetNextService(aIterator, aConfig, false);
}

otError UniqueService::GetNextService(otNetworkDataIterator *aIterator, otServiceConfig *aConfig, bool aMetadata)
{
    otError error;

    while ((error = mNetworkData.GetNextService(aIterator, aConfig)) == OT_ERROR_NONE)
    {
        if (GetServiceMetadata(*aConfig)->GetState() != ServiceMetadata::kStateDeleteService)
        {
            break;
        }
    }

    if (!aMetadata)
    {
        aConfig->mServerConfig.mServerDataLength -= sizeof(ServiceMetadata);
    }

    return error;
}

otError UniqueService::GetNextLeaderService(otNetworkDataIterator *aIterator, otServiceConfig *aConfig)
{
    otError               error;
    ThreadNetif &         netif = GetNetif();
    otServiceConfig       config;
    otServiceConfig       leaderConfig;
    otServiceConfig       preferredConfig;
    otNetworkDataIterator innerIterator;
    bool                  serviceExistsInNetworkDataLeader;

    while ((error = GetNextService(aIterator, &config, true)) == OT_ERROR_NONE)
    {
        serviceExistsInNetworkDataLeader = false;
        innerIterator                    = OT_NETWORK_DATA_ITERATOR_INIT;

        while (netif.GetNetworkDataLeader().GetNextService(&innerIterator, &leaderConfig) == OT_ERROR_NONE)
        {
            if (!ServiceCompare(config, leaderConfig))
            {
                continue;
            }

            if (!serviceExistsInNetworkDataLeader)
            {
                serviceExistsInNetworkDataLeader = true;
                preferredConfig                  = leaderConfig;
            }
            else if (GetServiceMetadata(config)->GetServerCompareCallback()(&preferredConfig.mServerConfig,
                                                                            &leaderConfig.mServerConfig) < 0)
            {
                preferredConfig = leaderConfig;
            }
        }

        if (serviceExistsInNetworkDataLeader)
        {
            *aConfig = preferredConfig;
            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

void UniqueService::HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags)
{
    aCallback.GetOwner<UniqueService>().HandleStateChanged(aFlags);
}

void UniqueService::HandleStateChanged(otChangedFlags aFlags)
{
    ThreadNetif &netif = GetNetif();

    if ((aFlags & OT_CHANGED_THREAD_NETDATA) != 0)
    {
        otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
        otNetworkDataIterator innerIterator;
        NetworkData::Local    oldNetworkData(mNetworkData);
        otServiceConfig       config;
        otServiceConfig       leaderConfig;
        otServiceConfig       preferredConfig;
        bool                  rlocIn;
        bool                  serviceExistsInNetworkDataLeader;
        bool                  startTimer = false;

        while (oldNetworkData.GetNextService(&iterator, &config) == OT_ERROR_NONE)
        {
            if (GetServiceMetadata(config)->GetState() == ServiceMetadata::kStateDeleteService)
            {
                continue;
            }

            serviceExistsInNetworkDataLeader = false;
            rlocIn                           = false;
            innerIterator                    = OT_NETWORK_DATA_ITERATOR_INIT;

            while (netif.GetNetworkDataLeader().GetNextService(&innerIterator, &leaderConfig) == OT_ERROR_NONE)
            {
                if (!ServiceCompare(config, leaderConfig))
                {
                    continue;
                }

                if (!serviceExistsInNetworkDataLeader)
                {
                    serviceExistsInNetworkDataLeader = true;
                    preferredConfig                  = leaderConfig;
                }
                else if (GetServiceMetadata(config)->GetServerCompareCallback()(&preferredConfig.mServerConfig,
                                                                                &leaderConfig.mServerConfig) < 0)
                {
                    preferredConfig = leaderConfig;
                }

                if (leaderConfig.mServerConfig.mRloc16 == GetNetif().GetMle().GetRloc16())
                {
                    rlocIn = true;
                }
            }

            if (serviceExistsInNetworkDataLeader)
            {
                if (rlocIn && preferredConfig.mServerConfig.mRloc16 != GetNetif().GetMle().GetRloc16() &&
                    GetServiceMetadata(config)->GetState() != ServiceMetadata::kStateUnregisterService)
                {
                    // The node has already registered the service to the Leader, but it has not been selected as the
                    // primary server, the node will remove itself from the server list of the service.

                    startTimer = true;

                    GetServiceMetadata(config)->SetState(ServiceMetadata::kStateUnregisterService);
                    GetServiceMetadata(config)->SetTimeout(Random::GetUint8InRange(1, kMaxUnregisterServiceDelay));
                    UpdateNetworkDataUniqueService(config);
                }
            }
            else if (GetServiceMetadata(config)->GetState() != ServiceMetadata::kStateRegisterService)
            {
                // No one provides the service, the node will register the sevice to the Leader.

                startTimer = true;

                GetServiceMetadata(config)->SetState(ServiceMetadata::kStateRegisterService);
                GetServiceMetadata(config)->SetTimeout(Random::GetUint8InRange(1, kMaxRegisterServiceDelay));
                UpdateNetworkDataUniqueService(config);
            }
        }

        if (startTimer && !mTimer.IsRunning())
        {
            mTimer.Start(kStateUpdatePeriod);
        }
    }
}

void UniqueService::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<UniqueService>().HandleTimer();
}

void UniqueService::HandleTimer(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    NetworkData::Local    oldNetworkData(mNetworkData);
    otServiceConfig       config;
    uint8_t               timeout;
    bool                  sendNotification = false;
    bool                  continueTimer    = false;

    while (oldNetworkData.GetNextService(&iterator, &config) == OT_ERROR_NONE)
    {
        if ((timeout = GetServiceMetadata(config)->GetTimeout()) == 0)
        {
            continue;
        }

        timeout--;

        GetServiceMetadata(config)->SetTimeout(timeout);
        UpdateNetworkDataUniqueService(config);

        if (timeout == 0)
        {
            switch (GetServiceMetadata(config)->GetState())
            {
            case ServiceMetadata::kStateRegisterService:
                if (!NetworkDataLeaderContainsService(config) && AddNetworkDataLocalService(config) == OT_ERROR_NONE)
                {
                    sendNotification = true;
                }
                break;

            case ServiceMetadata::kStateDeleteService:
                RemoveNetworkDataUniqueService(config);

                // fall through

            case ServiceMetadata::kStateUnregisterService:
                if (NetworkDataLeaderContainsService(config) && RemoveNetworkDataLocalService(config) == OT_ERROR_NONE)
                {
                    sendNotification = true;
                }
                break;

            default:
                break;
            }

            if (GetServiceMetadata(config)->GetState() != ServiceMetadata::kStateDeleteService)
            {
                GetServiceMetadata(config)->SetState(ServiceMetadata::kStateIdle);
                UpdateNetworkDataUniqueService(config);
            }
        }
        else
        {
            continueTimer = true;
        }
    }

    if (sendNotification)
    {
        GetNetif().GetNetworkDataLocal().SendServerDataNotification();
    }

    if (continueTimer)
    {
        mTimer.Start(kStateUpdatePeriod);
    }
}

otError UniqueService::AddNetworkDataLocalService(otServiceConfig &aConfig)
{
    return GetNetif().GetNetworkDataLocal().AddService(
        aConfig.mEnterpriseNumber, aConfig.mServiceData, aConfig.mServiceDataLength, aConfig.mServerConfig.mStable,
        aConfig.mServerConfig.mServerData, aConfig.mServerConfig.mServerDataLength - sizeof(ServiceMetadata));
}

otError UniqueService::RemoveNetworkDataLocalService(otServiceConfig &aConfig)
{
    return GetNetif().GetNetworkDataLocal().RemoveService(aConfig.mEnterpriseNumber, aConfig.mServiceData,
                                                          aConfig.mServiceDataLength);
}

otError UniqueService::AddNetworkDataUniqueService(otServiceConfig &aConfig)
{
    return mNetworkData.AddService(aConfig.mEnterpriseNumber, aConfig.mServiceData, aConfig.mServiceDataLength,
                                   aConfig.mServerConfig.mStable, aConfig.mServerConfig.mServerData,
                                   aConfig.mServerConfig.mServerDataLength);
}

otError UniqueService::RemoveNetworkDataUniqueService(otServiceConfig &aConfig)
{
    return mNetworkData.RemoveService(aConfig.mEnterpriseNumber, aConfig.mServiceData, aConfig.mServiceDataLength);
}

otError UniqueService::UpdateNetworkDataUniqueService(otServiceConfig &aConfig)
{
    return AddNetworkDataUniqueService(aConfig);
}

otError UniqueService::FindNetworkDataUniqueService(uint32_t         aEnterpriseNumber,
                                                    const uint8_t *  aServiceData,
                                                    uint8_t          aServiceDataLength,
                                                    otServiceConfig &aConfig)
{
    otError               error    = OT_ERROR_NOT_FOUND;
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otServiceConfig       config;

    while (GetNextService(&iterator, &config, true) == OT_ERROR_NONE)
    {
        if (config.mEnterpriseNumber == aEnterpriseNumber && config.mServiceDataLength == aServiceDataLength &&
            memcmp(config.mServiceData, aServiceData, aServiceDataLength) == 0)
        {
            aConfig = config;
            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

bool UniqueService::NetworkDataLeaderContainsService(const otServiceConfig &aConfig)
{
    bool                  ret      = false;
    ThreadNetif &         netif    = GetNetif();
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otServiceConfig       leaderConfig;

    while (netif.GetNetworkDataLeader().GetNextService(&iterator, &leaderConfig) == OT_ERROR_NONE)
    {
        VerifyOrExit(!ServiceCompare(aConfig, leaderConfig), ret = true);
    }

exit:
    return ret;
}

bool UniqueService::ServiceCompare(const otServiceConfig &aServiceA, const otServiceConfig &aServiceB) const
{
    return (aServiceA.mEnterpriseNumber == aServiceB.mEnterpriseNumber &&
            aServiceA.mServiceDataLength == aServiceB.mServiceDataLength &&
            memcmp(aServiceA.mServiceData, aServiceB.mServiceData, aServiceB.mServiceDataLength) == 0);
}

int UniqueService::DefaultServerCompare(const otServerConfig *aServerA, const otServerConfig *aServerB)
{
    int rval = 0;

    if (aServerA->mStable && aServerB->mStable)
    {
        rval = 0;
    }
    else if (aServerA->mStable)
    {
        rval = 1;
    }
    else if (aServerB->mStable)
    {
        rval = -1;
    }

    return rval;
}

ServiceMetadata *UniqueService::GetServiceMetadata(otServiceConfig &aConfig)
{
    return reinterpret_cast<ServiceMetadata *>(aConfig.mServerConfig.mServerData +
                                               aConfig.mServerConfig.mServerDataLength - sizeof(ServiceMetadata));
}

} // namespace Utils
} // namespace ot

#endif // OPENTHREAD_ENABLE_SERVICE
