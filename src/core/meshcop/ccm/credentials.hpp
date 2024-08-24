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
 *   This file includes definitions for credentials.
 */

#ifndef CREDENTIALS_HPP_
#define CREDENTIALS_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_CCM_ENABLE

#include "coap/coap_secure.hpp"
#include "common/locator.hpp"
#include "mbedtls/pk.h"

namespace ot {

namespace MeshCoP {

class Credentials : public InstanceLocator
{
public:
    static const size_t kMaxCertLength         = 1024;	// see Thread Conformance specification v1.2.1 or later.
    static const size_t kMaxKeyLength          = 256;
    static const size_t kMaxSerialNumberLength = 64;
    static const size_t kMaxSubjectNameLength  = 256;

    // According to section 4.2.1.2 of RFC 5280
    static const size_t kMaxKeyIdentifierLength = 20;

    explicit Credentials(Instance &aInstance);

    otError Restore(void);

    otError Store(void);

    void Clear(void);

    const uint8_t *GetManufacturerCert(size_t &aLength);

    const uint8_t *GetManufacturerPrivateKey(size_t &aLength);

    const uint8_t *GetManufacturerCACert(size_t &aLength);

    const uint8_t *GetOperationalCert(size_t &aLength);
    otError        SetOperationalCert(const uint8_t *aCert, size_t aLength);

    const uint8_t *GetOperationalPrivateKey(size_t &aLength);
    otError        SetOperationalPrivateKey(const uint8_t *aKey, size_t aLength);

    const uint8_t *GetDomainCACert(size_t &aLength);
    otError        SetDomainCACert(const uint8_t *aCert, size_t aLength);

    /**
     * Get an additional, top-level Domain CA cert that is stored in this device, other than the
     * Domain CA cert of GetDomainCACert().
     */
    const uint8_t *GetToplevelDomainCACert(size_t &aLength);

    /**
     * Set an optional additional, top-level Domain CA cert, other than the Domain CA cert set in
     * SetDomainCACert(). This top-level cert may be optionally delivered to this device via the
     * Voucher. Once this is set during AE enrollment, it stays as immutable Trust Anchor until
     * the device is factory-reset and cannot be replaced by any EST operations.
     */
    otError SetToplevelDomainCACert(const uint8_t *aCert, size_t aLength);

    otError GetAuthorityKeyId(uint8_t *aBuf, size_t &aLength, size_t aMaxLength);

    otError GetManufacturerSerialNumber(char *aBuf, size_t aMaxLength);

    otError GetManufacturerSubjectName(char *aBuf, size_t aMaxLength);

    // nullable
    const char *GetDomainName() const;

private:
    static otError ParseSerialNumberFromSubjectName(char *aBuf, size_t aMaxLength, const mbedtls_x509_crt &cert);

    static otError ParseDomainName(DomainName &aDomainName, const uint8_t *aCert, size_t aCertLength);

    // TODO(EskoDijk): can be shortened if this IDevID is known to be smaller - chosen at design time.
    uint8_t mManufacturerCert[kMaxCertLength];
    size_t  mManufacturerCertLength;
    uint8_t mManufacturerPrivateKey[kMaxKeyLength];
    size_t  mManufacturerPrivateKeyLength;
    // TODO(EskoDijk): can be shortened if this IDevID is known to be smaller - chosen at design time.
    uint8_t mManufacturerCACert[kMaxCertLength];
    size_t  mManufacturerCACertLength;
    uint8_t mOperationalCert[kMaxCertLength];
    size_t  mOperationalCertLength;
    uint8_t mOperationalPrivateKey[kMaxKeyLength];
    size_t  mOperationalPrivateKeyLength;
    uint8_t mDomainCACert[kMaxCertLength];
    size_t  mDomainCACertLength;
    uint8_t mToplevelDomainCACert[kMaxCertLength];
    size_t  mToplevelDomainCACertLength;

    DomainName mDomainName; ///< Thread Domain Name, taken from a subfield of SubjectAltName of operational cert
};

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_CCM_ENABLE

#endif // CREDENTIALS_HPP_
