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

#include "utf8.hpp"

#include "common/code_utils.hpp"

namespace ot {

bool ValidateUtf8(const uint8_t *aString)
{
    bool    ret = true;
    uint8_t byte;
    uint8_t continuation_bytes = 0;

    while ((byte = *aString++) != 0)
    {
        if ((byte & 0x80) == 0)
        {
            continue;
        }

        // This is a leading byte 1xxx-xxxx.

        if ((byte & 0x40) == 0) // 10xx-xxxx
        {
            // We got a continuation byte pattern without seeing a leading byte earlier.
            ExitNow(ret = false);
        }
        else if ((byte & 0x20) == 0) // 110x-xxxx
        {
            continuation_bytes = 1;
        }
        else if ((byte & 0x10) == 0) // 1110-xxxx
        {
            continuation_bytes = 2;
        }
        else if ((byte & 0x08) == 0) // 1111-0xxx
        {
            continuation_bytes = 3;
        }
        else // 1111-1xxx  (invalid pattern).
        {
            ExitNow(ret = false);
        }

        while (continuation_bytes-- != 0)
        {
            byte = *aString++;

            // Verify the continuation byte pattern 10xx-xxxx
            VerifyOrExit((byte & 0xc0) == 0x80, ret = false);
        }
    }

exit:
    return ret;
}

} // namespace ot
