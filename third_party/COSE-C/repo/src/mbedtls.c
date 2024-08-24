#include "cose.h"
#include "configure.h"
#include "cose_int.h"
#include "crypto.h"

#include <assert.h>
#ifndef __MBED__
#include <memory.h>
#endif
#include <stdlib.h>

#ifdef USE_MBED_TLS

#include "mbedtls/ccm.h"
#include "mbedtls/md.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ecdsa.h"

bool FUseCompressed = true;

#define MIN(A, B) ((A) < (B) ? (A) : (B))

bool AES_CCM_Decrypt(COSE_Enveloped * pcose, int TSize, int LSize, const byte * pbKey, size_t cbKey, const byte * pbCrypto, size_t cbCrypto, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr)
{

	mbedtls_ccm_context ctx;
	int cbOut;
	byte * rgbOut = NULL;
	int NSize = 15 - (LSize/8);
	byte rgbIV[15] = { 0 };
	const cn_cbor * pIV = NULL;
	mbedtls_cipher_id_t cipher;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = &pcose->m_message.m_allocContext;
#endif

	mbedtls_ccm_init(&ctx);
	
        //  Setup the IV/Nonce and put it into the message
	pIV = _COSE_map_get_int(&pcose->m_message, COSE_Header_IV, COSE_BOTH, NULL);
	if ((pIV == NULL) || (pIV->type!= CN_CBOR_BYTES)) {
		if (perr != NULL) perr->err = COSE_ERR_INVALID_PARAMETER;

	errorReturn:
		if (rgbOut != NULL) COSE_FREE(rgbOut, context);
		mbedtls_ccm_free(&ctx);
		return false;
	}
	CHECK_CONDITION(pIV->length == NSize, COSE_ERR_INVALID_PARAMETER);
	memcpy(rgbIV, pIV->v.str, pIV->length);

	//  Setup and run the mbedTLS code
	cipher = MBEDTLS_CIPHER_ID_AES;
	
        CHECK_CONDITION(!mbedtls_ccm_setkey(&ctx, cipher, pbKey, cbKey*8), COSE_ERR_CRYPTO_FAIL);
	TSize /= 8; // Comes in in bits not bytes.

	cbOut = (int)  cbCrypto - TSize;
	rgbOut = (byte *)COSE_CALLOC(cbOut, 1, context);
	CHECK_CONDITION(rgbOut != NULL, COSE_ERR_OUT_OF_MEMORY);
        
		
	CHECK_CONDITION(!mbedtls_ccm_auth_decrypt(&ctx, cbOut, rgbIV, NSize, pbAuthData, cbAuthData, pbCrypto, rgbOut, &pbCrypto[cbOut], TSize), COSE_ERR_CRYPTO_FAIL);
        
	mbedtls_ccm_free(&ctx);
	pcose->pbContent = rgbOut;
	pcose->cbContent = cbOut;

	return true;
}


