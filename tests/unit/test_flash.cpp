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

    void SetReaderWriter(bool aReadNew, bool aWriteNew)
    {
        mReadNew  = aReadNew;
        mWriteNew = aWriteNew;
    }

    void Init(void)
    {
        testFlashSet(0);
        mFlashV1.Init();

        testFlashSet(1);
        mFlashV2.Init();
    }

    otError Get(uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength) const
    {
        otError error;
        if (mReadNew)
        {
            testFlashSet(1);
            error = mFlashV2.Get(aKey, aIndex, aValue, aValueLength);
        }
        else
        {
            testFlashSet(0);
            error = mFlashV1.Get(aKey, aIndex, aValue, aValueLength);
        }
        return error;
    }

    otError Set(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
    {
        otError error;
        if (mWriteNew)
        {
            testFlashSet(1);
            error = mFlashV2.Set(aKey, aValue, aValueLength);
        }
        else
        {
            testFlashSet(0);
            error = mFlashV1.Set(aKey, aValue, aValueLength);
            LegacyPrepare();
        }
        return error;
    }

    otError Add(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
    {
        otError error;
        if (mWriteNew)
        {
            testFlashSet(1);
            error = mFlashV2.Add(aKey, aValue, aValueLength);
        }
        else
        {
            testFlashSet(0);
            error = mFlashV1.Add(aKey, aValue, aValueLength);
            LegacyPrepare();
        }
        return error;
    }

    otError Delete(uint16_t aKey, int aIndex)
    {
        otError error;
        if (mWriteNew)
        {
            testFlashSet(1);
            error = mFlashV2.Delete(aKey, aIndex);
        }
        else
        {
            testFlashSet(0);
            error = mFlashV1.Delete(aKey, aIndex);
            LegacyPrepare();
        }
        return error;
    }

    void Wipe(void)
    {
        if (mWriteNew)
        {
            testFlashSet(1);
            mFlashV2.Wipe();
        }
        else
        {
            testFlashSet(0);
            mFlashV1.Wipe();
            LegacyPrepare();
        }
    }

private:
    void LegacyPrepare()
    {
        if (!mWriteNew && mReadNew)
        {
            testFlashCopy();
            testFlashSet(1);
            mFlashV2.Init();
        }
    }

    FlashV1 mFlashV1;
    Flash   mFlashV2;

    bool mReadNew;
    bool mWriteNew;
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

void TestFlash(void)
{
    Instance *instance = testInitInstance();

    FlashTest flashTest(instance);

    // old read vs old write
    testFlashReset();
    flashTest.SetReaderWriter(false, false);
    TestFlash(flashTest);

    // new read vs new write
    testFlashReset();
    flashTest.SetReaderWriter(true, true);
    TestFlash(flashTest);

#if 0
    // new read vs old write
    testFlashReset();
    flashTest.SetReaderWriter(true, false);
    TestFlash(flashTest);
    printf("Format v2 Legacy compatibility passed\n");
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
