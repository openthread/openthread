/*
 *  Copyright (c) 2023, The OpenThread Authors.
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

#include "openthread-core-config.h"

#include "cli/cli_utils.hpp"

#include "cli/cli_tcat.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/error.hpp"
#include "common/string.hpp"

#include <openthread/ble_secure.h>

#include <mbedtls/oid.h>
#include <openthread/error.h>
#include <openthread/tcat.h>
#include <openthread/platform/ble.h>

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE && OPENTHREAD_CONFIG_CLI_BLE_SECURE_ENABLE

#define CERT_SET_COUNT 2
#define CERT_MAX_SIZE 1024
#define KEY_MAX_SIZE 512

// DeviceCert1 default identity for TCAT certification testing.
// DeviceCert2 extra example.
// WARNING: storage of private keys in code or program memory MUST NOT be used in production.
// The below code is for testing purposes only. For production, secure key storage must be
// used to store private keys.
static const char *const OT_CLI_TCAT_X509_CERT[CERT_SET_COUNT] = {R"(
-----BEGIN CERTIFICATE-----
MIIB6TCCAZCgAwIBAgICNekwCgYIKoZIzj0EAwIwcTEmMCQGA1UEAwwdVGhyZWFk
IENlcnRpZmljYXRpb24gRGV2aWNlQ0ExGTAXBgNVBAoMEFRocmVhZCBHcm91cCBJ
bmMxEjAQBgNVBAcMCVNhbiBSYW1vbjELMAkGA1UECAwCQ0ExCzAJBgNVBAYTAlVT
MCAXDTI0MDUwNzA5Mzk0NVoYDzI5OTkxMjMxMDkzOTQ1WjA8MSEwHwYDVQQDDBhU
Q0FUIEV4YW1wbGUgRGV2aWNlQ2VydDExFzAVBgNVBAUTDjQ3MjMtOTgzMy0wMDAx
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE11h/4vKZXVXv+1GDZo066spItloT
dpCi0bux0jvpQSHLdQBIc+40zVCxMDRUvbX//vJKGsSJKOVUlCojQ2wIdqNLMEkw
HwYDVR0jBBgwFoAUX6sbKWiIodS0MaiGYefnZlnt+BkwEAYJKwYBBAGC3yoCBAMC
AQUwFAYJKwYBBAGC3yoDBAcEBSABAQEBMAoGCCqGSM49BAMCA0cAMEQCIHWu+Rd1
VRlzrD8KbuyJcJFTXh2sQ9UIrFIA7+4e/GVcAiAVBdGqTxbt3TGkBBllpafAUB2/
s0GJj7E33oblqy5eHQ==
-----END CERTIFICATE-----
)",
                                                                  R"(
-----BEGIN CERTIFICATE-----
MIIB6TCCAZCgAwIBAgICNeowCgYIKoZIzj0EAwIwcTEmMCQGA1UEAwwdVGhyZWFk
IENlcnRpZmljYXRpb24gRGV2aWNlQ0ExGTAXBgNVBAoMEFRocmVhZCBHcm91cCBJ
bmMxEjAQBgNVBAcMCVNhbiBSYW1vbjELMAkGA1UECAwCQ0ExCzAJBgNVBAYTAlVT
MCAXDTI0MDUwNzA5Mzk0NVoYDzI5OTkxMjMxMDkzOTQ1WjA8MSEwHwYDVQQDDBhU
Q0FUIEV4YW1wbGUgRGV2aWNlQ2VydDIxFzAVBgNVBAUTDjQ3MjMtOTgzMy0wMDAy
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE30GMkqSBj3049NtK6G/MRTqcDxpm
i1LxTpSxFIB7P9HVoVM7Cd9X6bBUp5FrSZI+KHtX2HKtXzmzsdJ3gxAmi6NLMEkw
HwYDVR0jBBgwFoAUX6sbKWiIodS0MaiGYefnZlnt+BkwEAYJKwYBBAGC3yoCBAMC
AQUwFAYJKwYBBAGC3yoDBAcEBSABAQEBMAoGCCqGSM49BAMCA0cAMEQCIAbZzVbC
toNYgSWSgxRGzLRo1YJANqRC7yRtJNKTdQ1ZAiAlgGxEW2lkxCAGPUK1m9Wbb4kl
7AhBhYlK6vZz/omTsQ==
-----END CERTIFICATE-----
)"};

static const char *const OT_CLI_TCAT_PRIV_KEY[CERT_SET_COUNT] = {R"(
-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIIqKM1QTlNaquV74W6Viz/ggXoLqlPOP6LagSyaFO3oUoAoGCCqGSM49
AwEHoUQDQgAE11h/4vKZXVXv+1GDZo066spItloTdpCi0bux0jvpQSHLdQBIc+40
zVCxMDRUvbX//vJKGsSJKOVUlCojQ2wIdg==
-----END EC PRIVATE KEY----- 
)",
                                                                 R"(
-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIP7Al8tJA3QgwD3yIuOSEmJkT3GlWmcHQ59JfhZOjSdUoAoGCCqGSM49
AwEHoUQDQgAE30GMkqSBj3049NtK6G/MRTqcDxpmi1LxTpSxFIB7P9HVoVM7Cd9X
6bBUp5FrSZI+KHtX2HKtXzmzsdJ3gxAmiw==
-----END EC PRIVATE KEY----- 
)"};

#define OT_CLI_TCAT_TRUSTED_ROOT_CERTIFICATE                             \
    "-----BEGIN CERTIFICATE-----\n"                                      \
    "MIICOzCCAeGgAwIBAgIJAKOc2hehOGoBMAoGCCqGSM49BAMCMHExJjAkBgNVBAMM\n" \
    "HVRocmVhZCBDZXJ0aWZpY2F0aW9uIERldmljZUNBMRkwFwYDVQQKDBBUaHJlYWQg\n" \
    "R3JvdXAgSW5jMRIwEAYDVQQHDAlTYW4gUmFtb24xCzAJBgNVBAgMAkNBMQswCQYD\n" \
    "VQQGEwJVUzAeFw0yNDA1MDMyMDAyMThaFw00NDA0MjgyMDAyMThaMHExJjAkBgNV\n" \
    "BAMMHVRocmVhZCBDZXJ0aWZpY2F0aW9uIERldmljZUNBMRkwFwYDVQQKDBBUaHJl\n" \
    "YWQgR3JvdXAgSW5jMRIwEAYDVQQHDAlTYW4gUmFtb24xCzAJBgNVBAgMAkNBMQsw\n" \
    "CQYDVQQGEwJVUzBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGy850VBIPTkN3oL\n" \
    "x++zIUsZk2k26w4fuieFz9oNvjdb5W14+Yf3mvGWsl4NHyLxqhmamVAR4h7zWRlZ\n" \
    "0XyMVpKjYjBgMB4GA1UdEQQXMBWBE3RvbUB0aHJlYWRncm91cC5vcmcwDgYDVR0P\n" \
    "AQH/BAQDAgGGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFF+rGyloiKHUtDGo\n" \
    "hmHn52ZZ7fgZMAoGCCqGSM49BAMCA0gAMEUCIQCTq1qjPZs9fAJB6ppTXs588Pnu\n" \
    "eVFOwC8bd//D99KiHAIgU84kwFHIyDvFqu6y+u1hFqBGsiuTmKwZ2PHhVe/xK1k=\n" \
    "-----END CERTIFICATE-----\n"

namespace ot {
namespace Cli {

otTcatAdvertisedDeviceId sAdvertisedDeviceIds[OT_TCAT_DEVICE_ID_MAX];
otTcatGeneralDeviceId    sGeneralDeviceId;

const char kPskdVendor[]              = "JJJJJJ";
const char kInstallVendor[]           = "InstallCode";
const char kUrl[]                     = "dummy_url";
const char kApplicationServiceName1[] = "echo";
const char kApplicationServiceName2[] = "discard";

static bool IsDeviceIdSet(void)
{
    bool ret = false;
    for (const otTcatAdvertisedDeviceId &vendorDeviceId : sAdvertisedDeviceIds)
    {
        if (vendorDeviceId.mDeviceIdType != OT_TCAT_DEVICE_ID_EMPTY)
        {
            ExitNow(ret = true);
        }
    }
exit:
    return ret;
}

static void HandleBleSecureReceive(otInstance               *aInstance,
                                   const otMessage          *aMessage,
                                   int32_t                   aOffset,
                                   otTcatApplicationProtocol aTcatApplicationProtocol,
                                   void                     *aContext)
{
    OT_UNUSED_VARIABLE(aContext);

    static constexpr int kTextMaxLen = 100;

    if (aTcatApplicationProtocol == OT_TCAT_APPLICATION_PROTOCOL_1 ||
        aTcatApplicationProtocol == OT_TCAT_APPLICATION_PROTOCOL_VENDOR)
    {
        uint8_t  buf[kTextMaxLen];
        uint16_t nLen = otMessageRead(aMessage, static_cast<uint16_t>(aOffset), buf, sizeof(buf));
        IgnoreReturnValue(otBleSecureSendApplicationTlv(aInstance, OT_TCAT_APPLICATION_PROTOCOL_RESPONSE, buf, nLen));
    }
    else if (aTcatApplicationProtocol == OT_TCAT_APPLICATION_PROTOCOL_2)
    {
        uint8_t status = OT_TCAT_STATUS_SUCCESS;
        IgnoreReturnValue(otBleSecureSendApplicationTlv(aInstance, OT_TCAT_APPLICATION_PROTOCOL_STATUS, &status, 1));
    }
}

/**
 * @cli tcat advid
 * @code
 * tcat advid ianapen f378aabb
 * Done
 * @endcode
 * @cparam tcat advid [@ca{id_type}] [@ca{value}]
 * * The `id_type` has five possible values:
 *   * `clear` - removes all previously set advertised IDs.
 *   * `oui24` - sets OUI24 ID type.
 *   * `oui36` - sets OUI36 ID type.
 *   * `discriminator` - sets discriminator ID type.
 *   * `ianapen` - sets IANA PEN ID type.
 * * The `value` hexstring value of the ID.
 * @par
 * Sets/clears advertised ID type and value.
 */
