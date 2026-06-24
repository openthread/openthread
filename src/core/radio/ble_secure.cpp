/*
 *  Copyright (c) 2023, The OpenThread Authors.
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

#include "ble_secure.hpp"
#include "common/error.hpp"

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

#include <openthread/platform/ble.h>

#include "instance/instance.hpp"

using namespace ot;

/**
 * @file
 *   This file implements the secure Ble agent.
 */

namespace ot {
namespace Ble {

RegisterLogModule("BleSecure");

BleSecure::BleSecure(Instance &aInstance)
    : InstanceLocator(aInstance)
    , MeshCoP::Tls::Extension(mTls)
    , mTls(aInstance, kNoLinkSecurity, *this)
    , mTlvMode(false)
    , mReceivedMessage(nullptr)
    , mSendMessage(nullptr)
    , mTransmitTask(aInstance)
    , mBleState(kStopped)
    , mIsBleAdvRequested(false)
    , mMtuSize(kInitialMtuSize)
{
}

Error BleSecure::Start(ConnectCallback aConnectHandler, ReceiveCallback aReceiveHandler, bool aTlvMode, void *aContext)
{
    Error    error             = kErrorNone;
    uint16_t advertisementLen  = 0;
    uint8_t *advertisementData = nullptr;

    VerifyOrExit(mBleState == kStopped, error = kErrorAlready);

    mConnectCallback.Set(aConnectHandler, aContext);
    mReceiveCallback.Set(aReceiveHandler, aContext);
    mTlvMode = aTlvMode;
    mMtuSize = kInitialMtuSize;

    SuccessOrExit(error = otPlatBleEnable(&GetInstance()));

    SuccessOrExit(error = otPlatBleGetAdvertisementBuffer(&GetInstance(), &advertisementData));
    SuccessOrExit(error = Get<MeshCoP::TcatAgent>().GetAdvertisementData(advertisementLen, advertisementData));
    VerifyOrExit(advertisementData != nullptr, error = kErrorFailed);
    SuccessOrExit(error = otPlatBleGapAdvSetData(&GetInstance(), advertisementData, advertisementLen));

    SuccessOrExit(error = mTls.Open(HandleTransport, this));
    mTls.SetReceiveCallback(HandleTlsReceive, this);
    mTls.SetConnectCallback(HandleTlsConnectEvent, this);

    // attempt to start BLE advertising only if everything else succeeded.
    mBleState          = kNotAdvertising;
    mIsBleAdvRequested = true;
    error              = SetRequestedBleAdvertisementsState();

exit:
    if (error != kErrorNone && error != kErrorAlready)
    {
        mTls.Close();
        mBleState = kStopped;
    }
    return error;
}

Error BleSecure::TcatStart(const MeshCoP::TcatAgent::JoinCallback aHandler)
{
    Error error;

    VerifyOrExit(mBleState != kStopped && mTlvMode, error = kErrorInvalidState);

    error = Get<MeshCoP::TcatAgent>().Start(mReceiveCallback.GetHandler(), aHandler, mReceiveCallback.GetContext());

exit:
    return error;
}

void BleSecure::Stop(void)
{
    VerifyOrExit(mBleState != kStopped);

    // Even if stop-advertisements or disable BLE would fail, we continue closing TLS and stopping TCAT agent.
    IgnoreError(otPlatBleGapAdvStop(&GetInstance()));
    IgnoreError(otPlatBleDisable(&GetInstance()));
    mBleState          = kStopped;
    mIsBleAdvRequested = false;
    mMtuSize           = kInitialMtuSize;

    mTls.Close();
    Get<MeshCoP::TcatAgent>().Stop();

    mTransmitQueue.DequeueAndFreeAll();

    mConnectCallback.Clear();
    mReceiveCallback.Clear();

    FreeMessage(mReceivedMessage);
    mReceivedMessage = nullptr;
    FreeMessage(mSendMessage);
    mSendMessage = nullptr;

exit:
    return;
}

Error BleSecure::TcatActive(bool aActive, uint32_t aDelayMs, uint32_t aDurationMs)
{
    Error error;

    VerifyOrExit(mBleState != kStopped, error = kErrorInvalidState);

    if (aActive)
    {
        error = Get<MeshCoP::TcatAgent>().Activate(aDelayMs, aDurationMs);
    }
    else
    {
        error = Get<MeshCoP::TcatAgent>().Standby();
    }

exit:
    return error;
}

Error BleSecure::Connect(void)
{
    Ip6::SockAddr sockaddr;
    Error         error;

    VerifyOrExit(mBleState == kConnected, error = kErrorInvalidState);

    error = mTls.Connect(sockaddr);

exit:
    return error;
}

void BleSecure::Disconnect(void)
{
    if (mTls.IsConnected())
    {
        mTls.Disconnect();
    }

    if (mBleState == kConnected)
    {
        // Request the platform to close the BLE connection. When the platform signals completion
        // (asynchronously) it calls otPlatBleGapOnDisconnected() -> #HandleBleDisconnected(), which
        // in turn calls this method again - now with mBleState no longer kConnected - to invoke below
        // mConnectCallback and update the advertisement data. We therefore return early here when
        // the platform accepted the request, to avoid invoking the callback (and updating the
        // advertisement data) twice. If the platform did not start the disconnection, no completion
        // callback will follow and we handle mConnectCallback directly below.
        VerifyOrExit(otPlatBleGapDisconnect(&GetInstance()) != kErrorNone);
    }

    mConnectCallback.InvokeIfSet(&GetInstance(), false, false);
    // Update advertisement data
    IgnoreError(NotifyAdvertisementChanged());

exit:
    return;
}

Error BleSecure::NotifyAdvertisementChanged(void)
{
    Error    error             = kErrorNone;
    uint16_t advertisementLen  = 0;
    uint8_t *advertisementData = nullptr;

    VerifyOrExit(mBleState != kStopped);
    SuccessOrExit(error = otPlatBleGetAdvertisementBuffer(&GetInstance(), &advertisementData));
    SuccessOrExit(error = Get<MeshCoP::TcatAgent>().GetAdvertisementData(advertisementLen, advertisementData));

    if (mBleState == kAdvertising)
    {
        SuccessOrExit(error = otPlatBleGapAdvUpdateData(&GetInstance(), advertisementData, advertisementLen));
    }
    else
    {
        // Any other state: update buffered data for when advertising resumes.
        SuccessOrExit(error = otPlatBleGapAdvSetData(&GetInstance(), advertisementData, advertisementLen));
    }

exit:
    return error;
}

void BleSecure::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.ContainsAny(kEventActiveDatasetChanged | kEventThreadRoleChanged))
    {
        IgnoreError(NotifyAdvertisementChanged());
    }
}

