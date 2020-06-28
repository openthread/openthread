/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_aes.h"
#include "string.h"

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.aes"
#endif

/*!< Use standard C library memcpy  */
#define aes_memcpy memcpy

/*! @brief Definitions used for writing to the AES module CFG register. */
typedef enum _aes_configuration
{
    kAES_CfgEncryptEcb            = 0x001u, /*!< OUTTEXT_SEL, HOLD_SEL and INBLK_SEL = Encrypt Ecb */
    kAES_CfgDecryptEcb            = 0x001u, /*!< OUTTEXT_SEL, HOLD_SEL and INBLK_SEL = Decrypt Ecb */
    kAES_CfgEncryptCbc            = 0x023u, /*!< OUTTEXT_SEL, HOLD_SEL and INBLK_SEL = Encrypt Cbc */
    kAES_CfgDecryptCbc            = 0x211u, /*!< OUTTEXT_SEL, HOLD_SEL and INBLK_SEL = Decrypt Cbc */
    kAES_CfgEncryptCfb            = 0x132u, /*!< OUTTEXT_SEL, HOLD_SEL and INBLK_SEL = Encrypt Cfb */
    kAES_CfgDecryptCfb            = 0x112u, /*!< OUTTEXT_SEL, HOLD_SEL and INBLK_SEL = Decrypt Cfb */
    kAES_CfgCryptOfb              = 0x122u, /*!< OUTTEXT_SEL, HOLD_SEL and INBLK_SEL = Ofb */
    kAES_CfgCryptCtr              = 0x102u, /*!< OUTTEXT_SEL, HOLD_SEL and INBLK_SEL = Ctr */
    kAES_CfgCryptGcmTag           = 0x102u, /*!< OUTTEXT_SEL, HOLD_SEL and INBLK_SEL = Gcm */
    kAES_CfgSwap                  = 0x51u,  /*!< INTEXT swap bytes, OUTTEXT swap bytes, PROC_EN = Encrypt/Decrypt */
    kAES_CfgIntextSwap            = 0x11u,  /*!< INTEXT swap bytes, PROC_EN = Encrypt/Decrypt */
    kAES_CfgSwapIntextHashIn      = 0x12u,  /*!< Swap INTEXT only, Hash INTEXT */
    kAES_CfgSwapIntextHashOut     = 0x16u,  /*!< Swap INTEXT only, Hash OUTTEXT */
    kAES_CfgSwapEnDecHashIn       = 0x53u,  /*!< Swap INTEXT and OUTTEXT, Encrypt/Decrypt and Hash, Hash INTEXT */
    kAES_CfgSwapEnDecHashOut      = 0x57u,  /*!< Swap INTEXT and OUTTEXT, Encrypt/Decrypt and Hash, Hash OUTTEXT */
    kAES_CfgSwapIntextEnDecHashIn = 0x13u,  /*!< Swap INTEXT, Encrypt/Decrypt and Hash, Hash INTEXT */
} aes_configuration_t;

/*! @brief Actual operation with AES. */
typedef enum _aes_encryption_decryption_mode
{
    kAES_ModeEncrypt = 0U, /*!< Encryption */
    kAES_ModeDecrypt = 1U, /*!< Decryption */
} aes_encryption_decryption_mode_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static void aes_gcm_dec32(uint8_t *input);

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Reads an unaligned word.
 *
 * This function creates a 32-bit word from an input array of four bytes.
 *
 * @param src Input array of four bytes. The array can start at any address in memory.
 * @return 32-bit unsigned int created from the input byte array.
 */
static inline uint32_t aes_get_word_from_unaligned(const uint8_t *srcAddr)
{
#if (!(defined(__CORTEX_M)) || (defined(__CORTEX_M) && (__CORTEX_M == 0)))
    register const uint8_t *src = srcAddr;
    /* Cortex M0 does not support misaligned loads */
    if ((uint32_t)src & 0x3u)
    {
        union _align_bytes_t
        {
            uint32_t word;
            uint8_t byte[sizeof(uint32_t)];
        } my_bytes;

        my_bytes.byte[0] = *src;
        my_bytes.byte[1] = *(src + 1);
        my_bytes.byte[2] = *(src + 2);
        my_bytes.byte[3] = *(src + 3);
        return my_bytes.word;
    }
    else
    {
        /* addr aligned to 0-modulo-4 so it is safe to type cast */
        return *((const uint32_t *)src);
    }
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
    /* -O3 optimization in Keil uses LDM instruction here (LDM r4!, {r0})
     *    which is wrong, because srcAddr might be unaligned.
     *    LDM on unaligned address causes hard-fault. so use memcpy() */
    uint32_t ret;
    memcpy(&ret, srcAddr, sizeof(uint32_t));
    return ret;
#else
    return *((const uint32_t *)srcAddr);
#endif
}

/*!
 * @brief Converts a 32-bit word into a byte array.
 *
 * This function creates an output array of four bytes from an input 32-bit word.
 *
 * @param srcWord Input 32-bit unsigned integer.
 * @param[out] dst Output array of four bytes. The array can start at any address in memory.
 */
