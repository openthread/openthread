/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __FSL_FLASH_H_
#define __FSL_FLASH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "fsl_common.h"

/*!
 * @addtogroup jn_flash
 * @{
 */

/*! @file */

/*******************
 * EXPORTED MACROS  *
 ********************/

/* FLASH Commands */
#define FLASH_CMD_INIT 0
#define FLASH_CMD_POWERDOWN 1
#define FLASH_CMD_SET_READ_MODE 2
#define FLASH_CMD_READ_SINGLE_WORD 3
#define FLASH_CMD_ERASE_RANGE 4
#define FLASH_CMD_BLANK_CHECK 5
#define FLASH_CMD_MARGIN_CHECK 6
#define FLASH_CMD_CHECKSUM 7
#define FLASH_CMD_WRITE 8
#define FLASH_CMD_WRITE_PROG 10
#define FLASH_CMD_PROGRAM 12
#define FLASH_CMD_REPORT_ECC 13

/* FLASH Autoprogram modes */
#define FLASH_AUTO_OFF 0
#define FLASH_AUTO_WORD 1
#define FLASH_AUTO_PAGE 2

#define FLASH_BASE_ADDRESS 0
#define FLASH_PAGE_SIZE 512
#define FLASH_PAGE_SIZE_LOG 9

#define FLASH_CONFIG_PAGE_ADDR 0x9fc00
#define FLASH_TRIMMING_DATA_ADDR 0x9fe00

/**
 * @brief FLASH INT_STATUS register definitions
 */
#define FLASH_FAIL (1 << 0)    /*!< Command failed */
#define FLASH_ERR (1 << 1)     /*!< Illegal command */
#define FLASH_DONE (1 << 2)    /*!< Command complete */
#define FLASH_ECC_ERR (1 << 3) /*!< ECC error detected */

/**
 * @brief FLASH INT_ENABLE register definitions
 */
#define FLASH_FAIL (1 << 0)    /*!< Command failed */
#define FLASH_ERR (1 << 1)     /*!< Illegal command */
#define FLASH_DONE (1 << 2)    /*!< Command complete */
#define FLASH_ECC_ERR (1 << 3) /*!< ECC error detected */

#define FLASH_STAT_ALL (0xF)

/* FLASH Events */
#define FLASH_EVENT_RESET (1 << 0)
//#define FLASH_EVENT_WAKEUP         (1 << 1)
//#define FLASH_EVENT_ABORT          (1 << 2)

/******************************
 * EXPORTED TYPE DEFINITIONS  *
 ******************************/
typedef enum _flash_status
{
    kStatus_FLASH_Success         = FLASH_DONE,                             /*!< flash operation is successful*/
    kStatus_FLASH_Fail            = FLASH_DONE | FLASH_FAIL,                /*!< flash operation is not successful*/
    kStatus_FLASH_InvalidArgument = MAKE_STATUS(kStatusGroup_Generic, 4),   /*!< Invalid argument */
    kStatus_FLASH_AlignmentError  = MAKE_STATUS(kStatusGroup_FLASH, 6),     /*!< Alignment Error */
    kStatus_FLASH_EccError        = FLASH_DONE | FLASH_ECC_ERR,             /*!< ECC error detected */
    kStatus_FLASH_Error           = FLASH_DONE | FLASH_ERR,                 /*!< Illegal command */
} flash_status_t;


/* Read Mode related definitions */
#define FLASH_READ_MODE_RD_DMACC_SHIFT    15
#define FLASH_READ_MODE_SHIFT             10
#define FLASH_READ_MODE_NORMAL             0
#define FLASH_READ_MODE_MARGIN_VS_PROGRAM  1
#define FLASH_READ_MODE_MARGIN_VS_ERASE    2
#define FLASH_READ_MODE_ILLEGAL            3
#define FLASH_READ_MODE_MASK (FLASH_READ_MODE_ILLEGAL << FLASH_READ_MODE_SHIFT)
#define FLASH_READ_MODE_ECC_OFF_SHIFT      2

