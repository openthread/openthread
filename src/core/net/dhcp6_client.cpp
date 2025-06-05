/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements DHCPv6 Client.
 */

#include "dhcp6_client.hpp"

#if OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Dhcp6 {

RegisterLogModule("Dhcp6Client");

Client::Client(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(aInstance, *this)
    , mTrickleTimer(aInstance, Client::HandleTrickleTimer)
    , mStartTime(0)
    , mIdentityAssociationCurrent(nullptr)
{
    ClearAllBytes(mIdentityAssociations);
}

void Client::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadNetdataChanged))
    {
        UpdateAddresses();
    }
}

void Client::UpdateAddresses(void)
{
    bool                            found          = false;
    bool                            doesAgentExist = false;
    NetworkData::Iterator           iterator;
    NetworkData::OnMeshPrefixConfig config;

    // remove addresses directly if prefix not valid in network data
    for (IdentityAssociation &idAssociation : mIdentityAssociations)
    {
        if (idAssociation.mStatus == kIaStatusInvalid || idAssociation.mValidLifetime == 0)
        {
            continue;
        }

        found    = false;
        iterator = NetworkData::kIteratorInit;

        while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, config) == kErrorNone)
        {
            if (!config.mDhcp)
            {
                continue;
            }

            if (idAssociation.mNetifAddress.HasPrefix(config.GetPrefix()))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            Get<ThreadNetif>().RemoveUnicastAddress(idAssociation.mNetifAddress);
            idAssociation.mStatus = kIaStatusInvalid;
        }
    }

    // add IdentityAssociation for new configured prefix
    iterator = NetworkData::kIteratorInit;

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, config) == kErrorNone)
    {
        IdentityAssociation *idAssociation = nullptr;

        if (!config.mDhcp)
        {
            continue;
        }

        doesAgentExist = true;
        found          = false;

        for (IdentityAssociation &ia : mIdentityAssociations)
        {
            if (ia.mStatus == kIaStatusInvalid)
            {
                // record an available IdentityAssociation
                if (idAssociation == nullptr)
                {
                    idAssociation = &ia;
                }
            }
            else if (ia.mNetifAddress.HasPrefix(config.GetPrefix()))
            {
                found         = true;
                idAssociation = &ia;
                break;
            }
        }

        if (!found)
        {
            if (idAssociation != nullptr)
            {
                idAssociation->mNetifAddress.mAddress      = config.mPrefix.mPrefix;
                idAssociation->mNetifAddress.mPrefixLength = config.mPrefix.mLength;
                idAssociation->mStatus                     = kIaStatusSolicit;
                idAssociation->mValidLifetime              = 0;
            }
            else
            {
                LogWarn("Insufficient memory for new DHCP prefix");
                continue;
            }
        }

        idAssociation->mPrefixAgentRloc = config.mRloc16;
    }

    if (doesAgentExist)
    {
        Start();
    }
    else
    {
        Stop();
    }
}

void Client::Start(void)
{
    VerifyOrExit(!mSocket.IsBound());

    IgnoreError(mSocket.Open(Ip6::kNetifThreadInternal));
    IgnoreError(mSocket.Bind(kDhcpClientPort));

    ProcessNextIdentityAssociation();

exit:
    return;
}

void Client::Stop(void)
{
    mTrickleTimer.Stop();
    IgnoreError(mSocket.Close());
}

bool Client::ProcessNextIdentityAssociation(void)
{
    bool rval = false;

    // not interrupt in-progress solicit
    VerifyOrExit(mIdentityAssociationCurrent == nullptr || mIdentityAssociationCurrent->mStatus != kIaStatusSoliciting);

    mTrickleTimer.Stop();

    for (IdentityAssociation &idAssociation : mIdentityAssociations)
    {
        if (idAssociation.mStatus != kIaStatusSolicit)
        {
            continue;
        }

        mTransactionId.GenerateRandom();

        mIdentityAssociationCurrent = &idAssociation;

        mTrickleTimer.Start(TrickleTimer::kModeTrickle, Time::SecToMsec(kTrickleTimerImin),
                            Time::SecToMsec(kTrickleTimerImax));

        mTrickleTimer.IndicateInconsistent();

        ExitNow(rval = true);
    }

exit:
    return rval;
}

