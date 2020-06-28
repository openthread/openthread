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

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "thread/mle.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE

namespace ot {
namespace Dhcp6 {

Dhcp6Server::Dhcp6Server(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(Get<Ip6::Udp>())
    , mPrefixAgentsCount(0)
    , mPrefixAgentsMask(0)
{
    memset(mPrefixAgents, 0, sizeof(mPrefixAgents));
}

otError Dhcp6Server::UpdateService(void)
{
    otError                         error  = OT_ERROR_NONE;
    uint16_t                        rloc16 = Get<Mle::MleRouter>().GetRloc16();
    NetworkData::Iterator           iterator;
    NetworkData::OnMeshPrefixConfig config;
    Lowpan::Context                 lowpanContext;

    // remove dhcp agent aloc and prefix delegation
    for (size_t i = 0; i < OT_ARRAY_LENGTH(mPrefixAgents); i++)
    {
        bool found = false;

        if (!mPrefixAgents[i].IsValid())
        {
            continue;
        }

        iterator = NetworkData::kIteratorInit;

        while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, rloc16, config) == OT_ERROR_NONE)
        {
            if (!config.mDhcp)
            {
                continue;
            }

            error = Get<NetworkData::Leader>().GetContext(mPrefixAgents[i].GetPrefix(), lowpanContext);

            if ((error == OT_ERROR_NONE) && (mPrefixAgents[i].GetContextId() == lowpanContext.mContextId))
            {
                // still in network data
                found = true;
                break;
            }
        }

        if (!found)
        {
            mPrefixAgents[i].Clear();
            mPrefixAgentsCount--;
        }
    }

    // add dhcp agent aloc and prefix delegation
    iterator = NetworkData::kIteratorInit;

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, rloc16, config) == OT_ERROR_NONE)
    {
        if (!config.mDhcp)
        {
            continue;
        }

        error = Get<NetworkData::Leader>().GetContext(static_cast<const Ip6::Address &>(config.mPrefix.mPrefix),
                                                      lowpanContext);

        if (error == OT_ERROR_NONE)
        {
            AddPrefixAgent(config.mPrefix, lowpanContext);
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

    return error;
}

void Dhcp6Server::Start(void)
{
    Ip6::SockAddr sockaddr;

    sockaddr.mPort = kDhcpServerPort;
    IgnoreError(mSocket.Open(&Dhcp6Server::HandleUdpReceive, this));
    IgnoreError(mSocket.Bind(sockaddr));
}

void Dhcp6Server::Stop(void)
{
    IgnoreError(mSocket.Close());
}

void Dhcp6Server::AddPrefixAgent(const otIp6Prefix &aIp6Prefix, const Lowpan::Context &aContext)
{
    otError      error    = OT_ERROR_NONE;
    PrefixAgent *newEntry = nullptr;

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mPrefixAgents); i++)
    {
        if (!mPrefixAgents[i].IsValid())
        {
            newEntry = &mPrefixAgents[i];
        }
        else if (mPrefixAgents[i].IsPrefixMatch(aIp6Prefix))
        {
            // already added
            ExitNow();
        }
    }

    VerifyOrExit(newEntry != nullptr, error = OT_ERROR_NO_BUFS);

    newEntry->Set(aIp6Prefix, Get<Mle::MleRouter>().GetMeshLocalPrefix(), aContext.mContextId);
    Get<ThreadNetif>().AddUnicastAddress(newEntry->GetAloc());
    mPrefixAgentsCount++;

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogNoteIp6("Failed to add DHCPv6 prefix agent: %s", otThreadErrorToString(error));
    }
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

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(header), &header) == sizeof(header), OT_NOOP);
    aMessage.MoveOffset(sizeof(header));

    // discard if not solicit type
    VerifyOrExit((header.GetType() == kTypeSolicit), OT_NOOP);

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
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, kOptionClientIdentifier)) > 0, OT_NOOP);
    SuccessOrExit(ProcessClientIdentifier(aMessage, optionOffset, clientIdentifier));

    // Server Identifier (assuming Rapid Commit, discard if present)
    VerifyOrExit(FindOption(aMessage, offset, length, kOptionServerIdentifier) == 0, OT_NOOP);

    // Rapid Commit (assuming Rapid Commit, discard if not present)
    VerifyOrExit(FindOption(aMessage, offset, length, kOptionRapidCommit) > 0, OT_NOOP);

    // Elapsed Time if present
    if ((optionOffset = FindOption(aMessage, offset, length, kOptionElapsedTime)) > 0)
    {
        SuccessOrExit(ProcessElapsedTime(aMessage, optionOffset));
    }

    // IA_NA (discard if not present)
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, kOptionIaNa)) > 0, OT_NOOP);
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
        VerifyOrExit(aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option), OT_NOOP);

        if (option.GetCode() == aCode)
        {
            ExitNow(rval = aOffset);
        }

        aOffset += sizeof(option) + option.GetLength();
    }

