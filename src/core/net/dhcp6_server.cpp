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

#include "openthread/types.h"

#include <common/code_utils.hpp>
#include <common/encoding.hpp>
#include <common/logging.hpp>
#include <net/dhcp6_server.hpp>
#include <thread/mle.hpp>
#include <thread/thread_netif.hpp>

using Thread::Encoding::BigEndian::HostSwap16;
using Thread::Encoding::BigEndian::HostSwap32;

namespace Thread {
namespace Dhcp6 {

Dhcp6Server::Dhcp6Server(ThreadNetif &aThreadNetif):
    mSocket(aThreadNetif.GetIp6().mUdp),
    mNetif(aThreadNetif)
{
    for (uint8_t i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
    {
        memset(&(mPrefixAgents[i]), 0, sizeof(PrefixAgent));
        memset(&mAgentsAloc[i], 0, sizeof(mAgentsAloc[i]));
    }

    mPrefixAgentsCount = 0;
    mPrefixAgentsMask = 0;
}

ThreadError Dhcp6Server::UpdateService(void)
{
    ThreadError error = kThreadError_None;
    bool found;
    uint8_t i;
    uint16_t rloc16 = mNetif.GetMle().GetRloc16();
    Ip6::Address *address = NULL;
    otNetworkDataIterator iterator;
    otBorderRouterConfig config;
    Lowpan::Context lowpanContext;

    // remove dhcp agent aloc and prefix delegation
    for (i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
    {
        found = false;

        if (!mAgentsAloc[i].mValid)
        {
            continue;
        }

        address = &(mAgentsAloc[i].GetAddress());
        iterator = OT_NETWORK_DATA_ITERATOR_INIT;

        while (mNetif.GetNetworkDataLeader().GetNextOnMeshPrefix(&iterator, rloc16, &config) == kThreadError_None)
        {
            if (!config.mDhcp)
            {
                continue;
            }

            mNetif.GetNetworkDataLeader().GetContext(*static_cast<const Ip6::Address *>(&(config.mPrefix.mPrefix)),
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
            mNetif.GetNetworkDataLeader().GetContext(address->mFields.m8[15], lowpanContext);
            mNetif.RemoveUnicastAddress(mAgentsAloc[i]);
            mAgentsAloc[i].mValid = false;
            RemovePrefixAgent(lowpanContext.mPrefix);
        }
    }

    // add dhcp agent aloc and prefix delegation
    iterator = OT_NETWORK_DATA_ITERATOR_INIT;

    while (mNetif.GetNetworkDataLeader().GetNextOnMeshPrefix(&iterator, rloc16, &config) == kThreadError_None)
    {
        found = false;

        if (!config.mDhcp)
        {
            continue;
        }

        mNetif.GetNetworkDataLeader().GetContext(*static_cast<const Ip6::Address *>(&config.mPrefix.mPrefix),
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

        // alreay added
        if (found)
        {
            continue;
        }

        for (i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
        {
            if (!mAgentsAloc[i].mValid)
            {
                address = &(mAgentsAloc[i].GetAddress());
                memcpy(address, mNetif.GetMle().GetMeshLocalPrefix(), 8);
                address->mFields.m16[4] = HostSwap16(0x0000);
                address->mFields.m16[5] = HostSwap16(0x00ff);
                address->mFields.m16[6] = HostSwap16(0xfe00);
                address->mFields.m8[14] = Mle::kAloc16Mask;
                address->mFields.m8[15] = lowpanContext.mContextId;
                mAgentsAloc[i].mPrefixLength = 128;
                mAgentsAloc[i].mPreferred = true;
                mAgentsAloc[i].mValid = true;
                mNetif.AddUnicastAddress(mAgentsAloc[i]);
                AddPrefixAgent(config.mPrefix);
                break;
            }
        }

        // if no available Dhcp Agent Aloc resources
        if (i == OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES)
        {
            ExitNow(error = kThreadError_NoBufs);
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

ThreadError Dhcp6Server::Start(void)
{
    Ip6::SockAddr sockaddr;
    sockaddr.mPort = kDhcpServerPort;
    mSocket.Open(&Dhcp6Server::HandleUdpReceive, this);
    mSocket.Bind(sockaddr);

    return kThreadError_None;
}

ThreadError Dhcp6Server::Stop(void)
{
    mSocket.Close();
    return kThreadError_None;
}

ThreadError Dhcp6Server::AddPrefixAgent(otIp6Prefix &aIp6Prefix)
{
    ThreadError error = kThreadError_NoBufs;

    for (uint8_t i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
    {
        if (mPrefixAgents[i].GetPrefix()->mLength != 0)
        {
            continue;
        }

        mPrefixAgents[i].SetPrefix(aIp6Prefix);
        mPrefixAgentsCount++;
        ExitNow(error = kThreadError_None);
    }

exit:
    return error;
}

ThreadError Dhcp6Server::RemovePrefixAgent(const uint8_t *aIp6Address)
{
    ThreadError error = kThreadError_NotFound;
    otIp6Prefix *prefix = NULL;

    for (uint8_t i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
    {
        prefix = mPrefixAgents[i].GetPrefix();

        if (prefix->mLength == 0)
        {
            continue;
        }

        if (otIp6PrefixMatch(&(prefix->mPrefix), reinterpret_cast<const otIp6Address *>(aIp6Address)) >= prefix->mLength)
        {
            memset(&(mPrefixAgents[i]), 0, sizeof(PrefixAgent));
            mPrefixAgentsCount--;
            ExitNow(error = kThreadError_None);
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
    Dhcp6Header header;
    otIp6Address dst = aMessageInfo.mPeerAddr;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(header), &header) == sizeof(header),);
    aMessage.MoveOffset(sizeof(header));

    // discard if not solicit type
    VerifyOrExit((header.GetType() == kTypeSolicit), ;);

    ProcessSolicit(aMessage, dst, header.GetTransactionId());

exit:
    {}
}

void Dhcp6Server::ProcessSolicit(Message &aMessage, otIp6Address &aDst, uint8_t *aTransactionId)
{
    IaNa iana;
    ClientIdentifier clientIdentifier;
    uint16_t optionOffset;
    uint16_t offset = aMessage.GetOffset();
    uint16_t length = aMessage.GetLength() - aMessage.GetOffset();

    // Client Identifier (discard if not present)
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, kOptionClientIdentifier)) > 0, ;);
    SuccessOrExit(ProcessClientIdentifier(aMessage, optionOffset, clientIdentifier));

    // Server Identifier (assuming Rapid Commit, discard if present)
    VerifyOrExit(FindOption(aMessage, offset, length, kOptionServerIdentifier) == 0, ;);

    // Rapid Commit (assuming Rapid Commit, discard if not present)
    VerifyOrExit(FindOption(aMessage, offset, length, kOptionRapidCommit) > 0, ;);

    // Elapsed Time if present
    if ((optionOffset = FindOption(aMessage, offset, length, kOptionElapsedTime)) > 0)
    {
        SuccessOrExit(ProcessElapsedTime(aMessage, optionOffset));
    }

    // IA_NA (discard if not present)
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, kOptionIaNa)) > 0, ;);
    SuccessOrExit(ProcessIaNa(aMessage, optionOffset, iana));

    SuccessOrExit(SendReply(aDst, aTransactionId, clientIdentifier, iana));

exit:
    {}
}

uint16_t Dhcp6Server::FindOption(Message &aMessage, uint16_t aOffset, uint16_t aLength, Code aCode)
{
    uint16_t end = aOffset + aLength;

    while (aOffset <= end)
    {
        Dhcp6Option option;
        VerifyOrExit(aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option),);

        if (option.GetCode() == aCode)
        {
            return aOffset;
        }

        aOffset += sizeof(option) + option.GetLength();
    }

exit:
    return 0;
}
ThreadError Dhcp6Server::ProcessClientIdentifier(Message &aMessage, uint16_t aOffset, ClientIdentifier &aClient)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(((aMessage.Read(aOffset, sizeof(aClient), &aClient) == sizeof(aClient)) &&
                  (aClient.GetLength() == (sizeof(aClient) - sizeof(Dhcp6Option))) &&
                  (aClient.GetDuidType() == kDuidLL) &&
                  (aClient.GetDuidHardwareType() == kHardwareTypeEui64)),
                 error = kThreadError_Parse);
exit:
    return error;
}

ThreadError Dhcp6Server::ProcessElapsedTime(Message &aMessage, uint16_t aOffset)
{
    ThreadError error = kThreadError_None;
    ElapsedTime option;

    VerifyOrExit(((aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option)) &&
                  (option.GetLength() == ((sizeof(option) - sizeof(Dhcp6Option))))),
                 error = kThreadError_Parse);
exit:
    return error;
}

ThreadError Dhcp6Server::ProcessIaNa(Message &aMessage, uint16_t aOffset, IaNa &aIaNa)
{
    ThreadError error = kThreadError_None;
    uint16_t optionOffset;
    uint16_t length;

    VerifyOrExit((aMessage.Read(aOffset, sizeof(aIaNa), &aIaNa) == sizeof(aIaNa)), error = kThreadError_Parse);

    aOffset += sizeof(aIaNa);
    length = aIaNa.GetLength() + sizeof(Dhcp6Option) - sizeof(IaNa);

    VerifyOrExit(length <= aMessage.GetLength() - aOffset, error = kThreadError_Parse);

    mPrefixAgentsMask = 0;

    while (length > 0)
    {
        VerifyOrExit((optionOffset = FindOption(aMessage, aOffset, length, kOptionIaAddress)) > 0, ;);
        SuccessOrExit(error = ProcessIaAddress(aMessage, optionOffset));

        length -= ((optionOffset - aOffset) + sizeof(IaAddress));
        aOffset = optionOffset + sizeof(IaAddress);
    }

exit:
    return error;
}

ThreadError Dhcp6Server::ProcessIaAddress(Message &aMessage, uint16_t aOffset)
{
    ThreadError error = kThreadError_None;
    otIp6Prefix *prefix = NULL;
    IaAddress option;

    VerifyOrExit(((aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option)) &&
                  option.GetLength() == (sizeof(option) - sizeof(Dhcp6Option))),
                 error = kThreadError_Parse);

    // mask matching prefix
    for (uint8_t i = 0; i < OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES; i++)
    {
        prefix = mPrefixAgents[i].GetPrefix();

        if (prefix->mLength == 0)
        {
            continue;
        }

        if (otIp6PrefixMatch(option.GetAddress(), &(prefix->mPrefix)) >= prefix->mLength)
        {
            mPrefixAgentsMask |= (1 << i);;
            break;
        }
    }

exit:
    return error;
}

ThreadError Dhcp6Server::SendReply(otIp6Address &aDst, uint8_t *aTransactionId, ClientIdentifier &aClient, IaNa &aIaNa)
{
    ThreadError error = kThreadError_None;
    Ip6::MessageInfo messageInfo;
    Message *message;

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = AppendHeader(*message, aTransactionId));
    SuccessOrExit(error = AppendServerIdentifier(*message));
    SuccessOrExit(error = AppendClientIdentifier(*message, aClient));
    SuccessOrExit(error = AppendIaNa(*message, aIaNa));
    SuccessOrExit(error = AppendStatusCode(*message, kStatusSuccess));
    SuccessOrExit(error = AppendIaAddress(*message, aClient));
    SuccessOrExit(error = AppendRapidCommit(*message));

