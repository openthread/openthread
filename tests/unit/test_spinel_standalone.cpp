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

#include <string.h>

#include <openthread/platform/time.h>

#include "lib/spinel/spinel_decoder.hpp"
#include "lib/spinel/spinel_driver.hpp"
#include "lib/spinel/spinel_encoder.hpp"

#include "test_util.h"

// The consumer must inherit the configuration from the library target.
#include "lib/spinel/openthread-spinel-config.h"
#if !OPENTHREAD_SPINEL_CONFIG_STANDALONE_ENABLE || OPENTHREAD_SPINEL_CONFIG_LOG_ENABLE || \
    OPENTHREAD_SPINEL_CONFIG_OPENTHREAD_MESSAGE_ENABLE
#error "Standalone Spinel target must propagate its configuration"
#endif

// The standalone host supplies its own platform clock.
extern "C" uint64_t otPlatTimeGet(void) { return 0; }

// A transport supplying ordinary, deterministic co-processor responses.
class TestInterface : public ot::Spinel::SpinelInterface
{
public:
    otError Init(ReceiveFrameCallback aCallback, void *aContext, RxFrameBuffer &aBuffer) override
    {
        mCallback = aCallback;
        mContext  = aContext;
        mBuffer   = &aBuffer;
        mLength   = 0;
        mRequests = 0;
        return OT_ERROR_NONE;
    }

    void                         Deinit(void) override {}
    void                         UpdateFdSet(void *) override {}
    void                         Process(const void *) override {}
    uint32_t                     GetBusSpeed(void) const override { return 0; }
    otError                      HardwareReset(void) override { return OT_ERROR_NOT_IMPLEMENTED; }
    const otRcpInterfaceMetrics *GetRcpInterfaceMetrics(void) const override { return nullptr; }

    otError SendFrame(const uint8_t *aFrame, uint16_t aLength) override
    {
        uint8_t        header;
        unsigned int   command;
        unsigned int   key;
        spinel_ssize_t length;

        VerifyOrQuit(mLength == 0);
        VerifyOrQuit(spinel_datatype_unpack(aFrame, aLength, "Ci", &header, &command) > 0);
        if (command == SPINEL_CMD_RESET)
        {
            length = spinel_datatype_pack(mReply, sizeof(mReply), "Ciii", header, SPINEL_CMD_PROP_VALUE_IS,
                                          SPINEL_PROP_LAST_STATUS, SPINEL_STATUS_RESET_SOFTWARE);
        }
        else
        {
            VerifyOrQuit(command == SPINEL_CMD_PROP_VALUE_GET);
            VerifyOrQuit(spinel_datatype_unpack(aFrame, aLength, "Cii", &header, &command, &key) > 0);
            switch (key)
            {
            case SPINEL_PROP_PROTOCOL_VERSION:
                length =
                    spinel_datatype_pack(mReply, sizeof(mReply), "Ciiii", header, SPINEL_CMD_PROP_VALUE_IS, key,
                                         SPINEL_PROTOCOL_VERSION_THREAD_MAJOR, SPINEL_PROTOCOL_VERSION_THREAD_MINOR);
                break;
            case SPINEL_PROP_NCP_VERSION:
                length = spinel_datatype_pack(mReply, sizeof(mReply), "CiiU", header, SPINEL_CMD_PROP_VALUE_IS, key,
                                              "test-rcp");
                break;
            case SPINEL_PROP_CAPS:
                length = spinel_datatype_pack(mReply, sizeof(mReply), "Ciii", header, SPINEL_CMD_PROP_VALUE_IS, key,
                                              SPINEL_CAP_CONFIG_RADIO);
                break;
            default:
                return OT_ERROR_NOT_IMPLEMENTED;
            }
        }
        VerifyOrQuit(length > 0 && static_cast<size_t>(length) <= sizeof(mReply));
        mLength = static_cast<uint16_t>(length);
        mRequests++;
        return OT_ERROR_NONE;
    }

    otError WaitForFrame(uint64_t) override
    {
        VerifyOrQuit(mLength > 0);
        memcpy(mBuffer->GetFrame(), mReply, mLength);
        SuccessOrQuit(mBuffer->SetLength(mLength));
        mLength = 0;
        mCallback(mContext);
        return OT_ERROR_NONE;
    }

