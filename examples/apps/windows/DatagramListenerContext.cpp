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
#include "DatagramListenerContext.h"

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

DatagramListenerContext::DatagramListenerContext(
    IAsyncThreadNotify^ notify,
    DatagramSocket^     listener,
    ListenerArgs^       args) :
    _notify{ std::move(notify) },
    _listener{ std::move(listener) },
    _args{ std::move(args) }
{
}

DatagramListenerContext::~DatagramListenerContext()
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
DatagramListenerContext::Listen_Click(
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

    _listener->MessageReceived += ref new MessageHandler(
        this, &DatagramListenerContext::OnMessage);

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
DatagramListenerContext::CancelIO()
{
    return _listener->CancelIOAsync();
}

void
DatagramListenerContext::OnMessage(
    DatagramSocket^           socket,
    MessageReceivedEventArgs^ eventArgs)
{
    if (_outputStream != nullptr)
    {
        auto dataReader = eventArgs->GetDataReader();
        Receive(dataReader, dataReader->UnconsumedBufferLength, GetDataWriter());
        return;
    }

    // We do not have an output stream yet so create one.
    create_task(socket->GetOutputStreamAsync(eventArgs->RemoteAddress, eventArgs->RemotePort)).then(
        [this, socket, eventArgs](IOutputStream^ stream)
    {
        {
            std::lock_guard<mutex_t> lock(_mtx);

            // It might happen that the OnMessage was invoked more than once before the GetOutputStreamAsync call
            // completed. In this case we will end up with multiple streams - just keep one of them.
            if (_outputStream == nullptr)
            {
                _outputStream = stream;
            }
        }

        auto dataReader = eventArgs->GetDataReader();
        Receive(dataReader, dataReader->UnconsumedBufferLength, GetDataWriter());
    }).then([this](task<void> prevTask)
    {
        try
        {
            // Try getting all exceptions from the continuation chain above this point.
            prevTask.get();
        }
        catch (Exception^ ex)
        {
            _notify->NotifyFromAsyncThread("On message with an error: " + ex->Message,
                NotifyType::Error);
        }
        catch (task_canceled&)
        {
            // Do not print anything here - this will usually happen because user closed the client socket.
        }
    });
}

void
DatagramListenerContext::Receive(
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
DatagramListenerContext::CreateEchoMessage(
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
DatagramListenerContext::EchoMessage(
    DataWriter^ dataWriter,
    String^     echo)
{
    try
    {
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

Windows::Storage::Streams::DataWriter^
DatagramListenerContext::GetDataWriter()
{
    if (_dataWriter == nullptr)
    {
        _dataWriter = ref new DataWriter(_outputStream);
    }

    return _dataWriter;
}
