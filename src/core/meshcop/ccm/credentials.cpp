/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

/**
 * @file
 *   This file implements credentials storage for a Thread CCM device.
 */

#include "credentials.hpp"

#if OPENTHREAD_CONFIG_CCM_ENABLE

#include "crypto/mbedtls.hpp"
#include "instance/instance.hpp"
#include "mbedtls/oid.h"
#include "thread/key_manager.hpp"
#include "thread/tmf.hpp"

// include factory-written IDevID cert+key from separate file.
#include "idevid_x509_cert_key.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("Credentials");

Credentials::Credentials(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mManufacturerCert(reinterpret_cast<const uint8_t *>(&kIdevidCert))
    , mManufacturerCertLength(sizeof(kIdevidCert))
    , mOperationalCertLength(0)
    , mDomainCACertLength(0)
    , mToplevelDomainCACertLength(0)
{
    mDomainName = Get<MeshCoP::NetworkNameManager>().GetDomainName();
}

Error Credentials::Restore(void)
{
    // TODO
    return kErrorNotImplemented;
}

Error Credentials::Store(void)
{
    // TODO(wgtdkp): Store to settings
    return kErrorNotImplemented;
}

void Credentials::Clear(void)
{
    // TODO(wgtdkp): remove those certs and keys from cache and flash
    mOperationalCertLength = 0;
    mOperationalPrivateKey.SetDerLength(0);
    mDomainCACertLength         = 0;
    mToplevelDomainCACertLength = 0;
}

Error Credentials::ConfigureIdevid(SecureTransport *client)
{
    client->SetCertificate(reinterpret_cast<const uint8_t *>(kIdevidCert), sizeof(kIdevidCert),
                           reinterpret_cast<const uint8_t *>(kIdevidPrivateKey), sizeof(kIdevidPrivateKey));
    client->SetCaCertificateChain(reinterpret_cast<const uint8_t *>(kIdevidCaCert), sizeof(kIdevidCaCert));
    client->SetSslAuthMode(false); // for cBRSKI: MUST provisionally trust any Registrar

    return kErrorNone;
}

Error Credentials::ConfigureLdevid(SecureTransport *client)
{
    Error error = kErrorNone;
    VerifyOrExit(HasOperationalCert(), error = kErrorInvalidState);

    client->SetCertificate(reinterpret_cast<const uint8_t *>(mOperationalCert), mOperationalCertLength,
                           mOperationalPrivateKey.GetDerBytes(), mOperationalPrivateKey.GetDerLength());
    client->SetCaCertificateChain(reinterpret_cast<const uint8_t *>(mDomainCACert), mDomainCACertLength);
    client->SetSslAuthMode(true); // for LDevID ops: MUST trust Domain's Commissioner or Registrar.

exit:
    return error;
}

const uint8_t *Credentials::GetManufacturerCert(size_t &aLength)
{
    aLength = sizeof(kIdevidCert);
    return reinterpret_cast<const uint8_t *>(kIdevidCert);
}

const uint8_t *Credentials::GetManufacturerPrivateKey(size_t &aLength)
{
    aLength = sizeof(kIdevidPrivateKey);
    return reinterpret_cast<const uint8_t *>(kIdevidPrivateKey);
}

const uint8_t *Credentials::GetManufacturerCACert(size_t &aLength)
{
    aLength = sizeof(kIdevidCaCert);
    return reinterpret_cast<const uint8_t *>(kIdevidCaCert);
}

const uint8_t *Credentials::GetOperationalCert(size_t &aLength)
{
    aLength = mOperationalCertLength;
    return mOperationalCert;
}

Error Credentials::SetOperationalCert(const uint8_t *aCert, size_t aLength)
{
    Error error = kErrorNone;

    if (aCert == nullptr || aLength == 0)
    {
        mOperationalCertLength = 0;
        ExitNow();
    }
    VerifyOrExit(aLength <= kMaxCertLength, error = kErrorInvalidArgs);

    // FIXME: if no domain name included, assume 'DefaultDomain'.
    SuccessOrExit(error = ParseDomainName(mDomainName, aCert, aLength));

    // TODO(wgtdkp): should we listen to SECURITY_POLICY_CHANGED event
    // and reset DomainName if CCM is enabled/disabled ?
    // TODO validate if changing domain name while Thread/radio remains active works ok.
    SuccessOrExit(error = Get<MeshCoP::NetworkNameManager>().SetDomainName(mDomainName.GetAsData()));

    memcpy(mOperationalCert, aCert, aLength);
    mOperationalCertLength = aLength;

exit:
    return error;
}

bool Credentials::HasOperationalCert(void) const { return mOperationalCertLength > 0; }

const KeyInfo *Credentials::GetOperationalPrivateKey() { return &mOperationalPrivateKey; }

Error Credentials::SetOperationalPrivateKey(const KeyInfo &privKey)
{
    mOperationalPrivateKey = privKey;
    return kErrorNone;
}

const uint8_t *Credentials::GetDomainCACert(size_t &aLength)
{
    aLength = mDomainCACertLength;
    return mDomainCACert;
}

const uint8_t *Credentials::GetToplevelDomainCACert(size_t &aLength)
{
    aLength = mToplevelDomainCACertLength;
    return mToplevelDomainCACert;
}

Error Credentials::SetDomainCACert(const uint8_t *aCert, size_t aLength)
{
    Error error = kErrorNone;
    VerifyOrExit(aLength <= kMaxCertLength, error = kErrorInvalidArgs);

    memcpy(mDomainCACert, aCert, aLength);
    mDomainCACertLength = aLength;
exit:
    return error;
}

Error Credentials::SetToplevelDomainCACert(const uint8_t *aCert, size_t aLength)
{
    Error error = kErrorNone;
    VerifyOrExit(aLength <= kMaxCertLength, error = kErrorInvalidArgs);

    memcpy(mToplevelDomainCACert, aCert, aLength);
    mToplevelDomainCACertLength = aLength;
exit:
    return error;
}

Error Credentials::GetAuthorityKeyId(uint8_t *aBuf, size_t &aLength, size_t aMaxLength)
{
    static const int kTagSequence = MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE;
    Error            error        = kErrorParse;
    mbedtls_x509_crt manufacturerCert;
    uint8_t         *p;
    const uint8_t   *end;
    size_t           len;

    mbedtls_x509_crt_init(&manufacturerCert);

    VerifyOrExit(mbedtls_x509_crt_parse(&manufacturerCert, mManufacturerCert, mManufacturerCertLength) == 0);

    p   = manufacturerCert.v3_ext.p;
    end = p + manufacturerCert.v3_ext.len;
    VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, kTagSequence) == 0);

    while (p < end)
    {
        mbedtls_asn1_buf oidValue;
        uint8_t         *extEnd;

        VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, kTagSequence) == 0);
        extEnd = p + len;

        VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OID) == 0);
        oidValue.tag = MBEDTLS_ASN1_OID;
        oidValue.p   = p;
        oidValue.len = len;

        if (MBEDTLS_OID_CMP(MBEDTLS_OID_AUTHORITY_KEY_IDENTIFIER, &oidValue) != 0)
        {
            p = extEnd;
            continue;
        }

        p += len;

        VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OCTET_STRING) == 0);
        VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, kTagSequence) == 0);
        VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0) == 0);

        VerifyOrExit(len <= aMaxLength, error = kErrorNoBufs);
        memcpy(aBuf, p, len);
        aLength = len;

        ExitNow(error = kErrorNone);
    }

    error = kErrorNotFound;

