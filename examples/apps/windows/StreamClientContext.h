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

#include "IClientContext.h"
#include "IAsyncThreadNotify.h"
#include "ClientArgs.h"

namespace Thread
{

[Windows::Foundation::Metadata::WebHostHidden]
public ref class StreamClientContext sealed : public IClientContext
{
public:
    using StreamSocket = Windows::Networking::Sockets::StreamSocket;

    StreamClientContext(IAsyncThreadNotify^ notify, StreamSocket^ client, ClientArgs^ args);
    virtual ~StreamClientContext();

    virtual void Connect_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

    virtual void Send_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e, Platform::String^ input);

    virtual Windows::Foundation::IAsyncAction^ CancelIO();

private:
    using DataReader = Windows::Storage::Streams::DataReader;
    using DataWriter = Windows::Storage::Streams::DataWriter;
    using Args = ClientArgs;

    void OnConnection(StreamSocket^);

    void SetConnected(bool connected);
    bool IsConnected() const;

    void ReceiveLoop(StreamSocket^, DataReader^);
    void Receive(DataReader^, unsigned int strLen);

    void SendMessage(DataWriter^, String^ msg);

    DataReader^ GetDataReader();
    DataWriter^ GetDataWriter();

    IAsyncThreadNotify^ _notify;
    StreamSocket^       _client;
    Args^               _args;
    bool                _connected = false;
    DataReader^         _dataReader;
    DataWriter^         _dataWriter;
};

} // namespace Thread
