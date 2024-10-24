/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#include "srp_client.hpp"

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

#include "instance/instance.hpp"

/**
 * @file
 *   This file implements the SRP client.
 */

namespace ot {
namespace Srp {

RegisterLogModule("SrpClient");

//---------------------------------------------------------------------
// Client::HostInfo

void Client::HostInfo::Init(void)
{
    Clearable<HostInfo>::Clear();

    // State is directly set on `mState` instead of using `SetState()`
    // to avoid logging.
    mState = OT_SRP_CLIENT_ITEM_STATE_REMOVED;
}

void Client::HostInfo::Clear(void)
{
    Clearable<HostInfo>::Clear();
    SetState(kRemoved);
}

bool Client::HostInfo::SetState(ItemState aState)
{
    bool didChange;

    VerifyOrExit(aState != GetState(), didChange = false);

    LogInfo("HostInfo %s -> %s", ItemStateToString(GetState()), ItemStateToString(aState));

    mState    = MapEnum(aState);
    didChange = true;

exit:
    return didChange;
}

void Client::HostInfo::EnableAutoAddress(void)
{
    mAddresses    = nullptr;
    mNumAddresses = 0;
    mAutoAddress  = true;

    LogInfo("HostInfo enabled auto address");
}

void Client::HostInfo::SetAddresses(const Ip6::Address *aAddresses, uint8_t aNumAddresses)
{
    mAddresses    = aAddresses;
    mNumAddresses = aNumAddresses;
    mAutoAddress  = false;

    LogInfo("HostInfo set %d addrs", GetNumAddresses());

    for (uint8_t index = 0; index < GetNumAddresses(); index++)
    {
        LogInfo("%s", GetAddress(index).ToString().AsCString());
    }
}

//---------------------------------------------------------------------
// Client::Service

Error Client::Service::Init(void)
{
    Error error = kErrorNone;

    VerifyOrExit((GetName() != nullptr) && (GetInstanceName() != nullptr), error = kErrorInvalidArgs);
    VerifyOrExit((GetTxtEntries() != nullptr) || (GetNumTxtEntries() == 0), error = kErrorInvalidArgs);

    // State is directly set on `mState` instead of using `SetState()`
    // to avoid logging.
    mState = OT_SRP_CLIENT_ITEM_STATE_REMOVED;

    mLease    = Min(mLease, kMaxLease);
    mKeyLease = Min(mKeyLease, kMaxLease);

exit:
    return error;
}

bool Client::Service::SetState(ItemState aState)
{
    bool didChange;

    VerifyOrExit(GetState() != aState, didChange = false);

    LogInfo("Service %s -> %s, \"%s\" \"%s\"", ItemStateToString(GetState()), ItemStateToString(aState),
            GetInstanceName(), GetName());

    if (aState == kToAdd)
    {
        constexpr uint16_t kSubTypeLabelStringSize = 80;

        String<kSubTypeLabelStringSize> string;

        // Log more details only when entering `kToAdd` state.

        if (HasSubType())
        {
            const char *label;

            for (uint16_t index = 0; (label = GetSubTypeLabelAt(index)) != nullptr; index++)
            {
                string.Append("%s\"%s\"", (index != 0) ? ", " : "", label);
            }
        }

        LogInfo("subtypes:[%s] port:%d weight:%d prio:%d txts:%d", string.AsCString(), GetPort(), GetWeight(),
                GetPriority(), GetNumTxtEntries());
    }

    mState    = MapEnum(aState);
    didChange = true;

exit:
    return didChange;
}

bool Client::Service::Matches(const Service &aOther) const
{
    // This method indicates whether or not two service entries match,
    // i.e., have the same service and instance names. This is intended
    // for use by `LinkedList::FindMatching()` to search within the
    // `mServices` list.

    return StringMatch(GetName(), aOther.GetName()) && StringMatch(GetInstanceName(), aOther.GetInstanceName());
}

//---------------------------------------------------------------------
// Client::TxJitter

const uint32_t Client::TxJitter::kMaxJitters[] = {
    Client::kMaxTxJitterOnDeviceReboot,    // (0) kOnDeviceReboot
    Client::kMaxTxJitterOnServerStart,     // (1) kOnServerStart
    Client::kMaxTxJitterOnServerRestart,   // (2) kOnServerRestart
    Client::kMaxTxJitterOnServerSwitch,    // (3) kOnServerSwitch
    Client::kMaxTxJitterOnSlaacAddrAdd,    // (4) kOnSlaacAddrAdd
    Client::kMaxTxJitterOnSlaacAddrRemove, // (5) kOnSlaacAddrRemove
};

void Client::TxJitter::Request(Reason aReason)
{
    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kOnDeviceReboot);
        ValidateNextEnum(kOnServerStart);
        ValidateNextEnum(kOnServerRestart);
        ValidateNextEnum(kOnServerSwitch);
        ValidateNextEnum(kOnSlaacAddrAdd);
        ValidateNextEnum(kOnSlaacAddrRemove);
    };

    uint32_t maxJitter = kMaxJitters[aReason];

    LogInfo("Requesting max tx jitter %lu (%s)", ToUlong(maxJitter), ReasonToString(aReason));

    if (mRequestedMax != 0)
    {
        // If we have a previous request, adjust the `mRequestedMax`
        // based on the time elapsed since that request was made.

        uint32_t duration = TimerMilli::GetNow() - mRequestTime;

        mRequestedMax = (mRequestedMax > duration) ? mRequestedMax - duration : 0;
    }

    mRequestedMax = Max(mRequestedMax, maxJitter);
    mRequestTime  = TimerMilli::GetNow();
}

uint32_t Client::TxJitter::DetermineDelay(void)
{
    uint32_t delay;
    uint32_t maxJitter = kMaxTxJitterDefault;

    if (mRequestedMax != 0)
    {
        uint32_t duration = TimerMilli::GetNow() - mRequestTime;

        if (duration >= mRequestedMax)
        {
            LogInfo("Requested max tx jitter %lu already expired", ToUlong(mRequestedMax));
        }
        else
        {
            maxJitter = Max(mRequestedMax - duration, kMaxTxJitterDefault);
            LogInfo("Applying remaining max jitter %lu", ToUlong(maxJitter));
        }

        mRequestedMax = 0;
    }

    delay = Random::NonCrypto::GetUint32InRange(kMinTxJitter, maxJitter);
    LogInfo("Use random tx jitter %lu from [%lu, %lu]", ToUlong(delay), ToUlong(kMinTxJitter), ToUlong(maxJitter));

    return delay;
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
const char *Client::TxJitter::ReasonToString(Reason aReason)
{
    static const char *const kReasonStrings[] = {
        "OnDeviceReboot",    // (0) kOnDeviceReboot
        "OnServerStart",     // (1) kOnServerStart
        "OnServerRestart",   // (2) kOnServerRestart
        "OnServerSwitch",    // (3) kOnServerSwitch
        "OnSlaacAddrAdd",    // (4) kOnSlaacAddrAdd
        "OnSlaacAddrRemove", // (5) kOnSlaacAddrRemove
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kOnDeviceReboot);
        ValidateNextEnum(kOnServerStart);
        ValidateNextEnum(kOnServerRestart);
        ValidateNextEnum(kOnServerSwitch);
        ValidateNextEnum(kOnSlaacAddrAdd);
        ValidateNextEnum(kOnSlaacAddrRemove);
    };

    return kReasonStrings[aReason];
}
#endif

//---------------------------------------------------------------------
// Client::AutoStart

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE

Client::AutoStart::AutoStart(void)
{
    Clear();
    mState = kDefaultMode ? kFirstTimeSelecting : kDisabled;
}

bool Client::AutoStart::HasSelectedServer(void) const
{
    bool hasSelected = false;

    switch (mState)
    {
    case kDisabled:
    case kFirstTimeSelecting:
    case kReselecting:
        break;

    case kSelectedUnicastPreferred:
    case kSelectedUnicast:
    case kSelectedAnycast:
        hasSelected = true;
        break;
    }

    return hasSelected;
}

void Client::AutoStart::SetState(State aState)
{
    if (mState != aState)
    {
        LogInfo("AutoStartState %s -> %s", StateToString(mState), StateToString(aState));
        mState = aState;
    }
}

void Client::AutoStart::InvokeCallback(const Ip6::SockAddr *aServerSockAddr) const
{
    mCallback.InvokeIfSet(aServerSockAddr);
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
const char *Client::AutoStart::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Disabled",      // (0) kDisabled
        "1stTimeSelect", // (1) kFirstTimeSelecting
        "Reselect",      // (2) kReselecting
        "Unicast-prf",   // (3) kSelectedUnicastPreferred
        "Anycast",       // (4) kSelectedAnycast
        "Unicast",       // (5) kSelectedUnicast
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kDisabled);
        ValidateNextEnum(kFirstTimeSelecting);
        ValidateNextEnum(kReselecting);
        ValidateNextEnum(kSelectedUnicastPreferred);
        ValidateNextEnum(kSelectedAnycast);
        ValidateNextEnum(kSelectedUnicast);
    };

    return kStateStrings[aState];
}
#endif

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE

//---------------------------------------------------------------------
// Client

const char Client::kDefaultDomainName[] = "default.service.arpa";

Client::Client(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStateStopped)
    , mTxFailureRetryCount(0)
    , mShouldRemoveKeyLease(false)
    , mSingleServiceMode(false)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mServiceKeyRecordEnabled(false)
    , mUseShortLeaseOption(false)
#endif
    , mNextMessageId(0)
    , mResponseMessageId(0)
    , mAutoHostAddressCount(0)
    , mRetryWaitInterval(kMinRetryWaitInterval)
    , mTtl(0)
    , mLease(0)
    , mKeyLease(0)
    , mDefaultLease(kDefaultLease)
    , mDefaultKeyLease(kDefaultKeyLease)
    , mSocket(aInstance, *this)
    , mDomainName(kDefaultDomainName)
    , mTimer(aInstance)
#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
    , mGuardTimer(aInstance)
#endif
{
    // The `Client` implementation uses different constant array of
    // `ItemState` to define transitions between states in `Pause()`,
    // `Stop()`, `SendUpdate`, and `ProcessResponse()`, or to convert
    // an `ItemState` to string. Here, we assert that the enumeration
    // values are correct.

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kToAdd);
        ValidateNextEnum(kAdding);
        ValidateNextEnum(kToRefresh);
        ValidateNextEnum(kRefreshing);
        ValidateNextEnum(kToRemove);
        ValidateNextEnum(kRemoving);
        ValidateNextEnum(kRegistered);
        ValidateNextEnum(kRemoved);
    };

    mHostInfo.Init();
}

