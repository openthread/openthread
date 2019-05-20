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
#include "openthread-core-config.h"
#include "platform-posix.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <termios.h>
#include <unistd.h>

#include <openthread/instance.h>
#include <openthread/platform/cordio/ble-hci.h>

#include "code_utils.h"

#if OPENTHREAD_ENABLE_BLE_HOST

#define BLE_HCI_DEFAULT_BAUDRATE 115200
#define BLE_HCI_TYPE_SIZE 1
#define BLE_HCI_ACL_HEADER_SIZE 4
#define BLE_HCI_ACL_MAX_LENGTH 255
#define BLE_HCI_BUF_SIZE (BLE_HCI_TYPE_SIZE + BLE_HCI_ACL_HEADER_SIZE + BLE_HCI_ACL_MAX_LENGTH)
#define BLE_HCI_FRAGMENT_LENGTH 30

static uint8_t        sRxBuffer[BLE_HCI_BUF_SIZE];
static const uint8_t *sTxBuffer;
static uint16_t       sTxLength = 0;

static int  sBleSerialFd      = -1;
static bool sBleSerialEnabled = false;

static inline uint32_t ttyGetSpeed(uint32_t aSpeed)
{
    uint32_t rval = 0;

    switch (aSpeed)
    {
    case 9600:
        rval = B9600;
        break;
    case 19200:
        rval = B19200;
        break;
    case 38400:
        rval = B38400;
        break;
    case 57600:
        rval = B57600;
        break;
    case 115200:
        rval = B115200;
        break;
#ifdef B230400
    case 230400:
        rval = B230400;
        break;
#endif
#ifdef B460800
    case 460800:
        rval = B460800;
        break;
#endif
#ifdef B500000
    case 500000:
        rval = B500000;
        break;
#endif
#ifdef B576000
    case 576000:
        rval = B576000;
        break;
#endif
#ifdef B921600
    case 921600:
        rval = B921600;
        break;
#endif
#ifdef B1000000
    case 1000000:
        rval = B1000000;
        break;
#endif
#ifdef B1152000
    case 1152000:
        rval = B1152000;
        break;
#endif
#ifdef B1500000
    case 1500000:
        rval = B1500000;
        break;
#endif
#ifdef B2000000
    case 2000000:
        rval = B2000000;
        break;
#endif
#ifdef B2500000
    case 2500000:
        rval = B2500000;
        break;
#endif
#ifdef B3000000
    case 3000000:
        rval = B3000000;
        break;
#endif
#ifdef B3500000
    case 3500000:
        rval = B3500000;
        break;
#endif
#ifdef B4000000
    case 4000000:
        rval = B4000000;
        break;
#endif
    }

    return rval;
}

static int bleHciOpenSerial(const char *aPath, uint32_t aSpeed, bool aFlowControl)
{
    struct termios tios;
    int            fd;
    const char *   errStr = NULL;

    otEXPECT_ACTION((fd = open(aPath, O_RDWR | O_NOCTTY)) != -1, errStr = "open");
    otEXPECT_ACTION(isatty(fd), errStr = "isatty");

    otEXPECT_ACTION(tcgetattr(fd, &tios) == 0, errStr = "tcgetattr");
    cfmakeraw(&tios);

    otEXPECT_ACTION((aSpeed = ttyGetSpeed(aSpeed)) > 0, errStr = "Invalid BLE HCI baudrate");

    cfsetispeed(&tios, aSpeed);
    cfsetospeed(&tios, aSpeed);

    tios.c_cflag |= (CLOCAL | CREAD);           // enable the receiver and set local mode
    tios.c_cflag &= (unsigned long)(~CSTOPB);   // 1 stop bit
    tios.c_cflag &= (unsigned long)(~PARENB);   // no parity check
    tios.c_cflag &= (unsigned long)(~CSIZE);    // mask the data size bits
    tios.c_cflag |= CS8;                        // 8 bits data
    tios.c_cflag |= aFlowControl ? CRTSCTS : 0; // RTS/CTS flow control

    otEXPECT_ACTION(tcsetattr(fd, TCSANOW, &tios) == 0, errStr = "tcsetattr");
    otEXPECT_ACTION(tcflush(fd, TCIOFLUSH) == 0, errStr = "tcflush");

exit:
    if (errStr != NULL)
    {
        if (fd >= 0)
        {
            close(fd);
            fd = -1;
        }

        perror(errStr);
        exit(OT_EXIT_FAILURE);
    }

    return fd;
}

void platformBleHciInit(const char *aDeviceFile, uint32_t aBaudrate)
{
    if (aDeviceFile == NULL)
    {
        perror("Invalid BLE HCI device file!\r\n");
        exit(OT_EXIT_FAILURE);
    }

    aBaudrate    = (aBaudrate == 0) ? BLE_HCI_DEFAULT_BAUDRATE : aBaudrate;
    sBleSerialFd = bleHciOpenSerial(aDeviceFile, aBaudrate, true);
}

void platformBleHciDeinit(void)
{
    close(sBleSerialFd);
    sBleSerialFd = -1;
}

bool otCordioPlatHciIsEnabled(void)
{
    return sBleSerialEnabled;
}

otError otCordioPlatHciEnable(void)
{
    sBleSerialEnabled = true;
    return OT_ERROR_NONE;
}

otError otCordioPlatHciDisable(void)
{
    sBleSerialEnabled = true;
    return OT_ERROR_NONE;
}

otError otCordioPlatHciSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sBleSerialEnabled, error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(sTxLength == 0, error = OT_ERROR_BUSY);

    sTxBuffer = aBuf;
    sTxLength = aBufLength;

exit:
    return error;
}

void platformBleHciUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, fd_set *aErrorFdSet, int *aMaxFd)
{
    otEXPECT(sBleSerialFd > 0);

    if (aReadFdSet != NULL)
    {
        FD_SET(sBleSerialFd, aReadFdSet);

        if (aMaxFd != NULL && *aMaxFd < sBleSerialFd)
        {
            *aMaxFd = sBleSerialFd;
        }
    }

    if ((sTxLength != 0) && (aWriteFdSet != NULL))
    {
        FD_SET(sBleSerialFd, aWriteFdSet);

        if (aErrorFdSet != NULL)
        {
            FD_SET(sBleSerialFd, aErrorFdSet);
        }

        if (aMaxFd != NULL && *aMaxFd < sBleSerialFd)
        {
            *aMaxFd = sBleSerialFd;
        }
    }

exit:
    return;
}

void platformBleHciProcess(const fd_set *aReadFdSet, const fd_set *aWriteFdSet, const fd_set *aErrorFdSet)
{
    const char *errStr = NULL;
    ssize_t     rval;

    otEXPECT(sBleSerialFd > 0);

    if (FD_ISSET(sBleSerialFd, aErrorFdSet))
    {
        perror("sBleSerialFd error");
        close(sBleSerialFd);
        exit(OT_EXIT_FAILURE);
    }

    if (FD_ISSET(sBleSerialFd, aReadFdSet))
    {
        otEXPECT_ACTION((rval = read(sBleSerialFd, sRxBuffer, sizeof(sRxBuffer))) > 0, errStr = "read");
        if (sBleSerialEnabled)
        {
            otCordioPlatHciReceived(sRxBuffer, (uint8_t)rval);
        }
    }

    if ((sTxLength > 0) && FD_ISSET(sBleSerialFd, aWriteFdSet))
    {
        otEXPECT_ACTION((rval = write(sBleSerialFd, sTxBuffer, sTxLength)) > 0, errStr = "write");

        sTxBuffer += (uint16_t)rval;
        sTxLength -= (uint16_t)rval;

        if (sTxLength == 0 && sBleSerialEnabled)
        {
            otCordioPlatHciSendDone();
        }
    }

exit:
    if (errStr != NULL)
    {
        perror(errStr);
        exit(OT_EXIT_FAILURE);
    }
}

void otCordioPlatHciEnableInterrupt(void)
{
}

void otCordioPlatHciDisableInterrupt(void)
{
}

/**
 * The BLE HCI Uart driver weak functions definition.
 *
 */
OT_TOOL_WEAK void otCordioPlatHciSendDone(void)
{
}

OT_TOOL_WEAK void otCordioPlatHciReceived(uint8_t *aBuf, uint8_t aBufLength)
{
    OT_UNUSED_VARIABLE(aBuf);
    OT_UNUSED_VARIABLE(aBufLength);
}
#endif // OPENTHREAD_ENABLE_BLE_HOST
