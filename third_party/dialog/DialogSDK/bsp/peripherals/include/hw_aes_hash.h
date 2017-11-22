/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup AES_HASH
 * \{
 * \brief AES/Hash Engine
 */

/**
 ****************************************************************************************
 *
 * @file hw_aes_hash.h
 *
 * @brief Definition of API for the AES/HASH Engine Low Level Driver.
 *
 * Copyright (c) 2016, Dialog Semiconductor
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 ****************************************************************************************
 */

#ifndef HW_AES_HASH_H_
#define HW_AES_HASH_H_

#if dg_configUSE_HW_AES_HASH

#include <stdbool.h>
#include "sdk_defs.h"

/**
 * \brief AES/Hash callback
 *
 * This function is called by the AES/Hash driver when the interrupt is fired.
 *
 */
typedef void (*hw_aes_hash_cb)(void);

/**
 * \brief AES key sizes. Possible values: HW_AES_128, HW_AES_192, HW_AES_256.
 */
typedef enum {
        HW_AES_128 = 0,
        HW_AES_192 = 1,
        HW_AES_256 = 2
} hw_aes_key_size;

/**
 * \brief AES direction. Possible values: HW_AES_DECRYPT, HW_AES_ENCRYPT.
 */
typedef enum {
        HW_AES_DECRYPT = 0,
        HW_AES_ENCRYPT = 1
} hw_aes_direction;

/**
 * \brief AES/Hash modes. Possible values: HW_AES_ECB, HW_AES_CBC, HW_AES_CTR, HW_HASH_MD5,
 * HW_AES_SHA_1, HW_AES_SHA_256_224, HW_AES_SHA_256, HW_AES_SHA_384, HW_AES_SHA_512,
 * HW_AES_SHA_512_224, HW_AES_SHA_512_256.
 */
typedef enum {
        HW_AES_ECB,
        HW_AES_CBC,
        HW_AES_CTR,
        HW_HASH_MD5,
        HW_HASH_SHA_1,
        HW_HASH_SHA_256_224,
        HW_HASH_SHA_256,
        HW_HASH_SHA_384,
        HW_HASH_SHA_512,
        HW_HASH_SHA_512_224,
        HW_HASH_SHA_512_256
} hw_aes_hash_mode;

/**
 * \brief Key expansion modes.
 * Possible values HW_AES_PERFORM_KEY_EXPANSION, HW_AES_DO_NOT_PERFORM_KEY_EXPANSION
 */
typedef enum {
        HW_AES_PERFORM_KEY_EXPANSION = 0,       /**< Key expansion is performed by the engine */
        HW_AES_DO_NOT_PERFORM_KEY_EXPANSION     /**< Key expansion is performed by the software */
} hw_aes_hash_key_exp_t;

/**
 \brief AES/Hash setup structure.
 */
typedef struct {
        hw_aes_hash_mode mode; /**< AES/Hash mode. */
        hw_aes_direction aesDirection; /**< AES direction. Only used when the mode is an AES mode.*/
        hw_aes_key_size aesKeySize; /**< AES key size. Only used when the mode is an AES mode. */
        bool aesKeyExpand; /**< When true the key expansion process is execute. When false the key
         expansion process is not executed. The user should write the AES keys in CRYPTO_RAM.
         Only used when the mode is an AES mode. */
        uint32 aesKeys; /**< The start address of the buffer containing the AES key. */
        uint32 aesIvCtrblk_0_31; /**< In CBC mode IV[31:0] and in CTR mode the initial value of the
         32 bits counter. Only used when the mode is an AES CBC/CTR mode. */
        uint32 aesIvCtrblk_32_63; /**< In CBC mode IV[63:32] and in CTR[63:32].
         Only used when the mode is an AES CBC/CTR mode. */
        uint32 aesIvCtrblk_64_95; /**< In CBC mode IV[95:64] and in CTR[95:64].
         Only used when the mode is an AES CBC/CTR mode. */
        uint32 aesIvCtrblk_96_127; /**< In CBC mode IV[127:96] and in CTR[127:96].
         Only used when the mode is an AES CBC/CTR mode. */
        bool aesWriteBackAll; /**< When true all the AES resulting data is written to memory.
         When false only the final block of the AES resulting data is written to memory.
         Only used when the mode is an AES mode. */
        uint8 hashOutLength; /**< The number of bytes of the hash result to be saved to memory.
         Only used when mode is a Hash mode. */
        bool moreDataToCome; /**< When false this is the last data block. When true more data is to
         come. */
        uint32 sourceAddress; /**< The physical address of the input data that needs to be processed. */
        uint32 destinationAddress; /**< The physical address (RAM only) where the resulting data needs
         to be written. If NULL the register is not written.*/
        uint32 dataSize; /**< The number of bytes that need to be processed. If this number is not
         a multiple of a block size, the data is automatically extended with zeros. */
        bool enableInterrupt; /**< When true the callback function is called after the operation
         has ended. */
        hw_aes_hash_cb callback; /**< The callback function that is called when enable interrupt
         is true. */
} hw_aes_hash_setup;

/**
 * \brief Enable AES/HASH engine clock
 *
 * This function enables the AES/HASH engine clock.
 */
__STATIC_INLINE void hw_aes_hash_enable_clock(void)
{
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, CLK_AMBA_REG, AES_CLK_ENABLE);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Disable AES/HASH engine clock
 *
 * This function disables the AES/HASH engine clock.
 */
__STATIC_INLINE void hw_aes_hash_disable_clock(void)
{
        GLOBAL_INT_DISABLE();
        REG_CLR_BIT(CRG_TOP, CLK_AMBA_REG, AES_CLK_ENABLE);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Check if AES/HASH engine clock is enabled
 *
 * \return Non-zero value if enabled, 0 otherwise
 */
__STATIC_INLINE int hw_aes_hash_clock_is_enabled(void)
{
        return (CRG_TOP->CLK_AMBA_REG) & (CRG_TOP_CLK_AMBA_REG_AES_CLK_ENABLE_Msk);
}

/**
 * \brief AES/Hash enable.
 *
 * This function sets up and starts a AES/HASH engine operation. All the
 * configuration details are included in the setup input structure.
 *
 * \param [in] setup The setup structure with setup values.
 *
 * \deprecated You can use hw_aes_hash_init with hw_aes_hash_start instead.
 *
 * \sa hw_aes_hash_init
 * \sa hw_aes_hash_start
 *
 */
void hw_aes_hash_enable(const hw_aes_hash_setup setup) DEPRECATED;

/**
 * \brief AES/Hash initialize.
 *
 * This function sets up a AES/HASH engine operation. All the configuration details
 * are included in the setup input structure. The operation can then start by calling
 * hw_aes_hash_start
 *
 * \param [in] setup The setup structure with setup values.
 *
 * \note There are some restrictions in the value of dataSize of the setup structure depending on
 *       the mode. This function will do appropriate checking using assertions. The following
 *       table shows what the value of dataSize should be:
 *
 * mode                | moreDataToCome = true | moreDataToCome = false
 * ------------------- | --------------------- | ----------------------
 * HW_AES_ECB          | multiple of 16        | multiple of 16
 * HW_AES_CBC          | multiple of 16        | no restrictions
 * HW_AES_CTR          | multiple of 16        | no restrictions
 * HW_HASH_MD5         | multiple of 8         | no restrictions
 * HW_HASH_SHA_1       | multiple of 8         | no restrictions
 * HW_HASH_SHA_256_224 | multiple of 8         | no restrictions
 * HW_HASH_SHA_256     | multiple of 8         | no restrictions
 * HW_HASH_SHA_384     | multiple of 8         | no restrictions
 * HW_HASH_SHA_512     | multiple of 8         | no restrictions
 * HW_HASH_SHA_512_224 | multiple of 8         | no restrictions
 * HW_HASH_SHA_512_256 | multiple of 8         | no restrictions
 *
 * \sa hw_aes_hash_start
 *
 */
void hw_aes_hash_init(hw_aes_hash_setup *setup);

/**
 * \brief AES/Hash restart.
 *
 * This function restarts the AES/Hash engine. This function can be used when the engine waits for
 * more input data.
 *
 * \param [in] sourceAddress The start address of the data that needs to be processed.
 * \param [in] dataSize The number of bytes that need to be processed. If this number is not
 * a multiple of a block size, the data is automatically extended with zeros.
 * \param [in] moreDataToCome When false this is the last data block. When true more data is to
 * come.
 *
 */
void hw_aes_hash_restart(const uint32 sourceAddress, const uint32 dataSize,
        const bool moreDataToCome);

/**
 * \brief AES/Hash is active.
 *
 * This function tells if the AES/Hash engine is active or not.
 *
 * \return True if the AES/Hash engine is active and false when it is inactive.
 *
 */
bool hw_aes_hash_is_active();

/**
 * \brief AES/Hash is waiting for more data.
 *
 * This function tells if the AES/Hash engine is waiting for more data or not.
 *
 * \return True if the AES/Hash engine is waiting more data and false when it is not.
 *
 */
bool hw_aes_hash_wait_for_in();

/**
 * \brief AES/Hash disable.
 *
 * This function disables the AES/HASH engine and its interrupt request signal.
 *
 * \param [in] waitTillInactive When true the AES/HASH engine is disabled after any pending operation
 * finishes. When false the AES/Hash is disabled immediately.
 *
 */
void hw_aes_hash_disable(const bool waitTillInactive);

/**
 * \brief Mark next input block as being last
 *
 * This function is used to configure the engine so as to consider the next input block
 * as the last of the operation. When the operation finishes, the engine's status
 * becomes "inactive".
 */
__STATIC_INLINE void hw_aes_hash_mark_input_block_as_last(void)
{
        REG_CLR_BIT(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_MORE_IN);
}

/**
 * \brief Mark next input block as not being last
 *
 * This function is used to configure the engine so as to expect more input blocks
 * after the operation. When the operation finishes, the engine's status
 * becomes "waiting for input".
 */
__STATIC_INLINE void hw_aes_hash_mark_input_block_as_not_last(void)
{
        REG_SET_BIT(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_MORE_IN);
}

/**
 * \brief Configure engine for AES ECB encryption/decryption
 *
 * This function configures the engine for AES ECB encryption/decryption
 *
 * \param[in] key_size The size of the key used in the encryption/decryption
 *
 * \warning AES ECB is not recommended for use in cryptographic protocols.
 */
__STATIC_INLINE void hw_aes_hash_cfg_aes_ecb(hw_aes_key_size key_size)
{
        uint32 crypto_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;

        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypto_ctrl_reg, 0);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypto_ctrl_reg, 0);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypto_ctrl_reg, 0);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_AES_KEY_SZ, crypto_ctrl_reg, key_size);

        AES_HASH->CRYPTO_CTRL_REG = crypto_ctrl_reg;
}

