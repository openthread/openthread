#include <stdlib.h>
#ifndef __MBED__
#include <memory.h>
#endif

#include "cose.h"
#include "cose_int.h"
#include "configure.h"
#include "crypto.h"

#if INCLUDE_ENCRYPT || INCLUDE_ENCRYPT0 || INCLUDE_MAC || INCLUDE_MAC0
extern bool BuildContextBytes(COSE * pcose, int algID, size_t cbitKey, byte ** ppbContext, size_t * pcbContext, CBOR_CONTEXT_COMMA cose_errback * perr);
#endif

#if INCLUDE_ENCRYPT || INCLUDE_MAC
COSE* RecipientRoot = NULL;

/*! \private
* @brief Test if a HCOSE_RECIPIENT handle is valid
*
*  Internal function to test if a recipient handle is valid.
*  This will start returning invalid results and cause the code to
*  crash if handles are not released before the memory that underlies them
*  is deallocated.  This is an issue of a block allocator is used since
*  in that case it is common to allocate memory but never to de-allocate it
*  and just do that in a single big block.
*
*  @param h handle to be validated
*  @returns result of check
*/
bool IsValidRecipientHandle(HCOSE_RECIPIENT h)
{
	COSE_RecipientInfo * p = (COSE_RecipientInfo *)h;

	if (p == NULL) return false;
	return _COSE_IsInList(RecipientRoot, &p->m_encrypt.m_message);
}

HCOSE_RECIPIENT COSE_Recipient_Init(COSE_INIT_FLAGS flags, CBOR_CONTEXT_COMMA cose_errback * perr)
{
	CHECK_CONDITION(flags == COSE_INIT_FLAGS_NONE, COSE_ERR_INVALID_PARAMETER);
	COSE_RecipientInfo * pobj = (COSE_RecipientInfo *)COSE_CALLOC(1, sizeof(COSE_RecipientInfo), context);
	CHECK_CONDITION(pobj != NULL, COSE_ERR_OUT_OF_MEMORY);

	if (!_COSE_Init(flags | COSE_INIT_FLAGS_NO_CBOR_TAG, &pobj->m_encrypt.m_message, COSE_recipient_object, CBOR_CONTEXT_PARAM_COMMA perr)) {
		_COSE_Recipient_Free(pobj);
		return NULL;
	}

	_COSE_InsertInList(&RecipientRoot, &pobj->m_encrypt.m_message);
	return (HCOSE_RECIPIENT)pobj;

errorReturn:
	return NULL;
}

bool COSE_Recipient_Free(HCOSE_RECIPIENT hRecipient)
{
	if (IsValidRecipientHandle(hRecipient)) {
		COSE_RecipientInfo * p = (COSE_RecipientInfo *)hRecipient;
		_COSE_RemoveFromList(&RecipientRoot, &p->m_encrypt.m_message);

		_COSE_Recipient_Free(p);
		return true;
	}

	return false;
}

HCOSE_RECIPIENT COSE_Recipient_from_shared_secret(byte * rgbKey, int cbKey, byte * rgbKid, int cbKid, CBOR_CONTEXT_COMMA cose_errback * perr)
{
	HCOSE_RECIPIENT hRecipient = NULL;

	hRecipient = COSE_Recipient_Init(0, CBOR_CONTEXT_PARAM_COMMA perr);
	if (hRecipient == NULL) goto errorReturn;

	if (!COSE_Recipient_SetKey_secret(hRecipient, rgbKey, cbKey, rgbKid, cbKid, perr)) goto errorReturn;

	return hRecipient;

errorReturn:
	if (hRecipient != NULL) COSE_Recipient_Free(hRecipient);
	return NULL;
}



COSE_RecipientInfo * _COSE_Recipient_Init_From_Object(cn_cbor * cbor, CBOR_CONTEXT_COMMA cose_errback * perr)
{
	COSE_RecipientInfo * pRecipient = NULL;

	pRecipient = (COSE_RecipientInfo *)COSE_CALLOC(1, sizeof(COSE_RecipientInfo), context);
	CHECK_CONDITION(pRecipient != NULL, COSE_ERR_OUT_OF_MEMORY);

	CHECK_CONDITION(cbor->type == CN_CBOR_ARRAY, COSE_ERR_INVALID_PARAMETER);

	if (_COSE_Enveloped_Init_From_Object(cbor, &pRecipient->m_encrypt, CBOR_CONTEXT_PARAM_COMMA perr) == NULL) {
		goto errorReturn;
	}

	_COSE_InsertInList(&RecipientRoot, &pRecipient->m_encrypt.m_message);

	return pRecipient;

errorReturn:
	if (pRecipient != NULL) _COSE_Recipient_Free(pRecipient);
	return NULL;
}

void _COSE_Recipient_Free(COSE_RecipientInfo * pRecipient)
{
	if (pRecipient->m_encrypt.m_message.m_refCount > 1) {
		pRecipient->m_encrypt.m_message.m_refCount--;
		return;
	}

	COSE_FREE(pRecipient, &pRecipient->m_encrypt.m_message.m_allocContext);

	return;
}
#endif

#if INCLUDE_ENCRYPT || INCLUDE_ENCRYPT0 || INCLUDE_MAC || INCLUDE_MAC0
#if defined(USE_HKDF_SHA2) || defined(USE_HKDF_AES)
/**
* Perform an AES-CCM Decryption operation
*
* @param[in]	COSE *		Pointer to COSE Encryption context object
* @param[in]	int			Alorithm key is being generated for
* @param[in]	cn_cbor *	Private key
* @param[in]	cn_cbor *	Public Key
* @param[out]	byte *		Buffer to return new key in	
* @param[in]	size_t       Size of key to create in bits
* @param[in]    size_t		Size of digest function
* @param[in]	cbor_context * Allocation context
* @param[out]	cose_errback * Returned error information
* @return                   Did the function succeed?
*/

