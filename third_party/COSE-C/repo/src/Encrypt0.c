/** \file Encrypt0.c
* Contains implementation of the functions related to HCOSE_ENCRYPT handle objects.
*/

#include <stdlib.h>
#ifndef __MBED__
#include <memory.h>
#endif
#include <stdio.h>
#include <assert.h>

#include "cose.h"
#include "cose_int.h"
#include "configure.h"
#include "crypto.h"

#if INCLUDE_ENCRYPT0 || INCLUDE_MAC0
void _COSE_Encrypt_Release(COSE_Encrypt * p);

COSE * EncryptRoot = NULL;
#endif

#if INCLUDE_ENCRYPT0
/*! \private
* @brief Test if a HCOSE_ENCRYPT handle is valid
*
*  Internal function to test if an encrypt message handle is valid.
*  This will start returning invalid results and cause the code to
*  crash if handles are not released before the memory that underlies them
*  is deallocated.  This is an issue of a block allocator is used since
*  in that case it is common to allocate memory but never to de-allocate it
*  and just do that in a single big block.
*
*  @param h handle to be validated
*  @returns result of check
*/

bool IsValidEncryptHandle(HCOSE_ENCRYPT h)
{
	COSE_Encrypt * p = (COSE_Encrypt *)h;
	return _COSE_IsInList(EncryptRoot, (COSE *)p);
}
#endif

#if INCLUDE_ENCRYPT0 || INCLUDE_MAC0
HCOSE_ENCRYPT COSE_Encrypt_Init(COSE_INIT_FLAGS flags, CBOR_CONTEXT_COMMA cose_errback * perr)
{
	CHECK_CONDITION(flags == COSE_INIT_FLAGS_NONE, COSE_ERR_INVALID_PARAMETER);
	COSE_Encrypt * pobj = (COSE_Encrypt *)COSE_CALLOC(1, sizeof(COSE_Encrypt), context);
	CHECK_CONDITION(pobj != NULL, COSE_ERR_OUT_OF_MEMORY);

	if (!_COSE_Init(flags, &pobj->m_message, COSE_enveloped_object, CBOR_CONTEXT_PARAM_COMMA perr)) {
		_COSE_Encrypt_Release(pobj);
		COSE_FREE(pobj, context);
		return NULL;
	}

	_COSE_InsertInList(&EncryptRoot, &pobj->m_message);

	return (HCOSE_ENCRYPT) pobj;

errorReturn:
	return NULL;
}
#endif

#if INCLUDE_ENCRYPT0
HCOSE_ENCRYPT COSE_Encrypt_Init_From_Object(cn_cbor * cbor, CBOR_CONTEXT_COMMA cose_errback * perr)
{
    COSE_Encrypt * pobj;

    cose_errback error = { 0 };
    if (perr == NULL) perr = &error;

    pobj = (COSE_Encrypt *) COSE_CALLOC(1, sizeof(COSE_Encrypt), context);
    if (pobj == NULL) {
        perr->err = COSE_ERR_OUT_OF_MEMORY;
    errorReturn:
        if (pobj != NULL) {
            _COSE_Encrypt_Release(pobj);
            COSE_FREE(pobj, context);
        }
        return NULL;
    }

    if (!_COSE_Init_From_Object(&pobj->m_message, cbor, CBOR_CONTEXT_PARAM_COMMA perr)) {
        goto errorReturn;
    }

    _COSE_InsertInList(&EncryptRoot, &pobj->m_message);

    return(HCOSE_ENCRYPT) pobj;
    
}
#endif

#if INCLUDE_ENCRYPT0
HCOSE_ENCRYPT _COSE_Encrypt_Init_From_Object(cn_cbor * cbor, COSE_Encrypt * pIn, CBOR_CONTEXT_COMMA cose_errback * perr)
{
	COSE_Encrypt * pobj = pIn;
	cn_cbor * pRecipients = NULL;
	cose_errback error = { 0 };
	if (perr == NULL) perr = &error;

	if (pobj == NULL) pobj = (COSE_Encrypt *)COSE_CALLOC(1, sizeof(COSE_Encrypt), context);
	if (pobj == NULL) {
		perr->err = COSE_ERR_OUT_OF_MEMORY;
	errorReturn:
		if (pobj != NULL) {
			_COSE_Encrypt_Release(pobj);
			if (pIn == NULL)  COSE_FREE(pobj, context);
		}
		return NULL;
	}

	if (!_COSE_Init_From_Object(&pobj->m_message, cbor, CBOR_CONTEXT_PARAM_COMMA perr)) {
		goto errorReturn;
	}

	pRecipients = _COSE_arrayget_int(&pobj->m_message, INDEX_RECIPIENTS);
	CHECK_CONDITION(pRecipients == NULL, COSE_ERR_INVALID_PARAMETER);

	_COSE_InsertInList(&EncryptRoot, &pobj->m_message);

	return(HCOSE_ENCRYPT) pobj;
}

