/*
 *    Copyright (c) 2016-2017, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements general thread device required Spinel interface to the OpenThread stack.
 */

#include "ncp_base.hpp"

#include <stdarg.h>
#include <stdlib.h>

#include <openthread/diag.h>
#include <openthread/icmp6.h>
#include <openthread/link.h>
#include <openthread/ncp.h>
#include <openthread/platform/misc.h>
#include <openthread/platform/radio.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"

namespace ot {
namespace Ncp {

// ----------------------------------------------------------------------------
// MARK: Utility Functions
// ----------------------------------------------------------------------------

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
static bool HasOnly1BitSet(uint32_t aValue)
{
    return aValue != 0 && ((aValue & (aValue - 1)) == 0);
}

static uint8_t IndexOfMSB(uint32_t aValue)
{
    uint8_t index = 0;

    while (aValue >>= 1)
    {
        index++;
    }

    return index;
}
#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

spinel_status_t NcpBase::ThreadErrorToSpinelStatus(otError aError)
{
    spinel_status_t ret;

    switch (aError)
    {
    case OT_ERROR_NONE:
        ret = SPINEL_STATUS_OK;
        break;

    case OT_ERROR_FAILED:
        ret = SPINEL_STATUS_FAILURE;
        break;

    case OT_ERROR_DROP:
        ret = SPINEL_STATUS_DROPPED;
        break;

    case OT_ERROR_NO_BUFS:
        ret = SPINEL_STATUS_NOMEM;
        break;

    case OT_ERROR_BUSY:
        ret = SPINEL_STATUS_BUSY;
        break;

    case OT_ERROR_PARSE:
        ret = SPINEL_STATUS_PARSE_ERROR;
        break;

    case OT_ERROR_INVALID_ARGS:
        ret = SPINEL_STATUS_INVALID_ARGUMENT;
        break;

    case OT_ERROR_NOT_IMPLEMENTED:
        ret = SPINEL_STATUS_UNIMPLEMENTED;
        break;

    case OT_ERROR_INVALID_STATE:
        ret = SPINEL_STATUS_INVALID_STATE;
        break;

    case OT_ERROR_NO_ACK:
        ret = SPINEL_STATUS_NO_ACK;
        break;

    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
        ret = SPINEL_STATUS_CCA_FAILURE;
        break;

    case OT_ERROR_ALREADY:
        ret = SPINEL_STATUS_ALREADY;
        break;

    case OT_ERROR_NOT_FOUND:
        ret = SPINEL_STATUS_ITEM_NOT_FOUND;
        break;

    case OT_ERROR_DISABLED_FEATURE:
        ret = SPINEL_STATUS_INVALID_COMMAND_FOR_PROP;
        break;

    default:
        // Unknown error code. Wrap it as a Spinel status and return that.
        ret = static_cast<spinel_status_t>(SPINEL_STATUS_STACK_NATIVE__BEGIN + static_cast<uint32_t>(aError));
        break;
    }

    return ret;
}

static spinel_status_t ResetReasonToSpinelStatus(otPlatResetReason aReason)
{
    spinel_status_t ret;

    switch (aReason)
    {
    case OT_PLAT_RESET_REASON_POWER_ON:
        ret = SPINEL_STATUS_RESET_POWER_ON;
        break;

    case OT_PLAT_RESET_REASON_EXTERNAL:
        ret = SPINEL_STATUS_RESET_EXTERNAL;
        break;

    case OT_PLAT_RESET_REASON_SOFTWARE:
        ret = SPINEL_STATUS_RESET_SOFTWARE;
        break;

    case OT_PLAT_RESET_REASON_FAULT:
        ret = SPINEL_STATUS_RESET_FAULT;
        break;

    case OT_PLAT_RESET_REASON_CRASH:
        ret = SPINEL_STATUS_RESET_CRASH;
        break;

    case OT_PLAT_RESET_REASON_ASSERT:
        ret = SPINEL_STATUS_RESET_ASSERT;
        break;

    case OT_PLAT_RESET_REASON_WATCHDOG:
        ret = SPINEL_STATUS_RESET_WATCHDOG;
        break;

    case OT_PLAT_RESET_REASON_OTHER:
        ret = SPINEL_STATUS_RESET_OTHER;
        break;

    default:
        ret = SPINEL_STATUS_RESET_UNKNOWN;
        break;
    }

    return ret;
}

// ----------------------------------------------------------------------------
// MARK: Class Boilerplate
// ----------------------------------------------------------------------------

NcpBase *NcpBase::sNcpInstance = NULL;

NcpBase::NcpBase(Instance *aInstance)
    : mInstance(aInstance)
    , mTxFrameBuffer(mTxBuffer, sizeof(mTxBuffer))
    , mEncoder(mTxFrameBuffer)
    , mDecoder()
    , mHostPowerStateInProgress(false)
    , mLastStatus(SPINEL_STATUS_OK)
    , mScanChannelMask(OT_RADIO_SUPPORTED_CHANNELS)
    , mScanPeriod(200)
    , mDiscoveryScanJoinerFlag(false)
    , mDiscoveryScanEnableFiltering(false)
    , mDiscoveryScanPanId(0xffff)
    , mUpdateChangedPropsTask(*aInstance, &NcpBase::UpdateChangedProps, this)
    , mThreadChangedFlags(0)
    , mChangedPropsSet()
    , mHostPowerState(SPINEL_HOST_POWER_STATE_ONLINE)
    , mHostPowerReplyFrameTag(NcpFrameBuffer::kInvalidTag)
    , mHostPowerStateHeader(0)
    ,
#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    mAllowPeekDelegate(NULL)
    , mAllowPokeDelegate(NULL)
    ,
#endif
    mNextExpectedTid(0)
    , mResponseQueueHead(0)
    , mResponseQueueTail(0)
    , mAllowLocalNetworkDataChange(false)
    , mRequireJoinExistingNetwork(false)
    , mIsRawStreamEnabled(false)
    , mDisableStreamWrite(false)
    , mShouldEmitChildTableUpdate(false)
    ,
#if OPENTHREAD_FTD
    mPreferredRouteId(0)
    ,
#endif
#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    mCurTransmitTID(0)
    , mCurScanChannel(kInvalidScanChannel)
    , mSrcMatchEnabled(false)
    ,
#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
#if OPENTHREAD_MTD || OPENTHREAD_FTD
    mInboundSecureIpFrameCounter(0)
    , mInboundInsecureIpFrameCounter(0)
    , mOutboundSecureIpFrameCounter(0)
    , mOutboundInsecureIpFrameCounter(0)
    , mDroppedOutboundIpFrameCounter(0)
    , mDroppedInboundIpFrameCounter(0)
    ,
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
    mFramingErrorCounter(0)
    , mRxSpinelFrameCounter(0)
    , mRxSpinelOutOfOrderTidCounter(0)
    , mTxSpinelFrameCounter(0)
    , mDidInitialUpdates(false)
{
    assert(mInstance != NULL);

    sNcpInstance = this;

    mTxFrameBuffer.SetFrameRemovedCallback(&NcpBase::HandleFrameRemovedFromNcpBuffer, this);

    memset(&mResponseQueue, 0, sizeof(mResponseQueue));

#if OPENTHREAD_ENABLE_DHCP6_CLIENT
    memset(mDhcpAddresses, 0, sizeof(mDhcpAddresses));
#endif

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    otMessageQueueInit(&mMessageQueue);
    otSetStateChangedCallback(mInstance, &NcpBase::HandleStateChanged, this);
    otIp6SetReceiveCallback(mInstance, &NcpBase::HandleDatagramFromStack, this);
    otIp6SetReceiveFilterEnabled(mInstance, true);
#if OPENTHREAD_ENABLE_UDP_PROXY
    otUdpProxySetSender(mInstance, &NcpBase::HandleUdpProxyStream, this);
#endif
    otLinkSetPcapCallback(mInstance, &NcpBase::HandleRawFrame, static_cast<void *>(this));
    otIcmp6SetEchoMode(mInstance, OT_ICMP6_ECHO_HANDLER_DISABLED);
#if OPENTHREAD_FTD
    otThreadSetChildTableCallback(mInstance, &NcpBase::HandleChildTableChanged);
#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    memset(&mSteeringDataAddress, 0, sizeof(mSteeringDataAddress));
#endif
    otThreadRegisterParentResponseCallback(mInstance, &NcpBase::HandleParentResponseInfo, static_cast<void *>(this));
#endif // OPENTHREAD_FTD
#if OPENTHREAD_ENABLE_LEGACY
    mLegacyNodeDidJoin = false;
    mLegacyHandlers    = NULL;
    memset(mLegacyUlaPrefix, 0, sizeof(mLegacyUlaPrefix));
    memset(&mLegacyLastJoinedNode, 0, sizeof(mLegacyLastJoinedNode));
#endif
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
    mChangedPropsSet.AddLastStatus(SPINEL_STATUS_RESET_UNKNOWN);
    mUpdateChangedPropsTask.Post();
}

NcpBase *NcpBase::GetNcpInstance(void)
{
    return sNcpInstance;
}

// ----------------------------------------------------------------------------
// MARK: Serial Traffic Glue
// ----------------------------------------------------------------------------

NcpFrameBuffer::FrameTag NcpBase::GetLastOutboundFrameTag(void)
{
    return mTxFrameBuffer.InFrameGetLastTag();
}

void NcpBase::HandleReceive(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError      error  = OT_ERROR_NONE;
    uint8_t      header = 0;
    spinel_tid_t tid    = 0;

    mDisableStreamWrite = true;

    // Initialize the decoder with the newly received spinel frame.
    mDecoder.Init(aBuf, aBufLength);

    // Receiving any message from the host has the side effect of transitioning the host power state to online.
    mHostPowerState           = SPINEL_HOST_POWER_STATE_ONLINE;
    mHostPowerStateInProgress = false;

    // Skip if there is no header byte to read or this isn't a spinel frame.

    SuccessOrExit(mDecoder.ReadUint8(header));
    VerifyOrExit((SPINEL_HEADER_FLAG & header) == SPINEL_HEADER_FLAG);

    mRxSpinelFrameCounter++;

    // We only support IID zero for now.
    if (SPINEL_HEADER_GET_IID(header) != 0)
    {
        WriteLastStatusFrame(header, SPINEL_STATUS_INVALID_INTERFACE);
        ExitNow();
    }

    error = HandleCommand(header);

    if (error != OT_ERROR_NONE)
    {
        PrepareLastStatusResponse(header, ThreadErrorToSpinelStatus(error));
    }

    if (!IsResponseQueueEmpty())
    {
        // A response may have been prepared and queued for this command,
        // so we attempt to send/write any queued responses. Note that
        // if the response was prepared but cannot be sent now (not
        // enough buffer space available), it will be attempted again
        // from `HandleFrameRemovedFromNcpBuffer()` when buffer space
        // becomes available.

        IgnoreReturnValue(SendQueuedResponses());
    }

    // Check for out of sequence TIDs and update `mNextExpectedTid`,

    tid = SPINEL_HEADER_GET_TID(header);

    if ((mNextExpectedTid != 0) && (tid != mNextExpectedTid))
    {
        mRxSpinelOutOfOrderTidCounter++;
    }

    mNextExpectedTid = SPINEL_GET_NEXT_TID(tid);

exit:
    mDisableStreamWrite = false;
}

void NcpBase::HandleFrameRemovedFromNcpBuffer(void *                   aContext,
                                              NcpFrameBuffer::FrameTag aFrameTag,
                                              NcpFrameBuffer::Priority aPriority,
                                              NcpFrameBuffer *         aNcpBuffer)
{
    OT_UNUSED_VARIABLE(aNcpBuffer);
    OT_UNUSED_VARIABLE(aPriority);

    static_cast<NcpBase *>(aContext)->HandleFrameRemovedFromNcpBuffer(aFrameTag);
}

void NcpBase::HandleFrameRemovedFromNcpBuffer(NcpFrameBuffer::FrameTag aFrameTag)
{
    if (mHostPowerStateInProgress == true)
    {
        if (aFrameTag == mHostPowerReplyFrameTag)
        {
            mHostPowerStateInProgress = false;
        }
    }

    // A frame was removed from NCP TX buffer, so more space is now available.
    // We attempt to write/send any pending frames. Order of the checks
    // below is important: First any queued command responses, then
    // any queued IPv6 datagram messages, then any asynchronous property updates.
    // If a frame still can not fit in the available buffer, we exit immediately
    // and wait for next time this callback is invoked (when another frame is
    // removed and more buffer space becomes available).

    SuccessOrExit(SendQueuedResponses());

#if OPENTHREAD_ENABLE_NCP_VENDOR_HOOK
    VendorHandleFrameRemovedFromNcpBuffer(aFrameTag);
#endif

    // Check if `HOST_POWER_STATE` property update is required.

    if (mHostPowerStateHeader)
    {
        SuccessOrExit(WritePropertyValueIsFrame(mHostPowerStateHeader, SPINEL_PROP_HOST_POWER_STATE));

        mHostPowerStateHeader = 0;

        if (mHostPowerState != SPINEL_HOST_POWER_STATE_ONLINE)
        {
            mHostPowerReplyFrameTag   = GetLastOutboundFrameTag();
            mHostPowerStateInProgress = true;
        }
    }

#if OPENTHREAD_MTD || OPENTHREAD_FTD

    // Send any queued IPv6 datagram message.

    SuccessOrExit(SendQueuedDatagramMessages());
#endif

    // Send any unsolicited event-triggered property updates.

    UpdateChangedProps();

exit:
    return;
}

bool NcpBase::ShouldWakeHost(void)
{
    return (mHostPowerState != SPINEL_HOST_POWER_STATE_ONLINE && !mHostPowerStateInProgress);
}

bool NcpBase::ShouldDeferHostSend(void)
{
    return (mHostPowerState == SPINEL_HOST_POWER_STATE_DEEP_SLEEP && !mHostPowerStateInProgress);
}

void NcpBase::IncrementFrameErrorCounter(void)
{
    mFramingErrorCounter++;
}

otError NcpBase::StreamWrite(int aStreamId, const uint8_t *aDataPtr, int aDataLen)
{
    otError           error  = OT_ERROR_NONE;
    uint8_t           header = SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0;
    spinel_prop_key_t streamPropKey;

    if (aStreamId == 0)
    {
        streamPropKey = SPINEL_PROP_STREAM_DEBUG;
    }
    else
    {
        streamPropKey = static_cast<spinel_prop_key_t>(aStreamId);
    }

    VerifyOrExit(!mDisableStreamWrite, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!mChangedPropsSet.IsPropertyFiltered(streamPropKey), error = OT_ERROR_INVALID_STATE);

    // If there is a pending queued response we do not allow any new log
    // stream writes. This is to ensure that log messages can not continue
    // to use the NCP buffer space and block other spinel frames.

    VerifyOrExit(IsResponseQueueEmpty(), error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = mEncoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_IS, streamPropKey));
    SuccessOrExit(error = mEncoder.WriteData(aDataPtr, static_cast<uint16_t>(aDataLen)));
    SuccessOrExit(error = mEncoder.EndFrame());

exit:

