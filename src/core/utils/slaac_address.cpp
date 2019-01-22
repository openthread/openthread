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

#include "utils/wrap_string.h"

#include <openthread/ip6.h>
#include <openthread/netdata.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/random.hpp"
#include "crypto/sha256.hpp"
#include "mac/mac.hpp"
#include "net/ip6_address.hpp"

namespace ot {
namespace Utils {

void Slaac::UpdateAddresses(otInstance *    aInstance,
                            otNetifAddress *aAddresses,
                            uint32_t        aNumAddresses,
                            IidCreator      aIidCreator,
                            void *          aContext)
{
    otNetworkDataIterator iterator;
    otBorderRouterConfig  config;

    // remove addresses
    for (size_t i = 0; i < aNumAddresses; i++)
    {
        otNetifAddress *address = &aAddresses[i];
        bool            found   = false;

        if (!address->mValid)
        {
            continue;
        }

        iterator = OT_NETWORK_DATA_ITERATOR_INIT;

        while (otNetDataGetNextOnMeshPrefix(aInstance, &iterator, &config) == OT_ERROR_NONE)
        {
            if (config.mSlaac == false)
            {
                continue;
            }

            if (otIp6PrefixMatch(&config.mPrefix.mPrefix, &address->mAddress) >= config.mPrefix.mLength &&
                config.mPrefix.mLength == address->mPrefixLength)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            static_cast<Instance *>(aInstance)->GetThreadNetif().RemoveUnicastAddress(
                *static_cast<Ip6::NetifUnicastAddress *>(address));
            address->mValid = false;
        }
    }

    // add addresses
    iterator = OT_NETWORK_DATA_ITERATOR_INIT;

    while (otNetDataGetNextOnMeshPrefix(aInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        bool found = false;

        if (config.mSlaac == false)
        {
            continue;
        }

        for (const otNetifAddress *address = otIp6GetUnicastAddresses(aInstance); address != NULL;
             address                       = address->mNext)
        {
            if (otIp6PrefixMatch(&config.mPrefix.mPrefix, &address->mAddress) >= config.mPrefix.mLength &&
                config.mPrefix.mLength == address->mPrefixLength)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            for (size_t i = 0; i < aNumAddresses; i++)
            {
                otNetifAddress *address = &aAddresses[i];

                if (address->mValid)
                {
                    continue;
                }

                memset(address, 0, sizeof(*address));
                memcpy(&address->mAddress, &config.mPrefix.mPrefix, 8);

                address->mPrefixLength = config.mPrefix.mLength;
                address->mPreferred    = config.mPreferred;
                address->mValid        = true;

                if (aIidCreator(aInstance, address, aContext) != OT_ERROR_NONE)
                {
                    CreateRandomIid(aInstance, address, aContext);
                }

                static_cast<Instance *>(aInstance)->GetThreadNetif().AddUnicastAddress(
                    *static_cast<Ip6::NetifUnicastAddress *>(address));
                break;
            }
        }
    }
}

otError Slaac::CreateRandomIid(otInstance *, otNetifAddress *aAddress, void *)
{
    Random::FillBuffer(aAddress->mAddress.mFields.m8 + OT_IP6_ADDRESS_SIZE - OT_IP6_IID_SIZE, OT_IP6_IID_SIZE);
    return OT_ERROR_NONE;
}

otError SemanticallyOpaqueIidGenerator::CreateIid(otInstance *aInstance, otNetifAddress *aAddress)
{
    otError error = OT_ERROR_NONE;

    for (uint32_t i = 0; i <= kMaxRetries; i++)
    {
        error = CreateIidOnce(aInstance, aAddress);
        VerifyOrExit(error == OT_ERROR_IP6_ADDRESS_CREATION_FAILURE);

        mDadCounter++;
    }

exit:
    return error;
}

otError SemanticallyOpaqueIidGenerator::CreateIidOnce(otInstance *aInstance, otNetifAddress *aAddress)
{
    otError        error = OT_ERROR_NONE;
    Crypto::Sha256 sha256;
    uint8_t        hash[Crypto::Sha256::kHashSize];
    Ip6::Address * address = static_cast<Ip6::Address *>(&aAddress->mAddress);

    sha256.Start();

    sha256.Update(aAddress->mAddress.mFields.m8, aAddress->mPrefixLength / 8);

    VerifyOrExit(mInterfaceId != NULL, error = OT_ERROR_INVALID_ARGS);
    sha256.Update(mInterfaceId, mInterfaceIdLength);

    if (mNetworkIdLength)
    {
        VerifyOrExit(mNetworkId != NULL, error = OT_ERROR_INVALID_ARGS);
        sha256.Update(mNetworkId, mNetworkIdLength);
    }

    sha256.Update(static_cast<uint8_t *>(&mDadCounter), sizeof(mDadCounter));

    VerifyOrExit(mSecretKey != NULL, error = OT_ERROR_INVALID_ARGS);
    sha256.Update(mSecretKey, mSecretKeyLength);

    sha256.Finish(hash);

    memcpy(&aAddress->mAddress.mFields.m8[OT_IP6_ADDRESS_SIZE - OT_IP6_IID_SIZE], &hash[sizeof(hash) - OT_IP6_IID_SIZE],
           OT_IP6_IID_SIZE);

    VerifyOrExit(!IsAddressRegistered(aInstance, aAddress), error = OT_ERROR_IP6_ADDRESS_CREATION_FAILURE);
    VerifyOrExit(!address->IsIidReserved(), error = OT_ERROR_IP6_ADDRESS_CREATION_FAILURE);

exit:
    return error;
}

bool SemanticallyOpaqueIidGenerator::IsAddressRegistered(otInstance *aInstance, otNetifAddress *aCreatedAddress)
{
    bool                  result  = false;
    const otNetifAddress *address = otIp6GetUnicastAddresses(aInstance);

    while (address != NULL)
    {
        if (0 == memcmp(aCreatedAddress->mAddress.mFields.m8, address->mAddress.mFields.m8,
                        sizeof(address->mAddress.mFields)))
        {
            ExitNow(result = true);
        }

        address = address->mNext;
    }

exit:
    return result;
}

} // namespace Utils
} // namespace ot
