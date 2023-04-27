/*
 *  Copyright (c) 2023, The OpenThread Authors.
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

#include <cinttypes>

#include "test_util.hpp"
#include "common/bitflags.hpp"

namespace ot {

namespace {
enum Values : uint8_t
{
    k0 = 0,
    k1 = 1,
    k2 = 2,
    k3 = 3,
};
}

void TestBitFlags(void)
{
    using Flags = BitFlags<Values>;

    {
        Flags a;

        VerifyOrQuit(a.HasNone(Values::k0, Values::k1, Values::k2, Values::k3));
        VerifyOrQuit(a.GetRaw() == 0);
    }

    {
        Flags a;
        a.Set(Values::k0);
        VerifyOrQuit(a.GetRaw() == 0b00000001);
        VerifyOrQuit(a.HasNone(Values::k1, Values::k2, Values::k3));
        VerifyOrQuit(a.HasAll(Values::k0));
        VerifyOrQuit(!a.HasAll(Values::k1));
        VerifyOrQuit(a.HasExactly(Values::k0));
        VerifyOrQuit(a.HasAny(Values::k0));
        VerifyOrQuit(a.HasAny(Values::k0, Values::k1));
    }

    {
        Flags a(Values::k0, Values::k2);
        VerifyOrQuit(a.GetRaw() == 0b00000101);
        VerifyOrQuit(a.HasNone(Values::k1, Values::k3));
        VerifyOrQuit(a.HasAll(Values::k0));
        VerifyOrQuit(!a.HasExactly(Values::k0));
        VerifyOrQuit(!a.HasExactly(Values::k1));
        VerifyOrQuit(!a.HasExactly(Values::k0, Values::k1));
        VerifyOrQuit(a.HasExactly(Values::k0, Values::k2));
        VerifyOrQuit(a.HasAny(Values::k0));
        VerifyOrQuit(a.HasAny(Values::k0, Values::k2));
        VerifyOrQuit(a.HasAny(Values::k0, Values::k2, Values::k3));
    }

    {
        Flags a;
        a.SetRaw(0b00000001);
        VerifyOrQuit(a.HasNone(Values::k1, Values::k2, Values::k3));
        VerifyOrQuit(a.HasAll(Values::k0));
        VerifyOrQuit(!a.HasAll(Values::k1));
        VerifyOrQuit(a.HasExactly(Values::k0));
        VerifyOrQuit(a.HasAny(Values::k0, Values::k1));
    }

    {
        Flags a;
        a.SetRaw(0b00000101);
        VerifyOrQuit(a.HasNone(Values::k1, Values::k3));
        VerifyOrQuit(a.HasAll(Values::k0));
        VerifyOrQuit(!a.HasExactly(Values::k0));
        VerifyOrQuit(!a.HasExactly(Values::k1));
        VerifyOrQuit(!a.HasExactly(Values::k0, Values::k1));
        VerifyOrQuit(a.HasExactly(Values::k0, Values::k2));
        VerifyOrQuit(a.HasAny(Values::k0));
        VerifyOrQuit(a.HasAny(Values::k0, Values::k2));
        VerifyOrQuit(a.HasAny(Values::k0, Values::k2, Values::k3));

        a.Unset(Values::k2);
        VerifyOrQuit(a.HasNone(Values::k1, Values::k2, Values::k3));
        VerifyOrQuit(a.HasAll(Values::k0));
        VerifyOrQuit(!a.HasAll(Values::k1));
        VerifyOrQuit(a.HasExactly(Values::k0));
        VerifyOrQuit(a.HasAny(Values::k0, Values::k1));
    }
}

} // namespace ot

int main(void)
{
    ot::TestBitFlags();
    printf("All tests passed\n");
    return 0;
}
