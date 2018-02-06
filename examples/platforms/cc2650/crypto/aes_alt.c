/*
 * Copyright (c) 2017, The OpenThread Authors.
 * All rights reserved.
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

#include "aes_alt.h"
#include "mbedtls/aes.h"

#ifdef MBEDTLS_AES_ALT

#include <driverlib/crypto.h>
#include <driverlib/prcm.h>
#include <string.h>
#include <utils/code_utils.h>

#define CC2650_AES_KEY_UNUSED (-1)
#define CC2650_AES_CTX_MAGIC (0x7E)

/**
 * bitmap of which key stores are currently used
 */
static uint8_t sUsedKeys = 0;

/**
 * number of active contexts, used for power on/off of the crypto core
 */
static unsigned int sRefNum = 0;

void mbedtls_aes_init(mbedtls_aes_context *ctx)
{
    if (sRefNum++ == 0)
    {
        /* enable the crypto core */
        /* The TRNG should already be running before we ever ask the AES core
         * to do anything, if there is any scenario that the TRNG powers off
         * the peripheral power domain use this code to repower it

        PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);
        while (PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH) != PRCM_DOMAIN_POWER_ON);
        */
        PRCMPeripheralRunEnable(PRCM_PERIPH_CRYPTO);
        PRCMPeripheralSleepEnable(PRCM_PERIPH_CRYPTO);
        PRCMPeripheralDeepSleepEnable(PRCM_PERIPH_CRYPTO);
        PRCMLoadSet();

        while (!PRCMLoadGet())
            ;
    }

    ctx->magic   = CC2650_AES_CTX_MAGIC;
    ctx->key_idx = CC2650_AES_KEY_UNUSED;
}

void mbedtls_aes_free(mbedtls_aes_context *ctx)
{
    otEXPECT(ctx->magic == CC2650_AES_CTX_MAGIC);

    if (ctx->key_idx != CC2650_AES_KEY_UNUSED)
    {
        sUsedKeys &= ~(1 << ctx->key_idx);
    }

    if (--sRefNum == 0)
    {
        /* disable the crypto core */
        /* The TRNG core needs the peripheral power domain powered on to
         * function. if there is a situation where the power domain must be
         * powered off, use this code to do so.

        PRCMPowerDomainOff(PRCM_DOMAIN_PERIPH);
        while (PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH) != PRCM_DOMAIN_POWER_OFF);
        */
        PRCMPeripheralRunDisable(PRCM_PERIPH_CRYPTO);
        PRCMPeripheralSleepDisable(PRCM_PERIPH_CRYPTO);
        PRCMPeripheralDeepSleepDisable(PRCM_PERIPH_CRYPTO);
        PRCMLoadSet();

        while (!PRCMLoadGet())
            ;
    }

    memset((void *)ctx, 0x00, sizeof(ctx));

exit:
    return;
}

int mbedtls_aes_setkey_enc(mbedtls_aes_context *ctx, const unsigned char *key, unsigned int keybits)
{
    unsigned char key_idx;
    int           retval = 0;

    otEXPECT_ACTION(ctx->magic == CC2650_AES_CTX_MAGIC, retval = -1);

    if (ctx->key_idx != CC2650_AES_KEY_UNUSED)
    {
        sUsedKeys &= ~(1 << ctx->key_idx);
    }

    /* our hardware only supports 128 bit keys */
    otEXPECT_ACTION(keybits == 128u, retval = MBEDTLS_ERR_AES_INVALID_KEY_LENGTH);

    for (key_idx = 0; ((sUsedKeys >> key_idx) & 0x01) != 0 && key_idx < 8; key_idx++)
        ;

    /* we have no more room for this key */
    otEXPECT_ACTION(key_idx < 8, retval = -2);

    otEXPECT_ACTION(CRYPTOAesLoadKey((uint32_t *)key, key_idx) == AES_SUCCESS,
                    retval = MBEDTLS_ERR_AES_INVALID_KEY_LENGTH);

    sUsedKeys |= (1 << key_idx);
    ctx->key_idx = key_idx;
exit:
    return retval;
}

int mbedtls_aes_setkey_dec(mbedtls_aes_context *ctx, const unsigned char *key, unsigned int keybits)
{
    unsigned char key_idx;
    int           retval = 0;

    otEXPECT_ACTION(ctx->magic == CC2650_AES_CTX_MAGIC, retval = -1);

    if (ctx->key_idx != CC2650_AES_KEY_UNUSED)
    {
        sUsedKeys &= ~(1 << ctx->key_idx);
    }

    /* our hardware only supports 128 bit keys */
    otEXPECT_ACTION(keybits == 128u, retval = MBEDTLS_ERR_AES_INVALID_KEY_LENGTH);

    for (key_idx = 0; ((sUsedKeys >> key_idx) & 0x01) != 0 && key_idx < 8; key_idx++)
        ;

    /* we have no more room for this key */
    otEXPECT_ACTION(key_idx < 8, retval = -2);

    otEXPECT_ACTION(CRYPTOAesLoadKey((uint32_t *)key, key_idx) == AES_SUCCESS,
                    retval = MBEDTLS_ERR_AES_INVALID_KEY_LENGTH);

    sUsedKeys |= (1 << key_idx);
    ctx->key_idx = key_idx;
exit:
    return retval;
}

/**
 * \brief          AES-ECB block encryption/decryption
 *
 * \param ctx      AES context
 * \param mode     MBEDTLS_AES_ENCRYPT or MBEDTLS_AES_DECRYPT
 * \param input    16-byte input block
 * \param output   16-byte output block
 *
 * \return         0 if successful
 */
int mbedtls_aes_crypt_ecb(mbedtls_aes_context *ctx, int mode, const unsigned char input[16], unsigned char output[16])
{
    int retval = -1;

    retval = CRYPTOAesEcb((uint32_t *)input, (uint32_t *)output, ctx->key_idx, mode == MBEDTLS_AES_ENCRYPT, false);
    otEXPECT(retval == AES_SUCCESS);

    while ((retval = CRYPTOAesEcbStatus()) == AES_DMA_BSY)
        ;

    CRYPTOAesEcbFinish();

exit:
    return retval;
}

#endif /* MBEDTLS_AES_ALT */
