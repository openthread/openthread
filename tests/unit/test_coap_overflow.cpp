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

#include "test_platform.h"
#include "test_util.hpp"
#include "instance/instance.hpp"

namespace ot {

void TestCoapOverflow(void)
{
    Instance      *instance;
    Coap::Message *message;
    uint8_t        payload[] = {0x0e, 0xff, 0x18}; // Delta 0, Length 14 (extended 2 bytes), 0xff18
    // value16 = 0xff18 = 65304.
    // aValue = 65304 + 269 = 65573.
    // Truncated to uint16_t: 37.

    printf("TestCoapOverflow()\n");

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr);

    message = AsCoapMessagePtr(instance->Get<MessagePool>().Allocate(Message::kTypeOther));
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(message->Init(Coap::kTypeNonConfirmable, Coap::kCodePut));

    // Add some padding bytes to allow the "37" bytes read to not fail due to message end.
    uint8_t padding[100];
    memset(padding, 0, sizeof(padding));
    SuccessOrQuit(message->AppendBytes(payload, sizeof(payload)));
    SuccessOrQuit(message->AppendBytes(padding, sizeof(padding)));

    Coap::Option::Iterator iterator;
    Error                  error = iterator.Init(*message);

    // After fix, this should return kErrorParse.
    VerifyOrQuit(error == kErrorParse, "Fix failed: iterator.Init() did not return kErrorParse for truncated length!");

    message->Free();
    testFreeInstance(instance);
}

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
void TestReadBlockOptionValuesInvalidLength(void)
{
    Instance      *instance;
    Coap::Message *message;

    // Header for Block2 (23) with an invalid length of 6.
    // A well-formed Block Option's value is at most 3 bytes long.
    // A malformed message could have a longer length. The check in
    // `ReadBlockOptionValues` should reject this before attempting to
    // copy the value into a small local buffer, preventing an overflow.
    //
    // Delta 23 = 13 + 10.
    // Byte 0: 0xd6 (Delta 13, Length 6)
    // Byte 1: 0x0a (Delta extension)
    // Bytes 2-7: Option value (6 bytes)

    uint8_t payload[] = {0xd6, 0x0a, 0, 0, 0, 0, 0, 0};
    uint8_t padding[100];
    memset(padding, 0xaa, sizeof(padding)); // Fill with 0xaa to see it in overflow

    printf("TestReadBlockOptionValuesInvalidLength()\n");

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr);

    message = AsCoapMessagePtr(instance->Get<MessagePool>().Allocate(Message::kTypeOther));
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(message->Init(Coap::kTypeNonConfirmable, Coap::kCodePut));
    SuccessOrQuit(message->AppendBytes(payload, sizeof(payload)));
    SuccessOrQuit(message->AppendBytes(padding, sizeof(padding)));

    Coap::BlockInfo blockInfo;
    Error           error;
    // After fix, this should return kErrorParse and NOT crash.
    error = message->ReadBlockOptionValues(Coap::kOptionBlock2, blockInfo);
    VerifyOrQuit(error == kErrorParse,
                 "Fix failed: ReadBlockOptionValues did not return kErrorParse for oversized option!");

    message->Free();

    message = AsCoapMessagePtr(instance->Get<MessagePool>().Allocate(Message::kTypeOther));
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->Init(Coap::kTypeNonConfirmable, Coap::kCodePut));

    error = message->ReadBlockOptionValues(Coap::kOptionBlock2, blockInfo);
    VerifyOrQuit(error == kErrorNotFound, "ReadBlockOptionValues did not return kErrorNotFound for missing option!");

    message->Free();
    testFreeInstance(instance);
}
#endif

} // namespace ot

int main(void)
{
    ot::TestCoapOverflow();
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    ot::TestReadBlockOptionValuesInvalidLength();
#endif
    printf("All tests passed\n");
    return 0;
}