    memset(&messageInfo, 0, sizeof(messageInfo));
    memcpy(&messageInfo.GetPeerAddr().mFields.m8, &aDst, sizeof(otIp6Address));
    messageInfo.mPeerPort = kDhcpClientPort;
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

exit:

    if (message != NULL && error != kThreadError_None)
    {
        message->Free();
    }

    return error;
}

ThreadError Dhcp6Server::AppendHeader(Message &aMessage, uint8_t *aTransactionId)
{
    Dhcp6Header header;

    header.Init();
    header.SetType(kTypeReply);
    header.SetTransactionId(aTransactionId);
    return aMessage.Append(&header, sizeof(header));
}

ThreadError Dhcp6Server::AppendClientIdentifier(Message &aMessage, ClientIdentifier &aClient)
{
    return aMessage.Append(&aClient, sizeof(aClient));
}

ThreadError Dhcp6Server::AppendServerIdentifier(Message &aMessage)
{
    ThreadError error = kThreadError_None;
    ServerIdentifier option;

    option.Init();
    option.SetDuidType(kDuidLL);
    option.SetDuidHardwareType(kHardwareTypeEui64);
    option.SetDuidLinkLayerAddress(mNetif.GetMac().GetExtAddress());
    SuccessOrExit(error = aMessage.Append(&option, sizeof(option)));

exit:
    return error;
}

ThreadError Dhcp6Server::AppendIaNa(Message &aMessage, IaNa &aIaNa)
{
    ThreadError error = kThreadError_None;
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

ThreadError Dhcp6Server::AppendStatusCode(Message &aMessage, Status aStatus)
{
    StatusCode option;

    option.Init();
    option.SetStatusCode(aStatus);
    return aMessage.Append(&option, sizeof(option));
}

ThreadError Dhcp6Server::AppendIaAddress(Message &aMessage, ClientIdentifier &aClient)
{
    ThreadError error = kThreadError_None;
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
    else  // if not specified, apply all configured prefixes
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

ThreadError Dhcp6Server::AddIaAddress(Message &aMessage, otIp6Prefix &aIp6Prefix, ClientIdentifier &aClient)
{
    ThreadError error = kThreadError_None;
    IaAddress option;

    option.Init();
    memcpy((option.GetAddress()->mFields.m8), &(aIp6Prefix.mPrefix), 8);
    memcpy(&(option.GetAddress()->mFields.m8[8]), aClient.GetDuidLinkLayerAddress(), sizeof(Mac::ExtAddress));
    option.SetPreferredLifetime(OT_DHCP6_DEFAULT_PREFERRED_LIFETIME);
    option.SetValidLifetime(OT_DHCP6_DEFAULT_VALID_LIFETIME);
    SuccessOrExit(error = aMessage.Append(&option, sizeof(option)));
exit:
    return error;
}

ThreadError Dhcp6Server::AppendRapidCommit(Message &aMessage)
{
    RapidCommit option;

    option.Init();
    return aMessage.Append(&option, sizeof(option));
}

}  // namespace Dhcp6
}  // namespace Thread
