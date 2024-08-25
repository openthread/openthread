/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes definitions for CBOR.
 *   Ref: https://tools.ietf.org/html/rfc7049
 */

#ifndef CBOR_HPP_
#define CBOR_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_CCM_ENABLE

#include <stddef.h>
#include <stdint.h>

#include "core/common/error.hpp"
#include "openthread/error.h"

struct cn_cbor;

namespace ot {

namespace MeshCoP {

class CborValue
{
public:
    virtual Error Init() { return kErrorFailed; }
    void          Free();

    cn_cbor       *GetImpl() { return mCbor; }
    const cn_cbor *GetImpl() const { return mCbor; }

    bool IsValid() const { return mCbor != nullptr; }

    // This move the resource from src to dst
    static void Move(CborValue &dst, CborValue &src);

    Error        Serialize(uint8_t *aBuf, size_t &aLength, size_t aMaxLength) const;
    static Error Deserialize(CborValue &aValue, const uint8_t *aBuf, size_t aLength);

protected:
    CborValue();

    mutable bool mIsRoot;
    cn_cbor     *mCbor;
};

class CborMap : public CborValue
{
public:
    CborMap() {}
    virtual Error Init();

    Error Put(const char *aKey, int aValue);

    Error Put(const char *aKey, bool aValue);

    Error Put(const char *aKey, const char *aValue);

    Error Put(int aKey, const CborMap &aMap);

    Error Put(int aKey, int aValue);

    Error Put(int aKey, const uint8_t *aBytes, size_t aLength);

    Error Put(int aKey, const char *aStr);

    Error Get(int aKey, CborMap &aMap) const;

    Error Get(int aKey, int &aInt) const;

    Error Get(int aKey, const uint8_t *&aBytes, size_t &aLength) const;

    Error Get(int aKey, const char *&aStr, size_t &aLength) const;
};

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_CCM_ENABLE

#endif // CBOR_HPP_
