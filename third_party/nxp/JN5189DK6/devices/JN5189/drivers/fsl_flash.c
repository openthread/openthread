/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_flash.h"
#include "rom_api.h"
/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.jn_flash"
#endif

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

#define MAX_ERASE_LENGTH (FLASH_PAGE_SIZE * 100)

/*
 * Macros below participate to the Flash checksum calculation.
 * FLASH_CheckSum implements CMD_CHECKSUM function of the flash controller for whole pages strictly.
 * SW is required when needing to program a checksums or check over areas smaller than a whole flash page.
 */
#define RSHIFT_128BIT(_WORD_, _SHIFT_)     \
    _WORD_[0] >>= (uint32_t)_SHIFT_;\
    _WORD_[0] |= (uint32_t)((_WORD_[1] & (uint32_t)((1<<_SHIFT_)-1)) << (uint32_t)(32-_SHIFT_));\
    _WORD_[1] >>=(uint32_t)_SHIFT_;\
    _WORD_[1] |= (uint32_t)((_WORD_[2] & (uint32_t)((1<<_SHIFT_)-1)) << (uint32_t)(32-_SHIFT_));\
    _WORD_[2] >>=(uint32_t)_SHIFT_;\
    _WORD_[2] |= (uint32_t)((_WORD_[3] & (uint32_t)((1<<_SHIFT_)-1)) << (uint32_t)(32-_SHIFT_));\
    _WORD_[3] >>=(uint32_t)_SHIFT_;

#define PARITY(_WORD_) \
          (uint32_t)((_WORD_ & (uint32_t)(1<<0UL))>>0UL) \
        ^ (uint32_t)((_WORD_ & (uint32_t)(1<<2UL))>>2UL) \
        ^ (uint32_t)((_WORD_ & (uint32_t)(1<<27UL))>>27UL)\
        ^ (uint32_t)((_WORD_ & (uint32_t)(1<<29UL))>>29UL)
/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/
flash_config_t gFlashConfig;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief      Enable the FLASH
 *
 * @param      pFLASH Pointer to selected FLASHx peripheral
 *
 * @return     Nothing
 */
void FLASH_Init(FLASH_Type *pFLASH)
{
    int status;
    /* From Flash Adapter */
    gFlashConfig.PFlashSectorSize = FLASH_PAGE_SIZE;
    ROM_GetFlash(&gFlashConfig.PFlashBlockBase, &gFlashConfig.PFlashTotalSize);

    pFLASH->CMD = FLASH_CMD_INIT;

    status = FLASH_Wait(pFLASH);
    /* Loop if the flash controller detects an unrecoverable error */
    /* That should have been caught by the ROM code but might not !*/
    if (status & FLASH_FAIL)
    {
        while (1);
    }
}

/**
 * @brief      Power down the FLASH
 *
 * @param      pFLASH Pointer to selected FLASHx peripheral
 *
 * @return     Nothing
 */
void FLASH_Powerdown(FLASH_Type *pFLASH)
{
    pFLASH->INT_CLR_STATUS = FLASH_STAT_ALL;

    pFLASH->CMD = FLASH_CMD_POWERDOWN;

    FLASH_Wait(pFLASH);
}

/**
 * @brief      Wait for FLASH command to complete
 *
 * @param      pFLASH Pointer to selected FLASHx peripheral
 *
 * @return     INT_STATUS with ECC_ERR bit masked out
 * @see flash_status_t
 */
int FLASH_Wait(FLASH_Type *pFLASH)
{
    while (!(pFLASH->INT_STATUS & FLASH_DONE));
    /* mask out ECC_ERR bit that may raise independantly from flash commands */
    return (pFLASH->INT_STATUS & ~FLASH_ECC_ERR);
}

/**
 * @brief      Return unfiltered FLASH INT_STATUS.
 * In normal operation FLASH_DONE rises systematically but other status bits
 * may rise at the same time or have risen before to notify of an error.
 * Usually testing the value returned by FLASH_Wait is sufficionet but in some special
 * cases the raw value may be needed.
 *
 * @param      pFLASH Pointer to selected FLASHx peripheral.
 *
 * @return     INT_STATUS raw value.
 * @see flash_status_t
 */
int FLASH_GetStatus(FLASH_Type *pFLASH)
{
    return pFLASH->INT_STATUS;
}

/**
 * @brief      Erase page
 *
 * @param      pFLASH Pointer to selected FLASH peripheral
 * @param[in]  pu8Start Start address with page to inspect
 * @param[in]  pu8End End address (included in check)
 *
 * @return     INT_STATUS with ECC_ERR bit masked out
 * @see flash_status_t
 */
