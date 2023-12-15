/*
 *    Copyright (c) 2023, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file shows how to implement the Radio Spinel vendor hook.
 */

#if OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_ENABLE

#include OPENTRHEAD_SPINEL_CONFIG_VENDOR_HOOK_HEADER
#include "common/log.hpp"
#include "lib/platform/exit_code.h"

namespace ot {
namespace Spinel {

otError RadioSpinel::VendorHandleValueIs(spinel_prop_key_t aPropKey)
{
    otError error = OT_ERROR_NONE;

    switch (aPropKey)
    {
        // TODO: Implement your property get handlers here.
        //
        // Get handler should retrieve the property value and then encode and write the
        // value into the NCP buffer. If the "get" operation itself fails, handler should
        // write a `LAST_STATUS` with the error status into the NCP buffer. `OT_ERROR_NO_BUFS`
        // should be returned if NCP buffer is full and response cannot be written.
    default:
        error = OT_ERROR_NOT_FOUND;
        break;
    }
exit:
    return error;
}

} // namespace Spinel
} // namespace ot

extern ot::Spinel::RadioSpinel &GetRadioSpinel(void);

#endif // OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_ENABLE
