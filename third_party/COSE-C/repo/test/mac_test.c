#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cose.h>
#include <configure.h>
#include <cn-cbor/cn-cbor.h>
#if INCLUDE_MAC && !INCLUDE_ENCRYPT0
#include <cose_int.h>
#endif

#include "json.h"
#include "test.h"
#include "context.h"

#ifdef _MSC_VER
#pragma warning (disable: 4127)
#endif

#if INCLUDE_MAC
int _ValidateMAC(const cn_cbor * pControl, const byte * pbEncoded, size_t cbEncoded)
{
	const cn_cbor * pInput = cn_cbor_mapget_string(pControl, "input");
	const cn_cbor * pFail;
	const cn_cbor * pMac;
	const cn_cbor * pRecipients;
	HCOSE_MAC hMAC;
	int type;
	int iRecipient;
	bool fFail = false;
	bool fFailBody = false;
	bool fAlgNoSupport = false;
	int returnCode = 1;

	pFail = cn_cbor_mapget_string(pControl, "fail");
	if ((pFail != NULL) && (pFail->type == CN_CBOR_TRUE)) {
		fFailBody = true;
	}

	hMAC = (HCOSE_MAC) COSE_Decode(pbEncoded, cbEncoded, &type, COSE_mac_object, CBOR_CONTEXT_PARAM_COMMA NULL);
	if (hMAC == NULL) {
            if (fFailBody) return 0; else goto failTest;
        }

	if ((pInput == NULL) || (pInput->type != CN_CBOR_MAP)) goto failTest;
	pMac = cn_cbor_mapget_string(pInput, "mac");
	if ((pMac == NULL) || (pMac->type != CN_CBOR_MAP)) goto failTest;

	if (!SetReceivingAttributes((HCOSE)hMAC, pMac, Attributes_MAC_protected)) goto failTest;

	pRecipients = cn_cbor_mapget_string(pMac, "recipients");
	if ((pRecipients == NULL) || (pRecipients->type != CN_CBOR_ARRAY)) goto failTest;

	iRecipient = (int) pRecipients->length - 1;
	pRecipients = pRecipients->first_child;
	for (; pRecipients != NULL; iRecipient--, pRecipients=pRecipients->next) {
		fAlgNoSupport = false;
		cn_cbor * pkey = BuildKey(cn_cbor_mapget_string(pRecipients, "key"), false);
		if (pkey == NULL) {
			fFail = true;
			continue;
		}

		HCOSE_RECIPIENT hRecip = COSE_Mac_GetRecipient(hMAC, iRecipient, NULL);
		if (hRecip == NULL) {
			fFail = true;
			continue;
		}

		if (!SetReceivingAttributes((HCOSE)hRecip, pRecipients, Attributes_Recipient_protected)) goto failTest;

		if (!COSE_Recipient_SetKey(hRecip, pkey, NULL)) {
			fFail = true;
			continue;
		}

		cn_cbor * cnStatic = cn_cbor_mapget_string(pRecipients, "sender_key");
		if (cnStatic != NULL) {
			if (COSE_Recipient_map_get_int(hRecip, COSE_Header_ECDH_SPK, COSE_BOTH, NULL) == 0) {
				COSE_Recipient_map_put_int(hRecip, COSE_Header_ECDH_SPK, BuildKey(cnStatic, true), COSE_DONT_SEND, NULL);
			}
		}

		pFail = cn_cbor_mapget_string(pRecipients, "fail");

		cn_cbor * alg = COSE_Mac_map_get_int(hMAC, COSE_Header_Algorithm, COSE_BOTH, NULL);
		if (!IsAlgorithmSupported(alg)) fAlgNoSupport = true;

		alg = COSE_Recipient_map_get_int(hRecip, COSE_Header_Algorithm, COSE_BOTH, NULL);
		if (!IsAlgorithmSupported(alg)) fAlgNoSupport = true;

		if (COSE_Mac_validate(hMAC, hRecip, NULL)) {
			if (fAlgNoSupport) {
				fFail = true;
			}
			else if ((pFail != NULL) && (pFail->type != CN_CBOR_TRUE)) fFail = true;
		}
		else {
			if (fAlgNoSupport) {
				returnCode = 0;
			}
			else if ((pFail == NULL) || (pFail->type == CN_CBOR_FALSE)) fFail = true;
		}

		COSE_Recipient_Free(hRecip);
	}

	COSE_Mac_Free(hMAC);

	if (fFailBody) {
		if (!fFail) fFail = true;
		else fFail = false;
	}

	if (fFail) CFails += 1;
	return returnCode;

failTest:
	CFails += 1;
	return 0;
}

