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

#include <stdio.h>

#include <openthread/platform/dnssd.h>

#include "test_platform.h"
#include "test_util.h"
#include "common/code_utils.hpp"
#include "lib/spinel/spinel_buffer.hpp"
#include "lib/spinel/spinel_encoder.hpp"
#include "ncp/ncp_base.hpp"

#if OPENTHREAD_CONFIG_NCP_DNSSD_ENABLE && OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE

namespace ot {

constexpr uint16_t kMaxSpinelBufferSize = 2048;
static uint32_t    sRequestId;
static uint8_t     sError;

static otError GenerateSpinelDnssdSetStateFrame(otPlatDnssdState aState, uint8_t *aBuf, uint16_t &aLen)
{
    otError         error = OT_ERROR_NONE;
    uint8_t         buf[kMaxSpinelBufferSize];
    Spinel::Buffer  ncpBuffer(buf, kMaxSpinelBufferSize);
    Spinel::Encoder encoder(ncpBuffer);

    uint8_t header = SPINEL_HEADER_FLAG | 0 /* Iid */ | 1 /* Tid */;
    SuccessOrExit(error = encoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_DNSSD_STATE));
    SuccessOrExit(error = encoder.WriteUint8(aState));
    SuccessOrExit(error = encoder.EndFrame());

    SuccessOrExit(ncpBuffer.OutFrameBegin());
    aLen = ncpBuffer.OutFrameGetLength();
    VerifyOrExit(ncpBuffer.OutFrameRead(aLen, aBuf) == aLen, error = OT_ERROR_FAILED);

exit:
    return error;
}

static void TestPlatDnssdRegisterCallback(otInstance *aInstance, otPlatDnssdRequestId aRequestId, otError aError)
{
    sRequestId = aRequestId;
    sError     = aError;
}

static otError GenerateSpinelDnssdRequestResultFrame(uint32_t aRequestId, otError aError, uint8_t *aBuf, uint16_t &aLen)
{
    otError                     error = OT_ERROR_NONE;
    uint8_t                     buf[kMaxSpinelBufferSize];
    Spinel::Buffer              ncpBuffer(buf, kMaxSpinelBufferSize);
    Spinel::Encoder             encoder(ncpBuffer);
    otPlatDnssdRegisterCallback callback = &TestPlatDnssdRegisterCallback;

    uint8_t header = SPINEL_HEADER_FLAG | 0 /* Iid */ | 1 /* Tid */;
    SuccessOrExit(error = encoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_DNSSD_REQUEST_RESULT));
    SuccessOrExit(error = encoder.WriteUint8(aError));
    SuccessOrExit(error = encoder.WriteUint32(aRequestId));
    SuccessOrExit(error = encoder.WriteData(reinterpret_cast<const uint8_t *>(&callback), sizeof(callback)));
    SuccessOrExit(error = encoder.EndFrame());

    SuccessOrExit(ncpBuffer.OutFrameBegin());
    aLen = ncpBuffer.OutFrameGetLength();
    VerifyOrExit(ncpBuffer.OutFrameRead(aLen, aBuf) == aLen, error = OT_ERROR_FAILED);

exit:
    return error;
}

