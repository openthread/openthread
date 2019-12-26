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

/**
 * @file
 *   This file includes the implementation for the SPI interface to radio (RCP).
 */

#include "spi_interface.hpp"
#include "openthread-core-config.h"
#include "platform-posix.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/ucontext.h>

#if OPENTHREAD_POSIX_NCP_SPI_ENABLE
#include <linux/gpio.h>
#include <linux/ioctl.h>
#include <linux/spi/spidev.h>

namespace ot {
namespace PosixApp {

SpiInterface::SpiInterface(Callbacks &aCallbacks)
    : SpinelInterface()
    , mCallbacks(aCallbacks)
    , mSpiDevFd(-1)
    , mResetGpioValueFd(-1)
    , mIntGpioValueFd(-1)
    , mSlaveResetCount(0)
    , mSpiFrameCount(0)
    , mSpiValidFrameCount(0)
    , mSpiGarbageFrameCount(0)
    , mSpiDuplexFrameCount(0)
    , mSpiUnresponsiveFrameCount(0)
    , mSpiRxFrameCount(0)
    , mSpiRxFrameByteCount(0)
    , mSpiTxFrameCount(0)
    , mSpiTxFrameByteCount(0)
    , mSpiTxIsReady(false)
    , mSpiTxRefusedCount(0)
    , mSpiTxPayloadSize(0)
    , mDidPrintRateLimitLog(false)
    , mSpiSlaveDataLen(0)
{
}

otError SpiInterface::Init(const otPlatformConfig &aPlatformConfig)
{
    VerifyOrDie(aPlatformConfig.mSpiAlignAllowance <= kSpiAlignAllowanceMax, OT_EXIT_FAILURE);

    mSpiCsDelayUs       = aPlatformConfig.mSpiCsDelay;
    mSpiSmallPacketSize = aPlatformConfig.mSpiSmallPacketSize;
    mSpiAlignAllowance  = aPlatformConfig.mSpiAlignAllowance;

    ResetPinInit(aPlatformConfig.mSpiGpioResetDevice, aPlatformConfig.mSpiGpioResetLine);
    IntPinInit(aPlatformConfig.mSpiGpioIntDevice, aPlatformConfig.mSpiGpioIntLine);
    SpiDevInit(aPlatformConfig.mRadioFile, aPlatformConfig.mSpiMode, aPlatformConfig.mSpiSpeed);

    // Reset RCP chip.
    TrigerReset();

    // Waiting for the RCP chip starts up.
    usleep((useconds_t)aPlatformConfig.mSpiResetDelay * kUsecPerMsec);

    return OT_ERROR_NONE;
}

SpiInterface::~SpiInterface(void)
{
    Deinit();
}

void SpiInterface::Deinit(void)
{
    if (mSpiDevFd >= 0)
    {
        close(mSpiDevFd);
    }

    if (mResetGpioValueFd >= 0)
    {
        close(mResetGpioValueFd);
    }

    if (mIntGpioValueFd >= 0)
    {
        close(mIntGpioValueFd);
    }

    mSpiDevFd         = -1;
    mResetGpioValueFd = -1;
    mIntGpioValueFd   = -1;

    return;
}

int SpiInterface::SetupGpioHandle(int aFd, uint8_t aLine, uint32_t aHandleFlags, const char *aLabel)
{
    struct gpiohandle_request req;
    int                       ret;

    req.flags             = aHandleFlags;
    req.lines             = 1;
    req.lineoffsets[0]    = aLine;
    req.default_values[0] = 1;
    snprintf(req.consumer_label, sizeof(req.consumer_label), "%s", aLabel);

    VerifyOrExit((ret = ioctl(aFd, GPIO_GET_LINEHANDLE_IOCTL, &req)) != -1,
                 LogError("ioctl(GPIO_GET_LINEHANDLE_IOCTL)"));

exit:
    return ret >= 0 ? req.fd : -1;
}

int SpiInterface::SetupGpioEvent(int         aFd,
                                 uint8_t     aLine,
                                 uint32_t    aHandleFlags,
                                 uint32_t    aEventFlags,
                                 const char *aLabel)
{
    struct gpioevent_request req;
    int                      ret;

    req.lineoffset  = aLine;
    req.handleflags = aHandleFlags;
    req.eventflags  = aEventFlags;
    snprintf(req.consumer_label, sizeof(req.consumer_label), "%s", aLabel);

    VerifyOrExit((ret = ioctl(aFd, GPIO_GET_LINEEVENT_IOCTL, &req)) != -1, LogError("ioctl(GPIO_GET_LINEEVENT_IOCTL)"));

exit:
    return ret >= 0 ? req.fd : -1;
}

void SpiInterface::SetGpioValue(int aFd, uint8_t aValue)
{
    struct gpiohandle_data data;

    data.values[0] = aValue;
    VerifyOrDie(ioctl(aFd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data) != -1, OT_EXIT_ERROR_ERRNO);
}

uint8_t SpiInterface::GetGpioValue(int aFd)
{
    struct gpiohandle_data data;

    VerifyOrDie(ioctl(aFd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data) != -1, OT_EXIT_ERROR_ERRNO);
    return data.values[0];
}

void SpiInterface::ResetPinInit(const char *aCharDev, uint8_t aLine)
{
    int  fd      = -1;
    char label[] = "SOC_THREAD_RESET";

    VerifyOrExit((aCharDev != NULL) && (aLine < GPIOHANDLES_MAX));

    otLogDebgPlat("Reset Pin: charDev=%s, line=%d", aCharDev, aLine);

    VerifyOrExit((fd = open(aCharDev, O_RDWR)) != -1);
    VerifyOrExit((mResetGpioValueFd = SetupGpioHandle(fd, aLine, GPIOHANDLE_REQUEST_OUTPUT, label)) >= 0);

exit:
    if (fd >= 0)
    {
        close(fd);
    }

    if (mResetGpioValueFd == -1)
    {
        DieNowWithMessage("Initialize the Reset Pin failed !", OT_EXIT_INVALID_ARGUMENTS);
    }
}

void SpiInterface::IntPinInit(const char *aCharDev, uint8_t aLine)
{
    int  fd      = -1;
    char label[] = "THREAD_SOC_INT";

    VerifyOrExit((aCharDev != NULL) && (aLine < GPIOHANDLES_MAX));

    otLogDebgPlat("Interrupt Pin: charDev=%s, line=%d", aCharDev, aLine);

    VerifyOrExit((fd = open(aCharDev, O_RDWR)) != -1);

    mIntGpioValueFd = SetupGpioEvent(fd, aLine, GPIOHANDLE_REQUEST_INPUT, GPIOEVENT_REQUEST_FALLING_EDGE, label);

exit:
    if (fd >= 0)
    {
        close(fd);
    }

    if (mIntGpioValueFd == -1)
    {
        otLogNotePlat("SPI interface falls back to polling mode.");
    }

    return;
}

void SpiInterface::SpiDevInit(const char *aPath, uint8_t aMode, uint32_t aSpeed)
{
    int           fd       = -1;
    const uint8_t wordBits = 8;

    VerifyOrDie((aPath != NULL) && (aMode <= kSpiModeMax), OT_EXIT_FAILURE);

    otLogDebgPlat("SPI device: path=%s, mode=%d, speed=%d", aPath, aMode, aSpeed);

    VerifyOrExit((fd = open(aPath, O_RDWR | O_CLOEXEC)) != -1, LogError("open_spi_dev_path"));
    VerifyOrExit(ioctl(fd, SPI_IOC_WR_MODE, &aMode) != -1, LogError("ioctl(SPI_IOC_WR_MODE)"));
    VerifyOrExit(ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &aSpeed) != -1, LogError("ioctl(SPI_IOC_WR_MAX_SPEED_HZ)"));
    VerifyOrExit(ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &wordBits) != -1, LogError("ioctl(SPI_IOC_WR_BITS_PER_WORD)"));
    VerifyOrExit(flock(fd, LOCK_EX | LOCK_NB) != -1, LogError("flock"));

    mSpiDevFd   = fd;
    mSpiMode    = aMode;
    mSpiSpeedHz = aSpeed;
    fd          = -1;

exit:
    if (fd >= 0)
    {
        close(fd);
    }

    VerifyOrDie(mSpiDevFd >= 0, OT_EXIT_FAILURE);
}

void SpiInterface::TrigerReset(void)
{
    // Set Reset pin to low level.
    SetGpioValue(mResetGpioValueFd, 0);

    usleep(10 * kUsecPerMsec);

    // Set Reset pin to high level.
    SetGpioValue(mResetGpioValueFd, 1);

    otLogNotePlat("Triggered hardware reset");
}

uint8_t *SpiInterface::GetRealRxFrameStart(void)
{
    uint8_t *ret = mSpiRxFrameBuffer;
    int      i   = 0;

    for (i = 0; i < mSpiAlignAllowance; i++)
    {
        if (ret[0] != 0xFF)
        {
            break;
        }
        ret++;
    }

    return ret;
}

otError SpiInterface::DoSpiXfer(int aLength)
{
    int                     ret;
    struct spi_ioc_transfer xfer[2];

    memset(&xfer[0], 0, sizeof(xfer));

    // This part is the delay between C̅S̅ being asserted and the SPI clock
    // starting. This is not supported by all Linux SPI drivers.
    xfer[0].tx_buf        = 0;
    xfer[0].rx_buf        = 0;
    xfer[0].len           = 0;
    xfer[0].speed_hz      = (uint32_t)mSpiSpeedHz;
    xfer[0].delay_usecs   = (uint16_t)mSpiCsDelayUs;
    xfer[0].bits_per_word = 8;
    xfer[0].cs_change     = false;

    // This part is the actual SPI transfer.
    xfer[1].tx_buf        = (unsigned long)mSpiTxFrameBuffer;
    xfer[1].rx_buf        = (unsigned long)mSpiRxFrameBuffer;
    xfer[1].len           = (uint32_t)(aLength + kSpiFrameHeaderSize + mSpiAlignAllowance);
    xfer[1].speed_hz      = (uint32_t)mSpiSpeedHz;
    xfer[1].delay_usecs   = 0;
    xfer[1].bits_per_word = 8;
    xfer[1].cs_change     = false;

    if (mSpiCsDelayUs > 0)
    {
        // A C̅S̅ delay has been specified. Start transactions with both parts.
        ret = ioctl(mSpiDevFd, SPI_IOC_MESSAGE(2), &xfer[0]);
    }
    else
    {
        // No C̅S̅ delay has been specified, so we skip the first part because it causes some SPI drivers to croak.
        ret = ioctl(mSpiDevFd, SPI_IOC_MESSAGE(1), &xfer[1]);
    }

    if (ret != -1)
    {
        LogBuffer("SPI-TX", mSpiTxFrameBuffer, (int)xfer[1].len, false);
        LogBuffer("SPI-RX", mSpiRxFrameBuffer, (int)xfer[1].len, false);

        mSpiFrameCount++;
    }

    return (ret < 0) ? OT_ERROR_FAILED : OT_ERROR_NONE;
}

otError SpiInterface::PushPullSpi(void)
{
    otError       error;
    uint8_t *     spiRxFrameBuffer    = NULL;
    uint16_t      spiXferBytes        = 0;
    uint8_t       successfulExchanges = 0;
    uint8_t       slaveHeader;
    uint16_t      slaveAcceptLen;
    Ncp::SpiFrame txFrame(mSpiTxFrameBuffer);

    if (mSpiValidFrameCount == 0)
    {
        // Set the reset flag to indicate to our slave that we are coming up from scratch.
        txFrame.SetHeaderFlagByte(true);
    }
    else
    {
        txFrame.SetHeaderFlagByte(false);
    }

    // Zero out our rx_accept and our data_len for now.
    txFrame.SetHeaderAcceptLen(0);
    txFrame.SetHeaderDataLen(0);

    // Sanity check.
    if (mSpiSlaveDataLen > kMaxFrameSize)
    {
        mSpiSlaveDataLen = 0;
    }

    if (mSpiTxIsReady)
    {
        // Go ahead and try to immediately send a frame if we have it queued up.
        txFrame.SetHeaderDataLen(mSpiTxPayloadSize);

        if (mSpiTxPayloadSize > spiXferBytes)
        {
            spiXferBytes = mSpiTxPayloadSize;
        }
    }

    if (mSpiSlaveDataLen != 0)
    {
        // In a previous transaction the slave indicated it had something to send us. Make sure our transaction
        // is large enough to handle it.
        if (mSpiSlaveDataLen > spiXferBytes)
        {
            spiXferBytes = mSpiSlaveDataLen;
        }
    }
    else
    {
        // Set up a minimum transfer size to allow small frames the slave wants to send us to be handled in a
        // single transaction.
        if (spiXferBytes < mSpiSmallPacketSize)
        {
            spiXferBytes = mSpiSmallPacketSize;
        }
    }

    txFrame.SetHeaderAcceptLen(spiXferBytes);

    // Perform the SPI transaction.
    error = DoSpiXfer(spiXferBytes);

    if (error != OT_ERROR_NONE)
    {
        otLogCritPlat("PushPullSpi:DoSpiXfer: errno=%d (%s)", errno, strerror(errno));

        // Print out a helpful error message for a common error.
        if ((mSpiCsDelayUs != 0) && (errno == EINVAL))
        {
            otLogWarnPlat("SPI ioctl failed with EINVAL. Try adding `--spi-cs-delay=0` to command line arguments.");
        }

        LogStats();
        DieNow(OT_EXIT_FAILURE);
    }

    // Account for misalignment (0xFF bytes at the start)
    spiRxFrameBuffer = GetRealRxFrameStart();

    {
        Ncp::SpiFrame rxFrame(spiRxFrameBuffer);

        otLogDebgPlat("spi_xfer TX: H:%02X ACCEPT:%d DATA:%d", txFrame.GetHeaderFlagByte(),
                      txFrame.GetHeaderAcceptLen(), txFrame.GetHeaderDataLen());
        otLogDebgPlat("spi_xfer RX: H:%02X ACCEPT:%d DATA:%d", rxFrame.GetHeaderFlagByte(),
                      rxFrame.GetHeaderAcceptLen(), rxFrame.GetHeaderDataLen());

        slaveHeader = rxFrame.GetHeaderFlagByte();
        if ((slaveHeader == 0xFF) || (slaveHeader == 0x00))
        {
            if ((slaveHeader == spiRxFrameBuffer[1]) && (slaveHeader == spiRxFrameBuffer[2]) &&
                (slaveHeader == spiRxFrameBuffer[3]) && (slaveHeader == spiRxFrameBuffer[4]))
            {
                // Device is off or in a bad state. In some cases may be induced by flow control.
                if (mSpiSlaveDataLen == 0)
                {
                    otLogDebgPlat("Slave did not respond to frame. (Header was all 0x%02X)", slaveHeader);
                }
                else
                {
                    otLogWarnPlat("Slave did not respond to frame. (Header was all 0x%02X)", slaveHeader);
                }

                mSpiUnresponsiveFrameCount++;
            }
            else
            {
                // Header is full of garbage
                mSpiGarbageFrameCount++;

                otLogWarnPlat("Garbage in header : %02X %02X %02X %02X %02X", spiRxFrameBuffer[0], spiRxFrameBuffer[1],
                              spiRxFrameBuffer[2], spiRxFrameBuffer[3], spiRxFrameBuffer[4]);
                LogBuffer("SPI-TX", mSpiTxFrameBuffer, (int)spiXferBytes + kSpiFrameHeaderSize + mSpiAlignAllowance,
                          true);
                LogBuffer("SPI-RX", mSpiRxFrameBuffer, (int)spiXferBytes + kSpiFrameHeaderSize + mSpiAlignAllowance,
                          true);
            }

            mSpiTxRefusedCount++;
            ExitNow();
        }

        slaveAcceptLen   = rxFrame.GetHeaderAcceptLen();
        mSpiSlaveDataLen = rxFrame.GetHeaderDataLen();

        if (!rxFrame.IsValid() || (slaveAcceptLen > kMaxFrameSize) || (mSpiSlaveDataLen > kMaxFrameSize))
        {
            mSpiGarbageFrameCount++;
            mSpiTxRefusedCount++;
            mSpiSlaveDataLen = 0;

            otLogWarnPlat("Garbage in header : %02X %02X %02X %02X %02X", spiRxFrameBuffer[0], spiRxFrameBuffer[1],
                          spiRxFrameBuffer[2], spiRxFrameBuffer[3], spiRxFrameBuffer[4]);
            LogBuffer("SPI-TX", mSpiTxFrameBuffer, (int)spiXferBytes + kSpiFrameHeaderSize + mSpiAlignAllowance, true);
            LogBuffer("SPI-RX", mSpiRxFrameBuffer, (int)spiXferBytes + kSpiFrameHeaderSize + mSpiAlignAllowance, true);

            ExitNow();
        }

        mSpiValidFrameCount++;

        if (rxFrame.IsResetFlagSet())
        {
            mSlaveResetCount++;

            otLogNotePlat("Slave did reset (%llu resets so far)", (unsigned long long)mSlaveResetCount);
            LogStats();
        }

        // Handle received packet, if any.
        if ((mSpiSlaveDataLen != 0) && (mSpiSlaveDataLen <= txFrame.GetHeaderAcceptLen()))
        {
            mSpiRxFrameByteCount += mSpiSlaveDataLen;
            mSpiSlaveDataLen = 0;
            mSpiRxFrameCount++;
            successfulExchanges++;

            HandleReceivedFrame(rxFrame);
        }
    }

    // Handle transmitted packet, if any.
    if (mSpiTxIsReady && (mSpiTxPayloadSize == txFrame.GetHeaderDataLen()))
    {
        if (txFrame.GetHeaderDataLen() <= slaveAcceptLen)
        {
            // Our outbound packet has been successfully transmitted. Clear mSpiTxPayloadSize and mSpiTxIsReady so
            // that uplayer can pull another packet for us to send.
            successfulExchanges++;

            mSpiTxFrameCount++;
            mSpiTxFrameByteCount += mSpiTxPayloadSize;

            mSpiTxIsReady      = false;
            mSpiTxPayloadSize  = 0;
            mSpiTxRefusedCount = 0;
        }
        else
        {
            // The slave wasn't ready for what we had to send them. Incrementing this counter will turn on rate
            // limiting so that we don't waste a ton of CPU bombarding them with useless SPI transfers.
            mSpiTxRefusedCount++;
        }
    }

    if (!mSpiTxIsReady)
    {
        mSpiTxRefusedCount = 0;
    }

    if (successfulExchanges == 2)
    {
        mSpiDuplexFrameCount++;
    }

exit:
    return error;
}

bool SpiInterface::CheckInterrupt(void)
{
    return (mIntGpioValueFd >= 0) ? (GetGpioValue(mIntGpioValueFd) == kGpioIntAssertState) : true;
}

void SpiInterface::UpdateFdSet(fd_set &aReadFdSet, fd_set &aWriteFdSet, int &aMaxFd, struct timeval &aTimeout)
{
    int timeout = kMsecPerSec * 60 * 60 * 24; // 24 hours

    OT_UNUSED_VARIABLE(aWriteFdSet);

    if (mSpiTxIsReady)
    {
        // We have data to send to the slave.
        timeout = 0;
    }

    if (mIntGpioValueFd >= 0)
    {
        if (aMaxFd < mIntGpioValueFd)
        {
            aMaxFd = mIntGpioValueFd;
        }

        if (CheckInterrupt())
        {
            // Interrupt pin is asserted, set the timeout to be 0.
            timeout = 0;
            otLogDebgPlat("Interrupt.");
        }
        else
        {
            // The interrupt pin was not asserted, so we wait for the interrupt pin to be asserted by adding it to the
            // read set.
            FD_SET(mIntGpioValueFd, &aReadFdSet);
        }
    }
    else if (timeout > kSpiPollPeriodMs)
    {
        // In this case we don't have an interrupt, so we revert to SPI polling.
        timeout = kSpiPollPeriodMs;
    }

    if (mSpiTxRefusedCount)
    {
        int minTimeout = 0;

        // We are being rate-limited by the slave. This is fairly normal behavior. Based on number of times slave has
        // refused a transmission, we apply a minimum timeout.
        if (mSpiTxRefusedCount < kImmediateRetryCount)
        {
            minTimeout = kImmediateRetryTimeoutMs;
        }
        else if (mSpiTxRefusedCount < kFastRetryCount)
        {
            minTimeout = kFastRetryTimeoutMs;
        }
        else
        {
            minTimeout = kSlowRetryTimeoutMs;
        }

        if (timeout < minTimeout)
        {
            timeout = minTimeout;
        }

        if (mSpiTxIsReady && !mDidPrintRateLimitLog && (mSpiTxRefusedCount > 1))
        {
            // To avoid printing out this message over and over, we only print it out once the refused count is at two
            // or higher when we actually have something to send the slave. And then, we only print it once.
            otLogInfoPlat("Slave is rate limiting transactions");

            mDidPrintRateLimitLog = true;
        }

        if (mSpiTxRefusedCount == 30)
        {
            // Ua-oh. The slave hasn't given us a chance to send it anything for over thirty frames. If this ever
            // happens, print out a warning to the logs.
            otLogNotePlat("Slave seems stuck.");
        }
        else if (mSpiTxRefusedCount == 100)
        {
            // Double ua-oh. The slave hasn't given us a chance to send it anything for over a hundred frames.
            // This almost certainly means that the slave has locked up or gotten into an unrecoverable state.
            DieNowWithMessage("Slave seems REALLY stuck.", OT_EXIT_FAILURE);
        }
    }
    else
    {
        mDidPrintRateLimitLog = false;
    }

    if (static_cast<uint64_t>(timeout * US_PER_MS) <
        static_cast<uint64_t>(aTimeout.tv_sec * US_PER_S + aTimeout.tv_usec))
    {
        aTimeout.tv_sec  = static_cast<time_t>(timeout / kMsecPerSec);
        aTimeout.tv_usec = static_cast<suseconds_t>((timeout % kMsecPerSec) * kUsecPerMsec);
    }
}

void SpiInterface::Process(const fd_set &aReadFdSet, const fd_set &aWriteFdSet)
{
    OT_UNUSED_VARIABLE(aReadFdSet);
    OT_UNUSED_VARIABLE(aWriteFdSet);

    if (FD_ISSET(mIntGpioValueFd, &aReadFdSet))
    {
        struct gpioevent_data event;

        otLogDebgPlat("Interrupt.");

        // Read event data to clear interrupt.
        VerifyOrDie(read(mIntGpioValueFd, &event, sizeof(event)) != -1, OT_EXIT_FAILURE);
    }

    // Service the SPI port if we can receive a packet or we have a packet to be sent.
    if (mSpiTxIsReady || CheckInterrupt())
    {
        // We guard this with the above check because we don't want to overwrite any previously received frames.
        PushPullSpi();
    }
}

otError SpiInterface::WaitForFrame(struct timeval &aTimeout)
{
    otError  error   = OT_ERROR_NONE;
    uint32_t timeout = kMsecPerSec * 60 * 60 * 24; // 24 hours
    fd_set   readFdSet;
    int      ret;

    FD_ZERO(&readFdSet);

    if (mIntGpioValueFd >= 0)
    {
        if (CheckInterrupt())
        {
            // Interrupt pin is asserted, set the timeout to be 0.
            timeout = 0;
            otLogDebgPlat("Interrupt.");
        }
        else
        {
            // The interrupt pin was not asserted, so we wait for the interrupt pin to be asserted by adding it to the
            // read set.
            FD_SET(mIntGpioValueFd, &readFdSet);
        }
    }
    else if (timeout > kSpiPollPeriodMs)
    {
        // In this case we don't have an interrupt, so we revert to SPI polling.
        timeout = kSpiPollPeriodMs;
    }

    if (static_cast<uint64_t>(timeout * US_PER_MS) <
        static_cast<uint64_t>(aTimeout.tv_sec * US_PER_S + aTimeout.tv_usec))
    {
        aTimeout.tv_sec  = static_cast<time_t>(timeout / kMsecPerSec);
        aTimeout.tv_usec = static_cast<suseconds_t>((timeout % kMsecPerSec) * kUsecPerMsec);
    }

    ret = select(mIntGpioValueFd + 1, &readFdSet, NULL, NULL, &aTimeout);
    if ((ret < 0) && (errno != EINTR))
    {
        DieNow(OT_EXIT_ERROR_ERRNO);
    }

    if (FD_ISSET(mIntGpioValueFd, &readFdSet))
    {
        struct gpioevent_data event;

        otLogDebgPlat("Interrupt.");

        // Read event data to clear interrupt.
        VerifyOrDie(read(mIntGpioValueFd, &event, sizeof(event)) != -1, OT_EXIT_FAILURE);
    }

    // If we can receive a packet.
    if (CheckInterrupt())
    {
        PushPullSpi();
    }

    return error;
}

otError SpiInterface::SendFrame(const uint8_t *aFrame, uint16_t aLength)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aLength < (kMaxFrameSize - kSpiFrameHeaderSize), error = OT_ERROR_NO_BUFS);
    VerifyOrExit(!mSpiTxIsReady, error = OT_ERROR_BUSY);

    memcpy(&mSpiTxFrameBuffer[kSpiFrameHeaderSize], aFrame, aLength);

    mSpiTxIsReady     = true;
    mSpiTxPayloadSize = aLength;

    PushPullSpi();

exit:
    return error;
}

void SpiInterface::HandleReceivedFrame(Ncp::SpiFrame &aSpiFrame)
{
    const uint8_t *spinelFrame = aSpiFrame.GetData();

    for (uint16_t i = 0; i < aSpiFrame.GetHeaderDataLen(); i++)
    {
        if (GetRxFrameBuffer().WriteByte(spinelFrame[i]) != OT_ERROR_NONE)
        {
            GetRxFrameBuffer().DiscardFrame();
            otLogNotePlat("No enough memory buffers, drop packet");
            ExitNow();
        }
    }

    mCallbacks.HandleReceivedFrame();

exit:
    return;
}

void SpiInterface::LogError(const char *aString)
{
    fprintf(stderr, "%s: %s", aString, strerror(errno));
    otLogCritPlat("%s: %s", aString, strerror(errno));
}