static bool HKDF_X(COSE * pCose, bool fHMAC, bool fECDH, bool fStatic, bool fSend, int algResult, const cn_cbor * pKeyPrivate, const cn_cbor * pKeyPublic, byte * pbKey, size_t cbitKey, size_t cbitHash, CBOR_CONTEXT_COMMA cose_errback * perr)
{
	byte * pbContext = NULL;
	size_t cbContext;
	bool fRet = false;
	const cn_cbor * cn;
	byte rgbDigest[512 / 8];
	size_t cbDigest;
	byte * pbSecret = NULL;
	size_t cbSecret = 0;

	if (!BuildContextBytes(pCose, algResult, cbitKey, &pbContext, &cbContext, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;


	if (fECDH) {
#ifdef USE_ECDH
		cn_cbor * pkeyMessage;

		if (pKeyPrivate != NULL) {
			cn = cn_cbor_mapget_int(pKeyPrivate, COSE_Key_Type);
			CHECK_CONDITION((cn != NULL) && (cn->type == CN_CBOR_UINT), COSE_ERR_INVALID_PARAMETER);
			CHECK_CONDITION(cn->v.uint == COSE_Key_Type_EC2, COSE_ERR_INVALID_PARAMETER);
		}

		if (pKeyPublic != NULL) {
			cn = cn_cbor_mapget_int(pKeyPublic, COSE_Key_Type);
			CHECK_CONDITION((cn != NULL) && (cn->type == CN_CBOR_UINT), COSE_ERR_INVALID_PARAMETER);
			CHECK_CONDITION(cn->v.uint == COSE_Key_Type_EC2, COSE_ERR_INVALID_PARAMETER);
		}

		if (fSend) {
			CHECK_CONDITION(pKeyPublic != NULL, COSE_ERR_INVALID_PARAMETER);
			pkeyMessage = (cn_cbor *) pKeyPrivate;

			if (!ECDH_ComputeSecret(pCose, &pkeyMessage, pKeyPublic, &pbSecret, &cbSecret, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
			if (!fStatic && pkeyMessage->parent == NULL) {
				if (!_COSE_map_put(pCose, COSE_Header_ECDH_EPHEMERAL, pkeyMessage, COSE_UNPROTECT_ONLY, perr)) goto errorReturn;
			}
		}
		else {

			pkeyMessage = _COSE_map_get_int(pCose, fStatic ? COSE_Header_ECDH_STATIC : COSE_Header_ECDH_EPHEMERAL, COSE_BOTH, perr);

			CHECK_CONDITION(pkeyMessage != NULL, COSE_ERR_INVALID_PARAMETER);
			CHECK_CONDITION(pKeyPrivate != NULL, COSE_ERR_INVALID_PARAMETER);

			if (!ECDH_ComputeSecret(pCose, (cn_cbor **)&pKeyPrivate, pkeyMessage, &pbSecret, &cbSecret, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		}
#else
                goto errorReturn;
#endif
	}
	else {
		CHECK_CONDITION(pKeyPrivate != NULL, COSE_ERR_INVALID_PARAMETER);
			cn = cn_cbor_mapget_int(pKeyPrivate, COSE_Key_Type);
			CHECK_CONDITION((cn != NULL) && (cn->type == CN_CBOR_UINT), COSE_ERR_INVALID_PARAMETER);
			CHECK_CONDITION(cn->v.uint == COSE_Key_Type_OCTET, COSE_ERR_INVALID_PARAMETER);

		CHECK_CONDITION(cn->v.sint == 4, COSE_ERR_INVALID_PARAMETER);

		cn = cn_cbor_mapget_int(pKeyPrivate, -1);
		CHECK_CONDITION((cn != NULL) && (cn->type == CN_CBOR_BYTES), COSE_ERR_INVALID_PARAMETER);
		pbSecret = (byte *) cn->v.bytes;
		cbSecret = cn->length;
	}

	if (fHMAC) {
#ifdef USE_HKDF_SHA2
		if (!HKDF_Extract(pCose, pbSecret, cbSecret, cbitHash, rgbDigest, &cbDigest, perr)) goto errorReturn;

		if (!HKDF_Expand(pCose, cbitHash, rgbDigest, cbDigest, pbContext, cbContext, pbKey, cbitKey / 8, perr)) goto errorReturn;
#else
                goto errorReturn;
#endif
	}
	else {
#ifdef USE_HKDF_AES
		if (!HKDF_AES_Expand(pCose, cbitHash, pbSecret, cbSecret, pbContext, cbContext, pbKey, cbitKey / 8, perr)) goto errorReturn;
#else
                goto errorReturn;
#endif
	}
	fRet = true;

errorReturn:
	if (fECDH && pbSecret != NULL) {
		memset(pbSecret, 0, cbSecret);
		COSE_FREE(pbSecret, context);
	}
	memset(rgbDigest, 0, sizeof(rgbDigest));
	if (pbContext != NULL) COSE_FREE(pbContext, context);
	return fRet;
}
#endif // defined(USE_HKDF_SHA2) || defined(USE_HKDF_AES)

bool _COSE_Recipient_decrypt(COSE_RecipientInfo * pRecip, COSE_RecipientInfo * pRecipUse, int algIn, size_t cbitKeyOut, byte * pbKeyOut, cose_errback * perr)
{
	int alg;
	const cn_cbor * cn = NULL;
	COSE_RecipientInfo * pRecip2;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context;
#endif
	byte * pbAuthData = NULL;
	byte * pbProtected = NULL;
	COSE_Enveloped * pcose = &pRecip->m_encrypt;
	cn_cbor * cnBody = NULL;
	byte * pbContext = NULL;
	byte * pbSecret = NULL;
	int cbKey2;
	byte * pbKeyX = NULL;
	int cbitKeyX = 0;
	byte rgbKey[256 / 8];

	UNUSED(rgbKey);
	UNUSED(cbKey2);
	UNUSED(pRecipUse);
	UNUSED(algIn);
	UNUSED(pcose);

#ifdef USE_CBOR_CONTEXT
	context = &pcose->m_message.m_allocContext;
#else
	UNUSED(pcose);
#endif

	cn = _COSE_map_get_int(&pRecip->m_encrypt.m_message, COSE_Header_Algorithm, COSE_BOTH, perr);
	if (cn == NULL) {
	errorReturn:
		if (pbContext != NULL) COSE_FREE(pbContext, context);
		if (pbProtected != NULL) COSE_FREE(pbProtected, context);
		if (pbAuthData != NULL) COSE_FREE(pbAuthData, context);
		if (pbSecret != NULL) COSE_FREE(pbSecret, context);
		return false;
	}
	CHECK_CONDITION(cn->type != CN_CBOR_TEXT, COSE_ERR_UNKNOWN_ALGORITHM);
	CHECK_CONDITION((cn->type == CN_CBOR_UINT) || (cn->type == CN_CBOR_INT), COSE_ERR_INVALID_PARAMETER);
	alg = (int)cn->v.uint;

	CHECK_CONDITION(pbKeyOut != NULL, COSE_ERR_INVALID_PARAMETER);

	switch (alg) {
	case COSE_Algorithm_Direct:
		CHECK_CONDITION(pRecip->m_pkey != NULL, COSE_ERR_INVALID_PARAMETER);
		cn = cn_cbor_mapget_int(pRecip->m_pkey, -1);
		CHECK_CONDITION((cn != NULL) && (cn->type == CN_CBOR_BYTES), COSE_ERR_INVALID_PARAMETER);
		CHECK_CONDITION(((size_t)cn->length == cbitKeyOut / 8), COSE_ERR_INVALID_PARAMETER);
		memcpy(pbKeyOut, cn->v.bytes, cn->length);

		return true;

#ifdef USE_AES_KW_128
	case COSE_Algorithm_AES_KW_128:
		cbitKeyX = 128;
		break;
#endif

#ifdef USE_AES_KW_192
	case COSE_Algorithm_AES_KW_192:
		cbitKeyX = 192;
		break;
#endif

#ifdef USE_AES_KW_256
	case COSE_Algorithm_AES_KW_256:
		cbitKeyX = 192;
		break;
#endif

#ifdef USE_Direct_HKDF_AES_128
	case COSE_Algorithm_Direct_HKDF_AES_128:
		break;
#endif

#ifdef USE_Direct_HKDF_AES_256
	case COSE_Algorithm_Direct_HKDF_AES_256:
		break;
#endif

#ifdef USE_Direct_HKDF_HMAC_SHA_256
	case COSE_Algorithm_Direct_HKDF_HMAC_SHA_256:
		break;
#endif

#ifdef USE_Direct_HKDF_HMAC_SHA_512
	case COSE_Algorithm_Direct_HKDF_HMAC_SHA_512:
		break;
#endif

#ifdef USE_ECDH_ES_HKDF_256
	case COSE_Algorithm_ECDH_ES_HKDF_256:
		break;
#endif

#ifdef USE_ECDH_ES_HKDF_512
	case COSE_Algorithm_ECDH_ES_HKDF_512:
		break;
#endif

#ifdef USE_ECDH_SS_HKDF_256
	case COSE_Algorithm_ECDH_SS_HKDF_256:
		break;
#endif

#ifdef USE_ECDH_SS_HKDF_512
	case COSE_Algorithm_ECDH_SS_HKDF_512:
		break;
#endif

#ifdef USE_ECDH_ES_A128KW
	case COSE_Algorithm_ECDH_ES_A128KW:
		break;
#endif

#ifdef USE_ECDH_ES_A192KW
	case COSE_Algorithm_ECDH_ES_A192KW:
		break;
#endif

#ifdef USE_ECDH_ES_A256KW
	case COSE_Algorithm_ECDH_ES_A256KW:
		break;
#endif

#ifdef USE_ECDH_SS_A128KW
	case COSE_Algorithm_ECDH_SS_A128KW:
		break;
#endif

#ifdef USE_ECDH_SS_A192KW
	case COSE_Algorithm_ECDH_SS_A192KW:
		break;
#endif

#ifdef USE_ECDH_SS_A256KW
	case COSE_Algorithm_ECDH_SS_A256KW:
		break;
#endif

	default:
		FAIL_CONDITION(COSE_ERR_UNKNOWN_ALGORITHM);
		break;
	}

	if (pcose->m_recipientFirst != NULL) {
		//  If there is a recipient - ask it for the key
		CHECK_CONDITION(cbitKeyX != 0, COSE_ERR_INVALID_PARAMETER);
		pbKeyX = COSE_CALLOC(cbitKeyX / 8, 1, context);
		CHECK_CONDITION(pbKeyX != NULL, COSE_ERR_OUT_OF_MEMORY);

		for (pRecip2 = pcose->m_recipientFirst; pRecip2 != NULL; pRecip2 = pRecip->m_recipientNext) {
			if (_COSE_Recipient_decrypt(pRecip2, NULL, alg, cbitKeyX, pbKeyX, perr)) break;
		}
		CHECK_CONDITION(pRecip2 != NULL, COSE_ERR_NO_RECIPIENT_FOUND);
	}

	cnBody = _COSE_arrayget_int(&pcose->m_message, INDEX_BODY);
	CHECK_CONDITION(cnBody != NULL, COSE_ERR_INVALID_PARAMETER);

	switch (alg) {
#ifdef USE_AES_KW_128
	case COSE_Algorithm_AES_KW_128:
		if (pbKeyX != NULL) {
			int x = cbitKeyOut / 8;
			if (!AES_KW_Decrypt((COSE_Enveloped *)pcose, pbKeyX, cbitKeyX, cnBody->v.bytes, cnBody->length, pbKeyOut, &x, perr)) goto errorReturn;
		}
		else {
			CHECK_CONDITION(pRecip->m_pkey != NULL, COSE_ERR_INVALID_PARAMETER);
			int x = cbitKeyOut / 8;
			cn = cn_cbor_mapget_int(pRecip->m_pkey, -1);
			CHECK_CONDITION((cn != NULL) && (cn->type == CN_CBOR_BYTES), COSE_ERR_INVALID_PARAMETER);

			if (!AES_KW_Decrypt((COSE_Enveloped *)pcose, cn->v.bytes, cn->length * 8, cnBody->v.bytes, cnBody->length, pbKeyOut, &x, perr)) goto errorReturn;
		}
		break;
#endif

#ifdef USE_AES_KW_192
	case COSE_Algorithm_AES_KW_192:
		if (pbKeyX != NULL) {
			int x = cbitKeyOut / 8;
			if (!AES_KW_Decrypt((COSE_Enveloped *)pcose, pbKeyX, cbitKeyX, cnBody->v.bytes, cnBody->length, pbKeyOut, &x, perr)) goto errorReturn;
		}
		else {
			CHECK_CONDITION(pRecip->m_pkey != NULL, COSE_ERR_INVALID_PARAMETER);
			int x = cbitKeyOut / 8;
			cn = cn_cbor_mapget_int(pRecip->m_pkey, -1);
			CHECK_CONDITION((cn != NULL) && (cn->type == CN_CBOR_BYTES), COSE_ERR_INVALID_PARAMETER);

			if (!AES_KW_Decrypt((COSE_Enveloped *)pcose, cn->v.bytes, cn->length * 8, cnBody->v.bytes, cnBody->length, pbKeyOut, &x, perr)) goto errorReturn;
		}
		break;
#endif

#ifdef USE_AES_KW_256
	case COSE_Algorithm_AES_KW_256:
		if (pbKeyX != NULL) {
			int x = cbitKeyOut / 8;
			if (!AES_KW_Decrypt((COSE_Enveloped *)pcose, pbKeyX, cbitKeyX, cnBody->v.bytes, cnBody->length, pbKeyOut, &x, perr)) goto errorReturn;
		}
		else {
			CHECK_CONDITION(pRecip->m_pkey != NULL, COSE_ERR_INVALID_PARAMETER);
			int x = cbitKeyOut / 8;
			cn = cn_cbor_mapget_int(pRecip->m_pkey, -1);
			CHECK_CONDITION((cn != NULL) && (cn->type == CN_CBOR_BYTES), COSE_ERR_INVALID_PARAMETER);

			if (!AES_KW_Decrypt((COSE_Enveloped *)pcose, cn->v.bytes, cn->length * 8, cnBody->v.bytes, cnBody->length, pbKeyOut, &x, perr)) goto errorReturn;
		}
		break;
#endif

#ifdef USE_Direct_HKDF_HMAC_SHA_256
	case COSE_Algorithm_Direct_HKDF_HMAC_SHA_256:
		if (!HKDF_X(&pcose->m_message, true, false, false, false, algIn, pRecip->m_pkey, NULL, pbKeyOut, cbitKeyOut, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_Direct_HKDF_HMAC_SHA_512
	case COSE_Algorithm_Direct_HKDF_HMAC_SHA_512:
		if (!HKDF_X(&pcose->m_message, true, false, false, false, algIn, pRecip->m_pkey, NULL, pbKeyOut, cbitKeyOut, 512, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_Direct_HKDF_AES_128
	case COSE_Algorithm_Direct_HKDF_AES_128:
		if (!HKDF_X(&pcose->m_message, false, false, false, false, algIn, pRecip->m_pkey, NULL, pbKeyOut, cbitKeyOut, 128, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_Direct_HKDF_AES_256
	case COSE_Algorithm_Direct_HKDF_AES_256:
		if (!HKDF_X(&pcose->m_message, false, false, false, false, algIn, pRecip->m_pkey, NULL, pbKeyOut, cbitKeyOut, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_ES_HKDF_256
	case COSE_Algorithm_ECDH_ES_HKDF_256:
		if (!HKDF_X(&pcose->m_message, true, true, false, false, algIn, pRecip->m_pkey, NULL, pbKeyOut, cbitKeyOut, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_ES_HKDF_512
	case COSE_Algorithm_ECDH_ES_HKDF_512:
		if (!HKDF_X(&pcose->m_message, true, true, false, false, algIn, pRecip->m_pkey, NULL, pbKeyOut, cbitKeyOut, 512, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_SS_HKDF_256
	case COSE_Algorithm_ECDH_SS_HKDF_256:
		if (!HKDF_X(&pcose->m_message, true, true, true, false, algIn, pRecip->m_pkey, NULL, pbKeyOut, cbitKeyOut, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_SS_HKDF_512
	case COSE_Algorithm_ECDH_SS_HKDF_512:
		if (!HKDF_X(&pcose->m_message, true, true, true, false, algIn, pRecip->m_pkey, NULL, pbKeyOut, cbitKeyOut, 512, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_ES_A128KW
	case COSE_Algorithm_ECDH_ES_A128KW:
		if (!HKDF_X(&pcose->m_message, true, true, false, false, COSE_Algorithm_AES_KW_128, pRecip->m_pkey, NULL, rgbKey, 128, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;

		if (!AES_KW_Decrypt((COSE_Enveloped *)pcose, rgbKey, 128, cnBody->v.bytes, cnBody->length, pbKeyOut, &cbKey2, perr)) goto errorReturn;

		break;
#endif

#ifdef USE_ECDH_ES_A192KW
	case COSE_Algorithm_ECDH_ES_A192KW:
		if (!HKDF_X(&pcose->m_message, true, true, false, false, COSE_Algorithm_AES_KW_192, pRecip->m_pkey, NULL, rgbKey, 192, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;

		if (!AES_KW_Decrypt((COSE_Enveloped *)pcose, rgbKey, 192, cnBody->v.bytes, cnBody->length, pbKeyOut, &cbKey2, perr)) goto errorReturn;

		break;
#endif

#ifdef USE_ECDH_ES_A256KW
	case COSE_Algorithm_ECDH_ES_A256KW:
		if (!HKDF_X(&pcose->m_message, true, true, false, false, COSE_Algorithm_AES_KW_256, pRecip->m_pkey, NULL, rgbKey, 256, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;

		if (!AES_KW_Decrypt((COSE_Enveloped *)pcose, rgbKey, 256, cnBody->v.bytes, cnBody->length, pbKeyOut, &cbKey2, perr)) goto errorReturn;

		break;
#endif

#ifdef USE_ECDH_SS_A128KW
	case COSE_Algorithm_ECDH_SS_A128KW:
		if (!HKDF_X(&pcose->m_message, true, true, true, false, COSE_Algorithm_AES_KW_128, pRecip->m_pkey, NULL, rgbKey, 128, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;

		if (!AES_KW_Decrypt((COSE_Enveloped *)pcose, rgbKey, 128, cnBody->v.bytes, cnBody->length, pbKeyOut, &cbKey2, perr)) goto errorReturn;

		break;
#endif

#ifdef USE_ECDH_SS_A192KW
	case COSE_Algorithm_ECDH_SS_A192KW:
		if (!HKDF_X(&pcose->m_message, true, true, true, false, COSE_Algorithm_AES_KW_192, pRecip->m_pkey, NULL, rgbKey, 192, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;

		if (!AES_KW_Decrypt((COSE_Enveloped *)pcose, rgbKey, 192, cnBody->v.bytes, cnBody->length, pbKeyOut, &cbKey2, perr)) goto errorReturn;

		break;
#endif

#ifdef USE_ECDH_SS_A256KW
	case COSE_Algorithm_ECDH_SS_A256KW:
		if (!HKDF_X(&pcose->m_message, true, true, true, false, COSE_Algorithm_AES_KW_256, pRecip->m_pkey, NULL, rgbKey, 256, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;

		if (!AES_KW_Decrypt((COSE_Enveloped *)pcose, rgbKey, 256, cnBody->v.bytes, cnBody->length, pbKeyOut, &cbKey2, perr)) goto errorReturn;

		break;
#endif

	default:
		FAIL_CONDITION(COSE_ERR_UNKNOWN_ALGORITHM);
		break;
	}

	return true;
}

bool _COSE_Recipient_encrypt(COSE_RecipientInfo * pRecipient, const byte * pbContent, size_t cbContent, cose_errback * perr)
{
	int alg;
	int t = 0;
	COSE_RecipientInfo * pri;
	const cn_cbor * cn_Alg = NULL;
	byte * pbAuthData = NULL;
	cn_cbor * ptmp = NULL;
	size_t cbitKey;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = NULL;
#endif
	cn_cbor_errback cbor_error;
	bool fRet = false;
	byte * pbContext = NULL;
	byte rgbKey[256 / 8];
	byte * pbSecret = NULL;
	byte * pbKey = NULL;
	size_t cbKey = 0;

	UNUSED(pbContent);
	UNUSED(cbContent);

#ifdef USE_CBOR_CONTEXT
	context = &pRecipient->m_encrypt.m_message.m_allocContext;
#endif // USE_CBOR_CONTEXT

	cn_Alg = _COSE_map_get_int(&pRecipient->m_encrypt.m_message, COSE_Header_Algorithm, COSE_BOTH, perr);
	if (cn_Alg == NULL) goto errorReturn;

	CHECK_CONDITION(cn_Alg->type != CN_CBOR_TEXT, COSE_ERR_UNKNOWN_ALGORITHM);
	CHECK_CONDITION((cn_Alg->type == CN_CBOR_UINT) || (cn_Alg->type == CN_CBOR_INT), COSE_ERR_INVALID_PARAMETER);
	alg = (int)cn_Alg->v.uint;

	//  Get the key size

	switch (alg) {
	case COSE_Algorithm_Direct:
#ifdef USE_Direct_HKDF_HMAC_SHA_256
	case COSE_Algorithm_Direct_HKDF_HMAC_SHA_256:
#endif

#ifdef USE_Direct_HKDF_HMAC_SHA_512
	case COSE_Algorithm_Direct_HKDF_HMAC_SHA_512:
#endif
#ifdef USE_Direct_HKDF_AES_128
	case COSE_Algorithm_Direct_HKDF_AES_128:
#endif
#ifdef USE_Direct_HKDF_AES_256
	case COSE_Algorithm_Direct_HKDF_AES_256:
#endif
#ifdef USE_ECDH_ES_HKDF_256
	case COSE_Algorithm_ECDH_ES_HKDF_256:
#endif
#ifdef USE_ECDH_ES_HKDF_512
	case COSE_Algorithm_ECDH_ES_HKDF_512:
#endif
#ifdef USE_ECDH_SS_HKDF_256
	case COSE_Algorithm_ECDH_SS_HKDF_256:
#endif
#ifdef USE_ECDH_SS_HKDF_512
	case COSE_Algorithm_ECDH_SS_HKDF_512:
#endif
		//  This is a NOOP
		cbitKey = 0;
		CHECK_CONDITION(pRecipient->m_encrypt.m_recipientFirst == NULL, COSE_ERR_INVALID_PARAMETER);
		break;

#ifdef USE_AES_KW_128
	case COSE_Algorithm_AES_KW_128:
		cbitKey = 128;
		break;
#endif

#ifdef USE_ECDH_ES_A128KW
	case COSE_Algorithm_ECDH_ES_A128KW:
		cbitKey = 128;
		break;
#endif

#ifdef USE_ECDH_SS_A128KW
	case COSE_Algorithm_ECDH_SS_A128KW:
		cbitKey = 128;
		break;
#endif

#ifdef USE_AES_KW_192
	case COSE_Algorithm_AES_KW_192:
		cbitKey = 192;
		break;
#endif

#ifdef USE_ECDH_ES_A192KW
	case COSE_Algorithm_ECDH_ES_A192KW:
		cbitKey = 192;
		break;
#endif

#ifdef USE_ECDH_SS_A192KW
	case COSE_Algorithm_ECDH_SS_A192KW:
		cbitKey = 192;
		break;
#endif

#ifdef USE_AES_KW_256
	case COSE_Algorithm_AES_KW_256:
		cbitKey = 256;
		break;
#endif

#ifdef USE_ECDH_ES_A256KW
	case COSE_Algorithm_ECDH_ES_A256KW:
		cbitKey = 256;
		break;
#endif

#ifdef USE_ECDH_SS_A256KW
	case COSE_Algorithm_ECDH_SS_A256KW:
		cbitKey = 256;
		break;
#endif

	default:
		FAIL_CONDITION(COSE_ERR_UNKNOWN_ALGORITHM);
	}

	//  If we are doing direct encryption - then recipient generates the key

	if (pRecipient->m_encrypt.m_recipientFirst != NULL) {
		t = 0;
		for (pri = pRecipient->m_encrypt.m_recipientFirst; pri != NULL; pri = pri->m_recipientNext) {
			if (pri->m_encrypt.m_message.m_flags & 1) {
				t |= 1;
				pbKey = _COSE_RecipientInfo_generateKey(pri, alg, cbitKey, perr);
				if (pbKey == NULL) goto errorReturn;
				cbKey = cbitKey / 8;
			}
			else {
				t |= 2;
			}
		}
		CHECK_CONDITION(t != 3, COSE_ERR_INVALID_PARAMETER);

		// Do we need to generate a random key at this point -
			//   This is only true if we both haven't done it and and we have a recipient to encrypt it.
			
			if (t == 2) {
			pbKey = (byte *)COSE_CALLOC(cbitKey / 8, 1, context);
			CHECK_CONDITION(pbKey != NULL, COSE_ERR_OUT_OF_MEMORY);
			cbKey = cbitKey / 8;
			rand_bytes(pbKey, cbKey);
		}
	}

	//  Build protected headers

	const cn_cbor * cbProtected = _COSE_encode_protected(&pRecipient->m_encrypt.m_message, perr);
	if (cbProtected == NULL) goto errorReturn;

	//  Build authenticated data
	size_t cbAuthData = 0;
	if (!_COSE_Encrypt_Build_AAD(&pRecipient->m_encrypt.m_message, &pbAuthData, &cbAuthData, "Recipient", perr)) goto errorReturn;

	switch (alg) {

	case COSE_Algorithm_Direct:
#ifdef USE_Direct_HKDF_HMAC_SHA_256
	case COSE_Algorithm_Direct_HKDF_HMAC_SHA_256:
#endif

#ifdef USE_Direct_HKDF_HMAC_SHA_512
	case COSE_Algorithm_Direct_HKDF_HMAC_SHA_512:
#endif
#ifdef USE_Direct_HKDF_AES_128
	case COSE_Algorithm_Direct_HKDF_AES_128:
#endif
#ifdef USE_Direct_HKDF_AES_256
	case COSE_Algorithm_Direct_HKDF_AES_256:
#endif
#ifdef USE_ECDH_ES_HKDF_256
	case COSE_Algorithm_ECDH_ES_HKDF_256:
#endif
#ifdef USE_ECDH_ES_HKDF_512
	case COSE_Algorithm_ECDH_ES_HKDF_512:
#endif
#ifdef USE_ECDH_SS_HKDF_256
	case COSE_Algorithm_ECDH_SS_HKDF_256:
#endif
#ifdef USE_ECDH_SS_HKDF_512
	case COSE_Algorithm_ECDH_SS_HKDF_512:
#endif
		ptmp = cn_cbor_data_create(NULL, 0, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
		CHECK_CONDITION_CBOR(ptmp != NULL, cbor_error);
		CHECK_CONDITION_CBOR(_COSE_array_replace(&pRecipient->m_encrypt.m_message, ptmp, INDEX_BODY, CBOR_CONTEXT_PARAM_COMMA &cbor_error), cbor_error);
		ptmp = NULL;
		break;


#ifdef USE_AES_KW_128
	case COSE_Algorithm_AES_KW_128:
		if (pRecipient->m_pkey != NULL) {
			cn_cbor * pK = cn_cbor_mapget_int(pRecipient->m_pkey, -1);
			CHECK_CONDITION(pK != NULL, COSE_ERR_INVALID_PARAMETER);
			if (!AES_KW_Encrypt(pRecipient, pK->v.bytes, (int)pK->length * 8, pbContent, (int)cbContent, perr)) goto errorReturn;
		}
		else {
			if (!AES_KW_Encrypt(pRecipient, pbKey, (int)cbKey * 8, pbContent, (int)cbContent, perr)) goto errorReturn;
		}
		break;
#endif

#ifdef USE_AES_KW_192
	case COSE_Algorithm_AES_KW_192:
		if (pRecipient->m_pkey != NULL) {
			cn_cbor * pK = cn_cbor_mapget_int(pRecipient->m_pkey, -1);
			CHECK_CONDITION(pK != NULL, COSE_ERR_INVALID_PARAMETER);
			if (!AES_KW_Encrypt(pRecipient, pK->v.bytes, (int)pK->length * 8, pbContent, (int)cbContent, perr)) goto errorReturn;
		}
		else {
			if (!AES_KW_Encrypt(pRecipient, pbKey, (int)cbKey * 8, pbContent, (int)cbContent, perr)) goto errorReturn;
		}
		break;
#endif

#ifdef USE_AES_KW_256
	case COSE_Algorithm_AES_KW_256:
		if (pRecipient->m_pkey != NULL) {
			cn_cbor * pK = cn_cbor_mapget_int(pRecipient->m_pkey, -1);
			CHECK_CONDITION(pK != NULL, COSE_ERR_INVALID_PARAMETER);
			if (!AES_KW_Encrypt(pRecipient, pK->v.bytes, (int) pK->length*8, pbContent, (int) cbContent, perr)) goto errorReturn;
		}
		else {
			if (!AES_KW_Encrypt(pRecipient, pbKey, (int) cbKey*8, pbContent, (int) cbContent, perr)) goto errorReturn;
		}
		break;
#endif

#ifdef USE_ECDH_ES_A128KW
	case COSE_Algorithm_ECDH_ES_A128KW:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, true, true, false, true, COSE_Algorithm_AES_KW_128, NULL, pRecipient->m_pkey, rgbKey, 128, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		if (!AES_KW_Encrypt(pRecipient, rgbKey, 128, pbContent, (int)cbContent, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_ES_A192KW
	case COSE_Algorithm_ECDH_ES_A192KW:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, true, true, false, true, COSE_Algorithm_AES_KW_192, NULL, pRecipient->m_pkey, rgbKey, 192, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		if (!AES_KW_Encrypt(pRecipient, rgbKey, 192, pbContent, (int)cbContent, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_ES_A256KW
	case COSE_Algorithm_ECDH_ES_A256KW:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, true, true, false, true, COSE_Algorithm_AES_KW_256, NULL, pRecipient->m_pkey, rgbKey, 256, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		if (!AES_KW_Encrypt(pRecipient, rgbKey, 256, pbContent, (int)cbContent, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_SS_A128KW
	case COSE_Algorithm_ECDH_SS_A128KW:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, true, true, true, true, COSE_Algorithm_AES_KW_128, pRecipient->m_pkeyStatic, pRecipient->m_pkey, rgbKey, 128, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		if (!AES_KW_Encrypt(pRecipient, rgbKey, 128, pbContent, (int)cbContent, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_SS_A192KW
	case COSE_Algorithm_ECDH_SS_A192KW:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, true, true, true, true, COSE_Algorithm_AES_KW_192, pRecipient->m_pkeyStatic, pRecipient->m_pkey, rgbKey, 192, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		if (!AES_KW_Encrypt(pRecipient, rgbKey, 192, pbContent, (int)cbContent, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_SS_A256KW
	case COSE_Algorithm_ECDH_SS_A256KW:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, true, true, true, true, COSE_Algorithm_AES_KW_256, pRecipient->m_pkeyStatic, pRecipient->m_pkey, rgbKey, 256, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		if (!AES_KW_Encrypt(pRecipient, rgbKey, 256, pbContent, (int)cbContent, perr)) goto errorReturn;
		break;
#endif

	default:
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
	}

	for (pri = pRecipient->m_encrypt.m_recipientFirst; pri != NULL; pri = pri->m_recipientNext) {
		if (!_COSE_Recipient_encrypt(pri, pbKey, cbKey, perr)) goto errorReturn;
	}

	//  Figure out the clean up

	fRet = true;

errorReturn:
	memset(rgbKey, 0, sizeof(rgbKey));
	if (pbKey != NULL) {
		memset(pbKey, 0, cbKey);
		COSE_FREE(pbKey, context);
	}
	if (pbSecret != NULL) COSE_FREE(pbSecret, context);
	if (pbContext != NULL) COSE_FREE(pbContext, context);
	if (pbAuthData != NULL) COSE_FREE(pbAuthData, context);
	if (ptmp != NULL) cn_cbor_free(ptmp CBOR_CONTEXT_PARAM);
	return fRet;
}

byte * _COSE_RecipientInfo_generateKey(COSE_RecipientInfo * pRecipient, int algIn, size_t cbitKeySize, cose_errback * perr)
{
	int alg;
	const cn_cbor * cn_Alg = _COSE_map_get_int(&pRecipient->m_encrypt.m_message, COSE_Header_Algorithm, COSE_BOTH, perr);
	byte * pbContext = NULL;
	byte * pb = NULL;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = &pRecipient->m_encrypt.m_message.m_allocContext;
#endif
	const cn_cbor * pK;
	byte *pbSecret = NULL;

	UNUSED(algIn);

	CHECK_CONDITION(cn_Alg != NULL, COSE_ERR_INVALID_PARAMETER);
	CHECK_CONDITION((cn_Alg->type == CN_CBOR_UINT) || (cn_Alg->type == CN_CBOR_INT), COSE_ERR_INVALID_PARAMETER);
	alg = (int)cn_Alg->v.uint;

	_COSE_encode_protected(&pRecipient->m_encrypt.m_message, perr);

	pb = COSE_CALLOC(cbitKeySize / 8, 1, context);
	CHECK_CONDITION(pb != NULL, COSE_ERR_OUT_OF_MEMORY);

	switch (alg) {
	case COSE_Algorithm_Direct:
		CHECK_CONDITION(pRecipient->m_pkey != NULL, COSE_ERR_INVALID_PARAMETER);
			pK = cn_cbor_mapget_int(pRecipient->m_pkey, -1);
			CHECK_CONDITION((pK != NULL) && (pK->type == CN_CBOR_BYTES), COSE_ERR_INVALID_PARAMETER);
			CHECK_CONDITION((size_t)pK->length == cbitKeySize / 8, COSE_ERR_INVALID_PARAMETER);
			memcpy(pb, pK->v.bytes, cbitKeySize / 8);
	break;

#ifdef USE_Direct_HKDF_HMAC_SHA_256
	case COSE_Algorithm_Direct_HKDF_HMAC_SHA_256:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, true, false, false, true, algIn, pRecipient->m_pkey, NULL, pb, cbitKeySize, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_Direct_HKDF_HMAC_SHA_512
	case COSE_Algorithm_Direct_HKDF_HMAC_SHA_512:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, true, false, false, true, algIn, pRecipient->m_pkey, NULL, pb, cbitKeySize, 512, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_Direct_HKDF_AES_128
	case COSE_Algorithm_Direct_HKDF_AES_128:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, false, false, false, true, algIn, pRecipient->m_pkey, NULL, pb, cbitKeySize, 128, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_Direct_HKDF_AES_256
	case COSE_Algorithm_Direct_HKDF_AES_256:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, false, false, false, true, algIn, pRecipient->m_pkey, NULL, pb, cbitKeySize, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_ES_HKDF_256
	case COSE_Algorithm_ECDH_ES_HKDF_256:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, true, true, false, true, algIn, NULL, pRecipient->m_pkey, pb, cbitKeySize, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_ES_HKDF_512
	case COSE_Algorithm_ECDH_ES_HKDF_512:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, true, true, false, true, algIn, NULL, pRecipient->m_pkey, pb, cbitKeySize, 512, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_SS_HKDF_256
	case COSE_Algorithm_ECDH_SS_HKDF_256:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, true, true, true, true, algIn, pRecipient->m_pkeyStatic, pRecipient->m_pkey, pb, cbitKeySize, 256, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

#ifdef USE_ECDH_SS_HKDF_512
	case COSE_Algorithm_ECDH_SS_HKDF_512:
		if (!HKDF_X(&pRecipient->m_encrypt.m_message, true, true, true, true, algIn, pRecipient->m_pkeyStatic, pRecipient->m_pkey, pb, cbitKeySize, 512, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;
		break;
#endif

	default:
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
	}

	if (pbSecret != NULL) COSE_FREE(pbSecret, context);
	if (pbContext != NULL) COSE_FREE(pbContext, context);
	return pb;

errorReturn:

	if (pbSecret != NULL) COSE_FREE(pbSecret, context);
	if (pbContext != NULL) COSE_FREE(pbContext, context);
	if (pb != NULL) COSE_FREE(pb, context);
	return NULL;
}
#endif

#if INCLUDE_ENCRYPT || INCLUDE_MAC
bool COSE_Recipient_SetKey_secret(HCOSE_RECIPIENT hRecipient, const byte * rgbKey, int cbKey, const byte * rgbKid, int cbKid, cose_errback * perr)
{
	COSE_RecipientInfo * p;
	cn_cbor * cn_Temp = NULL;
	cn_cbor * cnTemp = NULL;
	cn_cbor_errback cbor_error;
	byte * pbTemp = NULL;
	byte * pbKey = NULL;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = NULL;
#endif

	CHECK_CONDITION(IsValidRecipientHandle(hRecipient), COSE_ERR_INVALID_HANDLE);
	CHECK_CONDITION(rgbKey != NULL, COSE_ERR_INVALID_PARAMETER);

	p = (COSE_RecipientInfo *)hRecipient;

#ifdef USE_CBOR_CONTEXT
	context = &p->m_encrypt.m_message.m_allocContext;
#endif

	cn_cbor * cnAlg = _COSE_map_get_int(&p->m_encrypt.m_message, COSE_Header_Algorithm, COSE_BOTH, perr);
	if (cnAlg != NULL) {
		CHECK_CONDITION(cnAlg->type == CN_CBOR_INT && cnAlg->v.sint == COSE_Algorithm_Direct, COSE_ERR_INVALID_PARAMETER);
	}
	else {
		cn_Temp = cn_cbor_int_create(COSE_Algorithm_Direct, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
		CHECK_CONDITION_CBOR(cn_Temp != NULL, cbor_error);
		if (!COSE_Recipient_map_put_int(hRecipient, COSE_Header_Algorithm, cn_Temp, COSE_UNPROTECT_ONLY, perr)) goto errorReturn;
		cn_Temp = NULL;
	}

	if (cbKid > 0) {
		pbTemp = (byte *)COSE_CALLOC(cbKid, 1, context);
		CHECK_CONDITION(pbTemp != NULL, COSE_ERR_OUT_OF_MEMORY);

		memcpy(pbTemp, rgbKid, cbKid);
		cnTemp = cn_cbor_data_create(pbTemp, cbKid, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
		CHECK_CONDITION_CBOR(cnTemp != NULL, cbor_error);
		pbTemp = NULL;

		if (!COSE_Recipient_map_put_int(hRecipient, COSE_Header_KID, cnTemp, COSE_UNPROTECT_ONLY, perr)) goto errorReturn;
	}

	pbKey = (byte *)COSE_CALLOC(cbKey, 1, context);
	CHECK_CONDITION(pbKey != NULL, COSE_ERR_OUT_OF_MEMORY);

	memcpy(pbKey, rgbKey, cbKey);

	cn_Temp = cn_cbor_map_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cn_Temp != NULL, cbor_error);

	cnTemp = cn_cbor_int_create(4, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cnTemp != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_mapput_int(cn_Temp, COSE_Key_Type, cnTemp, CBOR_CONTEXT_PARAM_COMMA &cbor_error), cbor_error);
	cnTemp = NULL;

	cnTemp = cn_cbor_data_create(pbKey, cbKey, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cnTemp != NULL, cbor_error);
	pbKey = NULL;
	CHECK_CONDITION_CBOR(cn_cbor_mapput_int(cn_Temp, -1, cnTemp, CBOR_CONTEXT_PARAM_COMMA &cbor_error), cbor_error);
	cnTemp = NULL;

	if (!COSE_Recipient_SetKey(hRecipient, cn_Temp, perr)) goto errorReturn;
	cn_Temp = NULL;

	return true;

errorReturn:
	if (cn_Temp != NULL) CN_CBOR_FREE(cn_Temp, context);
	if (cnTemp != NULL) CN_CBOR_FREE(cnTemp, context);
	if (pbTemp != NULL) COSE_FREE(pbTemp, context);
	if (pbKey != NULL) COSE_FREE(pbKey, context);
	return false;
}

bool COSE_Recipient_SetKey(HCOSE_RECIPIENT h, const cn_cbor * pKey, cose_errback * perr)
{
	COSE_RecipientInfo * p;

	CHECK_CONDITION(IsValidRecipientHandle(h), COSE_ERR_INVALID_HANDLE);
	CHECK_CONDITION(pKey != NULL, COSE_ERR_INVALID_PARAMETER);

	p = (COSE_RecipientInfo *)h;
	p->m_pkey = pKey;

	return true;

errorReturn:
	return false;
}

/*!
* @brief Set the senders static key for ECDH key agreement algorithms
*
* Set the Static private key to be used in computing ECDH key agreement
* operation.  
*
* Private portion of the key is not zeroed when the recipient object is
* released.
*
* @param h  Handle to the recipient object
* @param pKey pointer to COSE key structure contiaing the private key
* @param destination 0 - set nothing, 1 - set spk_kid, 2 - set spk
* @param perr location for return of error code
* @return true on success
*/

bool COSE_Recipient_SetSenderKey(HCOSE_RECIPIENT h, const cn_cbor * pKey, int destination, cose_errback * perr)
{
	COSE_RecipientInfo * p;
	bool f = false;
	cn_cbor * cn;
	cn_cbor * cn2 = NULL;
	cn_cbor * cn3 = NULL;
	cn_cbor_errback cbor_err;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = NULL;
#endif

	CHECK_CONDITION(IsValidRecipientHandle(h), COSE_ERR_INVALID_HANDLE);
	CHECK_CONDITION(pKey != NULL, COSE_ERR_INVALID_PARAMETER);

	p = (COSE_RecipientInfo *)h;

#ifdef USE_CBOR_CONTEXT
	context = &p->m_encrypt.m_message.m_allocContext;
#endif

	switch (destination) {
	case 0:
		break;

	case 1:
		cn = cn_cbor_mapget_int(pKey, COSE_Key_ID);
		CHECK_CONDITION(cn != NULL, COSE_ERR_INVALID_PARAMETER);
		cn2 = cn_cbor_clone(cn, CBOR_CONTEXT_PARAM_COMMA &cbor_err);
		CHECK_CONDITION_CBOR(cn2 != NULL, cbor_err);
		CHECK_CONDITION(_COSE_map_put(&p->m_encrypt.m_message, COSE_Header_ECDH_SPK_KID, cn2, COSE_UNPROTECT_ONLY, perr), perr->err);
		cn2 = NULL;
		break;

	case 2:
		cn2 = cn_cbor_map_create(CBOR_CONTEXT_PARAM_COMMA &cbor_err);
		CHECK_CONDITION_CBOR(cn2 != NULL, cbor_err);
		cn = cn_cbor_mapget_int(pKey, COSE_Key_Type);
		CHECK_CONDITION(cn != NULL, COSE_ERR_INVALID_PARAMETER);
		cn3 = cn_cbor_clone(cn, CBOR_CONTEXT_PARAM_COMMA &cbor_err);
		CHECK_CONDITION_CBOR(cn3 != NULL, cbor_err);
		CHECK_CONDITION_CBOR(cn_cbor_mapput_int(cn2, COSE_Key_Type, cn3, CBOR_CONTEXT_PARAM_COMMA &cbor_err), cbor_err);
		cn3 = NULL;
		cn = cn_cbor_mapget_int(pKey, COSE_Key_EC2_Curve);
		cn3 = cn_cbor_clone(cn, CBOR_CONTEXT_PARAM_COMMA &cbor_err);
		CHECK_CONDITION_CBOR(cn3 != NULL, cbor_err);
		CHECK_CONDITION_CBOR(cn_cbor_mapput_int(cn2, COSE_Key_EC2_Curve, cn3, CBOR_CONTEXT_PARAM_COMMA &cbor_err), cbor_err);
		cn3 = NULL;
		cn = cn_cbor_mapget_int(pKey, COSE_Key_EC2_X);
		cn3 = cn_cbor_clone(cn, CBOR_CONTEXT_PARAM_COMMA &cbor_err);
		CHECK_CONDITION_CBOR(cn3 != NULL, cbor_err);
		CHECK_CONDITION_CBOR(cn_cbor_mapput_int(cn2, COSE_Key_EC2_X, cn3, CBOR_CONTEXT_PARAM_COMMA &cbor_err), cbor_err);
		cn3 = NULL;
		cn = cn_cbor_mapget_int(pKey, COSE_Key_EC2_Y);
		cn3 = cn_cbor_clone(cn, CBOR_CONTEXT_PARAM_COMMA &cbor_err);
		CHECK_CONDITION_CBOR(cn3 != NULL, cbor_err);
		CHECK_CONDITION_CBOR(cn_cbor_mapput_int(cn2, COSE_Key_EC2_Y, cn3, CBOR_CONTEXT_PARAM_COMMA &cbor_err), cbor_err);
		cn3 = NULL;
		CHECK_CONDITION(_COSE_map_put(&p->m_encrypt.m_message, COSE_Header_ECDH_SPK, cn2, COSE_UNPROTECT_ONLY, perr), perr->err);
		cn2 = NULL;
		break;

	default:
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
	}

	p->m_pkeyStatic = pKey;

	f = true;
errorReturn:
	if (cn2 != NULL) CN_CBOR_FREE(cn2, context);
	if (cn3 != NULL) CN_CBOR_FREE(cn3, context);
	return f;
}

/*!
* @brief Set the application external data for authentication
*
* Recipient data objects support the authentication of external application
* supplied data.  This function is provided to supply that data to the library.
*
* The external data is not copied, nor will be it freed when the handle is released.
*
* @param hcose  Handle for the COSE recipient data object
* @param pbEternalData  point to the external data
* @param cbExternalData size of the external data
* @param perr  location to return errors
* @return result of the operation.
*/

bool COSE_Recipient_SetExternal(HCOSE_RECIPIENT hcose, const byte * pbExternalData, size_t cbExternalData, cose_errback * perr)
{
	if (!IsValidRecipientHandle(hcose)) {
		if (perr != NULL) perr->err = COSE_ERR_INVALID_HANDLE;
		return false;
	}

	return _COSE_SetExternal(&((COSE_RecipientInfo *)hcose)->m_encrypt.m_message, pbExternalData, cbExternalData, perr);
}


bool COSE_Recipient_map_put_int(HCOSE_RECIPIENT h, int key, cn_cbor * value, int flags, cose_errback * perr)
{
	CHECK_CONDITION(IsValidRecipientHandle(h), COSE_ERR_INVALID_HANDLE);
	CHECK_CONDITION(value != NULL, COSE_ERR_INVALID_PARAMETER);

	if (!_COSE_map_put(&((COSE_RecipientInfo *)h)->m_encrypt.m_message, key, value, flags, perr)) return false;

	if (key == COSE_Header_Algorithm) {
		if (value->type == CN_CBOR_INT) {
			switch (value->v.uint) {
			case COSE_Algorithm_Direct:
#ifdef USE_Direct_HKDF_AES_128
			case COSE_Algorithm_Direct_HKDF_AES_128:
#endif
#ifdef USE_Direct_HKDF_AES_256
			case COSE_Algorithm_Direct_HKDF_AES_256:
#endif
#ifdef USE_Direct_HKDF_HMAC_SHA_256
			case COSE_Algorithm_Direct_HKDF_HMAC_SHA_256:
#endif
#ifdef USE_Direct_HKDF_HMAC_SHA_512
			case COSE_Algorithm_Direct_HKDF_HMAC_SHA_512:
#endif
#ifdef USE_ECDH_ES_HKDF_256
			case COSE_Algorithm_ECDH_ES_HKDF_256:
#endif
#ifdef USE_ECDH_ES_HKDF_512
			case COSE_Algorithm_ECDH_ES_HKDF_512:
#endif
#ifdef USE_ECDH_SS_HKDF_256
			case COSE_Algorithm_ECDH_SS_HKDF_256:
#endif
#ifdef USE_ECDH_SS_HKDF_512
			case COSE_Algorithm_ECDH_SS_HKDF_512:
#endif
				((COSE_RecipientInfo *)h)->m_encrypt.m_message.m_flags |= 1;
				break;

			default:
				((COSE_RecipientInfo *)h)->m_encrypt.m_message.m_flags &= ~1;
				break;
			}
		}
		else {
			((COSE_RecipientInfo *)h)->m_encrypt.m_message.m_flags &= ~1;
		}
	}

	return true;

errorReturn:
	return false;
}
#endif

#if INCLUDE_ENCRYPT || INCLUDE_ENCRYPT0 || INCLUDE_MAC || INCLUDE_MAC0
bool BuildContextBytes(COSE * pcose, int algID, size_t cbitKey, byte ** ppbContext, size_t * pcbContext, CBOR_CONTEXT_COMMA cose_errback * perr)
{
	cn_cbor * pArray;
	cn_cbor_errback cbor_error;
	bool fReturn = false;
	cn_cbor * cnT = NULL;
	cn_cbor * cnArrayT = NULL;
	cn_cbor * cnParam;
	byte * pbContext = NULL;
	size_t cbContext;
	ssize_t written;

	pArray = cn_cbor_array_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(pArray != NULL, cbor_error);

	cnT = cn_cbor_int_create(algID, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cnT != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_array_append(pArray, cnT, &cbor_error), cbor_error);
	cnT = NULL;

	cnArrayT = cn_cbor_array_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cnArrayT != NULL, cbor_error);

	cnParam = _COSE_map_get_int(pcose, COSE_Header_KDF_U_name, COSE_BOTH, perr);
	if (cnParam != NULL) {
		cnT = cn_cbor_clone(cnParam, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	}
	else cnT = cn_cbor_null_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cnT != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_array_append(cnArrayT, cnT, &cbor_error), cbor_error);
	cnT = NULL;
	cnParam = NULL;

	cnParam = _COSE_map_get_int(pcose, COSE_Header_KDF_U_nonce, COSE_BOTH, perr);
	if (cnParam != NULL) {
		cnT = cn_cbor_clone(cnParam, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	}
	else cnT = cn_cbor_null_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cnT != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_array_append(cnArrayT, cnT, &cbor_error), cbor_error);
	cnT = NULL;
	cnParam = NULL;

	cnParam = _COSE_map_get_int(pcose, COSE_Header_KDF_U_other, COSE_BOTH, perr);
	if (cnParam != NULL) {
		cnT = cn_cbor_clone(cnParam, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	}
	else cnT = cn_cbor_null_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cnT != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_array_append(cnArrayT, cnT, &cbor_error), cbor_error);
	cnT = NULL;
	cnParam = NULL;

	CHECK_CONDITION_CBOR(cn_cbor_array_append(pArray, cnArrayT, &cbor_error), cbor_error);
	cnArrayT = NULL;

	cnArrayT = cn_cbor_array_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cnArrayT != NULL, cbor_error);

	cnParam = _COSE_map_get_int(pcose, COSE_Header_KDF_V_name, COSE_BOTH, perr);
	if (cnParam != NULL) {
		cnT = cn_cbor_clone(cnParam, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	}
	else cnT = cn_cbor_null_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cnT != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_array_append(cnArrayT, cnT, &cbor_error), cbor_error);
	cnT = NULL;
	cnParam = NULL;

	cnParam = _COSE_map_get_int(pcose, COSE_Header_KDF_V_nonce, COSE_BOTH, perr);
	if (cnParam != NULL) {
		cnT = cn_cbor_clone(cnParam, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	}
	else cnT = cn_cbor_null_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cnT != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_array_append(cnArrayT, cnT, &cbor_error), cbor_error);
	cnT = NULL;
	cnParam = NULL;

	cnParam = _COSE_map_get_int(pcose, COSE_Header_KDF_V_other, COSE_BOTH, perr);
	if (cnParam != NULL) {
		cnT = cn_cbor_clone(cnParam, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	}
	else cnT = cn_cbor_null_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cnT != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_array_append(cnArrayT, cnT, &cbor_error), cbor_error);
	cnT = NULL;
	cnParam = NULL;

	CHECK_CONDITION_CBOR(cn_cbor_array_append(pArray, cnArrayT, &cbor_error), cbor_error);
	cnArrayT = NULL;

	cnArrayT = cn_cbor_array_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cnArrayT != NULL, cbor_error);

	cnT = cn_cbor_int_create(cbitKey, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(cnT != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_array_append(cnArrayT, cnT, &cbor_error), cbor_error);
	cnT = NULL;

	cnParam = _COSE_arrayget_int(pcose, INDEX_PROTECTED);
	if (cnParam != NULL) {
		cnT = cn_cbor_clone(cnParam, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
		CHECK_CONDITION_CBOR(cnT != NULL, cbor_error);
		CHECK_CONDITION_CBOR(cn_cbor_array_append(cnArrayT, cnT, &cbor_error), cbor_error);
		cnT = NULL;
		cnParam = NULL;
	}

	cnParam = _COSE_map_get_int(pcose, COSE_Header_KDF_PUB_other, COSE_BOTH, perr);
	if (cnParam != NULL) {
		cnT = cn_cbor_clone(cnParam, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
		CHECK_CONDITION_CBOR(cnT != NULL, cbor_error);
		CHECK_CONDITION_CBOR(cn_cbor_array_append(cnArrayT, cnT, &cbor_error), cbor_error);
		cnT = NULL;
		cnParam = NULL;
	}

	CHECK_CONDITION_CBOR(cn_cbor_array_append(pArray, cnArrayT, &cbor_error), cbor_error);
	cnArrayT = NULL;


	cnParam = _COSE_map_get_int(pcose, COSE_Header_KDF_PRIV, COSE_BOTH, perr);
	if (cnParam != NULL) {
		cnT = cn_cbor_clone(cnParam, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
		CHECK_CONDITION_CBOR(cnT != NULL, cbor_error);
		CHECK_CONDITION_CBOR(cn_cbor_array_append(pArray, cnT, &cbor_error), cbor_error);
		cnT = NULL;
		cnParam = NULL;
	}

	cbContext = cn_cbor_encode_size(pArray);
	CHECK_CONDITION(cbContext > 0, COSE_ERR_CBOR);
	pbContext = (byte *)COSE_CALLOC(cbContext, 1, context);
	CHECK_CONDITION(pbContext != NULL, COSE_ERR_OUT_OF_MEMORY);
	written = cn_cbor_encoder_write(pbContext, 0, cbContext, pArray);
	CHECK_CONDITION(written >= 0 && (size_t)written == cbContext, COSE_ERR_CBOR);

	*ppbContext = pbContext;
	*pcbContext = cbContext;
	pbContext = NULL;
	fReturn = true;

returnHere:
	if (pbContext != NULL) COSE_FREE(pbContext, context);
	if (pArray != NULL) CN_CBOR_FREE(pArray, context);
	if (cnArrayT != NULL) CN_CBOR_FREE(cnArrayT, context);
	if (cnT != NULL) CN_CBOR_FREE(cnT, context);
	return fReturn;

errorReturn:
	fReturn = false;
	goto returnHere;
}
#endif

#if INCLUDE_ENCRYPT || INCLUDE_MAC
/*! brief Retrieve header parameter from a recipient structure
*
* Retrieve a header parameter from the message.
* Retrieved object is the same as the one in the message - do not delete it
*
* @param[in]	h	Handle of recipient object
* @param[in]    key	Key to look for
* @param[in]	flags	What buckets should we look for the message
* @param[out]	perror	Location to return error codes
* @return	Object which is found or NULL
*/

cn_cbor * COSE_Recipient_map_get_int(HCOSE_RECIPIENT h, int key, int flags, cose_errback * perror)
{
	if (!IsValidRecipientHandle(h)) {
		if (perror != NULL) perror->err = COSE_ERR_INVALID_HANDLE;
		return NULL;
	}

	return _COSE_map_get_int(&((COSE_RecipientInfo *)h)->m_encrypt.m_message, key, flags, perror);
}

HCOSE_RECIPIENT COSE_Recipient_GetRecipient(HCOSE_RECIPIENT cose, int iRecipient, cose_errback * perr)
{
	int i;
	COSE_RecipientInfo * p = NULL;

	CHECK_CONDITION(IsValidRecipientHandle(cose), COSE_ERR_INVALID_HANDLE);
	CHECK_CONDITION(iRecipient >= 0, COSE_ERR_INVALID_PARAMETER);

	p = ((COSE_RecipientInfo *)cose)->m_encrypt.m_recipientFirst;
	for (i = 0; i < iRecipient; i++) {
		CHECK_CONDITION(p != NULL, COSE_ERR_INVALID_PARAMETER);
		p = p->m_recipientNext;
	}
	if (p != NULL) p->m_encrypt.m_message.m_refCount++;

errorReturn:
	return (HCOSE_RECIPIENT)p;
}

bool COSE_Recipient_AddRecipient(HCOSE_RECIPIENT hEnc, HCOSE_RECIPIENT hRecip, cose_errback * perr)
{
	COSE_RecipientInfo * pRecip;
	COSE_Enveloped * pEncrypt;
	cn_cbor * pRecipients = NULL;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context;
#endif
	cn_cbor_errback cbor_error;

	CHECK_CONDITION(IsValidRecipientHandle(hEnc), COSE_ERR_INVALID_HANDLE);
	CHECK_CONDITION(IsValidRecipientHandle(hRecip), COSE_ERR_INVALID_HANDLE);

	pEncrypt = &((COSE_RecipientInfo *)hEnc)->m_encrypt;
	pRecip = (COSE_RecipientInfo *)hRecip;

#ifdef USE_CBOR_CONTEXT
	context = &pEncrypt->m_message.m_allocContext;
#endif // USE_CBOR_CONTEXT

	pRecip->m_recipientNext = pEncrypt->m_recipientFirst;
	pEncrypt->m_recipientFirst = pRecip;

	pRecipients = _COSE_arrayget_int(&pEncrypt->m_message, INDEX_RECIPIENTS);
	if (pRecipients == NULL) {
		pRecipients = cn_cbor_array_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
		CHECK_CONDITION_CBOR(pRecipients != NULL, cbor_error);

		if (!_COSE_array_replace(&pEncrypt->m_message, pRecipients, INDEX_RECIPIENTS, CBOR_CONTEXT_PARAM_COMMA &cbor_error)) {
			CN_CBOR_FREE(pRecipients, context);
			if (perr != NULL) perr->err = _MapFromCBOR(cbor_error);
			goto errorReturn;
		}
	}

	CHECK_CONDITION_CBOR(cn_cbor_array_append(pRecipients, pRecip->m_encrypt.m_message.m_cbor, &cbor_error), cbor_error);

	pRecip->m_encrypt.m_message.m_refCount++;

	return true;

errorReturn:
	return false;
}

#endif
