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
namespace NetworkData {

void PrintExternalRouteConfig(const ExternalRouteConfig &aConfig)
{
    printf("\nprefix:");

    for (uint8_t b : aConfig.mPrefix.mPrefix.mFields.m8)
    {
        printf("%02x", b);
    }

    printf(", length:%d, rloc16:%04x, preference:%d, nat64:%d, stable:%d, nexthop:%d", aConfig.mPrefix.mLength,
           aConfig.mRloc16, aConfig.mPreference, aConfig.mNat64, aConfig.mStable, aConfig.mNextHopIsThisDevice);
}

// Returns true if the two given ExternalRouteConfig match (intentionally ignoring mNextHopIsThisDevice).
bool CompareExternalRouteConfig(const otExternalRouteConfig &aConfig1, const otExternalRouteConfig &aConfig2)
{
    return (memcmp(aConfig1.mPrefix.mPrefix.mFields.m8, aConfig2.mPrefix.mPrefix.mFields.m8,
                   sizeof(aConfig1.mPrefix.mPrefix)) == 0) &&
           (aConfig1.mPrefix.mLength == aConfig2.mPrefix.mLength) && (aConfig1.mRloc16 == aConfig2.mRloc16) &&
           (aConfig1.mPreference == aConfig2.mPreference) && (aConfig1.mStable == aConfig2.mStable);
}

void TestNetworkDataIterator(void)
{
    class TestNetworkData : public NetworkData
    {
    public:
        TestNetworkData(ot::Instance *aInstance, const uint8_t *aTlvs, uint8_t aTlvsLength)
            : NetworkData(*aInstance, kTypeLeader)
        {
            memcpy(mTlvs, aTlvs, aTlvsLength);
            mLength = aTlvsLength;
        }
    };

    ot::Instance *      instance;
    Iterator            iter = kIteratorInit;
    ExternalRouteConfig config;

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr, "Null OpenThread instance\n");

    {
        const uint8_t kNetworkData[] = {
            0x08, 0x04, 0x0B, 0x02, 0x00, 0x00, 0x03, 0x14, 0x00, 0x40, 0xFD, 0x00, 0x12, 0x34,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xC8, 0x00, 0x40, 0x01, 0x03, 0x54, 0x00, 0x00,
        };

        otExternalRouteConfig routes[] = {
            {
                {
                    {{{0xfd, 0x00, 0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00}}},
                    64,
                },
                0xc800, // mRloc16
                1,      // mPreference
                false,  // mNat64
                false,  // mStable
                false,  // mNextHopIsThisDevice
            },
            {
                {
                    {{{0xfd, 0x00, 0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00}}},
                    64,
                },
                0x5400, // mRloc16
                0,      // mPreference
                false,  // mNat64
                true,   // mStable
                false,  // mNextHopIsThisDevice
            },
        };

        TestNetworkData netData(instance, kNetworkData, sizeof(kNetworkData));

        iter = OT_NETWORK_DATA_ITERATOR_INIT;

        printf("\nTest #1: Network data 1");
        printf("\n-------------------------------------------------");

        for (const auto &route : routes)
        {
            SuccessOrQuit(netData.GetNextExternalRoute(iter, config), "GetNextExternalRoute() failed");
            PrintExternalRouteConfig(config);
            VerifyOrQuit(CompareExternalRouteConfig(config, route) == true,
                         "external route config does not match expectation");
        }
    }

