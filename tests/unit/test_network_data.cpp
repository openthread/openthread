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

#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "thread/network_data_leader.hpp"
#include "thread/network_data_local.hpp"
#include "thread/network_data_service.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {
namespace NetworkData {

void PrintExternalRouteConfig(const ExternalRouteConfig &aConfig)
{
    printf("\nroute-prefix:");

    for (uint8_t b : aConfig.mPrefix.mPrefix.mFields.m8)
    {
        printf("%02x", b);
    }

    printf(", length:%d, rloc16:%04x, preference:%d, nat64:%d, stable:%d, nexthop:%d", aConfig.mPrefix.mLength,
           aConfig.mRloc16, aConfig.mPreference, aConfig.mNat64, aConfig.mStable, aConfig.mNextHopIsThisDevice);
}

void PrintOnMeshPrefixConfig(const OnMeshPrefixConfig &aConfig)
{
    printf("\non-mesh-prefix:");

    for (uint8_t b : aConfig.mPrefix.mPrefix.mFields.m8)
    {
        printf("%02x", b);
    }

    printf(", length:%d, rloc16:%04x, preference:%d, stable:%d, def-route:%d", aConfig.mPrefix.mLength, aConfig.mRloc16,
           aConfig.mPreference, aConfig.mStable, aConfig.mDefaultRoute);
}

// Returns true if the two given ExternalRouteConfig match (intentionally ignoring mNextHopIsThisDevice).
bool CompareExternalRouteConfig(const otExternalRouteConfig &aConfig1, const otExternalRouteConfig &aConfig2)
{
    return (memcmp(aConfig1.mPrefix.mPrefix.mFields.m8, aConfig2.mPrefix.mPrefix.mFields.m8,
                   sizeof(aConfig1.mPrefix.mPrefix)) == 0) &&
           (aConfig1.mPrefix.mLength == aConfig2.mPrefix.mLength) && (aConfig1.mRloc16 == aConfig2.mRloc16) &&
           (aConfig1.mPreference == aConfig2.mPreference) && (aConfig1.mStable == aConfig2.mStable);
}

// Returns true if the two given OnMeshprefix match.
bool CompareOnMeshPrefixConfig(const otBorderRouterConfig &aConfig1, const otBorderRouterConfig &aConfig2)
{
    return (memcmp(aConfig1.mPrefix.mPrefix.mFields.m8, aConfig2.mPrefix.mPrefix.mFields.m8,
                   sizeof(aConfig1.mPrefix.mPrefix)) == 0) &&
           (aConfig1.mPrefix.mLength == aConfig2.mPrefix.mLength) && (aConfig1.mRloc16 == aConfig2.mRloc16) &&
           (aConfig1.mPreference == aConfig2.mPreference) && (aConfig1.mStable == aConfig2.mStable) &&
           (aConfig1.mDefaultRoute == aConfig2.mDefaultRoute) && (aConfig1.mOnMesh == aConfig2.mOnMesh);
}

template <uint8_t kLength>
void VerifyRlocsArray(const uint16_t *aRlocs, uint16_t aRlocsLength, const uint16_t (&aExpectedRlocs)[kLength])
{
    VerifyOrQuit(aRlocsLength == kLength);

    printf("\nRLOCs: { ");

    for (uint8_t index = 0; index < aRlocsLength; index++)
    {
        VerifyOrQuit(aRlocs[index] == aExpectedRlocs[index]);
        printf("0x%04x ", aRlocs[index]);
    }

    printf("}");
}

void TestNetworkDataIterator(void)
{
    static constexpr uint8_t kMaxRlocsArray = 10;

    ot::Instance *      instance;
    Iterator            iter = kIteratorInit;
    ExternalRouteConfig rconfig;
    OnMeshPrefixConfig  pconfig;
    uint16_t            rlocs[kMaxRlocsArray];
    uint8_t             rlocsLength;

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

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

        const uint16_t kRlocs[] = {0xc800, 0x5400};

        NetworkData netData(*instance, kNetworkData, sizeof(kNetworkData));

        iter = OT_NETWORK_DATA_ITERATOR_INIT;

        printf("\nTest #1: Network data 1");
        printf("\n-------------------------------------------------");

        for (const auto &route : routes)
        {
            SuccessOrQuit(netData.GetNextExternalRoute(iter, rconfig));
            PrintExternalRouteConfig(rconfig);
            VerifyOrQuit(CompareExternalRouteConfig(rconfig, route));
        }

        rlocsLength = GetArrayLength(rlocs);
        SuccessOrQuit(netData.FindBorderRouters(kAnyRole, rlocs, rlocsLength));
        VerifyRlocsArray(rlocs, rlocsLength, kRlocs);
        VerifyOrQuit(netData.CountBorderRouters(kAnyRole) == GetArrayLength(kRlocs));

        rlocsLength = GetArrayLength(rlocs);
        SuccessOrQuit(netData.FindBorderRouters(kRouterRoleOnly, rlocs, rlocsLength));
        VerifyRlocsArray(rlocs, rlocsLength, kRlocs);
        VerifyOrQuit(netData.CountBorderRouters(kRouterRoleOnly) == GetArrayLength(kRlocs));

        rlocsLength = GetArrayLength(rlocs);
        SuccessOrQuit(netData.FindBorderRouters(kChildRoleOnly, rlocs, rlocsLength));
        VerifyOrQuit(rlocsLength == 0);
        VerifyOrQuit(netData.CountBorderRouters(kChildRoleOnly) == 0);
    }

