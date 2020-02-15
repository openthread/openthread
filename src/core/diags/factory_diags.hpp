/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file contains definitions for the diagnostics module.
 */

#ifndef FACTORY_DIAGS_HPP_
#define FACTORY_DIAGS_HPP_

#include "openthread-core-config.h"

#include <string.h>

#include <openthread/platform/radio.h>

#include "common/locator.hpp"

namespace ot {
namespace FactoryDiags {

#if OPENTHREAD_CONFIG_DIAG_ENABLE

class Diags : public InstanceLocator
{
public:
    /**
     * Constructor.
     *
     * @param[in]  aInstance  The OpenThread instance.
     *
     */
    explicit Diags(Instance &aInstance);

    /**
     * This method processes a factory diagnostics command line.
     *
     * @param[in]   aString        A NULL-terminated input string.
     * @param[out]  aOutput        The diagnostics execution result.
     * @param[in]   aOutputMaxLen  The output buffer size.
     *
     */
    void ProcessLine(const char *aString, char *aOutput, size_t aOutputMaxLen);

    /**
     * This method processes a factory diagnostics command line.
     *
     * @param[in]   aArgCount      The argument counter of diagnostics command line.
     * @param[in]   aArgVector     The argument vector of diagnostics command line.
     * @param[out]  aOutput        The diagnostics execution result.
     * @param[in]   aOutputMaxLen  The output buffer size.
     *
     */
    void ProcessCmd(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);

    /**
     * This method indicates whether or not the factory diagnostics mode is enabled.
     *
     * @retval TRUE if factory diagnostics mode is enabled
     * @retval FALSE if factory diagnostics mode is disabled.
     *
     */
    bool IsEnabled(void);

    /**
     * The platform driver calls this method to notify OpenThread diagnostics module that the alarm has fired.
     *
     */
    void AlarmFired(void);

    /**
     * The radio driver calls this method to notify OpenThread diagnostics module of a received frame.
     *
     * @param[in]  aFrame  A pointer to the received frame or NULL if the receive operation failed.
     * @param[in]  aError  OT_ERROR_NONE when successfully received a frame,
     *                     OT_ERROR_ABORT when reception was aborted and a frame was not received,
     *                     OT_ERROR_NO_BUFS when a frame could not be received due to lack of rx buffer space.
     *
     */
    void ReceiveDone(otRadioFrame *aFrame, otError aError);

    /**
     * The radio driver calls this method to notify OpenThread diagnostics module that the transmission has completed.
     *
     * @param[in]  aError  OT_ERROR_NONE when the frame was transmitted,
     *                     OT_ERROR_CHANNEL_ACCESS_FAILURE tx could not take place due to activity on channel,
     *                     OT_ERROR_ABORT when transmission was aborted for other reasons.
     *
     */
    void TransmitDone(otError aError);

private:
    struct Command
    {
        const char *mName;
        void (Diags::*mCommand)(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    };

    struct Stats
    {
        void Clear(void) { memset(this, 0, sizeof(*this)); }

        uint32_t mReceivedPackets;
        uint32_t mSentPackets;
        int8_t   mFirstRssi;
        uint8_t  mFirstLqi;
        int8_t   mLastRssi;
        uint8_t  mLastLqi;
    };

    void ProcessChannel(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    void ProcessPower(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    void ProcessRadio(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    void ProcessRepeat(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    void ProcessSend(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    void ProcessStart(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    void ProcessStats(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    void ProcessStop(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);

    void TransmitPacket(void);

    static void    AppendErrorResult(otError aError, char *aOutput, size_t aOutputMaxLen);
    static otError ParseLong(char *aString, long &aLong);

    static const struct Command sCommands[];

#if !OPENTHREAD_RADIO
    Stats mStats;

    otRadioFrame *mTxPacket;
    uint32_t      mTxPeriod;
    uint32_t      mTxPackets;
    uint8_t       mChannel;
    int8_t        mTxPower;
    uint8_t       mTxLen;
    bool          mRepeatActive;
#endif
};

#endif // #if OPENTHREAD_CONFIG_DIAG_ENABLE

} // namespace FactoryDiags
} // namespace ot

#endif // FACTORY_DIAGS_HPP_
