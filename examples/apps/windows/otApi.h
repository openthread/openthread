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

#include "otAdapter.h"
#include <collection.h>

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;

#define MAC8_FORMAT L"%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X"
#define MAC8_ARG(mac) mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7]

namespace ot
{

public delegate void otAdapterArrivalDelegate(otAdapter^ aAdapter);

// Helper class for OpenThread API
public ref class otApi sealed
{
private:

    void *_apiInstance;
    #define ApiInstance ((otApiInstance*)_apiInstance)

    CRITICAL_SECTION _cs;
    Vector<otAdapter^>^ _adapters;

public:

    // Event for device availability changes
    event otAdapterArrivalDelegate^ AdapterArrival;

    property IntPtr RawHandle
    {
        IntPtr get() { return _apiInstance; }
    }

    // Constructor
    otApi() :
        _adapters(ref new Vector<otAdapter^>())
    {
        // Initialize the API handle
        _apiInstance = otApiInit();
        if (_apiInstance == nullptr)
        {
            throw Exception::CreateException(E_UNEXPECTED, L"otApiInit failed.");
        }

        InitializeCriticalSection(&_cs);

        IInspectable* pInspectable = reinterpret_cast<IInspectable*>(this);

        // Register for device availability callbacks
        otSetDeviceAvailabilityChangedCallback(ApiInstance, ThreadDeviceAvailabilityCallback, pInspectable);

        // Query list of devices
        auto deviceList = otEnumerateDevices(ApiInstance);
        if (deviceList)
        {
            EnterCriticalSection(&_cs);

            // Add each adapter to our cache unless it already was inserted from a notification
            for (DWORD i = 0; i < deviceList->aDevicesLength; i++)
            {
                if (GetAdapter(deviceList->aDevices[i]) == nullptr)
                {
                    auto deviceInstance = otInstanceInit(ApiInstance, &deviceList->aDevices[i]);
                    if (deviceInstance)
                    {
                        _adapters->Append(ref new otAdapter(deviceInstance));
                    }
                }
            }

            LeaveCriticalSection(&_cs);

            otFreeMemory(deviceList);
        }
    }

    // Destructor
    virtual ~otApi()
    {
        // Clear registration for callbacks
        otSetDeviceAvailabilityChangedCallback(ApiInstance, nullptr, nullptr);

        DeleteCriticalSection(&_cs);

        // Clean up api
        otApiFinalize(ApiInstance);
        _apiInstance = nullptr;
    }

    // Returns the entire list of adapters
    IVectorView<otAdapter^>^ GetAdapters()
    {
        IVectorView<otAdapter^>^ adapters;
        EnterCriticalSection(&_cs);
        adapters = _adapters->GetView(); // TODO - Need to copy
        LeaveCriticalSection(&_cs);
        return adapters;
    }

    // Helper to get an adapter, given its device guid
    otAdapter^ GetAdapter(Guid aDeviceGuid)
    {
        otAdapter^ ret = nullptr;

        EnterCriticalSection(&_cs);

        for (auto&& adapter : _adapters)
        {
            if (adapter->InterfaceGuid == aDeviceGuid)
            {
                ret = adapter;
                break;
            }
        }

        LeaveCriticalSection(&_cs);

        return ret;
    }

    // Helper function to convert mac address to string
    static String^ MacToString(uint64_t mac)
    {
        WCHAR szMac[64] = { 0 };
        swprintf_s(szMac, 64, MAC8_FORMAT, MAC8_ARG(((UCHAR*)&mac)));
        return ref new String(szMac);
    }

    // Helper function to convert RLOC16/PANID to string
    static String^ Rloc16ToString(uint16_t rloc)
    {
        WCHAR szRloc[16] = { 0 };
        swprintf_s(szRloc, 16, L"0x%X", rloc);
        return ref new String(szRloc);
    }

    // Helper function to convert state to string
    static String^ ThreadStateToString(otThreadState state)
    {
        switch (state)
        {
        default:
        case otThreadState::Offline:    return L"Offline";
        case otThreadState::Disabled:   return L"Disabled";
        case otThreadState::Detached:   return L"Disconnected";
        case otThreadState::Child:      return L"Connected - Child";
        case otThreadState::Router:     return L"Connected - Router";
        case otThreadState::Leader:     return L"Connected - Leader";
        }
    }

private:

    // Callback from OpenThread indicating arrival or removal of interfaces
    static void OTCALL
    ThreadDeviceAvailabilityCallback(
        bool        aAdded,
        const GUID* aDeviceGuid,
        _In_ void*  aContext
    )
    {
        IInspectable* pInspectable = (IInspectable*)aContext;
        otApi^ pThis = reinterpret_cast<otApi^>(pInspectable);

        if (aAdded)
        {
            otAdapter^ adapter = nullptr;

            // Add the device to the list, if it isn't already there
            EnterCriticalSection(&pThis->_cs);

            if (pThis->GetAdapter(*aDeviceGuid) == nullptr)
            {
                auto deviceInstance = otInstanceInit((otApiInstance*)pThis->_apiInstance, aDeviceGuid);
                if (deviceInstance)
                {
                    pThis->_adapters->Append(adapter = ref new otAdapter(deviceInstance));
                }
            }

            LeaveCriticalSection(&pThis->_cs);

            if (adapter)
            {
                // Send a notification of arrival
                pThis->AdapterArrival(adapter);
            }
        }
        else
        {
            otAdapter^ ret = nullptr;
            Guid guid = *aDeviceGuid;

            EnterCriticalSection(&pThis->_cs);

            // Look up in the cached list of adapters to remove it
            uint32_t i = 0;
            for (auto&& adapter : pThis->_adapters)
            {
                if (adapter->InterfaceGuid == guid)
                {
                    ret = adapter;
                    pThis->_adapters->RemoveAt(i);
                    break;
                }
                i++;
            }

            LeaveCriticalSection(&pThis->_cs);

            if (ret)
            {
                ret->InvokeAdapterRemoval();
            }
        }
    }
};

} // namespace ot
