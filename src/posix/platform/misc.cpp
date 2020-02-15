/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <assert.h>
#include <fcntl.h>
#include <setjmp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openthread/platform/misc.h>

#include "common/code_utils.hpp"
#include "common/logging.hpp"

static otPlatResetReason   sPlatResetReason   = OT_PLAT_RESET_REASON_POWER_ON;
static otPlatMcuPowerState gPlatMcuPowerState = OT_PLAT_MCU_POWER_STATE_ON;

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sPlatResetReason;
}

void otPlatWakeHost(void)
{
    // TODO: implement an operation to wake the host from sleep state.
}

otError otPlatSetMcuPowerState(otInstance *aInstance, otPlatMcuPowerState aState)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    switch (aState)
    {
    case OT_PLAT_MCU_POWER_STATE_ON:
    case OT_PLAT_MCU_POWER_STATE_LOW_POWER:
        gPlatMcuPowerState = aState;
        break;

    default:
        error = OT_ERROR_FAILED;
        break;
    }

    return error;
}

otPlatMcuPowerState otPlatGetMcuPowerState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return gPlatMcuPowerState;
}

int SocketWithCloseExec(int aDomain, int aType, int aProtocol)
{
    int rval = 0;
    int fd   = -1;

#ifdef __APPLE__
    VerifyOrExit((fd = socket(aDomain, aType, aProtocol)) != -1, perror("socket(SOCK_CLOEXEC)"));

    VerifyOrExit((rval = fcntl(fd, F_GETFD, 0)) != -1, perror("fcntl(F_GETFD)"));
    VerifyOrExit((rval = fcntl(fd, F_SETFD, rval | FD_CLOEXEC)) != -1, perror("fcntl(F_SETFD)"));
#else
    VerifyOrExit((fd = socket(aDomain, aType | SOCK_CLOEXEC, aProtocol)) != -1, perror("socket(SOCK_CLOEXEC)"));
#endif

exit:
    if (rval == -1 && fd != -1)
    {
        VerifyOrDie(close(fd) == 0, OT_EXIT_ERROR_ERRNO);
        fd = -1;
    }

    return fd;
}

const char *otExitCodeToString(uint8_t aExitCode)
{
    const char *retval = NULL;

    switch (aExitCode)
    {
    case OT_EXIT_SUCCESS:
        retval = "Success";
        break;

    case OT_EXIT_FAILURE:
        retval = "Failure";
        break;

    case OT_EXIT_INVALID_ARGUMENTS:
        retval = "InvalidArgument";
        break;

    case OT_EXIT_RADIO_SPINEL_INCOMPATIBLE:
        retval = "RadioSpinelIncompatible";
        break;

    case OT_EXIT_RADIO_SPINEL_RESET:
        retval = "RadioSpinelReset";
        break;

    case OT_EXIT_ERROR_ERRNO:
        retval = strerror(errno);
        break;

    default:
        assert(false);
        retval = "UnknownExitCode";
        break;
    }

    return retval;
}
