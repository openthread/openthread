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
 *   This file implements DHCPv6 Server.
 */

#include "dhcp6_server.hpp"

#if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Dhcp6 {

RegisterLogModule("Dhcp6Server");

Server::Server(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(aInstance, *this)
    , mPrefixAgentsCount(0)
    , mPrefixAgentsMask(0)
{
    ClearAllBytes(mPrefixAgents);
}

void Server::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadNetdataChanged))
    {
        UpdateService();
    }
}

void Server::UpdateService(void)
{
    Error                           error  = kErrorNone;
    uint16_t                        rloc16 = Get<Mle::Mle>().GetRloc16();
    NetworkData::Iterator           iterator;
    NetworkData::OnMeshPrefixConfig config;
    Lowpan::Context                 lowpanContext;

    // remove dhcp agent aloc and prefix delegation
    for (PrefixAgent &prefixAgent : mPrefixAgents)
    {
        bool found = false;

        if (!prefixAgent.IsValid())
        {
            continue;
        }

        iterator = NetworkData::kIteratorInit;

        while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, rloc16, config) == kErrorNone)
        {
            if (!(config.mDhcp || config.mConfigure))
            {
                continue;
            }

            error = Get<NetworkData::Leader>().GetContext(prefixAgent.GetPrefixAsAddress(), lowpanContext);

            if ((error == kErrorNone) && (prefixAgent.GetContextId() == lowpanContext.mContextId))
            {
                // still in network data
                found = true;
                break;
            }
        }

        if (!found)
        {
            Get<ThreadNetif>().RemoveUnicastAddress(prefixAgent.GetAloc());
            prefixAgent.Clear();
            mPrefixAgentsCount--;
        }
    }

    // add dhcp agent aloc and prefix delegation
    iterator = NetworkData::kIteratorInit;

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, rloc16, config) == kErrorNone)
    {
        if (!(config.mDhcp || config.mConfigure))
        {
            continue;
        }

        error = Get<NetworkData::Leader>().GetContext(AsCoreType(&config.mPrefix.mPrefix), lowpanContext);

        if (error == kErrorNone)
        {
            AddPrefixAgent(config.GetPrefix(), lowpanContext);
        }
    }

    if (mPrefixAgentsCount > 0)
    {
        Start();
    }
    else
    {
        Stop();
    }
}

void Server::Start(void)
{
    VerifyOrExit(!mSocket.IsOpen());

    IgnoreError(mSocket.Open(Ip6::kNetifThreadInternal));
    IgnoreError(mSocket.Bind(kDhcpServerPort));

exit:
    return;
}

void Server::Stop(void) { IgnoreError(mSocket.Close()); }

void Server::AddPrefixAgent(const Ip6::Prefix &aIp6Prefix, const Lowpan::Context &aContext)
{
    Error        error    = kErrorNone;
    PrefixAgent *newEntry = nullptr;

    for (PrefixAgent &prefixAgent : mPrefixAgents)
    {
        if (!prefixAgent.IsValid())
        {
            newEntry = &prefixAgent;
        }
        else if (prefixAgent.GetPrefix() == aIp6Prefix)
        {
            // already added
            ExitNow();
        }
    }

    VerifyOrExit(newEntry != nullptr, error = kErrorNoBufs);

    newEntry->Set(aIp6Prefix, Get<Mle::Mle>().GetMeshLocalPrefix(), aContext.mContextId);
    Get<ThreadNetif>().AddUnicastAddress(newEntry->GetAloc());
    mPrefixAgentsCount++;

exit:
    LogWarnOnError(error, "add DHCPv6 prefix agent");
    OT_UNUSED_VARIABLE(error);
}

void Server::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Header header;

    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), header));
    aMessage.MoveOffset(sizeof(header));

    VerifyOrExit((header.GetMsgType() == kMsgTypeSolicit));

    ProcessSolicit(aMessage, aMessageInfo.GetPeerAddr(), header.GetTransactionId());

exit:
    return;
}

void Server::ProcessSolicit(Message &aMessage, const Ip6::Address &aDst, const TransactionId &aTransactionId)
{
    uint32_t        iaid;
    Mac::ExtAddress clientAddress;
    OffsetRange     offsetRange;

    SuccessOrExit(ClientIdOption::ReadAsEui64Duid(aMessage, clientAddress));

    // Server Identifier (assuming Rapid Commit, discard if present)
    VerifyOrExit(Option::FindOption(aMessage, Option::kServerId, offsetRange) == kErrorNotFound);

    // Rapid Commit (assuming Rapid Commit, discard if not present)
    SuccessOrExit(RapidCommitOption::FindIn(aMessage));

    // Elapsed Time if present
    SuccessOrExit(ProcessElapsedTimeOption(aMessage));

    // IA_NA (discard if not present)
    SuccessOrExit(ProcessIaNaOption(aMessage, iaid));

    SuccessOrExit(SendReply(aDst, aTransactionId, clientAddress, iaid));

exit:
    return;
}

Error Server::ProcessElapsedTimeOption(const Message &aMessage)
{
    Error       error = kErrorNone;
    OffsetRange offsetRange;

    // Check Elapsed Time to be valid only if present.

    error = Option::FindOption(aMessage, Option::kElapsedTime, offsetRange);

    if (error == kErrorNotFound)
    {
        ExitNow();
    }

    SuccessOrExit(error);

    VerifyOrExit(offsetRange.GetLength() >= sizeof(ElapsedTimeOption), error = kErrorParse);

exit:
    return error;
}

Error Server::ProcessIaNaOption(const Message &aMessage, uint32_t &aIaid)
{
    Error            error = kErrorNone;
    OffsetRange      offsetRange;
    IaNaOption       iaNaOption;
    Option::Iterator iterator;

    SuccessOrExit(error = Option::FindOption(aMessage, Option::kIaNa, offsetRange));
    SuccessOrExit(error = aMessage.Read(offsetRange, iaNaOption));

    aIaid = iaNaOption.GetIaid();

    offsetRange.AdvanceOffset(sizeof(IaNaOption));

    mPrefixAgentsMask = 0;

    // Iterate and parse `kIaAddress` sub-options within `IaNaOption`.

    for (iterator.Init(aMessage, offsetRange, Option::kIaAddress); !iterator.IsDone(); iterator.Advance())
    {
        IaAddressOption addressOption;

        SuccessOrExit(error = aMessage.Read(iterator.GetOptionOffsetRange(), addressOption));
        ProcessIaAddressOption(addressOption);
    }

    error = iterator.GetError();

exit:
    return error;
}

void Server::ProcessIaAddressOption(const IaAddressOption &aAddressOption)
{
    // mask matching prefix
    for (uint16_t i = 0; i < GetArrayLength(mPrefixAgents); i++)
    {
        if (mPrefixAgents[i].IsValid() && mPrefixAgents[i].IsPrefixMatch(aAddressOption.GetAddress()))
        {
            mPrefixAgentsMask |= (1 << i);
            break;
        }
    }
}

Error Server::SendReply(const Ip6::Address    &aDst,
                        const TransactionId   &aTransactionId,
                        const Mac::ExtAddress &aClientAddress,
                        uint32_t               aIaid)
{
    Error            error = kErrorNone;
    Ip6::MessageInfo messageInfo;
    Message         *message;

    VerifyOrExit((message = mSocket.NewMessage()) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = AppendHeader(*message, aTransactionId));
    SuccessOrExit(error = AppendServerIdOption(*message));
    SuccessOrExit(error = AppendClientIdOption(*message, aClientAddress));
    SuccessOrExit(error = AppendIaNaOption(*message, aIaid, aClientAddress));
    SuccessOrExit(error = AppendRapidCommitOption(*message));

    messageInfo.SetPeerAddr(aDst);
    messageInfo.SetPeerPort(kDhcpClientPort);
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Server::AppendHeader(Message &aMessage, const TransactionId &aTransactionId)
{
    Header header;

    header.Clear();
    header.SetMsgType(kMsgTypeReply);
    header.SetTransactionId(aTransactionId);
    return aMessage.Append(header);
}

Error Server::AppendClientIdOption(Message &aMessage, const Mac::ExtAddress &aClientAddress)
{
    return ClientIdOption::AppendWithEui64Duid(aMessage, aClientAddress);
}

Error Server::AppendServerIdOption(Message &aMessage)
{
    Mac::ExtAddress eui64;

    Get<Radio>().GetIeeeEui64(eui64);

    return ServerIdOption::AppendWithEui64Duid(aMessage, eui64);
}

Error Server::AppendIaNaOption(Message &aMessage, uint32_t aIaid, const Mac::ExtAddress &aClientAddress)
{
    Error      error = kErrorNone;
    IaNaOption iaNaOption;
    uint16_t   optionOffset;

    optionOffset = aMessage.GetLength();

    iaNaOption.Init();
    iaNaOption.SetIaid(aIaid);
    iaNaOption.SetT1(IaNaOption::kDefaultT1);
    iaNaOption.SetT2(IaNaOption::kDefaultT2);
    SuccessOrExit(error = aMessage.Append(iaNaOption));

    SuccessOrExit(error = AppendStatusCodeOption(aMessage, StatusCodeOption::kSuccess));
    SuccessOrExit(error = AppendIaAddressOptions(aMessage, aClientAddress));

    // Update IaNaOption length
    Option::UpdateOptionLengthInMessage(aMessage, optionOffset);

exit:
    return error;
}

Error Server::AppendStatusCodeOption(Message &aMessage, StatusCodeOption::Status aStatusCode)
{
    StatusCodeOption option;

    option.Init();
    option.SetStatusCode(aStatusCode);
    return aMessage.Append(option);
}

Error Server::AppendIaAddressOptions(Message &aMessage, const Mac::ExtAddress &aClientAddress)
{
    Error error = kErrorNone;

    if (mPrefixAgentsMask)
    {
        // if specified, only apply specified prefixes
        for (uint16_t i = 0; i < GetArrayLength(mPrefixAgents); i++)
        {
            if (mPrefixAgentsMask & (1 << i))
            {
                SuccessOrExit(
                    error = AppendIaAddressOption(aMessage, mPrefixAgents[i].GetPrefixAsAddress(), aClientAddress));
            }
        }
    }
    else
    {
        // if not specified, apply all configured prefixes
        for (const PrefixAgent &prefixAgent : mPrefixAgents)
        {
            if (prefixAgent.IsValid())
            {
                SuccessOrExit(error =
                                  AppendIaAddressOption(aMessage, prefixAgent.GetPrefixAsAddress(), aClientAddress));
            }
        }
    }

exit:
    return error;
}

Error Server::AppendIaAddressOption(Message               &aMessage,
                                    const Ip6::Address    &aPrefix,
                                    const Mac::ExtAddress &aClientAddress)
{
    Error           error = kErrorNone;
    IaAddressOption option;

    option.Init();
    option.GetAddress().SetPrefix(aPrefix.mFields.m8, OT_IP6_PREFIX_BITSIZE);
    option.GetAddress().GetIid().SetFromExtAddress(aClientAddress);
    option.SetPreferredLifetime(IaAddressOption::kDefaultPreferredLifetime);
    option.SetValidLifetime(IaAddressOption::kDefaultValidLifetime);
    SuccessOrExit(error = aMessage.Append(option));

exit:
    return error;
}

} // namespace Dhcp6
} // namespace ot

#endif //  OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE
