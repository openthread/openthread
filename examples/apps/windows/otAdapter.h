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
using namespace Platform::Metadata;
using namespace Windows::Foundation::Collections;
using namespace Windows::Networking;

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

[Flags]
public enum class otLinkModeFlags : unsigned int
{
    None                = 0,
    RxOnWhenIdle        = 0x1,  /* 1, if the sender has its receiver on when not transmitting.  0, otherwise. */
    SecureDataRequests  = 0x2,  /* 1, if the sender will use IEEE 802.15.4 to secure all data requests.  0, otherwise. */
    DeviceType          = 0x4,  /* 1, if the sender is an FFD.  0, otherwise. */
    NetworkData         = 0x8   /* 1, if the sender requires the full Network Data.  0, otherwise. */
};

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

    //
    // Properties
    //

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

    property signed int /*int8_t*/ MaxTransmitPower
    {
        signed int get() { return otGetMaxTransmitPower(DeviceInstance); }
        void set(signed int value)
        {
            if (value > 127) throw Exception::CreateException(E_INVALIDARG);
            otSetMaxTransmitPower(DeviceInstance, (int8_t)value);
        }
    }

    property uint32_t PollPeriod
    {
        uint32_t get() { return otGetPollPeriod(DeviceInstance); }
        void set(uint32_t value) { otSetPollPeriod(DeviceInstance, value); }
    }

    property uint8_t Channel 
    { 
        uint8_t get() { return otGetChannel(DeviceInstance); }
        void set(uint8_t value) { ThrowOnFailure(otSetChannel(DeviceInstance, value)); }
    }

    property uint16_t PanId
    {
        uint16_t get() { return otGetPanId(DeviceInstance); }
        void set(uint16_t value) { ThrowOnFailure(otSetPanId(DeviceInstance, value)); }
    }

    property uint16_t ShortAddress
    {
        uint16_t get() { return otGetShortAddress(DeviceInstance); }
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

    property otLinkModeFlags LinkMode
    {
        otLinkModeFlags get()
        {
            auto linkmode = otGetLinkMode(DeviceInstance);
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
            ThrowOnFailure(otSetLinkMode(DeviceInstance, linkmode));
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
            uint8_t keyLen;
            auto key = otGetMasterKey(DeviceInstance, &keyLen);
            WCHAR szKey[OT_MASTER_KEY_SIZE * 2 + 1] = { 0 };
            for (uint8_t i = 0; i < keyLen; i++)
            {
                szKey[2 * i]     = hexmap[(key[i] & 0xF0) >> 4];
                szKey[2 * i + 1] = hexmap[key[i] & 0x0F];
            }
            otFreeMemory(key);
            return ref new String(szKey);
        }
        void set(String^ value)
        {
            uint8_t key[OT_MASTER_KEY_SIZE];
            uint8_t keyLen = 0;
            if (value->Length() % 2 == 0)
            {
                for (uint32_t i = 0; i < value->Length(); i+=2)
                {
                    key[keyLen++] = (uint8_t)((charToValue(value->Data()[i]) << 4) |
                        charToValue(value->Data()[i + 1]));
                }
            }
            else
            {
                key[keyLen++] = (uint8_t)(charToValue(value->Data()[0]));
                for (uint32_t i = 1; i < value->Length(); i += 2)
                {
                    key[keyLen++] = (uint8_t)((charToValue(value->Data()[i]) << 4) |
                        charToValue(value->Data()[i + 1]));
                }
            }
            ThrowOnFailure(otSetMasterKey(DeviceInstance, key, keyLen));
        }
    }

    property String^ NetworkName
    {
        String^ get()
        {
            auto _name = otGetNetworkName(DeviceInstance);
            WCHAR name[OT_NETWORK_NAME_MAX_SIZE + 1];
            MultiByteToWideChar(CP_UTF8, 0, _name, -1, name, ARRAYSIZE(name));
            otFreeMemory(_name);
            return ref new String(name);
        }
        void set(String^ value)
        {
            char name[OT_NETWORK_NAME_MAX_SIZE + 1];
            WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, value->Data(), -1, name, ARRAYSIZE(name), nullptr, nullptr);
            ThrowOnFailure(otSetNetworkName(DeviceInstance, name));
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

    property bool Singleton
    {
        bool get() { return otIsSingleton(DeviceInstance); }
    }

    property bool RouterRoleEnabled
    {
        bool get() { return otIsRouterRoleEnabled(DeviceInstance); }
        void set(bool value) { otSetRouterRoleEnabled(DeviceInstance, value); }
    }

    /*property uint8_t PreferredRouterId
    {
        void set(uint8_t value) { ThrowOnFailure(otSetPreferredRouterId(DeviceInstance, value)); }
    }*/

    property HostName^ MeshLocalEid
    {
        HostName^ get()
        {
            auto addr = otGetMeshLocalEid(DeviceInstance);
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
            ThrowOnFailure(otGetLeaderRloc(DeviceInstance, (otIp6Address*)&addr));
            WCHAR szAddr[46];
            RtlIpv6AddressToString(&addr, szAddr);
            return ref new HostName(ref new String(szAddr));
        }
    }

    property uint8_t LocalLeaderWeight
    {
        uint8_t get() { return otGetLocalLeaderWeight(DeviceInstance); }
        void set(uint8_t value) { otSetLocalLeaderWeight(DeviceInstance, value); }
    }

    property uint32_t LocalLeaderPartitionId
    {
        uint32_t get() { return otGetLocalLeaderPartitionId(DeviceInstance); }
        void set(uint32_t value) { otSetLocalLeaderPartitionId(DeviceInstance, value); }
    }

    property uint8_t LeaderWeight
    {
        uint8_t get() { return otGetLeaderWeight(DeviceInstance); }
    }

    property uint32_t LeaderRouterId
    {
        uint32_t get() { return otGetLeaderRouterId(DeviceInstance); }
    }

    property uint32_t PartitionId
    {
        uint32_t get() { return otGetPartitionId(DeviceInstance); }
    }

    property uint16_t Rloc16
    {
        uint16_t get() { return otGetRloc16(DeviceInstance); }
    }

    property String^ State
    {
        String^ get()
        {
            switch (otGetDeviceRole(DeviceInstance))
            {
            case kDeviceRoleOffline:    return L"Offline";
            case kDeviceRoleDisabled:   return L"Disabled";
            case kDeviceRoleDetached:   return L"Disconnected";
            case kDeviceRoleChild:      return L"Connected - Child";
            case kDeviceRoleRouter:     return L"Connected - Router";
            case kDeviceRoleLeader:     return L"Connected - Leader";
            }

            throw Exception::CreateException(E_INVALIDARG);
        }
    }

    //
    // Constructor/Destructor
    //

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

    //
    // Functions
    //

    void PlatformReset()
    {
        otPlatformReset(DeviceInstance);
    }

    void FactoryReset()
    {
        otFactoryReset(DeviceInstance);
    }

    void BecomeRouter()
    {
        ThrowOnFailure(otBecomeRouter(DeviceInstance));
    }

    void BecomeLeader()
    {
        ThrowOnFailure(otBecomeLeader(DeviceInstance));
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

