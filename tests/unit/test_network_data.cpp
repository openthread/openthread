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

#include <openthread/config.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "thread/network_data_local.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {

class TestNetworkData : public NetworkData::NetworkData
{
public:
    TestNetworkData(ot::Instance *aInstance, const uint8_t *aTlvs, uint8_t aTlvsLength)
        : NetworkData::NetworkData(*aInstance, kTypeLeader)
    {
        memcpy(mTlvs, aTlvs, aTlvsLength);
        mLength = aTlvsLength;
    }
};

void PrintExternalRouteConfig(const otExternalRouteConfig &aConfig)
{
    printf("\nprefix:");

    for (uint8_t i = 0; i < 16; i++)
    {
        printf("%02x", aConfig.mPrefix.mPrefix.mFields.m8[i]);
    }

    printf(", length:%d, rloc16:%04x, preference:%d, stable:%d, nexthop:%d", aConfig.mPrefix.mLength, aConfig.mRloc16,
           aConfig.mPreference, aConfig.mStable, aConfig.mNextHopIsThisDevice);
}

// Returns true if the two given otExternalRouteConfig match (intentionally ignoring mNextHopIsThisDevice).
bool CompareExternalRouteConfig(const otExternalRouteConfig &aConfig1, const otExternalRouteConfig &aConfig2)
{
    return (memcmp(aConfig1.mPrefix.mPrefix.mFields.m8, aConfig2.mPrefix.mPrefix.mFields.m8,
                   sizeof(aConfig1.mPrefix.mPrefix)) == 0) &&
           (aConfig1.mPrefix.mLength == aConfig2.mPrefix.mLength) && (aConfig1.mRloc16 == aConfig2.mRloc16) &&
           (aConfig1.mPreference == aConfig2.mPreference) && (aConfig1.mStable == aConfig2.mStable);
}

void TestNetworkDataIterator(void)
{
    ot::Instance *        instance;
    otNetworkDataIterator iter = OT_NETWORK_DATA_ITERATOR_INIT;
    otExternalRouteConfig config;

    instance = testInitInstance();
    VerifyOrQuit(instance != NULL, "Null OpenThread instance\n");

    {
        const uint8_t kNetworkData[] = {0x08, 0x04, 0x0B, 0x02, 0x00, 0x00, 0x03, 0x14, 0x00, 0x40,
                                        0xFD, 0x00, 0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
                                        0xC8, 0x00, 0x40, 0x01, 0x03, 0x54, 0x00, 0x00};

        otExternalRouteConfig routes[] = {
            {
                {{{{0xfd, 0x00, 0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
                 64},
                0xc800,
                1,
                false,
                false,
            },
            {
                {{{{0xfd, 0x00, 0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
                 64},
                0x5400,
                0,
                true,
                false,
            }};

        TestNetworkData netData(instance, kNetworkData, sizeof(kNetworkData));

        iter = OT_NETWORK_DATA_ITERATOR_INIT;

        printf("\nTest #1: Network data 1");
        printf("\n-------------------------------------------------");

        for (uint8_t i = 0; i < OT_ARRAY_LENGTH(routes); i++)
        {
            SuccessOrQuit(netData.GetNextExternalRoute(&iter, &config), "GetNextExternalRoute() failed\n");
            PrintExternalRouteConfig(config);
            VerifyOrQuit(CompareExternalRouteConfig(config, routes[i]) == true,
                         "external route config does not match expectation");
        }
    }

    {
        const uint8_t kNetworkData[] = {
            0x08, 0x04, 0x0B, 0x02, 0x00, 0x00, 0x03, 0x1E, 0x00, 0x40, 0xFD, 0x00, 0x12, 0x34, 0x56, 0x78, 0x00, 0x00,
            0x07, 0x02, 0x11, 0x40, 0x00, 0x03, 0x10, 0x00, 0x40, 0x01, 0x03, 0x54, 0x00, 0x00, 0x05, 0x04, 0x54, 0x00,
            0x31, 0x00, 0x02, 0x0F, 0x00, 0x40, 0xFD, 0x00, 0xAB, 0xBA, 0xCD, 0xDC, 0x00, 0x00, 0x00, 0x03, 0x10, 0x00,
            0x00, 0x03, 0x0E, 0x00, 0x20, 0xFD, 0x00, 0xAB, 0xBA, 0x01, 0x06, 0x54, 0x00, 0x00, 0x04, 0x00, 0x00};

        otExternalRouteConfig routes[] = {
            {{{{{0xfd, 0x00, 0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}}, 64},
             0x1000,
             1,
             false,
             false},
            {{{{{0xfd, 0x00, 0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}}, 64},
             0x5400,
             0,
             true,
             false},
            {{{{{0xfd, 0x00, 0xab, 0xba, 0xcd, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}}, 64},
             0x1000,
             0,
             false,
             false},
            {{{{{0xfd, 0x00, 0xab, 0xba, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}}, 32},
             0x5400,
             0,
             true,
             false},
            {{{{{0xfd, 0x00, 0xab, 0xba, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}}, 32},
             0x0400,
             0,
             true,
             false}};

        TestNetworkData netData(instance, kNetworkData, sizeof(kNetworkData));

        iter = OT_NETWORK_DATA_ITERATOR_INIT;

        printf("\nTest #2: Network data 2");
        printf("\n-------------------------------------------------");

        for (uint8_t i = 0; i < OT_ARRAY_LENGTH(routes); i++)
        {
            SuccessOrQuit(netData.GetNextExternalRoute(&iter, &config), "GetNextExternalRoute() failed\n");
            PrintExternalRouteConfig(config);
            VerifyOrQuit(CompareExternalRouteConfig(config, routes[i]) == true,
                         "external route config does not match expectation");
        }
    }

    testFreeInstance(instance);
}

} // namespace ot

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    ot::TestNetworkDataIterator();

    printf("\nAll tests passed\n");
    return 0;
}
#endif
