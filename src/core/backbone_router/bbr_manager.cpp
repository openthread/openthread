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

/**
 * @file
 *   This file implements Backbone Router management.
 */

#include "bbr_manager.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#include "instance/instance.hpp"

namespace ot {

namespace BackboneRouter {

RegisterLogModule("BbrManager");

Manager::Manager(Instance &aInstance)
    : InstanceLocator(aInstance)
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    , mMulticastListenersTable(aInstance)
#endif
    , mTimer(aInstance)
    , mBackboneTmfAgent(aInstance)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE && OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    , mMlrResponseStatus(Mlr::kStatusSuccess)
    , mMlrResponseIsSpecified(false)
#endif
{
}

void Manager::HandleNotifierEvents(Events aEvents)
{
    Error error;

    if (aEvents.Contains(kEventThreadBackboneRouterStateChanged))
    {
        if (!Get<Local>().IsEnabled())
        {
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
            mMulticastListenersTable.Clear();
#endif
            mTimer.Stop();

            error = mBackboneTmfAgent.Stop();

            if (error != kErrorNone)
            {
                LogWarn("Stop Backbone TMF agent: %s", ErrorToString(error));
            }
            else
            {
                LogInfo("Stop Backbone TMF agent: %s", ErrorToString(error));
            }
        }
        else
        {
            if (!mTimer.IsRunning())
            {
                mTimer.Start(kTimerInterval);
            }

            error = mBackboneTmfAgent.Start();

            LogError("Start Backbone TMF agent", error);
        }
    }
}

void Manager::HandleTimer(void) { mTimer.Start(kTimerInterval); }

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
template <> void Manager::HandleTmf<kUriMlr>(Coap::Msg &aMsg)
{
    VerifyOrExit(Get<Local>().IsEnabled());
    HandleMulticastListenerRegistration(aMsg);

exit:
    return;
}

void Manager::HandleMulticastListenerRegistration(const Coap::Msg &aMsg)
{
    Error        error     = kErrorNone;
    bool         isPrimary = Get<Local>().IsPrimary();
    Mlr::Status  status    = Mlr::kStatusSuccess;
    OffsetRange  offsetRange;
    Ip6::Address address;
    Ip6::Address addresses[Mlr::kMaxIp6Addresses];
    uint8_t      failedAddressNum  = 0;
    uint8_t      successAddressNum = 0;
    TimeMilli    expireTime;
    uint32_t     timeout;
    uint16_t     commissionerSessionId;
    bool         hasCommissionerSessionIdTlv = false;
    bool         processTimeoutTlv           = false;

    VerifyOrExit(aMsg.IsConfirmable(), error = kErrorParse);

    VerifyOrExit(isPrimary, status = Mlr::kStatusBbrNotPrimary);

    VerifyOrExit(Tlv::FindTlvValueOffsetRange(aMsg.mMessage, Ip6AddressesTlv::kType, offsetRange) == kErrorNone,
                 error = kErrorParse);
    VerifyOrExit(offsetRange.GetLength() % sizeof(Ip6::Address) == 0, status = Mlr::kStatusGeneralFailure);
    VerifyOrExit(offsetRange.GetLength() / sizeof(Ip6::Address) <= Mlr::kMaxIp6Addresses,
                 status = Mlr::kStatusGeneralFailure);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    // Required by Test Specification 5.10.22 MATN-TC-26, only for certification purpose
    if (mMlrResponseIsSpecified)
    {
        mMlrResponseIsSpecified = false;
        status                  = mMlrResponseStatus;

        if (status != Mlr::kStatusSuccess)
        {
            while (!offsetRange.IsEmpty())
            {
                IgnoreError(aMsg.mMessage.ReadAndAdvance(offsetRange, address));
                addresses[failedAddressNum++] = address;
            }
        }

        ExitNow();
    }
#endif

    if (Tlv::Find<ThreadCommissionerSessionIdTlv>(aMsg.mMessage, commissionerSessionId) == kErrorNone)
    {
        uint16_t localSessionId;

        VerifyOrExit((Get<NetworkData::Leader>().FindCommissioningSessionId(localSessionId) == kErrorNone) &&
                         (localSessionId == commissionerSessionId),
                     status = Mlr::kStatusGeneralFailure);

        hasCommissionerSessionIdTlv = true;
    }

    processTimeoutTlv =
        hasCommissionerSessionIdTlv && (Tlv::Find<ThreadTimeoutTlv>(aMsg.mMessage, timeout) == kErrorNone);

    if (!processTimeoutTlv)
    {
        timeout = Get<Leader>().GetConfig().GetMlrTimeout();
    }
    else
    {
        VerifyOrExit(timeout < NumericLimits<uint32_t>::kMax, status = Mlr::kStatusNoPersistent);

        if (timeout != 0)
        {
            uint32_t origTimeout = timeout;

            timeout = Min(timeout, kMaxMlrTimeout);

            if (timeout != origTimeout)
            {
                LogNote("MLR.req: MLR timeout is normalized from %lu to %lu", ToUlong(origTimeout), ToUlong(timeout));
            }
        }
    }

    expireTime = TimerMilli::GetNow() + TimeMilli::SecToMsec(timeout);

    while (!offsetRange.IsEmpty())
    {
        IgnoreError(aMsg.mMessage.ReadAndAdvance(offsetRange, address));

        if (timeout == 0)
        {
            mMulticastListenersTable.Remove(address);

            // Put successfully de-registered addresses at the end of `addresses`.
            addresses[Mlr::kMaxIp6Addresses - (++successAddressNum)] = address;
        }
        else
        {
            bool failed = true;

            switch (mMulticastListenersTable.Add(address, expireTime))
            {
            case kErrorNone:
                failed = false;
                break;
            case kErrorInvalidArgs:
                if (status == Mlr::kStatusSuccess)
                {
                    status = Mlr::kStatusInvalid;
                }
                break;
            case kErrorNoBufs:
                if (status == Mlr::kStatusSuccess)
                {
                    status = Mlr::kStatusNoResources;
                }
                break;
            default:
                OT_ASSERT(false);
            }

            if (failed)
            {
                addresses[failedAddressNum++] = address;
            }
            else
            {
                // Put successfully registered addresses at the end of `addresses`.
                addresses[Mlr::kMaxIp6Addresses - (++successAddressNum)] = address;
            }
        }
    }

exit:
    if (error == kErrorNone)
    {
        SendMulticastListenerRegistrationResponse(aMsg, status, addresses, failedAddressNum);
    }

    if (successAddressNum > 0)
    {
        SendBackboneMulticastListenerRegistration(&addresses[Mlr::kMaxIp6Addresses - successAddressNum],
                                                  successAddressNum, timeout);
    }
}

void Manager::SendMulticastListenerRegistrationResponse(const Coap::Msg &aMsg,
                                                        Mlr::Status      aStatus,
                                                        Ip6::Address    *aFailedAddresses,
                                                        uint8_t          aFailedAddressNum)
{
    Error          error = kErrorNone;
    Coap::Message *message;

    message = Get<Tmf::Agent>().AllocateAndInitResponseFor(aMsg.mMessage);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(Tlv::Append<ThreadStatusTlv>(*message, aStatus));

    if (aFailedAddressNum > 0)
    {
        SuccessOrExit(error = Ip6AddressesTlv::AppendTo(*message, aFailedAddresses, aFailedAddressNum));
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, aMsg.mMessageInfo));

exit:
    FreeMessageOnError(message, error);
    LogInfo("Sent MLR.rsp (status=%d): %s", aStatus, ErrorToString(error));
}

void Manager::SendBackboneMulticastListenerRegistration(const Ip6::Address *aAddresses,
                                                        uint8_t             aAddressNum,
                                                        uint32_t            aTimeout)
{
    Error             error   = kErrorNone;
    Coap::Message    *message = nullptr;
    Ip6::MessageInfo  messageInfo;
    BackboneTmfAgent &backboneTmf = Get<BackboneRouter::BackboneTmfAgent>();

    OT_ASSERT(aAddressNum >= Mlr::kMinIp6Addresses && aAddressNum <= Mlr::kMaxIp6Addresses);

    message = backboneTmf.AllocateAndInitNonConfirmablePostMessage(kUriBackboneMlr);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Ip6AddressesTlv::AppendTo(*message, aAddresses, aAddressNum));