void Client::HandleTrickleTimer(TrickleTimer &aTrickleTimer) { aTrickleTimer.Get<Client>().HandleTrickleTimer(); }

void Client::HandleTrickleTimer(void)
{
    OT_ASSERT(mSocket.IsBound());

    VerifyOrExit(mIdentityAssociationCurrent != nullptr, mTrickleTimer.Stop());

    switch (mIdentityAssociationCurrent->mStatus)
    {
    case kIaStatusSolicit:
        mStartTime                           = TimerMilli::GetNow();
        mIdentityAssociationCurrent->mStatus = kIaStatusSoliciting;

        OT_FALL_THROUGH;

    case kIaStatusSoliciting:
        Solicit(mIdentityAssociationCurrent->mPrefixAgentRloc);
        break;

    case kIaStatusSolicitReplied:
        mIdentityAssociationCurrent = nullptr;

        if (!ProcessNextIdentityAssociation())
        {
            Stop();
            mTrickleTimer.Stop();
        }

        break;

    default:
        break;
    }

exit:
    return;
}

void Client::Solicit(uint16_t aRloc16)
{
    Error            error = kErrorNone;
    Message         *message;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = mSocket.NewMessage()) != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = AppendHeader(*message));
    SuccessOrExit(error = AppendElapsedTimeOption(*message));
    SuccessOrExit(error = AppendClientIdOption(*message));
    SuccessOrExit(error = AppendIaNaOption(*message, aRloc16));
    SuccessOrExit(error = AppendRapidCommitOption(*message));

#if OPENTHREAD_ENABLE_DHCP6_MULTICAST_SOLICIT
    messageInfo.GetPeerAddr().SetToRealmLocalAllRoutersMulticast();
#else
    messageInfo.GetPeerAddr().SetToRoutingLocator(Get<Mle::Mle>().GetMeshLocalPrefix(), aRloc16);
#endif
    messageInfo.SetSockAddr(Get<Mle::Mle>().GetMeshLocalRloc());
    messageInfo.mPeerPort = kDhcpServerPort;

    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));
    LogInfo("solicit");

exit:
    if (error != kErrorNone)
    {
        FreeMessage(message);
        LogWarnOnError(error, "send DHCPv6 Solicit");
    }
}

Error Client::AppendHeader(Message &aMessage)
{
    Header header;

    header.Clear();
    header.SetMsgType(kMsgTypeSolicit);
    header.SetTransactionId(mTransactionId);
    return aMessage.Append(header);
}

Error Client::AppendElapsedTimeOption(Message &aMessage)
{
    ElapsedTimeOption option;

    option.Init();
    option.SetElapsedTime(static_cast<uint16_t>(Time::MsecToSec(TimerMilli::GetNow() - mStartTime)));
    return aMessage.Append(option);
}

Error Client::AppendClientIdOption(Message &aMessage)
{
    Mac::ExtAddress eui64;

    Get<Radio>().GetIeeeEui64(eui64);

    return ClientIdOption::AppendWithEui64Duid(aMessage, eui64);
}

Error Client::AppendIaNaOption(Message &aMessage, uint16_t aRloc16)
{
    Error           error = kErrorNone;
    uint16_t        optionOffset;
    IaNaOption      option;
    IaAddressOption addressOption;

    VerifyOrExit(mIdentityAssociationCurrent != nullptr, error = kErrorDrop);

    optionOffset = aMessage.GetLength();

    option.Init();
    option.SetIaid(0);
    option.SetT1(0);
    option.SetT2(0);
    SuccessOrExit(error = aMessage.Append(option));

    // Append `IaAddressOption`s within the `IaNa`

    addressOption.Init();

    for (IdentityAssociation &idAssociation : mIdentityAssociations)
    {
        if ((idAssociation.mStatus == kIaStatusSolicit || idAssociation.mStatus == kIaStatusSoliciting) &&
            (idAssociation.mPrefixAgentRloc == aRloc16))
        {
            addressOption.SetAddress(idAssociation.mNetifAddress.GetAddress());
            addressOption.SetPreferredLifetime(0);
            addressOption.SetValidLifetime(0);
            SuccessOrExit(error = aMessage.Append(addressOption));
        }
    }

    // Update the `IaNaOption` length.
    Option::UpdateOptionLengthInMessage(aMessage, optionOffset);

exit:
    return error;
}

