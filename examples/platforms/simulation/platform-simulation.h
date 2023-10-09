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
 *   This file includes the platform-specific initializers.
 */

#ifndef PLATFORM_SIMULATION_H_
#define PLATFORM_SIMULATION_H_

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <openthread/instance.h>

#include "openthread-core-config.h"
#include "platform-config.h"

enum
{
    OT_SIM_EVENT_ALARM_FIRED        = 0,
    OT_SIM_EVENT_RADIO_RECEIVED     = 1,
    OT_SIM_EVENT_UART_WRITE         = 2,
    OT_SIM_EVENT_RADIO_SPINEL_WRITE = 3,
    OT_SIM_EVENT_OTNS_STATUS_PUSH   = 5,
    OT_EVENT_DATA_MAX_SIZE          = 1024,
};

OT_TOOL_PACKED_BEGIN
struct Event
{
    uint64_t mDelay;
    uint8_t  mEvent;
    uint16_t mDataLength;
    uint8_t  mData[OT_EVENT_DATA_MAX_SIZE];
} OT_TOOL_PACKED_END;

enum
{
    MAX_NETWORK_SIZE = OPENTHREAD_SIMULATION_MAX_NETWORK_SIZE,
};

/**
 * Unique node ID.
 *
 */
extern uint32_t gNodeId;

/**
 * Initializes the alarm service used by OpenThread.
 *
 */
void platformAlarmInit(uint32_t aSpeedUpFactor);

/**
 * Retrieves the time remaining until the alarm fires.
 *
 * @param[out]  aTimeout  A pointer to the timeval struct.
 *
 */
void platformAlarmUpdateTimeout(struct timeval *aTimeout);

/**
 * Performs alarm driver processing.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 *
 */
void platformAlarmProcess(otInstance *aInstance);

/**
 * Returns the duration to the next alarm event time (in micro seconds)
 *
 * @returns The duration (in micro seconds) to the next alarm event.
 *
 */
uint64_t platformAlarmGetNext(void);

/**
 * Returns the current alarm time.
 *
 * @returns The current alarm time.
 *
 */
uint64_t platformAlarmGetNow(void);

/**
 * Advances the alarm time by @p aDelta.
 *
 * @param[in]  aDelta  The amount of time to advance.
 *
 */
void platformAlarmAdvanceNow(uint64_t aDelta);

/**
 * Initializes the radio service used by OpenThread.
 *
 */
void platformRadioInit(void);

/**
 * Shuts down the radio service used by OpenThread.
 *
 */
void platformRadioDeinit(void);

/**
 * Inputs a received radio frame.
 *
 * @param[in]  aInstance   A pointer to the OpenThread instance.
 * @param[in]  aBuf        A pointer to the received radio frame.
 * @param[in]  aBufLength  The size of the received radio frame.
 *
 */
void platformRadioReceive(otInstance *aInstance, uint8_t *aBuf, uint16_t aBufLength);

/**
 * Updates the file descriptor sets with file descriptors used by the radio driver.
 *
 * @param[in,out]  aReadFdSet   A pointer to the read file descriptors.
 * @param[in,out]  aWriteFdSet  A pointer to the write file descriptors.
 * @param[in,out]  aTimeout     A pointer to the timeout.
 * @param[in,out]  aMaxFd       A pointer to the max file descriptor.
 *
 */
void platformRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, struct timeval *aTimeout, int *aMaxFd);

/**
 * Performs radio driver processing.
 *
 * @param[in]  aInstance    The OpenThread instance structure.
 * @param[in]  aReadFdSet   A pointer to the read file descriptors.
 * @param[in]  aWriteFdSet  A pointer to the write file descriptors.
 *
 */
void platformRadioProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet);

/**
 * Initializes the random number service used by OpenThread.
 *
 */
void platformRandomInit(void);

/**
 * This functions set the file name to use for logging.
 *
 * @param[in] aName  The file name.
 *
 */
void platformLoggingSetFileName(const char *aName);

/**
 * Initializes the platform logging service.
 *
 * @param[in] aName    The log module name to set with syslog.
 *
 */
void platformLoggingInit(const char *aName);

/**
 * Finalizes the platform logging service.
 *
 */
void platformLoggingDeinit(void);

/**
 * Updates the file descriptor sets with file descriptors used by the UART driver.
 *
 * @param[in,out]  aReadFdSet   A pointer to the read file descriptors.
 * @param[in,out]  aWriteFdSet  A pointer to the write file descriptors.
 * @param[in,out]  aMaxFd       A pointer to the max file descriptor.
 *
 */
void platformUartUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, fd_set *aErrorFdSet, int *aMaxFd);

/**
 * Performs radio driver processing.
 *
 */
void platformUartProcess(void);

/**
 * Restores the Uart.
 *
 */
void platformUartRestore(void);

/**
 * Sends a simulation event.
 *
 * @param[in]   aEvent  A pointer to the simulation event to send
 *
 */
void otSimSendEvent(const struct Event *aEvent);

/**
 * Sends Uart data through simulation.
 *
 * @param[in]   aData       A pointer to the UART data.
 * @param[in]   aLength     Length of UART data.
 *
 */
void otSimSendUartWriteEvent(const uint8_t *aData, uint16_t aLength);

/**
 * Checks if radio transmitting is pending.
 *
 * @returns Whether radio transmitting is pending.
 *
 */
bool platformRadioIsTransmitPending(void);

/**
 * Parses an environment variable as an unsigned 16-bit integer.
 *
 * If the environment variable does not exist, this function does nothing.
 * If it is not a valid integer, this function will terminate the process with an error message.
 *
 * @param[in]   aEnvName  The name of the environment variable.
 * @param[out]  aValue    A pointer to the unsigned 16-bit integer.
 *
 */
void parseFromEnvAsUint16(const char *aEnvName, uint16_t *aValue);

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

/**
 * Initializes the TREL service.
 *
 * @param[in] aSpeedUpFactor   The time speed-up factor.
 *
 */
void platformTrelInit(uint32_t aSpeedUpFactor);

/**
 * Shuts down the TREL service.
 *
 */
void platformTrelDeinit(void);

/**
 * Updates the file descriptor sets with file descriptors used by the TREL.
 *
 * @param[in,out]  aReadFdSet   A pointer to the read file descriptors.
 * @param[in,out]  aWriteFdSet  A pointer to the write file descriptors.
 * @param[in,out]  aTimeout     A pointer to the timeout.
 * @param[in,out]  aMaxFd       A pointer to the max file descriptor.
 *
 */
void platformTrelUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, struct timeval *aTimeout, int *aMaxFd);

/**
 * Performs TREL processing.
 *
 * @param[in]  aInstance    The OpenThread instance structure.
 * @param[in]  aReadFdSet   A pointer to the read file descriptors.
 * @param[in]  aWriteFdSet  A pointer to the write file descriptors.
 *
 */
void platformTrelProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet);

#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#endif // PLATFORM_SIMULATION_H_