static inline void aes_set_unaligned_from_word(uint32_t srcWord, uint8_t *dstAddr)
{
#if (!(defined(__CORTEX_M)) || (defined(__CORTEX_M) && (__CORTEX_M == 0)))
    register uint8_t *dst = dstAddr;
    /* Cortex M0 does not support misaligned stores */
    if ((uint32_t)dst & 0x3u)
    {
        *dst++ = (srcWord & 0x000000FFU);
        *dst++ = (srcWord & 0x0000FF00U) >> 8;
        *dst++ = (srcWord & 0x00FF0000U) >> 16;
        *dst++ = (srcWord & 0xFF000000U) >> 24;
    }
    else
    {
        *((uint32_t *)dstAddr) = srcWord; /* addr aligned to 0-modulo-4 so it is safe to type cast */
    }
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
    uint32_t ret;
    ret = srcWord;
    memcpy(dstAddr, &ret, sizeof(uint32_t));
    return;
#else
    *((uint32_t *)dstAddr) = srcWord;
#endif
}

/*!
 * @brief Swap bytes withing 32-bit word.
 *
 * This function changes endianess of a 32-bit word.
 *
 * @param in 32-bit unsigned integer
 * @return 32-bit unsigned integer with different endianess (big endian to little endian and vice versa).
 */
static uint32_t swap_bytes(uint32_t in)
{
    return (((in & 0x000000ffu) << 24) | ((in & 0x0000ff00u) << 8) | ((in & 0x00ff0000u) >> 8) |
            ((in & 0xff000000u) >> 24));
}

/*!
 * @brief Loads key into key registers.
 *
 * This function loads the key into the AES key registers.
 *
 * @param base AES peripheral base address.
 * @param key Input key in big endian format.
 * @param keySize Size of the input key in bytes. Must be 16, 24 or 32.
 * @return Status of the load key operation.
 */
static status_t aes_load_key(AES_Type *base, const uint8_t *key, size_t keySize)
{
    uint32_t aesCfgKeyCfg;
    uint32_t aesCfg;
    int index;
    int keyWords;

    switch (keySize)
    {
        case 16u:
            aesCfgKeyCfg = 0u;
            keyWords     = 4;
            break;

        case 24u:
            aesCfgKeyCfg = 1u;
            keyWords     = 6;
            break;

        case 32u:
            aesCfgKeyCfg = 2u;
            keyWords     = 8;
            break;

        default:
            keyWords = 0;
            break;
    }

    if (!keyWords)
    {
        /* invalidate a possibly valid key. user attempted to set a key but gives incorrect size. */
        base->CMD = AES_CMD_WIPE(1);
        /* wait for Idle */
        while (!(base->STAT & AES_STAT_IDLE_MASK))
        {
        }
        base->CMD = AES_CMD_WIPE(0u);
        return kStatus_InvalidArgument;
    }

    aesCfg = base->CFG;
    aesCfg &= ~AES_CFG_KEY_CFG_MASK;
    aesCfg |= AES_CFG_KEY_CFG(aesCfgKeyCfg);
    base->CFG = aesCfg;

    for (index = 0; index < keyWords; index++)
    {
        base->KEY[index] = swap_bytes(aes_get_word_from_unaligned(key));
        key += sizeof(uint32_t);
    }

    return kStatus_Success;
}

/*!
 * @brief AES process one 16-byte block.
 *
 * This function pushes 16 bytes of data to INTEXT and pops 16 bytes of data from OUTTEXT.
 *
 * @param base AES peripheral base address.
 * @param[out] output 16-byte block read from the OUTTEXT registers.
 * @param input 16-byte block written to the INTEXT registers.
 */
static void aes_one_block(AES_Type *base, uint8_t *output, const uint8_t *input)
{
    int index;
    uint32_t aesStat;

    /* If the IN_READY bit in STAT register is 1, write the input text in the INTEXT [3:0]
       registers. */
    index = 0;
    while (index < 4)
    {
        aesStat = base->STAT;

        if (aesStat & AES_STAT_IN_READY_MASK)
        {
            base->INTEXT[index] = aes_get_word_from_unaligned(input);
            input += sizeof(uint32_t);
            index++;
        }
    }

    index = 0;
    while (index < 4)
    {
        aesStat = base->STAT;

        if (aesStat & AES_STAT_OUT_READY_MASK)
        {
            aes_set_unaligned_from_word(base->OUTTEXT[index], output);
            output += sizeof(uint32_t);
            index++;
        }
    }
}

/*!
 * @brief AES switch to forward mode.
 *
 * This function sets the AES to forward mode if it is in reverse mode.
 *
 * @param base AES peripheral base address.
 */
static void aes_set_forward(AES_Type *base)
{
    if (AES_STAT_REVERSE_MASK == (base->STAT & AES_STAT_REVERSE_MASK))
    {
        /* currently in reverse, switch to forward */
        base->CMD = AES_CMD_SWITCH_MODE(1u);

        /* wait for Idle */
        while (!(base->STAT & AES_STAT_IDLE_MASK))
        {
        }

        base->CMD = AES_CMD_SWITCH_MODE(0u);
    }
}

/*!
 * @brief AES switch to reverse mode.
 *
 * This function sets the AES to reverse mode if it is in forward mode.
 *
 * @param base AES peripheral base address.
 */
