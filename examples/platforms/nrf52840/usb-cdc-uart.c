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

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <common/logging.hpp>
#include <utils/code_utils.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/misc.h>
#include <openthread/platform/uart.h>

#include "platform-nrf5.h"

#include "drivers/clock/nrf_drv_clock.h"
#include "drivers/power/nrf_drv_power.h"
#include "libraries/usb/app_usbd.h"
#include "libraries/usb/app_usbd_serial_num.h"
#include "libraries/usb/class/cdc/acm/app_usbd_cdc_acm.h"
#include "libraries/usb/nrf_dfu_trigger_usb.h"

#if (USB_CDC_AS_SERIAL_TRANSPORT == 1)

static void cdcAcmUserEventHandler(app_usbd_class_inst_t const *aInstance, app_usbd_cdc_acm_user_event_t aEvent);

#define CDC_ACM_COMM_EPIN NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_EPIN NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT NRF_DRV_USBD_EPOUT1

APP_USBD_CDC_ACM_GLOBAL_DEF(sAppCdcAcm,
                            cdcAcmUserEventHandler,
                            USB_CDC_ACM_COMM_INTERFACE,
                            USB_CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

// Rx buffer length must by multiple of NRF_DRV_USBD_EPSIZE.
static char sRxBuffer[NRF_DRV_USBD_EPSIZE * ((UART_RX_BUFFER_SIZE + NRF_DRV_USBD_EPSIZE - 1) / NRF_DRV_USBD_EPSIZE)];

static struct
{
    const uint8_t *mTxBuffer;
    uint16_t       mTxSize;
    size_t         mReceivedDataSize;
    bool           mUartEnabled;
    bool           mLastConnectionStatus;
    uint32_t       mOpenTimestamp;
    volatile bool  mConnected;
    volatile bool  mReadyToStart;
    volatile bool  mTransferInProgress;
    volatile bool  mTransferDone;
    volatile bool  mReceiveDone;
} sUsbState;

static void cdcAcmUserEventHandler(app_usbd_class_inst_t const *aCdcAcmInstance, app_usbd_cdc_acm_user_event_t aEvent)
{
    app_usbd_cdc_acm_t const *cdcAcmClass = app_usbd_cdc_acm_class_get(aCdcAcmInstance);

    switch (aEvent)
    {
    case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        // Setup first transfer.
        (void)app_usbd_cdc_acm_read_any(&sAppCdcAcm, sRxBuffer, sizeof(sRxBuffer));
        sUsbState.mOpenTimestamp = otPlatAlarmMilliGetNow();
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

    case APP_USBD_EVT_POWER_DETECTED:
        // Workaround for missing port open event.
        sAppCdcAcm.specific.p_data->ctx.line_state = 0;

        sUsbState.mConnected = true;
        break;

    case APP_USBD_EVT_POWER_REMOVED:
        sUsbState.mConnected = false;
        break;

    case APP_USBD_EVT_POWER_READY:
        sUsbState.mReadyToStart = true;
        break;

    default:
        break;
    }
}

static bool hasPortOpenDelayPassed(void)
{
    int32_t timeDiff = otPlatAlarmMilliGetNow() - sUsbState.mOpenTimestamp;

    return (timeDiff < 0) || (timeDiff > USB_HOST_UART_CONFIG_DELAY_MS);
}

static bool isPortOpened(void)
{
    uint32_t value;

    if (app_usbd_cdc_acm_line_state_get(&sAppCdcAcm, APP_USBD_CDC_ACM_LINE_STATE_DTR, &value) == NRF_SUCCESS)
    {
        return (value != 0) && hasPortOpenDelayPassed();
    }

    return false;
}

static void processConnection(void)
{
    bool connectionStatus = sUsbState.mUartEnabled && sUsbState.mConnected;

    if (sUsbState.mLastConnectionStatus != connectionStatus)
    {
        sUsbState.mLastConnectionStatus = connectionStatus;

        if (connectionStatus)
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
    if (sUsbState.mReadyToStart)
    {
        sUsbState.mReadyToStart = false;

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
        if (sUsbState.mReceivedDataSize != 0)
        {
            otPlatUartReceived((const uint8_t *)sRxBuffer, sUsbState.mReceivedDataSize);
            sUsbState.mReceivedDataSize = 0;
        }

        // Setup next transfer.
        if (app_usbd_cdc_acm_read_any(&sAppCdcAcm, sRxBuffer, sizeof(sRxBuffer)) == NRF_SUCCESS)
        {
            sUsbState.mReceiveDone = false;
        }
    }
}

static void processTransmit(void)
{
    // If some data was requested to send while port was closed, send it now.
    if ((sUsbState.mTxBuffer != NULL) && isPortOpened())
    {
        if (app_usbd_cdc_acm_write(&sAppCdcAcm, sUsbState.mTxBuffer, sUsbState.mTxSize) == NRF_SUCCESS)
        {
            sUsbState.mTransferInProgress = true;
            sUsbState.mTxBuffer           = NULL;
            sUsbState.mTxSize             = 0;
        }
    }
    else if (sUsbState.mTransferDone)
    {
        sUsbState.mTransferDone       = false;
        sUsbState.mTransferInProgress = false;

        otPlatUartSendDone();
    }
}

void nrf5UartInit(void)
{
    static const app_usbd_config_t usbdConfig = {.ev_state_proc = usbdUserEventHandler};

    memset((void *)&sUsbState, 0, sizeof(sUsbState));

    app_usbd_serial_num_generate();

    ret_code_t ret = app_usbd_init(&usbdConfig);
    assert(ret == NRF_SUCCESS);

#if NRF_MODULE_ENABLED(APP_USBD_NRF_DFU_TRIGGER)
    ret = nrf_dfu_trigger_usb_init();
    assert(ret == NRF_SUCCESS);
#endif // NRF_MODULE_ENABLED(APP_USBD_NRF_DFU_TRIGGER)

    app_usbd_class_inst_t const *cdcAcmInstance = app_usbd_cdc_acm_class_inst_get(&sAppCdcAcm);
    ret                                         = app_usbd_class_append(cdcAcmInstance);
    assert(ret == NRF_SUCCESS);

    ret = app_usbd_power_events_enable();
    assert(ret == NRF_SUCCESS);
}

void nrf5UartDeinit(void)
{
    if (nrf_drv_usbd_is_started())
    {
        app_usbd_stop();

        while (app_usbd_event_queue_process())
        {
        }
    }
    else if (nrf_drv_usbd_is_enabled())
    {
        app_usbd_disable();
    }

    app_usbd_class_remove_all();
    app_usbd_uninit();
}

void nrf5UartClearPendingData(void)
{
    sUsbState.mTransferInProgress = false;
    sUsbState.mTxBuffer           = NULL;
    sUsbState.mTxSize             = 0;
}

void nrf5UartProcess(void)
{
    while (app_usbd_event_queue_process())
    {
    }

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
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sUsbState.mTransferInProgress == false, error = OT_ERROR_BUSY);
    otEXPECT_ACTION(sUsbState.mTxBuffer == NULL, error = OT_ERROR_BUSY);

    if (!isPortOpened())
    {
        // If port is closed, queue the message until it can be sent.
        sUsbState.mTxBuffer = aBuf;
        sUsbState.mTxSize   = aBufLength;
    }
    else
    {
        otEXPECT_ACTION(app_usbd_cdc_acm_write(&sAppCdcAcm, aBuf, aBufLength) == NRF_SUCCESS, error = OT_ERROR_FAILED);
        sUsbState.mTransferInProgress = true;
    }

exit:

    return error;
}

#endif // USB_CDC_AS_SERIAL_TRANSPORT == 1
