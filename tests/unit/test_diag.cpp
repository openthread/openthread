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

#include <openthread/diag.h>
#include <openthread/platform/radio.h>

#include "utils/wrap_string.h"

#include "platform.h"
#include "test_util.h"

extern "C" void otTaskletsSignalPending(otInstance *)
{
}

extern "C" bool otTaskletsArePending(otInstance *)
{
    return false;
}

extern "C" void otPlatUartSendDone(void)
{
}

extern "C" void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
    (void)aBuf;
    (void)aBufLength;
}

extern "C" void otPlatAlarmMilliFired(otInstance *)
{
}

extern "C" void otPlatAlarmMicroFired(otInstance *)
{
}

extern "C" void otPlatRadioTxDone(otInstance *, otRadioFrame *aFrame, otRadioFrame *aAckFrame,  otError aError)
{
    (void)aFrame;
    (void)aAckFrame;
    (void)aError;
}

extern "C" void otPlatRadioReceiveDone(otInstance *, otRadioFrame *aFrame, otError aError)
{
    (void)aFrame;
    (void)aError;
}

extern "C" void otPlatRadioTxStarted(otInstance *, otRadioFrame *aFrame)
{
    (void) aFrame;
}

/**
 * Converts a given string replacing '\n', '\r', '\t', etc. with literal strings "\n", "\r", "\t", etc. to make it
 * printable on console.
 *
 */
const char *MakePrintable(const char *aString)
{
    static char printableString[256];

    char *ptr = &printableString[0];

    *ptr++ = '\"';

    while ((*aString != 0) && (ptr < &printableString[sizeof(printableString) - 4]))
    {
        // We ensure that at least 4 characters are remaining in output,
        // two for encoding the current char (in case it needs a backslash)
        // and two for the ending `"` and null char.

        char c = *aString++;
        bool addBackslash = true;

        switch (c)
        {
        case '\n':
            c = 'n';
            break;

        case '\r':
            c = 'r';
            break;

        case '\t':
            c = 't';
            break;

        case '\"':
            c = '\"';
            break;

        case '\'':
            c = '\'';
            break;

        case '\\':
            break;

        default:
            addBackslash = false;
            break;
        }

        if (addBackslash)
        {
            *ptr++ = '\\';
        }

        *ptr++ = c;
    }

    *ptr++ = '\"';
    *ptr = 0;

    return printableString;
}

void TestDiag(void)
{
    struct TestCommand
    {
        const char *mCommand;
        const char *mExpectedOutput;
    };

    static const TestCommand tests[] =
    {

        {
            "diag\n",
            "diagnostics mode is disabled\r\n",
        },
        {
            "diag send 10 100\n",
            "failed\r\nstatus 0xd\r\n",
        },
        {
            "diag start\n",
            "start diagnostics mode\r\nstatus 0x00\r\n",
        },
        {
            "diag",
            "diagnostics mode is enabled\r\n",
        },
        {
            "",
            "diagnostics mode is enabled\r\n",
        },
        {
            "diag channel 10\n",
            "failed\r\nstatus 0x7\r\n",
        },
        {
            "diag channel 11\n",
            "set channel to 11\r\nstatus 0x00\r\n",
        },
        {
            "diag channel\n",
            "channel: 11\r\n",
        },
        {
            "diag power -10\n",
            "set tx power to -10 dBm\r\nstatus 0x00\r\n",
        },
        {
            "diag power\n",
            "tx power: -10 dBm\r\n",
        },
        {
            "diag stats\n",
            "received packets: 0\r\nsent packets: 0\r\nfirst received packet: rssi=0, lqi=0\r\n",
        },
        {
            "diag send 20 100\n",
            "sending 0x14 packet(s), length 0x64\r\nstatus 0x00\r\n",
        },
        {
            "  diag \t send    \t 20\t100",           // Check parsing of extra space chars between args
            "sending 0x14 packet(s), length 0x64\r\nstatus 0x00\r\n",
        },
        {
            "diag repeat 100 100\n",
            "sending packets of length 0x64 at the delay of 0x64 ms\r\nstatus 0x00\r\n"
        },
        {
            "diag stop\n",
            "received packets: 0\r\nsent packets: 0\r\nfirst received packet: rssi=0, lqi=0\r\n\nstop diagnostics mode\r\nstatus 0x00\r\n",
        },
        {
            "diag",
            "diagnostics mode is disabled\r\n",
        },
        {
            "diag 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32",
            "failed: command string contains too many arguments\r\n",
        },
        {
            NULL,
            NULL,
        }
    };

    // initialize platform layer
    int argc = 2;
    char *argv[8] = {(char *)"test_diag", (char *)"1"};
    PlatformInit(argc, argv);

    // initialize diagnostics module
    otDiagInit(NULL);

    VerifyOrQuit(!otDiagIsEnabled(), "diagnostics mode should be disabled as default\n");

    for (const TestCommand *test = &tests[0]; test->mCommand != NULL; test++)
    {
        const char *output = NULL;

        printf("\nCommand: %s", MakePrintable(test->mCommand));
        output = otDiagProcessCmdLine(test->mCommand);
        printf("\nOutput:  %s\n", MakePrintable(output));

        VerifyOrQuit(strcmp(output, test->mExpectedOutput) == 0, "diagnostics output does not match expected result\n");
    }
}

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    TestDiag();
    printf("\nAll tests passed\n");
    return 0;
}
#endif