    unsigned int GetRequests(void) const { return mRequests; }

private:
    ReceiveFrameCallback mCallback = nullptr;
    void                *mContext  = nullptr;
    RxFrameBuffer       *mBuffer   = nullptr;
    uint8_t              mReply[64];
    uint16_t             mLength   = 0;
    unsigned int         mRequests = 0;
};

int main(void)
{
    ot::Spinel::SpinelDriver driver;
    ot::Spinel::Decoder      decoder;
    const uint8_t            frame[] = {'a', 0, 'b'};
    const uint8_t            empty[] = {0};
    const char              *value   = nullptr;

    VerifyOrQuit(driver.GetIid() == SPINEL_HEADER_INVALID_IID);

    TestInterface transport;
#if defined(OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE) && OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
    const spinel_iid_t iids[] = {0, 1, 2, 3};
    static_assert(ot::Spinel::kSpinelHeaderMaxNumIid == 4, "Multi-PAN requires four interfaces");
#else
    const spinel_iid_t iids[] = {0};
    static_assert(ot::Spinel::kSpinelHeaderMaxNumIid == 1, "Single-PAN requires one interface");
#endif
    VerifyOrQuit(driver.Init(transport, true, iids, sizeof(iids) / sizeof(iids[0])) == OT_COPROCESSOR_RCP);
    VerifyOrQuit(driver.GetIid() == iids[0]);
    VerifyOrQuit(driver.GetSpinelInterface() == &transport);
    VerifyOrQuit(driver.CoprocessorHasCap(SPINEL_CAP_CONFIG_RADIO));
    VerifyOrQuit(transport.GetRequests() == 4);
    driver.Deinit();
    transport.Deinit();
    VerifyOrQuit(driver.GetIid() == SPINEL_HEADER_INVALID_IID);
    VerifyOrQuit(driver.GetSpinelInterface() == nullptr);

    decoder.Init(frame, sizeof(frame));
    SuccessOrQuit(decoder.ReadUtf8(value));
    VerifyOrQuit(strcmp(value, "a") == 0);
    VerifyOrQuit(decoder.GetReadLength() == 2);
    VerifyOrQuit(decoder.ReadUtf8(value) == OT_ERROR_PARSE);
    VerifyOrQuit(decoder.GetReadLength() == 2);

    decoder.Init(empty, sizeof(empty));
    SuccessOrQuit(decoder.ReadUtf8(value));
    VerifyOrQuit(*value == 0);
    VerifyOrQuit(decoder.GetReadLength() == sizeof(empty));
    VerifyOrQuit(decoder.ReadUtf8(value) == OT_ERROR_PARSE);

    uint8_t             buffer[128];
    uint8_t             encoded[32];
    ot::Spinel::Buffer  spinelBuffer(buffer, sizeof(buffer));
    ot::Spinel::Encoder encoder(spinelBuffer);
    unsigned int        packed;
    uint16_t            length;

    SuccessOrQuit(encoder.BeginFrame(ot::Spinel::Buffer::kPriorityLow));
    SuccessOrQuit(encoder.WriteUintPacked(128));
    SuccessOrQuit(encoder.WriteUtf8("Spinel"));
    SuccessOrQuit(encoder.EndFrame());
    SuccessOrQuit(spinelBuffer.OutFrameBegin());
    length = spinelBuffer.OutFrameGetLength();
    VerifyOrQuit(length <= sizeof(encoded));
    VerifyOrQuit(spinelBuffer.OutFrameRead(length, encoded) == length);
    SuccessOrQuit(spinelBuffer.OutFrameRemove());

    // Compare against the expected wire encoding, not only our own decoder.
    const uint8_t expected[] = {0x80, 0x01, 'S', 'p', 'i', 'n', 'e', 'l', 0};
    VerifyOrQuit(length == sizeof(expected));
    VerifyOrQuit(memcmp(encoded, expected, sizeof(expected)) == 0);

    decoder.Init(encoded, length);
    SuccessOrQuit(decoder.ReadUintPacked(packed));
    VerifyOrQuit(packed == 128);
    SuccessOrQuit(decoder.ReadUtf8(value));
    VerifyOrQuit(strcmp(value, "Spinel") == 0);
    VerifyOrQuit(decoder.GetReadLength() == length);

    printf("All standalone Spinel tests passed.\n");
    return 0;
}
