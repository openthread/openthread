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

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <openthread.h>

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include <common/code_utils.hpp>
#include <platform/uart.h>
#include "platform-windows.h"

static HANDLE s_WorkerThread;
static HANDLE s_StopWorkerEvent;

DWORD WINAPI 
windowsUartWorkerThread(
    _In_ LPVOID lpThreadParameter
    )
{
    HANDLE waitHandles[] = { s_StopWorkerEvent, GetStdHandle(STD_INPUT_HANDLE) };
    
    // Cache the original console mode
    DWORD originalConsoleMode;
    GetConsoleMode(waitHandles[1], &originalConsoleMode);

    // Fake the first new line
    uint8_t ch = '\n';
    otPlatUartReceived(&ch, 1);

    // Wait for console events
    while (WaitForMultipleObjects(ARRAYSIZE(waitHandles), waitHandles, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
    {
        DWORD numberOfEvent = 0;
        if (GetNumberOfConsoleInputEvents(waitHandles[1], &numberOfEvent))
        {
            for (; numberOfEvent > 0; numberOfEvent--)
            {
                INPUT_RECORD record;
                DWORD numRead;
                if (!ReadConsoleInput(waitHandles[1], &record, 1, &numRead) ||
                    record.EventType != KEY_EVENT ||
                    !record.Event.KeyEvent.bKeyDown)
                    continue;
                
                uint8_t ch = (uint8_t)record.Event.KeyEvent.uChar.AsciiChar;
                otPlatUartReceived(&ch, 1);
            }
        }
    }

    return NO_ERROR;
}

EXTERN_C ThreadError otPlatUartEnable(void)
{
    ThreadError error = kThreadError_None;

    // Create the worker thread stop event
    VerifyOrExit((s_StopWorkerEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) != NULL, error = kThreadError_Error);

    // Start the worker thread
    VerifyOrExit((s_WorkerThread = CreateThread(NULL, 0, windowsUartWorkerThread, NULL, 0, NULL)) != NULL, error = kThreadError_Error);

    return error;

exit:
    CloseHandle(s_StopWorkerEvent);
    s_StopWorkerEvent = NULL;
    return error;
}

EXTERN_C ThreadError otPlatUartDisable(void)
{
    ThreadError error = kThreadError_None;

    // Set the shutdown event
    SetEvent(s_StopWorkerEvent);

    // Wait for the worker to compelte
    WaitForSingleObject(s_WorkerThread, INFINITE);
    
    CloseHandle(s_WorkerThread);
    s_WorkerThread = NULL;

    CloseHandle(s_StopWorkerEvent);
    s_StopWorkerEvent = NULL;

    return error;
}

EXTERN_C ThreadError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    ThreadError error = kThreadError_None;

    DWORD dwNumCharsWritten = 0;
    VerifyOrExit(WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), aBuf, aBufLength, &dwNumCharsWritten, NULL), error = kThreadError_Error);

    otPlatUartSendDone();

exit:
    return error;
}