    {
        const uint8_t kNetworkData[] = {
            0x08, 0x04, 0x0B, 0x02, 0x00, 0x00, 0x03, 0x1E, 0x00, 0x40, 0xFD, 0x00, 0x12, 0x34, 0x56, 0x78, 0x00, 0x00,
            0x07, 0x02, 0x11, 0x40, 0x00, 0x03, 0x10, 0x00, 0x40, 0x01, 0x03, 0x54, 0x00, 0x00, 0x05, 0x04, 0x54, 0x00,
            0x31, 0x00, 0x02, 0x0F, 0x00, 0x40, 0xFD, 0x00, 0xAB, 0xBA, 0xCD, 0xDC, 0x00, 0x00, 0x00, 0x03, 0x10, 0x00,
            0x20, 0x03, 0x0E, 0x00, 0x20, 0xFD, 0x00, 0xAB, 0xBA, 0x01, 0x06, 0x54, 0x00, 0x00, 0x04, 0x01, 0x00,
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
                0x0401, // mRloc16
                0,      // mPreference
                false,  // mNat64
                true,   // mStable
                false,  // mNextHopIsThisDevice
            },
        };

        const uint16_t kRlocsAnyRole[]    = {0x1000, 0x5400, 0x0401};
        const uint16_t kRlocsRouterRole[] = {0x1000, 0x5400};
        const uint16_t kRlocsChildRole[]  = {0x0401};

        NetworkData netData(*instance, kNetworkData, sizeof(kNetworkData));

        iter = OT_NETWORK_DATA_ITERATOR_INIT;

        printf("\nTest #2: Network data 2");
        printf("\n-------------------------------------------------");

        for (const auto &route : routes)
        {
            SuccessOrQuit(netData.GetNextExternalRoute(iter, rconfig));
            PrintExternalRouteConfig(rconfig);
            VerifyOrQuit(CompareExternalRouteConfig(rconfig, route));
        }

        rlocsLength = GetArrayLength(rlocs);
        SuccessOrQuit(netData.FindBorderRouters(kAnyRole, rlocs, rlocsLength));
        VerifyRlocsArray(rlocs, rlocsLength, kRlocsAnyRole);
        VerifyOrQuit(netData.CountBorderRouters(kAnyRole) == GetArrayLength(kRlocsAnyRole));

        rlocsLength = GetArrayLength(rlocs);
        SuccessOrQuit(netData.FindBorderRouters(kRouterRoleOnly, rlocs, rlocsLength));
        VerifyRlocsArray(rlocs, rlocsLength, kRlocsRouterRole);
        VerifyOrQuit(netData.CountBorderRouters(kRouterRoleOnly) == GetArrayLength(kRlocsRouterRole));

        rlocsLength = GetArrayLength(rlocs);
        SuccessOrQuit(netData.FindBorderRouters(kChildRoleOnly, rlocs, rlocsLength));
        VerifyRlocsArray(rlocs, rlocsLength, kRlocsChildRole);
        VerifyOrQuit(netData.CountBorderRouters(kChildRoleOnly) == GetArrayLength(kRlocsChildRole));

        // Test failure case when given array is smaller than number of RLOCs.
        rlocsLength = GetArrayLength(kRlocsAnyRole) - 1;
        VerifyOrQuit(netData.FindBorderRouters(kAnyRole, rlocs, rlocsLength) == kErrorNoBufs);
        VerifyOrQuit(rlocsLength == GetArrayLength(kRlocsAnyRole) - 1);
        for (uint8_t index = 0; index < rlocsLength; index++)
        {
            VerifyOrQuit(rlocs[index] == kRlocsAnyRole[index]);
        }

