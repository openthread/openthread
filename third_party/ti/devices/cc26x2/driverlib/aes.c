/******************************************************************************
*  Filename:       crypto.c
*  Revised:        2017-03-29 13:49:36 +0200 (Wed, 29 Mar 2017)
*  Revision:       48751
*
*  Description:    Driver for the aes functions of the crypto module
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
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
*
******************************************************************************/

#include "aes.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  AESStartDMAOperation
    #define AESStartDMAOperation            NOROM_AESStartDMAOperation
    #undef  AESLoadInitializationVector
    #define AESLoadInitializationVector     NOROM_AESLoadInitializationVector
    #undef  AESReadTag
    #define AESReadTag                      NOROM_AESReadTag
    #undef  AESWriteToKeyStore
    #define AESWriteToKeyStore              NOROM_AESWriteToKeyStore
    #undef  AESReadFromKeyStore
    #define AESReadFromKeyStore             NOROM_AESReadFromKeyStore
    #undef  AESWaitForIRQFlags
    #define AESWaitForIRQFlags              NOROM_AESWaitForIRQFlags
#endif

//*****************************************************************************
//
// Load the initialisation vector.
//
//*****************************************************************************
void AESLoadInitializationVector(uint32_t *initializationVector)
{
    // Write initialization vector to the aes registers
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV0) = initializationVector[0];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV1) = initializationVector[1];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV2) = initializationVector[2];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV3) = initializationVector[3];
}

//*****************************************************************************
//
// Start a crypto DMA operation.
//
//*****************************************************************************
void AESStartDMAOperation(uint8_t *channel0Addr, uint32_t channel0Length,  uint8_t *channel1Addr, uint32_t channel1Length)
{

    // Clear any outstanding events.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = CRYPTO_IRQCLR_RESULT_AVAIL_M | CRYPTO_IRQEN_DMA_IN_DONE_M; // This might need AES_IRQEN_DMA_IN_DONE as well

    while(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & (CRYPTO_IRQSTAT_DMA_IN_DONE_M | CRYPTO_IRQSTAT_RESULT_AVAIL_M));

    if (channel0Length && channel0Addr) {
        // Configure the DMA controller - enable both DMA channels.
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH0CTL, CRYPTO_DMACH0CTL_EN_BITN) = 1;

        // Base address of the payload data in ext. memory.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) = (uint32_t)channel0Addr;

        // Payload data length in bytes, equal to the cipher text length.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = channel0Length;
    }

    if (channel1Length && channel1Addr) {
        // Enable DMA channel 1.
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH1CTL, CRYPTO_DMACH1CTL_EN_BITN) = 1;

        // Base address of the output data buffer.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1EXTADDR) = (uint32_t)channel1Addr;

        // Output data length in bytes, equal to the cipher text length.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1LEN) = channel1Length;
    }
}

//*****************************************************************************
//
// Poll the IRQ status register and return.
//
//*****************************************************************************
uint32_t AESWaitForIRQFlags(uint32_t irqFlags)
{
    uint32_t irqTrigger = 0;
    // Wait for the DMA operation to complete. Add a delay to make sure we are
    // not flooding the bus with requests too much.
    do {
        CPUdelay(1);
    }
    while(!(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & irqFlags & (CRYPTO_IRQSTAT_DMA_IN_DONE_M | CRYPTO_IRQSTAT_RESULT_AVAIL_M)));

    // Save the IRQ trigger source
    irqTrigger = HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT);

    // Clear IRQ flags
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = irqFlags;

    while(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & irqFlags & (CRYPTO_IRQSTAT_DMA_IN_DONE_M | CRYPTO_IRQSTAT_RESULT_AVAIL_M));

    return irqTrigger;
}