Error Client::Start(const Ip6::SockAddr &aServerSockAddr, Requester aRequester)
{
    Error error;

    VerifyOrExit(GetState() == kStateStopped,
                 error = (aServerSockAddr == GetServerAddress()) ? kErrorNone : kErrorBusy);

    SuccessOrExit(error = mSocket.Open());

    error = mSocket.Connect(aServerSockAddr);
    if (error != kErrorNone)
    {
        LogInfo("Failed to connect to server %s: %s", aServerSockAddr.GetAddress().ToString().AsCString(),
                ErrorToString(error));
        IgnoreError(mSocket.Close());
        ExitNow();
    }

    LogInfo("%starting, server %s", (aRequester == kRequesterUser) ? "S" : "Auto-s",
            aServerSockAddr.ToString().AsCString());

    Resume();

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
    if (aRequester == kRequesterAuto)
    {
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE && OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_SERVER_ADDRESS_AUTO_SET_ENABLE
        Get<Dns::Client>().UpdateDefaultConfigAddress();
#endif
        mAutoStart.InvokeCallback(&aServerSockAddr);
    }
#endif

exit:
    return error;
}

void Client::Stop(Requester aRequester, StopMode aMode)
{
    // Change the state of host info and services so that they are
    // added/removed again once the client is started back. In the
    // case of `kAdding`, we intentionally move to `kToRefresh`
    // instead of `kToAdd` since the server may receive our add
    // request and the item may be registered on the server. This
    // ensures that if we are later asked to remove the item, we do
    // notify server.

    static const ItemState kNewStateOnStop[]{
        /* (0) kToAdd      -> */ kToAdd,
        /* (1) kAdding     -> */ kToRefresh,
        /* (2) kToRefresh  -> */ kToRefresh,
        /* (3) kRefreshing -> */ kToRefresh,
        /* (4) kToRemove   -> */ kToRemove,
        /* (5) kRemoving   -> */ kToRemove,
        /* (6) kRegistered -> */ kToRefresh,
        /* (7) kRemoved    -> */ kRemoved,
    };

    VerifyOrExit(GetState() != kStateStopped);

    mSingleServiceMode = false;

    // State changes:
    //   kAdding     -> kToRefresh
    //   kRefreshing -> kToRefresh
    //   kRemoving   -> kToRemove
    //   kRegistered -> kToRefresh

    ChangeHostAndServiceStates(kNewStateOnStop, kForAllServices);

    IgnoreError(mSocket.Close());

    mShouldRemoveKeyLease = false;
    mTxFailureRetryCount  = 0;
    mResponseMessageId    = mNextMessageId;

    if (aMode == kResetRetryInterval)
    {
        ResetRetryWaitInterval();
    }

    SetState(kStateStopped);

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
#if OPENTHREAD_CONFIG_SRP_CLIENT_SWITCH_SERVER_ON_FAILURE
    mAutoStart.ResetTimeoutFailureCount();
#endif
    if (aRequester == kRequesterAuto)
    {
        mAutoStart.InvokeCallback(nullptr);
    }
#endif

exit:
#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
    if (aRequester == kRequesterUser)
    {
        DisableAutoStartMode();
    }
#endif
}

void Client::Resume(void)
{
    SetState(kStateUpdated);
    UpdateState();
}

void Client::Pause(void)
{
    // Change the state of host info and services that are are being
    // added or removed so that they are added/removed again once the
    // client is resumed or started back.

    static const ItemState kNewStateOnPause[]{
        /* (0) kToAdd      -> */ kToAdd,
        /* (1) kAdding     -> */ kToRefresh,
        /* (2) kToRefresh  -> */ kToRefresh,
        /* (3) kRefreshing -> */ kToRefresh,
        /* (4) kToRemove   -> */ kToRemove,
        /* (5) kRemoving   -> */ kToRemove,
        /* (6) kRegistered -> */ kRegistered,
        /* (7) kRemoved    -> */ kRemoved,
    };

    mSingleServiceMode = false;

    // State changes:
    //   kAdding     -> kToRefresh
    //   kRefreshing -> kToRefresh
    //   kRemoving   -> kToRemove

    ChangeHostAndServiceStates(kNewStateOnPause, kForAllServices);

    SetState(kStatePaused);
}

void Client::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        HandleRoleChanged();
    }

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
    if (aEvents.ContainsAny(kEventThreadNetdataChanged | kEventThreadMeshLocalAddrChanged))
    {
        ProcessAutoStart();
    }
#endif

    if (aEvents.ContainsAny(kEventIp6AddressAdded | kEventIp6AddressRemoved | kEventThreadMeshLocalAddrChanged) &&
        ShouldUpdateHostAutoAddresses())
    {
        IgnoreError(UpdateHostInfoStateOnAddressChange());
        UpdateState();
    }
}

void Client::HandleRoleChanged(void)
{
    if (Get<Mle::Mle>().IsAttached())
    {
#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
        ApplyAutoStartGuardOnAttach();
#endif

        VerifyOrExit(GetState() == kStatePaused);
        Resume();
    }
    else
    {
        VerifyOrExit(GetState() != kStateStopped);
        Pause();
    }

exit:
    return;
}

#if OPENTHREAD_CONFIG_SRP_CLIENT_DOMAIN_NAME_API_ENABLE
Error Client::SetDomainName(const char *aName)
{
    Error error = kErrorNone;

    VerifyOrExit((mHostInfo.GetState() == kToAdd) || (mHostInfo.GetState() == kRemoved), error = kErrorInvalidState);

    mDomainName = (aName != nullptr) ? aName : kDefaultDomainName;
    LogInfo("Domain name \"%s\"", mDomainName);

exit:
    return error;
}
#endif

Error Client::SetHostName(const char *aName)
{
    Error error = kErrorNone;

    VerifyOrExit(aName != nullptr, error = kErrorInvalidArgs);

    VerifyOrExit((mHostInfo.GetState() == kToAdd) || (mHostInfo.GetState() == kRemoved), error = kErrorInvalidState);

    LogInfo("Host name \"%s\"", aName);
    mHostInfo.SetName(aName);
    mHostInfo.SetState(kToAdd);
    UpdateState();

exit:
    return error;
}

Error Client::EnableAutoHostAddress(void)
{
    Error error = kErrorNone;

    VerifyOrExit(!mHostInfo.IsAutoAddressEnabled());
    SuccessOrExit(error = UpdateHostInfoStateOnAddressChange());

    for (Ip6::Netif::UnicastAddress &unicastAddress : Get<ThreadNetif>().GetUnicastAddresses())
    {
        unicastAddress.mSrpRegistered = false;
    }

    mAutoHostAddressCount = 0;

    mHostInfo.EnableAutoAddress();
    UpdateState();

exit:
    return error;
}

Error Client::SetHostAddresses(const Ip6::Address *aAddresses, uint8_t aNumAddresses)
{
    Error error = kErrorNone;

    VerifyOrExit((aAddresses != nullptr) && (aNumAddresses > 0), error = kErrorInvalidArgs);
    SuccessOrExit(error = UpdateHostInfoStateOnAddressChange());

    mHostInfo.SetAddresses(aAddresses, aNumAddresses);
    UpdateState();

exit:
    return error;
}

void Client::HandleUnicastAddressEvent(Ip6::Netif::AddressEvent aEvent, const Ip6::Netif::UnicastAddress &aAddress)
{
    // This callback from `Netif` signals an impending addition or
    // removal of a unicast address, occurring before `Notifier`
    // events. If `AutoAddress` is enabled, we check whether the
    // address origin is SLAAC (e.g., an OMR address) and request a
    // longer `TxJitter`. This helps randomize the next SRP
    // update transmission time when triggered by an OMR prefix
    // change.

    VerifyOrExit(IsRunning());
    VerifyOrExit(mHostInfo.IsAutoAddressEnabled());

    VerifyOrExit(aAddress.GetOrigin() == Ip6::Netif::kOriginSlaac);

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
    // The `mGuardTimer`, started by `ApplyAutoStartGuardOnAttach()`,
    // tracks a guard interval after the attach event. If an
    // address change occurs within this short window, we do not
    // apply a longer TX jitter, as this likely indicates a device
    // reboot.
    VerifyOrExit(!mGuardTimer.IsRunning());
#endif

    mTxJitter.Request((aEvent == Ip6::Netif::kAddressAdded) ? TxJitter::kOnSlaacAddrAdd : TxJitter::kOnSlaacAddrRemove);

exit:
    return;
}

bool Client::ShouldUpdateHostAutoAddresses(void) const
{
    // Determine if any changes to the addresses on `ThreadNetif`
    // require registration with the server when `AutoHostAddress` is
    // enabled. This includes registering all preferred addresses,
    // excluding link-local and mesh-local addresses. If no eligible
    // address is available, the ML-EID will be registered.

    bool                        shouldUpdate    = false;
    uint16_t                    registeredCount = 0;
    Ip6::Netif::UnicastAddress &mlEid           = Get<Mle::Mle>().GetMeshLocalEidUnicastAddress();

    VerifyOrExit(mHostInfo.IsAutoAddressEnabled());

    // We iterate through all eligible addresses on the `ThreadNetif`.
    // If we encounter a new address that should be registered but
    // isn't, or a previously registered address has been removed, we
    // trigger an SRP update to reflect these changes. However, if a
    // previously registered address is being deprecated (e.g., due
    // to an OMR prefix removal from Network Data), we defer the SRP
    // update. The client will re-register after the deprecation
    // time has elapsed and the address is removed. In the meantime,
    // if any other event triggers the client to send an SRP update,
    // the updated address list will be included in that update.

    for (const Ip6::Netif::UnicastAddress &unicastAddress : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (&unicastAddress == &mlEid)
        {
            continue;
        }

        if (ShouldHostAutoAddressRegister(unicastAddress) != unicastAddress.mSrpRegistered)
        {
            // If this address was previously registered but is no
            // longer eligible, we skip sending an immediate update
            // only if the address is currently being deprecated
            // (it's still valid but no longer preferred).

            bool skip = unicastAddress.mSrpRegistered && unicastAddress.mValid && !unicastAddress.mPreferred;

            if (!skip)
            {
                ExitNow(shouldUpdate = true);
            }
        }

        if (unicastAddress.mSrpRegistered)
        {
            registeredCount++;
        }
    }

    if (registeredCount == 0)
    {
        ExitNow(shouldUpdate = !mlEid.mSrpRegistered);
    }

    // Compare the current number of addresses that are marked as
    // registered with the previous value `mAutoHostAddressCount`.
    // This check handles the case where a previously registered address
    // has been removed.

    shouldUpdate = (registeredCount != mAutoHostAddressCount);

exit:
    return shouldUpdate;
}

