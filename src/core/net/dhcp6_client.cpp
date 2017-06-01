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

#define WPP_NAME "dhcp6_client.tmh"

#include  "openthread/openthread_enable_defines.h"

#include "dhcp6_client.hpp"

#include <openthread/types.h>
#include <openthread/platform/random.h>

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/logging.hpp"
#include "mac/mac.hpp"
#include "net/dhcp6.hpp"
#include "thread/thread_netif.hpp"


#if OPENTHREAD_ENABLE_DHCP6_CLIENT

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {

namespace Dhcp6 {

Dhcp6Client::Dhcp6Client(ThreadNetif &aThreadNetif) :
    mTrickleTimer(aThreadNetif.GetIp6().mTimerScheduler, &Dhcp6Client::HandleTrickleTimer, NULL, this),
    mSocket(aThreadNetif.GetIp6().mUdp),
    mNetif(aThreadNetif),
    mStartTime(0),
    mAddresses(NULL),
    mNumAddresses(0)
{
    memset(mIdentityAssociations, 0, sizeof(IdentityAssociation));

    for (uint8_t i = 0; i < (OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES - 1); i++)
    {
        mIdentityAssociations[i].SetNext(&(mIdentityAssociations[i + 1]));
    }

    mIdentityAssociationHead = NULL;
    mIdentityAssociationAvail = &mIdentityAssociations[0];
}

otInstance *Dhcp6Client::GetInstance(void)
{
    return mNetif.GetInstance();
}

void Dhcp6Client::UpdateAddresses(otInstance *aInstance, otDhcpAddress *aAddresses, uint32_t aNumAddresses,
                                  void *aContext)
{
    (void)aContext;
    bool found = false;
    bool newAgent = false;
    otDhcpAddress *address = NULL;
    otNetworkDataIterator iterator;
    otBorderRouterConfig config;

    mAddresses = aAddresses;
    mNumAddresses = aNumAddresses;

    // remove addresses directly if prefix not valid in network data
    for (uint8_t i = 0; i < mNumAddresses; i++)
    {
        address = &mAddresses[i];

        if (address->mValidLifetime == 0)
        {
            continue;
        }

        found = false;
        iterator = OT_NETWORK_DATA_ITERATOR_INIT;

        while ((otNetDataGetNextPrefixInfo(aInstance, false, &iterator, &config)) == OT_ERROR_NONE)
        {
            if (!config.mDhcp)
            {
                continue;
            }

            if ((otIp6PrefixMatch(&(address->mAddress.mAddress), &(config.mPrefix.mPrefix)) >=
                 address->mAddress.mPrefixLength) &&
                (config.mPrefix.mLength == address->mAddress.mPrefixLength))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            otIp6RemoveUnicastAddress(aInstance, &(address->mAddress.mAddress));
            RemoveIdentityAssociation(config.mRloc16, config.mPrefix);
            memset(address, 0, sizeof(*address));
        }
    }

    // add IdentityAssociation for new configured prefix
    iterator = OT_NETWORK_DATA_ITERATOR_INIT;

    while (otNetDataGetNextPrefixInfo(aInstance, false, &iterator, &config) == OT_ERROR_NONE)
    {
        if (!config.mDhcp)
        {
            continue;
        }

        found = false;

        for (uint8_t i = 0; i < mNumAddresses; i++)
        {
            address = &mAddresses[i];

            if (address->mAddress.mPrefixLength == 0)
            {
                continue;
            }

            if ((otIp6PrefixMatch(&(config.mPrefix.mPrefix), &(address->mAddress.mAddress)) >=
                 config.mPrefix.mLength) &&
                (config.mPrefix.mLength == address->mAddress.mPrefixLength))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            for (size_t i = 0; i < mNumAddresses; i++)
            {
                address = &mAddresses[i];

                if (address->mAddress.mPrefixLength != 0)
                {
                    continue;
                }

                memset(address, 0, sizeof(*address));

                // suppose all configured prefix are ::/64
                memcpy(address->mAddress.mAddress.mFields.m8, config.mPrefix.mPrefix.mFields.m8, 8);
                address->mAddress.mPrefixLength = config.mPrefix.mLength;

                AddIdentityAssociation(config.mRloc16, config.mPrefix);
                newAgent = true;
                break;
            }
        }
    }

    if (newAgent)
    {
        Start();
    }
    else
    {
        Stop();
    }
}

void Dhcp6Client::AddIdentityAssociation(uint16_t aRloc16, otIp6Prefix &aIp6Prefix)
{
    IdentityAssociation *identityAssociation = NULL;
    IdentityAssociation *identityAssociationCursor = NULL;

    VerifyOrExit(mIdentityAssociationAvail);

    identityAssociation = mIdentityAssociationAvail;
    mIdentityAssociationAvail = mIdentityAssociationAvail->GetNext();

    identityAssociation->SetPrefixAgentRloc(aRloc16);
    identityAssociation->SetPrefix(aIp6Prefix);
    identityAssociation->SetStatus(IdentityAssociation::kStatusSolicit);

    identityAssociation->SetNext(NULL);

    if (mIdentityAssociationHead)
    {
        // append the new identityassociation to the tail of used list
        for (identityAssociationCursor = mIdentityAssociationHead; identityAssociationCursor->GetNext();
             identityAssociationCursor = identityAssociationCursor->GetNext()) {}

        identityAssociationCursor->SetNext(identityAssociation);
    }
    else
    {
        mIdentityAssociationHead = identityAssociation;
    }

exit:
    return;
}

void Dhcp6Client::RemoveIdentityAssociation(uint16_t aRloc16, otIp6Prefix &aIp6Prefix)
{
    IdentityAssociation *prevIdentityAssociation = NULL;
    IdentityAssociation *identityAssociation = NULL;

    VerifyOrExit(mIdentityAssociationHead);

    for (identityAssociation = mIdentityAssociationHead; identityAssociation;
         prevIdentityAssociation = identityAssociation, identityAssociation = identityAssociation->GetNext())
    {
        if (identityAssociation->GetPrefixAgentRloc() != aRloc16)
        {
            continue;
        }

        if (otIp6PrefixMatch(&(aIp6Prefix.mPrefix), &(identityAssociation->GetPrefix()->mPrefix)) < aIp6Prefix.mLength)
        {
            continue;
        }

        // remove from used list
        if (prevIdentityAssociation)
        {
            prevIdentityAssociation->SetNext(identityAssociation->GetNext());
        }
        else
        {
            mIdentityAssociationHead = identityAssociation->GetNext();
        }

        // return to available list
        memset(identityAssociation, 0, sizeof(*identityAssociation));
        identityAssociation->SetNext(mIdentityAssociationAvail);
        mIdentityAssociationAvail = identityAssociation;
        break;
    }

exit:
    return;
}

otError Dhcp6Client::Start(void)
{
    Ip6::SockAddr sockaddr;

    sockaddr.mPort = kDhcpClientPort;
    mSocket.Open(&Dhcp6Client::HandleUdpReceive, this);
    mSocket.Bind(sockaddr);

    ProcessNextIdentityAssociation();

    return OT_ERROR_NONE;
}

otError Dhcp6Client::Stop(void)
{
    mSocket.Close();
    return OT_ERROR_NONE;
}

bool Dhcp6Client::ProcessNextIdentityAssociation()
{
    bool rval = false;
    IdentityAssociation *prevIdentityAssociation = NULL;
    IdentityAssociation *identityAssociation = NULL;

    VerifyOrExit(mIdentityAssociationHead);
    // not interrupt in-progress solicit
    VerifyOrExit((mIdentityAssociationHead->GetStatus() != IdentityAssociation::kStatusSoliciting));

    mTrickleTimer.Stop();

    for (identityAssociation = mIdentityAssociationHead; identityAssociation;
         prevIdentityAssociation = identityAssociation, identityAssociation = identityAssociation->GetNext())
    {
        if (identityAssociation->GetStatus() != IdentityAssociation::kStatusSolicit)
        {
            continue;
        }

        // new transaction id
        for (uint8_t i = 0; i < kTransactionIdSize; i++)
        {
            mTransactionId[i] = static_cast<uint8_t>(otPlatRandomGet());
        }

        // ensure mIdentityAssociationHead is the prefix agent to solicit.
        if (prevIdentityAssociation)
        {
            prevIdentityAssociation->SetNext(identityAssociation->GetNext());
            identityAssociation->SetNext(mIdentityAssociationHead);
            mIdentityAssociationHead = identityAssociation;
        }

        mTrickleTimer.Start(
            Timer::SecToMsec(kTrickleTimerImin),
            Timer::SecToMsec(kTrickleTimerImax),
            TrickleTimer::kModeNormal);

        mTrickleTimer.IndicateInconsistent();

        ExitNow(rval = true);
    }

exit:
    return rval;
}

bool Dhcp6Client::HandleTrickleTimer(void *aContext)
{
    Dhcp6Client *obj = static_cast<Dhcp6Client *>(aContext);
    return obj->HandleTrickleTimer();
}

bool Dhcp6Client::HandleTrickleTimer(void)
{
    bool rval = true;

    VerifyOrExit(mIdentityAssociationHead, rval = false);

    switch (mIdentityAssociationHead->GetStatus())
    {
    case IdentityAssociation::kStatusSolicit:
        mStartTime = otPlatAlarmGetNow();
        mIdentityAssociationHead->SetStatus(IdentityAssociation::kStatusSoliciting);

    // fall through

    case IdentityAssociation::kStatusSoliciting:
        Solicit(mIdentityAssociationHead->GetPrefixAgentRloc());
        break;

    case IdentityAssociation::kStatusSolicitReplied:
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

otError Dhcp6Client::Solicit(uint16_t aRloc16)
{
    otError error = OT_ERROR_NONE;
    Message *message;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = AppendHeader(*message));
    SuccessOrExit(error = AppendElapsedTime(*message));
    SuccessOrExit(error = AppendClientIdentifier(*message));
    SuccessOrExit(error = AppendIaNa(*message, aRloc16));
    // specify which prefixes to solicit
    SuccessOrExit(error = AppendIaAddress(*message, aRloc16));
    SuccessOrExit(error = AppendRapidCommit(*message));

    memset(&messageInfo, 0, sizeof(messageInfo));
    memcpy(messageInfo.GetPeerAddr().mFields.m8, mNetif.GetMle().GetMeshLocalPrefix(), 8);
    messageInfo.GetPeerAddr().mFields.m16[4] = HostSwap16(0x0000);
    messageInfo.GetPeerAddr().mFields.m16[5] = HostSwap16(0x00ff);
    messageInfo.GetPeerAddr().mFields.m16[6] = HostSwap16(0xfe00);
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(aRloc16);
    messageInfo.SetSockAddr(mNetif.GetMle().GetMeshLocal16());
    messageInfo.mPeerPort = kDhcpServerPort;
    messageInfo.mInterfaceId = mNetif.GetInterfaceId();

    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));
    otLogInfoIp6(GetInstance(), "solicit");

exit:

    if (message != NULL && error != OT_ERROR_NONE)
    {
        message->Free();
    }

    return error;
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
    option.SetElapsedTime(static_cast<uint16_t>(Timer::MsecToSec(otPlatAlarmGetNow() - mStartTime)));
    return aMessage.Append(&option, sizeof(option));
}

otError Dhcp6Client::AppendClientIdentifier(Message &aMessage)
{
    ClientIdentifier option;

    option.Init();
    option.SetDuidType(kDuidLL);
    option.SetDuidHardwareType(kHardwareTypeEui64);
    option.SetDuidLinkLayerAddress(mNetif.GetMac().GetExtAddress());
    return aMessage.Append(&option, sizeof(option));
}

otError Dhcp6Client::AppendIaNa(Message &aMessage, uint16_t aRloc16)
{
    otError error = OT_ERROR_NONE;
    uint8_t count = 0;
    uint16_t length = 0;
    IdentityAssociation *identityAssociation = NULL;
    IaNa option;

    VerifyOrExit(mIdentityAssociationHead, error = OT_ERROR_DROP);

    for (identityAssociation = mIdentityAssociationHead; identityAssociation;
         identityAssociation = identityAssociation->GetNext())
    {
        if (identityAssociation->GetStatus() == IdentityAssociation::kStatusSolicitReplied)
        {
            continue;
        }

        if (identityAssociation->GetPrefixAgentRloc() == aRloc16)
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
    otError error = OT_ERROR_NONE;
    IdentityAssociation *identityAssociation = NULL;
    IaAddress option;

    VerifyOrExit(mIdentityAssociationHead, error = OT_ERROR_DROP);

    option.Init();

    for (identityAssociation = mIdentityAssociationHead; identityAssociation;
         identityAssociation = identityAssociation->GetNext())
    {
        if ((identityAssociation->GetStatus() != IdentityAssociation::kStatusSolicitReplied) &&
            (identityAssociation->GetPrefixAgentRloc() == aRloc16))
        {
            option.SetAddress(identityAssociation->GetPrefix()->mPrefix);
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
    Dhcp6Header header;
    (void)aMessageInfo;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(header), &header) == sizeof(header));
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

    // Server Identifier
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, kOptionServerIdentifier)) > 0);
    SuccessOrExit(ProcessServerIdentifier(aMessage, optionOffset));

    // Client Identifier
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, kOptionClientIdentifier)) > 0);
    SuccessOrExit(ProcessClientIdentifier(aMessage, optionOffset));

    // Rapid Commit
    VerifyOrExit(FindOption(aMessage, offset, length, kOptionRapidCommit) > 0);

    // IA_NA
    VerifyOrExit((optionOffset = FindOption(aMessage, offset, length, kOptionIaNa)) > 0);
    SuccessOrExit(ProcessIaNa(aMessage, optionOffset));

    HandleTrickleTimer();

