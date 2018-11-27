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

#if OPENTHREAD_ENABLE_SERVICE && OPENTHREAD_ENABLE_UNIQUE_SERVICE

namespace ot {
namespace Utils {

otError ServiceEntry::Init(const otServiceConfig & aConfig,
                           otServiceUpdateCallback aServiceUpdateCallback,
                           otServerCompareCallback aServerCompareCallback,
                           void *                  aContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aConfig.mServiceDataLength + aConfig.mServerConfig.mServerDataLength <= sizeof(mData),
                 error = OT_ERROR_NO_BUFS);

    mEnterpriseNumber  = aConfig.mEnterpriseNumber;
    mServiceDataLength = aConfig.mServiceDataLength;
    mServerStable      = aConfig.mServerConfig.mStable;
    mServerDataLength  = aConfig.mServerConfig.mServerDataLength;
    mUpdateCallback    = aServiceUpdateCallback;
    mCompareCallback   = aServerCompareCallback;
    mContext           = aContext;

    memcpy(mData, aConfig.mServiceData, mServiceDataLength);
    memcpy(mData + mServiceDataLength, aConfig.mServerConfig.mServerData, mServerDataLength);

exit:
    return error;
}

otServiceConfig ServiceEntry::GetServiceConfig(void)
{
    otServiceConfig config;

    memset(&config, 0, sizeof(otServiceConfig));

    config.mEnterpriseNumber               = mEnterpriseNumber;
    config.mServiceDataLength              = mServiceDataLength;
    config.mServerConfig.mStable           = mServerStable;
    config.mServerConfig.mServerDataLength = mServerDataLength;

    memcpy(config.mServiceData, GetServiceData(), GetServiceDataLength());
    memcpy(config.mServerConfig.mServerData, GetServerData(), GetServerDataLength());

    return config;
}

otError ServiceTable::AddService(const ServiceEntry &aEntry)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aEntry.GetSize() <= sizeof(mData) - mLength, error = OT_ERROR_NO_BUFS);
    memcpy(mData + mLength, &aEntry, aEntry.GetSize());
    mLength += aEntry.GetSize();

exit:
    return error;
}

otError ServiceTable::RemoveService(uint32_t aEnterpriseNumber, const uint8_t *aServiceData, uint8_t aServiceDataLength)
{
    otError       error = OT_ERROR_NONE;
    ServiceEntry *entry;
    uint8_t *     entryEnd;

    VerifyOrExit((entry = FindService(aEnterpriseNumber, aServiceData, aServiceDataLength)) != NULL,
                 error = OT_ERROR_NOT_FOUND);
    VerifyOrExit(entry->GetSize() <= mLength, error = OT_ERROR_PARSE);

    entryEnd = reinterpret_cast<uint8_t *>(entry) + entry->GetSize();
    memmove(entry, entryEnd, static_cast<size_t>(mData + mLength - entryEnd));
    mLength -= static_cast<uint8_t>(entryEnd - reinterpret_cast<uint8_t *>(entry));

exit:
    return error;
}

otError ServiceTable::GetNextService(Iterator &aIterator, ServiceEntry &aEntry)
{
    otError       error  = OT_ERROR_NONE;
    uint8_t       offset = *reinterpret_cast<uint8_t *>(&aIterator);
    ServiceEntry *entry;

    VerifyOrExit(offset < mLength, error = OT_ERROR_NOT_FOUND);

    entry = reinterpret_cast<ServiceEntry *>(mData + offset);
    offset += entry->GetSize();
    aEntry = *entry;

exit:
    aIterator = *reinterpret_cast<Iterator *>(&offset);

    return error;
}

