/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include <string.h>

#include "test_util.hpp"
#include "lib/spinel/spinel_prop_codec.hpp"

namespace ot {
namespace Spinel {

static void DnssdFakeCallback(otInstance *aInstance, otPlatDnssdRequestId aRequestId, otError aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aRequestId);
    OT_UNUSED_VARIABLE(aError);
}

void TestDnssd(void)
{
    constexpr uint16_t kMaxSpinelBufferSize = 2048;
    uint8_t            buf[kMaxSpinelBufferSize];
    uint16_t           len;
    Spinel::Buffer     ncpBuffer(buf, kMaxSpinelBufferSize);
    Spinel::Encoder    encoder(ncpBuffer);
    Spinel::Decoder    decoder;
    uint8_t            header;
    unsigned int       command;
    unsigned int       propKey;
    otError            error = OT_ERROR_NONE;

    // Test DnssdHost encoding and decoding
    otPlatDnssdHost dnssdHostEncode;
    otPlatDnssdHost dnssdHostDecode;
    otIp6Address    dnssdHostAddrs[] = {
           {0xfd, 0x2a, 0xc3, 0x0c, 0x87, 0xd3, 0x00, 0x01, 0xed, 0x1c, 0x0c, 0x91, 0xcc, 0xb6, 0x57, 0x8b},
    };
    otPlatDnssdRequestId requestId;
    const uint8_t       *callbackData;
    uint16_t             callbackDataLen;
    dnssdHostEncode.mHostName        = "ot-host1";
    dnssdHostEncode.mAddresses       = dnssdHostAddrs;
    dnssdHostEncode.mAddressesLength = 1;

    SuccessOrQuit(error = encoder.BeginFrame(SPINEL_HEADER_FLAG, SPINEL_CMD_PROP_VALUE_INSERTED));
    SuccessOrQuit(error = EncodeDnssd(encoder, dnssdHostEncode, 1 /* aRequestId */, DnssdFakeCallback));
    SuccessOrQuit(error = encoder.EndFrame());
    SuccessOrQuit(ncpBuffer.OutFrameBegin());
    len = ncpBuffer.OutFrameGetLength();
    VerifyOrQuit(ncpBuffer.OutFrameRead(len, buf) == len);

    decoder.Init(buf, len);
    SuccessOrQuit(error = decoder.ReadUint8(header));
    VerifyOrQuit(header == SPINEL_HEADER_FLAG);
    SuccessOrQuit(error = decoder.ReadUintPacked(command));
    VerifyOrQuit(command == SPINEL_CMD_PROP_VALUE_INSERTED);
    SuccessOrQuit(error = decoder.ReadUintPacked(propKey));
    VerifyOrQuit(static_cast<spinel_prop_key_t>(propKey) == SPINEL_PROP_DNSSD_HOST);
    SuccessOrQuit(error = DecodeDnssdHost(decoder, dnssdHostDecode, requestId, callbackData, callbackDataLen));
    VerifyOrQuit(strcmp(dnssdHostDecode.mHostName, dnssdHostEncode.mHostName) == 0);
    VerifyOrQuit(dnssdHostDecode.mAddressesLength == dnssdHostEncode.mAddressesLength);
    VerifyOrQuit(memcmp(dnssdHostDecode.mAddresses, dnssdHostEncode.mAddresses,
                        dnssdHostDecode.mAddressesLength * sizeof(otIp6Address)) == 0);
    VerifyOrQuit(requestId == 1);
    VerifyOrQuit(callbackDataLen == sizeof(otPlatDnssdRegisterCallback));
    VerifyOrQuit(*reinterpret_cast<const otPlatDnssdRegisterCallback *>(callbackData) == DnssdFakeCallback);

    // Test DnssdService encoding and decoding
    otPlatDnssdService dnssdServiceEncode;
    otPlatDnssdService dnssdServiceDecode;
    const char        *dnssdSubType[]       = {"cat", "dog", "fish"};
    const uint8_t      txtData[]            = {0x01, 0x02, 0x03, 0x04};
    dnssdServiceEncode.mHostName            = "ot-host2";
    dnssdServiceEncode.mServiceInstance     = "ot-service";
    dnssdServiceEncode.mServiceType         = "";
    dnssdServiceEncode.mSubTypeLabels       = dnssdSubType;
    dnssdServiceEncode.mSubTypeLabelsLength = sizeof(dnssdSubType) / sizeof(dnssdSubType[0]);
    dnssdServiceEncode.mTxtData             = txtData;
    dnssdServiceEncode.mTxtDataLength       = sizeof(txtData);
    dnssdServiceEncode.mPort                = 1234;
    dnssdServiceEncode.mPriority            = 567;
    dnssdServiceEncode.mWeight              = 890;
    dnssdServiceEncode.mTtl                 = 9999;

    ncpBuffer.Clear();
    SuccessOrQuit(error = encoder.BeginFrame(SPINEL_HEADER_FLAG, SPINEL_CMD_PROP_VALUE_INSERTED));
    SuccessOrQuit(error = EncodeDnssd(encoder, dnssdServiceEncode, 2 /* aRequestId */, DnssdFakeCallback));
    SuccessOrQuit(error = encoder.EndFrame());
    SuccessOrQuit(ncpBuffer.OutFrameBegin());
    len = ncpBuffer.OutFrameGetLength();
    VerifyOrQuit(ncpBuffer.OutFrameRead(len, buf) == len);

    decoder.Init(buf, len);
    SuccessOrQuit(error = decoder.ReadUint8(header));
    VerifyOrQuit(header == SPINEL_HEADER_FLAG);
    SuccessOrQuit(error = decoder.ReadUintPacked(command));
    VerifyOrQuit(command == SPINEL_CMD_PROP_VALUE_INSERTED);
    SuccessOrQuit(error = decoder.ReadUintPacked(propKey));
    VerifyOrQuit(static_cast<spinel_prop_key_t>(propKey) == SPINEL_PROP_DNSSD_SERVICE);
    const char *dnssdSubTypeDecode[] = {nullptr, nullptr, nullptr};
    uint16_t    dnssdSubTypeCount    = sizeof(dnssdSubTypeDecode) / sizeof(dnssdSubTypeDecode[0]);
    SuccessOrQuit(error = DecodeDnssdService(decoder, dnssdServiceDecode, dnssdSubTypeDecode, dnssdSubTypeCount,
                                             requestId, callbackData, callbackDataLen));
    VerifyOrQuit(strcmp(dnssdServiceDecode.mHostName, dnssdServiceEncode.mHostName) == 0);
    VerifyOrQuit(strcmp(dnssdServiceDecode.mServiceInstance, dnssdServiceEncode.mServiceInstance) == 0);
    VerifyOrQuit(strcmp(dnssdServiceDecode.mServiceType, dnssdServiceEncode.mServiceType) == 0);
    VerifyOrQuit(dnssdServiceDecode.mPort == dnssdServiceEncode.mPort);
    VerifyOrQuit(dnssdServiceDecode.mPriority == dnssdServiceEncode.mPriority);
    VerifyOrQuit(dnssdServiceDecode.mWeight == dnssdServiceEncode.mWeight);
    VerifyOrQuit(dnssdServiceDecode.mTtl == dnssdServiceEncode.mTtl);
    VerifyOrQuit(dnssdSubTypeCount == sizeof(dnssdSubType) / sizeof(dnssdSubType[0]));
    for (uint16_t i = 0; i < dnssdSubTypeCount; i++)
    {
        VerifyOrQuit(strcmp(dnssdSubTypeDecode[i], dnssdSubType[i]) == 0);
    }
    VerifyOrQuit(requestId == 2);
    VerifyOrQuit(callbackDataLen == sizeof(otPlatDnssdRegisterCallback));
    VerifyOrQuit(*reinterpret_cast<const otPlatDnssdRegisterCallback *>(callbackData) == DnssdFakeCallback);

    // Test DnssdKey encoding and decoding
    otPlatDnssdKey dnssdKeyEncode;
    otPlatDnssdKey dnssdKeyDecode;
    const uint8_t  keyData[]      = {0x05, 0x06, 0x07, 0x08};
    dnssdKeyEncode.mName          = "ot-key";
    dnssdKeyEncode.mServiceType   = nullptr;
    dnssdKeyEncode.mKeyData       = keyData;
    dnssdKeyEncode.mKeyDataLength = sizeof(keyData);
    dnssdKeyEncode.mClass         = 123;
    dnssdKeyEncode.mTtl           = 888;

    ncpBuffer.Clear();
    SuccessOrQuit(error = encoder.BeginFrame(SPINEL_HEADER_FLAG, SPINEL_CMD_PROP_VALUE_INSERTED));
    SuccessOrQuit(error = EncodeDnssd(encoder, dnssdKeyEncode, 3 /* aRequestId */, DnssdFakeCallback));
    SuccessOrQuit(error = encoder.EndFrame());
    SuccessOrQuit(ncpBuffer.OutFrameBegin());
    len = ncpBuffer.OutFrameGetLength();
    VerifyOrQuit(ncpBuffer.OutFrameRead(len, buf) == len);

    decoder.Init(buf, len);
    SuccessOrQuit(error = decoder.ReadUint8(header));
    VerifyOrQuit(header == SPINEL_HEADER_FLAG);
    SuccessOrQuit(error = decoder.ReadUintPacked(command));
    VerifyOrQuit(command == SPINEL_CMD_PROP_VALUE_INSERTED);
    SuccessOrQuit(error = decoder.ReadUintPacked(propKey));
    VerifyOrQuit(static_cast<spinel_prop_key_t>(propKey) == SPINEL_PROP_DNSSD_KEY_RECORD);
    SuccessOrQuit(error = DecodeDnssdKey(decoder, dnssdKeyDecode, requestId, callbackData, callbackDataLen));
    VerifyOrQuit(strcmp(dnssdKeyDecode.mName, dnssdKeyEncode.mName) == 0);
    VerifyOrQuit(dnssdKeyDecode.mServiceType == dnssdKeyEncode.mServiceType);
    VerifyOrQuit(dnssdKeyDecode.mKeyDataLength == dnssdKeyEncode.mKeyDataLength);
    VerifyOrQuit(memcmp(dnssdKeyDecode.mKeyData, dnssdKeyEncode.mKeyData, dnssdKeyDecode.mKeyDataLength) == 0);
    VerifyOrQuit(dnssdKeyDecode.mClass == dnssdKeyEncode.mClass);
    VerifyOrQuit(dnssdKeyDecode.mTtl == dnssdKeyEncode.mTtl);
    VerifyOrQuit(requestId == 3);
    VerifyOrQuit(callbackDataLen == sizeof(otPlatDnssdRegisterCallback));
    VerifyOrQuit(*reinterpret_cast<const otPlatDnssdRegisterCallback *>(callbackData) == DnssdFakeCallback);
}

} // namespace Spinel
} // namespace ot

int main(void)
{
    ot::Spinel::TestDnssd();
    printf("\nAll tests passed.\n");
    return 0;
}
