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
 * @brief
 *  This file defines the top-level functions for the OpenThread CLI server.
 */

#ifndef CLI_CONSOLE_H_
#define CLI_CONSOLE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function pointer is called to notify about Console output.
 *
 * @param[in]  aBuf        A pointer to a buffer with an output.
 * @param[in]  aBufLength  A length of the output data stored in the buffer.
 * @param[out] aContext    A user context pointer.
 *
 * @returns                Number of bytes processed by the callback.
 *
 */
typedef int (*otCliConsoleOutputCallback)(const char *aBuf,
                                          uint16_t aBufLength, void *aContext);

/**
 * Initialize the CLI CONSOLE module.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[in]  aCallback   A callback method called to process console output.
 * @param[in]  aContext    A user context pointer.
 *
 */
void otCliConsoleInit(otInstance *aInstance, otCliConsoleOutputCallback aCallback, void *aContext);

/**
 * This method is called to feed in a console input line.
 *
 * @param[in]  aBuf        A pointer to a buffer with an input.
 * @param[in]  aBufLength  A length of the input data stored in the buffer.
 *
 */
void otCliConsoleInputLine(char *aBuf, uint16_t aBufLength);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