exit:
    mbedtls_x509_crt_free(&manufacturerCert);
    return error;
}

Error Credentials::GetManufacturerSerialNumber(char *aBuf, size_t aMaxLength)
{
    Error            error;
    mbedtls_x509_crt manufacturerCert;

    mbedtls_x509_crt_init(&manufacturerCert);
    VerifyOrExit(mbedtls_x509_crt_parse(&manufacturerCert, mManufacturerCert, mManufacturerCertLength) == 0,
                 error = kErrorParse);
    error = ParseSerialNumberFromSubjectName(aBuf, aMaxLength, manufacturerCert);

exit:
    mbedtls_x509_crt_free(&manufacturerCert);
    return error;
}

Error Credentials::GetManufacturerSubjectName(char *aBuf, size_t aMaxLength)
{
    Error            error = kErrorNone;
    mbedtls_x509_crt manufacturerCert;
    int              rval;

    mbedtls_x509_crt_init(&manufacturerCert);

    VerifyOrExit(mbedtls_x509_crt_parse(&manufacturerCert, mManufacturerCert, mManufacturerCertLength) == 0,
                 error = kErrorParse);

    OT_ASSERT(aMaxLength > 1);

    rval = mbedtls_x509_dn_gets(aBuf, aMaxLength - 1, &manufacturerCert.subject);
    VerifyOrExit(rval >= 0, error = kErrorFailed);

    aBuf[rval] = '\0';

exit:
    mbedtls_x509_crt_free(&manufacturerCert);
    return error;
}

Error Credentials::ParseSerialNumberFromSubjectName(char *aBuf, size_t aMaxLength, const mbedtls_x509_crt &cert)
{
    Error                    error;
    const mbedtls_x509_name &subject = cert.subject;

    for (const mbedtls_x509_name *cur = &subject; cur != nullptr; cur = cur->next)
    {
        if (!(cur->oid.tag & MBEDTLS_ASN1_OID))
        {
            continue;
        }

        if (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_SERIAL_NUMBER, &cur->oid) != 0)
        {
            continue;
        }

        VerifyOrExit(cur->val.len + 1 <= aMaxLength, error = kErrorNoBufs);
        memcpy(aBuf, cur->val.p, cur->val.len);
        aBuf[cur->val.len] = '\0';
        ExitNow(error = kErrorNone);
    }
    error = kErrorNotFound;

exit:
    return error;
}

Error Credentials::ParseDomainName(DomainName &aDomainName, const uint8_t *aCert, size_t aCertLength)
{
    Error            error = kErrorNone;
    mbedtls_x509_crt cert;
    size_t           domainNameLength = ot::MeshCoP::DomainName::kMaxSize;

    mbedtls_x509_crt_init(&cert);
    VerifyOrExit(mbedtls_x509_crt_parse(&cert, aCert, aCertLength) == 0, error = kErrorSecurity);

    // FIXME merge constant 1 with tcat_agent constants (make them shared in SecureAgent?)
    SuccessOrExit(error = Get<Tmf::SecureAgent>().GetDtls().GetThreadAttributeFromCertificate(
                      &cert, 1, reinterpret_cast<uint8_t *>(&aDomainName.m8), &domainNameLength));
    VerifyOrExit(domainNameLength > 0, error = kErrorNotFound);

exit:
    mbedtls_x509_crt_free(&cert);
    return error;
}

DomainName *Credentials::GetDomainName() { return &mDomainName; }

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_CCM_ENABLE
