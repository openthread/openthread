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
#include <openthread/border_router.h>
#include <openthread/thread_ftd.h>
#include <openthread/commissioner.h>
#include <openthread/joiner.h>

#include <wrl.h>
#include <collection.h>

using namespace Platform;
using namespace Platform::Collections;
using namespace Platform::Metadata;
using namespace Windows::Foundation::Collections;
using namespace Windows::Networking;

namespace ot
{

ref class otAdapter;

public delegate void otAdapterRemovalDelegate(otAdapter^ sender);

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

[Flags]
public enum class otLinkModeFlags : unsigned int
{
    None                = 0,
    RxOnWhenIdle        = 0x1,  /* 1, if the sender has its receiver on when not transmitting.  0, otherwise. */
    SecureDataRequests  = 0x2,  /* 1, if the sender will use IEEE 802.15.4 to secure all data requests.  0, otherwise. */
    DeviceType          = 0x4,  /* 1, if the sender is an FTD.  0, otherwise. */
    NetworkData         = 0x8   /* 1, if the sender requires the full Network Data.  0, otherwise. */
};

public enum class otThreadState
{
    Offline,
    Disabled,
    Detached,
    Child,
    Router,
    Leader
};

// Helper class for OpenThread Interface/Adapter specific APIs
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

#pragma region Events

    event otAdapterRemovalDelegate^                 AdapterRemoval;

    event otIpAddressAddedDelegate^                 IpAddressAdded;
    event otIpAddressRemovedDelegate^               IpAddressRemoved;
    event otIpRlocAddedDelegate^                    IpRlocAdded;
    event otIpRlocRemovedDelegate^                  IpRlocRemoved;
    event otIpLinkLocalAddresChangedDelegate^       IpLinkLocalAddresChanged;
    event otIpMeshLocalAddresChangedDelegate^       IpMeshLocalAddresChanged;

    event otNetRoleChangedDelegate^                 NetRoleChanged;
    event otNetPartitionIdChangedDelegate^          NetPartitionIdChanged;
    event otNetKeySequenceCounterChangedDelegate^   NetKeySequenceCounterChanged;

    event otThreadChildAddedDelegate^               ThreadChildAdded;
    event otThreadChildRemovedDelegate^             ThreadChildRemoved;
    event otThreadNetDataUpdatedDelegate^           ThreadNetDataUpdated;

#pragma endregion

#pragma region Properties

    property IntPtr RawHandle
    {
        IntPtr get() { return _Instance; }
    }

    property Guid InterfaceGuid
    {
        Guid get() { return otGetDeviceGuid(DeviceInstance); }
    }

    property uint32_t IfIndex
    {
        uint32_t get() { return otGetDeviceIfIndex(DeviceInstance); }
    }

    property uint32_t CompartmentId
    {
        uint32_t get() { return otGetCompartmentId(DeviceInstance); }
    }

#pragma region Link Layer