template <> otError Tcat::Process<Cmd("advid")>(Arg aArgs[])
{
    otError                  error = OT_ERROR_NONE;
    otTcatAdvertisedDeviceId devId;
    static const char *const kVendorIdTypes[] = {"clear", "oui24", "oui36", "discriminator", "ianapen"};

    mVendorInfo.mAdvertisedDeviceIds = sAdvertisedDeviceIds;

    if (aArgs[0].IsEmpty())
    {
        if (mVendorInfo.mAdvertisedDeviceIds[0].mDeviceIdType != OT_TCAT_DEVICE_ID_EMPTY)
        {
            OutputLine("Set advertisedIds:");
            for (size_t i = 0; mVendorInfo.mAdvertisedDeviceIds[i].mDeviceIdType != OT_TCAT_DEVICE_ID_EMPTY; i++)
            {
                OutputFormat("type %s, value: ", kVendorIdTypes[mVendorInfo.mAdvertisedDeviceIds[i].mDeviceIdType]);
                OutputBytesLine(const_cast<uint8_t *>(mVendorInfo.mAdvertisedDeviceIds[i].mDeviceId),
                                mVendorInfo.mAdvertisedDeviceIds[i].mDeviceIdLen);
            }
        }
        ExitNow();
    }

    if (aArgs[0] == kVendorIdTypes[OT_TCAT_DEVICE_ID_OUI24])
    {
        devId.mDeviceIdType = OT_TCAT_DEVICE_ID_OUI24;
    }
    else if (aArgs[0] == kVendorIdTypes[OT_TCAT_DEVICE_ID_OUI36])
    {
        devId.mDeviceIdType = OT_TCAT_DEVICE_ID_OUI36;
    }
    else if (aArgs[0] == kVendorIdTypes[OT_TCAT_DEVICE_ID_DISCRIMINATOR])
    {
        devId.mDeviceIdType = OT_TCAT_DEVICE_ID_DISCRIMINATOR;
    }
    else if (aArgs[0] == kVendorIdTypes[OT_TCAT_DEVICE_ID_IANAPEN])
    {
        devId.mDeviceIdType = OT_TCAT_DEVICE_ID_IANAPEN;
    }
    else if (aArgs[0] == kVendorIdTypes[OT_TCAT_DEVICE_ID_EMPTY])
    {
        for (otTcatAdvertisedDeviceId &vendorDeviceId : sAdvertisedDeviceIds)
        {
            vendorDeviceId.mDeviceIdType = OT_TCAT_DEVICE_ID_EMPTY;
            vendorDeviceId.mDeviceIdLen  = 0;
        }
        ExitNow();
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    if (!aArgs[1].IsEmpty() && aArgs[1].GetLength() < (OT_TCAT_MAX_ADVERTISED_DEVICEID_SIZE * 2 + 1))
    {
        devId.mDeviceIdLen = OT_TCAT_MAX_ADVERTISED_DEVICEID_SIZE;
        SuccessOrExit(error = aArgs[1].ParseAsHexString(devId.mDeviceIdLen, devId.mDeviceId));
        for (otTcatAdvertisedDeviceId &vendorDeviceId : sAdvertisedDeviceIds)
        {
            if (vendorDeviceId.mDeviceIdType == devId.mDeviceIdType ||
                vendorDeviceId.mDeviceIdType == OT_TCAT_DEVICE_ID_EMPTY)
            {
                vendorDeviceId = devId;
                break;
            }
        }
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }
exit:
    return error;
}

/**
 * @cli tcat certid
 * @code
 * tcat devid certid 0
 * Done
 * @endcode
 * @cparam tcat certid [@ca{value}]
 * * The `value` int value of the ID.
 * @par
 * Selects a predefined certificate.
 */
template <> otError Tcat::Process<Cmd("certid")>(Arg aArgs[])
{
    Error   error         = kErrorNone;
    uint8_t certCandidate = 0;

    if (aArgs[0].IsEmpty())
    {
        OutputLine("%d", mSelectedCert);
        ExitNow();
    }

    SuccessOrExit(error = aArgs[0].ParseAsUint8(certCandidate));

    VerifyOrExit(certCandidate < CERT_SET_COUNT, error = kErrorInvalidArgs);

    mSelectedCert = certCandidate;

exit:
    return error;
}

/**
 * @cli tcat devid
 * @code
 * tcat devid ianapen f378aabb
 * Done
 * @endcode
 * @cparam tcat devid [@ca{value}|clear]
 * * The `value` hexstring value of the ID. `clear` is a special value removing previously set ID.
 * @par
 * Sets/clears vendor specific device ID.
 */
template <> otError Tcat::Process<Cmd("devid")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        if (sGeneralDeviceId.mDeviceIdLen != 0)
        {
            OutputLine("TCAT DeviceId:");
            OutputBytesLine(sGeneralDeviceId.mDeviceId, sGeneralDeviceId.mDeviceIdLen);
        }
        ExitNow();
    }

    if (aArgs[0] == "clear")
    {
        ClearAllBytes(sGeneralDeviceId);
    }
    else
    {
        VerifyOrExit(aArgs[0].GetLength() < (OT_TCAT_MAX_DEVICEID_SIZE * 2 + 1), error = OT_ERROR_INVALID_ARGS);
        sGeneralDeviceId.mDeviceIdLen = OT_TCAT_MAX_DEVICEID_SIZE;
        SuccessOrExit(error = aArgs[0].ParseAsHexString(sGeneralDeviceId.mDeviceIdLen, sGeneralDeviceId.mDeviceId));
    }

