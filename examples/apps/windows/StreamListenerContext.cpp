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
#include "StreamListenerContext.h"

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

StreamListenerContext::StreamListenerContext(
    IAsyncThreadNotify^   notify,
    StreamSocketListener^ listener,
    ListenerArgs^         args) :
    _notify{ std::move(notify) },
    _listener{ std::move(listener) },
    _args{ std::move(args) }
{
}

StreamListenerContext::~StreamListenerContext()
{
    // A Listener can be closed in two ways:
    //  - explicitly: using the 'delete' keyword (listener is closed even if there are outstanding references to it).
    //  - implicitly: removing the last reference to it (i.e., falling out-of-scope).
    //
    // When a Socket is closed implicitly, it can take several seconds for the local port being used
    // by it to be freed/reclaimed by the lower networking layers. During that time, other sockets on the machine
    // will not be able to use the port. Thus, it is strongly recommended that Socket instances be explicitly
    // closed before they go out of scope(e.g., before application exit). The call below explicitly closes the socket.
    if (_listener != nullptr)
    {
        delete _listener;
        _listener = nullptr;
    }
}

void
StreamListenerContext::Listen_Click(
    Object^          sender,
    RoutedEventArgs^ e)
{
    task<void> removeContext;

    if (CoreApplication::Properties->HasKey("listenerContext"))
    {
        auto listenerContext = dynamic_cast<IListenerContext^>(
            CoreApplication::Properties->Lookup("listenerContext"));
        if (listenerContext == nullptr)
        {
            throw ref new FailureException(L"No listenerContext");
        }

        removeContext = create_task(listenerContext->CancelIO()).then(
            []()
        {
            CoreApplication::Properties->Remove("listenerContext");
        });
    }
    else
    {
        removeContext = create_task([]() {});
    }

    _listener->ConnectionReceived += ref new ConnectionHandler(
        this, &StreamListenerContext::OnConnection);

    removeContext.then([this](task<void> prevTask)
    {
        try
        {
            // Try getting an exception.
            prevTask.get();

            // Events cannot be hooked up directly to the ScenarioInput1 object, as the object can fall out-of-scope and be
            // deleted. This would render any event hooked up to the object ineffective. The ListenerContext guarantees that
            // both the listener and object that serves its events have the same lifetime.
            CoreApplication::Properties->Insert("listenerContext", this);
        }
        catch (Exception^ ex)
        {
            _notify->NotifyFromAsyncThread(
                "Remove listenerContext error: " + ex->Message,
                NotifyType::Error);
        }
        catch (task_canceled&)
        {
        }
    }).then([this]()
    {
        _notify->NotifyFromAsyncThread("Start listening", NotifyType::Status);

        create_task(_listener->BindEndpointAsync(_args->ServerHostName, _args->ServerPort)).then(
            [this](task<void> prevTask)
        {
            try
            {
                // Try getting an exception.
                prevTask.get();
                _notify->NotifyFromAsyncThread(
                    "Listening on address " + _args->ServerHostName->CanonicalName,
                    NotifyType::Status);
            }
            catch (Exception^ ex)
            {
                _notify->NotifyFromAsyncThread(
                    "Start listening failed with error: " + ex->Message,
                    NotifyType::Error);
                CoreApplication::Properties->Remove("listenerContext");
            }
        });
    });
}

IAsyncAction^
StreamListenerContext::CancelIO()
{
    return _listener->CancelIOAsync();
}

void
StreamListenerContext::OnConnection(
    StreamSocketListener^        listener,
    ConnectionReceivedEventArgs^ args)
{
    auto dataReader = ref new DataReader(args->Socket->InputStream);
    auto dataWriter = ref new DataWriter(args->Socket->OutputStream);

    ReceiveLoop(args->Socket, dataReader, dataWriter);
}

void
StreamListenerContext::ReceiveLoop(
    StreamSocket^ streamSocket,
    DataReader^   dataReader,
    DataWriter^   dataWriter)
{
    // Read first 4 bytes (length of the subsequent string).
    create_task(dataReader->LoadAsync(sizeof(UINT32))).then(
        [this, streamSocket, dataReader, dataWriter](unsigned int size)
    {
        if (size < sizeof(UINT32))
        {
            // The underlying socket was closed before we were able to read the whole data.
            cancel_current_task();
        }

        unsigned int strLen = dataReader->ReadUInt32();
        return create_task(dataReader->LoadAsync(strLen)).then(
            [this, dataReader, dataWriter, strLen](unsigned int actualStrLen)
        {
            if (actualStrLen != strLen)
            {
                // The underlying socket was closed before we were able to read the whole data.
                cancel_current_task();
            }

            Receive(dataReader, strLen, dataWriter);
        });
    }).then([this, streamSocket, dataReader, dataWriter](task<void> previousTask)
    {
        try
        {
            // Try getting all exceptions from the continuation chain above this point.
            previousTask.get();

            // Everything went ok, so try to receive another string. The receive will continue until the stream is
            // broken (i.e. peer closed the socket).
            ReceiveLoop(streamSocket, dataReader, dataWriter);
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
StreamListenerContext::Receive(
    DataReader^  dataReader,
    unsigned int strLen,
    DataWriter^  dataWriter)
{
    if (!strLen)
    {
        return;
    }

    auto msg = dataReader->ReadString(strLen);
    _notify->NotifyFromAsyncThread("Received data from client: \"" + msg + "\"",
        NotifyType::Status);
    auto echo = CreateEchoMessage(msg);
    EchoMessage(dataWriter, echo);
}

String^
StreamListenerContext::CreateEchoMessage(
    String^ msg)
{
    wchar_t buf[256];
    auto len = swprintf_s(buf, L"Server%s received data from client : \"%s\"",
        _args->ServerName->IsEmpty() ? L"" : (" " + _args->ServerName)->Data(), msg->Data());

    len += swprintf_s(&buf[len], _countof(buf) - len, L" - got %d chars",
        msg->Length());
    return ref new String(buf);
}

void
StreamListenerContext::EchoMessage(
    DataWriter^ dataWriter,
    String^     echo)
{
    try
    {
        dataWriter->WriteUInt32(echo->Length());
        dataWriter->WriteString(echo);
    }
    catch (Exception^ ex)
    {
        _notify->NotifyFromAsyncThread("Echoing failed with error: " + ex->Message,
            NotifyType::Error);
    }

    create_task(dataWriter->StoreAsync()).then(
        [this](task<unsigned int> writeTask)
    {
        try
        {
            // Try getting all exceptions from the continuation chain above this point.
            writeTask.get();
        }
        catch (Exception^ ex)
        {
            _notify->NotifyFromAsyncThread("Echo message with an error: " + ex->Message,
                NotifyType::Error);
        }
    });
}
