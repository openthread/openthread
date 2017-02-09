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

#pragma once

#define OTDLL 1
#include <openthread.h>
#include <commissioning/commissioner.h>
#include <commissioning/joiner.h>

#include <wrl.h>
#include <collection.h>

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;

namespace Thread
{

ref class otAdapter;

public delegate void otIpAddressAddedDelegate(otAdapter^ sender);
public delegate void otIpAddressRemovedDelegate(otAdapter^ sender);
public delegate void otIpRlocAddedDelegate(otAdapter^ sender);
public delegate void otIpRlocRemovedDelegate(otAdapter^ sender);
public delegate void otIpLinkLocalAddresChangedDelegate(otAdapter^ sender);
public delegate void otIpMeshLocalAddresChangedDelegate(otAdapter^ sender);

public delegate void otNetRoleChangedDelegate(otAdapter^ sender);
public delegate void otNetPartitionIdChangedDelegate(otAdapter^ sender);
public delegate void otNetKeySequenceCounterChangedDelegate(otAdapter^ sender);

public delegate void otThreadChildAddedDelegate(otAdapter^ sender);
public delegate void otThreadChildRemovedDelegate(otAdapter^ sender);
public delegate void otThreadNetDataUpdatedDelegate(otAdapter^ sender);

public ref class otAdapter sealed
{
private:

    void *_Instance;
    #define DeviceInstance ((otInstance*)_Instance)

    #define ThrowOnFailure(exp) \
    do { \
        auto res = exp; \
        if (res != 0) \
            throw Exception::CreateException(TheadErrorToHResult(res), #exp); \
    } while (false)

public:

    //
    // Events
    //

    event otIpAddressAddedDelegate^ IpAddressAdded;
    event otIpAddressRemovedDelegate^ IpAddressRemoved;
    event otIpRlocAddedDelegate^ IpRlocAdded;
    event otIpRlocRemovedDelegate^ IpRlocRemoved;
    event otIpLinkLocalAddresChangedDelegate^ IpLinkLocalAddresChanged;
    event otIpMeshLocalAddresChangedDelegate^ IpMeshLocalAddresChanged;

    event otNetRoleChangedDelegate^ NetRoleChanged;
    event otNetPartitionIdChangedDelegate^ NetPartitionIdChanged;
    event otNetKeySequenceCounterChangedDelegate^ NetKeySequenceCounterChanged;

    event otThreadChildAddedDelegate^ ThreadChildAdded;
    event otThreadChildRemovedDelegate^ ThreadChildRemoved;
    event otThreadNetDataUpdatedDelegate^ ThreadNetDataUpdated;

    //
    // Properties
    //

    property Guid InterfaceGuid { Guid get() { return otGetDeviceGuid(DeviceInstance); } }

    property uint32_t IfIndex { uint32_t get() { return otGetDeviceIfIndex(DeviceInstance); } }

    property uint32_t CompartmentId { uint32_t get() { return otGetCompartmentId(DeviceInstance); } }

    property uint8_t Channel 
    { 
        uint8_t get() { return otGetChannel(DeviceInstance); }
        void set(uint8_t value) { ThrowOnFailure(otSetChannel(DeviceInstance, value)); }
    }

    property signed int /*int8_t*/ MaxTransmitPower
    {
        signed int get() { return otGetMaxTransmitPower(DeviceInstance); }
        void set(signed int value) { otSetMaxTransmitPower(DeviceInstance, (int8_t)value); }
    }

    property uint64_t ExtendedAddress
    {
        uint64_t get()
        { 
            auto addr = otGetExtendedAddress(DeviceInstance);
            auto ret = *(uint64_t*)addr;
            otFreeMemory(addr);
            return ret;
        }
        void set(uint64_t value) 
        {
            ThrowOnFailure(otSetExtendedAddress(DeviceInstance, (otExtAddress*)&value));
        }
    }

    property uint64_t FactoryAssignedIeeeEui64
    {
        uint64_t get()
        {
            uint64_t addr;
            otGetFactoryAssignedIeeeEui64(DeviceInstance, (otExtAddress*)&addr);
            return addr;
        }
    }

    property uint64_t HashMacAddress
    {
        uint64_t get()
        {
            uint64_t addr;
            otGetHashMacAddress(DeviceInstance, (otExtAddress*)&addr);
            return addr;
        }
    }

    property uint64_t ExtendedPanId
    {
        uint64_t get()
        {
            auto panid = otGetExtendedPanId(DeviceInstance);
            auto ret = *(uint64_t*)panid;
            otFreeMemory(panid);
            return ret;
        }
        void set(uint64_t value)
        {
            otSetExtendedPanId(DeviceInstance, (uint8_t*)&value);
        }
    }

    property otLinkModeConfig LinkMode
    {
        otLinkModeConfig get()
        {
            return otGetLinkMode(DeviceInstance);
        }
        void set(otLinkModeConfig value)
        {
            ThrowOnFailure(otSetLinkMode(DeviceInstance, value));
        }
    }

    property Vector<uint8_t>^ MasterKey
    {
        Vector<uint8_t>^ get()
        {
            uint8_t keyLen;
            auto key = otGetMasterKey(DeviceInstance, &keyLen);
            auto ret = ref new Vector<uint8_t>(key, keyLen);
            otFreeMemory(key);
            return ret;
        }
        void set(Vector<uint8_t>^ value)
        {
            WriteOnlyArray<uint8_t>^ data;
            auto dataLen = value->GetView()->GetMany(0, data);
            ThrowOnFailure(otSetMasterKey(DeviceInstance, data->Data, (uint8_t)dataLen));
        }
    }

    property uint8_t MaxAllowedChildren
    {
        uint8_t get() { return otGetMaxAllowedChildren(DeviceInstance); }
        void set(uint8_t value) { ThrowOnFailure(otSetMaxAllowedChildren(DeviceInstance, value)); }
    }

    property uint32_t ChildTimeout
    {
        uint32_t get() { return otGetChildTimeout(DeviceInstance); }
        void set(uint32_t value) { otSetChildTimeout(DeviceInstance, value); }
    }

    property bool InterfaceUp 
    { 
        bool get() { return otIsInterfaceUp(DeviceInstance); }
        void set(bool value)
        {
            if (value) ThrowOnFailure(otInterfaceUp(DeviceInstance));
            else       ThrowOnFailure(otInterfaceDown(DeviceInstance));
        }
    }

    property bool ThreadStarted
    {
        //bool get() { return otIsThreadStarted(DeviceInstance); }
        void set(bool value)
        {
            if (value) ThrowOnFailure(otThreadStart(DeviceInstance));
            else       ThrowOnFailure(otThreadStop(DeviceInstance));
        }
    }

    property bool Singleton { bool get() { return otIsSingleton(DeviceInstance); } }

    property otIp6Address LeaderRloc
    {
        otIp6Address get()
        {
            otIp6Address addr;
            otGetLeaderRloc(DeviceInstance, &addr);
            return addr;
        }
    }

    //
    // Constructor/Destructor
    //

    otAdapter(_In_ void /*otInstance*/* aInstance)
    {
        _Instance = aInstance;

        IInspectable* pInspectable = reinterpret_cast<IInspectable*>(this);

        // Register for device availability callbacks
        otSetStateChangedCallback(DeviceInstance, ThreadStateChangeCallback, pInspectable);
    }

    virtual ~otAdapter()
    {
        // Unregister for callbacks for the device
        otSetStateChangedCallback(DeviceInstance, nullptr, nullptr);

        // Free the device
        otFreeMemory(DeviceInstance);
    }

    //
    // Functions
    //

private:

    friend ref class otApi;

    static void OTCALL
    ThreadStateChangeCallback(
        uint32_t aFlags,
        _In_ void* aContext
        )
    {
        IInspectable* pInspectable = (IInspectable*)aContext;
        otAdapter^ pThis = reinterpret_cast<otAdapter^>(pInspectable);

        if (aFlags & OT_IP6_ADDRESS_ADDED)
        {
            pThis->IpAddressAdded(pThis);
        }

        if (aFlags & OT_IP6_ADDRESS_REMOVED)
        {
            pThis->IpAddressRemoved(pThis);
        }

        if (aFlags & OT_IP6_RLOC_ADDED)
        {
            pThis->IpRlocAdded(pThis);
        }

        if (aFlags & OT_IP6_RLOC_REMOVED)
        {
            pThis->IpRlocRemoved(pThis);
        }

        if (aFlags & OT_IP6_LL_ADDR_CHANGED)
        {
            pThis->IpLinkLocalAddresChanged(pThis);
        }

        if (aFlags & OT_IP6_ML_ADDR_CHANGED)
        {
            pThis->IpMeshLocalAddresChanged(pThis);
        }

        if (aFlags & OT_NET_ROLE)
        {
            pThis->NetRoleChanged(pThis);
        }

        if (aFlags & OT_NET_PARTITION_ID)
        {
            pThis->NetPartitionIdChanged(pThis);
        }

        if (aFlags & OT_NET_KEY_SEQUENCE_COUNTER)
        {
            pThis->NetKeySequenceCounterChanged(pThis);
        }

        if (aFlags & OT_THREAD_CHILD_ADDED)
        {
            pThis->ThreadChildAdded(pThis);
        }

        if (aFlags & OT_THREAD_CHILD_REMOVED)
        {
            pThis->ThreadChildRemoved(pThis);
        }

        if (aFlags & OT_THREAD_NETDATA_UPDATED)
        {
            pThis->ThreadNetDataUpdated(pThis);
        }
    }

    static HRESULT
    TheadErrorToHResult(
        int /* ThreadError */ error
    )
    {
        switch (error)
        {
        case kThreadError_NoBufs:           return E_OUTOFMEMORY;
        case kThreadError_Drop:
        case kThreadError_NoRoute:          return HRESULT_FROM_WIN32(ERROR_NETWORK_UNREACHABLE);
        case kThreadError_InvalidArgs:      return E_INVALIDARG;
        case kThreadError_Security:         return E_ACCESSDENIED;
        case kThreadError_NotCapable:
        case kThreadError_NotImplemented:   return E_NOTIMPL;
        case kThreadError_InvalidState:     return E_NOT_VALID_STATE;
        case kThreadError_NotFound:         return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        case kThreadError_Already:          return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        case kThreadError_ResponseTimeout:  return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        default:                            return E_FAIL;
        }
    }
};

} // namespace Thread

