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

/**
 * @file
 *   This module provides an implementation for file logging. It is intended to implement FILE log output on openthread
 * simulation platform.
 *
 */

#ifndef FILE_LOGGING_H
#define FILE_LOGGING_H

#include "platform-simulation.h"

#define FILE_NAME_MAX_LEN OPENTHREAD_CONFIG_FILE_LOGGING_BUFFER_SIZE

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the log file with a given file name.
 *
 * The file will be created if it doesn't exist before. If a file with the same name existed before, it will be cleared
 * for writting logs.
 *
 * @param[in]  aFileName  The file name. Its length must be less than or equal to `FILE_NAME_MAX_LEN`.
 *
 * @retval true   The log file has been successfully initialized.
 * @retval false  The log file wasn't successfully intialized.
 *
 */
bool initLogFile(const char *aFileName);

/**
 * De-initialize the log file created by `initLogFile`.
 *
 * It will flush all the log contents in internal buffer to the log file and close the file object. This method MUST be
 * called before the program exits. Otherwise some logs at the end may be missing.
 *
 */
void deinitLogFile(void);

/**
 * Write logs to the log file created.
 *
 * The contents will be first written to a internal buffer. Each time when the buffer is full, the whole buffer would be
 * written to the log file. The method should only be called when the log file is successfully initialized and not
 * closed. Otherwise, the contents will not go to the log file.
 *
 * @param[in]  aLogString  A pointer to the log content.
 * @param[in]  aLen        The length of the content in `aLogString`.
 *
 */
void writeFileLog(const char *aLogString, uint16_t aLen);

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // FILE_LOGGING_H
