/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#include <assert.h>
#include <string.h>

#include "bsp.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "gpiointerrupt.h"
#include "hal-config-board.h"
#include "hal_common.h"
#include "openthread-system.h"
#include "platform-efr32.h"
#include <common/logging.hpp>
#include <openthread-core-config.h>
#include <openthread/cli.h>
#include <openthread/config.h>
#include <openthread/dataset_ftd.h>
#include <openthread/diag.h>
#include <openthread/instance.h>
#include <openthread/link.h>
#include <openthread/message.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <openthread/udp.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/logging.h>

// Constants
#define MULTICAST_ADDR "ff03::1"
#define MULTICAST_PORT 123
#define RECV_PORT 234
#define SLEEPY_POLL_PERIOD_MS 5000
#define MTD_MESSAGE "mtd is awake"
#define FTD_MESSAGE "ftd button"

// Types
typedef struct ButtonArray
{
    GPIO_Port_TypeDef port;
    unsigned int      pin;
} ButtonArray_t;

// Prototypes
void deviceOutOfSleepCb(void);
bool sleepCb(void);
void setNetworkConfiguration(otInstance *aInstance);
void handleNetifStateChanged(uint32_t aFlags, void *aContext);
void gpioInit(void (*gpioCallback)(uint8_t pin));
void buttonCallback(uint8_t pin);
void initUdp(void);
void applicationTick(void);
void mtdReceiveCallback(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);

// Variables
static otInstance *        instance;
static otUdpSocket         sMtdSocket;
static otSockAddr          sMulticastSockAddr;
static const ButtonArray_t sButtonArray[BSP_BUTTON_COUNT] = BSP_BUTTON_INIT;
static bool                sButtonPressed                 = false;
static bool                sRxOnIdleButtonPressed         = false;
static bool                sLedOn                         = false;
static bool                sAllowDeepSleep                = false;
static bool                sTaskletsPendingSem            = true;

int main(int argc, char *argv[])
{
    otLinkModeConfig config;

    otSysInit(argc, argv);
    gpioInit(buttonCallback);

    instance = otInstanceInitSingle();
    assert(instance);

    otCliUartInit(instance);

    otLinkSetPollPeriod(instance, SLEEPY_POLL_PERIOD_MS);
    setNetworkConfiguration(instance);
    otSetStateChangedCallback(instance, handleNetifStateChanged, instance);

    config.mRxOnWhenIdle       = true;
    config.mSecureDataRequests = true;
    config.mDeviceType         = 0;
    config.mNetworkData        = 0;
    otThreadSetLinkMode(instance, config);

    initUdp();
    otIp6SetEnabled(instance, true);
    otThreadSetEnabled(instance, true);
    efr32SetSleepCallback(sleepCb, deviceOutOfSleepCb);

    while (!otSysPseudoResetWasRequested())
    {
        otTaskletsProcess(instance);
        otSysProcessDrivers(instance);

        applicationTick();

        // Put the EFR32 into deep sleep if callback sleepCb permits.
        efr32Sleep();
    }

    otInstanceFinalize(instance);
    return 0;
}

void deviceOutOfSleepCb(void)
{
    static uint32_t udpPacketSendTimer = 0;

    if (udpPacketSendTimer == 0)
    {
        udpPacketSendTimer = otPlatAlarmMilliGetNow();
    }

    if ((otPlatAlarmMilliGetNow() - udpPacketSendTimer) >= 5000)
    {
        sButtonPressed     = true;
        udpPacketSendTimer = 0;
    }
}

/*
 * Callback from efr32Sleep to indicate if it is ok to go into sleep mode.
 * This runs with interrupts disabled.
 */
bool sleepCb(void)
{
    bool allow;
    allow               = (sAllowDeepSleep && !sTaskletsPendingSem);
    sTaskletsPendingSem = false;
    return allow;
}

void otTaskletsSignalPending(otInstance *aInstance)
{
    (void)aInstance;
    sTaskletsPendingSem = true;
}

/*
 * Override default network settings, such as panid, so the devices can join a network
 */
void setNetworkConfiguration(otInstance *aInstance)
{
    static char          aNetworkName[] = "SleepyEFR32";
    otOperationalDataset aDataset;

    memset(&aDataset, 0, sizeof(otOperationalDataset));

    /*
     * Fields that can be configured in otOperationDataset to override defaults:
     *     Network Name, Mesh Local Prefix, Extended PAN ID, PAN ID, Delay Timer,
     *     Channel, Channel Mask Page 0, Network Master Key, PSKc, Security Policy
     */
    aDataset.mActiveTimestamp                      = 1;
    aDataset.mComponents.mIsActiveTimestampPresent = true;

    /* Set Channel to 15 */
    aDataset.mChannel                      = 15;
    aDataset.mComponents.mIsChannelPresent = true;

    /* Set Pan ID to 2222 */
    aDataset.mPanId                      = (otPanId)0x2222;
    aDataset.mComponents.mIsPanIdPresent = true;

    /* Set Extended Pan ID to C0DE1AB5C0DE1AB5 */
    uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0xC0, 0xDE, 0x1A, 0xB5, 0xC0, 0xDE, 0x1A, 0xB5};
    memcpy(aDataset.mExtendedPanId.m8, extPanId, sizeof(aDataset.mExtendedPanId));
    aDataset.mComponents.mIsExtendedPanIdPresent = true;

    /* Set master key to 1234C0DE1AB51234C0DE1AB51234C0DE */
    uint8_t key[OT_MASTER_KEY_SIZE] = {0x12, 0x34, 0xC0, 0xDE, 0x1A, 0xB5, 0x12, 0x34, 0xC0, 0xDE, 0x1A, 0xB5};
    memcpy(aDataset.mMasterKey.m8, key, sizeof(aDataset.mMasterKey));
    aDataset.mComponents.mIsMasterKeyPresent = true;

    /* Set Network Name to SleepyEFR32 */
    size_t length = strlen(aNetworkName);
    assert(length <= OT_NETWORK_NAME_MAX_SIZE);
    memcpy(aDataset.mNetworkName.m8, aNetworkName, length);
    aDataset.mComponents.mIsNetworkNamePresent = true;

    otDatasetSetActive(aInstance, &aDataset);
}

