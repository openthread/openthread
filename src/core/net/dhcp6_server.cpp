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
    IaNaOption     iana;
    ClientIdOption clientIdentifier;
    uint16_t       optionOffset;
    uint16_t       offset = aMessage.GetOffset();
    uint16_t       length = aMessage.GetLength() - aMessage.GetOffset();

    // Client Identifier (discard if not present)
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, Option::kClientId)) > 0);
    SuccessOrExit(ProcessClientIdOption(aMessage, optionOffset, clientIdentifier));

    // Server Identifier (assuming Rapid Commit, discard if present)
    VerifyOrExit(FindOption(aMessage, offset, length, Option::kServerId) == 0);

    // Rapid Commit (assuming Rapid Commit, discard if not present)
    VerifyOrExit(FindOption(aMessage, offset, length, Option::kRapidCommit) > 0);

    // Elapsed Time if present
    if ((optionOffset = FindOption(aMessage, offset, length, Option::kElapsedTime)) > 0)
    {
        SuccessOrExit(ProcessElapsedTimeOption(aMessage, optionOffset));
    }

    // IA_NA (discard if not present)
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, Option::kIaNa)) > 0);
    SuccessOrExit(ProcessIaNaOption(aMessage, optionOffset, iana));

    SuccessOrExit(SendReply(aDst, aTransactionId, clientIdentifier, iana));

exit:
    return;
}

uint16_t Server::FindOption(Message &aMessage, uint16_t aOffset, uint16_t aLength, Option::Code aCode)
{
    uint16_t end  = aOffset + aLength;
    uint16_t rval = 0;

    while (aOffset <= end)
    {
        Option option;

        SuccessOrExit(aMessage.Read(aOffset, option));

        if (option.GetCode() == aCode)
        {
            ExitNow(rval = aOffset);
        }

        aOffset += sizeof(option) + option.GetLength();
    }

exit:
    return rval;
}
Error Server::ProcessClientIdOption(Message &aMessage, uint16_t aOffset, ClientIdOption &aClientIdOption)
{
    Error error = kErrorNone;

    SuccessOrExit(error = aMessage.Read(aOffset, aClientIdOption));
    VerifyOrExit((aClientIdOption.GetLength() == sizeof(aClientIdOption) - sizeof(Option)) &&
                     (aClientIdOption.GetDuidType() == kDuidLinkLayerAddress) &&
                     (aClientIdOption.GetDuidHardwareType() == kHardwareTypeEui64),
                 error = kErrorParse);
exit:
    return error;
}

Error Server::ProcessElapsedTimeOption(Message &aMessage, uint16_t aOffset)
{
    Error             error = kErrorNone;
    ElapsedTimeOption option;

    SuccessOrExit(error = aMessage.Read(aOffset, option));
    VerifyOrExit(option.GetLength() == sizeof(option) - sizeof(Option), error = kErrorParse);
exit:
    return error;
}

Error Server::ProcessIaNaOption(Message &aMessage, uint16_t aOffset, IaNaOption &aIaNaOption)
{
    Error    error = kErrorNone;
    uint16_t optionOffset;
    uint16_t length;

    SuccessOrExit(error = aMessage.Read(aOffset, aIaNaOption));

    aOffset += sizeof(aIaNaOption);
    length = aIaNaOption.GetLength() + sizeof(Option) - sizeof(IaNaOption);

    VerifyOrExit(length <= aMessage.GetLength() - aOffset, error = kErrorParse);

    mPrefixAgentsMask = 0;

    while (length > 0)
    {
        VerifyOrExit((optionOffset = FindOption(aMessage, aOffset, length, Option::kIaAddress)) > 0);
        SuccessOrExit(error = ProcessIaAddressOption(aMessage, optionOffset));

        length -= ((optionOffset - aOffset) + sizeof(IaAddressOption));
        aOffset = optionOffset + sizeof(IaAddressOption);
    }

exit:
    return error;
}

Error Server::ProcessIaAddressOption(Message &aMessage, uint16_t aOffset)
{
    Error           error = kErrorNone;
    IaAddressOption option;

    SuccessOrExit(error = aMessage.Read(aOffset, option));
    VerifyOrExit(option.GetLength() == sizeof(option) - sizeof(Option), error = kErrorParse);

    // mask matching prefix
    for (uint16_t i = 0; i < GetArrayLength(mPrefixAgents); i++)
    {
        if (mPrefixAgents[i].IsValid() && mPrefixAgents[i].IsPrefixMatch(option.GetAddress()))
        {
            mPrefixAgentsMask |= (1 << i);
            break;
        }
    }

exit:
    return error;
}

Error Server::SendReply(const Ip6::Address  &aDst,
                        const TransactionId &aTransactionId,
                        ClientIdOption      &aClientIdOption,
                        IaNaOption          &aIaNaOption)
{
    Error            error = kErrorNone;
    Ip6::MessageInfo messageInfo;
    Message         *message;

    VerifyOrExit((message = mSocket.NewMessage()) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = AppendHeader(*message, aTransactionId));
    SuccessOrExit(error = AppendServerIdOption(*message));
    SuccessOrExit(error = AppendClientIdOption(*message, aClientIdOption));
    SuccessOrExit(error = AppendIaNaOption(*message, aIaNaOption));
    SuccessOrExit(error = AppendStatusCodeOption(*message, StatusCodeOption::kSuccess));
    SuccessOrExit(error = AppendIaAddressOption(*message, aClientIdOption));
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

Error Server::AppendClientIdOption(Message &aMessage, ClientIdOption &aClientIdOption)
{
    return aMessage.Append(aClientIdOption);
}

Error Server::AppendServerIdOption(Message &aMessage)
{
    Error           error = kErrorNone;
    ServerIdOption  option;
    Mac::ExtAddress eui64;

    Get<Radio>().GetIeeeEui64(eui64);

    option.Init();
    option.SetDuidType(kDuidLinkLayerAddress);
    option.SetDuidHardwareType(kHardwareTypeEui64);
    option.SetDuidLinkLayerAddress(eui64);
    SuccessOrExit(error = aMessage.Append(option));

exit:
    return error;
}

Error Server::AppendIaNaOption(Message &aMessage, IaNaOption &aIaNaOption)
{
    Error    error  = kErrorNone;
    uint16_t length = 0;

    if (mPrefixAgentsMask)
    {
        for (uint16_t i = 0; i < GetArrayLength(mPrefixAgents); i++)
        {
            if (mPrefixAgentsMask & (1 << i))
            {
                length += sizeof(IaAddressOption);
            }
        }
    }
    else
    {
        length += sizeof(IaAddressOption) * mPrefixAgentsCount;
    }

    length += sizeof(IaNaOption) + sizeof(StatusCodeOption) - sizeof(Option);

    aIaNaOption.SetLength(length);
    aIaNaOption.SetT1(IaNaOption::kDefaultT1);
    aIaNaOption.SetT2(IaNaOption::kDefaultT2);
    SuccessOrExit(error = aMessage.Append(aIaNaOption));

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

Error Server::AppendIaAddressOption(Message &aMessage, ClientIdOption &aClientIdOption)
{
    Error error = kErrorNone;

    if (mPrefixAgentsMask)
    {
        // if specified, only apply specified prefixes
        for (uint16_t i = 0; i < GetArrayLength(mPrefixAgents); i++)
        {
            if (mPrefixAgentsMask & (1 << i))
            {
                SuccessOrExit(error =
                                  AddIaAddressOption(aMessage, mPrefixAgents[i].GetPrefixAsAddress(), aClientIdOption));
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
                SuccessOrExit(error = AddIaAddressOption(aMessage, prefixAgent.GetPrefixAsAddress(), aClientIdOption));
            }
        }
    }

exit:
    return error;
}

Error Server::AddIaAddressOption(Message &aMessage, const Ip6::Address &aPrefix, ClientIdOption &aClientIdOption)
{
    Error           error = kErrorNone;
    IaAddressOption option;

    option.Init();
    option.GetAddress().SetPrefix(aPrefix.mFields.m8, OT_IP6_PREFIX_BITSIZE);
    option.GetAddress().GetIid().SetFromExtAddress(aClientIdOption.GetDuidLinkLayerAddress());
    option.SetPreferredLifetime(IaAddressOption::kDefaultPreferredLifetime);
    option.SetValidLifetime(IaAddressOption::kDefaultValidLifetime);
    SuccessOrExit(error = aMessage.Append(option));

exit:
    return error;
}

Error Server::AppendRapidCommitOption(Message &aMessage)
{
    RapidCommitOption option;

    option.Init();
    return aMessage.Append(option);
}

} // namespace Dhcp6
} // namespace ot

#endif //  OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE
