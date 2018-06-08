/*
 *  Copyright (c) 2017, The OpenThread Authors.
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

#include "sha256_alt.h"

#ifdef MBEDTLS_SHA256_ALT

#include <string.h>

/**
 * documented in sha256_alt.h
 */
void mbedtls_sha256_init(mbedtls_sha256_context *ctx)
{
    memset((void *)ctx, 0x00, sizeof(ctx));
}

/**
 * documented in sha256_alt.h
 */
void mbedtls_sha256_free(mbedtls_sha256_context *ctx)
{
    memset((void *)ctx, 0x00, sizeof(ctx));
}

/**
 * documented in sha256_alt.h
 */
void mbedtls_sha256_clone(mbedtls_sha256_context *dst, const mbedtls_sha256_context *src)
{
    *dst = *src;
}

/**
 * documented in sha256_alt.h
 */
int mbedtls_sha256_starts_ret(mbedtls_sha256_context *ctx, int is224)
{
    SHA256_initialize(ctx);

    if (is224 != 0)
    {
        /* SHA-224 */
        ctx->state[0] = 0xC1059ED8;
        ctx->state[1] = 0x367CD507;
        ctx->state[2] = 0x3070DD17;
        ctx->state[3] = 0xF70E5939;
        ctx->state[4] = 0xFFC00B31;
        ctx->state[5] = 0x68581511;
        ctx->state[6] = 0x64F98FA7;
        ctx->state[7] = 0xBEFA4FA4;
    }

    return 0;
}

/**
 * documented in sha256_alt.h
 */
int mbedtls_sha256_update_ret(mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen)
{
    SHA256_execute(ctx, (uint8_t *)input, (uint32_t)ilen);
    return 0;
}

char *workaround_cc2650_rom;

/**
 * documented in sha256_alt.h
 */
int mbedtls_sha256_finish_ret(mbedtls_sha256_context *ctx, unsigned char output[32])
{
    /* workaround for error in copy subroutine of SHA256 ROM implementation.
     * Allocate an extra 64 bytes on the stack to make sure we have buffer
     * room. This could be optomized out if you never call this function with
     * a call stack shorter than 16 words, approx. 8 stack frames.
     *
     * The simple description is this:
     *    If the stack pointer is within 64bytes of the end of RAM
     *    the bug exposes it self.
     *    If the stack pointer is more then 64bytes from end of RAM
     *    There is no bug...
     * Solution:
     *    Make a 64byte buffer on the stack..
     *    And force the compiler to think it requires this buffer.
     */
    char buffer[64];
    workaround_cc2650_rom = &buffer[0];
    SHA256_output(ctx, (uint8_t *)output);
    return 0;
}

/**
 * documented in sha256_alt.h
 */
int mbedtls_internal_sha256_process(mbedtls_sha256_context *ctx, const unsigned char data[64])
{
    return SHA256_execute(ctx, (uint8_t *)data, sizeof(unsigned char) * 64);
}

#endif /* MBEDTLS_SHA256_ALT */