        rlocsLength = GetArrayLength(kRlocsAnyRole);
        SuccessOrQuit(netData.FindBorderRouters(kAnyRole, rlocs, rlocsLength));
        VerifyRlocsArray(rlocs, rlocsLength, kRlocsAnyRole);
    }

    {
        const uint8_t kNetworkData[] = {
            0x08, 0x04, 0x0b, 0x02, 0x36, 0xcc, 0x03, 0x1c, 0x00, 0x40, 0xfd, 0x00, 0xbe, 0xef, 0xca, 0xfe,
            0x00, 0x00, 0x05, 0x0c, 0x28, 0x00, 0x33, 0x00, 0x28, 0x01, 0x33, 0x00, 0x4c, 0x00, 0x31, 0x00,
            0x07, 0x02, 0x11, 0x40, 0x03, 0x14, 0x00, 0x40, 0xfd, 0x00, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00,
            0x05, 0x04, 0x28, 0x00, 0x73, 0x00, 0x07, 0x02, 0x12, 0x40, 0x03, 0x12, 0x00, 0x40, 0xfd, 0x00,
            0x33, 0x33, 0x00, 0x00, 0x00, 0x00, 0x01, 0x06, 0xec, 0x00, 0x00, 0x28, 0x01, 0xc0,
        };

        otExternalRouteConfig routes[] = {
            {
                {
                    {{{0xfd, 0x00, 0x33, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00}}},
                    64,
                },
                0xec00, // mRloc16
                0,      // mPreference
                false,  // mNat64
                true,   // mStable
                false,  // mNextHopIsThisDevice
            },
            {
                {
                    {{{0xfd, 0x00, 0x33, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00}}},
                    64,
                },
                0x2801, // mRloc16
                -1,     // mPreference
                false,  // mNat64
                true,   // mStable
                false,  // mNextHopIsThisDevice
            },
        };

        otBorderRouterConfig prefixes[] = {
            {
                {
                    {{{0xfd, 0x00, 0xbe, 0xef, 0xca, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00}}},
                    64,
                },
                0,      // mPreference
                true,   // mPreferred
                true,   // mSlaac
                false,  // mDhcp
                true,   // mConfigure
                true,   // mDefaultRoute
                true,   // mOnMesh
                true,   // mStable
                false,  // mNdDns
                false,  // mDp
                0x2800, // mRloc16
            },
            {
                {
                    {{{0xfd, 0x00, 0xbe, 0xef, 0xca, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00}}},
                    64,
                },
                0,      // mPreference
                true,   // mPreferred
                true,   // mSlaac
                false,  // mDhcp
                true,   // mConfigure
                true,   // mDefaultRoute
                true,   // mOnMesh
                true,   // mStable
                false,  // mNdDns
                false,  // mDp
                0x2801, // mRloc16
            },
            {
                {
                    {{{0xfd, 0x00, 0xbe, 0xef, 0xca, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00}}},
                    64,
                },
                0,      // mPreference
                true,   // mPreferred
                true,   // mSlaac
                false,  // mDhcp
                true,   // mConfigure
                false,  // mDefaultRoute
                true,   // mOnMesh
                true,   // mStable
                false,  // mNdDns
                false,  // mDp
                0x4c00, // mRloc16
            },
            {
                {
                    {{{0xfd, 0x00, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00}}},
                    64,
                },
                1,      // mPreference
                true,   // mPreferred
                true,   // mSlaac
                false,  // mDhcp
                true,   // mConfigure
                true,   // mDefaultRoute
                true,   // mOnMesh
                true,   // mStable
                false,  // mNdDns
                false,  // mDp
                0x2800, // mRloc16
            },
        };

        const uint16_t kRlocsAnyRole[]    = {0xec00, 0x2801, 0x2800};
        const uint16_t kRlocsRouterRole[] = {0xec00, 0x2800};
        const uint16_t kRlocsChildRole[]  = {0x2801};

        NetworkData netData(*instance, kNetworkData, sizeof(kNetworkData));

        printf("\nTest #3: Network data 3");
        printf("\n-------------------------------------------------");

        iter = OT_NETWORK_DATA_ITERATOR_INIT;

        for (const auto &route : routes)
        {
            SuccessOrQuit(netData.GetNextExternalRoute(iter, rconfig));
            PrintExternalRouteConfig(rconfig);
            VerifyOrQuit(CompareExternalRouteConfig(rconfig, route));
        }

        iter = OT_NETWORK_DATA_ITERATOR_INIT;

        for (const auto &prefix : prefixes)
        {
            SuccessOrQuit(netData.GetNextOnMeshPrefix(iter, pconfig));
            PrintOnMeshPrefixConfig(pconfig);
            VerifyOrQuit(CompareOnMeshPrefixConfig(pconfig, prefix));
        }

        rlocsLength = GetArrayLength(rlocs);
        SuccessOrQuit(netData.FindBorderRouters(kAnyRole, rlocs, rlocsLength));
        VerifyRlocsArray(rlocs, rlocsLength, kRlocsAnyRole);
        VerifyOrQuit(netData.CountBorderRouters(kAnyRole) == GetArrayLength(kRlocsAnyRole));

        rlocsLength = GetArrayLength(rlocs);
        SuccessOrQuit(netData.FindBorderRouters(kRouterRoleOnly, rlocs, rlocsLength));
        VerifyRlocsArray(rlocs, rlocsLength, kRlocsRouterRole);
        VerifyOrQuit(netData.CountBorderRouters(kRouterRoleOnly) == GetArrayLength(kRlocsRouterRole));

        rlocsLength = GetArrayLength(rlocs);
        SuccessOrQuit(netData.FindBorderRouters(kChildRoleOnly, rlocs, rlocsLength));
        VerifyRlocsArray(rlocs, rlocsLength, kRlocsChildRole);
        VerifyOrQuit(netData.CountBorderRouters(kChildRoleOnly) == GetArrayLength(kRlocsChildRole));
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

    Error AddService(const ServiceData &aServiceData)
    {
        return Local::AddService(ServiceTlv::kThreadEnterpriseNumber, aServiceData, true, ServerData());
    }

    Error ValidateServiceData(const ServiceTlv *aServiceTlv, const ServiceData &aServiceData) const
    {
        Error       error = kErrorFailed;
        ServiceData serviceData;

        VerifyOrExit(aServiceTlv != nullptr);
        aServiceTlv->GetServiceData(serviceData);

        VerifyOrExit(aServiceData == serviceData);
        error = kErrorNone;

    exit:
        return error;
    }

    void Test(void)
    {
        const uint8_t kServiceData1[] = {0x02};
        const uint8_t kServiceData2[] = {0xab};
        const uint8_t kServiceData3[] = {0xab, 0x00};
        const uint8_t kServiceData4[] = {0x02, 0xab, 0xcd, 0xef};
        const uint8_t kServiceData5[] = {0x02, 0xab, 0xcd};

        const ServiceTlv *tlv;
        ServiceData       serviceData1;
        ServiceData       serviceData2;
        ServiceData       serviceData3;
        ServiceData       serviceData4;
        ServiceData       serviceData5;

        serviceData1.InitFrom(kServiceData1);
        serviceData2.InitFrom(kServiceData2);
        serviceData3.InitFrom(kServiceData3);
        serviceData4.InitFrom(kServiceData4);
        serviceData5.InitFrom(kServiceData5);

        SuccessOrQuit(AddService(serviceData1));
        SuccessOrQuit(AddService(serviceData2));
        SuccessOrQuit(AddService(serviceData3));
        SuccessOrQuit(AddService(serviceData4));
        SuccessOrQuit(AddService(serviceData5));

        DumpBuffer("netdata", GetBytes(), GetLength());

        // Iterate through all entries that start with { 0x02 } (kServiceData1)
        tlv = nullptr;
        tlv = FindNextService(tlv, ServiceTlv::kThreadEnterpriseNumber, serviceData1, kServicePrefixMatch);
        SuccessOrQuit(ValidateServiceData(tlv, serviceData1));
        tlv = FindNextService(tlv, ServiceTlv::kThreadEnterpriseNumber, serviceData1, kServicePrefixMatch);
        SuccessOrQuit(ValidateServiceData(tlv, serviceData4));
        tlv = FindNextService(tlv, ServiceTlv::kThreadEnterpriseNumber, serviceData1, kServicePrefixMatch);
        SuccessOrQuit(ValidateServiceData(tlv, serviceData5));
        tlv = FindNextService(tlv, ServiceTlv::kThreadEnterpriseNumber, serviceData1, kServicePrefixMatch);
        VerifyOrQuit(tlv == nullptr, "FindNextService() returned extra TLV");

        // Iterate through all entries that start with { 0xab } (serviceData2)
        tlv = nullptr;
        tlv = FindNextService(tlv, ServiceTlv::kThreadEnterpriseNumber, serviceData2, kServicePrefixMatch);
        SuccessOrQuit(ValidateServiceData(tlv, serviceData2));
        tlv = FindNextService(tlv, ServiceTlv::kThreadEnterpriseNumber, serviceData2, kServicePrefixMatch);
        SuccessOrQuit(ValidateServiceData(tlv, serviceData3));
        tlv = FindNextService(tlv, ServiceTlv::kThreadEnterpriseNumber, serviceData2, kServicePrefixMatch);
        VerifyOrQuit(tlv == nullptr, "FindNextService() returned extra TLV");

        // Iterate through all entries that start with serviceData5
        tlv = nullptr;
        tlv = FindNextService(tlv, ServiceTlv::kThreadEnterpriseNumber, serviceData5, kServicePrefixMatch);
        SuccessOrQuit(ValidateServiceData(tlv, serviceData4));
        tlv = FindNextService(tlv, ServiceTlv::kThreadEnterpriseNumber, serviceData5, kServicePrefixMatch);
        SuccessOrQuit(ValidateServiceData(tlv, serviceData5));
        tlv = FindNextService(tlv, ServiceTlv::kThreadEnterpriseNumber, serviceData5, kServicePrefixMatch);
        VerifyOrQuit(tlv == nullptr, "FindNextService() returned extra TLV");
    }
};

