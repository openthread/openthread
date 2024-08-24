
#include "cose.h"
#include "cose_int.h"

#include <memory.h>

#include <cn-cbor/cn-cbor.h>


#ifdef USE_MBED_TLS
#include <mbedtls/ecdsa.h>
#else
#include <openssl/cmac.h>
#include <openssl/ecdsa.h>
#include <openssl/hmac.h>
#endif

#define COSE_Key_EC_Curve -1
#define COSE_Key_EC_X -2
#define COSE_Key_EC_Y -3
#define COSE_Key_EC_d -4

#ifdef USE_MBED_TLS

void eckey_release(eckey_t * eckey)
{
    if (eckey != NULL) {
	    mbedtls_ecp_keypair_free(eckey);
    }
}

bool eckey_from_cbor(eckey_t * eckey, const cn_cbor * pKey, cose_errback * perr)
{
	byte  rgbKey[MBEDTLS_ECP_MAX_PT_LEN];
	int cbKey;
	int cbGroup;
	const cn_cbor * p;
	mbedtls_ecp_group_id groupId;

	mbedtls_ecp_keypair_init(eckey);

	p = cn_cbor_mapget_int(pKey, COSE_Key_Type);
	CHECK_CONDITION(p != NULL, COSE_ERR_INVALID_PARAMETER);
	if(p->type == CN_CBOR_UINT) {
		CHECK_CONDITION(p->v.uint == COSE_Key_Type_EC2, COSE_ERR_INVALID_PARAMETER);
	}
	else {
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
	}

	p = cn_cbor_mapget_int(pKey, COSE_Key_EC_Curve);
	CHECK_CONDITION((p != NULL) && (p->type == CN_CBOR_UINT), COSE_ERR_INVALID_PARAMETER);

	switch (p->v.uint) {
	case 1: // P-256
		groupId = MBEDTLS_ECP_DP_SECP256R1;
		break;

	case 2: // P-384
		groupId = MBEDTLS_ECP_DP_SECP384R1;
		break;

	case 3: // P-521
		groupId = MBEDTLS_ECP_DP_SECP521R1;
		break;

	default:
		FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);
	}
	CHECK_CONDITION(mbedtls_ecp_group_load(&eckey->grp, groupId) == 0, COSE_ERR_INVALID_PARAMETER);
	cbGroup = (eckey->grp.nbits + 7) / 8;

	p = cn_cbor_mapget_int(pKey, COSE_Key_EC_X);
	CHECK_CONDITION((p != NULL) && (p->type == CN_CBOR_BYTES), COSE_ERR_INVALID_PARAMETER);
	CHECK_CONDITION(p->length == cbGroup, COSE_ERR_INVALID_PARAMETER);
	memcpy(rgbKey+1, p->v.str, p->length);

	p = cn_cbor_mapget_int(pKey, COSE_Key_EC_Y);
	CHECK_CONDITION((p != NULL), COSE_ERR_INVALID_PARAMETER);
	if (p->type == CN_CBOR_BYTES) {
		rgbKey[0] = 0x04;
		cbKey = cbGroup * 2 + 1;
		CHECK_CONDITION(p->length == cbGroup, COSE_ERR_INVALID_PARAMETER);
		memcpy(rgbKey + p->length + 1, p->v.str, p->length);
	}
	else if (p->type == CN_CBOR_TRUE) {
		cbKey = cbGroup + 1;
		rgbKey[0] = 0x03;
	}
	else if (p->type == CN_CBOR_FALSE) {
		cbKey = cbGroup + 1;
		rgbKey[0] = 0x02;
	}
	else FAIL_CONDITION(COSE_ERR_INVALID_PARAMETER);

	CHECK_CONDITION(mbedtls_ecp_point_read_binary(&eckey->grp, &eckey->Q, rgbKey, cbKey) == 0, COSE_ERR_INVALID_PARAMETER);

	p = cn_cbor_mapget_int(pKey, COSE_Key_EC_d);
	if (p != NULL) {
		CHECK_CONDITION(p->type == CN_CBOR_BYTES, COSE_ERR_INVALID_PARAMETER);
		CHECK_CONDITION(mbedtls_mpi_read_binary( &eckey->d, p->v.bytes, p->length) == 0, COSE_ERR_CRYPTO_FAIL);
	}
	return true;

errorReturn:
	return false;
}

#else

extern EC_KEY * ECKey_From(const cn_cbor * pKey, int * cbGroup, cose_errback * perr);

void eckey_release(eckey_t * eckey)
{
	if (eckey != NULL && eckey->key != NULL) {
		EC_KEY_free(eckey->key);
		eckey->key = NULL;
	}
}

bool eckey_from_cbor(eckey_t * eckey, const cn_cbor * pKey, cose_errback * perr)
{
	eckey->key = ECKey_From(pKey, &eckey->group, perr);
	return eckey->key != NULL;
}

#endif // USE_MBED_TLS
