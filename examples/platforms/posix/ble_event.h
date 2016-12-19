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
 *   This defines the eventing mechanism between NimBLE host task
 *   and the main openthread task.
 */

#ifndef BLE_EVENT_H_
#define BLE_EVENT_H_

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/platform/ble.h>

#ifdef __cplusplus

class BleEvent
{
 protected:
    uint8_t     mEvent;
    otInstance *mInstance;

    typedef enum {
        kBleEventGapOnConnected,
        kBleEventGapOnDisconnected,
        kBleEventGapOnAdvReceived,
        kBleEventGapOnScanRespReceived,

        kBleEventGattClientOnReadResponse,
        kBleEventGattClientOnWriteResponse,
        kBleEventGattClientOnSubscribeResponse,
        kBleEventGattClientOnIndication,
        kBleEventGattClientOnServiceDiscovered,
        kBleEventGattClientOnCharacteristicsDiscoverDone,
        kBleEventGattClientOnDescriptorsDiscoverDone,
        kBleEventGattClientOnMtuExchangeResponse,

        kBleEventGattServerOnIndicationConfirmation,
        kBleEventGattServerOnWriteRequest,
        kBleEventGattServerOnReadRequest,
        kBleEventGattServerOnSubscribeRequest,

        kBleEventL2capOnConnectionRequest,
        kBleEventL2capOnConnectionResponse,
        kBleEventL2capOnSduReceived,
        kBleEventL2capOnSduSent,
        kBleEventL2capOnDisconnect,
    } BleEventType;

    BleEvent(BleEventType aEventType, otInstance *aInstance) :
        mEvent(aEventType),
        mInstance(aInstance)
    {
    }

    void CopyAddress(otPlatBleDeviceAddr *aDest, otPlatBleDeviceAddr *aSource)
    {
        memcpy(aDest, aSource, sizeof(otPlatBleDeviceAddr));
    }

    void CopyPacket(otBleRadioPacket *aDest, otBleRadioPacket *aSource)
    {
        uint16_t length = aSource->mLength;;
        aDest->mLength = length;
        aDest->mPower = aSource->mPower;
        aDest->mValue = new uint8_t[length];
        memcpy(aDest->mValue, aSource->mValue, length);
    }

    void CopyUuid(otPlatBleUuid *aDest, otPlatBleUuid *aSource)
    {
        aDest->mType = aSource->mType;
        switch(aSource->mType)
        {
        case OT_BLE_UUID_TYPE_16:
            aDest->mValue.mUuid16 = aSource->mValue.mUuid16;
            break;

        case OT_BLE_UUID_TYPE_32:
            aDest->mValue.mUuid32 = aSource->mValue.mUuid32;
            break;

        case OT_BLE_UUID_TYPE_128:
            aDest->mValue.mUuid128 = new uint8_t[16];
            memcpy(aDest->mValue.mUuid128, aSource->mValue.mUuid128, 16);
            break;

        default:
            break;
        }
    }

    void FreePacket(otBleRadioPacket &aPacket)
    {
        if (aPacket.mValue)
        {
            delete[] aPacket.mValue;
        }
    }

    void FreeUuid(otPlatBleUuid &aUuid)
    {
        if (aUuid.mType == OT_BLE_UUID_TYPE_128)
        {
            delete[] aUuid.mValue.mUuid128;
        }
    }

 public:
    virtual ~BleEvent() {}
    virtual void Dispatch() = 0;
};

class BleEventGapOnConnected : public BleEvent
{
    uint16_t mConnectionId;

 public:
    BleEventGapOnConnected(otInstance *aInstance, uint16_t aConnectionId) :
        BleEvent(kBleEventGapOnConnected, aInstance),
        mConnectionId(aConnectionId)
    {
    }

    void Dispatch()
    {
        otPlatBleGapOnConnected(mInstance, mConnectionId);
    }
};

class BleEventGapOnDisconnected : public BleEvent
{
    uint16_t mConnectionId;

 public:
    BleEventGapOnDisconnected(otInstance *aInstance, uint16_t aConnectionId) :
        BleEvent(kBleEventGapOnDisconnected, aInstance),
        mConnectionId(aConnectionId)
    {
    }

    void Dispatch()
    {
        otPlatBleGapOnDisconnected(mInstance, mConnectionId);
    }
};


class BleEventGapOnAdvReceived : public BleEvent
{
    otPlatBleDeviceAddr mAddress;
    otBleRadioPacket    mPacket;

