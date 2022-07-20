/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file includes definitions for the CPC interface to radio (RCP).
 */

#ifndef POSIX_APP_CPC_INTERFACE_HPP_
#define POSIX_APP_CPC_INTERFACE_HPP_

#include "openthread-posix-config.h"
#include "platform-posix.h"
#include "lib/spinel/spinel_interface.hpp"
#include "sl_cpc.h"

#if OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_CPC

namespace ot {
namespace Posix {

/**
 * This class defines a CPC interface to the Radio Co-processor (RCP)
 *
 */
class CpcInterface
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in] aCallback         Callback on frame received
     * @param[in] aCallbackContext  Callback context
     * @param[in] aFrameBuffer      A reference to a `RxFrameBuffer` object.
     *
     */
    CpcInterface(Spinel::SpinelInterface::ReceiveFrameCallback aCallback,
                 void *                                        aCallbackContext,
                 Spinel::SpinelInterface::RxFrameBuffer &      aFrameBuffer);

    /**
     * This destructor deinitializes the object.
     *
     */
    ~CpcInterface(void);

    /**
     * This method initializes the interface to the Radio Co-processor (RCP)
     *
     * @note This method should be called before reading and sending spinel frames to the interface.
     *
     * @param[in] aRadioUrl           Used to make definition consistent with other interfaces
     *                                such as HDLC and SPI.
     *
     * @retval OT_ERROR_NONE          The interface is initialized successfully
     * @retval OT_ERROR_FAILED        The interface initialization was unsuccessful
     * 
     */
    otError Init(const Url::Url &aRadioUrl);

    /**
     * This method deinitializes the interface to the RCP.
     *
     */
    void Deinit(void);

    /**
     * This method sends a spinel frame to Radio Co-processor (RCP) over the socket.
     *
     *
     * @param[in] aFrame     A pointer to buffer containing the spinel frame to send.
     * @param[in] aLength    The length (number of bytes) in the frame.
     *
     * @retval OT_ERROR_NONE     Successfully sent the spinel frame.
     */
    otError SendFrame(const uint8_t *aFrame, uint16_t aLength);

    /**
     * This method waits for receiving part or all of spinel frame within specified interval.
     *
     * @param[in]  aTimeout  The timeout value in microseconds.
     *
     * @retval OT_ERROR_NONE             Part or all of spinel frame is received.
     */
    otError WaitForFrame(uint64_t aTimeoutUs);

    /**
     * This is a stub, cpc does not use file descriptors.
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
    uint32_t GetBusSpeed(void) const { return 0; }

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

    /**
      * This method returns the RCP interface metrics.
      *
      * @returns The RCP interface metrics.
      *
      */
    const otRcpInterfaceMetrics *GetRcpInterfaceMetrics(void) const { return &mInterfaceMetrics; }

    /**
      * This method is called reinitialise the CPC interface if sCpcResetReq indicates that a restart
      * is required.
      */
    void CheckAndReInitCpc(void);

private:
    /**
     * This method instructs `CpcInterface` to read data from radio over the socket.
     *
     * The mReceiveFrameCallback is used to pass the received frame to be processed.
     *
     */
    void Read(uint64_t aTimeoutUs);

    /**
     * This method waits for the socket file descriptor associated with the HDLC interface to become writable within
     * `kMaxWaitTime` interval.
     *
     * @retval OT_ERROR_NONE   Socket is writable.
     * @retval OT_ERROR_FAILED Socket did not become writable within `kMaxWaitTime`.
     *
     */
    otError WaitForWritable(void);

    /**
     * This method writes a given frame to the socket.
     *
     * This is blocking call, i.e., if the socket is not writable, this method waits for it to become writable for
     * up to `kMaxWaitTime` interval.
     *
     * @param[in] aFrame  A pointer to buffer containing the frame to write.
     * @param[in] aLength The length (number of bytes) in the frame.
     *
     * @retval OT_ERROR_NONE    Frame was written successfully.
     * @retval OT_ERROR_FAILED  Failed to write due to socket not becoming writable within `kMaxWaitTime`.
     *
     */
    otError Write(const uint8_t *aFrame, uint16_t aLength);

    /**
     * This method generates and sends a reset response back to OT.
     *
     * This method is called after the CPC layer catches the SPINEL reset command. This is done so that 
     * CPC can handle resets of the RCP and OT is tricked into thinking it handles resets.
     *
     */
    void SendResetResponse(void);

    enum
    {
        kMaxFrameSize       = SL_CPC_READ_MINIMUM_SIZE,
        kMaxWaitTime        = 2000, ///< Maximum wait time in Milliseconds for socket to become writable (see `SendFrame`).
        kMaxSleepDuration   = 1000,
        kMaxRestartAttempts = 10,
        kResetCMDSize       = 4
    };

    Spinel::SpinelInterface::ReceiveFrameCallback mReceiveFrameCallback;
    void *                                        mReceiveFrameContext;
    Spinel::SpinelInterface::RxFrameBuffer &      mReceiveFrameBuffer;

    int                 mSockFd;
    cpc_handle_t        mHandle;
    cpc_endpoint_t      mEndpoint;
    cpc_read_flags_t    mReadFlags;
    cpc_write_flags_t   mWriteFlags;

    static void HandleSecondaryReset(void);
    static void SetCpcResetReq(bool state) { sCpcResetReq = state; }

    // Hard Coded Reset Response
    // 0x72 -> STATUS_RESET_SOFTWARE
    uint8_t mResetResponse[kResetCMDSize] = {0x80, 0x06, 0x00, 0x72};

    const   uint8_t     mId = SL_CPC_ENDPOINT_15_4;
    typedef uint8_t     cpcError;
    static  bool        sCpcResetReq;

    otRcpInterfaceMetrics mInterfaceMetrics;

    // Non-copyable, intentionally not implemented.
    CpcInterface(const CpcInterface &);
    CpcInterface &operator=(const CpcInterface &);
};

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_CPC
#endif // POSIX_APP_CPC_INTERFACE_HPP_