ServiceEntry *ServiceTable::FindService(uint32_t       aEnterpriseNumber,
                                        const uint8_t *aServiceData,
                                        uint8_t        aServiceDataLength)
{
    uint8_t *     start = mData;
    ServiceEntry *entry;

    while (start < mData + mLength)
    {
        entry = reinterpret_cast<ServiceEntry *>(start);
        if (entry->MatchService(aEnterpriseNumber, aServiceData, aServiceDataLength))
        {
            ExitNow();
        }

        start += entry->GetSize();
    }

    entry = NULL;

exit:
    return entry;
}

otError ServiceTable::FindService(uint32_t       aEnterpriseNumber,
                                  const uint8_t *aServiceData,
                                  uint8_t        aServiceDataLength,
                                  ServiceEntry & aEntry)
{
    otError       error = OT_ERROR_NONE;
    ServiceEntry *entry;

    VerifyOrExit((entry = FindService(aEnterpriseNumber, aServiceData, aServiceDataLength)) != NULL,
                 error = OT_ERROR_NOT_FOUND);
    aEntry = *entry;

exit:
    return error;
}

UniqueServiceManager::UniqueServiceManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mServiceTable()
    , mTimer(aInstance, &UniqueServiceManager::HandleTimer, this)
    , mNotifierCallback(&UniqueServiceManager::HandleStateChanged, this)
{
    aInstance.GetNotifier().RegisterCallback(mNotifierCallback);
}

otError UniqueServiceManager::RegisterService(const otServiceConfig * aConfig,
                                              otServiceUpdateCallback aServiceUpdateCallback,
                                              otServerCompareCallback aServerCompareCallback,
                                              void *                  aContext)
{
    otError                 error = OT_ERROR_NONE;
    otServiceConfig         config;
    ServiceEntry            entry;
    bool                    rlocIn;
    otServerCompareCallback compareCallback;

    VerifyOrExit(
        !mServiceTable.ContainsService(aConfig->mEnterpriseNumber, aConfig->mServiceData, aConfig->mServiceDataLength),
        error = OT_ERROR_ALREADY);

    compareCallback = (aServerCompareCallback != NULL) ? aServerCompareCallback : DefaultServerCompare;
    SuccessOrExit(error = entry.Init(*aConfig, aServiceUpdateCallback, compareCallback, aContext));
    SuccessOrExit(error = mServiceTable.AddService(entry));

    if (NetworkDataLeaderServiceLookup(entry, config, rlocIn) == OT_ERROR_NONE)
    {
        if (entry.GetUpdateCallback())
        {
            entry.GetUpdateCallback()(&config, entry.GetContext());
        }
    }
    else
    {
        AddNetworkDataLocalService(entry.GetServiceConfig());
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
    bool            rlocIn;
    otServiceConfig config;
    ServiceEntry    entry;

    SuccessOrExit(error = mServiceTable.FindService(aEnterpriseNumber, aServiceData, aServiceDataLength, entry));

    if (NetworkDataLeaderServiceLookup(entry, config, rlocIn) == OT_ERROR_NONE && rlocIn)
    {
        RemoveNetworkDataLocalService(entry.GetServiceConfig());
        GetNetif().GetNetworkDataLocal().SendServerDataNotification();
    }

    mServiceTable.RemoveService(aEnterpriseNumber, aServiceData, aServiceDataLength);

exit:
    return error;
}

otError UniqueServiceManager::GetServerConfig(uint32_t        aEnterpriseNumber,
                                              const uint8_t * aServiceData,
                                              uint8_t         aServiceDataLength,
                                              otServerConfig *aServerConfig)
{
    otError         error = OT_ERROR_NONE;
    ServiceEntry    entry;
    otServiceConfig config;
    bool            rlocIn;

    SuccessOrExit(error = mServiceTable.FindService(aEnterpriseNumber, aServiceData, aServiceDataLength, entry));
    SuccessOrExit(error = NetworkDataLeaderServiceLookup(entry, config, rlocIn));
    *aServerConfig = config.mServerConfig;

exit:
    return error;
}

void UniqueServiceManager::HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags)
{
    aCallback.GetOwner<UniqueServiceManager>().HandleStateChanged(aFlags);
}