void BleSecure::NotifySendAdvertisements(bool aSendAdvertisements)
{
    mIsBleAdvRequested = aSendAdvertisements;
    IgnoreError(SetRequestedBleAdvertisementsState());
}

// performs platform calls to start or stop BLE advertisements as requested, and if successful
// update mBleState to reflect actual state of kAdvertising / kNotAdvertising.
Error BleSecure::SetRequestedBleAdvertisementsState(void)
{
    Error error = kErrorNone;

    // Must not make GapAdv platform calls when kStopped, or kConnected.
    if (mIsBleAdvRequested && mBleState == kNotAdvertising)
    {
        SuccessOrExit(error = otPlatBleGapAdvStart(&GetInstance(), OT_BLE_ADV_INTERVAL_DEFAULT));
        mBleState = kAdvertising;
    }
    else if (!mIsBleAdvRequested && mBleState == kAdvertising)
    {
        SuccessOrExit(error = otPlatBleGapAdvStop(&GetInstance()));
        mBleState = kNotAdvertising;
    }

exit:
    LogWarnOnError(error, "start/stop advertisements");
    return error;
}

void BleSecure::SetPsk(const MeshCoP::JoinerPskd &aPskd)
{
    static_assert(static_cast<uint16_t>(MeshCoP::JoinerPskd::kMaxLength) <=
                      static_cast<uint16_t>(MeshCoP::Tls::kPskMaxLength),
                  "The maximum length of TLS PSK is smaller than joiner PSKd");

    SuccessOrAssert(mTls.SetPsk(reinterpret_cast<const uint8_t *>(aPskd.GetAsCString()), aPskd.GetLength()));
}

