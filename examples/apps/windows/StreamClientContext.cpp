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
#include "StreamClientContext.h"

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

StreamClientContext::StreamClientContext(
    IAsyncThreadNotify^ notify,
    StreamSocket^       client,
    ClientArgs^         args) :
    _notify{ std::move(notify) },
    _client{ std::move(client) },
    _args{ std::move(args) }
{
}

StreamClientContext::~StreamClientContext()
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
StreamClientContext::Connect_Click(
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
                OnConnection(_client);
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
ot::StreamClientContext::Send_Click(
    Platform::Object^                   sender,
    Windows::UI::Xaml::RoutedEventArgs^ e,
    Platform::String^                   input)
{
    SendMessage(GetDataWriter(), input);
}

IAsyncAction^
ot::StreamClientContext::CancelIO()
{
    return _client->CancelIOAsync();
}

void
StreamClientContext::OnConnection(
    StreamSocket^ streamSocket)
{
    SetConnected(true);
    ReceiveLoop(streamSocket, GetDataReader());
}

void
StreamClientContext::SetConnected(
    bool connected)
{
    _connected = connected;
}

bool
StreamClientContext::IsConnected() const
{
    return _connected;
}

void
StreamClientContext::ReceiveLoop(
    StreamSocket^ streamSocket,
    DataReader^   dataReader)
{
    // Read first 4 bytes (length of the subsequent string).
    create_task(dataReader->LoadAsync(sizeof(UINT32))).then(
        [this, dataReader](unsigned int size)
    {
        if (size < sizeof(UINT32))
        {
            // The underlying socket was closed before we were able to read the whole data.
            cancel_current_task();
        }

        auto strLen = dataReader->ReadUInt32();
        return create_task(dataReader->LoadAsync(strLen)).then(
            [this, dataReader, strLen](unsigned int actualStrLen)
        {
            if (actualStrLen != strLen)
            {
                // The underlying socket was closed before we were able to read the whole data.
                cancel_current_task();
            }

            Receive(dataReader, strLen);
        });
    }).then([this, streamSocket, dataReader](task<void> previousTask)
    {
        try
        {
            // Try getting all exceptions from the continuation chain above this point.
            previousTask.get();

            // Everything went ok, so try to receive another string. The receive will continue until the stream is
            // broken (i.e. peer closed the socket).
            ReceiveLoop(streamSocket, dataReader);
        }
        catch (Exception^ ex)
        {
            _notify->NotifyFromAsyncThread("Read stream failed with error: " + ex->Message,
                NotifyType::Error);

            // Explicitly close the socket.
            delete streamSocket;
        }
        catch (task_canceled&)
        {
            // Do not print anything here - this will usually happen because user closed the client socket.

            // Explicitly close the socket.
            delete streamSocket;
        }
    });
}

void
StreamClientContext::Receive(
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
StreamClientContext::SendMessage(
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
        dataWriter->WriteUInt32(msg->Length());
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

Windows::Storage::Streams::DataReader^
StreamClientContext::GetDataReader()
{
    if (_dataReader == nullptr)
    {
        _dataReader = ref new DataReader(_client->InputStream);
    }

    return _dataReader;
}

Windows::Storage::Streams::DataWriter^
StreamClientContext::GetDataWriter()
{
    if (_dataWriter == nullptr)
    {
        _dataWriter = ref new DataWriter(_client->OutputStream);
    }

    return _dataWriter;
}