void TestNcpDnssdGetState(void)
{
    Instance    *instance = static_cast<Instance *>(testInitInstance());
    Ncp::NcpBase ncpBase(instance);

    uint8_t          recvBuf[kMaxSpinelBufferSize];
    uint16_t         recvLen;
    otPlatDnssdState dnssdState;

    dnssdState = otPlatDnssdGetState(instance);
    VerifyOrQuit(dnssdState == OT_PLAT_DNSSD_STOPPED);

    SuccessOrQuit(GenerateSpinelDnssdSetStateFrame(OT_PLAT_DNSSD_READY, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    dnssdState = otPlatDnssdGetState(instance);
    VerifyOrQuit(dnssdState == OT_PLAT_DNSSD_READY);
}

void TestNcpDnssdRegistrations(void)
{
    Instance    *instance = static_cast<Instance *>(testInitInstance());
    Ncp::NcpBase ncpBase(instance);

    uint8_t            recvBuf[kMaxSpinelBufferSize];
    uint16_t           recvLen;
    otPlatDnssdHost    dnssdHost;
    otPlatDnssdService dnssdService;
    otPlatDnssdKey     dnssdKey;

    sRequestId = 0; // Use 0 to indicate `sRequestId` hasn't been set.

    // Register a dnssd host when the dnssd state is DISABLED
    dnssdHost.mHostName        = "ot-test";
    dnssdHost.mAddressesLength = 0;
    otPlatDnssdRegisterHost(instance, &dnssdHost, 1 /* aRequestId */, TestPlatDnssdRegisterCallback);
    VerifyOrQuit(sRequestId == 1);
    VerifyOrQuit(sError == OT_ERROR_INVALID_STATE);

    // Set the dnssd state to READY
    SuccessOrQuit(GenerateSpinelDnssdSetStateFrame(OT_PLAT_DNSSD_READY, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);

    otPlatDnssdUnregisterHost(instance, &dnssdHost, 2 /* aRequestId */, TestPlatDnssdRegisterCallback);
    SuccessOrQuit(GenerateSpinelDnssdRequestResultFrame(2 /* aRequestId */, OT_ERROR_NOT_FOUND, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    VerifyOrQuit(sRequestId == 2);
    VerifyOrQuit(sError == OT_ERROR_NOT_FOUND);

    dnssdService.mHostName            = "test-service";
    dnssdService.mServiceInstance     = nullptr;
    dnssdService.mServiceType         = nullptr;
    dnssdService.mSubTypeLabelsLength = 0;
    dnssdService.mPort                = 1234;
    dnssdService.mTxtDataLength       = 0;
    otPlatDnssdRegisterService(instance, &dnssdService, 3 /* aRequestId */, TestPlatDnssdRegisterCallback);
    SuccessOrQuit(GenerateSpinelDnssdRequestResultFrame(3 /* aRequestId */, OT_ERROR_NONE, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    VerifyOrQuit(sRequestId == 3);
    VerifyOrQuit(sError == OT_ERROR_NONE);

    sRequestId = 0;
    sError     = OT_ERROR_FAILED;
    otPlatDnssdUnregisterService(instance, &dnssdService, 3 /* aRequestId */, TestPlatDnssdRegisterCallback);
    ncpBase.HandleReceive(recvBuf, recvLen);
    VerifyOrQuit(sRequestId == 3);
    VerifyOrQuit(sError == OT_ERROR_NONE);

    dnssdKey.mName          = "test-key";
    dnssdKey.mServiceType   = "someType";
    dnssdKey.mKeyDataLength = 0;
    otPlatDnssdRegisterKey(instance, &dnssdKey, 4 /* aRequestId */, TestPlatDnssdRegisterCallback);
    SuccessOrQuit(GenerateSpinelDnssdRequestResultFrame(4 /* aRequestId */, OT_ERROR_NONE, recvBuf, recvLen));
    ncpBase.HandleReceive(recvBuf, recvLen);
    VerifyOrQuit(sRequestId == 4);
    VerifyOrQuit(sError == OT_ERROR_NONE);

    sRequestId = 0;
    sError     = OT_ERROR_FAILED;
    otPlatDnssdUnregisterKey(instance, &dnssdKey, 4 /* aRequestId */, TestPlatDnssdRegisterCallback);
    ncpBase.HandleReceive(recvBuf, recvLen);
    VerifyOrQuit(sRequestId == 4);
    VerifyOrQuit(sError == OT_ERROR_NONE);
}

static bool sDnssdBrowseCallbackInvoked = false;

static void TestDnssdBrowseCallback(otInstance *aInstance, const otPlatDnssdBrowseResult *aResult)
{
    VerifyOrQuit(strcmp(aResult->mServiceType, "_ms._tcp") == 0);
    VerifyOrQuit(strcmp(aResult->mSubTypeLabel, "_battery") == 0);
    VerifyOrQuit(strcmp(aResult->mServiceInstance, "GAT-X105 #1") == 0);
    VerifyOrQuit(aResult->mTtl == 12345);
    VerifyOrQuit(aResult->mInfraIfIndex == 2);

    sDnssdBrowseCallbackInvoked = true;
}

static otError GenerateSpinelDnssdBrowseResultFrame(const otPlatDnssdBrowseResult &aBrowseResult,
                                                    uint8_t                       *aBuf,
                                                    uint16_t                      &aLen)
{
    otError                   error = OT_ERROR_NONE;
    uint8_t                   buf[kMaxSpinelBufferSize];
    Spinel::Buffer            ncpBuffer(buf, kMaxSpinelBufferSize);
    Spinel::Encoder           encoder(ncpBuffer);
    otPlatDnssdBrowseCallback callback = &TestDnssdBrowseCallback;

    uint8_t header = SPINEL_HEADER_FLAG | 0 /* Iid */ | 1 /* Tid */;
    SuccessOrExit(error = encoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_DNSSD_BROWSE_RESULT));
    SuccessOrExit(error = EncodeDnssdBrowseResult(encoder, aBrowseResult, reinterpret_cast<const uint8_t *>(&callback),
                                                  sizeof(callback)));
    SuccessOrExit(error = encoder.EndFrame());

    SuccessOrExit(ncpBuffer.OutFrameBegin());
    aLen = ncpBuffer.OutFrameGetLength();
    VerifyOrExit(ncpBuffer.OutFrameRead(aLen, aBuf) == aLen, error = OT_ERROR_FAILED);

exit:
    return error;
}

void TestNcpDnssdBrowse(void)
{
    Instance               *instance = static_cast<Instance *>(testInitInstance());
    Ncp::NcpBase            ncpBase(instance);
    uint8_t                 recvBuf[kMaxSpinelBufferSize];
    uint16_t                recvLen;
    otPlatDnssdBrowser      browser;
    otPlatDnssdBrowseResult browseResult;

    browser.mServiceType  = "_ms._tcp";
    browser.mSubTypeLabel = "_battery";
    browser.mInfraIfIndex = 2;
    browser.mCallback     = TestDnssdBrowseCallback;

    otPlatDnssdStartBrowser(instance, &browser);

    browseResult.mServiceType     = "_ms._tcp";
    browseResult.mSubTypeLabel    = "_battery";
    browseResult.mServiceInstance = "GAT-X105 #1";
    browseResult.mTtl             = 12345;
    browseResult.mInfraIfIndex    = 2;

    SuccessOrQuit(GenerateSpinelDnssdBrowseResultFrame(browseResult, recvBuf, recvLen));

    ncpBase.HandleReceive(recvBuf, recvLen);

    VerifyOrQuit(sDnssdBrowseCallbackInvoked);
}

static bool sDnssdSrvCallbackInvoked = false;

static void TestDnssdSrvCallback(otInstance *aInstance, const otPlatDnssdSrvResult *aResult)
{
    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrQuit(strcmp(aResult->mServiceInstance, "GAT-X303 #1") == 0);
    VerifyOrQuit(strcmp(aResult->mServiceType, "_ms._tcp") == 0);
    VerifyOrQuit(strcmp(aResult->mHostName, "GAT-X303 #1._ms._tcp.local") == 0);
    VerifyOrQuit(aResult->mPort == 5353);
    VerifyOrQuit(aResult->mPriority == 1);
    VerifyOrQuit(aResult->mWeight == 10);
    VerifyOrQuit(aResult->mTtl == 120);
    VerifyOrQuit(aResult->mInfraIfIndex == 1);

    sDnssdSrvCallbackInvoked = true;
}

static otError GenerateSpinelDnssdSrvResultFrame(const otPlatDnssdSrvResult &aSrvResult, uint8_t *aBuf, uint16_t &aLen)
{
    otError                error = OT_ERROR_NONE;
    uint8_t                buf[kMaxSpinelBufferSize];
    Spinel::Buffer         ncpBuffer(buf, kMaxSpinelBufferSize);
    Spinel::Encoder        encoder(ncpBuffer);
    otPlatDnssdSrvCallback callback = &TestDnssdSrvCallback;

    uint8_t header = SPINEL_HEADER_FLAG | 0 /* Iid */ | 1 /* Tid */;
    SuccessOrExit(error = encoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_DNSSD_SRV_RESULT));
    SuccessOrExit(error = EncodeDnssdSrvResult(encoder, aSrvResult, reinterpret_cast<const uint8_t *>(&callback),
                                               sizeof(callback)));
    SuccessOrExit(error = encoder.EndFrame());

    SuccessOrExit(ncpBuffer.OutFrameBegin());
    aLen = ncpBuffer.OutFrameGetLength();
    VerifyOrExit(ncpBuffer.OutFrameRead(aLen, aBuf) == aLen, error = OT_ERROR_FAILED);

exit:
    return error;
}

void TestNcpDnssdSrvResolve(void)
{
    Instance              *instance = static_cast<Instance *>(testInitInstance());
    Ncp::NcpBase           ncpBase(instance);
    uint8_t                recvBuf[kMaxSpinelBufferSize];
    uint16_t               recvLen;
    otPlatDnssdSrvResolver resolver;
    otPlatDnssdSrvResult   srvResult;

    resolver.mServiceInstance = "GAT-X303 #1";
    resolver.mServiceType     = "_ms._tcp";
    resolver.mInfraIfIndex    = 1;
    resolver.mCallback        = TestDnssdSrvCallback;

    otPlatDnssdStartSrvResolver(instance, &resolver);

    srvResult.mServiceInstance = "GAT-X303 #1";
    srvResult.mServiceType     = "_ms._tcp";
    srvResult.mHostName        = "GAT-X303 #1._ms._tcp.local";
    srvResult.mPort            = 5353;
    srvResult.mPriority        = 1;
    srvResult.mWeight          = 10;
    srvResult.mTtl             = 120;
    srvResult.mInfraIfIndex    = 1;

    SuccessOrQuit(GenerateSpinelDnssdSrvResultFrame(srvResult, recvBuf, recvLen));

    ncpBase.HandleReceive(recvBuf, recvLen);

    VerifyOrQuit(sDnssdSrvCallbackInvoked);
}

static bool sDnssdTxtCallbackInvoked = false;

static const uint8_t kTestTxtData[] = {0x01, 0x02, 0x03};

static void TestDnssdTxtCallback(otInstance *aInstance, const otPlatDnssdTxtResult *aResult)
{
    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrQuit(strcmp(aResult->mServiceInstance, "GAT-X303 #1") == 0);
    VerifyOrQuit(strcmp(aResult->mServiceType, "_ms._tcp") == 0);
    VerifyOrQuit(aResult->mTxtDataLength == sizeof(kTestTxtData));
    VerifyOrQuit(aResult->mTxtData != nullptr);
    VerifyOrQuit(memcmp(aResult->mTxtData, kTestTxtData, sizeof(kTestTxtData)) == 0);
    VerifyOrQuit(aResult->mTtl == 90);
    VerifyOrQuit(aResult->mInfraIfIndex == 1);

    sDnssdTxtCallbackInvoked = true;
}

static otError GenerateSpinelDnssdTxtResultFrame(const otPlatDnssdTxtResult &aTxtResult, uint8_t *aBuf, uint16_t &aLen)
{
    otError                error = OT_ERROR_NONE;
    uint8_t                buf[kMaxSpinelBufferSize];
    Spinel::Buffer         ncpBuffer(buf, kMaxSpinelBufferSize);
    Spinel::Encoder        encoder(ncpBuffer);
    otPlatDnssdTxtCallback callback = &TestDnssdTxtCallback;

    uint8_t header = SPINEL_HEADER_FLAG | 0 /* Iid */ | 1 /* Tid */;
    SuccessOrExit(error = encoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_DNSSD_TXT_RESULT));
    SuccessOrExit(error = EncodeDnssdTxtResult(encoder, aTxtResult, reinterpret_cast<const uint8_t *>(&callback),
                                               sizeof(callback)));
    SuccessOrExit(error = encoder.EndFrame());

    SuccessOrExit(ncpBuffer.OutFrameBegin());
    aLen = ncpBuffer.OutFrameGetLength();
    VerifyOrExit(ncpBuffer.OutFrameRead(aLen, aBuf) == aLen, error = OT_ERROR_FAILED);

exit:
    return error;
}

void TestNcpDnssdTxtResolve(void)
{
    Instance              *instance = static_cast<Instance *>(testInitInstance());
    Ncp::NcpBase           ncpBase(instance);
    uint8_t                recvBuf[kMaxSpinelBufferSize];
    uint16_t               recvLen;
    otPlatDnssdTxtResolver resolver;
    otPlatDnssdTxtResult   txtResult;

    resolver.mServiceInstance = "GAT-X303 #1";
    resolver.mServiceType     = "_ms._tcp";
    resolver.mInfraIfIndex    = 1;
    resolver.mCallback        = TestDnssdTxtCallback;

    otPlatDnssdStartTxtResolver(instance, &resolver);

    txtResult.mServiceInstance = "GAT-X303 #1";
    txtResult.mServiceType     = "_ms._tcp";
    txtResult.mTxtData         = kTestTxtData;
    txtResult.mTxtDataLength   = sizeof(kTestTxtData);
    txtResult.mTtl             = 90;
    txtResult.mInfraIfIndex    = 1;

    SuccessOrQuit(GenerateSpinelDnssdTxtResultFrame(txtResult, recvBuf, recvLen));

    ncpBase.HandleReceive(recvBuf, recvLen);

    VerifyOrQuit(sDnssdTxtCallbackInvoked);
}

static bool sDnssdAddressCallbackInvoked = false;

static void TestDnssdAddressCallback(otInstance *aInstance, const otPlatDnssdAddressResult *aResult)
{
    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrQuit(strcmp(aResult->mHostName, "my-host") == 0);
    VerifyOrQuit(aResult->mInfraIfIndex == 3);
    VerifyOrQuit(aResult->mAddressesLength == 2);
    VerifyOrQuit(aResult->mAddresses != nullptr);
    VerifyOrQuit(aResult->mAddresses[0].mTtl == 60);
    VerifyOrQuit(aResult->mAddresses[1].mTtl == 120);
    VerifyOrQuit(aResult->mAddresses[0].mAddress.mFields.m8[0] == 0x11);
    VerifyOrQuit(aResult->mAddresses[1].mAddress.mFields.m8[0] == 0x22);

    sDnssdAddressCallbackInvoked = true;
}

static otError GenerateSpinelDnssdAddressResultFrame(const otPlatDnssdAddressResult &aAddressResult,
                                                     uint8_t                        *aBuf,
                                                     uint16_t                       &aLen)
{
    otError                    error = OT_ERROR_NONE;
    uint8_t                    buf[kMaxSpinelBufferSize];
    Spinel::Buffer             ncpBuffer(buf, kMaxSpinelBufferSize);
    Spinel::Encoder            encoder(ncpBuffer);
    otPlatDnssdAddressCallback callback = &TestDnssdAddressCallback;

    uint8_t header = SPINEL_HEADER_FLAG | 0 /* Iid */ | 1 /* Tid */;
    SuccessOrExit(error = encoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_DNSSD_IP6_ADDRESS_RESULT));
    SuccessOrExit(error = EncodeDnssdAddressResult(encoder, aAddressResult,
                                                   reinterpret_cast<const uint8_t *>(&callback), sizeof(callback)));
    SuccessOrExit(error = encoder.EndFrame());

    SuccessOrExit(ncpBuffer.OutFrameBegin());
    aLen = ncpBuffer.OutFrameGetLength();
    VerifyOrExit(ncpBuffer.OutFrameRead(aLen, aBuf) == aLen, error = OT_ERROR_FAILED);

exit:
    return error;
}

void TestNcpDnssdAddressResolve(void)
{
    Instance                  *instance = static_cast<Instance *>(testInitInstance());
    Ncp::NcpBase               ncpBase(instance);
    uint8_t                    recvBuf[kMaxSpinelBufferSize];
    uint16_t                   recvLen;
    otPlatDnssdAddressResolver resolver;
    otPlatDnssdAddressResult   addrResult;
    otPlatDnssdAddressAndTtl   addrArray[2];

    resolver.mHostName     = "my-host";
    resolver.mInfraIfIndex = 3;
    resolver.mCallback     = TestDnssdAddressCallback;

    otPlatDnssdStartIp6AddressResolver(instance, &resolver);

    memset(&addrArray[0].mAddress, 0x11, sizeof(addrArray[0].mAddress));
    addrArray[0].mTtl = 60;
    memset(&addrArray[1].mAddress, 0x22, sizeof(addrArray[1].mAddress));
    addrArray[1].mTtl = 120;

    addrResult.mHostName        = "my-host";
    addrResult.mInfraIfIndex    = 3;
    addrResult.mAddresses       = addrArray;
    addrResult.mAddressesLength = 2;

    SuccessOrQuit(GenerateSpinelDnssdAddressResultFrame(addrResult, recvBuf, recvLen));

    ncpBase.HandleReceive(recvBuf, recvLen);

    VerifyOrQuit(sDnssdAddressCallbackInvoked);
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_NCP_DNSSD_ENABLE && OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_NCP_DNSSD_ENABLE && OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE
    ot::TestNcpDnssdGetState();
    ot::TestNcpDnssdRegistrations();
    ot::TestNcpDnssdBrowse();
    ot::TestNcpDnssdSrvResolve();
    ot::TestNcpDnssdTxtResolve();
    ot::TestNcpDnssdAddressResolve();
#endif
    printf("All tests passed\n");
    return 0;
}