bool COSE_Encrypt_Free(HCOSE_ENCRYPT h)
{
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context *context;
#endif
	COSE_Encrypt * pEncrypt = (COSE_Encrypt *)h;

	if (!IsValidEncryptHandle(h)) return false;

#ifdef USE_CBOR_CONTEXT
	context = &((COSE_Encrypt *)h)->m_message.m_allocContext;
#endif

	_COSE_Encrypt_Release(pEncrypt);

	_COSE_RemoveFromList(&EncryptRoot, &pEncrypt->m_message);

	COSE_FREE((COSE_Encrypt *)h, context);

	return true;
}
#endif

#if INCLUDE_ENCRYPT0 || INCLUDE_MAC0
void _COSE_Encrypt_Release(COSE_Encrypt * p)
{
	if (p->pbContent != NULL) COSE_FREE((void *) p->pbContent, &p->m_message.m_allocContext);

	_COSE_Release(&p->m_message);
}
#endif

#if INCLUDE_ENCRYPT0
bool COSE_Encrypt_decrypt(HCOSE_ENCRYPT h, const byte * pbKey, size_t cbKey, cose_errback * perr)
{
	COSE_Encrypt * pcose = (COSE_Encrypt *)h;
	bool f;

	if (!IsValidEncryptHandle(h)) {
		if (perr != NULL) perr->err = COSE_ERR_INVALID_PARAMETER;
		return false;
	}

	f = _COSE_Enveloped_decrypt(pcose, NULL, pbKey, cbKey, "Encrypt0", perr);
	return f;
}

bool COSE_Encrypt_encrypt(HCOSE_ENCRYPT h, const byte * pbKey, size_t cbKey, cose_errback * perr)
{
	CHECK_CONDITION(IsValidEncryptHandle(h), COSE_ERR_INVALID_HANDLE);
	CHECK_CONDITION(pbKey != NULL, COSE_ERR_INVALID_PARAMETER);

	return _COSE_Enveloped_encrypt((COSE_Encrypt *)h, pbKey, cbKey, "Encrypt0", perr);

errorReturn:
	return false;
}

const byte * COSE_Encrypt_GetContent(HCOSE_ENCRYPT h, size_t * pcbContent, cose_errback * perror)
{
    COSE_Encrypt * cose = (COSE_Encrypt *) h;
    if (!IsValidEncryptHandle(h) || (pcbContent == NULL)) {
        if (perror != NULL) perror->err = COSE_ERR_INVALID_PARAMETER;
        return false;
    }

    *pcbContent = cose->cbContent;
    return cose->pbContent;
}


bool COSE_Encrypt_SetContent(HCOSE_ENCRYPT h, const byte * rgb, size_t cb, cose_errback * perror)
{
	if (!IsValidEncryptHandle(h) || (rgb == NULL)) {
		if (perror != NULL) perror->err = COSE_ERR_INVALID_PARAMETER;
		return false;
	}

	return _COSE_Encrypt_SetContent((COSE_Encrypt *)h, rgb, cb, perror);
}

bool _COSE_Encrypt_SetContent(COSE_Encrypt * cose, const byte * rgb, size_t cb, cose_errback * perror)
{
	byte * pb;
	cose->pbContent = pb = (byte *)COSE_CALLOC(cb, 1, &cose->m_message.m_allocContext);
	if (cose->pbContent == NULL) {
		if (perror != NULL) perror->err = COSE_ERR_INVALID_PARAMETER;
		return false;
	}
	memcpy(pb, rgb, cb);
	cose->cbContent = cb;

	return true;
}

/*!
* @brief Set the application external data for authentication
*
* Enveloped data objects support the authentication of external application
* supplied data.  This function is provided to supply that data to the library.
*
* The external data is not copied, nor will be it freed when the handle is released.
*
* @param hcose  Handle for the COSE Enveloped data object
* @param pbEternalData  point to the external data
* @param cbExternalData size of the external data
* @param perr  location to return errors
* @return result of the operation.
*/

bool COSE_Encrypt_SetExternal(HCOSE_ENCRYPT hcose, const byte * pbExternalData, size_t cbExternalData, cose_errback * perr)
{
	if (!IsValidEncryptHandle(hcose)) {
		if (perr != NULL) perr->err = COSE_ERR_INVALID_PARAMETER;
		return false;
	}

	return _COSE_SetExternal(&((COSE_Encrypt *)hcose)->m_message, pbExternalData, cbExternalData, perr);
}

cn_cbor * COSE_Encrypt_map_get_int(HCOSE_ENCRYPT h, int key, int flags, cose_errback * perror)
{
	if (!IsValidEncryptHandle(h)) {
		if (perror != NULL) perror->err = COSE_ERR_INVALID_PARAMETER;
		return NULL;
	}

	return _COSE_map_get_int(&((COSE_Encrypt *)h)->m_message, key, flags, perror);
}


bool COSE_Encrypt_map_put_int(HCOSE_ENCRYPT h, int key, cn_cbor * value, int flags, cose_errback * perror)
{
	if (!IsValidEncryptHandle(h) || (value == NULL)) {
		if (perror != NULL) perror->err = COSE_ERR_INVALID_PARAMETER;
		return false;
	}

	return _COSE_map_put(&((COSE_Encrypt *)h)->m_message, key, value, flags, perror);
}

#endif
