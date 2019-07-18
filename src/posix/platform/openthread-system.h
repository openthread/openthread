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
 *   This file defines the platform-specific functions needed by OpenThread's example applications.
 */

#ifndef OPENTHREAD_SYSTEM_H_
#define OPENTHREAD_SYSTEM_H_

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/select.h>

#include <openthread/error.h>
#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This enumeration represents exit codes used when OpenThread exits.
 *
 */
enum
{
    /**
     * Success.
     */
    OT_EXIT_SUCCESS = 0,

    /**
     * Generic failure.
     */
    OT_EXIT_FAILURE = 1,

    /**
     * Invalid arguments.
     */
    OT_EXIT_INVALID_ARGUMENTS = 2,

    /**
     * Incompatible radio spinel.
     */
    OT_EXIT_RADIO_SPINEL_INCOMPATIBLE = 3,

    /**
     * Unexpected radio spinel reset.
     */
    OT_EXIT_RADIO_SPINEL_RESET = 4,

    /**
     * System call or library function error.
     */
    OT_EXIT_ERROR_ERRNO = 5,
};

/**
 * This structure represents platform specific configurations.
 *
 */
typedef struct otPlatformConfig
{
    uint64_t    mNodeId;        /// Unique node ID.
    const char *mInterfaceName; /// Thread network interface name.
    const char *mRadioFile;     /// Radio file path.
    const char *mRadioConfig;   /// Radio configurations.
    uint32_t    mSpeedUpFactor; /// Speed up factor.
    bool        mResetRadio;    /// Whether to reset RCP when initializing.
} otPlatformConfig;

/**
 * This function performs all platform-specific initialization of OpenThread's drivers.
 *
 * @note This function is not called by the OpenThread library. Instead, the system/RTOS should call this function
 *       when initialization of OpenThread's drivers is most appropriate.
 *
 * @param[in]  aPlatformConfig  Argument vector.
 *
 * @returns A pointer to the OpenThread instance.
 *
 */
otInstance *otSysInit(otPlatformConfig *aPlatformConfig);

/**
 * This function performs all platform-specific deinitialization for OpenThread's drivers.
 *
 * @note This function is not called by the OpenThread library. Instead, the system/RTOS should call this function
 *       when deinitialization of OpenThread's drivers is most appropriate.
 *
 */
void otSysDeinit(void);

/**
 * This structure represents a context for a select() based mainloop.
 *
 */
typedef struct otSysMainloopContext
{
    fd_set         mReadFdSet;  ///< The read file descriptors.
    fd_set         mWriteFdSet; ///< The write file descriptors.
    fd_set         mErrorFdSet; ///< The error file descriptors.
    int            mMaxFd;      ///< The max file descriptor.
    struct timeval mTimeout;    ///< The timeout.
} otSysMainloopContext;

/**
 * This function updates the file descriptor sets with file descriptors used by OpenThread drivers.
 *
 * @param[in]       aInstance   The OpenThread instance structure.
 * @param[inout]    aMainloop   A pointer to the mainloop context.
 *
 */
void otSysMainloopUpdate(otInstance *aInstance, otSysMainloopContext *aMainloop);

/**
 * This function polls OpenThread's mainloop.
 *
 * @param[inout]    aMainloop   A pointer to the mainloop context.
 *
 * @returns value returned from select().
 *
 */
int otSysMainloopPoll(otSysMainloopContext *aMainloop);

/**
 * This function performs all platform-specific processing for OpenThread's example applications.
 *
 * @note This function is not called by the OpenThread library. Instead, the system/RTOS should call this function
 *       in the main loop when processing OpenThread's drivers is most appropriate.
 *
 * @param[in]   aInstance   The OpenThread instance structure.
 * @param[in]   aMainloop   A pointer to the mainloop context.
 *
 */
void otSysMainloopProcess(otInstance *aInstance, const otSysMainloopContext *aMainloop);

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_SYSTEM_H_