    if (error == OT_ERROR_NO_BUFS)
    {
        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_NOMEM);
        mUpdateChangedPropsTask.Post();
    }

    return error;
}

uint8_t NcpBase::ConvertLogLevel(otLogLevel aLogLevel)
{
    uint8_t spinelLogLevel = SPINEL_NCP_LOG_LEVEL_EMERG;

    switch (aLogLevel)
    {
    case OT_LOG_LEVEL_NONE:
        spinelLogLevel = SPINEL_NCP_LOG_LEVEL_EMERG;
        break;

    case OT_LOG_LEVEL_CRIT:
        spinelLogLevel = SPINEL_NCP_LOG_LEVEL_CRIT;
        break;

    case OT_LOG_LEVEL_WARN:
        spinelLogLevel = SPINEL_NCP_LOG_LEVEL_WARN;
        break;

    case OT_LOG_LEVEL_NOTE:
        spinelLogLevel = SPINEL_NCP_LOG_LEVEL_NOTICE;
        break;

    case OT_LOG_LEVEL_INFO:
        spinelLogLevel = SPINEL_NCP_LOG_LEVEL_INFO;
        break;

    case OT_LOG_LEVEL_DEBG:
        spinelLogLevel = SPINEL_NCP_LOG_LEVEL_DEBUG;
        break;
    }

    return spinelLogLevel;
}

unsigned int NcpBase::ConvertLogRegion(otLogRegion aLogRegion)
{
    unsigned int spinelLogRegion = SPINEL_NCP_LOG_REGION_NONE;

    switch (aLogRegion)
    {
    case OT_LOG_REGION_API:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_API;
        break;

    case OT_LOG_REGION_MLE:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_MLE;
        break;

    case OT_LOG_REGION_ARP:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_ARP;
        break;

    case OT_LOG_REGION_NET_DATA:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_NET_DATA;
        break;

    case OT_LOG_REGION_ICMP:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_ICMP;
        break;

    case OT_LOG_REGION_IP6:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_IP6;
        break;

    case OT_LOG_REGION_MAC:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_MAC;
        break;

    case OT_LOG_REGION_MEM:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_MEM;
        break;

    case OT_LOG_REGION_NCP:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_NCP;
        break;

    case OT_LOG_REGION_MESH_COP:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_MESH_COP;
        break;

    case OT_LOG_REGION_NET_DIAG:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_NET_DIAG;
        break;

    case OT_LOG_REGION_PLATFORM:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_PLATFORM;
        break;

    case OT_LOG_REGION_COAP:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_COAP;
        break;

    case OT_LOG_REGION_CLI:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_CLI;
        break;

    case OT_LOG_REGION_CORE:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_CORE;
        break;

    case OT_LOG_REGION_UTIL:
        spinelLogRegion = SPINEL_NCP_LOG_REGION_OT_UTIL;
        break;
    }

    return spinelLogRegion;
}

void NcpBase::Log(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aLogString)
{
    otError error  = OT_ERROR_NONE;
    uint8_t header = SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0;

    VerifyOrExit(!mDisableStreamWrite);
    VerifyOrExit(!mChangedPropsSet.IsPropertyFiltered(SPINEL_PROP_STREAM_LOG));

    // If there is a pending queued response we do not allow any new log
    // stream writes. This is to ensure that log messages can not continue
    // to use the NCP buffer space and block other spinel frames.

    VerifyOrExit(IsResponseQueueEmpty(), error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = mEncoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_STREAM_LOG));
    SuccessOrExit(error = mEncoder.WriteUtf8(aLogString));
    SuccessOrExit(error = mEncoder.WriteUint8(ConvertLogLevel(aLogLevel)));
    SuccessOrExit(error = mEncoder.WriteUintPacked(ConvertLogRegion(aLogRegion)));
    SuccessOrExit(error = mEncoder.EndFrame());

exit:

    if (error == OT_ERROR_NO_BUFS)
    {
        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_NOMEM);
        mUpdateChangedPropsTask.Post();
    }
}

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE

void NcpBase::RegisterPeekPokeDelagates(otNcpDelegateAllowPeekPoke aAllowPeekDelegate,
                                        otNcpDelegateAllowPeekPoke aAllowPokeDelegate)
{
    mAllowPeekDelegate = aAllowPeekDelegate;
    mAllowPokeDelegate = aAllowPokeDelegate;
}

#endif // OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE

// ----------------------------------------------------------------------------
// MARK: Raw frame handling
// ----------------------------------------------------------------------------

void NcpBase::HandleRawFrame(const otRadioFrame *aFrame, void *aContext)
{
    static_cast<NcpBase *>(aContext)->HandleRawFrame(aFrame);
}

void NcpBase::HandleRawFrame(const otRadioFrame *aFrame)
{
    uint16_t flags  = 0;
    uint8_t  header = SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0;

    if (!mIsRawStreamEnabled)
    {
        goto exit;
    }

    if (aFrame->mDidTx)
    {
        flags |= SPINEL_MD_FLAG_TX;
    }

    // Append frame header and frame length
    SuccessOrExit(mEncoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_STREAM_RAW));
    SuccessOrExit(mEncoder.WriteUint16(aFrame->mLength));

    // Append the frame contents
    SuccessOrExit(mEncoder.WriteData(aFrame->mPsdu, aFrame->mLength));

    // Append metadata (rssi, etc)
    SuccessOrExit(mEncoder.WriteInt8(aFrame->mInfo.mRxInfo.mRssi)); // RSSI
    SuccessOrExit(mEncoder.WriteInt8(-128));                        // Noise floor (Currently unused)
    SuccessOrExit(mEncoder.WriteUint16(flags));                     // Flags

    SuccessOrExit(mEncoder.OpenStruct()); // PHY-data
    // Empty for now
    SuccessOrExit(mEncoder.CloseStruct());

    SuccessOrExit(mEncoder.OpenStruct()); // Vendor-data
    // Empty for now
    SuccessOrExit(mEncoder.CloseStruct());

    SuccessOrExit(mEncoder.EndFrame());

exit:
    return;
}

// ----------------------------------------------------------------------------
// MARK: Spinel Response Handling
// ----------------------------------------------------------------------------

uint8_t NcpBase::GetWrappedResponseQueueIndex(uint8_t aPosition)
{
    while (aPosition >= kResponseQueueSize)
    {
        aPosition -= kResponseQueueSize;
    }

    return aPosition;
}

otError NcpBase::PrepareResponse(uint8_t aHeader, bool aIsLastStatus, bool aIsGetResponse, unsigned int aKeyOrStatus)
{
    otError        error = OT_ERROR_NONE;
    spinel_tid_t   tid   = SPINEL_HEADER_GET_TID(aHeader);
    ResponseEntry *entry;

    if (tid == 0)
    {
        // No response is required for TID zero. But we may emit a
        // `LAST_STATUS` error status (if not filtered) for TID
        // zero (e.g., for a dropped `STREAM_NET` set command).

        if (aIsLastStatus)
        {
            mChangedPropsSet.AddLastStatus(static_cast<spinel_status_t>(aKeyOrStatus));
        }

        ExitNow();
    }

    if ((mResponseQueueTail - mResponseQueueHead) >= kResponseQueueSize)
    {
        // If there is no room a for a response, emit an unsolicited
        // `DROPPED` error status to indicate a spinel response was
        // dropped.

        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_DROPPED);

        ExitNow(error = OT_ERROR_NO_BUFS);
    }

    // Transaction IDs are expected to come in sequence, if however, we
    // get an out of sequence TID, check if we already have a response
    // queued for this TID and if so mark the old entry as deleted.

    if (tid != mNextExpectedTid)
    {
        for (uint8_t cur = mResponseQueueHead; cur < mResponseQueueTail; cur++)
        {
            entry = &mResponseQueue[GetWrappedResponseQueueIndex(cur)];

            if (entry->mIsInUse && (entry->mTid == tid))
            {
                // Entry is just marked here and will be removed
                // from `SendQueuedResponses()`.

                entry->mIsInUse = false;
                break;
            }
        }
    }

    // Add the new entry in the queue at tail.

    entry = &mResponseQueue[GetWrappedResponseQueueIndex(mResponseQueueTail)];

    entry->mTid             = tid;
    entry->mIsInUse         = true;
    entry->mIsLastStatus    = aIsLastStatus;
    entry->mIsGetResponse   = aIsGetResponse;
    entry->mPropKeyOrStatus = aKeyOrStatus;

    mResponseQueueTail++;

exit:
    return error;
}

otError NcpBase::SendQueuedResponses(void)
{
    otError error = OT_ERROR_NONE;

    while (mResponseQueueHead != mResponseQueueTail)
    {
        ResponseEntry &entry = mResponseQueue[mResponseQueueHead];

        if (entry.mIsInUse)
        {
            uint8_t header = SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0;

            header |= static_cast<uint8_t>(entry.mTid << SPINEL_HEADER_TID_SHIFT);

            if (entry.mIsLastStatus)
            {
                spinel_status_t status = static_cast<spinel_status_t>(entry.mPropKeyOrStatus);

                SuccessOrExit(error = WriteLastStatusFrame(header, status));
            }
            else
            {
                spinel_prop_key_t propKey = static_cast<spinel_prop_key_t>(entry.mPropKeyOrStatus);

                SuccessOrExit(error = WritePropertyValueIsFrame(header, propKey, entry.mIsGetResponse));
            }
        }

        // Remove the response entry.

        entry.mIsInUse = false;

        mResponseQueueHead++;

        if (mResponseQueueHead == kResponseQueueSize)
        {
            // Only when `head` wraps, the `tail` will be wrapped as well.
            //
            // This ensures that `tail` is always bigger than `head` and
            // `(tail - head)` to correctly give the number of items in
            // the queue.

            mResponseQueueHead = 0;
            mResponseQueueTail = GetWrappedResponseQueueIndex(mResponseQueueTail);
        }
    }

exit:
    return error;
}

// ----------------------------------------------------------------------------
// MARK: Property/Status Changed
// ----------------------------------------------------------------------------

void NcpBase::UpdateChangedProps(Tasklet &aTasklet)
{
    OT_UNUSED_VARIABLE(aTasklet);
    GetNcpInstance()->UpdateChangedProps();
}

void NcpBase::UpdateChangedProps(void)
{
    uint8_t                       numEntries;
    spinel_prop_key_t             propKey;
    const ChangedPropsSet::Entry *entry;

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    ProcessThreadChangedFlags();
#endif

    VerifyOrExit(!mChangedPropsSet.IsEmpty());

    entry = mChangedPropsSet.GetSupportedEntries(numEntries);

    for (uint8_t index = 0; index < numEntries; index++, entry++)
    {
        if (!mChangedPropsSet.IsEntryChanged(index))
        {
            continue;
        }

        propKey = entry->mPropKey;

        if (propKey == SPINEL_PROP_LAST_STATUS)
        {
            spinel_status_t status = entry->mStatus;

            if (status == SPINEL_STATUS_RESET_UNKNOWN)
            {
                status = ResetReasonToSpinelStatus(otPlatGetResetReason(mInstance));
            }

            SuccessOrExit(WriteLastStatusFrame(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, status));
        }
        else if (mDidInitialUpdates)
        {
            SuccessOrExit(WritePropertyValueIsFrame(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, propKey));
        }

        mChangedPropsSet.RemoveEntry(index);
        VerifyOrExit(!mChangedPropsSet.IsEmpty());
    }

exit:
    mDidInitialUpdates = true;
    return;
}

// ----------------------------------------------------------------------------
// MARK: Inbound Command Handler
// ----------------------------------------------------------------------------

otError NcpBase::HandleCommand(uint8_t aHeader)
{
    otError      error = OT_ERROR_NONE;
    unsigned int command;

    SuccessOrExit(error = mDecoder.ReadUintPacked(command));

    switch (command)
    {
    case SPINEL_CMD_NOOP:
        error = CommandHandler_NOOP(aHeader);
        break;

    case SPINEL_CMD_RESET:
        error = CommandHandler_RESET(aHeader);
        break;

    case SPINEL_CMD_PROP_VALUE_GET:
    case SPINEL_CMD_PROP_VALUE_SET:
    case SPINEL_CMD_PROP_VALUE_INSERT:
    case SPINEL_CMD_PROP_VALUE_REMOVE:
        error = CommandHandler_PROP_VALUE_update(aHeader, command);
        break;

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    case SPINEL_CMD_PEEK:
        error = CommandHandler_PEEK(aHeader);
        break;

    case SPINEL_CMD_POKE:
        error = CommandHandler_POKE(aHeader);
        break;
#endif

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    case SPINEL_CMD_NET_SAVE:
        error = CommandHandler_NET_SAVE(aHeader);
        break;

    case SPINEL_CMD_NET_CLEAR:
        error = CommandHandler_NET_CLEAR(aHeader);
        break;

    case SPINEL_CMD_NET_RECALL:
        error = CommandHandler_NET_RECALL(aHeader);
        break;
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

    default:

#if OPENTHREAD_ENABLE_NCP_VENDOR_HOOK
        if (command >= SPINEL_CMD_VENDOR__BEGIN && command < SPINEL_CMD_VENDOR__END)
        {
            error = VendorCommandHandler(aHeader, command);
            break;
        }
#endif

        error = PrepareLastStatusResponse(aHeader, SPINEL_STATUS_INVALID_COMMAND);
        break;
    }

exit:
    return error;
}

// ----------------------------------------------------------------------------
// MARK: Property Get/Set/Insert/Remove Commands
// ----------------------------------------------------------------------------

// Returns `true` and updates the `aError` on success.
bool NcpBase::HandlePropertySetForSpecialProperties(uint8_t aHeader, spinel_prop_key_t aKey, otError &aError)
{
    bool didHandle = true;

    // Here the properties that require special treatment are handled.
    // These properties are expected to form/write the response from
    // their set handler directly.

    switch (aKey)
    {
    case SPINEL_PROP_HOST_POWER_STATE:
        ExitNow(aError = HandlePropertySet_SPINEL_PROP_HOST_POWER_STATE(aHeader));

#if OPENTHREAD_ENABLE_DIAG
    case SPINEL_PROP_NEST_STREAM_MFG:
        ExitNow(aError = HandlePropertySet_SPINEL_PROP_NEST_STREAM_MFG(aHeader));
#endif

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    case SPINEL_PROP_THREAD_COMMISSIONER_ENABLED:
        ExitNow(aError = HandlePropertySet_SPINEL_PROP_THREAD_COMMISSIONER_ENABLED(aHeader));
#endif

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    case SPINEL_PROP_STREAM_RAW:
        ExitNow(aError = HandlePropertySet_SPINEL_PROP_STREAM_RAW(aHeader));
#endif

    default:
        didHandle = false;
        break;
    }

exit:
    return didHandle;
}

otError NcpBase::HandleCommandPropertySet(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError         error   = OT_ERROR_NONE;
    PropertyHandler handler = FindSetPropertyHandler(aKey);

    if (handler != NULL)
    {
        mDisableStreamWrite = false;
        error               = (this->*handler)();
        mDisableStreamWrite = true;
    }
    else
    {
        // If there is no "set" handler, check if this property is one of the
        // ones that require different treatment.

        bool didHandle = HandlePropertySetForSpecialProperties(aHeader, aKey, error);

        VerifyOrExit(!didHandle);

#if OPENTHREAD_ENABLE_NCP_VENDOR_HOOK
        if (aKey >= SPINEL_PROP_VENDOR__BEGIN && aKey < SPINEL_PROP_VENDOR__END)
        {
            mDisableStreamWrite = false;
            error               = VendorSetPropertyHandler(aKey);
            mDisableStreamWrite = true;

            // An `OT_ERROR_NOT_FOUND` status from vendor handler indicates
            // that it does not support the given property key. In that
            // case, `didHandle` is set to `false` so a `LAST_STATUS` with
            // `PROP_NOT_FOUND` is emitted. Otherwise, we fall through to
            // prepare the response.

            didHandle = (error != OT_ERROR_NOT_FOUND);
        }
#endif

        VerifyOrExit(didHandle, error = PrepareLastStatusResponse(aHeader, SPINEL_STATUS_PROP_NOT_FOUND));
    }

    if (error == OT_ERROR_NONE)
    {
        error = PrepareSetResponse(aHeader, aKey);
    }
    else
    {
        error = PrepareLastStatusResponse(aHeader, ThreadErrorToSpinelStatus(error));
    }

exit:
    return error;
}

otError NcpBase::HandleCommandPropertyInsertRemove(uint8_t aHeader, spinel_prop_key_t aKey, unsigned int aCommand)
{
    otError         error           = OT_ERROR_NONE;
    PropertyHandler handler         = NULL;
    unsigned int    responseCommand = 0;
    const uint8_t * valuePtr;
    uint16_t        valueLen;

    switch (aCommand)
    {
    case SPINEL_CMD_PROP_VALUE_INSERT:
        handler         = FindInsertPropertyHandler(aKey);
        responseCommand = SPINEL_CMD_PROP_VALUE_INSERTED;
        break;

    case SPINEL_CMD_PROP_VALUE_REMOVE:
        handler         = FindRemovePropertyHandler(aKey);
        responseCommand = SPINEL_CMD_PROP_VALUE_REMOVED;
        break;

    default:
        assert(false);
        break;
    }

    VerifyOrExit(handler != NULL, error = PrepareLastStatusResponse(aHeader, SPINEL_STATUS_PROP_NOT_FOUND));

    // Save current read position in the decoder. Read the entire
    // content as a data blob (which is used in forming the response
    // in case of success), then reset the read position back so
    // that the `PropertyHandler` method can parse the content.

    mDecoder.SavePosition();
    mDecoder.ReadData(valuePtr, valueLen);
    mDecoder.ResetToSaved();

    mDisableStreamWrite = false;

    error = (this->*handler)();

    mDisableStreamWrite = true;

    VerifyOrExit(error == OT_ERROR_NONE, error = PrepareLastStatusResponse(aHeader, ThreadErrorToSpinelStatus(error)));

    error = WritePropertyValueInsertedRemovedFrame(aHeader, responseCommand, aKey, valuePtr, valueLen);

    // If the full response cannot be written now, instead prepare
    // a `LAST_STATUS(STATUS_OK)` update as response.

    if (error != OT_ERROR_NONE)
    {
        error = PrepareLastStatusResponse(aHeader, SPINEL_STATUS_OK);
    }

exit:
    return error;
}

// ----------------------------------------------------------------------------
// MARK: Outbound Frame Methods
// ----------------------------------------------------------------------------

otError NcpBase::WriteLastStatusFrame(uint8_t aHeader, spinel_status_t aLastStatus)
{
    otError error = OT_ERROR_NONE;

    if (SPINEL_HEADER_GET_IID(aHeader) == 0)
    {
        mLastStatus = aLastStatus;
    }

    SuccessOrExit(error = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_LAST_STATUS));
    SuccessOrExit(error = mEncoder.WriteUintPacked(aLastStatus));
    SuccessOrExit(error = mEncoder.EndFrame());

exit:
    return error;
}

otError NcpBase::WritePropertyValueIsFrame(uint8_t aHeader, spinel_prop_key_t aPropKey, bool aIsGetResponse)
{
    otError         error   = OT_ERROR_NONE;
    PropertyHandler handler = FindGetPropertyHandler(aPropKey);

    if (handler != NULL)
    {
        SuccessOrExit(error = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PROP_VALUE_IS, aPropKey));
        SuccessOrExit(error = (this->*handler)());
        ExitNow(error = mEncoder.EndFrame());
    }

#if OPENTHREAD_ENABLE_NCP_VENDOR_HOOK
    if (aPropKey >= SPINEL_PROP_VENDOR__BEGIN && aPropKey < SPINEL_PROP_VENDOR__END)
    {
        SuccessOrExit(error = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PROP_VALUE_IS, aPropKey));

        error = VendorGetPropertyHandler(aPropKey);

        // An `OT_ERROR_NOT_FOUND` status from vendor handler indicates that
        // it did not support the given property key. In that case, we fall
        // through to prepare a `LAST_STATUS` response.

        if (error != OT_ERROR_NOT_FOUND)
        {
            SuccessOrExit(error);
            ExitNow(error = mEncoder.EndFrame());
        }
    }
#endif

    if (aIsGetResponse)
    {
        SuccessOrExit(error = WriteLastStatusFrame(aHeader, SPINEL_STATUS_PROP_NOT_FOUND));
    }
    else
    {
        // Send a STATUS_OK for "set" response to a property that
        // has no corresponding get handler.

        SuccessOrExit(error = WriteLastStatusFrame(aHeader, SPINEL_STATUS_OK));
    }

exit:
    return error;
}

otError NcpBase::WritePropertyValueInsertedRemovedFrame(uint8_t           aHeader,
                                                        unsigned int      aResponseCommand,
                                                        spinel_prop_key_t aPropKey,
                                                        const uint8_t *   aValuePtr,
                                                        uint16_t          aValueLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mEncoder.BeginFrame(aHeader, aResponseCommand, aPropKey));
    SuccessOrExit(error = mEncoder.WriteData(aValuePtr, aValueLen));
    SuccessOrExit(error = mEncoder.EndFrame());

exit:
    return error;
}

// ----------------------------------------------------------------------------
// MARK: Individual Command Handlers
// ----------------------------------------------------------------------------

otError NcpBase::CommandHandler_NOOP(uint8_t aHeader)
{
    return PrepareLastStatusResponse(aHeader, SPINEL_STATUS_OK);
}

otError NcpBase::CommandHandler_RESET(uint8_t aHeader)
{
    otError error = OT_ERROR_NONE;

    OT_UNUSED_VARIABLE(aHeader);

    // Signal a platform reset. If implemented, this function
    // shouldn't return.
    otInstanceReset(mInstance);

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    // We only get to this point if the
    // platform doesn't support resetting.
    // In such a case we fake it.

    otThreadSetEnabled(mInstance, false);
    otIp6SetEnabled(mInstance, false);
#endif

    error = WriteLastStatusFrame(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_STATUS_RESET_SOFTWARE);

    if (error != OT_ERROR_NONE)
    {
        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_RESET_UNKNOWN);
        mUpdateChangedPropsTask.Post();
    }

    return error;
}

otError NcpBase::CommandHandler_PROP_VALUE_update(uint8_t aHeader, unsigned int aCommand)
{
    otError      error   = OT_ERROR_NONE;
    unsigned int propKey = 0;

    error = mDecoder.ReadUintPacked(propKey);

    VerifyOrExit(error == OT_ERROR_NONE, error = PrepareLastStatusResponse(aHeader, ThreadErrorToSpinelStatus(error)));

    switch (aCommand)
    {
    case SPINEL_CMD_PROP_VALUE_GET:
        error = PrepareGetResponse(aHeader, static_cast<spinel_prop_key_t>(propKey));
        break;

    case SPINEL_CMD_PROP_VALUE_SET:
        error = HandleCommandPropertySet(aHeader, static_cast<spinel_prop_key_t>(propKey));
        break;

    case SPINEL_CMD_PROP_VALUE_INSERT:
    case SPINEL_CMD_PROP_VALUE_REMOVE:
        error = HandleCommandPropertyInsertRemove(aHeader, static_cast<spinel_prop_key_t>(propKey), aCommand);
        break;

    default:
        break;
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE

otError NcpBase::CommandHandler_PEEK(uint8_t aHeader)
{
    otError  parseError    = OT_ERROR_NONE;
    otError  responseError = OT_ERROR_NONE;
    uint32_t address;
    uint16_t count;

    SuccessOrExit(parseError = mDecoder.ReadUint32(address));
    SuccessOrExit(parseError = mDecoder.ReadUint16(count));

    VerifyOrExit(count != 0, parseError = OT_ERROR_INVALID_ARGS);

    if (mAllowPeekDelegate != NULL)
    {
        VerifyOrExit(mAllowPeekDelegate(address, count), parseError = OT_ERROR_INVALID_ARGS);
    }

    SuccessOrExit(responseError = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PEEK_RET));
    SuccessOrExit(responseError = mEncoder.WriteUint32(address));
    SuccessOrExit(responseError = mEncoder.WriteUint16(count));
    SuccessOrExit(responseError = mEncoder.WriteData(reinterpret_cast<const uint8_t *>(address), count));
    SuccessOrExit(responseError = mEncoder.EndFrame());

exit:
    if (parseError != OT_ERROR_NONE)
    {
        responseError = PrepareLastStatusResponse(aHeader, ThreadErrorToSpinelStatus(parseError));
    }

    return responseError;
}

otError NcpBase::CommandHandler_POKE(uint8_t aHeader)
{
    otError        parseError = OT_ERROR_NONE;
    uint32_t       address;
    uint16_t       count;
    const uint8_t *dataPtr = NULL;
    uint16_t       dataLen;

    SuccessOrExit(parseError = mDecoder.ReadUint32(address));
    SuccessOrExit(parseError = mDecoder.ReadUint16(count));
    SuccessOrExit(parseError = mDecoder.ReadData(dataPtr, dataLen));

    VerifyOrExit(count != 0, parseError = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(count <= dataLen, parseError = OT_ERROR_INVALID_ARGS);

    if (mAllowPokeDelegate != NULL)
    {
        VerifyOrExit(mAllowPokeDelegate(address, count), parseError = OT_ERROR_INVALID_ARGS);
    }

    memcpy(reinterpret_cast<uint8_t *>(address), dataPtr, count);

exit:
    return PrepareLastStatusResponse(aHeader, ThreadErrorToSpinelStatus(parseError));
}

#endif // OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE

    // ----------------------------------------------------------------------------
    // MARK: Individual Property Getters and Setters
    // ----------------------------------------------------------------------------

#if OPENTHREAD_ENABLE_DIAG

otError NcpBase::HandlePropertySet_SPINEL_PROP_NEST_STREAM_MFG(uint8_t aHeader)
{
    const char *string = NULL;
    char        output[OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE];
    otError     error = OT_ERROR_NONE;

    error = mDecoder.ReadUtf8(string);

    VerifyOrExit(error == OT_ERROR_NONE, error = WriteLastStatusFrame(aHeader, ThreadErrorToSpinelStatus(error)));

    output[sizeof(output) - 1] = '\0';
    otDiagProcessCmdLine(string, output, sizeof(output) - 1);

    // Prepare the response
    SuccessOrExit(error = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_NEST_STREAM_MFG));
    SuccessOrExit(error = mEncoder.WriteUtf8(output));
    SuccessOrExit(error = mEncoder.EndFrame());

exit:
    return error;
}

#endif // OPENTHREAD_ENABLE_DIAG

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_PHY_ENABLED>(void)
{
#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    return mEncoder.WriteBool(otLinkRawIsEnabled(mInstance));
#else
    return mEncoder.WriteBool(false);
#endif
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_PHY_CHAN>(void)
{
    return mEncoder.WriteUint8(otLinkGetChannel(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_PHY_CHAN>(void)
{
    unsigned int channel = 0;
    otError      error   = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUintPacked(channel));

    error = otLinkSetChannel(mInstance, static_cast<uint8_t>(channel));

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

    SuccessOrExit(error);

    // Make sure we are update the receiving channel if raw link is enabled and we have raw
    // stream enabled already
    if (otLinkRawIsEnabled(mInstance) && mIsRawStreamEnabled)
    {
        error = otLinkRawReceive(mInstance, &NcpBase::LinkRawReceiveDone);
    }

#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_PROMISCUOUS_MODE>(void)
{
    return mEncoder.WriteUint8(otPlatRadioGetPromiscuous(mInstance) ? SPINEL_MAC_PROMISCUOUS_MODE_FULL
                                                                    : SPINEL_MAC_PROMISCUOUS_MODE_OFF);
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MAC_PROMISCUOUS_MODE>(void)
{
    uint8_t mode  = 0;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(mode));

    switch (mode)
    {
    case SPINEL_MAC_PROMISCUOUS_MODE_OFF:
        otPlatRadioSetPromiscuous(mInstance, false);
        break;

    case SPINEL_MAC_PROMISCUOUS_MODE_NETWORK:
    case SPINEL_MAC_PROMISCUOUS_MODE_FULL:
        otPlatRadioSetPromiscuous(mInstance, true);
        break;

    default:
        error = OT_ERROR_INVALID_ARGS;
        break;
    }

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_15_4_PANID>(void)
{
    return mEncoder.WriteUint16(otLinkGetPanId(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MAC_15_4_PANID>(void)
{
    uint16_t panid;
    otError  error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint16(panid));

    error = otLinkSetPanId(mInstance, panid);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_15_4_LADDR>(void)
{
    return mEncoder.WriteEui64(*otLinkGetExtendedAddress(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MAC_15_4_LADDR>(void)
{
    const otExtAddress *extAddress;
    otError             error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadEui64(extAddress));

    error = otLinkSetExtendedAddress(mInstance, extAddress);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_15_4_SADDR>(void)
{
    return mEncoder.WriteUint16(otLinkGetShortAddress(mInstance));
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_RAW_STREAM_ENABLED>(void)
{
    return mEncoder.WriteBool(mIsRawStreamEnabled);
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MAC_RAW_STREAM_ENABLED>(void)
{
    bool    enabled = false;
    otError error   = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadBool(enabled));

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

    if (otLinkRawIsEnabled(mInstance))
    {
        if (enabled)
        {
            error = otLinkRawReceive(mInstance, &NcpBase::LinkRawReceiveDone);
        }
        else
        {
            error = otLinkRawSleep(mInstance);
        }
    }

#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

    mIsRawStreamEnabled = enabled;

exit:
    return error;
}

otError NcpBase::EncodeChannelMask(uint32_t aChannelMask)
{
    otError error = OT_ERROR_NONE;

    for (uint8_t i = 0; i < 32; i++)
    {
        if (0 != (aChannelMask & (1 << i)))
        {
            SuccessOrExit(error = mEncoder.WriteUint8(i));
        }
    }

exit:
    return error;
}

otError NcpBase::DecodeChannelMask(uint32_t &aChannelMask)
{
    otError error = OT_ERROR_NONE;
    uint8_t channel;

    aChannelMask = 0;

    while (!mDecoder.IsAllReadInStruct())
    {
        SuccessOrExit(error = mDecoder.ReadUint8(channel));
        VerifyOrExit(channel <= 31, error = OT_ERROR_INVALID_ARGS);
        aChannelMask |= (1U << channel);
    }

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_SCAN_MASK>(void)
{
    return EncodeChannelMask(mScanChannelMask);
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MAC_SCAN_MASK>(void)
{
    uint32_t newMask = 0;
    otError  error   = OT_ERROR_NONE;

    SuccessOrExit(error = DecodeChannelMask(newMask));
    mScanChannelMask = newMask;

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_SCAN_PERIOD>(void)
{
    return mEncoder.WriteUint16(mScanPeriod);
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MAC_SCAN_PERIOD>(void)
{
    return mDecoder.ReadUint16(mScanPeriod);
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_SCAN_STATE>(void)
{
    uint8_t scanState = SPINEL_SCAN_STATE_IDLE;

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

    if (otLinkRawIsEnabled(mInstance))
    {
        scanState = (mCurScanChannel == kInvalidScanChannel) ? SPINEL_SCAN_STATE_IDLE : SPINEL_SCAN_STATE_ENERGY;
    }
    else

#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

    {
#if OPENTHREAD_MTD || OPENTHREAD_FTD
        if (otLinkIsActiveScanInProgress(mInstance))
        {
            scanState = SPINEL_SCAN_STATE_BEACON;
        }
        else if (otLinkIsEnergyScanInProgress(mInstance))
        {
            scanState = SPINEL_SCAN_STATE_ENERGY;
        }
        else if (otThreadIsDiscoverInProgress(mInstance))
        {
            scanState = SPINEL_SCAN_STATE_DISCOVER;
        }
        else
        {
            scanState = SPINEL_SCAN_STATE_IDLE;
        }
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
    }

    return mEncoder.WriteUint8(scanState);
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MAC_SCAN_STATE>(void)
{
    uint8_t state = 0;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(state));

    switch (state)
    {
    case SPINEL_SCAN_STATE_IDLE:
        error = OT_ERROR_NONE;
        break;

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    case SPINEL_SCAN_STATE_BEACON:
        error = otLinkActiveScan(mInstance, mScanChannelMask, mScanPeriod, &HandleActiveScanResult_Jump, this);
        SuccessOrExit(error);
        break;
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

    case SPINEL_SCAN_STATE_ENERGY:
#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
        if (otLinkRawIsEnabled(mInstance))
        {
            uint8_t scanChannel;

            // Make sure we aren't already scanning and that we have
            // only 1 bit set for the channel mask.
            VerifyOrExit(mCurScanChannel == kInvalidScanChannel, error = OT_ERROR_INVALID_STATE);
            VerifyOrExit(HasOnly1BitSet(mScanChannelMask), error = OT_ERROR_INVALID_ARGS);

            scanChannel     = IndexOfMSB(mScanChannelMask);
            mCurScanChannel = (int8_t)scanChannel;

            error = otLinkRawEnergyScan(mInstance, scanChannel, mScanPeriod, LinkRawEnergyScanDone);
        }
        else
#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
        {
#if OPENTHREAD_MTD || OPENTHREAD_FTD
            error = otLinkEnergyScan(mInstance, mScanChannelMask, mScanPeriod, &HandleEnergyScanResult_Jump, this);
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
        }

        SuccessOrExit(error);
        break;

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    case SPINEL_SCAN_STATE_DISCOVER:
        error = otThreadDiscover(mInstance, mScanChannelMask, mDiscoveryScanPanId, mDiscoveryScanJoinerFlag,
                                 mDiscoveryScanEnableFiltering, &HandleActiveScanResult_Jump, this);

        SuccessOrExit(error);
        break;
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

    default:
        error = OT_ERROR_NOT_IMPLEMENTED;
        break;
    }

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_UNSOL_UPDATE_FILTER>(void)
{
    otError                       error = OT_ERROR_NONE;
    uint8_t                       numEntries;
    const ChangedPropsSet::Entry *entry;

    entry = mChangedPropsSet.GetSupportedEntries(numEntries);

    for (uint8_t index = 0; index < numEntries; index++, entry++)
    {
        if (mChangedPropsSet.IsEntryFiltered(index))
        {
            SuccessOrExit(error = mEncoder.WriteUintPacked(entry->mPropKey));
        }
    }

exit:
    return error;
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_UNSOL_UPDATE_FILTER>(void)
{
    unsigned int propKey;
    otError      error = OT_ERROR_NONE;

    // First clear the current filter.
    mChangedPropsSet.ClearFilter();

    while (mDecoder.GetRemainingLengthInStruct() > 0)
    {
        SuccessOrExit(error = mDecoder.ReadUintPacked(propKey));

        IgnoreReturnValue(mChangedPropsSet.EnablePropertyFilter(static_cast<spinel_prop_key_t>(propKey), true));
    }

exit:
    // If we had an error, we may have actually changed
    // the state of the filter, So we need to report
    // those incomplete changes via an asynchronous
    // change event.

    if (error != OT_ERROR_NONE)
    {
        WritePropertyValueIsFrame(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_PROP_UNSOL_UPDATE_FILTER);
    }

    return error;
}

template <> otError NcpBase::HandlePropertyInsert<SPINEL_PROP_UNSOL_UPDATE_FILTER>(void)
{
    otError      error = OT_ERROR_NONE;
    unsigned int propKey;

    SuccessOrExit(error = mDecoder.ReadUintPacked(propKey));

    error = mChangedPropsSet.EnablePropertyFilter(static_cast<spinel_prop_key_t>(propKey), true);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyRemove<SPINEL_PROP_UNSOL_UPDATE_FILTER>(void)
{
    otError      error = OT_ERROR_NONE;
    unsigned int propKey;

    SuccessOrExit(error = mDecoder.ReadUintPacked(propKey));

    error = mChangedPropsSet.EnablePropertyFilter(static_cast<spinel_prop_key_t>(propKey), false);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_LAST_STATUS>(void)
{
    return mEncoder.WriteUintPacked(mLastStatus);
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_PROTOCOL_VERSION>(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_PROTOCOL_VERSION_THREAD_MAJOR));
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_PROTOCOL_VERSION_THREAD_MINOR));

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_INTERFACE_TYPE>(void)
{
    return mEncoder.WriteUintPacked(SPINEL_PROTOCOL_TYPE_THREAD);
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_VENDOR_ID>(void)
{
    return mEncoder.WriteUintPacked(0); // Vendor ID. Zero for unknown.
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_CAPS>(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_COUNTERS));
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_UNSOL_UPDATE_FILTER));

#if OPENTHREAD_CONFIG_NCP_ENABLE_MCU_POWER_STATE_CONTROL
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_MCU_POWER_STATE));
#endif

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_MAC_RAW));
#endif

#if OPENTHREAD_ENABLE_POSIX_APP
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_POSIX_APP));
#endif

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL)
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_OPENTHREAD_LOG_METADATA));
#endif

#if OPENTHREAD_MTD || OPENTHREAD_FTD

    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_NET_THREAD_1_1));
#if OPENTHREAD_ENABLE_MAC_FILTER
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_MAC_WHITELIST));
#endif

#if OPENTHREAD_ENABLE_JAM_DETECTION
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_JAM_DETECT));
#endif

#if OPENTHREAD_ENABLE_CHILD_SUPERVISION
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_CHILD_SUPERVISION));
#endif

#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_CHANNEL_MONITOR));
#endif

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER && OPENTHREAD_FTD
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_CHANNEL_MANAGER));
#endif

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_TIME_SYNC));
#endif

#if OPENTHREAD_CONFIG_ENABLE_TX_ERROR_RATE_TRACKING
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_ERROR_RATE_TRACKING));
#endif

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_OOB_STEERING_DATA));
#endif

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_PEEK_POKE));
#endif

    // TODO: Somehow get the following capability from the radio.
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_802_15_4_2450MHZ_OQPSK));

#if OPENTHREAD_CONFIG_MAX_CHILDREN > 0
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_ROLE_ROUTER));
#endif

    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_ROLE_SLEEPY));

#if OPENTHREAD_ENABLE_LEGACY
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_NEST_LEGACY_INTERFACE));
#endif

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_THREAD_COMMISSIONER));
#endif

#if OPENTHREAD_ENABLE_JOINER
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_THREAD_JOINER));
#endif

#if OPENTHREAD_ENABLE_UDP_PROXY
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_THREAD_UDP_PROXY));
#endif

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_NCP_VERSION>(void)
{
    return mEncoder.WriteUtf8(otGetVersionString());
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_INTERFACE_COUNT>(void)
{
    return mEncoder.WriteUint8(1); // Only one interface for now
}

#if OPENTHREAD_CONFIG_NCP_ENABLE_MCU_POWER_STATE_CONTROL

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_MCU_POWER_STATE>(void)
{
    spinel_mcu_power_state_t state = SPINEL_MCU_POWER_STATE_ON;

    switch (otPlatGetMcuPowerState(mInstance))
    {
    case OT_PLAT_MCU_POWER_STATE_ON:
        state = SPINEL_MCU_POWER_STATE_ON;
        break;

    case OT_PLAT_MCU_POWER_STATE_LOW_POWER:
        state = SPINEL_MCU_POWER_STATE_LOW_POWER;
        break;

    case OT_PLAT_MCU_POWER_STATE_OFF:
        state = SPINEL_MCU_POWER_STATE_OFF;
        break;
    }

    return mEncoder.WriteUint8(state);
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MCU_POWER_STATE>(void)
{
    otError             error = OT_ERROR_NONE;
    otPlatMcuPowerState powerState;
    uint8_t             state;

    SuccessOrExit(error = mDecoder.ReadUint8(state));

    switch (state)
    {
    case SPINEL_MCU_POWER_STATE_ON:
        powerState = OT_PLAT_MCU_POWER_STATE_ON;
        break;

    case SPINEL_MCU_POWER_STATE_LOW_POWER:
        powerState = OT_PLAT_MCU_POWER_STATE_LOW_POWER;
        break;

    case SPINEL_MCU_POWER_STATE_OFF:
        powerState = OT_PLAT_MCU_POWER_STATE_OFF;
        break;

    default:
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    SuccessOrExit(error = otPlatSetMcuPowerState(mInstance, powerState));

#if OPENTHREAD_FTD || OPENTHREAD_MTD

    // If the call `otPlatSetMcuPowerState()` was successful and the desire
    // state is `OFF`, ensure to disable Thread (MLE) operation (and stop
    // legacy) and also bring the IPv6 interface down.

    if (powerState == OT_PLAT_MCU_POWER_STATE_OFF)
    {
        if (otThreadGetDeviceRole(mInstance) != OT_DEVICE_ROLE_DISABLED)
        {
            otThreadSetEnabled(mInstance, false);
            StopLegacy();
        }

        if (otIp6IsEnabled(mInstance))
        {
            otIp6SetEnabled(mInstance, false);
        }
    }
#endif // #if OPENTHREAD_FTD || OPENTHREAD_MTD

exit:
    return error;
}

#else // OPENTHREAD_CONFIG_NCP_ENABLE_MCU_POWER_STATE_CONTROL

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_MCU_POWER_STATE>(void)
{
    return mEncoder.WriteUint8(SPINEL_MCU_POWER_STATE_ON);
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MCU_POWER_STATE>(void)
{
    return OT_ERROR_DISABLED_FEATURE;
}

#endif // OPENTHREAD_CONFIG_NCP_ENABLE_MCU_POWER_STATE_CONTROL

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_POWER_STATE>(void)
{
    return mEncoder.WriteUint8(SPINEL_POWER_STATE_ONLINE);
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_POWER_STATE>(void)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_HWADDR>(void)
{
    otExtAddress hwAddr;
    otPlatRadioGetIeeeEui64(mInstance, hwAddr.m8);

    return mEncoder.WriteEui64(hwAddr);
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_LOCK>(void)
{
    // TODO: Implement property lock (Needs API!)
    return mEncoder.OverwriteWithLastStatusError(SPINEL_STATUS_UNIMPLEMENTED);
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_HOST_POWER_STATE>(void)
{
    return mEncoder.WriteUint8(mHostPowerState);
}

// Setting `HOST_POWER_STATE` is treated and implemented differently from other
// handlers as it requires two special behaviors (a) the response frame for the
// set operation should be tracked and only when it is delivered we can assume
// that host is sleep (b) the response is critical so if there is no spinel
// buffer to prepare the response, the current spinel header is saved to
// prepare and send the response as soon as buffer space becomes available.
otError NcpBase::HandlePropertySet_SPINEL_PROP_HOST_POWER_STATE(uint8_t aHeader)
{
    uint8_t powerState;
    otError error = OT_ERROR_NONE;

    error = mDecoder.ReadUint8(powerState);

    if (error == OT_ERROR_NONE)
    {
        switch (powerState)
        {
        case SPINEL_HOST_POWER_STATE_OFFLINE:
        case SPINEL_HOST_POWER_STATE_DEEP_SLEEP:
        case SPINEL_HOST_POWER_STATE_LOW_POWER:
        case SPINEL_HOST_POWER_STATE_ONLINE:
            // Adopt the requested power state.
            mHostPowerState = static_cast<spinel_host_power_state_t>(powerState);
            break;

        case SPINEL_HOST_POWER_STATE_RESERVED:
            // Per the specification, treat this as synonymous with SPINEL_HOST_POWER_STATE_DEEP_SLEEP.
            mHostPowerState = SPINEL_HOST_POWER_STATE_DEEP_SLEEP;
            break;

        default:
            // Per the specification, treat unrecognized values as synonymous with SPINEL_HOST_POWER_STATE_LOW_POWER.
            mHostPowerState = SPINEL_HOST_POWER_STATE_LOW_POWER;
            break;
        }

        mHostPowerStateHeader = 0;

        error = WritePropertyValueIsFrame(aHeader, SPINEL_PROP_HOST_POWER_STATE);

        if (mHostPowerState != SPINEL_HOST_POWER_STATE_ONLINE)
        {
            if (error == OT_ERROR_NONE)
            {
                mHostPowerReplyFrameTag = GetLastOutboundFrameTag();
            }
            else
            {
                mHostPowerReplyFrameTag = NcpFrameBuffer::kInvalidTag;
            }

            mHostPowerStateInProgress = true;
        }

        if (error != OT_ERROR_NONE)
        {
            mHostPowerStateHeader = aHeader;

            // The reply will be queued when buffer space becomes available
            // in the NCP tx buffer so we return `success` to avoid sending a
            // NOMEM status for the same tid through `mDroppedReplyTid` list.

            error = OT_ERROR_NONE;
        }
    }
    else
    {
        error = WriteLastStatusFrame(aHeader, ThreadErrorToSpinelStatus(error));
    }

    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_UNSOL_UPDATE_LIST>(void)
{
    otError                       error = OT_ERROR_NONE;
    uint8_t                       numEntries;
    const ChangedPropsSet::Entry *entry;

    entry = mChangedPropsSet.GetSupportedEntries(numEntries);

    for (uint8_t index = 0; index < numEntries; index++, entry++)
    {
        if (entry->mFilterable)
        {
            SuccessOrExit(error = mEncoder.WriteUintPacked(entry->mPropKey));
        }
    }

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_PHY_RSSI>(void)
{
    return mEncoder.WriteInt8(otPlatRadioGetRssi(mInstance));
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_PHY_RX_SENSITIVITY>(void)
{
    return mEncoder.WriteInt8(otPlatRadioGetReceiveSensitivity(mInstance));
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_PHY_TX_POWER>(void)
{
    int8_t  power;
    otError error;

    error = otPlatRadioGetTransmitPower(mInstance, &power);

    if (error == OT_ERROR_NONE)
    {
        error = mEncoder.WriteInt8(power);
    }
    else
    {
        error = mEncoder.OverwriteWithLastStatusError(ThreadErrorToSpinelStatus(error));
    }

    return error;
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_PHY_TX_POWER>(void)
{
    int8_t  txPower = 0;
    otError error   = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadInt8(txPower));
    error = otPlatRadioSetTransmitPower(mInstance, txPower);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_DEBUG_TEST_ASSERT>(void)
{
    assert(false);

    // We only get to this point if `assert(false)`
    // does not cause an NCP reset on the platform.
    // In such a case we return `false` as the
    // property value to indicate this.

    OT_UNREACHABLE_CODE(return mEncoder.WriteBool(false);)
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_DEBUG_TEST_WATCHDOG>(void)
{
    while (true)
        ;

    OT_UNREACHABLE_CODE(return OT_ERROR_NONE;)
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_DEBUG_NCP_LOG_LEVEL>(void)
{
    return mEncoder.WriteUint8(ConvertLogLevel(otGetDynamicLogLevel(mInstance)));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_DEBUG_NCP_LOG_LEVEL>(void)
{
    uint8_t    spinelNcpLogLevel = 0;
    otLogLevel logLevel;
    otError    error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(spinelNcpLogLevel));

    switch (spinelNcpLogLevel)
    {
    case SPINEL_NCP_LOG_LEVEL_EMERG:
    case SPINEL_NCP_LOG_LEVEL_ALERT:
        logLevel = OT_LOG_LEVEL_NONE;
        break;

    case SPINEL_NCP_LOG_LEVEL_CRIT:
        logLevel = OT_LOG_LEVEL_CRIT;
        break;

    case SPINEL_NCP_LOG_LEVEL_ERR:
    case SPINEL_NCP_LOG_LEVEL_WARN:
        logLevel = OT_LOG_LEVEL_WARN;
        break;

    case SPINEL_NCP_LOG_LEVEL_NOTICE:
        logLevel = OT_LOG_LEVEL_NOTE;
        break;

    case SPINEL_NCP_LOG_LEVEL_INFO:
        logLevel = OT_LOG_LEVEL_INFO;
        break;

    case SPINEL_NCP_LOG_LEVEL_DEBUG:
        logLevel = OT_LOG_LEVEL_DEBG;
        break;

    default:
        ExitNow(error = OT_ERROR_INVALID_ARGS);
        break;
    }

    error = otSetDynamicLogLevel(mInstance, logLevel);

exit:
    return error;
}

} // namespace Ncp
} // namespace ot

// ----------------------------------------------------------------------------
// MARK: Peek/Poke delegate API
// ----------------------------------------------------------------------------

otError otNcpRegisterPeekPokeDelagates(otNcpDelegateAllowPeekPoke aAllowPeekDelegate,
                                       otNcpDelegateAllowPeekPoke aAllowPokeDelegate)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    ot::Ncp::NcpBase *ncp = ot::Ncp::NcpBase::GetNcpInstance();

    if (ncp != NULL)
    {
        ncp->RegisterPeekPokeDelagates(aAllowPeekDelegate, aAllowPokeDelegate);
    }
#else
    OT_UNUSED_VARIABLE(aAllowPeekDelegate);
    OT_UNUSED_VARIABLE(aAllowPokeDelegate);

    error = OT_ERROR_DISABLED_FEATURE;

#endif // OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE

    return error;
}

// ----------------------------------------------------------------------------
// MARK: Virtual Datastream I/O (Public API)
// ----------------------------------------------------------------------------

otError otNcpStreamWrite(int aStreamId, const uint8_t *aDataPtr, int aDataLen)
{
    otError           error = OT_ERROR_INVALID_STATE;
    ot::Ncp::NcpBase *ncp   = ot::Ncp::NcpBase::GetNcpInstance();

    if (ncp != NULL)
    {
        error = ncp->StreamWrite(aStreamId, aDataPtr, aDataLen);
    }

    return error;
}

extern "C" void otNcpPlatLogv(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, va_list aArgs)
{
    char logString[OPENTHREAD_CONFIG_NCP_SPINEL_LOG_MAX_SIZE];
    int  charsWritten;

    if ((charsWritten = vsnprintf(logString, sizeof(logString), aFormat, aArgs)) > 0)
    {
        if (charsWritten > static_cast<int>(sizeof(logString) - 1))
        {
            charsWritten = static_cast<int>(sizeof(logString) - 1);
        }

        otNcpStreamWrite(0, reinterpret_cast<uint8_t *>(logString), charsWritten);
    }

    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);
}

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL)

extern "C" void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    va_list           args;
    char              logString[OPENTHREAD_CONFIG_NCP_SPINEL_LOG_MAX_SIZE];
    ot::Ncp::NcpBase *ncp = ot::Ncp::NcpBase::GetNcpInstance();

    va_start(args, aFormat);

    if (vsnprintf(logString, sizeof(logString), aFormat, args) > 0)
    {
        if (ncp != NULL)
        {
            ncp->Log(aLogLevel, aLogRegion, logString);
        }
    }

    va_end(args);
}

#endif // (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL)