void SpiInterface::LogStats(void)
{
    otLogInfoPlat("INFO: mSlaveResetCount=%llu", (unsigned long long)mSlaveResetCount);
    otLogInfoPlat("INFO: mSpiFrameCount=%llu", (unsigned long long)mSpiFrameCount);
    otLogInfoPlat("INFO: mSpiValidFrameCount=%llu", (unsigned long long)mSpiValidFrameCount);
    otLogInfoPlat("INFO: mSpiDuplexFrameCount=%llu", (unsigned long long)mSpiDuplexFrameCount);
    otLogInfoPlat("INFO: mSpiUnresponsiveFrameCount=%llu", (unsigned long long)mSpiUnresponsiveFrameCount);
    otLogInfoPlat("INFO: mSpiGarbageFrameCount=%llu", (unsigned long long)mSpiGarbageFrameCount);
    otLogInfoPlat("INFO: mSpiRxFrameCount=%llu", (unsigned long long)mSpiRxFrameCount);
    otLogInfoPlat("INFO: mSpiRxFrameByteCount=%llu", (unsigned long long)mSpiRxFrameByteCount);
    otLogInfoPlat("INFO: mSpiTxFrameCount=%llu", (unsigned long long)mSpiTxFrameCount);
    otLogInfoPlat("INFO: mSpiTxFrameByteCount=%llu", (unsigned long long)mSpiTxFrameByteCount);
}

#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_WARN
void SpiInterface::LogBuffer(const char *aDesc, const uint8_t *aBuffer, int aLength, bool aForce)
{
    int i = 0;

    while (i < aLength)
    {
        int  j;
        char dumpString[kDebugBytesPerLine * 3 + 1];

        for (j = 0; i < aLength && j < kDebugBytesPerLine; i++, j++)
        {
            sprintf(dumpString + j * 3, "%02X ", aBuffer[i]);
        }

        if (aForce)
        {
            otLogWarnPlat("%s: %s%s", aDesc, dumpString, (i < aLength) ? " ..." : "");
        }
        else
        {
            otLogDebgPlat("%s: %s%s", aDesc, dumpString, (i < aLength) ? " ..." : "");
        }
    }
}
#else
void SpiInterface::LogBuffer(const char *aDesc, const uint8_t *aBuffer, int aLength, bool aForce)
{
    OT_UNUSED_VARIABLE(aDesc);
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aLength);
    OT_UNUSED_VARIABLE(aForce);
}
#endif
} // namespace PosixApp
} // namespace ot

#endif // OPENTHREAD_POSIX_NCP_SPI_ENABLE
