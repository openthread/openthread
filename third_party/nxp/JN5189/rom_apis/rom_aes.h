/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _ROM_AES_H
#define _ROM_AES_H

/*******************
 * INCLUDE FILES    *
 ********************/
#include <stdint.h>
#include "rom_common.h"

/*!
 * @addtogroup ROM_API
 * @{
 */

/*! @file */

typedef uint32_t ErrorCode_t;  /*!< enum defined in error.h */

/*******************
 * EXPORTED MACROS  *
 ********************/

#define AES_ENCDEC_MODE (1 << 0)
#define AES_GF128HASH_MODE (2 << 0)
#define AES_ENDEC_GF128HASH_MODE (3 << 0)
#define AES_GF128_SEL (1 << 2)
#define AES_INT_BSWAP (1 << 4)
#define AES_INT_WSWAP (1 << 5)
#define AES_OUTT_BSWAP (1 << 6)
#define AES_OUTT_WSWAP (1 << 7)
#define AES_KEYSIZE_128 (0 << 8)
#define AES_KEYSIZE_192 (1 << 8)
#define AES_KEYSIZE_256 (2 << 8)
#define AES_INB_FSEL(n) ((n) << 16) /*!< n->1=Input Text, n->2=Holding, n->3=Input Text XOR Holding */
#define AES_HOLD_FSEL(n) \
    ((n) << 20) /*!< n->0=Counter, n->1=Input Text, n->2=Output Block, n->3=Input Text XOR Output Block */
#define AES_OUTT_FSEL(n) ((n) << 24) /*!< n->0=OUTT, n->1=Output Block XOR Input Text, n->2=Output Block XOR Holding */

/*********************
 * EXPORTED TYPEDEFS  *
 **********************/

/*! @brief AES setup modes */
typedef enum
{
    AES_MODE_ECB_ENCRYPT = 0,
    AES_MODE_ECB_DECRYPT,
    AES_MODE_CBC_ENCRYPT,
    AES_MODE_CBC_DECRYPT,
    AES_MODE_CFB_ENCRYPT,
    AES_MODE_CFB_DECRYPT,
    AES_MODE_OFB,
    AES_MODE_CTR,
    AES_MODE_GCM_TAG,
    AES_MODE_UNUSED = 0x7FFFFFFF /*!< Not used, but forces enum to 32-bit size */
} AES_MODE_T;

/*! @brief Size of the AES key */
typedef enum
{
    AES_KEY_128BITS = 0, /*!< KEY size 128 bits */
    AES_KEY_192BITS, /*!< KEY size 192 bits */
    AES_KEY_256BITS, /*!< KEY size 256 bits */
    AES_FVAL = 0x7FFFFFFF /*!< Not used, but forces enum to 32-bit size and unsigned */
} AES_KEY_SIZE_T;

/*******************
 * EXPORTED DATA    *
 ********************/

/********************************
 * EXPORTED FUNCTIONS PROTOTYPES *
 *********************************/

/**
 * @brief	Initialize the AES
 *
 * @return	LPC_OK on success, or an error code (ERRORCODE_T) on failure
 * @note	Driver does not enable AES clock, power, or perform reset peripheral (if needed).
 */
static inline ErrorCode_t aesInit(void)
{
    ErrorCode_t (*p_aesInit)(void);
    p_aesInit = (ErrorCode_t (*)(void))0x03001161U;

    return p_aesInit();
}

/**
 * @brief	AES control function, byte write (useful for writing configuration register)
 * @brief	offset	: Register offset in AES, 32-bit aligned value
 * @brief	val8	: 8-bit value to write
 * @return	Nothing
 * @note	This is an obfuscated function available from the ROM API as a 2nd level API
 *			call. An application can used it perform byte level write access to a register.
 *			This function is not meant to be public.
 */
static inline void aesWriteByte(uint32_t offset, uint8_t val8)
{
    void (*p_aesWriteByte)(uint32_t offset, uint8_t val8);
    p_aesWriteByte = (void (*)(uint32_t offset, uint8_t val8))0x03001175U;

    p_aesWriteByte(offset, val8);
}

/**
 * @brief	AES control function, word write
 * @brief	offset	: Register offset in AES, 32-bit aligned value
 * @brief	val32	: 32-bit value to write
 * @return	Nothing
 * @note	This is an obfuscated function available from the ROM API as a 2nd level API
 *			call. An application can used it for write access to a register. This function
 *			is not meant to be public.
 */
static inline void aesWrite(uint32_t offset, uint32_t val32)
{
    void (*p_aesWrite)(uint32_t offset, uint32_t val32);
    p_aesWrite = (void (*)(uint32_t offset, uint32_t val32))0x0300118dU;

    p_aesWrite(offset, val32);
}

