/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements the OpenThread platform abstraction for UART communication over USB CDC.
 *
 */

#if defined(__GNUC__)

#pragma GCC diagnostic ignored "-Wpedantic"

#endif

#include <openthread/config.h>
#include <openthread-core-config.h>

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#include <openthread/types.h>
#include <openthread/platform/uart.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/misc.h>

#include "platform-nrf5.h"

#include "drivers/clock/nrf_drv_clock.h"
#include "drivers/power/nrf_drv_power.h"
#include "libraries/usb/app_usbd.h"
#include "libraries/usb/class/cdc/acm/app_usbd_cdc_acm.h"

#if (USB_CDC_AS_SERIAL_TRANSPORT == 1)

static void cdcAcmUserEventHandler(app_usbd_class_inst_t const *aInstance,
                                   app_usbd_cdc_acm_user_event_t aEvent);

#define CDC_ACM_COMM_INTERFACE    0
#define CDC_ACM_COMM_EPIN         NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE    1
#define CDC_ACM_DATA_EPIN         NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT        NRF_DRV_USBD_EPOUT1

#define SERIAL_NUMBER_STRING_SIZE 16

#define CDC_ACM_INTERFACES_CONFIG()                 \
    APP_USBD_CDC_ACM_CONFIG(CDC_ACM_COMM_INTERFACE, \
                            CDC_ACM_COMM_EPIN,      \
                            CDC_ACM_DATA_INTERFACE, \
                            CDC_ACM_DATA_EPIN,      \
                            CDC_ACM_DATA_EPOUT)


static const uint8_t sCdcAcmClassDescriptors[] =
{
    APP_USBD_CDC_ACM_DEFAULT_DESC(
        CDC_ACM_COMM_INTERFACE,
        CDC_ACM_COMM_EPIN,
        CDC_ACM_DATA_INTERFACE,
        CDC_ACM_DATA_EPIN,
        CDC_ACM_DATA_EPOUT)
};

APP_USBD_CDC_ACM_GLOBAL_DEF(sAppCdcAcm,
                            CDC_ACM_INTERFACES_CONFIG(),
                            cdcAcmUserEventHandler,
                            sCdcAcmClassDescriptors);

// Rx buffer length must by multiple of NRF_DRV_USBD_EPSIZE.
static char sRxBuffer[NRF_DRV_USBD_EPSIZE * ((UART_RX_BUFFER_SIZE + NRF_DRV_USBD_EPSIZE - 1) / NRF_DRV_USBD_EPSIZE)];

static volatile struct
{
    const uint8_t *mTxBuffer;
    uint16_t       mTxSize;
    size_t         mReceivedDataSize;
    bool           mUartEnabled;
    bool           mLastConnectionStatus;
    bool           mConnected;
    bool           mReady;
    bool           mTransferDone;
    bool           mReceiveDone;
} sUsbState;

uint16_t gExternSerialNumber[SERIAL_NUMBER_STRING_SIZE + 1];

static void serialNumberStringCreate(void)
{
    gExternSerialNumber[0] = (uint16_t)APP_USBD_DESCRIPTOR_STRING << 8 | sizeof(gExternSerialNumber);

    uint32_t deviceIdHi = NRF_FICR->DEVICEID[1];
    uint32_t deviceIdLo = NRF_FICR->DEVICEID[0];

    uint64_t deviceId = (((uint64_t)deviceIdHi) << 32) | deviceIdLo;

    for (size_t i = 1; i < SERIAL_NUMBER_STRING_SIZE + 1; ++i)
    {
        char tmp[2];
        snprintf(tmp, sizeof(tmp), "%x", (unsigned int)(deviceId & 0xF));
        gExternSerialNumber[i] = tmp[0];
        deviceId >>= 4;
    }
}

static void cdcAcmUserEventHandler(app_usbd_class_inst_t const *aCdcAcmInstance,
                                   app_usbd_cdc_acm_user_event_t aEvent)
{
    app_usbd_cdc_acm_t const *cdcAcmClass = app_usbd_cdc_acm_class_get(aCdcAcmInstance);

    switch (aEvent)
    {
    case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        // Setup first transfer.
        (void)app_usbd_cdc_acm_read(&sAppCdcAcm, sRxBuffer, sizeof(sRxBuffer));
        break;

    case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
        break;

    case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
        sUsbState.mTransferDone = true;
        break;

    case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        sUsbState.mReceiveDone = true;
        // Get amount of data received.
        sUsbState.mReceivedDataSize = app_usbd_cdc_acm_rx_size(cdcAcmClass);
        break;

    default:
        break;
    }
}

static void usbdUserEventHandler(app_usbd_event_type_t aEvent)
{
    switch (aEvent)
    {
    case APP_USBD_EVT_STOPPED:
        app_usbd_disable();
        break;

    default:
        break;
    }
}

