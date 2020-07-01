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

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"
#include "mac/mac.hpp"
#include "net/dhcp6.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {

namespace Dhcp6 {

Dhcp6Client::Dhcp6Client(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(Get<Ip6::Udp>())
    , mTrickleTimer(aInstance, Dhcp6Client::HandleTrickleTimer, nullptr, this)
    , mStartTime(0)
    , mIdentityAssociationCurrent(nullptr)
{
    memset(mIdentityAssociations, 0, sizeof(mIdentityAssociations));
}

bool Dhcp6Client::MatchNetifAddressWithPrefix(const Ip6::NetifUnicastAddress &aNetifAddress,
                                              const otIp6Prefix &             aIp6Prefix)
{
    return (aIp6Prefix.mLength == aNetifAddress.mPrefixLength) &&
           (aNetifAddress.GetAddress().PrefixMatch(aIp6Prefix.mPrefix) >= aIp6Prefix.mLength);
}

void Dhcp6Client::UpdateAddresses(void)
{
    bool                            found          = false;
    bool                            doesAgentExist = false;
    NetworkData::Iterator           iterator;
    NetworkData::OnMeshPrefixConfig config;

    // remove addresses directly if prefix not valid in network data
    for (size_t i = 0; i < OT_ARRAY_LENGTH(mIdentityAssociations); i++)
    {
        IdentityAssociation &ia = mIdentityAssociations[i];

        if (ia.mStatus == kIaStatusInvalid || ia.mValidLifetime == 0)
        {
            continue;
        }

        found    = false;
        iterator = NetworkData::kIteratorInit;

        while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, config) == OT_ERROR_NONE)
        {
            if (!config.mDhcp)
            {
                continue;
            }

            if (MatchNetifAddressWithPrefix(ia.mNetifAddress, config.mPrefix))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            Get<ThreadNetif>().RemoveUnicastAddress(ia.mNetifAddress);
            mIdentityAssociations[i].mStatus = kIaStatusInvalid;
        }
    }

    // add IdentityAssociation for new configured prefix
    iterator = NetworkData::kIteratorInit;

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, config) == OT_ERROR_NONE)
    {
        IdentityAssociation *ia = nullptr;

        if (!config.mDhcp)
        {
            continue;
        }

        doesAgentExist = true;
        found          = false;

        for (size_t i = 0; i < OT_ARRAY_LENGTH(mIdentityAssociations); i++)
        {
            if (mIdentityAssociations[i].mStatus == kIaStatusInvalid)
            {
                // record an available ia
                if (ia == nullptr)
                {
                    ia = &mIdentityAssociations[i];
                }
            }
            else if (MatchNetifAddressWithPrefix(mIdentityAssociations[i].mNetifAddress, config.mPrefix))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            if (ia != nullptr)
            {
                ia->mPrefixAgentRloc            = config.mRloc16;
                ia->mNetifAddress.mAddress      = config.mPrefix.mPrefix;
                ia->mNetifAddress.mPrefixLength = config.mPrefix.mLength;
                ia->mStatus                     = kIaStatusSolicit;
                ia->mValidLifetime              = 0;
            }
            else
            {
                otLogWarnIp6("Insufficient memory for new DHCP prefix");
                continue;
            }
        }
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

void Dhcp6Client::Start(void)
{
    Ip6::SockAddr sockaddr;

    VerifyOrExit(!mSocket.IsBound(), OT_NOOP);

    sockaddr.mPort = kDhcpClientPort;
    IgnoreError(mSocket.Open(&Dhcp6Client::HandleUdpReceive, this));
    IgnoreError(mSocket.Bind(sockaddr));

    ProcessNextIdentityAssociation();

exit:
    return;
}

void Dhcp6Client::Stop(void)
{
    IgnoreError(mSocket.Close());
}

bool Dhcp6Client::ProcessNextIdentityAssociation()
{
    bool rval = false;

    // not interrupt in-progress solicit
    VerifyOrExit(mIdentityAssociationCurrent == nullptr || mIdentityAssociationCurrent->mStatus != kIaStatusSoliciting,
                 OT_NOOP);

    mTrickleTimer.Stop();

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mIdentityAssociations); ++i)
    {
        if (mIdentityAssociations[i].mStatus != kIaStatusSolicit)
        {
            continue;
        }

        // new transaction id
        IgnoreError(Random::Crypto::FillBuffer(mTransactionId, kTransactionIdSize));

        mIdentityAssociationCurrent = &mIdentityAssociations[i];

        mTrickleTimer.Start(Time::SecToMsec(kTrickleTimerImin), Time::SecToMsec(kTrickleTimerImax),
                            TrickleTimer::kModeNormal);

        mTrickleTimer.IndicateInconsistent();

        ExitNow(rval = true);
    }