static void aes_set_reverse(AES_Type *base)
{
    if (!(base->STAT & AES_STAT_REVERSE_MASK))
    {
        /* currently in forward, switch to reverse */
        base->CMD = AES_CMD_SWITCH_MODE(1u);

        /* wait for Idle */
        while (!(base->STAT & AES_STAT_IDLE_MASK))
        {
        }

        base->CMD = AES_CMD_SWITCH_MODE(0u);
    }
}

/*!
 * @brief AES write HOLDING registers.
 *
 * This function writes input 16-byte data to AES HOLDING registers.
 *
 * @param base AES peripheral base address.
 * @param input 16-byte input data.
 */
static void aes_set_holding(AES_Type *base, const uint8_t *input)
{
    int index;

    for (index = 0; index < 4; index++)
    {
        base->HOLDING[index] = swap_bytes(aes_get_word_from_unaligned(input));
        input += sizeof(uint32_t);
    }
}

/*!
 * @brief AES check KEY_VALID.
 *
 * This function check the KEY_VALID bit in AES status register.
 *
 * @param base AES peripheral base address.
 * @return kStatus_Success if the key is valid.
 * @return kStatus_InvalidArgument if the key is invalid.
 */
static status_t aes_check_key_valid(AES_Type *base)
{
    status_t status;

    /* Check the key is valid */
    status =
        (AES_STAT_KEY_VALID_MASK == (base->STAT & AES_STAT_KEY_VALID_MASK)) ? kStatus_Success : kStatus_InvalidArgument;

    return status;
}

/*!
 * brief Sets AES key.
 *
 * Sets AES key.
 *
 * param base AES peripheral base address
 * param key Input key to use for encryption or decryption
 * param keySize Size of the input key, in bytes. Must be 16, 24, or 32.
 * return Status from Set Key operation
 */
status_t AES_SetKey(AES_Type *base, const uint8_t *key, size_t keySize)
{
    return aes_load_key(base, key, keySize);
}

/*!
 * brief Encrypts AES using the ECB block mode.
 *
 * Encrypts AES using the ECB block mode.
 *
 * param base AES peripheral base address
 * param plaintext Input plain text to encrypt
 * param[out] ciphertext Output cipher text
 * param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * return Status from encrypt operation
 */
status_t AES_EncryptEcb(AES_Type *base, const uint8_t *plaintext, uint8_t *ciphertext, size_t size)
{
    status_t status;

    /* ECB mode, size must be 16-byte multiple */
    if (size % 16u)
    {
        return kStatus_InvalidArgument;
    }

    /*5. Select the required crypto operation (encryption/decryption/hash) and AES mode in the CFG register.*/
    *(volatile uint8_t *)((uint32_t)base)         = (uint8_t)kAES_CfgSwap;
    *(volatile uint16_t *)(((uint32_t)base + 2u)) = (uint16_t)kAES_CfgEncryptEcb;

    status = aes_check_key_valid(base);
    if (status != kStatus_Success)
    {
        return status;
    }

    /* make sure AES is set to forward mode */
    aes_set_forward(base);

    while (size)
    {
        aes_one_block(base, ciphertext, plaintext);
        ciphertext += 16;
        plaintext += 16;
        size -= 16u;
    }

    return status;
}

/*!
 * brief Decrypts AES using the ECB block mode.
 *
 * Decrypts AES using the ECB block mode.
 *
 * param base AES peripheral base address
 * param ciphertext Input ciphertext to decrypt
 * param[out] plaintext Output plain text
 * param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * return Status from decrypt operation
 */
status_t AES_DecryptEcb(AES_Type *base, const uint8_t *ciphertext, uint8_t *plaintext, size_t size)
{
    status_t status;

    /* ECB mode, size must be 16-byte multiple */
    if (size % 16u)
    {
        return kStatus_InvalidArgument;
    }

    status = aes_check_key_valid(base);
    if (status != kStatus_Success)
    {
        return status;
    }

    /*5. Select the required crypto operation (encryption/decryption/hash) and AES mode in the CFG register.*/
    *(volatile uint8_t *)((uint32_t)base)         = (uint8_t)kAES_CfgSwap;
    *(volatile uint16_t *)(((uint32_t)base + 2u)) = (uint16_t)kAES_CfgDecryptEcb;

    /* make sure AES is set to reverse mode */
    aes_set_reverse(base);

    while (size)
    {
        aes_one_block(base, plaintext, ciphertext);
        ciphertext += 16;
        plaintext += 16;
        size -= 16u;
    }

    return status;
}

/*!
 * @brief Main function for CBC, CFB and OFB modes.
 *
 * @param base AES peripheral base address
 * @param input Input plaintext for encryption, input ciphertext for decryption.
 * @param[out] output Output ciphertext for encryption, output plaintext for decryption.
 * @param size Size of input and output data in bytes. For CBC and CFB it must be multiple of 16-bytes.
 * @param iv Input initial vector to combine with the first input block.
 * @return Status from the operation.
 */
static status_t aes_block_mode(AES_Type *base,
                               const uint8_t *input,
                               uint8_t *output,
                               size_t size,
                               const uint8_t iv[AES_IV_SIZE],
                               aes_configuration_t configuration)
{
    status_t status;

    /* CBC and CFB128 mode, size must be 16-byte multiple */
    switch (configuration)
    {
        case kAES_CfgEncryptCfb:
        case kAES_CfgDecryptCfb:
        case kAES_CfgEncryptCbc:
        case kAES_CfgDecryptCbc:
            if (size % 16u)
            {
                status = kStatus_InvalidArgument;
            }
            else
            {
                status = kStatus_Success;
            }
            break;

        case kAES_CfgCryptOfb:
            status = kStatus_Success;
            break;

        default:
            status = kStatus_InvalidArgument;
            break;
    }
    if (status != kStatus_Success)
    {
        return status;
    }

    /* Check the key is valid */
    status = aes_check_key_valid(base);
    if (status != kStatus_Success)
    {
        return status;
    }

    /*5. Select the required crypto operation (encryption/decryption/hash) and AES mode in the CFG register.*/
    *(volatile uint8_t *)((uint32_t)base)         = (uint8_t)kAES_CfgSwap;
    *(volatile uint16_t *)(((uint32_t)base + 2u)) = (uint16_t)configuration;

    /* CBC decrypt use reverse AES cipher. */
    switch (configuration)
    {
        case kAES_CfgDecryptCbc:
            aes_set_reverse(base);
            break;

        default:
            aes_set_forward(base);
            break;
    }

    /* For CBC, CFB and OFB modes write the Initialization Vector (IV) in the HOLDING
       [3:0] registers. For CTR mode write the initial counter value in the HOLDING [3:0]
       registers. */
    aes_set_holding(base, iv);

    while (size >= 16u)
    {
        aes_one_block(base, output, input);
        size -= 16u;
        output += 16;
        input += 16;
    }

    /* OFB can have non-block multiple size. CBC and CFB128 have size zero at this moment. */
    if (size)
    {
        uint8_t blkTemp[16] = {0};

        aes_memcpy(blkTemp, input, size);
        aes_one_block(base, blkTemp, blkTemp);
        aes_memcpy(output, blkTemp, size);
    }

    return status;
}

/*!
 * brief Encrypts AES using CBC block mode.
 *
 * param base AES peripheral base address
 * param plaintext Input plain text to encrypt
 * param[out] ciphertext Output cipher text
 * param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * param iv Input initial vector to combine with the first input block.
 * return Status from encrypt operation
 */
status_t AES_EncryptCbc(
    AES_Type *base, const uint8_t *plaintext, uint8_t *ciphertext, size_t size, const uint8_t iv[AES_IV_SIZE])
{
    return aes_block_mode(base, plaintext, ciphertext, size, iv, kAES_CfgEncryptCbc);
}

/*!
 * brief Decrypts AES using CBC block mode.
 *
 * param base AES peripheral base address
 * param ciphertext Input cipher text to decrypt
 * param[out] plaintext Output plain text
 * param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * param iv Input initial vector to combine with the first input block.
 * return Status from decrypt operation
 */
status_t AES_DecryptCbc(
    AES_Type *base, const uint8_t *ciphertext, uint8_t *plaintext, size_t size, const uint8_t iv[AES_IV_SIZE])
{
    return aes_block_mode(base, ciphertext, plaintext, size, iv, kAES_CfgDecryptCbc);
}

/*!
 * brief Encrypts AES using CFB block mode.
 *
 * param base AES peripheral base address
 * param plaintext Input plain text to encrypt
 * param[out] ciphertext Output cipher text
 * param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * param iv Input Initial vector to be used as the first input block.
 * return Status from encrypt operation
 */
status_t AES_EncryptCfb(
    AES_Type *base, const uint8_t *plaintext, uint8_t *ciphertext, size_t size, const uint8_t iv[AES_IV_SIZE])
{
    return aes_block_mode(base, plaintext, ciphertext, size, iv, kAES_CfgEncryptCfb);
}

/*!
 * brief Decrypts AES using CFB block mode.
 *
 * param base AES peripheral base address
 * param ciphertext Input cipher text to decrypt
 * param[out] plaintext Output plain text
 * param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * param iv Input Initial vector to be used as the first input block.
 * return Status from decrypt operation
 */
status_t AES_DecryptCfb(
    AES_Type *base, const uint8_t *ciphertext, uint8_t *plaintext, size_t size, const uint8_t iv[AES_IV_SIZE])
{
    return aes_block_mode(base, ciphertext, plaintext, size, iv, kAES_CfgDecryptCfb);
}

/*!
 * brief Encrypts AES using OFB block mode.
 *
 * param base AES peripheral base address
 * param plaintext Input plain text to encrypt
 * param[out] ciphertext Output cipher text
 * param size Size of input and output data in bytes.
 * param iv Input Initial vector to be used as the first input block.
 * return Status from encrypt operation
 */
status_t AES_EncryptOfb(
    AES_Type *base, const uint8_t *plaintext, uint8_t *ciphertext, size_t size, const uint8_t iv[AES_IV_SIZE])
{
    return aes_block_mode(base, plaintext, ciphertext, size, iv, kAES_CfgCryptOfb);
}

/*!
 * brief Decrypts AES using OFB block mode.
 *
 * param base AES peripheral base address
 * param ciphertext Input cipher text to decrypt
 * param[out] plaintext Output plain text
 * param size Size of input and output data in bytes.
 * param iv Input Initial vector to be used as the first input block.
 * return Status from decrypt operation
 */
status_t AES_DecryptOfb(
    AES_Type *base, const uint8_t *ciphertext, uint8_t *plaintext, size_t size, const uint8_t iv[AES_IV_SIZE])
{
    return aes_block_mode(base, ciphertext, plaintext, size, iv, kAES_CfgCryptOfb);
}

/*!
 * brief Encrypts or decrypts AES using CTR block mode.
 *
 * Encrypts or decrypts AES using CTR block mode.
 * AES CTR mode uses only forward AES cipher and same algorithm for encryption and decryption.
 * The only difference between encryption and decryption is that, for encryption, the input argument
 * is plain text and the output argument is cipher text. For decryption, the input argument is cipher text
 * and the output argument is plain text.
 *
 * param base AES peripheral base address
 * param input Input data for CTR block mode
 * param[out] output Output data for CTR block mode
 * param size Size of input and output data in bytes
 * param[in,out] counter Input counter (updates on return)
 * param[out] counterlast Output cipher of last counter, for chained CTR calls. NULL can be passed if chained calls are
 * not used.
 * param[out] szLeft Output number of bytes in left unused in counterlast block. NULL can be passed if chained calls
 * are not used.
 * return Status from crypt operation
 */
status_t AES_CryptCtr(AES_Type *base,
                      const uint8_t *input,
                      uint8_t *output,
                      size_t size,
                      uint8_t counter[AES_BLOCK_SIZE],
                      uint8_t counterlast[AES_BLOCK_SIZE],
                      size_t *szLeft)
{
    status_t status;
    uint32_t lastSize;
    uint8_t lastBlock[AES_BLOCK_SIZE] = {0};
    uint8_t *lastEncryptedCounter;

    /* Check the key is valid */
    status = aes_check_key_valid(base);
    if (status != kStatus_Success)
    {
        return status;
    }

    /*5. Select the required crypto operation (encryption/decryption/hash) and AES mode in the CFG register.*/
    *(volatile uint8_t *)((uint32_t)base)         = (uint8_t)kAES_CfgSwap;
    *(volatile uint16_t *)(((uint32_t)base + 2u)) = (uint16_t)kAES_CfgCryptCtr;
    aes_set_forward(base);

    /* For CBC, CFB and OFB modes write the Initialization Vector (IV) in the HOLDING
       [3:0] registers. For CTR mode write the initial counter value in the HOLDING [3:0]
       registers. */
    aes_set_holding(base, counter);

    /* set counter increment by one */
    base->CTR_INCR = 0x1U;

    /* Split the size into full 16-byte chunks and last incomplete block */
    lastSize = 0U;
    if (size <= 16U)
    {
        lastSize = size;
        size     = 0U;
    }
    else
    {
        /* Process all 16-byte data chunks. */
        lastSize = size % 16U;
        if (lastSize == 0U)
        {
            lastSize = 16U;
            size -= 16U;
        }
        else
        {
            size -= lastSize; /* size will be rounded down to 16 byte boundary. remaining bytes in lastSize */
        }
    }

    /* full 16-byte blocks */
    while (size)
    {
        aes_one_block(base, output, input);
        size -= 16u;
        input += 16;
        output += 16;
    }

    /* last block */
    if (counterlast)
    {
        lastEncryptedCounter = counterlast;
    }
    else
    {
        lastEncryptedCounter = lastBlock;
    }
    aes_one_block(base, lastEncryptedCounter, lastBlock); /* lastBlock is all zeroes, so I get directly ECB of the last
                                                             counter, as the XOR with zeroes doesn't change */

    /* remain output = input XOR counterlast */
    for (uint32_t i = 0; i < lastSize; i++)
    {
        output[i] = input[i] ^ lastEncryptedCounter[i];
    }

    /* read counter value after last encryption */
    aes_set_unaligned_from_word(swap_bytes(base->HOLDING[3]), &counter[12]);
    aes_gcm_dec32(&counter[0]); /* TODO HW problem? (first time it increments HOLDING3 twice) */

    if (szLeft)
    {
        *szLeft = 16U - lastSize;
    }

    return status;
}

/*!
 * @brief AES write input to INTEXT registers.
 *
 * This function writes input 16-byte data to AES INTEXT registers
 * and wait for the engine Idle.
 *
 * @param base AES peripheral base address.
 * @param input 16-byte input data.
 */
static void aes_gcm_one_block_input_only(AES_Type *base, const uint8_t *input)
{
    int index;
    uint32_t aesStat;

    /* If the IN_READY bit in STAT register is 1, write the input text in the INTEXT [3:0]
       registers. */
    index = 0;
    while (index < 4)
    {
        aesStat = base->STAT;

        if (aesStat & AES_STAT_IN_READY_MASK)
        {
            base->INTEXT[index] = aes_get_word_from_unaligned(input);
            input += sizeof(uint32_t);
            index++;
        }
    }

    while (0 == (base->STAT & AES_STAT_IDLE_MASK))
    {
    }
}

/*!
 * @brief AES execute command.
 *
 * This function issues command to AES CMD register.
 *
 * @param base AES peripheral base address.
 * @param cmdMask Bit mask for the command to be issued.
 */
