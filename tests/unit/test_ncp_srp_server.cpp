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

#include <openthread/srp_server.h>

#include "test_platform.h"
#include "test_util.h"
#include "common/code_utils.hpp"
#include "lib/spinel/spinel_buffer.hpp"
#include "lib/spinel/spinel_encoder.hpp"
#include "ncp/ncp_base.hpp"

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

namespace ot {

constexpr uint16_t kMaxSpinelBufferSize = 2048;

static otError GenerateSpinelSrpServerFrame(spinel_prop_key_t aProp, bool aEnable, uint8_t *aBuf, uint16_t &aLen)
{
    otError         error = OT_ERROR_NONE;
    uint8_t         buf[kMaxSpinelBufferSize];
    Spinel::Buffer  ncpBuffer(buf, kMaxSpinelBufferSize);
    Spinel::Encoder encoder(ncpBuffer);

    uint8_t header = SPINEL_HEADER_FLAG | 0 /* Iid */ | 1 /* Tid */;
    SuccessOrExit(error = encoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_SET, aProp));
    SuccessOrExit(error = encoder.WriteBool(aEnable));
    SuccessOrExit(error = encoder.EndFrame());

    SuccessOrExit(ncpBuffer.OutFrameBegin());
    aLen = ncpBuffer.OutFrameGetLength();
    VerifyOrExit(ncpBuffer.OutFrameRead(aLen, aBuf) == aLen, error = OT_ERROR_FAILED);

exit:
    return error;
}

void TestNcpSrpServerSetEnabled(void)
{
    Instance    *instance = static_cast<Instance *>(testInitInstance());
    Ncp::NcpBase ncpBase(instance);

    uint8_t          recvBuf[kMaxSpinelBufferSize];
    uint16_t         recvLen;
    otSrpServerState state;

    state = otSrpServerGetState(instance);
    VerifyOrQuit(state == OT_SRP_SERVER_STATE_DISABLED);

    SuccessOrQuit(GenerateSpinelSrpServerFrame(SPINEL_PROP_SRP_SERVER_ENABLED, true, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    state = otSrpServerGetState(instance);
    VerifyOrQuit(state == OT_SRP_SERVER_STATE_STOPPED);

    SuccessOrQuit(GenerateSpinelSrpServerFrame(SPINEL_PROP_SRP_SERVER_ENABLED, false, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    state = otSrpServerGetState(instance);
    VerifyOrQuit(state == OT_SRP_SERVER_STATE_DISABLED);

    printf("TestNcpSrpServerSetEnabled passed.\n");
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
void TestNcpSrpServerSetAutoEnableMode(void)
{
    Instance    *instance = static_cast<Instance *>(testInitInstance());
    Ncp::NcpBase ncpBase(instance);

    uint8_t  recvBuf[kMaxSpinelBufferSize];
    uint16_t recvLen;
    bool     isAutoEnableMode;

    isAutoEnableMode = otSrpServerIsAutoEnableMode(instance);
    VerifyOrQuit(isAutoEnableMode == false);

    SuccessOrQuit(GenerateSpinelSrpServerFrame(SPINEL_PROP_SRP_SERVER_AUTO_ENABLE_MODE, true, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    isAutoEnableMode = otSrpServerIsAutoEnableMode(instance);
    VerifyOrQuit(isAutoEnableMode == true);

    SuccessOrQuit(GenerateSpinelSrpServerFrame(SPINEL_PROP_SRP_SERVER_AUTO_ENABLE_MODE, false, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    isAutoEnableMode = otSrpServerIsAutoEnableMode(instance);
    VerifyOrQuit(isAutoEnableMode == false);

    printf("TestNcpSrpServerSetAutoEnableMode passed.\n");
}
#endif

} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    ot::TestNcpSrpServerSetEnabled();
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    ot::TestNcpSrpServerSetAutoEnableMode();
#endif
#endif
    printf("All tests passed\n");
    return 0;
}