    ~BleEventGapOnAdvReceived()
    {
        FreePacket(mPacket);
    }

 public:
    BleEventGapOnAdvReceived(otInstance *aInstance,
                             otPlatBleDeviceAddr *aAddress,
                             otBleRadioPacket *aPacket) :
        BleEvent(kBleEventGapOnAdvReceived, aInstance)
    {
        CopyAddress(&mAddress, aAddress);
        CopyPacket(&mPacket, aPacket);
    }

    void Dispatch()
    {
        otPlatBleGapOnAdvReceived(mInstance, &mAddress, &mPacket);
    }
};

class BleEventGapOnScanRespReceived : public BleEvent
{
    otPlatBleDeviceAddr mAddress;
    otBleRadioPacket    mPacket;

    ~BleEventGapOnScanRespReceived()
    {
        FreePacket(mPacket);
    }

 public:
    BleEventGapOnScanRespReceived(otInstance *aInstance,
                                  otPlatBleDeviceAddr *aAddress,
                                  otBleRadioPacket *aPacket) :
        BleEvent(kBleEventGapOnScanRespReceived, aInstance)
    {
        CopyAddress(&mAddress, aAddress);
        CopyPacket(&mPacket, aPacket);
    }

    void Dispatch()
    {
        otPlatBleGapOnScanRespReceived(mInstance, &mAddress, &mPacket);
    }
};

class BleEventGattClientOnReadResponse : public BleEvent
{
    otBleRadioPacket mPacket;

    ~BleEventGattClientOnReadResponse()
    {
        FreePacket(mPacket);
    }

 public:
    BleEventGattClientOnReadResponse(otInstance *aInstance,
                                     otBleRadioPacket *aPacket) :
        BleEvent(kBleEventGattClientOnReadResponse, aInstance)
    {
        CopyPacket(&mPacket, aPacket);
    }

    void Dispatch()
    {
        otPlatBleGattClientOnReadResponse(mInstance, &mPacket);
    }
};

class BleEventGattClientOnWriteResponse : public BleEvent
{
    uint16_t mHandle;

 public:
    BleEventGattClientOnWriteResponse(otInstance *aInstance,
                                      uint16_t aHandle) :
        BleEvent(kBleEventGattClientOnWriteResponse, aInstance),
        mHandle(aHandle)
    {
    }

    void Dispatch()
    {
        otPlatBleGattClientOnWriteResponse(mInstance, mHandle);
    }
};

class BleEventGattClientOnSubscribeResponse : public BleEvent
{
    uint16_t mHandle;

 public:
    BleEventGattClientOnSubscribeResponse(otInstance *aInstance,
                                      uint16_t aHandle) :
        BleEvent(kBleEventGattClientOnSubscribeResponse, aInstance),
        mHandle(aHandle)
    {
    }

    void Dispatch()
    {
        otPlatBleGattClientOnSubscribeResponse(mInstance, mHandle);
    }
};

class BleEventGattClientOnIndication : public BleEvent
{
    uint16_t         mHandle;
    otBleRadioPacket mPacket;

    ~BleEventGattClientOnIndication()
    {
        FreePacket(mPacket);
    }

 public:
    BleEventGattClientOnIndication(otInstance *aInstance,
                                   uint16_t aHandle,
                                   otBleRadioPacket *aPacket) :
        BleEvent(kBleEventGattClientOnIndication, aInstance),
        mHandle(aHandle)
    {
        CopyPacket(&mPacket, aPacket);
    }

    void Dispatch()
    {
        otPlatBleGattClientOnIndication(mInstance, mHandle, &mPacket);
    }
};

class BleEventGattClientOnServiceDiscovered : public BleEvent
{
    uint16_t    mStartHandle;
    uint16_t    mEndHandle;
    uint16_t    mServiceUuid;
    otError     mError;

 public:

    BleEventGattClientOnServiceDiscovered(otInstance *aInstance,
                                          uint16_t    aStartHandle,
                                          uint16_t    aEndHandle,
                                          uint16_t    aServiceUuid,
                                          otError     aError) :
        BleEvent(kBleEventGattClientOnServiceDiscovered, aInstance),
        mStartHandle(aStartHandle),
        mEndHandle(aEndHandle),
        mServiceUuid(aServiceUuid),
        mError(aError)
    {
    }

