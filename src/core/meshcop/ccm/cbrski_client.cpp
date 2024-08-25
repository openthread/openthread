/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements the cBRSKI client and necessary EST-coaps client functions.
 */

#include "cbrski_client.hpp"
#if OPENTHREAD_CONFIG_CCM_ENABLE

#include <openthread/platform/entropy.h>

#include "cbor.hpp"
#include "cose.hpp"
#include "credentials.hpp"
#include "common/debug.hpp"
#include "instance/instance.hpp"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/x509_csr.h"
#include "meshcop/meshcop.hpp"
#include "thread/key_manager.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("cBrskiClient");

static int cbrski_ctr_drbg_random_func(void *aData, unsigned char *aOutput, size_t aInLen);
static int cbrski_ctr_drbg_random_func_2(void *aData, unsigned char *aOutput, size_t aInLen, size_t *aOutLen);

CbrskiClient::CbrskiClient(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mCoapSecure(nullptr)
    , mIsDoingReenroll(false)
    , mVoucherReq(nullptr)
{
    mbedtls_x509_crt_init(&mRegistrarCert);
    mbedtls_x509_crt_init(&mPinnedDomainCert);
    mbedtls_x509_crt_init(&mDomainCaCert);
    mbedtls_x509_crt_init(&mOperationalCert);
    mbedtls_entropy_init(&mEntropyContext);
}

void CbrskiClient::StartEnroll(Coap::CoapSecure &aConnectedCoapSecure, otJoinerCallback aCallback, void *aContext)
{
    Error error;

    VerifyOrExit(aConnectedCoapSecure.IsConnected(), error = kErrorInvalidArgs);
    VerifyOrExit(!IsBusy(), error = kErrorInvalidState);

    mCoapSecure      = &aConnectedCoapSecure;
    mIsDoingReenroll = Get<Credentials>().HasOperationalCert();
    mCallback.Set(aCallback, aContext);

    mbedtls_x509_crt_init(&mRegistrarCert);
    mbedtls_x509_crt_init(&mPinnedDomainCert);
    mbedtls_x509_crt_init(&mDomainCaCert);
    mbedtls_x509_crt_init(&mOperationalCert);
    mbedtls_entropy_init(&mEntropyContext);
    // FIXME: 16 as constant; check why duplicate code with crypto_platform.cpp for creating source.
    mbedtls_entropy_add_source(&mEntropyContext, cbrski_ctr_drbg_random_func_2, nullptr, 16,
                               MBEDTLS_ENTROPY_SOURCE_STRONG);

    if (mIsDoingReenroll)
    {
        SuccessOrExit(error = SendEnrollRequest());
    }
    else
    {
        SuccessOrExit(error = SendVoucherRequest());
    }

exit:
    if (error != kErrorNone)
    {
        Finish(error, /* aInvokeCallback */ true); // free resources, if VR-send fails. No more response comes.
    }
}

void CbrskiClient::Finish(Error aError, bool aInvokeCallback)
{
    if (mCoapSecure != nullptr)
    {
        LogDebg("client finish - err=%s", ErrorToString(aError));

        // Reset and release possibly acquired resources.
        mCoapSecure      = nullptr;
        mIsDoingReenroll = false;
        Instance::GetHeap().Free(mVoucherReq);
        mVoucherReq = nullptr; // TODO check if needs to be freed.
        mbedtls_x509_crt_free(&mRegistrarCert);
        mbedtls_x509_crt_free(&mPinnedDomainCert);
        mbedtls_x509_crt_free(&mDomainCaCert);
        mbedtls_x509_crt_free(&mOperationalCert);
        mbedtls_entropy_free(&mEntropyContext);

        if (aInvokeCallback)
        {
            mCallback.InvokeIfSet(aError);
        }
        mCallback.Clear(); // TODO check if needed
    }
}

Error CbrskiClient::SendVoucherRequest()
{
    Error          error;
    Coap::Message *message;
    uint8_t        registrarCert[Credentials::kMaxCertLength];
    size_t         registrarCertLength;
    uint8_t        signedVoucherBuf[kMaxVoucherSize];
    size_t         signedVoucherLength;

    VerifyOrExit((message = mCoapSecure->NewMessage()) != nullptr, error = kErrorNoBufs);

    message->Init(Coap::kTypeConfirmable, Coap::kCodePost);
    message->AppendUriPathOptions(OT_URI_PATH_JOINER_REQUEST_VOUCHER);
    message->AppendContentFormatOption(OT_COAP_OPTION_CONTENT_FORMAT_COSE_SIGN1);
    message->AppendUintOption(OT_COAP_OPTION_ACCEPT, OT_COAP_OPTION_CONTENT_FORMAT_COSE_SIGN1);
    message->SetPayloadMarker();
    message->SetOffset(message->GetLength());

    SuccessOrExit(error = mCoapSecure->GetDtls().GetPeerCertificateDer(registrarCert, &registrarCertLength,
                                                                       sizeof(registrarCert)));
    VerifyOrExit(mbedtls_x509_crt_parse_der(&mRegistrarCert, registrarCert, registrarCertLength) == 0,
                 error = kErrorParse);
    OT_ASSERT(mVoucherReq == nullptr);
    mVoucherReq = reinterpret_cast<VoucherRequest *>(Instance::GetHeap().CAlloc(1, sizeof(*mVoucherReq)));
    VerifyOrExit(mVoucherReq != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = CreateVoucherRequest(*mVoucherReq, mRegistrarCert));
    SuccessOrExit(
        error = SignVoucherRequest(signedVoucherBuf, signedVoucherLength, sizeof(signedVoucherBuf), *mVoucherReq));
    SuccessOrExit(error = message->AppendBytes(signedVoucherBuf, signedVoucherLength));

    SuccessOrExit(error = mCoapSecure->SendMessage(*message, &HandleVoucherResponse, this));

exit:
    LogDebg("SendVoucherRequest() done, err=%s", ErrorToString(error));

    if (error != kErrorNone && message != nullptr)
    {
        message->Free();
    }
    return error;
}

Error CbrskiClient::CreateVoucherRequest(VoucherRequest &aVoucherReq, mbedtls_x509_crt &aRegistrarCert)
{
    Error  error;
    size_t keyLen;

    aVoucherReq.mAssertion = kProximity;

    SuccessOrExit(error = Random::Crypto::FillBuffer(aVoucherReq.mNonce, sizeof(aVoucherReq.mNonce)));
    SuccessOrExit(error = Get<Credentials>().GetManufacturerSerialNumber(aVoucherReq.mSerialNumber,
                                                                         sizeof(aVoucherReq.mSerialNumber)));
    keyLen = mbedtls_pk_write_pubkey_der(&aRegistrarCert.pk, aVoucherReq.mRegPubKey, sizeof(aVoucherReq.mRegPubKey));
    VerifyOrExit(keyLen > 0, error = kErrorNoBufs);

    memmove(aVoucherReq.mRegPubKey, aVoucherReq.mRegPubKey + sizeof(aVoucherReq.mRegPubKey) - keyLen, keyLen);
    aVoucherReq.mRegPubKeyLength = keyLen;

exit:
    return error;
}

Error CbrskiClient::SignVoucherRequest(uint8_t              *aBuf,
                                       size_t               &aLength,
                                       size_t                aBufLength,
                                       const VoucherRequest &aVoucherReq)
{
    Error              error;
    uint8_t            voucherBuf[kMaxVoucherSize];
    size_t             voucherLength;
    CoseSignObject     sign1Msg;
    size_t             rawManufacturerKeyLength;
    const uint8_t     *rawManufacturerKey;
    mbedtls_pk_context manufacturerKey;

    mbedtls_pk_init(&manufacturerKey);

    rawManufacturerKey = Get<Credentials>().GetManufacturerPrivateKey(rawManufacturerKeyLength);

    VerifyOrExit(mbedtls_pk_parse_key(&manufacturerKey, rawManufacturerKey, rawManufacturerKeyLength, nullptr, 0,
                                      Crypto::MbedTls::CryptoSecurePrng, nullptr) == 0,
                 error = kErrorParse);

    SuccessOrExit(error = SerializeVoucherRequest(voucherBuf, voucherLength, sizeof(voucherBuf), aVoucherReq));

    SuccessOrExit(error = sign1Msg.Init(COSE_INIT_FLAGS_NONE, &Crypto::MbedTls::CryptoSecurePrng));
    SuccessOrExit(error = sign1Msg.SetContent(voucherBuf, voucherLength));
    SuccessOrExit(error =
                      sign1Msg.AddAttribute(COSE_Header_Algorithm, COSE_Algorithm_ECDSA_SHA_256, COSE_PROTECT_ONLY));
    SuccessOrExit(error = sign1Msg.Sign(manufacturerKey));
    SuccessOrExit(error = sign1Msg.Serialize(aBuf, aLength, aBufLength));

exit:
    return error;
}

Error CbrskiClient::SerializeVoucherRequest(uint8_t              *aBuf,
                                            size_t               &aLength,
                                            size_t                aMaxLength,
                                            const VoucherRequest &aVoucherReq)
{
    Error   error;
    CborMap voucher;
    CborMap container;
    int     key;

    SuccessOrExit(error = voucher.Init());
    SuccessOrExit(error = container.Init());

    // Assertion
    key = VoucherRequestSID::kAssertion - VoucherRequestSID::kVoucher;
    SuccessOrExit(error = container.Put(key, aVoucherReq.mAssertion));

    // Nonce
    key = VoucherRequestSID::kNonce - VoucherRequestSID::kVoucher;
    SuccessOrExit(error = container.Put(key, aVoucherReq.mNonce, sizeof(aVoucherReq.mNonce)));

    // Serial-Number
    key = VoucherRequestSID::kSerialNumber - VoucherRequestSID::kVoucher;
    SuccessOrExit(error = container.Put(key, aVoucherReq.mSerialNumber));

    // Proximity registrar subject public key info
    key = VoucherRequestSID::kProxRegistrarSPKI - VoucherRequestSID::kVoucher;
    SuccessOrExit(error = container.Put(key, aVoucherReq.mRegPubKey, aVoucherReq.mRegPubKeyLength));

    key = VoucherRequestSID::kVoucher;
    SuccessOrExit(error = voucher.Put(key, container));

    SuccessOrExit(error = voucher.Serialize(aBuf, aLength, aMaxLength));

exit:
    container.Free();
    voucher.Free();
    return error;
}

void CbrskiClient::HandleVoucherResponse(void                *aContext,
                                         otMessage           *aMessage,
                                         const otMessageInfo *aMessageInfo,
                                         Error                aResult)
{
    static_cast<CbrskiClient *>(aContext)->HandleVoucherResponse(
        *static_cast<Coap::Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void CbrskiClient::HandleVoucherResponse(Coap::Message &aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult)
{
    Error   error     = kErrorFailed;
    Error   enrollErr = kErrorNone;
    uint8_t voucherBuf[kMaxVoucherSize];
    size_t  voucherLength;
    OT_UNUSED_VARIABLE(aMessageInfo);

    VerifyOrExit(aResult == kErrorNone && aMessage.GetCode() == OT_COAP_CODE_CHANGED);

    voucherLength = aMessage.GetLength() - aMessage.GetOffset();
    VerifyOrExit(voucherLength <= sizeof(voucherBuf));

    VerifyOrExit(voucherLength == aMessage.ReadBytes(aMessage.GetOffset(), voucherBuf, voucherLength));

    SuccessOrExit(error = ProcessVoucher(voucherBuf, voucherLength));

    SuccessOrExit(enrollErr = SendEnrollRequest()); // not assigned to 'error' - if fails: report error as below.

    error = kErrorNone;

exit:
    LogWarn("Handle Voucher resp - err=%s code=%d.%02d", ErrorToString(error), aMessage.GetCode() >> 5,
            aMessage.GetCode() & 0x1F);
    ReportStatusTelemetry(OT_URI_PATH_JOINER_VOUCHER_STATUS, error, "validating voucher");

    if (enrollErr != kErrorNone)
    {
        ReportStatusTelemetry(OT_URI_PATH_JOINER_ENROLL_STATUS, enrollErr, "sending enroll req");
    }
    if (error != kErrorNone || enrollErr != kErrorNone)
    {
        Finish(error, /* aInvokeCallback */ true);
    }
}

void CbrskiClient::ReportStatusTelemetry(const char *aUri, Error aError, const char *aContext)
{
    Error          error;
    Coap::Message *message;
    CborMap        status;
    uint8_t        statusBuf[256];
    size_t         statusLength;
    bool           reportStatusSuccess = (aError == kErrorNone);

    VerifyOrExit((message = mCoapSecure->NewMessage()) != nullptr, error = kErrorNoBufs);

    message->Init(Coap::kTypeConfirmable, Coap::kCodePost);
    SuccessOrExit(error = message->AppendUriPathOptions(aUri));
    SuccessOrExit(error = message->AppendContentFormatOption(OT_COAP_OPTION_CONTENT_FORMAT_CBOR));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = status.Init());
    SuccessOrExit(error = status.Put("version", 1));
    SuccessOrExit(error = status.Put("status", reportStatusSuccess));
    if (!reportStatusSuccess)
    {
        SuccessOrExit(error = status.Put("reason", otThreadErrorToString(aError)));
        SuccessOrExit(error = status.Put("reason-context", aContext));
    }
    SuccessOrExit(error = status.Serialize(statusBuf, statusLength, sizeof(statusBuf)));
    SuccessOrExit(error = message->AppendBytes(statusBuf, statusLength));
    message->SetOffset(message->GetLength());

    SuccessOrExit(error = mCoapSecure->SendMessage(*message, nullptr, nullptr));
    LogInfo("Joiner sent status report '%s': status=%d", aUri, reportStatusSuccess);

exit:
    if (error != kErrorNone && message != nullptr)
    {
        message->Free();
    }
    status.Free();

    if (error != kErrorNone)
    {
        LogWarn("Joiner status report '%s' failed: %s", aUri, otThreadErrorToString(error));
    }
}

Error CbrskiClient::ProcessVoucher(const uint8_t *aVoucher, size_t aVoucherLength)
{
    Error            error, errorGetIdevidIssuer;
    const uint8_t   *cert;
    size_t           certLen = 0;
    const uint8_t   *rawVoucher;
    size_t           voucherLen;
    int              key;
    int              assertion;
    const uint8_t   *nonce;
    size_t           nonceLength;
    const char      *serialNumber;
    size_t           serialNumberLen;
    const uint8_t   *idevidIssuer;
    size_t           idevidIssuerLen;
    CoseSignObject   coseSign;
    CborMap          voucher, container;
    mbedtls_x509_crt manufacturerCACert;
    uint32_t         certVerifyResultFlags;
    int              numChecksPassed = 0;

    mbedtls_x509_crt_init(&manufacturerCACert);
    cert = Get<Credentials>().GetManufacturerCACert(certLen); // sets certLen
    OT_ASSERT(cert != nullptr);
    numChecksPassed++;

    VerifyOrExit(mbedtls_x509_crt_parse(&manufacturerCACert, cert, certLen) == 0, error = kErrorSecurity);
    numChecksPassed++;

    SuccessOrExit(error = CoseSignObject::Deserialize(coseSign, aVoucher, aVoucherLength));
    numChecksPassed++;
    SuccessOrExit(error = coseSign.Validate(manufacturerCACert.pk));
    numChecksPassed++;

    rawVoucher = coseSign.GetPayload(voucherLen);
    VerifyOrExit(rawVoucher != nullptr, error = kErrorParse);
    numChecksPassed++;

    SuccessOrExit(error = CborValue::Deserialize(voucher, rawVoucher, voucherLen));
    numChecksPassed++;

    // Open the CBOR 'Voucher' container
    // note: remaining code assumes always short/compressed SIDs using delta encoding are used.
    //   In general this won't parse all Voucher formats. But we can apply it here because MASA
    //   (of same vendor) controls the voucher generation to use delta encoding.
    SuccessOrExit(error = voucher.Get(VoucherSID::kVoucher, container));
    numChecksPassed++;

    // Assertion
    key = VoucherSID::kAssertion - VoucherSID::kVoucher;
    SuccessOrExit(error = container.Get(key, assertion));
    numChecksPassed++;
    //    verify Voucher's assertion equals assertion in the Voucher Request, check if correct TODO(EskoDijk)
    VerifyOrExit(assertion == mVoucherReq->mAssertion, error = kErrorSecurity);
    numChecksPassed++;

    // Nonce
    key = VoucherSID::kNonce - VoucherSID::kVoucher;
    SuccessOrExit(error = container.Get(key, nonce, nonceLength));
    numChecksPassed++;
    //    verify Voucher's Nonce equals the Voucher Request Nonce.
    VerifyOrExit(nonceLength == sizeof(mVoucherReq->mNonce) && memcmp(nonce, mVoucherReq->mNonce, nonceLength) == 0,
                 error = kErrorSecurity);
    numChecksPassed++;

    // Serial-Number
    key = VoucherSID::kSerialNumber - VoucherSID::kVoucher;
    SuccessOrExit(error = container.Get(key, serialNumber, serialNumberLen));
    numChecksPassed++;
    VerifyOrExit(serialNumberLen == strlen(mVoucherReq->mSerialNumber) &&
                     memcmp(serialNumber, mVoucherReq->mSerialNumber, serialNumberLen) == 0,
                 error = kErrorSecurity);
    numChecksPassed++;

    // Idevid-Issuer field is optionally present. If present, MUST be verified.
    key                  = VoucherSID::kIdevidIssuer - VoucherSID::kVoucher;
    errorGetIdevidIssuer = container.Get(key, idevidIssuer, idevidIssuerLen);
    numChecksPassed++;
    if (errorGetIdevidIssuer == kErrorNone)
    {
        uint8_t authKeyId[Credentials::kMaxKeyIdentifierLength];
        size_t  authKeyIdLength;

        // Match idevid-issuer against Authority Key Identifier.
        SuccessOrExit(error = Get<Credentials>().GetAuthorityKeyId(authKeyId, authKeyIdLength, sizeof(authKeyId)));
        numChecksPassed++;
        VerifyOrExit(idevidIssuerLen == authKeyIdLength && memcmp(idevidIssuer, authKeyId, authKeyIdLength) == 0,
                     error = kErrorSecurity);
    }
    else
    {
        VerifyOrExit(errorGetIdevidIssuer == kErrorNotFound, error = kErrorParse);
        numChecksPassed++;
    }
    numChecksPassed++;

    // Pinned-Domain-Cert: is either a Domain CA cert (signer CA of Registrar) or Domain Registrar's EE (RA) cert
    // itself. the reason we know it's either one of these two is because the MASA (operated by same vendor as this
    // device) controls the choice.
    key = VoucherSID::kPinnedDomainCert - VoucherSID::kVoucher;
    SuccessOrExit(error = container.Get(key, cert, certLen));
    numChecksPassed++;
    VerifyOrExit(mbedtls_x509_crt_parse_der(&mPinnedDomainCert, cert, certLen) == 0, error = kErrorParse);
    numChecksPassed++;

    //   verify the Registrar's cert against the mDomainCert Trust Anchor (TA) provided in the Voucher.
    //   The TA MAY be equal to the Registrar's cert and this is also ok.
    VerifyOrExit(mbedtls_x509_crt_verify(&mRegistrarCert, &mPinnedDomainCert, nullptr, nullptr, &certVerifyResultFlags,
                                         nullptr, nullptr) == 0,
                 error = kErrorSecurity);
    numChecksPassed++;

exit:
    LogDebg("Process Voucher - err=%s pass=%i", ErrorToString(error), numChecksPassed);
    container.Free();
    voucher.Free();
    coseSign.Free();
    mbedtls_x509_crt_free(&manufacturerCACert);
    Instance::GetHeap().Free(mVoucherReq); // free the voucher req after we're done with the Voucher.
    mVoucherReq = nullptr;

    return error;
}

Error CbrskiClient::SendCaCertsRequest()
{
    Error          error;
    Coap::Message *message;

    VerifyOrExit((message = mCoapSecure->NewMessage()) != nullptr, error = kErrorNoBufs);

    message->Init(Coap::kTypeConfirmable, Coap::kCodeGet);
    SuccessOrExit(error = message->AppendUriPathOptions(OT_URI_PATH_JOINER_CA_CERTS));
    SuccessOrExit(error = message->AppendUintOption(OT_COAP_OPTION_ACCEPT, 287)); // FIXME const and content type
    message->SetOffset(message->GetLength());

    SuccessOrExit(error = mCoapSecure->SendMessage(*message, HandleCaCertsResponse, this));

exit:
    LogInfo("Send CAcerts req - err=%s", ErrorToString(error));
    if (error != kErrorNone && message != nullptr)
    {
        message->Free();
    }
    return error;
}

void CbrskiClient::HandleCaCertsResponse(void                *aContext,
                                         otMessage           *aMessage,
                                         const otMessageInfo *aMessageInfo,
                                         Error                aResult)
{
    static_cast<CbrskiClient *>(aContext)->HandleCaCertsResponse(
        *static_cast<Coap::Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void CbrskiClient::HandleCaCertsResponse(Coap::Message &aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    int     numChecksPassed = 0;
    Error   error           = kErrorFailed;
    uint8_t cert[Credentials::kMaxCertLength];
    size_t  certLen;

    // verify no prior CoAP error(s) and CoAP response is 2.05 Content
    VerifyOrExit(aResult == kErrorNone && aMessage.GetCode() == OT_COAP_CODE_CONTENT);
    numChecksPassed++;

    // get CA cert bytes from payload
    certLen = aMessage.GetLength() - aMessage.GetOffset();
    VerifyOrExit(certLen > 0 && certLen <= Credentials::kMaxCertLength);
    numChecksPassed++;

    VerifyOrExit(certLen == aMessage.ReadBytes(aMessage.GetOffset(), &cert, certLen));
    numChecksPassed++;

    // parse bytes as X509 cert into 'mDomainCACert'.
    VerifyOrExit(mbedtls_x509_crt_parse_der(&mDomainCaCert, cert, certLen) == 0, error = kErrorParse);
    numChecksPassed++;

    SuccessOrExit(error = ProcessCertsIntoTrustStore());
    numChecksPassed++;

exit:
    LogDebg("Handle CAcerts resp - err=%s pass=%i", ErrorToString(error), numChecksPassed);
    // we are done after final verification and storage.
    Finish(error, /* aInvokeCallback */ true);
}

Error CbrskiClient::ProcessCertsIntoTrustStore()
{
    Error    error;
    int      numChecksPassed = 0;
    uint32_t certVerifyResultFlags;

    // store my new LDevID, its private key, and Domain CA cert.
    SuccessOrExit(error = Get<Credentials>().SetOperationalCert(mOperationalCert.raw.p, mOperationalCert.raw.len));
    numChecksPassed++;
    SuccessOrExit(error = Get<Credentials>().SetDomainCACert(mDomainCaCert.raw.p, mDomainCaCert.raw.len));
    numChecksPassed++;
    // Mbedtls writes the key to the end of the buffer
    VerifyOrExit(Get<Credentials>().SetOperationalPrivateKey(mOperationalKey) == kErrorNone);
    numChecksPassed++;

    // TODO(wgtdkp): trigger event: OT_CHANGED_OPERATIONAL_CERT.

    // verify if pinned-domain-cert is the signer of the Domain CA cert, and if so, store
    // pinned-domain-cert as top-level CA in the Trust Store. If not, then the
    // pinned-domain-cert is simply discarded and not used beyond the current enrollment process.
    if (!IsCertsEqual(&mDomainCaCert, &mPinnedDomainCert) && mbedtls_x509_crt_get_ca_istrue(&mPinnedDomainCert))
    {
        int v = mbedtls_x509_crt_verify(&mDomainCaCert, &mPinnedDomainCert, nullptr, nullptr, &certVerifyResultFlags,
                                        nullptr, nullptr);
        if (v == 0)
        {
            SuccessOrExit(
                error = Get<Credentials>().SetToplevelDomainCACert(mPinnedDomainCert.raw.p, mPinnedDomainCert.raw.len));
            LogInfo("Stored toplevel Domain CA cert");
        }
    }
    numChecksPassed++;

exit:
    LogDebg("Store LDevID/certs in trust store - err=%s pass=%i", ErrorToString(error), numChecksPassed);
    return error;
}

Error CbrskiClient::SendEnrollRequest()
{
    Error          error;
    char           subjectName[Credentials::kMaxSubjectNameLength];
    Coap::Message *message;
    unsigned char  csrData[kMaxCsrSize];
    size_t         csrDataLen;
    int            numChecksPassed = 0;

    VerifyOrExit((message = mCoapSecure->NewMessage()) != nullptr, error = kErrorNoBufs);
    numChecksPassed++;
    message->Init(Coap::kTypeConfirmable, Coap::kCodePost);

    SuccessOrExit(error = message->AppendUriPathOptions(mIsDoingReenroll ? OT_URI_PATH_JOINER_REENROLL
                                                                         : OT_URI_PATH_JOINER_ENROLL));
    numChecksPassed++;
    SuccessOrExit(error = message->AppendContentFormatOption(OT_COAP_OPTION_CONTENT_FORMAT_PKCS10));
    numChecksPassed++;
    SuccessOrExit(error = message->AppendUintOption(OT_COAP_OPTION_ACCEPT, OT_COAP_OPTION_CONTENT_FORMAT_PKIX_CERT));
    numChecksPassed++;
    SuccessOrExit(error = message->SetPayloadMarker());
    numChecksPassed++;
    message->SetOffset(message->GetLength());

    // Always generate new operational key for LDevID, for both enroll and reenroll.
    SuccessOrExit(error = mOperationalKey.Generate());
    numChecksPassed++;

    SuccessOrExit(error = Get<Credentials>().GetManufacturerSubjectName(subjectName, sizeof(subjectName)));
    numChecksPassed++;

    SuccessOrExit(error = CreateCsrData(mOperationalKey, subjectName, csrData, sizeof(csrData), csrDataLen));
    numChecksPassed++;

    OT_ASSERT(csrDataLen <= sizeof(csrData));

    SuccessOrExit(error = message->AppendBytes(&csrData[sizeof(csrData) - csrDataLen], csrDataLen));
    numChecksPassed++;

    SuccessOrExit(error = mCoapSecure->SendMessage(*message, HandleEnrollResponse, this));
    numChecksPassed++;

exit:
    LogInfo("Send Enroll req - err=%s pass=%d", ErrorToString(error), numChecksPassed);

    if (error != kErrorNone)
    {
        if (message != nullptr)
        {
            message->Free();
        }
        ReportStatusTelemetry(OT_URI_PATH_JOINER_ENROLL_STATUS, error, "send enroll req");
    }
    return error;
}

void CbrskiClient::HandleEnrollResponse(void                *aContext,
                                        otMessage           *aMessage,
                                        const otMessageInfo *aMessageInfo,
                                        Error                aResult)
{
    static_cast<CbrskiClient *>(aContext)->HandleEnrollResponse(
        *static_cast<Coap::Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void CbrskiClient::HandleEnrollResponse(Coap::Message &aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    uint8_t cert[Credentials::kMaxCertLength];
    size_t  certLen;
    Error   error                = kErrorFailed;
    int     numChecksPassed      = 0;
    bool    isNeedCaCertsRequest = true;

    VerifyOrExit(aResult == kErrorNone && aMessage.GetCode() == OT_COAP_CODE_CHANGED);
    numChecksPassed++;

    // TODO(wgtdkp): verify content format equals OT_COAP_OPTION_CONTENT_FORMAT_PKIX_CERT

    certLen = aMessage.GetLength() - aMessage.GetOffset();
    VerifyOrExit(certLen > 0 && certLen <= Credentials::kMaxCertLength);
    numChecksPassed++;

    VerifyOrExit(certLen == aMessage.ReadBytes(aMessage.GetOffset(), cert, certLen));
    numChecksPassed++;

    // process the new LDevID cert, and determine if I need to do additional cacerts request (isNeedCaCertsRequest will
    // be set)
    SuccessOrExit(error = ProcessOperationalCert(cert, certLen, isNeedCaCertsRequest));
    numChecksPassed++;

    // if cert not correct, assume we need a new Domain CA cert - fetch it using /crts, and then later verify again.
    if (isNeedCaCertsRequest)
    {
        SuccessOrExit(error = SendCaCertsRequest());
    }
    else
    {
        SuccessOrExit(error = ProcessCertsIntoTrustStore());
    }
    numChecksPassed++;

    error = kErrorNone;

exit:
    ReportStatusTelemetry(OT_URI_PATH_JOINER_ENROLL_STATUS, error, "validating LDevID");
    LogDebg("Enroll response LDevID processed - err=%s pass=%i needCaCertsReq=%d", ErrorToString(error),
            numChecksPassed, isNeedCaCertsRequest);
    if (!isNeedCaCertsRequest || error != kErrorNone)
    {
        Finish(error, /* aInvokeCallback */ true);
    }
}

Error CbrskiClient::ProcessOperationalCert(const uint8_t *aCert, size_t aLength, bool &isNeedCaCertsRequest)
{
    Error    error = kErrorSecurity;
    uint32_t certVerifyResultFlags;
    int      numChecksPassed = 0;
    int      mbedtlsErrorCode;

    LogDebg("Validating new LDevID cert - len=%luB", aLength);

    VerifyOrExit((mbedtlsErrorCode = mbedtls_x509_crt_parse_der(&mOperationalCert, aCert, aLength)) == 0);
    numChecksPassed++;

    // TODO(wgtdkp): match the certificate against CSR

    // TODO(wgtdkp): make sure mOperationalKey matches public key in the cert

    // TODO(wgtdkp): set expected Common Name.

    // Verify new LDevID cert against my existing Domain CA TA, in case of re-enroll. And in
    // case of enroll, check if the pinned-domain-cert is already good to be used as Domain CA.
    // The outcomes determine whether a next CA Certs request is still needed or not.
    isNeedCaCertsRequest = true;
    if (mIsDoingReenroll)
    {
        mbedtlsErrorCode = mbedtls_x509_crt_verify(&mOperationalCert, &mDomainCaCert, nullptr, nullptr,
                                                   &certVerifyResultFlags, nullptr, nullptr);
        if (mbedtlsErrorCode == 0)
            isNeedCaCertsRequest = false;
    }
    else
    {
        if (mbedtls_x509_crt_get_ca_istrue(&mPinnedDomainCert))
        {
            mbedtlsErrorCode = mbedtls_x509_crt_verify(&mOperationalCert, &mPinnedDomainCert, nullptr, nullptr,
                                                       &certVerifyResultFlags, nullptr, nullptr);
            if (mbedtlsErrorCode == 0)
            {
                isNeedCaCertsRequest = false;
                // set the pinned-cert as the domain ca cert, by parsing it (again).
                VerifyOrExit((mbedtlsErrorCode = mbedtls_x509_crt_parse_der(&mDomainCaCert, mPinnedDomainCert.raw.p,
                                                                            mPinnedDomainCert.raw.len)) == 0);
                // PrintEncodedCert(mDomainCaCert.raw.p, mDomainCaCert.raw.len); // used for debug.
            }
        }
    }
    numChecksPassed++;
    error = kErrorNone;

exit:
    LogDebg("Validation done - err=%s pass=%i mbedCode=%i", ErrorToString(error), numChecksPassed, mbedtlsErrorCode);
    PrintEncodedCert(aCert, aLength); // TODO: only make this call if log level is 'debug', to reduce code.
    return error;
}

// TODO verify that platform call gives sufficient quality PRNG (assumes ctr_drbg method but may depend on flags)
static int cbrski_ctr_drbg_random_func(void *aData, unsigned char *aOutput, size_t aInLen)
{
    int rval = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;

    SuccessOrExit(otPlatEntropyGet(reinterpret_cast<uint8_t *>(aOutput), static_cast<uint16_t>(aInLen)));
    rval = 0;

exit:
    OT_UNUSED_VARIABLE(aData);
    return rval;
}

// TODO verify that platform call gives sufficient quality PRNG (assumes ctr_drbg method but may depend on flags)
static int cbrski_ctr_drbg_random_func_2(void *aData, unsigned char *aOutput, size_t aInLen, size_t *aOutLen)
{
    int rval = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;

    SuccessOrExit(otPlatEntropyGet(reinterpret_cast<uint8_t *>(aOutput), static_cast<uint16_t>(aInLen)));
    rval = 0;

    VerifyOrExit(aOutLen != nullptr);
    *aOutLen = aInLen;

exit:
    OT_UNUSED_VARIABLE(aData);
    return rval;
}

Error CbrskiClient::CreateCsrData(KeyInfo       &aKey,
                                  const char    *aSubjectName,
                                  unsigned char *aBuf,
                                  size_t         aBufLen,
                                  size_t        &aCsrLen)
{
    Error                    error = kErrorSecurity;
    mbedtls_x509write_csr    csr;
    mbedtls_ctr_drbg_context ctrDrbg;
    otExtAddress             eui64;
    ssize_t                  length;
    mbedtls_pk_context       pk;
    int                      mbedErr = 0;

    mbedtls_pk_init(&pk);
    mbedtls_x509write_csr_init(&csr);
    mbedtls_ctr_drbg_init(&ctrDrbg);

    SuccessOrExit(mbedtls_pk_parse_key(&pk, aKey.GetDerBytes(), aKey.GetDerLength(), nullptr, 0,
                                       cbrski_ctr_drbg_random_func, &mEntropyContext));
    mbedtls_x509write_csr_set_md_alg(&csr, MBEDTLS_MD_SHA256);

    // if( opt.key_usage )
    //     mbedtls_x509write_csr_set_key_usage( &req, opt.key_usage );

    // if( opt.ns_cert_type )
    //     mbedtls_x509write_csr_set_ns_cert_type( &req, opt.ns_cert_type );

    otPlatRadioGetIeeeEui64(&GetInstance(), eui64.m8);
    SuccessOrExit(mbedErr = mbedtls_ctr_drbg_seed(&ctrDrbg, cbrski_ctr_drbg_random_func, &mEntropyContext, eui64.m8,
                                                  sizeof(eui64)));

    SuccessOrExit(mbedtls_x509write_csr_set_subject_name(&csr, aSubjectName));
    mbedtls_x509write_csr_set_key(&csr, &pk);

    // TODO document why ctr_drbg entropy source is needed extra (and not standard one)
    // cbrski_ctr_drbg_random_func
    length = mbedtls_x509write_csr_der(&csr, aBuf, aBufLen, cbrski_ctr_drbg_random_func, &mEntropyContext);
    if (length < 0)
    { // signals an error
        mbedErr = length;
    }
    VerifyOrExit(length > 0);

    aCsrLen = length;
    error   = kErrorNone;

exit:
    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctrDrbg);
    mbedtls_x509write_csr_free(&csr);
    LogDebg("CSR create - err=%s mbedCode=%d", ErrorToString(error), mbedErr);
    return error;
}

bool CbrskiClient::IsCertsEqual(const mbedtls_x509_crt *cert1, const mbedtls_x509_crt *cert2)
{
    if (cert1->raw.len != cert2->raw.len)
        return false;
    return (memcmp(cert1->raw.p, cert2->raw.p, cert1->raw.len) == 0);
}

static void LogDebgBytesInHex(const uint8_t *aBuf, size_t aBufLen)
{
    const size_t kMaxBufLen = 40;
    char         line[2 * kMaxBufLen + 1];

    OT_ASSERT(aBufLen <= kMaxBufLen);
    StringWriter writer(line, 2 * kMaxBufLen + 1);
    writer.AppendHexBytes(aBuf, aBufLen);
    writer.Append("\n");
    LogDebg("%s", line);
}

void CbrskiClient::PrintEncodedCert(const uint8_t *aCert, size_t aLength)
{
    size_t l = 40;

    LogDebg("PrintEncodedCert(len=%lu):", aLength);
    if (aLength == 0)
    {
        return;
    }
    for (size_t i = 0; i < aLength; i += 40)
    {
        if ((i + l) > aLength)
            l = aLength - i;
        LogDebgBytesInHex(aCert + i, l);
    }
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE && OPENTHREAD_CONFIG_CCM_ENABLE
