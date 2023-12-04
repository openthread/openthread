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

#include "cli/cli_output.hpp"

#include "cli/cli_tcat.hpp"

#include <openthread/ble_secure.h>

#include <mbedtls/oid.h>
#include <openthread/tcat.h>
#include <openthread/platform/ble.h>

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE && OPENTHREAD_CONFIG_CLI_BLE_SECURE_ENABLE

#define OT_CLI_TCAT_X509_CERT                                              \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "MIIBmDCCAT+gAwIBAgIEAQIDBDAKBggqhkjOPQQDAjBvMQswCQYDVQQGEwJYWDEQ\r\n" \
    "MA4GA1UECBMHTXlTdGF0ZTEPMA0GA1UEBxMGTXlDaXR5MQ8wDQYDVQQLEwZNeVVu\r\n" \
    "aXQxETAPBgNVBAoTCE15VmVuZG9yMRkwFwYDVQQDExB3d3cubXl2ZW5kb3IuY29t\r\n" \
    "MB4XDTIzMTAxNjEwMzk1NFoXDTI0MTAxNjEwMzk1NFowIjEgMB4GA1UEAxMXbXl2\r\n" \
    "ZW5kb3IuY29tL3RjYXQvbXlkZXYwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQB\r\n" \
    "aWwFDNj1bpQIdN+Kp2cHWw55U/+fa+OmZnoy1B4BOT+822jdwPBuyXWAQoBdYdQJ\r\n" \
    "ff4RgmhczyV4PhArPIuAoxYwFDASBgkrBgEEAYLfKgMEBQABAQEBMAoGCCqGSM49\r\n" \
    "BAMCA0cAMEQCIBEHxiEDij26y6V77Q311Gj4CZAuZuPGXZpnzL2BLk7bAiAlFk6G\r\n" \
    "mYGzkcrYyssFI9HlPgrisWoMmgummaTtCuvrEw==\r\n"                         \
    "-----END CERTIFICATE-----\r\n"

#define OT_CLI_TCAT_PRIV_KEY                                               \
    "-----BEGIN EC PRIVATE KEY-----\r\n"                                   \
    "MHcCAQEEIDeJ6lVQKiOIBxKwTZp6TkU5QVHt9pvXOR9CGpPBI3DhoAoGCCqGSM49\r\n" \
    "AwEHoUQDQgAEAWlsBQzY9W6UCHTfiqdnB1sOeVP/n2vjpmZ6MtQeATk/vNto3cDw\r\n" \
    "bsl1gEKAXWHUCX3+EYJoXM8leD4QKzyLgA==\r\n"                             \
    "-----END EC PRIVATE KEY-----\r\n"

#define OT_CLI_TCAT_TRUSTED_ROOT_CERTIFICATE                               \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "MIICCDCCAa2gAwIBAgIJAIKxygBXoH+5MAoGCCqGSM49BAMCMG8xCzAJBgNVBAYT\r\n" \
    "AlhYMRAwDgYDVQQIEwdNeVN0YXRlMQ8wDQYDVQQHEwZNeUNpdHkxDzANBgNVBAsT\r\n" \
    "Bk15VW5pdDERMA8GA1UEChMITXlWZW5kb3IxGTAXBgNVBAMTEHd3dy5teXZlbmRv\r\n" \
    "ci5jb20wHhcNMjMxMDE2MTAzMzE1WhcNMjYxMDE2MTAzMzE1WjBvMQswCQYDVQQG\r\n" \
    "EwJYWDEQMA4GA1UECBMHTXlTdGF0ZTEPMA0GA1UEBxMGTXlDaXR5MQ8wDQYDVQQL\r\n" \
    "EwZNeVVuaXQxETAPBgNVBAoTCE15VmVuZG9yMRkwFwYDVQQDExB3d3cubXl2ZW5k\r\n" \
    "b3IuY29tMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEWdyzPAXGKeZY94OhHAWX\r\n" \
    "HzJfQIjGSyaOzlgL9OEFw2SoUDncLKPGwfPAUSfuMyEkzszNDM0HHkBsDLqu4n25\r\n" \
    "/6MyMDAwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQU4EynoSw9eDKZEVPkums2\r\n" \
    "IWLAJCowCgYIKoZIzj0EAwIDSQAwRgIhAMYGGL9xShyE6P9wEU+MAYF6W3CzdrwV\r\n" \
    "kuerX1encIH2AiEA5rq490NUobM1Au43roxJq1T6Z43LscPVbGZfULD1Jq0=\r\n"     \
    "-----END CERTIFICATE-----\r\n"

namespace ot {

namespace Cli {

otTcatVendorInfo sVendorInfo;
const char       kPskdVendor[] = "J01NM3";
const char       kUrl[]        = "dummy_url";

static void HandleBleSecureReceive(otInstance               *aInstance,
                                   const otMessage          *aMessage,
                                   int32_t                   aOffset,
                                   otTcatApplicationProtocol aTcatApplicationProtocol,
                                   const char               *aServiceName,
                                   void                     *aContext)
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aTcatApplicationProtocol);
    OT_UNUSED_VARIABLE(aServiceName);
    static constexpr int     kTextMaxLen   = 100;
    static constexpr uint8_t kBufPrefixLen = 5;
    uint16_t                 nLen;
    uint8_t                  buf[kTextMaxLen];

    nLen = otMessageRead(aMessage, (uint16_t)aOffset, buf + kBufPrefixLen, sizeof(buf) - kBufPrefixLen - 1);

    memcpy(buf, "RECV:", kBufPrefixLen);

    buf[nLen + kBufPrefixLen] = 0;

    IgnoreReturnValue(otBleSecureSendApplicationTlv(aInstance, buf, (uint16_t)strlen((char *)buf)));
    IgnoreReturnValue(otBleSecureFlush(aInstance));
}

template <> otError Tcat::Process<Cmd("start")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;

    sVendorInfo.mPskdString      = kPskdVendor;
    sVendorInfo.mProvisioningUrl = kUrl;

    otBleSecureSetCertificate(GetInstancePtr(), reinterpret_cast<const uint8_t *>(OT_CLI_TCAT_X509_CERT),
                              sizeof(OT_CLI_TCAT_X509_CERT), reinterpret_cast<const uint8_t *>(OT_CLI_TCAT_PRIV_KEY),
                              sizeof(OT_CLI_TCAT_PRIV_KEY));

    otBleSecureSetCaCertificateChain(GetInstancePtr(),
                                     reinterpret_cast<const uint8_t *>(OT_CLI_TCAT_TRUSTED_ROOT_CERTIFICATE),
                                     sizeof(OT_CLI_TCAT_TRUSTED_ROOT_CERTIFICATE));

    otBleSecureSetSslAuthMode(GetInstancePtr(), true);

    SuccessOrExit(error = otBleSecureStart(GetInstancePtr(), nullptr, HandleBleSecureReceive, true, nullptr));
    SuccessOrExit(error = otBleSecureTcatStart(GetInstancePtr(), &sVendorInfo, nullptr));

exit:
    return error;
}

template <> otError Tcat::Process<Cmd("stop")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);
    otError error = OT_ERROR_NONE;

    otBleSecureStop(GetInstancePtr());

    return error;
}

otError Tcat::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                            \
    {                                                       \
        aCommandString, &Tcat::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {CmdEntry("start"), CmdEntry("stop")};

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

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
