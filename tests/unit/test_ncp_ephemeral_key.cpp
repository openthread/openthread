/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include <openthread/border_agent.h>

#include "test_platform.h"
#include "test_util.h"
#include "common/code_utils.hpp"
#include "lib/spinel/spinel_buffer.hpp"
#include "lib/spinel/spinel_encoder.hpp"
#include "ncp/ncp_base.hpp"

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

namespace ot {

constexpr uint16_t kMaxSpinelBufferSize = 2048;

static otError GenerateEphemeralKeySetEnabledFrame(bool aEnable, uint8_t *aBuf, uint16_t &aLen)
{
    otError         error = OT_ERROR_NONE;
    uint8_t         buf[kMaxSpinelBufferSize];
    Spinel::Buffer  ncpBuffer(buf, kMaxSpinelBufferSize);
    Spinel::Encoder encoder(ncpBuffer);

    uint8_t header = SPINEL_HEADER_FLAG | 0 /* Iid */ | 1 /* Tid */;
    SuccessOrExit(
        error = encoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_BORDER_AGENT_EPHEMERAL_KEY_ENABLE));
    SuccessOrExit(error = encoder.WriteBool(aEnable));
    SuccessOrExit(error = encoder.EndFrame());

    SuccessOrExit(ncpBuffer.OutFrameBegin());
    aLen = ncpBuffer.OutFrameGetLength();
    VerifyOrExit(ncpBuffer.OutFrameRead(aLen, aBuf) == aLen, error = OT_ERROR_FAILED);

exit:
    return error;
}

static otError GenerateEphemeralKeyStartFrame(const char *aEpskc,
                                              uint32_t    aTimeout,
                                              uint16_t    aPort,
                                              uint8_t    *aBuf,
                                              uint16_t   &aLen)
{
    otError         error = OT_ERROR_NONE;
    uint8_t         buf[kMaxSpinelBufferSize];
    Spinel::Buffer  ncpBuffer(buf, kMaxSpinelBufferSize);
    Spinel::Encoder encoder(ncpBuffer);

    uint8_t header = SPINEL_HEADER_FLAG | 0 /* Iid */ | 1 /* Tid */;
    SuccessOrExit(
        error = encoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_BORDER_AGENT_EPHEMERAL_KEY_ACTIVATE));
    SuccessOrExit(error = encoder.WriteUtf8(aEpskc));
    SuccessOrExit(error = encoder.WriteUint32(aTimeout));
    SuccessOrExit(error = encoder.WriteUint16(aPort));
    SuccessOrExit(error = encoder.EndFrame());

    SuccessOrExit(ncpBuffer.OutFrameBegin());
    aLen = ncpBuffer.OutFrameGetLength();
    VerifyOrExit(ncpBuffer.OutFrameRead(aLen, aBuf) == aLen, error = OT_ERROR_FAILED);

exit:
    return error;
}

static otError GenerateEphemeralKeyStopFrame(uint8_t *aBuf, uint16_t &aLen)
{
    otError         error = OT_ERROR_NONE;
    uint8_t         buf[kMaxSpinelBufferSize];
    Spinel::Buffer  ncpBuffer(buf, kMaxSpinelBufferSize);
    Spinel::Encoder encoder(ncpBuffer);

    uint8_t header = SPINEL_HEADER_FLAG | 0 /* Iid */ | 1 /* Tid */;
    SuccessOrExit(error = encoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_SET,
                                             SPINEL_PROP_BORDER_AGENT_EPHEMERAL_KEY_DEACTIVATE));
    SuccessOrExit(error = encoder.WriteBool(false)); // Not retain active session. Doesn't matter here.
    SuccessOrExit(error = encoder.EndFrame());

    SuccessOrExit(ncpBuffer.OutFrameBegin());
    aLen = ncpBuffer.OutFrameGetLength();
    VerifyOrExit(ncpBuffer.OutFrameRead(aLen, aBuf) == aLen, error = OT_ERROR_FAILED);

exit:
    return error;
}

void TestEphemeralKeySetEnabled(void)
{
    Instance    *instance = static_cast<Instance *>(testInitInstance());
    Ncp::NcpBase ncpBase(instance);

    uint8_t  recvBuf[kMaxSpinelBufferSize];
    uint16_t recvLen;

    // Set Ephemeral Key feature to enabled.
    SuccessOrQuit(GenerateEphemeralKeySetEnabledFrame(true, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);

    // The state should be 'Stopped' after enabling it.
    VerifyOrQuit(otBorderAgentEphemeralKeyGetState(instance) == OT_BORDER_AGENT_STATE_STOPPED);

    // Set Ephemeral Key feature to disabled.
    SuccessOrQuit(GenerateEphemeralKeySetEnabledFrame(false, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);

    // The state should be 'Disabled' after disabling it.
    VerifyOrQuit(otBorderAgentEphemeralKeyGetState(instance) == OT_BORDER_AGENT_STATE_DISABLED);

    // Set Ephemeral Key feature to enabled again.
    SuccessOrQuit(GenerateEphemeralKeySetEnabledFrame(true, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);

    // The state should be 'Stopped' after enabling it.
    VerifyOrQuit(otBorderAgentEphemeralKeyGetState(instance) == OT_BORDER_AGENT_STATE_STOPPED);
}

void TestEphemeralKeyStartAndStop(void)
{
    Instance    *instance = static_cast<Instance *>(testInitInstance());
    Ncp::NcpBase ncpBase(instance);

    uint8_t  recvBuf[kMaxSpinelBufferSize];
    uint16_t recvLen;

    // Set Ephemeral Key feature to enabled.
    SuccessOrQuit(GenerateEphemeralKeySetEnabledFrame(true, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    VerifyOrQuit(otBorderAgentEphemeralKeyGetState(instance) == OT_BORDER_AGENT_STATE_STOPPED);

    // Activate the Ephemeral Key mode.
    SuccessOrQuit(GenerateEphemeralKeyStartFrame("123456789", 300000, 12345, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    VerifyOrQuit(otBorderAgentEphemeralKeyGetState(instance) == OT_BORDER_AGENT_STATE_STARTED);

    // Deactivate the Ephemeral Key mode.
    SuccessOrQuit(GenerateEphemeralKeyStopFrame(recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    VerifyOrQuit(otBorderAgentEphemeralKeyGetState(instance) == OT_BORDER_AGENT_STATE_STOPPED);
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    ot::TestEphemeralKeySetEnabled();
    ot::TestEphemeralKeyStartAndStop();
#endif
    printf("All tests passed\n");
    return 0;
}