    SuccessOrExit(error = Tlv::Append<ThreadTimeoutTlv>(*message, aTimeout));

    messageInfo.SetPeerAddr(Get<Local>().GetAllNetworkBackboneRoutersAddress());
    messageInfo.SetPeerPort(BackboneRouter::kBackboneUdpPort); // TODO: Provide API for configuring Backbone COAP port.

    messageInfo.SetHopLimit(kDefaultHoplimit);
    messageInfo.SetIsHostInterface(true);

    SuccessOrExit(error = backboneTmf.SendMessage(*message, messageInfo));

exit:
    FreeMessageOnError(message, error);
    LogInfo("Sent BMLR.ntf: %s", ErrorToString(error));
}
#endif // OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
void Manager::ConfigNextMulticastListenerRegistrationResponse(Mlr::Status aStatus)
{
    mMlrResponseIsSpecified = true;
    mMlrResponseStatus      = aStatus;
}
#endif
#endif

void Manager::LogError(const char *aText, Error aError) const
{
    OT_UNUSED_VARIABLE(aText);

    if (aError == kErrorNone)
    {
        LogInfo("%s: %s", aText, ErrorToString(aError));
    }
    else
    {
        LogWarn("%s: %s", aText, ErrorToString(aError));
    }
}

} // namespace BackboneRouter

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
