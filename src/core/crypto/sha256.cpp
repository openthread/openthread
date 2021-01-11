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
 *   This file implements SHA-256.
 */

#include "sha256.hpp"

#include "common/message.hpp"

namespace ot {
namespace Crypto {

Sha256::Sha256(void)
{
    mbedtls_sha256_init(&mContext);
}

Sha256::~Sha256(void)
{
    mbedtls_sha256_free(&mContext);
}

void Sha256::Start(void)
{
    mbedtls_sha256_starts_ret(&mContext, 0);
}

void Sha256::Update(const void *aBuf, uint16_t aBufLength)
{
    mbedtls_sha256_update_ret(&mContext, reinterpret_cast<const uint8_t *>(aBuf), aBufLength);
}

void Sha256::Update(const Message &aMessage, uint16_t aOffset, uint16_t aLength)
{
    Message::Chunk chunk;

    aMessage.GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        Update(chunk.GetData(), chunk.GetLength());
        aMessage.GetNextChunk(aLength, chunk);
    }
}

void Sha256::Finish(Hash &aHash)
{
    mbedtls_sha256_finish_ret(&mContext, aHash.m8);
}

} // namespace Crypto
} // namespace ot
