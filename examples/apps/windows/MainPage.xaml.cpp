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

#include "pch.h"
#include "MainPage.xaml.h"

using namespace Thread;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

MainPage^ MainPage::Current = nullptr;

#define GUID_FORMAT L"{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}"
#define GUID_ARG(guid) guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]

#define MAC8_FORMAT L"%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X"
#define MAC8_ARG(mac) mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7]

PCWSTR ToString(otDeviceRole role)
{
    switch (role)
    {
    case kDeviceRoleOffline:    return L"Offline";
    case kDeviceRoleDisabled:   return L"Disabled";
    case kDeviceRoleDetached:   return L"Disconnected";
    case kDeviceRoleChild:      return L"Connected - Child";
    case kDeviceRoleRouter:     return L"Connected - Router";
    case kDeviceRoleLeader:     return L"Connected - Leader";
    }

    return L"Unknown Role State";
}

void OTCALL
ThreadDeviceAvailabilityCallback(
    bool        /* aAdded */, 
    const GUID* /* aDeviceGuid */, 
    _In_ void*  /* aContext */
    )
{
    // Trigger the interface list to update
    MainPage::Current->Dispatcher->RunAsync(
        Windows::UI::Core::CoreDispatcherPriority::Normal,
        ref new Windows::UI::Core::DispatchedHandler(
            [=]() {
                MainPage::Current->BuildInterfaceList();
            }
        )
    );
}

void OTCALL
ThreadStateChangeCallback(
    uint32_t aFlags, 
    _In_ void* /* aContext */
    )
{
    if ((aFlags & OT_NET_ROLE) != 0)
    {
        // Trigger the interface list to update
        MainPage::Current->Dispatcher->RunAsync(
            Windows::UI::Core::CoreDispatcherPriority::Normal,
            ref new Windows::UI::Core::DispatchedHandler(
                [=]() {
                    MainPage::Current->BuildInterfaceList();
                }
            )
        );
    }
}

MainPage::MainPage()
{
    InitializeComponent();

    MainPage::Current = this;
    _isFullScreen = false;
    
    _apiInstance = nullptr;
    InterfaceConfigCancelButton->Click +=
        ref new RoutedEventHandler(
            [=](Platform::Object^, RoutedEventArgs^) {
                InterfaceConfiguration->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            }
    );
    InterfaceConfigOkButton->Click +=
        ref new RoutedEventHandler(
            [=](Platform::Object^, RoutedEventArgs^) {
                InterfaceConfiguration->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                ConnectNetwork(_currentInterface);
            }
    );
    InterfaceDetailsCloseButton->Click +=
        ref new RoutedEventHandler(
            [=](Platform::Object^, RoutedEventArgs^) {
                InterfaceDetails->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            }
    );
}

void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
    Window::Current->CoreWindow->VisibilityChanged += ref new TypedEventHandler<Windows::UI::Core::CoreWindow^, Windows::UI::Core::VisibilityChangedEventArgs^>(this, &MainPage::OnVisibilityChanged);

    SizeChanged += ref new SizeChangedEventHandler(this, &MainPage::OnWindowSizeChanged);
    Loaded += ref new RoutedEventHandler(this, &MainPage::OnLoaded);
    Unloaded += ref new RoutedEventHandler(this, &MainPage::OnUnloaded);

    _isVisible = Window::Current->CoreWindow->Visible;
}

void MainPage::OnLoaded(Object^ sender, RoutedEventArgs^ e)
{
    // Initialize api handle
    _apiInstance = otApiInit();
    if (_apiInstance)
    {
        // Register for device availability callbacks
        otSetDeviceAvailabilityChangedCallback(ApiInstance, ThreadDeviceAvailabilityCallback, nullptr);

        // Build the initial list
        BuildInterfaceList();
    }
}

void MainPage::OnUnloaded(Object^ sender, RoutedEventArgs^ e)
{
    // Unregister for callbacks
    otSetDeviceAvailabilityChangedCallback(ApiInstance, nullptr, nullptr);

    // Clean up api handle
    otApiFinalize(ApiInstance);
    _apiInstance = nullptr;
}