exit:
    return;
}

uint16_t Dhcp6Client::FindOption(Message &aMessage, uint16_t aOffset, uint16_t aLength, Dhcp6::Code aCode)
{
    uint16_t end = aOffset + aLength;

    while (aOffset <= end)
    {
        Dhcp6Option option;
        VerifyOrExit(aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option));

        if (option.GetCode() == (aCode))
        {
            return aOffset;
        }

        aOffset += sizeof(option) + option.GetLength();
    }

exit:
    return 0;
}

otError Dhcp6Client::ProcessServerIdentifier(Message &aMessage, uint16_t aOffset)
{
    otError error = OT_ERROR_NONE;
    ServerIdentifier option;

    VerifyOrExit(((aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option)) &&
                  (option.GetLength() == (sizeof(option) - sizeof(Dhcp6Option))) &&
                  (option.GetDuidType() == kDuidLL) &&
                  (option.GetDuidHardwareType() == kHardwareTypeEui64)),
                 error = OT_ERROR_PARSE);
exit:
    return error;
}

otError Dhcp6Client::ProcessClientIdentifier(Message &aMessage, uint16_t aOffset)
{
    otError error = OT_ERROR_NONE;
    ClientIdentifier option;

    VerifyOrExit((((aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option)) &&
                   (option.GetLength() == (sizeof(option) - sizeof(Dhcp6Option))) &&
                   (option.GetDuidType() == kDuidLL) &&
                   (option.GetDuidHardwareType() == kHardwareTypeEui64)) &&
                  (!memcmp(option.GetDuidLinkLayerAddress(), mNetif.GetMac().GetExtAddress(),
                           sizeof(Mac::ExtAddress)))),
                 error = OT_ERROR_PARSE);