int ValidateMAC(const cn_cbor * pControl)
{
	int cbEncoded;
	byte * pbEncoded = GetCBOREncoding(pControl, &cbEncoded);

	return _ValidateMAC(pControl, pbEncoded, cbEncoded);
}

int BuildMacMessage(const cn_cbor * pControl)
{
	int iRecipient;

	//
	//  We don't run this for all control sequences - skip those marked fail.
	//

	const cn_cbor * pFail = cn_cbor_mapget_string(pControl, "fail");
	if ((pFail != NULL) && (pFail->type == CN_CBOR_TRUE)) return 0;

	HCOSE_MAC hMacObj = COSE_Mac_Init(0, CBOR_CONTEXT_PARAM_COMMA NULL);

	const cn_cbor * pInputs = cn_cbor_mapget_string(pControl, "input");
	if (pInputs == NULL) goto returnError;
	const cn_cbor * pMac = cn_cbor_mapget_string(pInputs, "mac");
	if (pMac == NULL) goto returnError;

	const cn_cbor * pContent = cn_cbor_mapget_string(pInputs, "plaintext");
	if (!COSE_Mac_SetContent(hMacObj, pContent->v.bytes, pContent->length, NULL)) goto returnError;

	if (!SetSendingAttributes((HCOSE)hMacObj, pMac, Attributes_MAC_protected)) goto returnError;

	const cn_cbor * pRecipients = cn_cbor_mapget_string(pMac, "recipients");
	if ((pRecipients == NULL) || (pRecipients->type != CN_CBOR_ARRAY)) goto returnError;

	pRecipients = pRecipients->first_child;
	for (iRecipient = 0; pRecipients != NULL; iRecipient++, pRecipients = pRecipients->next) {
		cn_cbor * pkey = BuildKey(cn_cbor_mapget_string(pRecipients, "key"), true);
		if (pkey == NULL) goto returnError;

		HCOSE_RECIPIENT hRecip = COSE_Recipient_Init(0, CBOR_CONTEXT_PARAM_COMMA NULL);
		if (hRecip == NULL) goto returnError;

		if (!SetSendingAttributes((HCOSE) hRecip, pRecipients, Attributes_Recipient_protected)) goto returnError;

		if (!COSE_Recipient_SetKey(hRecip, pkey, NULL))goto returnError;

		cn_cbor * pSenderKey = cn_cbor_mapget_string(pRecipients, "sender_key");
		if (pSenderKey != NULL) {
			cn_cbor * pSendKey = BuildKey(pSenderKey, false);
			if (!COSE_Recipient_SetSenderKey(hRecip, pSendKey, 2, NULL)) goto returnError;
		}

		if (!COSE_Mac_AddRecipient(hMacObj, hRecip, NULL)) goto returnError;

		COSE_Recipient_Free(hRecip);
	}

	if (!COSE_Mac_encrypt(hMacObj, NULL)) goto returnError;

	size_t cb = COSE_Encode((HCOSE)hMacObj, NULL, 0, 0) + 1;
	byte * rgb = (byte *)malloc(cb);
	cb = COSE_Encode((HCOSE)hMacObj, rgb, 0, cb);

	COSE_Mac_Free(hMacObj);

	int f = _ValidateMAC(pControl, rgb, cb);

	free(rgb);
	return f;

returnError:
	CFails += 1;
	return 1;
}

