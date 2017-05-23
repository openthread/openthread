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
#include "TalkGrid.xaml.h"

using namespace ot;

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

#define GUID_FORMAT L"{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}"
#define GUID_ARG(guid) guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]

void otLog(PCSTR aFormat, ...)
{
    va_list args;
    va_start(args, aFormat);

    CHAR logString[512] = { 0 };
    int charsWritten = vsprintf_s(logString, sizeof(logString), aFormat, args);
    if (charsWritten > 0) OutputDebugStringA(logString);

    va_end(args);
}

MainPage::MainPage() : _otApi(nullptr)
{
    InitializeComponent();

    InterfaceConfigCancelButton->Click +=
        ref new RoutedEventHandler(
            [=](Platform::Object^, RoutedEventArgs^) {
                this->InterfaceConfiguration->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                this->_curAdapter = nullptr;
            }
    );
    InterfaceConfigOkButton->Click +=
        ref new RoutedEventHandler(
            [=](Platform::Object^, RoutedEventArgs^) {
                this->InterfaceConfiguration->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                this->ConnectNetwork(_curAdapter);
                this->_curAdapter = nullptr;
            }
    );
    InterfaceDetailsCloseButton->Click +=
        ref new RoutedEventHandler(
            [=](Platform::Object^, RoutedEventArgs^) {
                this->InterfaceDetails->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            }
    );
    Talk->Click +=
        ref new RoutedEventHandler(
            [=](Platform::Object^, RoutedEventArgs^) {
                this->ThreadGrid->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                this->TalkGrid->Visibility = Windows::UI::Xaml::Visibility::Visible;
            }
    );

    TlkGrid->Init(this);
}

void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
    Loaded += ref new RoutedEventHandler(this, &MainPage::OnLoaded);
    Unloaded += ref new RoutedEventHandler(this, &MainPage::OnUnloaded);
}

void MainPage::OnLoaded(Object^ sender, RoutedEventArgs^ e)
{
    try
    {
        // Initialize api handle
        _otApi = ref new otApi();

        // Register for state changes
        _adapterArrivalToken =
            _otApi->AdapterArrival +=
                ref new otAdapterArrivalDelegate(
                    [=](otAdapter^ adapter) {
                        // Update on the UI thread
                        this->Dispatcher->RunAsync(
                            Windows::UI::Core::CoreDispatcherPriority::Normal,
                            ref new Windows::UI::Core::DispatchedHandler(
                                [=]() {
                                    this->AddAdapterToList(adapter);
                                }));
                    });
        
        // Enumerate the adapter list
        auto adapters = _otApi->GetAdapters();
        for (auto&& adapter : adapters) {
            AddAdapterToList(adapter);
        }
    }
    catch (Exception^)
    {
    }
}

void MainPage::OnUnloaded(Object^ sender, RoutedEventArgs^ e)
{
    if (_otApi)
    {
        // Unregister
        _otApi->AdapterArrival -= _adapterArrivalToken;

        // Clear current adapter
        _curAdapter = nullptr;

        // Remove the adapter list
        auto adapters = _otApi->GetAdapters();
        for (auto&& adapter : adapters) {
            adapter->InvokeAdapterRemoval();
        }

        // Free the api handle
        _otApi = nullptr;
    }
}

void MainPage::OnResuming()
{
}