/**
 * \brief Configure engine for AES CTR encryption/decryption
 *
 * This function configures the engine for AES CTR encryption/decryption
 *
 * \param[in] key_size The size of the key used in the encryption/decryption
 */
__STATIC_INLINE void hw_aes_hash_cfg_aes_ctr(hw_aes_key_size key_size)
{
        uint32 crypto_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;

        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypto_ctrl_reg, 0);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypto_ctrl_reg, 2);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypto_ctrl_reg, 0);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_AES_KEY_SZ, crypto_ctrl_reg, key_size);

        AES_HASH->CRYPTO_CTRL_REG = crypto_ctrl_reg;
}

/**
 * \brief Configure engine for AES CBC encryption/decryption
 *
 * This function configures the engine for AES CBC encryption/decryption
 *
 * \param[in] key_size The size of the key used in the encryption/decryption
 */
__STATIC_INLINE void hw_aes_hash_cfg_aes_cbc(hw_aes_key_size key_size)
{
        uint32 crypto_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;

        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypto_ctrl_reg, 0);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypto_ctrl_reg, 3);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypto_ctrl_reg, 0);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_AES_KEY_SZ, crypto_ctrl_reg, key_size);

        AES_HASH->CRYPTO_CTRL_REG = crypto_ctrl_reg;
}

