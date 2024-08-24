/** \file MacMessage.c
* Contains implementation of the functions related to HCOSE_MAC handle objects.
*/

#include <stdlib.h>
#ifndef __MBED__
#include <memory.h>
#endif
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cose.h"
#include "cose_int.h"
#include "configure.h"
#include "crypto.h"

#if INCLUDE_MAC

COSE * MacRoot = NULL;

/*! \private
* @brief Test if a HCOSE_MAC handle is valid
*
*  Internal function to test if a MAC message handle is valid.
*  This will start returning invalid results and cause the code to
*  crash if handles are not released before the memory that underlies them
*  is deallocated.  This is an issue of a block allocator is used since
*  in that case it is common to allocate memory but never to de-allocate it
*  and just do that in a single big block.
*
*  @param h handle to be validated
*  @returns result of check
*/

bool IsValidMacHandle(HCOSE_MAC h)
{
	COSE_MacMessage * p = (COSE_MacMessage *)h;
	return _COSE_IsInList(MacRoot, (COSE *) p);
}


HCOSE_MAC COSE_Mac_Init(COSE_INIT_FLAGS flags, CBOR_CONTEXT_COMMA cose_errback * perr)
{
	COSE_MacMessage * pobj = NULL;

	CHECK_CONDITION(flags == COSE_INIT_FLAGS_NONE, COSE_ERR_INVALID_PARAMETER);

	pobj = (COSE_MacMessage *)COSE_CALLOC(1, sizeof(COSE_MacMessage), context);
	CHECK_CONDITION(pobj != NULL, COSE_ERR_OUT_OF_MEMORY);

	if (!_COSE_Init(flags, &pobj->m_message, COSE_mac_object, CBOR_CONTEXT_PARAM_COMMA perr)) {
		goto errorReturn;
	}

	_COSE_InsertInList(&MacRoot, &pobj->m_message);

	return (HCOSE_MAC)pobj;

errorReturn:
	if (pobj != NULL) {
		_COSE_Mac_Release(pobj);
		COSE_FREE(pobj, context);
	}
	return NULL;
}

HCOSE_MAC _COSE_Mac_Init_From_Object(cn_cbor * cbor, COSE_MacMessage * pIn, CBOR_CONTEXT_COMMA cose_errback * perr)
{
	COSE_MacMessage * pobj = pIn;
	cn_cbor * pRecipients = NULL;
	// cn_cbor * tmp;
	cose_errback error = { COSE_ERR_NONE };
	if (perr == NULL) perr = &error;

	if (pobj == NULL) pobj = (COSE_MacMessage *)COSE_CALLOC(1, sizeof(COSE_MacMessage), context);
	if (pobj == NULL) {
		perr->err = COSE_ERR_OUT_OF_MEMORY;
	errorReturn:
		if (pobj != NULL) {
			_COSE_Mac_Release(pobj);
			if (pIn == NULL) {
				COSE_FREE(pobj, context);
			}
		}
		return NULL;
	}

	if (!_COSE_Init_From_Object(&pobj->m_message, cbor, CBOR_CONTEXT_PARAM_COMMA perr)) {
		goto errorReturn;
	}

	pRecipients = _COSE_arrayget_int(&pobj->m_message, INDEX_MAC_RECIPIENTS);
	if (pRecipients != NULL) {
		CHECK_CONDITION(pRecipients->type == CN_CBOR_ARRAY, COSE_ERR_INVALID_PARAMETER);

		pRecipients = pRecipients->first_child;
		while (pRecipients != NULL) {
			COSE_RecipientInfo * pInfo = _COSE_Recipient_Init_From_Object(pRecipients, CBOR_CONTEXT_PARAM_COMMA perr);
			if (pInfo == NULL) goto errorReturn;

			pInfo->m_recipientNext = pobj->m_recipientFirst;
			pobj->m_recipientFirst = pInfo;
			pRecipients = pRecipients->next;
		}
	}

	_COSE_InsertInList(&MacRoot, &pobj->m_message);

	return(HCOSE_MAC)pobj;
}

bool COSE_Mac_Free(HCOSE_MAC h)
{
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context *context;
#endif
	COSE_MacMessage * p = (COSE_MacMessage *)h;

	if (!IsValidMacHandle(h)) return false;

	if (p->m_message.m_refCount > 1) {
		p->m_message.m_refCount--;
		return true;
	}

	_COSE_RemoveFromList(&MacRoot, &p->m_message);

#ifdef USE_CBOR_CONTEXT
	context = &((COSE_MacMessage *)h)->m_message.m_allocContext;
#endif

	_COSE_Mac_Release((COSE_MacMessage *)h);

	COSE_FREE((COSE_MacMessage *)h, context);

	return true;
}

bool _COSE_Mac_Release(COSE_MacMessage * p)
{
	COSE_RecipientInfo * pRecipient;
	COSE_RecipientInfo * pRecipient2;

	for (pRecipient = p->m_recipientFirst; pRecipient != NULL; pRecipient = pRecipient2) {
		pRecipient2 = pRecipient->m_recipientNext;
		_COSE_Recipient_Free(pRecipient);
	}

	_COSE_Release(&p->m_message);

	return true;
}


bool COSE_Mac_SetContent(HCOSE_MAC cose, const byte * rgbContent, size_t cbContent, cose_errback * perr)
{
	COSE_MacMessage * p = (COSE_MacMessage *)cose;
#ifdef USE_CBOR_CONTEXT        
	cn_cbor_context * context = &p->m_message.m_allocContext;
#endif
	cn_cbor * ptmp = NULL;
	cn_cbor_errback cbor_error;

	CHECK_CONDITION(IsValidMacHandle(cose), COSE_ERR_INVALID_PARAMETER);

	ptmp = cn_cbor_data_create(rgbContent, (int) cbContent, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(ptmp != NULL, cbor_error);

	CHECK_CONDITION_CBOR(_COSE_array_replace(&p->m_message, ptmp, INDEX_BODY, CBOR_CONTEXT_PARAM_COMMA &cbor_error),  cbor_error);
	ptmp = NULL;

	return true;

errorReturn:
	if (ptmp != NULL) CN_CBOR_FREE(ptmp, context);
	return false;
}

/*!
* @brief Set the application external data for authentication
*
* MAC data objects support the authentication of external application
* supplied data.  This function is provided to supply that data to the library.
*
* The external data is not copied, nor will be it freed when the handle is released.
*
* @param hcose  Handle for the COSE MAC data object
* @param pbEternalData  point to the external data
* @param cbExternalData size of the external data
* @param perr  location to return errors
* @return result of the operation.
*/

bool COSE_Mac_SetExternal(HCOSE_MAC hcose, const byte * pbExternalData, size_t cbExternalData, cose_errback * perr)
{
	if (!IsValidMacHandle(hcose)) {
		if (perr != NULL) perr->err = COSE_ERR_INVALID_PARAMETER;
		return false;
	}

	return _COSE_SetExternal(&((COSE_MacMessage *)hcose)->m_message, pbExternalData, cbExternalData, perr);
}


cn_cbor * COSE_Mac_map_get_int(HCOSE_MAC h, int key, int flags, cose_errback * perror)
{
	if (!IsValidMacHandle(h)) {
		if (perror != NULL) perror->err = COSE_ERR_INVALID_PARAMETER;
		return NULL;
	}

	return _COSE_map_get_int(&((COSE_MacMessage *)h)->m_message, key, flags, perror);
}


bool COSE_Mac_map_put_int(HCOSE_MAC h, int key, cn_cbor * value, int flags, cose_errback * perror)
{
	if (!IsValidMacHandle(h) || (value == NULL)) {
		if (perror != NULL) perror->err = COSE_ERR_INVALID_PARAMETER;
		return false;
	}

	return _COSE_map_put(&((COSE_MacMessage *)h)->m_message, key, value, flags, perror);
}
#endif

#if INCLUDE_MAC || INCLUDE_MAC0
bool _COSE_Mac_Build_AAD(COSE * pCose, const char * szContext, byte ** ppbAuthData, size_t * pcbAuthData, CBOR_CONTEXT_COMMA cose_errback * perr)
{
	cn_cbor * pAuthData = NULL;
	bool fRet = false;
	cn_cbor_errback cbor_error;
	cn_cbor * ptmp = NULL;
	cn_cbor * pcn;
	size_t cbAuthData;
	ssize_t written;
	byte * pbAuthData = NULL;

	//  Build authenticated data
	//  Protected headers
	//  external data
	//  body

	pAuthData = cn_cbor_array_create(CBOR_CONTEXT_PARAM_COMMA NULL);
	CHECK_CONDITION(pAuthData != NULL, COSE_ERR_OUT_OF_MEMORY);

	//  Add the context string

	ptmp = cn_cbor_string_create(szContext, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(ptmp != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_array_append(pAuthData, ptmp, &cbor_error), cbor_error);
	ptmp = NULL;

	// Add the protected attributes

	pcn = _COSE_arrayget_int(pCose, INDEX_PROTECTED);
	CHECK_CONDITION((pcn != NULL) && (pcn->type == CN_CBOR_BYTES), COSE_ERR_INVALID_PARAMETER);

	if ((pcn->length == 1) && (pcn->v.bytes[0] == 0xa0)) {
		ptmp = cn_cbor_data_create(NULL, 0, CBOR_CONTEXT_PARAM_COMMA NULL);
	}
	else {
		ptmp = cn_cbor_data_create(pcn->v.bytes, (int)pcn->length, CBOR_CONTEXT_PARAM_COMMA NULL);
	}
	CHECK_CONDITION(ptmp != NULL, COSE_ERR_CBOR);

	CHECK_CONDITION(cn_cbor_array_append(pAuthData, ptmp, NULL), COSE_ERR_CBOR);
	ptmp = NULL;

	//  Add the external bytes

	ptmp = cn_cbor_data_create(pCose->m_pbExternal, (int) pCose->m_cbExternal, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(ptmp != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_array_append(pAuthData, ptmp, &cbor_error), cbor_error);
	ptmp = NULL;

	//  Add the content
	pcn = _COSE_arrayget_int(pCose, INDEX_BODY);
	ptmp = cn_cbor_data_create(pcn->v.bytes, (int)pcn->length, CBOR_CONTEXT_PARAM_COMMA &cbor_error);
	CHECK_CONDITION_CBOR(ptmp != NULL, cbor_error);
	CHECK_CONDITION_CBOR(cn_cbor_array_append(pAuthData, ptmp, &cbor_error), cbor_error);
	ptmp = NULL;

	//  Turn it into bytes
	cbAuthData = cn_cbor_encode_size(pAuthData);
	CHECK_CONDITION(cbAuthData > 0, COSE_ERR_CBOR);
	pbAuthData = (byte *)COSE_CALLOC(cbAuthData, 1, context);
	CHECK_CONDITION(pbAuthData != NULL, COSE_ERR_OUT_OF_MEMORY);
	written = cn_cbor_encoder_write(pbAuthData, 0, cbAuthData, pAuthData);
	CHECK_CONDITION(written >= 0 && (size_t)written == cbAuthData, COSE_ERR_CBOR);

	*ppbAuthData = pbAuthData;
	*pcbAuthData = cbAuthData;
	pbAuthData = NULL;
	fRet = true;

errorReturn:
	if (pbAuthData != NULL) COSE_FREE(pbAuthData, context);
	if (pAuthData != NULL) CN_CBOR_FREE(pAuthData, context);
	if (ptmp != NULL) CN_CBOR_FREE(ptmp, context);
	return fRet;
}
#endif

#if INCLUDE_MAC
bool COSE_Mac_encrypt(HCOSE_MAC h, cose_errback * perr)
{
	COSE_MacMessage * pcose = (COSE_MacMessage *)h;

	CHECK_CONDITION(IsValidMacHandle(h), COSE_ERR_INVALID_HANDLE);
	CHECK_CONDITION(pcose->m_recipientFirst != NULL, COSE_ERR_INVALID_PARAMETER);

		return _COSE_Mac_compute(pcose, NULL, 0, "MAC", perr);

	errorReturn:
		return false;
}
#endif

#if INCLUDE_MAC || INCLUDE_MAC0
bool _COSE_Mac_compute(COSE_MacMessage * pcose, const byte * pbKeyIn, size_t cbKeyIn, const char * szContext, cose_errback * perr)
{
	int alg;
	int t;
	COSE_RecipientInfo * pri;
	const cn_cbor * cn_Alg = NULL;
	byte * pbAuthData = NULL;
	size_t cbitKey;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = &pcose->m_message.m_allocContext;
#endif
	bool fRet = false;
	size_t cbAuthData = 0;
	const byte * pbKey = NULL;
	byte * pbKeyNew = NULL;
	size_t cbKey = 0;

	cn_Alg = _COSE_map_get_int(&pcose->m_message, COSE_Header_Algorithm, COSE_BOTH, perr);
	if (cn_Alg == NULL) goto errorReturn;
	CHECK_CONDITION(cn_Alg->type != CN_CBOR_TEXT, COSE_ERR_UNKNOWN_ALGORITHM);
	CHECK_CONDITION(((cn_Alg->type == CN_CBOR_UINT || cn_Alg->type == CN_CBOR_INT)), COSE_ERR_INVALID_PARAMETER);

	alg = (int) cn_Alg->v.uint;

	//  Get the key size

	switch (alg) {
#ifdef USE_AES_CBC_MAC_128_64
	case COSE_Algorithm_CBC_MAC_128_64: 
		cbitKey = 128; 
		break;
#endif

#ifdef USE_AES_CBC_MAC_128_128
	case COSE_Algorithm_CBC_MAC_128_128:
		cbitKey = 128; 
		break;
#endif

#ifdef USE_AES_CBC_MAC_256_64
	case COSE_Algorithm_CBC_MAC_256_64: 
		cbitKey = 256; 
		break;
#endif

#ifdef USE_AES_CBC_MAC_256_128
	case COSE_Algorithm_CBC_MAC_256_128: 
		cbitKey = 256; 
		break;
#endif

#ifdef USE_HMAC_256_64
	case COSE_Algorithm_HMAC_256_64: 
		cbitKey = 256; 
		break;
#endif

#ifdef USE_HMAC_256_256
	case COSE_Algorithm_HMAC_256_256: 
		cbitKey = 256; 
		break;
#endif

#ifdef USE_HMAC_384_384
	case COSE_Algorithm_HMAC_384_384: 
		cbitKey = 384; 
		break;
#endif

#ifdef USE_HMAC_512_512
	case COSE_Algorithm_HMAC_512_512: 
		cbitKey = 512; 
		break;
#endif

	default:
		FAIL_CONDITION(COSE_ERR_UNKNOWN_ALGORITHM);
	}

	//  If we are doing direct encryption - then recipient generates the key

	if (pbKeyIn != NULL) {
		CHECK_CONDITION(cbKeyIn == cbitKey / 8, COSE_ERR_INVALID_PARAMETER);
		pbKey = pbKeyIn;
		cbKey = cbKeyIn;
	}
	else {
			t = 0;
			for (pri = pcose->m_recipientFirst; pri != NULL; pri = pri->m_recipientNext) {
				if (pri->m_encrypt.m_message.m_flags & 1) {
					CHECK_CONDITION(pbKey == NULL, COSE_ERR_INVALID_PARAMETER);

					t |= 1;
					pbKeyNew = _COSE_RecipientInfo_generateKey(pri, alg, cbitKey, perr);
					cbKey = cbitKey / 8;
					CHECK_CONDITION(pbKeyNew != NULL, COSE_ERR_OUT_OF_MEMORY);
					pbKey = pbKeyNew;
				}
				else {
					t |= 2;
				}
			}
			CHECK_CONDITION(t != 3, COSE_ERR_INVALID_PARAMETER);

		if (t == 2) {
			pbKeyNew = (byte *)COSE_CALLOC(cbitKey / 8, 1, context);
			CHECK_CONDITION(pbKeyNew != NULL, COSE_ERR_OUT_OF_MEMORY);
			pbKey = pbKeyNew;
			cbKey = cbitKey / 8;
			rand_bytes(pbKeyNew, cbKey);
		}
	}

	//  Build protected headers

	const cn_cbor * cbProtected = _COSE_encode_protected(&pcose->m_message, perr);
	if (cbProtected == NULL) goto errorReturn;

	//  Build authenticated data

	if (!_COSE_Mac_Build_AAD(&pcose->m_message, szContext, &pbAuthData, &cbAuthData, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;

	switch (alg) {
#ifdef USE_AES_CBC_MAC_128_64
	case COSE_Algorithm_CBC_MAC_128_64:
		if (!AES_CBC_MAC_Create(pcose, 64, pbKey, cbKey, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_AES_CBC_MAC_256_64
	case COSE_Algorithm_CBC_MAC_256_64:
		if (!AES_CBC_MAC_Create(pcose, 64, pbKey, cbKey, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_AES_CBC_MAC_128_128
	case COSE_Algorithm_CBC_MAC_128_128:
		if (!AES_CBC_MAC_Create(pcose, 128, pbKey, cbKey, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_AES_CBC_MAC_256_128
	case COSE_Algorithm_CBC_MAC_256_128:
		if (!AES_CBC_MAC_Create(pcose, 128, pbKey, cbKey, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_HMAC_256_64
	case COSE_Algorithm_HMAC_256_64:
		if (!HMAC_Create(pcose, 256, 64, pbKey, cbKey, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_HMAC_256_256
	case COSE_Algorithm_HMAC_256_256:
		if (!HMAC_Create(pcose, 256, 256, pbKey, cbKey, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_HMAC_384_384
	case COSE_Algorithm_HMAC_384_384:
		if (!HMAC_Create(pcose, 384, 384, pbKey, cbKey, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_HMAC_512_512
	case COSE_Algorithm_HMAC_512_512:
		if (!HMAC_Create(pcose, 512, 512, pbKey, cbKey, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

	default:
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
	}

	for (pri = pcose->m_recipientFirst; pri != NULL; pri = pri->m_recipientNext) {
		if (!_COSE_Recipient_encrypt(pri, pbKey, cbKey, perr)) goto errorReturn;
	}

	//  Figure out the clean up

	fRet = true;

errorReturn:
	if (pbKeyNew != NULL) {
		memset(pbKeyNew, 0, cbKey);
		COSE_FREE(pbKeyNew, context);
	}
	if (pbAuthData != NULL) COSE_FREE(pbAuthData, context);
	return fRet;
}
#endif

#if INCLUDE_MAC
bool COSE_Mac_validate(HCOSE_MAC h, HCOSE_RECIPIENT hRecip, cose_errback * perr)
{
	COSE_MacMessage * pcose = (COSE_MacMessage *)h;
	COSE_RecipientInfo * pRecip = (COSE_RecipientInfo *)hRecip;

	CHECK_CONDITION(IsValidMacHandle(h) && IsValidRecipientHandle(hRecip), COSE_ERR_INVALID_PARAMETER);

	return _COSE_Mac_validate(pcose, pRecip, NULL, 0, "MAC", perr);

errorReturn:
	return false;
}
#endif

#if INCLUDE_MAC || INCLUDE_MAC0
bool _COSE_Mac_validate(COSE_MacMessage * pcose, COSE_RecipientInfo * pRecip, const byte * pbKeyIn, size_t cbKeyIn, const char * szContext, cose_errback * perr)
{
	byte * pbAuthData = NULL;
	size_t cbitKey = 0;
	bool fRet = false;

	int alg;
	const cn_cbor * cn = NULL;
	byte * pbKeyNew = NULL;
	const byte * pbKey = NULL;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = &pcose->m_message.m_allocContext;
#endif
	size_t cbAuthData;

	CHECK_CONDITION(!((pRecip != NULL) && (pbKeyIn != NULL)), COSE_ERR_INTERNAL);

	cn = _COSE_map_get_int(&pcose->m_message, COSE_Header_Algorithm, COSE_BOTH, perr);
	if (cn == NULL) goto errorReturn;

	if (cn->type == CN_CBOR_TEXT) {
			FAIL_CONDITION(COSE_ERR_UNKNOWN_ALGORITHM);
	}
	else {
		CHECK_CONDITION((cn->type == CN_CBOR_UINT || cn->type == CN_CBOR_INT), COSE_ERR_INVALID_PARAMETER);

		alg = (int)cn->v.uint;

		switch (alg) {
#ifdef USE_AES_CBC_MAC_128_64
		case COSE_Algorithm_CBC_MAC_128_64:
			cbitKey = 128;
			break;
#endif

#ifdef USE_AES_CBC_MAC_128_128
		case COSE_Algorithm_CBC_MAC_128_128:
			cbitKey = 128;
			break;
#endif

#ifdef USE_AES_CBC_MAC_256_64
		case COSE_Algorithm_CBC_MAC_256_64: 
			cbitKey = 256; 
			break;
#endif

#ifdef USE_AES_CBC_MAC_256_128
		case COSE_Algorithm_CBC_MAC_256_128: 
			cbitKey = 256; 
			break;
#endif

#ifdef USE_HMAC_256_64
		case COSE_Algorithm_HMAC_256_64: 
			cbitKey = 256; 
			break;
#endif

#ifdef USE_HMAC_256_256
		case COSE_Algorithm_HMAC_256_256: 
			cbitKey = 256; 
			break;
#endif

#ifdef USE_HMAC_384_384
		case COSE_Algorithm_HMAC_384_384: 
			cbitKey = 384; 
			break;
#endif

#ifdef USE_HMAC_512_512
		case COSE_Algorithm_HMAC_512_512: 
			cbitKey = 512; 
			break;
#endif
		default:
			FAIL_CONDITION(COSE_ERR_UNKNOWN_ALGORITHM);
			break;
		}
	}

	//  Allocate the key if we have not already done so

	if (pbKeyIn != NULL) {
		CHECK_CONDITION(cbitKey / 8 == cbKeyIn, COSE_ERR_INVALID_PARAMETER);
		pbKey = pbKeyIn;
	}
	else {
		if (pbKeyNew == NULL) {
			pbKeyNew = COSE_CALLOC(cbitKey / 8, 1, context);
			CHECK_CONDITION(pbKeyNew != NULL, COSE_ERR_OUT_OF_MEMORY);
			pbKey = pbKeyNew;
		}

		//  If there is a recipient - ask it for the key

		if (pRecip != NULL) {
			COSE_RecipientInfo * pRecipX;

			for (pRecipX = pcose->m_recipientFirst; pRecipX != NULL; pRecipX = pRecipX->m_recipientNext) {
				if (pRecip == pRecipX) {
					if (!_COSE_Recipient_decrypt(pRecipX, pRecip, alg, cbitKey, pbKeyNew, perr)) goto errorReturn;
					break;
				}
				else if (pRecipX->m_encrypt.m_recipientFirst != NULL) {
					if (_COSE_Recipient_decrypt(pRecipX, pRecip, alg, cbitKey, pbKeyNew, perr)) break;
				}
			}
			CHECK_CONDITION(pRecipX != NULL, COSE_ERR_NO_RECIPIENT_FOUND);
		}
		else {
			for (pRecip = pcose->m_recipientFirst; pRecip != NULL; pRecip = pRecip->m_recipientNext) {
				if (_COSE_Recipient_decrypt(pRecip, NULL, alg, cbitKey, pbKeyNew, perr)) break;
			}
			CHECK_CONDITION(pRecip != NULL, COSE_ERR_NO_RECIPIENT_FOUND);
		}
	}

	//  Build authenticated data

	if (!_COSE_Mac_Build_AAD(&pcose->m_message, szContext, &pbAuthData, &cbAuthData, CBOR_CONTEXT_PARAM_COMMA perr)) goto errorReturn;

	switch (alg) {
#ifdef USE_HMAC_256_256
	case COSE_Algorithm_HMAC_256_256:
		if (!HMAC_Validate(pcose, 256, 256, pbKey, cbitKey/8, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_HMAC_256_64
	case COSE_Algorithm_HMAC_256_64:
		if (!HMAC_Validate(pcose, 256, 64, pbKey, cbitKey/8, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_HMAC_384_384
	case COSE_Algorithm_HMAC_384_384:
		if (!HMAC_Validate(pcose, 384, 384, pbKey, cbitKey/8, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_HMAC_512_512
	case COSE_Algorithm_HMAC_512_512:
		if (!HMAC_Validate(pcose, 512, 512, pbKey, cbitKey/8, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_AES_CBC_MAC_128_64
	case COSE_Algorithm_CBC_MAC_128_64:
		if (!AES_CBC_MAC_Validate(pcose, 64, pbKey, cbitKey / 8, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_AES_CBC_MAC_256_64
	case COSE_Algorithm_CBC_MAC_256_64:
		if (!AES_CBC_MAC_Validate(pcose, 64, pbKey, cbitKey/8, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_AES_CBC_MAC_128_128
	case COSE_Algorithm_CBC_MAC_128_128:
		if (!AES_CBC_MAC_Validate(pcose, 128, pbKey, cbitKey / 8, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

#ifdef USE_AES_CBC_MAC_256_128
	case COSE_Algorithm_CBC_MAC_256_128:
		if (!AES_CBC_MAC_Validate(pcose, 128, pbKey, cbitKey/8, pbAuthData, cbAuthData, perr)) goto errorReturn;
		break;
#endif

	default:
		FAIL_CONDITION(COSE_ERR_UNKNOWN_ALGORITHM);
		break;
	}

	fRet = true;

errorReturn:
	if (pbKeyNew != NULL) {
		memset(pbKeyNew, 0xff, cbitKey / 8);
		COSE_FREE(pbKeyNew, context);
	}

	return fRet;
}
#endif

#if INCLUDE_MAC
bool COSE_Mac_AddRecipient(HCOSE_MAC hMac, HCOSE_RECIPIENT hRecip, cose_errback * perr)
{
	COSE_RecipientInfo * pRecip;
	COSE_MacMessage * pMac;
	cn_cbor * pRecipients = NULL;
	cn_cbor * pRecipientsT = NULL;
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context * context = NULL;
#endif
	cn_cbor_errback cbor_error;

	CHECK_CONDITION(IsValidMacHandle(hMac), COSE_ERR_INVALID_PARAMETER);
	CHECK_CONDITION(IsValidRecipientHandle(hRecip), COSE_ERR_INVALID_PARAMETER);

	pMac = (COSE_MacMessage *)hMac;
	pRecip = (COSE_RecipientInfo *)hRecip;

	pRecip->m_recipientNext = pMac->m_recipientFirst;
	pMac->m_recipientFirst = pRecip;

#ifdef USE_CBOR_CONTEXT
	context = &pMac->m_message.m_allocContext;
#endif // USE_CBOR_CONTEXT

	pRecipients = _COSE_arrayget_int(&pMac->m_message, INDEX_MAC_RECIPIENTS);
	if (pRecipients == NULL) {
		pRecipientsT = cn_cbor_array_create(CBOR_CONTEXT_PARAM_COMMA &cbor_error);
		CHECK_CONDITION_CBOR(pRecipientsT != NULL, cbor_error);

		CHECK_CONDITION_CBOR(_COSE_array_replace(&pMac->m_message, pRecipientsT, INDEX_MAC_RECIPIENTS, CBOR_CONTEXT_PARAM_COMMA &cbor_error), cbor_error);
		pRecipients = pRecipientsT;
		pRecipientsT = NULL;
	}

	CHECK_CONDITION_CBOR(cn_cbor_array_append(pRecipients, pRecip->m_encrypt.m_message.m_cbor, &cbor_error), cbor_error);
	pRecip->m_encrypt.m_message.m_refCount++;

	return true;

errorReturn:
	if (pRecipientsT == NULL) CN_CBOR_FREE(pRecipientsT, context);
	return false;
}


HCOSE_RECIPIENT COSE_Mac_GetRecipient(HCOSE_MAC cose, int iRecipient, cose_errback * perr)
{
	int i;
	COSE_RecipientInfo * p;

	CHECK_CONDITION(IsValidMacHandle(cose), COSE_ERR_INVALID_PARAMETER);

	p = ((COSE_MacMessage *)cose)->m_recipientFirst;
	for (i = 0; i < iRecipient; i++) {
		CHECK_CONDITION(p != NULL, COSE_ERR_NO_RECIPIENT_FOUND);
		p = p->m_recipientNext;
	}
	if (p != NULL) p->m_encrypt.m_message.m_refCount++;
	return (HCOSE_RECIPIENT)p;

errorReturn:
	return NULL;
}

#endif