    void Dispatch()
    {
        otPlatBleGattClientOnServiceDiscovered(mInstance, mStartHandle, mEndHandle,
                                           mServiceUuid, mError);
    }
};

class BleEventGattClientOnCharacteristicsDiscoverDone : public BleEvent
{
    otPlatBleGattCharacteristic *mChars;
    uint16_t                     mCount;
    otError                      mError;

    ~BleEventGattClientOnCharacteristicsDiscoverDone()
    {
        if (mChars)
        {
            for (int i = 0; i < mCount; i++)
            {
                FreeUuid(mChars[i].mUuid);
            }
            delete[] mChars;
        }
    }

 public:

    BleEventGattClientOnCharacteristicsDiscoverDone(otInstance *                 aInstance,
                                                    otPlatBleGattCharacteristic *aChars,
                                                    uint16_t                     aCount,
                                                    otError                      aError) :
        BleEvent(kBleEventGattClientOnCharacteristicsDiscoverDone, aInstance),
        mCount(aCount),
        mError(aError)
    {
        mChars = new otPlatBleGattCharacteristic[aCount];
        memcpy(mChars, aChars, aCount*sizeof(otPlatBleGattCharacteristic));

        // foreach characteristic, copy uuid128 
        for (int i = 0; i < aCount; i++)
        {
            CopyUuid(&mChars[i].mUuid, &aChars[i].mUuid);
        }
    }

    void Dispatch()
    {
        otPlatBleGattClientOnCharacteristicsDiscoverDone(mInstance, mChars, mCount, mError);
    }
};

class BleEventGattClientOnDescriptorsDiscoverDone : public BleEvent
{
    otPlatBleGattDescriptor *mDescs;
    uint16_t                 mCount;
    otError                  mError;

    ~BleEventGattClientOnDescriptorsDiscoverDone()
    {
        if (mDescs)
        {
            for (int i = 0; i < mCount; i++)
            {
                FreeUuid(mDescs[i].mUuid);
            }
            delete[] mDescs;
        }
    }

 public:
    BleEventGattClientOnDescriptorsDiscoverDone(otInstance *             aInstance,
                                                otPlatBleGattDescriptor *aDescs,
                                                uint16_t                 aCount,
                                                otError                  aError) :
        BleEvent(kBleEventGattClientOnDescriptorsDiscoverDone, aInstance),
        mCount(aCount),
        mError(aError)
    {
        mDescs = new otPlatBleGattDescriptor[aCount];
        memcpy(mDescs, aDescs, aCount*sizeof(otPlatBleGattDescriptor));

        for (int i = 0; i < aCount; i++)
        {
            CopyUuid(&mDescs[i].mUuid, &aDescs[i].mUuid);
        }
    }

    void Dispatch()
    {
        otPlatBleGattClientOnDescriptorsDiscoverDone(mInstance, mDescs, mCount, mError);
    }
};

class BleEventGattClientOnMtuExchangeResponse : public BleEvent
{
    uint16_t mMtu;
    otError  mError;

 public:
    BleEventGattClientOnMtuExchangeResponse(otInstance *aInstance,
                                            uint16_t aMtu,
                                            otError aError) :
        BleEvent(kBleEventGattClientOnMtuExchangeResponse, aInstance),
        mMtu(aMtu),
        mError(aError)
    {
    }

    void Dispatch()
    {
        otPlatBleGattClientOnMtuExchangeResponse(mInstance, mMtu, mError);
    }
};


class BleEventGattServerOnWriteRequest : public BleEvent
{
    uint16_t         mHandle;
    otBleRadioPacket mPacket;

    ~BleEventGattServerOnWriteRequest()
    {
        FreePacket(mPacket);
    }

 public:

    BleEventGattServerOnWriteRequest(otInstance *aInstance,
                                     uint16_t aHandle,
                                     otBleRadioPacket *aPacket) :
        BleEvent(kBleEventGattServerOnWriteRequest, aInstance),
        mHandle(aHandle)
    {
        CopyPacket(&mPacket, aPacket);
    }

    void Dispatch()
    {
        otPlatBleGattServerOnWriteRequest(mInstance, mHandle, &mPacket);
    }
};


class BleEventGattServerOnReadRequest : public BleEvent
{
    uint16_t         mHandle;
    otBleRadioPacket mPacket;

    ~BleEventGattServerOnReadRequest()
    {
        FreePacket(mPacket);
    }

 public:

    BleEventGattServerOnReadRequest(otInstance *aInstance,
                                     uint16_t aHandle,
                                     otBleRadioPacket *aPacket) :
        BleEvent(kBleEventGattServerOnReadRequest, aInstance),
        mHandle(aHandle)
    {
        CopyPacket(&mPacket, aPacket);
    }

    void Dispatch()
    {
        otPlatBleGattServerOnReadRequest(mInstance, mHandle, &mPacket);
    }
};

class BleEventGattServerOnSubscribeRequest : public BleEvent
{
    uint16_t mHandle;
    bool     mSubscribing;

 public:
     BleEventGattServerOnSubscribeRequest(otInstance *aInstance,
                                          uint16_t aHandle,
                                          bool aSubscribing) :
        BleEvent(kBleEventGattServerOnSubscribeRequest, aInstance),
        mHandle(aHandle),
        mSubscribing(aSubscribing)
    {
    }

    void Dispatch()
    {
        otPlatBleGattServerOnSubscribeRequest(mInstance, mHandle, mSubscribing);
    }
};

class BleEventGattServerOnIndicationConfirmation : public BleEvent
{
    uint16_t mHandle;

 public:
    BleEventGattServerOnIndicationConfirmation(otInstance *aInstance,
                                               uint16_t aHandle) :
        BleEvent(kBleEventGattServerOnIndicationConfirmation, aInstance),
        mHandle(aHandle)
    {
    }

    void Dispatch()
    {
        otPlatBleGattServerOnIndicationConfirmation(mInstance, mHandle);
    }
};

class BleEventL2capOnDisconnect : public BleEvent
{
    uint16_t    mLocalCid;
    uint16_t    mPeerCid;

 public:
    BleEventL2capOnDisconnect(otInstance *aInstance,
                              uint16_t aLocalCid,
                              uint16_t aPeerCid) :
        BleEvent(kBleEventL2capOnDisconnect, aInstance),
        mLocalCid(aLocalCid),
        mPeerCid(aPeerCid)
    {
    }

    void Dispatch()
    {
        otPlatBleL2capOnDisconnect(mInstance, mLocalCid, mPeerCid);
    }
};

class BleEventL2capOnConnectionRequest : public BleEvent
{
    uint16_t    mPsm;
    bool        mMtu;
    uint16_t    mPeerCid;

 public:
    BleEventL2capOnConnectionRequest(otInstance *aInstance,
                                     uint16_t aPsm,
                                     uint16_t aMtu,
                                     uint16_t aPeerCid) :
        BleEvent(kBleEventL2capOnConnectionRequest, aInstance),
        mPsm(aPsm),
        mMtu(aMtu),
        mPeerCid(aPeerCid)
    {
    }

    void Dispatch()
    {
        otPlatBleL2capOnConnectionRequest(mInstance, mPsm, mMtu, mPeerCid);
    }
};

class BleEventL2capOnConnectionResponse : public BleEvent
{
    otPlatBleL2capError mResult;
    bool                   mMtu;
    uint16_t           mPeerCid;

 public:
    BleEventL2capOnConnectionResponse(otInstance *aInstance,
                                      otPlatBleL2capError aResult,
                                      uint16_t            aMtu,
                                      uint16_t            aPeerCid) :
        BleEvent(kBleEventL2capOnConnectionResponse, aInstance),
        mResult(aResult),
        mMtu(aMtu),
        mPeerCid(aPeerCid)
    {
    }

    void Dispatch()
    {
        otPlatBleL2capOnConnectionResponse(mInstance, mResult, mMtu, mPeerCid);
    }
};

class BleEventL2capOnSduReceived : public BleEvent
{
    uint16_t          mLocalCid;
    uint16_t          mPeerCid;
    otBleRadioPacket  mPacket;

    ~BleEventL2capOnSduReceived()
    {
        FreePacket(mPacket);
    }

 public:
    BleEventL2capOnSduReceived(otInstance *      aInstance,
                               uint16_t          aLocalCid,
                               uint16_t          aPeerCid,
                               otBleRadioPacket *aPacket) :
        BleEvent(kBleEventL2capOnSduReceived, aInstance),
        mLocalCid(aLocalCid),
        mPeerCid(aPeerCid)
    {
        CopyPacket(&mPacket, aPacket);
    }

    void Dispatch()
    {
        otPlatBleL2capOnSduReceived(mInstance, mLocalCid, mPeerCid, &mPacket);
    }
};