void TestNetworkDataFindNextService(void)
{
    ot::Instance *instance;

    printf("\n\n-------------------------------------------------");
    printf("\nTestNetworkDataFindNextService()\n");

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    {
        TestNetworkData netData(*instance);
        netData.Test();
    }
}

#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

void TestNetworkDataDsnSrpServices(void)
{
    static const char *kOriginStrings[] = {
        "service-data", // (0) Service::DnsSrpUnicast::kFromServiceData
        "server-data",  // (1) Service::DnsSrpUnicast::kFromServerData
    };

    class TestLeader : public Leader
    {
    public:
        void Populate(const uint8_t *aTlvs, uint8_t aTlvsLength)
        {
            memcpy(GetBytes(), aTlvs, aTlvsLength);
            SetLength(aTlvsLength);
        }
    };

    ot::Instance *instance;

    printf("\n\n-------------------------------------------------");
    printf("\nTestNetworkDataDsnSrpServices()\n");

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    {
        struct AnycastEntry
        {
            uint16_t mAloc16;
            uint8_t  mSequenceNumber;

            bool Matches(Service::DnsSrpAnycast::Info aInfo) const
            {
                VerifyOrQuit(aInfo.mAnycastAddress.GetIid().IsAnycastServiceLocator());

                return (aInfo.mAnycastAddress.GetIid().GetLocator() == mAloc16) &&
                       (aInfo.mSequenceNumber == mSequenceNumber);
            }
        };

        struct UnicastEntry
        {
            const char *                   mAddress;
            uint16_t                       mPort;
            Service::DnsSrpUnicast::Origin mOrigin;

            bool Matches(Service::DnsSrpUnicast::Info aInfo) const
            {
                Ip6::SockAddr sockAddr;

                SuccessOrQuit(sockAddr.GetAddress().FromString(mAddress));
                sockAddr.SetPort(mPort);

                return (aInfo.mSockAddr == sockAddr) && (aInfo.mOrigin == mOrigin);
            }
        };

        const uint8_t kNetworkData[] = {
            0x0b, 0x08, 0x80, 0x02, 0x5c, 0x02, 0x0d, 0x02, 0x28, 0x00, 0x0b, 0x08, 0x81, 0x02, 0x5c, 0xff, 0x0d, 0x02,
            0x6c, 0x00, 0x0b, 0x09, 0x82, 0x02, 0x5c, 0x03, 0x0d, 0x03, 0x4c, 0x00, 0xaa, 0x0b, 0x35, 0x83, 0x13, 0x5d,
            0xfd, 0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0x00, 0x2d, 0x0e, 0xc6, 0x27, 0x55, 0x56, 0x18, 0xd9, 0x12, 0x34,
            0x0d, 0x02, 0x00, 0x00, 0x0d, 0x14, 0x6c, 0x00, 0xfd, 0x00, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11,
            0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xab, 0xcd, 0x0d, 0x04, 0x28, 0x00, 0x56, 0x78, 0x0b, 0x23, 0x84, 0x01,
            0x5d, 0x0d, 0x02, 0x00, 0x00, 0x0d, 0x14, 0x4c, 0x00, 0xfd, 0x00, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde,
            0xf0, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0x00, 0x0e, 0x0d, 0x04, 0x6c, 0x00, 0xcd, 0x12,
        };

        const AnycastEntry kAnycastEntries[] = {
            {0xfc10, 0x02},
            {0xfc11, 0xff},
            {0xfc12, 0x03},
        };

        const UnicastEntry kUnicastEntries[] = {
            {"fdde:ad00:beef:0:2d0e:c627:5556:18d9", 0x1234, Service::DnsSrpUnicast::kFromServiceData},
            {"fd00:aabb:ccdd:eeff:11:2233:4455:6677", 0xabcd, Service::DnsSrpUnicast::kFromServerData},
            {"fdde:ad00:beef:0:0:ff:fe00:2800", 0x5678, Service::DnsSrpUnicast::kFromServerData},
            {"fd00:1234:5678:9abc:def0:123:4567:89ab", 0x0e, Service::DnsSrpUnicast::kFromServerData},
            {"fdde:ad00:beef:0:0:ff:fe00:6c00", 0xcd12, Service::DnsSrpUnicast::kFromServerData},
        };

        const uint8_t kPreferredAnycastEntryIndex = 2;

        Service::Manager &           manager = instance->Get<Service::Manager>();
        Service::Manager::Iterator   iterator;
        Service::DnsSrpAnycast::Info anycastInfo;
        Service::DnsSrpUnicast::Info unicastInfo;

        reinterpret_cast<TestLeader &>(instance->Get<Leader>()).Populate(kNetworkData, sizeof(kNetworkData));

        DumpBuffer("netdata", kNetworkData, sizeof(kNetworkData));

        // Verify all the "DNS/SRP Anycast Service" entries in Network Data

        printf("\n- - - - - - - - - - - - - - - - - - - -");
        printf("\nDNS/SRP Anycast Service entries\n");

        for (const AnycastEntry &entry : kAnycastEntries)
        {
            SuccessOrQuit(manager.GetNextDnsSrpAnycastInfo(iterator, anycastInfo));

            printf("\nanycastInfo { %s, seq:%d }", anycastInfo.mAnycastAddress.ToString().AsCString(),
                   anycastInfo.mSequenceNumber);

            VerifyOrQuit(entry.Matches(anycastInfo), "GetNextDnsSrpAnycastInfo() returned incorrect info");
        }

        VerifyOrQuit(manager.GetNextDnsSrpAnycastInfo(iterator, anycastInfo) == kErrorNotFound,
                     "GetNextDnsSrpAnycastInfo() returned unexpected extra entry");

        // Find the preferred "DNS/SRP Anycast Service" entries in Network Data

        SuccessOrQuit(manager.FindPreferredDnsSrpAnycastInfo(anycastInfo));

        printf("\n\nPreferred anycastInfo { %s, seq:%d }", anycastInfo.mAnycastAddress.ToString().AsCString(),
               anycastInfo.mSequenceNumber);

        VerifyOrQuit(kAnycastEntries[kPreferredAnycastEntryIndex].Matches(anycastInfo),
                     "FindPreferredDnsSrpAnycastInfo() returned invalid info");

        printf("\n\n- - - - - - - - - - - - - - - - - - - -");
        printf("\nDNS/SRP Unicast Service entries\n");

        iterator.Clear();

        for (const UnicastEntry &entry : kUnicastEntries)
        {
            SuccessOrQuit(manager.GetNextDnsSrpUnicastInfo(iterator, unicastInfo));
            printf("\nunicastInfo { %s, origin:%s }", unicastInfo.mSockAddr.ToString().AsCString(),
                   kOriginStrings[unicastInfo.mOrigin]);

            VerifyOrQuit(entry.Matches(unicastInfo), "GetNextDnsSrpUnicastInfo() returned incorrect info");
        }

        VerifyOrQuit(manager.GetNextDnsSrpUnicastInfo(iterator, unicastInfo) == kErrorNotFound,
                     "GetNextDnsSrpUnicastInfo() returned unexpected extra entry");

        printf("\n");
    }

    testFreeInstance(instance);
}

