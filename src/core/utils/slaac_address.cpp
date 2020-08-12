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

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"
#include "common/settings.hpp"
#include "crypto/sha256.hpp"
#include "net/ip6_address.hpp"

#if OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE

namespace ot {
namespace Utils {

Slaac::Slaac(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(true)
    , mFilter(nullptr)
{
    memset(mAddresses, 0, sizeof(mAddresses));
}

void Slaac::Enable(void)
{
    VerifyOrExit(!mEnabled, OT_NOOP);

    otLogInfoUtil("SLAAC:: Enabling");
    mEnabled = true;
    Update(kModeAdd);

exit:
    return;
}

void Slaac::Disable(void)
{
    VerifyOrExit(mEnabled, OT_NOOP);

    otLogInfoUtil("SLAAC:: Disabling");
    mEnabled = false;
    Update(kModeRemove);

exit:
    return;
}

void Slaac::SetFilter(otIp6SlaacPrefixFilter aFilter)
{
    VerifyOrExit(aFilter != mFilter, OT_NOOP);

    mFilter = aFilter;
    otLogInfoUtil("SLAAC: Filter %s", (mFilter != nullptr) ? "updated" : "disabled");

    VerifyOrExit(mEnabled, OT_NOOP);
    Update(kModeAdd | kModeRemove);

exit:
    return;
}

bool Slaac::ShouldFilter(const Ip6::Prefix &aPrefix) const
{
    return (mFilter != nullptr) && mFilter(&GetInstance(), &aPrefix);
}

void Slaac::HandleNotifierEvents(Events aEvents)
{
    UpdateMode mode = kModeNone;

    VerifyOrExit(mEnabled, OT_NOOP);

    if (aEvents.Contains(kEventThreadNetdataChanged))
    {
        mode |= kModeAdd | kModeRemove;
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

        mode |= kModeAdd;
    }

    if (mode != kModeNone)
    {
        Update(mode);
    }

exit:
    return;
}

bool Slaac::DoesConfigMatchNetifAddr(const NetworkData::OnMeshPrefixConfig &aConfig,
                                     const Ip6::NetifUnicastAddress &       aAddr)
{
    return (((aConfig.mOnMesh && (aAddr.mPrefixLength == aConfig.mPrefix.mLength)) ||
             (!aConfig.mOnMesh && (aAddr.mPrefixLength == 128))) &&
            (aAddr.GetAddress().MatchesPrefix(aConfig.GetPrefix())));
}

void Slaac::Update(UpdateMode aMode)
{
    NetworkData::Iterator           iterator;
    NetworkData::OnMeshPrefixConfig config;
    bool                            found;

    if (aMode & kModeRemove)
    {
        // If enabled, remove any SLAAC addresses with no matching on-mesh prefix,
        // otherwise (when disabled) remove all previously added SLAAC addresses.

        for (Ip6::NetifUnicastAddress &slaacAddr : mAddresses)
        {
            if (!slaacAddr.mValid)
            {
                continue;
            }

            found = false;

            if (mEnabled)
            {
                iterator = NetworkData::kIteratorInit;

                while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, config) == OT_ERROR_NONE)
                {
                    if (config.mDp)
                    {
                        // Skip domain prefix which is processed in MLE.
                        continue;
                    }

                    if (config.mSlaac && !ShouldFilter(config.GetPrefix()) &&
                        DoesConfigMatchNetifAddr(config, slaacAddr))
                    {
                        found = true;
                        break;
                    }
                }
            }

            if (!found)
            {
                otLogInfoUtil("SLAAC: Removing address %s", slaacAddr.GetAddress().ToString().AsCString());

                Get<ThreadNetif>().RemoveUnicastAddress(slaacAddr);
                slaacAddr.mValid = false;
            }
        }
    }

