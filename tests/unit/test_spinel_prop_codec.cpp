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

static void FakeDnssdBrowseCallback(otInstance *aInstance, const otPlatDnssdBrowseResult *aResult)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aResult);
}

static void FakeDnssdSrvCallback(otInstance *aInstance, const otPlatDnssdSrvResult *aResult)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aResult);
}

static constexpr uint16_t kMaxSpinelBufferSize = 2048;

void TestDnssd(void)
{
    uint8_t         buf[kMaxSpinelBufferSize];
    uint16_t        len;
    Spinel::Buffer  ncpBuffer(buf, kMaxSpinelBufferSize);
    Spinel::Encoder encoder(ncpBuffer);
    Spinel::Decoder decoder;
    uint8_t         header;
    unsigned int    command;
    unsigned int    propKey;
    otError         error = OT_ERROR_NONE;

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

void TestDnssdBrowser(void)
{
    uint8_t         buf[kMaxSpinelBufferSize];
    Spinel::Buffer  ncpBuffer(buf, kMaxSpinelBufferSize);
    uint16_t        len;
    Spinel::Encoder encoder(ncpBuffer);
    Spinel::Decoder decoder;
    uint8_t         header;
    unsigned int    command;
    unsigned int    propKey;
    const uint8_t  *callbackData;
    uint16_t        callbackDataLen;
    otError         error = OT_ERROR_NONE;

    otPlatDnssdBrowser dnssdBrowserEncode;
    otPlatDnssdBrowser dnssdBrowserDecode;

    dnssdBrowserEncode.mServiceType  = "_meshcop._udp";
    dnssdBrowserEncode.mSubTypeLabel = nullptr;
    dnssdBrowserEncode.mInfraIfIndex = 1;
    dnssdBrowserEncode.mCallback     = FakeDnssdBrowseCallback;

    ncpBuffer.Clear();
    SuccessOrQuit(
        error = encoder.BeginFrame(SPINEL_HEADER_FLAG, SPINEL_CMD_PROP_VALUE_INSERTED, SPINEL_PROP_DNSSD_BROWSER));
    SuccessOrQuit(error = EncodeDnssdDiscovery(encoder, dnssdBrowserEncode));
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
    VerifyOrQuit(static_cast<spinel_prop_key_t>(propKey) == SPINEL_PROP_DNSSD_BROWSER);
    SuccessOrQuit(error = DecodeDnssdBrowser(decoder, dnssdBrowserDecode, callbackData, callbackDataLen));
    VerifyOrQuit(strcmp(dnssdBrowserDecode.mServiceType, dnssdBrowserEncode.mServiceType) == 0);
    VerifyOrQuit(dnssdBrowserDecode.mSubTypeLabel == dnssdBrowserEncode.mSubTypeLabel);
    VerifyOrQuit(dnssdBrowserDecode.mInfraIfIndex == dnssdBrowserEncode.mInfraIfIndex);
    VerifyOrQuit(callbackDataLen == sizeof(otPlatDnssdBrowseCallback));
    VerifyOrQuit(*reinterpret_cast<const otPlatDnssdBrowseCallback *>(callbackData) == FakeDnssdBrowseCallback);
}

void TestDnssdBrowserResult(void)
{
    uint8_t         buf[kMaxSpinelBufferSize];
    Spinel::Buffer  ncpBuffer(buf, kMaxSpinelBufferSize);
    uint16_t        len;
    Spinel::Encoder encoder(ncpBuffer);
    Spinel::Decoder decoder;
    uint8_t         header;
    unsigned int    command;
    unsigned int    propKey;
    const uint8_t  *callbackData;
    uint16_t        callbackDataLen;
    otError         error = OT_ERROR_NONE;

    otPlatDnssdBrowseResult dnssdBrowseResultEncode;
    otPlatDnssdBrowseResult dnssdBrowseResultDecode;

    dnssdBrowseResultEncode.mServiceType     = "_ms._tcp";
    dnssdBrowseResultEncode.mSubTypeLabel    = "_nuclear";
    dnssdBrowseResultEncode.mServiceInstance = "ZGMF-X09A #1";
    dnssdBrowseResultEncode.mTtl             = 1999;
    dnssdBrowseResultEncode.mInfraIfIndex    = 1;

    otPlatDnssdBrowseCallback callback = &FakeDnssdBrowseCallback;

    ncpBuffer.Clear();
    SuccessOrQuit(
        error = encoder.BeginFrame(SPINEL_HEADER_FLAG, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_DNSSD_BROWSE_RESULT));
    SuccessOrQuit(error = EncodeDnssdBrowseResult(encoder, dnssdBrowseResultEncode,
                                                  reinterpret_cast<const uint8_t *>(&callback), sizeof(callback)));
    SuccessOrQuit(error = encoder.EndFrame());
    SuccessOrQuit(ncpBuffer.OutFrameBegin());
    len = ncpBuffer.OutFrameGetLength();
    VerifyOrQuit(ncpBuffer.OutFrameRead(len, buf) == len);

    decoder.Init(buf, len);
    SuccessOrQuit(error = decoder.ReadUint8(header));
    VerifyOrQuit(header == SPINEL_HEADER_FLAG);
    SuccessOrQuit(error = decoder.ReadUintPacked(command));
    VerifyOrQuit(command == SPINEL_CMD_PROP_VALUE_SET);
    SuccessOrQuit(error = decoder.ReadUintPacked(propKey));
    VerifyOrQuit(static_cast<spinel_prop_key_t>(propKey) == SPINEL_PROP_DNSSD_BROWSE_RESULT);
    SuccessOrQuit(error = DecodeDnssdBrowseResult(decoder, dnssdBrowseResultDecode, callbackData, callbackDataLen));
    VerifyOrQuit(strcmp(dnssdBrowseResultDecode.mServiceType, dnssdBrowseResultEncode.mServiceType) == 0);
    VerifyOrQuit(strcmp(dnssdBrowseResultDecode.mSubTypeLabel, dnssdBrowseResultEncode.mSubTypeLabel) == 0);
    VerifyOrQuit(strcmp(dnssdBrowseResultDecode.mServiceInstance, dnssdBrowseResultEncode.mServiceInstance) == 0);
    VerifyOrQuit(dnssdBrowseResultDecode.mInfraIfIndex == dnssdBrowseResultEncode.mInfraIfIndex);
    VerifyOrQuit(callbackDataLen == sizeof(otPlatDnssdBrowseCallback));
    VerifyOrQuit(*reinterpret_cast<const otPlatDnssdBrowseCallback *>(callbackData) == FakeDnssdBrowseCallback);
}

void TestDnssdSrvResolver(void)
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

    otError error = OT_ERROR_NONE;

    otPlatDnssdSrvResolver srvResolverEncode;
    otPlatDnssdSrvResolver srvResolverDecode;
    const uint8_t         *callbackData;
    uint16_t               callbackDataLen;

    srvResolverEncode.mServiceInstance = "ZGMF-X10A #1";
    srvResolverEncode.mServiceType     = "_ms._tcp";
    srvResolverEncode.mInfraIfIndex    = 1;
    srvResolverEncode.mCallback        = FakeDnssdSrvCallback;

    ncpBuffer.Clear();
    SuccessOrQuit(
        error = encoder.BeginFrame(SPINEL_HEADER_FLAG, SPINEL_CMD_PROP_VALUE_INSERTED, SPINEL_PROP_DNSSD_SRV_RESOLVER));
    SuccessOrQuit(error = EncodeDnssdDiscovery(encoder, srvResolverEncode));
    SuccessOrQuit(error = encoder.EndFrame());
    SuccessOrQuit(ncpBuffer.OutFrameBegin());
    len = ncpBuffer.OutFrameGetLength();
    VerifyOrQuit(ncpBuffer.OutFrameRead(len, buf) == len);

    decoder.Init(buf, len);
    SuccessOrQuit(error = decoder.ReadUint8(header));
    SuccessOrQuit(error = decoder.ReadUintPacked(command));
    SuccessOrQuit(error = decoder.ReadUintPacked(propKey));
    SuccessOrQuit(error = DecodeDnssdSrvResolver(decoder, srvResolverDecode, callbackData, callbackDataLen));
    VerifyOrQuit(strcmp(srvResolverDecode.mServiceInstance, srvResolverEncode.mServiceInstance) == 0);
    VerifyOrQuit(strcmp(srvResolverDecode.mServiceType, srvResolverEncode.mServiceType) == 0);
    VerifyOrQuit(srvResolverDecode.mInfraIfIndex == srvResolverEncode.mInfraIfIndex);
    VerifyOrQuit(callbackDataLen == sizeof(otPlatDnssdSrvCallback));
    VerifyOrQuit(*reinterpret_cast<const otPlatDnssdSrvCallback *>(callbackData) == FakeDnssdSrvCallback);
}