/**
 * \brief Configure engine for MD5 hash
 *
 * This function configures the engine to perform MD5 hashing
 *
 * \param[in] result_size The size in bytes of the result that the engine will write
 *                        to the output memory. Accepted values are 1 to 16. Out of
 *                        range values are adjusted to the closest limit.
 */
__STATIC_INLINE void hw_aes_hash_cfg_md5(unsigned int result_size)
{
        uint32_t crypto_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;

        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_OUT_LEN, crypto_ctrl_reg,
                (result_size > 16)? 15: (result_size == 0)? 0 : (result_size - 1));
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypto_ctrl_reg, 1);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypto_ctrl_reg, 0);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypto_ctrl_reg, 0);

        AES_HASH->CRYPTO_CTRL_REG = crypto_ctrl_reg;
}

/**
 * \brief Configure engine for SHA1 hash
 *
 * This function configures the engine to perform SHA1 hashing
 *
 * \param[in] result_size The size in bytes of the result that the engine will write
 *                        to the output memory. Accepted values are 1 to 20. Out of
 *                        range values are adjusted to the closest limit.
 */
__STATIC_INLINE void hw_aes_hash_cfg_sha1(unsigned int result_size)
{
        uint32_t crypto_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;

        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_OUT_LEN, crypto_ctrl_reg,
                (result_size > 20)? 19: (result_size == 0)? 0 : (result_size - 1));
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypto_ctrl_reg, 1);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypto_ctrl_reg, 0);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypto_ctrl_reg, 1);

        AES_HASH->CRYPTO_CTRL_REG = crypto_ctrl_reg;
}

