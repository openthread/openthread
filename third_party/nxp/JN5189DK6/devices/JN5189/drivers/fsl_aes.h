/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_AES_H_
#define _FSL_AES_H_

#include "fsl_common.h"

/*!
 * @addtogroup aes
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief Defines LPC AES driver version 2.0.1.
 *
 * Change log:
 * - Version 2.0.0
 *   - initial version
 * - Version 2.0.1
 *   - GCM constant time tag comparison
 */
#define FSL_AES_DRIVER_VERSION (MAKE_VERSION(2, 0, 1))
/*@}*/

/*******************************************************************************
 * API
 *******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name AES Functional Operation
 * @{
 */

/*! AES block size in bytes */
#define AES_BLOCK_SIZE 16
/*! AES Input Vector size in bytes */
#define AES_IV_SIZE 16

/*!
 * @brief Sets AES key.
 *
 * Sets AES key.
 *
 * @param base AES peripheral base address
 * @param key Input key to use for encryption or decryption
 * @param keySize Size of the input key, in bytes. Must be 16, 24, or 32.
 * @return Status from Set Key operation
 */
status_t AES_SetKey(AES_Type *base, const uint8_t *key, size_t keySize);

/*!
 * @brief Encrypts AES using the ECB block mode.
 *
 * Encrypts AES using the ECB block mode.
 *
 * @param base AES peripheral base address
 * @param plaintext Input plain text to encrypt
 * @param[out] ciphertext Output cipher text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @return Status from encrypt operation
 */
status_t AES_EncryptEcb(AES_Type *base, const uint8_t *plaintext, uint8_t *ciphertext, size_t size);

/*!
 * @brief Decrypts AES using the ECB block mode.
 *
 * Decrypts AES using the ECB block mode.
 *
 * @param base AES peripheral base address
 * @param ciphertext Input ciphertext to decrypt
 * @param[out] plaintext Output plain text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @return Status from decrypt operation
 */
status_t AES_DecryptEcb(AES_Type *base, const uint8_t *ciphertext, uint8_t *plaintext, size_t size);

/*!
 * @brief Encrypts AES using CBC block mode.
 *
 * @param base AES peripheral base address
 * @param plaintext Input plain text to encrypt
 * @param[out] ciphertext Output cipher text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @param iv Input initial vector to combine with the first input block.
 * @return Status from encrypt operation
 */
status_t AES_EncryptCbc(
    AES_Type *base, const uint8_t *plaintext, uint8_t *ciphertext, size_t size, const uint8_t iv[AES_IV_SIZE]);

/*!
 * @brief Decrypts AES using CBC block mode.
 *
 * @param base AES peripheral base address
 * @param ciphertext Input cipher text to decrypt
 * @param[out] plaintext Output plain text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @param iv Input initial vector to combine with the first input block.
 * @return Status from decrypt operation
 */
status_t AES_DecryptCbc(
    AES_Type *base, const uint8_t *ciphertext, uint8_t *plaintext, size_t size, const uint8_t iv[AES_IV_SIZE]);

/*!
 * @brief Encrypts AES using CFB block mode.
 *
 * @param base AES peripheral base address
 * @param plaintext Input plain text to encrypt
 * @param[out] ciphertext Output cipher text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @param iv Input Initial vector to be used as the first input block.
 * @return Status from encrypt operation
 */
status_t AES_EncryptCfb(
    AES_Type *base, const uint8_t *plaintext, uint8_t *ciphertext, size_t size, const uint8_t iv[AES_IV_SIZE]);

/*!
 * @brief Decrypts AES using CFB block mode.
 *
 * @param base AES peripheral base address
 * @param ciphertext Input cipher text to decrypt
 * @param[out] plaintext Output plain text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @param iv Input Initial vector to be used as the first input block.
 * @return Status from decrypt operation
 */
status_t AES_DecryptCfb(
    AES_Type *base, const uint8_t *ciphertext, uint8_t *plaintext, size_t size, const uint8_t iv[AES_IV_SIZE]);

/*!
 * @brief Encrypts AES using OFB block mode.
 *
 * @param base AES peripheral base address
 * @param plaintext Input plain text to encrypt
 * @param[out] ciphertext Output cipher text
 * @param size Size of input and output data in bytes.
 * @param iv Input Initial vector to be used as the first input block.
 * @return Status from encrypt operation
 */
status_t AES_EncryptOfb(
    AES_Type *base, const uint8_t *plaintext, uint8_t *ciphertext, size_t size, const uint8_t iv[AES_IV_SIZE]);

/*!
 * @brief Decrypts AES using OFB block mode.
 *
 * @param base AES peripheral base address
 * @param ciphertext Input cipher text to decrypt
 * @param[out] plaintext Output plain text
 * @param size Size of input and output data in bytes.
 * @param iv Input Initial vector to be used as the first input block.
 * @return Status from decrypt operation
 */
status_t AES_DecryptOfb(
    AES_Type *base, const uint8_t *ciphertext, uint8_t *plaintext, size_t size, const uint8_t iv[AES_IV_SIZE]);

/*!
 * @brief Encrypts or decrypts AES using CTR block mode.
 *
 * Encrypts or decrypts AES using CTR block mode.
 * AES CTR mode uses only forward AES cipher and same algorithm for encryption and decryption.
 * The only difference between encryption and decryption is that, for encryption, the input argument
 * is plain text and the output argument is cipher text. For decryption, the input argument is cipher text
 * and the output argument is plain text.
 *
 * @param base AES peripheral base address
 * @param input Input data for CTR block mode
 * @param[out] output Output data for CTR block mode
 * @param size Size of input and output data in bytes
 * @param[in,out] counter Input counter (updates on return)
 * @param[out] counterlast Output cipher of last counter, for chained CTR calls. NULL can be passed if chained calls are
 * not used.
 * @param[out] szLeft Output number of bytes in left unused in counterlast block. NULL can be passed if chained calls
 * are not used.
 * @return Status from crypt operation
 */
status_t AES_CryptCtr(AES_Type *base,
                      const uint8_t *input,
                      uint8_t *output,
                      size_t size,
                      uint8_t counter[AES_BLOCK_SIZE],
                      uint8_t counterlast[AES_BLOCK_SIZE],
                      size_t *szLeft);

/*!
 * @brief Encrypts AES and tags using GCM block mode.
 *
 * Encrypts AES and optionally tags using GCM block mode. If plaintext is NULL, only the GHASH is calculated and output
 * in the 'tag' field.
 *
 * @param base AES peripheral base address
 * @param plaintext Input plain text to encrypt
 * @param[out] ciphertext Output cipher text.
 * @param size Size of input and output data in bytes
 * @param iv Input initial vector
 * @param ivSize Size of the IV
 * @param aad Input additional authentication data
 * @param aadSize Input size in bytes of AAD
 * @param[out] tag Output hash tag. Set to NULL to skip tag processing.
 * @param tagSize Input size of the tag to generate, in bytes. Must be 4,8,12,13,14,15 or 16.
 * @return Status from encrypt operation
 */
status_t AES_EncryptTagGcm(AES_Type *base,
                           const uint8_t *plaintext,
                           uint8_t *ciphertext,
                           size_t size,
                           const uint8_t *iv,
                           size_t ivSize,
                           const uint8_t *aad,
                           size_t aadSize,
                           uint8_t *tag,
                           size_t tagSize);

/*!
 * @brief Decrypts AES and authenticates using GCM block mode.
 *
 * Decrypts AES and optionally authenticates using GCM block mode. If ciphertext is NULL, only the GHASH is calculated
 * and compared with the received GHASH in 'tag' field.
 *
 * @param base AES peripheral base address
 * @param ciphertext Input cipher text to decrypt
 * @param[out] plaintext Output plain text.
 * @param size Size of input and output data in bytes
 * @param iv Input initial vector
 * @param ivSize Size of the IV
 * @param aad Input additional authentication data
 * @param aadSize Input size in bytes of AAD
 * @param tag Input hash tag to compare. Set to NULL to skip tag processing.
 * @param tagSize Input size of the tag, in bytes. Must be 4, 8, 12, 13, 14, 15, or 16.
 * @return Status from decrypt operation
 */
status_t AES_DecryptTagGcm(AES_Type *base,
                           const uint8_t *ciphertext,
                           uint8_t *plaintext,
                           size_t size,
                           const uint8_t *iv,
                           size_t ivSize,
                           const uint8_t *aad,
                           size_t aadSize,
                           const uint8_t *tag,
                           size_t tagSize);

void AES_Init(AES_Type *base);

void AES_Deinit(AES_Type *base);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @}*/
/*! @}*/ /* end of group aes */

#endif /* _FSL_AES_H_ */
