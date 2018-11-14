/*
 *  Copyright (c) 2017, The OpenThread Authors.
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

#include "platform-cc2650.h"

#include <openthread/error.h>

/**
 * @warning this file only implements stubs for the function calls. There is
 * not enough space on the cc2650 to support NV as an SoC.
 */

otError utilsFlashInit(void)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

uint32_t utilsFlashGetSize(void)
{
    return 0;
}

otError utilsFlashErasePage(uint32_t aAddress)
{
    OT_UNUSED_VARIABLE(aAddress);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError utilsFlashStatusWait(uint32_t aTimeout)
{
    OT_UNUSED_VARIABLE(aTimeout);
    return OT_ERROR_NONE;
}

uint32_t utilsFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aAddress);
    OT_UNUSED_VARIABLE(aData);
    OT_UNUSED_VARIABLE(aSize);
    return 0;
}

uint32_t utilsFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aAddress);
    OT_UNUSED_VARIABLE(aData);
    OT_UNUSED_VARIABLE(aSize);
    return 0;
}
