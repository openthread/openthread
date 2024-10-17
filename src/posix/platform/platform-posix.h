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

#ifndef OT_PLATFORM_POSIX_H_
#define OT_PLATFORM_POSIX_H_

#include "openthread-posix-config.h"

#include <errno.h>
#include <net/if.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/logging.h>
#include <openthread/openthread-system.h>
#include <openthread/platform/time.h>

#include "lib/platform/exit_code.h"
#include "lib/spinel/coprocessor_type.h"
#include "lib/url/url.hpp"

/**
 * @def OPENTHREAD_POSIX_VIRTUAL_TIME
 *
 * This setting configures whether to use virtual time.
 */
#ifndef OPENTHREAD_POSIX_VIRTUAL_TIME
#define OPENTHREAD_POSIX_VIRTUAL_TIME 0
#endif

/**
 * This is the socket name used by daemon mode.
 */
#define OPENTHREAD_POSIX_DAEMON_SOCKET_NAME OPENTHREAD_POSIX_CONFIG_DAEMON_SOCKET_BASENAME ".sock"

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
struct VirtualTimeEvent
{
    uint64_t mDelay;
    uint8_t  mEvent;
    uint16_t mDataLength;
    uint8_t  mData[OT_EVENT_DATA_MAX_SIZE];
} OT_TOOL_PACKED_END;

/**
 * Initializes the alarm service used by OpenThread.
 *
 * @param[in]  aSpeedUpFactor   The speed up factor.
 * @param[in]  aRealTimeSignal  The real time signal for microsecond alarms.
 */
void platformAlarmInit(uint32_t aSpeedUpFactor, int aRealTimeSignal);

/**
 * Retrieves the time remaining until the alarm fires.
 *
 * @param[out]  aTimeval  A pointer to the timeval struct.
 */
void platformAlarmUpdateTimeout(struct timeval *tv);

/**
 * Performs alarm driver processing.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 */
void platformAlarmProcess(otInstance *aInstance);

/**
 * Returns the next alarm event time.
 *
 * @returns The next alarm fire time.
 */
int32_t platformAlarmGetNext(void);

/**
 * Advances the alarm time by @p aDelta.
 *
 * @param[in]  aDelta  The amount of time to advance.
 */
void platformAlarmAdvanceNow(uint64_t aDelta);

/**
 * Initializes the radio service used by OpenThread.
 *
 * @note Even when @p aPlatformConfig->mResetRadio is false, a reset event (i.e. a PROP_LAST_STATUS between
 * [SPINEL_STATUS_RESET__BEGIN, SPINEL_STATUS_RESET__END]) is still expected from RCP.
 *
 * @param[in]   aUrl  A pointer to the null-terminated radio URL.
 */
void platformRadioInit(const char *aUrl);

/**
 * Shuts down the radio service used by OpenThread.
 */
void platformRadioDeinit(void);

/**
 * Handles the state change events for the radio driver.
 *
 * @param[in] aInstance  A pointer to the OpenThread instance.
 * @param[in] aFlags     Flags that denote the state change events.
 */
void platformRadioHandleStateChange(otInstance *aInstance, otChangedFlags aFlags);

/**
 * Inputs a received radio frame.
 *
 * @param[in]  aInstance   A pointer to the OpenThread instance.
 * @param[in]  aBuf        A pointer to the received radio frame.
 * @param[in]  aBufLength  The size of the received radio frame.
 */
void platformRadioReceive(otInstance *aInstance, uint8_t *aBuf, uint16_t aBufLength);

/**
 * Updates the file descriptor sets with file descriptors used by the radio driver.
 *
 * @param[in]   aContext    A pointer to the mainloop context.
 */
void platformRadioUpdateFdSet(otSysMainloopContext *aContext);

/**
 * Performs radio driver processing.
 *
 * @param[in]   aContext    A pointer to the mainloop context.
 */