/**
 * \brief Configure engine for SHA-224 hash
 *
 * This function configures the engine to perform SHA-224 hashing
 *
 * \param[in] result_size The size in bytes of the result that the engine will write
 *                        to the output memory. Accepted values are 1 to 28. Out of
 *                        range values are adjusted to the closest limit.
 */
__STATIC_INLINE void hw_aes_hash_cfg_sha_224(unsigned int result_size)
{
        uint32_t crypto_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;

        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_OUT_LEN, crypto_ctrl_reg,
                (result_size > 28)? 27: (result_size == 0)? 0 : (result_size - 1));
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypto_ctrl_reg, 1);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypto_ctrl_reg, 0);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypto_ctrl_reg, 2);

        AES_HASH->CRYPTO_CTRL_REG = crypto_ctrl_reg;
}

/**
 * \brief Configure engine for SHA-256 hash
 *
 * This function configures the engine to perform SHA-256 hashing
 *
 * \param[in] result_size The size in bytes of the result that the engine will write
 *                        to the output memory. Accepted values are 1 to 32. Out of
 *                        range values are adjusted to the closest limit.
 */
__STATIC_INLINE void hw_aes_hash_cfg_sha_256(unsigned int result_size)
{
        uint32_t crypto_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;

        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_OUT_LEN, crypto_ctrl_reg,
                (result_size > 32)? 31: (result_size == 0)? 0 : (result_size - 1));
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypto_ctrl_reg, 1);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypto_ctrl_reg, 0);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypto_ctrl_reg, 3);

        AES_HASH->CRYPTO_CTRL_REG = crypto_ctrl_reg;
}

/**
 * \brief Configure engine for SHA-384 hash
 *
 * This function configures the engine to perform SHA-384 hashing
 *
 * \param[in] result_size The size in bytes of the result that the engine will write
 *                        to the output memory. Accepted values are 1 to 48. Out of
 *                        range values are adjusted to the closest limit.
 */
__STATIC_INLINE void hw_aes_hash_cfg_sha_384(unsigned int result_size)
{
        uint32_t crypto_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;

        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_OUT_LEN, crypto_ctrl_reg,
                (result_size > 48)? 47: (result_size == 0)? 0 : (result_size - 1));
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypto_ctrl_reg, 1);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypto_ctrl_reg, 1);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypto_ctrl_reg, 0);

        AES_HASH->CRYPTO_CTRL_REG = crypto_ctrl_reg;
}

/**
 * \brief Configure engine for SHA-512 hash
 *
 * This function configures the engine to perform SHA-512 hashing
 *
 * \param[in] result_size The size in bytes of the result that the engine will write
 *                        to the output memory. Accepted values are 1 to 64. Out of
 *                        range values are adjusted to the closest limit.
 */
__STATIC_INLINE void hw_aes_hash_cfg_sha_512(unsigned int result_size)
{
        uint32_t crypto_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;

        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_OUT_LEN, crypto_ctrl_reg,
                (result_size > 64)? 63: (result_size == 0)? 0 : (result_size - 1));
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypto_ctrl_reg, 1);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypto_ctrl_reg, 1);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypto_ctrl_reg, 1);

        AES_HASH->CRYPTO_CTRL_REG = crypto_ctrl_reg;
}

/**
 * \brief Configure engine for SHA-512/224 hash
 *
 * This function configures the engine to perform SHA-512/224 hashing
 *
 * \param[in] result_size The size in bytes of the result that the engine will write
 *                        to the output memory. Accepted values are 1 to 28. Out of
 *                        range values are adjusted to the closest limit.
 */
__STATIC_INLINE void hw_aes_hash_cfg_sha_512_224(unsigned int result_size)
{
        uint32_t crypto_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;

        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_OUT_LEN, crypto_ctrl_reg,
                (result_size > 28)? 27: (result_size == 0)? 0 : (result_size - 1));
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypto_ctrl_reg, 1);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypto_ctrl_reg, 1);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypto_ctrl_reg, 2);

        AES_HASH->CRYPTO_CTRL_REG = crypto_ctrl_reg;
}

/**
 * \brief Configure engine for SHA-512/256 hash
 *
 * This function configures the engine to perform SHA-512/256 hashing
 *
 * \param[in] result_size The size in bytes of the result that the engine will write
 *                        to the output memory. Accepted values are 1 to 32. Out of
 *                        range values are adjusted to the closest limit.
 */
__STATIC_INLINE void hw_aes_hash_cfg_sha_512_256(unsigned int result_size)
{
        uint32_t crypto_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;

        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_OUT_LEN, crypto_ctrl_reg,
                (result_size > 32)? 31: (result_size == 0)? 0 : (result_size - 1));
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypto_ctrl_reg, 1);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypto_ctrl_reg, 1);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypto_ctrl_reg, 3);

        AES_HASH->CRYPTO_CTRL_REG = crypto_ctrl_reg;
}

/**
 * \brief Store initialization vector in AES/HASH engine memory
 *
 * This function stores the initialization vector (IV) that is necessary for AES CBC
 * mode.
 *
 *
 * \sa hw_aes_store_ic
 *
 * \param[in] iv The address of the buffer containing the initialization vector
 */
void hw_aes_hash_store_iv(const uint8 *iv);

/**
 * \brief Store counter initialization in AES/HASH engine memory
 *
 * This function stores the counter initialization that is necessary for AES CTR
 * mode.
 *
 *
 * \sa hw_aes_store_iv
 *
 * \param[in] ic The address of the buffer containing the counter initialization
 */
void hw_aes_hash_store_ic(const uint8 *ic);

/**
 * \brief Store AES keys in AES/HASH engine memory
 *
 * This function stores the keys used for AES encryption/decryption in AES/HASH engine memory.
 * If key expansion is performed by the engine, then aes_keys should contain only the base
 * key. Otherwise it should contain all the expanded keys.
 *
 * \note
 * After an AES operation with a certain key, further operations using the same key do not
 * need storing it to the AES/HASH memory again.
 *
 * \param[in] key_size The size of the key used in the encryption/decryption
 * \param[in] aes_keys The address of the buffer containing the keys
 * \param[in] key_exp Define if key expansion will be performed by hw or sw
 */
void hw_aes_hash_store_keys(hw_aes_key_size key_size, const uint8 *aes_keys, hw_aes_hash_key_exp_t key_exp);

/**
 * \brief Configure DMA for data manipulation
 *
 * This function configures the DMA machine with the source and destination buffers.
 *
 * \param[in] src The physical address of the buffer containing the input data for the operation
 * \param[in] dst The physical address (RAM or Cache RAM only) of the buffer where the output of the
 *                operation will be stored. The dst address must be NULL when configuring the DMA
 *                while the engine is waiting for more input data.
 * \param[in] len The length of the input data
 */
void hw_aes_hash_cfg_dma(const uint8 *src, uint8 *dst, unsigned int len);

/**
 * \brief Start AES/HASH engine operation
 *
 * This function starts a AES/HASH operation. The operation depends on the configuration that
 * has been applied before calling this function
 *
 * \sa hw_aes_hash_init
 *
 */
__STATIC_INLINE void hw_aes_hash_start(void)
{
        AES_HASH->CRYPTO_START_REG = 1;
}

/**
 * \brief Start AES encryption
 *
 * This function starts an AES encryption. AES mode, key and input/output data should be configured
 * before calling this function.
 *
 * \sa hw_aes_hash_init
 * \sa hw_aes_hash_cfg_aes_ecb
 * \sa hw_aes_hash_cfg_aes_ctr
 * \sa hw_aes_hash_cfg_aes_cbc
 * \sa hw_aes_hash_store_keys
 * \sa hw_aes_hash_cfg_dma
 */
__STATIC_INLINE void hw_aes_hash_encrypt(void)
{
        REG_SET_BIT(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ENCDEC);

        AES_HASH->CRYPTO_START_REG = 1;
}

/**
 * \brief Start AES decryption
 *
 * This function starts an AES decryption. AES mode, key and input/output data should be configured
 * before calling this function.
 *
 * \sa hw_aes_hash_init
 * \sa hw_aes_hash_cfg_aes_ecb
 * \sa hw_aes_hash_cfg_aes_ctr
 * \sa hw_aes_hash_cfg_aes_cbc
 * \sa hw_aes_hash_store_keys
 * \sa hw_aes_hash_cfg_dma
 */
__STATIC_INLINE void hw_aes_hash_decrypt(void)
{
        REG_CLR_BIT(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ENCDEC);

        AES_HASH->CRYPTO_START_REG = 1;
}

/**
 * \brief Enable AES/HASH engine interrupt
 *
 * This function enables the AES/HASH engine interrupt and sets a callback function
 * to be called when the interrupt occurs.
 *
 * \param[in] cb The callback function for the interrupt
 *
 * \deprecated Consider using hw_crypto API along with hw_aes_hash_enable_interrupt_source()
 */
DEPRECATED_MSG("consider using hw_crypto API along with hw_aes_hash_enable_interrupt_source")
void hw_aes_hash_enable_interrupt(hw_aes_hash_cb cb);

/**
 * \brief Disable AES/HASH engine interrupt
 *
 * This function disables the AES/HASH engine interrupt.
 *
 * \deprecated Consider using hw_crypto API along with hw_aes_hash_disable_interrupt_source()
 */
DEPRECATED_MSG("consider using hw_crypto API along with hw_aes_hash_disable_interrupt_source")
void hw_aes_hash_disable_interrupt(void);

/**
 * \brief Enable AES/HASH engine interrupt source
 *
 * This function enables AES/HASH engine interrupt source.
 *
 * \note AES/HASH engine and ECC engine are common sources of CRYPTO system interrupt. This
 *       function does not enable the CRYPTO interrupt itself. Use hw_crypto_enable_aes_hash_interrupt()
 *       in addition, in order to enable the CRYPTO interrupt.
 */
static inline void hw_aes_hash_enable_interrupt_source(void)
{
        REG_SET_BIT(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_IRQ_EN);
}

/**
 * \brief Disable AES/HASH engine interrupt source
 *
 * This function disables AES/HASH engine interrupt source.
 *
 * \note AES/HASH engine and ECC engine are common sources of CRYPTO system interrupt. This
 *       function does not disable the CRYPTO interrupt itself. Use hw_crypto_disable_aes_hash_interrupt()
 *       in order to disable the CRYPTO interrupt.
 */
static inline void hw_aes_hash_disable_interrupt_source(void)
{
        REG_CLR_BIT(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_IRQ_EN);
}

/**
 * \brief Clear AES/HASH engine pending interrupt
 *
 * This function clears AES/HASH engine pending interrupt request.
 *
 * \note AES/HASH engine and ECC engine are common sources of CRYPTO system interrupt. This
 *       function does not clear pending CRYPTO interrupt. Use hw_crypto_clear_pending_interrupt()
 *       in order to clear pending CRYPTO interrupt.
 */
static inline void hw_aes_hash_clear_interrupt_req(void)
{
        AES_HASH->CRYPTO_CLRIRQ_REG = 0x1;
}

/**
 * \brief Set output mode to write all
 *
 * This function configures the AES/HASH engine to write back to memory all the
 * resulting data.
 *
 * \note
 * Only applicable to AES operations.
 */
__STATIC_INLINE void hw_aes_hash_output_mode_write_all(void)
{
        REG_CLR_BIT(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_OUT_MD);
}

/**
 * \brief Set output mode to write final
 *
 * This function configures the AES/HASH engine to write back to memory only the
 * the last block of the resulting data.
 *
 * \note
 * Only applicable to AES operations.
 */
__STATIC_INLINE void hw_aes_hash_output_mode_write_final(void)
{
        REG_SET_BIT(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_OUT_MD);
}

/**
 * \brief Check input data size restriction
 *
 * This function checks the restrictions of input data length. It returns 0 if the restrictions are not
 * violated, -1 otherwise. It checks the configured values at the time it is called so it should be
 * used just before starting an operation. The function can be useful for debugging. The following
 * table summarizes the restrictions for the input data length.
 *
 * ALGORITHM           | NOT LAST DATA BLOCK   | LAST DATA BLOCK
 * ------------------- | --------------------- | ----------------
 * HW_AES_ECB          | multiple of 16        | multiple of 16
 * HW_AES_CBC          | multiple of 16        | no restrictions
 * HW_AES_CTR          | multiple of 16        | no restrictions
 * HW_HASH_MD5         | multiple of 8         | no restrictions
 * HW_HASH_SHA_1       | multiple of 8         | no restrictions
 * HW_HASH_SHA_256_224 | multiple of 8         | no restrictions
 * HW_HASH_SHA_256     | multiple of 8         | no restrictions
 * HW_HASH_SHA_384     | multiple of 8         | no restrictions
 * HW_HASH_SHA_512     | multiple of 8         | no restrictions
 * HW_HASH_SHA_512_224 | multiple of 8         | no restrictions
 * HW_HASH_SHA_512_256 | multiple of 8         | no restrictions
 *
 */
int hw_aes_hash_check_restrictions(void);

#endif /* dg_configUSE_HW_AES_HASH */

#endif /* HW_AES_HASH_H_ */
/**
 * \}
 * \}
 * \}
 */