void handleNetifStateChanged(uint32_t aFlags, void *aContext)
{
    otLinkModeConfig config;

    if ((aFlags & OT_CHANGED_THREAD_ROLE) != 0)
    {
        otDeviceRole changedRole = otThreadGetDeviceRole(aContext);

        switch (changedRole)
        {
        case OT_DEVICE_ROLE_LEADER:
        case OT_DEVICE_ROLE_ROUTER:
            break;

        case OT_DEVICE_ROLE_CHILD:
            config.mRxOnWhenIdle       = 0;
            config.mSecureDataRequests = true;
            config.mDeviceType         = 0;
            config.mNetworkData        = 0;
            otThreadSetLinkMode(instance, config);
            sAllowDeepSleep = true;
            break;

        case OT_DEVICE_ROLE_DETACHED:
        case OT_DEVICE_ROLE_DISABLED:
            break;
        }
    }
}

/*
 * Provide, if required an "otPlatLog()" function
 */
#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_APP
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);
    OT_UNUSED_VARIABLE(aFormat);

    va_list ap;
    va_start(ap, aFormat);
    otCliPlatLogv(aLogLevel, aLogRegion, aFormat, ap);
    va_end(ap);
}
#endif

void gpioInit(void (*callback)(uint8_t pin))
{
    // set up button GPIOs to input with pullups
    for (int i = 0; i < BSP_BUTTON_COUNT; i++)
    {
        GPIO_PinModeSet(sButtonArray[i].port, sButtonArray[i].pin, gpioModeInputPull, 1);
    }
    // set up interrupt based callback function on falling edge
    GPIOINT_Init();
    GPIOINT_CallbackRegister(sButtonArray[0].pin, callback);
    GPIOINT_CallbackRegister(sButtonArray[1].pin, callback);
    GPIO_IntConfig(sButtonArray[0].port, sButtonArray[0].pin, false, true, true);
    GPIO_IntConfig(sButtonArray[1].port, sButtonArray[1].pin, false, true, true);

    BSP_LedsInit();
    BSP_LedClear(0);
    BSP_LedClear(1);
}

void initUdp(void)
{
    otError    error;
    otSockAddr sockaddr;

    memset(&sMulticastSockAddr, 0, sizeof sMulticastSockAddr);
    otIp6AddressFromString(MULTICAST_ADDR, &sMulticastSockAddr.mAddress);
    sMulticastSockAddr.mPort = MULTICAST_PORT;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.mPort = RECV_PORT;

    error = otUdpOpen(instance, &sMtdSocket, mtdReceiveCallback, NULL);

    if (error != OT_ERROR_NONE)
    {
        return;
    }

    error = otUdpBind(&sMtdSocket, &sockaddr);

    if (error != OT_ERROR_NONE)
    {
        otUdpClose(&sMtdSocket);
        return;
    }
}

void buttonCallback(uint8_t pin)
{
    if ((pin & 0x01) == 0x01)
    {
        sButtonPressed = true;
    }
    else if ((pin & 0x01) == 0x00)
    {
        sRxOnIdleButtonPressed = true;
    }
}

void applicationTick(void)
{
    otError          error = 0;
    otMessageInfo    messageInfo;
    otMessage *      message = NULL;
    char *           payload = MTD_MESSAGE;
    otLinkModeConfig config;

    if (sRxOnIdleButtonPressed == true)
    {
        sRxOnIdleButtonPressed     = false;
        sAllowDeepSleep            = !sAllowDeepSleep;
        config.mRxOnWhenIdle       = !sAllowDeepSleep;
        config.mSecureDataRequests = true;
        config.mDeviceType         = 0;
        config.mNetworkData        = 0;
        otThreadSetLinkMode(instance, config);
    }

    if (sButtonPressed == true)
    {
        sButtonPressed = false;

        memset(&messageInfo, 0, sizeof(messageInfo));
        memcpy(&messageInfo.mPeerAddr, &sMulticastSockAddr.mAddress, sizeof messageInfo.mPeerAddr);
        messageInfo.mPeerPort = sMulticastSockAddr.mPort;

        message = otUdpNewMessage(instance, NULL);

        if (message != NULL)
        {
            error = otMessageAppend(message, payload, (uint16_t)strlen(payload));

            if (error == OT_ERROR_NONE)
            {
                error = otUdpSend(&sMtdSocket, message, &messageInfo);

                if (error == OT_ERROR_NONE)
                {
                    return;
                }
            }
        }

        if (message != NULL)
        {
            otMessageFree(message);
        }
    }
}

void mtdReceiveCallback(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);
    uint8_t buf[1500];
    int     length;

    length      = otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);
    buf[length] = '\0';

    if (strcmp((char *)buf, FTD_MESSAGE) == 0)
    {
        sLedOn = !sLedOn;

        if (sLedOn)
        {
            BSP_LedSet(0);
        }
        else
        {
            BSP_LedClear(0);
        }
    }
}