void MainPage::OnResuming()
{
}

void MainPage::OnWindowSizeChanged(Object^ sender, SizeChangedEventArgs^ args)
{
    _windowSize = args->NewSize;
}

void MainPage::OnVisibilityChanged(Windows::UI::Core::CoreWindow^ coreWindow, Windows::UI::Core::VisibilityChangedEventArgs^ args)
{
    // The Visible property is toggled when the app enters and exits minimized state.
    // But obscuring the app with another window does not change Visible state, oddly enough.
    _isVisible = args->Visible;
}

void MainPage::ShowInterfaceDetails(Platform::Guid InterfaceGuid)
{
    if (ApiInstance == nullptr) return;

    GUID deviceGuid = InterfaceGuid;
    auto device = otInstanceInit(ApiInstance, &deviceGuid);

    auto extendedAddress = otLinkGetExtendedAddress(device);
    if (extendedAddress)
    {
        WCHAR szMac[256] = { 0 };
        swprintf_s(szMac, 256, MAC8_FORMAT, MAC8_ARG(extendedAddress));
        InterfaceMacAddress->Text = ref new String(szMac);

        otFreeMemory(extendedAddress);
    }
    else
    {
        InterfaceMacAddress->Text = L"ERROR";
    }

    auto ml_eid = otThreadGetMeshLocalEid(device);
    if (ml_eid)
    {
        WCHAR szAddress[46] = { 0 };
        RtlIpv6AddressToStringW((const PIN6_ADDR)ml_eid, szAddress);
        InterfaceML_EID->Text = ref new String(szAddress);

        otFreeMemory(ml_eid);
    }
    else
    {
        InterfaceML_EID->Text = L"ERROR";
    }
    
    auto rloc16 = otThreadGetRloc16(device);
    WCHAR szRloc[16] = { 0 };
    swprintf_s(szRloc, 16, L"%4x", rloc16);
    InterfaceRLOC->Text = ref new String(szRloc);

    if (otThreadGetDeviceRole(device) > kDeviceRoleChild)
    {
        uint8_t index = 0;
        otChildInfo childInfo;
        while (kThreadError_None == otThreadGetChildInfoByIndex(device, index, &childInfo))
        {
            index++;
        }

        WCHAR szText[64] = { 0 };
        swprintf_s(szText, 64, L"%d", index);
        InterfaceChildren->Text = ref new String(szText);

        InterfaceNeighbors->Text = L"unknown";

        InterfaceNeighbors->Visibility = Windows::UI::Xaml::Visibility::Visible;
        InterfaceNeighborsText->Visibility = Windows::UI::Xaml::Visibility::Visible;
        InterfaceChildren->Visibility = Windows::UI::Xaml::Visibility::Visible;
        InterfaceChildrenText->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }

    // Show the details
    InterfaceDetails->Visibility = Windows::UI::Xaml::Visibility::Visible;

    otFreeMemory(device);
}

