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
#include <stdio.h>
#include <sys/select.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/platform/misc.h>

#include "lib/spinel/radio_spinel_metrics.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents default parameters for the SPI interface.
 *
 */
enum
{
    OT_PLATFORM_CONFIG_SPI_DEFAULT_MODE           = 0,       ///< Default SPI Mode: CPOL=0, CPHA=0.
    OT_PLATFORM_CONFIG_SPI_DEFAULT_SPEED_HZ       = 1000000, ///< Default SPI speed in hertz.
    OT_PLATFORM_CONFIG_SPI_DEFAULT_CS_DELAY_US    = 20,      ///< Default delay after SPI C̅S̅ assertion, in µsec.
    OT_PLATFORM_CONFIG_SPI_DEFAULT_RESET_DELAY_MS = 0, ///< Default delay after R̅E̅S̅E̅T̅ assertion, in milliseconds.
    OT_PLATFORM_CONFIG_SPI_DEFAULT_ALIGN_ALLOWANCE =
        16, ///< Default maximum number of 0xFF bytes to clip from start of MISO frame.
    OT_PLATFORM_CONFIG_SPI_DEFAULT_SMALL_PACKET_SIZE =
        32,                                ///< Default smallest SPI packet size we can receive in a single transaction.
    OT_PLATFORM_CONFIG_MAX_RADIO_URLS = 2, ///< Max number of Radio URLs.
};

/**
 * Represents platform specific configurations.
 *
 */
typedef struct otPlatformConfig
{
    const char *mBackboneInterfaceName;                        ///< Backbone network interface name.
    const char *mInterfaceName;                                ///< Thread network interface name.
    const char *mRadioUrls[OT_PLATFORM_CONFIG_MAX_RADIO_URLS]; ///< Radio URLs.
    uint8_t     mRadioUrlNum;                                  ///< Number of Radio URLs.
    int         mRealTimeSignal;                               ///< The real-time signal for microsecond timer.
    uint32_t    mSpeedUpFactor;                                ///< Speed up factor.
    bool        mPersistentInterface;                          ///< Whether persistent the interface
    bool        mDryRun;                                       ///< If 'DryRun' is set, the posix daemon will exit
                                                               ///< directly after initialization.
} otPlatformConfig;

/**
 * Performs all platform-specific initialization of OpenThread's drivers and initializes the OpenThread
 * instance.
 *
 * @note This function is not called by the OpenThread library. Instead, the system/RTOS should call this function
 *       when initialization of OpenThread's drivers is most appropriate.
 *
 * @param[in]  aPlatformConfig  Platform configuration structure.
 *
 * @returns A pointer to the OpenThread instance.
 *
 */
otInstance *otSysInit(otPlatformConfig *aPlatformConfig);

/**
 * Finalizes the OpenThread instance and performs all platform-specific deinitialization for OpenThread's
 * drivers.
 *
 * @note This function is not called by the OpenThread library. Instead, the system/RTOS should call this function
 *       when deinitialization of OpenThread's drivers is most appropriate.
 *
 */
void otSysDeinit(void);

/**
 * Represents a context for a select() based mainloop.
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
 * Updates the file descriptor sets with file descriptors used by OpenThread drivers.
 *
 * @param[in]       aInstance   The OpenThread instance structure.
 * @param[in,out]   aMainloop   A pointer to the mainloop context.
 *
 */
void otSysMainloopUpdate(otInstance *aInstance, otSysMainloopContext *aMainloop);

/**
 * Polls OpenThread's mainloop.
 *
 * @param[in,out]   aMainloop   A pointer to the mainloop context.
 *
 * @returns value returned from select().
 *
 */
int otSysMainloopPoll(otSysMainloopContext *aMainloop);

/**
 * Performs all platform-specific processing for OpenThread's example applications.
 *
 * @note This function is not called by the OpenThread library. Instead, the system/RTOS should call this function
 *       in the main loop when processing OpenThread's drivers is most appropriate.
 *
 * @param[in]   aInstance   The OpenThread instance structure.
 * @param[in]   aMainloop   A pointer to the mainloop context.
 *
 */
void otSysMainloopProcess(otInstance *aInstance, const otSysMainloopContext *aMainloop);

/**
 * Returns the radio url help string.
 *
 * @returns the radio url help string.
 *
 */
const char *otSysGetRadioUrlHelpString(void);

extern otPlatResetReason gPlatResetReason;

/**
 * Returns the Thread network interface name.
 *
 * @returns The Thread network interface name.
 *
 */
const char *otSysGetThreadNetifName(void);

/**
 * Returns the Thread network interface index.
 *
 * @returns The Thread network interface index.
 *
 */
unsigned int otSysGetThreadNetifIndex(void);

/**
 * Returns the infrastructure network interface name.
 *
 * @returns The infrastructure network interface name, or `nullptr` if not specified.
 *
 */
const char *otSysGetInfraNetifName(void);

/**
 * Returns the infrastructure network interface index.
 *
 * @returns The infrastructure network interface index.
 *
 */
uint32_t otSysGetInfraNetifIndex(void);

/**
 * Returns the radio spinel metrics.
 *
 * @returns The radio spinel metrics.
 *
 */
const otRadioSpinelMetrics *otSysGetRadioSpinelMetrics(void);

/**
 * Returns the RCP interface metrics.
 *
 * @returns The RCP interface metrics.
 *
 */
const otRcpInterfaceMetrics *otSysGetRcpInterfaceMetrics(void);

/**
 * Returns the ifr_flags of the infrastructure network interface.
 *
 * @returns The ifr_flags of infrastructure network interface.
 *
 */
uint32_t otSysGetInfraNetifFlags(void);

typedef struct otSysInfraNetIfAddressCounters
{
    uint32_t mLinkLocalAddresses;
    uint32_t mUniqueLocalAddresses;
    uint32_t mGlobalUnicastAddresses;
} otSysInfraNetIfAddressCounters;

/**
 * This functions counts the number of addresses on the infrastructure network interface.
 *
 * @param[out] aAddressCounters  The counters of addresses on infrastructure network interface.
 *
 */
void otSysCountInfraNetifAddresses(otSysInfraNetIfAddressCounters *aAddressCounters);

/**
 * Sets the infrastructure network interface and the ICMPv6 socket.
 *
 * This function specifies the network interface name and the ICMPv6 socket on that interface. After calling this
 * function, the caller can call otBorderRoutingInit() to let Border Routing work on that interface.
 *
 * @param[in] aInfraNetifName  The name of the infrastructure network interface.
 * @param[in] aIcmp6Socket     A SOCK_RAW socket running on the infrastructure network interface.
 *
 */
void otSysSetInfraNetif(const char *aInfraNetifName, int aIcmp6Socket);

/**
 * Returns TRUE if the infrastructure interface is running.
 *
 * @returns TRUE if the infrastructure interface is running, FALSE if not.
 *
 */
bool otSysInfraIfIsRunning(void);

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_SYSTEM_H_
