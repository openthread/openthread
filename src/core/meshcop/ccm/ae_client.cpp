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
 *   This file implements the EST Client.
 */

#include "ae_client.hpp"
#if OPENTHREAD_CONFIG_CCM_ENABLE

#include "cbor.hpp"
#include "cose.hpp"
#include "credentials.hpp"
#include "common/locator_getters.hpp"
#include "common/logging.hpp"
#include "common/debug.hpp"
#include "mbedtls/entropy.h"
#include "mbedtls/x509_csr.h"
#include "mbedtls/ctr_drbg.h"
#include "meshcop/meshcop.hpp"
#include "thread/key_manager.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("AeClient");

AeClient::AeClient(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mCoapSecure(NULL)
    , mCallback(NULL)
    , mCallbackContext(NULL)
    , mIsDoingReenroll(false)
    , mVoucherReq(NULL)
{
    mbedtls_x509_crt_init(&mRegistrarCert);
    mbedtls_x509_crt_init(&mPinnedDomainCert);
    mbedtls_x509_crt_init(&mDomainCaCert);
    mbedtls_x509_crt_init(&mOperationalCert);
    mbedtls_pk_init(&mOperationalKey);
    mbedtls_entropy_init(&mEntropyContext);
}

void AeClient::StartEnroll(Coap::CoapSecure &aConnectedCoapSecure, Callback aCallback, void *aContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aConnectedCoapSecure.IsConnected(), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(!IsBusy(), error = OT_ERROR_INVALID_STATE);

    mCoapSecure      = &aConnectedCoapSecure;
    mCallback        = aCallback;
    mCallbackContext = aContext;
    mIsDoingReenroll = false;

    mbedtls_x509_crt_init(&mRegistrarCert);
    mbedtls_x509_crt_init(&mPinnedDomainCert);
    mbedtls_x509_crt_init(&mDomainCaCert);
    mbedtls_x509_crt_init(&mOperationalCert);
    mbedtls_pk_init(&mOperationalKey);

    SuccessOrExit(error = SendVoucherRequest());

exit:
    if (error != OT_ERROR_NONE)
    {
        Finish(error);  // free resources when sending Voucher Request fails. No more response msg will come.
    }
    return;
}

void AeClient::Finish(otError aError)
{
    if (mCoapSecure != NULL)
    {
        mCoapSecure = NULL;
        if (mCallback != NULL)
        {
            mCallback(aError, mCallbackContext);
        }

        mCallback        = NULL;
        mCallbackContext = NULL;
        mIsDoingReenroll = false;

        // Release possibly acquired resources.
        GetInstance().GetHeap().Free(mVoucherReq);
        mVoucherReq = NULL;
        mbedtls_x509_crt_free(&mRegistrarCert);
        mbedtls_x509_crt_free(&mPinnedDomainCert);
        mbedtls_x509_crt_free(&mDomainCaCert);
        mbedtls_x509_crt_free(&mOperationalCert);
        mbedtls_pk_free(&mOperationalKey);
    }
}

otError AeClient::SendVoucherRequest()
{
    otError        error = OT_ERROR_NONE;
    Coap::Message *message;
    uint8_t        registrarCert[Credentials::kMaxCertLength];
    size_t         registrarCertLength;
    uint8_t        signedVoucherBuf[kMaxVoucherSize];
    size_t         signedVoucherLength;

    VerifyOrExit((message = mCoapSecure->NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(Coap::kTypeConfirmable, Coap::kCodePost);
    message->AppendUriPathOptions(PathForUri(kUriWellknownBrskiReqVoucher));
    message->AppendContentFormatOption(OT_COAP_OPTION_CONTENT_FORMAT_COSE_SIGN1);
    message->AppendUintOption(OT_COAP_OPTION_ACCEPT, OT_COAP_OPTION_CONTENT_FORMAT_COSE_SIGN1);
    message->SetPayloadMarker();
    message->SetOffset(message->GetLength());

    SuccessOrExit(error = mCoapSecure->GetPeerCertificateBase64(registrarCert, &registrarCertLength, sizeof(registrarCert)));
    VerifyOrExit(mbedtls_x509_crt_parse(&mRegistrarCert, registrarCert, registrarCertLength) == 0,
                 error = OT_ERROR_PARSE);

    OT_ASSERT(mVoucherReq == NULL);
    mVoucherReq = reinterpret_cast<VoucherRequest *>(GetInstance().GetHeap().CAlloc(1, sizeof(*mVoucherReq)));
    VerifyOrExit(mVoucherReq != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = CreateVoucherRequest(*mVoucherReq, mRegistrarCert));
    SuccessOrExit(error = SignVoucherRequest(signedVoucherBuf, signedVoucherLength, sizeof(signedVoucherBuf), *mVoucherReq));

    SuccessOrExit(error = message->AppendBytes(signedVoucherBuf, signedVoucherLength));

    SuccessOrExit(error = mCoapSecure->SendMessage(*message, &AeClient::HandleVoucherResponse, this));

exit:
	LogInfo("SendVoucherRequest() err=%i" , error);

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
    return error;
}

otError AeClient::CreateVoucherRequest(VoucherRequest &aVoucherReq, mbedtls_x509_crt &aRegistrarCert)
{
    otError error = OT_ERROR_NONE;
    size_t  keyLen;

    aVoucherReq.mAssertion = kProximity;

    SuccessOrExit(error = Random::Crypto::FillBuffer(aVoucherReq.mNonce, sizeof(aVoucherReq.mNonce)));

    error =
        Get<Credentials>().GetManufacturerSerialNumber(aVoucherReq.mSerialNumber, sizeof(aVoucherReq.mSerialNumber));
    VerifyOrExit(error == OT_ERROR_NONE);

    keyLen = mbedtls_pk_write_pubkey_der(&aRegistrarCert.pk, aVoucherReq.mRegPubKey, sizeof(aVoucherReq.mRegPubKey));
    VerifyOrExit(keyLen > 0, error = OT_ERROR_NO_BUFS);

    memmove(aVoucherReq.mRegPubKey, aVoucherReq.mRegPubKey + sizeof(aVoucherReq.mRegPubKey) - keyLen, keyLen);
    aVoucherReq.mRegPubKeyLength = keyLen;

exit:
    return error;
}

otError AeClient::SignVoucherRequest(uint8_t *aBuf, size_t &aLength, size_t aBufLength, const VoucherRequest &aVoucherReq)
{
    otError error = OT_ERROR_NONE;
    uint8_t        voucherBuf[kMaxVoucherSize];
    size_t         voucherLength;
    CoseSignObject sign1Msg;
    size_t rawManufacturerKeyLength;
    const uint8_t *rawManufacturerKey;
    mbedtls_pk_context manufacturerKey;

    mbedtls_pk_init(&manufacturerKey);

    rawManufacturerKey = Get<Credentials>().GetManufacturerPrivateKey(rawManufacturerKeyLength);

    VerifyOrExit(mbedtls_pk_parse_key(&manufacturerKey, rawManufacturerKey, rawManufacturerKeyLength, NULL, 0) == 0,
                 error = OT_ERROR_PARSE);

    SuccessOrExit(error = SerializeVoucherRequest(voucherBuf, voucherLength, sizeof(voucherBuf), aVoucherReq));

    SuccessOrExit(error = sign1Msg.Init(COSE_INIT_FLAGS_NONE));
    SuccessOrExit(error = sign1Msg.SetContent(voucherBuf, voucherLength));
    SuccessOrExit(error = sign1Msg.AddAttribute(COSE_Header_Algorithm, COSE_Algorithm_ECDSA_SHA_256, COSE_PROTECT_ONLY));
    SuccessOrExit(error = sign1Msg.Sign(manufacturerKey));
    SuccessOrExit(error = sign1Msg.Serialize(aBuf, aLength, aBufLength));

exit:
    return error;
}

otError AeClient::SerializeVoucherRequest(uint8_t *             aBuf,
                                           size_t &              aLength,
                                           size_t                aMaxLength,
                                           const VoucherRequest &aVoucherReq)
{
    otError error = OT_ERROR_NONE;
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

otError AeClient::GetPeerCertificate(mbedtls_x509_crt &aCert)
{
    otError error = OT_ERROR_NONE;
    uint8_t certBuf[Credentials::kMaxCertLength];
    size_t  certLength;

    SuccessOrExit(error = mCoapSecure->GetPeerCertificateBase64(certBuf, &certLength, sizeof(certBuf)));

    VerifyOrExit(mbedtls_x509_crt_parse(&aCert, certBuf, certLength) == 0, error = OT_ERROR_PARSE);

exit:
    return error;
}

void AeClient::HandleVoucherResponse(void *               aContext,
                                      otMessage *          aMessage,
                                      const otMessageInfo *aMessageInfo,
                                      otError              aResult)
{
    static_cast<AeClient *>(aContext)->HandleVoucherResponse(
        *static_cast<Coap::Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void AeClient::HandleVoucherResponse(Coap::Message &aMessage, const Ip6::MessageInfo *aMessageInfo, otError aResult)
{
    otError error = OT_ERROR_FAILED;
    uint8_t voucherBuf[kMaxVoucherSize];
    size_t  voucherLength;
    OT_UNUSED_VARIABLE(aMessageInfo);

    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage.GetCode() == OT_COAP_CODE_CHANGED);

    voucherLength = aMessage.GetLength() - aMessage.GetOffset();
    VerifyOrExit(voucherLength <= sizeof(voucherBuf));

    VerifyOrExit(voucherLength == aMessage.ReadBytes(aMessage.GetOffset(), voucherBuf, voucherLength));

    SuccessOrExit(error = ProcessVoucher(voucherBuf, voucherLength));

    SuccessOrExit(SendCsrRequest());

    error = OT_ERROR_NONE;

exit:
    // FIXME
#define OT_URI_PATH_JOINER_VOUCHER_STATUS ".well-known/brski/vs"
    ReportStatus(OT_URI_PATH_JOINER_VOUCHER_STATUS, error, "validating voucher");

    if (error != OT_ERROR_NONE)
    {
        LogWarn("HandleVoucherResponse() err=%i, CoAP-code=%d.%02d, [error = %s]",
        		         error,
        		         aMessage.GetCode() >> 5,
				 aMessage.GetCode() & 0x1F,
                                 otThreadErrorToString(error)); // FIXME
        Finish(error);
    }
}

void AeClient::ReportStatus(const char *aUri, otError aError, const char *aContext)
{
    otError        error = OT_ERROR_NONE;
    Coap::Message *message;
    CborMap status;
    uint8_t statusBuf[256];
    size_t  statusLength;

    VerifyOrExit((message = mCoapSecure->NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(Coap::kTypeConfirmable, Coap::kCodePost);
    SuccessOrExit(error = message->AppendUriPathOptions(aUri));
    SuccessOrExit(error = message->AppendContentFormatOption(OT_COAP_OPTION_CONTENT_FORMAT_CBOR));

    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = status.Init());
    SuccessOrExit(error = status.Put("version", 1));
    SuccessOrExit(error = status.Put("status", (aError == OT_ERROR_NONE) ));
    if (aError != OT_ERROR_NONE)
    {
        SuccessOrExit(error = status.Put("reason", otThreadErrorToString(aError)));
        SuccessOrExit(error = status.Put("reason-context", aContext));
    }

    SuccessOrExit(error = status.Serialize(statusBuf, statusLength, sizeof(statusBuf)));

    SuccessOrExit(error = message->AppendBytes(statusBuf, statusLength));

    message->SetOffset(message->GetLength());

    SuccessOrExit(error = mCoapSecure->SendMessage(*message, NULL, NULL));

    LogInfo("Joiner sent status report to %s", aUri);

exit:
    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
    status.Free();

    if (error != OT_ERROR_NONE)
    {
        LogWarn("Joiner sent status report to %s failed: %s", aUri, otThreadErrorToString(error));
    }
}

otError AeClient::ProcessVoucher(const uint8_t *aVoucher, size_t aVoucherLength)
{
    otError          error = OT_ERROR_NONE;
    const uint8_t *  cert;
    size_t           certLen = 0;
    const uint8_t *  rawVoucher;
    size_t           voucherLen;
    int              key;
    int              assertion;
    const uint8_t *  nonce;
    size_t           nonceLength;
    const char *     serialNumber;
    size_t           serialNumberLen;
    const uint8_t *  idevidIssuer;
    size_t           idevidIssuerLen;
    CoseSignObject   coseSign;
    CborMap          voucher;
    CborMap          container;
    mbedtls_x509_crt manufacturerCACert;
    uint32_t         certVerifyResultFlags;
    int              numChecksPassed = 0;
    
    mbedtls_x509_crt_init(&manufacturerCACert);
    cert = Get<Credentials>().GetManufacturerCACert(certLen);  // sets certLen
    OT_ASSERT(cert != NULL);
    numChecksPassed++;
    
    VerifyOrExit(mbedtls_x509_crt_parse(&manufacturerCACert, cert, certLen) == 0, error = OT_ERROR_SECURITY);
    numChecksPassed++;
    
    SuccessOrExit(error = CoseSignObject::Deserialize(coseSign, aVoucher, aVoucherLength));
    numChecksPassed++;
    SuccessOrExit(error = coseSign.Validate(manufacturerCACert.pk));
    numChecksPassed++;

    rawVoucher = coseSign.GetPayload(voucherLen);
    VerifyOrExit(rawVoucher != NULL, error = OT_ERROR_PARSE);
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
    VerifyOrExit(assertion == mVoucherReq->mAssertion, error = OT_ERROR_SECURITY);
    numChecksPassed++;
    
    // Nonce
    key = VoucherSID::kNonce - VoucherSID::kVoucher;
    SuccessOrExit(error = container.Get(key, nonce, nonceLength));
    numChecksPassed++;
    //    verify Voucher's Nonce equals the Voucher Request Nonce.
    VerifyOrExit(nonceLength == sizeof(mVoucherReq->mNonce) && memcmp(nonce, mVoucherReq->mNonce, nonceLength) == 0,
                 error = OT_ERROR_SECURITY);
    numChecksPassed++;

    // Serial-Number
    key = VoucherSID::kSerialNumber - VoucherSID::kVoucher;
    SuccessOrExit(error = container.Get(key, serialNumber, serialNumberLen));
    numChecksPassed++;
    VerifyOrExit(serialNumberLen == strlen(mVoucherReq->mSerialNumber) &&
                     memcmp(serialNumber, mVoucherReq->mSerialNumber, serialNumberLen) == 0,
                 error = OT_ERROR_SECURITY);
    numChecksPassed++;

    // Idevid-Issuer
    key = VoucherSID::kIdevidIssuer - VoucherSID::kVoucher;
    if (container.Get(key, idevidIssuer, idevidIssuerLen) == OT_ERROR_NONE)
    {
        uint8_t authKeyId[Credentials::kMaxKeyIdentifierLength];
        size_t  authKeyIdLength;

        // Match idevid-issuer against Authority Key Identifier.
        SuccessOrExit(error = Get<Credentials>().GetAuthorityKeyId(authKeyId, authKeyIdLength, sizeof(authKeyId)));
        numChecksPassed++;
        VerifyOrExit(idevidIssuerLen == authKeyIdLength && memcmp(idevidIssuer, authKeyId, authKeyIdLength) == 0,
                     error = OT_ERROR_SECURITY);
        numChecksPassed++;
    }else{
    	SuccessOrExit( error = OT_ERROR_SECURITY );
    }

    // Pinned-Domain-Cert: is either a Domain CA cert (signer CA of Registrar) or Domain Registrar's EE (RA) cert itself.
    // the reason we know it's either one of these two is because the MASA (operated by same vendor as this device) controls the choice.
    key = VoucherSID::kPinnedDomainCert - VoucherSID::kVoucher;
    SuccessOrExit(error = container.Get(key, cert, certLen));
    numChecksPassed++;
    VerifyOrExit(mbedtls_x509_crt_parse_der(&mPinnedDomainCert, cert, certLen) == 0,
                 error = OT_ERROR_PARSE);
    numChecksPassed++;

    //   verify the Registrar's cert against the mDomainCert Trust Anchor (TA) provided in the Voucher.
    //   The TA MAY be equal to the Registrar's cert and this is also ok.
    VerifyOrExit(mbedtls_x509_crt_verify(&mRegistrarCert, &mPinnedDomainCert, NULL, NULL, &certVerifyResultFlags, NULL, NULL) == 0,
                 error = OT_ERROR_SECURITY);
    numChecksPassed++;


exit:
    LogDebg("ProcessVoucher() err=%i, pass=%i", error, numChecksPassed ); // FIXME
    container.Free();
    voucher.Free();
    coseSign.Free();
    mbedtls_x509_crt_free(&manufacturerCACert);
    GetInstance().GetHeap().Free(mVoucherReq); // free the voucher req after we're done with the Voucher.
    mVoucherReq = NULL;

    return error;
}

otError AeClient::SendCaCertsRequest()
{
    otError        error = OT_ERROR_NONE;
    Coap::Message *message;

    VerifyOrExit((message = mCoapSecure->NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(Coap::kTypeConfirmable, Coap::kCodeGet);
    message->AppendUriPathOptions(".well-known/est/crts"); // FIXME error handling (file-wide check) and const
    message->AppendUintOption(OT_COAP_OPTION_ACCEPT, 287); // FIXME const and content type

    message->SetOffset(message->GetLength());

    SuccessOrExit(error = mCoapSecure->SendMessage(*message, HandleCaCertsResponse, this));

exit:
	LogInfo("SendCaCertsRequest() err=%i" , error);
    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
    return error;
}

void AeClient::HandleCaCertsResponse(void *               aContext,
                                      otMessage *          aMessage,
                                      const otMessageInfo *aMessageInfo,
                                      otError              aResult)
{
    static_cast<AeClient *>(aContext)->HandleCaCertsResponse(
        *static_cast<Coap::Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void AeClient::HandleCaCertsResponse(Coap::Message &aMessage, const Ip6::MessageInfo *aMessageInfo, otError aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
	int      numChecksPassed = 0;
    otError  error = OT_ERROR_FAILED;
    uint8_t  cert[Credentials::kMaxCertLength];
    size_t 	 certLen = 0;

    // verify no prior CoAP error(s) and CoAP response is 2.05 Content
    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage.GetCode() == OT_COAP_CODE_CONTENT );
    numChecksPassed++;

	// get CA cert bytes from payload
    certLen = aMessage.GetLength() - aMessage.GetOffset();
    VerifyOrExit(certLen > 0 && certLen <= Credentials::kMaxCertLength);
    numChecksPassed++;

	VerifyOrExit(certLen == aMessage.ReadBytes(aMessage.GetOffset(), &cert, certLen));
	numChecksPassed++;

	// parse bytes as X509 cert into 'mDomainCACert'.
	VerifyOrExit(mbedtls_x509_crt_parse_der(&mDomainCaCert, cert, certLen) == 0, error = OT_ERROR_PARSE);
	numChecksPassed++;

	SuccessOrExit(ProcessCertsIntoTrustStore());
	numChecksPassed++;

    error = OT_ERROR_NONE;

exit:
	LogDebg("HandleCaCertsResponse() err=%i, pass=%i", error, numChecksPassed ); // FIXME
    // we are done after final verification and storage.
    Finish(error);
}

otError AeClient::ProcessCertsIntoTrustStore()
{
	otError          error = OT_ERROR_NONE;
	int              numChecksPassed = 0;
	uint32_t         certVerifyResultFlags;
    uint8_t          key[Credentials::kMaxKeyLength];
    ssize_t          keyLength;

	// store my new LDevID, its private key, and Domain CA cert.
	SuccessOrExit(error = Get<Credentials>().SetOperationalCert(mOperationalCert.raw.p, mOperationalCert.raw.len));
	numChecksPassed++;
	SuccessOrExit(error = Get<Credentials>().SetDomainCACert(mDomainCaCert.raw.p, mDomainCaCert.raw.len));
	numChecksPassed++;
    VerifyOrExit((keyLength = mbedtls_pk_write_key_der(&mOperationalKey, key, sizeof(key))) > 0);
    numChecksPassed++;
    // Mbedtls writes the key to the end of the buffer
    VerifyOrExit(Get<Credentials>().SetOperationalPrivateKey(key + sizeof(key) - keyLength, keyLength) == OT_ERROR_NONE);
    numChecksPassed++;

    // TODO(wgtdkp): trigger event: OT_CHANGED_OPERATIONAL_CERT.

	// verify if pinned-domain-cert is the signer of the Domain CA cert, and if so, store
	// pinned-domain-cert as top-level CA in the Trust Store. If not, then the
	// pinned-domain-cert is simply discarded and not used beyond the current enrollment process.
    if (!IsCertsEqual(&mDomainCaCert, &mPinnedDomainCert) && mPinnedDomainCert.ca_istrue) {
		int v = mbedtls_x509_crt_verify(&mDomainCaCert, &mPinnedDomainCert, NULL, NULL, &certVerifyResultFlags, NULL, NULL) ;
		if (v==0) {
			SuccessOrExit(error = Get<Credentials>().SetToplevelDomainCACert( mPinnedDomainCert.raw.p, mPinnedDomainCert.raw.len));
			LogInfo("Stored toplevel Domain CA cert");
		}
    }
	numChecksPassed++;

exit:
	LogDebg("ProcessCertsIntoTrustStore() err=%i, pass=%i", error, numChecksPassed ); // FIXME
	return error;
}

otError AeClient::SendCsrRequest(/* aKeyPair */)
{
    otError        error = OT_ERROR_NONE;
    char           subjectName[Credentials::kMaxSubjectNameLength];
    Coap::Message *message;
    unsigned char  csrData[kMaxCsrSize];
    size_t         csrDataLen;

    VerifyOrExit((message = mCoapSecure->NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(Coap::kTypeConfirmable, Coap::kCodePost);
    // FIXME
#define OT_URI_PATH_JOINER_ENROLL ".well-known/est/sen"
#define OT_URI_PATH_JOINER_REENROLL ".well-known/est/sren"
    message->AppendUriPathOptions(mIsDoingReenroll ? OT_URI_PATH_JOINER_REENROLL : OT_URI_PATH_JOINER_ENROLL);
    message->AppendContentFormatOption(OT_COAP_OPTION_CONTENT_FORMAT_PKCS10);
    message->AppendUintOption(OT_COAP_OPTION_ACCEPT, OT_COAP_OPTION_CONTENT_FORMAT_PKIX_CERT);
    message->SetPayloadMarker();
    message->SetOffset(message->GetLength());

    // Always generate new operational key for LDevID, for both enroll and reenroll.
    SuccessOrExit(error = GenerateECKey(mOperationalKey));

    SuccessOrExit(error = Get<Credentials>().GetManufacturerSubjectName(subjectName, sizeof(subjectName)));

    SuccessOrExit(error = CreateCsrData(mOperationalKey, subjectName, csrData, sizeof(csrData), csrDataLen));

    OT_ASSERT(csrDataLen <= sizeof(csrData));

    SuccessOrExit(error = message->AppendBytes(&csrData[sizeof(csrData) - csrDataLen], csrDataLen));

    SuccessOrExit(error = mCoapSecure->SendMessage(*message, HandleCsrResponse, this));

exit:
	LogDebg("SendCsrRequest() err=%i" , error); // FIXME

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
    return error;
}

void AeClient::HandleCsrResponse(void *               aContext,
                                  otMessage *          aMessage,
                                  const otMessageInfo *aMessageInfo,
                                  otError              aResult)
{
    static_cast<AeClient *>(aContext)->HandleCsrResponse(*static_cast<Coap::Message *>(aMessage),
                                                          static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void AeClient::HandleCsrResponse(Coap::Message &aMessage, const Ip6::MessageInfo *aMessageInfo, otError aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    uint8_t  cert[Credentials::kMaxCertLength];
    size_t 	 certLen = 0;
    otError  error = OT_ERROR_FAILED;
    int      numChecksPassed = 0;
    bool     isNeedCaCertsRequest = true;

    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage.GetCode() == OT_COAP_CODE_CHANGED);
    numChecksPassed++;

    // TODO(wgtdkp): verify content format equals OT_COAP_OPTION_CONTENT_FORMAT_PKIX_CERT

    certLen = aMessage.GetLength() - aMessage.GetOffset();
    VerifyOrExit(certLen > 0 && certLen <= Credentials::kMaxCertLength);
    numChecksPassed++;

    VerifyOrExit(certLen == aMessage.ReadBytes(aMessage.GetOffset(), cert, certLen));
    numChecksPassed++;

    // process the new LDevID cert, and determine if I need to do additional cacerts request (isNeedCaCertsRequest will be set)
    SuccessOrExit(error = ProcessOperationalCert(cert, certLen, isNeedCaCertsRequest));
    numChecksPassed++;

	// if cert not correct, assume we need a new Domain CA cert - fetch it using /crts, and then later verify again.
	if (isNeedCaCertsRequest){
		SuccessOrExit( error = SendCaCertsRequest() );
	} else {
		SuccessOrExit( error = ProcessCertsIntoTrustStore() );
	}
	numChecksPassed++;

    error = OT_ERROR_NONE;

exit:
    // FIXME
#define OT_URI_PATH_JOINER_ENROLL_STATUS ".well-known/brski/es"
    ReportStatus(OT_URI_PATH_JOINER_ENROLL_STATUS, error, "validating LDevID");
    LogDebg("HandleCsrResponse() err=%i, pass=%i, isNeedCaCertsReq=%i",
    		error , numChecksPassed, (int) isNeedCaCertsRequest ); // FIXME
    if (!isNeedCaCertsRequest || error != OT_ERROR_NONE )
    	Finish(error);
}

otError AeClient::ProcessOperationalCert(const uint8_t *aCert, size_t aLength, bool &isNeedCaCertsRequest)
{
    otError		error = OT_ERROR_SECURITY;
    uint32_t 	certVerifyResultFlags;
    int         numChecksPassed = 0;
    int         mbedtlsErrorCode = 0;

    LogDebg("ProcessOperationalCert() LDevID len=%lu", aLength); // FIXME
    numChecksPassed++; LogDebg("  pass=%i", numChecksPassed); // FIXME

    // parse LDevID cert
    VerifyOrExit( (mbedtlsErrorCode = mbedtls_x509_crt_parse_der(&mOperationalCert, aCert, aLength)) == 0);
    numChecksPassed++; LogDebg("  pass=%i", numChecksPassed); // FIXME

    // TODO(wgtdkp): match the certificate against CSR

    // TODO(wgtdkp): make sure mOperationalKey matches public key in the cert

    // TODO(wgtdkp): set expected Common Name.

    // Verify new LDevID cert against my existing Domain CA TA, in case of re-enroll. And in
   	// case of enroll, check if the pinned-domain-cert is already good to be used as Domain CA.
    // The outcomes determine whether a next CA Certs request is still needed or not.
    isNeedCaCertsRequest = true;
    if (mIsDoingReenroll) {
       	mbedtlsErrorCode = mbedtls_x509_crt_verify(&mOperationalCert, &mDomainCaCert, NULL, NULL, &certVerifyResultFlags, NULL, NULL);
       	numChecksPassed++; LogDebg("  ree pass=%i", numChecksPassed);
   		if (mbedtlsErrorCode == 0)
   			isNeedCaCertsRequest = false;
    } else {
    	if (mPinnedDomainCert.ca_istrue)
    	{
			mbedtlsErrorCode = mbedtls_x509_crt_verify(&mOperationalCert, &mPinnedDomainCert, NULL, NULL, &certVerifyResultFlags, NULL, NULL);
			numChecksPassed++; LogDebg("  enr pass=%i", numChecksPassed);
			if (mbedtlsErrorCode == 0) {
				isNeedCaCertsRequest = false;
				// set the pinned-cert as the domain ca cert, by parsing it (again).
				VerifyOrExit( (mbedtlsErrorCode = mbedtls_x509_crt_parse_der(&mDomainCaCert, mPinnedDomainCert.raw.p, mPinnedDomainCert.raw.len)) == 0);
				numChecksPassed++;
				//PrintEncodedCert(mDomainCaCert.raw.p, mDomainCaCert.raw.len); // used for debug.
			}
    	}
    }
	numChecksPassed++; LogDebg("  pass=%i", numChecksPassed);

    error = OT_ERROR_NONE;

exit:
    LogDebg("  err=%i, pass=%i, mbedtlsErr=%i", error, numChecksPassed , mbedtlsErrorCode); // FIXME
    PrintEncodedCert(aCert,aLength);
    return error;
}

otError AeClient::GenerateECKey(mbedtls_pk_context &aKey)
{
    otError                  error = OT_ERROR_SECURITY;
    otExtAddress             eui64;
    mbedtls_ctr_drbg_context ctrDrbg;

    mbedtls_ctr_drbg_init(&ctrDrbg);

    otPlatRadioGetIeeeEui64(&GetInstance(), eui64.m8);
    SuccessOrExit(
        mbedtls_ctr_drbg_seed(&ctrDrbg, mbedtls_entropy_func, &mEntropyContext, eui64.m8, sizeof(eui64)));

    SuccessOrExit(mbedtls_pk_setup(&aKey, mbedtls_pk_info_from_type(kOperationalKeyType)));
    SuccessOrExit(mbedtls_ecp_gen_key(kEcpGroupId, mbedtls_pk_ec(aKey), mbedtls_ctr_drbg_random, &ctrDrbg));

    error = OT_ERROR_NONE;

exit:
    mbedtls_ctr_drbg_free(&ctrDrbg);
    return error;
}

otError AeClient::CreateCsrData(mbedtls_pk_context &aKey,
                                 const char *        aSubjectName,
                                 unsigned char *     aBuf,
                                 size_t              aBufLen,
                                 size_t &            aCsrLen)
{
    otError                  error = OT_ERROR_SECURITY;
    mbedtls_x509write_csr    csr;
    mbedtls_ctr_drbg_context ctrDrbg;
    otExtAddress             eui64;
    ssize_t                  length;

    mbedtls_x509write_csr_init(&csr);
    mbedtls_x509write_csr_set_md_alg(&csr, MBEDTLS_MD_SHA1);
    mbedtls_ctr_drbg_init(&ctrDrbg);

    // if( opt.key_usage )
    //     mbedtls_x509write_csr_set_key_usage( &req, opt.key_usage );

    // if( opt.ns_cert_type )
    //     mbedtls_x509write_csr_set_ns_cert_type( &req, opt.ns_cert_type );

    otPlatRadioGetIeeeEui64(&GetInstance(), eui64.m8);
    SuccessOrExit(
        mbedtls_ctr_drbg_seed(&ctrDrbg, mbedtls_entropy_func, &mEntropyContext, eui64.m8, sizeof(eui64)));

    SuccessOrExit(mbedtls_x509write_csr_set_subject_name(&csr, aSubjectName));

    mbedtls_x509write_csr_set_key(&csr, &aKey);

    length = mbedtls_x509write_csr_der(&csr, aBuf, aBufLen, mbedtls_ctr_drbg_random, &ctrDrbg);
    VerifyOrExit(length > 0);

    aCsrLen = length;
    error   = OT_ERROR_NONE;

exit:
    mbedtls_ctr_drbg_free(&ctrDrbg);
    mbedtls_x509write_csr_free(&csr);
    return error;
}

bool AeClient::IsCertsEqual(const mbedtls_x509_crt * cert1, const mbedtls_x509_crt * cert2)
{
	if (cert1->raw.len != cert2->raw.len)
		return false;
	return (memcmp(cert1->raw.p, cert2->raw.p, cert1->raw.len) == 0);
}

void AeClient::PrintEncodedCert(const uint8_t * aCert, size_t aLength)
{
    OT_UNUSED_VARIABLE(aCert); // FIXME
	char hexCert[84];
	LogDebg("PrintEncodedCert(l=%lu):", aLength);
	size_t l=40;
    for(size_t i=0; i < aLength; i+=40){
      if( (i+l)>aLength )
	   l = aLength - i ;
      hexCert[0] = '\0';
      hexCert[ 40*2+0 ] = '\0';
      hexCert[ 40*2+1 ] = '\0';
      hexCert[ 40*2+2 ] = '\0';
      // Hex(hexCert, 84, aCert+i, l); // FIXME use OT GenerateNextHexDumpLine ?
      LogInfo("%s", hexCert);
    }
}


} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE && OPENTHREAD_CONFIG_CCM_ENABLE

