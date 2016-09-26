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

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <openthread.h>
#include <openthread-types.h>
#include <common/code_utils.hpp>
#include <utils/global_address.hpp>

#include <assert.h>
#include <string.h>

namespace Thread {
namespace Utils {

void Slaac::UpdateAddresses(otInstance *aInstance, otNetifAddress *aAddresses, uint32_t aNumAddresses,
                            IidCreator aIidCreator, void *aContext)
{
    otNetworkDataIterator iterator;
    otBorderRouterConfig config;

    // remove addresses
    for (size_t i = 0; i < aNumAddresses; i++)
    {
        otNetifAddress *address = &aAddresses[i];
        bool found = false;

        if (address->mValidLifetime == 0)
        {
            continue;
        }

        iterator = OT_NETWORK_DATA_ITERATOR_INIT;

        while (otGetNextOnMeshPrefix(aInstance, false, &iterator, &config) == kThreadError_None)
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
            otRemoveUnicastAddress(aInstance, &address->mAddress);
            address->mValidLifetime = 0;
        }
    }

    // add addresses
    iterator = OT_NETWORK_DATA_ITERATOR_INIT;

    while (otGetNextOnMeshPrefix(aInstance, false, &iterator, &config) == kThreadError_None)
    {
        bool found = false;

        if (config.mSlaac == false)
        {
            continue;
        }

        for (size_t i = 0; i < aNumAddresses; i++)
        {
            otNetifAddress *address = &aAddresses[i];

            if (address->mValidLifetime == 0)
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
            for (size_t i = 0; i < aNumAddresses; i++)
            {
                otNetifAddress *address = &aAddresses[i];

                if (address->mValidLifetime != 0)
                {
                    continue;
                }

                memset(address, 0, sizeof(*address));
                memcpy(&address->mAddress, &config.mPrefix.mPrefix, 8);

                address->mPrefixLength = config.mPrefix.mLength;
                address->mPreferredLifetime = config.mPreferred ? 0xffffffff : 0;
                address->mValidLifetime = 0xffffffff;

                if (aIidCreator(aInstance, address, aContext) != kThreadError_None)
                {
                    CreateRandomIid(aInstance, address, aContext);
                }

                otAddUnicastAddress(aInstance, address);
                break;
            }
        }
    }
}

ThreadError Slaac::CreateRandomIid(otInstance *aInstance, otNetifAddress *aAddress, void *aContext)
{
    (void) aInstance;
    (void) aContext;

    for (size_t i = sizeof(aAddress[i].mAddress) - OT_IP6_IID_SIZE; i < sizeof(aAddress[i].mAddress); i++)
    {
        aAddress->mAddress.mFields.m8[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    return kThreadError_None;
}


}  // namespace Slaac
}  // namespace Thread