bool AES_CCM_Encrypt(COSE_Enveloped * pcose, int TSize, int LSize, const byte * pbKey, size_t cbKey, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr)
{
	mbedtls_ccm_context ctx;
	int cbOut;
	byte * rgbOut = NULL;
	int NSize = 15 - (LSize/8);
	const cn_cbor * cbor_iv = NULL;
	cn_cbor * cbor_iv_t = NULL;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = &pcose->m_message.m_allocContext;
#endif
	cn_cbor * cnTmp = NULL;
	mbedtls_cipher_id_t cipher;
	byte rgbIV[16];
	byte * pbIV = NULL;
	cn_cbor_errback cbor_error;

        mbedtls_ccm_init(&ctx);

	cipher = MBEDTLS_CIPHER_ID_AES;			
	
	//  Setup the IV/Nonce and put it into the message
	cbor_iv = _COSE_map_get_int(&pcose->m_message, COSE_Header_IV, COSE_BOTH, perr);
	if (cbor_iv == NULL) {
	
                pbIV = COSE_CALLOC(NSize, 1, context);
		CHECK_CONDITION(pbIV != NULL, COSE_ERR_OUT_OF_MEMORY);
		rand_bytes(pbIV, NSize);
		memcpy(rgbIV, pbIV, NSize);
		cbor_iv_t = cn_cbor_data_create(pbIV, NSize, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
		CHECK_CONDITION_CBOR(cbor_iv_t != NULL, cbor_error);
		pbIV = NULL;

		if (!_COSE_map_put(&pcose->m_message, COSE_Header_IV, cbor_iv_t, COSE_UNPROTECT_ONLY, perr)) goto errorReturn;
		cbor_iv_t = NULL;
	}
	else {
		CHECK_CONDITION(cbor_iv->type == CN_CBOR_BYTES, COSE_ERR_INVALID_PARAMETER);
		CHECK_CONDITION(cbor_iv->length == NSize, COSE_ERR_INVALID_PARAMETER);
		memcpy(rgbIV, cbor_iv->v.str, cbor_iv->length);
	}

	//  Setup and run the mbedTLS code
	
	//cbKey comes in bytes not bits
 	CHECK_CONDITION(!mbedtls_ccm_setkey(&ctx, cipher, pbKey, cbKey*8), COSE_ERR_CRYPTO_FAIL);
	
	TSize /= 8; // Comes in in bits not bytes.

        cbOut = pcose->cbContent; // M00BUG - This is a missing call?
	rgbOut = (byte *)COSE_CALLOC(cbOut+TSize, 1, context);
	CHECK_CONDITION(rgbOut != NULL, COSE_ERR_OUT_OF_MEMORY);

	CHECK_CONDITION(!mbedtls_ccm_encrypt_and_tag(&ctx, pcose->cbContent, rgbIV, NSize, pbAuthData, cbAuthData, pcose->pbContent, rgbOut, &rgbOut[pcose->cbContent], TSize), COSE_ERR_CRYPTO_FAIL);	

	cnTmp = cn_cbor_data_create(rgbOut, (int)pcose->cbContent + TSize, CBOR_CONTEXT_PARAM_COMMA NULL);
	CHECK_CONDITION(cnTmp != NULL, COSE_ERR_CBOR);
	rgbOut = NULL;

	CHECK_CONDITION(_COSE_array_replace(&pcose->m_message, cnTmp, INDEX_BODY, CBOR_CONTEXT_PARAM_COMMA NULL), COSE_ERR_CBOR);
	cnTmp = NULL;

	mbedtls_ccm_free(&ctx);
	
	return true;

errorReturn:
	if (pbIV != NULL) COSE_FREE(pbIV, context);
	if (cbor_iv_t != NULL) COSE_FREE(cbor_iv_t, context);
	if (rgbOut != NULL) COSE_FREE(rgbOut, context);
	if (cnTmp != NULL) COSE_FREE(cnTmp, context);
	mbedtls_ccm_free(&ctx);
	return false;
}
/*
bool AES_GCM_Decrypt(COSE_Enveloped * pcose, const byte * pbKey, size_t cbKey, const byte * pbCrypto, size_t cbCrypto, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr)
{
	EVP_CIPHER_CTX ctx;
	int cbOut;
	byte * rgbOut = NULL;
	int outl = 0;
	byte rgbIV[15] = { 0 };
	const cn_cbor * pIV = NULL;
	const EVP_CIPHER * cipher;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = &pcose->m_message.m_allocContext;
#endif
	int TSize = 128 / 8;

	EVP_CIPHER_CTX_init(&ctx);

	//  Setup the IV/Nonce and put it into the message

	pIV = _COSE_map_get_int(&pcose->m_message, COSE_Header_IV, COSE_BOTH, NULL);
	if ((pIV == NULL) || (pIV->type != CN_CBOR_BYTES)) {
		if (perr != NULL) perr->err = COSE_ERR_INVALID_PARAMETER;

	errorReturn:
		if (rgbOut != NULL) COSE_FREE(rgbOut, context);
		EVP_CIPHER_CTX_cleanup(&ctx);
		return false;
	}

	CHECK_CONDITION(pIV->length == 96/8, COSE_ERR_INVALID_PARAMETER);
	memcpy(rgbIV, pIV->v.str, pIV->length);

	//  Setup and run the OpenSSL code

	switch (cbKey) {
	case 128 / 8:
		cipher = EVP_aes_128_gcm();
		break;

	case 192 / 8:
		cipher = EVP_aes_192_gcm();
		break;

	case 256 / 8:
		cipher = EVP_aes_256_gcm();
		break;

	default:
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
		break;
	}

	//  Do the setup for OpenSSL

	CHECK_CONDITION(EVP_DecryptInit_ex(&ctx, cipher, NULL, NULL, NULL), COSE_ERR_DECRYPT_FAILED);

	CHECK_CONDITION(EVP_CIPHER_CTX_ctrl(&ctx, EVP_CTRL_CCM_SET_TAG, TSize, (void *)&pbCrypto[cbCrypto - TSize]), COSE_ERR_DECRYPT_FAILED);

	CHECK_CONDITION(EVP_DecryptInit(&ctx, 0, pbKey, rgbIV), COSE_ERR_DECRYPT_FAILED);
	
	//  Pus in the AAD

	CHECK_CONDITION(EVP_DecryptUpdate(&ctx, NULL, &outl, pbAuthData, (int) cbAuthData), COSE_ERR_DECRYPT_FAILED);

	//  

	cbOut = (int)cbCrypto - TSize;
	rgbOut = (byte *)COSE_CALLOC(cbOut, 1, context);
	CHECK_CONDITION(rgbOut != NULL, COSE_ERR_OUT_OF_MEMORY);

	//  Process content

	CHECK_CONDITION(EVP_DecryptUpdate(&ctx, rgbOut, &cbOut, pbCrypto, (int)cbCrypto - TSize), COSE_ERR_DECRYPT_FAILED);

	//  Process Tag

	CHECK_CONDITION(EVP_CIPHER_CTX_ctrl(&ctx, EVP_CTRL_GCM_SET_TAG, TSize, (byte *)pbCrypto + cbCrypto - TSize), COSE_ERR_DECRYPT_FAILED);

	//  Check the result

	CHECK_CONDITION(EVP_DecryptFinal(&ctx, rgbOut + cbOut, &cbOut), COSE_ERR_DECRYPT_FAILED);

	EVP_CIPHER_CTX_cleanup(&ctx);

	pcose->pbContent = rgbOut;
	pcose->cbContent = cbOut;

	return true;
}

bool AES_GCM_Encrypt(COSE_Enveloped * pcose, const byte * pbKey, size_t cbKey, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr)
{
	EVP_CIPHER_CTX ctx;
	int cbOut;
	byte * rgbOut = NULL;
	int outl = 0;
	byte rgbIV[16] = { 0 };
	byte * pbIV = NULL;
	const cn_cbor * cbor_iv = NULL;
	cn_cbor * cbor_iv_t = NULL;
	const EVP_CIPHER * cipher;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = &pcose->m_message.m_allocContext;
#endif
	cn_cbor_errback cbor_error;

	// Make it first so we can clean it up
	EVP_CIPHER_CTX_init(&ctx);

	//  Setup the IV/Nonce and put it into the message

	cbor_iv = _COSE_map_get_int(&pcose->m_message, COSE_Header_IV, COSE_BOTH, perr);
	if (cbor_iv == NULL) {
		pbIV = COSE_CALLOC(96, 1, context);
		CHECK_CONDITION(pbIV != NULL, COSE_ERR_OUT_OF_MEMORY);
		rand_bytes(pbIV, 96 / 8);
		memcpy(rgbIV, pbIV, 96 / 8);
		cbor_iv_t = cn_cbor_data_create(pbIV, 96 / 8, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
		CHECK_CONDITION_CBOR(cbor_iv_t != NULL, cbor_error);
		pbIV = NULL;

		if (!_COSE_map_put(&pcose->m_message, COSE_Header_IV, cbor_iv_t, COSE_UNPROTECT_ONLY, perr)) goto errorReturn;
		cbor_iv_t = NULL;
	}
	else {
		CHECK_CONDITION(cbor_iv->type == CN_CBOR_BYTES, COSE_ERR_INVALID_PARAMETER);
		CHECK_CONDITION(cbor_iv->length == 96 / 8, COSE_ERR_INVALID_PARAMETER);
		memcpy(rgbIV, cbor_iv->v.str, cbor_iv->length);
	}


	switch (cbKey*8) {
	case 128:
		cipher = EVP_aes_128_gcm();
		break;

	case 192:
		cipher = EVP_aes_192_gcm();
		break;

	case 256:
		cipher = EVP_aes_256_gcm();
		break;

	default:
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
		break;
	}

	//  Setup and run the OpenSSL code

	CHECK_CONDITION(EVP_EncryptInit_ex(&ctx, cipher, NULL, NULL, NULL), COSE_ERR_CRYPTO_FAIL);

	CHECK_CONDITION(EVP_EncryptInit(&ctx, 0, pbKey, rgbIV), COSE_ERR_CRYPTO_FAIL);

	CHECK_CONDITION(EVP_EncryptUpdate(&ctx, NULL, &outl, pbAuthData, (int) cbAuthData), COSE_ERR_CRYPTO_FAIL);

	rgbOut = (byte *)COSE_CALLOC(pcose->cbContent + 128/8, 1, context);
	CHECK_CONDITION(rgbOut != NULL, COSE_ERR_OUT_OF_MEMORY);

	CHECK_CONDITION(EVP_EncryptUpdate(&ctx, rgbOut, &cbOut, pcose->pbContent, (int)pcose->cbContent), COSE_ERR_CRYPTO_FAIL);

	CHECK_CONDITION(EVP_EncryptFinal_ex(&ctx, &rgbOut[cbOut], &cbOut), COSE_ERR_CRYPTO_FAIL);

	CHECK_CONDITION(EVP_CIPHER_CTX_ctrl(&ctx, EVP_CTRL_GCM_GET_TAG, 128/8, &rgbOut[pcose->cbContent]), COSE_ERR_CRYPTO_FAIL);

	cn_cbor * cnTmp = cn_cbor_data_create(rgbOut, (int)pcose->cbContent + 128/8, CBOR_CONTEXT_PARAM_COMMA NULL);
	CHECK_CONDITION(cnTmp != NULL, COSE_ERR_CBOR);
	rgbOut = NULL;
	CHECK_CONDITION(_COSE_array_replace(&pcose->m_message, cnTmp, INDEX_BODY, CBOR_CONTEXT_PARAM_COMMA NULL), COSE_ERR_CBOR);

	EVP_CIPHER_CTX_cleanup(&ctx);
	return true;

errorReturn:
	if (pbIV != NULL) COSE_FREE(pbIV, context);
	if (cbor_iv_t != NULL) COSE_FREE(cbor_iv_t, context);
	if (rgbOut != NULL) COSE_FREE(rgbOut, context);
	EVP_CIPHER_CTX_cleanup(&ctx);
	return false;
}


bool AES_CBC_MAC_Create(COSE_MacMessage * pcose, int TSize, const byte * pbKey, size_t cbKey, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr)
{
	const EVP_CIPHER * pcipher = NULL;
	EVP_CIPHER_CTX ctx;
	int cbOut;
	byte rgbIV[16] = { 0 };
	byte * rgbOut = NULL;
	bool f = false;
	unsigned int i;
	cn_cbor * cn = NULL;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = &pcose->m_message.m_allocContext;
#endif

	EVP_CIPHER_CTX_init(&ctx);

	rgbOut = COSE_CALLOC(16, 1, context);
	CHECK_CONDITION(rgbOut != NULL, COSE_ERR_OUT_OF_MEMORY);

	switch (cbKey*8) {
	case 128:
		pcipher = EVP_aes_128_cbc();
		break;

	case 256:
		pcipher = EVP_aes_256_cbc();
		break;

	default:
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
	}

	//  Setup and run the OpenSSL code

	CHECK_CONDITION(EVP_EncryptInit_ex(&ctx, pcipher, NULL, pbKey, rgbIV), COSE_ERR_CRYPTO_FAIL);

	for (i = 0; i < (unsigned int)cbAuthData / 16; i++) {
		CHECK_CONDITION(EVP_EncryptUpdate(&ctx, rgbOut, &cbOut, pbAuthData + (i * 16), 16), COSE_ERR_CRYPTO_FAIL);
	}
	if (cbAuthData % 16 != 0) {
		CHECK_CONDITION(EVP_EncryptUpdate(&ctx, rgbOut, &cbOut, pbAuthData + (i * 16), cbAuthData % 16), COSE_ERR_CRYPTO_FAIL);
		CHECK_CONDITION(EVP_EncryptUpdate(&ctx, rgbOut, &cbOut, rgbIV, 16 - (cbAuthData % 16)), COSE_ERR_CRYPTO_FAIL);
	}

	cn = cn_cbor_data_create(rgbOut, TSize / 8, CBOR_CONTEXT_PARAM_COMMA NULL);
	CHECK_CONDITION(cn != NULL, COSE_ERR_OUT_OF_MEMORY);
	rgbOut = NULL;

	CHECK_CONDITION(_COSE_array_replace(&pcose->m_message, cn, INDEX_MAC_TAG, CBOR_CONTEXT_PARAM_COMMA NULL), COSE_ERR_CBOR);
	cn = NULL;

	EVP_CIPHER_CTX_cleanup(&ctx);
	return !f;

errorReturn:
	if (rgbOut != NULL) COSE_FREE(rgbOut, context);
	if (cn != NULL) CN_CBOR_FREE(cn, context);
	EVP_CIPHER_CTX_cleanup(&ctx);
	return false;
}

bool AES_CBC_MAC_Validate(COSE_MacMessage * pcose, int TSize, const byte * pbKey, size_t cbKey, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr)
{
	const EVP_CIPHER * pcipher = NULL;
	EVP_CIPHER_CTX ctx;
	int cbOut;
	byte rgbIV[16] = { 0 };
	byte rgbTag[16] = { 0 };
	bool f = false;
	unsigned int i;

	switch (cbKey*8) {
	case 128:
		pcipher = EVP_aes_128_cbc();
		break;

	case 256:
		pcipher = EVP_aes_256_cbc();
		break;

	default:
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
	}

	//  Setup and run the OpenSSL code

	EVP_CIPHER_CTX_init(&ctx);
	CHECK_CONDITION(EVP_EncryptInit_ex(&ctx, pcipher, NULL, pbKey, rgbIV), COSE_ERR_CRYPTO_FAIL);

	TSize /= 8;

	for (i = 0; i < (unsigned int) cbAuthData / 16; i++) {
		CHECK_CONDITION(EVP_EncryptUpdate(&ctx, rgbTag, &cbOut, pbAuthData+(i*16), 16), COSE_ERR_CRYPTO_FAIL);
	}
	if (cbAuthData % 16 != 0) {
		CHECK_CONDITION(EVP_EncryptUpdate(&ctx, rgbTag, &cbOut, pbAuthData + (i * 16), cbAuthData % 16), COSE_ERR_CRYPTO_FAIL);
		CHECK_CONDITION(EVP_EncryptUpdate(&ctx, rgbTag, &cbOut, rgbIV, 16 - (cbAuthData % 16)), COSE_ERR_CRYPTO_FAIL);
	}

	cn_cbor * cn = _COSE_arrayget_int(&pcose->m_message, INDEX_MAC_TAG);
	CHECK_CONDITION(cn != NULL, COSE_ERR_CBOR);

	for (i = 0; i < (unsigned int)TSize; i++) f |= (cn->v.bytes[i] != rgbTag[i]);

	EVP_CIPHER_CTX_cleanup(&ctx);
	return !f;

errorReturn:
	EVP_CIPHER_CTX_cleanup(&ctx);
	return false;
}

#if 0
//  We are doing CBC-MAC not CMAC at this time
bool AES_CMAC_Validate(COSE_MacMessage * pcose, int KeySize, int TagSize, const byte * pbAuthData, int cbAuthData, cose_errback * perr)
{
	CMAC_CTX * pctx = NULL;
	const EVP_CIPHER * pcipher = NULL;
	byte * rgbOut = NULL;
	size_t cbOut;
	bool f = false;
	unsigned int i;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = &pcose->m_message.m_allocContext;
#endif

	pctx = CMAC_CTX_new();


	switch (KeySize) {
	case 128: pcipher = EVP_aes_128_cbc(); break;
	case 256: pcipher = EVP_aes_256_cbc(); break;
	default: FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER); break;
	}

	rgbOut = COSE_CALLOC(128/8, 1, context);
	CHECK_CONDITION(rgbOut != NULL, COSE_ERR_OUT_OF_MEMORY);

	CHECK_CONDITION(CMAC_Init(pctx, pcose->pbKey, pcose->cbKey, pcipher, NULL ) == 1, COSE_ERR_CRYPTO_FAIL);
	CHECK_CONDITION(CMAC_Update(pctx, pbAuthData, cbAuthData), COSE_ERR_CRYPTO_FAIL);
	CHECK_CONDITION(CMAC_Final(pctx, rgbOut, &cbOut), COSE_ERR_CRYPTO_FAIL);

	cn_cbor * cn = _COSE_arrayget_int(&pcose->m_message, INDEX_MAC_TAG);
	CHECK_CONDITION(cn != NULL, COSE_ERR_CBOR);

	for (i = 0; i < (unsigned int)TagSize / 8; i++) f |= (cn->v.bytes[i] != rgbOut[i]);

	COSE_FREE(rgbOut, context);
	CMAC_CTX_cleanup(pctx);
	CMAC_CTX_free(pctx);
	return !f;

errorReturn:
	COSE_FREE(rgbOut, context);
	CMAC_CTX_cleanup(pctx);
	CMAC_CTX_free(pctx);
	return false;

}
#endif

bool HKDF_AES_Expand(COSE * pcose, size_t cbitKey, const byte * pbPRK, size_t cbPRK, const byte * pbInfo, size_t cbInfo, byte * pbOutput, size_t cbOutput, cose_errback * perr)
{
	const EVP_CIPHER * pcipher = NULL;
	EVP_CIPHER_CTX ctx;
	int cbOut;
	byte rgbIV[16] = { 0 };
	byte bCount = 1;
	size_t ib;
	byte rgbDigest[128 / 8];
	int cbDigest = 0;
	byte rgbOut[16];

	EVP_CIPHER_CTX_init(&ctx);

	switch (cbitKey) {
	case 128:
		pcipher = EVP_aes_128_cbc();
		break;

	case 256:
		pcipher = EVP_aes_256_cbc();
		break;

	default:
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
	}
	CHECK_CONDITION(cbPRK == cbitKey / 8, COSE_ERR_INVALID_PARAMETER);

	//  Setup and run the OpenSSL code


	for (ib = 0; ib < cbOutput; ib += 16, bCount += 1) {
		size_t ib2;

		CHECK_CONDITION(EVP_EncryptInit_ex(&ctx, pcipher, NULL, pbPRK, rgbIV), COSE_ERR_CRYPTO_FAIL);

		CHECK_CONDITION(EVP_EncryptUpdate(&ctx, rgbOut, &cbOut, rgbDigest, cbDigest), COSE_ERR_CRYPTO_FAIL);
		for (ib2 = 0; ib2 < cbInfo; ib2+=16) {
			CHECK_CONDITION(EVP_EncryptUpdate(&ctx, rgbOut, &cbOut, pbInfo+ib2, (int) MIN(16, cbInfo-ib2)), COSE_ERR_CRYPTO_FAIL);
		}
		CHECK_CONDITION(EVP_EncryptUpdate(&ctx, rgbOut, &cbOut, &bCount, 1), COSE_ERR_CRYPTO_FAIL);
		if ((cbInfo + 1) % 16 != 0) {
			CHECK_CONDITION(EVP_EncryptUpdate(&ctx, rgbOut, &cbOut, rgbIV, (int) 16-(cbInfo+1)%16), COSE_ERR_CRYPTO_FAIL);
		}
		memcpy(rgbDigest, rgbOut, cbOut);
		cbDigest = cbOut;
		memcpy(pbOutput + ib, rgbDigest, MIN(16, cbOutput - ib));
	}

	EVP_CIPHER_CTX_cleanup(&ctx);
	return true;

errorReturn:
	EVP_CIPHER_CTX_cleanup(&ctx);
	return false;
}


bool HKDF_Extract(COSE * pcose, const byte * pbKey, size_t cbKey, size_t cbitDigest, byte * rgbDigest, size_t * pcbDigest, cose_errback * perr)
{
	byte rgbSalt[EVP_MAX_MD_SIZE] = { 0 };
	int cbSalt;
	cn_cbor * cnSalt;
	HMAC_CTX ctx;
	const EVP_MD * pmd = NULL;
	unsigned int cbDigest;

	HMAC_CTX_init(&ctx);

	if (0) {
	errorReturn:
		HMAC_cleanup(&ctx);
		return false;
	}

	switch (cbitDigest) {
	case 256: pmd = EVP_sha256(); cbSalt = 256 / 8;  break;
	case 384: pmd = EVP_sha384(); cbSalt = 384 / 8; break;
	case 512: pmd = EVP_sha512(); cbSalt = 512 / 8; break;
	default: FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER); break;
	}

	cnSalt = _COSE_map_get_int(pcose, COSE_Header_HKDF_salt, COSE_BOTH, perr);

	if (cnSalt != NULL) {
		CHECK_CONDITION(HMAC_Init(&ctx, cnSalt->v.bytes, (int) cnSalt->length, pmd), COSE_ERR_CRYPTO_FAIL);
	}
	else {
		CHECK_CONDITION(HMAC_Init(&ctx, rgbSalt, cbSalt, pmd), COSE_ERR_CRYPTO_FAIL);
	}
	CHECK_CONDITION(HMAC_Update(&ctx, pbKey, (int)cbKey), COSE_ERR_CRYPTO_FAIL);
	CHECK_CONDITION(HMAC_Final(&ctx, rgbDigest, &cbDigest), COSE_ERR_CRYPTO_FAIL);
	*pcbDigest = cbDigest;
	HMAC_cleanup(&ctx);
	return true;
}

bool HKDF_Expand(COSE * pcose, size_t cbitDigest, const byte * pbPRK, size_t cbPRK, const byte * pbInfo, size_t cbInfo, byte * pbOutput, size_t cbOutput, cose_errback * perr)
{
	HMAC_CTX ctx;
	const EVP_MD * pmd = NULL;
	size_t ib;
	int cbSalt;
	unsigned int cbDigest = 0;
	byte rgbDigest[EVP_MAX_MD_SIZE];
	byte bCount = 1;

	HMAC_CTX_init(&ctx);

	if (0) {
	errorReturn:
		HMAC_cleanup(&ctx);
		return false;
	}

	switch (cbitDigest) {
	case 256: pmd = EVP_sha256(); cbSalt = 256 / 8;  break;
	case 384: pmd = EVP_sha384(); cbSalt = 384 / 8; break;
	case 512: pmd = EVP_sha512(); cbSalt = 512 / 8; break;
	default: FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER); break;
	}


	for (ib = 0; ib < cbOutput; ib += cbDigest, bCount += 1) {
		CHECK_CONDITION(HMAC_Init_ex(&ctx, pbPRK, (int)cbPRK, pmd, NULL), COSE_ERR_CRYPTO_FAIL);
		CHECK_CONDITION(HMAC_Update(&ctx, rgbDigest, cbDigest), COSE_ERR_CRYPTO_FAIL);
		CHECK_CONDITION(HMAC_Update(&ctx, pbInfo, cbInfo), COSE_ERR_CRYPTO_FAIL);
		CHECK_CONDITION(HMAC_Update(&ctx, &bCount, 1), COSE_ERR_CRYPTO_FAIL);
		CHECK_CONDITION(HMAC_Final(&ctx, rgbDigest, &cbDigest), COSE_ERR_CRYPTO_FAIL);

		memcpy(pbOutput + ib, rgbDigest, MIN(cbDigest, cbOutput - ib));
	}

	HMAC_cleanup(&ctx);
	return true;

}

void dump_output(byte* b, size_t s){
	for(int i = 0; i < s; i++){
		printf("%02x", *b);
		b++;
	}
	printf("\n");
}

void diff(unsigned char* a, size_t a_l, unsigned char* b, size_t b_l){
	size_t s;
	s = (a_l < b_l) ? a_l : b_l;
	unsigned char* tmp = a;
	printf("size = %d\n",s );
//	printf("%02x\n", *tmp);
	int i;
	for(i = 0; i < s; ++i){
		printf("%02x", *tmp);
		tmp++;
	}
	printf("\n");

	tmp = b;
	for(i = 0; i < s; ++i){
		printf("%02x", *tmp);
		tmp++;
	}
	printf("\n");

	for(i = 0; i < s; ++i){

		if(*a != *b){
			printf("^^");

		} else {
			printf("__");

		}

		a++;
		b++;
	}
	printf("\n");
}
*/

bool HMAC_Create(COSE_MacMessage * pcose, int HSize, int TSize, const byte * pbKey, size_t cbKey, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr)
{
	byte * rgbOut = NULL;
//	unsigned int cbOut;
	mbedtls_md_context_t contx;
	const char* md_name;
	const struct mbedtls_md_info_t * info;

#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = &pcose->m_message.m_allocContext;
#endif

	switch (HSize) {
		case 256: md_name = "SHA256"; break;
		case 384: md_name = "SHA384"; break;
		case 512: md_name = "SHA512"; break;
		default: FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER); break;
	}

	if (0) {
	errorReturn:
		COSE_FREE(rgbOut, context);
		mbedtls_md_free(&contx); 
		return false;
	}

	mbedtls_md_init(&contx);
	info = mbedtls_md_info_from_string (md_name);
	mbedtls_md_setup( &contx, info, 1 );

	rgbOut = COSE_CALLOC(mbedtls_md_get_size(info), 1, context);
	CHECK_CONDITION(rgbOut != NULL, COSE_ERR_OUT_OF_MEMORY);

	CHECK_CONDITION(!(mbedtls_md_hmac_starts (&contx, pbKey, cbKey)), COSE_ERR_CRYPTO_FAIL);
	CHECK_CONDITION(!(mbedtls_md_hmac_update (&contx, pbAuthData, cbAuthData)), COSE_ERR_CRYPTO_FAIL);
	CHECK_CONDITION(!(mbedtls_md_hmac_finish (&contx, rgbOut)), COSE_ERR_CRYPTO_FAIL);

	CHECK_CONDITION(_COSE_array_replace(&pcose->m_message, cn_cbor_data_create(rgbOut, TSize / 8, CBOR_CONTEXT_PARAM_COMMA NULL), INDEX_MAC_TAG, CBOR_CONTEXT_PARAM_COMMA NULL), COSE_ERR_CBOR);

	mbedtls_md_free(&contx); 
	return true;
}