    if ((aMode & kModeAdd) && mEnabled)
    {
        // Generate and add SLAAC addresses for any newly added on-mesh prefixes.

        iterator = NetworkData::kIteratorInit;

        while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, config) == OT_ERROR_NONE)
        {
            Ip6::Prefix &prefix = config.GetPrefix();

            if (config.mDp || !config.mSlaac || ShouldFilter(prefix))
            {
                continue;
            }

            found = false;

            for (const Ip6::NetifUnicastAddress *netifAddr = Get<ThreadNetif>().GetUnicastAddresses();
                 netifAddr != nullptr; netifAddr           = netifAddr->GetNext())
            {
                if (DoesConfigMatchNetifAddr(config, *netifAddr))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                bool added = false;

                for (Ip6::NetifUnicastAddress &slaacAddr : mAddresses)
                {
                    if (slaacAddr.mValid)
                    {
                        continue;
                    }

                    slaacAddr.InitAsSlaacOrigin(config.mOnMesh ? prefix.mLength : 128, config.mPreferred);
                    slaacAddr.GetAddress().SetPrefix(prefix);

                    IgnoreError(GenerateIid(slaacAddr));

                    otLogInfoUtil("SLAAC: Adding address %s", slaacAddr.GetAddress().ToString().AsCString());

                    Get<ThreadNetif>().AddUnicastAddress(slaacAddr);

                    added = true;
                    break;
                }

                if (!added)
                {
                    otLogWarnUtil("SLAAC: Failed to add - max %d addresses supported and already in use",
                                  OT_ARRAY_LENGTH(mAddresses));
                }
            }
        }
    }
}

otError Slaac::GenerateIid(Ip6::NetifUnicastAddress &aAddress,
                           uint8_t *                 aNetworkId,
                           uint8_t                   aNetworkIdLength,
                           uint8_t *                 aDadCounter) const
{
    /*
     *  This method generates a semantically opaque IID per RFC 7217.
     *
     * RID = F(Prefix, Net_Iface, Network_ID, DAD_Counter, secret_key)
     *
     *  - RID is random (but stable) Identifier.
     *  - For pseudo-random function `F()` SHA-256 is used in this method.
     *  - `Net_Iface` is set to constant string "wpan".
     *  - `Network_ID` is not used if `aNetworkId` is nullptr (optional per RF-7217).
     *  - The `secret_key` is randomly generated on first use (using true
     *    random number generator) and saved in non-volatile settings for
     *    future use.
     *
     */

    otError        error      = OT_ERROR_FAILED;
    const uint8_t  netIface[] = {'w', 'p', 'a', 'n'};
    uint8_t        dadCounter = aDadCounter ? *aDadCounter : 0;
    IidSecretKey   secretKey;
    Crypto::Sha256 sha256;
    uint8_t        hash[Crypto::Sha256::kHashSize];

    static_assert(sizeof(hash) >= Ip6::InterfaceIdentifier::kSize,
                  "SHA-256 hash size is too small to use as IPv6 address IID");

    GetIidSecretKey(secretKey);

    for (uint16_t count = 0; count < kMaxIidCreationAttempts; count++, dadCounter++)
    {
        sha256.Start();
        sha256.Update(aAddress.mAddress.mFields.m8, BitVectorBytes(aAddress.mPrefixLength));

        if (aNetworkId)
        {
            sha256.Update(aNetworkId, aNetworkIdLength);
        }

        sha256.Update(netIface, sizeof(netIface));
        sha256.Update(reinterpret_cast<uint8_t *>(&dadCounter), sizeof(dadCounter));
        sha256.Update(secretKey.m8, sizeof(IidSecretKey));
        sha256.Finish(hash);

        aAddress.GetAddress().GetIid().SetBytes(&hash[0]);

        // If the IID is reserved, try again with a new dadCounter
        if (aAddress.GetAddress().GetIid().IsReserved())
        {
            continue;
        }

        if (aDadCounter)
        {
            *aDadCounter = dadCounter;
        }

        // Exit and return the address if the IID is not reserved,
        ExitNow(error = OT_ERROR_NONE);
    }

    otLogWarnUtil("SLAAC: Failed to generate a non-reserved IID after %d attempts", kMaxIidCreationAttempts);

exit:
    return error;
}

void Slaac::GetIidSecretKey(IidSecretKey &aKey) const
{
    otError error;

    error = Get<Settings>().ReadSlaacIidSecretKey(aKey);
    VerifyOrExit(error != OT_ERROR_NONE, OT_NOOP);

    // If there is no previously saved secret key, generate
    // a random one and save it.

    error = Random::Crypto::FillBuffer(aKey.m8, sizeof(IidSecretKey));

    if (error != OT_ERROR_NONE)
    {
        IgnoreError(Random::Crypto::FillBuffer(aKey.m8, sizeof(IidSecretKey)));
    }

    IgnoreError(Get<Settings>().SaveSlaacIidSecretKey(aKey));

    otLogInfoUtil("SLAAC: Generated and saved secret key");

exit:
    return;
}

} // namespace Utils
} // namespace ot

#endif // OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
