/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#include "test_platform.h"

#include <string.h>

#include <openthread/config.h>

#include "test_util.hpp"
#include "common/code_utils.hpp"
#include "common/owned_ptr.hpp"
#include "common/retain_ptr.hpp"

namespace ot {

class TestObject : public RetainCountable
{
public:
    TestObject(void)
        : mWasFreed(false)
    {
    }

    void     Free(void) { mWasFreed = true; }
    void     ResetTestFlags(void) { mWasFreed = false; }
    uint16_t GetRetainCount(void) const { return RetainCountable::GetRetainCount(); }
    bool     WasFreed(void) const { return mWasFreed && (GetRetainCount() == 0); }

private:
    bool mWasFreed;
};

static constexpr uint16_t kSkipRetainCountCheck = 0xffff;

template <typename PointerType>
void VerifyPointer(const PointerType &aPointer,
                   const TestObject  *aObject,
                   uint16_t           aRetainCount = kSkipRetainCountCheck)
{
    if (aObject == nullptr)
    {
        VerifyOrQuit(aPointer.IsNull());
        VerifyOrQuit(aPointer.Get() == nullptr);
        VerifyOrQuit(aPointer == nullptr);
    }
    else
    {
        VerifyOrQuit(!aPointer.IsNull());
        VerifyOrQuit(aPointer.Get() == aObject);
        VerifyOrQuit(aPointer == aObject);
        VerifyOrQuit(aPointer != nullptr);

        VerifyOrQuit(!aPointer->WasFreed());
        VerifyOrQuit(!(*aPointer).WasFreed());

        if (aRetainCount != kSkipRetainCountCheck)
        {
            VerifyOrQuit(aObject->GetRetainCount() == aRetainCount);
        }
    }

    VerifyOrQuit(aPointer == aPointer);
}

void TestOwnedPtr(void)
{
    TestObject obj1;
    TestObject obj2;
    TestObject obj3;

    printf("\n====================================================================================\n");
    printf("Testing `OwnedPtr`\n");

    printf("\n - Default constructor (null pointer)");

    {
        OwnedPtr<TestObject> ptr;

        VerifyPointer(ptr, nullptr);
    }

    printf("\n - Constructor taking ownership of an object");
    obj1.ResetTestFlags();

    {
        OwnedPtr<TestObject> ptr(&obj1);

        VerifyPointer(ptr, &obj1);
    }

    VerifyOrQuit(obj1.WasFreed());

    printf("\n - Move constructor taking over from another");
    obj1.ResetTestFlags();

    {
        OwnedPtr<TestObject> ptr1(&obj1);
        OwnedPtr<TestObject> ptr2(ptr1.PassOwnership());

        VerifyPointer(ptr1, nullptr);
        VerifyPointer(ptr2, &obj1);
    }

    VerifyOrQuit(obj1.WasFreed());

    printf("\n - `Free()` method");
    obj1.ResetTestFlags();

    {
        OwnedPtr<TestObject> ptr(&obj1);

        VerifyPointer(ptr, &obj1);

        ptr.Free();

        VerifyOrQuit(obj1.WasFreed());
        VerifyPointer(ptr, nullptr);

        ptr.Free();

        VerifyOrQuit(obj1.WasFreed());
        VerifyPointer(ptr, nullptr);
    }

    printf("\n - `Reset()` method");
    obj1.ResetTestFlags();
    obj2.ResetTestFlags();
    obj3.ResetTestFlags();

    {
        OwnedPtr<TestObject> ptr(&obj1);

        VerifyPointer(ptr, &obj1);

        ptr.Reset(&obj2);

        VerifyOrQuit(obj1.WasFreed());
        VerifyOrQuit(!obj2.WasFreed());
        VerifyPointer(ptr, &obj2);

        ptr.Reset();
        VerifyOrQuit(obj2.WasFreed());
        VerifyPointer(ptr, nullptr);

        ptr.Reset(&obj3);
        VerifyPointer(ptr, &obj3);
    }

    VerifyOrQuit(obj1.WasFreed());
    VerifyOrQuit(obj2.WasFreed());
    VerifyOrQuit(obj3.WasFreed());

    printf("\n - Self `Reset()`");
    obj1.ResetTestFlags();

    {
        OwnedPtr<TestObject> ptr1(&obj1);
        OwnedPtr<TestObject> ptr2;

        VerifyPointer(ptr1, &obj1);

        ptr1.Reset(&obj1);
        VerifyPointer(ptr1, &obj1);

        ptr2.Reset(nullptr);
        VerifyPointer(ptr2, nullptr);
    }

    VerifyOrQuit(obj1.WasFreed());

    printf("\n - Move assignment (operator `=`)");
    obj1.ResetTestFlags();
    obj2.ResetTestFlags();
    obj3.ResetTestFlags();

    {
        OwnedPtr<TestObject> ptr1(&obj1);
        OwnedPtr<TestObject> ptr2(&obj2);
        OwnedPtr<TestObject> ptr3(&obj3);

        VerifyPointer(ptr1, &obj1);
        VerifyPointer(ptr2, &obj2);
        VerifyPointer(ptr3, &obj3);

        // Move from non-null (ptr1) to non-null (ptr2)
        ptr2 = ptr1.PassOwnership();
        VerifyPointer(ptr1, nullptr);
        VerifyPointer(ptr2, &obj1);
        VerifyOrQuit(!obj1.WasFreed());
        VerifyOrQuit(obj2.WasFreed());

        // Move from null (ptr1) to non-null (ptr3)
        ptr3 = ptr1.PassOwnership();
        VerifyPointer(ptr1, nullptr);
        VerifyPointer(ptr3, nullptr);
        VerifyOrQuit(obj3.WasFreed());

        // Move from non-null (ptr2) to null (ptr1)
        ptr1 = ptr2.PassOwnership();
        VerifyPointer(ptr1, &obj1);
        VerifyPointer(ptr2, nullptr);
        VerifyOrQuit(!obj1.WasFreed());

        // Move from null (ptr2) to null (ptr3)
        ptr3 = ptr2.PassOwnership();
        VerifyPointer(ptr2, nullptr);
        VerifyPointer(ptr3, nullptr);
        VerifyOrQuit(!obj1.WasFreed());
    }

    VerifyOrQuit(obj1.WasFreed());

    printf("\n - Self move assignment (operator `=`)");
    obj1.ResetTestFlags();

    {
        OwnedPtr<TestObject> ptr1(&obj1);
        OwnedPtr<TestObject> ptr2;

        VerifyPointer(ptr1, &obj1);
        VerifyPointer(ptr2, nullptr);

        // Move from non-null (ptr1) to itself
        ptr1 = ptr1.PassOwnership();
        VerifyPointer(ptr1, &obj1);

        // Move from null (ptr2) to itself
        ptr2 = ptr2.PassOwnership();
        VerifyPointer(ptr2, nullptr);
    }

    VerifyOrQuit(obj1.WasFreed());

    printf("\n - `Release()` method");
    obj1.ResetTestFlags();

    {
        OwnedPtr<TestObject> ptr(&obj1);

        VerifyPointer(ptr, &obj1);

        VerifyOrQuit(ptr.Release() == &obj1);
        VerifyOrQuit(!obj1.WasFreed());
        VerifyPointer(ptr, nullptr);

        VerifyOrQuit(ptr.Release() == nullptr);
        VerifyOrQuit(!obj1.WasFreed());
        VerifyPointer(ptr, nullptr);
    }

    printf("\n\n-- PASS\n");
}

void TestRetainPtr(void)
{
    TestObject obj1;
    TestObject obj2;
    TestObject obj3;

    printf("\n====================================================================================\n");
    printf("Testing `RetainPtr`\n");

    VerifyOrQuit(obj1.GetRetainCount() == 0);
    VerifyOrQuit(obj2.GetRetainCount() == 0);
    VerifyOrQuit(obj3.GetRetainCount() == 0);

    printf("\n - Default constructor (null pointer)");

    {
        RetainPtr<TestObject> ptr;

        VerifyPointer(ptr, nullptr);
    }

    printf("\n - Constructor taking over management of an object");
    obj1.ResetTestFlags();

    {
        RetainPtr<TestObject> ptr(&obj1);

        VerifyPointer(ptr, &obj1, 1);
    }

    VerifyOrQuit(obj1.WasFreed());

    printf("\n - Two constructed `RetainPtr`s of the same object");
    obj1.ResetTestFlags();

    {
        RetainPtr<TestObject> ptr1(&obj1);
        RetainPtr<TestObject> ptr2(&obj1);

        VerifyPointer(ptr1, &obj1, 2);
        VerifyPointer(ptr2, &obj1, 2);
    }

    VerifyOrQuit(obj1.WasFreed());

    printf("\n - Copy constructor");
    obj1.ResetTestFlags();

    {
        RetainPtr<TestObject> ptr1(&obj1);
        RetainPtr<TestObject> ptr2(ptr1);

        VerifyPointer(ptr1, &obj1, 2);
        VerifyPointer(ptr2, &obj1, 2);
    }

    VerifyOrQuit(obj1.WasFreed());

    printf("\n - `Reset()` method");
    obj1.ResetTestFlags();
    obj2.ResetTestFlags();
    obj3.ResetTestFlags();

    {
        RetainPtr<TestObject> ptr(&obj1);

        VerifyPointer(ptr, &obj1, 1);

        ptr.Reset(&obj2);
        VerifyOrQuit(obj1.WasFreed());
        VerifyOrQuit(!obj2.WasFreed());
        VerifyPointer(ptr, &obj2, 1);

        ptr.Reset();
        VerifyOrQuit(obj2.WasFreed());
        VerifyPointer(ptr, nullptr);

        ptr.Reset(&obj3);
        VerifyPointer(ptr, &obj3, 1);
    }

    VerifyOrQuit(obj1.WasFreed());
    VerifyOrQuit(obj2.WasFreed());
    VerifyOrQuit(obj3.WasFreed());

    printf("\n - Self `Reset()`");
    obj1.ResetTestFlags();

    {
        RetainPtr<TestObject> ptr1(&obj1);
        RetainPtr<TestObject> ptr2;

        VerifyPointer(ptr1, &obj1, 1);

        ptr1.Reset(&obj1);
        VerifyPointer(ptr1, &obj1, 1);

        ptr2.Reset(nullptr);
        VerifyPointer(ptr2, nullptr);
    }

    VerifyOrQuit(obj1.WasFreed());

    printf("\n - Assignment `=`");
    obj1.ResetTestFlags();
    obj2.ResetTestFlags();

    {
        RetainPtr<TestObject> ptr1(&obj1);
        RetainPtr<TestObject> ptr2(&obj2);
        RetainPtr<TestObject> ptr3;

        VerifyPointer(ptr1, &obj1, 1);
        VerifyPointer(ptr2, &obj2, 1);
        VerifyPointer(ptr3, nullptr);

        VerifyOrQuit(ptr1 != ptr2);
        VerifyOrQuit(ptr1 != ptr3);
        VerifyOrQuit(ptr2 != ptr3);

        // Set from non-null (ptr1) to non-null (ptr2)
        ptr2 = ptr1;
        VerifyPointer(ptr1, &obj1, 2);
        VerifyPointer(ptr2, &obj1, 2);
        VerifyOrQuit(obj2.WasFreed());
        VerifyOrQuit(ptr1 == ptr2);

        // Set from null (ptr3) to non-null (ptr1)
        ptr1 = ptr3;
        VerifyPointer(ptr1, nullptr);
        VerifyPointer(ptr3, nullptr);
        VerifyPointer(ptr2, &obj1, 1);
        VerifyOrQuit(ptr1 == ptr3);

        // Move from null (ptr1) to null (ptr3)
        ptr3 = ptr1;
        VerifyPointer(ptr1, nullptr);
        VerifyPointer(ptr3, nullptr);
        VerifyOrQuit(ptr1 == ptr3);

        // Move from non-null (ptr2) to null (ptr3)
        ptr3 = ptr2;
        VerifyPointer(ptr2, &obj1, 2);
        VerifyPointer(ptr3, &obj1, 2);
        VerifyOrQuit(ptr2 == ptr3);
    }

    VerifyOrQuit(obj1.WasFreed());
    VerifyOrQuit(obj2.WasFreed());

    printf("\n - Self assignment `=`");
    obj1.ResetTestFlags();

    {
        RetainPtr<TestObject> ptr1(&obj1);
        RetainPtr<TestObject> ptr2;

        VerifyPointer(ptr1, &obj1, 1);
        VerifyPointer(ptr2, nullptr);

        // Set from non-null (ptr1) to itself. We use `*(&ptr1) to
        // silence `clang` error/warning not allowing explicit self
        // assignment to same variable.
        ptr1 = *(&ptr1);
        VerifyPointer(ptr1, &obj1, 1);

        // Set from null (ptr2) to itself
        ptr2 = *(&ptr2);
        VerifyPointer(ptr2, nullptr);
    }

    VerifyOrQuit(obj1.WasFreed());

    printf("\n - `Release()` method");
    obj1.ResetTestFlags();

    {
        RetainPtr<TestObject> ptr(&obj1);

        VerifyPointer(ptr, &obj1, 1);

        VerifyOrQuit(ptr.Release() == &obj1);
        VerifyPointer(ptr, nullptr);

        VerifyOrQuit(ptr.Release() == nullptr);
        VerifyPointer(ptr, nullptr);
    }

    VerifyOrQuit(!obj1.WasFreed());
    VerifyOrQuit(obj1.GetRetainCount() == 1);

    printf("\n\n-- PASS\n");
}

} // namespace ot

int main(void)
{
    ot::TestOwnedPtr();
    ot::TestRetainPtr();
    printf("\nAll tests passed.\n");
    return 0;
}
