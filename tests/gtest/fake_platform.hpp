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

#include "openthread-core-config.h"

#include <map>
#include <vector>

#include <inttypes.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>

namespace ot {

class FakePlatform
{
public:
    FakePlatform();
    virtual ~FakePlatform();

    static FakePlatform &CurrentPlatform() { return *sPlatform; }
    static otInstance   *CurrentInstance() { return CurrentPlatform().mInstance; }

    /**
     * Run until something happened or timeout.
     *
     * @param aTimeout The timeout in us.
     *
     * @returns the remaining timeout.
     */
    uint64_t Run(uint64_t aTimeoutInUs = 0);

    void GoInUs(uint64_t aTimeoutInUs = 0);

    void GoInMs(uint32_t aTimeoutInMs = 0) { GoInUs(aTimeoutInMs * OT_US_PER_MS); }

    virtual uint64_t GetNow() const { return mNow; }

    virtual void StartMilliAlarm(uint32_t aT0, uint32_t aDt);
    virtual void StopMilliAlarm();
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    virtual void StartMicroAlarm(uint32_t aT0, uint32_t aDt);
    virtual void StopMicroAlarm();
#endif

    uint8_t               GetReceiveChannel(void) const { return mChannel; }
    virtual otRadioFrame *GetTransmitBuffer() { return &mTransmitFrame; }
    virtual otError       Transmit(otRadioFrame *aFrame);
    virtual otError       Receive(uint8_t aChannel)
    {
        mChannel = aChannel;
        return OT_ERROR_NONE;
    }

    virtual otError SettingsGet(uint16_t aKey, uint16_t aIndex, uint8_t *aValue, uint16_t *aValueLength) const;
    virtual otError SettingsSet(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);
    virtual otError SettingsAdd(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);
    virtual otError SettingsDelete(uint16_t aKey, int aIndex);
    virtual void    SettingsWipe();

    virtual void     FlashInit();
    virtual void     FlashErase(uint8_t aSwapIndex);
    virtual void     FlashRead(uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize) const;
    virtual void     FlashWrite(uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize);
    virtual uint32_t FlashGetSwapSize() const { return kFlashSwapSize; }

    virtual uint64_t GetEui64() const { return 0; }

protected:
    void ProcessAlarm(uint64_t &aTimeout);

    static constexpr uint64_t kAlarmStop = 0xffffffffffffffffUL;

    static constexpr uint32_t kFlashSwapSize = 2048;
    static constexpr uint32_t kFlashSwapNum  = 2;

    static FakePlatform *sPlatform;

    otInstance *mInstance = nullptr;

    uint64_t mNow = 0;
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    uint64_t mMicroAlarmStart = kAlarmStop;
#endif
    uint64_t mMilliAlarmStart = kAlarmStop;

    otRadioFrame mTransmitFrame;
    uint8_t      mTransmitBuffer[OT_RADIO_FRAME_MAX_SIZE];
    uint8_t      mChannel = 0;

    uint8_t mFlash[kFlashSwapSize * kFlashSwapNum];

    std::map<uint32_t, std::vector<std::vector<uint8_t>>> mSettings;
};

} // namespace ot

#endif // OT_GTEST_FAKE_PLATFORM_HPP_
