/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file provides a reference implementation of the OpenThread vendor
 *   interface to RCP (Radio Co-Processor) over a POSIX file descriptor (e.g.,
 *   a UART or SPI device node specified in the radio URL).
 *
 *   It implements:
 *     - Init / Deinit lifecycle
 *     - Frame send / receive via read()/write() on a file descriptor
 *     - select()-based mainloop integration (UpdateFdSet / Process)
 *     - WaitForFrame with microsecond timeout
 *     - HardwareReset via GPIO or platform-specific call
 *     - RCP interface metrics tracking
 */

#include "openthread-posix-config.h"

#if OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include "vendor_interface.hpp"
#include "common/code_utils.hpp"
#include "common/logging.hpp"
#include "common/new.hpp"

namespace ot {
namespace Posix {
using ot::Spinel::SpinelInterface;

// Default bus speed in bits per second (1 Mbps).
static constexpr uint32_t kDefaultBusSpeedHz = 1000000;

// Maximum raw frame size this interface can handle.
static constexpr uint16_t kMaxFrameSize = 2048;

/**
 * Defines the vendor implementation object.
 *
 * Manages a POSIX file descriptor (opened from the radio URL path) and
 * provides frame-level send/receive plus mainloop fd-set integration.
 */
class VendorInterfaceImpl
{
public:
    explicit VendorInterfaceImpl(const Url::Url &aRadioUrl)
        : mRadioUrl(aRadioUrl)
        , mFd(-1)
        , mReceiveFrameCallback(nullptr)
        , mReceiveFrameCallbackContext(nullptr)
        , mReceiveFrameBuffer(nullptr)
        , mBusSpeed(kDefaultBusSpeedHz)
    {
        memset(&mMetrics, 0, sizeof(mMetrics));
        memset(mReceiveBuffer, 0, sizeof(mReceiveBuffer));
    }

    /**
     * Opens and configures the interface described by the radio URL.
     *
     * @param[in] aCallback         Called when a complete frame is received.
     * @param[in] aCallbackContext  Opaque context pointer passed to @p aCallback.
     * @param[in] aFrameBuffer      Buffer used to assemble incoming frames.
     *
     * @retval OT_ERROR_NONE   Successfully initialized.
     * @retval OT_ERROR_FAILED Failed to open or configure the device.
     */
    otError Init(SpinelInterface::ReceiveFrameCallback aCallback,
                 void *                                aCallbackContext,
                 SpinelInterface::RxFrameBuffer &      aFrameBuffer)
    {
        otError     error = OT_ERROR_NONE;
        const char *path  = mRadioUrl.GetPath();

        VerifyOrExit(path != nullptr, error = OT_ERROR_INVALID_ARGS);

        // Open the device node (e.g., /dev/ttyUSB0).
        mFd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
        VerifyOrExit(mFd >= 0, error = OT_ERROR_FAILED);

        // Configure as raw UART if it is a TTY.
        if (isatty(mFd))
        {
            struct termios tty;

            VerifyOrExit(tcgetattr(mFd, &tty) == 0, error = OT_ERROR_FAILED);

            cfmakeraw(&tty);
            cfsetispeed(&tty, B115200);
            cfsetospeed(&tty, B115200);

            tty.c_cc[VMIN]  = 0; // Non-blocking read
            tty.c_cc[VTIME] = 0;

            VerifyOrExit(tcsetattr(mFd, TCSANOW, &tty) == 0, error = OT_ERROR_FAILED);
        }

        mReceiveFrameCallback        = aCallback;
        mReceiveFrameCallbackContext = aCallbackContext;
        mReceiveFrameBuffer          = &aFrameBuffer;

        otLogInfoPlat("[VendorInterface] Initialized on %s", path);

    exit:
        if (error != OT_ERROR_NONE && mFd >= 0)
        {
            close(mFd);
            mFd = -1;
        }

        return error;
    }

    /**
     * Closes the file descriptor and resets all state.
     */
    void Deinit(void)
    {
        if (mFd >= 0)
        {
            close(mFd);
            mFd = -1;
            otLogInfoPlat("[VendorInterface] Deinitialized");
        }

        mReceiveFrameCallback        = nullptr;
        mReceiveFrameCallbackContext = nullptr;
        mReceiveFrameBuffer          = nullptr;
    }

    /**
     * Returns the configured bus speed in Hz.
     */
    uint32_t GetBusSpeed(void) const { return mBusSpeed; }

