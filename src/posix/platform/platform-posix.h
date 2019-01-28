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
 *   This file includes the (posix or windows) platform-specific initializers.
 */

#ifndef PLATFORM_POSIX_H_
#define PLATFORM_POSIX_H_

#include <sys/select.h>
#include <sys/time.h>

#include <openthread/instance.h>

/**
 * @def OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE
 *
 * Define as 1 to enable PTY device support in POSIX app.
 *
 */
#ifndef OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE
#define OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    OT_SIM_EVENT_ALARM_FIRED        = 0,
    OT_SIM_EVENT_RADIO_RECEIVED     = 1,
    OT_SIM_EVENT_UART_WRITE         = 2,
    OT_SIM_EVENT_RADIO_SPINEL_WRITE = 3,
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
};

/**
 * Unique node ID.
 *
 */
extern uint64_t gNodeId;

/**
 * This function initializes the alarm service used by OpenThread.
 *
 */
void platformAlarmInit(uint32_t aSpeedUpFactor);

/**
 * This function retrieves the time remaining until the alarm fires.
 *
 * @param[out]  aTimeval  A pointer to the timeval struct.
 *
 */
void platformAlarmUpdateTimeout(struct timeval *tv);

/**
 * This function performs alarm driver processing.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 *
 */
void platformAlarmProcess(otInstance *aInstance);

/**
 * This function returns the next alarm event time.
 *
 * @returns The next alarm fire time.
 *
 */
int32_t platformAlarmGetNext(void);

/**
 * This function returns the current alarm time.
 *
 * @returns The current alarm time.
 *
 */
uint64_t platformAlarmGetNow(void);

/**
 * This function advances the alarm time by @p aDelta.
 *
 * @param[in]  aDelta  The amount of time to advance.
 *
 */
void platformAlarmAdvanceNow(uint64_t aDelta);

/**
 * This function initializes the radio service used by OpenThread.
 *
 * @param[in]  aRadioFile       A pointer to the radio file.
 * @param[in]  aRadioConfig     A pointer to the radio config.
 *
 */
void platformRadioInit(const char *aRadioFile, const char *aRadioConfig);

/**
 * This function shuts down the radio service used by OpenThread.
 *
 */
void platformRadioDeinit(void);

/**
 * This function inputs a received radio frame.
 *
 * @param[in]  aInstance   A pointer to the OpenThread instance.
 * @param[in]  aBuf        A pointer to the received radio frame.
 * @param[in]  aBufLength  The size of the received radio frame.
 *
 */
void platformRadioReceive(otInstance *aInstance, uint8_t *aBuf, uint16_t aBufLength);

/**
 * This function updates the file descriptor sets with file descriptors used by the radio driver.
 *
 * @param[inout]  aReadFdSet   A pointer to the read file descriptors.
 * @param[inout]  aWriteFdSet  A pointer to the write file descriptors.
 * @param[inout]  aMaxFd       A pointer to the max file descriptor.
 * @param[inout]  aTimeout     A pointer to the timeout.
 *
 */
void platformRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd, struct timeval *aTimeout);

/**
 * This function performs radio driver processing.
 *
 * @param[in]   aInstance       A pointer to the OpenThread instance.
 * @param[in]   aReadFdSet      A pointer to the read file descriptors.
 * @param[in]   aWriteFdSet     A pointer to the write file descriptors.
 *
 */
void platformRadioProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet);

/**
 * This function initializes the random number service used by OpenThread.
 *
 */
void platformRandomInit(void);

/**
 * This function initializes the logging service used by OpenThread.
 *
 * @param[in] aName   A name string which will be prefixed to each log line.
 *
 */
void platformLoggingInit(const char *aName);

/**
 * This function updates the file descriptor sets with file descriptors used by the UART driver.
 *
 * @param[inout]  aReadFdSet   A pointer to the read file descriptors.
 * @param[inout]  aWriteFdSet  A pointer to the write file descriptors.
 * @param[inout]  aMaxFd       A pointer to the max file descriptor.
 *
 */
void platformUartUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, fd_set *aErrorFdSet, int *aMaxFd);

/**
 * This function performs radio driver processing.
 *
 * @param[in]   aReadFdSet      A pointer to the read file descriptors.
 * @param[in]   aWriteFdSet     A pointer to the write file descriptors.
 * @param[in]   aErrorFdSet     A pointer to the error file descriptors.
 *
 */
void platformUartProcess(const fd_set *aReadFdSet, const fd_set *aWriteFdSet, const fd_set *aErrorFdSet);

/**
 * This function initializes platform netif.
 *
 * @param[in]   aInstance       A pointer to the OpenThread instance.
 *
 */
void platformNetifInit(otInstance *aInstance);

/**
 * This function updates the file descriptor sets with file descriptors used by platform netif module.
 *
 * @param[inout]  aReadFdSet    A pointer to the read file descriptors.
 * @param[inout]  aWriteFdSet   A pointer to the write file descriptors.
 * @param[inout]  aErrorFdSet   A pointer to the error file descriptors.
 * @param[inout]  aMaxFd        A pointer to the max file descriptor.
 *
 */
void platformNetifUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, fd_set *aErrorFdSet, int *aMaxFd);

/**
 * This function performs platform netif processing.
 *
 * @param[in]   aReadFdSet      A pointer to the read file descriptors.
 * @param[in]   aWriteFdSet     A pointer to the write file descriptors.
 * @param[in]   aErrorFdSet     A pointer to the error file descriptors.
 *
 */
void platformNetifProcess(const fd_set *aReadFdSet, const fd_set *aWriteFdSet, const fd_set *aErrorFdSet);

/**
 * This function restores the Uart.
 *
 */
void platformUartRestore(void);

/**
 * This function initialize simulation.
 *
 */
void otSimInit(void);

/**
 * This function deinitialize simulation.
 *
 */
void otSimDeinit(void);

/**
 * This function gets simulation time.
 *
 * @param[in] aTime   A pointer to a timeval receiving the current time.
 *
 */
void otSimGetTime(struct timeval *aTime);

/**
 * This function performs simulation processing.
 *
 * @param[in]   aInstance       A pointer to the OpenThread instance.
 * @param[in]   aReadFdSet      A pointer to the read file descriptors.
 * @param[in]   aWriteFdSet     A pointer to the write file descriptors.
 *
 */
void otSimProcess(otInstance *  aInstance,
                  const fd_set *aReadFdSet,
                  const fd_set *aWriteFdSet,
                  const fd_set *aErrorFdSet);

/**
 * This function updates the file descriptor sets with file descriptors used by the simulation.
 *
 * @param[inout]  aReadFdSet   A pointer to the read file descriptors.
 * @param[inout]  aWriteFdSet  A pointer to the write file descriptors.
 * @param[inout]  aErrorFdSet  A pointer to the error file descriptors.
 * @param[inout]  aMaxFd       A pointer to the max file descriptor.
 * @param[inout]  aTimeout     A pointer to the timeout.
 *
 */
void otSimUpdateFdSet(fd_set *        aReadFdSet,
                      fd_set *        aWriteFdSet,
                      fd_set *        aErrorFdSet,
                      int *           aMaxFd,
                      struct timeval *aTimeout);

/**
 * This function sends radio spinel frame through simulation.
 *
 * @param[in] aData     A pointer to the spinel frame.
 * @param[in] aLength   Length of the spinel frame.
 *
 */
void otSimSendRadioSpinelWriteEvent(const uint8_t *aData, uint16_t aLength);

/**
 * This function receives a simulation event.
 *
 * @param[out]  aEvent  A pointer to the event receiving the event.
 *
 */
void otSimReceiveEvent(struct Event *aEvent);

/**
 * This function sends sleep event through simulation.
 *
 * @param[in]   aTimeout    A pointer to the time sleeping.
 *
 */
void otSimSendSleepEvent(const struct timeval *aTimeout);

/**
 * This function updates the file descriptor sets with file descriptors used by radio spinel.
 *
 * @param[out]   aTimeout    A pointer to the timeout event to be updated.
 *
 */
void otSimRadioSpinelUpdate(struct timeval *atimeout);

/**
 * This function performs radio spinel processing.
 *
 * @param[in]   aInstance   A pointer to the OpenThread instance.
 * @param[in]   aEvent      A pointer to the current event.
 *
 */
void otSimRadioSpinelProcess(otInstance *aInstance, const struct Event *aEvent);

#if OPENTHREAD_POSIX_VIRTUAL_TIME
#define otSysGetTime(aTime) otSimGetTime(aTime)
#else
#define otSysGetTime(aTime) gettimeofday(aTime, NULL)
#endif

/**
 * This function initializes platform UDP driver.
 *
 * @param[in]   aIfName   The name of Thread's platform network interface.
 *
 */
void platformUdpInit(const char *aIfName);

/**
 * This function performs platform UDP driver processing.
 *
 * @param[in]   aInstance   The OpenThread instance structure.
 * @param[in]   aReadFdSet  A pointer to the read file descriptors.
 *
 */
void platformUdpProcess(otInstance *aInstance, const fd_set *aReadSet);

/**
 * This function updates the file descriptor sets with file descriptors used by the platform UDP driver.
 *
 * @param[in]     aInstance    The OpenThread instance structure.
 * @param[inout]  aReadFdSet   A pointer to the read file descriptors.
 * @param[inout]  aMaxFd       A pointer to the max file descriptor.
 */
void platformUdpUpdateFdSet(otInstance *aInstance, fd_set *aReadFdSet, int *aMaxFd);

/**
 * This function ends the current process with exit code @p aExitCode if @p aCondition is false.
 *
 * @param[in]   aCondition  The condition to verify
 * @param[in]   aExitCode   The exit code if exits.
 *
 */
void VerifyOrDie(bool aCondition, int aExitCode);

/**
 * This function ends the current process if @p aError is not OT_ERROR_NONE.
 * The error code will be mapped from @p aError.
 *
 * @param[in]   aError  The OpenThread error code.
 *
 */
void SuccessOrDie(otError aError);

#ifdef __cplusplus
}
#endif
#endif // PLATFORM_POSIX_H_
