/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file defines the APIs for integrating with select() based event loop.
 */

#ifndef OPENTHREAD_SELECT_H_
#define OPENTHREAD_SELECT_H_

#include <sys/select.h>

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Updates the file descriptor sets with file descriptors used by OpenThread drivers.
 *
 * @param[in]       aInstance   The OpenThread instance structure.
 * @param[in,out]   aMaxFd      A pointer to the max file descriptor.
 * @param[in,out]   aReadFdSet  A pointer to the read file descriptors, which may already contain some FDs.
 * @param[in,out]   aWriteFdSet A pointer to the write file descriptors, which may already contain some FDs.
 * @param[in,out]   aErrorFdSet A pointer to the error file descriptors, which may already contain some FDs.
 * @param[in,out]   aTimeout    A pointer to an initialized timeout,  The caller must initialize this to the maximum
 *                              desired timeout before calling this function; the function may reduce the value, but
 *                              will not increase it. The output should be no larger than the input.
 */
void otSysUpdateEvents(otInstance     *aInstance,
                       int            *aMaxFd,
                       fd_set         *aReadFdSet,
                       fd_set         *aWriteFdSet,
                       fd_set         *aErrorFdSet,
                       struct timeval *aTimeout);

/**
 * Performs all platform-specific processing for OpenThread's example applications.
 *
 * @note This function is not called by the OpenThread library. Instead, the system/RTOS should call this function
 *       in the main loop when processing OpenThread's drivers is most appropriate.
 * @note This should only be called when the fd_set are meaningful, that is, the select() call was successful.
 *
 * @param[in]       aInstance   The OpenThread instance structure.
 * @param[in]       aReadFdSet  A pointer to the read file descriptors.
 * @param[in]       aWriteFdSet A pointer to the write file descriptors.
 * @param[in]       aErrorFdSet A pointer to the error file descriptors.
 */
void otSysProcessEvents(otInstance   *aInstance,
                        const fd_set *aReadFdSet,
                        const fd_set *aWriteFdSet,
                        const fd_set *aErrorFdSet);

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_SELECT_H_
