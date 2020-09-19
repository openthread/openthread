/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#include <openthread/platform/flash.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "utils/flash.hpp"

#include "test_platform.h"
#include "test_util.h"
#include "test_util.hpp"

#include "test_flash/flash_v1.hpp"

namespace ot {

#if OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
class FlashTest
{
public:
    FlashTest(Instance *aInstance)
        : mFlashV2(*aInstance)
    {
    }

    void SetReaderWriter(uint8_t aReader, uint8_t aWriter, bool aAlwaysReinit)
    {
        mReader       = aReader;
        mWriter       = aWriter;
        mAlwaysReinit = aAlwaysReinit;
    }

    void Init(void)
    {
        Switch(0, true);
        Switch(1, true);
    }

    otError Get(uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
    {
        Switch(mReader);
        return (mReader == 0) ? mFlashV1.Get(aKey, aIndex, aValue, aValueLength)
                              : mFlashV2.Get(aKey, aIndex, aValue, aValueLength);
    }

    otError Set(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
    {
        Switch(mWriter);
        return (mWriter == 0) ? mFlashV1.Set(aKey, aValue, aValueLength) : mFlashV2.Set(aKey, aValue, aValueLength);
    }

    otError Add(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
    {
        Switch(mWriter);
        return (mWriter == 0) ? mFlashV1.Add(aKey, aValue, aValueLength) : mFlashV2.Add(aKey, aValue, aValueLength);
    }

    otError Delete(uint16_t aKey, int aIndex)
    {
        Switch(mWriter);
        return (mWriter == 0) ? mFlashV1.Delete(aKey, aIndex) : mFlashV2.Delete(aKey, aIndex);
    }

    void Wipe(void)
    {
        Switch(mWriter);
        if (mWriter == 0)
        {
            mFlashV1.Wipe();
        }
        else
        {
            mFlashV2.Wipe();
        }
    }

    uint16_t GetEraseCounter(void)
    {
        Switch(mWriter);
        return (mWriter == 0) ? 0 : mFlashV2.GetEraseCounter();
    }

private:
    void Switch(uint8_t aArea, bool aReinit)
    {
        testFlashSet(aArea);
        if (aReinit)
        {
            if (aArea == 0)
            {
                mFlashV1.Init();
            }
            else
            {
                mFlashV2.Init();
            }
        }
    }

    void Switch(uint8_t aArea)
    {
        bool reinit = mAlwaysReinit;

        if (mWriter == 0 && mReader == 1 && aArea == 1)
        {
            // always reinit if we need to read with new format and the write operations have the old format
            testFlashCopy();
            reinit = true;
        }

        Switch(aArea, reinit);
    }

    FlashV1 mFlashV1;
    Flash   mFlashV2;

    uint8_t mReader;
    uint8_t mWriter;
    bool    mAlwaysReinit;
};

void TestFlash(FlashTest &flash)
{
    uint8_t readBuffer[256];
    uint8_t writeBuffer[256];

    for (uint32_t i = 0; i < sizeof(readBuffer); i++)
    {
        readBuffer[i]  = i & 0xff;
        writeBuffer[i] = 0x55;
    }

    flash.Init();

    // No records in settings

    VerifyOrQuit(flash.Delete(0, 0) == OT_ERROR_NOT_FOUND, "Delete() failed");
    VerifyOrQuit(flash.Get(0, 0, nullptr, nullptr) == OT_ERROR_NOT_FOUND, "Get() failed");

    // Multiple records with different keys

    for (uint16_t key = 0; key < 16; key++)
    {
        uint16_t length = key;

        SuccessOrQuit(flash.Add(key, writeBuffer, length), "Add() failed");
    }

    for (uint16_t key = 0; key < 16; key++)
    {
        uint16_t length = key;

        SuccessOrQuit(flash.Get(key, 0, readBuffer, &length), "Get() failed");
        VerifyOrQuit(length == key, "Get() did not return expected length");
        VerifyOrQuit(memcmp(readBuffer, writeBuffer, length) == 0, "Get() did not return expected value");
    }

    for (uint16_t key = 0; key < 16; key++)
    {
        SuccessOrQuit(flash.Delete(key, 0), "Delete() failed");
    }

    for (uint16_t key = 0; key < 16; key++)
    {
        VerifyOrQuit(flash.Delete(key, 0) == OT_ERROR_NOT_FOUND, "Delete() failed");
        VerifyOrQuit(flash.Get(key, 0, nullptr, nullptr) == OT_ERROR_NOT_FOUND, "Get() failed");
    }

    // Multiple records with the same key

    for (uint16_t index = 0; index < 16; index++)
    {
        uint16_t length = index;

        SuccessOrQuit(flash.Add(0, writeBuffer, length), "Add() failed");
    }

    for (uint16_t index = 0; index < 16; index++)
    {
        uint16_t length = index;

        SuccessOrQuit(flash.Get(0, index, readBuffer, &length), "Get() failed");
        VerifyOrQuit(length == index, "Get() did not return expected length");
        VerifyOrQuit(memcmp(readBuffer, writeBuffer, length) == 0, "Get() did not return expected value");
    }

    for (uint16_t index = 0; index < 16; index++)
    {
        SuccessOrQuit(flash.Delete(0, 0), "Delete() failed");
    }

    VerifyOrQuit(flash.Delete(0, 0) == OT_ERROR_NOT_FOUND, "Delete() failed");
    VerifyOrQuit(flash.Get(0, 0, nullptr, nullptr) == OT_ERROR_NOT_FOUND, "Get() failed");

    // Multiple records with the same key

    for (uint16_t index = 0; index < 16; index++)
    {
        uint16_t length = index;

        if ((index % 4) == 0)
        {
            SuccessOrQuit(flash.Set(0, writeBuffer, length), "Set() failed");
        }
        else
        {
            SuccessOrQuit(flash.Add(0, writeBuffer, length), "Add() failed");
        }
    }

    for (uint16_t index = 0; index < 4; index++)
    {
        uint16_t length = index + 12;

        SuccessOrQuit(flash.Get(0, index, readBuffer, &length), "Get() failed");
        VerifyOrQuit(length == (index + 12), "Get() did not return expected length");
        VerifyOrQuit(memcmp(readBuffer, writeBuffer, length) == 0, "Get() did not return expected value");
    }

    for (uint16_t index = 0; index < 4; index++)
    {
        SuccessOrQuit(flash.Delete(0, 0), "Delete() failed");
    }

    VerifyOrQuit(flash.Delete(0, 0) == OT_ERROR_NOT_FOUND, "Delete() failed");
    VerifyOrQuit(flash.Get(0, 0, nullptr, nullptr) == OT_ERROR_NOT_FOUND, "Get() failed");

    // Wipe()

    for (uint16_t key = 0; key < 16; key++)
    {
        uint16_t length = key;

        SuccessOrQuit(flash.Add(key, writeBuffer, length), "Add() failed");
    }

    flash.Wipe();

    for (uint16_t key = 0; key < 16; key++)
    {
        VerifyOrQuit(flash.Delete(key, 0) == OT_ERROR_NOT_FOUND, "Delete() failed");
        VerifyOrQuit(flash.Get(key, 0, nullptr, nullptr) == OT_ERROR_NOT_FOUND, "Get() failed");
    }

    // Test swap

    for (uint16_t index = 0; index < 4096; index++)
    {
        uint16_t key    = index & 0xf;
        uint16_t length = index & 0xf;

        SuccessOrQuit(flash.Set(key, writeBuffer, length), "Set() failed");
    }

    for (uint16_t key = 0; key < 16; key++)
    {
        uint16_t length = key;

        SuccessOrQuit(flash.Get(key, 0, readBuffer, &length), "Get() failed");
        VerifyOrQuit(length == key, "Get() did not return expected length");
        VerifyOrQuit(memcmp(readBuffer, writeBuffer, length) == 0, "Get() did not return expected value");
    }
}

void TestFlashEraseCounter(FlashTest &flash, uint32_t aSwapSize)
{
    static constexpr uint8_t  flashWordSize    = 8;
    static constexpr uint8_t  swapHeaderSize   = 8;
    static constexpr uint8_t  recordHeaderSize = 8;
    static constexpr uint8_t  testDataSize     = 17;
    static constexpr uint32_t recordSize =
        (testDataSize + recordHeaderSize + (flashWordSize - 1)) & ~(flashWordSize - 1);

    uint8_t  writeBuffer[256];
    uint32_t recordsPerSwap = (aSwapSize - swapHeaderSize) / recordSize;
    uint32_t counter        = 1;
    uint32_t recordsInSwap  = 0;

    memset(writeBuffer, 0, sizeof(writeBuffer));

    flash.Init();

    VerifyOrQuit(flash.GetEraseCounter() == counter, "GetEraseCounter() did not return expected value");

    for (uint32_t j = 0; j < 100; j++)
    {
        // force swap. swap[1] will be the valid swap area.
        for (uint32_t i = recordsInSwap; i < recordsPerSwap + 1; i++)
        {
            SuccessOrQuit(flash.Set(0, writeBuffer, testDataSize), "Set() failed");
        }
        recordsInSwap = 2; // now the active swap contains 2 records, one invalidated and the other valid

        VerifyOrQuit(flash.GetEraseCounter() == counter, "GetEraseCounter() did not return expected value");

        // force swap. swap[0] will be the valid swap area, and the erase counter should increment.
        for (uint32_t i = recordsInSwap; i < recordsPerSwap + 1; i++)
        {
            SuccessOrQuit(flash.Set(0, writeBuffer, testDataSize), "Set() failed");
        }
        recordsInSwap = 2; // now the active swap contains 2 records, one invalidated and the other valid

        if (counter < 0xffff)
        {
            counter++;
        }

        VerifyOrQuit(flash.GetEraseCounter() == counter, "GetEraseCounter() did not return expected value");
    }
}

void TestFlash(void)
{
    Instance *instance = testInitInstance();

    FlashTest flashTest(instance);

#if OPENTHREAD_CONFIG_FLASH_LEGACY_COMPATIBILITY
    // old read vs old write
    printf("Testing old driver #1\n");
    testFlashReset();
    flashTest.SetReaderWriter(0, 0, false);
    TestFlash(flashTest);

    // old read vs old write - reinit before each operation
    printf("Testing old driver #2\n");
    testFlashReset();
    flashTest.SetReaderWriter(0, 0, true);
    TestFlash(flashTest);
#endif

    // new read vs new write
    printf("Testing new driver #1\n");
    testFlashReset();
    flashTest.SetReaderWriter(1, 1, false);
    TestFlash(flashTest);

    // new read vs new write - reinit before each operation
    printf("Testing new driver #2\n");
    testFlashReset();
    flashTest.SetReaderWriter(1, 1, true);
    TestFlash(flashTest);

    printf("Testing new driver #3\n");
    testFlashReset();
    flashTest.SetReaderWriter(1, 1, false);
    TestFlashEraseCounter(flashTest, otPlatFlashGetSwapSize(instance));

    printf("Testing new driver #4\n");
    testFlashReset();
    flashTest.SetReaderWriter(1, 1, true);
    TestFlashEraseCounter(flashTest, otPlatFlashGetSwapSize(instance));

#if OPENTHREAD_CONFIG_FLASH_LEGACY_COMPATIBILITY
    // new read vs old write
    printf("Testing old+new driver #1\n");
    testFlashReset();
    flashTest.SetReaderWriter(1, 0, false);
    TestFlash(flashTest);

    // new read vs old write - reinit before each operation
    printf("Testing old+new driver #2\n");
    testFlashReset();
    flashTest.SetReaderWriter(1, 0, true);
    TestFlash(flashTest);
#endif
}

#endif // OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE

} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
    ot::TestFlash();
#endif
    printf("All tests passed\n");
    return 0;
}