//*****************************************************************************
//
// Transfer a key from CM3 memory to a key store location.
//
//*****************************************************************************
uint32_t AESWriteToKeyStore(uint8_t *aesKey, uint32_t aesKeySizeBytes, uint32_t keyStoreArea)
{
    // Check the arguments.
    ASSERT((keyStoreArea == AES_KEY_AREA_0) ||
           (keyStoreArea == AES_KEY_AREA_1) ||
           (keyStoreArea == AES_KEY_AREA_2) ||
           (keyStoreArea == AES_KEY_AREA_3) ||
           (keyStoreArea == AES_KEY_AREA_4) ||
           (keyStoreArea == AES_KEY_AREA_5) ||
           (keyStoreArea == AES_KEY_AREA_6) ||
           (keyStoreArea == AES_KEY_AREA_7));

    ASSERT((aesKeySizeBytes == AES_128_KEY_LENGTH_BYTES) ||
           (aesKeySizeBytes == AES_192_KEY_LENGTH_BYTES) ||
           (aesKeySizeBytes == AES_256_KEY_LENGTH_BYTES));

    uint32_t keySize = 0;

    switch (aesKeySizeBytes) {
        case AES_128_KEY_LENGTH_BYTES:
            keySize = CRYPTO_KEYSIZE_SIZE_128_BIT;
            break;
        case AES_192_KEY_LENGTH_BYTES:
            keySize = CRYPTO_KEYSIZE_SIZE_192_BIT;
            break;
        case AES_256_KEY_LENGTH_BYTES:
            keySize = CRYPTO_KEYSIZE_SIZE_256_BIT;
            break;
    }

    // Clear any previously written key at the keyLocation
    AESInvalidateKey(keyStoreArea);

    // Disable the external interrupt to stop the interrupt form propagating
    // from the module to the System CPU.
    IntDisable(INT_CRYPTO_RESULT_AVAIL_IRQ);

    // Enable internal interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQTYPE) = CRYPTO_IRQTYPE_LEVEL_M;
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) = CRYPTO_IRQEN_DMA_IN_DONE_M | CRYPTO_IRQEN_RESULT_AVAIL_M;

    // Configure master control module.
    HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = CRYPTO_ALGSEL_KEY_STORE;

    // Clear any outstanding events.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = (CRYPTO_IRQCLR_DMA_IN_DONE | CRYPTO_IRQCLR_RESULT_AVAIL);

    // Configure the size of keys contained within the key store
    // Do not write to the register if the correct key size is already set.
    // Writing to this register causes all current keys to be invalidated.
    uint32_t keyStoreKeySize = HWREG(CRYPTO_BASE + CRYPTO_O_KEYSIZE);
    if (keySize != keyStoreKeySize) {
        HWREG(CRYPTO_BASE + CRYPTO_O_KEYSIZE) = keySize;
    }

    // Enable key to write (e.g. Key 0).
    HWREG(CRYPTO_BASE + CRYPTO_O_KEYWRITEAREA) = (1 << keyStoreArea);

    // Total key length in bytes (16 for 1 x 128-bit key and 32 for 1 x 256-bit key).
    AESStartDMAOperation(aesKey, aesKeySizeBytes, 0, 0);

    // Wait for the DMA operation to complete.
    uint32_t irqTrigger = AESWaitForIRQFlags(CRYPTO_IRQCLR_DMA_IN_DONE | CRYPTO_IRQCLR_RESULT_AVAIL | CRYPTO_IRQSTAT_DMA_BUS_ERR | CRYPTO_IRQSTAT_KEY_ST_WR_ERR);

    // If we correctly transfered the key into the keystore, return success.
    if ((irqTrigger & (CRYPTO_IRQSTAT_DMA_BUS_ERR_M | CRYPTO_IRQSTAT_KEY_ST_WR_ERR_M)) && !(HWREG(CRYPTO_BASE + CRYPTO_O_KEYWRITTENAREA) & (1 << keyStoreArea))) {
        // There was an error in writing to the keyStore.
        return AES_KEYSTORE_ERROR;
    }
    else {
        return AES_SUCCESS;
    }
}

//*****************************************************************************
//
// Transfer a key from the keyStoreArea to the internal buffer of the module.
//
//*****************************************************************************
uint32_t AESReadFromKeyStore(uint32_t keyStoreArea)
{
    // Check the arguments.
    ASSERT((keyStoreArea == AES_KEY_AREA_0) ||
           (keyStoreArea == AES_KEY_AREA_1) ||
           (keyStoreArea == AES_KEY_AREA_2) ||
           (keyStoreArea == AES_KEY_AREA_3) ||
           (keyStoreArea == AES_KEY_AREA_4) ||
           (keyStoreArea == AES_KEY_AREA_5) ||
           (keyStoreArea == AES_KEY_AREA_6) ||
           (keyStoreArea == AES_KEY_AREA_7));

    // Check if there is a valid key in the specified keyStoreArea
    if (!(HWREG(CRYPTO_BASE + CRYPTO_O_KEYWRITTENAREA) & (1 << keyStoreArea))) {
        return AES_KEYSTORE_AREA_INVALID;
    }

    // Enable keys to read (e.g. Key 0).
    HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) = keyStoreArea;

    // Wait until key is loaded to the AES module.
    // We cannot simply poll the IRQ status as only an error is communicated through
    // the IRQ status and not the completion of the transfer.
    do {
        CPUdelay(1);
    }
    while((HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) & CRYPTO_KEYREADAREA_BUSY_M));

    // Check for keyStore read error.
    if((HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & CRYPTO_IRQSTAT_KEY_ST_RD_ERR_M)) {
        return AES_KEYSTORE_ERROR;
    }
    else {
        return AES_SUCCESS;
    }
}

//*****************************************************************************
//
// Read the tag after a completed CCM or CBC-MAC operation.
//
//*****************************************************************************
uint32_t AESReadTag(uint32_t *tag)
{
    // If the tag is not ready, return an error code.
    if (!(HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) & CRYPTO_AESCTL_SAVED_CONTEXT_RDY_M)) {
        return AES_TAG_NOT_READY;
    }

    // Read tag
    tag[0] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT0);
    tag[1] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT1);
    tag[2] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT2);
    tag[3] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT3);

    return AES_SUCCESS;
}