UIElement^ MainPage::CreateNewInterface(Platform::Guid InterfaceGuid)
{
    GUID deviceGuid = InterfaceGuid;

    auto device = otInstanceInit(ApiInstance, &deviceGuid);
    auto deviceRole = otThreadGetDeviceRole(device);

    WCHAR szText[256] = { 0 };
    swprintf_s(szText, 256, L"%s\r\n\t" GUID_FORMAT L"\r\n\t%s",
        L"openthread interface", // TODO ...
        GUID_ARG(deviceGuid), 
        ::ToString(deviceRole));

    auto InterfaceStackPanel = ref new StackPanel();
    InterfaceStackPanel->Orientation = Orientation::Horizontal;

    auto InterfaceTextBlock = ref new TextBlock();
    InterfaceTextBlock->Text = ref new String(szText);
    InterfaceTextBlock->FontSize = 16;
    InterfaceTextBlock->Margin = Thickness(10);
    InterfaceTextBlock->TextWrapping = TextWrapping::Wrap;
    InterfaceStackPanel->Children->Append(InterfaceTextBlock);

    if (deviceRole == kDeviceRoleDisabled)
    {
        auto ConnectButton = ref new Button();
        ConnectButton->Content = ref new String(L"Connect");
        ConnectButton->Click +=
            ref new RoutedEventHandler(
                [=](Platform::Object^, RoutedEventArgs^) {
                    _currentInterface = InterfaceGuid;
                    InterfaceConfiguration->Visibility = Windows::UI::Xaml::Visibility::Visible;
                }
        );
        InterfaceStackPanel->Children->Append(ConnectButton);
    }
    else
    {
        auto DetailsButton = ref new Button();
        DetailsButton->Content = ref new String(L"Details");
        DetailsButton->Click +=
            ref new RoutedEventHandler(
                [=](Platform::Object^, RoutedEventArgs^) {
                    ShowInterfaceDetails(InterfaceGuid);
                }
        );
        InterfaceStackPanel->Children->Append(DetailsButton);

        auto DisconnectButton = ref new Button();
        DisconnectButton->Content = ref new String(L"Disconnect");
        DisconnectButton->Click +=
            ref new RoutedEventHandler(
                [=](Platform::Object^, RoutedEventArgs^) {
                    DisconnectNetwork(InterfaceGuid);
                }
        );
        InterfaceStackPanel->Children->Append(DisconnectButton);
    }

    // Register for callbacks on the device
    otSetStateChangedCallback(device, ThreadStateChangeCallback, nullptr);

    // Cache the device
    _devices.push_back(device);

    return InterfaceStackPanel;
}

void MainPage::BuildInterfaceList()
{
    if (ApiInstance == nullptr) return;

    // Clear all existing children
    InterfaceList->Items->Clear();

    // Clean up devices
    for each (auto device in _devices)
    {
        // Unregister for callbacks for the device
        otSetStateChangedCallback((otInstance*)device, nullptr, nullptr);

        // Free the device
        otFreeMemory(device);
    }
    _devices.clear();

    // Enumerate the new device list
    auto deviceList = otEnumerateDevices(ApiInstance);
    if (deviceList)
    {
        // Dump the results to the console
        for (DWORD dwIndex = 0; dwIndex < deviceList->aDevicesLength; dwIndex++)
        {
            InterfaceList->Items->Append(
                CreateNewInterface(deviceList->aDevices[dwIndex]));
        }

        otFreeMemory(deviceList);
    }
}

void MainPage::ConnectNetwork(Platform::Guid InterfaceGuid)
{
    if (ApiInstance == nullptr) return;

    GUID deviceGuid = InterfaceGuid;
    auto device = otInstanceInit(ApiInstance, &deviceGuid);

    //
    // Configure
    //

    otNetworkName networkName = {};
    wcstombs(networkName.m8, InterfaceConfigName->Text->Data(), sizeof(networkName.m8));
    otThreadSetNetworkName(device, networkName.m8);

    otMasterKey masterKey = {};
    wcstombs((char*)masterKey.m8, InterfaceConfigKey->Text->Data(), sizeof(masterKey.m8));
    otThreadSetMasterKey(device, masterKey.m8, sizeof(masterKey.m8));

    otLinkSetChannel(device, (uint8_t)InterfaceConfigChannel->Value);
    otThreadSetMaxAllowedChildren(device, (uint8_t)InterfaceConfigMaxChildren->Value);

    otLinkSetPanId(device, 0x4567);

    //
    // Bring up the interface and start the Thread logic
    //

    otIp6SetEnabled(device, true);

    otThreadSetEnabled(device, true);

    // Cleanup
    otFreeMemory(device);
}

void MainPage::DisconnectNetwork(Platform::Guid InterfaceGuid)
{
    if (ApiInstance == nullptr) return;

    GUID deviceGuid = InterfaceGuid;
    auto device = otInstanceInit(ApiInstance, &deviceGuid);

    //
    // Start the Thread logic and the interface
    //

    otThreadSetEnabled(device, false);

    otIp6SetEnabled(device, false);

    // Cleanup
    otFreeMemory(device);
}
