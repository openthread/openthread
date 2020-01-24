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
 *   This file includes definitions for the SPI interface to radio (RCP).
 */

#ifndef POSIX_APP_SPI_INTERFACE_HPP_
#define POSIX_APP_SPI_INTERFACE_HPP_

#include "openthread-posix-config.h"

#include "spinel_interface.hpp"
#include "ncp/hdlc.hpp"

#include <openthread-system.h>

#if OPENTHREAD_POSIX_RCP_SPI_ENABLE

#include "ncp/ncp_spi.hpp"

namespace ot {
namespace PosixApp {

/**
 * This class defines an SPI interface to the Radio Co-processor (RCP).
 *
 */
class SpiInterface
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in] aCallback     A reference to a `Callback` object.
     * @param[in] aFrameBuffer  A reference to a `RxFrameBuffer` object.
     *
     */
    SpiInterface(SpinelInterface::Callbacks &aCallback, SpinelInterface::RxFrameBuffer &aFrameBuffer);

    /**
     * This destructor deinitializes the object.
     *
     */
    ~SpiInterface(void);

    /**
     * This method initializes the interface to the Radio Co-processor (RCP).
     *
     * @note This method should be called before reading and sending spinel frames to the interface.
     *
     * @param[in]  aPlatformConfig  Platform configuration structure.
     *
     * @retval OT_ERROR_NONE          The interface is initialized successfully.
     * @retval OT_ERROR_ALREADY       The interface is already initialized.
     * @retval OT_ERROR_INVALID_ARGS  The UART device or executable cannot be found or failed to open/run.
     *
     */
    otError Init(const otPlatformConfig &aPlatformConfig);

    /**
     * This method deinitializes the interface to the RCP.
     *
     */
    void Deinit(void);

    /**
     * This method encodes and sends a spinel frame to Radio Co-processor (RCP) over the socket.
     *
     * @param[in] aFrame     A pointer to buffer containing the spinel frame to send.
     * @param[in] aLength    The length (number of bytes) in the frame.
     *
     * @retval OT_ERROR_NONE     Successfully encoded and sent the spinel frame.
     * @retval OT_ERROR_BUSY     Failed due to another operation is on going.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space available to encode the frame.
     * @retval OT_ERROR_FAILED   Failed to call the SPI driver to send the frame.
     *
     */
    otError SendFrame(const uint8_t *aFrame, uint16_t aLength);

    /**
     * This method waits for receiving part or all of spinel frame within specified interval.
     *
     * @param[in]  aTimeout  A reference to the timeout.
     *
     * @retval OT_ERROR_NONE             Part or all of spinel frame is received.
     * @retval OT_ERROR_RESPONSE_TIMEOUT No spinel frame is received within @p aTimeout.
     *
     */
    otError WaitForFrame(const struct timeval &aTimeout);

    /**
     * This method updates the file descriptor sets with file descriptors used by the radio driver.
     *
     * @param[inout]  aReadFdSet   A reference to the read file descriptors.
     * @param[inout]  aWriteFdSet  A reference to the write file descriptors.
     * @param[inout]  aMaxFd       A reference to the max file descriptor.
     * @param[inout]  aTimeout     A reference to the timeout.
     *
     */
    void UpdateFdSet(fd_set &aReadFdSet, fd_set &aWriteFdSet, int &aMaxFd, struct timeval &aTimeout);

    /**
     * This method performs radio driver processing.
     *
     * @param[in]   aReadFdSet      A reference to the read file descriptors.
     * @param[in]   aWriteFdSet     A reference to the write file descriptors.
     *
     */
    void Process(const fd_set &aReadFdSet, const fd_set &aWriteFdSet);

private:
    int     SetupGpioHandle(int aFd, uint8_t aLine, uint32_t aHandleFlags, const char *aLabel);
    int     SetupGpioEvent(int aFd, uint8_t aLine, uint32_t aHandleFlags, uint32_t aEventFlags, const char *aLabel);
    void    SetGpioValue(int aFd, uint8_t aValue);
    uint8_t GetGpioValue(int aFd);

    void InitResetPin(const char *aCharDev, uint8_t aLine);
    void InitIntPin(const char *aCharDev, uint8_t aLine);
    void InitSpiDev(const char *aPath, uint8_t aMode, uint32_t aSpeed);
    void TrigerReset(void);

    uint8_t *GetRealRxFrameStart(void);
    otError  DoSpiTransfer(uint32_t aLength);
    otError  PushPullSpi(void);

    bool CheckInterrupt(void);
    void HandleReceivedFrame(Ncp::SpiFrame &aSpiFrame);
    void LogStats(void);
    void LogError(const char * aString);
    void LogBuffer(const char *aDesc, const uint8_t *aBuffer, uint16_t aLength, bool aForce);

    enum
    {
        kSpiModeMax              = 3,
        kSpiAlignAllowanceMax    = 16,
        kSpiFrameHeaderSize      = 5,
        kSpiBitsPerWord          = 8,
        kSpiTxRefuseWarnCount    = 30,
        kSpiTxRefuseExitCount    = 100,
        kImmediateRetryCount     = 5,
        kFastRetryCount          = 15,
        kDebugBytesPerLine       = 16,
        kGpioIntAssertState      = 0,
        kGpioResetAssertState    = 0,
    };

    enum
    {
        kMsecPerSec              = 1000,
        kUsecPerMsec             = 1000,
        kSpiPollPeriodUs         = kMsecPerSec * kUsecPerMsec / 30,
        kSecPerDay               = 60 * 60 * 24,
        kResetHoldOnUsec         = 10 * kUsecPerMsec,
        kImmediateRetryTimeoutUs = 1 * kUsecPerMsec,
        kFastRetryTimeoutUs      = 10 * kUsecPerMsec,
        kSlowRetryTimeoutUs      = 33 * kUsecPerMsec,
    };

    enum
    {
        kMaxFrameSize = SpinelInterface::kMaxFrameSize,
    };

    SpinelInterface::Callbacks &    mCallbacks;
    SpinelInterface::RxFrameBuffer &mRxFrameBuffer;

    int mSpiDevFd;
    int mResetGpioValueFd;
    int mIntGpioValueFd;

    uint8_t  mSpiMode;
    uint8_t  mSpiAlignAllowance;
    uint16_t mSpiCsDelayUs;
    uint16_t mSpiSmallPacketSize;
    uint32_t mSpiSpeedHz;

    uint64_t mSlaveResetCount;
    uint64_t mSpiFrameCount;
    uint64_t mSpiValidFrameCount;
    uint64_t mSpiGarbageFrameCount;
    uint64_t mSpiDuplexFrameCount;
    uint64_t mSpiUnresponsiveFrameCount;
    uint64_t mSpiRxFrameCount;
    uint64_t mSpiRxFrameByteCount;
    uint64_t mSpiTxFrameCount;
    uint64_t mSpiTxFrameByteCount;

    uint8_t  mSpiRxFrameBuffer[kMaxFrameSize + kSpiAlignAllowanceMax];

    bool     mSpiTxIsReady;
    uint16_t mSpiTxRefusedCount;
    uint16_t mSpiTxPayloadSize;
    uint8_t  mSpiTxFrameBuffer[kMaxFrameSize + kSpiAlignAllowanceMax];

    bool     mDidPrintRateLimitLog;
    uint16_t mSpiSlaveDataLen;
};

} // namespace PosixApp
} // namespace ot

#endif // OPENTHREAD_POSIX_RCP_SPI_ENABLE
#endif // POSIX_APP_SPI_INTERFACE_HPP_
