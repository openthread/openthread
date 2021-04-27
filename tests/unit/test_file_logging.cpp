/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#include <stdio.h>

#include "common/code_utils.hpp"

#include "file_logging.h"

#include "test_util.h"

constexpr uint16_t kRandomTextLen  = 1024;
constexpr uint16_t kTestLogEntries = 100;
constexpr uint16_t kLogMinLen      = 1;
constexpr uint16_t kLogMaxLen      = 600;

char sRandomText[kRandomTextLen];

const char *sLogFile = "file_logging_unit_test.log";

struct LogEntry
{
    uint16_t mPos;
    uint16_t mLen;
} sLogEntries[kTestLogEntries];

void GenerateRandomText(void)
{
    const char characters[] =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()-_+={[}]|<,>./?";

    for (uint16_t i = 0; i < kRandomTextLen; i++)
    {
        int tmp        = rand() % (sizeof(characters) - 1);
        sRandomText[i] = characters[tmp];
    }
    sRandomText[kRandomTextLen] = '\0';

    printf("Generated random text:\n");
    for (uint16_t i = 0; i < kRandomTextLen; i += 64)
    {
        printf("%.*s\n", ((kRandomTextLen - i > 64) ? 64 : (kRandomTextLen - i)), sRandomText + i);
    }
    printf("\n");
}

void WriteRandomLogs(void)
{
    for (uint16_t i = 0; i < kTestLogEntries; i++)
    {
        sLogEntries[i].mLen = OT_MAX(kLogMinLen, rand() % kLogMaxLen);
        sLogEntries[i].mPos = rand() % (kRandomTextLen - sLogEntries[i].mLen);

        writeFileLog(sRandomText + sLogEntries[i].mPos, sLogEntries[i].mLen);
        writeFileLog("\n", 1);
    }
}

void CheckLogs(void)
{
    bool     rval = true;
    ssize_t  logLen;
    char *   line    = nullptr;
    size_t   len     = 0;
    uint16_t lineNum = 0;

    FILE *file = fopen(sLogFile, "r");
    VerifyOrQuit(file != nullptr, "Cannot open file log");

    while ((logLen = getline(&line, &len, file)) != -1)
    {
        VerifyOrQuit(lineNum < kTestLogEntries, "Line number doesn't match!");

        printf("Line[%d]:\n", lineNum);
        printf("Should be:\n%.*s\n", sLogEntries[lineNum].mLen, sRandomText + sLogEntries[lineNum].mPos);
        printf("Actual:\n%s\n", line);

        VerifyOrQuit(logLen - 1 == sLogEntries[lineNum].mLen, "Log length doesn't match!");
        VerifyOrQuit(strncmp(line, sRandomText + sLogEntries[lineNum].mPos, sLogEntries[lineNum].mLen) == 0,
                     "Log content doesn't match!");

        lineNum++;
    }
}

int main(void)
{
    GenerateRandomText();

    VerifyOrQuit(initLogFile(sLogFile), "Fail to init log file.");
    WriteRandomLogs();
    deinitLogFile();

    CheckLogs();

    remove(sLogFile);

    printf("All tests passed\n");
    return 0;
}
