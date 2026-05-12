/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include "instance/instance.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {

class UnitTester
{
public:
    static void TestCoapMessage(void)
    {
        Instance                          *instance;
        Coap::Message                     *message;
        Coap::HeaderInfo                   headerInfo;
        Coap::Token                        readToken;
        Coap::Token                        token;
        Coap::Option::Iterator             iterator;
        uint8_t                            tokenLength;
        uint16_t                           length;
        Ip6::MessageInfo                   messageInfo;
        Coap::Message::UriPathStringBuffer uriBuffer;

        printf("TestCoapMessage()\n");

        instance = static_cast<Instance *>(testInitInstance());
        VerifyOrQuit(instance != nullptr);

        message = AsCoapMessagePtr(instance->Get<MessagePool>().Allocate(Message::kTypeOther));
        VerifyOrQuit(message != nullptr);

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        SuccessOrQuit(message->Init(Coap::kTypeNonConfirmable, Coap::kCodePut));

        SuccessOrQuit(message->ParseHeaderInfo(headerInfo));
        VerifyOrQuit(headerInfo.GetType() == Coap::kTypeNonConfirmable);
        VerifyOrQuit(headerInfo.GetCode() == Coap::kCodePut);
        VerifyOrQuit(headerInfo.GetMessageId() == 0);
        VerifyOrQuit(headerInfo.GetToken().GetLength() == 0);

        VerifyOrQuit(message->ReadType() == Coap::kTypeNonConfirmable);
        VerifyOrQuit(message->ReadCode() == Coap::kCodePut);
        VerifyOrQuit(message->ReadMessageId() == 0);

        SuccessOrQuit(message->ReadTokenLength(tokenLength));
        VerifyOrQuit(tokenLength == 0);

        SuccessOrQuit(message->ReadToken(readToken));
        VerifyOrQuit(readToken.GetLength() == 0);

        SuccessOrQuit(iterator.Init(*message));
        VerifyOrQuit(iterator.IsDone());
        VerifyOrQuit(!iterator.HasPayloadMarker());
        VerifyOrQuit(iterator.GetPayloadMessageOffset() == message->GetLength());

        {
            Coap::Msg msg(*message, messageInfo);

            SuccessOrQuit(msg.ParseHeaderAndOptions(Coap::Msg::kRemovePayloadMarkerIfNoPayload));

            VerifyOrQuit(msg.GetType() == Coap::kTypeNonConfirmable);
            VerifyOrQuit(msg.GetCode() == Coap::kCodePut);
            VerifyOrQuit(msg.GetMessageId() == 0);
            VerifyOrQuit(msg.GetToken().GetLength() == 0);

            VerifyOrQuit(msg.mMessage.GetOffset() == msg.mMessage.GetLength());
        }

        // AppendPayloadMaker

        SuccessOrQuit(message->AppendPayloadMarker());
        VerifyOrQuit(message->GetOffset() == message->GetLength());

        SuccessOrQuit(iterator.Init(*message));
        VerifyOrQuit(iterator.IsDone());
        VerifyOrQuit(iterator.HasPayloadMarker());
        VerifyOrQuit(iterator.GetPayloadMessageOffset() == message->GetLength());

        {
            Coap::Msg msg(*message, messageInfo);

            VerifyOrQuit(msg.ParseHeaderAndOptions(Coap::Msg::kRejectIfNoPayloadWithPayloadMarker) != kErrorNone);
        }

        // AppendPayloadMaker again should not make any changes

        length = message->GetLength();
        SuccessOrQuit(message->AppendPayloadMarker());
        VerifyOrQuit(message->GetLength() == length);
        VerifyOrQuit(message->GetOffset() == length);

        // Ensure payload marker is removed since we have no payload

        {
            Coap::Msg msg(*message, messageInfo);

            SuccessOrQuit(msg.ParseHeaderAndOptions(Coap::Msg::kRemovePayloadMarkerIfNoPayload));

            VerifyOrQuit(msg.GetType() == Coap::kTypeNonConfirmable);
            VerifyOrQuit(msg.GetCode() == Coap::kCodePut);
            VerifyOrQuit(msg.GetMessageId() == 0);
            VerifyOrQuit(msg.GetToken().GetLength() == 0);

            VerifyOrQuit(msg.mMessage.GetOffset() == msg.mMessage.GetLength());
        }

        SuccessOrQuit(iterator.Init(*message));
        VerifyOrQuit(iterator.IsDone());
        VerifyOrQuit(!iterator.HasPayloadMarker());
        VerifyOrQuit(iterator.GetPayloadMessageOffset() == message->GetLength());

        SuccessOrQuit(message->Append<uint8_t>(0xaa));
        VerifyOrQuit(iterator.Init(*message) != kErrorNone);

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        SuccessOrQuit(message->Init(Coap::kTypeConfirmable, Coap::kCodePost, 0x1234));

        SuccessOrQuit(message->ParseHeaderInfo(headerInfo));
        VerifyOrQuit(headerInfo.GetType() == Coap::kTypeConfirmable);
        VerifyOrQuit(headerInfo.GetCode() == Coap::kCodePost);
        VerifyOrQuit(headerInfo.GetMessageId() == 0x1234);
        VerifyOrQuit(headerInfo.GetToken().GetLength() == 0);

        VerifyOrQuit(message->ReadType() == Coap::kTypeConfirmable);
        VerifyOrQuit(message->ReadCode() == Coap::kCodePost);
        VerifyOrQuit(message->ReadMessageId() == 0x1234);

        SuccessOrQuit(message->ReadTokenLength(tokenLength));
        VerifyOrQuit(tokenLength == 0);

        SuccessOrQuit(message->ReadToken(readToken));
        VerifyOrQuit(readToken.GetLength() == 0);

        SuccessOrQuit(iterator.Init(*message));
        VerifyOrQuit(iterator.IsDone());
        VerifyOrQuit(!iterator.HasPayloadMarker());
        VerifyOrQuit(iterator.GetPayloadMessageOffset() == message->GetLength());

        {
            Coap::Msg msg(*message, messageInfo);

            SuccessOrQuit(msg.ParseHeaderAndOptions(Coap::Msg::kRemovePayloadMarkerIfNoPayload));

            VerifyOrQuit(msg.GetType() == Coap::kTypeConfirmable);
            VerifyOrQuit(msg.GetCode() == Coap::kCodePost);
            VerifyOrQuit(msg.GetMessageId() == 0x1234);
            VerifyOrQuit(msg.GetToken().GetLength() == 0);

            VerifyOrQuit(msg.mMessage.GetOffset() == msg.mMessage.GetLength());
        }

        // Write Token

        token.mLength = 2;
        token.m8[0]   = 0x11;
        token.m8[1]   = 0x22;

        SuccessOrQuit(message->WriteToken(token));

        SuccessOrQuit(message->ParseHeaderInfo(headerInfo));
        VerifyOrQuit(headerInfo.GetType() == Coap::kTypeConfirmable);
        VerifyOrQuit(headerInfo.GetCode() == Coap::kCodePost);
        VerifyOrQuit(headerInfo.GetMessageId() == 0x1234);
        VerifyOrQuit(headerInfo.GetToken().GetLength() == 2);

        SuccessOrQuit(message->ReadToken(readToken));
        VerifyOrQuit(readToken.GetLength() == 2);
        VerifyOrQuit(readToken == token);

        SuccessOrQuit(iterator.Init(*message));
        VerifyOrQuit(iterator.IsDone());
        VerifyOrQuit(!iterator.HasPayloadMarker());
        VerifyOrQuit(iterator.GetPayloadMessageOffset() == message->GetLength());

        {
            Coap::Msg msg(*message, messageInfo);

            SuccessOrQuit(msg.ParseHeaderAndOptions(Coap::Msg::kRemovePayloadMarkerIfNoPayload));

            VerifyOrQuit(msg.GetType() == Coap::kTypeConfirmable);
            VerifyOrQuit(msg.GetCode() == Coap::kCodePost);
            VerifyOrQuit(msg.GetMessageId() == 0x1234);
            VerifyOrQuit(msg.GetToken().GetLength() == 2);
            VerifyOrQuit(msg.GetToken() == token);

            VerifyOrQuit(msg.mMessage.GetOffset() == msg.mMessage.GetLength());
        }

        // Append URI-Path Option

        SuccessOrQuit(message->AppendUriPathOptions("uri"));

        SuccessOrQuit(message->ParseHeaderInfo(headerInfo));
        VerifyOrQuit(headerInfo.GetType() == Coap::kTypeConfirmable);
        VerifyOrQuit(headerInfo.GetCode() == Coap::kCodePost);
        VerifyOrQuit(headerInfo.GetMessageId() == 0x1234);
        VerifyOrQuit(headerInfo.GetToken().GetLength() == 2);

        SuccessOrQuit(iterator.Init(*message));

        VerifyOrQuit(!iterator.IsDone());
        VerifyOrQuit(iterator.GetOption() != nullptr);
        VerifyOrQuit(iterator.GetOption()->GetNumber() == Coap::kOptionUriPath);
        VerifyOrQuit(iterator.GetOption()->GetLength() == 3);

        SuccessOrQuit(iterator.Advance());
        VerifyOrQuit(iterator.IsDone());
        VerifyOrQuit(!iterator.HasPayloadMarker());
        VerifyOrQuit(iterator.GetPayloadMessageOffset() == message->GetLength());

        SuccessOrQuit(message->AppendPayloadMarker());

        SuccessOrQuit(iterator.Init(*message));
        VerifyOrQuit(!iterator.IsDone());
        SuccessOrQuit(iterator.Advance());
        VerifyOrQuit(iterator.IsDone());
        VerifyOrQuit(iterator.HasPayloadMarker());
        VerifyOrQuit(iterator.GetPayloadMessageOffset() == message->GetLength());

        // Append some payload after marker

        length = message->GetLength();
        SuccessOrQuit(message->Append<uint8_t>(0xef));

        SuccessOrQuit(iterator.Init(*message));
        VerifyOrQuit(!iterator.IsDone());
        SuccessOrQuit(iterator.Advance());
        VerifyOrQuit(iterator.IsDone());
        VerifyOrQuit(iterator.HasPayloadMarker());
        VerifyOrQuit(iterator.GetPayloadMessageOffset() == length);

        {
            Coap::Msg msg(*message, messageInfo);

            SuccessOrQuit(msg.ParseHeaderAndOptions(Coap::Msg::kRemovePayloadMarkerIfNoPayload));

            VerifyOrQuit(msg.GetType() == Coap::kTypeConfirmable);
            VerifyOrQuit(msg.GetCode() == Coap::kCodePost);
            VerifyOrQuit(msg.GetMessageId() == 0x1234);
            VerifyOrQuit(msg.GetToken().GetLength() == 2);
            VerifyOrQuit(msg.GetToken() == token);

            VerifyOrQuit(msg.mMessage.GetOffset() == length);
        }

        // Re-write Token

        token.mLength = 2;
        token.m8[0]   = 0x33;
        token.m8[1]   = 0x44;

        SuccessOrQuit(message->WriteToken(token));

        SuccessOrQuit(message->ParseHeaderInfo(headerInfo));
        VerifyOrQuit(headerInfo.GetType() == Coap::kTypeConfirmable);
        VerifyOrQuit(headerInfo.GetCode() == Coap::kCodePost);
        VerifyOrQuit(headerInfo.GetMessageId() == 0x1234);
        VerifyOrQuit(headerInfo.GetToken().GetLength() == 2);

        SuccessOrQuit(message->ReadToken(readToken));
        VerifyOrQuit(readToken.GetLength() == 2);
        VerifyOrQuit(readToken == token);

        {
            Coap::Msg msg(*message, messageInfo);

            SuccessOrQuit(msg.ParseHeaderAndOptions(Coap::Msg::kRemovePayloadMarkerIfNoPayload));

            VerifyOrQuit(msg.GetType() == Coap::kTypeConfirmable);
            VerifyOrQuit(msg.GetCode() == Coap::kCodePost);
            VerifyOrQuit(msg.GetMessageId() == 0x1234);
            VerifyOrQuit(msg.GetToken().GetLength() == 2);
            VerifyOrQuit(msg.GetToken() == token);

            VerifyOrQuit(msg.mMessage.GetOffset() == length);
        }

        // Re-write token - changing token length afterwards is not allowed

        token.mLength = 3;
        token.m8[2]   = 0x55;

        VerifyOrQuit(message->WriteToken(token) != kErrorNone);

        SuccessOrQuit(message->ParseHeaderInfo(headerInfo));
        VerifyOrQuit(headerInfo.GetToken().GetLength() == 2);

        token.mLength = 2;

        SuccessOrQuit(message->ReadToken(readToken));
        VerifyOrQuit(readToken.GetLength() == 2);
        VerifyOrQuit(readToken == token);

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        SuccessOrQuit(message->Init(Coap::kTypeConfirmable, Coap::kCodeGet, kUriCommissionerSet));

        SuccessOrQuit(message->ParseHeaderInfo(headerInfo));
        VerifyOrQuit(headerInfo.GetType() == Coap::kTypeConfirmable);
        VerifyOrQuit(headerInfo.GetCode() == Coap::kCodeGet);
        VerifyOrQuit(headerInfo.GetMessageId() == 0);
        VerifyOrQuit(headerInfo.GetToken().GetLength() == Coap::Token::kDefaultLength);

        VerifyOrQuit(message->ReadType() == Coap::kTypeConfirmable);
        VerifyOrQuit(message->ReadCode() == Coap::kCodeGet);
        VerifyOrQuit(message->ReadMessageId() == 0);

        SuccessOrQuit(message->ReadTokenLength(tokenLength));
        VerifyOrQuit(tokenLength == Coap::Token::kDefaultLength);

        SuccessOrQuit(iterator.Init(*message));

        VerifyOrQuit(!iterator.IsDone());
        VerifyOrQuit(iterator.GetOption() != nullptr);
        VerifyOrQuit(iterator.GetOption()->GetNumber() == Coap::kOptionUriPath);

        SuccessOrQuit(iterator.Advance());
        VerifyOrQuit(!iterator.IsDone());
        VerifyOrQuit(iterator.GetOption() != nullptr);
        VerifyOrQuit(iterator.GetOption()->GetNumber() == Coap::kOptionUriPath);

        SuccessOrQuit(iterator.Advance());
        VerifyOrQuit(iterator.IsDone());
        VerifyOrQuit(!iterator.HasPayloadMarker());
        VerifyOrQuit(iterator.GetPayloadMessageOffset() == message->GetLength());

        SuccessOrQuit(message->ReadUriPathOptions(uriBuffer));
        VerifyOrQuit(StringMatch(uriBuffer, PathForUri(kUriCommissionerSet)));

        {
            Coap::Msg msg(*message, messageInfo);

            SuccessOrQuit(msg.ParseHeaderAndOptions(Coap::Msg::kRemovePayloadMarkerIfNoPayload));

            VerifyOrQuit(msg.GetType() == Coap::kTypeConfirmable);
            VerifyOrQuit(msg.GetCode() == Coap::kCodeGet);
            VerifyOrQuit(msg.GetMessageId() == 0);
            VerifyOrQuit(msg.GetToken().GetLength() == 2);

            VerifyOrQuit(msg.mMessage.GetOffset() == msg.mMessage.GetLength());
        }

        // Re-write code, type, and message ID

        message->WriteType(Coap::kTypeNonConfirmable);
        message->WriteCode(Coap::kCodePost);
        message->WriteMessageId(0x9876);

        SuccessOrQuit(message->ParseHeaderInfo(headerInfo));
        VerifyOrQuit(headerInfo.GetType() == Coap::kTypeNonConfirmable);
        VerifyOrQuit(headerInfo.GetCode() == Coap::kCodePost);
        VerifyOrQuit(headerInfo.GetMessageId() == 0x9876);

        {
            Coap::Msg msg(*message, messageInfo);

            SuccessOrQuit(msg.ParseHeaderAndOptions(Coap::Msg::kRemovePayloadMarkerIfNoPayload));

            VerifyOrQuit(msg.GetType() == Coap::kTypeNonConfirmable);
            VerifyOrQuit(msg.GetCode() == Coap::kCodePost);
            VerifyOrQuit(msg.GetMessageId() == 0x9876);
            VerifyOrQuit(msg.GetToken().GetLength() == 2);

            VerifyOrQuit(msg.mMessage.GetOffset() == msg.mMessage.GetLength());

            // Msg::UpdateType()

            msg.UpdateType(Coap::kTypeConfirmable);

            VerifyOrQuit(msg.GetType() == Coap::kTypeConfirmable);
            VerifyOrQuit(msg.GetCode() == Coap::kCodePost);
            VerifyOrQuit(msg.GetMessageId() == 0x9876);

            VerifyOrQuit(message->ReadType() == Coap::kTypeConfirmable);

            // Msg::UpdateMessageId()

            msg.UpdateMessageId(0xabcd);

            VerifyOrQuit(msg.GetType() == Coap::kTypeConfirmable);
            VerifyOrQuit(msg.GetCode() == Coap::kCodePost);
            VerifyOrQuit(msg.GetMessageId() == 0xabcd);

            VerifyOrQuit(message->ReadMessageId() == 0xabcd);
        }

        message->Free();
        testFreeInstance(instance);
    }
};

} // namespace ot

int main(void)
{
    ot::UnitTester::TestCoapMessage();
    printf("All tests passed\n");
    return 0;
}