static void aes_command(AES_Type *base, uint32_t cmdMask)
{
    base->CMD = cmdMask;
    /* wait for Idle */
    while (!(base->STAT & AES_STAT_IDLE_MASK))
    {
    }
    base->CMD = 0;
}

/*!
 * @brief AES read GCM_TAG.
 *
 * This function reads data from GCM_TAG registers.
 *
 * @param base AES peripheral base address.
 * @param output 16 byte memory to write the GCM_TAG to.
 */
static void aes_gcm_get_tag(AES_Type *base, uint8_t *output)
{
    int index;

    for (index = 0; index < 4; index++)
    {
        aes_set_unaligned_from_word(swap_bytes(base->GCM_TAG[index]), output);
        output += sizeof(uint32_t);
    }
}

/*!
 * @brief AES GCM check validity of input arguments.
 *
 * This function checks the validity of input arguments.
 *
 * @param base AES peripheral base address.
 * @param src Source address (plaintext or ciphertext).
 * @param iv Initialization vector address.
 * @param aad Additional authenticated data address.
 * @param dst Destination address (plaintext or ciphertext).
 * @param inputSize Size of input (same size as output) data in bytes.
 * @param ivSize Size of Initialization vector in bytes.
 * @param aadSize Size of additional data in bytes.
 * @param tagSize Size of the GCM tag in bytes.
 */
static status_t aes_gcm_check_input_args(AES_Type *base,
                                         const uint8_t *src,
                                         const uint8_t *iv,
                                         const uint8_t *aad,
                                         uint8_t *dst,
                                         size_t inputSize,
                                         size_t ivSize,
                                         size_t aadSize,
                                         size_t tagSize)
{
    if (!base)
    {
        return kStatus_InvalidArgument;
    }

    /* tag can be NULL to skip tag processing */
    if ((ivSize && (!iv)) || (aadSize && (!aad)) || (inputSize && ((!src) || (!dst))))
    {
        return kStatus_InvalidArgument;
    }

    /* octet length of tag (tagSize) must be element of 4,8,12,13,14,15,16 */
    if (((tagSize > 16u) || (tagSize < 12u)) && (tagSize != 4u) && (tagSize != 8u))
    {
        return kStatus_InvalidArgument;
    }

    /* no IV AAD DATA makes no sense */
    if (0 == (inputSize + ivSize + aadSize))
    {
        return kStatus_InvalidArgument;
    }

    /* check length of input strings. This is more strict than the GCM specificaiton due to 32-bit architecture.
     * The API interface would work on 64-bit architecture as well, but as it has not been tested, let it limit to
     * 32-bits.
     */
    if (!((ivSize >= 1u) && (sizeof(size_t) <= 4u)))
    {
        return kStatus_InvalidArgument;
    }

    return kStatus_Success;
}

/*!
 * @brief Increment a 16 byte integer.
 *
 * This function increments by one a 16 byte integer.
 *
 * @param input Pointer to a 16 byte integer to be incremented by one.
 */
static void aes_gcm_incr32(uint8_t *input)
{
    int i = 15;
    while (input[i] == (uint8_t)0xFFu)
    {
        input[i] = (uint8_t)0x00u;
        i--;
        if (i < 0)
        {
            return;
        }
    }

    if (i >= 0)
    {
        input[i] += (uint8_t)1u;
    }
}

/*!
 * @brief Decrement a 16 byte integer.
 *
 * This function decrements by one a 16 byte integer.
 *
 * @param input Pointer to a 16 byte integer to be decremented by one.
 */
static void aes_gcm_dec32(uint8_t *input)
{
    int i = 15;
    while (input[i] == (uint8_t)0x00u)
    {
        input[i] = (uint8_t)0xFFu;
        i--;
        if (i < 0)
        {
            return;
        }
    }

    if (i >= 0)
    {
        input[i] -= (uint8_t)1u;
    }
}

/*!
 * @brief AES read GF128_Z registers.
 *
 * This function reads AES module GF128_Z registers.
 *
 * @param base AES peripheral base address.
 * @param input Pointer to a 16 byte integer to write the GF128_Z value to.
 */
static void aes_get_gf128(AES_Type *base, uint8_t *output)
{
    int i;
    for (i = 0; i < 4; i++)
    {
        aes_set_unaligned_from_word(swap_bytes(base->GF128_Z[i]), output);
        output += sizeof(uint32_t);
    }
}

/*!
 * @brief GCM process.
 *
 * This function is the main function for AES GCM encryption/decryption and tag generation/comparison.
 *
 * @param base AES peripheral base address.
 * @param encryptMode Whether the actual operation is an encryption or a decryption.
 * @param src Input plaintext for encryption or ciphertext for decryption.
 * @param inputSize Size of input and output in bytes.
 * @param iv Input initial vector.
 * @param ivSize Size of initial vector in bytes.
 * @param aad Additional authenticated data.
 * @param aadSize Size of additional authenticated data in bytes.
 * @param dst Output ciphertext for encryption or plaintext for decryption.
 * @param tag For encryption, GCM tag output. For decryption, GCM tag input for comparison.
 * @param tagSize Size of the GCM tag in bytes.
 */
static status_t aes_gcm_process(AES_Type *base,
                                aes_encryption_decryption_mode_t encryptMode,
                                const uint8_t *src,
                                size_t inputSize,
                                const uint8_t *iv,
                                size_t ivSize,
                                const uint8_t *aad,
                                size_t aadSize,
                                uint8_t *dst,
                                uint8_t *tag,
                                size_t tagSize)
{
    status_t status;
    uint8_t blkZero[16] = {0};
    uint8_t blkJ0[16]   = {0};
    uint8_t blkTag[16]  = {0};
    uint32_t saveSize;
    uint32_t saveAadSize;
    uint32_t saveIvSize;
    uint8_t lastBlock[AES_BLOCK_SIZE] = {0};

    status = aes_gcm_check_input_args(base, src, iv, aad, dst, inputSize, ivSize, aadSize, tagSize);
    if (status != kStatus_Success)
    {
        return status;
    }

    status = aes_check_key_valid(base);
    if (status != kStatus_Success)
    {
        return status;
    }

    saveSize    = inputSize;
    saveAadSize = aadSize;
    saveIvSize  = ivSize;

    /* 1. Let H = CIPHK(0_128). */
    *(volatile uint8_t *)((uint32_t)base) = (uint8_t)
        kAES_CfgIntextSwap; /* do not swap OUTTEXT, as OUTTEXT is copied for further use into GF128_Y register */
    *(volatile uint16_t *)(((uint32_t)base + 2u)) = (uint16_t)kAES_CfgEncryptEcb;

    /* make sure AES is set to forward mode */
    aes_set_forward(base);

    /* move 128-bit zeroes to INTEXT */
    aes_gcm_one_block_input_only(base, blkZero);

    /* set COPY_TO_Y command */
    aes_command(base, AES_CMD_COPY_TO_Y(1));

    if (ivSize != 12u)
    {
        /* if ivSize is not 12 bytes, we have to compute GF128 hash to get the initial counter J0 */
        uint8_t ivBlkZero[16] = {0};

        *(volatile uint8_t *)((uint32_t)base) = (uint8_t)kAES_CfgSwapIntextHashIn;
        while (ivSize >= 16u)
        {
            aes_gcm_one_block_input_only(base, iv);
            iv += 16;
            ivSize -= 16u;
        }
        if (ivSize)
        {
            aes_memcpy(ivBlkZero, iv, ivSize);
            aes_gcm_one_block_input_only(base, ivBlkZero);
        }

        aes_memcpy(ivBlkZero, blkZero, 16);
        aes_set_unaligned_from_word(swap_bytes(8 * saveIvSize), &ivBlkZero[12]);
        aes_gcm_one_block_input_only(base, ivBlkZero);

        aes_get_gf128(base, blkJ0);
        aes_gcm_incr32(blkJ0);

        /* put back the hash sub-key. this will abort running GF128, because we have to start new GF128 again for AAD
         * and C */
        *(volatile uint8_t *)((uint32_t)base) = (uint8_t)
            kAES_CfgIntextSwap; /* do not swap OUTTEXT, as OUTTEXT is copied for further use into GF128_Y register */
        *(volatile uint16_t *)(((uint32_t)base + 2u)) = (uint16_t)kAES_CfgEncryptEcb;
        aes_gcm_one_block_input_only(base, blkZero);
        aes_command(base, AES_CMD_COPY_TO_Y(1));
    }
    else
    {
        aes_memcpy(blkJ0, iv, 12);
        blkJ0[15] = 0x02U; /* add one to Counter for the first encryption with plaintext - see GCM specification */
    }

    /* process all AAD as GF128 of INTEXT, pad to full 16-bytes block with zeroes */
    if (aadSize)
    {
        *(volatile uint8_t *)((uint32_t)base) = (uint8_t)kAES_CfgSwapIntextHashIn;
        while (aadSize >= 16u)
        {
            aes_gcm_one_block_input_only(base, aad);
            aad += 16;
            aadSize -= 16u;
        }
        if (aadSize)
        {
            uint8_t aadBlkZero[16] = {0};
            aes_memcpy(aadBlkZero, aad, aadSize);
            aes_gcm_one_block_input_only(base, aadBlkZero);
        }
    }

    /* switch to EnryptDecryptHash, Hash of OUTTEXT, EncryptDecrypt in GCM mode */
    /* Process all plaintext, pad to full 16-bytes block with zeroes */
    if (encryptMode == kAES_ModeEncrypt)
    {
        *(volatile uint8_t *)((uint32_t)base) =
            (uint8_t)kAES_CfgSwapEnDecHashOut; /* Encrypt operation adds output to hash */
    }
    else
    {
        *(volatile uint8_t *)((uint32_t)base) =
            (uint8_t)kAES_CfgSwapEnDecHashIn; /* Decrypt operation adds input to hash */
    }

    *(volatile uint16_t *)(((uint32_t)base + 2u)) = (uint16_t) /*kAES_CfgCryptGcmTag*/ kAES_CfgCryptCtr;

    aes_set_holding(base, blkJ0);

    /* set counter increment by one */
    base->CTR_INCR = 0x1U;

    if (inputSize)
    {
        /* full 16-byte blocks */
        while (inputSize >= 16u)
        {
            aes_one_block(base, dst, src);
            inputSize -= 16u;
            src += 16;
            dst += 16;
        }
        /* last incomplete block. output shall be padded. */
        if (inputSize)
        {
            if (encryptMode == kAES_ModeEncrypt)
            {
                uint8_t outputBlkZero[16] = {0};

                /* I need to pad ciphertext, so I turn off the hash, and after I have ciphertext, I pad it with zeroes
                 * and hash manually */
                *(volatile uint8_t *)((uint32_t)base) = (uint8_t)kAES_CfgSwap;
                aes_memcpy(lastBlock, src, inputSize);
                aes_one_block(base, lastBlock, lastBlock);
                aes_memcpy(dst, lastBlock, inputSize);
                /* pad the last output block with zeroes and add it to GF128 hash */
                aes_memcpy(outputBlkZero, lastBlock, inputSize);
                *(volatile uint8_t *)((uint32_t)base) = (uint8_t)kAES_CfgSwapIntextHashIn;
                aes_gcm_one_block_input_only(base, outputBlkZero);
            }
            else
            {
                aes_memcpy(lastBlock, src, inputSize);
                aes_one_block(base, lastBlock, lastBlock);
                aes_memcpy(dst, lastBlock, inputSize);
            }
        }
    }

    /* Process AAD len plus Ciphertext len */
    *(volatile uint8_t *)((uint32_t)base)         = (uint8_t)kAES_CfgIntextSwap;
    *(volatile uint16_t *)(((uint32_t)base + 2u)) = (uint16_t)kAES_CfgEncryptEcb;
    aes_gcm_dec32(blkJ0);
    aes_gcm_one_block_input_only(base, blkJ0);
    *(volatile uint8_t *)((uint32_t)base) = (uint8_t)kAES_CfgSwapIntextHashIn;
    if (saveSize)
    {
        aes_set_unaligned_from_word(swap_bytes(saveSize * 8u), &blkZero[12]);
    }
    if (saveAadSize)
    {
        aes_set_unaligned_from_word(swap_bytes(saveAadSize * 8u), &blkZero[4]);
    }
    aes_gcm_one_block_input_only(base, blkZero); /* len(A) || len(C) */
    aes_gcm_get_tag(base, blkTag);

    if (tag && tagSize)
    {
        if (encryptMode == kAES_ModeEncrypt)
        {
            aes_memcpy(tag, blkTag, tagSize);
        }
        else
        {
            size_t i       = 0;
            uint32_t equal = 0;
            uint32_t chXor;

            while (i < tagSize)
            {
                chXor = tag[i] ^ blkTag[i];
                equal |= chXor;
                i++;
            }

            if (equal != 0)
            {
                status = kStatus_Fail;
            }
        }
    }

    return status;
}

/*!
 * brief Encrypts AES and tags using GCM block mode.
 *
 * Encrypts AES and optionally tags using GCM block mode. If plaintext is NULL, only the GHASH is calculated and output
 * in the 'tag' field.
 *
 * param base AES peripheral base address
 * param plaintext Input plain text to encrypt
 * param[out] ciphertext Output cipher text.
 * param size Size of input and output data in bytes
 * param iv Input initial vector
 * param ivSize Size of the IV
 * param aad Input additional authentication data
 * param aadSize Input size in bytes of AAD
 * param[out] tag Output hash tag. Set to NULL to skip tag processing.
 * param tagSize Input size of the tag to generate, in bytes. Must be 4,8,12,13,14,15 or 16.
 * return Status from encrypt operation
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
                           size_t tagSize)
{
    status_t status;
    status =
        aes_gcm_process(base, kAES_ModeEncrypt, plaintext, size, iv, ivSize, aad, aadSize, ciphertext, tag, tagSize);
    return status;
}

/*!
 * brief Decrypts AES and authenticates using GCM block mode.
 *
 * Decrypts AES and optionally authenticates using GCM block mode. If ciphertext is NULL, only the GHASH is calculated
 * and compared with the received GHASH in 'tag' field.
 *
 * param base AES peripheral base address
 * param ciphertext Input cipher text to decrypt
 * param[out] plaintext Output plain text.
 * param size Size of input and output data in bytes
 * param iv Input initial vector
 * param ivSize Size of the IV
 * param aad Input additional authentication data
 * param aadSize Input size in bytes of AAD
 * param tag Input hash tag to compare. Set to NULL to skip tag processing.
 * param tagSize Input size of the tag, in bytes. Must be 4, 8, 12, 13, 14, 15, or 16.
 * return Status from decrypt operation
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
                           size_t tagSize)
{
    uint8_t temp_tag[16] = {0}; /* max. octet length of MAC (tag) is 16 */
    uint8_t *tag_ptr;
    status_t status;

    tag_ptr = NULL;
    if (tag)
    {
        aes_memcpy(temp_tag, tag, tagSize);
        tag_ptr = &temp_tag[0];
    }

    status = aes_gcm_process(base, kAES_ModeDecrypt, ciphertext, size, iv, ivSize, aad, aadSize, plaintext, tag_ptr,
                             tagSize);
    return status;
}

void AES_Init(AES_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* ungate clock */
    CLOCK_EnableClock(kCLOCK_Aes);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void AES_Deinit(AES_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* gate clock */
    CLOCK_DisableClock(kCLOCK_Aes);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}
