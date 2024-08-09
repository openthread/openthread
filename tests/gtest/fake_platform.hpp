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

#ifndef OT_GTEST_FAKE_PLATFORM_HPP_
#define OT_GTEST_FAKE_PLATFORM_HPP_

#include <openthread/config.h>

#include <inttypes.h>

#include <openthread/instance.h>
#include <openthread/platform/radio.h>

namespace ot {

class FakePlatform
{
public:
    FakePlatform();
    virtual ~FakePlatform();

    static FakePlatform &CurrentPlatform();

    /**
     * Run until something happened or timeout.
     *
     * @param aTimeout The timeout in us.
     *
     * @returns the remaining timeout.
     *
     */
    uint64_t Run(uint64_t aTimeout = 0);

    /**
     * Run until timeout.
     *
     * @param aTimeout The timeout in us.
     *
     */
    void Go(uint64_t aTimeout = 0);

    virtual otError Transmit(otRadioFrame *aFrame) { return OT_ERROR_NONE; }

    virtual uint64_t GetNow() { return mNow; }

    virtual void StartMilliAlarm(uint32_t aT0, uint32_t aDt);
    virtual void StopMilliAlarm();
    virtual void StartMicroAlarm(uint32_t aT0, uint32_t aDt);
    virtual void StopMicroAlarm();

    otRadioFrame mTransmitFrame;
    uint8_t      mTransmitBuffer[OT_RADIO_FRAME_MAX_SIZE];

    otInstance *mInstance = nullptr;

    uint64_t mNow =
        1; ///< Starts with 1 to allow 0 as a special value indicating no alarm is set for mMicroStart and mMilliStart.
    uint64_t mMicroAlarmStart = 0;
    uint64_t mMilliAlarmStart = 0;
};

} // namespace ot

#endif // OT_GTEST_FAKE_PLATFORM_HPP_