int FLASH_Erase(FLASH_Type *pFLASH, uint8_t *pu8Start, uint8_t *pu8End)
{
    int status = 0;

    uint32_t erase_length = pu8End - pu8Start;

    while (erase_length > 0)
    {
        pFLASH->INT_CLR_STATUS = FLASH_STAT_ALL;

        if (erase_length > MAX_ERASE_LENGTH)
        {
            pu8End = pu8Start + MAX_ERASE_LENGTH - 1;
            erase_length -= MAX_ERASE_LENGTH;
        }
        else
        {
            pu8End       = pu8Start + erase_length;
            erase_length = 0;
        }

        /* Set end address */
        *pu8End = 0xAA;

        /* Set start address */
        *pu8Start = 0xBB;

        pu8Start = pu8End + 1;

        pFLASH->CMD = FLASH_CMD_ERASE_RANGE;

        status = FLASH_Wait(pFLASH);
    }

    return status;
}

/**
 * @brief      Erase multiple pages
 *
 * @param      pFLASH Pointer to selected FLASH peripheral
 * @param[in]  u32StartPage Index of page to start erasing from
 * @param[in]  u32PageCount Number of pages to erase
 *
 * @return     INT_STATUS with ECC_ERR bit masked out
 * @see flash_status_t
 */
int FLASH_ErasePages(FLASH_Type *pFLASH, uint32_t u32StartPage, uint32_t u32PageCount)
{
    uint8_t *pu8Start = (uint8_t *)(FLASH_PAGE_SIZE * u32StartPage);
    uint8_t *pu8End   = (pu8Start + FLASH_PAGE_SIZE * u32PageCount) - 1;

    return FLASH_Erase(pFLASH, pu8Start, pu8End);
}

/**
 * @brief      Page Blank check
 *
 * @param      pFLASH Pointer to selected FLASH peripheral
 * @param[in]  pu8Start Start address with page to inspect
 * @param[in]  pu8End End address (included in check)
 *
 * @return     INT_STATUS with ECC_ERR bit masked out
 * @see flash_status_t
 */
int FLASH_BlankCheck(FLASH_Type *pFLASH, uint8_t *pu8Start, uint8_t *pu8End)
{
    pFLASH->INT_CLR_STATUS = FLASH_STAT_ALL;

    /* Set end address */
    *pu8End = 0xAA;

    /* Set start address */
    *pu8Start = 0xBB;

    pFLASH->CMD = FLASH_CMD_BLANK_CHECK;

    return FLASH_Wait(pFLASH);
}

/**
 * @brief      Margin Check
 *
 * @param      pFLASH Pointer to selected FLASH peripheral
 * @param[in]  pu8Start Start address with page to inspect
 * @param[in]  pu8End End address (included in check)
 *
 * @return     INT_STATUS with ECC_ERR bit masked out
 * @see flash_status_t
 */
int FLASH_MarginCheck(FLASH_Type *pFLASH, uint8_t *pu8Start, uint8_t *pu8End)
{
    pFLASH->INT_CLR_STATUS = FLASH_STAT_ALL;

    /* Set end address */
    *pu8End = 0xAA;

    /* Set start address */
    *pu8Start = 0xBB;

    pFLASH->CMD = FLASH_CMD_MARGIN_CHECK;

    return FLASH_Wait(pFLASH);
}

/**
 * @brief      Program page
 *
 * @param[in]  pFLASH Pointer to selected FLASH peripheral
 * @param[out] pu32Start Pointer location that must be programmed in flash
 * @param[in]  pu32Data Pointer to source buffer being written to flash
 * @param[in]  u32Length Number of bytes to be programmed
 *
 * @return     INT_STATUS with ECC_ERR bit masked out
 * @see flash_status_t
 */
int FLASH_Program(FLASH_Type *pFLASH, uint32_t *pu32Start, uint32_t *pu32Data, uint32_t u32Length)
{
    int status = 0;

    uint32_t end  = (uint32_t)pu32Start + u32Length;

    uint32_t padding  = (FLASH_PAGE_SIZE - (end & (FLASH_PAGE_SIZE-1))) & (FLASH_PAGE_SIZE-1);

    pFLASH->INT_CLR_STATUS = FLASH_STAT_ALL;

    pFLASH->AUTOPROG = FLASH_AUTO_PAGE;

    memcpy(pu32Start, pu32Data, u32Length);

    while (padding-- > 0)
    {
        *(uint8_t*)end ++ = 0;
    }

    status = FLASH_Wait(pFLASH);

    pFLASH->AUTOPROG = FLASH_AUTO_OFF;

    return status;
}

