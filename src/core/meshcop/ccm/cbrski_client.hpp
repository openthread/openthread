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
 *   This file includes definitions for the cBRSKI client.
 *   Ref: https://datatracker.ietf.org/doc/html/draft-ietf-anima-constrained-voucher-25
 */

#ifndef CBRSKI_CLIENT_HPP_
#define CBRSKI_CLIENT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_CCM_ENABLE

#include "credentials.hpp"
#include "coap/coap_secure.hpp"
#include "common/locator.hpp"
#include "crypto/ecdsa.hpp"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/pk.h"

namespace ot {

namespace MeshCoP {

// URIs used in cBRSKI
#define OT_URI_PATH_JOINER_ENROLL_STATUS ".well-known/brski/es"
#define OT_URI_PATH_JOINER_REQUEST_VOUCHER ".well-known/brski/rv"
#define OT_URI_PATH_JOINER_VOUCHER_STATUS ".well-known/brski/vs"
#define OT_URI_PATH_JOINER_CA_CERTS ".well-known/est/crts"
#define OT_URI_PATH_JOINER_ENROLL ".well-known/est/sen"
#define OT_URI_PATH_JOINER_REENROLL ".well-known/est/sren"

class CbrskiClient : InstanceLocator
{
public:
    explicit CbrskiClient(Instance &aInstance);

    /**
     * Starts Thread Autonomous Enrollment (AE) using IETF cBRSKI on a connected CoAPS session to Registrar.
     *
     * @param[in]  aConnectedCoapSecure  A connected CoAPs session, using IDevID identity.
     * @param[in]  aCallback             A callback function to be called at the end of enrollment.
     * @param[in]  aContext              A context for the callback function.
     *
     * The aCallback is guaranteed to be called whether the enrollment succeeds or not.
     */
    void StartEnroll(Coap::CoapSecure &aConnectedCoapSecure, otJoinerCallback aCallback, void *aContext);

    /**
     * Check whether the cBRSKI client is busy/active.
     *
     * @return true if busy/active, false otherwise.
     */
    bool IsBusy() const { return mCoapSecure != nullptr; }

    /**
     * Cleanup all temporary data structures and finish operation of this client.
     *
     * @param[in]  aError                Error code indicating cause of finishing.
     * @param[in]  aInvokeCallback       Whether to invoke finished-callback, or not.
     */
    void Finish(Error aError, bool aInvokeCallback);

private:
    static const size_t kMaxCsrSize         = 512;
    static const size_t kMaxVoucherSize     = 1024; // MASA service of Vendor defines max voucher size.
    static const size_t kVoucherNonceLength = 8;

    static const mbedtls_pk_type_t    kOperationalKeyType = MBEDTLS_PK_ECKEY;
    static const mbedtls_ecp_group_id kEcpGroupId         = MBEDTLS_ECP_DP_SECP256R1;

    /**
     * The constrained voucher request SID values defined in draft-ietf-anima-rfc8366bis-NN
     */
    struct VoucherRequestSID
    {
        static const int kVoucher               = 2501;
        static const int kAssertion             = kVoucher + 1;
        static const int kCreatedOn             = kVoucher + 2;
        static const int kDomainCertRevChecks   = kVoucher + 3;
        static const int kExpiresOn             = kVoucher + 4;
        static const int kIdevidIssuer          = kVoucher + 5;
        static const int kLastRenewalDate       = kVoucher + 6;
        static const int kNonce                 = kVoucher + 7;
        static const int kPinnedDomainCert      = kVoucher + 8;
        static const int kPriorSignedVoucherReq = kVoucher + 9;
        static const int kProxRegistrarCert     = kVoucher + 10;
        static const int kSha256RegistrarSPKI   = kVoucher + 11;
        static const int kProxRegistrarSPKI     = kVoucher + 12;
        static const int kSerialNumber          = kVoucher + 13;

    private:
        VoucherRequestSID() {}
    };

    /**
     * The constrained voucher SID values defined in draft-ietf-anima-rfc8366bis-NN
     */
    struct VoucherSID
    {
        static const int kVoucher                  = 2451;
        static const int kAssertion                = kVoucher + 1;
        static const int kCreatedOn                = kVoucher + 2;
        static const int kDomainCertRevChecks      = kVoucher + 3;
        static const int kExpiresOn                = kVoucher + 4;
        static const int kIdevidIssuer             = kVoucher + 5;
        static const int kLastRenewalDate          = kVoucher + 6;
        static const int kNonce                    = kVoucher + 7;
        static const int kPinnedDomainCert         = kVoucher + 8;
        static const int kPinnedDomainPubKey       = kVoucher + 9;
        static const int kPinnedDomainPubKeySha256 = kVoucher + 10;
        static const int kSerialNumber             = kVoucher + 11;

    private:
        VoucherSID() {}
    };

    enum VoucherAssertion
    {
        kVerified       = 0,
        kLogged         = 1,
        kProximity      = 2,
        kAgentProximity = 3,
    };

    struct VoucherRequest
    {
        int     mAssertion;
        uint8_t mNonce[kVoucherNonceLength];
        char    mSerialNumber[Credentials::kMaxSerialNumberLength + 1];
        uint8_t mRegPubKey[Credentials::kMaxKeyLength];
        size_t  mRegPubKeyLength;
    };

    Error SendVoucherRequest();
    Error CreateVoucherRequest(VoucherRequest &aVoucherReq, mbedtls_x509_crt &aRegistrarCert);
    Error SignVoucherRequest(uint8_t *aBuf, size_t &aLength, size_t aBufLength, const VoucherRequest &aVoucherReq);
    Error SerializeVoucherRequest(uint8_t *aBuf, size_t &aLength, size_t aMaxLength, const VoucherRequest &aVoucherReq);

    static void HandleVoucherResponse(void                *aContext,
                                      otMessage           *aMessage,
                                      const otMessageInfo *aMessageInfo,
                                      Error                aResult);
    void        HandleVoucherResponse(Coap::Message &aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult);

    void ReportStatusTelemetry(const char *aUri, Error aError, const char *aContext);

    Error       SendCaCertsRequest();
    static void HandleCaCertsResponse(void                *aContext,
                                      otMessage           *aMessage,
                                      const otMessageInfo *aMessageInfo,
                                      Error                aResult);
    void        HandleCaCertsResponse(Coap::Message &aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult);

    Error       SendEnrollRequest();
    static void HandleEnrollResponse(void                *aContext,
                                     otMessage           *aMessage,
                                     const otMessageInfo *aMessageInfo,
                                     Error                aResult);
    void        HandleEnrollResponse(Coap::Message &aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult);

    /**
     * Process a received Voucher, check contents, verify security, and store any relevant info from it in this
     * EstClient. Stores the pinned-domain-cert in 'mPinnedDomainCert'.
     */
    Error ProcessVoucher(const uint8_t *aVoucher, size_t aVoucherLength);

    /**
     * Process a received LDevID operational cert, verify syntax, and store as 'mOperationalCert' in this EstClient.
     * @param [out] isNeedCaCertsRequest set True if a CA Certs (/crts) request is needed additionally to obtain
     *                                   the Domain CA cert needed to validate the LDevID. False, if not needed.
     */
    Error ProcessOperationalCert(const uint8_t *aCert, size_t aLength, bool &isNeedCaCertsRequest);

    /**
     * Store the LDevID, Domain CA, toplevel Domain CA (if present), all into the local Credentials store (Trust Store).
     */
    Error ProcessCertsIntoTrustStore();

    // The data is written to the end of the buffer
    Error CreateCsrData(KeyInfo &aKey, const char *aSubjectName, unsigned char *aBuf, size_t aBufLen, size_t &aCsrLen);

    /**
     * Compare two encoded certs for equality.
     *
     * @return true if equal, false otherwise (e.g. if one or both is `nullptr`).
     */
    bool IsCertsEqual(const mbedtls_x509_crt *cert1, const mbedtls_x509_crt *cert2);

    void PrintEncodedCert(const uint8_t *aCert, size_t aLength);

    Coap::CoapSecure          *mCoapSecure;
    Callback<otJoinerCallback> mCallback;

    // whether current process is a reenrollment (true) or not (false)
    bool mIsDoingReenroll;

    // the voucher request object
    VoucherRequest *mVoucherReq;

    // peer Registrar cert obtained during this session
    mbedtls_x509_crt mRegistrarCert;

    // pinned domain cert, if any, from Voucher, obtained during this session. May or may not be
    // equal to mDomainCACert.
    mbedtls_x509_crt mPinnedDomainCert;

    // Domain CA cert, if any, from EST /crts obtained during this session or from own Trust Store.
    mbedtls_x509_crt mDomainCaCert;

    // LDevID operational cert, if any, obtained with EST (/sen, /sren) during this session
    mbedtls_x509_crt mOperationalCert;

    // LDevID operational public key generated during this session.
    KeyInfo mOperationalKey;

    mbedtls_entropy_context mEntropyContext;
};

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_CCM_ENABLE

#endif // CBRSKI_CLIENT_HPP_
