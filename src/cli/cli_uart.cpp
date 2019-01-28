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

/**
 * @file
 *   This file implements the CLI server on the UART service.
 */

#include "cli_uart.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils/wrap_string.h"

#include <openthread/cli.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/uart.h>
#include "cli/cli.hpp"
#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/logging.hpp"
#include "common/new.hpp"
#include "common/tasklet.hpp"

#if OPENTHREAD_CONFIG_ENABLE_DEBUG_UART
#include <openthread/platform/debug_uart.h>
#endif

#ifdef OT_CLI_UART_LOCK_HDR_FILE

#include OT_CLI_UART_LOCK_HDR_FILE

#else

/**
 * Macro to acquire an exclusive lock of uart cli output
 * Default implementation does nothing
 *
 */
#ifndef OT_CLI_UART_OUTPUT_LOCK
#define OT_CLI_UART_OUTPUT_LOCK() \
    do                            \
    {                             \
    } while (0)
#endif

/**
 * Macro to release the exclusive lock of uart cli output
 * Default implementation does nothing
 *
 */
#ifndef OT_CLI_UART_OUTPUT_UNLOCK
#define OT_CLI_UART_OUTPUT_UNLOCK() \
    do                              \
    {                               \
    } while (0)
#endif

#endif // OT_CLI_UART_LOCK_HDR_FILE

namespace ot {
namespace Cli {

static otDEFINE_ALIGNED_VAR(sCliUartRaw, sizeof(Uart), uint64_t);

extern "C" void otCliUartInit(otInstance *aInstance)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    Server::sServer = new (&sCliUartRaw) Uart(instance);
}

Uart::Uart(Instance *aInstance)
    : Server(aInstance)
{
    mRxLength   = 0;
    mTxHead     = 0;
    mTxLength   = 0;
    mSendLength = 0;

    otPlatUartEnable();
}

extern "C" void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
    static_cast<Uart *>(Server::sServer)->ReceiveTask(aBuf, aBufLength);
}

void Uart::ReceiveTask(const uint8_t *aBuf, uint16_t aBufLength)
{
    static const char sCommandPrompt[] = {'>', ' '};

#if OPENTHREAD_CONFIG_UART_CLI_RAW
    if (aBufLength > 0)
    {
        memcpy(mRxBuffer + mRxLength, aBuf, aBufLength);
        mRxLength += aBufLength;
    }

    if (aBuf[aBufLength - 1] == '\r' || aBuf[aBufLength - 1] == '\n')
    {
        mRxBuffer[mRxLength] = '\0';
        ProcessCommand();
        Output(sCommandPrompt, sizeof(sCommandPrompt));
    }
#else // OPENTHREAD_CONFIG_UART_CLI_RAW
    static const char sEraseString[] = {'\b', ' ', '\b'};
    static const char CRNL[]         = {'\r', '\n'};
    const uint8_t *   end;

    end = aBuf + aBufLength;

    for (; aBuf < end; aBuf++)
    {
        switch (*aBuf)
        {
        case '\r':
        case '\n':
            Output(CRNL, sizeof(CRNL));

            if (mRxLength > 0)
            {
                mRxBuffer[mRxLength] = '\0';
                ProcessCommand();
            }

            Output(sCommandPrompt, sizeof(sCommandPrompt));

            break;

#if OPENTHREAD_POSIX

        case 0x04: // ASCII for Ctrl-D
            exit(EXIT_SUCCESS);
            break;
#endif

        case '\b':
        case 127:
            if (mRxLength > 0)
            {
                Output(sEraseString, sizeof(sEraseString));
                mRxBuffer[--mRxLength] = '\0';
            }

            break;

        default:
            if (mRxLength < kRxBufferSize - 1)
            {
                Output(reinterpret_cast<const char *>(aBuf), 1);
                mRxBuffer[mRxLength++] = static_cast<char>(*aBuf);
            }

            break;
        }
    }
#endif // OPENTHREAD_CONFIG_UART_CLI_RAW
}

otError Uart::ProcessCommand(void)
{
    otError error = OT_ERROR_NONE;

    if (mRxBuffer[mRxLength - 1] == '\n')
    {
        mRxBuffer[--mRxLength] = '\0';
    }

    if (mRxBuffer[mRxLength - 1] == '\r')
    {
        mRxBuffer[--mRxLength] = '\0';
    }

#if OPENTHREAD_CONFIG_LOG_OUTPUT != OPENTHREAD_CONFIG_LOG_OUTPUT_NONE
    /*
     * Note this is here for this reason:
     *
     * TEXT (command) input ... in a test automation script occurs
     * rapidly and often without gaps between the command and the
     * terminal CR
     *
     * In contrast as a human is typing there is a delay between the
     * last character of a command and the terminal CR which executes
     * a command.
     *
     * During that human induced delay a tasklet may be scheduled and
     * the LOG becomes confusing and it is hard to determine when
     * something happened.  Which happened first? the command-CR or
     * the tasklet.
     *
     * Yes, while rare it is a race condition that is hard to debug.
     *
     * Thus this is here to affirmatively LOG exactly when the CLI
     * command is being executed.
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    /* TODO: how exactly do we get the instance here? */
#else
    otLogInfoCli("execute command: %s", mRxBuffer);
#endif
#endif
    if (mRxLength > 0)
    {
        mInterpreter.ProcessLine(mRxBuffer, mRxLength, *this);
    }

    mRxLength = 0;

    return error;
}

int Uart::Output(const char *aBuf, uint16_t aBufLength)
{
    OT_CLI_UART_OUTPUT_LOCK();
    uint16_t remaining = kTxBufferSize - mTxLength;
    uint16_t tail;

    if (aBufLength > remaining)
    {
        aBufLength = remaining;
    }

    for (int i = 0; i < aBufLength; i++)
    {
        tail            = (mTxHead + mTxLength) % kTxBufferSize;
        mTxBuffer[tail] = *aBuf++;
        mTxLength++;
    }

    Send();
    OT_CLI_UART_OUTPUT_UNLOCK();

    return aBufLength;
}

void Uart::Send(void)
{
    VerifyOrExit(mSendLength == 0);

    if (mTxLength > kTxBufferSize - mTxHead)
    {
        mSendLength = kTxBufferSize - mTxHead;
    }
    else
    {
        mSendLength = mTxLength;
    }

    if (mSendLength > 0)
    {
#if OPENTHREAD_CONFIG_ENABLE_DEBUG_UART
        /* duplicate the output to the debug uart */
        otPlatDebugUart_write_bytes(reinterpret_cast<uint8_t *>(mTxBuffer + mTxHead), mSendLength);
#endif
        otPlatUartSend(reinterpret_cast<uint8_t *>(mTxBuffer + mTxHead), mSendLength);
    }

exit:
    return;
}

extern "C" void otPlatUartSendDone(void)
{
    static_cast<Uart *>(Server::sServer)->SendDoneTask();
}

void Uart::SendDoneTask(void)
{
    mTxHead = (mTxHead + mSendLength) % kTxBufferSize;
    mTxLength -= mSendLength;
    mSendLength = 0;

    Send();
}

} // namespace Cli
} // namespace ot
