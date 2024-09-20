/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include <stdio.h>

#include <openthread/border_routing.h>
#include <openthread/platform/infra_if.h>

#include "test_platform.h"
#include "test_util.h"
#include "common/code_utils.hpp"
#include "lib/spinel/spinel_buffer.hpp"
#include "lib/spinel/spinel_encoder.hpp"
#include "ncp/ncp_base.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE && OPENTHREAD_CONFIG_NCP_INFRA_IF_ENABLE

namespace ot {

constexpr uint16_t kMaxSpinelBufferSize = 2048;

static otError GenerateSpinelInfraIfStateFrame(uint32_t            akInfraIfIndex,
                                               bool                aIsRunning,
                                               const otIp6Address *aAddrs,
                                               uint8_t             aAddrCount,
                                               uint8_t            *aBuf,
                                               uint16_t           &aLen)
{
    otError         error = OT_ERROR_NONE;
    uint8_t         buf[kMaxSpinelBufferSize];
    Spinel::Buffer  ncpBuffer(buf, kMaxSpinelBufferSize);
    Spinel::Encoder encoder(ncpBuffer);

    uint8_t header = SPINEL_HEADER_FLAG | 0 /* Iid */ | 1 /* Tid */;
    SuccessOrExit(error = encoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_INFRA_IF_STATE));
    SuccessOrExit(error = encoder.WriteUint32(akInfraIfIndex));
    SuccessOrExit(error = encoder.WriteBool(true));
    for (uint8_t i = 0; i < aAddrCount; i++)
    {
        SuccessOrExit(error = encoder.WriteIp6Address(aAddrs[i]));
    }
    SuccessOrExit(error = encoder.EndFrame());

    SuccessOrExit(ncpBuffer.OutFrameBegin());
    aLen = ncpBuffer.OutFrameGetLength();
    VerifyOrExit(ncpBuffer.OutFrameRead(aLen, aBuf) == aLen, error = OT_ERROR_FAILED);

exit:
    return error;
}

void TestNcpInfraIfSetUp(void)
{
    Instance    *instance = static_cast<Instance *>(testInitInstance());
    Ncp::NcpBase ncpBase(instance);

    uint8_t            recvBuf[kMaxSpinelBufferSize];
    uint16_t           recvLen;
    constexpr uint32_t kInfraIfIndex = 1;

    const otIp6Address infraIfAddresses[] = {
        {0xfd, 0x35, 0x7a, 0x7d, 0x0f, 0x16, 0xe7, 0xe3, 0xc9, 0x79, 0x59, 0x29, 0xc8, 0xc2, 0xa3, 0x7b},
    };

    VerifyOrQuit(otBorderRoutingGetState(instance) == OT_BORDER_ROUTING_STATE_UNINITIALIZED);

    SuccessOrQuit(GenerateSpinelInfraIfStateFrame(kInfraIfIndex, true /* IsRunning */, infraIfAddresses,
                                                  sizeof(infraIfAddresses) / sizeof(infraIfAddresses[0]), recvBuf,
                                                  recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    VerifyOrQuit(otBorderRoutingGetState(instance) == OT_BORDER_ROUTING_STATE_STOPPED);
    VerifyOrQuit(otPlatInfraIfHasAddress(kInfraIfIndex, &infraIfAddresses[0]));
    VerifyOrQuit(!otPlatInfraIfHasAddress(kInfraIfIndex + 100, &infraIfAddresses[0]));

    SuccessOrQuit(
        GenerateSpinelInfraIfStateFrame(kInfraIfIndex, true /* IsRunning */, infraIfAddresses, 0, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    VerifyOrQuit(otBorderRoutingGetState(instance) == OT_BORDER_ROUTING_STATE_STOPPED);
    VerifyOrQuit(!otPlatInfraIfHasAddress(kInfraIfIndex, &infraIfAddresses[0]));

    printf("Test Ncp Infra If SetUp passed.\n");
}

void TestNcpInfraIfUpdate(void)
{
    Instance    *instance = static_cast<Instance *>(testInitInstance());
    Ncp::NcpBase ncpBase(instance);

    uint8_t            recvBuf[kMaxSpinelBufferSize];
    uint16_t           recvLen;
    constexpr uint32_t kInfraIfIndex1 = 1;
    constexpr uint32_t kInfraIfIndex2 = 2;

    const otIp6Address infraIfAddresses[] = {
        {0xfd, 0x35, 0x7a, 0x7d, 0x0f, 0x16, 0xe7, 0xe3, 0xc9, 0x79, 0x59, 0x29, 0xc8, 0xc2, 0xa3, 0x7b},
        {0xfd, 0x35, 0x7a, 0x7d, 0x0f, 0x16, 0xe7, 0xe3, 0x7b, 0xa3, 0xc2, 0xc8, 0x29, 0x59, 0x79, 0xc9},
    };

    SuccessOrQuit(
        GenerateSpinelInfraIfStateFrame(kInfraIfIndex1, true /* IsRunning */, infraIfAddresses, 1, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    VerifyOrQuit(otPlatInfraIfHasAddress(kInfraIfIndex1, &infraIfAddresses[0]));
    VerifyOrQuit(!otPlatInfraIfHasAddress(kInfraIfIndex1, &infraIfAddresses[1]));

    SuccessOrQuit(
        GenerateSpinelInfraIfStateFrame(kInfraIfIndex1, true /* IsRunning */, infraIfAddresses, 2, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    VerifyOrQuit(otPlatInfraIfHasAddress(kInfraIfIndex1, &infraIfAddresses[0]));
    VerifyOrQuit(otPlatInfraIfHasAddress(kInfraIfIndex1, &infraIfAddresses[1]));

    SuccessOrQuit(
        GenerateSpinelInfraIfStateFrame(kInfraIfIndex2, true /* IsRunning */, infraIfAddresses, 2, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    VerifyOrQuit(!otPlatInfraIfHasAddress(kInfraIfIndex1, &infraIfAddresses[0]));
    VerifyOrQuit(!otPlatInfraIfHasAddress(kInfraIfIndex1, &infraIfAddresses[1]));
    VerifyOrQuit(otPlatInfraIfHasAddress(kInfraIfIndex2, &infraIfAddresses[0]));
    VerifyOrQuit(otPlatInfraIfHasAddress(kInfraIfIndex2, &infraIfAddresses[1]));
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE && OPENTHREAD_CONFIG_NCP_INFRA_IF_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE && OPENTHREAD_CONFIG_NCP_INFRA_IF_ENABLE
    ot::TestNcpInfraIfSetUp();
    ot::TestNcpInfraIfUpdate();
#endif
    printf("All tests passed\n");
    return 0;
}
