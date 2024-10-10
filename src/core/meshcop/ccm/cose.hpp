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
 *   This file defines COSE signing and validation functions.
 *   Ref: https://tools.ietf.org/html/rfc8152
 */

#ifndef COSE_HPP_
#define COSE_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_CCM_ENABLE

#include <stddef.h>
#include <stdint.h>

#include "cbor.hpp"
#include "cose.h"
#include "mbedtls/pk.h"
#include "openthread/error.h"

namespace ot {

namespace MeshCoP {

class CoseObject
{
};

// TODO: document class and public interface
class CoseSignObject : public CoseObject
{
public:
    CoseSignObject(void);

    otError Init(int aCoseInitFlags, int (*mbedTlsSecurePrng)(void *, unsigned char *aBuffer, size_t aSize));

    otError        Serialize(uint8_t *aBuf, size_t &aLength, size_t aBufLength);
    static otError Deserialize(CoseSignObject &aCose, const uint8_t *aBuf, size_t aLength);
    void           Free();

    otError Validate(const CborMap &aCborKey);

    otError Validate(const mbedtls_pk_context &aPubKey);

    otError Sign(const mbedtls_pk_context &aPrivateKey);

    otError SetContent(const uint8_t *aContent, size_t aLength);

    void SetExternalData(const uint8_t *aExternalData, size_t aExternalDataLength)
    {
        mExternalData       = aExternalData;
        mExternalDataLength = aExternalDataLength;
    }

    otError AddAttribute(int aKey, int aValue, int aFlags);

    const uint8_t *GetPayload(size_t &aLength);

private:
    HCOSE_SIGN0    mSign;
    const uint8_t *mExternalData;
    size_t         mExternalDataLength;
};

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_CCM_ENABLE

#endif // COSE_HPP_