void platformRadioProcess(otInstance *aInstance, const otSysMainloopContext *aContext);

/**
 * Initializes the random number service used by OpenThread.
 */
void platformRandomInit(void);

/**
 * Initializes the logging service used by OpenThread.
 *
 * @param[in] aName   A name string which will be prefixed to each log line.
 */
void platformLoggingInit(const char *aName);

/**
 * Updates the file descriptor sets with file descriptors used by the UART driver.
 *
 * @param[in]   aContext    A pointer to the mainloop context.
 */
void platformUartUpdateFdSet(otSysMainloopContext *aContext);

/**
 * Performs radio driver processing.
 *
 * @param[in]   aContext    A pointer to the mainloop context.
 */
void platformUartProcess(const otSysMainloopContext *aContext);

/**
 * Initializes platform netif.
 *
 * @note This function is called before OpenThread instance is created.
 *
 * @param[in]   aInterfaceName  A pointer to Thread network interface name.
 */
void platformNetifInit(otPlatformConfig *aPlatformConfig);

/**
 * Sets up platform netif.
 *
 * @note This function is called after OpenThread instance is created.
 *
 * @param[in]   aInstance       A pointer to the OpenThread instance.
 */
void platformNetifSetUp(void);

/**
 * Tears down platform netif.
 *
 * @note This function is called before OpenThread instance is destructed.
 */
void platformNetifTearDown(void);

/**
 * Deinitializes platform netif.
 *
 * @note This function is called after OpenThread instance is destructed.
 */
void platformNetifDeinit(void);

/**
 * Updates the file descriptor sets with file descriptors used by platform netif module.
 *
 * @param[in,out]  aContext  A pointer to the mainloop context.
 */
void platformNetifUpdateFdSet(otSysMainloopContext *aContext);

/**
 * Performs platform netif processing.
 *
 * @param[in]  aContext  A pointer to the mainloop context.
 */
void platformNetifProcess(const otSysMainloopContext *aContext);

/**
 * Performs notifies state changes to platform netif.
 *
 * @param[in]   aInstance       A pointer to the OpenThread instance.
 * @param[in]   aFlags          Flags that denote the state change events.
 */
void platformNetifStateChange(otInstance *aInstance, otChangedFlags aFlags);

/**
 * Initialize virtual time simulation.
 *
 * @params[in]  aNodeId     Node id of this simulated device.
 */
void virtualTimeInit(uint16_t aNodeId);

/**
 * Deinitialize virtual time simulation.
 */
void virtualTimeDeinit(void);

/**
 * Performs virtual time simulation processing.
 *
 * @param[in]  aContext  A pointer to the mainloop context.
 */
void virtualTimeProcess(otInstance *aInstance, const otSysMainloopContext *aContext);

/**
 * Updates the file descriptor sets with file descriptors
 * used by the virtual time simulation.
 *
 * @param[in,out]  aContext  A pointer to the mainloop context.
 */
void virtualTimeUpdateFdSet(otSysMainloopContext *aContext);

/**
 * Sends radio spinel event of virtual time simulation.
 *
 * @param[in] aData     A pointer to the spinel frame.
 * @param[in] aLength   Length of the spinel frame.
 */
void virtualTimeSendRadioSpinelWriteEvent(const uint8_t *aData, uint16_t aLength);

/**
 * Receives an event of virtual time simulation.
 *
 * @param[out]  aEvent  A pointer to the event receiving the event.
 */
void virtualTimeReceiveEvent(struct VirtualTimeEvent *aEvent);

/**
 * Sends sleep event through virtual time simulation.
 *
 * @param[in]   aTimeout    A pointer to the time sleeping.
 */
void virtualTimeSendSleepEvent(const struct timeval *aTimeout);

/**
 * Performs radio processing of virtual time simulation.
 *
 * @param[in]   aInstance   A pointer to the OpenThread instance.
 * @param[in]   aEvent      A pointer to the current event.
 */