Error BleSecure::SendMessage(ot::Message &aMessage)
{
    Error error = kErrorNone;

    VerifyOrExit(IsConnected(), error = kErrorInvalidState);
    if (mSendMessage == nullptr)
    {
        mSendMessage = Get<MessagePool>().Allocate(Message::kTypeBle);
        VerifyOrExit(mSendMessage != nullptr, error = kErrorNoBufs);
    }
    SuccessOrExit(error = mSendMessage->AppendBytesFromMessage(aMessage, 0, aMessage.GetLength()));
    SuccessOrExit(error = Flush());

    aMessage.Free();
exit:
    return error;
}

Error BleSecure::Send(uint8_t *aBuf, uint16_t aLength)
{
    Error error = kErrorNone;

    VerifyOrExit(IsConnected(), error = kErrorInvalidState);
    if (mSendMessage == nullptr)
    {
        mSendMessage = Get<MessagePool>().Allocate(Message::kTypeBle);
        VerifyOrExit(mSendMessage != nullptr, error = kErrorNoBufs);
    }
    SuccessOrExit(error = mSendMessage->AppendBytes(aBuf, aLength));

exit:
    return error;
}

Error BleSecure::SendApplicationTlv(MeshCoP::TcatAgent::TcatApplicationProtocol aTcatApplicationProtocol,
                                    uint8_t                                    *aBuf,
                                    uint16_t                                    aLength)
{
    Error error = kErrorNone;

    VerifyOrExit(aTcatApplicationProtocol != MeshCoP::TcatAgent::kApplicationProtocolNone &&
                     ((aTcatApplicationProtocol != MeshCoP::TcatAgent::kApplicationProtocolStatus &&
                       aTcatApplicationProtocol != MeshCoP::TcatAgent::kApplicationProtocolResponse) ||
                      Get<MeshCoP::TcatAgent>().GetApplicationResponsePending()),
                 error = kErrorRejected);

    if (aLength > Tlv::kBaseTlvMaxLength)
    {
        ot::ExtendedTlv tlv;

        tlv.SetType(static_cast<uint8_t>(aTcatApplicationProtocol));
        tlv.SetLength(aLength);
        SuccessOrExit(error = Send(reinterpret_cast<uint8_t *>(&tlv), sizeof(tlv)));
    }
    else
    {
        ot::Tlv tlv;

        tlv.SetType(static_cast<uint8_t>(aTcatApplicationProtocol));
        tlv.SetLength(static_cast<uint8_t>(aLength));
        SuccessOrExit(error = Send(reinterpret_cast<uint8_t *>(&tlv), sizeof(tlv)));
    }

    SuccessOrExit(error = Send(aBuf, aLength));

    if (aTcatApplicationProtocol == MeshCoP::TcatAgent::kApplicationProtocolStatus ||
        aTcatApplicationProtocol == MeshCoP::TcatAgent::kApplicationProtocolResponse)
    {
        Get<MeshCoP::TcatAgent>().NotifyApplicationResponseSent();
    }

exit:
    return error;
}

Error BleSecure::Flush(void)
{
    Error        error   = kErrorNone;
    ot::Message *message = nullptr;
    uint16_t     length;

    VerifyOrExit(mSendMessage != nullptr);
    VerifyOrExit(IsConnected(), error = kErrorInvalidState);
    length = mSendMessage->GetLength();

    // Split send buffer in chunks which can later be processed by mTls.Send(..)
    while (length > kTlsDataMaxSize)
    {
        VerifyOrExit((message = Get<MessagePool>().Allocate(Message::kTypeBle, 0)) != nullptr, error = kErrorNoBufs);
        SuccessOrExit(error = message->AppendBytesFromMessage(*mSendMessage, 0, kTlsDataMaxSize));

        // We accept an expensive copy operation in favor of optimal buffer usage for long messages
        mSendMessage->WriteBytesFromMessage(0, *mSendMessage, kTlsDataMaxSize, length - kTlsDataMaxSize);
        length -= kTlsDataMaxSize;

        // Should never fail since we are decreasing the length of the message
        IgnoreError(mSendMessage->SetLength(length));
        mTransmitQueue.Enqueue(*message);
        mTransmitTask.Post();
        message = nullptr;
    }

    VerifyOrExit(length != 0, error = kErrorNone);
    mTransmitQueue.Enqueue(*mSendMessage);
    mTransmitTask.Post();
    mSendMessage = nullptr;

exit:
    FreeMessage(message);

    if (mSendMessage != nullptr)
    {
        mSendMessage->Free();
        mSendMessage = nullptr;
    }

    return error;
}

void BleSecure::HandleBleReceive(uint8_t *aBuf, uint16_t aLength)
{
    ot::Message     *message = nullptr;
    Ip6::MessageInfo messageInfo;
    Error            error = kErrorNone;

    if ((message = Get<MessagePool>().Allocate(Message::kTypeBle, 0)) == nullptr)
    {
        ExitNow(error = kErrorNoBufs);
    }
    SuccessOrExit(error = message->AppendBytes(aBuf, aLength));

    // Cannot call Receive(..) directly because Setup(..) and mState are private
    mTls.HandleReceive(*message, messageInfo);

exit:
    // if BLE packets go missing, the TLS layer will catch the damaged records - so we just warn here.
    LogWarnOnError(error, "HandleBleReceive");
    FreeMessage(message);
}

void BleSecure::HandleBleConnected(uint16_t aConnectionId)
{
    OT_UNUSED_VARIABLE(aConnectionId);

    // if getting ATT MTU size fails, it stays at the default
    mMtuSize = kInitialMtuSize;
    IgnoreError(otPlatBleGattMtuGet(&GetInstance(), &mMtuSize));
    mBleState = kConnected;
    mConnectCallback.InvokeIfSet(&GetInstance(), IsConnected(), true);
}

void BleSecure::HandleBleDisconnected(uint16_t aConnectionId)
{
    OT_UNUSED_VARIABLE(aConnectionId);

    mBleState = kNotAdvertising; // per otPlatBleGapAdvStart() API, advertising stopped already when client connected.
    mMtuSize  = kInitialMtuSize;

    Disconnect(); // Stop TLS connection and update advertisement data

    // Resume advertising (or fulfill a different advertising state requested while a client was connected).
    IgnoreError(SetRequestedBleAdvertisementsState());
}

void BleSecure::HandleBleMtuUpdate(uint16_t aMtu)
{
    OT_ASSERT(aMtu >= kMinMtuSize);
    // if higher MTU is available than configured, we still obey our configured maximum.
    if (aMtu > kMaxMtuSize)
    {
        aMtu = kMaxMtuSize;
    }
    mMtuSize = aMtu;
}

void BleSecure::HandleTlsConnectEvent(MeshCoP::Tls::ConnectEvent aEvent, void *aContext)
{
    return static_cast<BleSecure *>(aContext)->HandleTlsConnectEvent(aEvent);
}