exit:
    return rval;
}

bool Dhcp6Client::HandleTrickleTimer(TrickleTimer &aTrickleTimer)
{
    return aTrickleTimer.GetOwner<Dhcp6Client>().HandleTrickleTimer();
}

bool Dhcp6Client::HandleTrickleTimer(void)
{
    bool rval = true;

    VerifyOrExit(mIdentityAssociationCurrent != nullptr, rval = false);

    switch (mIdentityAssociationCurrent->mStatus)
    {
    case kIaStatusSolicit:
        mStartTime                           = TimerMilli::GetNow();
        mIdentityAssociationCurrent->mStatus = kIaStatusSoliciting;

        // fall through

    case kIaStatusSoliciting:
        Solicit(mIdentityAssociationCurrent->mPrefixAgentRloc);
        break;

    case kIaStatusSolicitReplied:
        mIdentityAssociationCurrent = nullptr;

        if (!ProcessNextIdentityAssociation())
        {
            mTrickleTimer.Stop();
            Stop();
            rval = false;
        }

        break;

    default:
        break;
    }

exit:
    return rval;
}

void Dhcp6Client::Solicit(uint16_t aRloc16)
{
    otError          error = OT_ERROR_NONE;
    Message *        message;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = mSocket.NewMessage(0)) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = AppendHeader(*message));
    SuccessOrExit(error = AppendElapsedTime(*message));
    SuccessOrExit(error = AppendClientIdentifier(*message));
    SuccessOrExit(error = AppendIaNa(*message, aRloc16));
    // specify which prefixes to solicit
    SuccessOrExit(error = AppendIaAddress(*message, aRloc16));
    SuccessOrExit(error = AppendRapidCommit(*message));

#if OPENTHREAD_ENABLE_DHCP6_MULTICAST_SOLICIT
    messageInfo.GetPeerAddr().SetToRealmLocalAllRoutersMulticast();
#else
    messageInfo.GetPeerAddr().SetToRoutingLocator(Get<Mle::MleRouter>().GetMeshLocalPrefix(), aRloc16);
#endif
    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.mPeerPort = kDhcpServerPort;

    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));
    otLogInfoIp6("solicit");

exit:

    if (message != nullptr)
    {
        otLogWarnIp6("Failed to send DHCPv6 Solicit: %s", otThreadErrorToString(error));

        if (error != OT_ERROR_NONE)
        {
            message->Free();
        }
    }
}

otError Dhcp6Client::AppendHeader(Message &aMessage)
{
    Dhcp6Header header;

    header.Init();
    header.SetType(kTypeSolicit);
    header.SetTransactionId(mTransactionId);
    return aMessage.Append(&header, sizeof(header));
}

otError Dhcp6Client::AppendElapsedTime(Message &aMessage)
{
    ElapsedTime option;

    option.Init();
    option.SetElapsedTime(static_cast<uint16_t>(Time::MsecToSec(TimerMilli::GetNow() - mStartTime)));
    return aMessage.Append(&option, sizeof(option));
}

otError Dhcp6Client::AppendClientIdentifier(Message &aMessage)
{
    ClientIdentifier option;
    Mac::ExtAddress  eui64;

    Get<Radio>().GetIeeeEui64(eui64);

    option.Init();
    option.SetDuidType(kDuidLL);
    option.SetDuidHardwareType(kHardwareTypeEui64);
    option.SetDuidLinkLayerAddress(eui64);

    return aMessage.Append(&option, sizeof(option));
}

