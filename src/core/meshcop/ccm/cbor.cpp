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
 *   This file implements the CBOR.
 */

#include "cbor.hpp"

#if OPENTHREAD_CONFIG_CCM_ENABLE

#include <assert.h>

#include "cn-cbor/cn-cbor.h"

#include "common/debug.hpp"

namespace ot {
namespace MeshCoP {

CborValue::CborValue()
    : mIsRoot(true)
    , mCbor(NULL)
{
}

void CborValue::Free(void)
{
    if (mIsRoot && mCbor != NULL)
    {
        cn_cbor_free(mCbor);
        mCbor = NULL;
    }
}

void CborValue::Move(CborValue &dst, CborValue &src)
{
    dst.Free();
    dst = src;
    src = CborValue();
}

otError CborValue::Serialize(uint8_t *aBuf, size_t &aLength, size_t aMaxLength) const
{
    OT_ASSERT(mIsRoot && mCbor != NULL);
    ssize_t written = cn_cbor_encoder_write(aBuf, 0, aMaxLength, mCbor);
    if (written == -1)
        return OT_ERROR_NO_BUFS;
    aLength = written;
    return OT_ERROR_NONE;
}

otError CborValue::Deserialize(CborValue &aValue, const uint8_t *aBuf, size_t aLength)
{
    cn_cbor *cbor = cn_cbor_decode(aBuf, aLength, NULL);
    if (cbor == NULL)
        return OT_ERROR_PARSE;
    aValue.mIsRoot = true;
    aValue.mCbor   = cbor;
    return OT_ERROR_NONE;
}

otError CborMap::Init(void)
{
    OT_ASSERT(mCbor == NULL);
    mIsRoot = true;
    mCbor   = cn_cbor_map_create(NULL);
    return mCbor ? OT_ERROR_NONE : OT_ERROR_NO_BUFS;
}

otError CborMap::Put(const char *aKey, int aValue)
{
    cn_cbor *cborInt = cn_cbor_int_create(aValue, NULL);
    bool     succeed = cborInt && cn_cbor_mapput_string(mCbor, aKey, cborInt, NULL);
    return succeed ? OT_ERROR_NONE : OT_ERROR_NO_BUFS;
}

otError CborMap::Put(const char *aKey, const char *aValue)
{
    cn_cbor *cborStr = cn_cbor_string_create(aValue, NULL);
    bool     succeed = cborStr && cn_cbor_mapput_string(mCbor, aKey, cborStr, NULL);
    return succeed ? OT_ERROR_NONE : OT_ERROR_NO_BUFS;
}

otError CborMap::Put(int aKey, const CborMap &aMap)
{
    aMap.mIsRoot = false;
    return cn_cbor_mapput_int(mCbor, aKey, aMap.mCbor, NULL) ? OT_ERROR_NONE : OT_ERROR_NO_BUFS;
}

otError CborMap::Put(int aKey, int aValue)
{
    cn_cbor *cborInt = cn_cbor_int_create(aValue, NULL);
    bool     succeed = cborInt && cn_cbor_mapput_int(mCbor, aKey, cborInt, NULL);
    return succeed ? OT_ERROR_NONE : OT_ERROR_NO_BUFS;
}

otError CborMap::Put(int aKey, const uint8_t *aBytes, size_t aLength)
{
    cn_cbor *cborBytes = cn_cbor_data_create(aBytes, aLength, NULL);
    bool     succeed   = cborBytes && cn_cbor_mapput_int(mCbor, aKey, cborBytes, NULL);
    return succeed ? OT_ERROR_NONE : OT_ERROR_NO_BUFS;
}

otError CborMap::Put(int aKey, const char *aStr)
{
    cn_cbor *cborStr = cn_cbor_string_create(aStr, NULL);
    bool     succeed = cborStr && cn_cbor_mapput_int(mCbor, aKey, cborStr, NULL);
    return succeed ? OT_ERROR_NONE : OT_ERROR_NO_BUFS;
}

otError CborMap::Get(int aKey, CborMap &aMap) const
{
    cn_cbor *cbor = cn_cbor_mapget_int(mCbor, aKey);
    if (cbor == NULL)
        return OT_ERROR_NOT_FOUND;
    aMap.mIsRoot = false;
    aMap.mCbor   = cbor;
    return OT_ERROR_NONE;
}

otError CborMap::Get(int aKey, int &aInt) const
{
    cn_cbor *cborInt = cn_cbor_mapget_int(mCbor, aKey);
    if (cborInt == NULL)
        return OT_ERROR_NOT_FOUND;
    aInt = cborInt->v.sint;
    return OT_ERROR_NONE;
}

otError CborMap::Get(int aKey, const uint8_t *&aBytes, size_t &aLength) const
{
    cn_cbor *cborBytes = cn_cbor_mapget_int(mCbor, aKey);
    if (cborBytes == NULL)
        return OT_ERROR_NOT_FOUND;
    aBytes  = cborBytes->v.bytes;
    aLength = cborBytes->length;
    return OT_ERROR_NONE;
}

otError CborMap::Get(int aKey, const char *&aStr, size_t &aLength) const
{
    cn_cbor *cborStr = cn_cbor_mapget_int(mCbor, aKey);
    if (cborStr == NULL)
        return OT_ERROR_NOT_FOUND;
    aStr    = cborStr->v.str;
    aLength = cborStr->length;
    return OT_ERROR_NONE;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_CCM_ENABLE
