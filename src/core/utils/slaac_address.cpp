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
 *   This file implements the Thread IPv6 global addresses configuration utilities.
 */

#include "slaac_address.hpp"

#if OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE

#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/numeric_limits.hpp"
#include "common/random.hpp"
#include "common/settings.hpp"
#include "crypto/sha256.hpp"
#include "instance/instance.hpp"
#include "net/ip6_address.hpp"

namespace ot {
namespace Utils {

RegisterLogModule("Slaac");

Slaac::Slaac(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(true)
    , mFilter(nullptr)
{
    memset(mAddresses, 0, sizeof(mAddresses));
}

void Slaac::Enable(void)
{
    VerifyOrExit(!mEnabled);

    mEnabled = true;
    LogInfo("Enabled");

    AddAddresses();

exit:
    return;
}

void Slaac::Disable(void)
{
    VerifyOrExit(mEnabled);

    RemoveAllAddresses();
    LogInfo("Disabled");
    mEnabled = false;

exit:
    return;
}

void Slaac::SetFilter(PrefixFilter aFilter)
{
    VerifyOrExit(aFilter != mFilter);

    mFilter = aFilter;
    LogInfo("Filter %s", (mFilter != nullptr) ? "updated" : "disabled");

    VerifyOrExit(mEnabled);
    RemoveAddresses();
    AddAddresses();

exit:
    return;
}

bool Slaac::ShouldUseForSlaac(const NetworkData::OnMeshPrefixConfig &aConfig) const
{
    bool shouldUse = false;

    VerifyOrExit(aConfig.mSlaac && !aConfig.mDp);
    VerifyOrExit(aConfig.GetPrefix().GetLength() == Ip6::NetworkPrefix::kLength);

    if (mFilter != nullptr)
    {
        VerifyOrExit(!mFilter(&GetInstance(), &aConfig.GetPrefix()));
    }

    shouldUse = true;

exit:
    return shouldUse;
}

void Slaac::HandleNotifierEvents(Events aEvents)
{
    VerifyOrExit(mEnabled);

    if (aEvents.Contains(kEventThreadNetdataChanged))
    {
        RemoveAddresses();
        AddAddresses();
        ExitNow();
    }

    if (aEvents.Contains(kEventIp6AddressRemoved))
    {
        // When an IPv6 address is removed, we ensure to check if a SLAAC address
        // needs to be added (replacing the removed address).
        //
        // Note that if an address matching a newly added on-mesh prefix (with
        // SLAAC flag) is already present (e.g., user previously added an address
        // with same prefix), the SLAAC module will not add a SLAAC address with same
        // prefix. So on IPv6 address removal event, we check if SLAAC module need
        // to add any addresses.

        AddAddresses();
    }

exit:
    return;
}

bool Slaac::DoesConfigMatchNetifAddr(const NetworkData::OnMeshPrefixConfig &aConfig,
                                     const Ip6::Netif::UnicastAddress      &aAddr)
{
    return (((aConfig.mOnMesh && (aAddr.mPrefixLength == aConfig.mPrefix.mLength)) ||
             (!aConfig.mOnMesh && (aAddr.mPrefixLength == 128))) &&
            (aAddr.GetAddress().MatchesPrefix(aConfig.GetPrefix())));
}

void Slaac::RemoveAddresses(void)
{
    // Remove any SLAAC addresses with no matching on-mesh prefix.

    for (Ip6::Netif::UnicastAddress &slaacAddr : mAddresses)
    {
        NetworkData::Iterator           iterator;
        NetworkData::OnMeshPrefixConfig config;
        bool                            found = false;

        if (!slaacAddr.mValid)
        {
            continue;
        }

        iterator = NetworkData::kIteratorInit;

        while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, config) == kErrorNone)
        {
            if (ShouldUseForSlaac(config) && DoesConfigMatchNetifAddr(config, slaacAddr))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            RemoveAddress(slaacAddr);
        }
    }
}

void Slaac::RemoveAllAddresses(void)
{
    for (Ip6::Netif::UnicastAddress &slaacAddr : mAddresses)
    {
        if (slaacAddr.mValid)
        {
            RemoveAddress(slaacAddr);
        }
    }
}

void Slaac::RemoveAddress(Ip6::Netif::UnicastAddress &aAddress)
{
    LogInfo("Removing %s", aAddress.GetAddress().ToString().AsCString());

    Get<ThreadNetif>().RemoveUnicastAddress(aAddress);
    aAddress.mValid = false;
}

void Slaac::AddAddresses(void)
{
    NetworkData::Iterator           iterator;
    NetworkData::OnMeshPrefixConfig config;

    // Generate and add SLAAC addresses for any newly added on-mesh prefixes.

    iterator = NetworkData::kIteratorInit;

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, config) == kErrorNone)
    {
        bool found = false;

        if (!ShouldUseForSlaac(config))
        {
            continue;
        }

        for (const Ip6::Netif::UnicastAddress &netifAddr : Get<ThreadNetif>().GetUnicastAddresses())
        {
            if (DoesConfigMatchNetifAddr(config, netifAddr))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            AddAddressFor(config);
        }
    }
}

