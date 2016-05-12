/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file implements the CLI server on the serial service.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cli/cli.hpp>
#include <cli/cli_serial.hpp>
#include <common/code_utils.hpp>
#include <common/encoding.hpp>
#include <common/tasklet.hpp>
#include <platform/serial.h>

namespace Thread {
namespace Cli {

static const uint8_t sEraseString[] = {'\b', ' ', '\b'};
static const uint8_t CRNL[] = {'\r', '\n'};
static Serial *sServer;

Tasklet Serial::sReceiveTask(&ReceiveTask, NULL);

Serial::Serial(void)
{
    sServer = this;
}

ThreadError Serial::Start(void)
{
    mRxLength = 0;
    otPlatSerialEnable();
    return kThreadError_None;
}

extern "C" void otPlatSerialSignalSendDone(void)
{
}

extern "C" void otPlatSerialSignalReceive(void)
{
    Serial::sReceiveTask.Post();
}

void Serial::ReceiveTask(void *aContext)
{
    sServer->ReceiveTask();
}

void Serial::ReceiveTask(void)
{
    uint16_t bufLength;
    const uint8_t *buf;
    const uint8_t *end;

    buf = otPlatSerialGetReceivedBytes(&bufLength);
    end = buf + bufLength;

    for (; buf < end; buf++)
    {
        switch (*buf)
        {
        case '\r':
        case '\n':
            otPlatSerialSend(CRNL, sizeof(CRNL));

            if (mRxLength > 0)
            {
                mRxBuffer[mRxLength] = '\0';
                ProcessCommand();
            }

            break;

        case 3: // CTRL-C
            exit(1);
            break;

        case '\b':
        case 127:
            otPlatSerialSend(sEraseString, sizeof(sEraseString));

            if (mRxLength > 0)
            {
                mRxBuffer[--mRxLength] = '\0';
            }

            break;

        default:
            otPlatSerialSend(buf, 1);
            mRxBuffer[mRxLength++] = *buf;
            break;
        }
    }

    otPlatSerialHandleReceiveDone();
}

ThreadError Serial::ProcessCommand(void)
{
    ThreadError error = kThreadError_None;

    if (mRxBuffer[mRxLength - 1] == '\n')
    {
        mRxBuffer[--mRxLength] = '\0';
    }

    if (mRxBuffer[mRxLength - 1] == '\r')
    {
        mRxBuffer[--mRxLength] = '\0';
    }

    Interpreter::ProcessLine(mRxBuffer, mRxLength, *this);

    mRxLength = 0;

    return error;
}

ThreadError Serial::Output(const char *aBuf, uint16_t aBufLength)
{
    return otPlatSerialSend(reinterpret_cast<const uint8_t *>(aBuf), aBufLength);
}

}  // namespace Cli
}  // namespace Thread
