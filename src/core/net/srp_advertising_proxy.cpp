/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file includes implementation of SRP Advertising Proxy.
 */

#include "srp_advertising_proxy.hpp"

#if OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Srp {

RegisterLogModule("SrpAdvProxy");

//---------------------------------------------------------------------------------------------------------------------
// AdvertisingProxy

AdvertisingProxy::AdvertisingProxy(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStateStopped)
    , mCurrentRequestId(0)
    , mAdvTimeout(kAdvTimeout)
    , mTimer(aInstance)
    , mTasklet(aInstance)
{
    mCounters.Clear();
}

void AdvertisingProxy::Start(void)
{
    VerifyOrExit(mState != kStateRunning);

    mState = kStateRunning;
    mCounters.mStateChanges++;
    LogInfo("Started");

    // Advertise all existing and committed entries on SRP sever.

    for (Host &host : Get<Server>().mHosts)
    {
        LogInfo("Adv existing host '%s'", host.GetFullName());
        Advertise(host);
    }

exit:
    return;
}

void AdvertisingProxy::Stop(void)
{
    VerifyOrExit(mState != kStateStopped);

    mState = kStateStopped;
    mCounters.mStateChanges++;

    while (true)
    {
        OwnedPtr<AdvInfo> advPtr = mAdvInfoList.Pop();

        if (advPtr.IsNull())
        {
            break;
        }

        mCounters.mAdvRejected++;

        UnregisterHostAndItsServicesAndKeys(advPtr->mHost);

        advPtr->mError = kErrorAbort;
        advPtr->mHost.mAdvIdRange.Clear();
        advPtr->mBlockingAdv = nullptr;
        advPtr->SignalServerToCommit();
    }

    for (Host &host : Get<Server>().GetHosts())
    {
        UnregisterHostAndItsServicesAndKeys(host);

        host.mAdvIdRange.Clear();
        host.mAdvId        = kInvalidRequestId;
        host.mIsRegistered = false;

        for (Service &service : host.mServices)
        {
            service.mAdvId        = kInvalidRequestId;
            service.mIsRegistered = false;
        }
    }

    LogInfo("Stopped");

exit:
    return;
}

void AdvertisingProxy::UpdateState(void)
{
    if (!Get<Dnssd>().IsReady() || !Get<BorderRouter::InfraIf>().IsRunning())
    {
        Stop();
        ExitNow();
    }

    switch (Get<Server>().GetState())
    {
    case Server::kStateDisabled:
    case Server::kStateStopped:
        Stop();
        break;

    case Server::kStateRunning:
        Start();
        break;
    }

exit:
    return;
}