bool Client::ShouldHostAutoAddressRegister(const Ip6::Netif::UnicastAddress &aUnicastAddress) const
{
    bool shouldRegister = false;

    VerifyOrExit(aUnicastAddress.mValid);
    VerifyOrExit(aUnicastAddress.mPreferred);
    VerifyOrExit(!aUnicastAddress.GetAddress().IsLinkLocalUnicast());
    VerifyOrExit(!Get<Mle::Mle>().IsMeshLocalAddress(aUnicastAddress.GetAddress()));

    shouldRegister = true;

exit:
    return shouldRegister;
}

Error Client::UpdateHostInfoStateOnAddressChange(void)
{
    Error error = kErrorNone;

    VerifyOrExit((mHostInfo.GetState() != kToRemove) && (mHostInfo.GetState() != kRemoving),
                 error = kErrorInvalidState);

    if (mHostInfo.GetState() == kRemoved)
    {
        mHostInfo.SetState(kToAdd);
    }
    else if (mHostInfo.GetState() != kToAdd)
    {
        mHostInfo.SetState(kToRefresh);
    }

exit:
    return error;
}

Error Client::AddService(Service &aService)
{
    Error error;

    VerifyOrExit(mServices.FindMatching(aService) == nullptr, error = kErrorAlready);

    SuccessOrExit(error = aService.Init());
    mServices.Push(aService);

    aService.SetState(kToAdd);
    UpdateState();

exit:
    return error;
}

Error Client::RemoveService(Service &aService)
{
    Error               error = kErrorNone;
    LinkedList<Service> removedServices;

    VerifyOrExit(mServices.Contains(aService), error = kErrorNotFound);

    UpdateServiceStateToRemove(aService);
    UpdateState();

exit:
    return error;
}

void Client::UpdateServiceStateToRemove(Service &aService)
{
    if (aService.GetState() != kRemoving)
    {
        aService.SetState(kToRemove);
    }
}

Error Client::ClearService(Service &aService)
{
    Error error;

    SuccessOrExit(error = mServices.Remove(aService));
    aService.SetNext(nullptr);
    aService.SetState(kRemoved);
    UpdateState();

exit:
    return error;
}

Error Client::RemoveHostAndServices(bool aShouldRemoveKeyLease, bool aSendUnregToServer)
{
    Error error = kErrorNone;

    LogInfo("Remove host & services");

    VerifyOrExit(mHostInfo.GetState() != kRemoved, error = kErrorAlready);

    if ((mHostInfo.GetState() == kToRemove) || (mHostInfo.GetState() == kRemoving))
    {
        // Host info remove is already ongoing, if "key lease" remove mode is
        // the same, there is no need to send a new update message.
        VerifyOrExit(mShouldRemoveKeyLease != aShouldRemoveKeyLease);
    }

    mShouldRemoveKeyLease = aShouldRemoveKeyLease;

    for (Service &service : mServices)
    {
        UpdateServiceStateToRemove(service);
    }

    if ((mHostInfo.GetState() == kToAdd) && !aSendUnregToServer)
    {
        // Host info is not added yet (not yet registered with
        // server), so we can remove it and all services immediately.
        mHostInfo.SetState(kRemoved);
        HandleUpdateDone();
        ExitNow();
    }

    mHostInfo.SetState(kToRemove);
    UpdateState();

exit:
    return error;
}

void Client::ClearHostAndServices(void)
{
    LogInfo("Clear host & services");

    switch (GetState())
    {
    case kStateStopped:
    case kStatePaused:
        break;

    case kStateToUpdate:
    case kStateUpdating:
    case kStateUpdated:
    case kStateToRetry:
        SetState(kStateUpdated);
        break;
    }

    mTxFailureRetryCount = 0;
    ResetRetryWaitInterval();

    mServices.Clear();
    mHostInfo.Clear();
}

void Client::SetState(State aState)
{
    VerifyOrExit(aState != mState);

    LogInfo("State %s -> %s", StateToString(mState), StateToString(aState));
    mState = aState;

    switch (mState)
    {
    case kStateStopped:
    case kStatePaused:
    case kStateUpdated:
        mTimer.Stop();
        break;

    case kStateToUpdate:
        mTimer.Start(mTxJitter.DetermineDelay());
        break;

    case kStateUpdating:
        mTimer.Start(GetRetryWaitInterval());
        break;

    case kStateToRetry:
        break;
    }
exit:
    return;
}

bool Client::ChangeHostAndServiceStates(const ItemState *aNewStates, ServiceStateChangeMode aMode)
{
    bool anyChanged;

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE && OPENTHREAD_CONFIG_SRP_CLIENT_SAVE_SELECTED_SERVER_ENABLE
    ItemState oldHostState = mHostInfo.GetState();
#endif

    anyChanged = mHostInfo.SetState(aNewStates[mHostInfo.GetState()]);

    for (Service &service : mServices)
    {
        if ((aMode == kForServicesAppendedInMessage) && !service.IsAppendedInMessage())
        {
            continue;
        }

        anyChanged |= service.SetState(aNewStates[service.GetState()]);
    }

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE && OPENTHREAD_CONFIG_SRP_CLIENT_SAVE_SELECTED_SERVER_ENABLE
    if ((oldHostState != kRegistered) && (mHostInfo.GetState() == kRegistered))
    {
        Settings::SrpClientInfo info;

        switch (mAutoStart.GetState())
        {
        case AutoStart::kDisabled:
        case AutoStart::kFirstTimeSelecting:
        case AutoStart::kReselecting:
            break;

        case AutoStart::kSelectedUnicastPreferred:
        case AutoStart::kSelectedUnicast:
            info.SetServerAddress(GetServerAddress().GetAddress());
            info.SetServerPort(GetServerAddress().GetPort());
            IgnoreError(Get<Settings>().Save(info));
            break;

        case AutoStart::kSelectedAnycast:
            IgnoreError(Get<Settings>().Delete<Settings::SrpClientInfo>());
            break;
        }
    }
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE && OPENTHREAD_CONFIG_SRP_CLIENT_SAVE_SELECTED_SERVER_ENABLE

    return anyChanged;
}

void Client::InvokeCallback(Error aError) const { InvokeCallback(aError, mHostInfo, nullptr); }

void Client::InvokeCallback(Error aError, const HostInfo &aHostInfo, const Service *aRemovedServices) const
{
    mCallback.InvokeIfSet(aError, &aHostInfo, mServices.GetHead(), aRemovedServices);
}

void Client::SendUpdate(void)
{
    static const ItemState kNewStateOnMessageTx[]{
        /* (0) kToAdd      -> */ kAdding,
        /* (1) kAdding     -> */ kAdding,
        /* (2) kToRefresh  -> */ kRefreshing,
        /* (3) kRefreshing -> */ kRefreshing,
        /* (4) kToRemove   -> */ kRemoving,
        /* (5) kRemoving   -> */ kRemoving,
        /* (6) kRegistered -> */ kRegistered,
        /* (7) kRemoved    -> */ kRemoved,
    };

    Error    error = kErrorNone;
    MsgInfo  info;
    uint32_t length;
    bool     anyChanged;

    info.mMessage.Reset(mSocket.NewMessage());
    VerifyOrExit(info.mMessage != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = PrepareUpdateMessage(info));

    length = info.mMessage->GetLength() + sizeof(Ip6::Udp::Header) + sizeof(Ip6::Header);

    if (length >= Ip6::kMaxDatagramLength)
    {
        LogInfo("Msg len %lu is larger than MTU, enabling single service mode", ToUlong(length));
        mSingleServiceMode = true;
        IgnoreError(info.mMessage->SetLength(0));
        SuccessOrExit(error = PrepareUpdateMessage(info));
    }

    SuccessOrExit(error = mSocket.SendTo(*info.mMessage, Ip6::MessageInfo()));

    // Ownership of the message is transferred to the socket upon a
    // successful `SendTo()` call.

    info.mMessage.Release();

    LogInfo("Send update, msg-id:0x%x", mNextMessageId);

    // State changes:
    //   kToAdd     -> kAdding
    //   kToRefresh -> kRefreshing
    //   kToRemove  -> kRemoving

    anyChanged = ChangeHostAndServiceStates(kNewStateOnMessageTx, kForServicesAppendedInMessage);

    // `mNextMessageId` tracks the message ID used in the prepared
    // update message. It is incremented after a successful
    // `mSocket.SendTo()` call. If unsuccessful, the same ID can be
    // reused for the next update.
    //
    // Acceptable response message IDs fall within the range starting
    // at `mResponseMessageId ` and ending before `mNextMessageId`.
    //
    // `anyChanged` tracks if any host or service states have changed.
    // If not, the prepared message is identical to the last one with
    // the same hosts/services, allowing us to accept earlier message
    // IDs. If changes occur, `mResponseMessageId ` is updated to
    // ensure only responses to the latest message are accepted.

    if (anyChanged)
    {
        mResponseMessageId = mNextMessageId;
    }

    mNextMessageId++;

    // Remember the update message tx time to use later to determine the
    // lease renew time.
    mLeaseRenewTime      = TimerMilli::GetNow();
    mTxFailureRetryCount = 0;

    SetState(kStateUpdating);

    if (!Get<Mle::Mle>().IsRxOnWhenIdle())
    {
        // If device is sleepy send fast polls while waiting for
        // the response from server.
        Get<DataPollSender>().SendFastPolls(kFastPollsAfterUpdateTx);
    }

exit:
    if (error != kErrorNone)
    {
        // If there is an error in preparation or transmission of the
        // update message (e.g., no buffer to allocate message), up to
        // `kMaxTxFailureRetries` times, we wait for a short interval
        // `kTxFailureRetryInterval` and try again. After this, we
        // continue to retry using the `mRetryWaitInterval` (which keeps
        // growing on each failure).

        LogInfo("Failed to send update: %s", ErrorToString(error));

        mSingleServiceMode = false;

        SetState(kStateToRetry);

        if (mTxFailureRetryCount < kMaxTxFailureRetries)
        {
            uint32_t interval;

            mTxFailureRetryCount++;
            interval = Random::NonCrypto::AddJitter(kTxFailureRetryInterval, kTxFailureRetryJitter);
            mTimer.Start(interval);

            LogInfo("Quick retry %u in %lu msec", mTxFailureRetryCount, ToUlong(interval));

            // Do not report message preparation errors to user
            // until `kMaxTxFailureRetries` are exhausted.
        }
        else
        {
            LogRetryWaitInterval();
            mTimer.Start(Random::NonCrypto::AddJitter(GetRetryWaitInterval(), kRetryIntervalJitter));
            GrowRetryWaitInterval();
            InvokeCallback(error);
        }
    }
}

