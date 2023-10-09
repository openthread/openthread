/*
 *  Copyright (c) 2016-2019, The OpenThread Authors.
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
 *   This file implements MAC types.
 */

#include "mac_types.hpp"

#include <stdio.h>

#include "common/code_utils.hpp"
#include "common/random.hpp"
#include "common/string.hpp"

namespace ot {
namespace Mac {

PanId GenerateRandomPanId(void)
{
    PanId panId;

    do
    {
        panId = Random::NonCrypto::GetUint16();
    } while (panId == kPanIdBroadcast);

    return panId;
}

#if !OPENTHREAD_RADIO
void ExtAddress::GenerateRandom(void)
{
    IgnoreError(Random::Crypto::Fill(*this));
    SetGroup(false);
    SetLocal(true);
}
#endif

ExtAddress::InfoString ExtAddress::ToString(void) const
{
    InfoString string;

    string.AppendHexBytes(m8, sizeof(ExtAddress));

    return string;
}

void ExtAddress::CopyAddress(uint8_t *aDst, const uint8_t *aSrc, CopyByteOrder aByteOrder)
{
    switch (aByteOrder)
    {
    case kNormalByteOrder:
        memcpy(aDst, aSrc, sizeof(ExtAddress));
        break;

    case kReverseByteOrder:
        aSrc += sizeof(ExtAddress) - 1;
        for (uint8_t len = sizeof(ExtAddress); len > 0; len--)
        {
            *aDst++ = *aSrc--;
        }
        break;
    }
}

Address::InfoString Address::ToString(void) const
{
    InfoString string;

    if (mType == kTypeExtended)
    {
        string.AppendHexBytes(GetExtended().m8, sizeof(ExtAddress));
    }
    else if (mType == kTypeNone)
    {
        string.Append("None");
    }
    else
    {
        string.Append("0x%04x", GetShort());
    }

    return string;
}

void PanIds::SetSource(PanId aPanId)
{
    mSource          = aPanId;
    mIsSourcePresent = true;
}

void PanIds::SetDestination(PanId aPanId)
{
    mDestination          = aPanId;
    mIsDestinationPresent = true;
}

void PanIds::SetBothSourceDestination(PanId aPanId)
{
    SetSource(aPanId);
    SetDestination(aPanId);
}

#if OPENTHREAD_CONFIG_MULTI_RADIO

const RadioType RadioTypes::kAllRadioTypes[kNumRadioTypes] = {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    kRadioTypeIeee802154,
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    kRadioTypeTrel,
#endif
};

void RadioTypes::AddAll(void)
{
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    Add(kRadioTypeIeee802154);
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    Add(kRadioTypeTrel);
#endif
}

RadioTypes::InfoString RadioTypes::ToString(void) const
{
    InfoString string;
    bool       addComma = false;

    string.Append("{");
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    if (Contains(kRadioTypeIeee802154))
    {
        string.Append("%s%s", addComma ? ", " : " ", RadioTypeToString(kRadioTypeIeee802154));
        addComma = true;
    }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    if (Contains(kRadioTypeTrel))
    {
        string.Append("%s%s", addComma ? ", " : " ", RadioTypeToString(kRadioTypeTrel));
        addComma = true;
    }
#endif

    OT_UNUSED_VARIABLE(addComma);

    string.Append(" }");

    return string;
}

const char *RadioTypeToString(RadioType aRadioType)
{
    const char *str = "unknown";

    switch (aRadioType)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    case kRadioTypeIeee802154:
        str = "15.4";
        break;
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    case kRadioTypeTrel:
        str = "trel";
        break;
#endif
    }

    return str;
}

uint32_t LinkFrameCounters::Get(RadioType aRadioType) const
{
    uint32_t counter = 0;

    switch (aRadioType)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    case kRadioTypeIeee802154:
        counter = m154Counter;
        break;
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    case kRadioTypeTrel:
        counter = mTrelCounter;
        break;
#endif
    }

    return counter;
}

void LinkFrameCounters::Set(RadioType aRadioType, uint32_t aCounter)
{
    switch (aRadioType)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    case kRadioTypeIeee802154:
        m154Counter = aCounter;
        break;
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    case kRadioTypeTrel:
        mTrelCounter = aCounter;
        break;
#endif
    }
}

#endif // OPENTHREAD_CONFIG_MULTI_RADIO

uint32_t LinkFrameCounters::GetMaximum(void) const
{
    uint32_t counter = 0;

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    counter = Max(counter, m154Counter);
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    counter = Max(counter, mTrelCounter);
#endif

    return counter;
}

void LinkFrameCounters::SetAll(uint32_t aCounter)
{
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    m154Counter = aCounter;
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    mTrelCounter = aCounter;
#endif
}

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
KeyMaterial &KeyMaterial::operator=(const KeyMaterial &aOther)
{
    VerifyOrExit(GetKeyRef() != aOther.GetKeyRef());
    DestroyKey();
    SetKeyRef(aOther.GetKeyRef());

exit:
    return *this;
}
#endif

void KeyMaterial::Clear(void)
{
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    DestroyKey();
    SetKeyRef(kInvalidKeyRef);
#else
    GetKey().Clear();
#endif
}

void KeyMaterial::SetFrom(const Key &aKey, bool aIsExportable)
{
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    {
        KeyRef keyRef = 0;

        DestroyKey();

        SuccessOrAssert(Crypto::Storage::ImportKey(keyRef, Crypto::Storage::kKeyTypeAes,
                                                   Crypto::Storage::kKeyAlgorithmAesEcb,
                                                   (aIsExportable ? Crypto::Storage::kUsageExport : 0) |
                                                       Crypto::Storage::kUsageEncrypt | Crypto::Storage::kUsageDecrypt,
                                                   Crypto::Storage::kTypeVolatile, aKey.GetBytes(), Key::kSize));

        SetKeyRef(keyRef);
    }
#else
    SetKey(aKey);
    OT_UNUSED_VARIABLE(aIsExportable);
#endif
}

void KeyMaterial::ExtractKey(Key &aKey) const
{
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    aKey.Clear();

    if (Crypto::Storage::IsKeyRefValid(GetKeyRef()))
    {
        size_t keySize;

        SuccessOrAssert(Crypto::Storage::ExportKey(GetKeyRef(), aKey.m8, Key::kSize, keySize));
    }
#else
    aKey = GetKey();
#endif
}

void KeyMaterial::ConvertToCryptoKey(Crypto::Key &aCryptoKey) const
{
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    aCryptoKey.SetAsKeyRef(GetKeyRef());
#else
    aCryptoKey.Set(GetKey().GetBytes(), Key::kSize);
#endif
}

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
void KeyMaterial::DestroyKey(void)
{
    Crypto::Storage::DestroyKey(GetKeyRef());
    SetKeyRef(kInvalidKeyRef);
}
#endif

bool KeyMaterial::operator==(const KeyMaterial &aOther) const
{
    return
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
        (GetKeyRef() == aOther.GetKeyRef());
#else
        (GetKey() == aOther.GetKey());
#endif
}

} // namespace Mac
} // namespace ot
