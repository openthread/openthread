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
#include "DatagramClientContext.h"

using namespace ot;

using namespace Concurrency;
using namespace Platform;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Networking::Sockets;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

DatagramClientContext::DatagramClientContext(
    IAsyncThreadNotify^ notify,
    DatagramSocket^     client,
    ClientArgs^         args) :
    _notify{ std::move(notify) },
    _client{ std::move(client) },
    _args{ std::move(args) }
{
}

DatagramClientContext::~DatagramClientContext()
{
    // A Client can be closed in two ways:
    //  - explicitly: using the 'delete' keyword (client is closed even if there are outstanding references to it).
    //  - implicitly: removing the last reference to it (i.e., falling out-of-scope).
    //
    // When a Socket is closed implicitly, it can take several seconds for the local port being used
    // by it to be freed/reclaimed by the lower networking layers. During that time, other sockets on the machine
    // will not be able to use the port. Thus, it is strongly recommended that Socket instances be explicitly
    // closed before they go out of scope(e.g., before application exit). The call below explicitly closes the socket.
    if (_client != nullptr)
    {
        delete _client;
        _client = nullptr;
    }
}

void
DatagramClientContext::Connect_Click(
    Object^          sender,
    RoutedEventArgs^ e)
{
    task<void> removeContext;

    if (CoreApplication::Properties->HasKey("clientContext"))
    {
        auto clientContext = dynamic_cast<IClientContext^>(
            CoreApplication::Properties->Lookup("clientContext"));
        if (clientContext == nullptr)
        {
            throw ref new FailureException(L"No clientContext");
        }

        removeContext = create_task(clientContext->CancelIO()).then(
            []()
        {
            CoreApplication::Properties->Remove("clientContext");
        });
    }
    else
    {
        removeContext = create_task([]() {});
    }

    _client->MessageReceived += ref new MessageHandler(
        this, &DatagramClientContext::OnMessage);

    removeContext.then([this](task<void> prevTask)
    {
        try
        {
            // Try getting an exception.
            prevTask.get();

            // Events cannot be hooked up directly to the ScenarioInput2 object, as the object can fall out-of-scope and be
            // deleted. This would render any event hooked up to the object ineffective. The ClientContext guarantees that
            // both the socket and object that serves its events have the same lifetime.
            CoreApplication::Properties->Insert("clientContext", this);
        }
        catch (Exception^ ex)
        {
            _notify->NotifyFromAsyncThread(
                "Remove clientContext error: " + ex->Message,
                NotifyType::Error);
        }
        catch (task_canceled&)
        {
        }
    }).then([this]()
    {
        auto endpointPair = ref new EndpointPair(_args->ClientHostName, _args->ClientPort,
            _args->ServerHostName, _args->ServerPort);

        _notify->NotifyFromAsyncThread("Start connecting", NotifyType::Status);

        create_task(_client->ConnectAsync(endpointPair)).then(
            [this, endpointPair](task<void> prevTask)
        {
            try
            {
                // Try getting an exception.
                prevTask.get();
                _notify->NotifyFromAsyncThread(
                    "Connect from " + endpointPair->LocalHostName->CanonicalName +
                    " to " + endpointPair->RemoteHostName->CanonicalName,
                    NotifyType::Status);
                SetConnected(true);
            }
            catch (Exception^ ex)
            {
                _notify->NotifyFromAsyncThread(
                    "Start binding failed with error: " + ex->Message,
                    NotifyType::Error);
                CoreApplication::Properties->Remove("clientContext");
            }
            catch (task_canceled&)
            {
                CoreApplication::Properties->Remove("clientContext");
            }
        });
    });
}

void
DatagramClientContext::Send_Click(
    Object^          sender,
    RoutedEventArgs^ e,
    String^          input)
{
    SendMessage(GetDataWriter(), input);
}

IAsyncAction^
DatagramClientContext::CancelIO()
{
    return _client->CancelIOAsync();
}

void
DatagramClientContext::SetConnected(
    bool connected)
{
    _connected = connected;
}

bool
ot::DatagramClientContext::IsConnected() const
{
    return _connected;
}

void
ot::DatagramClientContext::OnMessage(
    DatagramSocket^           socket,
    MessageReceivedEventArgs^ eventArgs)
{
    try
    {
        auto dataReader = eventArgs->GetDataReader();
        Receive(dataReader, dataReader->UnconsumedBufferLength);
    }
    catch (Exception^ ex)
    {
        auto socketError = SocketError::GetStatus(ex->HResult);
        if (socketError == SocketErrorStatus::ConnectionResetByPeer)
        {
            // This error would indicate that a previous send operation resulted in an ICMP "Port Unreachable" message.
            _notify->NotifyFromAsyncThread(
                "Peer does not listen on the specific port. Please make sure that you run step 1 first "
                "or you have a server properly working on a remote server.",
                NotifyType::Error);
        }
        else if (socketError != SocketErrorStatus::Unknown)
        {
            _notify->NotifyFromAsyncThread(
                "Error happened when receiving a datagram: " + socketError.ToString(),
                NotifyType::Error);
        }
        else
        {
            throw;
        }
    }
}

void
DatagramClientContext::Receive(
    DataReader^  dataReader,
    unsigned int strLen)
{
    if (!strLen)
    {
        return;
    }

    auto msg = dataReader->ReadString(strLen);
    _notify->NotifyFromAsyncThread("Received data from server: \"" + msg + "\"",
        NotifyType::Status);
}

void
DatagramClientContext::SendMessage(
    DataWriter^ dataWriter,
    String^     msg)
{
    if (!IsConnected())
    {
        _notify->NotifyFromAsyncThread("This socket is not yet connected.", NotifyType::Error);
        return;
    }

    try
    {
        dataWriter->WriteString(msg);
        _notify->NotifyFromAsyncThread("Sending - " + msg, NotifyType::Status);
    }
    catch (Exception^ ex)
    {
        _notify->NotifyFromAsyncThread("Sending failed with error: " + ex->Message, NotifyType::Error);
    }

    // Write the locally buffered data to the network. Please note that write operation will succeed
    // even if the server is not listening.
    create_task(dataWriter->StoreAsync()).then(
        [this](task<unsigned int> writeTask)
    {
        try
        {
            // Try getting an exception.
            writeTask.get();
        }
        catch (Exception^ ex)
        {
            _notify->NotifyFromAsyncThread("Send failed with error: " + ex->Message, NotifyType::Error);
        }
    });
}

Windows::Storage::Streams::DataWriter^
DatagramClientContext::GetDataWriter()
{
    if (_dataWriter == nullptr)
    {
        _dataWriter = ref new DataWriter(_client->OutputStream);
    }

    return _dataWriter;
}