exit:
    return error;
}

otError Dhcp6Client::ProcessIaNa(Message &aMessage, uint16_t aOffset)
{
    otError error = OT_ERROR_NONE;
    IaNa option;
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
    otError error = OT_ERROR_NONE;
    StatusCode option;

    VerifyOrExit(((aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option)) &&
                  (option.GetLength() == (sizeof(option) - sizeof(Dhcp6Option))) &&
                  (option.GetStatusCode() == kStatusSuccess)),
                 error = OT_ERROR_PARSE);

exit:
    return error;
}

otError Dhcp6Client::ProcessIaAddress(Message &aMessage, uint16_t aOffset)
{
    otError error = OT_ERROR_NONE;
    IdentityAssociation *identityAssociation = NULL;
    otDhcpAddress *address = NULL;
    otIp6Prefix *prefix  = NULL;

    IaAddress option;

    VerifyOrExit(((aMessage.Read(aOffset, sizeof(option), &option) == sizeof(option)) &&
                  (option.GetLength() == (sizeof(option) - sizeof(Dhcp6Option)))),
                 error = OT_ERROR_PARSE);

    for (uint8_t i = 0; i < mNumAddresses; i++)
    {
        address = &mAddresses[i];

        if (address->mValidLifetime != 0)
        {
            continue;
        }

        if (otIp6PrefixMatch(&(address->mAddress.mAddress), option.GetAddress()) >= address->mAddress.mPrefixLength)
        {
            memcpy(address->mAddress.mAddress.mFields.m8, option.GetAddress()->mFields.m8, sizeof(otIp6Address));
            address->mPreferredLifetime = option.GetPreferredLifetime();
            address->mValidLifetime = option.GetValidLifetime();
            address->mAddress.mPreferred = address->mPreferredLifetime != 0;
            address->mAddress.mValid = address->mValidLifetime != 0;
            otIp6AddUnicastAddress(mNetif.GetInstance(), &address->mAddress);
            break;
        }
    }

    // mark IdentityAssociation as replied
    for (identityAssociation = mIdentityAssociationHead; identityAssociation;
         identityAssociation = identityAssociation->GetNext())
    {
        prefix = identityAssociation->GetPrefix();

        if (otIp6PrefixMatch(option.GetAddress(), &(prefix->mPrefix)) >= prefix->mLength)
        {
            identityAssociation->SetStatus(IdentityAssociation::kStatusSolicitReplied);
            break;
        }
    }

exit:
    return error;
}

}  // namespace Dhcp6
}  // namespace ot

#endif //OPENTHREAD_ENABLE_DHCP6_CLIENT