Error Client::PrepareUpdateMessage(MsgInfo &aInfo)
{
    constexpr uint16_t kHeaderOffset = 0;

    Error             error = kErrorNone;
    Dns::UpdateHeader header;

    aInfo.mDomainNameOffset = MsgInfo::kUnknownOffset;
    aInfo.mHostNameOffset   = MsgInfo::kUnknownOffset;
    aInfo.mRecordCount      = 0;

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    aInfo.mKeyInfo.SetKeyRef(kSrpEcdsaKeyRef);
#endif

    SuccessOrExit(error = ReadOrGenerateKey(aInfo.mKeyInfo));

    header.SetMessageId(mNextMessageId);

    // SRP Update (DNS Update) message must have exactly one record in
    // Zone section, no records in Prerequisite Section, can have
    // multiple records in Update Section (tracked as they are added),
    // and two records in Additional Data Section (OPT and SIG records).
    // The SIG record itself should not be included in calculation of
    // SIG(0) signature, so the addition record count is set to one
    // here. After signature calculation and appending of SIG record,
    // the additional record count is updated to two and the header is
    // rewritten in the message.

    header.SetZoneRecordCount(1);
    header.SetAdditionalRecordCount(1);
    SuccessOrExit(error = aInfo.mMessage->Append(header));

    // Prepare Zone section

    aInfo.mDomainNameOffset = aInfo.mMessage->GetLength();
    SuccessOrExit(error = Dns::Name::AppendName(mDomainName, *aInfo.mMessage));
    SuccessOrExit(error = aInfo.mMessage->Append(Dns::Zone()));

    // Prepare Update section

    SuccessOrExit(error = AppendServiceInstructions(aInfo));
    SuccessOrExit(error = AppendHostDescriptionInstruction(aInfo));

    header.SetUpdateRecordCount(aInfo.mRecordCount);
    aInfo.mMessage->Write(kHeaderOffset, header);

    // Prepare Additional Data section

    SuccessOrExit(error = AppendUpdateLeaseOptRecord(aInfo));
    SuccessOrExit(error = AppendSignature(aInfo));

    header.SetAdditionalRecordCount(2); // Lease OPT and SIG RRs
    aInfo.mMessage->Write(kHeaderOffset, header);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
Error Client::ReadOrGenerateKey(KeyInfo &aKeyInfo)
{
    Error                        error = kErrorNone;
    Crypto::Ecdsa::P256::KeyPair keyPair;

    VerifyOrExit(!Crypto::Storage::HasKey(aKeyInfo.GetKeyRef()));
    error = Get<Settings>().Read<Settings::SrpEcdsaKey>(keyPair);

    if (error == kErrorNone)
    {
        if (aKeyInfo.ImportKeyPair(keyPair) != kErrorNone)
        {
            SuccessOrExit(error = aKeyInfo.Generate());
        }
        IgnoreError(Get<Settings>().Delete<Settings::SrpEcdsaKey>());
    }
    else
    {
        SuccessOrExit(error = aKeyInfo.Generate());
    }
exit:
    return error;
}
#else
Error Client::ReadOrGenerateKey(KeyInfo &aKeyInfo)
{
    Error error;

    error = Get<Settings>().Read<Settings::SrpEcdsaKey>(aKeyInfo);

    if (error == kErrorNone)
    {
        Crypto::Ecdsa::P256::PublicKey publicKey;

        if (aKeyInfo.GetPublicKey(publicKey) == kErrorNone)
        {
            ExitNow();
        }
    }

    SuccessOrExit(error = aKeyInfo.Generate());
    IgnoreError(Get<Settings>().Save<Settings::SrpEcdsaKey>(aKeyInfo));

exit:
    return error;
}
#endif //  OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE

Error Client::AppendServiceInstructions(MsgInfo &aInfo)
{
    Error error = kErrorNone;

    if ((mHostInfo.GetState() == kToRemove) || (mHostInfo.GetState() == kRemoving))
    {
        // When host is being removed, there is no need to include
        // services in the message (server is expected to remove any
        // previously registered services by this client). However, we
        // still mark all services as if they are appended in the message
        // so to ensure to update their state after sending the message.

        for (Service &service : mServices)
        {
            service.MarkAsAppendedInMessage();
        }

        mLease    = 0;
        mKeyLease = mShouldRemoveKeyLease ? 0 : mDefaultKeyLease;
        ExitNow();
    }

    mLease    = kUnspecifiedInterval;
    mKeyLease = kUnspecifiedInterval;

    // We first go through all services which are being updated (in any
    // of `...ing` states) and determine the lease and key lease intervals
    // associated with them. By the end of the loop either of `mLease` or
    // `mKeyLease` may be set or may still remain `kUnspecifiedInterval`.

    for (Service &service : mServices)
    {
        uint32_t lease    = DetermineLeaseInterval(service.GetLease(), mDefaultLease);
        uint32_t keyLease = Max(DetermineLeaseInterval(service.GetKeyLease(), mDefaultKeyLease), lease);

        service.ClearAppendedInMessageFlag();

        switch (service.GetState())
        {
        case kAdding:
        case kRefreshing:
            OT_ASSERT((mLease == kUnspecifiedInterval) || (mLease == lease));
            mLease = lease;

            OT_FALL_THROUGH;

        case kRemoving:
            OT_ASSERT((mKeyLease == kUnspecifiedInterval) || (mKeyLease == keyLease));
            mKeyLease = keyLease;
            break;

        case kToAdd:
        case kToRefresh:
        case kToRemove:
        case kRegistered:
        case kRemoved:
            break;
        }
    }

    // We go through all services again and append the services that
    // match the selected `mLease` and `mKeyLease`. If the lease intervals
    // are not yet set, the first appended service will determine them.

    for (Service &service : mServices)
    {
        // Skip over services that are already registered in this loop.
        // They may be added from the loop below once the lease intervals
        // are determined.

        if ((service.GetState() != kRegistered) && CanAppendService(service))
        {
            SuccessOrExit(error = AppendServiceInstruction(service, aInfo));

            if (mSingleServiceMode)
            {
                // In "single service mode", we allow only one service
                // to be appended in the message.
                break;
            }
        }
    }

    if (!mSingleServiceMode)
    {
        for (Service &service : mServices)
        {
            if ((service.GetState() == kRegistered) && CanAppendService(service) && ShouldRenewEarly(service))
            {
                // If the lease needs to be renewed or if we are close to the
                // renewal time of a registered service, we refresh the service
                // early and include it in this update. This helps put more
                // services on the same lease refresh schedule.

                service.SetState(kToRefresh);
                SuccessOrExit(error = AppendServiceInstruction(service, aInfo));
            }
        }
    }

    // `mLease` or `mKeylease` may be determined from the set of
    // services included in the message. If they are not yet set we
    // use the default intervals.

    mLease    = DetermineLeaseInterval(mLease, mDefaultLease);
    mKeyLease = DetermineLeaseInterval(mKeyLease, mDefaultKeyLease);

    // When message only contains removal of a previously registered
    // service, then `mKeyLease` is set but `mLease` remains unspecified.
    // In such a case, we end up using `mDefaultLease` but then we need
    // to make sure it is not greater than the selected `mKeyLease`.

    mLease = Min(mLease, mKeyLease);

exit:
    return error;
}

bool Client::CanAppendService(const Service &aService)
{
    // Check the lease intervals associated with `aService` to see if
    // it can be included in this message. When removing a service,
    // only key lease interval should match. In all other cases, both
    // lease and key lease should match. The `mLease` and/or `mKeyLease`
    // may be updated if they were unspecified.

    bool     canAppend = false;
    uint32_t lease     = DetermineLeaseInterval(aService.GetLease(), mDefaultLease);
    uint32_t keyLease  = Max(DetermineLeaseInterval(aService.GetKeyLease(), mDefaultKeyLease), lease);

    switch (aService.GetState())
    {
    case kToAdd:
    case kAdding:
    case kToRefresh:
    case kRefreshing:
    case kRegistered:
        VerifyOrExit((mLease == kUnspecifiedInterval) || (mLease == lease));
        VerifyOrExit((mKeyLease == kUnspecifiedInterval) || (mKeyLease == keyLease));
        mLease    = lease;
        mKeyLease = keyLease;
        canAppend = true;
        break;

    case kToRemove:
    case kRemoving:
        VerifyOrExit((mKeyLease == kUnspecifiedInterval) || (mKeyLease == keyLease));
        mKeyLease = keyLease;
        canAppend = true;
        break;

    case kRemoved:
        break;
    }

exit:
    return canAppend;
}

Error Client::AppendServiceInstruction(Service &aService, MsgInfo &aInfo)
{
    Error               error    = kErrorNone;
    bool                removing = ((aService.GetState() == kToRemove) || (aService.GetState() == kRemoving));
    Dns::ResourceRecord rr;
    Dns::SrvRecord      srv;
    uint16_t            serviceNameOffset;
    uint16_t            instanceNameOffset;
    uint16_t            offset;

    aService.MarkAsAppendedInMessage();

    //----------------------------------
    // Service Discovery Instruction

    // PTR record

    // "service name labels" + (pointer to) domain name.
    serviceNameOffset = aInfo.mMessage->GetLength();
    SuccessOrExit(error = Dns::Name::AppendMultipleLabels(aService.GetName(), *aInfo.mMessage));
    SuccessOrExit(error = Dns::Name::AppendPointerLabel(aInfo.mDomainNameOffset, *aInfo.mMessage));

    // On remove, we use "Delete an RR from an RRSet" where class is set
    // to NONE and TTL to zero (RFC 2136 - section 2.5.4).

    rr.Init(Dns::ResourceRecord::kTypePtr, removing ? Dns::PtrRecord::kClassNone : Dns::PtrRecord::kClassInternet);
    rr.SetTtl(removing ? 0 : DetermineTtl());
    offset = aInfo.mMessage->GetLength();
    SuccessOrExit(error = aInfo.mMessage->Append(rr));

    // "Instance name" + (pointer to) service name.
    instanceNameOffset = aInfo.mMessage->GetLength();
    SuccessOrExit(error = Dns::Name::AppendLabel(aService.GetInstanceName(), *aInfo.mMessage));
    SuccessOrExit(error = Dns::Name::AppendPointerLabel(serviceNameOffset, *aInfo.mMessage));

    UpdateRecordLengthInMessage(rr, offset, *aInfo.mMessage);
    aInfo.mRecordCount++;

    if (aService.HasSubType() && !removing)
    {
        const char *subTypeLabel;
        uint16_t    subServiceNameOffset = 0;

        for (uint16_t index = 0; (subTypeLabel = aService.GetSubTypeLabelAt(index)) != nullptr; ++index)
        {
            // subtype label + "_sub" label + (pointer to) service name.

            SuccessOrExit(error = Dns::Name::AppendLabel(subTypeLabel, *aInfo.mMessage));

            if (index == 0)
            {
                subServiceNameOffset = aInfo.mMessage->GetLength();
                SuccessOrExit(error = Dns::Name::AppendLabel("_sub", *aInfo.mMessage));
                SuccessOrExit(error = Dns::Name::AppendPointerLabel(serviceNameOffset, *aInfo.mMessage));
            }
            else
            {
                SuccessOrExit(error = Dns::Name::AppendPointerLabel(subServiceNameOffset, *aInfo.mMessage));
            }

            // `rr` is already initialized as PTR.
            offset = aInfo.mMessage->GetLength();
            SuccessOrExit(error = aInfo.mMessage->Append(rr));

            SuccessOrExit(error = Dns::Name::AppendPointerLabel(instanceNameOffset, *aInfo.mMessage));
            UpdateRecordLengthInMessage(rr, offset, *aInfo.mMessage);
            aInfo.mRecordCount++;
        }
    }

    //----------------------------------
    // Service Description Instruction

    // "Delete all RRsets from a name" for Instance Name.

    SuccessOrExit(error = Dns::Name::AppendPointerLabel(instanceNameOffset, *aInfo.mMessage));
    SuccessOrExit(error = AppendDeleteAllRrsets(aInfo));
    aInfo.mRecordCount++;

    VerifyOrExit(!removing);

    // SRV RR

    SuccessOrExit(error = Dns::Name::AppendPointerLabel(instanceNameOffset, *aInfo.mMessage));
    srv.Init();
    srv.SetTtl(DetermineTtl());
    srv.SetPriority(aService.GetPriority());
    srv.SetWeight(aService.GetWeight());
    srv.SetPort(aService.GetPort());
    offset = aInfo.mMessage->GetLength();
    SuccessOrExit(error = aInfo.mMessage->Append(srv));
    SuccessOrExit(error = AppendHostName(aInfo));
    UpdateRecordLengthInMessage(srv, offset, *aInfo.mMessage);
    aInfo.mRecordCount++;

    // TXT RR

    SuccessOrExit(error = Dns::Name::AppendPointerLabel(instanceNameOffset, *aInfo.mMessage));
    rr.Init(Dns::ResourceRecord::kTypeTxt);
    offset = aInfo.mMessage->GetLength();
    SuccessOrExit(error = aInfo.mMessage->Append(rr));
    SuccessOrExit(
        error = Dns::TxtEntry::AppendEntries(aService.GetTxtEntries(), aService.GetNumTxtEntries(), *aInfo.mMessage));
    UpdateRecordLengthInMessage(rr, offset, *aInfo.mMessage);
    aInfo.mRecordCount++;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    if (mServiceKeyRecordEnabled)
    {
        // KEY RR is optional in "Service Description Instruction". It
        // is added here under `REFERENCE_DEVICE` config and is intended
        // for testing only.

        SuccessOrExit(error = Dns::Name::AppendPointerLabel(instanceNameOffset, *aInfo.mMessage));
        SuccessOrExit(error = AppendKeyRecord(aInfo));
    }
#endif

exit:
    return error;
}

Error Client::AppendHostDescriptionInstruction(MsgInfo &aInfo)
{
    Error error = kErrorNone;

    //----------------------------------
    // Host Description Instruction

    // "Delete all RRsets from a name" for Host Name.

    SuccessOrExit(error = AppendHostName(aInfo));
    SuccessOrExit(error = AppendDeleteAllRrsets(aInfo));
    aInfo.mRecordCount++;

    // AAAA RRs

    if (mHostInfo.IsAutoAddressEnabled())
    {
        // Append all preferred addresses on Thread netif excluding link-local
        // and mesh-local addresses. If no address is appended, we include
        // the mesh local EID.

        mAutoHostAddressCount = 0;

        for (Ip6::Netif::UnicastAddress &unicastAddress : Get<ThreadNetif>().GetUnicastAddresses())
        {
            if (ShouldHostAutoAddressRegister(unicastAddress))
            {
                SuccessOrExit(error = AppendAaaaRecord(unicastAddress.GetAddress(), aInfo));
                unicastAddress.mSrpRegistered = true;
                mAutoHostAddressCount++;
            }
            else
            {
                unicastAddress.mSrpRegistered = false;
            }
        }

        if (mAutoHostAddressCount == 0)
        {
            Ip6::Netif::UnicastAddress &mlEid = Get<Mle::Mle>().GetMeshLocalEidUnicastAddress();

            SuccessOrExit(error = AppendAaaaRecord(mlEid.GetAddress(), aInfo));
            mlEid.mSrpRegistered = true;
            mAutoHostAddressCount++;
        }
    }
    else
    {
        for (uint8_t index = 0; index < mHostInfo.GetNumAddresses(); index++)
        {
            SuccessOrExit(error = AppendAaaaRecord(mHostInfo.GetAddress(index), aInfo));
        }
    }

    // KEY RR

    SuccessOrExit(error = AppendHostName(aInfo));
    SuccessOrExit(error = AppendKeyRecord(aInfo));

exit:
    return error;
}

Error Client::AppendAaaaRecord(const Ip6::Address &aAddress, MsgInfo &aInfo) const
{
    Error               error;
    Dns::ResourceRecord rr;

    rr.Init(Dns::ResourceRecord::kTypeAaaa);
    rr.SetTtl(DetermineTtl());
    rr.SetLength(sizeof(Ip6::Address));

    SuccessOrExit(error = AppendHostName(aInfo));
    SuccessOrExit(error = aInfo.mMessage->Append(rr));
    SuccessOrExit(error = aInfo.mMessage->Append(aAddress));
    aInfo.mRecordCount++;

exit:
    return error;
}

Error Client::AppendKeyRecord(MsgInfo &aInfo) const
{
    Error                          error;
    Dns::KeyRecord                 key;
    Crypto::Ecdsa::P256::PublicKey publicKey;

    key.Init();
    key.SetTtl(DetermineTtl());
    key.SetFlags(Dns::KeyRecord::kAuthConfidPermitted, Dns::KeyRecord::kOwnerNonZone,
                 Dns::KeyRecord::kSignatoryFlagGeneral);
    key.SetProtocol(Dns::KeyRecord::kProtocolDnsSec);
    key.SetAlgorithm(Dns::KeyRecord::kAlgorithmEcdsaP256Sha256);
    key.SetLength(sizeof(Dns::KeyRecord) - sizeof(Dns::ResourceRecord) + sizeof(Crypto::Ecdsa::P256::PublicKey));
    SuccessOrExit(error = aInfo.mMessage->Append(key));
    SuccessOrExit(error = aInfo.mKeyInfo.GetPublicKey(publicKey));
    SuccessOrExit(error = aInfo.mMessage->Append(publicKey));
    aInfo.mRecordCount++;

exit:
    return error;
}

Error Client::AppendDeleteAllRrsets(MsgInfo &aInfo) const
{
    // "Delete all RRsets from a name" (RFC 2136 - 2.5.3)
    // Name should be already appended in the message.

    Dns::ResourceRecord rr;

    rr.Init(Dns::ResourceRecord::kTypeAny, Dns::ResourceRecord::kClassAny);
    rr.SetTtl(0);
    rr.SetLength(0);

    return aInfo.mMessage->Append(rr);
}

Error Client::AppendHostName(MsgInfo &aInfo, bool aDoNotCompress) const
{
    Error error;

    if (aDoNotCompress)
    {
        // Uncompressed (canonical form) of host name is used for SIG(0)
        // calculation.
        SuccessOrExit(error = Dns::Name::AppendMultipleLabels(mHostInfo.GetName(), *aInfo.mMessage));
        error = Dns::Name::AppendName(mDomainName, *aInfo.mMessage);
        ExitNow();
    }

    // If host name was previously added in the message, add it
    // compressed as pointer to the previous one. Otherwise,
    // append it and remember the offset.

    if (aInfo.mHostNameOffset != MsgInfo::kUnknownOffset)
    {
        ExitNow(error = Dns::Name::AppendPointerLabel(aInfo.mHostNameOffset, *aInfo.mMessage));
    }

    aInfo.mHostNameOffset = aInfo.mMessage->GetLength();
    SuccessOrExit(error = Dns::Name::AppendMultipleLabels(mHostInfo.GetName(), *aInfo.mMessage));
    error = Dns::Name::AppendPointerLabel(aInfo.mDomainNameOffset, *aInfo.mMessage);

exit:
    return error;
}

Error Client::AppendUpdateLeaseOptRecord(MsgInfo &aInfo)
{
    Error            error;
    Dns::OptRecord   optRecord;
    Dns::LeaseOption leaseOption;
    uint16_t         optionSize;

    // Append empty (root domain) as OPT RR name.
    SuccessOrExit(error = Dns::Name::AppendTerminator(*aInfo.mMessage));

    // `Init()` sets the type and clears (set to zero) the extended
    // Response Code, version and all flags.
    optRecord.Init();
    optRecord.SetUdpPayloadSize(kUdpPayloadSize);
    optRecord.SetDnsSecurityFlag();

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    if (mUseShortLeaseOption)
    {
        LogInfo("Test mode - appending short variant of Lease Option");
        mKeyLease = mLease;
        leaseOption.InitAsShortVariant(mLease);
    }
    else
#endif
    {
        leaseOption.InitAsLongVariant(mLease, mKeyLease);
    }

    optionSize = static_cast<uint16_t>(leaseOption.GetSize());

    optRecord.SetLength(optionSize);

    SuccessOrExit(error = aInfo.mMessage->Append(optRecord));
    error = aInfo.mMessage->AppendBytes(&leaseOption, optionSize);

exit:
    return error;
}

Error Client::AppendSignature(MsgInfo &aInfo)
{
    Error                          error;
    Dns::SigRecord                 sig;
    Crypto::Sha256                 sha256;
    Crypto::Sha256::Hash           hash;
    Crypto::Ecdsa::P256::Signature signature;
    uint16_t                       offset;
    uint16_t                       len;

    // Prepare SIG RR: TTL, type covered, labels count should be set
    // to zero. Since we have no clock, inception and expiration time
    // are also set to zero. The RDATA length will be set later (not
    // yet known due to variably (and possible compression) of signer's
    // name.

    sig.Clear();
    sig.Init(Dns::ResourceRecord::kClassAny);
    sig.SetAlgorithm(Dns::KeyRecord::kAlgorithmEcdsaP256Sha256);

    // Append the SIG RR with full uncompressed form of the host name
    // as the signer's name. This is used for SIG(0) calculation only.
    // It will be overwritten with host name compressed.

    offset = aInfo.mMessage->GetLength();
    SuccessOrExit(error = aInfo.mMessage->Append(sig));
    SuccessOrExit(error = AppendHostName(aInfo, /* aDoNotCompress */ true));

    // Calculate signature (RFC 2931): Calculated over "data" which is
    // concatenation of (1) the SIG RR RDATA wire format (including
    // the canonical form of the signer's name), entirely omitting the
    // signature subfield, (2) DNS query message, including DNS header
    // but not UDP/IP header before the header RR counts have been
    // adjusted for the inclusion of SIG(0).

    sha256.Start();

    // (1) SIG RR RDATA wire format
    len = aInfo.mMessage->GetLength() - offset - sizeof(Dns::ResourceRecord);
    sha256.Update(*aInfo.mMessage, offset + sizeof(Dns::ResourceRecord), len);

    // (2) Message from DNS header before SIG
    sha256.Update(*aInfo.mMessage, 0, offset);

    sha256.Finish(hash);
    SuccessOrExit(error = aInfo.mKeyInfo.Sign(hash, signature));

    // Move back in message and append SIG RR now with compressed host
    // name (as signer's name) along with the calculated signature.

    IgnoreError(aInfo.mMessage->SetLength(offset));

    // SIG(0) uses owner name of root (single zero byte).
    SuccessOrExit(error = Dns::Name::AppendTerminator(*aInfo.mMessage));

    offset = aInfo.mMessage->GetLength();
    SuccessOrExit(error = aInfo.mMessage->Append(sig));
    SuccessOrExit(error = AppendHostName(aInfo));
    SuccessOrExit(error = aInfo.mMessage->Append(signature));
    UpdateRecordLengthInMessage(sig, offset, *aInfo.mMessage);

exit:
    return error;
}

void Client::UpdateRecordLengthInMessage(Dns::ResourceRecord &aRecord, uint16_t aOffset, Message &aMessage) const
{
    // This method is used to calculate an RR DATA length and update
    // (rewrite) it in a message. This should be called immediately
    // after all the fields in the record are written in the message.
    // `aOffset` gives the offset in the message to the start of the
    // record.

    aRecord.SetLength(aMessage.GetLength() - aOffset - sizeof(Dns::ResourceRecord));
    aMessage.Write(aOffset, aRecord);
}

void Client::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    ProcessResponse(aMessage);
}

void Client::ProcessResponse(Message &aMessage)
{
    static const ItemState kNewStateOnUpdateDone[]{
        /* (0) kToAdd      -> */ kToAdd,
        /* (1) kAdding     -> */ kRegistered,
        /* (2) kToRefresh  -> */ kToRefresh,
        /* (3) kRefreshing -> */ kRegistered,
        /* (4) kToRemove   -> */ kToRemove,
        /* (5) kRemoving   -> */ kRemoved,
        /* (6) kRegistered -> */ kRegistered,
        /* (7) kRemoved    -> */ kRemoved,
    };

    Error               error = kErrorNone;
    Dns::UpdateHeader   header;
    uint16_t            offset = aMessage.GetOffset();
    uint16_t            recordCount;
    LinkedList<Service> removedServices;

    switch (GetState())
    {
    case kStateToUpdate:
    case kStateUpdating:
    case kStateToRetry:
        break;
    case kStateStopped:
    case kStatePaused:
    case kStateUpdated:
        ExitNow();
    }

    SuccessOrExit(error = aMessage.Read(offset, header));

    VerifyOrExit(header.GetType() == Dns::Header::kTypeResponse, error = kErrorParse);
    VerifyOrExit(header.GetQueryType() == Dns::Header::kQueryTypeUpdate, error = kErrorParse);

    VerifyOrExit(IsResponseMessageIdValid(header.GetMessageId()), error = kErrorDrop);
    mResponseMessageId = header.GetMessageId() + 1;

    if (!Get<Mle::Mle>().IsRxOnWhenIdle())
    {
        Get<DataPollSender>().StopFastPolls();
    }

    LogInfo("Received response, msg-id:0x%x", header.GetMessageId());

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE && OPENTHREAD_CONFIG_SRP_CLIENT_SWITCH_SERVER_ON_FAILURE
    mAutoStart.ResetTimeoutFailureCount();
#endif

    error = Dns::Header::ResponseCodeToError(header.GetResponseCode());

    if (error != kErrorNone)
    {
        LogInfo("Server rejected %s code:%d", ErrorToString(error), header.GetResponseCode());

        if (mHostInfo.GetState() == kAdding)
        {
            // Since server rejected the update message, we go back to
            // `kToAdd` state to allow user to give a new name using
            // `SetHostName()`.
            mHostInfo.SetState(kToAdd);
        }

        // Wait for the timer to expire to retry. Note that timer is
        // already scheduled for the current wait interval when state
        // was changed to `kStateUpdating`.

        LogRetryWaitInterval();
        GrowRetryWaitInterval();
        SetState(kStateToRetry);
        InvokeCallback(error);

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE && OPENTHREAD_CONFIG_SRP_CLIENT_SWITCH_SERVER_ON_FAILURE
        if ((error == kErrorDuplicated) || (error == kErrorSecurity))
        {
            // If the server rejects the update with specific errors
            // (indicating duplicate name and/or security error), we
            // try to switch the server (we check if another can be
            // found in the Network Data).
            //
            // Note that this is done after invoking the callback and
            // notifying the user of the error from server. This works
            // correctly even if user makes changes from callback
            // (e.g., calls SRP client APIs like `Stop` or disables
            // auto-start), since we have a guard check at the top of
            // `SelectNextServer()` to verify that client is still
            // running and auto-start is enabled and selected the
            // server.

            SelectNextServer(/* aDisallowSwitchOnRegisteredHost */ true);
        }
#endif
        ExitNow(error = kErrorNone);
    }

    offset += sizeof(header);

    // Skip over all sections till Additional Data section
    // SPEC ENHANCEMENT: Server can echo the request back or not
    // include any of RRs. Would be good to explicitly require SRP server
    // to not echo back RRs.

    if (header.GetZoneRecordCount() != 0)
    {
        VerifyOrExit(header.GetZoneRecordCount() == 1, error = kErrorParse);
        SuccessOrExit(error = Dns::Name::ParseName(aMessage, offset));
        VerifyOrExit(offset + sizeof(Dns::Zone) <= aMessage.GetLength(), error = kErrorParse);
        offset += sizeof(Dns::Zone);
    }

    // Check for Update Lease OPT RR. This determines the lease
    // interval accepted by server. If not present, then use the
    // transmitted lease interval from the update request message.

    recordCount =
        header.GetPrerequisiteRecordCount() + header.GetUpdateRecordCount() + header.GetAdditionalRecordCount();

    while (recordCount > 0)
    {
        uint16_t            startOffset = offset;
        Dns::ResourceRecord rr;

        SuccessOrExit(error = ReadResourceRecord(aMessage, offset, rr));
        recordCount--;

        if (rr.GetType() == Dns::ResourceRecord::kTypeOpt)
        {
            SuccessOrExit(error = ProcessOptRecord(aMessage, startOffset, static_cast<Dns::OptRecord &>(rr)));
        }
    }

    // Calculate the lease renew time based on update message tx time
    // and the lease time. `kLeaseRenewGuardInterval` is used to
    // ensure that we renew the lease before server expires it. In the
    // unlikely (but maybe useful for testing) case where the accepted
    // lease interval is too short (shorter than twice the guard time)
    // we just use half of the accepted lease interval.

    if (mLease > 2 * kLeaseRenewGuardInterval)
    {
        uint32_t interval = Time::SecToMsec(mLease - kLeaseRenewGuardInterval);

        mLeaseRenewTime += Random::NonCrypto::AddJitter(interval, kLeaseRenewJitter);
    }
    else
    {
        mLeaseRenewTime += Time::SecToMsec(mLease) / 2;
    }

    for (Service &service : mServices)
    {
        if ((service.GetState() == kAdding) || (service.GetState() == kRefreshing))
        {
            service.SetLeaseRenewTime(mLeaseRenewTime);
        }
    }

    // State changes:
    //   kAdding     -> kRegistered
    //   kRefreshing -> kRegistered
    //   kRemoving   -> kRemoved

    ChangeHostAndServiceStates(kNewStateOnUpdateDone, kForServicesAppendedInMessage);

    HandleUpdateDone();
    UpdateState();

exit:
    if (error != kErrorNone)
    {
        LogInfo("Failed to process response %s", ErrorToString(error));
    }
}

