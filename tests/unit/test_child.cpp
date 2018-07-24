/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#include "test_platform.h"

#include <openthread/config.h>

#include "test_util.h"
#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "thread/topology.hpp"

namespace ot {

static ot::Instance *sInstance;

enum
{
    kMaxChildIp6Addresses = OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD,
};

void VerifyChildIp6Addresses(const Child &aChild, uint8_t aAddressListLength, const Ip6::Address aAddressList[])
{
    Child::Ip6AddressIterator iterator;
    Ip6::Address              address;
    bool                      addressObserved[kMaxChildIp6Addresses];
    bool                      addressIsMeshLocal[kMaxChildIp6Addresses];
    bool                      hasMeshLocal = false;

    for (uint8_t index = 0; index < aAddressListLength; index++)
    {
        VerifyOrQuit(aChild.HasIp6Address(*sInstance, aAddressList[index]), "HasIp6Address() failed\n");
    }

    memset(addressObserved, 0, sizeof(addressObserved));
    memset(addressIsMeshLocal, 0, sizeof(addressObserved));

    for (uint8_t index = 0; index < aAddressListLength; index++)
    {
        {
            addressIsMeshLocal[index] = true;
        }
    }

    while (aChild.GetNextIp6Address(*sInstance, iterator, address) == OT_ERROR_NONE)
    {
        bool addressIsInList = false;

        for (uint8_t index = 0; index < aAddressListLength; index++)
        {
            if (address == aAddressList[index])
            {
                addressIsInList        = true;
                addressObserved[index] = true;
                break;
            }
        }

        VerifyOrQuit(addressIsInList, "Child::GetNextIp6Address() returned an address not in the expected list\n");
    }

    for (uint8_t index = 0; index < aAddressListLength; index++)
    {
        VerifyOrQuit(addressObserved[index], "Child::GetNextIp6Address() missed an entry from the expected list\n");

        if (sInstance->GetThreadNetif().GetMle().IsMeshLocalAddress(aAddressList[index]))
        {
            SuccessOrQuit(aChild.GetMeshLocalIp6Address(*sInstance, address),
                          "Child::GetMeshLocalIp6Address() failed\n");
            VerifyOrQuit(address == aAddressList[index], "GetMeshLocalIp6Address() did not return expected address\n");
            hasMeshLocal = true;
        }
    }

    if (!hasMeshLocal)
    {
        VerifyOrQuit(aChild.GetMeshLocalIp6Address(*sInstance, address) == OT_ERROR_NOT_FOUND,
                     "Child::GetMeshLocalIp6Address() returned an address not in the exptect list\n");
    }
}

void TestChildIp6Address(void)
{
    Child        child;
    Ip6::Address addresses[kMaxChildIp6Addresses];
    uint8_t      numAddresses;
    const char * ip6Addresses[] = {
        "fd00:1234::1234",
        "fd6b:e251:52fb:0:12e6:b94c:1c28:c56a",
        "fd00:1234::204c:3d7c:98f6:9a1b",
    };

    const uint8_t meshLocalIid[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

    sInstance = testInitInstance();
    VerifyOrQuit(sInstance != NULL, "Null instance");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    printf("\nConverting IPv6 addresses from string");

    numAddresses = 0;

    // First addresses uses the mesh local prefix (mesh-local address).
    addresses[numAddresses] = sInstance->GetThreadNetif().GetMle().GetMeshLocal64();
    addresses[numAddresses].SetIid(meshLocalIid);

    numAddresses++;

    for (uint8_t index = 0; index < static_cast<uint8_t>(OT_ARRAY_LENGTH(ip6Addresses)); index++)
    {
        VerifyOrQuit(numAddresses < kMaxChildIp6Addresses, "Too many IPv6 addresses in the unit test");
        SuccessOrQuit(addresses[numAddresses++].FromString(ip6Addresses[index]),
                      "could not convert IPv6 address from string");
    }

    printf(" -- PASS\n");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    printf("Child state after init");
    memset(&child, 0, sizeof(child));
    VerifyChildIp6Addresses(child, 0, NULL);
    printf(" -- PASS\n");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    printf("Adding a single IPv6 address");

    for (uint8_t index = 0; index < numAddresses; index++)
    {
        SuccessOrQuit(child.AddIp6Address(*sInstance, addresses[index]), "AddIp6Address() failed");
        VerifyChildIp6Addresses(child, 1, &addresses[index]);

        child.ClearIp6Addresses();
        VerifyChildIp6Addresses(child, 0, NULL);
    }

    printf(" -- PASS\n");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    printf("Adding multiple IPv6 addresses");

    for (uint8_t index = 0; index < numAddresses; index++)
    {
        SuccessOrQuit(child.AddIp6Address(*sInstance, addresses[index]), "AddIp6Address() failed");
        VerifyChildIp6Addresses(child, index + 1, addresses);
    }

    printf(" -- PASS\n");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    printf("Checking for failure when adding an address already in list");

    for (uint8_t index = 0; index < numAddresses; index++)
    {
        VerifyOrQuit(child.AddIp6Address(*sInstance, addresses[index]) == OT_ERROR_ALREADY,
                     "AddIp6Address() did not fail when adding same address");
        VerifyChildIp6Addresses(child, numAddresses, addresses);
    }

    printf(" -- PASS\n");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    printf("Removing addresses from list starting from front of the list");

    for (uint8_t index = 0; index < numAddresses; index++)
    {
        SuccessOrQuit(child.RemoveIp6Address(*sInstance, addresses[index]), "RemoveIp6Address() failed");
        VerifyChildIp6Addresses(child, numAddresses - 1 - index, &addresses[index + 1]);

        VerifyOrQuit(child.RemoveIp6Address(*sInstance, addresses[index]) == OT_ERROR_NOT_FOUND,
                     "RemoveIp6Address() did not fail when removing an address not on the list");
    }

    VerifyChildIp6Addresses(child, 0, NULL);
    printf(" -- PASS\n");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    printf("Removing addresses from list starting from back of the list");

    for (uint8_t index = 0; index < numAddresses; index++)
    {
        SuccessOrQuit(child.AddIp6Address(*sInstance, addresses[index]), "AddIp6Address() failed");
    }

    for (uint8_t index = numAddresses - 1; index > 0; index--)
    {
        SuccessOrQuit(child.RemoveIp6Address(*sInstance, addresses[index]), "RemoveIp6Address() failed");
        VerifyChildIp6Addresses(child, index, &addresses[0]);

        VerifyOrQuit(child.RemoveIp6Address(*sInstance, addresses[index]) == OT_ERROR_NOT_FOUND,
                     "RemoveIp6Address() did not fail when removing an address not on the list");
    }

    printf(" -- PASS\n");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    printf("Removing address entries from middle of the list");

    for (uint8_t indexToRemove = 1; indexToRemove < numAddresses - 1; indexToRemove++)
    {
        child.ClearIp6Addresses();

        for (uint8_t index = 0; index < numAddresses; index++)
        {
            SuccessOrQuit(child.AddIp6Address(*sInstance, addresses[index]), "AddIp6Address() failed");
        }

        SuccessOrQuit(child.RemoveIp6Address(*sInstance, addresses[indexToRemove]), "RemoveIp6Address() failed");

        VerifyOrQuit(child.RemoveIp6Address(*sInstance, addresses[indexToRemove]) == OT_ERROR_NOT_FOUND,
                     "RemoveIp6Address() did not fail when removing an address not on the list");

        {
            Ip6::Address updatedAddressList[kMaxChildIp6Addresses];
            uint8_t      updatedListIndex = 0;

            for (uint8_t index = 0; index < numAddresses; index++)
            {
                if (index != indexToRemove)
                {
                    updatedAddressList[updatedListIndex++] = addresses[index];
                }
            }

            VerifyChildIp6Addresses(child, updatedListIndex, updatedAddressList);
        }
    }

    printf(" -- PASS\n");

    testFreeInstance(sInstance);
}

} // namespace ot

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    ot::TestChildIp6Address();
    printf("\nAll tests passed.\n");
    return 0;
}
#endif