/**
 * @brief      Page Checksum
 *
 * @param      pFLASH Pointer to selected FLASH peripheral
 * @param[in]  pu8Start Pointer to data within starting page  page checksum must be computed
 * @param[in]  pu8End Pointer to data whose page is the last of the checksum calculation
 * @param[out] au32Checksum Four 32bit word array to store checksum calculation result
 *
 * @return     INT_STATUS with ECC_ERR bit masked out
 * @see flash_status_t
 */
int FLASH_Checksum(FLASH_Type *pFLASH, uint8_t *pu8Start, uint8_t *pu8End, uint32_t au32Checksum[4])
{
    int status;
    pFLASH->INT_CLR_STATUS = FLASH_STAT_ALL;

    /* Set end address */
    *pu8End = 0xAA;

    /* Set start address */
    *pu8Start = 0xBB;

    pFLASH->CMD = FLASH_CMD_CHECKSUM;

    status = FLASH_Wait(pFLASH);

    au32Checksum[0] = pFLASH->DATAW[0];
    au32Checksum[1] = pFLASH->DATAW[1];
    au32Checksum[2] = pFLASH->DATAW[2];
    au32Checksum[3] = pFLASH->DATAW[3];

    return status;
}

/**
 * @brief      Read flash word (16 byte worth of data)
 *
 * @param      pFLASH Pointer to selected FLASH peripheral
 * @param[in]  pu8Start Pointer to data to be read
 * @param[in]  u32ReadMode Read mode see also flash_read_mode_t
 * @param[out] au32Data Four 32bit word array to store read result
 *
 * @return     INT_STATUS with ECC_ERR bit masked out
 * @see flash_status_t
 */
int FLASH_Read(FLASH_Type *pFLASH, uint8_t *pu8Start, uint32_t u32ReadMode, uint32_t au32Data[4])
{
    int status;
    pFLASH->INT_CLR_STATUS = FLASH_STAT_ALL;

    /* Set start address */
    *pu8Start = 0xBB;

    /* Set read mode */
    pFLASH->DATAW[0] = u32ReadMode;

    pFLASH->CMD = FLASH_CMD_READ_SINGLE_WORD;

    status = FLASH_Wait(pFLASH);

    au32Data[0] = pFLASH->DATAW[0];
    au32Data[1] = pFLASH->DATAW[1];
    au32Data[2] = pFLASH->DATAW[2];
    au32Data[3] = pFLASH->DATAW[3];

    return status;
}

/*
* Details on the fields that can be updated through the FLASH_CMD_SET_READ_MODE cmd.
* bit 31   : prefetch enable
* bit 30   : ignore hprot[0] and assume that all accesses are code accesses
* bit 29-28: 00: hprot[3] specifies whether an access is cacheable
*            01: reserved
*            10: hprot[3] ignored, all accesses are not cacheable
*            11: hprot[3] ignored, all accesses are cacheable
* bit 27-8 : reserved
* bit 7    : ewle read mode active. Default value after reset is: 0
* bit 6-4  : number of extra precharge states.
* bit 3-0  : number of extra evaluation states.
*
* After reset, the only value field set is the "number of extra evaluation states" field. In other words, if you want to
* return to the default values, you shall write DEFAULT_READ_MODE to DATAW[0]
*/
#define DEFAULT_READ_MODE_VAL          0x00000000
#define EWLE_MODE_MASK                 0x80

/**
 * @brief      Configure the flash wait state depending of the elwe mode and CPU frequency.
 * When the CPU clock frequency is decreased, the Set Read command shall be called after the frequency change.
 * When the CPU clock frequency is increased, the Set Read command shall be called before the frequency change.
 *
 * @param      pFLASH Pointer to selected FLASHx peripheral
 * @param      freq_48M_not_32M CPU clock frequency @48MHz - lower or equal to 32Mhz if 0
 *
 * @return     Nothing
 */
void FLASH_SetReadMode(FLASH_Type *pFLASH, bool cpu_freq_48M_not_32M)
{
   int flash_ws = DEFAULT_READ_MODE_VAL;

   pFLASH->INT_CLR_STATUS = FLASH_STAT_ALL;

   flash_ws += cpu_freq_48M_not_32M ? 1 : 0;

   pFLASH->DATAW[0] = (EWLE_MODE_MASK | flash_ws);

   pFLASH->CMD = FLASH_CMD_SET_READ_MODE;

   /* no need to wait until command is completed: further accesses are stalled
    * until the command is completed. */
   //status = FLASH_Wait(pFLASH);
}

/**
 * @brief      Calculate checksum using the same checksum algorithm as the CMD_CHECKSUM implementation of the
 * Flash controller. When executed over a 512 byte page (page size) must return the same value as FLASH_Checksum.
 *
 * @param[in]  input Pointer to data over which checksum calculation must be executed.
 * @param[in]  nb_128b_words Number of 16 byte words on flash.
 * @param[out] misr Pointer on a four 32bit word array to store calculated checksum.
 * @param[in]  init Set to true to clear the misr buffer.
 *
 * @return     Nothing
 */
void FLASH_CalculateChecksum(const uint32_t *input,
                            size_t nb_128b_words,
                            uint32_t* misr,
                            int init)
{
    int i;

    if (init)
    {
        for (i = 0; i < 4; i++)
        {
            misr[i] = 0;
        }
    }

    for (i = 0; i < nb_128b_words*4; )
    {
        int cy;

        /* Compute carry */
        cy = PARITY(misr[0]);
        /* Shift right the 128 bits */
        RSHIFT_128BIT(misr, 1UL);
        /* Let Carry become the MISR[127] bit */
        misr[3] ^= (uint32_t)((cy&1UL) << 31);

        /* Xor with 128 bit word */
        misr[0] ^= input[i++];
        misr[1] ^= input[i++];
        misr[2] ^= input[i++];
        misr[3] ^= input[i++];
    }

}

/*
 * Expected values for Config page checksum and GPO checksum.
 */
const uint32_t CONFIG_PAGE_CHSUM[4] = {0x11112222U, 0x33334444U, 0x55556666U, 0x77778888U};
const uint32_t GPO_CHKSUM[4] =         {0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U};

/**
 * @brief      Calculate checksum over page (N-2) aka CONFIG page and check it matches the expected value.
 *
 * @param[in]  page_buffer Pointer to data over which checksum calculation must be executed.
 * @param[out] misr Pointer on a four 32bit word array to store calculated checksum.
 *             Note: this buffer is only useful for debugging purposes.
 *
 * @return Result of the page checksum verification:
 *         -  0: Verification successfully.
 *         - -1: Verification failed.
 */
int FLASH_ConfigPageVerifyPageChecksum(const uint32_t *page_buffer,
                                      uint32_t *misr)
{
    int res = 0;
    FLASH_CalculateChecksum(page_buffer, 32, &misr[0], 1);
    for (int i = 0; i < 4; i++)
    {
        if (misr[i] != CONFIG_PAGE_CHSUM[i])
        {
            res = -1;
            break;
        }
    }
    return res;
}

/**
 * @brief      Calculate checksum over GPO array of CONFIG page and check it matches the expected value
 *
 * @param[in]  page_buffer Pointer to data over which checksum calculation must be executed.
 * @param[out] misr Pointer on a 4 32bit word array to store calculated checksum.
 *             Note: this buffer is only useful for debugging purposes.
 *
 * @return Result of the GPO array checksum verification:
 *         -  0: Verification successfully.
 *         - -1: Verification failed.
 */
int FLASH_ConfigPageVerifyGpoChecksum(const uint32_t *page_buffer,
                                     uint32_t *misr)
{
    int res = 0;
    FLASH_CalculateChecksum(page_buffer, 5, misr, 1);
    for (int i = 0; i < 4; i++)
    {
        if (misr[i] != GPO_CHKSUM[i])
        {
            res = -1;
            break;
        }
    }
    return res;
}

/**
 * @brief      Configure the flash wait state depending of the elwe mode and CPU frequency.
 * When the CPU clock frequency is decreased, the Set Read command shall be called after the frequency change.
 * When the CPU clock frequency is increased, the Set Read command shall be called before the frequency change.
 *
 * @param      page_ram_buffer Pointer to RAM page buffer in which the read-modify-write of page (N-2) is performed
 * @param      gpo_chksum Pointer on a four 32bit word array to store calculated checksum.
 * @param      page_chksum Pointer on a four 32bit word array to store calculated checksum.
 *
 * @return     Nothing
 */
void FLASH_ConfigPageUpdate(uint32_t *page_ram_buffer,
                          uint32_t *gpo_chksum,
                          uint32_t *page_chksum )
{

    FLASH_CalculateChecksum(page_ram_buffer, 4, gpo_chksum, 1);
    FLASH_CalculateChecksum(GPO_CHKSUM, 1, gpo_chksum, 0);
    for (int i = 0; i < 4; i++)
    {
        page_ram_buffer[16+i] = gpo_chksum[i];
    }
    FLASH_CalculateChecksum(&page_ram_buffer[0], 31, page_chksum, 1);
    FLASH_CalculateChecksum(CONFIG_PAGE_CHSUM, 1, page_chksum, 0);
    for (int i = 0; i < 4; i++)
    {
        page_ram_buffer[124+i] = page_chksum[i];
    }
}