bool Client::IsResponseMessageIdValid(uint16_t aId) const
{
    // Semantically equivalent to `(aId >= mResponseMessageId) && (aId < mNextMessageId)`

    return !SerialNumber::IsLess(aId, mResponseMessageId) && SerialNumber::IsLess(aId, mNextMessageId);
}

void Client::HandleUpdateDone(void)
{
    HostInfo            hostInfoCopy = mHostInfo;
    LinkedList<Service> removedServices;

    if (mHostInfo.GetState() == kRemoved)
    {
        mHostInfo.Clear();
    }

    ResetRetryWaitInterval();
    SetState(kStateUpdated);

    GetRemovedServices(removedServices);
    InvokeCallback(kErrorNone, hostInfoCopy, removedServices.GetHead());
}

void Client::GetRemovedServices(LinkedList<Service> &aRemovedServices)
{
    mServices.RemoveAllMatching(kRemoved, aRemovedServices);
}

Error Client::ReadResourceRecord(const Message &aMessage, uint16_t &aOffset, Dns::ResourceRecord &aRecord)
{
    // Reads and skips over a Resource Record (RR) from message at
    // given offset. On success, `aOffset` is updated to point to end
    // of RR.

    Error error;

    SuccessOrExit(error = Dns::Name::ParseName(aMessage, aOffset));
    SuccessOrExit(error = aMessage.Read(aOffset, aRecord));
    VerifyOrExit(aOffset + aRecord.GetSize() <= aMessage.GetLength(), error = kErrorParse);
    aOffset += static_cast<uint16_t>(aRecord.GetSize());

exit:
    return error;
}