int MacMessage()
{
	HCOSE_MAC hEncObj = COSE_Mac_Init(0, CBOR_CONTEXT_PARAM_COMMA NULL);
	char * sz = "This is the content to be used";
	byte rgbSecret[256 / 8] = { 'a', 'b', 'c' };
	byte  rgbKid[6] = { 'a', 'b', 'c', 'd', 'e', 'f' };
	int cbKid = 6;
	size_t cb;
	byte * rgb;

	if (hEncObj == NULL) goto errorReturn;

	if (!COSE_Mac_map_put_int(hEncObj, COSE_Header_Algorithm, cn_cbor_int_create(COSE_Algorithm_HMAC_256_256, CBOR_CONTEXT_PARAM_COMMA NULL), COSE_PROTECT_ONLY, NULL)) goto errorReturn;
	if (!COSE_Mac_SetContent(hEncObj, (byte *)sz, strlen(sz), NULL))goto errorReturn;

	HCOSE_RECIPIENT hRecip = COSE_Recipient_from_shared_secret(rgbSecret, sizeof(rgbSecret), rgbKid, cbKid, CBOR_CONTEXT_PARAM_COMMA NULL);
	if (hRecip == NULL) goto errorReturn;
	if (!COSE_Mac_AddRecipient(hEncObj, hRecip, NULL)) goto errorReturn;

	if (!COSE_Mac_encrypt(hEncObj, NULL)) goto errorReturn;

	cb = COSE_Encode((HCOSE)hEncObj, NULL, 0, 0);
	if (cb == 0) goto errorReturn;

	rgb = (byte *)malloc(cb);
	if (rgb == NULL) goto errorReturn;
	cb = COSE_Encode((HCOSE)hEncObj, rgb, 0, cb);
	if (cb == 0) goto errorReturn;

	COSE_Mac_Free(hEncObj);

	FILE * fp = fopen("test.mac.cbor", "wb");
	fwrite(rgb, cb, 1, fp);
	fclose(fp);

#if 0
	char * szX;
	int cbPrint = 0;
	cn_cbor * cbor = COSE_get_cbor((HCOSE)hEncObj);
	cbPrint = cn_cbor_printer_write(NULL, 0, cbor, "  ", "\r\n");
	szX = malloc(cbPrint);
	cn_cbor_printer_write(szX, cbPrint, cbor, "  ", "\r\n");
	fprintf(stdout, "%s", szX);
	fprintf(stdout, "\r\n");
#endif

	/* */

	int typ;
	hEncObj = (HCOSE_MAC) COSE_Decode(rgb,  (int) cb, &typ, COSE_mac_object, CBOR_CONTEXT_PARAM_COMMA NULL);
	if (hEncObj == NULL) goto errorReturn;

	int iRecipient = 0;
	do {
		HCOSE_RECIPIENT hRecip2;

		hRecip2 = COSE_Mac_GetRecipient(hEncObj, iRecipient, NULL);
		if (hRecip2 == NULL) break;

		if (!COSE_Recipient_SetKey_secret(hRecip2, rgbSecret, sizeof(rgbSecret), NULL, 0, NULL)) goto errorReturn;

		if (!COSE_Mac_validate(hEncObj, hRecip2, NULL)) goto errorReturn;

		iRecipient += 1;

		COSE_Recipient_Free(hRecip2);

	} while (true);

	COSE_Mac_Free(hEncObj);

	return 1;

errorReturn:
	CFails++;
	return 1;
}
#endif

#if INCLUDE_MAC0
int _ValidateMac0(const cn_cbor * pControl, const byte * pbEncoded, size_t cbEncoded)
{
	const cn_cbor * pInput = cn_cbor_mapget_string(pControl, "input");
	const cn_cbor * pFail;
	const cn_cbor * pMac;
	const cn_cbor * pRecipients;
	HCOSE_MAC0 hMAC;
	int type;
	bool fFail = false;
	bool fFailBody = false;
	bool fUnsuportedAlg = false;

	pFail = cn_cbor_mapget_string(pControl, "fail");
	if ((pFail != NULL) && (pFail->type == CN_CBOR_TRUE)) {
		fFailBody = true;
	}

	hMAC = (HCOSE_MAC0)COSE_Decode(pbEncoded, cbEncoded, &type, COSE_mac0_object, CBOR_CONTEXT_PARAM_COMMA NULL);
	if (hMAC == NULL) {
		if (fFailBody) return 0; else goto errorReturn;
	}

	if ((pInput == NULL) || (pInput->type != CN_CBOR_MAP)) goto errorReturn;
	pMac = cn_cbor_mapget_string(pInput, "mac0");
	if ((pMac == NULL) || (pMac->type != CN_CBOR_MAP)) goto errorReturn;

	if (!SetReceivingAttributes((HCOSE)hMAC, pMac, Attributes_MAC0_protected)) goto errorReturn;

	pRecipients = cn_cbor_mapget_string(pMac, "recipients");
	if ((pRecipients == NULL) || (pRecipients->type != CN_CBOR_ARRAY)) goto errorReturn;

	pRecipients = pRecipients->first_child;

	cn_cbor * pkey = BuildKey(cn_cbor_mapget_string(pRecipients, "key"), true);
	if (pkey == NULL) {
		fFail = true;
		goto exitHere;
	}

	cn_cbor *k = cn_cbor_mapget_int(pkey, -1);

	cn_cbor * alg = COSE_Mac0_map_get_int(hMAC, COSE_Header_Algorithm, COSE_BOTH, NULL);
	if (!IsAlgorithmSupported(alg)) fUnsuportedAlg = true;

	pFail = cn_cbor_mapget_string(pRecipients, "fail");
	if (COSE_Mac0_validate(hMAC, k->v.bytes, k->length, NULL)) {
		if (fUnsuportedAlg) {
			fFail = true;
			fUnsuportedAlg = false;
		}
		else if ((pFail != NULL) && (pFail->type != CN_CBOR_TRUE)) fFail = true;
	}
	else {

		if ((pFail == NULL) || (pFail->type == CN_CBOR_FALSE)) fFail = true;
		if (fUnsuportedAlg) fFail = false;
	}



	COSE_Mac0_Free(hMAC);

	if (fFailBody) {
		if (!fFail) fFail = true;
		else fFail = false;
	}
exitHere:
	if (fFail) CFails += 1;
	return 0;

errorReturn:
	CFails += 1;
	return (fFail || fUnsuportedAlg) ? 0 : 1;
}