void virtualTimeRadioProcess(otInstance *aInstance, const struct VirtualTimeEvent *aEvent);

/**
 * Performs radio  processing of virtual time simulation.
 *
 * @param[in]   aInstance   A pointer to the OpenThread instance.
 * @param[in]   aEvent      A pointer to the current event.
 */
void virtualTimeSpinelProcess(otInstance *aInstance, const struct VirtualTimeEvent *aEvent);

enum SocketBlockOption
{
    kSocketBlock,
    kSocketNonBlock,
};

/**
 * Initializes platform TREL UDP6 driver.
 *
 * @param[in]   aTrelUrl   The TREL URL (configuration for TREL platform).
 */
void platformTrelInit(const char *aTrelUrl);

/**
 * Shuts down the platform TREL UDP6 platform driver.
 */
void platformTrelDeinit(void);

/**
 * Updates the file descriptor sets with file descriptors used by the TREL driver.
 *
 * @param[in,out]  aContext  A pointer to the mainloop context.
 */
void platformTrelUpdateFdSet(otSysMainloopContext *aContext);

/**
 * Performs TREL driver processing.
 *
 * @param[in]  aContext  A pointer to the mainloop context.
 */
void platformTrelProcess(otInstance *aInstance, const otSysMainloopContext *aContext);

/**
 * Creates a socket with SOCK_CLOEXEC flag set.
 *
 * @param[in]   aDomain       The communication domain.
 * @param[in]   aType         The semantics of communication.
 * @param[in]   aProtocol     The protocol to use.
 * @param[in]   aBlockOption  Whether to add nonblock flags.
 *
 * @returns The file descriptor of the created socket.
 *
 * @retval  -1  Failed to create socket.
 */
int SocketWithCloseExec(int aDomain, int aType, int aProtocol, SocketBlockOption aBlockOption);

/**
 * The name of Thread network interface.
 */
extern char gNetifName[IFNAMSIZ];

/**
 * The index of Thread network interface.
 */
extern unsigned int gNetifIndex;

/**
 * A pointer to the OpenThread instance.
 */
extern otInstance *gInstance;

/**
 * Initializes backtrace module.
 */
void platformBacktraceInit(void);

/**
 * Initializes the spinel service used by OpenThread.
 *
 * @param[in]   aUrl  A pointer to the null-terminated spinel URL.
 *
 * @retval  OT_COPROCESSOR_UNKNOWN  The initialization fails.
 * @retval  OT_COPROCESSOR_RCP      The Co-processor is a RCP.
 * @retval  OT_COPROCESSOR_NCP      The Co-processor is a NCP.
 */
CoprocessorType platformSpinelManagerInit(const char *aUrl);

/**
 * Shuts down the spinel service used by OpenThread.
 */
void platformSpinelManagerDeinit(void);

/**
 * Performs spinel driver processing.
 *
 * @param[in]   aInstance   A pointer to the OT instance.
 * @param[in]   aContext    A pointer to the mainloop context.
 */
void platformSpinelManagerProcess(otInstance *aInstance, const otSysMainloopContext *aContext);

/**
 * Updates the file descriptor sets with file descriptors used by the spinel driver.
 *
 * @param[in]   aContext    A pointer to the mainloop context.
 */
void platformSpinelManagerUpdateFdSet(otSysMainloopContext *aContext);

/**
 * Initializes the resolver used by OpenThread.
 */
void platformResolverInit(void);

/**
 * Updates the file descriptor sets with file descriptors used by the resolver.
 *
 * @param[in]   aContext    A pointer to the mainloop context.
 */
void platformResolverUpdateFdSet(otSysMainloopContext *aContext);

/**
 * Performs the resolver processing.
 *
 * @param[in]  aContext  A pointer to the mainloop context.
 */
void platformResolverProcess(const otSysMainloopContext *aContext);

#ifdef __cplusplus
}
#endif
#endif // OT_PLATFORM_POSIX_H_
