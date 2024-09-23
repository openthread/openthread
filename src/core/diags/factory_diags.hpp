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

#if OPENTHREAD_CONFIG_DIAG_ENABLE

#include <string.h>

#include <openthread/diag.h>
#include <openthread/platform/radio.h>

#include "common/clearable.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/string.hpp"

namespace ot {
namespace FactoryDiags {

class Diags : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Constructor.
     *
     * @param[in]  aInstance  The OpenThread instance.
     */
    explicit Diags(Instance &aInstance);

    /**
     * Processes a factory diagnostics command line.
     *
     * @param[in]   aString        A null-terminated input string.
     */
    Error ProcessLine(const char *aString);

    /**
     * Processes a factory diagnostics command line.
     *
     * @param[in]   aArgsLength    The number of args in @p aArgs.
     * @param[in]   aArgs          The arguments of diagnostics command line.
     *
     * @retval  kErrorInvalidArgs       The command is supported but invalid arguments provided.
     * @retval  kErrorNone              The command is successfully process.
     * @retval  kErrorNotImplemented    The command is not supported.
     */
    Error ProcessCmd(uint8_t aArgsLength, char *aArgs[]);

    /**
     * Indicates whether or not the factory diagnostics mode is enabled.
     *
     * @retval TRUE if factory diagnostics mode is enabled
     * @retval FALSE if factory diagnostics mode is disabled.
     */
    bool IsEnabled(void);

    /**
     * The platform driver calls this method to notify OpenThread diagnostics module that the alarm has fired.
     */
    void AlarmFired(void);

    /**
     * The radio driver calls this method to notify OpenThread diagnostics module of a received frame.
     *
     * @param[in]  aFrame  A pointer to the received frame or `nullptr` if the receive operation failed.
     * @param[in]  aError  kErrorNone when successfully received a frame,
     *                     kErrorAbort when reception was aborted and a frame was not received,
     *                     kErrorNoBufs when a frame could not be received due to lack of rx buffer space.
     */
    void ReceiveDone(otRadioFrame *aFrame, Error aError);

    /**
     * The radio driver calls this method to notify OpenThread diagnostics module that the transmission has completed.
     *
     * @param[in]  aError  kErrorNone when the frame was transmitted,
     *                     kErrorChannelAccessFailure tx could not take place due to activity on channel,
     *                     kErrorAbort when transmission was aborted for other reasons.
     */
    void TransmitDone(Error aError);

    /**
     * Sets the diag output callback.
     *
     * @param[in]  aCallback   A callback method called to output diag messages.
     * @param[in]  aContext    A user context pointer.
     */
    void SetOutputCallback(otDiagOutputCallback aCallback, void *aContext);

private:
    static constexpr uint8_t kMaxArgs = OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX;

    struct Command
    {
        const char *mName;
        Error (Diags::*mCommand)(uint8_t aArgsLength, char *aArgs[]);
    };

    struct Stats : public Clearable<Stats>
    {
        uint32_t mReceivedPackets;
        uint32_t mSentPackets;
        int8_t   mFirstRssi;
        uint8_t  mFirstLqi;
        int8_t   mLastRssi;
        uint8_t  mLastLqi;
    };

    struct RawPowerSetting
    {
        static constexpr uint16_t       kMaxDataSize    = OPENTHREAD_CONFIG_POWER_CALIBRATION_RAW_POWER_SETTING_SIZE;
        static constexpr uint16_t       kInfoStringSize = kMaxDataSize * 2 + 1;
        typedef String<kInfoStringSize> InfoString;

        InfoString ToString(void) const
        {
            InfoString string;

            string.AppendHexBytes(mData, mLength);

            return string;
        }

        bool operator!=(const RawPowerSetting &aOther) const
        {
            return (mLength != aOther.mLength) || (memcmp(mData, aOther.mData, mLength) != 0);
        }

        uint8_t  mData[kMaxDataSize];
        uint16_t mLength;
    };

    struct PowerSettings
    {
        bool operator!=(const PowerSettings &aOther) const
        {
            return (mTargetPower != aOther.mTargetPower) || (mActualPower != aOther.mActualPower) ||
                   (mRawPowerSetting != aOther.mRawPowerSetting);
        }

        int16_t         mTargetPower;
        int16_t         mActualPower;
        RawPowerSetting mRawPowerSetting;
    };

    Error ParseCmd(char *aString, uint8_t &aArgsLength, char *aArgs[]);
    Error ProcessChannel(uint8_t aArgsLength, char *aArgs[]);
    Error ProcessFrame(uint8_t aArgsLength, char *aArgs[]);
    Error ProcessContinuousWave(uint8_t aArgsLength, char *aArgs[]);
    Error ProcessGpio(uint8_t aArgsLength, char *aArgs[]);
    Error ProcessPower(uint8_t aArgsLength, char *aArgs[]);
    Error ProcessRadio(uint8_t aArgsLength, char *aArgs[]);
    Error ProcessRepeat(uint8_t aArgsLength, char *aArgs[]);
    Error ProcessPowerSettings(uint8_t aArgsLength, char *aArgs[]);
    Error ProcessRawPowerSetting(uint8_t aArgsLength, char *aArgs[]);
    Error ProcessSend(uint8_t aArgsLength, char *aArgs[]);
    Error ProcessStart(uint8_t aArgsLength, char *aArgs[]);
    Error ProcessStats(uint8_t aArgsLength, char *aArgs[]);
    Error ProcessStop(uint8_t aArgsLength, char *aArgs[]);
    Error ProcessStream(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_RADIO && !OPENTHREAD_RADIO_CLI
    Error ProcessEcho(uint8_t aArgsLength, char *aArgs[]);
#endif

    Error GetRawPowerSetting(RawPowerSetting &aRawPowerSetting);
    Error GetPowerSettings(uint8_t aChannel, PowerSettings &aPowerSettings);

    void TransmitPacket(void);
    void Output(const char *aFormat, ...);
    void AppendErrorResult(Error aError);
    void ResetTxPacket(void);

    static Error ParseLong(char *aString, long &aLong);
    static Error ParseBool(char *aString, bool &aBool);

    static const struct Command sCommands[];

#if OPENTHREAD_FTD || OPENTHREAD_MTD || (OPENTHREAD_RADIO && OPENTHREAD_RADIO_CLI)
    Stats mStats;

    otRadioFrame *mTxPacket;
    uint32_t      mTxPeriod;
    uint32_t      mTxPackets;
    uint8_t       mChannel;
    int8_t        mTxPower;
    uint8_t       mTxLen;
    bool          mIsTxPacketSet;
    bool          mRepeatActive;
    bool          mDiagSendOn;
#endif

    otDiagOutputCallback mOutputCallback;
    void                *mOutputContext;
};

} // namespace FactoryDiags
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_DIAG_ENABLE

#endif // FACTORY_DIAGS_HPP_
