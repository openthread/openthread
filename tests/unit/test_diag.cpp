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

#include "test_util.h"
#include <string.h>
#include <openthread-diag.h>
#include <platform/platform.h>
#include <platform/radio.h>

extern "C" void otSignalTaskletPending(void)
{
}

extern "C" bool otAreTaskletsPending(void)
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

extern "C" void otPlatAlarmFired()
{
}

extern "C" void otPlatRadioTransmitDone(bool aRxPending, ThreadError aError)
{
    (void)aRxPending;
    (void)aError;
}

extern "C" void otPlatRadioReceiveDone(RadioPacket *aFrame, ThreadError aError)
{
    (void)aFrame;
    (void)aError;
}


/**
 *  diagnostics module tests
 */
void TestDiag()
{
    static const struct
    {
        const char *command;
        const char *output;
    } tests[] =
    {
        {
            "diag\n",
            "diagnostics mode is disabled\r\n",
        },
        {
            "diag send 10 100\n",
            "failed\r\nstatus 0xe\r\n",
        },
        {
            "diag start\n",
            "start diagnostics mode\r\nstatus 0x00\r\n",
        },
        {
            "diag\n",
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
            "diag repeat 100 100\n",
            "sending packets of length 0x64 at the delay of 0x64 ms\r\nstatus 0x00\r\n"
        },
        {
            "diag sleep\n",
            "sleeping now...\r\n",
        },
        {
            "diag stop\n",
            "received packets: 0\r\nsent packets: 0\r\nfirst received packet: rssi=0, lqi=0\r\n\nstop diagnostics mode\r\nstatus 0x00\r\n",
        },
        {
            "diag\n",
            "diagnostics mode is disabled\r\n",
        },
    };

    // initialize platform layer
    int argc = 2;
    char *argv[2] = {(char *)"test_diag", (char *)"1"};
    PlatformInit(argc, argv);

    // initialize diagnostics module
    diagInit();

    // test diagnostics commands
    VerifyOrQuit(!isDiagEnabled(), "diagnostics mode shoud be disabled as default\n");

    for (unsigned int i = 0; i < sizeof(tests) / sizeof(tests[0]);  i++)
    {
        char string[50];
        int length = strlen(tests[i].command);

        char *cmd;
        char *argv[8];
        int argc = 0;
        char *output = NULL;

        memcpy(string, tests[i].command, length + 1);

        for (cmd = string + 1; (cmd < string + length) && (cmd != NULL); ++cmd)
        {
            if (*cmd == ' ' || *cmd == '\r' || *cmd == '\n')
            {
                *cmd = '\0';
            }

            if (*(cmd - 1) == '\0' && *cmd != ' ')
            {
                argv[argc++] = cmd;
            }
        }

        output = diagProcessCmd(argc, argv);
        VerifyOrQuit(memcmp(output, tests[i].output, strlen(tests[i].output)) == 0,
                     "Test Diagnostics module failed\r\n");
    }
}

int main(void)
{
    TestDiag();
    printf("All tests passed\n");
    return 0;
}