void Client::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Header header;

    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), header));
    aMessage.MoveOffset(sizeof(header));

    if ((header.GetMsgType() == kMsgTypeReply) && (header.GetTransactionId() == mTransactionId))
    {
        ProcessReply(aMessage);
    }

exit:
    return;
}

void Client::ProcessReply(const Message &aMessage)
{
    VerifyOrExit(StatusCodeOption::ReadStatusFrom(aMessage) == StatusCodeOption::kSuccess);
    SuccessOrExit(ProcessServerIdOption(aMessage));
    SuccessOrExit(ProcessClientIdOption(aMessage));
    SuccessOrExit(RapidCommitOption::FindIn(aMessage));
    SuccessOrExit(ProcessIaNaOption(aMessage));

    HandleTrickleTimer();

exit:
    return;
}

Error Client::ProcessServerIdOption(const Message &aMessage)
{
    OffsetRange duidOffsetRange;

    return ServerIdOption::ReadDuid(aMessage, duidOffsetRange);
}

Error Client::ProcessClientIdOption(const Message &aMessage)
{
    Mac::ExtAddress eui64;

    Get<Radio>().GetIeeeEui64(eui64);

    return ClientIdOption::MatchesEui64Duid(aMessage, eui64);
}

Error Client::ProcessIaNaOption(const Message &aMessage)
{
    Error            error = kErrorNone;
    OffsetRange      offsetRange;
    IaNaOption       option;
    Option::Iterator iterator;

    SuccessOrExit(error = Option::FindOption(aMessage, Option::kIaNa, offsetRange));
    SuccessOrExit(error = aMessage.Read(offsetRange, option));

    offsetRange.AdvanceOffset(sizeof(IaNaOption));

    // Iterate over and check the sub-options within `IaNaOption`.

    VerifyOrExit(StatusCodeOption::ReadStatusFrom(aMessage, offsetRange) == StatusCodeOption::kSuccess,
                 error = kErrorFailed);

    for (iterator.Init(aMessage, offsetRange, Option::kIaAddress); !iterator.IsDone(); iterator.Advance())
    {
        IaAddressOption addressOption;

        SuccessOrExit(error = aMessage.Read(iterator.GetOptionOffsetRange(), addressOption));
        SuccessOrExit(error = ProcessIaAddressOption(addressOption));
    }

    error = iterator.GetError();

exit:
    return error;
}

Error Client::ProcessIaAddressOption(const IaAddressOption &aOption)
{
    Error error = kErrorNone;

    for (IdentityAssociation &idAssociation : mIdentityAssociations)
    {
        if (idAssociation.mStatus == kIaStatusInvalid || idAssociation.mValidLifetime != 0)
        {
            continue;
        }

        if (idAssociation.mNetifAddress.GetAddress().PrefixMatch(aOption.GetAddress()) >=
            idAssociation.mNetifAddress.mPrefixLength)
        {
            idAssociation.mNetifAddress.mAddress       = aOption.GetAddress();
            idAssociation.mPreferredLifetime           = aOption.GetPreferredLifetime();
            idAssociation.mValidLifetime               = aOption.GetValidLifetime();
            idAssociation.mNetifAddress.mAddressOrigin = Ip6::Netif::kOriginDhcp6;
            idAssociation.mNetifAddress.mPreferred     = aOption.GetPreferredLifetime() != 0;
            idAssociation.mNetifAddress.mValid         = aOption.GetValidLifetime() != 0;
            idAssociation.mStatus                      = kIaStatusSolicitReplied;
            Get<ThreadNetif>().AddUnicastAddress(idAssociation.mNetifAddress);
            ExitNow(error = kErrorNone);
        }
    }

    error = kErrorNotFound;

exit:
    return error;
}

} // namespace Dhcp6
} // namespace ot

#endif // OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE
