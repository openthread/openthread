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
 *   This file implements the subset of IEEE 802.15.4 primitives required for Thread.
 */

#define WPP_NAME "mac.tmh"

#include <openthread/config.h>

#include "mac.hpp"

#include "utils/wrap_string.h"

#include <openthread/platform/random.h>

#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/logging.hpp"
#include "crypto/aes_ccm.hpp"
#include "crypto/sha256.hpp"
#include "mac/mac_frame.hpp"
#include "thread/link_quality.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"

using ot::Encoding::BigEndian::HostSwap64;

namespace ot {
namespace Mac {

static const uint8_t sMode2Key[] =
{
    0x78, 0x58, 0x16, 0x86, 0xfd, 0xb4, 0x58, 0x0f, 0xb0, 0x92, 0x54, 0x6a, 0xec, 0xbd, 0x15, 0x66
};

static const otExtAddress sMode2ExtAddress =
{
    { 0x35, 0x06, 0xfe, 0xb8, 0x23, 0xd4, 0x87, 0x12 },
};

static const uint8_t sExtendedPanidInit[] = {0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0xca, 0xfe};
static const char sNetworkNameInit[] = "OpenThread";

#ifdef _WIN32
const uint32_t kMinBackoffSum = kMinBackoff + (kUnitBackoffPeriod *OT_RADIO_SYMBOL_TIME * (1 << kMinBE)) / 1000;
const uint32_t kMaxBackoffSum = kMinBackoff + (kUnitBackoffPeriod *OT_RADIO_SYMBOL_TIME * (1 << kMaxBE)) / 1000;
static_assert(kMinBackoffSum > 0, "The min backoff value should be greater than zero!");
#endif

void Mac::StartCsmaBackoff(void)
{
    if (RadioSupportsCsmaBackoff())
    {
        // If the radio supports CSMA back off logic, immediately schedule the send.
        HandleBeginTransmit();
    }
    else
    {
        uint32_t backoffExponent = kMinBE + mTransmitAttempts + mCsmaAttempts;
        uint32_t backoff;
        bool shouldReceive;

        if (backoffExponent > kMaxBE)
        {
            backoffExponent = kMaxBE;
        }

        backoff = (otPlatRandomGet() % (1UL << backoffExponent));
        backoff *= (static_cast<uint32_t>(kUnitBackoffPeriod) * OT_RADIO_SYMBOL_TIME);

        // Put the radio in either sleep or receive mode depending on
        // `mRxOnWhenIdle` flag before starting the backoff timer.

        shouldReceive = (mRxOnWhenIdle || otPlatRadioGetPromiscuous(&GetInstance()));

        if (!shouldReceive)
        {
            if (RadioSleep() == OT_ERROR_INVALID_STATE)
            {
                // If `RadioSleep()` returns `OT_ERROR_INVALID_STATE`
                // indicating sleep is being delayed, the radio should
                // be put in receive mode.

                shouldReceive = true;
            }
        }

        if (shouldReceive)
        {
            switch (mOperation)
            {
            case kOperationActiveScan:
            case kOperationEnergyScan:
                RadioReceive(mScanChannel);
                break;

            default:
                RadioReceive(mChannel);
                break;
            }
        }

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
        mBackoffTimer.Start(backoff);
#else // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
        mBackoffTimer.Start(backoff / 1000UL);
#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    }
}

Mac::Mac(ThreadNetif &aThreadNetif):
    ThreadNetifLocator(aThreadNetif),
    mMacTimer(aThreadNetif.GetInstance(), &Mac::HandleMacTimer, this),
    mBackoffTimer(aThreadNetif.GetInstance(), &Mac::HandleBeginTransmit, this),
    mReceiveTimer(aThreadNetif.GetInstance(), &Mac::HandleReceiveTimer, this),
    mShortAddress(kShortAddrInvalid),
    mPanId(kPanIdBroadcast),
    mChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL),
    mMaxTransmitPower(OPENTHREAD_CONFIG_DEFAULT_MAX_TRANSMIT_POWER),
    mSendHead(NULL),
    mSendTail(NULL),
    mReceiveHead(NULL),
    mReceiveTail(NULL),
    mOperation(kOperationIdle),
    mPendingActiveScan(false),
    mPendingEnergyScan(false),
    mPendingTransmitBeacon(false),
    mPendingTransmitData(false),
    mPendingWaitingForData(false),
    mRxOnWhenIdle(false),
    mBeaconsEnabled(false),
#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
    mDelaySleep(false),
#endif
    mBeaconSequence(static_cast<uint8_t>(otPlatRandomGet())),
    mDataSequence(static_cast<uint8_t>(otPlatRandomGet())),
    mCsmaAttempts(0),
    mTransmitAttempts(0),
    mScanChannels(0xff),
    mScanDuration(0),
    mScanChannel(OT_RADIO_CHANNEL_MIN),
    mEnergyScanCurrentMaxRssi(kInvalidRssiValue),
    mScanContext(NULL),
    mActiveScanHandler(NULL), // Initialize `mActiveScanHandler` and `mEnergyScanHandler` union
    mEnergyScanSampleRssiTask(aThreadNetif.GetInstance(), &Mac::HandleEnergyScanSampleRssi, this),
    mPcapCallback(NULL),
    mPcapCallbackContext(NULL),
#if OPENTHREAD_ENABLE_MAC_FILTER
    mFilter(),
#endif  // OPENTHREAD_ENABLE_MAC_FILTER
    mTxFrame(static_cast<Frame *>(otPlatRadioGetTransmitBuffer(&aThreadNetif.GetInstance()))),
    mKeyIdMode2FrameCounter(0)
{
    GenerateExtAddress(&mExtAddress);

    memset(&mCounters, 0, sizeof(otMacCounters));

    otPlatRadioEnable(&GetInstance());

    SetExtendedPanId(sExtendedPanidInit);
    SetNetworkName(sNetworkNameInit);
    SetPanId(mPanId);
    SetExtAddress(mExtAddress);
    SetShortAddress(mShortAddress);
}

otError Mac::ActiveScan(uint32_t aScanChannels, uint16_t aScanDuration, ActiveScanHandler aHandler, void *aContext)
{
    otError error;

    SuccessOrExit(error = Scan(kOperationActiveScan, aScanChannels, aScanDuration, aContext));
    mActiveScanHandler = aHandler;

exit:
    return error;
}

otError Mac::EnergyScan(uint32_t aScanChannels, uint16_t aScanDuration, EnergyScanHandler aHandler, void *aContext)
{
    otError error;

    SuccessOrExit(error = Scan(kOperationEnergyScan, aScanChannels, aScanDuration, aContext));
    mEnergyScanHandler = aHandler;

exit:
    return error;
}

otError Mac::Scan(Operation aScanOperation, uint32_t aScanChannels, uint16_t aScanDuration, void *aContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!IsActiveScanInProgress() && !IsEnergyScanInProgress(), error = OT_ERROR_BUSY);

    mScanContext = aContext;
    mScanChannels = (aScanChannels == 0) ? static_cast<uint32_t>(kScanChannelsAll) : aScanChannels;
    mScanDuration = (aScanDuration == 0) ? static_cast<uint16_t>(kScanDurationDefault) : aScanDuration;

    mScanChannel = OT_RADIO_CHANNEL_MIN;
    mScanChannels >>= OT_RADIO_CHANNEL_MIN;

    while ((mScanChannels & 1) == 0)
    {
        mScanChannels >>= 1;
        mScanChannel++;
    }

    StartOperation(aScanOperation);

exit:
    return error;
}

bool Mac::IsActiveScanInProgress(void)
{
    return (mOperation == kOperationActiveScan) || (mPendingActiveScan);
}

bool Mac::IsEnergyScanInProgress(void)
{
    return (mOperation == kOperationEnergyScan) || (mPendingEnergyScan);
}

bool Mac::IsInTransmitState(void)
{
    return (mOperation == kOperationTransmitData) || (mOperation == kOperationTransmitBeacon);
}

otError Mac::ConvertBeaconToActiveScanResult(Frame *aBeaconFrame, otActiveScanResult &aResult)
{
    otError error = OT_ERROR_NONE;
    Address address;
    Beacon *beacon = NULL;
    BeaconPayload *beaconPayload = NULL;
    uint8_t payloadLength;
    char stringBuffer[BeaconPayload::kInfoStringSize];

    memset(&aResult, 0, sizeof(otActiveScanResult));

    VerifyOrExit(aBeaconFrame != NULL, error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit(aBeaconFrame->GetType() == Frame::kFcfFrameBeacon, error = OT_ERROR_PARSE);
    SuccessOrExit(error = aBeaconFrame->GetSrcAddr(address));
    VerifyOrExit(address.mLength == sizeof(address.mExtAddress), error = OT_ERROR_PARSE);
    memcpy(&aResult.mExtAddress, &address.mExtAddress, sizeof(aResult.mExtAddress));

    aBeaconFrame->GetSrcPanId(aResult.mPanId);
    aResult.mChannel = aBeaconFrame->GetChannel();
    aResult.mRssi = aBeaconFrame->GetPower();
    aResult.mLqi = aBeaconFrame->GetLqi();

    payloadLength = aBeaconFrame->GetPayloadLength();

    beacon = reinterpret_cast<Beacon *>(aBeaconFrame->GetPayload());
    beaconPayload = reinterpret_cast<BeaconPayload *>(beacon->GetPayload());

    if ((payloadLength >= (sizeof(*beacon) + sizeof(*beaconPayload))) && beacon->IsValid() && beaconPayload->IsValid())
    {
        aResult.mVersion = beaconPayload->GetProtocolVersion();
        aResult.mIsJoinable = beaconPayload->IsJoiningPermitted();
        aResult.mIsNative = beaconPayload->IsNative();
        memcpy(&aResult.mNetworkName, beaconPayload->GetNetworkName(), sizeof(aResult.mNetworkName));
        memcpy(&aResult.mExtendedPanId, beaconPayload->GetExtendedPanId(), sizeof(aResult.mExtendedPanId));
    }

    otLogInfoMac(GetInstance(), "Received Beacon, %s", beaconPayload->ToInfoString(stringBuffer, sizeof(stringBuffer)));

    OT_UNUSED_VARIABLE(stringBuffer);

exit:
    return error;
}

void Mac::StartEnergyScan(void)
{
    if (!(otPlatRadioGetCaps(&GetInstance()) & OT_RADIO_CAPS_ENERGY_SCAN))
    {
        mEnergyScanCurrentMaxRssi = kInvalidRssiValue;
        mMacTimer.Start(mScanDuration);
        mEnergyScanSampleRssiTask.Post();
        RadioReceive(mScanChannel);
    }
    else
    {
        otError error = otPlatRadioEnergyScan(&GetInstance(), mScanChannel, mScanDuration);

        if (error != OT_ERROR_NONE)
        {
            // Cancel scan
            mEnergyScanHandler(mScanContext, NULL);
            FinishOperation();
        }
    }
}

extern "C" void otPlatRadioEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi)
{
    VerifyOrExit(otInstanceIsInitialized(aInstance));

#if OPENTHREAD_ENABLE_RAW_LINK_API

    if (aInstance->mLinkRaw.IsEnabled())
    {
        aInstance->mLinkRaw.InvokeEnergyScanDone(aEnergyScanMaxRssi);
    }
    else
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
    {
        aInstance->mThreadNetif.GetMac().EnergyScanDone(aEnergyScanMaxRssi);
    }

exit:
    return;
}

void Mac::EnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    // Trigger a energy scan handler callback if necessary
    if (aEnergyScanMaxRssi != kInvalidRssiValue)
    {
        otEnergyScanResult result;

        result.mChannel = mScanChannel;
        result.mMaxRssi = aEnergyScanMaxRssi;
        mEnergyScanHandler(mScanContext, &result);
    }

    // Update to the next scan channel
    do
    {
        mScanChannels >>= 1;
        mScanChannel++;

        // If we have scanned all the channels, then fire the final callback
        // and finish scan operation.
        if (mScanChannels == 0 || mScanChannel > OT_RADIO_CHANNEL_MAX)
        {
            mEnergyScanHandler(mScanContext, NULL);
            FinishOperation();
            ExitNow();
        }
    }
    while ((mScanChannels & 1) == 0);

    // Start scanning the next channel
    StartEnergyScan();

exit:
    return;
}

void Mac::HandleEnergyScanSampleRssi(Tasklet &aTasklet)
{
    GetOwner(aTasklet).HandleEnergyScanSampleRssi();
}

void Mac::HandleEnergyScanSampleRssi(void)
{
    int8_t rssi;

    VerifyOrExit(mOperation == kOperationEnergyScan);

    rssi = otPlatRadioGetRssi(&GetInstance());

    if (rssi != kInvalidRssiValue)
    {
        if ((mEnergyScanCurrentMaxRssi == kInvalidRssiValue) || (rssi > mEnergyScanCurrentMaxRssi))
        {
            mEnergyScanCurrentMaxRssi = rssi;
        }
    }

    mEnergyScanSampleRssiTask.Post();

exit:
    return;
}

otError Mac::RegisterReceiver(Receiver &aReceiver)
{
    assert(mReceiveTail != &aReceiver && aReceiver.mNext == NULL);

    if (mReceiveTail == NULL)
    {
        mReceiveHead = &aReceiver;
        mReceiveTail = &aReceiver;
    }
    else
    {
        mReceiveTail->mNext = &aReceiver;
        mReceiveTail = &aReceiver;
    }

    return OT_ERROR_NONE;
}

void Mac::SetRxOnWhenIdle(bool aRxOnWhenIdle)
{
    VerifyOrExit(mRxOnWhenIdle != aRxOnWhenIdle);

    mRxOnWhenIdle = aRxOnWhenIdle;

    // If the new value for `mRxOnWhenIdle` is `true` (i.e., radio should
    // remain in Rx while idle) we stop any ongoing or pending `WaitinForData`
    // operation (since this operation only applies to sleepy devices).

    if (mRxOnWhenIdle)
    {
        mPendingWaitingForData = false;

        if (mOperation == kOperationWaitingForData)
        {
            mReceiveTimer.Stop();
            FinishOperation();
        }
    }

    UpdateIdleMode();

exit:
    return;
}

void Mac::GenerateExtAddress(ExtAddress *aExtAddress)
{
    for (size_t i = 0; i < sizeof(ExtAddress); i++)
    {
        aExtAddress->m8[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    aExtAddress->SetGroup(false);
    aExtAddress->SetLocal(true);
}

void Mac::SetExtAddress(const ExtAddress &aExtAddress)
{
    otExtAddress address;

    otLogFuncEntry();

    for (size_t i = 0; i < sizeof(address); i++)
    {
        address.m8[i] = aExtAddress.m8[7 - i];
    }

    otPlatRadioSetExtendedAddress(&GetInstance(), &address);
    mExtAddress = aExtAddress;

    otLogFuncExit();
}

void Mac::GetHashMacAddress(ExtAddress *aHashMacAddress)
{
    Crypto::Sha256 sha256;
    uint8_t buf[Crypto::Sha256::kHashSize];

    otLogFuncEntry();

    otPlatRadioGetIeeeEui64(&GetInstance(), buf);
    sha256.Start();
    sha256.Update(buf, OT_EXT_ADDRESS_SIZE);
    sha256.Finish(buf);

    memcpy(aHashMacAddress->m8, buf, OT_EXT_ADDRESS_SIZE);
    aHashMacAddress->SetLocal(true);

    otLogFuncExitMsg("%llX", HostSwap64(*reinterpret_cast<uint64_t *>(aHashMacAddress)));
}

otError Mac::SetShortAddress(ShortAddress aShortAddress)
{
    otLogFuncEntryMsg("%d", aShortAddress);
    mShortAddress = aShortAddress;
    otPlatRadioSetShortAddress(&GetInstance(), aShortAddress);
    otLogFuncExit();
    return OT_ERROR_NONE;
}

otError Mac::SetChannel(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    otLogFuncEntryMsg("%d", aChannel);

    VerifyOrExit(OT_RADIO_CHANNEL_MIN <= aChannel && aChannel <= OT_RADIO_CHANNEL_MAX, error = OT_ERROR_INVALID_ARGS);

    mChannel = aChannel;
    UpdateIdleMode();

exit:
    otLogFuncExit();
    return error;
}

otError Mac::SetNetworkName(const char *aNetworkName)
{
    otError error = OT_ERROR_NONE;

    otLogFuncEntryMsg("%s", aNetworkName);

    VerifyOrExit(strlen(aNetworkName) <= OT_NETWORK_NAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);

    (void)strlcpy(mNetworkName.m8, aNetworkName, sizeof(mNetworkName));

exit:
    otLogFuncExitErr(error);
    return error;
}

otError Mac::SetPanId(PanId aPanId)
{
    otLogFuncEntryMsg("%d", aPanId);
    mPanId = aPanId;
    otPlatRadioSetPanId(&GetInstance(), mPanId);
    otLogFuncExit();
    return OT_ERROR_NONE;
}

otError Mac::SetExtendedPanId(const uint8_t *aExtPanId)
{
    memcpy(mExtendedPanId.m8, aExtPanId, sizeof(mExtendedPanId));
    return OT_ERROR_NONE;
}

otError Mac::SendFrameRequest(Sender &aSender)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mSendTail != &aSender && aSender.mNext == NULL, error = OT_ERROR_ALREADY);

    if (mSendHead == NULL)
    {
        mSendHead = &aSender;
        mSendTail = &aSender;
    }
    else
    {
        mSendTail->mNext = &aSender;
        mSendTail = &aSender;
    }

    StartOperation(kOperationTransmitData);

exit:
    return error;
}

void Mac::UpdateIdleMode(void)
{
    VerifyOrExit(mOperation == kOperationIdle);

    if (!mRxOnWhenIdle && !mReceiveTimer.IsRunning() && !otPlatRadioGetPromiscuous(&GetInstance()))
    {
        if (RadioSleep() != OT_ERROR_INVALID_STATE)
        {
            otLogDebgMac(GetInstance(), "Idle mode: Radio sleeping");
            ExitNow();
        }

        // If `RadioSleep()` returns `OT_ERROR_INVALID_STATE`
        // indicating sleep is being delayed, continue to put
        // the radio in receive mode.
    }

    otLogDebgMac(GetInstance(), "Idle mode: Radio receiving on channel %d", mChannel);
    RadioReceive(mChannel);

exit:
    return;
}

void Mac::StartOperation(Operation aOperation)
{
    if (aOperation != kOperationIdle)
    {
        otLogDebgMac(GetInstance(), "Request to start operation \"%s\"", OperationToString(aOperation));
    }

    switch (aOperation)
    {
    case kOperationIdle:
        break;

    case kOperationActiveScan:
        mPendingActiveScan = true;
        break;

    case kOperationEnergyScan:
        mPendingEnergyScan = true;
        break;

    case kOperationTransmitBeacon:
        mPendingTransmitBeacon = true;
        break;

    case kOperationTransmitData:
        mPendingTransmitData = true;
        break;

    case kOperationWaitingForData:
        mPendingWaitingForData = true;
        break;
    }

    VerifyOrExit(mOperation == kOperationIdle);

    // `WaitingForData` should be checked before any other pending
    // operations since radio should remain in receive mode after
    // a data poll ack indicating a pending frame from parent.

    if (mPendingWaitingForData)
    {
        mPendingWaitingForData = false;
        mOperation = kOperationWaitingForData;
        RadioReceive(mChannel);
    }
    else if (mPendingActiveScan)
    {
        mPendingActiveScan = false;
        mOperation = kOperationActiveScan;
        StartCsmaBackoff();
    }
    else if (mPendingEnergyScan)
    {
        mPendingEnergyScan = false;
        mOperation = kOperationEnergyScan;
        StartEnergyScan();
    }
    else if (mPendingTransmitBeacon)
    {
        mPendingTransmitBeacon = false;
        mOperation = kOperationTransmitBeacon;
        StartCsmaBackoff();
    }
    else if (mPendingTransmitData)
    {
        mPendingTransmitData = false;
        mOperation = kOperationTransmitData;
        StartCsmaBackoff();
    }
    else
    {
        UpdateIdleMode();
    }

    if (mOperation != kOperationIdle)
    {
        otLogDebgMac(GetInstance(), "Starting operation \"%s\"", OperationToString(mOperation));
    }

exit:
    return;
}

void Mac::FinishOperation(void)
{
    // Clear the current operation and start any pending ones.

    otLogDebgMac(GetInstance(), "Finishing operation \"%s\"", OperationToString(mOperation));

    mOperation = kOperationIdle;
    StartOperation(kOperationIdle);
}

void Mac::GenerateNonce(const ExtAddress &aAddress, uint32_t aFrameCounter, uint8_t aSecurityLevel, uint8_t *aNonce)
{
    // source address
    for (int i = 0; i < 8; i++)
    {
        aNonce[i] = aAddress.m8[i];
    }

    aNonce += 8;

    // frame counter
    aNonce[0] = (aFrameCounter >> 24) & 0xff;
    aNonce[1] = (aFrameCounter >> 16) & 0xff;
    aNonce[2] = (aFrameCounter >> 8) & 0xff;
    aNonce[3] = (aFrameCounter >> 0) & 0xff;
    aNonce += 4;

    // security level
    aNonce[0] = aSecurityLevel;
}

void Mac::SendBeaconRequest(Frame &aFrame)
{
    // initialize MAC header
    uint16_t fcf = Frame::kFcfFrameMacCmd | Frame::kFcfDstAddrShort | Frame::kFcfSrcAddrNone;
    aFrame.InitMacHeader(fcf, Frame::kSecNone);
    aFrame.SetDstPanId(kShortAddrBroadcast);
    aFrame.SetDstAddr(kShortAddrBroadcast);
    aFrame.SetCommandId(Frame::kMacCmdBeaconRequest);
    otLogInfoMac(GetInstance(), "Sending Beacon Request");
}

void Mac::SendBeacon(Frame &aFrame)
{
    uint8_t numUnsecurePorts;
    uint8_t beaconLength;
    uint16_t fcf;
    Beacon *beacon = NULL;
    BeaconPayload *beaconPayload = NULL;
    char stringBuffer[BeaconPayload::kInfoStringSize];

    // initialize MAC header
    fcf = Frame::kFcfFrameBeacon | Frame::kFcfDstAddrNone | Frame::kFcfSrcAddrExt;
    aFrame.InitMacHeader(fcf, Frame::kSecNone);
    aFrame.SetSrcPanId(mPanId);
    aFrame.SetSrcAddr(mExtAddress);

    // write payload
    beacon = reinterpret_cast<Beacon *>(aFrame.GetPayload());
    beacon->Init();
    beaconLength = sizeof(*beacon);

    beaconPayload = reinterpret_cast<BeaconPayload *>(beacon->GetPayload());

    if (GetNetif().GetKeyManager().GetSecurityPolicyFlags() & OT_SECURITY_POLICY_BEACONS)
    {
        beaconPayload->Init();

        // set the Joining Permitted flag
        GetNetif().GetIp6Filter().GetUnsecurePorts(numUnsecurePorts);

        if (numUnsecurePorts)
        {
            beaconPayload->SetJoiningPermitted();
        }
        else
        {
            beaconPayload->ClearJoiningPermitted();
        }

        beaconPayload->SetNetworkName(mNetworkName.m8);
        beaconPayload->SetExtendedPanId(mExtendedPanId.m8);

        beaconLength += sizeof(*beaconPayload);
    }

    aFrame.SetPayloadLength(beaconLength);

    otLogInfoMac(GetInstance(), "Sending Beacon, %s", beaconPayload->ToInfoString(stringBuffer, sizeof(stringBuffer)));

    OT_UNUSED_VARIABLE(stringBuffer);
}

void Mac::ProcessTransmitSecurity(Frame &aFrame)
{
    KeyManager &keyManager = GetNetif().GetKeyManager();
    uint32_t frameCounter = 0;
    uint8_t securityLevel;
    uint8_t keyIdMode;
    uint8_t nonce[kNonceSize];
    uint8_t tagLength;
    Crypto::AesCcm aesCcm;
    const uint8_t *key = NULL;
    const ExtAddress *extAddress = NULL;
    otError error;

    if (aFrame.GetSecurityEnabled() == false)
    {
        ExitNow();
    }

    aFrame.GetKeyIdMode(keyIdMode);

    switch (keyIdMode)
    {
    case Frame::kKeyIdMode0:
        key = keyManager.GetKek();
        extAddress = &mExtAddress;

        if (!aFrame.IsARetransmission())
        {
            aFrame.SetFrameCounter(keyManager.GetKekFrameCounter());
            keyManager.IncrementKekFrameCounter();
        }

        break;

    case Frame::kKeyIdMode1:
        key = keyManager.GetCurrentMacKey();
        extAddress = &mExtAddress;

        // If the frame is marked as a retransmission, the `Mac::Sender` which
        // prepared the frame should set the frame counter and key id to the
        // same values used in the earlier transmit attempt. For a new frame (not
        // a retransmission), we get a new frame counter and key id from the key
        // manager.

        if (!aFrame.IsARetransmission())
        {
            aFrame.SetFrameCounter(keyManager.GetMacFrameCounter());
            keyManager.IncrementMacFrameCounter();
            aFrame.SetKeyId((keyManager.GetCurrentKeySequence() & 0x7f) + 1);
        }

        break;

    case Frame::kKeyIdMode2:
    {
        const uint8_t keySource[] = {0xff, 0xff, 0xff, 0xff};
        key = sMode2Key;
        mKeyIdMode2FrameCounter++;
        aFrame.SetFrameCounter(mKeyIdMode2FrameCounter);
        aFrame.SetKeySource(keySource);
        aFrame.SetKeyId(0xff);
        extAddress = static_cast<const ExtAddress *>(&sMode2ExtAddress);
        break;
    }

    default:
        assert(false);
        break;
    }

    aFrame.GetSecurityLevel(securityLevel);
    aFrame.GetFrameCounter(frameCounter);

    GenerateNonce(*extAddress, frameCounter, securityLevel, nonce);

    aesCcm.SetKey(key, 16);
    tagLength = aFrame.GetFooterLength() - Frame::kFcsSize;

    error = aesCcm.Init(aFrame.GetHeaderLength(), aFrame.GetPayloadLength(), tagLength, nonce, sizeof(nonce));
    assert(error == OT_ERROR_NONE);

    aesCcm.Header(aFrame.GetHeader(), aFrame.GetHeaderLength());
    aesCcm.Payload(aFrame.GetPayload(), aFrame.GetPayload(), aFrame.GetPayloadLength(), true);
    aesCcm.Finalize(aFrame.GetFooter(), &tagLength);

exit:
    return;
}

void Mac::HandleBeginTransmit(Timer &aTimer)
{
    GetOwner(aTimer).HandleBeginTransmit();
}

void Mac::HandleBeginTransmit(void)
{
    Frame &sendFrame(*mTxFrame);
    otError error = OT_ERROR_NONE;

    if (mCsmaAttempts == 0 && mTransmitAttempts == 0)
    {
        sendFrame.SetPower(mMaxTransmitPower);

        switch (mOperation)
        {
        case kOperationActiveScan:
            otPlatRadioSetPanId(&GetInstance(), kPanIdBroadcast);
            sendFrame.SetChannel(mScanChannel);
            SendBeaconRequest(sendFrame);
            sendFrame.SetSequence(0);
            sendFrame.SetMaxTxAttempts(kDirectFrameMacTxAttempts);
            break;

        case kOperationTransmitBeacon:
            sendFrame.SetChannel(mChannel);
            SendBeacon(sendFrame);
            sendFrame.SetSequence(mBeaconSequence++);
            sendFrame.SetMaxTxAttempts(kDirectFrameMacTxAttempts);
            break;

        case kOperationTransmitData:
            sendFrame.SetChannel(mChannel);
            SuccessOrExit(error = mSendHead->HandleFrameRequest(sendFrame));

            // If the frame is marked as a retransmission, then data sequence number is already set by the `Sender`.
            if (!sendFrame.IsARetransmission())
            {
                sendFrame.SetSequence(mDataSequence);
            }

            break;

        default:
            assert(false);
            break;
        }

        // Security Processing
        ProcessTransmitSecurity(sendFrame);

        if (sendFrame.GetPower() > mMaxTransmitPower)
        {
            sendFrame.SetPower(mMaxTransmitPower);
        }
    }

    error = RadioReceive(sendFrame.GetChannel());
    assert(error == OT_ERROR_NONE);

    error = RadioTransmit(&sendFrame);
    assert(error == OT_ERROR_NONE);

    if (mPcapCallback)
    {
        sendFrame.mDidTX = true;
        mPcapCallback(&sendFrame, mPcapCallbackContext);
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        TransmitDoneTask(mTxFrame, NULL, OT_ERROR_ABORT);
    }
}

extern "C" void otPlatRadioTxStarted(otInstance *aInstance, otRadioFrame *aFrame)
{
    otLogFuncEntry();
    VerifyOrExit(otInstanceIsInitialized(aInstance));
    aInstance->mThreadNetif.GetMac().TransmitStartedTask(aFrame);
exit:
    otLogFuncExit();
}

void Mac::TransmitStartedTask(otRadioFrame *aFrame)
{
    Frame *frame = static_cast<Frame *>(aFrame);

    if (frame->GetAckRequest() && !(otPlatRadioGetCaps(&GetInstance()) & OT_RADIO_CAPS_ACK_TIMEOUT))
    {
        mMacTimer.Start(kAckTimeout);
        otLogDebgMac(GetInstance(), "Ack timer start");
    }
}

extern "C" void otPlatRadioTxDone(otInstance *aInstance, otRadioFrame *aFrame, otRadioFrame *aAckFrame,
                                  otError aError)
{
    otLogFuncEntryMsg("%!otError!", aError);
    VerifyOrExit(otInstanceIsInitialized(aInstance));

#if OPENTHREAD_ENABLE_RAW_LINK_API

    if (aInstance->mLinkRaw.IsEnabled())
    {
        aInstance->mLinkRaw.InvokeTransmitDone(aFrame, aAckFrame, aError);
    }
    else
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
    {
        aInstance->mThreadNetif.GetMac().TransmitDoneTask(aFrame, aAckFrame, aError);
    }

exit:
    otLogFuncExit();
}

void Mac::TransmitDoneTask(otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError)
{
    Frame *txFrame = static_cast<Frame *>(aFrame);
    Address addr;
    bool framePending = false;

    mMacTimer.Stop();

    mCounters.mTxTotal++;

    txFrame->GetDstAddr(addr);

    if (aError == OT_ERROR_NONE && txFrame->GetAckRequest() && aAckFrame != NULL)
    {
        Frame *ackFrame = static_cast<Frame *>(aAckFrame);
        Neighbor *neighbor;

        framePending = ackFrame->GetFramePending();
        neighbor = GetNetif().GetMle().GetNeighbor(addr);

        if (neighbor != NULL)
        {
            neighbor->GetLinkInfo().AddRss(GetNoiseFloor(), ackFrame->GetPower());
        }
    }

    if (addr.mShortAddress == kShortAddrBroadcast)
    {
        // Broadcast frame
        mCounters.mTxBroadcast++;
    }
    else
    {
        // Unicast frame
        mCounters.mTxUnicast++;
    }

    if (aError == OT_ERROR_ABORT)
    {
        mCounters.mTxErrAbort++;
    }

    if (aError == OT_ERROR_CHANNEL_ACCESS_FAILURE)
    {
        mCounters.mTxErrCca++;
    }

    if (!RadioSupportsCsmaBackoff() &&
        aError == OT_ERROR_CHANNEL_ACCESS_FAILURE &&
        mCsmaAttempts < kMaxCSMABackoffs)
    {
        mCsmaAttempts++;
        StartCsmaBackoff();

        ExitNow();
    }

    mCsmaAttempts = 0;

    switch (mOperation)
    {
    case kOperationTransmitData:

        if (framePending && txFrame->IsDataRequestCommand())
        {
            mReceiveTimer.Start(kDataPollTimeout);
            StartOperation(kOperationWaitingForData);
        }

    // fall through

    case kOperationActiveScan:
    case kOperationTransmitBeacon:
        SentFrame(aError);
        break;

    default:
#if OPENTHREAD_ENABLE_RAW_LINK_API
        if (GetInstance().mLinkRaw.IsEnabled())
        {
            GetInstance().mLinkRaw.InvokeTransmitDone(mTxFrame, NULL, OT_ERROR_NO_ACK);
        }
        else
#endif
        {
            assert(false);
        }

        break;
    }

exit:
    return;
}

otError Mac::RadioTransmit(Frame *aSendFrame)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS

    if (!mRxOnWhenIdle)
    {
        // Cancel delay sleep timer
        mReceiveTimer.Stop();

        // Delay sleep if we have another frame pending to transmit
        mDelaySleep = aSendFrame->GetFramePending();
    }

#endif

    SuccessOrExit(error = otPlatRadioTransmit(&GetInstance(), static_cast<otRadioFrame *>(aSendFrame)));

    otLogWarnMac(GetInstance(), "otPlatRadioTransmit() failed with error %s", otThreadErrorToString(error));

exit:
    return error;
}

otError Mac::RadioReceive(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS

    if (!mRxOnWhenIdle)
    {
        // Cancel delay sleep timer
        mReceiveTimer.Stop();
    }

#endif

    SuccessOrExit(error = otPlatRadioReceive(&GetInstance(), aChannel));

    otLogWarnMac(GetInstance(), "otPlatRadioReceive() failed with error %s", otThreadErrorToString(error));

exit:
    return error;
}

otError Mac::RadioSleep(void)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS

    if (mDelaySleep)
    {
        otLogDebgMac(GetInstance(), "Delaying sleep waiting for frame rx/tx");

        mReceiveTimer.Start(kSleepDelay);
        mDelaySleep = false;

        // If sleep is delayed, `OT_ERROR_INVALID_STATE` is
        // returned to inform the caller to put/keep the
        // radio in receive mode.

        ExitNow(error = OT_ERROR_INVALID_STATE);
    }

#endif
    SuccessOrExit(error = otPlatRadioSleep(&GetInstance()));

    otLogWarnMac(GetInstance(), "otPlatRadioSleep() failed with error %s", otThreadErrorToString(error));

exit:
    return error;
}

void Mac::HandleMacTimer(Timer &aTimer)
{
    GetOwner(aTimer).HandleMacTimer();
}

void Mac::HandleMacTimer(void)
{
    Address addr;

    switch (mOperation)
    {
    case kOperationActiveScan:
        do
        {
            mScanChannels >>= 1;
            mScanChannel++;

            if (mScanChannels == 0 || mScanChannel > OT_RADIO_CHANNEL_MAX)
            {
                otPlatRadioSetPanId(&GetInstance(), mPanId);
                mActiveScanHandler(mScanContext, NULL);
                FinishOperation();
                ExitNow();
            }
        }
        while ((mScanChannels & 1) == 0);

        StartCsmaBackoff();
        break;

    case kOperationEnergyScan:
        EnergyScanDone(mEnergyScanCurrentMaxRssi);
        break;

    case kOperationTransmitData:
        otLogDebgMac(GetInstance(), "Ack timer fired");
        mCounters.mTxTotal++;

        mTxFrame->GetDstAddr(addr);

        if (addr.mShortAddress == kShortAddrBroadcast)
        {
            // Broadcast frame
            mCounters.mTxBroadcast++;
        }
        else
        {
            // Unicast Frame
            mCounters.mTxUnicast++;
        }

        SentFrame(OT_ERROR_NO_ACK);
        break;

    default:
        assert(false);
        break;
    }

exit:
    return;
}

void Mac::HandleReceiveTimer(Timer &aTimer)
{
    GetOwner(aTimer).HandleReceiveTimer();
}

void Mac::HandleReceiveTimer(void)
{
    // `mReceiveTimer` is used for two purposes: (1) for data poll timeout
    // (i.e., waiting to receive a data frame after a data poll ack
    // indicating a pending frame from parent), and (2) for delaying sleep
    // when feature `OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS` is
    // enabled.

    if (mOperation == kOperationWaitingForData)
    {
        otLogDebgMac(GetInstance(), "Data poll timeout");

        FinishOperation();

        for (Receiver *receiver = mReceiveHead; receiver; receiver = receiver->mNext)
        {
            receiver->HandleDataPollTimeout();
        }
    }
    else
    {
        otLogDebgMac(GetInstance(), "Sleep delay timeout expired");

        UpdateIdleMode();
    }
}

void Mac::SentFrame(otError aError)
{
    Frame &sendFrame(*mTxFrame);
    Sender *sender;

    mTransmitAttempts++;

    switch (aError)
    {
    case OT_ERROR_NONE:
        break;

    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
    case OT_ERROR_ABORT:
    case OT_ERROR_NO_ACK:
    {
        char stringBuffer[Frame::kInfoStringSize];

        otLogInfoMac(GetInstance(), "Frame tx failed, error:%s, attempt:%d/%d, %s", otThreadErrorToString(aError),
                     mTransmitAttempts, sendFrame.GetMaxTxAttempts(),
                     sendFrame.ToInfoString(stringBuffer, sizeof(stringBuffer)));
        otDumpDebgMac(GetInstance(), "TX ERR", sendFrame.GetHeader(), 16);

        if (!RadioSupportsRetries() &&
            mTransmitAttempts < sendFrame.GetMaxTxAttempts())
        {
            StartCsmaBackoff();
            mCounters.mTxRetry++;
            ExitNow();
        }

        OT_UNUSED_VARIABLE(stringBuffer);
        break;
    }

    default:
        assert(false);
        break;
    }

    mTransmitAttempts = 0;
    mCsmaAttempts = 0;

    if (sendFrame.GetAckRequest())
    {
        mCounters.mTxAckRequested++;

        if (aError == OT_ERROR_NONE)
        {
            mCounters.mTxAcked++;
        }
    }
    else
    {
        mCounters.mTxNoAckRequested++;
    }

    switch (mOperation)
    {
    case kOperationActiveScan:
        mCounters.mTxBeaconRequest++;
        mMacTimer.Start(mScanDuration);
        break;

    case kOperationTransmitBeacon:
        mCounters.mTxBeacon++;
        FinishOperation();
        break;

    case kOperationTransmitData:
        if (mReceiveTimer.IsRunning())
        {
            mCounters.mTxDataPoll++;
        }
        else
        {
            mCounters.mTxData++;
        }

        sender = mSendHead;
        mSendHead = mSendHead->mNext;

        if (mSendHead == NULL)
        {
            mSendTail = NULL;
        }

        sender->mNext = NULL;

        if (!sendFrame.IsARetransmission())
        {
            mDataSequence++;
        }

        otDumpDebgMac(GetInstance(), "TX", sendFrame.GetHeader(), sendFrame.GetLength());
        sender->HandleSentFrame(sendFrame, aError);

        FinishOperation();
        break;

    default:
        assert(false);
        break;
    }

exit:
    return;
}

otError Mac::ProcessReceiveSecurity(Frame &aFrame, const Address &aSrcAddr, Neighbor *aNeighbor)
{
    KeyManager &keyManager = GetNetif().GetKeyManager();
    otError error = OT_ERROR_NONE;
    uint8_t securityLevel;
    uint8_t keyIdMode;
    uint32_t frameCounter;
    uint8_t nonce[kNonceSize];
    uint8_t tag[Frame::kMaxMicSize];
    uint8_t tagLength;
    uint8_t keyid;
    uint32_t keySequence = 0;
    const uint8_t *macKey;
    const ExtAddress *extAddress;
    Crypto::AesCcm aesCcm;

    aFrame.SetSecurityValid(false);

    if (aFrame.GetSecurityEnabled() == false)
    {
        ExitNow();
    }

    aFrame.GetSecurityLevel(securityLevel);
    aFrame.GetFrameCounter(frameCounter);
    otLogDebgMac(GetInstance(), "Frame counter %u", frameCounter);

    aFrame.GetKeyIdMode(keyIdMode);

    switch (keyIdMode)
    {
    case Frame::kKeyIdMode0:
        VerifyOrExit((macKey = keyManager.GetKek()) != NULL, error = OT_ERROR_SECURITY);
        extAddress = &aSrcAddr.mExtAddress;
        break;

    case Frame::kKeyIdMode1:
        VerifyOrExit(aNeighbor != NULL, error = OT_ERROR_SECURITY);

        aFrame.GetKeyId(keyid);
        keyid--;

        if (keyid == (keyManager.GetCurrentKeySequence() & 0x7f))
        {
            // same key index
            keySequence = keyManager.GetCurrentKeySequence();
            macKey = keyManager.GetCurrentMacKey();
        }
        else if (keyid == ((keyManager.GetCurrentKeySequence() - 1) & 0x7f))
        {
            // previous key index
            keySequence = keyManager.GetCurrentKeySequence() - 1;
            macKey = keyManager.GetTemporaryMacKey(keySequence);
        }
        else if (keyid == ((keyManager.GetCurrentKeySequence() + 1) & 0x7f))
        {
            // next key index
            keySequence = keyManager.GetCurrentKeySequence() + 1;
            macKey = keyManager.GetTemporaryMacKey(keySequence);
        }
        else
        {
            ExitNow(error = OT_ERROR_SECURITY);
        }

        // If the frame is from a neighbor not in valid state (e.g., it is from a child being
        // restored), skip the key sequence and frame counter checks but continue to verify
        // the tag/MIC. Such a frame is later filtered in `RxDoneTask` which only allows MAC
        // Data Request frames from a child being restored.

        if (aNeighbor->GetState() == Neighbor::kStateValid)
        {
            if (keySequence < aNeighbor->GetKeySequence())
            {
                ExitNow(error = OT_ERROR_SECURITY);
            }
            else if (keySequence == aNeighbor->GetKeySequence())
            {
                if ((frameCounter + 1) < aNeighbor->GetLinkFrameCounter())
                {
                    ExitNow(error = OT_ERROR_SECURITY);
                }
                else if ((frameCounter + 1) == aNeighbor->GetLinkFrameCounter())
                {
                    // drop duplicated frames
                    ExitNow(error = OT_ERROR_DUPLICATED);
                }
            }
        }

        extAddress = &aSrcAddr.mExtAddress;

        break;

    case Frame::kKeyIdMode2:
        macKey = sMode2Key;
        extAddress = static_cast<const ExtAddress *>(&sMode2ExtAddress);
        break;

    default:
        ExitNow(error = OT_ERROR_SECURITY);
        break;
    }

    GenerateNonce(*extAddress, frameCounter, securityLevel, nonce);
    tagLength = aFrame.GetFooterLength() - Frame::kFcsSize;

    aesCcm.SetKey(macKey, 16);

    error = aesCcm.Init(aFrame.GetHeaderLength(), aFrame.GetPayloadLength(), tagLength, nonce, sizeof(nonce));
    VerifyOrExit(error == OT_ERROR_NONE, error = OT_ERROR_SECURITY);

    aesCcm.Header(aFrame.GetHeader(), aFrame.GetHeaderLength());
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    aesCcm.Payload(aFrame.GetPayload(), aFrame.GetPayload(), aFrame.GetPayloadLength(), false);
#else
    // for fuzz tests, execute AES but do not alter the payload
    uint8_t fuzz[OT_RADIO_FRAME_MAX_SIZE];
    aesCcm.Payload(fuzz, aFrame.GetPayload(), aFrame.GetPayloadLength(), false);
#endif
    aesCcm.Finalize(tag, &tagLength);

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    VerifyOrExit(memcmp(tag, aFrame.GetFooter(), tagLength) == 0, error = OT_ERROR_SECURITY);
#endif

    if ((keyIdMode == Frame::kKeyIdMode1) && (aNeighbor->GetState() == Neighbor::kStateValid))
    {
        if (aNeighbor->GetKeySequence() != keySequence)
        {
            aNeighbor->SetKeySequence(keySequence);
            aNeighbor->SetMleFrameCounter(0);
        }

        aNeighbor->SetLinkFrameCounter(frameCounter + 1);

        if (keySequence > keyManager.GetCurrentKeySequence())
        {
            keyManager.SetCurrentKeySequence(keySequence);
        }
    }

    aFrame.SetSecurityValid(true);

exit:
    return error;
}

extern "C" void otPlatRadioReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    otLogFuncEntryMsg("%!otError!", aError);
    VerifyOrExit(otInstanceIsInitialized(aInstance));

#if OPENTHREAD_ENABLE_RAW_LINK_API

    if (aInstance->mLinkRaw.IsEnabled())
    {
        aInstance->mLinkRaw.InvokeReceiveDone(aFrame, aError);
    }
    else
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
    {
        aInstance->mThreadNetif.GetMac().ReceiveDoneTask(static_cast<Frame *>(aFrame), aError);
    }

exit:
    otLogFuncExit();
}

void Mac::ReceiveDoneTask(Frame *aFrame, otError aError)
{
    Address srcaddr;
    Address dstaddr;
    PanId panid;
    Neighbor *neighbor;
    bool receive = false;
    otError error = aError;
#if OPENTHREAD_ENABLE_MAC_FILTER
    int8_t rssi = OT_MAC_FILTER_FIXED_RSS_DISABLED;
#endif  // OPENTHREAD_ENABLE_MAC_FILTER

    mCounters.mRxTotal++;

    VerifyOrExit(error == OT_ERROR_NONE);
    VerifyOrExit(aFrame != NULL, error = OT_ERROR_NO_FRAME_RECEIVED);

    aFrame->SetSecurityValid(false);

    if (mPcapCallback)
    {
        aFrame->mDidTX = false;
        mPcapCallback(aFrame, mPcapCallbackContext);
    }

    // Ensure we have a valid frame before attempting to read any contents of
    // the buffer received from the radio.
    SuccessOrExit(error = aFrame->ValidatePsdu());

    aFrame->GetSrcAddr(srcaddr);
    neighbor = GetNetif().GetMle().GetNeighbor(srcaddr);

    switch (srcaddr.mLength)
    {
    case 0:
        break;

    case sizeof(ShortAddress):
        otLogDebgMac(GetInstance(), "Received frame from short address 0x%04x", srcaddr.mShortAddress);

        if (neighbor == NULL)
        {
            ExitNow(error = OT_ERROR_UNKNOWN_NEIGHBOR);
        }

        srcaddr.mLength = sizeof(srcaddr.mExtAddress);
        srcaddr.mExtAddress = neighbor->GetExtAddress();
        break;

    case sizeof(ExtAddress):
        break;

    default:
        ExitNow(error = OT_ERROR_INVALID_SOURCE_ADDRESS);
    }

    // Duplicate Address Protection
    if (memcmp(&srcaddr.mExtAddress, &mExtAddress, sizeof(srcaddr.mExtAddress)) == 0)
    {
        ExitNow(error = OT_ERROR_INVALID_SOURCE_ADDRESS);
    }

#if OPENTHREAD_ENABLE_MAC_FILTER

    // Source filter Processing.
    if (srcaddr.mLength != 0)
    {
        // check if filtered out by whitelist or blacklist.
        SuccessOrExit(error = mFilter.Apply(srcaddr.mExtAddress, rssi));

        // override with the rssi in setting
        if (rssi != OT_MAC_FILTER_FIXED_RSS_DISABLED)
        {
            aFrame->mPower = rssi;
        }
    }

#endif  // OPENTHREAD_ENABLE_MAC_FILTER

    // Destination Address Filtering
    aFrame->GetDstAddr(dstaddr);

    switch (dstaddr.mLength)
    {
    case 0:
        break;

    case sizeof(ShortAddress):
        aFrame->GetDstPanId(panid);
        VerifyOrExit((panid == kShortAddrBroadcast || panid == mPanId) &&
                     ((mRxOnWhenIdle && dstaddr.mShortAddress == kShortAddrBroadcast) ||
                      dstaddr.mShortAddress == mShortAddress), error = OT_ERROR_DESTINATION_ADDRESS_FILTERED);
        break;

    case sizeof(ExtAddress):
        aFrame->GetDstPanId(panid);
        VerifyOrExit(panid == mPanId &&
                     memcmp(&dstaddr.mExtAddress, &mExtAddress, sizeof(dstaddr.mExtAddress)) == 0,
                     error = OT_ERROR_DESTINATION_ADDRESS_FILTERED);
        break;
    }

    // Increment counters
    if (dstaddr.mShortAddress == kShortAddrBroadcast)
    {
        // Broadcast frame
        mCounters.mRxBroadcast++;
    }
    else
    {
        // Unicast frame
        mCounters.mRxUnicast++;
    }

    // Security Processing
    SuccessOrExit(error = ProcessReceiveSecurity(*aFrame, srcaddr, neighbor));

    if (neighbor != NULL)
    {
#if OPENTHREAD_ENABLE_MAC_FILTER

        // make assigned rssi to take effect quickly
        if (rssi != OT_MAC_FILTER_FIXED_RSS_DISABLED)
        {
            neighbor->GetLinkInfo().Clear();
        }

#endif  // OPENTHREAD_ENABLE_MAC_FILTER

        neighbor->GetLinkInfo().AddRss(GetNoiseFloor(), aFrame->mPower);

        if (aFrame->GetSecurityEnabled() == true)
        {
            switch (neighbor->GetState())
            {
            case Neighbor::kStateValid:
                break;

            case Neighbor::kStateRestored:
            case Neighbor::kStateChildUpdateRequest:

                // Only accept a "MAC Data Request" frame from a child being restored.
                VerifyOrExit(aFrame->IsDataRequestCommand(), error = OT_ERROR_DROP);
                break;

            default:
                ExitNow(error = OT_ERROR_UNKNOWN_NEIGHBOR);
            }
        }
    }

    switch (mOperation)
    {
    case kOperationActiveScan:

        if (aFrame->GetType() == Frame::kFcfFrameBeacon)
        {
            mCounters.mRxBeacon++;
            mActiveScanHandler(mScanContext, aFrame);
            ExitNow();
        }

    // Fall-through

    case kOperationEnergyScan:

        // We can possibly receive a data frame while either active or
        // energy scan is ongoing. We continue to process the frame only
        // if the current scan channel matches `mChannel`.

        VerifyOrExit(mScanChannel == mChannel, mCounters.mRxOther++);
        break;

    case kOperationWaitingForData:

        if (dstaddr.mLength != 0)
        {
            mReceiveTimer.Stop();

#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
            mDelaySleep = aFrame->GetFramePending();
#endif
            FinishOperation();
        }

        break;

    default:
        break;
    }

    switch (aFrame->GetType())
    {
    case Frame::kFcfFrameMacCmd:
        if (HandleMacCommand(*aFrame) == OT_ERROR_DROP)
        {
            ExitNow(error = OT_ERROR_NONE);
        }

        receive = true;
        break;

    case Frame::kFcfFrameBeacon:
        mCounters.mRxBeacon++;
        receive = true;
        break;

    case Frame::kFcfFrameData:
        mCounters.mRxData++;
        receive = true;
        break;

    default:
        mCounters.mRxOther++;
        break;
    }

    if (receive)
    {
        otDumpDebgMac(GetInstance(), "RX", aFrame->GetHeader(), aFrame->GetLength());

        for (Receiver *receiver = mReceiveHead; receiver; receiver = receiver->mNext)
        {
            receiver->HandleReceivedFrame(*aFrame);
        }
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        if (aFrame == NULL)
        {
            otLogInfoMac(GetInstance(), "Frame rx failed, error:%s", otThreadErrorToString(error));
        }
        else
        {
            char stringBuffer[Frame::kInfoStringSize];

            otLogInfoMac(GetInstance(), "Frame rx failed, error:%s, %s", otThreadErrorToString(error),
                         aFrame->ToInfoString(stringBuffer, sizeof(stringBuffer)));

            OT_UNUSED_VARIABLE(stringBuffer);
        }

        switch (error)
        {
        case OT_ERROR_SECURITY:
            mCounters.mRxErrSec++;
            break;

        case OT_ERROR_FCS:
            mCounters.mRxErrFcs++;
            break;

        case OT_ERROR_NO_FRAME_RECEIVED:
            mCounters.mRxErrNoFrame++;
            break;

        case OT_ERROR_UNKNOWN_NEIGHBOR:
            mCounters.mRxErrUnknownNeighbor++;
            break;

        case OT_ERROR_INVALID_SOURCE_ADDRESS:
            mCounters.mRxErrInvalidSrcAddr++;
            break;

        case OT_ERROR_WHITELIST_FILTERED:
            mCounters.mRxWhitelistFiltered++;
            break;

        case OT_ERROR_DESTINATION_ADDRESS_FILTERED:
            mCounters.mRxDestAddrFiltered++;
            break;

        case OT_ERROR_DUPLICATED:
            mCounters.mRxDuplicated++;
            break;

        default:
            mCounters.mRxErrOther++;
            break;
        }
    }
}

otError Mac::HandleMacCommand(Frame &aFrame)
{
    otError error = OT_ERROR_NONE;
    uint8_t commandId;

    aFrame.GetCommandId(commandId);

    switch (commandId)
    {
    case Frame::kMacCmdBeaconRequest:
        mCounters.mRxBeaconRequest++;
        otLogInfoMac(GetInstance(), "Received Beacon Request");

        if ((mBeaconsEnabled)
#if OPENTHREAD_CONFIG_ENABLE_BEACON_RSP_IF_JOINABLE
            && (IsBeaconJoinable())
#endif // OPENTHREAD_CONFIG_ENABLE_BEACON_RSP_IF_JOINABLE
           )
        {
            StartOperation(kOperationTransmitBeacon);
        }

        ExitNow(error = OT_ERROR_DROP);

    case Frame::kMacCmdDataRequest:
        mCounters.mRxDataPoll++;
        break;

    default:
        mCounters.mRxOther++;
        break;
    }

exit:
    return error;
}

void Mac::SetPcapCallback(otLinkPcapCallback aPcapCallback, void *aCallbackContext)
{
    mPcapCallback = aPcapCallback;
    mPcapCallbackContext = aCallbackContext;
}

bool Mac::IsPromiscuous(void)
{
    return otPlatRadioGetPromiscuous(&GetInstance());
}

void Mac::SetPromiscuous(bool aPromiscuous)
{
    otPlatRadioSetPromiscuous(&GetInstance(), aPromiscuous);
    UpdateIdleMode();
}

bool Mac::RadioSupportsCsmaBackoff(void)
{
    // Check for either of the following conditions:
    //   1) Radio provides the CSMA backoff capability (i.e.,
    //      `OT_RADIO_CAPS_CSMA_BACKOFF` bit is set) or;
    //   2) It provides `OT_RADIO_CAPS_TRANSMIT_RETRIES` which
    //      indicates support for MAC retries along with CSMA
    //      backoff.

    return (otPlatRadioGetCaps(&GetInstance()) & (OT_RADIO_CAPS_TRANSMIT_RETRIES | OT_RADIO_CAPS_CSMA_BACKOFF)) != 0;
}

bool Mac::RadioSupportsRetries(void)
{
    return (otPlatRadioGetCaps(&GetInstance()) & OT_RADIO_CAPS_TRANSMIT_RETRIES) != 0;
}

void Mac::FillMacCountersTlv(NetworkDiagnostic::MacCountersTlv &aMacCounters) const
{
    aMacCounters.SetIfInUnknownProtos(mCounters.mRxOther);
    aMacCounters.SetIfInErrors(mCounters.mRxErrNoFrame + mCounters.mRxErrUnknownNeighbor + mCounters.mRxErrInvalidSrcAddr +
                               mCounters.mRxErrSec + mCounters.mRxErrFcs + mCounters.mRxErrOther);
    aMacCounters.SetIfOutErrors(mCounters.mTxErrCca);
    aMacCounters.SetIfInUcastPkts(mCounters.mRxUnicast);
    aMacCounters.SetIfInBroadcastPkts(mCounters.mRxBroadcast);
    aMacCounters.SetIfInDiscards(mCounters.mRxWhitelistFiltered + mCounters.mRxDestAddrFiltered + mCounters.mRxDuplicated);
    aMacCounters.SetIfOutUcastPkts(mCounters.mTxUnicast);
    aMacCounters.SetIfOutBroadcastPkts(mCounters.mTxBroadcast);
    aMacCounters.SetIfOutDiscards(0);
}

void Mac::ResetCounters(void)
{
    memset(&mCounters, 0, sizeof(mCounters));
}

#if OPENTHREAD_CONFIG_ENABLE_BEACON_RSP_IF_JOINABLE
bool Mac::IsBeaconJoinable(void)
{
    uint8_t numUnsecurePorts;
    bool joinable = false;

    GetNetif().GetIp6Filter().GetUnsecurePorts(numUnsecurePorts);

    if (numUnsecurePorts)
    {
        joinable = true;
    }
    else
    {
        joinable = false;
    }

    return joinable;
}
#endif // OPENTHREAD_CONFIG_ENABLE_BEACON_RSP_IF_JOINABLE

Mac &Mac::GetOwner(const Context &aContext)
{
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    Mac &mac = *static_cast<Mac *>(aContext.GetContext());
#else
    Mac &mac = otGetInstance()->mThreadNetif.GetMac();
    OT_UNUSED_VARIABLE(aContext);
#endif
    return mac;
}

const char *Mac::OperationToString(Operation aOperation)
{
    const char *retval = "";

    switch (aOperation)
    {
    case kOperationIdle:
        retval = "Idle";
        break;

    case kOperationActiveScan:
        retval = "ActiveScan";
        break;

    case kOperationEnergyScan:
        retval = "EnergyScan";
        break;

    case kOperationTransmitBeacon:
        retval = "TransmitBeacon";
        break;

    case kOperationTransmitData:
        retval = "TransmitData";
        break;

    case kOperationWaitingForData:
        retval = "WaitingForData";
        break;
    }

    return retval;
}

}  // namespace Mac
}  // namespace ot
