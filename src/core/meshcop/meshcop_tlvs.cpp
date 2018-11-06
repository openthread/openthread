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

    case Tlv::kPSKc:
        rval = static_cast<const PSKcTlv &>(aTlv).IsValid();
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

bool SteeringDataTlv::IsCleared(void) const
{
    bool rval = true;

    for (uint8_t i = 0; i < GetLength(); i++)
    {
        if (mSteeringData[i] != 0)
        {
            rval = false;
            break;
        }
    }

    return rval;
}

void SteeringDataTlv::ComputeBloomFilter(const otExtAddress &aJoinerId)
{
    Crc16 ccitt(Crc16::kCcitt);
    Crc16 ansi(Crc16::kAnsi);

    for (size_t j = 0; j < sizeof(otExtAddress); j++)
    {
        uint8_t byte = aJoinerId.m8[j];
        ccitt.Update(byte);
        ansi.Update(byte);
    }

    SetBit(ccitt.Get() % GetNumBits());
    SetBit(ansi.Get() % GetNumBits());
}

const ChannelMaskEntryBase *ChannelMaskEntryBase::GetNext(const Tlv *aChannelMaskBaseTlv) const
{
    const uint8_t *entry = reinterpret_cast<const uint8_t *>(this) + GetSize();
    const uint8_t *end   = aChannelMaskBaseTlv->GetValue() + aChannelMaskBaseTlv->GetSize();

    return (entry < end) ? reinterpret_cast<const ChannelMaskEntryBase *>(entry) : NULL;
}

const ChannelMaskEntryBase *ChannelMaskBaseTlv::GetFirstEntry(void) const
{
    const ChannelMaskEntryBase *entry = NULL;

    VerifyOrExit(GetLength() >= sizeof(ChannelMaskEntryBase));

    entry = reinterpret_cast<const ChannelMaskEntryBase *>(GetValue());
    VerifyOrExit(GetLength() >= entry->GetSize(), entry = NULL);

exit:
    return entry;
}

const ChannelMaskEntry *ChannelMaskBaseTlv::GetMaskEntry(uint8_t aChannelPage) const
{
    const ChannelMaskEntry *pageEntry = NULL;

    for (const ChannelMaskEntryBase *entry = GetFirstEntry(); entry != NULL; entry = entry->GetNext(this))
    {
        if (entry->GetChannelPage() == aChannelPage)
        {
            pageEntry = static_cast<const ChannelMaskEntry *>(entry);

            if (pageEntry->IsValid())
            {
                ExitNow();
            }
        }
    }

    pageEntry = NULL;

exit:
    return pageEntry;
}

} // namespace MeshCoP
} // namespace ot
