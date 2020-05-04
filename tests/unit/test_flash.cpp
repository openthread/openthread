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
        mFlashV1.Init();
        mFlashV2.Init();
    }

    otError Get(uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength) const
    {
        otError error;
        if (mReadNew)
        {
            error = mFlashV2.Get(aKey, aIndex, aValue, aValueLength);
        }
        else
        {
            error = mFlashV1.Get(aKey, aIndex, aValue, aValueLength);
        }
        return error;
    }

    otError Set(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
    {
        otError error;
        if (mWriteNew)
        {
            error = mFlashV2.Set(aKey, aValue, aValueLength);
            mFlashV1.Init();
        }
        else
        {
            error = mFlashV1.Set(aKey, aValue, aValueLength);
            mFlashV2.Init();
        }
        return error;
    }

    otError Add(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
    {
        otError error;
        if (mWriteNew)
        {
            error = mFlashV2.Add(aKey, aValue, aValueLength);
            mFlashV1.Init();
        }
        else
        {
            error = mFlashV1.Add(aKey, aValue, aValueLength);
            mFlashV2.Init();
        }
        return error;
    }

    otError Delete(uint16_t aKey, int aIndex)
    {
        otError error;
        if (mWriteNew)
        {
            error = mFlashV2.Delete(aKey, aIndex);
            mFlashV1.Init();
        }
        else
        {
            error = mFlashV1.Delete(aKey, aIndex);
            mFlashV2.Init();
        }
        return error;
    }

    void Wipe(void)
    {
        if (mWriteNew)
        {
            mFlashV2.Wipe();
            mFlashV1.Init();
        }
        else
        {
            mFlashV1.Wipe();
            mFlashV2.Init();
        }
    }

private:
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
        readBuffer[i] = i & 0xff;
    }

    flash.Init();

    // No records in settings

    VerifyOrQuit(flash.Delete(0, 0) == OT_ERROR_NOT_FOUND, "Delete() failed");
    VerifyOrQuit(flash.Get(0, 0, NULL, NULL) == OT_ERROR_NOT_FOUND, "Get() failed");

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
        VerifyOrQuit(flash.Get(key, 0, NULL, NULL) == OT_ERROR_NOT_FOUND, "Get() failed");
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
    VerifyOrQuit(flash.Get(0, 0, NULL, NULL) == OT_ERROR_NOT_FOUND, "Get() failed");

    // Multiple records with the same key

    for (uint16_t index = 0; index < 16; index++)
    {
        uint16_t length = index;

        if ((index % 4) == 0)
        {
            SuccessOrQuit(flash.Set(0, writeBuffer, length), "Add() failed");
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
    VerifyOrQuit(flash.Get(0, 0, NULL, NULL) == OT_ERROR_NOT_FOUND, "Get() failed");

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
        VerifyOrQuit(flash.Get(key, 0, NULL, NULL) == OT_ERROR_NOT_FOUND, "Get() failed");
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

    // Test for the mark-first elision

    {
        uint16_t length;

        flash.Wipe();
        SuccessOrQuit(flash.Add(0, writeBuffer, 1), "Add() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 2), "Add() failed");
        SuccessOrQuit(flash.Set(0, writeBuffer, 3), "Set() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 4), "Add() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 5), "Add() failed");

        SuccessOrQuit(flash.Get(0, 0, readBuffer, &length), "Get() failed");
        SuccessOrQuit(flash.Get(0, 1, readBuffer, &length), "Get() failed");
        SuccessOrQuit(flash.Get(0, 2, readBuffer, &length), "Get() failed");

        SuccessOrQuit(flash.Delete(0, 0), "Delete() failed");

        SuccessOrQuit(flash.Get(0, 0, readBuffer, &length), "Get() failed");
        VerifyOrQuit(length == 4, "Get() did not return expected length");

        SuccessOrQuit(flash.Get(0, 1, readBuffer, &length), "Get() failed");
        VerifyOrQuit(length == 5, "Get() did not return expected length");

        flash.Wipe();
        SuccessOrQuit(flash.Add(0, writeBuffer, 1), "Add() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 2), "Add() failed");
        SuccessOrQuit(flash.Set(0, writeBuffer, 3), "Set() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 4), "Add() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 5), "Add() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 180), "Add() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 248), "Add() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 248), "Add() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 248), "Add() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 248), "Add() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 248), "Add() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 248), "Add() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 248), "Add() failed");
        SuccessOrQuit(flash.Delete(0, 0), "Delete() failed");
        SuccessOrQuit(flash.Add(0, writeBuffer, 6), "Add() failed");

        SuccessOrQuit(flash.Get(0, 0, readBuffer, &length), "Get() failed");
        VerifyOrQuit(length == 4, "Get() did not return expected length");

        SuccessOrQuit(flash.Get(0, 1, readBuffer, &length), "Get() failed");
        VerifyOrQuit(length == 5, "Get() did not return expected length");
    }
}

void TestFlash(void)
{
    Instance *instance = testInitInstance();

    FlashTest flashTest(instance);

    testPlatResetToDefaults();
    g_flashIgnoreMultipleWrites = false;
    flashTest.SetReaderWriter(true, true);
    TestFlash(flashTest);

    testPlatResetToDefaults();
    g_flashIgnoreMultipleWrites = true;
    flashTest.SetReaderWriter(false, false);
    TestFlash(flashTest);

    testPlatResetToDefaults();
    g_flashIgnoreMultipleWrites = true;
    flashTest.SetReaderWriter(true, false);
    TestFlash(flashTest);
}

} // namespace ot

int main(void)
{
    ot::TestFlash();
    printf("All tests passed\n");
    return 0;
}