class BleEventL2capOnSduSent : public BleEvent
{
 public:
    BleEventL2capOnSduSent(otInstance * aInstance) :
        BleEvent(kBleEventL2capOnSduSent, aInstance)
    {
    }

    void Dispatch()
    {
        otPlatBleL2capOnSduSent(mInstance);
    }
};

BleEvent *event_otPlatBleGapOnConnected(otInstance *aInstance, uint16_t aConnectionId);
BleEvent *event_otPlatBleGapOnDisconnected(otInstance *aInstance, uint16_t aConnectionId);
BleEvent *event_otPlatBleGapOnAdvReceived(otInstance *aInstance,
                                          otPlatBleDeviceAddr *aAddress,
                                          otBleRadioPacket *aPacket);
BleEvent *event_otPlatBleGapOnScanRespReceived(otInstance *         aInstance,
                                               otPlatBleDeviceAddr *aAddress,
                                               otBleRadioPacket *   aPacket);

BleEvent *event_otPlatBleGattServerOnReadRequest(otInstance *aInstance,
                                                 uint16_t aHandle,
                                                 otBleRadioPacket *aPacket);
BleEvent *event_otPlatBleGattServerOnWriteRequest(otInstance *aInstance,
                                                  uint16_t aHandle,
                                                  otBleRadioPacket *aPacket);
BleEvent *event_otPlatBleGattServerOnReadRequest(otInstance *aInstance,
                                                 uint16_t aHandle,
                                                 otBleRadioPacket *aPacket);
BleEvent *event_otPlatBleGattServerOnSubscribeRequest(otInstance *aInstance,
                                                      uint16_t aHandle,
                                                      bool aSubscribing);
BleEvent *event_otPlatBleGattServerOnIndicationConfirmation(otInstance *aInstance,
                                                            uint16_t aHandle);

BleEvent *event_otPlatBleGattClientOnReadResponse(otInstance *aInstance,
                                                  otBleRadioPacket *aPacket);
BleEvent *event_otPlatBleGattClientOnWriteResponse(otInstance *aInstance,
                                                   uint16_t aHandle);
BleEvent *event_otPlatBleGattClientOnIndication(otInstance *aInstance,
                                                uint16_t aHandle,
                                                otBleRadioPacket *aPacket);
BleEvent *event_otPlatBleGattClientOnSubscribeResponse(otInstance *aInstance,
                                                       uint16_t aHandle);
BleEvent *event_otPlatBleGattClientOnServiceDiscovered(otInstance *aInstance,
                                                       uint16_t    aStartHandle,
                                                       uint16_t    aEndHandle,
                                                       uint16_t    aServiceUuid,
                                                       otError     aError);
BleEvent *event_otPlatBleGattClientOnCharacteristicsDiscoverDone(otInstance *                 aInstance,
                                                                 otPlatBleGattCharacteristic *aChars,
                                                                 uint16_t                     aCount,
                                                                 otError                      aError);
BleEvent *event_otPlatBleGattClientOnDescriptorsDiscoverDone(otInstance *             aInstance,
                                                             otPlatBleGattDescriptor *aDescs,
                                                             uint16_t                 aCount,
                                                             otError                  aError);
BleEvent *event_otPlatBleGattClientOnMtuExchangeResponse(otInstance *aInstance,
                                                         uint16_t aMtu,
                                                         otError aError);
BleEvent *event_otPlatBleL2capOnDisconnect(otInstance *aInstance,
                                           uint16_t aLocalCid,
                                           uint16_t aPeerCid);
BleEvent *event_otPlatBleL2capOnConnectionRequest(otInstance *aInstance,
                                                  uint16_t aPsm,
                                                  uint16_t aMtu,
                                                  uint16_t aPeerCid);
BleEvent *event_otPlatBleL2capOnConnectionResponse(otInstance *        aInstance,
                                                   otPlatBleL2capError aResult,
                                                   uint16_t            aMtu,
                                                   uint16_t            aPeerCid);
BleEvent *event_otPlatBleL2capOnSduReceived(otInstance *      aInstance,
                                            uint16_t          aLocalCid,
                                            uint16_t          aPeerCid,
                                            otBleRadioPacket *aPacket);
BleEvent *event_otPlatBleL2capOnSduSent(otInstance *aInstance);

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

#define DISPATCH_OT_BLE(otPlatBleFunction) \
      platformBleSignal(event_##otPlatBleFunction);

void platformBleSignal(BleEvent *event);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // BLE_EVENT_H_