AdvertisingProxy::RequestId AdvertisingProxy::AllocateNextRequestId(void)
{
    mCurrentRequestId++;

    if (kInvalidRequestId == mCurrentRequestId)
    {
        mCurrentRequestId++;
    }

    return mCurrentRequestId;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
template <> void AdvertisingProxy::UpdateAdvIdRangeOn(Host &aHost)
{
    // Determine and update `mAdvIdRange` on `aHost` based on
    // `mAdvId` and `mKeyAdvId` of host and its services.

    aHost.mAdvIdRange.Clear();

    for (const Service &service : aHost.mServices)
    {
        if (service.mKeyAdvId != kInvalidRequestId)
        {
            aHost.mAdvIdRange.Add(service.mKeyAdvId);
        }

        if (service.mAdvId != kInvalidRequestId)
        {
            aHost.mAdvIdRange.Add(service.mAdvId);
        }
    }

    if (aHost.mKeyAdvId != kInvalidRequestId)
    {
        aHost.mAdvIdRange.Add(aHost.mKeyAdvId);
    }

    if (aHost.mAdvId != kInvalidRequestId)
    {
        aHost.mAdvIdRange.Add(aHost.mAdvId);
    }

    if (aHost.mAdvIdRange.IsEmpty())
    {
        mTasklet.Post();
    }
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
template <> void AdvertisingProxy::UpdateAdvIdRangeOn(Service &aService)
{
    // Updates `mAdvIdRange` on the `Host` associated with
    // `aService`.

    UpdateAdvIdRangeOn<Host>(*aService.mHost);
}

void AdvertisingProxy::AdvertiseRemovalOf(Host &aHost)
{
    LogInfo("Adv removal of host '%s'", aHost.GetFullName());
    mCounters.mAdvHostRemovals++;

    VerifyOrExit(mState == kStateRunning);
    VerifyOrExit(aHost.IsDeleted());

    aHost.mShouldAdvertise = aHost.mIsRegistered;

    for (Service &service : aHost.mServices)
    {
        if (!service.mIsDeleted)
        {
            service.mIsDeleted = true;
        }

        service.mShouldAdvertise = service.mIsRegistered;
    }

    // Reject any outstanding `AdvInfo` that matches `aHost` that is
    // being removed.

    for (AdvInfo &adv : mAdvInfoList)
    {
        Host &advHost = adv.mHost;

        if (!aHost.Matches(advHost.GetFullName()) || advHost.IsDeleted())
        {
            continue;
        }

        for (Service &advService : advHost.mServices)
        {
            Service *service;

            service = aHost.FindService(advService.GetInstanceName());

            if (service == nullptr)
            {
                // `AdvInfo` contains a service that is not present in
                // `aHost`, we unregister the service and its key.

                if (!advService.IsDeleted())
                {
                    UnregisterService(advService);
                }

                UnregisterKey(advService);
            }
            else
            {
                service->mShouldAdvertise = true;

                if (aHost.mKeyLease == 0)
                {
                    advService.mIsKeyRegistered = false;
                }
            }

            advService.mAdvId      = kInvalidRequestId;
            advService.mKeyAdvId   = kInvalidRequestId;
            advService.mIsReplaced = true;
        }

        if (aHost.mKeyLease == 0)
        {
            advHost.mIsKeyRegistered = false;
        }

        advHost.mAdvId      = kInvalidRequestId;
        advHost.mKeyAdvId   = kInvalidRequestId;
        advHost.mIsReplaced = true;
        advHost.mAdvIdRange.Clear();

        adv.mError = kErrorAbort;
        mTasklet.Post();
    }

    for (Service &service : aHost.mServices)
    {
        if (service.mShouldAdvertise)
        {
            UnregisterService(service);
        }

        if (aHost.mKeyLease == 0)
        {
            UnregisterKey(service);
        }
    }

    if (aHost.mShouldAdvertise)
    {
        UnregisterHost(aHost);
    }

    if (aHost.mKeyLease == 0)
    {
        UnregisterKey(aHost);
    }

exit:
    return;
}

void AdvertisingProxy::AdvertiseRemovalOf(Service &aService)
{
    LogInfo("Adv removal of service '%s' '%s'", aService.GetInstanceLabel(), aService.GetServiceName());
    mCounters.mAdvServiceRemovals++;

    VerifyOrExit(mState == kStateRunning);

    aService.mShouldAdvertise = aService.mIsRegistered;

    // Check if any outstanding `AdvInfo` is re-adding the `aService`
    // (which is being removed), and if so, skip unregistering the
    // service and its key.

    for (const AdvInfo &adv : mAdvInfoList)
    {
        const Host    &advHost = adv.mHost;
        const Service *advService;

        if (!aService.mHost->Matches(advHost.GetFullName()))
        {
            continue;
        }

        if (advHost.IsDeleted())
        {
            break;
        }

        advService = advHost.FindService(aService.GetInstanceName());

        if ((advService != nullptr) && !advService->IsDeleted())
        {
            ExitNow();
        }
    }

    if (aService.mShouldAdvertise)
    {
        UnregisterService(aService);
    }

    if (aService.mKeyLease == 0)
    {
        UnregisterKey(aService);
    }

exit:
    return;
}

void AdvertisingProxy::Advertise(Host &aHost, const Server::MessageMetadata &aMetadata)
{
    AdvInfo *advPtr = nullptr;
    Host    *existingHost;

    LogInfo("Adv update for '%s'", aHost.GetFullName());

    mCounters.mAdvTotal++;

    VerifyOrExit(mState == kStateRunning);

    advPtr = AdvInfo::Allocate(aHost, aMetadata, mAdvTimeout);
    VerifyOrExit(advPtr != nullptr);
    mAdvInfoList.Push(*advPtr);

    // Compare the new `aHost` with outstanding advertisements and
    // already committed entries on server.

    for (AdvInfo &adv : mAdvInfoList)
    {
        if (!aHost.Matches(adv.mHost.GetFullName()))
        {
            continue;
        }

        if (CompareAndUpdateHostAndServices(aHost, adv.mHost))
        {
            // If the new `aHost` replaces an entry in the outstanding
            // `adv`, we mark the new advertisement as blocked so
            // that it is not committed before the earlier one. This
            // ensures that SRP Updates are committed in the order
            // they are advertised, avoiding issues such as re-adding
            // a removed entry due to a delay in registration on
            // infra DNS-SD.

            if (advPtr->mBlockingAdv == nullptr)
            {
                mCounters.mAdvReplaced++;
                advPtr->mBlockingAdv = &adv;
            }
        }
    }

    existingHost = Get<Server>().mHosts.FindMatching(aHost.GetFullName());

    if (existingHost != nullptr)
    {
        CompareAndUpdateHostAndServices(aHost, *existingHost);
    }

    Advertise(aHost);

exit:
    if (advPtr != nullptr)
    {
        if (advPtr->IsCompleted())
        {
            mTasklet.Post();
        }
        else
        {
            mTimer.FireAtIfEarlier(advPtr->mExpireTime);
        }
    }
    else
    {
        LogInfo("Adv skipped '%s'", aHost.GetFullName());
        mCounters.mAdvSkipped++;
        Get<Server>().CommitSrpUpdate(kErrorNone, aHost, aMetadata);
    }
}

template <typename Entry> bool AdvertisingProxy::IsKeyRegisteredOrRegistering(const Entry &aEntry) const
{
    return (aEntry.mIsKeyRegistered || (aEntry.mKeyAdvId != kInvalidRequestId));
}

template <typename Entry> bool AdvertisingProxy::IsRegisteredOrRegistering(const Entry &aEntry) const
{
    return (aEntry.mIsRegistered || (aEntry.mAdvId != kInvalidRequestId));
}

template <typename Entry>
void AdvertisingProxy::DecideToAdvertise(Entry &aEntry, bool aUnregisterEntry, bool aUnregisterKey)
{
    // Decides whether to advertise `aEntry` or register its key.

    if (!aUnregisterKey && !IsKeyRegisteredOrRegistering(aEntry))
    {
        aEntry.mShouldRegisterKey = true;
        aEntry.mKeyAdvId          = AllocateNextRequestId();
    }

    VerifyOrExit(!aEntry.mShouldAdvertise);

    if (aUnregisterEntry || aEntry.IsDeleted())
    {
        aEntry.mShouldAdvertise = aEntry.mIsRegistered;
    }
    else if (!IsRegisteredOrRegistering(aEntry))
    {
        aEntry.mShouldAdvertise = true;
        aEntry.mAdvId           = AllocateNextRequestId();
    }

exit:
    return;
}

void AdvertisingProxy::Advertise(Host &aHost)
{
    bool shouldUnregisterHostAndServices = aHost.IsDeleted();
    bool shouldUnregisterKeys            = (aHost.mKeyLease == 0);

    DecideToAdvertise(aHost, shouldUnregisterHostAndServices, shouldUnregisterKeys);

    for (Service &service : aHost.mServices)
    {
        DecideToAdvertise(service, shouldUnregisterHostAndServices, shouldUnregisterKeys);
    }

    // We call `UpdateAdvIdRangeOn()` to determine the `mAdvIdRange`
    // on `aHost` before we call any of `UnregisterHost()`,
    // `UnregisterService()`, or `UnregisterKey()` methods, and
    // and receive any `HandleRegistered()` callbacks. The DNS-SD
    // platform may invoke `HandleRegistered()` callbacks from within
    // the `Register{Host/Service/Key}()` calls.

    UpdateAdvIdRangeOn(aHost);

    if (shouldUnregisterKeys)
    {
        UnregisterKey(aHost);
    }
    else if (aHost.mShouldRegisterKey)
    {
        RegisterKey(aHost);
    }

    // We register host first before any of its services.
    // But if we need to unregister host, it is done after
    // all services.

    if (aHost.mShouldAdvertise && !shouldUnregisterHostAndServices)
    {
        RegisterHost(aHost);
    }

    for (Service &service : aHost.mServices)
    {
        if (shouldUnregisterKeys)
        {
            UnregisterKey(service);
        }
        else if (service.mShouldRegisterKey)
        {
            RegisterKey(service);
        }

        if (service.mShouldAdvertise)
        {
            if (shouldUnregisterHostAndServices || service.IsDeleted())
            {
                UnregisterService(service);
            }
            else
            {
                RegisterService(service);
            }
        }
    }

    if (aHost.mShouldAdvertise && shouldUnregisterHostAndServices)
    {
        UnregisterHost(aHost);
    }
}

void AdvertisingProxy::UnregisterHostAndItsServicesAndKeys(Host &aHost)
{
    for (Service &service : aHost.mServices)
    {
        if (service.mIsKeyRegistered)
        {
            UnregisterKey(service);
        }

        if (!service.mIsReplaced && IsRegisteredOrRegistering(service))
        {
            UnregisterService(service);
        }
    }

    if (aHost.mIsKeyRegistered)
    {
        UnregisterKey(aHost);
    }

    if (!aHost.mIsReplaced && IsRegisteredOrRegistering(aHost))
    {
        UnregisterHost(aHost);
    }
}

bool AdvertisingProxy::CompareAndUpdateHostAndServices(Host &aHost, Host &aExistingHost)
{
    // This method compares and updates flags used by `AdvertisingProxy`
    // on new `aHost` and `aExistingHost` and their services.
    //
    // It returns a boolean indicating whether the new `aHost` replaced
    // any of the entries on the `aExistingHost`.
    //
    // The `AdvertisingProxy` uses the following flags and variables
    // on `Host` and `Service` entries:
    //
    // - `mIsRegistered` indicates whether or not the entry has been
    //   successfully registered by the proxy.
    //
    // - `mIsKeyRegistered` indicates whether or not a key record
    //   associated with the entry name has been successfully
    //   registered by the proxy on infrastructure DNS-SD.
    //
    // - `mAdvId` specifies the ongoing registration request ID
    //   associated with this entry by the proxy. A value of zero or
    //   `kInvalidRequestId` indicates that there is no ongoing
    //   registration for this entry.
    //
    // - `mKeyAdvId` is similar to `mAdvId` but for registering the
    //   key record.
    //
    // - `mIsReplaced` tracks whether this entry has been replaced by
    //   a newer advertisement request that changes some of its
    //   parameters. For example, the address list could have been
    //   changed on a `Host`, or TXT Data, or the list of sub-types,
    //   or port number could have been changed on a `Service`.
    //
    // - `mShouldAdvertise` is only used in the `Advertise()` call
    //   chain to track whether we need to advertise the entry.
    //
    // - `mShouldRegisterKey` is similar to `mShouldAdvertise` and
    //   only used in `Advertise()` call chain.

    bool replaced = false;

    VerifyOrExit(&aHost != &aExistingHost);

    replaced = CompareAndUpdateHost(aHost, aExistingHost);

    // Compare services of `aHost` against services of
    // `aExistingHost`.

    for (Service &service : aHost.mServices)
    {
        Service *existingService = aExistingHost.mServices.FindMatching(service.GetInstanceName());

        if (existingService != nullptr)
        {
            replaced |= CompareAndUpdateService(service, *existingService);
        }
    }

exit:
    return replaced;
}

template <typename Entry> void AdvertisingProxy::UpdateKeyRegistrationStatus(Entry &aEntry, const Entry &aExistingEntry)
{
    // Updates key registration status on `aEntry` based
    // on its state on `aExistingEntry`.

    static_assert(TypeTraits::IsSame<Entry, Host>::kValue || TypeTraits::IsSame<Entry, Service>::kValue,
                  "`Entry` must be `Host` or `Service` types");

    // If the new `aEntry` has a zero key lease, we always unregister
    // it, just to be safe. Therefore, there is no need to check the
    // key registration status of the existing `aExistingEntry`.

    VerifyOrExit(aEntry.GetKeyLease() != 0);

    VerifyOrExit(!IsKeyRegisteredOrRegistering(aEntry));

    if (aExistingEntry.mIsKeyRegistered)
    {
        aEntry.mIsKeyRegistered = true;
    }
    else
    {
        // Use the key registration request ID by `aExistingEntry` for
        // the new `aEntry` if there is any. If there is none the
        // `mKeyAdvId` remains as `kInvalidRequestId`.

        aEntry.mKeyAdvId = aExistingEntry.mKeyAdvId;
    }

exit:
    return;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
template <> bool AdvertisingProxy::EntriesMatch(const Host &aFirstHost, const Host &aSecondHost)
{
    bool match = false;

    VerifyOrExit(aFirstHost.IsDeleted() == aSecondHost.IsDeleted());

    if (aFirstHost.IsDeleted())
    {
        match = true;
        ExitNow();
    }

    VerifyOrExit(aFirstHost.mAddresses.GetLength() == aSecondHost.mAddresses.GetLength());

    for (const Ip6::Address &address : aFirstHost.mAddresses)
    {
        VerifyOrExit(aSecondHost.mAddresses.Contains(address));
    }

    match = true;

exit:
    return match;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
template <> bool AdvertisingProxy::EntriesMatch(const Service &aFirstService, const Service &aSecondService)
{
    bool match = false;

    VerifyOrExit(aFirstService.IsDeleted() == aSecondService.IsDeleted());

    if (aFirstService.IsDeleted())
    {
        match = true;
        ExitNow();
    }

    VerifyOrExit(aFirstService.GetPort() == aSecondService.GetPort());
    VerifyOrExit(aFirstService.GetWeight() == aSecondService.GetWeight());
    VerifyOrExit(aFirstService.GetPriority() == aSecondService.GetPriority());
    VerifyOrExit(aFirstService.GetTtl() == aSecondService.GetTtl());

    VerifyOrExit(aFirstService.GetNumberOfSubTypes() == aSecondService.GetNumberOfSubTypes());

    for (uint16_t index = 0; index < aFirstService.GetNumberOfSubTypes(); index++)
    {
        VerifyOrExit(aSecondService.HasSubTypeServiceName(aFirstService.GetSubTypeServiceNameAt(index)));
    }

    VerifyOrExit(aFirstService.GetTxtDataLength() == aSecondService.GetTxtDataLength());
    VerifyOrExit(!memcmp(aFirstService.GetTxtData(), aSecondService.GetTxtData(), aFirstService.GetTxtDataLength()));

    match = true;

exit:
    return match;
}

template <typename Entry> bool AdvertisingProxy::CompareAndUpdate(Entry &aEntry, Entry &aExistingEntry)
{
    // This is called when the new `aEntry` is not deleted.

    bool replaced = false;

    // If we previously determined that `aEntry` is registered,
    // nothing else to do.

    VerifyOrExit(!aEntry.mIsRegistered);

    if (aEntry.mShouldAdvertise || aExistingEntry.mIsReplaced || !EntriesMatch(aEntry, aExistingEntry))
    {
        // If we previously determined that we should advertise the
        // new `aEntry`, we enter this block to mark `aExistingEntry`
        // as being replaced.
        //
        // If `aExistingEntry` was already marked as replaced, we
        // cannot compare it to the new `aEntry`. Therefore, we assume
        // that there may be a change and always advertise the new
        // `aEntry`. Otherwise, we compare it to the new `aEntry` using
        // `EntriesMatch()` and only if there are any differences, we
        // mark that `aEntry` needs to be advertised.

        aExistingEntry.mIsReplaced = true;
        replaced                   = true;

        if (aEntry.mAdvId == kInvalidRequestId)
        {
            aEntry.mShouldAdvertise = true;
            aEntry.mAdvId           = AllocateNextRequestId();
        }

        // If there is an outstanding registration request for
        // `aExistingEntry` we replace it with the request ID of the
        // new `aEntry` registration.

        if (aExistingEntry.mAdvId != kInvalidRequestId)
        {
            aExistingEntry.mAdvId = aEntry.mAdvId;
            UpdateAdvIdRangeOn(aExistingEntry);
        }

        ExitNow();
    }

    // `aEntry` fully matches `aExistingEntry` and `aExistingEntry` was
    // not replaced.

    VerifyOrExit(aEntry.mAdvId == kInvalidRequestId);

    if (aExistingEntry.mIsRegistered)
    {
        aEntry.mIsRegistered = true;
    }
    else if (aExistingEntry.mAdvId != kInvalidRequestId)
    {
        // There is an outstanding registration request for
        // `aExistingEntry`. We use the same ID for the new `aEntry`.

        aEntry.mAdvId = aExistingEntry.mAdvId;
    }
    else
    {
        // The earlier advertisement of `aExistingEntry` seems to have
        // failed since there is no outstanding registration request
        // (no ID) and it is not marked as registered. We mark the
        // new `aEntry` to be advertised (to try again).

        aEntry.mShouldAdvertise    = true;
        aEntry.mAdvId              = AllocateNextRequestId();
        aExistingEntry.mIsReplaced = true;
    }

exit:
    return replaced;
}

bool AdvertisingProxy::CompareAndUpdateHost(Host &aHost, Host &aExistingHost)
{
    bool replaced = false;

    UpdateKeyRegistrationStatus(aHost, aExistingHost);

    if (!aHost.IsDeleted())
    {
        replaced = CompareAndUpdate(aHost, aExistingHost);
        ExitNow();
    }

    // The new `aHost` is removing the host and all its services.

    if (aExistingHost.IsDeleted())
    {
        // If `aHost` has zero key-lease (fully removed),
        // we need to unregister keys for any services on
        // existing host that are not present in `aHost`.

        if (aHost.mKeyLease == 0)
        {
            for (Service &existingService : aExistingHost.mServices)
            {
                if (!aHost.HasService(existingService.GetInstanceName()))
                {
                    UnregisterKey(existingService);
                }
            }
        }

        ExitNow();
    }

    // `aExistingHost` is updating the same host that is being
    // removed by the new `aHost` update. We need to advertise
    // the new `aHost` to make sure it is unregistered.

    aHost.mShouldAdvertise = true;

    // We unregister any services that were registered by
    // `aExistingHost` but are not included in the now being
    // removed `aHost`, and unregister any registered keys when
    // `aHost` has zero key lease.

    for (Service &existingService : aExistingHost.mServices)
    {
        if (existingService.IsDeleted())
        {
            if (aHost.GetKeyLease() == 0)
            {
                existingService.mIsReplaced = true;
                UnregisterKey(existingService);
            }

            continue;
        }

        if (aHost.HasService(existingService.GetInstanceName()))
        {
            // The `existingService` that are contained in `aHost`
            // are updated in `CompareAndUpdateService()`.
            continue;
        }

        UnregisterService(existingService);

        existingService.mIsReplaced = true;

        if (aHost.GetKeyLease() == 0)
        {
            UnregisterKey(existingService);
        }
    }

    aExistingHost.mAdvId      = kInvalidRequestId;
    aExistingHost.mIsReplaced = true;
    replaced                  = true;

    if (aHost.GetKeyLease() == 0)
    {
        UnregisterKey(aExistingHost);
    }

    UpdateAdvIdRangeOn(aExistingHost);

exit:
    return replaced;
}

bool AdvertisingProxy::CompareAndUpdateService(Service &aService, Service &aExistingService)
{
    bool replaced = false;

    UpdateKeyRegistrationStatus(aService, aExistingService);

    if (!aService.IsDeleted())
    {
        replaced = CompareAndUpdate(aService, aExistingService);
        ExitNow();
    }

    if (aExistingService.IsDeleted())
    {
        ExitNow();
    }

    aService.mShouldAdvertise = true;

    aExistingService.mIsReplaced = true;
    replaced                     = true;

    if (aExistingService.mAdvId != kInvalidRequestId)
    {
        // If there is an outstanding registration request for the
        // existing service, clear its request ID.

        aExistingService.mAdvId = kInvalidRequestId;

        UpdateAdvIdRangeOn(*aExistingService.mHost);
    }

exit:
    return replaced;
}

void AdvertisingProxy::RegisterHost(Host &aHost)
{
    Error                     error = kErrorNone;
    Dnssd::Host               hostInfo;
    DnsName                   hostName;
    Heap::Array<Ip6::Address> hostAddresses;

    aHost.mShouldAdvertise = false;

    CopyNameAndRemoveDomain(hostName, aHost.GetFullName());

    SuccessOrExit(error = hostAddresses.ReserveCapacity(aHost.mAddresses.GetLength()));

    for (const Ip6::Address &address : aHost.mAddresses)
    {
        if (!address.IsLinkLocalUnicast() && !Get<Mle::Mle>().IsMeshLocalAddress(address))
        {
            IgnoreError(hostAddresses.PushBack(address));
        }
    }

    LogInfo("Registering host '%s', id:%lu", hostName, ToUlong(aHost.mAdvId));

    hostInfo.Clear();
    hostInfo.mHostName        = hostName;
    hostInfo.mAddresses       = hostAddresses.AsCArray();
    hostInfo.mAddressesLength = hostAddresses.GetLength();
    hostInfo.mTtl             = aHost.GetTtl();
    hostInfo.mInfraIfIndex    = Get<BorderRouter::InfraIf>().GetIfIndex();

    Get<Dnssd>().RegisterHost(hostInfo, aHost.mAdvId, HandleRegistered);

exit:
    if (error != kErrorNone)
    {
        LogWarn("Error %s registering host '%s'", ErrorToString(error), hostName);
    }
}

void AdvertisingProxy::UnregisterHost(Host &aHost)
{
    Dnssd::Host hostInfo;
    DnsName     hostName;

    aHost.mShouldAdvertise = false;
    aHost.mIsRegistered    = false;
    aHost.mAdvId           = false;

    CopyNameAndRemoveDomain(hostName, aHost.GetFullName());

    LogInfo("Unregistering host '%s'", hostName);

    hostInfo.Clear();
    hostInfo.mHostName     = hostName;
    hostInfo.mInfraIfIndex = Get<BorderRouter::InfraIf>().GetIfIndex();

    Get<Dnssd>().UnregisterHost(hostInfo, 0, nullptr);
}

void AdvertisingProxy::RegisterService(Service &aService)
{
    Error                     error = kErrorNone;
    Dnssd::Service            serviceInfo;
    DnsName                   hostName;
    DnsName                   serviceName;
    Heap::Array<Heap::String> subTypeHeapStrings;
    Heap::Array<const char *> subTypeLabels;

    aService.mShouldAdvertise = false;

    CopyNameAndRemoveDomain(hostName, aService.GetHost().GetFullName());
    CopyNameAndRemoveDomain(serviceName, aService.GetServiceName());

    SuccessOrExit(error = subTypeHeapStrings.ReserveCapacity(aService.mSubTypes.GetLength()));
    SuccessOrExit(error = subTypeLabels.ReserveCapacity(aService.mSubTypes.GetLength()));

    for (const Heap::String &subTypeName : aService.mSubTypes)
    {
        char         label[Dns::Name::kMaxLabelSize];
        Heap::String labelString;

        IgnoreError(Server::Service::ParseSubTypeServiceName(subTypeName.AsCString(), label, sizeof(label)));
        SuccessOrExit(error = labelString.Set(label));
        IgnoreError(subTypeHeapStrings.PushBack(static_cast<Heap::String &&>(labelString)));
        IgnoreError(subTypeLabels.PushBack(subTypeHeapStrings.Back()->AsCString()));
    }

    LogInfo("Registering service '%s' '%s' on '%s', id:%lu", aService.GetInstanceLabel(), serviceName, hostName,
            ToUlong(aService.mAdvId));

    serviceInfo.Clear();
    serviceInfo.mHostName            = hostName;
    serviceInfo.mServiceInstance     = aService.GetInstanceLabel();
    serviceInfo.mServiceType         = serviceName;
    serviceInfo.mSubTypeLabels       = subTypeLabels.AsCArray();
    serviceInfo.mSubTypeLabelsLength = subTypeLabels.GetLength();
    serviceInfo.mTxtData             = aService.GetTxtData();
    serviceInfo.mTxtDataLength       = aService.GetTxtDataLength();
    serviceInfo.mPort                = aService.GetPort();
    serviceInfo.mWeight              = aService.GetWeight();
    serviceInfo.mPriority            = aService.GetPriority();
    serviceInfo.mTtl                 = aService.GetTtl();
    serviceInfo.mInfraIfIndex        = Get<BorderRouter::InfraIf>().GetIfIndex();

    Get<Dnssd>().RegisterService(serviceInfo, aService.mAdvId, HandleRegistered);

exit:
    if (error != kErrorNone)
    {
        LogWarn("Error %s registering service '%s' '%s'", ErrorToString(error), aService.GetInstanceLabel(),
                serviceName);
    }
}

void AdvertisingProxy::UnregisterService(Service &aService)
{
    Dnssd::Service serviceInfo;
    DnsName        hostName;
    DnsName        serviceName;

    aService.mShouldAdvertise = false;
    aService.mIsRegistered    = false;
    aService.mAdvId           = kInvalidRequestId;

    CopyNameAndRemoveDomain(hostName, aService.GetHost().GetFullName());
    CopyNameAndRemoveDomain(serviceName, aService.GetServiceName());

    LogInfo("Unregistering service '%s' '%s' on '%s'", aService.GetInstanceLabel(), serviceName, hostName);

    serviceInfo.Clear();
    serviceInfo.mHostName        = hostName;
    serviceInfo.mServiceInstance = aService.GetInstanceLabel();
    serviceInfo.mServiceType     = serviceName;
    serviceInfo.mInfraIfIndex    = Get<BorderRouter::InfraIf>().GetIfIndex();

    Get<Dnssd>().UnregisterService(serviceInfo, 0, nullptr);
}

void AdvertisingProxy::RegisterKey(Host &aHost)
{
    DnsName hostName;

    aHost.mShouldRegisterKey = false;

    CopyNameAndRemoveDomain(hostName, aHost.GetFullName());

    LogInfo("Registering key for host '%s', id:%lu", hostName, ToUlong(aHost.mKeyAdvId));

    RegisterKey(hostName, /* aServiceType */ nullptr, aHost.mKey, aHost.mKeyAdvId, aHost.GetTtl());
}

void AdvertisingProxy::RegisterKey(Service &aService)
{
    DnsName serviceType;

    aService.mShouldRegisterKey = false;

    CopyNameAndRemoveDomain(serviceType, aService.GetServiceName());

    LogInfo("Registering key for service '%s' '%s', id:%lu", aService.GetInstanceLabel(), serviceType,
            ToUlong(aService.mKeyAdvId));

    RegisterKey(aService.GetInstanceLabel(), serviceType, aService.mHost->mKey, aService.mKeyAdvId, aService.GetTtl());
}

void AdvertisingProxy::RegisterKey(const char      *aName,
                                   const char      *aServiceType,
                                   const Host::Key &aKey,
                                   RequestId        aRequestId,
                                   uint32_t         aTtl)
{
    Dnssd::Key             keyInfo;
    Dns::Ecdsa256KeyRecord keyRecord;

    keyRecord.Init();
    keyRecord.SetFlags(Dns::KeyRecord::kAuthConfidPermitted, Dns::KeyRecord::kOwnerNonZone,
                       Dns::KeyRecord::kSignatoryFlagGeneral);
    keyRecord.SetProtocol(Dns::KeyRecord::kProtocolDnsSec);
    keyRecord.SetAlgorithm(Dns::KeyRecord::kAlgorithmEcdsaP256Sha256);
    keyRecord.SetLength(sizeof(Dns::Ecdsa256KeyRecord) - sizeof(Dns::ResourceRecord));
    keyRecord.SetKey(aKey);

    keyInfo.Clear();
    keyInfo.mName          = aName;
    keyInfo.mServiceType   = aServiceType;
    keyInfo.mKeyData       = reinterpret_cast<uint8_t *>(&keyRecord) + sizeof(Dns::ResourceRecord);
    keyInfo.mKeyDataLength = keyRecord.GetLength();
    keyInfo.mClass         = Dns::ResourceRecord::kClassInternet;
    keyInfo.mTtl           = aTtl;
    keyInfo.mInfraIfIndex  = Get<BorderRouter::InfraIf>().GetIfIndex();

    Get<Dnssd>().RegisterKey(keyInfo, aRequestId, HandleRegistered);
}

void AdvertisingProxy::UnregisterKey(Host &aHost)
{
    DnsName hostName;

    aHost.mIsKeyRegistered = false;
    aHost.mKeyAdvId        = kInvalidRequestId;

    CopyNameAndRemoveDomain(hostName, aHost.GetFullName());

    LogInfo("Unregistering key for host '%s'", hostName);

    UnregisterKey(hostName, /* aServiceType */ nullptr);
}

void AdvertisingProxy::UnregisterKey(Service &aService)
{
    DnsName serviceType;

    aService.mIsKeyRegistered = false;
    aService.mKeyAdvId        = kInvalidRequestId;

    CopyNameAndRemoveDomain(serviceType, aService.GetServiceName());

    LogInfo("Unregistering key for service '%s' '%s'", aService.GetInstanceLabel(), serviceType);

    UnregisterKey(aService.GetInstanceLabel(), serviceType);
}

void AdvertisingProxy::UnregisterKey(const char *aName, const char *aServiceType)
{
    Dnssd::Key keyInfo;

    keyInfo.Clear();
    keyInfo.mName         = aName;
    keyInfo.mServiceType  = aServiceType;
    keyInfo.mInfraIfIndex = Get<BorderRouter::InfraIf>().GetIfIndex();

    Get<Dnssd>().UnregisterKey(keyInfo, 0, nullptr);
}

void AdvertisingProxy::CopyNameAndRemoveDomain(DnsName &aName, const char *aFullName)
{
    IgnoreError(Dns::Name::ExtractLabels(aFullName, Get<Server>().GetDomain(), aName, sizeof(aName)));
}

void AdvertisingProxy::HandleRegistered(otInstance *aInstance, otPlatDnssdRequestId aRequestId, otError aError)
{
    AsCoreType(aInstance).Get<AdvertisingProxy>().HandleRegistered(aRequestId, aError);
}

void AdvertisingProxy::HandleRegistered(RequestId aRequestId, Error aError)
{
    LogInfo("Register callback, id:%lu, error:%s", ToUlong(aRequestId), ErrorToString(aError));

    VerifyOrExit(mState == kStateRunning);

    for (Host &host : Get<Server>().mHosts)
    {
        HandleRegisteredRequestIdOn(host, aRequestId, aError);
    }

    for (AdvInfo &adv : mAdvInfoList)
    {
        if (HandleRegisteredRequestIdOn(adv.mHost, aRequestId, aError))
        {
            if (adv.mError == kErrorNone)
            {
                adv.mError = aError;
            }

            if (adv.IsCompleted())
            {
                mTasklet.Post();
            }
        }
    }

exit:
    return;
}

bool AdvertisingProxy::HandleRegisteredRequestIdOn(Host &aHost, RequestId aRequestId, Error aError)
{
    // Handles "registration request callback" for `aRequestId` on a
    // given `aHost`. Returns `true`, if the ID matched an entry
    // on `aHost` and `aHost` was updated, `false` otherwise.

    bool didUpdate = false;

    VerifyOrExit(aHost.mAdvIdRange.Contains(aRequestId));

    if (aHost.mAdvId == aRequestId)
    {
        aHost.mAdvId        = kInvalidRequestId;
        aHost.mIsRegistered = (aError == kErrorNone);
        didUpdate           = true;
    }

    if (aHost.mKeyAdvId == aRequestId)
    {
        aHost.mKeyAdvId        = kInvalidRequestId;
        aHost.mIsKeyRegistered = true;
        didUpdate              = true;
    }

    for (Service &service : aHost.mServices)
    {
        if (service.mAdvId == aRequestId)
        {
            service.mAdvId        = kInvalidRequestId;
            service.mIsRegistered = (aError == kErrorNone);
            didUpdate             = true;
        }

        if (service.mKeyAdvId == aRequestId)
        {
            service.mKeyAdvId        = kInvalidRequestId;
            service.mIsKeyRegistered = true;
            didUpdate                = true;
        }
    }

    UpdateAdvIdRangeOn(aHost);

exit:
    return didUpdate;
}

void AdvertisingProxy::HandleTimer(void)
{
    NextFireTime        nextTime;
    OwningList<AdvInfo> expiredList;

    VerifyOrExit(mState == kStateRunning);

    mAdvInfoList.RemoveAllMatching(AdvInfo::ExpirationChecker(nextTime.GetNow()), expiredList);

    for (AdvInfo &adv : mAdvInfoList)
    {
        nextTime.UpdateIfEarlier(adv.mExpireTime);
    }

    mTimer.FireAtIfEarlier(nextTime);

    for (AdvInfo &adv : expiredList)
    {
        adv.mError       = kErrorResponseTimeout;
        adv.mBlockingAdv = nullptr;
        adv.mHost.mAdvIdRange.Clear();
        SignalAdvCompleted(adv);
    }

exit:
    return;
}

void AdvertisingProxy::HandleTasklet(void)
{
    VerifyOrExit(mState == kStateRunning);

    while (true)
    {
        OwningList<AdvInfo> completedList;

        mAdvInfoList.RemoveAllMatching(AdvInfo::CompletionChecker(), completedList);

        VerifyOrExit(!completedList.IsEmpty());

        // `RemoveAllMatching()` reverses the order of removed entries
        // from `mAdvInfoList` (which itself keeps the later requests
        // towards the head of the list). This means that the
        // `completedList` will be sorted from earliest request to
        // latest and this is the order that we want to notify
        // `Srp::Server`.

        for (AdvInfo &adv : completedList)
        {
            SignalAdvCompleted(adv);
        }

        completedList.Clear();
    }

exit:
    return;
}

void AdvertisingProxy::SignalAdvCompleted(AdvInfo &aAdvInfo)
{
    // Check if any outstanding advertisements in the list
    // is blocked by `aAdvInfo` and unblock.

    for (AdvInfo &adv : mAdvInfoList)
    {
        if (adv.mBlockingAdv == &aAdvInfo)
        {
            adv.mBlockingAdv = nullptr;

            if (adv.IsCompleted())
            {
                mTasklet.Post();
            }
        }
    }

    switch (aAdvInfo.mError)
    {
    case kErrorNone:
        mCounters.mAdvSuccessful++;
        break;
    case kErrorResponseTimeout:
        mCounters.mAdvTimeout++;
        break;
    default:
        mCounters.mAdvRejected++;
        break;
    }

    aAdvInfo.SignalServerToCommit();
}

//---------------------------------------------------------------------------------------------------------------------
// AdvertisingProxy::AdvInfo

AdvertisingProxy::AdvInfo::AdvInfo(Host &aHost, const Server::MessageMetadata &aMetadata, uint32_t aTimeout)
    : mNext(nullptr)
    , mBlockingAdv(nullptr)
    , mHost(aHost)
    , mExpireTime(TimerMilli::GetNow() + aTimeout)
    , mMessageMetadata(aMetadata)
    , mError(kErrorNone)
{
    if (aMetadata.mMessageInfo != nullptr)
    {
        // If `mMessageInfo` is not null in the given `aMetadata` keep
        // a copy of it in `AdvInfo` structure and update the
        // `mMessageMetadata` to point to the local copy instead.

        mMessageInfo                  = *aMetadata.mMessageInfo;
        mMessageMetadata.mMessageInfo = &mMessageInfo;
    }
}

void AdvertisingProxy::AdvInfo::SignalServerToCommit(void)
{
    LogInfo("Adv done '%s', error:%s", mHost.GetFullName(), ErrorToString(mError));
    Get<Server>().CommitSrpUpdate(mError, mHost, mMessageMetadata);
}

bool AdvertisingProxy::AdvInfo::IsCompleted(void) const
{
    bool isCompleted = false;

    VerifyOrExit(mBlockingAdv == nullptr);
    isCompleted = (mError != kErrorNone) || mHost.mAdvIdRange.IsEmpty();

exit:
    return isCompleted;
}

} // namespace Srp
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE
