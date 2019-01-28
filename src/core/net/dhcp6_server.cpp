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

#define WPP_NAME "dhcp6_server.tmh"

#include "dhcp6_server.hpp"

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "thread/mle.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_ENABLE_DHCP6_SERVER

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {
namespace Dhcp6 {

Dhcp6Server::Dhcp6Server(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(GetNetif().GetIp6().GetUdp())
{
    for (uint8_t i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
    {
        memset(&(mPrefixAgents[i]), 0, sizeof(PrefixAgent));
        memset(&mAgentsAloc[i], 0, sizeof(mAgentsAloc[i]));
    }

    mPrefixAgentsCount = 0;
    mPrefixAgentsMask  = 0;
}

otError Dhcp6Server::UpdateService(void)
{
    ThreadNetif &         netif = GetNetif();
    otError               error = OT_ERROR_NONE;
    bool                  found;
    uint8_t               i;
    uint16_t              rloc16  = netif.GetMle().GetRloc16();
    Ip6::Address *        address = NULL;
    otNetworkDataIterator iterator;
    otBorderRouterConfig  config;
    Lowpan::Context       lowpanContext;

    // remove dhcp agent aloc and prefix delegation
    for (i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
    {
        found = false;

        if (!mAgentsAloc[i].mValid)
        {
            continue;
        }

        address  = &(mAgentsAloc[i].GetAddress());
        iterator = OT_NETWORK_DATA_ITERATOR_INIT;

        while (netif.GetNetworkDataLeader().GetNextOnMeshPrefix(&iterator, rloc16, &config) == OT_ERROR_NONE)
        {
            if (!config.mDhcp)
            {
                continue;
            }

            netif.GetNetworkDataLeader().GetContext(*static_cast<const Ip6::Address *>(&(config.mPrefix.mPrefix)),
                                                    lowpanContext);

            if (address->mFields.m8[15] == lowpanContext.mContextId)
            {
                // still in network data
                found = true;
                break;
            }
        }

        if (!found)
        {
            netif.GetNetworkDataLeader().GetContext(address->mFields.m8[15], lowpanContext);
            netif.RemoveUnicastAddress(mAgentsAloc[i]);
            mAgentsAloc[i].mValid = false;
            RemovePrefixAgent(lowpanContext.mPrefix);
        }
    }

    // add dhcp agent aloc and prefix delegation
    iterator = OT_NETWORK_DATA_ITERATOR_INIT;

    while (netif.GetNetworkDataLeader().GetNextOnMeshPrefix(&iterator, rloc16, &config) == OT_ERROR_NONE)
    {
        found = false;

        if (!config.mDhcp)
        {
            continue;
        }

        netif.GetNetworkDataLeader().GetContext(*static_cast<const Ip6::Address *>(&config.mPrefix.mPrefix),
                                                lowpanContext);

        for (i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
        {
            address = &(mAgentsAloc[i].GetAddress());

            if ((mAgentsAloc[i].mValid) && (address->mFields.m8[15] == lowpanContext.mContextId))
            {
                found = true;
                break;
            }
        }

        // already added
        if (found)
        {
            continue;
        }

        for (i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
        {
            if (!mAgentsAloc[i].mValid)
            {
                address = &(mAgentsAloc[i].GetAddress());
                memcpy(address, netif.GetMle().GetMeshLocalPrefix().m8, sizeof(otMeshLocalPrefix));
                address->mFields.m16[4]      = HostSwap16(0x0000);
                address->mFields.m16[5]      = HostSwap16(0x00ff);
                address->mFields.m16[6]      = HostSwap16(0xfe00);
                address->mFields.m8[14]      = Ip6::Address::kAloc16Mask;
                address->mFields.m8[15]      = lowpanContext.mContextId;
                mAgentsAloc[i].mPrefixLength = 64;
                mAgentsAloc[i].mPreferred    = true;
                mAgentsAloc[i].mValid        = true;
                netif.AddUnicastAddress(mAgentsAloc[i]);
                AddPrefixAgent(config.mPrefix);
                break;
            }
        }

        // if no available Dhcp Agent Aloc resources
        if (i == OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES)
        {
            ExitNow(error = OT_ERROR_NO_BUFS);
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

exit:
    return error;
}

otError Dhcp6Server::Start(void)
{
    Ip6::SockAddr sockaddr;
    sockaddr.mPort = kDhcpServerPort;
    mSocket.Open(&Dhcp6Server::HandleUdpReceive, this);
    mSocket.Bind(sockaddr);

    return OT_ERROR_NONE;
}

otError Dhcp6Server::Stop(void)
{
    mSocket.Close();
    return OT_ERROR_NONE;
}

otError Dhcp6Server::AddPrefixAgent(otIp6Prefix &aIp6Prefix)
{
    otError error = OT_ERROR_NO_BUFS;

    for (uint8_t i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
    {
        if (mPrefixAgents[i].GetPrefix()->mLength != 0)
        {
            continue;
        }

        mPrefixAgents[i].SetPrefix(aIp6Prefix);
        mPrefixAgentsCount++;
        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    return error;
}

otError Dhcp6Server::RemovePrefixAgent(const uint8_t *aIp6Address)
{
    otError      error  = OT_ERROR_NOT_FOUND;
    otIp6Prefix *prefix = NULL;

    for (uint8_t i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
    {
        prefix = mPrefixAgents[i].GetPrefix();

        if (prefix->mLength == 0)
        {
            continue;
        }

        if (otIp6PrefixMatch(&(prefix->mPrefix), reinterpret_cast<const otIp6Address *>(aIp6Address)) >=
            prefix->mLength)
        {
            memset(&(mPrefixAgents[i]), 0, sizeof(PrefixAgent));
            mPrefixAgentsCount--;
            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

void Dhcp6Server::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    Dhcp6Server *obj = static_cast<Dhcp6Server *>(aContext);
    obj->HandleUdpReceive(*static_cast<Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Dhcp6Server::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Dhcp6Header  header;
    otIp6Address dst = aMessageInfo.mPeerAddr;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(header), &header) == sizeof(header));
    aMessage.MoveOffset(sizeof(header));

    // discard if not solicit type
    VerifyOrExit((header.GetType() == kTypeSolicit));

    ProcessSolicit(aMessage, dst, header.GetTransactionId());

exit:
    return;
}

void Dhcp6Server::ProcessSolicit(Message &aMessage, otIp6Address &aDst, uint8_t *aTransactionId)
{
    IaNa             iana;
    ClientIdentifier clientIdentifier;
    uint16_t         optionOffset;
    uint16_t         offset = aMessage.GetOffset();
    uint16_t         length = aMessage.GetLength() - aMessage.GetOffset();

    // Client Identifier (discard if not present)
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, kOptionClientIdentifier)) > 0);
    SuccessOrExit(ProcessClientIdentifier(aMessage, optionOffset, clientIdentifier));

    // Server Identifier (assuming Rapid Commit, discard if present)
    VerifyOrExit(FindOption(aMessage, offset, length, kOptionServerIdentifier) == 0);

    // Rapid Commit (assuming Rapid Commit, discard if not present)
    VerifyOrExit(FindOption(aMessage, offset, length, kOptionRapidCommit) > 0);

    // Elapsed Time if present
    if ((optionOffset = FindOption(aMessage, offset, length, kOptionElapsedTime)) > 0)
    {
        SuccessOrExit(ProcessElapsedTime(aMessage, optionOffset));
    }

    // IA_NA (discard if not present)
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, kOptionIaNa)) > 0);
    SuccessOrExit(ProcessIaNa(aMessage, optionOffset, iana));

    SuccessOrExit(SendReply(aDst, aTransactionId, clientIdentifier, iana));

exit:
    return;
}

uint16_t Dhcp6Server::FindOption(Message &aMessage, uint16_t aOffset, uint16_t aLength, Code aCode)
{
    uint16_t end  = aOffset + aLength;
    uint16_t rval = 0;

    while (aOffset <= end)
    {
        Dhcp6Option option;
        VerifyOrExit(aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option));

        if (option.GetCode() == aCode)
        {
            ExitNow(rval = aOffset);
        }

        aOffset += sizeof(option) + option.GetLength();
    }

exit:
    return rval;
}
otError Dhcp6Server::ProcessClientIdentifier(Message &aMessage, uint16_t aOffset, ClientIdentifier &aClient)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(((aMessage.Read(aOffset, sizeof(aClient), &aClient) == sizeof(aClient)) &&
                  (aClient.GetLength() == (sizeof(aClient) - sizeof(Dhcp6Option))) &&
                  (aClient.GetDuidType() == kDuidLL) && (aClient.GetDuidHardwareType() == kHardwareTypeEui64)),
                 error = OT_ERROR_PARSE);
exit:
    return error;
}

otError Dhcp6Server::ProcessElapsedTime(Message &aMessage, uint16_t aOffset)
{
    otError     error = OT_ERROR_NONE;
    ElapsedTime option;

    VerifyOrExit(((aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option)) &&
                  (option.GetLength() == ((sizeof(option) - sizeof(Dhcp6Option))))),
                 error = OT_ERROR_PARSE);
exit:
    return error;
}

otError Dhcp6Server::ProcessIaNa(Message &aMessage, uint16_t aOffset, IaNa &aIaNa)
{
    otError  error = OT_ERROR_NONE;
    uint16_t optionOffset;
    uint16_t length;

    VerifyOrExit((aMessage.Read(aOffset, sizeof(aIaNa), &aIaNa) == sizeof(aIaNa)), error = OT_ERROR_PARSE);

    aOffset += sizeof(aIaNa);
    length = aIaNa.GetLength() + sizeof(Dhcp6Option) - sizeof(IaNa);

    VerifyOrExit(length <= aMessage.GetLength() - aOffset, error = OT_ERROR_PARSE);

    mPrefixAgentsMask = 0;

    while (length > 0)
    {
        VerifyOrExit((optionOffset = FindOption(aMessage, aOffset, length, kOptionIaAddress)) > 0);
        SuccessOrExit(error = ProcessIaAddress(aMessage, optionOffset));

        length -= ((optionOffset - aOffset) + sizeof(IaAddress));
        aOffset = optionOffset + sizeof(IaAddress);
    }

exit:
    return error;
}

otError Dhcp6Server::ProcessIaAddress(Message &aMessage, uint16_t aOffset)
{
    otError      error  = OT_ERROR_NONE;
    otIp6Prefix *prefix = NULL;
    IaAddress    option;

    VerifyOrExit(((aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option)) &&
                  option.GetLength() == (sizeof(option) - sizeof(Dhcp6Option))),
                 error = OT_ERROR_PARSE);

    // mask matching prefix
    for (uint8_t i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
    {
        prefix = mPrefixAgents[i].GetPrefix();

        if (prefix->mLength == 0)
        {
            continue;
        }

        if (otIp6PrefixMatch(&option.GetAddress(), &prefix->mPrefix) >= prefix->mLength)
        {
            mPrefixAgentsMask |= (1 << i);
            break;
        }
    }

exit:
    return error;
}

otError Dhcp6Server::SendReply(otIp6Address &aDst, uint8_t *aTransactionId, ClientIdentifier &aClient, IaNa &aIaNa)
{
    otError          error = OT_ERROR_NONE;
    Ip6::MessageInfo messageInfo;
    Message *        message;

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, aTransactionId));
    SuccessOrExit(error = AppendServerIdentifier(*message));
    SuccessOrExit(error = AppendClientIdentifier(*message, aClient));
    SuccessOrExit(error = AppendIaNa(*message, aIaNa));
    SuccessOrExit(error = AppendStatusCode(*message, kStatusSuccess));
    SuccessOrExit(error = AppendIaAddress(*message, aClient));
    SuccessOrExit(error = AppendRapidCommit(*message));

    memcpy(messageInfo.GetPeerAddr().mFields.m8, &aDst, sizeof(otIp6Address));
    messageInfo.mPeerPort = kDhcpClientPort;
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

exit:

    if (message != NULL && error != OT_ERROR_NONE)
    {
        message->Free();
    }

    return error;
}

otError Dhcp6Server::AppendHeader(Message &aMessage, uint8_t *aTransactionId)
{
    Dhcp6Header header;

    header.Init();
    header.SetType(kTypeReply);
    header.SetTransactionId(aTransactionId);
    return aMessage.Append(&header, sizeof(header));
}

otError Dhcp6Server::AppendClientIdentifier(Message &aMessage, ClientIdentifier &aClient)
{
    return aMessage.Append(&aClient, sizeof(aClient));
}

otError Dhcp6Server::AppendServerIdentifier(Message &aMessage)
{
    otError          error = OT_ERROR_NONE;
    ServerIdentifier option;
    Mac::ExtAddress  eui64;

    otPlatRadioGetIeeeEui64(&GetInstance(), eui64.m8);

    option.Init();
    option.SetDuidType(kDuidLL);
    option.SetDuidHardwareType(kHardwareTypeEui64);
    option.SetDuidLinkLayerAddress(eui64);
    SuccessOrExit(error = aMessage.Append(&option, sizeof(option)));

exit:
    return error;
}

otError Dhcp6Server::AppendIaNa(Message &aMessage, IaNa &aIaNa)
{
    otError  error  = OT_ERROR_NONE;
    uint16_t length = 0;

    if (mPrefixAgentsMask)
    {
        for (uint8_t i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
        {
            if ((mPrefixAgentsMask & (1 << i)))
            {
                length += sizeof(IaAddress);
            }
        }
    }
    else
    {
        length += sizeof(IaAddress) * mPrefixAgentsCount;
    }

    length += sizeof(IaNa) + sizeof(StatusCode) - sizeof(Dhcp6Option);

    aIaNa.SetLength(length);
    aIaNa.SetT1(OT_DHCP6_DEFAULT_IA_NA_T1);
    aIaNa.SetT2(OT_DHCP6_DEFAULT_IA_NA_T2);
    SuccessOrExit(error = aMessage.Append(&aIaNa, sizeof(IaNa)));

exit:
    return error;
}

otError Dhcp6Server::AppendStatusCode(Message &aMessage, Status aStatus)
{
    StatusCode option;

    option.Init();
    option.SetStatusCode(aStatus);
    return aMessage.Append(&option, sizeof(option));
}

otError Dhcp6Server::AppendIaAddress(Message &aMessage, ClientIdentifier &aClient)
{
    otError      error  = OT_ERROR_NONE;
    otIp6Prefix *prefix = NULL;

    // if specified, only apply specified prefixes
    if (mPrefixAgentsMask)
    {
        for (uint8_t i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
        {
            if (mPrefixAgentsMask & (1 << i))
            {
                prefix = mPrefixAgents[i].GetPrefix();
                SuccessOrExit(error = AddIaAddress(aMessage, *prefix, aClient));
            }
        }
    }
    else // if not specified, apply all configured prefixes
    {
        for (uint8_t i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
        {
            prefix = mPrefixAgents[i].GetPrefix();

            if (prefix->mLength == 0)
            {
                continue;
            }

            SuccessOrExit(error = AddIaAddress(aMessage, *prefix, aClient));
        }
    }

exit:
    return error;
}

otError Dhcp6Server::AddIaAddress(Message &aMessage, otIp6Prefix &aIp6Prefix, ClientIdentifier &aClient)
{
    otError   error = OT_ERROR_NONE;
    IaAddress option;

    option.Init();
    memcpy(option.GetAddress().mFields.m8, &aIp6Prefix.mPrefix, 8);
    option.GetAddress().SetIid(*reinterpret_cast<Mac::ExtAddress *>(aClient.GetDuidLinkLayerAddress()));
    option.SetPreferredLifetime(OT_DHCP6_DEFAULT_PREFERRED_LIFETIME);
    option.SetValidLifetime(OT_DHCP6_DEFAULT_VALID_LIFETIME);
    SuccessOrExit(error = aMessage.Append(&option, sizeof(option)));
exit:
    return error;
}

otError Dhcp6Server::AppendRapidCommit(Message &aMessage)
{
    RapidCommit option;

    option.Init();
    return aMessage.Append(&option, sizeof(option));
}

} // namespace Dhcp6
} // namespace ot

#endif //  OPENTHREAD_ENABLE_DHCP6_SERVER
