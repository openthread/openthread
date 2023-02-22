/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements the spinel based radio transceiver.
 */

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#include <openthread/dataset.h>
#include <openthread/logging.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/time.h>

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/new.hpp"
#include "common/settings.hpp"
#include "lib/platform/exit_code.h"
#include "lib/spinel/radio_spinel.hpp"
#include "lib/spinel/spinel.h"
#include "lib/spinel/spinel_decoder.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "radio/radio.hpp"
#include "thread/key_manager.hpp"

#ifndef MS_PER_S
#define MS_PER_S 1000
#endif
#ifndef US_PER_MS
#define US_PER_MS 1000
#endif
#ifndef US_PER_S
#define US_PER_S (MS_PER_S * US_PER_MS)
#endif

#ifndef TX_WAIT_US
#define TX_WAIT_US (5 * US_PER_S)
#endif

using ot::Spinel::Decoder;

namespace ot {
namespace Spinel {

static inline otError SpinelStatusToOtError(spinel_status_t aError)
{
    otError ret;

    switch (aError)
    {
    case SPINEL_STATUS_OK:
        ret = OT_ERROR_NONE;
        break;

    case SPINEL_STATUS_FAILURE:
        ret = OT_ERROR_FAILED;
        break;

    case SPINEL_STATUS_DROPPED:
        ret = OT_ERROR_DROP;
        break;

    case SPINEL_STATUS_NOMEM:
        ret = OT_ERROR_NO_BUFS;
        break;

    case SPINEL_STATUS_BUSY:
        ret = OT_ERROR_BUSY;
        break;

    case SPINEL_STATUS_PARSE_ERROR:
        ret = OT_ERROR_PARSE;
        break;

    case SPINEL_STATUS_INVALID_ARGUMENT:
        ret = OT_ERROR_INVALID_ARGS;
        break;

    case SPINEL_STATUS_UNIMPLEMENTED:
        ret = OT_ERROR_NOT_IMPLEMENTED;
        break;

    case SPINEL_STATUS_INVALID_STATE:
        ret = OT_ERROR_INVALID_STATE;
        break;

    case SPINEL_STATUS_NO_ACK:
        ret = OT_ERROR_NO_ACK;
        break;

    case SPINEL_STATUS_CCA_FAILURE:
        ret = OT_ERROR_CHANNEL_ACCESS_FAILURE;
        break;

    case SPINEL_STATUS_ALREADY:
        ret = OT_ERROR_ALREADY;
        break;

    case SPINEL_STATUS_PROP_NOT_FOUND:
        ret = OT_ERROR_NOT_IMPLEMENTED;
        break;

    case SPINEL_STATUS_ITEM_NOT_FOUND:
        ret = OT_ERROR_NOT_FOUND;
        break;

    default:
        if (aError >= SPINEL_STATUS_STACK_NATIVE__BEGIN && aError <= SPINEL_STATUS_STACK_NATIVE__END)
        {
            ret = static_cast<otError>(aError - SPINEL_STATUS_STACK_NATIVE__BEGIN);
        }
        else
        {
            ret = OT_ERROR_FAILED;
        }
        break;
    }

    return ret;
}

static inline void LogIfFail(const char *aText, otError aError)
{
    OT_UNUSED_VARIABLE(aText);
    OT_UNUSED_VARIABLE(aError);

    if (aError != OT_ERROR_NONE && aError != OT_ERROR_NO_ACK)
    {
        otLogWarnPlat("%s: %s", aText, otThreadErrorToString(aError));
    }
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::HandleReceivedFrame(void *aContext)
{
    static_cast<RadioSpinel *>(aContext)->HandleReceivedFrame();
}

template <typename InterfaceType, typename ProcessContextType>
RadioSpinel<InterfaceType, ProcessContextType>::RadioSpinel(void)
    : mInstance(nullptr)
    , mRxFrameBuffer()
    , mSpinelInterface(HandleReceivedFrame, this, mRxFrameBuffer)
    , mCmdTidsInUse(0)
    , mCmdNextTid(1)
    , mTxRadioTid(0)
    , mWaitingTid(0)
    , mWaitingKey(SPINEL_PROP_LAST_STATUS)
    , mPropertyFormat(nullptr)
    , mExpectedCommand(0)
    , mError(OT_ERROR_NONE)
    , mTransmitFrame(nullptr)
    , mShortAddress(0)
    , mPanId(0xffff)
    , mRadioCaps(0)
    , mChannel(0)
    , mRxSensitivity(0)
    , mState(kStateDisabled)
    , mIsPromiscuous(false)
    , mIsReady(false)
    , mSupportsLogStream(false)
    , mIsTimeSynced(false)
#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    , mRcpFailureCount(0)
    , mSrcMatchShortEntryCount(0)
    , mSrcMatchExtEntryCount(0)
    , mMacKeySet(false)
    , mCcaEnergyDetectThresholdSet(false)
    , mTransmitPowerSet(false)
    , mCoexEnabledSet(false)
    , mFemLnaGainSet(false)
    , mRcpFailed(false)
    , mEnergyScanning(false)
#endif
#if OPENTHREAD_CONFIG_DIAG_ENABLE
    , mDiagMode(false)
    , mDiagOutput(nullptr)
    , mDiagOutputMaxLen(0)
#endif
    , mTxRadioEndUs(UINT64_MAX)
    , mRadioTimeRecalcStart(UINT64_MAX)
    , mRadioTimeOffset(UINT64_MAX)
{
    mVersion[0] = '\0';
    memset(&mRadioSpinelMetrics, 0, sizeof(mRadioSpinelMetrics));
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::Init(bool aRestoreDatasetFromNcp, bool aSkipRcpCompatibilityCheck)
{
    otError error = OT_ERROR_NONE;
    bool    supportsRcpApiVersion;
    bool    supportsRcpMinHostApiVersion;

    ResetRcp();
    SuccessOrExit(error = CheckSpinelVersion());
    SuccessOrExit(error = Get(SPINEL_PROP_NCP_VERSION, SPINEL_DATATYPE_UTF8_S, mVersion, sizeof(mVersion)));
    SuccessOrExit(error = Get(SPINEL_PROP_HWADDR, SPINEL_DATATYPE_EUI64_S, mIeeeEui64.m8));

    if (!IsRcp(supportsRcpApiVersion, supportsRcpMinHostApiVersion))
    {
        uint8_t exitCode = OT_EXIT_RADIO_SPINEL_INCOMPATIBLE;

        if (aRestoreDatasetFromNcp)
        {
#if !OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
            exitCode = (RestoreDatasetFromNcp() == OT_ERROR_NONE) ? OT_EXIT_SUCCESS : OT_EXIT_FAILURE;
#endif
        }

        DieNow(exitCode);
    }

    if (!aSkipRcpCompatibilityCheck)
    {
        SuccessOrDie(CheckRcpApiVersion(supportsRcpApiVersion, supportsRcpMinHostApiVersion));
        SuccessOrDie(CheckRadioCapabilities());
    }

    mRxRadioFrame.mPsdu  = mRxPsdu;
    mTxRadioFrame.mPsdu  = mTxPsdu;
    mAckRadioFrame.mPsdu = mAckPsdu;

exit:
    SuccessOrDie(error);
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::ResetRcp(void)
{
    mIsReady = false;

    mSpinelInterface.ResetStates();
    SuccessOrDie(SendReset(SPINEL_RESET_STACK));
    SuccessOrDie(mSpinelInterface.ResetConnection());

    if (WaitForResetReason() != OT_ERROR_NONE)
    {
        mSpinelInterface.ResetStates();
        SuccessOrDie(mSpinelInterface.HardwareReset());
        SuccessOrDie(mSpinelInterface.ResetConnection());
        otLogInfoPlat("Hardware reset RCP");
        SuccessOrDie(WaitForResetReason());
    }
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::CheckSpinelVersion(void)
{
    otError      error = OT_ERROR_NONE;
    unsigned int versionMajor;
    unsigned int versionMinor;

    SuccessOrExit(error =
                      Get(SPINEL_PROP_PROTOCOL_VERSION, (SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S),
                          &versionMajor, &versionMinor));

    if ((versionMajor != SPINEL_PROTOCOL_VERSION_THREAD_MAJOR) ||
        (versionMinor != SPINEL_PROTOCOL_VERSION_THREAD_MINOR))
    {
        otLogCritPlat("Spinel version mismatch - Posix:%d.%d, RCP:%d.%d", SPINEL_PROTOCOL_VERSION_THREAD_MAJOR,
                      SPINEL_PROTOCOL_VERSION_THREAD_MINOR, versionMajor, versionMinor);
        DieNow(OT_EXIT_RADIO_SPINEL_INCOMPATIBLE);
    }

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
bool RadioSpinel<InterfaceType, ProcessContextType>::IsRcp(bool &aSupportsRcpApiVersion,
                                                           bool &aSupportsRcpMinHostApiVersion)
{
    uint8_t        capsBuffer[kCapsBufferSize];
    const uint8_t *capsData         = capsBuffer;
    spinel_size_t  capsLength       = sizeof(capsBuffer);
    bool           supportsRawRadio = false;
    bool           isRcp            = false;

    aSupportsRcpApiVersion        = false;
    aSupportsRcpMinHostApiVersion = false;

    SuccessOrDie(Get(SPINEL_PROP_CAPS, SPINEL_DATATYPE_DATA_S, capsBuffer, &capsLength));

    while (capsLength > 0)
    {
        unsigned int   capability;
        spinel_ssize_t unpacked;

        unpacked = spinel_datatype_unpack(capsData, capsLength, SPINEL_DATATYPE_UINT_PACKED_S, &capability);
        VerifyOrDie(unpacked > 0, OT_EXIT_RADIO_SPINEL_INCOMPATIBLE);

        if (capability == SPINEL_CAP_MAC_RAW)
        {
            supportsRawRadio = true;
        }

        if (capability == SPINEL_CAP_CONFIG_RADIO)
        {
            isRcp = true;
        }

        if (capability == SPINEL_CAP_OPENTHREAD_LOG_METADATA)
        {
            mSupportsLogStream = true;
        }

        if (capability == SPINEL_CAP_RCP_API_VERSION)
        {
            aSupportsRcpApiVersion = true;
        }

        if (capability == SPINEL_PROP_RCP_MIN_HOST_API_VERSION)
        {
            aSupportsRcpMinHostApiVersion = true;
        }

        capsData += unpacked;
        capsLength -= static_cast<spinel_size_t>(unpacked);
    }

    if (!supportsRawRadio && isRcp)
    {
        otLogCritPlat("RCP capability list does not include support for radio/raw mode");
        DieNow(OT_EXIT_RADIO_SPINEL_INCOMPATIBLE);
    }

    return isRcp;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::CheckRadioCapabilities(void)
{
    const otRadioCaps kRequiredRadioCaps =
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
        OT_RADIO_CAPS_TRANSMIT_SEC | OT_RADIO_CAPS_TRANSMIT_TIMING |
#endif
        OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_TRANSMIT_RETRIES | OT_RADIO_CAPS_CSMA_BACKOFF;

    otError      error = OT_ERROR_NONE;
    unsigned int radioCaps;

    SuccessOrExit(error = Get(SPINEL_PROP_RADIO_CAPS, SPINEL_DATATYPE_UINT_PACKED_S, &radioCaps));
    mRadioCaps = static_cast<otRadioCaps>(radioCaps);

    if ((mRadioCaps & kRequiredRadioCaps) != kRequiredRadioCaps)
    {
        otRadioCaps missingCaps = (mRadioCaps & kRequiredRadioCaps) ^ kRequiredRadioCaps;

        // missingCaps may be an unused variable when otLogCritPlat is blank
        // avoid compiler warning in that case
        OT_UNUSED_VARIABLE(missingCaps);

        otLogCritPlat("RCP is missing required capabilities: %s%s%s%s%s",
                      (missingCaps & OT_RADIO_CAPS_ACK_TIMEOUT) ? "ack-timeout " : "",
                      (missingCaps & OT_RADIO_CAPS_TRANSMIT_RETRIES) ? "tx-retries " : "",
                      (missingCaps & OT_RADIO_CAPS_CSMA_BACKOFF) ? "CSMA-backoff " : "",
                      (missingCaps & OT_RADIO_CAPS_TRANSMIT_SEC) ? "tx-security " : "",
                      (missingCaps & OT_RADIO_CAPS_TRANSMIT_TIMING) ? "tx-timing " : "");

        DieNow(OT_EXIT_RADIO_SPINEL_INCOMPATIBLE);
    }

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::CheckRcpApiVersion(bool aSupportsRcpApiVersion,
                                                                           bool aSupportsRcpMinHostApiVersion)
{
    otError error = OT_ERROR_NONE;

    static_assert(SPINEL_MIN_HOST_SUPPORTED_RCP_API_VERSION <= SPINEL_RCP_API_VERSION,
                  "MIN_HOST_SUPPORTED_RCP_API_VERSION must be smaller than or equal to RCP_API_VERSION");

    if (aSupportsRcpApiVersion)
    {
        // Make sure RCP is not too old and its version is within the
        // range host supports.

        unsigned int rcpApiVersion;

        SuccessOrExit(error = Get(SPINEL_PROP_RCP_API_VERSION, SPINEL_DATATYPE_UINT_PACKED_S, &rcpApiVersion));

        if (rcpApiVersion < SPINEL_MIN_HOST_SUPPORTED_RCP_API_VERSION)
        {
            otLogCritPlat("RCP and host are using incompatible API versions");
            otLogCritPlat("RCP API Version %u is older than min required by host %u", rcpApiVersion,
                          SPINEL_MIN_HOST_SUPPORTED_RCP_API_VERSION);
            DieNow(OT_EXIT_RADIO_SPINEL_INCOMPATIBLE);
        }
    }

    if (aSupportsRcpMinHostApiVersion)
    {
        // Check with RCP about min host API version it can work with,
        // and make sure on host side our version is within the supported
        // range.

        unsigned int minHostRcpApiVersion;

        SuccessOrExit(
            error = Get(SPINEL_PROP_RCP_MIN_HOST_API_VERSION, SPINEL_DATATYPE_UINT_PACKED_S, &minHostRcpApiVersion));

        if (SPINEL_RCP_API_VERSION < minHostRcpApiVersion)
        {
            otLogCritPlat("RCP and host are using incompatible API versions");
            otLogCritPlat("RCP requires min host API version %u but host is older and at version %u",
                          minHostRcpApiVersion, SPINEL_RCP_API_VERSION);
            DieNow(OT_EXIT_RADIO_SPINEL_INCOMPATIBLE);
        }
    }

exit:
    return error;
}

#if !OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::RestoreDatasetFromNcp(void)
{
    otError error = OT_ERROR_NONE;

    Instance::Get().template Get<SettingsDriver>().Init(nullptr, 0);

    otLogInfoPlat("Trying to get saved dataset from NCP");
    SuccessOrExit(
        error = Get(SPINEL_PROP_THREAD_ACTIVE_DATASET, SPINEL_DATATYPE_VOID_S, &RadioSpinel::ThreadDatasetHandler));
    SuccessOrExit(
        error = Get(SPINEL_PROP_THREAD_PENDING_DATASET, SPINEL_DATATYPE_VOID_S, &RadioSpinel::ThreadDatasetHandler));

exit:
    Instance::Get().template Get<SettingsDriver>().Deinit();
    return error;
}
#endif

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::Deinit(void)
{
    mSpinelInterface.Deinit();
    // This allows implementing pseudo reset.
    new (this) RadioSpinel();
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::HandleReceivedFrame(void)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        header;
    spinel_ssize_t unpacked;

    LogSpinelFrame(mRxFrameBuffer.GetFrame(), mRxFrameBuffer.GetLength(), false);
    unpacked = spinel_datatype_unpack(mRxFrameBuffer.GetFrame(), mRxFrameBuffer.GetLength(), "C", &header);

    VerifyOrExit(unpacked > 0 && (header & SPINEL_HEADER_FLAG) == SPINEL_HEADER_FLAG &&
                     SPINEL_HEADER_GET_IID(header) == 0,
                 error = OT_ERROR_PARSE);

    if (SPINEL_HEADER_GET_TID(header) == 0)
    {
        HandleNotification(mRxFrameBuffer);
    }
    else
    {
        HandleResponse(mRxFrameBuffer.GetFrame(), mRxFrameBuffer.GetLength());
        mRxFrameBuffer.DiscardFrame();
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        mRxFrameBuffer.DiscardFrame();
        otLogWarnPlat("Error handling hdlc frame: %s", otThreadErrorToString(error));
    }

    UpdateParseErrorCount(error);
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::HandleNotification(SpinelInterface::RxFrameBuffer &aFrameBuffer)
{
    spinel_prop_key_t key;
    spinel_size_t     len = 0;
    spinel_ssize_t    unpacked;
    uint8_t          *data = nullptr;
    uint32_t          cmd;
    uint8_t           header;
    otError           error           = OT_ERROR_NONE;
    bool              shouldSaveFrame = false;

    unpacked = spinel_datatype_unpack(aFrameBuffer.GetFrame(), aFrameBuffer.GetLength(), "CiiD", &header, &cmd, &key,
                                      &data, &len);
    VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
    VerifyOrExit(SPINEL_HEADER_GET_TID(header) == 0, error = OT_ERROR_PARSE);

    switch (cmd)
    {
    case SPINEL_CMD_PROP_VALUE_IS:
        // Some spinel properties cannot be handled during `WaitResponse()`, we must cache these events.
        // `mWaitingTid` is released immediately after received the response. And `mWaitingKey` is be set
        // to `SPINEL_PROP_LAST_STATUS` at the end of `WaitResponse()`.

        if (!IsSafeToHandleNow(key))
        {
            ExitNow(shouldSaveFrame = true);
        }

        HandleValueIs(key, data, static_cast<uint16_t>(len));
        break;

    case SPINEL_CMD_PROP_VALUE_INSERTED:
    case SPINEL_CMD_PROP_VALUE_REMOVED:
        otLogInfoPlat("Ignored command %lu", ToUlong(cmd));
        break;

    default:
        ExitNow(error = OT_ERROR_PARSE);
    }

exit:
    if (shouldSaveFrame)
    {
        aFrameBuffer.SaveFrame();
    }
    else
    {
        aFrameBuffer.DiscardFrame();
    }

    UpdateParseErrorCount(error);
    LogIfFail("Error processing notification", error);
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::HandleNotification(const uint8_t *aFrame, uint16_t aLength)
{
    spinel_prop_key_t key;
    spinel_size_t     len = 0;
    spinel_ssize_t    unpacked;
    uint8_t          *data = nullptr;
    uint32_t          cmd;
    uint8_t           header;
    otError           error = OT_ERROR_NONE;

    unpacked = spinel_datatype_unpack(aFrame, aLength, "CiiD", &header, &cmd, &key, &data, &len);
    VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
    VerifyOrExit(SPINEL_HEADER_GET_TID(header) == 0, error = OT_ERROR_PARSE);
    VerifyOrExit(cmd == SPINEL_CMD_PROP_VALUE_IS);
    HandleValueIs(key, data, static_cast<uint16_t>(len));

exit:
    UpdateParseErrorCount(error);
    LogIfFail("Error processing saved notification", error);
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::HandleResponse(const uint8_t *aBuffer, uint16_t aLength)
{
    spinel_prop_key_t key;
    uint8_t          *data   = nullptr;
    spinel_size_t     len    = 0;
    uint8_t           header = 0;
    uint32_t          cmd    = 0;
    spinel_ssize_t    rval   = 0;
    otError           error  = OT_ERROR_NONE;

    rval = spinel_datatype_unpack(aBuffer, aLength, "CiiD", &header, &cmd, &key, &data, &len);
    VerifyOrExit(rval > 0 && cmd >= SPINEL_CMD_PROP_VALUE_IS && cmd <= SPINEL_CMD_PROP_VALUE_REMOVED,
                 error = OT_ERROR_PARSE);

    if (mWaitingTid == SPINEL_HEADER_GET_TID(header))
    {
        HandleWaitingResponse(cmd, key, data, static_cast<uint16_t>(len));
        FreeTid(mWaitingTid);
        mWaitingTid = 0;
    }
    else if (mTxRadioTid == SPINEL_HEADER_GET_TID(header))
    {
        if (mState == kStateTransmitting)
        {
            HandleTransmitDone(cmd, key, data, static_cast<uint16_t>(len));
        }

        FreeTid(mTxRadioTid);
        mTxRadioTid = 0;
    }
    else
    {
        otLogWarnPlat("Unexpected Spinel transaction message: %u", SPINEL_HEADER_GET_TID(header));
        error = OT_ERROR_DROP;
    }

exit:
    UpdateParseErrorCount(error);
    LogIfFail("Error processing response", error);
}

#if !OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::ThreadDatasetHandler(const uint8_t *aBuffer, uint16_t aLength)
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset opDataset;
    bool                 isActive = ((mWaitingKey == SPINEL_PROP_THREAD_ACTIVE_DATASET) ? true : false);
    Spinel::Decoder      decoder;
    MeshCoP::Dataset     dataset;

    memset(&opDataset, 0, sizeof(otOperationalDataset));
    decoder.Init(aBuffer, aLength);

    while (!decoder.IsAllReadInStruct())
    {
        unsigned int propKey;

        SuccessOrExit(error = decoder.OpenStruct());
        SuccessOrExit(error = decoder.ReadUintPacked(propKey));

        switch (static_cast<spinel_prop_key_t>(propKey))
        {
        case SPINEL_PROP_NET_NETWORK_KEY:
        {
            const uint8_t *key;
            uint16_t       len;

            SuccessOrExit(error = decoder.ReadData(key, len));
            VerifyOrExit(len == OT_NETWORK_KEY_SIZE, error = OT_ERROR_INVALID_ARGS);
            memcpy(opDataset.mNetworkKey.m8, key, len);
            opDataset.mComponents.mIsNetworkKeyPresent = true;
            break;
        }

        case SPINEL_PROP_NET_NETWORK_NAME:
        {
            const char *name;
            size_t      len;

            SuccessOrExit(error = decoder.ReadUtf8(name));
            len = StringLength(name, OT_NETWORK_NAME_MAX_SIZE);
            memcpy(opDataset.mNetworkName.m8, name, len);
            opDataset.mNetworkName.m8[len]              = '\0';
            opDataset.mComponents.mIsNetworkNamePresent = true;
            break;
        }

        case SPINEL_PROP_NET_XPANID:
        {
            const uint8_t *xpanid;
            uint16_t       len;

            SuccessOrExit(error = decoder.ReadData(xpanid, len));
            VerifyOrExit(len == OT_EXT_PAN_ID_SIZE, error = OT_ERROR_INVALID_ARGS);
            memcpy(opDataset.mExtendedPanId.m8, xpanid, len);
            opDataset.mComponents.mIsExtendedPanIdPresent = true;
            break;
        }

        case SPINEL_PROP_IPV6_ML_PREFIX:
        {
            const otIp6Address *addr;
            uint8_t             prefixLen;

            SuccessOrExit(error = decoder.ReadIp6Address(addr));
            SuccessOrExit(error = decoder.ReadUint8(prefixLen));
            VerifyOrExit(prefixLen == OT_IP6_PREFIX_BITSIZE, error = OT_ERROR_INVALID_ARGS);
            memcpy(opDataset.mMeshLocalPrefix.m8, addr, OT_MESH_LOCAL_PREFIX_SIZE);
            opDataset.mComponents.mIsMeshLocalPrefixPresent = true;
            break;
        }

        case SPINEL_PROP_DATASET_DELAY_TIMER:
        {
            SuccessOrExit(error = decoder.ReadUint32(opDataset.mDelay));
            opDataset.mComponents.mIsDelayPresent = true;
            break;
        }

        case SPINEL_PROP_MAC_15_4_PANID:
        {
            SuccessOrExit(error = decoder.ReadUint16(opDataset.mPanId));
            opDataset.mComponents.mIsPanIdPresent = true;
            break;
        }

        case SPINEL_PROP_PHY_CHAN:
        {
            uint8_t channel;

            SuccessOrExit(error = decoder.ReadUint8(channel));
            opDataset.mChannel                      = channel;
            opDataset.mComponents.mIsChannelPresent = true;
            break;
        }

        case SPINEL_PROP_NET_PSKC:
        {
            const uint8_t *psk;
            uint16_t       len;

            SuccessOrExit(error = decoder.ReadData(psk, len));
            VerifyOrExit(len == OT_PSKC_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
            memcpy(opDataset.mPskc.m8, psk, OT_PSKC_MAX_SIZE);
            opDataset.mComponents.mIsPskcPresent = true;
            break;
        }

        case SPINEL_PROP_DATASET_SECURITY_POLICY:
        {
            uint8_t flags[2];
            uint8_t flagsLength = 1;

            SuccessOrExit(error = decoder.ReadUint16(opDataset.mSecurityPolicy.mRotationTime));
            SuccessOrExit(error = decoder.ReadUint8(flags[0]));
            if (otThreadGetVersion() >= OT_THREAD_VERSION_1_2 && decoder.GetRemainingLengthInStruct() > 0)
            {
                SuccessOrExit(error = decoder.ReadUint8(flags[1]));
                ++flagsLength;
            }
            static_cast<SecurityPolicy &>(opDataset.mSecurityPolicy).SetFlags(flags, flagsLength);
            opDataset.mComponents.mIsSecurityPolicyPresent = true;
            break;
        }

        case SPINEL_PROP_PHY_CHAN_SUPPORTED:
        {
            uint8_t channel;

            opDataset.mChannelMask = 0;

            while (!decoder.IsAllReadInStruct())
            {
                SuccessOrExit(error = decoder.ReadUint8(channel));
                VerifyOrExit(channel <= 31, error = OT_ERROR_INVALID_ARGS);
                opDataset.mChannelMask |= (1UL << channel);
            }
            opDataset.mComponents.mIsChannelMaskPresent = true;
            break;
        }

        default:
            break;
        }

        SuccessOrExit(error = decoder.CloseStruct());
    }

    /*
     * Initially set Active Timestamp to 0. This is to allow the node to join the network
     * yet retrieve the full Active Dataset from a neighboring device if one exists.
     */
    memset(&opDataset.mActiveTimestamp, 0, sizeof(opDataset.mActiveTimestamp));
    opDataset.mComponents.mIsActiveTimestampPresent = true;

    SuccessOrExit(error = dataset.SetFrom(static_cast<MeshCoP::Dataset::Info &>(opDataset)));
    SuccessOrExit(error = Instance::Get().template Get<SettingsDriver>().Set(
                      isActive ? SettingsBase::kKeyActiveDataset : SettingsBase::kKeyPendingDataset, dataset.GetBytes(),
                      dataset.GetSize()));

exit:
    return error;
}
#endif // #if !OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::HandleWaitingResponse(uint32_t          aCommand,
                                                                           spinel_prop_key_t aKey,
                                                                           const uint8_t    *aBuffer,
                                                                           uint16_t          aLength)
{
    if (aKey == SPINEL_PROP_LAST_STATUS)
    {
        spinel_status_t status;
        spinel_ssize_t  unpacked = spinel_datatype_unpack(aBuffer, aLength, "i", &status);

        VerifyOrExit(unpacked > 0, mError = OT_ERROR_PARSE);
        mError = SpinelStatusToOtError(status);
    }
#if OPENTHREAD_CONFIG_DIAG_ENABLE
    else if (aKey == SPINEL_PROP_NEST_STREAM_MFG)
    {
        spinel_ssize_t unpacked;

        mError = OT_ERROR_NONE;
        VerifyOrExit(mDiagOutput != nullptr);
        unpacked =
            spinel_datatype_unpack_in_place(aBuffer, aLength, SPINEL_DATATYPE_UTF8_S, mDiagOutput, &mDiagOutputMaxLen);
        VerifyOrExit(unpacked > 0, mError = OT_ERROR_PARSE);
    }
#endif
    else if (aKey == mWaitingKey)
    {
        if (mPropertyFormat)
        {
            if (static_cast<spinel_datatype_t>(mPropertyFormat[0]) == SPINEL_DATATYPE_VOID_C)
            {
                // reserved SPINEL_DATATYPE_VOID_C indicate caller want to parse the spinel response itself
                ResponseHandler handler = va_arg(mPropertyArgs, ResponseHandler);

                assert(handler != nullptr);
                mError = (this->*handler)(aBuffer, aLength);
            }
            else
            {
                spinel_ssize_t unpacked =
                    spinel_datatype_vunpack_in_place(aBuffer, aLength, mPropertyFormat, mPropertyArgs);

                VerifyOrExit(unpacked > 0, mError = OT_ERROR_PARSE);
                mError = OT_ERROR_NONE;
            }
        }
        else
        {
            if (aCommand == mExpectedCommand)
            {
                mError = OT_ERROR_NONE;
            }
            else
            {
                mError = OT_ERROR_DROP;
            }
        }
    }
    else
    {
        mError = OT_ERROR_DROP;
    }

exit:
    UpdateParseErrorCount(mError);
    LogIfFail("Error processing result", mError);
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::HandleValueIs(spinel_prop_key_t aKey,
                                                                   const uint8_t    *aBuffer,
                                                                   uint16_t          aLength)
{
    otError        error = OT_ERROR_NONE;
    spinel_ssize_t unpacked;

    if (aKey == SPINEL_PROP_STREAM_RAW)
    {
        SuccessOrExit(error = ParseRadioFrame(mRxRadioFrame, aBuffer, aLength, unpacked));
        RadioReceive();
    }
    else if (aKey == SPINEL_PROP_LAST_STATUS)
    {
        spinel_status_t status = SPINEL_STATUS_OK;

        unpacked = spinel_datatype_unpack(aBuffer, aLength, "i", &status);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        if (status >= SPINEL_STATUS_RESET__BEGIN && status <= SPINEL_STATUS_RESET__END)
        {
            if (IsEnabled())
            {
                HandleRcpUnexpectedReset(status);
                ExitNow();
            }

            otLogInfoPlat("RCP reset: %s", spinel_status_to_cstr(status));
            mIsReady = true;
        }
        else
        {
            otLogInfoPlat("RCP last status: %s", spinel_status_to_cstr(status));
        }
    }
    else if (aKey == SPINEL_PROP_MAC_ENERGY_SCAN_RESULT)
    {
        uint8_t scanChannel;
        int8_t  maxRssi;

        unpacked = spinel_datatype_unpack(aBuffer, aLength, "Cc", &scanChannel, &maxRssi);

        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
        mEnergyScanning = false;
#endif

        otPlatRadioEnergyScanDone(mInstance, maxRssi);
    }
    else if (aKey == SPINEL_PROP_STREAM_DEBUG)
    {
        char         logStream[OPENTHREAD_CONFIG_NCP_SPINEL_LOG_MAX_SIZE + 1];
        unsigned int len = sizeof(logStream);

        unpacked = spinel_datatype_unpack_in_place(aBuffer, aLength, SPINEL_DATATYPE_DATA_S, logStream, &len);
        assert(len < sizeof(logStream));
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        logStream[len] = '\0';
        otLogDebgPlat("RCP => %s", logStream);
    }
    else if ((aKey == SPINEL_PROP_STREAM_LOG) && mSupportsLogStream)
    {
        const char *logString;
        uint8_t     logLevel;

        unpacked = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_UTF8_S, &logString);
        VerifyOrExit(unpacked >= 0, error = OT_ERROR_PARSE);
        aBuffer += unpacked;
        aLength -= unpacked;

        unpacked = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_UINT8_S, &logLevel);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        switch (logLevel)
        {
        case SPINEL_NCP_LOG_LEVEL_EMERG:
        case SPINEL_NCP_LOG_LEVEL_ALERT:
        case SPINEL_NCP_LOG_LEVEL_CRIT:
            otLogCritPlat("RCP => %s", logString);
            break;

        case SPINEL_NCP_LOG_LEVEL_ERR:
        case SPINEL_NCP_LOG_LEVEL_WARN:
            otLogWarnPlat("RCP => %s", logString);
            break;

        case SPINEL_NCP_LOG_LEVEL_NOTICE:
            otLogNotePlat("RCP => %s", logString);
            break;

        case SPINEL_NCP_LOG_LEVEL_INFO:
            otLogInfoPlat("RCP => %s", logString);
            break;

        case SPINEL_NCP_LOG_LEVEL_DEBUG:
        default:
            otLogDebgPlat("RCP => %s", logString);
            break;
        }
    }

exit:
    UpdateParseErrorCount(error);
    LogIfFail("Failed to handle ValueIs", error);
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::ParseRadioFrame(otRadioFrame   &aFrame,
                                                                        const uint8_t  *aBuffer,
                                                                        uint16_t        aLength,
                                                                        spinel_ssize_t &aUnpacked)
{
    otError        error        = OT_ERROR_NONE;
    uint16_t       flags        = 0;
    int8_t         noiseFloor   = -128;
    spinel_size_t  size         = OT_RADIO_FRAME_MAX_SIZE;
    unsigned int   receiveError = 0;
    spinel_ssize_t unpacked;

    VerifyOrExit(aLength > 0, aFrame.mLength = 0);

    unpacked = spinel_datatype_unpack_in_place(aBuffer, aLength,
                                               SPINEL_DATATYPE_DATA_WLEN_S                          // Frame
                                                   SPINEL_DATATYPE_INT8_S                           // RSSI
                                                       SPINEL_DATATYPE_INT8_S                       // Noise Floor
                                                           SPINEL_DATATYPE_UINT16_S                 // Flags
                                                               SPINEL_DATATYPE_STRUCT_S(            // PHY-data
                                                                   SPINEL_DATATYPE_UINT8_S          // 802.15.4 channel
                                                                       SPINEL_DATATYPE_UINT8_S      // 802.15.4 LQI
                                                                           SPINEL_DATATYPE_UINT64_S // Timestamp (us).
                                                                   ) SPINEL_DATATYPE_STRUCT_S(      // Vendor-data
                                                                   SPINEL_DATATYPE_UINT_PACKED_S    // Receive error
                                                                   ),
                                               aFrame.mPsdu, &size, &aFrame.mInfo.mRxInfo.mRssi, &noiseFloor, &flags,
                                               &aFrame.mChannel, &aFrame.mInfo.mRxInfo.mLqi,
                                               &aFrame.mInfo.mRxInfo.mTimestamp, &receiveError);

    VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
    aUnpacked = unpacked;

    aBuffer += unpacked;
    aLength -= static_cast<uint16_t>(unpacked);

    if (mRadioCaps & OT_RADIO_CAPS_TRANSMIT_SEC)
    {
        unpacked =
            spinel_datatype_unpack_in_place(aBuffer, aLength,
                                            SPINEL_DATATYPE_STRUCT_S(        // MAC-data
                                                SPINEL_DATATYPE_UINT8_S      // Security key index
                                                    SPINEL_DATATYPE_UINT32_S // Security frame counter
                                                ),
                                            &aFrame.mInfo.mRxInfo.mAckKeyId, &aFrame.mInfo.mRxInfo.mAckFrameCounter);

        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        aUnpacked += unpacked;
    }

    if (receiveError == OT_ERROR_NONE)
    {
        aFrame.mLength = static_cast<uint8_t>(size);

        aFrame.mInfo.mRxInfo.mAckedWithFramePending = ((flags & SPINEL_MD_FLAG_ACKED_FP) != 0);
        aFrame.mInfo.mRxInfo.mAckedWithSecEnhAck    = ((flags & SPINEL_MD_FLAG_ACKED_SEC) != 0);
    }
    else if (receiveError < OT_NUM_ERRORS)
    {
        error = static_cast<otError>(receiveError);
    }
    else
    {
        error = OT_ERROR_PARSE;
    }

exit:
    UpdateParseErrorCount(error);
    LogIfFail("Handle radio frame failed", error);
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::ProcessFrameQueue(void)
{
    uint8_t *frame = nullptr;
    uint16_t length;

    while (mRxFrameBuffer.GetNextSavedFrame(frame, length) == OT_ERROR_NONE)
    {
        HandleNotification(frame, length);
    }

    mRxFrameBuffer.ClearSavedFrames();
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::RadioReceive(void)
{
    if (!mIsPromiscuous)
    {
        switch (mState)
        {
        case kStateDisabled:
        case kStateSleep:
            ExitNow();

        case kStateReceive:
        case kStateTransmitting:
        case kStateTransmitDone:
            break;
        }
    }

#if OPENTHREAD_CONFIG_DIAG_ENABLE
    if (otPlatDiagModeGet())
    {
        otPlatDiagRadioReceiveDone(mInstance, &mRxRadioFrame, OT_ERROR_NONE);
    }
    else
#endif
    {
        otPlatRadioReceiveDone(mInstance, &mRxRadioFrame, OT_ERROR_NONE);
    }

exit:
    return;
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::TransmitDone(otRadioFrame *aFrame,
                                                                  otRadioFrame *aAckFrame,
                                                                  otError       aError)
{
#if OPENTHREAD_CONFIG_DIAG_ENABLE
    if (otPlatDiagModeGet())
    {
        otPlatDiagRadioTransmitDone(mInstance, aFrame, aError);
    }
    else
#endif
    {
        otPlatRadioTxDone(mInstance, aFrame, aAckFrame, aError);
    }
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::ProcessRadioStateMachine(void)
{
    if (mState == kStateTransmitDone)
    {
        mState        = kStateReceive;
        mTxRadioEndUs = UINT64_MAX;

        TransmitDone(mTransmitFrame, (mAckRadioFrame.mLength != 0) ? &mAckRadioFrame : nullptr, mTxError);
    }
    else if (mState == kStateTransmitting && otPlatTimeGet() >= mTxRadioEndUs)
    {
        // Frame has been successfully passed to radio, but no `TransmitDone` event received within TX_WAIT_US.
        otLogWarnPlat("radio tx timeout");
        HandleRcpTimeout();
    }
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::Process(const ProcessContextType &aContext)
{
    if (mRxFrameBuffer.HasSavedFrame())
    {
        ProcessFrameQueue();
        RecoverFromRcpFailure();
    }

    GetSpinelInterface().Process(aContext);
    RecoverFromRcpFailure();

    if (mRxFrameBuffer.HasSavedFrame())
    {
        ProcessFrameQueue();
        RecoverFromRcpFailure();
    }

    ProcessRadioStateMachine();
    RecoverFromRcpFailure();
    CalcRcpTimeOffset();
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SetPromiscuous(bool aEnable)
{
    otError error;

    uint8_t mode = (aEnable ? SPINEL_MAC_PROMISCUOUS_MODE_NETWORK : SPINEL_MAC_PROMISCUOUS_MODE_OFF);
    SuccessOrExit(error = Set(SPINEL_PROP_MAC_PROMISCUOUS_MODE, SPINEL_DATATYPE_UINT8_S, mode));
    mIsPromiscuous = aEnable;

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SetShortAddress(uint16_t aAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mShortAddress != aAddress);
    SuccessOrExit(error = Set(SPINEL_PROP_MAC_15_4_SADDR, SPINEL_DATATYPE_UINT16_S, aAddress));
    mShortAddress = aAddress;

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SetMacKey(uint8_t                 aKeyIdMode,
                                                                  uint8_t                 aKeyId,
                                                                  const otMacKeyMaterial *aPrevKey,
                                                                  const otMacKeyMaterial *aCurrKey,
                                                                  const otMacKeyMaterial *aNextKey)
{
    otError error;
    size_t  aKeySize;

    VerifyOrExit((aPrevKey != nullptr) && (aCurrKey != nullptr) && (aNextKey != nullptr), error = kErrorInvalidArgs);

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    SuccessOrExit(error = otPlatCryptoExportKey(aPrevKey->mKeyMaterial.mKeyRef, aPrevKey->mKeyMaterial.mKey.m8,
                                                sizeof(aPrevKey->mKeyMaterial.mKey.m8), &aKeySize));
    SuccessOrExit(error = otPlatCryptoExportKey(aCurrKey->mKeyMaterial.mKeyRef, aCurrKey->mKeyMaterial.mKey.m8,
                                                sizeof(aCurrKey->mKeyMaterial.mKey.m8), &aKeySize));
    SuccessOrExit(error = otPlatCryptoExportKey(aNextKey->mKeyMaterial.mKeyRef, aNextKey->mKeyMaterial.mKey.m8,
                                                sizeof(aNextKey->mKeyMaterial.mKey.m8), &aKeySize));
#else
    OT_UNUSED_VARIABLE(aKeySize);
#endif

    SuccessOrExit(error = Set(SPINEL_PROP_RCP_MAC_KEY,
                              SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_DATA_WLEN_S
                                  SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_DATA_WLEN_S,
                              aKeyIdMode, aKeyId, aPrevKey->mKeyMaterial.mKey.m8, sizeof(otMacKey),
                              aCurrKey->mKeyMaterial.mKey.m8, sizeof(otMacKey), aNextKey->mKeyMaterial.mKey.m8,
                              sizeof(otMacKey)));

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    mKeyIdMode = aKeyIdMode;
    mKeyId     = aKeyId;
    memcpy(mPrevKey.m8, aPrevKey->mKeyMaterial.mKey.m8, OT_MAC_KEY_SIZE);
    memcpy(mCurrKey.m8, aCurrKey->mKeyMaterial.mKey.m8, OT_MAC_KEY_SIZE);
    memcpy(mNextKey.m8, aNextKey->mKeyMaterial.mKey.m8, OT_MAC_KEY_SIZE);
    mMacKeySet = true;
#endif

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SetMacFrameCounter(uint32_t aMacFrameCounter, bool aSetIfLarger)
{
    otError error;

    SuccessOrExit(error = Set(SPINEL_PROP_RCP_MAC_FRAME_COUNTER, SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_BOOL_S,
                              aMacFrameCounter, aSetIfLarger));

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::GetIeeeEui64(uint8_t *aIeeeEui64)
{
    memcpy(aIeeeEui64, mIeeeEui64.m8, sizeof(mIeeeEui64.m8));

    return OT_ERROR_NONE;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SetExtendedAddress(const otExtAddress &aExtAddress)
{
    otError error;

    SuccessOrExit(error = Set(SPINEL_PROP_MAC_15_4_LADDR, SPINEL_DATATYPE_EUI64_S, aExtAddress.m8));
    mExtendedAddress = aExtAddress;

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SetPanId(uint16_t aPanId)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mPanId != aPanId);
    SuccessOrExit(error = Set(SPINEL_PROP_MAC_15_4_PANID, SPINEL_DATATYPE_UINT16_S, aPanId));
    mPanId = aPanId;

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::EnableSrcMatch(bool aEnable)
{
    return Set(SPINEL_PROP_MAC_SRC_MATCH_ENABLED, SPINEL_DATATYPE_BOOL_S, aEnable);
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::AddSrcMatchShortEntry(uint16_t aShortAddress)
{
    otError error;

    SuccessOrExit(error = Insert(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, SPINEL_DATATYPE_UINT16_S, aShortAddress));

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    assert(mSrcMatchShortEntryCount < OPENTHREAD_CONFIG_MLE_MAX_CHILDREN);

    for (int i = 0; i < mSrcMatchShortEntryCount; ++i)
    {
        if (mSrcMatchShortEntries[i] == aShortAddress)
        {
            ExitNow();
        }
    }
    mSrcMatchShortEntries[mSrcMatchShortEntryCount] = aShortAddress;
    ++mSrcMatchShortEntryCount;
#endif

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::AddSrcMatchExtEntry(const otExtAddress &aExtAddress)
{
    otError error;

    SuccessOrExit(error =
                      Insert(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, SPINEL_DATATYPE_EUI64_S, aExtAddress.m8));

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    assert(mSrcMatchExtEntryCount < OPENTHREAD_CONFIG_MLE_MAX_CHILDREN);

    for (int i = 0; i < mSrcMatchExtEntryCount; ++i)
    {
        if (memcmp(aExtAddress.m8, mSrcMatchExtEntries[i].m8, OT_EXT_ADDRESS_SIZE) == 0)
        {
            ExitNow();
        }
    }
    mSrcMatchExtEntries[mSrcMatchExtEntryCount] = aExtAddress;
    ++mSrcMatchExtEntryCount;
#endif

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::ClearSrcMatchShortEntry(uint16_t aShortAddress)
{
    otError error;

    SuccessOrExit(error = Remove(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, SPINEL_DATATYPE_UINT16_S, aShortAddress));

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    for (int i = 0; i < mSrcMatchShortEntryCount; ++i)
    {
        if (mSrcMatchShortEntries[i] == aShortAddress)
        {
            mSrcMatchShortEntries[i] = mSrcMatchShortEntries[mSrcMatchShortEntryCount - 1];
            --mSrcMatchShortEntryCount;
            break;
        }
    }
#endif

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::ClearSrcMatchExtEntry(const otExtAddress &aExtAddress)
{
    otError error;

    SuccessOrExit(error =
                      Remove(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, SPINEL_DATATYPE_EUI64_S, aExtAddress.m8));

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    for (int i = 0; i < mSrcMatchExtEntryCount; ++i)
    {
        if (memcmp(mSrcMatchExtEntries[i].m8, aExtAddress.m8, OT_EXT_ADDRESS_SIZE) == 0)
        {
            mSrcMatchExtEntries[i] = mSrcMatchExtEntries[mSrcMatchExtEntryCount - 1];
            --mSrcMatchExtEntryCount;
            break;
        }
    }
#endif

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::ClearSrcMatchShortEntries(void)
{
    otError error;

    SuccessOrExit(error = Set(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, nullptr));

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    mSrcMatchShortEntryCount = 0;
#endif

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::ClearSrcMatchExtEntries(void)
{
    otError error;

    SuccessOrExit(error = Set(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, nullptr));

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    mSrcMatchExtEntryCount = 0;
#endif

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::GetTransmitPower(int8_t &aPower)
{
    otError error = Get(SPINEL_PROP_PHY_TX_POWER, SPINEL_DATATYPE_INT8_S, &aPower);

    LogIfFail("Get transmit power failed", error);
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::GetCcaEnergyDetectThreshold(int8_t &aThreshold)
{
    otError error = Get(SPINEL_PROP_PHY_CCA_THRESHOLD, SPINEL_DATATYPE_INT8_S, &aThreshold);

    LogIfFail("Get CCA ED threshold failed", error);
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::GetFemLnaGain(int8_t &aGain)
{
    otError error = Get(SPINEL_PROP_PHY_FEM_LNA_GAIN, SPINEL_DATATYPE_INT8_S, &aGain);

    LogIfFail("Get FEM LNA gain failed", error);
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
int8_t RadioSpinel<InterfaceType, ProcessContextType>::GetRssi(void)
{
    int8_t  rssi  = OT_RADIO_RSSI_INVALID;
    otError error = Get(SPINEL_PROP_PHY_RSSI, SPINEL_DATATYPE_INT8_S, &rssi);

    LogIfFail("Get RSSI failed", error);
    return rssi;
}

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SetCoexEnabled(bool aEnabled)
{
    otError error;

    SuccessOrExit(error = Set(SPINEL_PROP_RADIO_COEX_ENABLE, SPINEL_DATATYPE_BOOL_S, aEnabled));

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    mCoexEnabled    = aEnabled;
    mCoexEnabledSet = true;
#endif

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
bool RadioSpinel<InterfaceType, ProcessContextType>::IsCoexEnabled(void)
{
    bool    enabled;
    otError error = Get(SPINEL_PROP_RADIO_COEX_ENABLE, SPINEL_DATATYPE_BOOL_S, &enabled);

    LogIfFail("Get Coex State failed", error);
    return enabled;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::GetCoexMetrics(otRadioCoexMetrics &aCoexMetrics)
{
    otError error;

    error = Get(SPINEL_PROP_RADIO_COEX_METRICS,
                SPINEL_DATATYPE_STRUCT_S(                                    // Tx Coex Metrics Structure
                    SPINEL_DATATYPE_UINT32_S                                 // NumTxRequest
                        SPINEL_DATATYPE_UINT32_S                             // NumTxGrantImmediate
                            SPINEL_DATATYPE_UINT32_S                         // NumTxGrantWait
                                SPINEL_DATATYPE_UINT32_S                     // NumTxGrantWaitActivated
                                    SPINEL_DATATYPE_UINT32_S                 // NumTxGrantWaitTimeout
                                        SPINEL_DATATYPE_UINT32_S             // NumTxGrantDeactivatedDuringRequest
                                            SPINEL_DATATYPE_UINT32_S         // NumTxDelayedGrant
                                                SPINEL_DATATYPE_UINT32_S     // AvgTxRequestToGrantTime
                    ) SPINEL_DATATYPE_STRUCT_S(                              // Rx Coex Metrics Structure
                    SPINEL_DATATYPE_UINT32_S                                 // NumRxRequest
                        SPINEL_DATATYPE_UINT32_S                             // NumRxGrantImmediate
                            SPINEL_DATATYPE_UINT32_S                         // NumRxGrantWait
                                SPINEL_DATATYPE_UINT32_S                     // NumRxGrantWaitActivated
                                    SPINEL_DATATYPE_UINT32_S                 // NumRxGrantWaitTimeout
                                        SPINEL_DATATYPE_UINT32_S             // NumRxGrantDeactivatedDuringRequest
                                            SPINEL_DATATYPE_UINT32_S         // NumRxDelayedGrant
                                                SPINEL_DATATYPE_UINT32_S     // AvgRxRequestToGrantTime
                                                    SPINEL_DATATYPE_UINT32_S // NumRxGrantNone
                    ) SPINEL_DATATYPE_BOOL_S                                 // Stopped
                    SPINEL_DATATYPE_UINT32_S,                                // NumGrantGlitch
                &aCoexMetrics.mNumTxRequest, &aCoexMetrics.mNumTxGrantImmediate, &aCoexMetrics.mNumTxGrantWait,
                &aCoexMetrics.mNumTxGrantWaitActivated, &aCoexMetrics.mNumTxGrantWaitTimeout,
                &aCoexMetrics.mNumTxGrantDeactivatedDuringRequest, &aCoexMetrics.mNumTxDelayedGrant,
                &aCoexMetrics.mAvgTxRequestToGrantTime, &aCoexMetrics.mNumRxRequest, &aCoexMetrics.mNumRxGrantImmediate,
                &aCoexMetrics.mNumRxGrantWait, &aCoexMetrics.mNumRxGrantWaitActivated,
                &aCoexMetrics.mNumRxGrantWaitTimeout, &aCoexMetrics.mNumRxGrantDeactivatedDuringRequest,
                &aCoexMetrics.mNumRxDelayedGrant, &aCoexMetrics.mAvgRxRequestToGrantTime, &aCoexMetrics.mNumRxGrantNone,
                &aCoexMetrics.mStopped, &aCoexMetrics.mNumGrantGlitch);

    LogIfFail("Get Coex Metrics failed", error);
    return error;
}
#endif

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SetTransmitPower(int8_t aPower)
{
    otError error;

    SuccessOrExit(error = Set(SPINEL_PROP_PHY_TX_POWER, SPINEL_DATATYPE_INT8_S, aPower));

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    mTransmitPower    = aPower;
    mTransmitPowerSet = true;
#endif

exit:
    LogIfFail("Set transmit power failed", error);
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SetCcaEnergyDetectThreshold(int8_t aThreshold)
{
    otError error;

    SuccessOrExit(error = Set(SPINEL_PROP_PHY_CCA_THRESHOLD, SPINEL_DATATYPE_INT8_S, aThreshold));

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    mCcaEnergyDetectThreshold    = aThreshold;
    mCcaEnergyDetectThresholdSet = true;
#endif

exit:
    LogIfFail("Set CCA ED threshold failed", error);
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SetFemLnaGain(int8_t aGain)
{
    otError error;

    SuccessOrExit(error = Set(SPINEL_PROP_PHY_FEM_LNA_GAIN, SPINEL_DATATYPE_INT8_S, aGain));

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    mFemLnaGain    = aGain;
    mFemLnaGainSet = true;
#endif

exit:
    LogIfFail("Set FEM LNA gain failed", error);
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration)
{
    otError error;

    VerifyOrExit(mRadioCaps & OT_RADIO_CAPS_ENERGY_SCAN, error = OT_ERROR_NOT_CAPABLE);

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    mScanChannel    = aScanChannel;
    mScanDuration   = aScanDuration;
    mEnergyScanning = true;
#endif

    SuccessOrExit(error = Set(SPINEL_PROP_MAC_SCAN_MASK, SPINEL_DATATYPE_DATA_S, &aScanChannel, sizeof(uint8_t)));
    SuccessOrExit(error = Set(SPINEL_PROP_MAC_SCAN_PERIOD, SPINEL_DATATYPE_UINT16_S, aScanDuration));
    SuccessOrExit(error = Set(SPINEL_PROP_MAC_SCAN_STATE, SPINEL_DATATYPE_UINT8_S, SPINEL_SCAN_STATE_ENERGY));

    mChannel = aScanChannel;

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::Get(spinel_prop_key_t aKey, const char *aFormat, ...)
{
    otError error;

    assert(mWaitingTid == 0);

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    do
    {
        RecoverFromRcpFailure();
#endif
        va_start(mPropertyArgs, aFormat);
        error = RequestWithPropertyFormatV(aFormat, SPINEL_CMD_PROP_VALUE_GET, aKey, nullptr, mPropertyArgs);
        va_end(mPropertyArgs);
#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    } while (mRcpFailed);
#endif

    return error;
}

// This is not a normal use case for VALUE_GET command and should be only used to get RCP timestamp with dummy payload
template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::GetWithParam(spinel_prop_key_t aKey,
                                                                     const uint8_t    *aParam,
                                                                     spinel_size_t     aParamSize,
                                                                     const char       *aFormat,
                                                                     ...)
{
    otError error;

    assert(mWaitingTid == 0);

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    do
    {
        RecoverFromRcpFailure();
#endif
        va_start(mPropertyArgs, aFormat);
        error = RequestWithPropertyFormat(aFormat, SPINEL_CMD_PROP_VALUE_GET, aKey, SPINEL_DATATYPE_DATA_S, aParam,
                                          aParamSize);
        va_end(mPropertyArgs);
#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    } while (mRcpFailed);
#endif

    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::Set(spinel_prop_key_t aKey, const char *aFormat, ...)
{
    otError error;

    assert(mWaitingTid == 0);

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    do
    {
        RecoverFromRcpFailure();
#endif
        va_start(mPropertyArgs, aFormat);
        error = RequestWithExpectedCommandV(SPINEL_CMD_PROP_VALUE_IS, SPINEL_CMD_PROP_VALUE_SET, aKey, aFormat,
                                            mPropertyArgs);
        va_end(mPropertyArgs);
#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    } while (mRcpFailed);
#endif

    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::Insert(spinel_prop_key_t aKey, const char *aFormat, ...)
{
    otError error;

    assert(mWaitingTid == 0);

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    do
    {
        RecoverFromRcpFailure();
#endif
        va_start(mPropertyArgs, aFormat);
        error = RequestWithExpectedCommandV(SPINEL_CMD_PROP_VALUE_INSERTED, SPINEL_CMD_PROP_VALUE_INSERT, aKey, aFormat,
                                            mPropertyArgs);
        va_end(mPropertyArgs);
#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    } while (mRcpFailed);
#endif

    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::Remove(spinel_prop_key_t aKey, const char *aFormat, ...)
{
    otError error;

    assert(mWaitingTid == 0);

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    do
    {
        RecoverFromRcpFailure();
#endif
        va_start(mPropertyArgs, aFormat);
        error = RequestWithExpectedCommandV(SPINEL_CMD_PROP_VALUE_REMOVED, SPINEL_CMD_PROP_VALUE_REMOVE, aKey, aFormat,
                                            mPropertyArgs);
        va_end(mPropertyArgs);
#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    } while (mRcpFailed);
#endif

    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::WaitResponse(bool aWaitingReset)
{
    uint64_t end = otPlatTimeGet() + kMaxWaitTime * US_PER_MS;

    otLogDebgPlat("Wait response: tid=%u key=%lu", mWaitingTid, ToUlong(mWaitingKey));

    do
    {
        uint64_t now;

        now = otPlatTimeGet();
        if ((end <= now) || (mSpinelInterface.WaitForFrame(end - now) != OT_ERROR_NONE))
        {
            otLogWarnPlat("Wait for response timeout");
            if (!aWaitingReset)
            {
                HandleRcpTimeout();
            }
            ExitNow(mError = OT_ERROR_NONE);
        }
    } while (mWaitingTid || !mIsReady);

    LogIfFail("Error waiting response", mError);
    // This indicates end of waiting response.
    mWaitingKey = SPINEL_PROP_LAST_STATUS;

exit:
    return mError;
}

template <typename InterfaceType, typename ProcessContextType>
spinel_tid_t RadioSpinel<InterfaceType, ProcessContextType>::GetNextTid(void)
{
    spinel_tid_t tid = mCmdNextTid;

    while (((1 << tid) & mCmdTidsInUse) != 0)
    {
        tid = SPINEL_GET_NEXT_TID(tid);

        if (tid == mCmdNextTid)
        {
            // We looped back to `mCmdNextTid` indicating that all
            // TIDs are in-use.

            ExitNow(tid = 0);
        }
    }

    mCmdTidsInUse |= (1 << tid);
    mCmdNextTid = SPINEL_GET_NEXT_TID(tid);

exit:
    return tid;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SendReset(uint8_t aResetType)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        buffer[kMaxSpinelFrame];
    spinel_ssize_t packed;

    // Pack the header, command and key
    packed = spinel_datatype_pack(buffer, sizeof(buffer), SPINEL_DATATYPE_COMMAND_S SPINEL_DATATYPE_UINT8_S,
                                  SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_CMD_RESET, aResetType);

    VerifyOrExit(packed > 0 && static_cast<size_t>(packed) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = mSpinelInterface.SendFrame(buffer, static_cast<uint16_t>(packed)));
    mWaitingKey = SPINEL_PROP_LAST_STATUS;
    LogSpinelFrame(buffer, static_cast<uint16_t>(packed), true);

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SendCommand(uint32_t          aCommand,
                                                                    spinel_prop_key_t aKey,
                                                                    spinel_tid_t      tid,
                                                                    const char       *aFormat,
                                                                    va_list           args)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        buffer[kMaxSpinelFrame];
    spinel_ssize_t packed;
    uint16_t       offset;

    // Pack the header, command and key
    packed = spinel_datatype_pack(buffer, sizeof(buffer), "Cii", SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0 | tid,
                                  aCommand, aKey);

    VerifyOrExit(packed > 0 && static_cast<size_t>(packed) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

    offset = static_cast<uint16_t>(packed);

    // Pack the data (if any)
    if (aFormat)
    {
        packed = spinel_datatype_vpack(buffer + offset, sizeof(buffer) - offset, aFormat, args);
        VerifyOrExit(packed > 0 && static_cast<size_t>(packed + offset) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

        offset += static_cast<uint16_t>(packed);
    }

    SuccessOrExit(error = mSpinelInterface.SendFrame(buffer, offset));
    LogSpinelFrame(buffer, offset, true);

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::RequestV(uint32_t          command,
                                                                 spinel_prop_key_t aKey,
                                                                 const char       *aFormat,
                                                                 va_list           aArgs)
{
    otError      error = OT_ERROR_NONE;
    spinel_tid_t tid   = GetNextTid();

    VerifyOrExit(tid > 0, error = OT_ERROR_BUSY);

    error = SendCommand(command, aKey, tid, aFormat, aArgs);
    SuccessOrExit(error);

    if (aKey == SPINEL_PROP_STREAM_RAW)
    {
        // not allowed to send another frame before the last frame is done.
        assert(mTxRadioTid == 0);
        VerifyOrExit(mTxRadioTid == 0, error = OT_ERROR_BUSY);
        mTxRadioTid = tid;
    }
    else
    {
        mWaitingKey = aKey;
        mWaitingTid = tid;
        error       = WaitResponse();
    }

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::Request(uint32_t          aCommand,
                                                                spinel_prop_key_t aKey,
                                                                const char       *aFormat,
                                                                ...)
{
    va_list args;
    va_start(args, aFormat);
    otError status = RequestV(aCommand, aKey, aFormat, args);
    va_end(args);
    return status;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::RequestWithPropertyFormat(const char       *aPropertyFormat,
                                                                                  uint32_t          aCommand,
                                                                                  spinel_prop_key_t aKey,
                                                                                  const char       *aFormat,
                                                                                  ...)
{
    otError error;
    va_list args;

    va_start(args, aFormat);
    error = RequestWithPropertyFormatV(aPropertyFormat, aCommand, aKey, aFormat, args);
    va_end(args);

    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::RequestWithPropertyFormatV(const char       *aPropertyFormat,
                                                                                   uint32_t          aCommand,
                                                                                   spinel_prop_key_t aKey,
                                                                                   const char       *aFormat,
                                                                                   va_list           aArgs)
{
    otError error;

    mPropertyFormat = aPropertyFormat;
    error           = RequestV(aCommand, aKey, aFormat, aArgs);
    mPropertyFormat = nullptr;

    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::RequestWithExpectedCommandV(uint32_t          aExpectedCommand,
                                                                                    uint32_t          aCommand,
                                                                                    spinel_prop_key_t aKey,
                                                                                    const char       *aFormat,
                                                                                    va_list           aArgs)
{
    otError error;

    mExpectedCommand = aExpectedCommand;
    error            = RequestV(aCommand, aKey, aFormat, aArgs);
    mExpectedCommand = SPINEL_CMD_NOOP;

    return error;
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::HandleTransmitDone(uint32_t          aCommand,
                                                                        spinel_prop_key_t aKey,
                                                                        const uint8_t    *aBuffer,
                                                                        uint16_t          aLength)
{
    otError         error         = OT_ERROR_NONE;
    spinel_status_t status        = SPINEL_STATUS_OK;
    bool            framePending  = false;
    bool            headerUpdated = false;
    spinel_ssize_t  unpacked;

    VerifyOrExit(aCommand == SPINEL_CMD_PROP_VALUE_IS && aKey == SPINEL_PROP_LAST_STATUS, error = OT_ERROR_FAILED);

    unpacked = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_UINT_PACKED_S, &status);
    VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

    aBuffer += unpacked;
    aLength -= static_cast<uint16_t>(unpacked);

    unpacked = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_BOOL_S, &framePending);
    VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

    aBuffer += unpacked;
    aLength -= static_cast<uint16_t>(unpacked);

    unpacked = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_BOOL_S, &headerUpdated);
    VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

    aBuffer += unpacked;
    aLength -= static_cast<uint16_t>(unpacked);

    if (status == SPINEL_STATUS_OK)
    {
        SuccessOrExit(error = ParseRadioFrame(mAckRadioFrame, aBuffer, aLength, unpacked));
        aBuffer += unpacked;
        aLength -= static_cast<uint16_t>(unpacked);
    }
    else
    {
        error = SpinelStatusToOtError(status);
    }

    static_cast<Mac::TxFrame *>(mTransmitFrame)->SetIsHeaderUpdated(headerUpdated);

    if ((mRadioCaps & OT_RADIO_CAPS_TRANSMIT_SEC) && headerUpdated &&
        static_cast<Mac::TxFrame *>(mTransmitFrame)->GetSecurityEnabled())
    {
        uint8_t  keyId;
        uint32_t frameCounter;

        // Replace transmit frame security key index and frame counter with the one filled by RCP
        unpacked = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT32_S, &keyId,
                                          &frameCounter);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        static_cast<Mac::TxFrame *>(mTransmitFrame)->SetKeyId(keyId);
        static_cast<Mac::TxFrame *>(mTransmitFrame)->SetFrameCounter(frameCounter);
    }

exit:
    mState   = kStateTransmitDone;
    mTxError = error;
    UpdateParseErrorCount(error);
    LogIfFail("Handle transmit done failed", error);
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::Transmit(otRadioFrame &aFrame)
{
    otError error = OT_ERROR_INVALID_STATE;

    VerifyOrExit(mState == kStateReceive || (mState == kStateSleep && (mRadioCaps & OT_RADIO_CAPS_SLEEP_TO_TX)));

    mTransmitFrame = &aFrame;

    // `otPlatRadioTxStarted()` is triggered immediately for now, which may be earlier than real started time.
    otPlatRadioTxStarted(mInstance, mTransmitFrame);

    error = Request(SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_STREAM_RAW,
                    SPINEL_DATATYPE_DATA_WLEN_S                                   // Frame data
                        SPINEL_DATATYPE_UINT8_S                                   // Channel
                            SPINEL_DATATYPE_UINT8_S                               // MaxCsmaBackoffs
                                SPINEL_DATATYPE_UINT8_S                           // MaxFrameRetries
                                    SPINEL_DATATYPE_BOOL_S                        // CsmaCaEnabled
                                        SPINEL_DATATYPE_BOOL_S                    // IsHeaderUpdated
                                            SPINEL_DATATYPE_BOOL_S                // IsARetx
                                                SPINEL_DATATYPE_BOOL_S            // SkipAes
                                                    SPINEL_DATATYPE_UINT32_S      // TxDelay
                                                        SPINEL_DATATYPE_UINT32_S, // TxDelayBaseTime
                    mTransmitFrame->mPsdu, mTransmitFrame->mLength, mTransmitFrame->mChannel,
                    mTransmitFrame->mInfo.mTxInfo.mMaxCsmaBackoffs, mTransmitFrame->mInfo.mTxInfo.mMaxFrameRetries,
                    mTransmitFrame->mInfo.mTxInfo.mCsmaCaEnabled, mTransmitFrame->mInfo.mTxInfo.mIsHeaderUpdated,
                    mTransmitFrame->mInfo.mTxInfo.mIsARetx, mTransmitFrame->mInfo.mTxInfo.mIsSecurityProcessed,
                    mTransmitFrame->mInfo.mTxInfo.mTxDelay, mTransmitFrame->mInfo.mTxInfo.mTxDelayBaseTime);

    if (error == OT_ERROR_NONE)
    {
        // Waiting for `TransmitDone` event.
        mState        = kStateTransmitting;
        mTxRadioEndUs = otPlatTimeGet() + TX_WAIT_US;
        mChannel      = mTransmitFrame->mChannel;
    }

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::Receive(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState != kStateDisabled, error = OT_ERROR_INVALID_STATE);

    if (mChannel != aChannel)
    {
        error = Set(SPINEL_PROP_PHY_CHAN, SPINEL_DATATYPE_UINT8_S, aChannel);
        SuccessOrExit(error);
        mChannel = aChannel;
    }

    if (mState == kStateSleep)
    {
        error = Set(SPINEL_PROP_MAC_RAW_STREAM_ENABLED, SPINEL_DATATYPE_BOOL_S, true);
        SuccessOrExit(error);
    }

    if (mTxRadioTid != 0)
    {
        FreeTid(mTxRadioTid);
        mTxRadioTid = 0;
    }

    mState = kStateReceive;

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::Sleep(void)
{
    otError error = OT_ERROR_NONE;

    switch (mState)
    {
    case kStateReceive:
        error = Set(SPINEL_PROP_MAC_RAW_STREAM_ENABLED, SPINEL_DATATYPE_BOOL_S, false);
        SuccessOrExit(error);

        mState = kStateSleep;
        break;

    case kStateSleep:
        break;

    default:
        error = OT_ERROR_INVALID_STATE;
        break;
    }

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::Enable(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!IsEnabled());

    mInstance = aInstance;

    SuccessOrExit(error = Set(SPINEL_PROP_PHY_ENABLED, SPINEL_DATATYPE_BOOL_S, true));
    SuccessOrExit(error = Set(SPINEL_PROP_MAC_15_4_PANID, SPINEL_DATATYPE_UINT16_S, mPanId));
    SuccessOrExit(error = Set(SPINEL_PROP_MAC_15_4_SADDR, SPINEL_DATATYPE_UINT16_S, mShortAddress));
    SuccessOrExit(error = Get(SPINEL_PROP_PHY_RX_SENSITIVITY, SPINEL_DATATYPE_INT8_S, &mRxSensitivity));

    mState = kStateSleep;

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("RadioSpinel enable: %s", otThreadErrorToString(error));
        error = OT_ERROR_FAILED;
    }

    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::Disable(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled());
    VerifyOrExit(mState == kStateSleep, error = OT_ERROR_INVALID_STATE);

    SuccessOrDie(Set(SPINEL_PROP_PHY_ENABLED, SPINEL_DATATYPE_BOOL_S, false));
    mState    = kStateDisabled;
    mInstance = nullptr;

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DIAG_ENABLE
template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::PlatDiagProcess(const char *aString,
                                                                        char       *aOutput,
                                                                        size_t      aOutputMaxLen)
{
    otError error;

    mDiagOutput       = aOutput;
    mDiagOutputMaxLen = aOutputMaxLen;

    error = Set(SPINEL_PROP_NEST_STREAM_MFG, SPINEL_DATATYPE_UTF8_S, aString);

    mDiagOutput       = nullptr;
    mDiagOutputMaxLen = 0;

    return error;
}
#endif

template <typename InterfaceType, typename ProcessContextType>
uint32_t RadioSpinel<InterfaceType, ProcessContextType>::GetRadioChannelMask(bool aPreferred)
{
    uint8_t        maskBuffer[kChannelMaskBufferSize];
    otError        error       = OT_ERROR_NONE;
    uint32_t       channelMask = 0;
    const uint8_t *maskData    = maskBuffer;
    spinel_size_t  maskLength  = sizeof(maskBuffer);

    SuccessOrDie(Get(aPreferred ? SPINEL_PROP_PHY_CHAN_PREFERRED : SPINEL_PROP_PHY_CHAN_SUPPORTED,
                     SPINEL_DATATYPE_DATA_S, maskBuffer, &maskLength));

    while (maskLength > 0)
    {
        uint8_t        channel;
        spinel_ssize_t unpacked;

        unpacked = spinel_datatype_unpack(maskData, maskLength, SPINEL_DATATYPE_UINT8_S, &channel);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_FAILED);
        VerifyOrExit(channel < kChannelMaskBufferSize, error = OT_ERROR_PARSE);
        channelMask |= (1UL << channel);

        maskData += unpacked;
        maskLength -= static_cast<spinel_size_t>(unpacked);
    }

    channelMask &= mMaxPowerTable.GetSupportedChannelMask();

exit:
    UpdateParseErrorCount(error);
    LogIfFail("Get radio channel mask failed", error);
    return channelMask;
}

template <typename InterfaceType, typename ProcessContextType>
otRadioState RadioSpinel<InterfaceType, ProcessContextType>::GetState(void) const
{
    static const otRadioState sOtRadioStateMap[] = {
        OT_RADIO_STATE_DISABLED, OT_RADIO_STATE_SLEEP,    OT_RADIO_STATE_RECEIVE,
        OT_RADIO_STATE_TRANSMIT, OT_RADIO_STATE_TRANSMIT,
    };

    return sOtRadioStateMap[mState];
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::CalcRcpTimeOffset(void)
{
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    otError        error = OT_ERROR_NONE;
    uint64_t       localTxTimestamp;
    uint64_t       localRxTimestamp;
    uint64_t       remoteTimestamp = 0;
    uint8_t        buffer[sizeof(remoteTimestamp)];
    spinel_ssize_t packed;

    /*
     * Use a modified Network Time Protocol(NTP) to calculate the time offset
     * Assume the time offset is D so that local can calculate remote time with,
     *         T' = T + D
     * Where T is the local time and T' is the remote time.
     * The time offset is calculated using timestamp measured at local and remote.
     *
     *              T0  P    P T2
     *  local time --+----+----+--->
     *                \   |   ^
     *              get\  |  /is
     *                  v | /
     * remote time -------+--------->
     *                    T1'
     *
     * Based on the assumptions,
     * 1. If the propagation time(P) from local to remote and from remote to local are same.
     * 2. Both the host and RCP can accurately measure the time they send or receive a message.
     * The degree to which these assumptions hold true determines the accuracy of the offset.
     * Then,
     *         T1' = T0 + P + D and T1' = T2 - P + D
     * Time offset can be calculated with,
     *         D = T1' - ((T0 + T2)/ 2)
     */

    VerifyOrExit(!mIsTimeSynced || (otPlatTimeGet() >= GetNextRadioTimeRecalcStart()));

    otLogDebgPlat("Trying to get RCP time offset");

    packed = spinel_datatype_pack(buffer, sizeof(buffer), SPINEL_DATATYPE_UINT64_S, remoteTimestamp);
    VerifyOrExit(packed > 0 && static_cast<size_t>(packed) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

    localTxTimestamp = otPlatTimeGet();

    // Dummy timestamp payload to make request length same as response
    error = GetWithParam(SPINEL_PROP_RCP_TIMESTAMP, buffer, static_cast<spinel_size_t>(packed),
                         SPINEL_DATATYPE_UINT64_S, &remoteTimestamp);

    localRxTimestamp = otPlatTimeGet();

    VerifyOrExit(error == OT_ERROR_NONE, mRadioTimeRecalcStart = localRxTimestamp);

    mRadioTimeOffset      = (remoteTimestamp - ((localRxTimestamp / 2) + (localTxTimestamp / 2)));
    mIsTimeSynced         = true;
    mRadioTimeRecalcStart = localRxTimestamp + OPENTHREAD_POSIX_CONFIG_RCP_TIME_SYNC_INTERVAL;

exit:
    LogIfFail("Error calculating RCP time offset: %s", error);
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
}

template <typename InterfaceType, typename ProcessContextType>
uint64_t RadioSpinel<InterfaceType, ProcessContextType>::GetNow(void)
{
    return (mIsTimeSynced) ? (otPlatTimeGet() + mRadioTimeOffset) : UINT64_MAX;
}

template <typename InterfaceType, typename ProcessContextType>
uint32_t RadioSpinel<InterfaceType, ProcessContextType>::GetBusSpeed(void) const
{
    return mSpinelInterface.GetBusSpeed();
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::HandleRcpUnexpectedReset(spinel_status_t aStatus)
{
    OT_UNUSED_VARIABLE(aStatus);

    mRadioSpinelMetrics.mRcpUnexpectedResetCount++;
    otLogCritPlat("Unexpected RCP reset: %s", spinel_status_to_cstr(aStatus));

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    mRcpFailed = true;
#else
    DieNow(OT_EXIT_RADIO_SPINEL_RESET);
#endif
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::HandleRcpTimeout(void)
{
    mRadioSpinelMetrics.mRcpTimeoutCount++;

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    mRcpFailed = true;
#else
    DieNow(OT_EXIT_RADIO_SPINEL_NO_RESPONSE);
#endif
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::RecoverFromRcpFailure(void)
{
#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
    constexpr int16_t kMaxFailureCount = OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT;
    State             recoveringState  = mState;

    if (!mRcpFailed)
    {
        ExitNow();
    }
    mRcpFailed = false;

    otLogWarnPlat("RCP failure detected");

    ++mRadioSpinelMetrics.mRcpRestorationCount;
    ++mRcpFailureCount;
    if (mRcpFailureCount > kMaxFailureCount)
    {
        otLogCritPlat("Too many rcp failures, exiting");
        DieNow(OT_EXIT_FAILURE);
    }

    otLogWarnPlat("Trying to recover (%d/%d)", mRcpFailureCount, kMaxFailureCount);

    mState = kStateDisabled;
    mRxFrameBuffer.Clear();
    mCmdTidsInUse = 0;
    mCmdNextTid   = 1;
    mTxRadioTid   = 0;
    mWaitingTid   = 0;
    mError        = OT_ERROR_NONE;
    mIsTimeSynced = false;

    ResetRcp();
    SuccessOrDie(Set(SPINEL_PROP_PHY_ENABLED, SPINEL_DATATYPE_BOOL_S, true));
    mState = kStateSleep;

    RestoreProperties();

    switch (recoveringState)
    {
    case kStateDisabled:
    case kStateSleep:
        break;
    case kStateReceive:
        SuccessOrDie(Set(SPINEL_PROP_MAC_RAW_STREAM_ENABLED, SPINEL_DATATYPE_BOOL_S, true));
        mState = kStateReceive;
        break;
    case kStateTransmitting:
    case kStateTransmitDone:
        SuccessOrDie(Set(SPINEL_PROP_MAC_RAW_STREAM_ENABLED, SPINEL_DATATYPE_BOOL_S, true));
        mTxError = OT_ERROR_ABORT;
        mState   = kStateTransmitDone;
        break;
    }

    if (mEnergyScanning)
    {
        SuccessOrDie(EnergyScan(mScanChannel, mScanDuration));
    }

    --mRcpFailureCount;
    otLogNotePlat("RCP recovery is done");

exit:
    return;
#endif // OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
}

#if OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0
template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::RestoreProperties(void)
{
    Settings::NetworkInfo networkInfo;

    SuccessOrDie(Set(SPINEL_PROP_MAC_15_4_PANID, SPINEL_DATATYPE_UINT16_S, mPanId));
    SuccessOrDie(Set(SPINEL_PROP_MAC_15_4_SADDR, SPINEL_DATATYPE_UINT16_S, mShortAddress));
    SuccessOrDie(Set(SPINEL_PROP_MAC_15_4_LADDR, SPINEL_DATATYPE_EUI64_S, mExtendedAddress.m8));
    SuccessOrDie(Set(SPINEL_PROP_PHY_CHAN, SPINEL_DATATYPE_UINT8_S, mChannel));

    if (mMacKeySet)
    {
        SuccessOrDie(Set(SPINEL_PROP_RCP_MAC_KEY,
                         SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_DATA_WLEN_S
                             SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_DATA_WLEN_S,
                         mKeyIdMode, mKeyId, mPrevKey.m8, sizeof(otMacKey), mCurrKey.m8, sizeof(otMacKey), mNextKey.m8,
                         sizeof(otMacKey)));
    }

    if (mInstance != nullptr)
    {
        SuccessOrDie(static_cast<Instance *>(mInstance)->template Get<Settings>().Read(networkInfo));
        SuccessOrDie(
            Set(SPINEL_PROP_RCP_MAC_FRAME_COUNTER, SPINEL_DATATYPE_UINT32_S, networkInfo.GetMacFrameCounter()));
    }

    for (int i = 0; i < mSrcMatchShortEntryCount; ++i)
    {
        SuccessOrDie(
            Insert(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, SPINEL_DATATYPE_UINT16_S, mSrcMatchShortEntries[i]));
    }

    for (int i = 0; i < mSrcMatchExtEntryCount; ++i)
    {
        SuccessOrDie(
            Insert(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, SPINEL_DATATYPE_EUI64_S, mSrcMatchExtEntries[i].m8));
    }

    if (mCcaEnergyDetectThresholdSet)
    {
        SuccessOrDie(Set(SPINEL_PROP_PHY_CCA_THRESHOLD, SPINEL_DATATYPE_INT8_S, mCcaEnergyDetectThreshold));
    }

    if (mTransmitPowerSet)
    {
        SuccessOrDie(Set(SPINEL_PROP_PHY_TX_POWER, SPINEL_DATATYPE_INT8_S, mTransmitPower));
    }

    if (mCoexEnabledSet)
    {
        SuccessOrDie(Set(SPINEL_PROP_RADIO_COEX_ENABLE, SPINEL_DATATYPE_BOOL_S, mCoexEnabled));
    }

    if (mFemLnaGainSet)
    {
        SuccessOrDie(Set(SPINEL_PROP_PHY_FEM_LNA_GAIN, SPINEL_DATATYPE_INT8_S, mFemLnaGain));
    }

#if OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
    for (uint8_t channel = Radio::kChannelMin; channel <= Radio::kChannelMax; channel++)
    {
        int8_t power = mMaxPowerTable.GetTransmitPower(channel);

        if (power != OT_RADIO_POWER_INVALID)
        {
            // Some old RCPs doesn't support max transmit power
            otError error = SetChannelMaxTransmitPower(channel, power);

            if (error != OT_ERROR_NONE && error != OT_ERROR_NOT_FOUND)
            {
                DieNow(OT_EXIT_FAILURE);
            }
        }
    }
#endif // OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE

    CalcRcpTimeOffset();
}
#endif // OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT > 0

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SetChannelMaxTransmitPower(uint8_t aChannel, int8_t aMaxPower)
{
    otError error = OT_ERROR_NONE;
    VerifyOrExit(aChannel >= Radio::kChannelMin && aChannel <= Radio::kChannelMax, error = OT_ERROR_INVALID_ARGS);
    mMaxPowerTable.SetTransmitPower(aChannel, aMaxPower);
    error = Set(SPINEL_PROP_PHY_CHAN_MAX_POWER, SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT8_S, aChannel, aMaxPower);

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SetRadioRegion(uint16_t aRegionCode)
{
    otError error;

    error = Set(SPINEL_PROP_PHY_REGION_CODE, SPINEL_DATATYPE_UINT16_S, aRegionCode);

    if (error == OT_ERROR_NONE)
    {
        otLogNotePlat("Set region code \"%c%c\" successfully", static_cast<char>(aRegionCode >> 8),
                      static_cast<char>(aRegionCode));
    }
    else
    {
        otLogWarnPlat("Failed to set region code \"%c%c\": %s", static_cast<char>(aRegionCode >> 8),
                      static_cast<char>(aRegionCode), otThreadErrorToString(error));
    }

    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::GetRadioRegion(uint16_t *aRegionCode)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aRegionCode != nullptr, error = OT_ERROR_INVALID_ARGS);
    error = Get(SPINEL_PROP_PHY_REGION_CODE, SPINEL_DATATYPE_UINT16_S, aRegionCode);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::ConfigureEnhAckProbing(otLinkMetrics        aLinkMetrics,
                                                                               const otShortAddress aShortAddress,
                                                                               const otExtAddress  &aExtAddress)
{
    otError error = OT_ERROR_NONE;
    uint8_t flags = 0;

    if (aLinkMetrics.mPduCount)
    {
        flags |= SPINEL_THREAD_LINK_METRIC_PDU_COUNT;
    }

    if (aLinkMetrics.mLqi)
    {
        flags |= SPINEL_THREAD_LINK_METRIC_LQI;
    }

    if (aLinkMetrics.mLinkMargin)
    {
        flags |= SPINEL_THREAD_LINK_METRIC_LINK_MARGIN;
    }

    if (aLinkMetrics.mRssi)
    {
        flags |= SPINEL_THREAD_LINK_METRIC_RSSI;
    }

    error =
        Set(SPINEL_PROP_RCP_ENH_ACK_PROBING, SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_EUI64_S SPINEL_DATATYPE_UINT8_S,
            aShortAddress, aExtAddress.m8, flags);

    return error;
}
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
template <typename InterfaceType, typename ProcessContextType>
uint8_t RadioSpinel<InterfaceType, ProcessContextType>::GetCslAccuracy(void)
{
    uint8_t accuracy = UINT8_MAX;
    otError error    = Get(SPINEL_PROP_RCP_CSL_ACCURACY, SPINEL_DATATYPE_UINT8_S, &accuracy);

    LogIfFail("Get CSL Accuracy failed", error);
    return accuracy;
}
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
template <typename InterfaceType, typename ProcessContextType>
uint8_t RadioSpinel<InterfaceType, ProcessContextType>::GetCslUncertainty(void)
{
    uint8_t uncertainty = UINT8_MAX;
    otError error       = Get(SPINEL_PROP_RCP_CSL_UNCERTAINTY, SPINEL_DATATYPE_UINT8_S, &uncertainty);

    LogIfFail("Get CSL Uncertainty failed", error);
    return uncertainty;
}
#endif

#if OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE
template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::AddCalibratedPower(uint8_t        aChannel,
                                                                           int16_t        aActualPower,
                                                                           const uint8_t *aRawPowerSetting,
                                                                           uint16_t       aRawPowerSettingLength)
{
    otError error;

    assert(aRawPowerSetting != nullptr);
    SuccessOrExit(error = Insert(SPINEL_PROP_PHY_CALIBRATED_POWER,
                                 SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT16_S SPINEL_DATATYPE_DATA_WLEN_S, aChannel,
                                 aActualPower, aRawPowerSetting, aRawPowerSettingLength));

exit:
    return error;
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::ClearCalibratedPowers(void)
{
    return Set(SPINEL_PROP_PHY_CALIBRATED_POWER, nullptr);
}

template <typename InterfaceType, typename ProcessContextType>
otError RadioSpinel<InterfaceType, ProcessContextType>::SetChannelTargetPower(uint8_t aChannel, int16_t aTargetPower)
{
    otError error = OT_ERROR_NONE;
    VerifyOrExit(aChannel >= Radio::kChannelMin && aChannel <= Radio::kChannelMax, error = OT_ERROR_INVALID_ARGS);
    error =
        Set(SPINEL_PROP_PHY_CHAN_TARGET_POWER, SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT16_S, aChannel, aTargetPower);

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE

template <typename InterfaceType, typename ProcessContextType>
uint32_t RadioSpinel<InterfaceType, ProcessContextType>::Snprintf(char *aDest, uint32_t aSize, const char *aFormat, ...)
{
    int     len;
    va_list args;

    va_start(args, aFormat);
    len = vsnprintf(aDest, static_cast<size_t>(aSize), aFormat, args);
    va_end(args);

    return (len < 0) ? 0 : Min(static_cast<uint32_t>(len), aSize - 1);
}

template <typename InterfaceType, typename ProcessContextType>
void RadioSpinel<InterfaceType, ProcessContextType>::LogSpinelFrame(const uint8_t *aFrame, uint16_t aLength, bool aTx)
{
    otError           error                               = OT_ERROR_NONE;
    char              buf[OPENTHREAD_CONFIG_LOG_MAX_SIZE] = {0};
    spinel_ssize_t    unpacked;
    uint8_t           header;
    uint32_t          cmd;
    spinel_prop_key_t key;
    uint8_t          *data;
    spinel_size_t     len;
    const char       *prefix = nullptr;
    char             *start  = buf;
    char             *end    = buf + sizeof(buf);

    VerifyOrExit(otLoggingGetLevel() >= OT_LOG_LEVEL_DEBG);

    prefix   = aTx ? "Sent spinel frame" : "Received spinel frame";
    unpacked = spinel_datatype_unpack(aFrame, aLength, "CiiD", &header, &cmd, &key, &data, &len);
    VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

    start += Snprintf(start, static_cast<uint32_t>(end - start), "%s, flg:0x%x, tid:%u, cmd:%s", prefix,
                      SPINEL_HEADER_GET_FLAG(header), SPINEL_HEADER_GET_TID(header), spinel_command_to_cstr(cmd));
    VerifyOrExit(cmd != SPINEL_CMD_RESET);

    start += Snprintf(start, static_cast<uint32_t>(end - start), ", key:%s", spinel_prop_key_to_cstr(key));
    VerifyOrExit(cmd != SPINEL_CMD_PROP_VALUE_GET);

    switch (key)
    {
    case SPINEL_PROP_LAST_STATUS:
    {
        spinel_status_t status;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT_PACKED_S, &status);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", status:%s", spinel_status_to_cstr(status));
    }
    break;

    case SPINEL_PROP_MAC_RAW_STREAM_ENABLED:
    case SPINEL_PROP_MAC_SRC_MATCH_ENABLED:
    case SPINEL_PROP_PHY_ENABLED:
    case SPINEL_PROP_RADIO_COEX_ENABLE:
    {
        bool enabled;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_BOOL_S, &enabled);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", enabled:%u", enabled);
    }
    break;

    case SPINEL_PROP_PHY_CCA_THRESHOLD:
    case SPINEL_PROP_PHY_FEM_LNA_GAIN:
    case SPINEL_PROP_PHY_RX_SENSITIVITY:
    case SPINEL_PROP_PHY_RSSI:
    case SPINEL_PROP_PHY_TX_POWER:
    {
        const char *name = nullptr;
        int8_t      value;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_INT8_S, &value);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        switch (key)
        {
        case SPINEL_PROP_PHY_TX_POWER:
            name = "power";
            break;
        case SPINEL_PROP_PHY_CCA_THRESHOLD:
            name = "threshold";
            break;
        case SPINEL_PROP_PHY_FEM_LNA_GAIN:
            name = "gain";
            break;
        case SPINEL_PROP_PHY_RX_SENSITIVITY:
            name = "sensitivity";
            break;
        case SPINEL_PROP_PHY_RSSI:
            name = "rssi";
            break;
        }

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", %s:%d", name, value);
    }
    break;

    case SPINEL_PROP_MAC_PROMISCUOUS_MODE:
    case SPINEL_PROP_MAC_SCAN_STATE:
    case SPINEL_PROP_PHY_CHAN:
    case SPINEL_PROP_RCP_CSL_ACCURACY:
    case SPINEL_PROP_RCP_CSL_UNCERTAINTY:
    {
        const char *name = nullptr;
        uint8_t     value;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT8_S, &value);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        switch (key)
        {
        case SPINEL_PROP_MAC_SCAN_STATE:
            name = "state";
            break;
        case SPINEL_PROP_RCP_CSL_ACCURACY:
            name = "accuracy";
            break;
        case SPINEL_PROP_RCP_CSL_UNCERTAINTY:
            name = "uncertainty";
            break;
        case SPINEL_PROP_MAC_PROMISCUOUS_MODE:
            name = "mode";
            break;
        case SPINEL_PROP_PHY_CHAN:
            name = "channel";
            break;
        }

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", %s:%u", name, value);
    }
    break;

    case SPINEL_PROP_MAC_15_4_PANID:
    case SPINEL_PROP_MAC_15_4_SADDR:
    case SPINEL_PROP_MAC_SCAN_PERIOD:
    case SPINEL_PROP_PHY_REGION_CODE:
    {
        const char *name = nullptr;
        uint16_t    value;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT16_S, &value);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        switch (key)
        {
        case SPINEL_PROP_MAC_SCAN_PERIOD:
            name = "period";
            break;
        case SPINEL_PROP_PHY_REGION_CODE:
            name = "region";
            break;
        case SPINEL_PROP_MAC_15_4_SADDR:
            name = "saddr";
            break;
        case SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES:
            name = "saddr";
            break;
        case SPINEL_PROP_MAC_15_4_PANID:
            name = "panid";
            break;
        }

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", %s:0x%04x", name, value);
    }
    break;

    case SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES:
    {
        uint16_t saddr;

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", saddr:");

        if (len < sizeof(saddr))
        {
            start += Snprintf(start, static_cast<uint32_t>(end - start), "none");
        }
        else
        {
            while (len >= sizeof(saddr))
            {
                unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT16_S, &saddr);
                VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
                data += unpacked;
                len -= static_cast<spinel_size_t>(unpacked);
                start += Snprintf(start, static_cast<uint32_t>(end - start), "0x%04x ", saddr);
            }
        }
    }
    break;

    case SPINEL_PROP_RCP_MAC_FRAME_COUNTER:
    case SPINEL_PROP_RCP_TIMESTAMP:
    {
        const char *name;
        uint32_t    value;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT32_S, &value);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        name = (key == SPINEL_PROP_RCP_TIMESTAMP) ? "timestamp" : "counter";
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", %s:%u", name, value);
    }
    break;

    case SPINEL_PROP_RADIO_CAPS:
    case SPINEL_PROP_RCP_API_VERSION:
    case SPINEL_PROP_RCP_MIN_HOST_API_VERSION:
    {
        const char  *name;
        unsigned int value;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT_PACKED_S, &value);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        switch (key)
        {
        case SPINEL_PROP_RADIO_CAPS:
            name = "caps";
            break;
        case SPINEL_PROP_RCP_API_VERSION:
            name = "version";
            break;
        case SPINEL_PROP_RCP_MIN_HOST_API_VERSION:
            name = "min-host-version";
            break;
        default:
            name = "";
            break;
        }

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", %s:%u", name, value);
    }
    break;

    case SPINEL_PROP_MAC_ENERGY_SCAN_RESULT:
    case SPINEL_PROP_PHY_CHAN_MAX_POWER:
    {
        const char *name;
        uint8_t     channel;
        int8_t      value;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT8_S, &channel, &value);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        name = (key == SPINEL_PROP_MAC_ENERGY_SCAN_RESULT) ? "rssi" : "power";
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", channel:%u, %s:%d", channel, name, value);
    }
    break;

    case SPINEL_PROP_CAPS:
    {
        unsigned int capability;

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", caps:");

        while (len > 0)
        {
            unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT_PACKED_S, &capability);
            VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
            data += unpacked;
            len -= static_cast<spinel_size_t>(unpacked);
            start += Snprintf(start, static_cast<uint32_t>(end - start), "%s ", spinel_capability_to_cstr(capability));
        }
    }
    break;

    case SPINEL_PROP_PROTOCOL_VERSION:
    {
        unsigned int major;
        unsigned int minor;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S,
                                          &major, &minor);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", major:%u, minor:%u", major, minor);
    }
    break;

    case SPINEL_PROP_PHY_CHAN_PREFERRED:
    case SPINEL_PROP_PHY_CHAN_SUPPORTED:
    {
        uint8_t        maskBuffer[kChannelMaskBufferSize];
        uint32_t       channelMask = 0;
        const uint8_t *maskData    = maskBuffer;
        spinel_size_t  maskLength  = sizeof(maskBuffer);

        unpacked = spinel_datatype_unpack_in_place(data, len, SPINEL_DATATYPE_DATA_S, maskBuffer, &maskLength);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        while (maskLength > 0)
        {
            uint8_t channel;

            unpacked = spinel_datatype_unpack(maskData, maskLength, SPINEL_DATATYPE_UINT8_S, &channel);
            VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
            VerifyOrExit(channel < kChannelMaskBufferSize, error = OT_ERROR_PARSE);
            channelMask |= (1UL << channel);

            maskData += unpacked;
            maskLength -= static_cast<spinel_size_t>(unpacked);
        }

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", channelMask:0x%08x", channelMask);
    }
    break;

    case SPINEL_PROP_NCP_VERSION:
    {
        const char *version;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UTF8_S, &version);
        VerifyOrExit(unpacked >= 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", version:%s", version);
    }
    break;

    case SPINEL_PROP_STREAM_RAW:
    {
        otRadioFrame frame;

        if (cmd == SPINEL_CMD_PROP_VALUE_IS)
        {
            uint16_t     flags;
            int8_t       noiseFloor;
            unsigned int receiveError;

            unpacked = spinel_datatype_unpack(data, len,
                                              SPINEL_DATATYPE_DATA_WLEN_S                          // Frame
                                                  SPINEL_DATATYPE_INT8_S                           // RSSI
                                                      SPINEL_DATATYPE_INT8_S                       // Noise Floor
                                                          SPINEL_DATATYPE_UINT16_S                 // Flags
                                                              SPINEL_DATATYPE_STRUCT_S(            // PHY-data
                                                                  SPINEL_DATATYPE_UINT8_S          // 802.15.4 channel
                                                                      SPINEL_DATATYPE_UINT8_S      // 802.15.4 LQI
                                                                          SPINEL_DATATYPE_UINT64_S // Timestamp (us).
                                                                  ) SPINEL_DATATYPE_STRUCT_S(      // Vendor-data
                                                                  SPINEL_DATATYPE_UINT_PACKED_S    // Receive error
                                                                  ),
                                              &frame.mPsdu, &frame.mLength, &frame.mInfo.mRxInfo.mRssi, &noiseFloor,
                                              &flags, &frame.mChannel, &frame.mInfo.mRxInfo.mLqi,
                                              &frame.mInfo.mRxInfo.mTimestamp, &receiveError);
            VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
            start += Snprintf(start, static_cast<uint32_t>(end - start), ", len:%u, rssi:%d ...", frame.mLength,
                              frame.mInfo.mRxInfo.mRssi);
            otLogDebgPlat("%s", buf);

            start = buf;
            start += Snprintf(start, static_cast<uint32_t>(end - start),
                              "... noise:%d, flags:0x%04x, channel:%u, lqi:%u, timestamp:%lu, rxerr:%u", noiseFloor,
                              flags, frame.mChannel, frame.mInfo.mRxInfo.mLqi,
                              static_cast<unsigned long>(frame.mInfo.mRxInfo.mTimestamp), receiveError);
        }
        else if (cmd == SPINEL_CMD_PROP_VALUE_SET)
        {
            bool csmaCaEnabled;
            bool isHeaderUpdated;
            bool isARetx;
            bool skipAes;

            unpacked = spinel_datatype_unpack(
                data, len,
                SPINEL_DATATYPE_DATA_WLEN_S                                   // Frame data
                    SPINEL_DATATYPE_UINT8_S                                   // Channel
                        SPINEL_DATATYPE_UINT8_S                               // MaxCsmaBackoffs
                            SPINEL_DATATYPE_UINT8_S                           // MaxFrameRetries
                                SPINEL_DATATYPE_BOOL_S                        // CsmaCaEnabled
                                    SPINEL_DATATYPE_BOOL_S                    // IsHeaderUpdated
                                        SPINEL_DATATYPE_BOOL_S                // IsARetx
                                            SPINEL_DATATYPE_BOOL_S            // SkipAes
                                                SPINEL_DATATYPE_UINT32_S      // TxDelay
                                                    SPINEL_DATATYPE_UINT32_S, // TxDelayBaseTime
                &frame.mPsdu, &frame.mLength, &frame.mChannel, &frame.mInfo.mTxInfo.mMaxCsmaBackoffs,
                &frame.mInfo.mTxInfo.mMaxFrameRetries, &csmaCaEnabled, &isHeaderUpdated, &isARetx, &skipAes,
                &frame.mInfo.mTxInfo.mTxDelay, &frame.mInfo.mTxInfo.mTxDelayBaseTime);

            VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
            start += Snprintf(start, static_cast<uint32_t>(end - start),
                              ", len:%u, channel:%u, maxbackoffs:%u, maxretries:%u ...", frame.mLength, frame.mChannel,
                              frame.mInfo.mTxInfo.mMaxCsmaBackoffs, frame.mInfo.mTxInfo.mMaxFrameRetries);
            otLogDebgPlat("%s", buf);

            start = buf;
            start += Snprintf(start, static_cast<uint32_t>(end - start),
                              "... csmaCaEnabled:%u, isHeaderUpdated:%u, isARetx:%u, skipAes:%u"
                              ", txDelay:%u, txDelayBase:%u",
                              csmaCaEnabled, isHeaderUpdated, isARetx, skipAes, frame.mInfo.mTxInfo.mTxDelay,
                              frame.mInfo.mTxInfo.mTxDelayBaseTime);
        }
    }
    break;

    case SPINEL_PROP_STREAM_DEBUG:
    {
        char          debugString[OPENTHREAD_CONFIG_NCP_SPINEL_LOG_MAX_SIZE + 1];
        spinel_size_t stringLength = sizeof(debugString);

        unpacked = spinel_datatype_unpack_in_place(data, len, SPINEL_DATATYPE_DATA_S, debugString, &stringLength);
        assert(stringLength < sizeof(debugString));
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        debugString[stringLength] = '\0';
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", debug:%s", debugString);
    }
    break;

    case SPINEL_PROP_STREAM_LOG:
    {
        const char *logString;
        uint8_t     logLevel;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UTF8_S, &logString);
        VerifyOrExit(unpacked >= 0, error = OT_ERROR_PARSE);
        data += unpacked;
        len -= static_cast<spinel_size_t>(unpacked);

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT8_S, &logLevel);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", level:%u, log:%s", logLevel, logString);
    }
    break;

    case SPINEL_PROP_NEST_STREAM_MFG:
    {
        const char *output;
        size_t      outputLen;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UTF8_S, &output, &outputLen);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", diag:%s", output);
    }
    break;

    case SPINEL_PROP_RCP_MAC_KEY:
    {
        uint8_t      keyIdMode;
        uint8_t      keyId;
        otMacKey     prevKey;
        unsigned int prevKeyLen = sizeof(otMacKey);
        otMacKey     currKey;
        unsigned int currKeyLen = sizeof(otMacKey);
        otMacKey     nextKey;
        unsigned int nextKeyLen = sizeof(otMacKey);

        unpacked = spinel_datatype_unpack(data, len,
                                          SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_DATA_WLEN_S
                                              SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_DATA_WLEN_S,
                                          &keyIdMode, &keyId, prevKey.m8, &prevKeyLen, currKey.m8, &currKeyLen,
                                          nextKey.m8, &nextKeyLen);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start),
                          ", keyIdMode:%u, keyId:%u, prevKey:***, currKey:***, nextKey:***", keyIdMode, keyId);
    }
    break;

    case SPINEL_PROP_HWADDR:
    case SPINEL_PROP_MAC_15_4_LADDR:
    {
        const char *name                    = nullptr;
        uint8_t     m8[OT_EXT_ADDRESS_SIZE] = {0};

        unpacked = spinel_datatype_unpack_in_place(data, len, SPINEL_DATATYPE_EUI64_S, &m8[0]);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        name = (key == SPINEL_PROP_HWADDR) ? "eui64" : "laddr";
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", %s:%02x%02x%02x%02x%02x%02x%02x%02x", name,
                          m8[0], m8[1], m8[2], m8[3], m8[4], m8[5], m8[6], m8[7]);
    }
    break;

    case SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES:
    {
        uint8_t m8[OT_EXT_ADDRESS_SIZE];

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", extaddr:");

        if (len < sizeof(m8))
        {
            start += Snprintf(start, static_cast<uint32_t>(end - start), "none");
        }
        else
        {
            while (len >= sizeof(m8))
            {
                unpacked = spinel_datatype_unpack_in_place(data, len, SPINEL_DATATYPE_EUI64_S, m8);
                VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
                data += unpacked;
                len -= static_cast<spinel_size_t>(unpacked);
                start += Snprintf(start, static_cast<uint32_t>(end - start), "%02x%02x%02x%02x%02x%02x%02x%02x ", m8[0],
                                  m8[1], m8[2], m8[3], m8[4], m8[5], m8[6], m8[7]);
            }
        }
    }
    break;

    case SPINEL_PROP_RADIO_COEX_METRICS:
    {
        otRadioCoexMetrics metrics;
        unpacked = spinel_datatype_unpack(
            data, len,
            SPINEL_DATATYPE_STRUCT_S(                                    // Tx Coex Metrics Structure
                SPINEL_DATATYPE_UINT32_S                                 // NumTxRequest
                    SPINEL_DATATYPE_UINT32_S                             // NumTxGrantImmediate
                        SPINEL_DATATYPE_UINT32_S                         // NumTxGrantWait
                            SPINEL_DATATYPE_UINT32_S                     // NumTxGrantWaitActivated
                                SPINEL_DATATYPE_UINT32_S                 // NumTxGrantWaitTimeout
                                    SPINEL_DATATYPE_UINT32_S             // NumTxGrantDeactivatedDuringRequest
                                        SPINEL_DATATYPE_UINT32_S         // NumTxDelayedGrant
                                            SPINEL_DATATYPE_UINT32_S     // AvgTxRequestToGrantTime
                ) SPINEL_DATATYPE_STRUCT_S(                              // Rx Coex Metrics Structure
                SPINEL_DATATYPE_UINT32_S                                 // NumRxRequest
                    SPINEL_DATATYPE_UINT32_S                             // NumRxGrantImmediate
                        SPINEL_DATATYPE_UINT32_S                         // NumRxGrantWait
                            SPINEL_DATATYPE_UINT32_S                     // NumRxGrantWaitActivated
                                SPINEL_DATATYPE_UINT32_S                 // NumRxGrantWaitTimeout
                                    SPINEL_DATATYPE_UINT32_S             // NumRxGrantDeactivatedDuringRequest
                                        SPINEL_DATATYPE_UINT32_S         // NumRxDelayedGrant
                                            SPINEL_DATATYPE_UINT32_S     // AvgRxRequestToGrantTime
                                                SPINEL_DATATYPE_UINT32_S // NumRxGrantNone
                ) SPINEL_DATATYPE_BOOL_S                                 // Stopped
                SPINEL_DATATYPE_UINT32_S,                                // NumGrantGlitch
            &metrics.mNumTxRequest, &metrics.mNumTxGrantImmediate, &metrics.mNumTxGrantWait,
            &metrics.mNumTxGrantWaitActivated, &metrics.mNumTxGrantWaitTimeout,
            &metrics.mNumTxGrantDeactivatedDuringRequest, &metrics.mNumTxDelayedGrant,
            &metrics.mAvgTxRequestToGrantTime, &metrics.mNumRxRequest, &metrics.mNumRxGrantImmediate,
            &metrics.mNumRxGrantWait, &metrics.mNumRxGrantWaitActivated, &metrics.mNumRxGrantWaitTimeout,
            &metrics.mNumRxGrantDeactivatedDuringRequest, &metrics.mNumRxDelayedGrant,
            &metrics.mAvgRxRequestToGrantTime, &metrics.mNumRxGrantNone, &metrics.mStopped, &metrics.mNumGrantGlitch);

        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        otLogDebgPlat("%s ...", buf);
        otLogDebgPlat(" txRequest:%lu", ToUlong(metrics.mNumTxRequest));
        otLogDebgPlat(" txGrantImmediate:%lu", ToUlong(metrics.mNumTxGrantImmediate));
        otLogDebgPlat(" txGrantWait:%lu", ToUlong(metrics.mNumTxGrantWait));
        otLogDebgPlat(" txGrantWaitActivated:%lu", ToUlong(metrics.mNumTxGrantWaitActivated));
        otLogDebgPlat(" txGrantWaitTimeout:%lu", ToUlong(metrics.mNumTxGrantWaitTimeout));
        otLogDebgPlat(" txGrantDeactivatedDuringRequest:%lu", ToUlong(metrics.mNumTxGrantDeactivatedDuringRequest));
        otLogDebgPlat(" txDelayedGrant:%lu", ToUlong(metrics.mNumTxDelayedGrant));
        otLogDebgPlat(" avgTxRequestToGrantTime:%lu", ToUlong(metrics.mAvgTxRequestToGrantTime));
        otLogDebgPlat(" rxRequest:%lu", ToUlong(metrics.mNumRxRequest));
        otLogDebgPlat(" rxGrantImmediate:%lu", ToUlong(metrics.mNumRxGrantImmediate));
        otLogDebgPlat(" rxGrantWait:%lu", ToUlong(metrics.mNumRxGrantWait));
        otLogDebgPlat(" rxGrantWaitActivated:%lu", ToUlong(metrics.mNumRxGrantWaitActivated));
        otLogDebgPlat(" rxGrantWaitTimeout:%lu", ToUlong(metrics.mNumRxGrantWaitTimeout));
        otLogDebgPlat(" rxGrantDeactivatedDuringRequest:%lu", ToUlong(metrics.mNumRxGrantDeactivatedDuringRequest));
        otLogDebgPlat(" rxDelayedGrant:%lu", ToUlong(metrics.mNumRxDelayedGrant));
        otLogDebgPlat(" avgRxRequestToGrantTime:%lu", ToUlong(metrics.mAvgRxRequestToGrantTime));
        otLogDebgPlat(" rxGrantNone:%lu", ToUlong(metrics.mNumRxGrantNone));
        otLogDebgPlat(" stopped:%u", metrics.mStopped);

        start = buf;
        start += Snprintf(start, static_cast<uint32_t>(end - start), " grantGlitch:%u", metrics.mNumGrantGlitch);
    }
    break;

    case SPINEL_PROP_MAC_SCAN_MASK:
    {
        constexpr uint8_t kNumChannels = 16;
        uint8_t           channels[kNumChannels];
        spinel_size_t     size;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_DATA_S, channels, &size);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", channels:");

        for (uint8_t i = 0; i < size; i++)
        {
            start += Snprintf(start, static_cast<uint32_t>(end - start), "%u ", channels[i]);
        }
    }
    break;

    case SPINEL_PROP_RCP_ENH_ACK_PROBING:
    {
        uint16_t saddr;
        uint8_t  m8[OT_EXT_ADDRESS_SIZE];
        uint8_t  flags;

        unpacked = spinel_datatype_unpack(
            data, len, SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_EUI64_S SPINEL_DATATYPE_UINT8_S, &saddr, m8, &flags);

        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start),
                          ", saddr:%04x, extaddr:%02x%02x%02x%02x%02x%02x%02x%02x, flags:0x%02x", saddr, m8[0], m8[1],
                          m8[2], m8[3], m8[4], m8[5], m8[6], m8[7], flags);
    }
    break;

    case SPINEL_PROP_PHY_CALIBRATED_POWER:
    {
        if (cmd == SPINEL_CMD_PROP_VALUE_INSERT)
        {
            uint8_t      channel;
            int16_t      actualPower;
            uint8_t     *rawPowerSetting;
            unsigned int rawPowerSettingLength;

            unpacked = spinel_datatype_unpack(
                data, len, SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT16_S SPINEL_DATATYPE_DATA_WLEN_S, &channel,
                &actualPower, &rawPowerSetting, &rawPowerSettingLength);
            VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

            start += Snprintf(start, static_cast<uint32_t>(end - start),
                              ", ch:%u, actualPower:%d, rawPowerSetting:", channel, actualPower);
            for (uint16_t i = 0; i < rawPowerSettingLength; i++)
            {
                start += Snprintf(start, static_cast<uint32_t>(end - start), "%02x", rawPowerSetting[i]);
            }
        }
    }
    break;

    case SPINEL_PROP_PHY_CHAN_TARGET_POWER:
    {
        uint8_t channel;
        int16_t targetPower;

        unpacked =
            spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT16_S, &channel, &targetPower);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", ch:%u, targetPower:%d", channel, targetPower);
    }
    break;
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        otLogDebgPlat("%s", buf);
    }
    else
    {
        otLogDebgPlat("%s, failed to parse spinel frame !", prefix);
    }

    return;
}

} // namespace Spinel
} // namespace ot
