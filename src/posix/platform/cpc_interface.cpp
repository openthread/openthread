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

/**
 * @file
 *   This file includes the implementation for the CPCd interface to radio (RCP).
 */

#include "cpc_interface.hpp"

#include "platform-posix.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include "common/code_utils.hpp"
#include "common/logging.hpp"
#include "lib/spinel/spinel.h"

#if OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_CPC

using ot::Spinel::SpinelInterface;

namespace ot {
namespace Posix {

bool CpcInterface::sCpcResetReq = false;

CpcInterface::CpcInterface(SpinelInterface::ReceiveFrameCallback aCallback,
                             void *                                aCallbackContext,
                             SpinelInterface::RxFrameBuffer &      aFrameBuffer)
    : mReceiveFrameCallback(aCallback)
    , mReceiveFrameContext(aCallbackContext)
    , mReceiveFrameBuffer(aFrameBuffer)
    , mSockFd(-1)
{
    memset(&mInterfaceMetrics, 0, sizeof(mInterfaceMetrics));
    mInterfaceMetrics.mRcpInterfaceType = OT_POSIX_RCP_BUS_CPC;
}

void CpcInterface::OnRcpReset(void)
{
}

otError CpcInterface::Init(const Url::Url &aRadioUrl)
{
    otError error = OT_ERROR_NONE;
    OT_UNUSED_VARIABLE(aRadioUrl);

    VerifyOrExit(mSockFd == -1, error = OT_ERROR_ALREADY);

    if (cpc_init(&mHandle, aRadioUrl.GetPath(), false, HandleSecondaryReset) != 0)
    {
      otLogCritPlat("CPC init failed. Ensure radio-url argument has the form 'spinel+cpc://cpcd_0?iid=<1..3>'");
      DieNow(OT_EXIT_FAILURE);
    }

    mSockFd = cpc_open_endpoint(mHandle, &mEndpoint, mId, 1);

    if (-1 == mSockFd)
    {
      otLogCritPlat("CPC endpoint open failed");
      error = OT_ERROR_FAILED;
    }

exit:
    return error;
}

void CpcInterface::HandleSecondaryReset(void)
{
    SetCpcResetReq(true);
}

CpcInterface::~CpcInterface(void)
{
    Deinit();
}

void CpcInterface::Deinit(void)
{
    VerifyOrExit(mEndpoint.ptr != nullptr);

    VerifyOrExit(0 == cpc_close_endpoint(&mEndpoint), perror("close cpc endpoint"));

exit:
    return;
}

void CpcInterface::Read(uint64_t aTimeoutUs)
{
    uint8_t buffer[kMaxFrameSize];
    uint8_t *ptr = buffer;
    ssize_t bytesRead;
    bool block = false;

    if(aTimeoutUs > 0)
    {
        cpc_timeval_t timeout;

        timeout.seconds = static_cast<int>(aTimeoutUs / US_PER_S);
        timeout.microseconds = static_cast<int>(aTimeoutUs % US_PER_S);

        block = true;
        cpc_set_endpoint_option(mEndpoint, CPC_OPTION_BLOCKING, &block, sizeof(block));
        cpc_set_endpoint_option(mEndpoint, CPC_OPTION_RX_TIMEOUT, &timeout, sizeof(timeval));
    }
    else
    {
        cpc_set_endpoint_option(mEndpoint, CPC_OPTION_BLOCKING, &block, sizeof(block));
    }

    bytesRead = cpc_read_endpoint(mEndpoint, buffer, sizeof(buffer), mReadFlags);

    if (bytesRead > 0)
    {
        while(bytesRead--)
        {
            if(mReceiveFrameBuffer.CanWrite(sizeof(uint8_t)))
            {
                IgnoreError(mReceiveFrameBuffer.WriteByte(*(ptr++)));
            }
        }

        mReceiveFrameCallback(mReceiveFrameContext);

    }
    else if (errno == ECONNRESET)
    {
        SetCpcResetReq(true);
    }
    else if ((errno != EAGAIN) && (errno != EINTR))
    {
        DieNow(OT_EXIT_ERROR_ERRNO);
    }
}

otError CpcInterface::SendFrame(const uint8_t *aFrame, uint16_t aLength)
{
    otError error;

    CheckAndReInitCpc();
    error = Write(aFrame, aLength);
    return error;
}

otError CpcInterface::Write(const uint8_t *aFrame, uint16_t aLength)
{
    otError error = OT_ERROR_NONE;

    // We are catching the SPINEL reset command and returning
    // a SPINEL reset response immediately
    if(SPINEL_HEADER_GET_TID(*aFrame) == 0 &&
        *(aFrame + 1) == SPINEL_CMD_RESET)
    {
        SendResetResponse();
        return error;
    }

    while (aLength)
    {
        ssize_t bytesWritten = cpc_write_endpoint(mEndpoint, aFrame, aLength, mWriteFlags | SL_CPC_FLAG_NON_BLOCK);

        if (bytesWritten == aLength)
        {
            break;
        }
        else if (bytesWritten > 0)
        {
            aLength -= static_cast<uint16_t>(bytesWritten);
            aFrame += static_cast<uint16_t>(bytesWritten);
        }
        else if (bytesWritten < 0)
        {
            VerifyOrExit((errno == EPIPE), SetCpcResetReq(true));
            VerifyOrDie((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR), OT_EXIT_ERROR_ERRNO);
        }

    }

exit:
    return error;
}

otError CpcInterface::WaitForFrame(uint64_t aTimeoutUs)
{
    otError        error = OT_ERROR_NONE;

    CheckAndReInitCpc();
    Read(aTimeoutUs);

    return error;
}

void CpcInterface::UpdateFdSet(fd_set &aReadFdSet, fd_set &aWriteFdSet, int &aMaxFd, struct timeval &aTimeout)
{
    OT_UNUSED_VARIABLE(aWriteFdSet);
    OT_UNUSED_VARIABLE(aTimeout);

    FD_SET(mSockFd, &aReadFdSet);

    if (aMaxFd < mSockFd)
    {
        aMaxFd = mSockFd;
    }
}

void CpcInterface::Process(const RadioProcessContext &aContext)
{
    OT_UNUSED_VARIABLE(aContext);
    CheckAndReInitCpc();
    Read(0);
}

void CpcInterface::CheckAndReInitCpc(void)
{
    int result;
    int attempts = 0;
    
    //Check if CPC needs to be restarted
    VerifyOrExit(sCpcResetReq);
    
    do
    {
        //Add some delay before attempting to restart
        usleep(kMaxSleepDuration);
        //Try to restart CPC
        result = cpc_restart(&mHandle);
        //Mark how many times the restart was attempted
        attempts++;
        //Continue to try and restore CPC communication until we
        //have exhausted the retries or restart was successful
    }   while ((result != 0) && (attempts < kMaxRestartAttempts));

    //If the restart failed, exit.
    VerifyOrDie(result == 0, OT_EXIT_ERROR_ERRNO);

    //Reopen the endpoint for communication
    mSockFd = cpc_open_endpoint(mHandle, &mEndpoint, mId, 1);

    //If the restart failed, exit.
    VerifyOrDie(mSockFd != -1, OT_EXIT_ERROR_ERRNO);

    otLogCritPlat("Restarted CPC successfully");

    //Clear the flag
    SetCpcResetReq(false);

exit:
    return;
}

void CpcInterface::SendResetResponse(void)
{

    // Put CPC Reset call here

    for(int i=0; i<kResetCMDSize; ++i)
    {
        if(mReceiveFrameBuffer.CanWrite(sizeof(uint8_t)))
        {
            IgnoreError(mReceiveFrameBuffer.WriteByte(mResetResponse[i]));
        }
    }

    mReceiveFrameCallback(mReceiveFrameContext);
}

} // namespace Posix
} // namespace ot
#endif // OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_CPC