otError Dhcp6Client::AppendIaNa(Message &aMessage, uint16_t aRloc16)
{
    otError  error  = OT_ERROR_NONE;
    uint8_t  count  = 0;
    uint16_t length = 0;
    IaNa     option;

    VerifyOrExit(mIdentityAssociationCurrent != nullptr, error = OT_ERROR_DROP);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mIdentityAssociations); ++i)
    {
        if (mIdentityAssociations[i].mStatus == kIaStatusInvalid ||
            mIdentityAssociations[i].mStatus == kIaStatusSolicitReplied)
        {
            continue;
        }

        if (mIdentityAssociations[i].mPrefixAgentRloc == aRloc16)
        {
            count++;
        }
    }

    // compute the right length
    length = sizeof(IaNa) + sizeof(IaAddress) * count - sizeof(Dhcp6Option);

    option.Init();
    option.SetLength(length);
    option.SetIaid(0);
    option.SetT1(0);
    option.SetT2(0);
    SuccessOrExit(error = aMessage.Append(&option, sizeof(IaNa)));

exit:
    return error;
}

otError Dhcp6Client::AppendIaAddress(Message &aMessage, uint16_t aRloc16)
{
    otError   error = OT_ERROR_NONE;
    IaAddress option;

    VerifyOrExit(mIdentityAssociationCurrent, error = OT_ERROR_DROP);

    option.Init();

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mIdentityAssociations); ++i)
    {
        if ((mIdentityAssociations[i].mStatus == kIaStatusSolicit ||
             mIdentityAssociations[i].mStatus == kIaStatusSoliciting) &&
            (mIdentityAssociations[i].mPrefixAgentRloc == aRloc16))
        {
            option.SetAddress(mIdentityAssociations[i].mNetifAddress.mAddress);
            option.SetPreferredLifetime(0);
            option.SetValidLifetime(0);
            SuccessOrExit(error = aMessage.Append(&option, sizeof(option)));
        }
    }

exit:
    return error;
}

otError Dhcp6Client::AppendRapidCommit(Message &aMessage)
{
    RapidCommit option;

    option.Init();
    return aMessage.Append(&option, sizeof(option));
}

void Dhcp6Client::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    Dhcp6Client *obj = static_cast<Dhcp6Client *>(aContext);
    obj->HandleUdpReceive(*static_cast<Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Dhcp6Client::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Dhcp6Header header;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(header), &header) == sizeof(header), OT_NOOP);
    aMessage.MoveOffset(sizeof(header));

    if ((header.GetType() == kTypeReply) && (!memcmp(header.GetTransactionId(), mTransactionId, kTransactionIdSize)))
    {
        ProcessReply(aMessage);
    }

exit:
    return;
}

void Dhcp6Client::ProcessReply(Message &aMessage)
{
    uint16_t offset = aMessage.GetOffset();
    uint16_t length = aMessage.GetLength() - aMessage.GetOffset();
    uint16_t optionOffset;

    if ((optionOffset = FindOption(aMessage, offset, length, kOptionStatusCode)) > 0)
    {
        SuccessOrExit(ProcessStatusCode(aMessage, optionOffset));
    }

    // Server Identifier
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, kOptionServerIdentifier)) > 0, OT_NOOP);
    SuccessOrExit(ProcessServerIdentifier(aMessage, optionOffset));

    // Client Identifier
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, kOptionClientIdentifier)) > 0, OT_NOOP);
    SuccessOrExit(ProcessClientIdentifier(aMessage, optionOffset));

    // Rapid Commit
    VerifyOrExit(FindOption(aMessage, offset, length, kOptionRapidCommit) > 0, OT_NOOP);

    // IA_NA
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, kOptionIaNa)) > 0, OT_NOOP);
    SuccessOrExit(ProcessIaNa(aMessage, optionOffset));

    HandleTrickleTimer();

exit:
    return;
}

uint16_t Dhcp6Client::FindOption(Message &aMessage, uint16_t aOffset, uint16_t aLength, Dhcp6::Code aCode)
{
    uint16_t end  = aOffset + aLength;
    uint16_t rval = 0;

    while (aOffset <= end)
    {
        Dhcp6Option option;
        VerifyOrExit(aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option), OT_NOOP);

        if (option.GetCode() == (aCode))
        {
            ExitNow(rval = aOffset);
        }

        aOffset += sizeof(option) + option.GetLength();
    }

exit:
    return rval;
}

otError Dhcp6Client::ProcessServerIdentifier(Message &aMessage, uint16_t aOffset)
{
    otError          error = OT_ERROR_NONE;
    ServerIdentifier option;

    VerifyOrExit((aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option)), OT_NOOP);
    VerifyOrExit(((option.GetDuidType() == kDuidLLT) && (option.GetDuidHardwareType() == kHardwareTypeEthernet)) ||
                     ((option.GetLength() == (sizeof(option) - sizeof(Dhcp6Option))) &&
                      (option.GetDuidType() == kDuidLL) && (option.GetDuidHardwareType() == kHardwareTypeEui64)),
                 error = OT_ERROR_PARSE);