int ValidateMac0(const cn_cbor * pControl)
{
	int cbEncoded;
	byte * pbEncoded = GetCBOREncoding(pControl, &cbEncoded);

	return _ValidateMac0(pControl, pbEncoded, cbEncoded);
}

int BuildMac0Message(const cn_cbor * pControl)
{

	//
	//  We don't run this for all control sequences - skip those marked fail.
	//

	const cn_cbor * pFail = cn_cbor_mapget_string(pControl, "fail");
	if ((pFail != NULL) && (pFail->type == CN_CBOR_TRUE)) return 0;

	HCOSE_MAC0 hMacObj = COSE_Mac0_Init(0, CBOR_CONTEXT_PARAM_COMMA NULL);

	const cn_cbor * pInputs = cn_cbor_mapget_string(pControl, "input");
	if (pInputs == NULL) goto returnError;
	const cn_cbor * pMac = cn_cbor_mapget_string(pInputs, "mac0");
	if (pMac == NULL) goto returnError;

	const cn_cbor * pContent = cn_cbor_mapget_string(pInputs, "plaintext");
	if (!COSE_Mac0_SetContent(hMacObj, pContent->v.bytes, pContent->length, NULL)) goto returnError;

	if (!SetSendingAttributes((HCOSE)hMacObj, pMac, Attributes_MAC0_protected)) goto returnError;

	const cn_cbor * pRecipients = cn_cbor_mapget_string(pMac, "recipients");
	if ((pRecipients == NULL) || (pRecipients->type != CN_CBOR_ARRAY)) goto returnError;

	pRecipients = pRecipients->first_child;

	cn_cbor * pkey = BuildKey(cn_cbor_mapget_string(pRecipients, "key"), false);
		if (pkey == NULL) goto returnError;

		cn_cbor * k = cn_cbor_mapget_int(pkey, -1);

	if (!COSE_Mac0_encrypt(hMacObj, k->v.bytes, k->length, NULL)) goto returnError;

	size_t cb = COSE_Encode((HCOSE)hMacObj, NULL, 0, 0) + 1;
	byte * rgb = (byte *)malloc(cb);
	cb = COSE_Encode((HCOSE)hMacObj, rgb, 0, cb);

	COSE_Mac0_Free(hMacObj);

	int f = _ValidateMac0(pControl, rgb, cb);

	free(rgb);
	return f;

returnError:
	CFails += 1;
	return 1;
}
#endif

#if INCLUDE_MAC
void MAC_Corners()
{
    HCOSE_MAC hMAC;
    HCOSE_ENCRYPT hEncrypt = NULL;
	HCOSE_RECIPIENT hRecipient = NULL;
    byte rgb[10];
	cn_cbor * cn = cn_cbor_int_create(5, CBOR_CONTEXT_PARAM_COMMA NULL);
	cose_errback cose_error;

    //  Missing case - addref then release on item

    //  Incorrect algorithm

    hMAC = (HCOSE_MAC) COSE_Mac_Init(0, CBOR_CONTEXT_PARAM_COMMA NULL);

    //  Invalid Handle checks

	if (COSE_Mac_SetContent((HCOSE_MAC)hEncrypt, rgb, 10, NULL)) CFails++;
	if (COSE_Mac_map_get_int((HCOSE_MAC)hEncrypt, 1, COSE_BOTH, NULL)) CFails++;
	if (COSE_Mac_map_put_int((HCOSE_MAC)hEncrypt, 1, cn, COSE_PROTECT_ONLY, NULL)) CFails++;
	if (COSE_Mac_encrypt((HCOSE_MAC)hEncrypt, NULL)) CFails++;
	if (COSE_Mac_validate((HCOSE_MAC)hEncrypt, (HCOSE_RECIPIENT)hMAC, NULL)) CFails++;
	if (COSE_Mac_AddRecipient((HCOSE_MAC)hEncrypt, (HCOSE_RECIPIENT)hMAC, NULL)) CFails++;
	if (COSE_Mac_GetRecipient((HCOSE_MAC)hEncrypt, 0, NULL)) CFails++;
	if (COSE_Mac_SetExternal((HCOSE_MAC)hEncrypt, rgb, 0, NULL)) CFails++;
	if (COSE_Mac_Free((HCOSE_MAC)hEncrypt)) CFails++;

#if INCLUDE_ENCRYPT0
	hEncrypt = COSE_Encrypt_Init(0, CBOR_CONTEXT_PARAM_COMMA NULL);
#else
	hEncrypt = (HCOSE_ENCRYPT)COSE_CALLOC(1, sizeof(COSE), context);
#endif

	if (COSE_Mac_SetContent((HCOSE_MAC)hEncrypt, rgb, 10, NULL)) CFails++;
	if (COSE_Mac_map_get_int((HCOSE_MAC)hEncrypt, 1, COSE_BOTH, NULL)) CFails++;
	if (COSE_Mac_map_put_int((HCOSE_MAC)hEncrypt, 1, cn, COSE_PROTECT_ONLY, NULL)) CFails++;
	if (COSE_Mac_encrypt((HCOSE_MAC)hEncrypt, NULL)) CFails++;
	if (COSE_Mac_validate((HCOSE_MAC)hEncrypt, (HCOSE_RECIPIENT)hMAC, NULL)) CFails++;
	if (COSE_Mac_AddRecipient((HCOSE_MAC)hEncrypt, (HCOSE_RECIPIENT)hMAC, NULL)) CFails++;
	if (COSE_Mac_GetRecipient((HCOSE_MAC)hEncrypt, 0, NULL)) CFails++;
	if (COSE_Mac_SetExternal((HCOSE_MAC)hEncrypt, rgb, 0, NULL)) CFails++;
	if (COSE_Mac_Free((HCOSE_MAC)hEncrypt)) CFails++;

    //
	//  Unsupported algorithm

	hMAC = COSE_Mac_Init(0, CBOR_CONTEXT_PARAM_COMMA NULL);
	if (hMAC == NULL) CFails++;
	if (!COSE_Mac_SetContent(hMAC, (byte *) "Message", 7, NULL)) CFails++;
	if (!COSE_Mac_map_put_int(hMAC, COSE_Header_Algorithm, cn_cbor_int_create(-99, CBOR_CONTEXT_PARAM_COMMA NULL), COSE_PROTECT_ONLY, NULL)) CFails++;
	hRecipient = COSE_Recipient_from_shared_secret(rgb, sizeof(rgb), rgb, sizeof(rgb), CBOR_CONTEXT_PARAM_COMMA NULL);
	if (hRecipient == NULL) CFails++;
	if (!COSE_Mac_AddRecipient(hMAC, hRecipient, NULL)) CFails++;
	CHECK_FAILURE(COSE_Mac_encrypt(hMAC, &cose_error), COSE_ERR_UNKNOWN_ALGORITHM, CFails++);
	COSE_Mac_Free(hMAC);
	COSE_Recipient_Free(hRecipient);

	hMAC = COSE_Mac_Init(0, CBOR_CONTEXT_PARAM_COMMA NULL);
	if (hMAC == NULL) CFails++;
	if (!COSE_Mac_SetContent(hMAC, (byte *) "Message", 7, NULL)) CFails++;
	if (!COSE_Mac_map_put_int(hMAC, COSE_Header_Algorithm, cn_cbor_string_create("hmac", CBOR_CONTEXT_PARAM_COMMA NULL), COSE_PROTECT_ONLY, NULL)) CFails++;
	hRecipient = COSE_Recipient_from_shared_secret(rgb, sizeof(rgb), rgb, sizeof(rgb), CBOR_CONTEXT_PARAM_COMMA NULL);
	if (hRecipient == NULL) CFails++;
	if (!COSE_Mac_AddRecipient(hMAC, hRecipient, NULL)) CFails++;
	CHECK_FAILURE(COSE_Mac_encrypt(hMAC, &cose_error), COSE_ERR_UNKNOWN_ALGORITHM, CFails++);
	COSE_Recipient_Free(hRecipient);
	COSE_Mac_Free(hMAC);

	if (COSE_Mac_GetRecipient(hMAC, 9, NULL)) CFails++;

    return;
}
#endif

