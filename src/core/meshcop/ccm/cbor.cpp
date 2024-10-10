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
    , mCbor(nullptr)
{
}

void CborValue::Free(void)
{
    if (mIsRoot && mCbor != nullptr)
    {
        cn_cbor_free(mCbor);
        mCbor = nullptr;
    }
}

void CborValue::Move(CborValue &dst, CborValue &src)
{
    dst.Free();
    dst = src;
    src = CborValue();
}

Error CborValue::Serialize(uint8_t *aBuf, size_t &aLength, size_t aMaxLength) const
{
    OT_ASSERT(mIsRoot && mCbor != nullptr);
    ssize_t written = cn_cbor_encoder_write(aBuf, 0, aMaxLength, mCbor);
    if (written == -1)
        return kErrorNoBufs;
    aLength = written;
    return kErrorNone;
}

Error CborValue::Deserialize(CborValue &aValue, const uint8_t *aBuf, size_t aLength)
{
    cn_cbor *cbor = cn_cbor_decode(aBuf, aLength, nullptr);
    if (cbor == nullptr)
        return kErrorParse;
    aValue.mIsRoot = true;
    aValue.mCbor   = cbor;
    return kErrorNone;
}

Error CborMap::Init(void)
{
    OT_ASSERT(mCbor == nullptr);
    mIsRoot = true;
    mCbor   = cn_cbor_map_create(nullptr);
    return mCbor ? kErrorNone : kErrorNoBufs;
}

Error CborMap::Put(const char *aKey, int aValue)
{
    cn_cbor *cborInt = cn_cbor_int_create(aValue, nullptr);
    bool     succeed = cborInt && cn_cbor_mapput_string(mCbor, aKey, cborInt, nullptr);
    return succeed ? kErrorNone : kErrorNoBufs;
}

Error CborMap::Put(const char *aKey, bool aValue)
{
    cn_cbor *cborBool = cn_cbor_int_create(0, nullptr);
    if (cborBool == nullptr)
        return kErrorNoBufs;
    if (aValue)
        cborBool->type = CN_CBOR_TRUE;
    else
        cborBool->type = CN_CBOR_FALSE;
    bool succeed = cn_cbor_mapput_string(mCbor, aKey, cborBool, nullptr);
    return succeed ? kErrorNone : kErrorNoBufs;
}

Error CborMap::Put(const char *aKey, const char *aValue)
{
    cn_cbor *cborStr = cn_cbor_string_create(aValue, nullptr);
    bool     succeed = cborStr && cn_cbor_mapput_string(mCbor, aKey, cborStr, nullptr);
    return succeed ? kErrorNone : kErrorNoBufs;
}

Error CborMap::Put(int aKey, const CborMap &aMap)
{
    aMap.mIsRoot = false;
    return cn_cbor_mapput_int(mCbor, aKey, aMap.mCbor, nullptr) ? kErrorNone : kErrorNoBufs;
}

Error CborMap::Put(int aKey, int aValue)
{
    cn_cbor *cborInt = cn_cbor_int_create(aValue, nullptr);
    bool     succeed = cborInt && cn_cbor_mapput_int(mCbor, aKey, cborInt, nullptr);
    return succeed ? kErrorNone : kErrorNoBufs;
}

Error CborMap::Put(int aKey, const uint8_t *aBytes, size_t aLength)
{
    cn_cbor *cborBytes = cn_cbor_data_create(aBytes, aLength, nullptr);
    bool     succeed   = cborBytes && cn_cbor_mapput_int(mCbor, aKey, cborBytes, nullptr);
    return succeed ? kErrorNone : kErrorNoBufs;
}

Error CborMap::Put(int aKey, const char *aStr)
{
    cn_cbor *cborStr = cn_cbor_string_create(aStr, nullptr);
    bool     succeed = cborStr && cn_cbor_mapput_int(mCbor, aKey, cborStr, nullptr);
    return succeed ? kErrorNone : kErrorNoBufs;
}

Error CborMap::Get(int aKey, CborMap &aMap) const
{
    cn_cbor *cbor = cn_cbor_mapget_int(mCbor, aKey);
    if (cbor == nullptr)
        return kErrorNotFound;
    aMap.mIsRoot = false;
    aMap.mCbor   = cbor;
    return kErrorNone;
}

Error CborMap::Get(int aKey, int &aInt) const
{
    cn_cbor *cborInt = cn_cbor_mapget_int(mCbor, aKey);
    if (cborInt == nullptr)
        return kErrorNotFound;
    aInt = cborInt->v.sint;
    return kErrorNone;
}

Error CborMap::Get(int aKey, const uint8_t *&aBytes, size_t &aLength) const
{
    cn_cbor *cborBytes = cn_cbor_mapget_int(mCbor, aKey);
    if (cborBytes == nullptr)
        return kErrorNotFound;
    aBytes  = cborBytes->v.bytes;
    aLength = cborBytes->length;
    return kErrorNone;
}

Error CborMap::Get(int aKey, const char *&aStr, size_t &aLength) const
{
    cn_cbor *cborStr = cn_cbor_mapget_int(mCbor, aKey);
    if (cborStr == nullptr)
        return kErrorNotFound;
    aStr    = cborStr->v.str;
    aLength = cborStr->length;
    return kErrorNone;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_CCM_ENABLE