typedef enum _flash_read_modes
{
    FLASH_ReadModeNormal         = (FLASH_READ_MODE_NORMAL << FLASH_READ_MODE_SHIFT),
    FLASH_ReadModeNormalEccOff   = (FLASH_READ_MODE_NORMAL << FLASH_READ_MODE_SHIFT)|(1<<FLASH_READ_MODE_ECC_OFF_SHIFT),                /*!< flash operation is not successful*/
    FLASH_ReadModeDMACC          = (1<<FLASH_READ_MODE_RD_DMACC_SHIFT),
    FLASH_ReadModeMarginProgram  = (FLASH_READ_MODE_MARGIN_VS_PROGRAM << FLASH_READ_MODE_SHIFT),
    FLASH_ReadModeMarginErase    = (FLASH_READ_MODE_MARGIN_VS_ERASE << FLASH_READ_MODE_SHIFT),
} flash_read_mode_t;

/*! @brief Flash configuration information.
 *
 * An instance of this structure is allocated by the user of the flash driver and
 * at initialization.
 */
typedef struct _flash_config
{
    uint32_t PFlashBlockBase;  /*!< A base address of the first PFlash block */
    uint32_t PFlashTotalSize;  /*!< The size of the combined PFlash block. */
    uint32_t PFlashSectorSize; /*!< The size in bytes of a sector of PFlash. */
} flash_config_t;

extern flash_config_t gFlashConfig;

/**
 * @brief      Enable the FLASH
 *
 * @param      pFLASH Pointer to selected FLASHx peripheral
 *
 * @return     Nothing
 */
void FLASH_Init(FLASH_Type *pFLASH);

/**
 * @brief      Power down the FLASH
 *
 * @param      pFLASH Pointer to selected FLASHx peripheral
 *
 * @return     Nothing
 */
void FLASH_Powerdown(FLASH_Type *pFLASH);

/**
 * @brief      Wait for FLASH command to complete
 *
 * @param      pFLASH Pointer to selected FLASHx peripheral
 *
 * @return     INT_STATUS with ECC_ERR bit masked out
 */
int FLASH_Wait(FLASH_Type *pFLASH);

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
int FLASH_Erase(FLASH_Type *pFLASH, uint8_t *pu8Start, uint8_t *pu8End);

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
int FLASH_ErasePages(FLASH_Type *pFLASH, uint32_t u32StartPage, uint32_t u32PageCount);

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
int FLASH_BlankCheck(FLASH_Type *pFLASH, uint8_t *pu8Start, uint8_t *pu8End);

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
int FLASH_MarginCheck(FLASH_Type *pFLASH, uint8_t *pu8Start, uint8_t *pu8End);

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
int FLASH_Program(FLASH_Type *pFLASH, uint32_t *pu32Start, uint32_t *pu32Data, uint32_t u32Length);

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
int FLASH_Checksum(FLASH_Type *pFLASH, uint8_t *pu8Start, uint8_t *pu8End, uint32_t au32Checksum[4]);

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
int FLASH_Read(FLASH_Type *pFLASH, uint8_t *pu8Start, uint32_t u32ReadMode, uint32_t au32Data[4]);

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
void FLASH_SetReadMode(FLASH_Type *pFLASH, bool freq_48M_not_32M);

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
                            int init);

/**
 * @brief      Calculate checksum over page (N-2) aka CONFIG page and check it matches the expected value.
 *
 * @param[in]  page_buffer Pointer to data over which checksum calculation must be executed.
 * @param[out] misr Pointer on a four 32bit word array to store calculated checksum.
 *             Note: this buffer is only useful for debugging purposes.
 *
 * @return Result of the page checksum verification:
 *         -  0: Verify successfully.
 *         - -1: Verification failed.
 */
int FLASH_ConfigPageVerifyPageChecksum(const uint32_t *page_buffer,
                                      uint32_t *misr);

/**
 * @brief      Calculate checksum over GPO array of CONFIG page and check it matches the expected value
 *
 * @param[in]  page_buffer Pointer to data over which checksum calculation must be executed.
 * @param[out] misr Pointer on a 4 32bit word array to store calculated checksum.
 *             Note: this buffer is only useful for debugging purposes.
 *
 * @return Result of the GPO array checksum verification:
 *         -  0: Verify successfully.
 *         - -1: Verification failed.
 */
int FLASH_ConfigPageVerifyGpoChecksum(const uint32_t *page_buffer,
                                     uint32_t *misr);

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
                          uint32_t *page_chksum);

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
int FLASH_GetStatus(FLASH_Type *pFLASH);


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __FSL_FLASH_H_ */