void UniqueServiceManager::HandleStateChanged(otChangedFlags aFlags)
{
    ServiceTable::Iterator iterator   = ServiceTable::kIteratorInit;
    bool                   startTimer = false;
    otServiceConfig        config;
    ServiceEntry           entry;
    bool                   rlocIn;

    VerifyOrExit((aFlags & OT_CHANGED_THREAD_NETDATA) != 0);

    while (mServiceTable.GetNextService(iterator, entry) == OT_ERROR_NONE)
    {
        if (NetworkDataLeaderServiceLookup(entry, config, rlocIn) == OT_ERROR_NONE)
        {
            // If the server is not elected as the primary server, it should unregister the service.
            if (rlocIn && config.mServerConfig.mRloc16 != GetNetif().GetMle().GetRloc16())
            {
                // MLE layer will send SVR_DATA.ntf when it detects the mismatch between NetworkDataLeader and
                // NetworkDataLocal in the function HandleStateChanged().
                RemoveNetworkDataLocalService(entry.GetServiceConfig());
            }

            if (entry.GetUpdateCallback())
            {
                entry.GetUpdateCallback()(&config, entry.GetContext());
            }
        }
        else
        {
            // If there are no servers provide the service, server delays a random period, and then registers the
            // service.
            startTimer = true;

            if (entry.GetUpdateCallback())
            {
                entry.GetUpdateCallback()(NULL, entry.GetContext());
            }
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
    ServiceTable::Iterator iterator         = ServiceTable::kIteratorInit;
    bool                   sendNotification = false;
    ServiceEntry           entry;

    while (mServiceTable.GetNextService(iterator, entry) == OT_ERROR_NONE)
    {
        if (!GetNetif().GetNetworkDataLeader().ContainsService(entry.GetEnterpriseNumber(), entry.GetServiceData(),
                                                               entry.GetServiceDataLength()))
        {
            AddNetworkDataLocalService(entry.GetServiceConfig());
            sendNotification = true;
        }
    }

    if (sendNotification)
    {
        GetNetif().GetNetworkDataLocal().SendServerDataNotification();
    }
}

otError UniqueServiceManager::NetworkDataLeaderServiceLookup(ServiceEntry &   aEntry,
                                                             otServiceConfig &aConfig,
                                                             bool &           aRlocIn)
{
    otError               error    = OT_ERROR_NONE;
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    ThreadNetif &         netif    = GetNetif();
    otServiceConfig       config;
    bool                  serviceExistsInNetworkDataLeader = false;

    aRlocIn = false;

    while (netif.GetNetworkDataLeader().GetNextService(&iterator, &config) == OT_ERROR_NONE)
    {
        if (!aEntry.MatchService(config.mEnterpriseNumber, config.mServiceData, config.mServiceDataLength))
        {
            continue;
        }

        if (!serviceExistsInNetworkDataLeader)
        {
            serviceExistsInNetworkDataLeader = true;
            aConfig                          = config;
        }
        else if (!aEntry.GetCompareCallback()(&aConfig.mServerConfig, &config.mServerConfig, aEntry.GetContext()))
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

    if (Mle::Mle::IsActiveRouter(aServerA->mRloc16) == Mle::Mle::IsActiveRouter(aServerB->mRloc16))
    {
        // Prefer the server with smaller rloc16
        if (aServerA->mRloc16 <= aServerB->mRloc16)
        {
            rval = true;
        }
    }
    else if (Mle::Mle::IsActiveRouter(aServerA->mRloc16))
    {
        rval = true;
    }

    OT_UNUSED_VARIABLE(aContext);

    return rval;
}

} // namespace Utils
} // namespace ot

#endif // #if OPENTHREAD_ENABLE_SERVICE && OPENTHREAD_ENABLE_UNIQUE_SERVICE