Error Client::ProcessOptRecord(const Message &aMessage, uint16_t aOffset, const Dns::OptRecord &aOptRecord)
{
    // Read and process all options (in an OPT RR) from a message.
    // The `aOffset` points to beginning of record in `aMessage`.

    Error            error = kErrorNone;
    Dns::LeaseOption leaseOption;

    IgnoreError(Dns::Name::ParseName(aMessage, aOffset));
    aOffset += sizeof(Dns::OptRecord);

    switch (error = leaseOption.ReadFrom(aMessage, aOffset, aOptRecord.GetLength()))
    {
    case kErrorNone:
        mLease    = Min(leaseOption.GetLeaseInterval(), kMaxLease);
        mKeyLease = Min(leaseOption.GetKeyLeaseInterval(), kMaxLease);
        break;

    case kErrorNotFound:
        // If server does not include a lease option in its response, it
        // indicates that it accepted what we requested.
        error = kErrorNone;
        break;

    default:
        ExitNow();
    }

exit:
    return error;
}

void Client::UpdateState(void)
{
    NextFireTime nextRenewTime;
    bool         shouldUpdate = false;

    VerifyOrExit((GetState() != kStateStopped) && (GetState() != kStatePaused));
    VerifyOrExit(mHostInfo.GetName() != nullptr);

    // Go through the host info and all the services to check if there
    // are any new changes (i.e., anything new to add or remove). This
    // is used to determine whether to send an SRP update message or
    // not. Also keep track of the earliest renew time among the
    // previously registered services. This is used to schedule the
    // timer for next refresh.

    switch (mHostInfo.GetState())
    {
    case kAdding:
    case kRefreshing:
    case kRemoving:
        break;

    case kRegistered:
        if (nextRenewTime.GetNow() < mLeaseRenewTime)
        {
            break;
        }

        mHostInfo.SetState(kToRefresh);

        // Fall through

    case kToAdd:
    case kToRefresh:
        // Make sure we have at least one service and at least one
        // host address, otherwise no need to send SRP update message.
        // The exception is when removing host info where we allow
        // for empty service list.
        VerifyOrExit(!mServices.IsEmpty() && (mHostInfo.IsAutoAddressEnabled() || (mHostInfo.GetNumAddresses() > 0)));

        // Fall through

    case kToRemove:
        shouldUpdate = true;
        break;

    case kRemoved:
        ExitNow();
    }

    // If host info is being removed, we skip over checking service list
    // for new adds (or removes). This handles the situation where while
    // remove is ongoing and before we get a response from the server,
    // user adds a new service to be registered. We wait for remove to
    // finish (receive response from server) before starting with a new
    // service adds.

    if (mHostInfo.GetState() != kRemoving)
    {
        for (Service &service : mServices)
        {
            switch (service.GetState())
            {
            case kToAdd:
            case kToRefresh:
            case kToRemove:
                shouldUpdate = true;
                break;

            case kRegistered:
                if (service.GetLeaseRenewTime() <= nextRenewTime.GetNow())
                {
                    service.SetState(kToRefresh);
                    shouldUpdate = true;
                }
                else
                {
                    nextRenewTime.UpdateIfEarlier(service.GetLeaseRenewTime());
                }

                break;

            case kAdding:
            case kRefreshing:
            case kRemoving:
            case kRemoved:
                break;
            }
        }
    }

    if (shouldUpdate)
    {
        SetState(kStateToUpdate);
        ExitNow();
    }

    if (GetState() == kStateUpdated)
    {
        mTimer.FireAt(nextRenewTime);
    }

exit:
    return;
}

void Client::GrowRetryWaitInterval(void)
{
    mRetryWaitInterval =
        mRetryWaitInterval / kRetryIntervalGrowthFactorDenominator * kRetryIntervalGrowthFactorNumerator;
    mRetryWaitInterval = Min(mRetryWaitInterval, kMaxRetryWaitInterval);
}

uint32_t Client::DetermineLeaseInterval(uint32_t aInterval, uint32_t aDefaultInterval) const
{
    // Determine the lease or key lease interval.
    //
    // We use `aInterval` if it is non-zero, otherwise, use the
    // `aDefaultInterval`. We also ensure that the returned value is
    // never greater than `kMaxLease`. The `kMaxLease` is selected
    // such the lease intervals in msec can still fit in a `uint32_t`
    // `Time` variable (`kMaxLease` is ~ 24.8 days).

    return Min(kMaxLease, (aInterval != kUnspecifiedInterval) ? aInterval : aDefaultInterval);
}

uint32_t Client::DetermineTtl(void) const
{
    // Determine the TTL to use based on current `mLease`.
    // If `mLease == 0`, it indicates we are removing host
    // and so we use `mDefaultLease` instead.

    uint32_t lease = (mLease == 0) ? mDefaultLease : mLease;

    return (mTtl == kUnspecifiedInterval) ? lease : Min(mTtl, lease);
}

bool Client::ShouldRenewEarly(const Service &aService) const
{
    // Check if we reached the service renew time or close to it. The
    // "early renew interval" is used to allow early refresh. It is
    // calculated as a factor of the service requested lease interval.
    // The  "early lease renew factor" is given as a fraction (numerator
    // and denominator). If the denominator is set to zero (i.e., factor
    // is set to infinity), then service is always included in all SRP
    // update messages.

    bool shouldRenew;

#if OPENTHREAD_CONFIG_SRP_CLIENT_EARLY_LEASE_RENEW_FACTOR_DENOMINATOR != 0
    uint32_t earlyRenewInterval;

    earlyRenewInterval = Time::SecToMsec(DetermineLeaseInterval(aService.GetLease(), mDefaultLease));
    earlyRenewInterval = earlyRenewInterval / kEarlyLeaseRenewFactorDenominator * kEarlyLeaseRenewFactorNumerator;

    shouldRenew = (aService.GetLeaseRenewTime() <= TimerMilli::GetNow() + earlyRenewInterval);
#else
    OT_UNUSED_VARIABLE(aService);
    shouldRenew = true;
#endif

    return shouldRenew;
}

void Client::HandleTimer(void)
{
    switch (GetState())
    {
    case kStateStopped:
    case kStatePaused:
        break;

    case kStateToUpdate:
    case kStateToRetry:
        SendUpdate();
        break;

    case kStateUpdating:
        mSingleServiceMode = false;
        LogRetryWaitInterval();
        LogInfo("Timed out, no response");
        GrowRetryWaitInterval();
        SetState(kStateToUpdate);
        InvokeCallback(kErrorResponseTimeout);

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE && OPENTHREAD_CONFIG_SRP_CLIENT_SWITCH_SERVER_ON_FAILURE

        // After certain number of back-to-back timeout failures, we try
        // to switch the server. This is again done after invoking the
        // callback. It works correctly due to the guard check at the
        // top of `SelectNextServer()`.

        mAutoStart.IncrementTimeoutFailureCount();

        if (mAutoStart.GetTimeoutFailureCount() >= kMaxTimeoutFailuresToSwitchServer)
        {
            SelectNextServer(kDisallowSwitchOnRegisteredHost);
        }
#endif
        break;

    case kStateUpdated:
        UpdateState();
        break;
    }
}

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE

void Client::EnableAutoStartMode(AutoStartCallback aCallback, void *aContext)
{
    mAutoStart.SetCallback(aCallback, aContext);

    VerifyOrExit(mAutoStart.GetState() == AutoStart::kDisabled);

    mAutoStart.SetState(AutoStart::kFirstTimeSelecting);
    ApplyAutoStartGuardOnAttach();

    ProcessAutoStart();

exit:
    return;
}

void Client::ApplyAutoStartGuardOnAttach(void)
{
    VerifyOrExit(Get<Mle::Mle>().IsAttached());
    VerifyOrExit(!IsRunning());
    VerifyOrExit(mAutoStart.GetState() == AutoStart::kFirstTimeSelecting);

    // The `mGuardTimer` tracks a guard interval after the attach
    // event while `AutoStart` has yet to select a server for the
    // first time.
    //
    // This is used by `ProcessAutoStart()` to apply different TX
    // jitter values. If server selection occurs within this short
    // window, a shorter TX jitter is used. This typically represents
    // the device rebooting or being paired.
    //
    // The guard time is also checked when handling SLAAC address change
    // events, to decide whether or not to request longer TX jitter.

    mGuardTimer.Start(kGuardTimeAfterAttachToUseShorterTxJitter);

exit:
    return;
}

