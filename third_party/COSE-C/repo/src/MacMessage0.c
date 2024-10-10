/** \file MacMessage0.c
* Contains implementation of the functions related to HCOSE_MAC0 handle objects.
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

#if INCLUDE_MAC0

COSE * Mac0Root = NULL;

/*! \private
* @brief Test if a HCOSE_MAC0 handle is valid
*
*  Internal function to test if a MAC0 message handle is valid.
*  This will start returning invalid results and cause the code to
*  crash if handles are not released before the memory that underlies them
*  is deallocated.  This is an issue of a block allocator is used since
*  in that case it is common to allocate memory but never to de-allocate it
*  and just do that in a single big block.
*
*  @param h handle to be validated
*  @returns result of check
*/

bool IsValidMac0Handle(HCOSE_MAC0 h)
{
	COSE_Mac0Message * p = (COSE_Mac0Message *)h;
	return _COSE_IsInList(Mac0Root, (COSE *) p);
}

HCOSE_MAC0 COSE_Mac0_Init(COSE_INIT_FLAGS flags, CBOR_CONTEXT_COMMA cose_errback * perr)
{
	COSE_Mac0Message * pobj = NULL;

	CHECK_CONDITION(flags == COSE_INIT_FLAGS_NONE, COSE_ERR_INVALID_PARAMETER);

	pobj = (COSE_Mac0Message *)COSE_CALLOC(1, sizeof(COSE_Mac0Message), context);
	CHECK_CONDITION(pobj != NULL, COSE_ERR_OUT_OF_MEMORY);

	if (!_COSE_Init(flags, &pobj->m_message, COSE_mac0_object, CBOR_CONTEXT_PARAM_COMMA perr)) {
		goto errorReturn;
	}

	_COSE_InsertInList(&Mac0Root, &pobj->m_message);

	return (HCOSE_MAC0)pobj;

errorReturn:
	if (pobj != NULL) {
		_COSE_Mac0_Release(pobj);
		COSE_FREE(pobj, context);
	}
	return NULL;
}

HCOSE_MAC0 _COSE_Mac0_Init_From_Object(cn_cbor * cbor, COSE_Mac0Message * pIn, CBOR_CONTEXT_COMMA cose_errback * perr)
{
	COSE_Mac0Message * pobj = pIn;
	cn_cbor * pRecipients = NULL;
	// cn_cbor * tmp;
	cose_errback error = { COSE_ERR_NONE };
	if (perr == NULL) perr = &error;

	if (pobj == NULL) pobj = (COSE_Mac0Message *)COSE_CALLOC(1, sizeof(COSE_Mac0Message), context);
	if (pobj == NULL) {
		perr->err = COSE_ERR_OUT_OF_MEMORY;
	errorReturn:
		if (pobj != NULL) {
			_COSE_Mac0_Release(pobj);
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
	CHECK_CONDITION(pRecipients == NULL, COSE_ERR_INVALID_PARAMETER);

	_COSE_InsertInList(&Mac0Root, &pobj->m_message);

	return(HCOSE_MAC0)pobj;
}

bool COSE_Mac0_Free(HCOSE_MAC0 h)
{
#ifdef USE_CBOR_CONTEXT
	cn_cbor_context *context;
#endif
	COSE_Mac0Message * p = (COSE_Mac0Message *)h;

	if (!IsValidMac0Handle(h)) return false;

	if (p->m_message.m_refCount > 1) {
		p->m_message.m_refCount--;
		return true;
	}

	_COSE_RemoveFromList(&Mac0Root, &p->m_message);

#ifdef USE_CBOR_CONTEXT
	context = &p->m_message.m_allocContext;
#endif

	_COSE_Mac0_Release(p);

	COSE_FREE(p, context);

	return true;
}

bool _COSE_Mac0_Release(COSE_Mac0Message * p)
{
	_COSE_Release(&p->m_message);

	return true;
}

bool COSE_Mac0_SetContent(HCOSE_MAC0 cose, const byte * rgbContent, size_t cbContent, cose_errback * perr)
{
	COSE_Mac0Message * p = (COSE_Mac0Message *)cose;
#ifdef USE_CBOR_CONTEXT        
	cn_cbor_context * context = &p->m_message.m_allocContext;
#endif
	cn_cbor * ptmp = NULL;
	cn_cbor_errback cbor_error;

	CHECK_CONDITION(IsValidMac0Handle(cose), COSE_ERR_INVALID_PARAMETER);

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

bool COSE_Mac0_SetExternal(HCOSE_MAC0 hcose, const byte * pbExternalData, size_t cbExternalData, cose_errback * perr)
{
	if (!IsValidMac0Handle(hcose)) {
		if (perr != NULL) perr->err = COSE_ERR_INVALID_PARAMETER;
		return false;
	}

	return _COSE_SetExternal(&((COSE_Mac0Message *)hcose)->m_message, pbExternalData, cbExternalData, perr);
}

cn_cbor * COSE_Mac0_map_get_int(HCOSE_MAC0 h, int key, int flags, cose_errback * perror)
{
	if (!IsValidMac0Handle(h)) {
		if (perror != NULL) perror->err = COSE_ERR_INVALID_PARAMETER;
		return NULL;
	}

	return _COSE_map_get_int(&((COSE_Mac0Message *)h)->m_message, key, flags, perror);
}


bool COSE_Mac0_map_put_int(HCOSE_MAC0 h, int key, cn_cbor * value, int flags, cose_errback * perror)
{
	if (!IsValidMac0Handle(h) || (value == NULL)) {
		if (perror != NULL) perror->err = COSE_ERR_INVALID_PARAMETER;
		return false;
	}

	return _COSE_map_put(&((COSE_Mac0Message *)h)->m_message, key, value, flags, perror);
}


bool COSE_Mac0_encrypt(HCOSE_MAC0 h, const byte * pbKey, size_t cbKey, cose_errback * perr)
{
	COSE_Mac0Message * pcose = (COSE_Mac0Message *)h;

	CHECK_CONDITION(IsValidMac0Handle(h), COSE_ERR_INVALID_HANDLE);
	CHECK_CONDITION(pbKey != NULL, COSE_ERR_INVALID_PARAMETER);

	return _COSE_Mac_compute(pcose, pbKey, cbKey, "MAC0", perr);

errorReturn:
	return false;
}

bool COSE_Mac0_validate(HCOSE_MAC0 h, const byte * pbKey, size_t cbKey, cose_errback * perr)
{
	COSE_Mac0Message * pcose = (COSE_Mac0Message *)h;
	CHECK_CONDITION(IsValidMac0Handle(h), COSE_ERR_INVALID_HANDLE);
	CHECK_CONDITION(pbKey != NULL, COSE_ERR_INVALID_PARAMETER);

	return _COSE_Mac_validate(pcose, NULL, pbKey, cbKey, "MAC0", perr);

errorReturn:
	return false;
}

#endif
