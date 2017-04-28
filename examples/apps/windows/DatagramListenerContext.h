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

#include <mutex>
#include "IListenerContext.h"
#include "IAsyncThreadNotify.h"
#include "ListenerArgs.h"

namespace ot
{

[Windows::Foundation::Metadata::WebHostHidden]
public ref class DatagramListenerContext sealed : public IListenerContext
{
public:
    using DatagramSocket = Windows::Networking::Sockets::DatagramSocket;

    DatagramListenerContext(IAsyncThreadNotify^ notify, DatagramSocket^ listener, ListenerArgs^ args);
    virtual ~DatagramListenerContext();

    virtual void Listen_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

    virtual Windows::Foundation::IAsyncAction^ CancelIO();

private:
    using MessageReceivedEventArgs = Windows::Networking::Sockets::DatagramSocketMessageReceivedEventArgs;
    using MessageHandler = Windows::Foundation::TypedEventHandler<DatagramSocket^, MessageReceivedEventArgs^>;
    using DataReader = Windows::Storage::Streams::DataReader;
    using DataWriter = Windows::Storage::Streams::DataWriter;
    using IOutputStream = Windows::Storage::Streams::IOutputStream;
    using Listener = DatagramSocket;
    using Args = ListenerArgs;
    using mutex_t = std::mutex;

    void OnMessage(DatagramSocket^ socket, MessageReceivedEventArgs^ eventArgs);
    void Receive(DataReader^, unsigned int strLen, DataWriter^);
    String^ CreateEchoMessage(String^ msg);
    void EchoMessage(DataWriter^, String^ echo);

    DataWriter^ GetDataWriter();

    IAsyncThreadNotify^ _notify;
    Listener^           _listener;
    Args^               _args;
    DataWriter^         _dataWriter;

    mutable mutex_t _mtx;
    // mutex protected data
    IOutputStream^  _outputStream;
};

} // namespace ot