exit:
    return error;
}

/**
 * @cli tcat start
 * @code
 * tcat start
 * Done
 * @endcode
 * @par
 * Starts TCAT operation.
 * @sa otBleSecureSetCertificate
 * @sa otBleSecureSetCaCertificateChain
 * @sa otBleSecureSetSslAuthMode
 * @sa otBleSecureSetTcatVendorInfo
 * @sa otBleSecureStart
 * @sa otBleSecureTcatStart
 */
template <> otError Tcat::Process<Cmd("start")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;

    ClearAllBytes(mVendorInfo);
    mVendorInfo.mPskdString                 = kPskdVendor;
    mVendorInfo.mProvisioningUrl            = kUrl;
    mVendorInfo.mInstallCode                = kInstallVendor;
    mVendorInfo.mApplicationServiceName[0]  = kApplicationServiceName1;
    mVendorInfo.mApplicationServiceName[1]  = kApplicationServiceName2;
    mVendorInfo.mApplicationServiceIsTcp[1] = true;

    if (IsDeviceIdSet())
    {
        mVendorInfo.mAdvertisedDeviceIds = sAdvertisedDeviceIds;
    }

    if (sGeneralDeviceId.mDeviceIdLen != 0)
    {
        mVendorInfo.mGeneralDeviceId = &sGeneralDeviceId;
    }

    otBleSecureSetCertificate(GetInstancePtr(), reinterpret_cast<const uint8_t *>(OT_CLI_TCAT_X509_CERT[mSelectedCert]),
                              StringLength(OT_CLI_TCAT_X509_CERT[mSelectedCert], CERT_MAX_SIZE) + 1,
                              reinterpret_cast<const uint8_t *>(OT_CLI_TCAT_PRIV_KEY[mSelectedCert]),
                              StringLength(OT_CLI_TCAT_PRIV_KEY[mSelectedCert], KEY_MAX_SIZE) + 1);

    otBleSecureSetCaCertificateChain(GetInstancePtr(),
                                     reinterpret_cast<const uint8_t *>(OT_CLI_TCAT_TRUSTED_ROOT_CERTIFICATE),
                                     sizeof(OT_CLI_TCAT_TRUSTED_ROOT_CERTIFICATE));

    otBleSecureSetSslAuthMode(GetInstancePtr(), true);

    SuccessOrExit(error = otBleSecureSetTcatVendorInfo(GetInstancePtr(), &mVendorInfo));
    SuccessOrExit(error = otBleSecureStart(GetInstancePtr(), nullptr, HandleBleSecureReceive, true, nullptr));
    SuccessOrExit(error = otBleSecureTcatStart(GetInstancePtr(), nullptr));

exit:
    return error;
}

/**
 * @cli tcat stop
 * @code
 * tcat stop
 * Done
 * @endcode
 * @par
 * Stops TCAT operation.
 * @sa otBleSecureStop
 */
template <> otError Tcat::Process<Cmd("stop")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otBleSecureStop(GetInstancePtr());

    return OT_ERROR_NONE;
}

otError Tcat::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                            \
    {                                                       \
        aCommandString, &Tcat::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {CmdEntry("advid"), CmdEntry("devid"), CmdEntry("start"), CmdEntry("stop")};

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

#undef CmdEntry

    otError        error = OT_ERROR_NONE;
    const Command *command;

    if (aArgs[0].IsEmpty() || (aArgs[0] == "help"))
    {
        OutputCommandTable(kCommands);
        ExitNow(error = aArgs[0].IsEmpty() ? error : OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

} // namespace Cli
} // namespace ot
#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE && OPENTHREAD_CONFIG_CLI_BLE_SECURE_ENABLE