void BleSecure::HandleTlsConnectEvent(MeshCoP::Tls::ConnectEvent aEvent)
{
    if (aEvent == MeshCoP::Tls::kConnected)
    {
        Error err;

        if (mReceivedMessage == nullptr)
        {
            mReceivedMessage = Get<MessagePool>().Allocate(Message::kTypeBle);
        }
        if (mReceivedMessage == nullptr)
        {
            err = kErrorNoBufs;
        }
        else
        {
            err = Get<MeshCoP::TcatAgent>().Connected(*this);
        }

        if (err != kErrorNone)
        {
            mTls.Disconnect(); // must not use Close(), so that next Commissioner can connect
            LogWarn("Rejected TCAT Commissioner, error: %s", ErrorToString(err));
            ExitNow();
        }
    }
    else
    {
        FreeMessage(mReceivedMessage);
        mReceivedMessage = nullptr;
        FreeMessage(mSendMessage);
        mSendMessage = nullptr;
        Get<MeshCoP::TcatAgent>().Disconnected();
    }

    mConnectCallback.InvokeIfSet(&GetInstance(), aEvent == MeshCoP::Tls::kConnected, mBleState == kConnected);

exit:
    return;
}

void BleSecure::HandleTlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength)
{
    return static_cast<BleSecure *>(aContext)->HandleTlsReceive(aBuf, aLength);
}

void BleSecure::HandleTlsReceive(uint8_t *aBuf, uint16_t aLength)
{
    Error    error = kErrorNone;
    Tlv      tlv;
    uint32_t requiredBytes;
    uint32_t offset;

    VerifyOrExit(mReceivedMessage != nullptr);
    DumpDebg("Rx", aBuf, aLength);

    if (!mTlvMode)
    {
        SuccessOrExit(error = mReceivedMessage->AppendBytes(aBuf, aLength));
        mReceiveCallback.InvokeIfSet(&GetInstance(), mReceivedMessage, 0, OT_TCAT_APPLICATION_PROTOCOL_NONE);
        IgnoreError(mReceivedMessage->SetLength(0));
        ExitNow();
    }

    requiredBytes = sizeof(Tlv);

    while (aLength > 0)
    {
        if (mReceivedMessage->GetLength() < requiredBytes)
        {
            uint32_t missingBytes = requiredBytes - mReceivedMessage->GetLength();

            if (missingBytes > aLength)
            {
                error = mReceivedMessage->AppendBytes(aBuf, aLength);
                ExitNow();
            }

            SuccessOrExit(error = mReceivedMessage->AppendBytes(aBuf, static_cast<uint16_t>(missingBytes)));
            aLength -= missingBytes;
            aBuf += missingBytes;
        }

        IgnoreError(mReceivedMessage->Read(0, tlv));

        if (tlv.IsExtended())
        {
            ExtendedTlv extTlv;

            requiredBytes = sizeof(extTlv);

            if (mReceivedMessage->GetLength() < requiredBytes)
            {
                continue;
            }

            IgnoreError(mReceivedMessage->Read(0, extTlv));
            requiredBytes = extTlv.GetSize();
            offset        = sizeof(extTlv);
        }
        else
        {
            requiredBytes = tlv.GetSize();
            offset        = sizeof(tlv);
        }

        if (mReceivedMessage->GetLength() < requiredBytes)
        {
            continue;
        }

        // TLV fully loaded - let TCAT agent handle it, if connected

        if (Get<MeshCoP::TcatAgent>().IsConnected())
        {
            Error errorTcatAgent = kErrorNone;

            IgnoreError(Flush());

            if (mSendMessage == nullptr)
            {
                mSendMessage = Get<MessagePool>().Allocate(Message::kTypeBle);
                VerifyOrExit(mSendMessage != nullptr, error = kErrorNoBufs);
            }

            errorTcatAgent = Get<MeshCoP::TcatAgent>().HandleSingleTlv(*mReceivedMessage, *mSendMessage);

            switch (errorTcatAgent)
            {
            case kErrorNone:
                SuccessOrExit(error = Flush());
                break;

            case kErrorAbort:
                // `kErrorAbort` indicates that a Disconnect command TLV
                // has been received.
                LogInfo("Disconnecting TCAT client.");
                errorTcatAgent = kErrorNone;

                OT_FALL_THROUGH;

            default:
                LogWarnOnError(errorTcatAgent, "HandleSingleTlv");

                // BleSecure is disconnected but not stopped here, it
                // must remain active in advertising state and must be
                // ready to receive a next TCAT commissioner.

                Disconnect();
                ExitNow();
            }
        }
        else
        {
            // If the TCAT agent is not connected - do callback using
            // the TLV's value as the message
            mReceivedMessage->SetOffset(static_cast<uint16_t>(offset));
            mReceiveCallback.InvokeIfSet(&GetInstance(), mReceivedMessage, static_cast<int32_t>(offset),
                                         OT_TCAT_APPLICATION_PROTOCOL_NONE);
        }

        IgnoreError(mReceivedMessage->SetLength(0));
        requiredBytes = sizeof(Tlv);

    } // while loop

exit:
    if (error != kErrorNone)
    {
        // In this very rare case, a partial TLV is received, or a TLV
        // has been fully dropped, or `Flush()` failed. `mSendMessage` is
        // most likely not initialized; so appending a `GeneralError`
        // status TLV to `mSendMessage` would fail also. In this case
        // it's not possible to recover TLV integrity and client/server
        // sync. It's handled by logging the error and (necessarily)
        // closing the secure connection.

        LogCritOnError(error, "HandleTlsReceive");
        Disconnect();
    }
}