void MainPage::ShowInterfaceDetails(otAdapter^ adapter)
{
    try
    {
        InterfaceMacAddress->Text = otApi::MacToString(adapter->ExtendedAddress);
        InterfaceML_EID->Text = adapter->MeshLocalEid->ToString();
        InterfaceRLOC->Text = otApi::Rloc16ToString(adapter->Rloc16);

        if (adapter->State > otThreadState::Child)
        {
            uint8_t index = 0;
            otChildInfo childInfo;
            while (OT_ERROR_NONE == otThreadGetChildInfoByIndex((otInstance*)(void*)adapter->RawHandle, index, &childInfo))
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
    }
    catch (Exception^)
    {

    }
}

void MainPage::AddAdapterToList(otAdapter^ adapter)
{
    try
    {
        GUID interfaceGuid = adapter->InterfaceGuid;
        WCHAR szName[256] = { 0 };
        swprintf_s(szName, 256, GUID_FORMAT, GUID_ARG(interfaceGuid));

        auto InterfaceStackPanel = ref new StackPanel();
        InterfaceStackPanel->Name = ref new String(szName);
        InterfaceStackPanel->Orientation = Orientation::Horizontal;

        otLog("%S arrival!\n", InterfaceStackPanel->Name->Data());

        // Basic description text
        auto InterfaceTextBlock = ref new TextBlock();
        InterfaceTextBlock->Text = ref new String(L"openthread interface");
        InterfaceTextBlock->FontSize = 16;
        InterfaceTextBlock->Margin = Thickness(10);
        InterfaceTextBlock->TextWrapping = TextWrapping::Wrap;
        InterfaceStackPanel->Children->Append(InterfaceTextBlock);

        // Connect button
        auto ConnectButton = ref new Button();
        ConnectButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        ConnectButton->Content = ref new String(L"Connect");
        ConnectButton->Click +=
            ref new RoutedEventHandler(
                [=](Platform::Object^, RoutedEventArgs^) {
                    this->_curAdapter = adapter;
                    this->InterfaceConfiguration->Visibility = Windows::UI::Xaml::Visibility::Visible;
                }
            );
        InterfaceStackPanel->Children->Append(ConnectButton);

        // Details button
        auto DetailsButton = ref new Button();
        DetailsButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        DetailsButton->Content = ref new String(L"Details");
        DetailsButton->Click +=
            ref new RoutedEventHandler(
                [=](Platform::Object^, RoutedEventArgs^) {
                    this->ShowInterfaceDetails(adapter);
                }
            );
        InterfaceStackPanel->Children->Append(DetailsButton);

        // Disconnect button
        auto DisconnectButton = ref new Button();
        DisconnectButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        DisconnectButton->Content = ref new String(L"Disconnect");
        DisconnectButton->Click +=
            ref new RoutedEventHandler(
                [=](Platform::Object^, RoutedEventArgs^) {
                    this->DisconnectNetwork(adapter);
                }
            );
        InterfaceStackPanel->Children->Append(DisconnectButton);

        // Delegate for handling role changes
        auto OnAdapterRoleChanged =
            [=]() {
                GUID interfaceGuid = adapter->InterfaceGuid;
                auto state = adapter->State;
                auto stateStr = otApi::ThreadStateToString(adapter->State);

                WCHAR szText[256] = { 0 };
                swprintf_s(szText, 256, GUID_FORMAT L"\r\n\t%s\r\n\t%s",
                    GUID_ARG(interfaceGuid),
                    stateStr->Data(),
                    state >= otThreadState::Child ? 
                        adapter->MeshLocalEid->ToString()->Data() : 
                        L"");

                InterfaceTextBlock->Text = ref new String(szText);

                otLog("%S state = %S\n", InterfaceStackPanel->Name->Data(), stateStr->Data());

                if (state == otThreadState::Disabled)
                {
                    ConnectButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
                    DetailsButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                    DisconnectButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                }
                else
                {
                    ConnectButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                    DetailsButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
                    DisconnectButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
                }
            };

        // Register for role change callbacks
        auto adapterRoleChangedToken = 
            adapter->NetRoleChanged +=
                ref new otNetRoleChangedDelegate(
                    [=](auto sender) {
                        // Update the text on the UI thread
                        this->Dispatcher->RunAsync(
                            Windows::UI::Core::CoreDispatcherPriority::Normal,
                            ref new Windows::UI::Core::DispatchedHandler(
                                [=]() {
                                    OnAdapterRoleChanged();
                                }
                            )
                        );
                    }
                );

        // Register for address change callbacks
        auto adapterMeshLocalAddresChangedToken =
            adapter->IpMeshLocalAddresChanged +=
                ref new otIpMeshLocalAddresChangedDelegate(
                    [=](auto sender) {
                        // Update the text on the UI thread
                        this->Dispatcher->RunAsync(
                            Windows::UI::Core::CoreDispatcherPriority::Normal,
                            ref new Windows::UI::Core::DispatchedHandler(
                                [=]() {
                                    OnAdapterRoleChanged();
                                }
                            )
                        );
                    }
                );

        // Register for adapter removal callbacks
        Windows::Foundation::EventRegistrationToken adapterRemovalToken;
        adapterRemovalToken =
            adapter->AdapterRemoval +=
                ref new otAdapterRemovalDelegate(
                    [=](otAdapter^ adapter) {
                        // Unregister
                        adapter->NetRoleChanged -= adapterRoleChangedToken;
                        adapter->IpMeshLocalAddresChanged -= adapterMeshLocalAddresChangedToken;
                        adapter->AdapterRemoval -= adapterRemovalToken;

                        // Remove the item on the UI thread
                        this->Dispatcher->RunAsync(
                            Windows::UI::Core::CoreDispatcherPriority::Normal,
                            ref new Windows::UI::Core::DispatchedHandler(
                                [=]() {
                                    for (uint32_t i = 0; i < this->InterfaceList->Items->Size; i++)
                                    {
                                        auto Item = dynamic_cast<StackPanel^>(this->InterfaceList->Items->GetAt(i));
                                        if (Item == InterfaceStackPanel)
                                        {
                                            otLog("%S removal!\n", InterfaceStackPanel->Name->Data());
                                            this->InterfaceList->Items->RemoveAt(i);
                                            break;
                                        }
                                    }
                                }));
                    });

        // Trigger the initial role change
        OnAdapterRoleChanged();

        // Add the interface to the list
        InterfaceList->Items->Append(InterfaceStackPanel);
    }
    catch (Exception^)
    {
    }
}

void MainPage::ConnectNetwork(otAdapter^ adapter)
{
    try
    {
        GUID interfaceGuid = adapter->InterfaceGuid;
        WCHAR szName[256] = { 0 };
        swprintf_s(szName, 256, GUID_FORMAT, GUID_ARG(interfaceGuid));
        otLog("%S starting connection...\n", szName);

        // Configure
        adapter->NetworkName = InterfaceConfigName->Text;
        adapter->MasterKey = InterfaceConfigKey->Text;
        adapter->Channel = (uint8_t)InterfaceConfigChannel->Value;
        adapter->MaxAllowedChildren = (uint8_t)InterfaceConfigMaxChildren->Value;
        adapter->PanId = 0x4567;

        // Bring up the interface and start the Thread logic
        adapter->IpEnabled = true;
        adapter->ThreadEnabled = true;
    }
    catch (Exception^)
    {

    }
}

void MainPage::DisconnectNetwork(otAdapter^ adapter)
{
    try
    {
        GUID interfaceGuid = adapter->InterfaceGuid;
        WCHAR szName[256] = { 0 };
        swprintf_s(szName, 256, GUID_FORMAT, GUID_ARG(interfaceGuid));
        otLog("%S disconnecting...\n", szName);

        // Stop the Thread network and bring down the interface
        adapter->ThreadEnabled = false;
        adapter->IpEnabled = false;
    }
    catch (Exception^)
    {

    }
}

Windows::UI::Xaml::UIElement^
MainPage::ThreadGrid::get()
{
    return ThrdGrid;
}

Windows::UI::Xaml::UIElement^
MainPage::TalkGrid::get()
{
    return TlkGrid;
}