bool HMAC_Validate(COSE_MacMessage * pcose, int HSize, int TSize, const byte * pbKey, size_t cbKey, const byte * pbAuthData, size_t cbAuthData, cose_errback * perr)
{
	mbedtls_md_context_t contx;
	const char* md_name;
	const struct mbedtls_md_info_t * info;
	byte * rgbOut = NULL;
	unsigned int cbOut;
	bool f = false;
	unsigned int i;

#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = &pcose->m_message.m_allocContext;
#endif

	switch (HSize) {
		case 256: md_name = "SHA256"; break;
		case 384: md_name = "SHA384"; break;
		case 512: md_name = "SHA512"; break;
		default: FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER); break;
	}

	mbedtls_md_init(&contx);
	info = mbedtls_md_info_from_string (md_name);
	mbedtls_md_setup( &contx, info, 1 );
	
	cbOut = mbedtls_md_get_size(info);
	rgbOut = COSE_CALLOC(cbOut, 1, context);
	CHECK_CONDITION(rgbOut != NULL, COSE_ERR_OUT_OF_MEMORY);

	CHECK_CONDITION(!(mbedtls_md_hmac_starts (&contx, pbKey, cbKey)), COSE_ERR_CRYPTO_FAIL);
	CHECK_CONDITION(!(mbedtls_md_hmac_update (&contx, pbAuthData, cbAuthData)), COSE_ERR_CRYPTO_FAIL);
	CHECK_CONDITION(!(mbedtls_md_hmac_finish (&contx, rgbOut)), COSE_ERR_CRYPTO_FAIL);

	cn_cbor * cn = _COSE_arrayget_int(&pcose->m_message, INDEX_MAC_TAG);
	CHECK_CONDITION(cn != NULL, COSE_ERR_CBOR);

	if (cn->length > (int) cbOut) return false;
	for (i = 0; i < (unsigned int) TSize/8; i++) f |= (cn->v.bytes[i] != rgbOut[i]);

	mbedtls_md_free(&contx); 
	return !f;

errorReturn:
	COSE_FREE(rgbOut, context);
	mbedtls_md_free(&contx); 
	return false;
}

/*
cn_cbor * EC_FromKey(const EC_KEY * pKey, CBOR_CONTEXT_COMMA cose_errback * perr)
{
	cn_cbor * pkey = NULL;
	const EC_GROUP * pgroup;
	int cose_group;
	cn_cbor * p = NULL;
	cn_cbor_errback cbor_error;
	const EC_POINT * pPoint;
	size_t cbSize;
	byte * pbOut = NULL;

	pgroup = EC_KEY_get0_group(pKey);
	CHECK_CONDITION(pgroup != NULL, COSE_ERR_INVALID_PARAMETER);

	switch (EC_GROUP_get_curve_name(pgroup)) {
	case NID_X9_62_prime256v1:		cose_group = 1;		break;
	case NID_secp384r1: cose_group = 2; break;
	case NID_secp521r1: cose_group = 3; break;

	default:
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
	}

	pkey = cn_cbor_map_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(pkey != NULL, cbor_error);

	p = cn_cbor_int_create(cose_group, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(p != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_mapput_int(pkey, COSE_Key_EC_Curve, p, CBOR_CONTEXT_PARAM_COMMA &cbor_error), cbor_error);
	p = NULL;

	pPoint = EC_KEY_get0_public_key(pKey);
	CHECK_CONDITION(pPoint != NULL, COSE_ERR_INVALID_PARAMETER);

	if (FUseCompressed) {
		cbSize = EC_POINT_point2oct(pgroup, pPoint, POINT_CONVERSION_COMPRESSED, NULL, 0, NULL);
		CHECK_CONDITION(cbSize > 0, COSE_ERR_CRYPTO_FAIL);
		pbOut = COSE_CALLOC(cbSize, 1, context);
		CHECK_CONDITION(pbOut != NULL, COSE_ERR_OUT_OF_MEMORY);
		CHECK_CONDITION(EC_POINT_point2oct(pgroup, pPoint, POINT_CONVERSION_COMPRESSED, pbOut, cbSize, NULL) == cbSize, COSE_ERR_CRYPTO_FAIL);
	}
	else {
		cbSize = EC_POINT_point2oct(pgroup, pPoint, POINT_CONVERSION_UNCOMPRESSED, NULL, 0, NULL);
		CHECK_CONDITION(cbSize > 0, COSE_ERR_CRYPTO_FAIL);
		pbOut = COSE_CALLOC(cbSize, 1, context);
		CHECK_CONDITION(pbOut != NULL, COSE_ERR_OUT_OF_MEMORY);
		CHECK_CONDITION(EC_POINT_point2oct(pgroup, pPoint, POINT_CONVERSION_UNCOMPRESSED, pbOut, cbSize, NULL) == cbSize, COSE_ERR_CRYPTO_FAIL);
	}
	p = cn_cbor_data_create(pbOut+1, (int) (cbSize / 2), CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(p != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_mapput_int(pkey, COSE_Key_EC_X, p, CBOR_CONTEXT_PARAM_COMMA &cbor_error), cbor_error);
	p = NULL;

	if (FUseCompressed) {
		p = cn_cbor_bool_create(pbOut[0] & 1, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
		CHECK_CONDITION_CBOR(p != NULL, cbor_error);
		CHECK_CONDITION_CBOR(cn_cbor_mapput_int(pkey, COSE_Key_EC_Y, p, CBOR_CONTEXT_PARAM_COMMA &cbor_error), cbor_error);
		p = NULL;
	}
	else {
		p = cn_cbor_data_create(pbOut + cbSize / 2 + 1, (int)(cbSize / 2), CBOR_CONTEXT_PARAM_COMMA &cbor_error);
		pbOut = NULL;   // It is already part of the other one.
		CHECK_CONDITION_CBOR(p != NULL, cbor_error);
		CHECK_CONDITION_CBOR(cn_cbor_mapput_int(pkey, COSE_Key_EC_Y, p, CBOR_CONTEXT_PARAM_COMMA &cbor_error), cbor_error);
		p = NULL;
	}

	p = cn_cbor_int_create(COSE_Key_Type_EC2, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(p != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_mapput_int(pkey, COSE_Key_Type, p, CBOR_CONTEXT_PARAM_COMMA &cbor_error), cbor_error);
	p = NULL;

returnHere:
	if (pbOut != NULL) COSE_FREE(pbOut, context);
	if (p != NULL) CN_CBOR_FREE(p, context);
	return pkey;

errorReturn:
	CN_CBOR_FREE(pkey, context);
	pkey = NULL;
	goto returnHere;
}
*/

bool ECDSA_Sign(COSE * pSigner, int index, const eckey_t * eckey, int cbitDigest, const byte * rgbToSign, size_t cbToSign, cose_errback * perr)
{
#if defined(MBEDTLS_ECDSA_DETERMINISTIC)
	byte rgbDigest[MBEDTLS_MD_MAX_SIZE];
	uint8_t * pbSig = NULL;
	cn_cbor_errback cbor_error;
	int cbR;
	mbedtls_md_type_t mdType;
	const mbedtls_md_info_t *pmdInfo;
	mbedtls_mpi r;
	mbedtls_mpi s;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = &pSigner->m_allocContext;
#endif
	cn_cbor * p = NULL;
	bool result = false;

	mbedtls_mpi_init(&r);
	mbedtls_mpi_init(&s);

	CHECK_CONDITION(eckey->d.n != 0, COSE_ERR_INVALID_PARAMETER);

	switch(cbitDigest)
	{
	case 256:
		mdType = MBEDTLS_MD_SHA256;
		break;

	case 384:
		mdType = MBEDTLS_MD_SHA384;
		break;

	case 512:
		mdType = MBEDTLS_MD_SHA512;
		break;

	default:
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
	}
	pmdInfo = mbedtls_md_info_from_type(mdType);
	CHECK_CONDITION(pmdInfo != NULL, COSE_ERR_INVALID_PARAMETER);
	CHECK_CONDITION(mbedtls_md(pmdInfo, rgbToSign, cbToSign, rgbDigest) == 0, COSE_ERR_INVALID_PARAMETER);

	CHECK_CONDITION(mbedtls_ecdsa_sign_det((mbedtls_ecp_group*)&eckey->grp, &r, &s, &eckey->d, rgbDigest, mbedtls_md_get_size(pmdInfo), mdType) == 0, COSE_ERR_CRYPTO_FAIL);

	cbR = (eckey->grp.nbits + 7) / 8;

	pbSig = COSE_CALLOC(cbR, 2, context);
	CHECK_CONDITION(pbSig != NULL, COSE_ERR_OUT_OF_MEMORY);

	CHECK_CONDITION(mbedtls_mpi_write_binary(&r, pbSig, cbR) == 0, COSE_ERR_INTERNAL);
	CHECK_CONDITION(mbedtls_mpi_write_binary(&s, pbSig + cbR, cbR) == 0, COSE_ERR_INTERNAL);

	p = cn_cbor_data_create(pbSig, cbR*2, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(p != NULL, cbor_error);

	CHECK_CONDITION(_COSE_array_replace(pSigner, p, index, CBOR_CONTEXT_PARAM_COMMA NULL), COSE_ERR_CBOR);

	p = NULL;
	pbSig = NULL;
	result = true;

errorReturn:
	cn_cbor_free(p CBOR_CONTEXT_PARAM);
	COSE_FREE(pbSig, context);
	mbedtls_mpi_free(&r);
	mbedtls_mpi_free(&s);
	return result;
#else
	return false;
#endif
}


bool ECDSA_Verify(COSE * pSigner, int index, const eckey_t * eckey, int cbitDigest, const byte * rgbToSign, size_t cbToSign, cose_errback * perr)
{
	mbedtls_mpi r;
	mbedtls_mpi s;
	mbedtls_md_type_t mdType;
	const mbedtls_md_info_t *pmdInfo;
	byte rgbDigest[MBEDTLS_MD_MAX_SIZE];
	cn_cbor * pSig;
	bool result = false;

	mbedtls_mpi_init(&r);
	mbedtls_mpi_init(&s);

	switch(cbitDigest)
	{
	case 256:
		mdType = MBEDTLS_MD_SHA256;
		break;

	case 384:
		mdType = MBEDTLS_MD_SHA384;
		break;

	case 512:
		mdType = MBEDTLS_MD_SHA512;
		break;

	default:
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
	}
	pmdInfo = mbedtls_md_info_from_type(mdType);
	CHECK_CONDITION(pmdInfo != NULL, COSE_ERR_INVALID_PARAMETER);
	CHECK_CONDITION(mbedtls_md(pmdInfo, rgbToSign, cbToSign, rgbDigest) == 0, COSE_ERR_INVALID_PARAMETER);

	pSig = _COSE_arrayget_int(pSigner, index);
	CHECK_CONDITION((pSig != NULL) && (pSig->type == CN_CBOR_BYTES), COSE_ERR_INVALID_PARAMETER);

	CHECK_CONDITION(mbedtls_mpi_read_binary( &r, pSig->v.bytes, pSig->length / 2 ) == 0, COSE_ERR_OUT_OF_MEMORY);
	CHECK_CONDITION(mbedtls_mpi_read_binary( &s, pSig->v.bytes + pSig->length / 2, pSig->length / 2 ) == 0, COSE_ERR_OUT_OF_MEMORY);
	CHECK_CONDITION(mbedtls_ecdsa_verify((mbedtls_ecp_group*)&eckey->grp, rgbDigest, mbedtls_md_get_size(pmdInfo), &eckey->Q, &r, &s) == 0, COSE_ERR_CRYPTO_FAIL);

	result = true;

errorReturn:
	mbedtls_mpi_free(&r);
	mbedtls_mpi_free(&s);
	return result;
}

/*
bool AES_KW_Decrypt(COSE_Enveloped * pcose, const byte * pbKeyIn, size_t cbitKey, const byte * pbCipherText, size_t cbCipherText, byte * pbKeyOut, int * pcbKeyOut, cose_errback * perr)
{
	byte rgbOut[512 / 8];
	AES_KEY key;

	CHECK_CONDITION(AES_set_decrypt_key(pbKeyIn, (int)cbitKey, &key) == 0, COSE_ERR_CRYPTO_FAIL);

	CHECK_CONDITION(AES_unwrap_key(&key, NULL, rgbOut, pbCipherText, (int) cbCipherText), COSE_ERR_CRYPTO_FAIL);

	memcpy(pbKeyOut, rgbOut, cbCipherText - 8);
	*pcbKeyOut = (int) (cbCipherText - 8);

	return true;
errorReturn:
	return false;
}

bool AES_KW_Encrypt(COSE_RecipientInfo * pcose, const byte * pbKeyIn, int cbitKey, const byte *  pbContent, int  cbContent, cose_errback * perr)
{
	byte  *pbOut = NULL;
	AES_KEY key;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = &pcose->m_encrypt.m_message.m_allocContext;
#endif
	cn_cbor * cnTmp = NULL;

	pbOut = COSE_CALLOC(cbContent + 8, 1, context);
	CHECK_CONDITION(pbOut != NULL, COSE_ERR_OUT_OF_MEMORY);

	CHECK_CONDITION(AES_set_encrypt_key(pbKeyIn, cbitKey, &key) == 0, COSE_ERR_CRYPTO_FAIL);

	CHECK_CONDITION(AES_wrap_key(&key, NULL, pbOut, pbContent, cbContent), COSE_ERR_CRYPTO_FAIL);

	cnTmp = cn_cbor_data_create(pbOut, (int)cbContent + 8, CBOR_CONTEXT_PARAM_COMMA NULL);
	CHECK_CONDITION(cnTmp != NULL, COSE_ERR_CBOR);
	pbOut = NULL;
	CHECK_CONDITION(_COSE_array_replace(&pcose->m_encrypt.m_message, cnTmp, INDEX_BODY, CBOR_CONTEXT_PARAM_COMMA NULL), COSE_ERR_CBOR);
	cnTmp = NULL;

	return true;

errorReturn:
	COSE_FREE(cnTmp, context);
	if (pbOut != NULL) COSE_FREE(pbOut, context);
	return false;
}

*/
/*
//#include <stdio.h> //TODO
void rand_bytes(byte * pb, size_t cb){
//ctx->aes_ctx->rk e null... i callchainen l�ngst in. prova ist�llet:
//kolla https://tls.mbed.org/kb/how-to/add-a-random-generator
        //init random
       mbedtls_ctr_drbg_context ctr_drbg;
          char *personalization = "my_app_specific_string";

        ret = mbedtls_ctr_drbg_init( &ctr_drbg, mbedtls_entropy_func, &entropy,
                                         (const unsigned char *) personalization,
                                                             strlen( personalization ) ); 
        
        if(ret != 0) {
            //printf TODO
        }    
    
        mbedtls_ctr_drbg_random(&ctx,pb, cb); 	

	mbedtls_ctr_drbg_free(&ctx);	
        printf("rand byute done\n");
}*/
//TODO HOW TO GENERATE GOOD RANDOM BYTES
static const unsigned char entropy_source_pr[96] =
   { 0xc1, 0x80, 0x81, 0xa6, 0x5d, 0x44, 0x02, 0x16,
     0x19, 0xb3, 0xf1, 0x80, 0xb1, 0xc9, 0x20, 0x02,
     0x6a, 0x54, 0x6f, 0x0c, 0x70, 0x81, 0x49, 0x8b,
     0x6e, 0xa6, 0x62, 0x52, 0x6d, 0x51, 0xb1, 0xcb,
     0x58, 0x3b, 0xfa, 0xd5, 0x37, 0x5f, 0xfb, 0xc9,
     0xff, 0x46, 0xd2, 0x19, 0xc7, 0x22, 0x3e, 0x95,
     0x45, 0x9d, 0x82, 0xe1, 0xe7, 0x22, 0x9f, 0x63,
     0x31, 0x69, 0xd2, 0x6b, 0x57, 0x47, 0x4f, 0xa3,
     0x37, 0xc9, 0x98, 0x1c, 0x0b, 0xfb, 0x91, 0x31,
     0x4d, 0x55, 0xb9, 0xe9, 0x1c, 0x5a, 0x5e, 0xe4,
     0x93, 0x92, 0xcf, 0xc5, 0x23, 0x12, 0xd5, 0x56,
     0x2c, 0x4a, 0x6e, 0xff, 0xdc, 0x10, 0xd0, 0x68 };

static const unsigned char nonce_pers_pr[16] =
     { 0xd2, 0x54, 0xfc, 0xff, 0x02, 0x1e, 0x69, 0xd2,
      0x29, 0xc9, 0xcf, 0xad, 0x85, 0xfa, 0x48, 0x6c };

static size_t test_offset;
static int ctr_drbg_self_test_entropy( void *data, unsigned char *buf, size_t len ) {
    const unsigned char *p = data;
    memcpy( buf, p + test_offset, len );
    test_offset += len;
    return( 0 );
 }

void rand_bytes(byte* pb, size_t cb){
     
     mbedtls_ctr_drbg_context ctx;
    // unsigned char buf[16];
     
     mbedtls_ctr_drbg_init( &ctx );
     
     mbedtls_ctr_drbg_seed( &ctx, ctr_drbg_self_test_entropy, (void *) entropy_source_pr, nonce_pers_pr, 16); // FIXME 32 last param was removed due to newer mbedTLS version
     
     //mbedtls_ctr_drbg_set_prediction_resistance( &ctx, MBEDTLS_CTR_DRBG_PR_ON );
    
     mbedtls_ctr_drbg_random( &ctx, pb, cb );
     //mbedtls_ctr_drbg_random( &ctx, buf, MBEDTLS_CTR_DRBG_BLOCKSIZE );
     //memcmp( buf, result_pr, MBEDTLS_CTR_DRBG_BLOCKSIZE ) );
     
     mbedtls_ctr_drbg_free( &ctx );
}
//END OF TODO RANDOM BYTES

/*!
*
* @param[in] pRecipent	Pointer to the message object
* @param[in] ppKeyPrivate	Address of key with private portion
* @param[in] pKeyPublic	Address of the key w/o a private portion
* @param[in/out] ppbSecret	pointer to buffer to hold the computed secret
* @param[in/out] pcbSecret	size of the computed secret
* @param[in] context		cbor allocation context structure
* @param[out] perr			location to return error information
* @returns		success of the function
*/
/*
bool ECDH_ComputeSecret(COSE * pRecipient, cn_cbor ** ppKeyPrivate, const cn_cbor * pKeyPublic, byte ** ppbSecret, size_t * pcbSecret, CBOR_CONTEXT_COMMA cose_errback *perr)
{
	EC_KEY * peckeyPrivate = NULL;
	EC_KEY * peckeyPublic = NULL;
	int cbGroup;
	int cbsecret;
	byte * pbsecret = NULL;
	bool fRet = false;

	peckeyPublic = ECKey_From(pKeyPublic, &cbGroup, perr);
	if (peckeyPublic == NULL) goto errorReturn;

	if (*ppKeyPrivate == NULL) {
		{
			cn_cbor * pCompress = _COSE_map_get_int(pRecipient, COSE_Header_UseCompressedECDH, COSE_BOTH, perr);
			if (pCompress == NULL) FUseCompressed = false;
			else FUseCompressed = (pCompress->type == CN_CBOR_TRUE);
		}
		peckeyPrivate = EC_KEY_new();
		EC_KEY_set_group(peckeyPrivate, EC_KEY_get0_group(peckeyPublic));
		CHECK_CONDITION(EC_KEY_generate_key(peckeyPrivate) == 1, COSE_ERR_CRYPTO_FAIL);
		*ppKeyPrivate = EC_FromKey(peckeyPrivate, CBOR_CONTEXT_PARAM_COMMA perr);
		if (*ppKeyPrivate == NULL) goto errorReturn;
	}
	else {
		peckeyPrivate = ECKey_From(*ppKeyPrivate, &cbGroup, perr);
		if (peckeyPrivate == NULL) goto errorReturn;
	}

	pbsecret = COSE_CALLOC(cbGroup, 1, context);
	CHECK_CONDITION(pbsecret != NULL, COSE_ERR_OUT_OF_MEMORY);

	cbsecret = ECDH_compute_key(pbsecret, cbGroup, EC_KEY_get0_public_key(peckeyPublic), peckeyPrivate, NULL);
	CHECK_CONDITION(cbsecret > 0, COSE_ERR_CRYPTO_FAIL);

	*ppbSecret = pbsecret;
	*pcbSecret = cbsecret;
	pbsecret = NULL;

	fRet = true;

errorReturn:
	if (pbsecret != NULL) COSE_FREE(pbsecret, context);
	if (peckeyPublic != NULL) EC_KEY_free(peckeyPublic);
	if (peckeyPrivate != NULL) EC_KEY_free(peckeyPrivate);

	return fRet;
}
*/
#endif // USE_MBED_TLS