    {
        const uint8_t kNetworkData[] = {
            0x08, 0x04, 0x0B, 0x02, 0x00, 0x00, 0x03, 0x1E, 0x00, 0x40, 0xFD, 0x00, 0x12, 0x34, 0x56, 0x78, 0x00, 0x00,
            0x07, 0x02, 0x11, 0x40, 0x00, 0x03, 0x10, 0x00, 0x40, 0x01, 0x03, 0x54, 0x00, 0x00, 0x05, 0x04, 0x54, 0x00,
            0x31, 0x00, 0x02, 0x0F, 0x00, 0x40, 0xFD, 0x00, 0xAB, 0xBA, 0xCD, 0xDC, 0x00, 0x00, 0x00, 0x03, 0x10, 0x00,
            0x20, 0x03, 0x0E, 0x00, 0x20, 0xFD, 0x00, 0xAB, 0xBA, 0x01, 0x06, 0x54, 0x00, 0x00, 0x04, 0x00, 0x00,
        };

        otExternalRouteConfig routes[] = {
            {
                {
                    {{{0xfd, 0x00, 0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00}}},
                    64,
                },
                0x1000, // mRloc16
                1,      // mPreference
                false,  // mNat64
                false,  // mStable
                false,  // mNextHopIsThisDevice
            },
            {
                {
                    {{{0xfd, 0x00, 0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00}}},
                    64,
                },
                0x5400, // mRloc16
                0,      // mPreference
                false,  // mNat64
                true,   // mStable
                false,  // mNextHopIsThisDevice
            },
            {
                {
                    {{{0xfd, 0x00, 0xab, 0xba, 0xcd, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00}}},
                    64,
                },
                0x1000, // mRloc16
                0,      // mPreference
                true,   // mNat64
                false,  // mStable
                false,  // mNextHopIsThisDevice
            },
            {
                {
                    {{{0xfd, 0x00, 0xab, 0xba, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00}}},
                    32,
                },
                0x5400, // mRloc16
                0,      // mPreference
                false,  // mNat64
                true,   // mStable
                false,  // mNextHopIsThisDevice
            },
            {
                {
                    {{{0xfd, 0x00, 0xab, 0xba, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00}}},
                    32,
                },
                0x0400, // mRloc16
                0,      // mPreference
                false,  // mNat64
                true,   // mStable
                false,  // mNextHopIsThisDevice
            },
        };

        TestNetworkData netData(instance, kNetworkData, sizeof(kNetworkData));

        iter = OT_NETWORK_DATA_ITERATOR_INIT;

        printf("\nTest #2: Network data 2");
        printf("\n-------------------------------------------------");

        for (const auto &route : routes)
        {
            SuccessOrQuit(netData.GetNextExternalRoute(iter, config), "GetNextExternalRoute() failed");
            PrintExternalRouteConfig(config);
            VerifyOrQuit(CompareExternalRouteConfig(config, route) == true,
                         "external route config does not match expectation");
        }
    }

    testFreeInstance(instance);
}

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

class TestNetworkData : public Local
{
public:
    explicit TestNetworkData(ot::Instance &aInstance)
        : Local(aInstance)
    {
    }

    template <uint8_t kSize> Error AddService(const uint8_t (&aServiceData)[kSize])
    {
        return Local::AddService(ServiceTlv::kThreadEnterpriseNumber, aServiceData, kSize, true, nullptr, 0);
    }

    template <uint8_t kSize>
    Error ValidateServiceData(const ServiceTlv *aServiceTlv, const uint8_t (&aServiceData)[kSize]) const
    {
        return

            ((aServiceTlv != nullptr) && (aServiceTlv->GetServiceDataLength() == kSize) &&
             (memcmp(aServiceTlv->GetServiceData(), aServiceData, kSize) == 0))
                ? kErrorNone
                : kErrorFailed;
    }

    void Test(void)
    {
        const uint8_t kServiceData1[] = {0x02};
        const uint8_t kServiceData2[] = {0xab};
        const uint8_t kServiceData3[] = {0xab, 0x00};
        const uint8_t kServiceData4[] = {0x02, 0xab, 0xcd, 0xef};
        const uint8_t kServiceData5[] = {0x02, 0xab, 0xcd};

        const ServiceTlv *tlv;

        SuccessOrQuit(AddService(kServiceData1), "AddService() failed");
        SuccessOrQuit(AddService(kServiceData2), "AddService() failed");
        SuccessOrQuit(AddService(kServiceData3), "AddService() failed");
        SuccessOrQuit(AddService(kServiceData4), "AddService() failed");
        SuccessOrQuit(AddService(kServiceData5), "AddService() failed");

        DumpBuffer("netdata", mTlvs, mLength);

        // Iterate through all entries that start with { 0x02 } (kServiceData1)
        tlv = nullptr;
        tlv = FindNextMatchingService(tlv, ServiceTlv::kThreadEnterpriseNumber, kServiceData1, sizeof(kServiceData1));
        SuccessOrQuit(ValidateServiceData(tlv, kServiceData1), "FindNextMatchingService() failed");
        tlv = FindNextMatchingService(tlv, ServiceTlv::kThreadEnterpriseNumber, kServiceData1, sizeof(kServiceData1));
        SuccessOrQuit(ValidateServiceData(tlv, kServiceData4), "FindNextMatchingService() failed");
        tlv = FindNextMatchingService(tlv, ServiceTlv::kThreadEnterpriseNumber, kServiceData1, sizeof(kServiceData1));
        SuccessOrQuit(ValidateServiceData(tlv, kServiceData5), "FindNextMatchingService() failed");
        tlv = FindNextMatchingService(tlv, ServiceTlv::kThreadEnterpriseNumber, kServiceData1, sizeof(kServiceData1));
        VerifyOrQuit(tlv == nullptr, "FindNextMatchingService() returned extra TLV");

        // Iterate through all entries that start with { 0xab } (kServiceData2)
        tlv = nullptr;
        tlv = FindNextMatchingService(tlv, ServiceTlv::kThreadEnterpriseNumber, kServiceData2, sizeof(kServiceData2));
        SuccessOrQuit(ValidateServiceData(tlv, kServiceData2), "FindNextMatchingService() failed");
        tlv = FindNextMatchingService(tlv, ServiceTlv::kThreadEnterpriseNumber, kServiceData2, sizeof(kServiceData2));
        SuccessOrQuit(ValidateServiceData(tlv, kServiceData3), "FindNextMatchingService() failed");
        tlv = FindNextMatchingService(tlv, ServiceTlv::kThreadEnterpriseNumber, kServiceData2, sizeof(kServiceData2));
        VerifyOrQuit(tlv == nullptr, "FindNextMatchingService() returned extra TLV");

        // Iterate through all entries that start with kServiceData5
        tlv = nullptr;
        tlv = FindNextMatchingService(tlv, ServiceTlv::kThreadEnterpriseNumber, kServiceData5, sizeof(kServiceData5));
        SuccessOrQuit(ValidateServiceData(tlv, kServiceData4), "FindNextMatchingService() failed");
        tlv = FindNextMatchingService(tlv, ServiceTlv::kThreadEnterpriseNumber, kServiceData5, sizeof(kServiceData5));
        SuccessOrQuit(ValidateServiceData(tlv, kServiceData5), "FindNextMatchingService() failed");
        tlv = FindNextMatchingService(tlv, ServiceTlv::kThreadEnterpriseNumber, kServiceData5, sizeof(kServiceData5));
        VerifyOrQuit(tlv == nullptr, "FindNextMatchingService() returned extra TLV");
    }
};

void TestNetworkDataFindNextService(void)
{
    ot::Instance *instance;

    printf("\n\n-------------------------------------------------");
    printf("\nTestNetworkDataFindNextService()\n");

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr, "Null OpenThread instance\n");

    {
        TestNetworkData netData(*instance);
        netData.Test();
    }
}

#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

} // namespace NetworkData
} // namespace ot

int main(void)
{
    ot::NetworkData::TestNetworkDataIterator();
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    ot::NetworkData::TestNetworkDataFindNextService();
#endif

    printf("\nAll tests passed\n");
    return 0;
}
