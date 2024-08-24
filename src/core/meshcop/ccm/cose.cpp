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
 *   This file implements COSE signing and validation functions.
 */

#include "cose.hpp"

#if OPENTHREAD_CONFIG_CCM_ENABLE

#include <assert.h>

#include "cn-cbor/cn-cbor.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "mbedtls/ecp.h"
#include "openthread/error.h"

namespace ot {
namespace MeshCoP {

CoseSignObject::CoseSignObject(void)
    : mSign(NULL)
    , mExternalData(NULL)
    , mExternalDataLength(0)
{
}

otError CoseSignObject::Init(int aCoseInitFlags)
{
    mSign = COSE_Sign0_Init(static_cast<COSE_INIT_FLAGS>(aCoseInitFlags), NULL);
    return mSign ? OT_ERROR_NONE : OT_ERROR_NO_BUFS;
}

void CoseSignObject::Free(void)
{
    if (mSign != NULL)
    {
        COSE_Sign0_Free(mSign);
        mSign = NULL;
    }
    mExternalData       = NULL;
    mExternalDataLength = 0;
}

otError CoseSignObject::Serialize(uint8_t *aBuf,  size_t &aLength, size_t aBufLength)
{
    otError error = OT_ERROR_NONE;
    size_t length;

    length = COSE_Encode((HCOSE)mSign, NULL, 0, 0) + 1;
    VerifyOrExit(length <= aBufLength, error = OT_ERROR_NO_BUFS);

    aLength = COSE_Encode((HCOSE)mSign, aBuf, 0, aBufLength);

exit:
    return error;
}

otError CoseSignObject::Deserialize(CoseSignObject &aCose, const uint8_t *aBuf, size_t aLength)
{
    otError     error = OT_ERROR_NONE;
    int         type;
    HCOSE_SIGN0 sign;

    sign = reinterpret_cast<HCOSE_SIGN0>(COSE_Decode(aBuf, aLength, &type, COSE_sign0_object, NULL));
    VerifyOrExit(sign != NULL && type == COSE_sign0_object, error = OT_ERROR_PARSE);

    aCose.mSign = sign;

exit:
    if (error != OT_ERROR_NONE && sign != NULL)
    {
        COSE_Sign0_Free(sign);
    }
    return error;
}

otError CoseSignObject::Validate(const CborMap &aCborKey)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aCborKey.IsValid(), error = OT_ERROR_INVALID_ARGS);

    if (mExternalData != NULL)
    {
        VerifyOrExit(COSE_Sign0_SetExternal(mSign, mExternalData, mExternalDataLength, NULL), error = OT_ERROR_FAILED);
    }

    VerifyOrExit(COSE_Sign0_validate(mSign, aCborKey.GetImpl(), NULL), error = OT_ERROR_SECURITY);

exit:
    return error;
}

otError CoseSignObject::Validate(const mbedtls_pk_context &aPubKey)
{
    otError                    error = OT_ERROR_NONE;
    const mbedtls_ecp_keypair *eckey;

    // Accepts only EC keys
    // FIXME(wgtdkp): accepts only ECDSA?
    VerifyOrExit(mbedtls_pk_can_do(&aPubKey, MBEDTLS_PK_ECDSA), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit((eckey = mbedtls_pk_ec(aPubKey)) != NULL, error = OT_ERROR_INVALID_ARGS);

    if (mExternalData != NULL)
    {
        VerifyOrExit(COSE_Sign0_SetExternal(mSign, mExternalData, mExternalDataLength, NULL), error = OT_ERROR_FAILED);
    }

    VerifyOrExit(COSE_Sign0_validate_eckey(mSign, eckey, NULL), error = OT_ERROR_SECURITY);

exit:
    return error;
}

otError CoseSignObject::Sign(const mbedtls_pk_context &aPrivateKey)
{
    otError                    error = OT_ERROR_NONE;
    const mbedtls_ecp_keypair *eckey = NULL;

    VerifyOrExit(mbedtls_pk_can_do(&aPrivateKey, MBEDTLS_PK_ECDSA), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit((eckey = mbedtls_pk_ec(aPrivateKey)) != NULL, error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit(COSE_Sign0_Sign_eckey(mSign, eckey, NULL), error = OT_ERROR_SECURITY);

exit:
    return error;
}

otError CoseSignObject::SetContent(const uint8_t *aContent, size_t aLength)
{
    otError error = OT_ERROR_NONE;
    uint8_t emptyContent = 0;

    if (aLength > 0)
    {
        VerifyOrExit(COSE_Sign0_SetContent(mSign, aContent, aLength, NULL), error = OT_ERROR_FAILED);
    }
    else
    {
        VerifyOrExit(COSE_Sign0_SetContent(mSign, &emptyContent, 0, NULL), error = OT_ERROR_FAILED);
    }

exit:
    return error;
}

otError CoseSignObject::AddAttribute(int aKey, int aValue, int aFlags)
{
    otError error = OT_ERROR_NONE;

    cn_cbor *cbor = cn_cbor_int_create(aValue, NULL);
    VerifyOrExit(cbor != NULL, error = OT_ERROR_NO_BUFS);

    VerifyOrExit(COSE_Sign0_map_put_int(mSign, aKey, cbor, aFlags, NULL), error = OT_ERROR_FAILED);

exit:
    if (cbor != NULL && cbor->parent == NULL)
    {
        cn_cbor_free(cbor);
    }
    return error;
}

static cn_cbor *CborArrayAt(cn_cbor *arr, size_t index)
{
    cn_cbor *ele = NULL;

    VerifyOrExit(index <= static_cast<size_t>(arr->length));

    ele = arr->first_child;
    for (size_t i = 0; i < index; ++i)
    {
        ele = ele->next;
    }
exit:
    return ele;
}

const uint8_t *CoseSignObject::GetPayload(size_t &aLength)
{
    const uint8_t *ret = NULL;
    cn_cbor *      cbor;
    cn_cbor *      payload;

    OT_ASSERT(mSign != NULL);
    VerifyOrExit((cbor = COSE_get_cbor(reinterpret_cast<HCOSE>(mSign))) != NULL);

    VerifyOrExit(cbor->type == CN_CBOR_ARRAY && (payload = CborArrayAt(cbor, 2)) != NULL);

    VerifyOrExit(payload->type == CN_CBOR_BYTES);

    ret     = payload->v.bytes;
    aLength = payload->length;

exit:
    return ret;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_CCM_ENABLE
