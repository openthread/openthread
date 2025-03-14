
/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements common MeshCoP utility functions.
 */

#include "meshcop.hpp"

#include "common/crc.hpp"
#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("MeshCoP");

namespace MeshCoP {

Error JoinerPskd::SetFrom(const char *aPskdString)
{
    Error error = kErrorNone;

    VerifyOrExit(IsPskdValid(aPskdString), error = kErrorInvalidArgs);

    Clear();
    memcpy(m8, aPskdString, StringLength(aPskdString, sizeof(m8)));

exit:
    return error;
}

bool JoinerPskd::operator==(const JoinerPskd &aOther) const
{
    bool isEqual = true;

    for (size_t i = 0; i < sizeof(m8); i++)
    {
        if (m8[i] != aOther.m8[i])
        {
            isEqual = false;
            ExitNow();
        }

        if (m8[i] == '\0')
        {
            break;
        }
    }

exit:
    return isEqual;
}

bool JoinerPskd::IsPskdValid(const char *aPskdString)
{
    bool     valid      = false;
    uint16_t pskdLength = StringLength(aPskdString, kMaxLength + 1);

    VerifyOrExit(pskdLength >= kMinLength && pskdLength <= kMaxLength);

    for (uint16_t i = 0; i < pskdLength; i++)
    {
        char c = aPskdString[i];

        VerifyOrExit(IsDigit(c) || IsUppercase(c));
        VerifyOrExit(c != 'I' && c != 'O' && c != 'Q' && c != 'Z');
    }

    valid = true;

exit:
    return valid;
}

void JoinerDiscerner::GenerateJoinerId(Mac::ExtAddress &aJoinerId) const
{
    aJoinerId.GenerateRandom();
    CopyTo(aJoinerId);
    aJoinerId.SetLocal(true);
}

bool JoinerDiscerner::Matches(const Mac::ExtAddress &aJoinerId) const
{
    uint64_t mask;

    OT_ASSERT(IsValid());

    mask = GetMask();

    return (BigEndian::ReadUint64(aJoinerId.m8) & mask) == (mValue & mask);
}

void JoinerDiscerner::CopyTo(Mac::ExtAddress &aExtAddress) const
{
    // Copies the discerner value up to its bit length to `aExtAddress`
    // array, assuming big-endian encoding (i.e., the discerner lowest bits
    // are copied at end of `aExtAddress.m8[]` array). Any initial/remaining
    // bits of `aExtAddress` array remain unchanged.

    uint8_t *cur       = &aExtAddress.m8[sizeof(Mac::ExtAddress) - 1];
    uint8_t  remaining = mLength;
    uint64_t value     = mValue;

    OT_ASSERT(IsValid());

    // Write full bytes
    while (remaining >= kBitsPerByte)
    {
        *cur = static_cast<uint8_t>(value & 0xff);
        value >>= kBitsPerByte;
        cur--;
        remaining -= kBitsPerByte;
    }

    // Write any remaining bits (not a full byte)
    if (remaining != 0)
    {
        uint8_t mask = static_cast<uint8_t>((1U << remaining) - 1);

        // `mask` has it lower (lsb) `remaining` bits as `1` and rest as `0`.
        // Example with `remaining = 3` -> (1 << 3) - 1 = 0b1000 - 1 = 0b0111.

        *cur &= ~mask;
        *cur |= static_cast<uint8_t>(value & mask);
    }
}

bool JoinerDiscerner::operator==(const JoinerDiscerner &aOther) const
{
    uint64_t mask = GetMask();

    return IsValid() && (mLength == aOther.mLength) && ((mValue & mask) == (aOther.mValue & mask));
}

JoinerDiscerner::InfoString JoinerDiscerner::ToString(void) const
{
    InfoString string;

    if (mLength <= BitSizeOf(uint16_t))
    {
        string.Append("0x%04x", static_cast<uint16_t>(mValue));
    }
    else if (mLength <= BitSizeOf(uint32_t))
    {
        string.Append("0x%08lx", ToUlong(static_cast<uint32_t>(mValue)));
    }
    else
    {
        string.Append("0x%lx-%08lx", ToUlong(static_cast<uint32_t>(mValue >> 32)),
                      ToUlong(static_cast<uint32_t>(mValue)));
    }

    string.Append("/len:%d", mLength);

    return string;
}

void SteeringData::Init(uint8_t aLength)
{
    OT_ASSERT(aLength <= kMaxLength);
    mLength = aLength;
    ClearAllBytes(m8);
}

void SteeringData::SetToPermitAllJoiners(void)
{
    Init(1);
    m8[0] = kPermitAll;
}

void SteeringData::UpdateBloomFilter(const Mac::ExtAddress &aJoinerId)
{
    HashBitIndexes indexes;

    CalculateHashBitIndexes(aJoinerId, indexes);
    UpdateBloomFilter(indexes);
}

void SteeringData::UpdateBloomFilter(const JoinerDiscerner &aDiscerner)
{
    HashBitIndexes indexes;

    CalculateHashBitIndexes(aDiscerner, indexes);
    UpdateBloomFilter(indexes);
}

void SteeringData::UpdateBloomFilter(const HashBitIndexes &aIndexes)
{
    OT_ASSERT((mLength > 0) && (mLength <= kMaxLength));

    SetBit(aIndexes.mIndex[0] % GetNumBits());
    SetBit(aIndexes.mIndex[1] % GetNumBits());
}

bool SteeringData::Contains(const Mac::ExtAddress &aJoinerId) const
{
    HashBitIndexes indexes;

    CalculateHashBitIndexes(aJoinerId, indexes);

    return Contains(indexes);
}

bool SteeringData::Contains(const JoinerDiscerner &aDiscerner) const
{
    HashBitIndexes indexes;

    CalculateHashBitIndexes(aDiscerner, indexes);

    return Contains(indexes);
}

bool SteeringData::Contains(const HashBitIndexes &aIndexes) const
{
    return (mLength > 0) && GetBit(aIndexes.mIndex[0] % GetNumBits()) && GetBit(aIndexes.mIndex[1] % GetNumBits());
}

void SteeringData::CalculateHashBitIndexes(const Mac::ExtAddress &aJoinerId, HashBitIndexes &aIndexes)
{
    aIndexes.mIndex[0] = CrcCalculator<uint16_t>(kCrc16CcittPolynomial).Feed(aJoinerId);
    aIndexes.mIndex[1] = CrcCalculator<uint16_t>(kCrc16AnsiPolynomial).Feed(aJoinerId);
}

void SteeringData::CalculateHashBitIndexes(const JoinerDiscerner &aDiscerner, HashBitIndexes &aIndexes)
{
    Mac::ExtAddress address;

    address.Clear();
    aDiscerner.CopyTo(address);

    CalculateHashBitIndexes(address, aIndexes);
}

bool SteeringData::DoesAllMatch(uint8_t aMatch) const
{
    bool matches = true;

    for (uint8_t i = 0; i < mLength; i++)
    {
        if (m8[i] != aMatch)
        {
            matches = false;
            break;
        }
    }

    return matches;
}

void ComputeJoinerId(const Mac::ExtAddress &aEui64, Mac::ExtAddress &aJoinerId)
{
    Crypto::Sha256       sha256;
    Crypto::Sha256::Hash hash;

    sha256.Start();
    sha256.Update(aEui64);
    sha256.Finish(hash);

    memcpy(&aJoinerId, hash.GetBytes(), sizeof(aJoinerId));
    aJoinerId.SetLocal(true);
}

#if OPENTHREAD_FTD
Error GeneratePskc(const char          *aPassPhrase,
                   const NetworkName   &aNetworkName,
                   const ExtendedPanId &aExtPanId,
                   Pskc                &aPskc)
{
    Error      error        = kErrorNone;
    const char saltPrefix[] = "Thread";
    uint8_t    salt[OT_CRYPTO_PBDKF2_MAX_SALT_SIZE];
    uint16_t   saltLen = 0;
    uint16_t   passphraseLen;
    uint8_t    networkNameLen;

    VerifyOrExit(IsValidUtf8String(aPassPhrase), error = kErrorInvalidArgs);

    passphraseLen  = static_cast<uint16_t>(StringLength(aPassPhrase, OT_COMMISSIONING_PASSPHRASE_MAX_SIZE + 1));
    networkNameLen = static_cast<uint8_t>(StringLength(aNetworkName.GetAsCString(), OT_NETWORK_NAME_MAX_SIZE + 1));

    VerifyOrExit((passphraseLen >= OT_COMMISSIONING_PASSPHRASE_MIN_SIZE) &&
                     (passphraseLen <= OT_COMMISSIONING_PASSPHRASE_MAX_SIZE) &&
                     (networkNameLen <= OT_NETWORK_NAME_MAX_SIZE),
                 error = kErrorInvalidArgs);

    ClearAllBytes(salt);
    memcpy(salt, saltPrefix, sizeof(saltPrefix) - 1);
    saltLen += static_cast<uint16_t>(sizeof(saltPrefix) - 1);

    memcpy(salt + saltLen, aExtPanId.m8, sizeof(aExtPanId));
    saltLen += OT_EXT_PAN_ID_SIZE;

    memcpy(salt + saltLen, aNetworkName.GetAsCString(), networkNameLen);
    saltLen += networkNameLen;

    error = otPlatCryptoPbkdf2GenerateKey(reinterpret_cast<const uint8_t *>(aPassPhrase), passphraseLen, salt, saltLen,
                                          16384, OT_PSKC_MAX_SIZE, aPskc.m8);

exit:
    return error;
}
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

void LogCertMessage(const char *aText, const Coap::Message &aMessage)
{
    OT_UNUSED_VARIABLE(aText);

    uint8_t  buf[kBufferSize];
    uint16_t length = aMessage.GetLength() - aMessage.GetOffset();

    VerifyOrExit(length <= sizeof(buf));
    aMessage.ReadBytes(aMessage.GetOffset(), buf, length);

    DumpCert(aText, buf, length);

exit:
    return;
}

#endif

} // namespace MeshCoP
} // namespace ot