void Slaac::AddAddressFor(const NetworkData::OnMeshPrefixConfig &aConfig)
{
    Ip6::Netif::UnicastAddress *newAddress = nullptr;
    uint8_t                     dadCounter = 0;

    for (Ip6::Netif::UnicastAddress &slaacAddr : mAddresses)
    {
        if (!slaacAddr.mValid)
        {
            newAddress = &slaacAddr;
            break;
        }
    }

    if (newAddress == nullptr)
    {
        LogWarn("Failed to add - already have max %u addresses", kNumAddresses);
        ExitNow();
    }

    newAddress->InitAsSlaacOrigin(aConfig.mOnMesh ? aConfig.GetPrefix().mLength : 128, aConfig.mPreferred);
    newAddress->GetAddress().SetPrefix(aConfig.GetPrefix());

    IgnoreError(GenerateIid(*newAddress, dadCounter));

    LogInfo("Adding address %s", newAddress->GetAddress().ToString().AsCString());

    Get<ThreadNetif>().AddUnicastAddress(*newAddress);

exit:
    return;
}

Error Slaac::GenerateIid(Ip6::Netif::UnicastAddress &aAddress, uint8_t &aDadCounter) const
{
    /*
     *  This method generates a semantically opaque IID per RFC 7217.
     *
     * RID = F(Prefix, Net_Iface, Network_ID, DAD_Counter, secret_key)
     *
     *  - RID is random (but stable) Identifier.
     *  - For pseudo-random function `F()` SHA-256 is used in this method.
     *  - `Net_Iface` is set to constant string "wpan".
     *  - `Network_ID` is not used  (optional per RFC 7217).
     *  - The `secret_key` is randomly generated on first use (using true
     *    random number generator) and saved in non-volatile settings for
     *    future use.
     *
     */

    Error                error      = kErrorFailed;
    const uint8_t        netIface[] = {'w', 'p', 'a', 'n'};
    IidSecretKey         secretKey;
    Crypto::Sha256       sha256;
    Crypto::Sha256::Hash hash;

    static_assert(sizeof(hash) >= Ip6::InterfaceIdentifier::kSize,
                  "SHA-256 hash size is too small to use as IPv6 address IID");

    GetIidSecretKey(secretKey);

    for (uint16_t count = 0; count < kMaxIidCreationAttempts; count++, aDadCounter++)
    {
        sha256.Start();
        sha256.Update(aAddress.mAddress.mFields.m8, BytesForBitSize(aAddress.mPrefixLength));

        sha256.Update(netIface);
        sha256.Update(aDadCounter);
        sha256.Update(secretKey);
        sha256.Finish(hash);

        aAddress.GetAddress().GetIid().SetBytes(hash.GetBytes());

        // If the IID is reserved, try again with a new dadCounter
        if (aAddress.GetAddress().GetIid().IsReserved())
        {
            continue;
        }

        // Exit and return the address if the IID is not reserved,
        ExitNow(error = kErrorNone);
    }

    LogWarn("Failed to generate a non-reserved IID after %d attempts", kMaxIidCreationAttempts);

exit:
    return error;
}

void Slaac::GetIidSecretKey(IidSecretKey &aKey) const
{
    Error error;

    error = Get<Settings>().Read<Settings::SlaacIidSecretKey>(aKey);
    VerifyOrExit(error != kErrorNone);

    // If there is no previously saved secret key, generate
    // a random one and save it.

    error = Random::Crypto::Fill(aKey);

    if (error != kErrorNone)
    {
        IgnoreError(Random::Crypto::Fill(aKey));
    }

    IgnoreError(Get<Settings>().Save<Settings::SlaacIidSecretKey>(aKey));

    LogInfo("Generated and saved secret key");

exit:
    return;
}

} // namespace Utils
} // namespace ot

#endif // OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
