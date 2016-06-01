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
 * @brief
 *   This file includes the posix platform-specific initializers.
 */

#ifndef POSIX_PLATFORM_H_
#define POSIX_PLATFORM_H_

#include <stdint.h>
#include <sys/select.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unique node ID.
 *
 */
extern uint32_t NODE_ID;

/**
 * Well-known Unique ID used by a simulated radio that supports promiscuous mode.
 *
 */
extern uint32_t WELLKNOWN_NODE_ID;

/**
 * This method performs all platform-specific initialization.
 *
 */
void posixPlatformInit(void);

/**
 * This method performs all platform-specific processing.
 *
 */
void posixPlatformProcessDrivers(void);

/**
 * This method initializes the alarm service used by OpenThread.
 *
 */
void posixPlatformAlarmInit(void);

/**
 * This method retrieves the time remaining until the alarm fires.
 *
 * @param[out]  aTimeval  A pointer to the timeval struct.
 *
 */
void posixPlatformAlarmUpdateTimeout(struct timeval *tv);

/**
 * This method performs alarm driver processing.
 *
 */
void posixPlatformAlarmProcess(void);

/**
 * This method initializes the radio service used by OpenThread.
 *
 */
void posixPlatformRadioInit(void);

/**
 * This method updates the file descriptor sets with file descriptors used by the radio driver.
 *
 * @param[inout]  aReadFdSet   A pointer to the read file descriptors.
 * @param[inout]  aWriteFdSet  A pointer to the write file descriptors.
 * @param[inout]  aMaxFd       A pointer to the max file descriptor.
 *
 */
void posixPlatformRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd);

/**
 * This method performs radio driver processing.
 *
 */
void posixPlatformRadioProcess(void);

/**
 * This method initializes the random number service used by OpenThread.
 *
 */
void posixPlatformRandomInit(void);

/**
 * This method updates the file descriptor sets with file descriptors used by the serial driver.
 *
 * @param[inout]  aReadFdSet   A pointer to the read file descriptors.
 * @param[inout]  aWriteFdSet  A pointer to the write file descriptors.
 * @param[inout]  aMaxFd       A pointer to the max file descriptor.
 *
 */
void posixPlatformSerialUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd);

/**
 * This method performs radio driver processing.
 *
 */
void posixPlatformSerialProcess(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // POSIX_PLATFORM_H_