    property signed int /*int8_t*/ TransmitPower
    {
        signed int get() {
            int8_t value;
            ThrowOnFailure(otPlatRadioGetTransmitPower(DeviceInstance, &value));
            return value;
        }
        void set(signed int value)
        {
            if (value > 127) throw Exception::CreateException(E_INVALIDARG);
            ThrowOnFailure(otPlatRadioSetTransmitPower(DeviceInstance, (int8_t)value);
        }
    }

    property uint32_t PollPeriod
    {
        uint32_t get() { return otLinkGetPollPeriod(DeviceInstance); }
        void set(uint32_t value) { ThrowOnFailure(otLinkSetPollPeriod(DeviceInstance, value)); }
    }

    property uint8_t Channel
    { 
        uint8_t get() { return otLinkGetChannel(DeviceInstance); }
        void set(uint8_t value) { ThrowOnFailure(otLinkSetChannel(DeviceInstance, value)); }
    }

    property uint16_t PanId
    {
        uint16_t get() { return otLinkGetPanId(DeviceInstance); }
        void set(uint16_t value) { ThrowOnFailure(otLinkSetPanId(DeviceInstance, value)); }
    }

    property uint16_t ShortAddress
    {
        uint16_t get() { return otLinkGetShortAddress(DeviceInstance); }
    }

    property uint64_t ExtendedAddress
    {
        uint64_t get()
        {
            auto addr = otLinkGetExtendedAddress(DeviceInstance);
            auto ret = *(uint64_t*)addr;
            otFreeMemory(addr);
            return ret;
        }
        void set(uint64_t value) 
        {
            ThrowOnFailure(otLinkSetExtendedAddress(DeviceInstance, (otExtAddress*)&value));
        }
    }

    property uint64_t FactoryAssignedIeeeEui64
    {
        uint64_t get()
        {
            uint64_t addr;
            otLinkGetFactoryAssignedIeeeEui64(DeviceInstance, (otExtAddress*)&addr);
            return addr;
        }
    }

    property uint64_t JoinerId
    {
        uint64_t get()
        {
            uint64_t addr;
            otJoinerGetId(DeviceInstance, (otExtAddress*)&addr);
            return addr;
        }
    }

#pragma endregion

#pragma region IP Layer

    property bool IpEnabled
    {
        bool get() { return otIp6IsEnabled(DeviceInstance); }
        void set(bool value) { ThrowOnFailure(otIp6SetEnabled(DeviceInstance, value)); }
    }

#pragma endregion

#pragma region Thread Layer

    property uint64_t ExtendedPanId
    {
        uint64_t get()
        {
            auto panid = otThreadGetExtendedPanId(DeviceInstance);
            auto ret = *(uint64_t*)panid;
            otFreeMemory(panid);
            return ret;
        }
        void set(uint64_t value)
        {
            otThreadSetExtendedPanId(DeviceInstance, (uint8_t*)&value);
        }
    }

    property otLinkModeFlags LinkMode
    {
        otLinkModeFlags get()
        {
            auto linkmode = otThreadGetLinkMode(DeviceInstance);
            otLinkModeFlags flags = otLinkModeFlags::None;
            if (linkmode.mRxOnWhenIdle)         flags = flags | otLinkModeFlags::RxOnWhenIdle;
            if (linkmode.mSecureDataRequests)   flags = flags | otLinkModeFlags::SecureDataRequests;
            if (linkmode.mDeviceType)           flags = flags | otLinkModeFlags::DeviceType;
            if (linkmode.mNetworkData)          flags = flags | otLinkModeFlags::NetworkData;
            return flags;
        }
        void set(otLinkModeFlags value)
        {
            otLinkModeConfig linkmode = { 0 };
            if ((value & otLinkModeFlags::RxOnWhenIdle) != otLinkModeFlags::None)
                linkmode.mRxOnWhenIdle = true;
            if ((value & otLinkModeFlags::SecureDataRequests) != otLinkModeFlags::None)
                linkmode.mSecureDataRequests = true;
            if ((value & otLinkModeFlags::DeviceType) != otLinkModeFlags::None)
                linkmode.mDeviceType = true;
            if ((value & otLinkModeFlags::NetworkData) != otLinkModeFlags::None)
                linkmode.mNetworkData = true;
            ThrowOnFailure(otThreadSetLinkMode(DeviceInstance, linkmode));
        }
    }

    static uint32_t charToValue(wchar_t c)
    {
        if (c >= L'a' && c <= L'f')
        {
            return c - L'a';
        }
        else if (c >= L'A' && c <= L'F')
        {
            return c - L'A';
        }
        else if (c >= L'0' && c <= L'9')
        {
            return c - L'0';
        }
        else
        {
            throw Exception::CreateException(E_INVALIDARG);
        }
    }

    property String^ MasterKey
    {
        String^ get()
        {
            constexpr char hexmap[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
            auto key = otThreadGetMasterKey(DeviceInstance);
            WCHAR szKey[OT_MASTER_KEY_SIZE * 2 + 1] = { 0 };
            for (uint8_t i = 0; i < OT_MASTER_KEY_SIZE; i++)
            {
                szKey[2 * i]     = hexmap[(key->m8[i] & 0xF0) >> 4];
                szKey[2 * i + 1] = hexmap[key->m8[i] & 0x0F];
            }
            otFreeMemory(key);
            return ref new String(szKey);
        }
        void set(String^ value)
        {
            otMasterKey key;
            uint8_t keyLen = 0;
            for (uint32_t i = 0; i < value->Length() - 1; i+=2)
            {
                key.m8[keyLen++] = (uint8_t)((charToValue(value->Data()[i]) << 4) |
                    charToValue(value->Data()[i + 1]));
            }
            if (keyLen * 2 == value->Length() - 1)
            {
                key.m8[keyLen++] = (uint8_t)(charToValue(value->Data()[value->Length()-1])) << 4;
            }
            memset(key.m8 + keyLen, 0, sizeof(key) - keyLen);
            ThrowOnFailure(otThreadSetMasterKey(DeviceInstance, &key));
        }
    }

    property String^ NetworkName
    {
        String^ get()
        {
            auto _name = otThreadGetNetworkName(DeviceInstance);
            WCHAR name[OT_NETWORK_NAME_MAX_SIZE + 1];
            MultiByteToWideChar(CP_UTF8, 0, _name, -1, name, ARRAYSIZE(name));
            otFreeMemory(_name);
            return ref new String(name);
        }
        void set(String^ value)
        {
            char name[OT_NETWORK_NAME_MAX_SIZE + 1];
            auto len = WideCharToMultiByte(CP_UTF8, 0, value->Data(), -1, name, ARRAYSIZE(name), nullptr, nullptr);
            if (len <= 0) throw Exception::CreateException(E_INVALIDARG);
            ThrowOnFailure(otThreadSetNetworkName(DeviceInstance, name));
        }
    }

    property uint8_t MaxAllowedChildren
    {
        uint8_t get() { return otThreadGetMaxAllowedChildren(DeviceInstance); }
        void set(uint8_t value) { ThrowOnFailure(otThreadSetMaxAllowedChildren(DeviceInstance, value)); }
    }

    property uint32_t ChildTimeout
    {
        uint32_t get() { return otThreadGetChildTimeout(DeviceInstance); }
        void set(uint32_t value) { otThreadSetChildTimeout(DeviceInstance, value); }
    }

    property bool ThreadEnabled
    {
        //bool get() { return otIsThreadStarted(DeviceInstance); }
        void set(bool value) { ThrowOnFailure(otThreadSetEnabled(DeviceInstance, value)); }
    }

    property bool AutoStart
    {
        bool get() { return otThreadGetAutoStart(DeviceInstance); }
        void set(bool value) { ThrowOnFailure(otThreadSetAutoStart(DeviceInstance, value)); }
    }

    property bool Singleton
    {
        bool get() { return otThreadIsSingleton(DeviceInstance); }
    }

    property bool RouterRoleEnabled
    {
        bool get() { return otThreadIsRouterRoleEnabled(DeviceInstance); }
        void set(bool value) { otThreadSetRouterRoleEnabled(DeviceInstance, value); }
    }

    property uint8_t PreferredRouterId
    {
        void set(uint8_t value) { ThrowOnFailure(otThreadSetPreferredRouterId(DeviceInstance, value)); }
    }

    property HostName^ MeshLocalEid
    {
        HostName^ get()
        {
            auto addr = otThreadGetMeshLocalEid(DeviceInstance);
            WCHAR szAddr[46];
            RtlIpv6AddressToString((IN6_ADDR*)addr, szAddr);
            otFreeMemory(addr);
            return ref new HostName(ref new String(szAddr));
        }
    }

    property HostName^ LeaderRloc
    {
        HostName^ get()
        {
            IN6_ADDR addr;
            ThrowOnFailure(otThreadGetLeaderRloc(DeviceInstance, (otIp6Address*)&addr));
            WCHAR szAddr[46];
            RtlIpv6AddressToString(&addr, szAddr);
            return ref new HostName(ref new String(szAddr));
        }
    }

    property uint8_t LocalLeaderWeight
    {
        uint8_t get() { return otThreadGetLocalLeaderWeight(DeviceInstance); }
        void set(uint8_t value) { otThreadSetLocalLeaderWeight(DeviceInstance, value); }
    }

    property uint32_t LocalLeaderPartitionId
    {
        uint32_t get() { return otThreadGetLocalLeaderPartitionId(DeviceInstance); }
        void set(uint32_t value) { otThreadSetLocalLeaderPartitionId(DeviceInstance, value); }
    }

    property uint8_t LeaderWeight
    {
        uint8_t get() { return otThreadGetLeaderWeight(DeviceInstance); }
    }

    property uint32_t LeaderRouterId
    {
        uint32_t get() { return otThreadGetLeaderRouterId(DeviceInstance); }
    }

    property uint32_t PartitionId
    {
        uint32_t get() { return otThreadGetPartitionId(DeviceInstance); }
    }

    property uint16_t Rloc16
    {
        uint16_t get() { return otThreadGetRloc16(DeviceInstance); }
    }

    property otThreadState State
    {
        otThreadState get()
        {
            return (otThreadState)otThreadGetDeviceRole(DeviceInstance);
        }
    }

#pragma endregion

#pragma endregion

#pragma region Constructor/Destructor

    otAdapter(_In_ IntPtr /*otInstance**/ aInstance)
    {
        _Instance = (void*)aInstance;

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

#pragma endregion

#pragma region Functions

    void PlatformReset()
    {
        otInstanceReset(DeviceInstance);
    }

    void FactoryReset()
    {
        otInstanceFactoryReset(DeviceInstance);
    }

    void BecomeRouter()
    {
        ThrowOnFailure(otThreadBecomeRouter(DeviceInstance));
    }

    void BecomeLeader()
    {
        ThrowOnFailure(otThreadBecomeLeader(DeviceInstance));
    }

#pragma endregion

internal:

    void InvokeAdapterRemoval()
    {
        AdapterRemoval(this);
    }

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

        if (aFlags & OT_CHANGED_IP6_ADDRESS_ADDED)
        {
            pThis->IpAddressAdded(pThis);
        }

        if (aFlags & OT_CHANGED_IP6_ADDRESS_REMOVED)
        {
            pThis->IpAddressRemoved(pThis);
        }

        if (aFlags & OT_CHANGED_THREAD_RLOC_ADDED)
        {
            pThis->IpRlocAdded(pThis);
        }

        if (aFlags & OT_CHANGED_THREAD_RLOC_REMOVED)
        {
            pThis->IpRlocRemoved(pThis);
        }

        if (aFlags & OT_CHANGED_THREAD_LL_ADDR)
        {
            pThis->IpLinkLocalAddresChanged(pThis);
        }

        if (aFlags & OT_CHANGED_THREAD_ML_ADDR)
        {
            pThis->IpMeshLocalAddresChanged(pThis);
        }

        if (aFlags & OT_CHANGED_THREAD_ROLE)
        {
            pThis->NetRoleChanged(pThis);
        }

        if (aFlags & OT_CHANGED_THREAD_PARTITION_ID)
        {
            pThis->NetPartitionIdChanged(pThis);
        }

        if (aFlags & OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER)
        {
            pThis->NetKeySequenceCounterChanged(pThis);
        }

        if (aFlags & OT_CHANGED_THREAD_CHILD_ADDED)
        {
            pThis->ThreadChildAdded(pThis);
        }

        if (aFlags & OT_CHANGED_THREAD_CHILD_REMOVED)
        {
            pThis->ThreadChildRemoved(pThis);
        }

        if (aFlags & OT_CHANGED_THREAD_NETDATA)
        {
            pThis->ThreadNetDataUpdated(pThis);
        }
    }

    static HRESULT
    TheadErrorToHResult(
        int /* otError */ error
    )
    {
        switch (error)
        {
        case OT_ERROR_NO_BUFS:           return E_OUTOFMEMORY;
        case OT_ERROR_DROP:
        case OT_ERROR_NO_ROUTE:          return HRESULT_FROM_WIN32(ERROR_NETWORK_UNREACHABLE);
        case OT_ERROR_INVALID_ARGS:      return E_INVALIDARG;
        case OT_ERROR_SECURITY:          return E_ACCESSDENIED;
        case OT_ERROR_NOT_CAPABLE:
        case OT_ERROR_NOT_IMPLEMENTED:   return E_NOTIMPL;
        case OT_ERROR_INVALID_STATE:     return E_NOT_VALID_STATE;
        case OT_ERROR_NOT_FOUND:         return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        case OT_ERROR_ALREADY:           return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        case OT_ERROR_RESPONSE_TIMEOUT:  return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        default:                         return E_FAIL;
        }
    }
};

} // namespace ot