void Client::ProcessAutoStart(void)
{
    Ip6::SockAddr     serverSockAddr;
    DnsSrpAnycastInfo anycastInfo;
    DnsSrpUnicastInfo unicastInfo;
    AutoStart::State  oldAutoStartState = mAutoStart.GetState();
    bool              shouldRestart     = false;

    // If auto start mode is enabled, we check the Network Data entries
    // to discover and select the preferred SRP server to register with.
    // If we currently have a selected server, we ensure that it is
    // still present in the Network Data and is still the preferred one.

    VerifyOrExit(mAutoStart.GetState() != AutoStart::kDisabled);

    // If SRP client is running, we check to make sure that auto-start
    // did select the current server, and server was not specified by
    // user directly.

    if (IsRunning())
    {
        VerifyOrExit(mAutoStart.HasSelectedServer());
    }

    // There are three types of entries in Network Data:
    //
    // 1) Preferred unicast entries with address included in service data.
    // 2) Anycast entries (each having a seq number).
    // 3) Unicast entries with address info included in server data.

    serverSockAddr.Clear();

    if (SelectUnicastEntry(NetworkData::Service::kAddrInServiceData, unicastInfo) == kErrorNone)
    {
        mAutoStart.SetState(AutoStart::kSelectedUnicastPreferred);
        serverSockAddr = unicastInfo.mSockAddr;
    }
    else if (Get<NetworkData::Service::Manager>().FindPreferredDnsSrpAnycastInfo(anycastInfo) == kErrorNone)
    {
        serverSockAddr.SetAddress(anycastInfo.mAnycastAddress);
        serverSockAddr.SetPort(kAnycastServerPort);

        // We check if we are selecting an anycast entry for first
        // time, or if the seq number has changed. Even if the
        // anycast address remains the same as before, on a seq
        // number change, the client still needs to restart to
        // re-register its info.

        if ((mAutoStart.GetState() != AutoStart::kSelectedAnycast) ||
            (mAutoStart.GetAnycastSeqNum() != anycastInfo.mSequenceNumber))
        {
            shouldRestart = true;
            mAutoStart.SetAnycastSeqNum(anycastInfo.mSequenceNumber);
        }

        mAutoStart.SetState(AutoStart::kSelectedAnycast);
    }
    else if (SelectUnicastEntry(NetworkData::Service::kAddrInServerData, unicastInfo) == kErrorNone)
    {
        mAutoStart.SetState(AutoStart::kSelectedUnicast);
        serverSockAddr = unicastInfo.mSockAddr;
    }

    if (IsRunning())
    {
        VerifyOrExit((GetServerAddress() != serverSockAddr) || shouldRestart);
        Stop(kRequesterAuto, kResetRetryInterval);
    }

    if (serverSockAddr.GetAddress().IsUnspecified())
    {
        if (mAutoStart.HasSelectedServer())
        {
            mAutoStart.SetState(AutoStart::kReselecting);
        }

        ExitNow();
    }

    // Before calling `Start()`, determine the trigger reason for
    // starting the client with the newly discovered server based on
    // `AutoStart` state transitions. This reason is then used to
    // select the appropriate TX jitter interval (randomizing the
    // initial SRP update transmission to the new server).

    switch (oldAutoStartState)
    {
    case AutoStart::kDisabled:
        break;

    case AutoStart::kFirstTimeSelecting:

        // If the device is attaching to an established Thread mesh
        // (e.g., after a reboot or pairing), the Network Data it
        // receives should already include a server entry, leading to
        // a quick server selection after attachment. The `mGuardTimer`,
        // started by `ApplyAutoStartGuardOnAttach()`, tracks a guard
        // interval after the attach event. If server selection
        // occurs within this short window, a shorter TX jitter is
        // used (`TxJitter::kOnDeviceReboot`), allowing the device to
        // register quickly and become discoverable.
        //
        // If server discovery takes longer, a longer TX jitter
        // is used (`TxJitter::kOnServerStart`). This situation
        // can indicate a server/BR starting up or a network-wide
        // restart of many nodes (e.g., due to a power outage).

        if (mGuardTimer.IsRunning())
        {
            mTxJitter.Request(TxJitter::kOnDeviceReboot);
        }
        else
        {
            mTxJitter.Request(TxJitter::kOnServerStart);
        }

        break;

    case AutoStart::kReselecting:
        // Server is restarted (or possibly a new server started).
        mTxJitter.Request(TxJitter::kOnServerRestart);
        break;

    case AutoStart::kSelectedUnicastPreferred:
    case AutoStart::kSelectedAnycast:
    case AutoStart::kSelectedUnicast:
        mTxJitter.Request(TxJitter::kOnServerSwitch);
        break;
    }

    IgnoreError(Start(serverSockAddr, kRequesterAuto));

exit:
    return;
}

Error Client::SelectUnicastEntry(DnsSrpUnicastType aType, DnsSrpUnicastInfo &aInfo) const
{
    Error                                   error = kErrorNotFound;
    DnsSrpUnicastInfo                       unicastInfo;
    NetworkData::Service::Manager::Iterator iterator;
#if OPENTHREAD_CONFIG_SRP_CLIENT_SAVE_SELECTED_SERVER_ENABLE
    Settings::SrpClientInfo savedInfo;
    bool                    hasSavedServerInfo = false;

    if (!IsRunning())
    {
        hasSavedServerInfo = (Get<Settings>().Read(savedInfo) == kErrorNone);
    }
#endif

    while (Get<NetworkData::Service::Manager>().GetNextDnsSrpUnicastInfo(iterator, aType, unicastInfo) == kErrorNone)
    {
        if (mAutoStart.HasSelectedServer() && (GetServerAddress() == unicastInfo.mSockAddr))
        {
            aInfo = unicastInfo;
            error = kErrorNone;
            ExitNow();
        }

#if OPENTHREAD_CONFIG_SRP_CLIENT_SAVE_SELECTED_SERVER_ENABLE
        if (hasSavedServerInfo && (unicastInfo.mSockAddr.GetAddress() == savedInfo.GetServerAddress()) &&
            (unicastInfo.mSockAddr.GetPort() == savedInfo.GetServerPort()))
        {
            // Stop the search if we see a match for the previously
            // saved server info in the network data entries.

            aInfo = unicastInfo;
            error = kErrorNone;
            ExitNow();
        }
#endif

        // Prefer the numerically lowest server address

        if ((error == kErrorNotFound) || (unicastInfo.mSockAddr.GetAddress() < aInfo.mSockAddr.GetAddress()))
        {
            aInfo = unicastInfo;
            error = kErrorNone;
        }
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_SRP_CLIENT_SWITCH_SERVER_ON_FAILURE
void Client::SelectNextServer(bool aDisallowSwitchOnRegisteredHost)
{
    // This method tries to find the next unicast server info entry in the
    // Network Data after the current one selected. If found, it
    // restarts the client with the new server (keeping the retry wait
    // interval as before).

    Ip6::SockAddr     serverSockAddr;
    bool              selectNext = false;
    DnsSrpUnicastType type       = NetworkData::Service::kAddrInServiceData;

    serverSockAddr.Clear();

    // Ensure that client is running, auto-start is enabled and
    // auto-start selected the server and it is a unicast entry.

    VerifyOrExit(IsRunning());

    switch (mAutoStart.GetState())
    {
    case AutoStart::kSelectedUnicastPreferred:
        type = NetworkData::Service::kAddrInServiceData;
        break;

    case AutoStart::kSelectedUnicast:
        type = NetworkData::Service::kAddrInServerData;
        break;

    case AutoStart::kSelectedAnycast:
    case AutoStart::kDisabled:
    case AutoStart::kFirstTimeSelecting:
    case AutoStart::kReselecting:
        ExitNow();
    }

    if (aDisallowSwitchOnRegisteredHost)
    {
        // Ensure that host info is not yet registered (indicating that no
        // service has yet been registered either).
        VerifyOrExit((mHostInfo.GetState() == kAdding) || (mHostInfo.GetState() == kToAdd));
    }

    // We go through all entries to find the one matching the currently
    // selected one, then set `selectNext` to `true` so to select the
    // next one.

    do
    {
        DnsSrpUnicastInfo                       unicastInfo;
        NetworkData::Service::Manager::Iterator iterator;

        while (Get<NetworkData::Service::Manager>().GetNextDnsSrpUnicastInfo(iterator, type, unicastInfo) == kErrorNone)
        {
            if (selectNext)
            {
                serverSockAddr = unicastInfo.mSockAddr;
                ExitNow();
            }

            if (GetServerAddress() == unicastInfo.mSockAddr)
            {
                selectNext = true;
            }
        }

        // We loop back to handle the case where the current entry
        // is the last one.

    } while (selectNext);

    // If we reach here it indicates we could not find the entry
    // associated with currently selected server in the list. This
    // situation is rather unlikely but can still happen if Network
    // Data happens to be changed and the entry removed but
    // the "changed" event from `Notifier` may have not yet been
    // processed (note that events are emitted from their own
    // tasklet). In such a case we keep `serverSockAddr` as empty.

exit:
    if (!serverSockAddr.GetAddress().IsUnspecified() && (GetServerAddress() != serverSockAddr))
    {
        // We specifically update `mHostInfo` to `kToAdd` state. This
        // ensures that `Stop()` will keep it as kToAdd` and we detect
        // that the host info has not been registered yet and allow the
        // `SelectNextServer()` to happen again if the timeouts/failures
        // continue to happen with the new server.

        mHostInfo.SetState(kToAdd);
        Stop(kRequesterAuto, kKeepRetryInterval);
        IgnoreError(Start(serverSockAddr, kRequesterAuto));
    }
}
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_SWITCH_SERVER_ON_FAILURE

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE

const char *Client::ItemStateToString(ItemState aState)
{
    static const char *const kItemStateStrings[] = {
        "ToAdd",      // kToAdd      (0)
        "Adding",     // kAdding     (1)
        "ToRefresh",  // kToRefresh  (2)
        "Refreshing", // kRefreshing (3)
        "ToRemove",   // kToRemove   (4)
        "Removing",   // kRemoving   (5)
        "Registered", // kRegistered (6)
        "Removed",    // kRemoved    (7)
    };

    return kItemStateStrings[aState];
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *Client::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Stopped",  // kStateStopped  (0)
        "Paused",   // kStatePaused   (1)
        "ToUpdate", // kStateToUpdate (2)
        "Updating", // kStateUpdating (3)
        "Updated",  // kStateUpdated  (4)
        "ToRetry",  // kStateToRetry  (5)
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kStateStopped);
        ValidateNextEnum(kStatePaused);
        ValidateNextEnum(kStateToUpdate);
        ValidateNextEnum(kStateUpdating);
        ValidateNextEnum(kStateUpdated);
        ValidateNextEnum(kStateToRetry);
    };

    return kStateStrings[aState];
}

void Client::LogRetryWaitInterval(void) const
{
    constexpr uint16_t kLogInMsecLimit = 5000; // Max interval (in msec) to log the value in msec unit

    uint32_t interval = GetRetryWaitInterval();

    LogInfo("Retry interval %lu %s", ToUlong((interval < kLogInMsecLimit) ? interval : Time::MsecToSec(interval)),
            (interval < kLogInMsecLimit) ? "ms" : "sec");
}

#endif // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

} // namespace Srp
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
