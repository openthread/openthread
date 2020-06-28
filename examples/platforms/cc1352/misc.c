/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#include <openthread/config.h>

#include <driverlib/sys_ctrl.h>
#include <openthread/platform/misc.h>

/*
 * NOTE: if the system is flashed with Flash Programmer 2 or Uniflash, this
 * reset will not work the first time. Both programs use the cJTAG module,
 * which sets the halt in boot flag. The device must be manually reset the
 * first time after being programmed through the JTAG interface.
 */

/**
 * Function documented in platform/misc.h
 */
void otPlatReset(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    SysCtrlSystemReset();
}

/**
 * Function documented in platform/misc.h
 */
otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otPlatResetReason ret;

    switch (SysCtrlResetSourceGet())
    {
    case RSTSRC_PWR_ON:
        ret = OT_PLAT_RESET_REASON_POWER_ON;
        break;

    case RSTSRC_PIN_RESET:
        ret = OT_PLAT_RESET_REASON_EXTERNAL;
        break;

    case RSTSRC_VDDS_LOSS:
    case RSTSRC_VDDR_LOSS:
    case RSTSRC_CLK_LOSS:
        ret = OT_PLAT_RESET_REASON_CRASH;
        break;

    case RSTSRC_WARMRESET:
    case RSTSRC_SYSRESET:
    case RSTSRC_WAKEUP_FROM_SHUTDOWN:
        ret = OT_PLAT_RESET_REASON_SOFTWARE;
        break;

    default:
        ret = OT_PLAT_RESET_REASON_UNKNOWN;
        break;
    }

    return ret;
}

void otPlatWakeHost(void)
{
    // TODO: implement an operation to wake the host from sleep state.
}