void BleSecure::HandleTransmit(void)
{
    Error        error   = kErrorNone;
    ot::Message *message = mTransmitQueue.GetHead();

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)
    uint16_t len;
    uint8_t  buf[kTlsDataMaxSize];
#endif

    VerifyOrExit(message != nullptr);
    mTransmitQueue.Dequeue(*message);

    if (mTransmitQueue.GetHead() != nullptr)
    {
        mTransmitTask.Post();
    }

    SuccessOrExit(error = mTls.Send(*message));

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)
    len = message->ReadBytes(message->GetOffset(), buf, sizeof(buf));
    DumpDebg("Tx", buf, len);
#endif

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "transmit");
}

Error BleSecure::HandleTransport(void *aContext, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    return static_cast<BleSecure *>(aContext)->HandleTransport(aMessage);
}

Error BleSecure::HandleTransport(ot::Message &aMessage)
{
    Error       error = kErrorNone;
    RadioPacket packet;
    OffsetRange offsetRange;

    offsetRange.InitFromMessageFullLength(aMessage);

    while (!offsetRange.IsEmpty())
    {
        packet.mLength = Min(offsetRange.GetLength(), Min<uint16_t>(mMtuSize - kGattOverhead, kPacketBufferSize));

        IgnoreError(aMessage.ReadAndAdvance(offsetRange, mPacketBuffer, packet.mLength));
        packet.mValue = mPacketBuffer;
        packet.mPower = OT_BLE_DEFAULT_POWER;

        SuccessOrExit(error = otPlatBleGattServerIndicate(&GetInstance(), kTxBleHandle, &packet));
    }

    aMessage.Free();

exit:
    return error;
}

} // namespace Ble
} // namespace ot

void otPlatBleGattServerOnWriteRequest(otInstance *aInstance, uint16_t aHandle, const otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aHandle); // Only a single handle is expected for RX

    VerifyOrExit(aPacket != nullptr);
    AsCoreType(aInstance).Get<Ble::BleSecure>().HandleBleReceive(aPacket->mValue, aPacket->mLength);
exit:
    return;
}

void otPlatBleGapOnConnected(otInstance *aInstance, uint16_t aConnectionId)
{
    AsCoreType(aInstance).Get<Ble::BleSecure>().HandleBleConnected(aConnectionId);
}

void otPlatBleGapOnDisconnected(otInstance *aInstance, uint16_t aConnectionId)
{
    AsCoreType(aInstance).Get<Ble::BleSecure>().HandleBleDisconnected(aConnectionId);
}

void otPlatBleGattOnMtuUpdate(otInstance *aInstance, uint16_t aMtu)
{
    AsCoreType(aInstance).Get<Ble::BleSecure>().HandleBleMtuUpdate(aMtu);
}

#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
