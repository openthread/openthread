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
 *   This file includes definitions for the IPC binder interface to radio (RCP).
 */

#ifndef ANDROID_THREAD_NETWORK_HAL_INTERFACE_HPP_
#define ANDROID_THREAD_NETWORK_HAL_INTERFACE_HPP_

#include "openthread-posix-config.h"

#include "platform-posix.h"
#include "lib/spinel/spinel_interface.hpp"

#include <openthread/openthread-system.h>

#if OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_VENDOR

#include <aidl/android/hardware/threadnetwork/BnThreadChipCallback.h>
#include <aidl/android/hardware/threadnetwork/IThreadChip.h>
#include <aidl/android/hardware/threadnetwork/IThreadChipCallback.h>

namespace ot {
namespace Posix {

/**
 * This class defines an IPC Binder interface to the Radio Co-processor (RCP).
 *
 */
class HalInterface
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in] aCallback         A reference to a `Callback` object.
     * @param[in] aCallbackContext  The context pointer passed to the callback.
     * @param[in] aFrameBuffer      A reference to a `RxFrameBuffer` object.
     *
     */
    HalInterface(Spinel::SpinelInterface::ReceiveFrameCallback aCallback,
                 void                                         *aCallbackContext,
                 Spinel::SpinelInterface::RxFrameBuffer       &aFrameBuffer);

    /**
     * This destructor deinitializes the object.
     *
     */
    ~HalInterface(void);

    /**
     * This method initializes the interface to the Radio Co-processor (RCP).
     *
     * @note This method should be called before reading and sending spinel frames to the interface.
     *
     * @param[in]  aRadioUrl          Arguments parsed from radio url.
     *
     * @retval OT_ERROR_NONE          The interface is initialized successfully.
     * @retval OT_ERROR_ALREADY       The interface is already initialized.
     * @retval OT_ERROR_INVALID_ARGS  The UART device or executable cannot be found or failed to open/run.
     *
     */
    otError Init(const Url::Url &aRadioUrl);

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
     * @retval OT_ERROR_FAILED   Failed to call the HAL to send the frame.
     *
     */
    otError SendFrame(const uint8_t *aFrame, uint16_t aLength);

    /**
     * This method waits for receiving part or all of spinel frame within specified interval.
     *
     * @param[in]  aTimeout  The timeout value in microseconds.
     *
     * @retval OT_ERROR_NONE             Part or all of spinel frame is received.
     * @retval OT_ERROR_RESPONSE_TIMEOUT No spinel frame is received within @p aTimeout.
     *
     */
    otError WaitForFrame(uint64_t aTimeoutUs);

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
     * @param[in]   aContext        The context containing fd_sets.
     *
     */
    void Process(const RadioProcessContext &aContext);

    /**
     * This method returns the bus speed between the host and the radio.
     *
     * @returns   Bus speed in bits/second.
     *
     */
    uint32_t GetBusSpeed(void) const;

    /**
     * This method is called when RCP failure detected and resets internal states of the interface.
     *
     */
    void OnRcpReset(void);

    /**
     * This method is called when RCP is reset to recreate the connection with it.
     * Intentionally empty.
     *
     */
    otError ResetConnection(void) { return OT_ERROR_NONE; }

private:
    void        ReceiveFrameCallback(const std::vector<uint8_t> &aFrame);
    static void BinderDeathCallback(void *aContext);
    otError     StatusToError(::ndk::ScopedAStatus &aStatus);

    class ThreadChipCallback : public ::aidl::android::hardware::threadnetwork::BnThreadChipCallback
    {
    public:
        ThreadChipCallback(HalInterface *aInterface)
            : mInterface(aInterface)
        {
        }

        ::ndk::ScopedAStatus onReceiveSpinelFrame(const std::vector<uint8_t> &in_aFrame)
        {
            mInterface->ReceiveFrameCallback(in_aFrame);
            return ndk::ScopedAStatus::ok();
        }

    private:
        HalInterface *mInterface;
    };

    enum
    {
        kMaxFrameSize = Spinel::SpinelInterface::kMaxFrameSize,
    };

    Spinel::SpinelInterface::ReceiveFrameCallback mRxFrameCallback;
    void                                         *mRxFrameContext;
    Spinel::SpinelInterface::RxFrameBuffer       &mRxFrameBuffer;

    std::shared_ptr<::aidl::android::hardware::threadnetwork::IThreadChip>         mThreadChip;
    std::shared_ptr<::aidl::android::hardware::threadnetwork::IThreadChipCallback> mThreadChipCallback;

    ::ndk::ScopedAIBinder_DeathRecipient mDeathRecipient;
    int                                  mBinderFd;

    // Non-copyable, intentionally not implemented.
    HalInterface(const HalInterface &);
    HalInterface &operator=(const HalInterface &);
};

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_VENDOR
#endif // ANDROID_THREAD_NETWORK_HAL_INTERFACE_HPP_