void TestNetworkDataDsnSrpAnycastSeqNumSelection(void)
{
    class TestLeader : public Leader
    {
    public:
        void Populate(const uint8_t *aTlvs, uint8_t aTlvsLength)
        {
            memcpy(GetBytes(), aTlvs, aTlvsLength);
            SetLength(aTlvsLength);
        }
    };

    struct TestInfo
    {
        const uint8_t *mNetworkData;
        uint8_t        mNetworkDataLength;
        const uint8_t *mSeqNumbers;
        uint8_t        mSeqNumbersLength;
        uint8_t        mPreferredSeqNum;
    };

    ot::Instance *instance;

    printf("\n\n-------------------------------------------------");
    printf("\nTestNetworkDataDsnSrpAnycastSeqNumSelection()\n");

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    const uint8_t kNetworkData1[] = {
        0x08, 0x04, 0x0b, 0x02, 0x50, 0xb0,                         // Service TLV
        0x0b, 0x08, 0x80, 0x02, 0x5c, 0x01, 0x0d, 0x02, 0x50, 0x00, // Server sub-TLV
        0x0b, 0x08, 0x81, 0x02, 0x5c, 0x81, 0x0d, 0x02, 0x50, 0x01, // Server sub-TLV
    };
    const uint8_t kSeqNumbers1[]    = {1, 129};
    const uint8_t kPreferredSeqNum1 = 129;

    const uint8_t kNetworkData2[] = {
        0x08, 0x04, 0x0b, 0x02, 0x50, 0xb0,                         // Service TLV
        0x0b, 0x08, 0x80, 0x02, 0x5c, 0x85, 0x0d, 0x02, 0x50, 0x00, // Server sub-TLV
        0x0b, 0x08, 0x81, 0x02, 0x5c, 0x05, 0x0d, 0x02, 0x50, 0x01, // Server sub-TLV
    };
    const uint8_t kSeqNumbers2[]    = {133, 5};
    const uint8_t kPreferredSeqNum2 = 133;

    const uint8_t kNetworkData3[] = {
        0x08, 0x04, 0x0b, 0x02, 0x50, 0xb0,                         // Service TLV
        0x0b, 0x08, 0x80, 0x02, 0x5c, 0x01, 0x0d, 0x02, 0x50, 0x00, // Server sub-TLV
        0x0b, 0x08, 0x81, 0x02, 0x5c, 0x02, 0x0d, 0x02, 0x50, 0x01, // Server sub-TLV
        0x0b, 0x08, 0x82, 0x02, 0x5c, 0xff, 0x0d, 0x02, 0x50, 0x02, // Server sub-TLV
    };
    const uint8_t kSeqNumbers3[]    = {1, 2, 255};
    const uint8_t kPreferredSeqNum3 = 2;

    const uint8_t kNetworkData4[] = {
        0x08, 0x04, 0x0b, 0x02, 0x50, 0xb0,                         // Service TLV
        0x0b, 0x08, 0x80, 0x02, 0x5c, 0x0a, 0x0d, 0x02, 0x50, 0x00, // Server sub-TLV
        0x0b, 0x08, 0x81, 0x02, 0x5c, 0x82, 0x0d, 0x02, 0x50, 0x01, // Server sub-TLV
        0x0b, 0x08, 0x82, 0x02, 0x5c, 0xfa, 0x0d, 0x02, 0x50, 0x02, // Server sub-TLV
    };
    const uint8_t kSeqNumbers4[]    = {10, 130, 250};
    const uint8_t kPreferredSeqNum4 = 250;

    const uint8_t kNetworkData5[] = {
        0x08, 0x04, 0x0b, 0x02, 0x50, 0xb0,                         // Service TLV
        0x0b, 0x08, 0x80, 0x02, 0x5c, 0x82, 0x0d, 0x02, 0x50, 0x00, // Server sub-TLV
        0x0b, 0x08, 0x81, 0x02, 0x5c, 0xfa, 0x0d, 0x02, 0x50, 0x01, // Server sub-TLV
        0x0b, 0x08, 0x82, 0x02, 0x5c, 0x0a, 0x0d, 0x02, 0x50, 0x02, // Server sub-TLV
    };
    const uint8_t kSeqNumbers5[]    = {130, 250, 10};
    const uint8_t kPreferredSeqNum5 = 250;

    const uint8_t kNetworkData6[] = {
        0x08, 0x04, 0x0b, 0x02, 0x50, 0xb0,                         // Service TLV
        0x0b, 0x08, 0x80, 0x02, 0x5c, 0xfa, 0x0d, 0x02, 0x50, 0x00, // Server sub-TLV
        0x0b, 0x08, 0x81, 0x02, 0x5c, 0x0a, 0x0d, 0x02, 0x50, 0x01, // Server sub-TLV
        0x0b, 0x08, 0x82, 0x02, 0x5c, 0x82, 0x0d, 0x02, 0x50, 0x02, // Server sub-TLV
    };
    const uint8_t kSeqNumbers6[]    = {250, 10, 130};
    const uint8_t kPreferredSeqNum6 = 250;

    const uint8_t kNetworkData7[] = {
        0x08, 0x04, 0x0b, 0x02, 0x50, 0xb0,                         // Service TLV
        0x0b, 0x08, 0x80, 0x02, 0x5c, 0xfa, 0x0d, 0x02, 0x50, 0x00, // Server sub-TLV
        0x0b, 0x08, 0x81, 0x02, 0x5c, 0x0a, 0x0d, 0x02, 0x50, 0x01, // Server sub-TLV
        0x0b, 0x08, 0x82, 0x02, 0x5c, 0x8A, 0x0d, 0x02, 0x50, 0x02, // Server sub-TLV
    };
    const uint8_t kSeqNumbers7[]    = {250, 10, 138};
    const uint8_t kPreferredSeqNum7 = 250;

    const uint8_t kNetworkData8[] = {
        0x08, 0x04, 0x0b, 0x02, 0x50, 0xb0,                         // Service TLV
        0x0b, 0x08, 0x80, 0x02, 0x5c, 0x01, 0x0d, 0x02, 0x50, 0x00, // Server sub-TLV
        0x0b, 0x08, 0x81, 0x02, 0x5c, 0x02, 0x0d, 0x02, 0x50, 0x01, // Server sub-TLV
        0x0b, 0x08, 0x82, 0x02, 0x5c, 0xff, 0x0d, 0x02, 0x50, 0x02, // Server sub-TLV
        0x0b, 0x08, 0x83, 0x02, 0x5c, 0xfe, 0x0d, 0x02, 0x50, 0x03, // Server sub-TLV

    };
    const uint8_t kSeqNumbers8[]    = {1, 2, 255, 254};
    const uint8_t kPreferredSeqNum8 = 2;

    const uint8_t kNetworkData9[] = {
        0x08, 0x04, 0x0b, 0x02, 0x50, 0xb0,                         // Service TLV
        0x0b, 0x08, 0x80, 0x02, 0x5c, 0x01, 0x0d, 0x02, 0x50, 0x00, // Server sub-TLV
        0x0b, 0x08, 0x81, 0x02, 0x5c, 0x02, 0x0d, 0x02, 0x50, 0x01, // Server sub-TLV
        0x0b, 0x08, 0x82, 0x02, 0x5c, 0xff, 0x0d, 0x02, 0x50, 0x02, // Server sub-TLV
        0x0b, 0x08, 0x83, 0x02, 0x5c, 0xfe, 0x0d, 0x02, 0x50, 0x03, // Server sub-TLV

    };
    const uint8_t kSeqNumbers9[]    = {1, 2, 255, 254};
    const uint8_t kPreferredSeqNum9 = 2;

    const uint8_t kNetworkData10[] = {
        0x08, 0x04, 0x0b, 0x02, 0x50, 0xb0,                         // Service TLV
        0x0b, 0x08, 0x80, 0x02, 0x5c, 0xfe, 0x0d, 0x02, 0x50, 0x00, // Server sub-TLV
        0x0b, 0x08, 0x81, 0x02, 0x5c, 0x02, 0x0d, 0x02, 0x50, 0x01, // Server sub-TLV
        0x0b, 0x08, 0x82, 0x02, 0x5c, 0x78, 0x0d, 0x02, 0x50, 0x02, // Server sub-TLV
        0x0b, 0x08, 0x83, 0x02, 0x5c, 0x01, 0x0d, 0x02, 0x50, 0x03, // Server sub-TLV

    };
    const uint8_t kSeqNumbers10[]    = {254, 2, 120, 1};
    const uint8_t kPreferredSeqNum10 = 120;

    const uint8_t kNetworkData11[] = {
        0x08, 0x04, 0x0b, 0x02, 0x50, 0xb0,                         // Service TLV
        0x0b, 0x08, 0x80, 0x02, 0x5c, 0xf0, 0x0d, 0x02, 0x50, 0x00, // Server sub-TLV
        0x0b, 0x08, 0x81, 0x02, 0x5c, 0x02, 0x0d, 0x02, 0x50, 0x01, // Server sub-TLV
        0x0b, 0x08, 0x82, 0x02, 0x5c, 0x78, 0x0d, 0x02, 0x50, 0x02, // Server sub-TLV
        0x0b, 0x08, 0x83, 0x02, 0x5c, 0x01, 0x0d, 0x02, 0x50, 0x03, // Server sub-TLV

    };
    const uint8_t kSeqNumbers11[]    = {240, 2, 120, 1};
    const uint8_t kPreferredSeqNum11 = 240;

    const TestInfo kTests[] = {
        {kNetworkData1, sizeof(kNetworkData1), kSeqNumbers1, sizeof(kSeqNumbers1), kPreferredSeqNum1},
        {kNetworkData2, sizeof(kNetworkData2), kSeqNumbers2, sizeof(kSeqNumbers2), kPreferredSeqNum2},
        {kNetworkData3, sizeof(kNetworkData3), kSeqNumbers3, sizeof(kSeqNumbers3), kPreferredSeqNum3},
        {kNetworkData4, sizeof(kNetworkData4), kSeqNumbers4, sizeof(kSeqNumbers4), kPreferredSeqNum4},
        {kNetworkData5, sizeof(kNetworkData5), kSeqNumbers5, sizeof(kSeqNumbers5), kPreferredSeqNum5},
        {kNetworkData6, sizeof(kNetworkData6), kSeqNumbers6, sizeof(kSeqNumbers6), kPreferredSeqNum6},
        {kNetworkData7, sizeof(kNetworkData7), kSeqNumbers7, sizeof(kSeqNumbers7), kPreferredSeqNum7},
        {kNetworkData8, sizeof(kNetworkData8), kSeqNumbers8, sizeof(kSeqNumbers8), kPreferredSeqNum8},
        {kNetworkData9, sizeof(kNetworkData9), kSeqNumbers9, sizeof(kSeqNumbers9), kPreferredSeqNum9},
        {kNetworkData10, sizeof(kNetworkData10), kSeqNumbers10, sizeof(kSeqNumbers10), kPreferredSeqNum10},
        {kNetworkData11, sizeof(kNetworkData11), kSeqNumbers11, sizeof(kSeqNumbers11), kPreferredSeqNum11},
    };

    Service::Manager &manager   = instance->Get<Service::Manager>();
    uint8_t           testIndex = 0;

    for (const TestInfo &test : kTests)
    {
        Service::Manager::Iterator   iterator;
        Service::DnsSrpAnycast::Info anycastInfo;

        reinterpret_cast<TestLeader &>(instance->Get<Leader>()).Populate(test.mNetworkData, test.mNetworkDataLength);

        printf("\n- - - - - - - - - - - - - - - - - - - -");
        printf("\nDNS/SRP Anycast Service entries for test %d", ++testIndex);

        for (uint8_t index = 0; index < test.mSeqNumbersLength; index++)
        {
            SuccessOrQuit(manager.GetNextDnsSrpAnycastInfo(iterator, anycastInfo));

            printf("\n { %s, seq:%d }", anycastInfo.mAnycastAddress.ToString().AsCString(),
                   anycastInfo.mSequenceNumber);

            VerifyOrQuit(anycastInfo.mSequenceNumber == test.mSeqNumbers[index]);
        }

        VerifyOrQuit(manager.GetNextDnsSrpAnycastInfo(iterator, anycastInfo) == kErrorNotFound);
        SuccessOrQuit(manager.FindPreferredDnsSrpAnycastInfo(anycastInfo));

        printf("\n preferred -> seq:%d ", anycastInfo.mSequenceNumber);
        VerifyOrQuit(anycastInfo.mSequenceNumber == test.mPreferredSeqNum);
    }

    testFreeInstance(instance);
}

} // namespace NetworkData
} // namespace ot

int main(void)
{
    ot::NetworkData::TestNetworkDataIterator();
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    ot::NetworkData::TestNetworkDataFindNextService();
#endif
    ot::NetworkData::TestNetworkDataDsnSrpServices();
    ot::NetworkData::TestNetworkDataDsnSrpAnycastSeqNumSelection();

    printf("\nAll tests passed\n");
    return 0;
}