static void powerUsbEventHandler(nrf_drv_power_usb_evt_t aEvent)
{
    switch (aEvent)
    {
    case NRF_DRV_POWER_USB_EVT_DETECTED:
        // Workaround for missing port open event.
        sAppCdcAcm.specific.p_data->ctx.line_state = 0;

        sUsbState.mConnected = true;
        break;

    case NRF_DRV_POWER_USB_EVT_REMOVED:
        sUsbState.mConnected = false;
        break;

    case NRF_DRV_POWER_USB_EVT_READY:
        sUsbState.mReady = true;
        break;

    default:
        assert(false);
    }
}

static bool isPortOpened(void)
{
    uint32_t value;

    if (app_usbd_cdc_acm_line_state_get(&sAppCdcAcm, APP_USBD_CDC_ACM_LINE_STATE_DTR, &value) == NRF_SUCCESS)
    {
        return value;
    }

    return false;
}

static void processConnection(void)
{
    if (sUsbState.mLastConnectionStatus != (sUsbState.mUartEnabled && sUsbState.mConnected))
    {
        sUsbState.mLastConnectionStatus = sUsbState.mUartEnabled && sUsbState.mConnected;

        if (sUsbState.mUartEnabled && sUsbState.mConnected)
        {
            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
        }
        else
        {
            if (nrf_drv_usbd_is_started())
            {
                app_usbd_stop();
            }
            else
            {
                app_usbd_disable();
            }
        }
    }

    // Provide some delay so the OS can re-enumerate the device in case of reset.
    if (sUsbState.mReady)
    {
        if (((otPlatGetResetReason(NULL) == OT_PLAT_RESET_REASON_EXTERNAL) ||
             (otPlatGetResetReason(NULL) == OT_PLAT_RESET_REASON_SOFTWARE)) &&
            (otPlatAlarmMilliGetNow() < USB_INITIAL_DELAY_MS))
        {
            return;
        }

        sUsbState.mReady = false;

        if (nrf_drv_usbd_is_enabled())
        {
            app_usbd_start();
        }
    }
}

static void processReceive(void)
{
    if (sUsbState.mReceiveDone)
    {
        sUsbState.mReceiveDone = false;

        otPlatUartReceived((const uint8_t *)sRxBuffer, sUsbState.mReceivedDataSize);

        // Setup next transfer.
        (void)app_usbd_cdc_acm_read(&sAppCdcAcm, sRxBuffer, sizeof(sRxBuffer));
    }
}

static void processTransmit(void)
{
    // If some data was requested to send while port was closed, send it now.
    if ((sUsbState.mTxBuffer != NULL) && isPortOpened())
    {
        if (app_usbd_cdc_acm_write(&sAppCdcAcm, sUsbState.mTxBuffer, sUsbState.mTxSize) == NRF_SUCCESS)
        {
            sUsbState.mTxBuffer = NULL;
            sUsbState.mTxSize = 0;
        }
    }

    if (sUsbState.mTransferDone)
    {
        sUsbState.mTransferDone = false;

        otPlatUartSendDone();
    }
}

void nrf5UartInit(void)
{
    static const nrf_drv_power_usbevt_config_t powerConfig =
    {
        .handler = powerUsbEventHandler
    };

    static const app_usbd_config_t usbdConfig =
    {
        .ev_state_proc = usbdUserEventHandler
    };

    memset((void *)&sUsbState, 0, sizeof(sUsbState));

    serialNumberStringCreate();

    ret_code_t ret = nrf_drv_power_init(NULL);
    assert(ret == NRF_SUCCESS);

    ret = app_usbd_init(&usbdConfig);
    assert(ret == NRF_SUCCESS);

    app_usbd_class_inst_t const *cdcAcmInstance = app_usbd_cdc_acm_class_inst_get(&sAppCdcAcm);
    ret = app_usbd_class_append(cdcAcmInstance);
    assert(ret == NRF_SUCCESS);

    ret = nrf_drv_power_usbevt_init(&powerConfig);
    assert(ret == NRF_SUCCESS);
}

void nrf5UartDeinit(void)
{
    nrf_drv_power_usbevt_uninit();

    app_usbd_class_remove_all();
    app_usbd_uninit();

    nrf_drv_power_uninit();
}

void nrf5UartProcess(void)
{
    while (app_usbd_event_queue_process()) { }

    processConnection();
    processReceive();
    processTransmit();
}

otError otPlatUartEnable(void)
{
    sUsbState.mUartEnabled = true;

    return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
    sUsbState.mUartEnabled = false;

    return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    if (!isPortOpened())
    {
        // If port is closed, queue the message until it can be sent.
        if (sUsbState.mTxBuffer == NULL)
        {
            sUsbState.mTxBuffer = aBuf;
            sUsbState.mTxSize = aBufLength;
        }
        else
        {
            return OT_ERROR_BUSY;
        }
    }
    else if (app_usbd_cdc_acm_write(&sAppCdcAcm, aBuf, aBufLength) != NRF_SUCCESS)
    {
        return OT_ERROR_FAILED;
    }

    return OT_ERROR_NONE;
}

#endif // USB_CDC_AS_SERIAL_TRANSPORT == 1
