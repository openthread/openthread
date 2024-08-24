#include "cose.h"
#include "configure.h"
#include "cose_int.h"
#include "crypto.h"

#if USE_BCRYPT

#include <Windows.h>

bool AES_CCM_Encrypt(COSE_Encrypt * pcose, int TSize, int LSize, int KSize, byte * pbAuthData, int cbAuthData)
{
	NTSTATUS err;
	BCRYPT_ALG_HANDLE hAlg = NULL;
	BCRYPT_KEY_DATA_BLOB_HEADER * pHdr = NULL;
	BCRYPT_KEY_HANDLE hKey = NULL;
	BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo = { 0 };
	byte rgbTag[16];
	int cbOut;
	byte * pbOut = NULL;

	err = BCryptOpenAlgorithmProvider(&hAlg, "AES_CCM", NULL, 0);
	if (err != 0) {
	error:
		if (pbOut != NULL) free(pbOut);
		if (pHdr != NULL) free(pHdr);
		if (hKey != NULL) BCryptDestroyKey(hKey);
		if (hAlg != NULL) BCryptCloseAlgorithmProvider(hAlg, 0);
		return false;
	}

	pHdr = (BCRYPT_KEY_DATA_BLOB_HEADER *)malloc(sizeof(*pHdr) + KSize / 8);
	if (pHdr == NULL) goto error;
	pHdr->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
	pHdr->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
	pHdr->cbKeyData = KSize / 8;
	memcpy(&pHdr[1], pcose->pbKey, pcose->cbKey);

	err = BCryptImportKey(hAlg, NULL, BCRYPT_KEY_DATA_BLOB, &hKey, NULL, 0, pHdr, (sizeof(*pHdr) + KSize / 8), 0);
	if (err != 0) goto error;

	BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
	authInfo.pbNonce = pcose->pbIV;
	authInfo.cbNonce = pcose->cbIV;
	authInfo.pbAuthData = pbAuthData;
	authInfo.cbAuthData = cbAuthData;
	authInfo.pbTag = rgbTag;
	authInfo.cbTag = TSize / 8;
	authInfo.pbMacContext = NULL;

	cbOut = pcose->cbContent + TSize / 8;
	pbOut = (byte *)malloc(cbOut);
	if (pbOut == NULL) goto error;

	err = BCryptEncrypt(hKey, pcose->pbContent, pcose->cbContent, &authInfo, NULL, 0, pbOut, cbOut, 0, 0);
	if (err != 0) goto error;

	memcpy(&pbOut[pcose->cbContent], rgbTag, TSize / 8);

	cn_cbor_mapput_int(pcose->m_message.m_cbor, COSE_Header_Ciphertext, cn_cbor_data_create(pbOut, cbOut, NULL), NULL);

	return true;
}

#endif // USE_BCRYPT