    /**
     * Performs a hardware reset of the RCP by toggling DTR on the UART,
     * which is conventionally wired to the RCP reset line.
     *
     * @retval OT_ERROR_NONE    Reset signal sent successfully.
     * @retval OT_ERROR_FAILED  File descriptor not open.
     */
    otError HardwareReset(void)
    {
        otError error = OT_ERROR_NONE;

        VerifyOrExit(mFd >= 0, error = OT_ERROR_FAILED);

        if (isatty(mFd))
        {
            // Assert DTR (pulls reset line low on most designs).
            int flags;
            ioctl(mFd, TIOCMGET, &flags);
            flags &= ~TIOCM_DTR;
            ioctl(mFd, TIOCMSET, &flags);

            usleep(10000); // Hold reset for 10 ms.

            // Deassert DTR (release reset).
            flags |= TIOCM_DTR;
            ioctl(mFd, TIOCMSET, &flags);

            otLogInfoPlat("[VendorInterface] Hardware reset via DTR");
        }

    exit:
        return error;
    }

    /**
     * Adds the interface file descriptor to the mainloop fd sets so that
     * select() will wake up when data is available to read.
     *
     * @param[in,out] aMainloopContext  Pointer to a `MainloopContext` (fd_set + max_fd).
     */
    void UpdateFdSet(void *aMainloopContext)
    {
        if (mFd < 0 || aMainloopContext == nullptr)
        {
            return;
        }

        // Cast to the standard OpenThread POSIX mainloop context.
        otSysMainloopContext *ctx = static_cast<otSysMainloopContext *>(aMainloopContext);

        FD_SET(mFd, &ctx->mReadFdSet);

        if (mFd > ctx->mMaxFd)
        {
            ctx->mMaxFd = mFd;
        }
    }

    /**
     * Called each mainloop iteration. If the fd is readable, reads available
     * bytes and fires the receive callback for each complete frame.
     *
     * @param[in] aMainloopContext  Pointer to a `MainloopContext` carrying select() results.
     */
    void Process(const void *aMainloopContext)
    {
        if (mFd < 0 || aMainloopContext == nullptr)
        {
            return;
        }

        const otSysMainloopContext *ctx = static_cast<const otSysMainloopContext *>(aMainloopContext);

        if (!FD_ISSET(mFd, &ctx->mReadFdSet))
        {
            return;
        }

        ssize_t bytesRead = read(mFd, mReceiveBuffer, sizeof(mReceiveBuffer));

        if (bytesRead > 0)
        {
            mMetrics.mRcpFrameCount++;

            if (mReceiveFrameBuffer != nullptr && mReceiveFrameCallback != nullptr)
            {
                // Write raw bytes into the frame buffer. The buffer will
                // accumulate bytes and signal a complete frame as needed.
                if (mReceiveFrameBuffer->Write(mReceiveBuffer, static_cast<uint16_t>(bytesRead)) == OT_ERROR_NONE)
                {
                    mReceiveFrameCallback(mReceiveFrameCallbackContext);
                }
                else
                {
                    mMetrics.mRcpFrameCount--; // Undo increment on failure.
                    otLogWarnPlat("[VendorInterface] RX frame buffer write failed, dropping %zd bytes", bytesRead);
                }
            }
        }
        else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            otLogWarnPlat("[VendorInterface] read() error: %s", strerror(errno));
        }
    }

    /**
     * Blocks until a frame is available or the timeout elapses.
     *
     * @param[in] aTimeoutUs  Timeout in microseconds. 0 means return immediately.
     *
     * @retval OT_ERROR_NONE           A frame (or data) became available.
     * @retval OT_ERROR_RESPONSE_TIMEOUT  Timed out with no data.
     * @retval OT_ERROR_FAILED         File descriptor not open or select() error.
     */
    otError WaitForFrame(uint64_t aTimeoutUs)
    {
        otError error = OT_ERROR_NONE;

        VerifyOrExit(mFd >= 0, error = OT_ERROR_FAILED);

        {
            fd_set         readFds;
            struct timeval timeout;

            FD_ZERO(&readFds);
            FD_SET(mFd, &readFds);

            timeout.tv_sec  = static_cast<time_t>(aTimeoutUs / 1000000ULL);
            timeout.tv_usec = static_cast<suseconds_t>(aTimeoutUs % 1000000ULL);

            int result = select(mFd + 1, &readFds, nullptr, nullptr, &timeout);

            if (result == 0)
            {
                error = OT_ERROR_RESPONSE_TIMEOUT;
            }
            else if (result < 0)
            {
                otLogWarnPlat("[VendorInterface] select() error: %s", strerror(errno));
                error = OT_ERROR_FAILED;
            }
            // result > 0: data available, error stays OT_ERROR_NONE.
        }

    exit:
        return error;
    }

    /**
     * Sends a Spinel frame to the RCP over the file descriptor.
     *
     * @param[in] aFrame   Pointer to the frame data.
     * @param[in] aLength  Length of the frame in bytes.
     *
     * @retval OT_ERROR_NONE      Frame sent successfully.
     * @retval OT_ERROR_FAILED    Write error or fd not open.
     * @retval OT_ERROR_NO_BUFS  Frame exceeds maximum supported size.
     */
    otError SendFrame(const uint8_t *aFrame, uint16_t aLength)
    {
        otError error = OT_ERROR_NONE;

        VerifyOrExit(mFd >= 0, error = OT_ERROR_FAILED);
        VerifyOrExit(aFrame != nullptr, error = OT_ERROR_INVALID_ARGS);
        VerifyOrExit(aLength > 0 && aLength <= kMaxFrameSize, error = OT_ERROR_NO_BUFS);

        {
            ssize_t written = write(mFd, aFrame, aLength);

            if (written < 0)
            {
                otLogWarnPlat("[VendorInterface] write() error: %s", strerror(errno));
                error = OT_ERROR_FAILED;
            }
            else if (static_cast<uint16_t>(written) != aLength)
            {
                otLogWarnPlat("[VendorInterface] Partial write: %zd of %u bytes", written, aLength);
                error = OT_ERROR_FAILED;
            }
            else
            {
                mMetrics.mTransferredFrameCount++;
                mMetrics.mTransferredValidFrameCount++;
            }
        }

    exit:
        return error;
    }

    /**
     * Returns a pointer to the accumulated RCP interface metrics.
     *
     * Metrics include frame counts for both TX and RX directions.
     */
    const otRcpInterfaceMetrics *GetRcpInterfaceMetrics(void) const { return &mMetrics; }

private:
    const Url::Url &mRadioUrl;

    // File descriptor for the radio device (e.g., /dev/ttyUSB0).
    int mFd;

    // Receive callback registered by the upper layer.
    SpinelInterface::ReceiveFrameCallback mReceiveFrameCallback;
    void *                               mReceiveFrameCallbackContext;
    SpinelInterface::RxFrameBuffer *     mReceiveFrameBuffer;

    // Raw receive buffer for a single read() call.
    uint8_t mReceiveBuffer[kMaxFrameSize];

    // Bus speed in Hz, reported via GetBusSpeed().
    uint32_t mBusSpeed;

    // Accumulated interface metrics reported to the OpenThread stack.
    otRcpInterfaceMetrics mMetrics;
};

// ----------------------------------------------------------------------------
// `VendorInterface` public API — thin wrappers delegating to the impl object.
// ----------------------------------------------------------------------------

static OT_DEFINE_ALIGNED_VAR(sVendorInterfaceImplRaw, sizeof(VendorInterfaceImpl), uint64_t);

VendorInterface::VendorInterface(const Url::Url &aRadioUrl)
{
    new (&sVendorInterfaceImplRaw) VendorInterfaceImpl(aRadioUrl);
}

VendorInterface::~VendorInterface(void) { Deinit(); }

otError VendorInterface::Init(ReceiveFrameCallback aCallback, void *aCallbackContext, RxFrameBuffer &aFrameBuffer)
{
    return reinterpret_cast<VendorInterfaceImpl *>(&sVendorInterfaceImplRaw)
        ->Init(aCallback, aCallbackContext, aFrameBuffer);
}

void VendorInterface::Deinit(void)
{
    reinterpret_cast<VendorInterfaceImpl *>(&sVendorInterfaceImplRaw)->Deinit();
}

uint32_t VendorInterface::GetBusSpeed(void) const
{
    return reinterpret_cast<const VendorInterfaceImpl *>(&sVendorInterfaceImplRaw)->GetBusSpeed();
}

otError VendorInterface::HardwareReset(void)
{
    return reinterpret_cast<VendorInterfaceImpl *>(&sVendorInterfaceImplRaw)->HardwareReset();
}

void VendorInterface::UpdateFdSet(void *aMainloopContext)
{
    reinterpret_cast<VendorInterfaceImpl *>(&sVendorInterfaceImplRaw)->UpdateFdSet(aMainloopContext);
}

void VendorInterface::Process(const void *aMainloopContext)
{
    reinterpret_cast<VendorInterfaceImpl *>(&sVendorInterfaceImplRaw)->Process(aMainloopContext);
}

otError VendorInterface::WaitForFrame(uint64_t aTimeoutUs)
{
    return reinterpret_cast<VendorInterfaceImpl *>(&sVendorInterfaceImplRaw)->WaitForFrame(aTimeoutUs);
}

otError VendorInterface::SendFrame(const uint8_t *aFrame, uint16_t aLength)
{
    return reinterpret_cast<VendorInterfaceImpl *>(&sVendorInterfaceImplRaw)->SendFrame(aFrame, aLength);
}

const otRcpInterfaceMetrics *VendorInterface::GetRcpInterfaceMetrics(void) const
{
    return reinterpret_cast<const VendorInterfaceImpl *>(&sVendorInterfaceImplRaw)->GetRcpInterfaceMetrics();
}

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE