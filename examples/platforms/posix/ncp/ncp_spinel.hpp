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

#ifndef NCP_SPINEL_HPP_
#define NCP_SPINEL_HPP_

#include <ncp/hdlc.hpp>

#include "frame_cache.hpp"
#include "ncp.h"

namespace ot {

class NcpSpinel
{
public:
    /**
     * This constructor initializes the spinel based NCP.
     *
     */
    NcpSpinel(void);

    /**
     * Initialize this NCP.
     *
     * @param[in]   aNcpFile    The path to either a uart device or executable.
     * @param[in]   aNcpConfig  Parameters given to the device or executable.
     *
     */
    void Init(const char *aNcpFile, const char *aNcpConfig);

    /**
     * Deinitialize this NCP.
     *
     */
    void Deinit(void);

    /**
     * This method returns the file descriptor of the underlying radio driver.
     *
     * @returns The file descriptor of the underlying radio driver.
     *
     */
    int GetFd(void) const { return mSockFd; };

    /**
     * This method performs spinel processing.
     *
     * @param[in]   aFrame      A pointer to the frame used to receive a radio frame.
     * @param[in]   aRead       Whether there's new data available.
     *
     */
    void Process(otRadioFrame *aFrame, bool aRead);

    /**
     * This method binds this NCP with the given instance.
     *
     */
    void Bind(otInstance *aInstance, ReceivedHandler aReceivedHandler, TransmittedHandler aTransmittedHandler)
    {
        mInstance           = aInstance;
        mReceivedHandler    = aReceivedHandler;
        mTransmittedHandler = aTransmittedHandler;
    }

    /**
     * This method returns whether there are pending spinel frames.
     *
     */
    bool IsFrameCached(void) const;

    /**
     * This method tries to retrieve a spinel property from NCP.
     *
     * @param[in]   aKey        Spinel property key.
     * @param[in]   aFormat     Spinel formatter to unpack property value.
     * @param[out]  aArgs       Variable arguments list.
     *
     * @returns OT_ERROR_NONE on success, otherwise the property value is not available in arguments provided in aArgs.
     *
     */
    otError Get(spinel_prop_key_t aKey, const char *aFormat, va_list aArgs);

    /**
     * This method tries to update a spinel property of NCP.
     *
     * @param[in]   aKey        Spinel property key.
     * @param[in]   aFormat     Spinel formatter to pack property value.
     * @param[in]   aArgs       Variable arguments list.
     *
     * @returns OT_ERROR_NONE on success, otherwise the property is not updated.
     *
     */
    otError Set(spinel_prop_key_t aKey, const char *aFormat, va_list aArgs);

    /**
     * This method tries to insert a item into a spinel list property of NCP.
     *
     * @param[in]   aKey        Spinel property key.
     * @param[in]   aFormat     Spinel formatter to pack the item.
     * @param[in]   aArgs       Variable arguments list.
     *
     * @returns OT_ERROR_NONE on success, otherwise the item is not inserted.
     *
     */
    otError Insert(spinel_prop_key_t aKey, const char *aFormat, va_list aArgs);

    /**
     * This method tries to remove a item from a spinel list property of NCP.
     *
     * @param[in]   aKey        Spinel property key.
     * @param[in]   aFormat     Spinel formatter to pack the item.
     * @param[in]   aArgs       Variable arguments list.
     *
     * @returns OT_ERROR_NONE on success, otherwise the property is not removed.
     *
     */
    otError Remove(spinel_prop_key_t aKey, const char *aFormat, va_list aArgs);

    /**
     * This method transmits a radio frame through NCP.
     *
     * @param[in]   aFrame      A pointer to the radio frame to be transmitted.
     * @param[in]   aAckFrame   A pointer to a radio frame to receive ACK frame.
     *
     * @returns OT_ERROR_NONE on success, otherwise not transmitted.
     *
     */
    otError Transmit(const otRadioFrame *aFrame, otRadioFrame *aAckFrame);

private:
    enum
    {
        kMaxSpinelFrame = 2048, ///< Max size in bytes for transfering spinel frames.
        kMaxWaitTime    = 2000, ///< Max time to wait for response in milliseconds.
    };

    void         Receive(void);
    spinel_tid_t GetNextTid(void);
    void         FreeTid(spinel_tid_t tid) { mCmdTidsInUse &= ~(1 << tid); }
    otError      RequestV(bool aWait, uint32_t aCommand, spinel_prop_key_t aKey, const char *aFormat, va_list aArgs);
    otError      Request(bool aWait, uint32_t aCommand, spinel_prop_key_t aKey, const char *aFormat, ...);
    otError      WaitResponse(void);
    otError      SendAll(const uint8_t *aBuffer, uint16_t aLength);
    otError      SendReset(void);
    otError      SendCommand(uint32_t          command,
                             spinel_prop_key_t key,
                             spinel_tid_t      tid,
                             const char *      pack_format,
                             va_list           args);

    static void HandleHdlcError(void *aContext, otError aError, uint8_t *aBuffer, uint16_t aLength)
    {
        (void)aContext;
        (void)aError;
        (void)aBuffer;
        (void)aLength;
    }

    static void HandleHdlcFrame(void *aContext, uint8_t *aBuffer, uint16_t aLength)
    {
        static_cast<NcpSpinel *>(aContext)->HandleHdlcFrame(aBuffer, aLength);
    }
    void HandleHdlcFrame(const uint8_t *aBuffer, uint16_t aLength);

    otError ParseRawStream(otRadioFrame *aFrame, const uint8_t *aBuffer, uint16_t aLength);
    void    ProcessCommand(uint32_t aCommand, const uint8_t *aBuffer, uint16_t aLength);
    void    ProcessValueIs(spinel_prop_key_t aKey, const uint8_t *aBuffer, uint16_t aLength);
    void    HandleResult(uint32_t aCommand, spinel_prop_key_t aKey, const uint8_t *aBuffer, uint16_t aLength);
    void    HandleTransmitDone(uint32_t aCommand, spinel_prop_key_t aKey, const uint8_t *aBuffer, uint16_t aLength);
    void    ProcessNotification(const uint8_t *aBuffer, uint16_t aLength);
    void    ProcessResponse(const uint8_t *aBuffer, uint16_t aLength);
    void    ProcessCache(void);

    uint16_t     mCmdTidsInUse;
    spinel_tid_t mCmdNextTid;

    spinel_tid_t      mStreamTid;
    spinel_tid_t      mWaitingTid;
    spinel_prop_key_t mWaitingKey;
    const char *      mFormat;
    va_list           mArgs;
    uint32_t          mExpectedCommand;

    uint8_t       mHdlcBuffer[kMaxSpinelFrame];
    Hdlc::Decoder mHdlcDecoder;
    Hdlc::Encoder mHdlcEncoder;
    FrameCache    mFrameCache;

    int     mSockFd;
    bool    mIsReady;
    otError mLastError;

    otRadioFrame *mReceiveFrame;
    otRadioFrame *mAckFrame;

    otInstance *       mInstance;
    ReceivedHandler    mReceivedHandler;
    TransmittedHandler mTransmittedHandler;
};

} // namespace ot

#endif // NCP_SPINEL_HPP_