#if INCLUDE_MAC0
void MAC0_Corners()
{
	HCOSE_ENCRYPT hEncrypt = NULL;
	HCOSE_MAC0 hMAC;
	byte rgb[10];
	cn_cbor * cn = cn_cbor_int_create(5, CBOR_CONTEXT_PARAM_COMMA NULL);
	cose_errback cose_error;

	hEncrypt = COSE_Encrypt_Init(0, CBOR_CONTEXT_PARAM_COMMA NULL);

	//  Missing case - addref then release on item

	//  Invalid Handle checks

	if (COSE_Mac0_SetContent((HCOSE_MAC0)hEncrypt, rgb, 10, NULL)) CFails++;
	if (COSE_Mac0_map_get_int((HCOSE_MAC0)hEncrypt, 1, COSE_BOTH, NULL)) CFails++;
	if (COSE_Mac0_map_put_int((HCOSE_MAC0)hEncrypt, 1, cn, COSE_PROTECT_ONLY, NULL)) CFails++;
	if (COSE_Mac0_encrypt((HCOSE_MAC0)hEncrypt, rgb, 10, NULL)) CFails++;
	if (COSE_Mac0_validate((HCOSE_MAC0)hEncrypt, rgb, 10, NULL)) CFails++;
	if (COSE_Mac0_SetExternal((HCOSE_MAC0)hEncrypt, rgb, 0, NULL)) CFails++;
	if (COSE_Mac0_Free((HCOSE_MAC0)hEncrypt)) CFails++;

	hEncrypt = COSE_Encrypt_Init(0, CBOR_CONTEXT_PARAM_COMMA NULL);

	if (COSE_Mac0_SetContent((HCOSE_MAC0)hEncrypt, rgb, 10, NULL)) CFails++;
	if (COSE_Mac0_map_get_int((HCOSE_MAC0)hEncrypt, 1, COSE_BOTH, NULL)) CFails++;
	if (COSE_Mac0_map_put_int((HCOSE_MAC0)hEncrypt, 1, cn, COSE_PROTECT_ONLY, NULL)) CFails++;
	if (COSE_Mac0_encrypt((HCOSE_MAC0)hEncrypt, rgb, 10, NULL)) CFails++;
	if (COSE_Mac0_validate((HCOSE_MAC0)hEncrypt, rgb, 10, NULL)) CFails++;
	if (COSE_Mac0_SetExternal((HCOSE_MAC0)hEncrypt, rgb, 0, NULL)) CFails++;

	if (COSE_Mac0_Free((HCOSE_MAC0)hEncrypt)) CFails++;

	//
	//  Unsupported algorithm

	hMAC = COSE_Mac0_Init(0, CBOR_CONTEXT_PARAM_COMMA NULL);
	if (hMAC == NULL) CFails++;
	if (!COSE_Mac0_SetContent(hMAC, (byte *) "Message", 7, NULL)) CFails++;
	if (!COSE_Mac0_map_put_int(hMAC, COSE_Header_Algorithm, cn_cbor_int_create(-99, CBOR_CONTEXT_PARAM_COMMA NULL), COSE_PROTECT_ONLY, NULL)) CFails++;
	CHECK_FAILURE(COSE_Mac0_encrypt(hMAC, rgb, sizeof(rgb), &cose_error), COSE_ERR_UNKNOWN_ALGORITHM, CFails++);
	COSE_Mac0_Free(hMAC);

	hMAC = COSE_Mac0_Init(0, CBOR_CONTEXT_PARAM_COMMA NULL);
	if (hMAC == NULL) CFails++;
	if (!COSE_Mac0_SetContent(hMAC, (byte *) "Message", 7, NULL)) CFails++;
	if (!COSE_Mac0_map_put_int(hMAC, COSE_Header_Algorithm, cn_cbor_string_create("hmac", CBOR_CONTEXT_PARAM_COMMA NULL), COSE_PROTECT_ONLY, NULL)) CFails++;
	CHECK_FAILURE(COSE_Mac0_encrypt(hMAC, rgb, sizeof(rgb), &cose_error), COSE_ERR_UNKNOWN_ALGORITHM, CFails++);
	COSE_Mac0_Free(hMAC);

	return;
}
#endif