void TestDnssdSrvResult(void)
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

    otPlatDnssdSrvResult srvResultEncode;
    otPlatDnssdSrvResult srvResultDecode;
    const uint8_t       *callbackData;
    uint16_t             callbackDataLen;

    srvResultEncode.mServiceInstance = "ZGMF-X13A #1";
    srvResultEncode.mServiceType     = "_ms._tcp";
    srvResultEncode.mHostName        = "ZGMF-X13A #1._ms._tcp.local.";
    srvResultEncode.mPort            = 5353;
    srvResultEncode.mPriority        = 10;
    srvResultEncode.mWeight          = 100;
    srvResultEncode.mTtl             = 120;
    srvResultEncode.mInfraIfIndex    = 1;

    otPlatDnssdSrvCallback callback = FakeDnssdSrvCallback;

    ncpBuffer.Clear();
    SuccessOrQuit(error =
                      encoder.BeginFrame(SPINEL_HEADER_FLAG, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_DNSSD_SRV_RESULT));
    SuccessOrQuit(error = EncodeDnssdSrvResult(encoder, srvResultEncode, reinterpret_cast<const uint8_t *>(&callback),
                                               sizeof(callback)));
    SuccessOrQuit(error = encoder.EndFrame());
    SuccessOrQuit(ncpBuffer.OutFrameBegin());
    len = ncpBuffer.OutFrameGetLength();
    VerifyOrQuit(ncpBuffer.OutFrameRead(len, buf) == len);

    decoder.Init(buf, len);
    SuccessOrQuit(error = decoder.ReadUint8(header));
    SuccessOrQuit(error = decoder.ReadUintPacked(command));
    SuccessOrQuit(error = decoder.ReadUintPacked(propKey));
    SuccessOrQuit(error = DecodeDnssdSrvResult(decoder, srvResultDecode, callbackData, callbackDataLen));
    VerifyOrQuit(strcmp(srvResultDecode.mServiceInstance, srvResultEncode.mServiceInstance) == 0);
    VerifyOrQuit(strcmp(srvResultDecode.mServiceType, srvResultEncode.mServiceType) == 0);
    VerifyOrQuit(strcmp(srvResultDecode.mHostName, srvResultEncode.mHostName) == 0);
    VerifyOrQuit(srvResultDecode.mPort == srvResultEncode.mPort);
    VerifyOrQuit(srvResultDecode.mPriority == srvResultEncode.mPriority);
    VerifyOrQuit(srvResultDecode.mWeight == srvResultEncode.mWeight);
    VerifyOrQuit(srvResultDecode.mTtl == srvResultEncode.mTtl);
    VerifyOrQuit(srvResultDecode.mInfraIfIndex == srvResultEncode.mInfraIfIndex);
    VerifyOrQuit(callbackDataLen == sizeof(otPlatDnssdSrvCallback));
    VerifyOrQuit(*reinterpret_cast<const otPlatDnssdSrvCallback *>(callbackData) == FakeDnssdSrvCallback);
}

} // namespace Spinel
} // namespace ot

int main(void)
{
    ot::Spinel::TestDnssd();
    ot::Spinel::TestDnssdBrowser();
    ot::Spinel::TestDnssdBrowserResult();
    ot::Spinel::TestDnssdSrvResolver();
    ot::Spinel::TestDnssdSrvResult();
    printf("\nAll tests passed.\n");
    return 0;
}
