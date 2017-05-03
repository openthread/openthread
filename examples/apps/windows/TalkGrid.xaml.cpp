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
#include "TalkGrid.xaml.h"
#include "ClientControl.xaml.h"
#include "ServerControl.xaml.h"

using namespace ot;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

TalkGrid::TalkGrid()
{
    InitializeComponent();

    TcpRadio->IsChecked = true;
    ServerRadio->IsChecked = true;
}


void
TalkGrid::Init(
    IMainPageUIElements^ mainPageUIElements)
{
    ServerRole->Init(this, mainPageUIElements);
    ClientRole->Init(this, mainPageUIElements);
}

void
TalkGrid::NotifyFromAsyncThread(
    String^    message,
    NotifyType type)
{
    Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
        ref new DispatchedHandler([this, message, type]()
    {
        Notify(message, type);
    }));
}

void
TalkGrid::Notify(
    String^    message,
    NotifyType type)
{
    switch (type)
    {
    case NotifyType::Status:
        StatusBorder->Background = ref new SolidColorBrush(Colors::Green);
        break;
    case NotifyType::Error:
        StatusBorder->Background = ref new SolidColorBrush(Colors::Red);
        break;
    default:
        break;
    }

    StatusBlock->Text = message;

    // Collapse the StatusBlock if it has no text to conserve real estate.
    if (StatusBlock->Text != "")
    {
        StatusBorder->Visibility = Xaml::Visibility::Visible;
    }
    else
    {
        StatusBorder->Visibility = Xaml::Visibility::Collapsed;
    }
}

void
TalkGrid::Protocol_Changed(
    Object^          sender,
    RoutedEventArgs^ e)
{
    auto radioBtn = dynamic_cast<RadioButton^>(sender);
    if (!radioBtn)
    {
        return;
    }

    auto protocol = (radioBtn == TcpRadio) ? Protocol::TCP : Protocol::UDP;

    ServerRole->ProtocolChanged(protocol);
    ClientRole->ProtocolChanged(protocol);
}

void
TalkGrid::Role_Changed(
    Object^          sender,
    RoutedEventArgs^ e)
{
    auto radioBtn = dynamic_cast<RadioButton^>(sender);
    if (!radioBtn)
    {
        return;
    }

    if (radioBtn == ServerRadio)
    {
        // switch to server role UI
        ClientRole->Visibility = Xaml::Visibility::Collapsed;
        ServerRole->Visibility = Xaml::Visibility::Visible;
    }
    else
    {
        // switch to client role UI
        ServerRole->Visibility = Xaml::Visibility::Collapsed;
        ClientRole->Visibility = Xaml::Visibility::Visible;
    }
}
