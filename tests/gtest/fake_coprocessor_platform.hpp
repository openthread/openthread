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

#ifndef OT_GTEST_FAKE_COPROCESSOR_PLATFORM_HPP_
#define OT_GTEST_FAKE_COPROCESSOR_PLATFORM_HPP_

#include <openthread/config.h>

#include "fake_platform.hpp"

#include "lib/spinel/radio_spinel.hpp"

namespace ot {

/**
 * This spinel interface directly connects to the coprocessor with function calls.
 */
class DirectSpinelInterface : public Spinel::SpinelInterface
{
public:
    virtual otError Init(ReceiveFrameCallback aCallback, void *aCallbackContext, RxFrameBuffer &aFrameBuffer) override
    {
        mDecoderBuffer        = &aFrameBuffer;
        mReceiveFrameCallback = aCallback;
        mReceiveFrameContext  = aCallbackContext;
        return kErrorNone;
    }

    virtual void Deinit(void) override {}

    virtual otError SendFrame(const uint8_t *aFrame, uint16_t aLength) override;

    virtual otError WaitForFrame(uint64_t aTimeoutUs) override;

    virtual void UpdateFdSet(void *aMainloopContext) override {}
    virtual void Process(const void *aMainloopContext) override {}

    virtual uint32_t GetBusSpeed(void) const override { return 0; }

    virtual otError HardwareReset(void) override { return kErrorNone; }

    virtual const otRcpInterfaceMetrics *GetRcpInterfaceMetrics(void) const override { return nullptr; }

    static void OnReceived(void *aContext, otError aError)
    {
        static_cast<DirectSpinelInterface *>(aContext)->OnReceived(aError);
    }

    void OnReceived(otError aError)
    {
        mReceived = true;
        if (aError == kErrorNone)
        {
            mReceiveFrameCallback(mReceiveFrameContext);
        }
    }

    int Receive(const uint8_t *aBuffer, uint16_t aLength);

private:
    ReceiveFrameCallback            mReceiveFrameCallback = nullptr;
    void                           *mReceiveFrameContext  = nullptr;
    SpinelInterface::RxFrameBuffer *mDecoderBuffer        = nullptr;
    bool                            mReceived             = false;
};

class FakeCoprocessorPlatform : public FakePlatform
{
public:
    FakeCoprocessorPlatform();
    virtual ~FakeCoprocessorPlatform() = default;

    Spinel::RadioSpinel   mRadioSpinel;
    Spinel::SpinelDriver  mSpinelDriver;
    DirectSpinelInterface mSpinelInterface;
};

} // namespace ot
#endif // OT_GTEST_FAKE_COPROCESSOR_PLATFORM_HPP_
