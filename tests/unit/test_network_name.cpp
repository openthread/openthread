/*
 *  Copyright (c) 2022, The OpenThread Authors.
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

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "meshcop/network_name.hpp"

#include "test_util.h"

namespace ot {

void CompareNetworkName(const MeshCoP::NetworkName &aNetworkName, const char *aNameString)
{
    uint8_t len = static_cast<uint8_t>(strlen(aNameString));

    VerifyOrQuit(strcmp(aNetworkName.GetAsCString(), aNameString) == 0);

    VerifyOrQuit(aNetworkName.GetAsData().GetLength() == len);
    VerifyOrQuit(memcmp(aNetworkName.GetAsData().GetBuffer(), aNameString, len) == 0);
}

void TestNetworkName(void)
{
    const char kEmptyName[]   = "";
    const char kName1[]       = "network";
    const char kName2[]       = "network-name";
    const char kLongName[]    = "0123456789abcdef";
    const char kTooLongName[] = "0123456789abcdef0";

    char                 buffer[sizeof(kTooLongName) + 2];
    uint8_t              len;
    MeshCoP::NetworkName networkName;
    MeshCoP::NetworkName networkName2;

    CompareNetworkName(networkName, kEmptyName);

    SuccessOrQuit(networkName.Set(MeshCoP::NameData(kName1, sizeof(kName1))));
    CompareNetworkName(networkName, kName1);

    VerifyOrQuit(networkName.Set(MeshCoP::NameData(kName1, sizeof(kName1))) == kErrorAlready,
                 "failed to detect duplicate");
    CompareNetworkName(networkName, kName1);

    VerifyOrQuit(networkName.Set(MeshCoP::NameData(kName1, sizeof(kName1) - 1)) == kErrorAlready,
                 "failed to detect duplicate");

    SuccessOrQuit(networkName.Set(MeshCoP::NameData(kName2, sizeof(kName2))));
    CompareNetworkName(networkName, kName2);

    SuccessOrQuit(networkName.Set(MeshCoP::NameData(kEmptyName, 0)));
    CompareNetworkName(networkName, kEmptyName);

    SuccessOrQuit(networkName.Set(MeshCoP::NameData(kLongName, sizeof(kLongName))));
    CompareNetworkName(networkName, kLongName);

    VerifyOrQuit(networkName.Set(MeshCoP::NameData(kLongName, sizeof(kLongName) - 1)) == kErrorAlready,
                 "failed to detect duplicate");

    SuccessOrQuit(networkName.Set(MeshCoP::NameData(kName1, sizeof(kName1))));

    VerifyOrQuit(networkName.Set(MeshCoP::NameData(kTooLongName, sizeof(kTooLongName))) == kErrorInvalidArgs,
                 "accepted an invalid (too long) name");

    CompareNetworkName(networkName, kName1);

    memset(buffer, 'a', sizeof(buffer));
    len = networkName.GetAsData().CopyTo(buffer, 1);
    VerifyOrQuit(len == 1, "NameData::CopyTo() failed");
    VerifyOrQuit(buffer[0] == kName1[0], "NameData::CopyTo() failed");
    VerifyOrQuit(buffer[1] == 'a', "NameData::CopyTo() failed");

    memset(buffer, 'a', sizeof(buffer));
    len = networkName.GetAsData().CopyTo(buffer, sizeof(kName1) - 1);
    VerifyOrQuit(len == sizeof(kName1) - 1, "NameData::CopyTo() failed");
    VerifyOrQuit(memcmp(buffer, kName1, sizeof(kName1) - 1) == 0, "NameData::CopyTo() failed");
    VerifyOrQuit(buffer[sizeof(kName1)] == 'a', "NameData::CopyTo() failed");

    memset(buffer, 'a', sizeof(buffer));
    len = networkName.GetAsData().CopyTo(buffer, sizeof(buffer));
    VerifyOrQuit(len == sizeof(kName1) - 1, "NameData::CopyTo() failed");
    VerifyOrQuit(memcmp(buffer, kName1, sizeof(kName1) - 1) == 0, "NameData::CopyTo() failed");
    VerifyOrQuit(buffer[sizeof(kName1)] == 0, "NameData::CopyTo() failed");

    SuccessOrQuit(networkName2.Set(MeshCoP::NameData(kName1, sizeof(kName1))));
    VerifyOrQuit(networkName == networkName2);

    SuccessOrQuit(networkName2.Set(kName2));
    VerifyOrQuit(networkName != networkName2);
}

} // namespace ot

int main(void)
{
    ot::TestNetworkName();
    printf("All tests passed\n");
    return 0;
}
