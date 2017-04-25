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

#include "IListenerContext.h"
#include "IAsyncThreadNotify.h"
#include "ListenerArgs.h"

namespace ot
{

[Windows::Foundation::Metadata::WebHostHidden]
public ref class StreamListenerContext sealed : public IListenerContext
{
public:
    using StreamSocketListener = Windows::Networking::Sockets::StreamSocketListener;

    StreamListenerContext(IAsyncThreadNotify^ notify, StreamSocketListener^ listener, ListenerArgs^ args);
    virtual ~StreamListenerContext();

    virtual void Listen_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

    virtual Windows::Foundation::IAsyncAction^ CancelIO();

private:
    using ConnectionReceivedEventArgs = Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs;
    using ConnectionHandler = Windows::Foundation::TypedEventHandler<StreamSocketListener^, ConnectionReceivedEventArgs^>;
    using DataReader = Windows::Storage::Streams::DataReader;
    using DataWriter = Windows::Storage::Streams::DataWriter;
    using Listener = StreamSocketListener;
    using StreamSocket = Windows::Networking::Sockets::StreamSocket;
    using Args = ListenerArgs;

    void OnConnection(StreamSocketListener^ listener, ConnectionReceivedEventArgs^ args);
    void ReceiveLoop(StreamSocket^, DataReader^, DataWriter^);
    void Receive(DataReader^, unsigned int strLen, DataWriter^);
    String^ CreateEchoMessage(String^ msg);
    void EchoMessage(DataWriter^, String^ echo);

    IAsyncThreadNotify^ _notify;
    Listener^           _listener;
    Args^               _args;
};

} // namespace ot