exit:
    return error;
}

otError Dhcp6Client::ProcessClientIdentifier(Message &aMessage, uint16_t aOffset)
{
    otError          error = OT_ERROR_NONE;
    ClientIdentifier option;
    Mac::ExtAddress  eui64;

    Get<Radio>().GetIeeeEui64(eui64);

    VerifyOrExit((((aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option)) &&
                   (option.GetLength() == (sizeof(option) - sizeof(Dhcp6Option))) &&
                   (option.GetDuidType() == kDuidLL) && (option.GetDuidHardwareType() == kHardwareTypeEui64)) &&
                  (!memcmp(option.GetDuidLinkLayerAddress(), eui64.m8, sizeof(Mac::ExtAddress)))),
                 error = OT_ERROR_PARSE);
exit:
    return error;
}

otError Dhcp6Client::ProcessIaNa(Message &aMessage, uint16_t aOffset)
{
    otError  error = OT_ERROR_NONE;
    IaNa     option;
    uint16_t optionOffset;
    uint16_t length;

    VerifyOrExit(aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option), error = OT_ERROR_PARSE);

    aOffset += sizeof(option);
    length = option.GetLength() - (sizeof(option) - sizeof(Dhcp6Option));

    VerifyOrExit(length <= aMessage.GetLength() - aOffset, error = OT_ERROR_PARSE);

    if ((optionOffset = FindOption(aMessage, aOffset, length, kOptionStatusCode)) > 0)
    {
        SuccessOrExit(error = ProcessStatusCode(aMessage, optionOffset));
    }

    while (length > 0)
    {
        if ((optionOffset = FindOption(aMessage, aOffset, length, kOptionIaAddress)) == 0)
        {
            ExitNow();
        }

        SuccessOrExit(error = ProcessIaAddress(aMessage, optionOffset));

        length -= ((optionOffset - aOffset) + sizeof(IaAddress));
        aOffset = optionOffset + sizeof(IaAddress);
    }

exit:
    return error;
}

otError Dhcp6Client::ProcessStatusCode(Message &aMessage, uint16_t aOffset)
{
    otError    error = OT_ERROR_NONE;
    StatusCode option;

    VerifyOrExit(((aMessage.Read(aOffset, sizeof(option), &option) >= sizeof(option)) &&
                  (option.GetLength() >= (sizeof(option) - sizeof(Dhcp6Option))) &&
                  (option.GetStatusCode() == kStatusSuccess)),
                 error = OT_ERROR_PARSE);

exit:
    return error;
}

otError Dhcp6Client::ProcessIaAddress(Message &aMessage, uint16_t aOffset)
{
    otError   error = OT_ERROR_NONE;
    IaAddress option;

    VerifyOrExit(((aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option)) &&
                  (option.GetLength() == (sizeof(option) - sizeof(Dhcp6Option)))),
                 error = OT_ERROR_PARSE);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mIdentityAssociations); ++i)
    {
        IdentityAssociation &ia = mIdentityAssociations[i];

        if (ia.mStatus == kIaStatusInvalid || ia.mValidLifetime != 0)
        {
            continue;
        }

        if (ia.mNetifAddress.GetAddress().PrefixMatch(option.GetAddress()) >= ia.mNetifAddress.mPrefixLength)
        {
            mIdentityAssociations[i].mNetifAddress.mAddress       = option.GetAddress();
            mIdentityAssociations[i].mPreferredLifetime           = option.GetPreferredLifetime();
            mIdentityAssociations[i].mValidLifetime               = option.GetValidLifetime();
            mIdentityAssociations[i].mNetifAddress.mAddressOrigin = OT_ADDRESS_ORIGIN_DHCPV6;
            mIdentityAssociations[i].mNetifAddress.mPreferred     = option.GetPreferredLifetime() != 0;
            mIdentityAssociations[i].mNetifAddress.mValid         = option.GetValidLifetime() != 0;
            mIdentityAssociations[i].mStatus                      = kIaStatusSolicitReplied;
            Get<ThreadNetif>().AddUnicastAddress(ia.mNetifAddress);
            break;
        }
    }

exit:
    return error;
}

} // namespace Dhcp6
} // namespace ot

#endif // OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE
