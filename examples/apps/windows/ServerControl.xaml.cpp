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
#include <utility>
#include <algorithm>
#include "ServerControl.xaml.h"
#include "Factory.h"
#include "TalkHelper.h"

using namespace ot;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

ServerControl::ServerControl()
{
    InitializeComponent();

    ServerPort->Text = DEF_PORT.ToString();
}

void
ServerControl::Init(
    IAsyncThreadNotify^  notify,
    IMainPageUIElements^ mainPageUIElements)
{
    _notify = std::move(notify);
    _mainPageUIElements = std::move(mainPageUIElements);
}

void
ServerControl::ProtocolChanged(
    Protocol protocol)
{
    _protocol = protocol;
}

void
ServerControl::Listen_Click(
    Object^          sender,
    RoutedEventArgs^ e)
{
    try
    {
        auto listenerArgs = ref new ListenerArgs();

        listenerArgs->ServerName = ServerName->Text;

        auto serverIP = ServerIP->Text;
        if (serverIP->IsEmpty())
        {
            throw Exception::CreateException(E_INVALIDARG, "No Server IP");
        }

        // check valid chars of Ipv6 Address
        if (!TalkHelper::AllValidIpv6Chars(serverIP->Data(), serverIP->Data() + serverIP->Length()))
        {
            throw Exception::CreateException(E_INVALIDARG, "Not a valid Server IPv6 address");
        }

        listenerArgs->ServerHostName = ref new HostName(serverIP);

        if (ServerPort->Text->IsEmpty())
        {
            throw Exception::CreateException(E_INVALIDARG, "No Server Port");
        }
        listenerArgs->ServerPort = ServerPort->Text;

        auto listenerContext = Factory::CreateListenerContext(_notify, listenerArgs, _protocol);
        listenerContext->Listen_Click(sender, e);
    }
    catch (Exception^ ex)
    {
        _notify->NotifyFromAsyncThread(
            "Listening failed with input error: " + ex->Message,
            NotifyType::Error);
    }
}

void
ServerControl::Exit_Click(
    Object^          sender,
    RoutedEventArgs^ e)
{
    _mainPageUIElements->TalkGrid->Visibility = Xaml::Visibility::Collapsed;
    _mainPageUIElements->ThreadGrid->Visibility = Xaml::Visibility::Visible;
}
