/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file implements MechCop TLV helper functions.
 */

#include "meshcop_tlvs.hpp"

#include "common/debug.hpp"
#include "meshcop/meshcop.hpp"

namespace ot {
namespace MeshCoP {

bool Tlv::IsValid(const Tlv &aTlv)
{
    bool rval = true;

    switch (aTlv.GetType())
    {
    case Tlv::kChannel:
        rval = static_cast<const ChannelTlv &>(aTlv).IsValid();
        break;

    case Tlv::kPanId:
        rval = static_cast<const PanIdTlv &>(aTlv).IsValid();
        break;

    case Tlv::kExtendedPanId:
        rval = static_cast<const ExtendedPanIdTlv &>(aTlv).IsValid();
        break;

    case Tlv::kNetworkName:
        rval = static_cast<const NetworkNameTlv &>(aTlv).IsValid();
        break;

    case Tlv::kNetworkMasterKey:
        rval = static_cast<const NetworkMasterKeyTlv &>(aTlv).IsValid();
        break;

    case Tlv::kPskc:
        rval = static_cast<const PskcTlv &>(aTlv).IsValid();
        break;

    case Tlv::kMeshLocalPrefix:
        rval = static_cast<const MeshLocalPrefixTlv &>(aTlv).IsValid();
        break;

    case Tlv::kSecurityPolicy:
        rval = static_cast<const SecurityPolicyTlv &>(aTlv).IsValid();
        break;

    default:
        break;
    }

    return rval;
}

const Tlv *Tlv::FindTlv(const uint8_t *aTlvsStart, uint16_t aTlvsLength, Type aType)
{
    const Tlv *tlv;
    const Tlv *end = reinterpret_cast<const Tlv *>(aTlvsStart + aTlvsLength);

    for (tlv = reinterpret_cast<const Tlv *>(aTlvsStart); tlv < end; tlv = tlv->GetNext())
    {
        VerifyOrExit((tlv + 1) <= end, tlv = nullptr);
        VerifyOrExit(!tlv->IsExtended() ||
                         (reinterpret_cast<const ExtendedTlv *>(tlv) + 1 <= reinterpret_cast<const ExtendedTlv *>(end)),
                     tlv = nullptr);
        VerifyOrExit(tlv->GetNext() <= end, tlv = nullptr);

        if (tlv->GetType() == aType)
        {
            ExitNow();
        }
    }

    tlv = nullptr;

exit:
    return tlv;
}

Mac::NameData NetworkNameTlv::GetNetworkName(void) const
{
    uint8_t len = GetLength();

    if (len > sizeof(mNetworkName))
    {
        len = sizeof(mNetworkName);
    }

    return Mac::NameData(mNetworkName, len);
}

void NetworkNameTlv::SetNetworkName(const Mac::NameData &aNameData)
{
    uint8_t len;

    len = aNameData.CopyTo(mNetworkName, sizeof(mNetworkName));
    SetLength(len);
}

void SteeringDataTlv::CopyTo(SteeringData &aSteeringData) const
{
    aSteeringData.Init(GetSteeringDataLength());
    memcpy(aSteeringData.GetData(), mSteeringData, GetSteeringDataLength());
}

bool ChannelTlv::IsValid(void) const
{
    bool ret = false;

    VerifyOrExit(GetLength() == sizeof(*this) - sizeof(Tlv), OT_NOOP);
    VerifyOrExit(mChannelPage <= OT_RADIO_CHANNEL_PAGE_MAX, OT_NOOP);
    VerifyOrExit((1U << mChannelPage) & Radio::kSupportedChannelPages, OT_NOOP);
    VerifyOrExit(Radio::kChannelMin <= GetChannel() && GetChannel() <= Radio::kChannelMax, OT_NOOP);
    ret = true;

exit:
    return ret;
}

void ChannelTlv::SetChannel(uint16_t aChannel)
{
    uint8_t channelPage = OT_RADIO_CHANNEL_PAGE_0;

#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
    if ((OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN <= aChannel) && (aChannel <= OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX))
    {
        channelPage = OT_RADIO_CHANNEL_PAGE_0;
    }
#endif

#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    if ((OT_RADIO_915MHZ_OQPSK_CHANNEL_MIN <= aChannel) && (aChannel <= OT_RADIO_915MHZ_OQPSK_CHANNEL_MAX))
    {
        channelPage = OT_RADIO_CHANNEL_PAGE_2;
    }
#endif

    SetChannelPage(channelPage);
    mChannel = HostSwap16(aChannel);
}

const ChannelMaskEntryBase *ChannelMaskBaseTlv::GetFirstEntry(void) const
{
    const ChannelMaskEntryBase *entry = nullptr;

    VerifyOrExit(GetLength() >= sizeof(ChannelMaskEntryBase), OT_NOOP);

    entry = reinterpret_cast<const ChannelMaskEntryBase *>(GetValue());
    VerifyOrExit(GetLength() >= entry->GetEntrySize(), entry = nullptr);

exit:
    return entry;
}

ChannelMaskEntryBase *ChannelMaskBaseTlv::GetFirstEntry(void)
{
    return const_cast<ChannelMaskEntryBase *>(static_cast<const ChannelMaskBaseTlv *>(this)->GetFirstEntry());
}

void ChannelMaskTlv::SetChannelMask(uint32_t aChannelMask)
{
    uint8_t           length = 0;
    ChannelMaskEntry *entry;

    entry = static_cast<ChannelMaskEntry *>(GetFirstEntry());

#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    if (aChannelMask & OT_RADIO_915MHZ_OQPSK_CHANNEL_MASK)
    {
        OT_ASSERT(entry != nullptr);
        entry->Init();
        entry->SetChannelPage(OT_RADIO_CHANNEL_PAGE_2);
        entry->SetMask(aChannelMask & OT_RADIO_915MHZ_OQPSK_CHANNEL_MASK);

        length += sizeof(MeshCoP::ChannelMaskEntry);

        entry = static_cast<MeshCoP::ChannelMaskEntry *>(entry->GetNext());
    }
#endif

#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
    if (aChannelMask & OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MASK)
    {
        OT_ASSERT(entry != nullptr);
        entry->Init();
        entry->SetChannelPage(OT_RADIO_CHANNEL_PAGE_0);
        entry->SetMask(aChannelMask & OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MASK);

        length += sizeof(MeshCoP::ChannelMaskEntry);
    }
#endif

    SetLength(length);
}

uint32_t ChannelMaskTlv::GetChannelMask(void) const
{
    uint32_t                mask = 0;
    const ChannelMaskEntry *cur  = static_cast<const ChannelMaskEntry *>(GetFirstEntry());
    const ChannelMaskEntry *end  = reinterpret_cast<const ChannelMaskEntry *>(GetValue() + GetLength());

    for (; cur < end; cur = static_cast<const ChannelMaskEntry *>(cur->GetNext()))
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end, OT_NOOP);

#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
        if (cur->GetChannelPage() == OT_RADIO_CHANNEL_PAGE_2)
        {
            mask |= cur->GetMask() & OT_RADIO_915MHZ_OQPSK_CHANNEL_MASK;
        }
#endif

#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
        if (cur->GetChannelPage() == OT_RADIO_CHANNEL_PAGE_0)
        {
            mask |= cur->GetMask() & OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MASK;
        }
#endif
    }

exit:
    return mask;
}

uint32_t ChannelMaskTlv::GetChannelMask(const Message &aMessage)
{
    uint32_t mask = 0;
    uint16_t offset;
    uint16_t end;

    SuccessOrExit(FindTlvValueOffset(aMessage, kChannelMask, offset, end));
    end += offset;

    while (offset + sizeof(ChannelMaskEntryBase) <= end)
    {
        ChannelMaskEntry entry;

        aMessage.Read(offset, sizeof(ChannelMaskEntryBase), &entry);
        VerifyOrExit(offset + entry.GetEntrySize() <= end, OT_NOOP);

        switch (entry.GetChannelPage())
        {
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
        case OT_RADIO_CHANNEL_PAGE_0:
            aMessage.Read(offset, sizeof(entry), &entry);
            mask |= entry.GetMask() & OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MASK;
            break;
#endif

#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
        case OT_RADIO_CHANNEL_PAGE_2:
            aMessage.Read(offset, sizeof(entry), &entry);
            mask |= entry.GetMask() & OT_RADIO_915MHZ_OQPSK_CHANNEL_MASK;
            break;
#endif
        }

        offset += entry.GetEntrySize();
    }

exit:
    return mask;
}

} // namespace MeshCoP
} // namespace ot