/**
 * @brief	AES control function, word read
 * @brief	offset	: Register offset in AES, 32-bit aligned value
 * @brief	pVal32	: Pointer to 32-bit area to read into
 * @return	Nothing
 * @note	This is an obfuscated function available from the ROM API as a 2nd level API
 *			call. An application can used it for read access to a register. This function
 *			is not meant to be public.
 */
static inline void aesRead(uint32_t offset, uint32_t *pVal32)
{
    void (*p_aesRead)(uint32_t offset, uint32_t *pVal32);
    p_aesRead = (void (*)(uint32_t offset, uint32_t *pVal32))0x030011a5U;

    p_aesRead(offset, pVal32);
}

/**
 * @brief	AES control function, block write (used for multi-register block writes)
 * @brief	offset		: Register offset in AES, 32-bit aligned value
 * @brief	pVal32		: Pointer to 32-bit array to write
 * @brief	numBytes	: Number of bytes to write, must be 32-bit aligned
 * @return	Nothing
 * @note	This is an obfuscated function available from the ROM API as a 2nd level API
 *			call. An application can used it for write access to a register. This function
 *			is not meant to be public. Writes occur in 32-bit chunks.
 */
static inline void aesWriteBlock(uint32_t offset, uint32_t *pVal32, uint32_t numBytes)
{
    void (*p_aesWriteBlock)(uint32_t offset, uint32_t *pVal32, uint32_t numBytes);
    p_aesWriteBlock = (void (*)(uint32_t offset, uint32_t *pVal32, uint32_t numBytes))0x030011c1U;

    p_aesWriteBlock(offset, pVal32, numBytes);
}

/**
 * @brief	AES control function, block read (used for multi-register block read)
 * @brief	offset		: Register offset in AES, 32-bit aligned value
 * @brief	pVal32		: Pointer to 32-bit array to read into
 * @brief	numBytes	: Number of bytes to read, must be 32-bit aligned
 * @return	Nothing
 * @note	This is an obfuscated function available from the ROM API as a 2nd level API
 *			call. An application can used it for read access to a register. This function
 *			is not meant to be public. Reads occur in 32-bit chunks. Read data if undefined
 *			if AES not present.
 */
static inline void aesReadBlock(uint32_t offset, uint32_t *pVal32, uint32_t numBytes)
{
    void (*p_aesReadBlock)(uint32_t offset, uint32_t *pVal32, uint32_t numBytes);
    p_aesReadBlock = (void (*)(uint32_t offset, uint32_t *pVal32, uint32_t numBytes))0x030011e9U;

    p_aesReadBlock(offset, pVal32, numBytes);
}

/**
 * @brief	Sets up the AES mode
 * @param	wipe	: use true to invalidate AES key and disable cipher
 * @param	flags	: Applies extra flags (Or'ed in config), normally should be 0, useful for swap bits only
 * @return	LPC_OK on success, or an error code (ERRORCODE_T) on failure
 */
static inline ErrorCode_t aesMode(AES_MODE_T modeVal, uint32_t flags)
{
    ErrorCode_t (*p_aesMode)(AES_MODE_T modeVal, uint32_t flags);
    p_aesMode = (ErrorCode_t (*)(AES_MODE_T modeVal, uint32_t flags))0x03001211U;

    return p_aesMode(modeVal, flags);
}

/**
 * @brief	Aborts optional AES operation and wipes AES engine
 * @param	wipe	: use true to invalidate AES key and disable cipher
 * @return	LPC_OK on success, or an error code (ERRORCODE_T) on failure
 */
static inline ErrorCode_t aesAbort(int wipe)
{
    ErrorCode_t (*p_aesAbort)(int wipe);
    p_aesAbort = (ErrorCode_t (*)(int wipe))0x03001269U;

    return p_aesAbort(wipe);
}

/**
 * @brief	Loads the increment that is used when in counter modes in the AES block
 * @param	counter	: 32-bit initial increment counter value
 * @return	LPC_OK on success, or an error code (ERRORCODE_T) on failure
 */
static inline ErrorCode_t aesLoadCounter(uint32_t counter)
{
    ErrorCode_t (*p_aesLoadCounter)(uint32_t counter);
    p_aesLoadCounter = (ErrorCode_t (*)(uint32_t counter))0x03001291U;

    return p_aesLoadCounter(counter);
}

/**
 * @brief	Loads the passed (software) key into the AES block
 * @param	keySize	: 0 = 128-bits, 1 = 192-bits, 2 = 256-bits, all other values are invalid (AES_KEY_SIZE_T)
 * @param	key		: Pointer to up to a 256-bit key array
 * @return	LPC_OK on success, or an error code (ERRORCODE_T) on failure
 */
static inline ErrorCode_t aesLoadKeyFromSW(AES_KEY_SIZE_T keySize, uint32_t *key)
{
    ErrorCode_t (*p_aesLoadKeyFromSW)(AES_KEY_SIZE_T keySize, uint32_t *key);
    p_aesLoadKeyFromSW = (ErrorCode_t (*)(AES_KEY_SIZE_T keySize, uint32_t *key))0x030012a9U;

    return p_aesLoadKeyFromSW(keySize, key);
}

/**
 * @brief	Loads the Initialization Vector (IV) into the AES block
 * @param	iv	: 32-bit initialization vector
 * @return	LPC_OK on success, or an error code (ERRORCODE_T) on failure
 */
static inline ErrorCode_t aesLoadIV(uint32_t *pIv)
{
    ErrorCode_t (*p_aesLoadIV)(uint32_t *pIv);
    p_aesLoadIV = (ErrorCode_t (*)(uint32_t *pIv))0x030012fdU;

    return p_aesLoadIV(pIv);
}

/**
 * @brief	Process AES blocks (descrypt or encrypt)
 * @param	pBlockIn	: 32-bit aligned pointer to input block of data
 * @param	pBlockOut	: 32-bit aligned pointer to output block of data
 * @param	numBlocks	: Number of blocks to process, block size = 128 bits
 * @return	LPC_OK on success, or an error code (ERRORCODE_T) on failure
 * @note	The AES mode and key must be setup prior to calling this function.
 *			For encryption. the plain text is used as the input and encrypted
 *			text is output. For descryption, plain text is output while
 *			encrypted text is input.
 */
static inline ErrorCode_t aesProcess(uint32_t *pBlockIn, uint32_t *pBlockOut, uint32_t numBlocks)
{
    ErrorCode_t (*p_aesProcess)(uint32_t *pBlockIn, uint32_t *pBlockOut, uint32_t numBlocks);
    p_aesProcess = (ErrorCode_t (*)(uint32_t *pBlockIn, uint32_t *pBlockOut, uint32_t numBlocks))0x030013d1U;

    return p_aesProcess(pBlockIn, pBlockOut, numBlocks);
}

/**
 * @brief	Sets the Y input of the GF128 hash used in GCM mode
 * @param	pYGf128	: Y input of GF128 hash (4x32-bit words)
 * @return	LPC_OK on success, or an error code (ERRORCODE_T) on failure
 * @note	Calling this function will reset the hash logic.
 */
static inline ErrorCode_t aesWriteYInputGf128(uint32_t *pYGf128)
{
    ErrorCode_t (*p_aesWriteYInputGf128)(uint32_t *pYGf128);
    p_aesWriteYInputGf128 = (ErrorCode_t (*)(uint32_t *pYGf128))0x0300132dU;

    return p_aesWriteYInputGf128(pYGf128);
}

/**
 * @brief	Reads the results of the GF128(Z) hash used in GCM mode
 * @param	pGf128Hash	: Array of 4x32-bit words to read hash into
 * @return	LPC_OK on success, or an error code (ERRORCODE_T) on failure
 * @note	Value is undefined if AES is not present.
 */
static inline ErrorCode_t aesReadGf128Hash(uint32_t *pGf128Hash)
{
    ErrorCode_t (*p_aesReadGf128Hash)(uint32_t *pGf128Hash);
    p_aesReadGf128Hash = (ErrorCode_t (*)(uint32_t *pGf128Hash))0x0300135dU;

    return p_aesReadGf128Hash(pGf128Hash);
}

/**
 * @brief	Reads the GCM tag
 * @param	pGcmTag	: Array of 4x32-bit words to read GCM tage into
 * @return	LPC_OK on success, or an error code (ERRORCODE_T) on failure
 * @note	The GCM tage is an XOR value of the Output Text and GF128(Z) hash value.
 *			Value is undefined if AES is not present.
 */
static inline ErrorCode_t aesReadGcmTag(uint32_t *pGcmTag)
{
    ErrorCode_t (*p_aesReadGcmTag)(uint32_t *pGcmTag);
    p_aesReadGcmTag = (ErrorCode_t (*)(uint32_t *pGcmTag))0x0300138dU;

    return p_aesReadGcmTag(pGcmTag);
}

/**
 * @brief	Returns the version of the AES driver in ROM
 * @return	Driver version, example 0x00000100 = v1.0
 */
static inline uint32_t aesGetDriverVersion(void)
{
    uint32_t (*p_aesGetDriverVersion)(void);
    p_aesGetDriverVersion = (uint32_t (*)(void))0x030013bdU;

    return p_aesGetDriverVersion();
}

/**
 * @brief	Returns status of AES IP block (supported or not)
 * @return	LPC_OK if enabled, ERR_SEC_AES_NOT_SUPPORTED if not supported
 */
static inline ErrorCode_t aesIsSupported(void)
{
    ErrorCode_t (*p_aesIsSupported)(void);
    p_aesIsSupported = (ErrorCode_t (*)(void))0x0300115dU;

    return p_aesIsSupported();
}

#endif /* _ROM_AES_H      Do not add any thing below this line */