exit:
    return rval;
}
otError Dhcp6Server::ProcessClientIdentifier(Message &aMessage, uint16_t aOffset, ClientIdentifier &aClientId)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(((aMessage.Read(aOffset, sizeof(aClientId), &aClientId) == sizeof(aClientId)) &&
                  (aClientId.GetLength() == (sizeof(aClientId) - sizeof(Dhcp6Option))) &&
                  (aClientId.GetDuidType() == kDuidLL) && (aClientId.GetDuidHardwareType() == kHardwareTypeEui64)),
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
        VerifyOrExit((optionOffset = FindOption(aMessage, aOffset, length, kOptionIaAddress)) > 0, OT_NOOP);
        SuccessOrExit(error = ProcessIaAddress(aMessage, optionOffset));

        length -= ((optionOffset - aOffset) + sizeof(IaAddress));
        aOffset = optionOffset + sizeof(IaAddress);
    }

exit:
    return error;
}

otError Dhcp6Server::ProcessIaAddress(Message &aMessage, uint16_t aOffset)
{
    otError   error = OT_ERROR_NONE;
    IaAddress option;

    VerifyOrExit(((aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option)) &&
                  option.GetLength() == (sizeof(option) - sizeof(Dhcp6Option))),
                 error = OT_ERROR_PARSE);

    // mask matching prefix
    for (size_t i = 0; i < OT_ARRAY_LENGTH(mPrefixAgents); i++)
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

otError Dhcp6Server::SendReply(otIp6Address &aDst, uint8_t *aTransactionId, ClientIdentifier &aClientId, IaNa &aIaNa)
{
    otError          error = OT_ERROR_NONE;
    Ip6::MessageInfo messageInfo;
    Message *        message;

    VerifyOrExit((message = mSocket.NewMessage(0)) != nullptr, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, aTransactionId));
    SuccessOrExit(error = AppendServerIdentifier(*message));
    SuccessOrExit(error = AppendClientIdentifier(*message, aClientId));
    SuccessOrExit(error = AppendIaNa(*message, aIaNa));
    SuccessOrExit(error = AppendStatusCode(*message, kStatusSuccess));
    SuccessOrExit(error = AppendIaAddress(*message, aClientId));
    SuccessOrExit(error = AppendRapidCommit(*message));

    memcpy(messageInfo.GetPeerAddr().mFields.m8, &aDst, sizeof(otIp6Address));
    messageInfo.mPeerPort = kDhcpClientPort;
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

exit:

    if (message != nullptr && error != OT_ERROR_NONE)
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

otError Dhcp6Server::AppendClientIdentifier(Message &aMessage, ClientIdentifier &aClientId)
{
    return aMessage.Append(&aClientId, sizeof(aClientId));
}

otError Dhcp6Server::AppendServerIdentifier(Message &aMessage)
{
    otError          error = OT_ERROR_NONE;
    ServerIdentifier option;
    Mac::ExtAddress  eui64;

    Get<Radio>().GetIeeeEui64(eui64);

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
        for (size_t i = 0; i < OT_ARRAY_LENGTH(mPrefixAgents); i++)
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

otError Dhcp6Server::AppendStatusCode(Message &aMessage, Status aStatusCode)
{
    StatusCode option;

    option.Init();
    option.SetStatusCode(aStatusCode);
    return aMessage.Append(&option, sizeof(option));
}

otError Dhcp6Server::AppendIaAddress(Message &aMessage, ClientIdentifier &aClientId)
{
    otError error = OT_ERROR_NONE;

    if (mPrefixAgentsMask)
    {
        // if specified, only apply specified prefixes
        for (size_t i = 0; i < OT_ARRAY_LENGTH(mPrefixAgents); i++)
        {
            if (mPrefixAgentsMask & (1 << i))
            {
                SuccessOrExit(error = AddIaAddress(aMessage, mPrefixAgents[i].GetPrefix(), aClientId));
            }
        }
    }
    else
    {
        // if not specified, apply all configured prefixes
        for (size_t i = 0; i < OT_ARRAY_LENGTH(mPrefixAgents); i++)
        {
            if (mPrefixAgents[i].IsValid())
            {
                SuccessOrExit(error = AddIaAddress(aMessage, mPrefixAgents[i].GetPrefix(), aClientId));
            }
        }
    }

exit:
    return error;
}

otError Dhcp6Server::AddIaAddress(Message &aMessage, const Ip6::Address &aPrefix, ClientIdentifier &aClientId)
{
    otError   error = OT_ERROR_NONE;
    IaAddress option;

    option.Init();
    option.GetAddress().SetPrefix(aPrefix.mFields.m8, OT_IP6_PREFIX_BITSIZE);
    option.GetAddress().SetIid(*reinterpret_cast<Mac::ExtAddress *>(aClientId.GetDuidLinkLayerAddress()));
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

void Dhcp6Server::ApplyMeshLocalPrefix(void)
{
    for (size_t i = 0; i < OT_ARRAY_LENGTH(mPrefixAgents); i++)
    {
        if (mPrefixAgents[i].IsValid())
        {
            PrefixAgent *entry = &mPrefixAgents[i];

            Get<ThreadNetif>().RemoveUnicastAddress(entry->GetAloc());
            entry->GetAloc().GetAddress().SetPrefix(Get<Mle::MleRouter>().GetMeshLocalPrefix());
            Get<ThreadNetif>().AddUnicastAddress(entry->GetAloc());
        }
    }
}

} // namespace Dhcp6
} // namespace ot

#endif //  OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE
