/*! *********************************************************************************
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017, 2019 NXP
* All rights reserved.
*
* \file
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

#ifndef __FLASH_ADAPTER_H__
#define __FLASH_ADAPTER_H__

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "EmbeddedTypes.h"
#include "fsl_flash.h"


/*! *********************************************************************************
*************************************************************************************
* Public macros
*************************************************************************************
********************************************************************************** */
/*
 * Name: gNvDisableIntCmdSeq_c
 * Description: this macro is used to enable/disable interrupts when the
 *              FTFL controller executes a command sequence. This macro
 *              has to be set according to NVM configuration. Therefore,
 *              if the FLASH region used by the NVM is placed in the same
 *              program block as the ISR's executable code, the interrupts
 *              MUST be disabled (because no code pre-fetching can be performed
 *              while FTFL controller executes a command sequence, i.e.
 *              program/erase). If the interrupts are not disabled, the
 *              system will assert a hard fault when the flash controller
 *              executes a command sequnce and an IRQ is about to be handled.
 *              If the NVM region is placed in a different program block than
 *              the ISR's code, this macro shall be set to FALSE (recommended).
 */
#ifndef gNvDisableIntCmdSeq_c
#define gNvDisableIntCmdSeq_c           (1)
#endif

/* size of array to copy__Launch_Command function to.*/
/* It should be at least equal to actual size of __Launch_Command func */
/* User can change this value based on RAM size availability and actual size of __Launch_Command function */
#define LAUNCH_CMD_SIZE         64
#define PGM_SIZE_BYTE           FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE
#define P_BLOCK_NUM             FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT
#define P_SECTOR_SIZE           FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE
/* Data Flash block information */
#define FLEXNVM_BASE            FSL_FEATURE_FLASH_FLEX_NVM_START_ADDRESS
#define FLEXNVM_SECTOR_SIZE     FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_SECTOR_SIZE
#define FLEXNVM_BLOCK_SIZE      FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_SIZE
#define FLEXNVM_BLOCK_NUM       FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_COUNT
/* Other defines */
#define DEBUGENABLE             0x00
#define FTFx_REG_BASE           0x40020000
#define P_FLASH_BASE            0x00000000

/* Flex Ram block information */
#define EERAM_BASE              FSL_FEATURE_FLASH_FLEX_RAM_START_ADDRESS
#define EERAM_SIZE              FSL_FEATURE_FLASH_FLEX_RAM_SIZE

#define READ_NORMAL_MARGIN        0x00
#define READ_USER_MARGIN          0x01
#define READ_FACTORY_MARGIN       0x02



#define NV_FlashRead(pSrc, pDest, size) FLib_MemCpy((void*)(pDest), (void*)(pSrc), size);

#if defined(CPU_QN908X)
#define kStatus_FLASH_AlignmentError  MAKE_STATUS(kStatusGroup_FLASH, 6)
#elif defined(CPU_JN518X)

#define FLASH_AUDIT_DONE 0xc65c
int PsectorUpdateFlashAudit(uint32_t new_value);


#endif

#define PGM_SIZE_BYTE_LOG LOG(PGM_SIZE_BYTE)
#define FLASH_PAGE_SZ_LOG LOG(FLASH_PAGE_SIZE)
#define IS_MULTIPLE_OF_SECT_SIZE(len) (((len) & (FLASH_PAGE_SIZE-1)) == 0)
#define SIZE2SEGNB(sz, sz_log) ((sz)>>(sz_log))
#define ADDR2SEG(addr, sz_log) (((uint32_t)(addr))>>(sz_log))

#if defined(__CC_ARM)

  extern uint32_t Image$$INT_STORAGE$$Base[];
  extern uint32_t Image$$INT_STORAGE$$Length[];
  extern uint32_t Image$$INT_STORAGE$$End[];

  #define INT_STORAGE_START_OFFSET          ((uint32_t)Image$$INT_STORAGE$$Base)
  #define INT_STORAGE_TOTAL_SIZE            ((uint32_t)Image$$INT_STORAGE$$Length)
  #define INT_STORAGE_END_OFFSET            ((uint32_t)Image$$INT_STORAGE$$End)

#else /* defined(__CC_ARM) */
extern uint32_t INT_STORAGE_START[];
extern uint32_t INT_STORAGE_SIZE[];
extern uint32_t INT_STORAGE_END[];
extern uint32_t INT_STORAGE_SECTOR_SIZE[];
#define INT_STORAGE_START_OFFSET        ((uint32_t)INT_STORAGE_END)
#define INT_STORAGE_END_OFFSET          ((uint32_t)INT_STORAGE_START)
#define INT_STORAGE_TOTAL_SIZE          ((uint32_t)INT_STORAGE_SIZE)

  /*
   * Name: NV_STORAGE_END_ADDRESS
   * Description: NV_STORAGE_END_ADDRESS from linker command file is used by this code
   *              as Raw Sector Start Address.
   */
extern uint32_t NV_STORAGE_END_ADDRESS[];

  /*
   * Name: NV_STORAGE_SECTOR_SIZE
   * Description: external symbol from linker command file, it represents the size
   *              of a FLASH sector
   */
extern uint32_t NV_STORAGE_SECTOR_SIZE[];

  /*
   * Name:  NV_STORAGE_MAX_SECTORS
   * Description: external symbol from linker command file, it represents the sectors
   *              count used by the ENVM storage system; it has to be a multiple of 2
   */
extern uint32_t NV_STORAGE_MAX_SECTORS[];

  /*
     * Name: NV_STORAGE_START_ADDRESS
     * Description: NV_STORAGE_START_ADDRESS from linker command file is used by this code
     *              as Raw Sector End Address. Start is End address
     */
extern uint32_t NV_STORAGE_START_ADDRESS[];

#endif /* defined(__CC_ARM) */

/* Flash address of the Product Data sector */
#if defined(__CC_ARM)
extern uint32_t Load$$LR$$LR_Prod_Data$$Base[];
#define FREESCALE_PROD_DATA_BASE_ADDR Load$$LR$$LR_Prod_Data$$Base
#else
extern uint32_t FREESCALE_PROD_DATA_BASE_ADDR[];
#endif /* defined(__CC_ARM) */

/*! *********************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
********************************************************************************** */
/*! GCC would warn about packing, so add pragma to repress warning */
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wattributes"
#endif
typedef PACKED_STRUCT hardwareParameters_tag
{
    uint8_t  identificationWord[10];   /* valid data present */
#ifdef CPU_JN518X
    uint8_t  flash_audit_done;
    uint8_t  reserved[3];
#else
    uint8_t  reserved[32];             /* for backward compatibillity */
    uint8_t  ieee_802_15_4_address[8]; /* IEEE 802.15.4 MAC address */
    uint8_t  bluetooth_address[6];     /* Bluetooth address */
    uint32_t xtalTrim;                 /* KW4x only */
    uint32_t edCalibrationOffset;      /* KW01 ED offset */
    uint32_t pllFStepOffset;           /* KW01 fine tune pll */
#endif
    uint32_t gInternalStorageAddr;     /* The start address of the internal storage used for OTA update.
                                          A value of 0xFFFFFFFF means that the External storage is used.
                                          Warning: The offset to this field in respect to the start address of the structure
                                          must not be changed.*/
    /* For forward compatibility additional fields may be added here
       Existing data in flash will not be compatible after modifying the hardwareParameters_t typedef*/
    uint16_t hardwareParamsCrc;        /* crc for data between start of reserved area and start of hardwareParamsCrc field (not included). */
}hardwareParameters_t;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */
extern flash_config_t gFlashConfig;

extern hardwareParameters_t gHardwareParameters;

/*! *********************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
********************************************************************************** */
void NV_Init(void);

uint32_t NV_FlashProgram(         uint32_t dest,
                                  uint32_t size,
                                  uint8_t* pData);

uint32_t NV_FlashProgramUnaligned(uint32_t dest,
                                  uint32_t size,
                                  uint8_t* pData);

uint32_t NV_FlashEraseSector(uint32_t dest,
                             uint32_t size);

#if defined(CPU_JN518X)
#define FLASH_ERASE_AREA(ADDR, SZ) FLASH_Erase(FLASH, (uint8_t *)(ADDR), (uint8_t *)((ADDR) + (SZ) - 1))
#define FLASH_PROGRAM_AREA(ADDR, SRC, SZ) FLASH_Program(FLASH, (uint32_t *)(ADDR), (uint32_t*)(SRC), (SZ))
#define FLASH_INIT() FLASH_Init(FLASH)
#define FLASH_VERIFY_ERASE(ADDR, SZ) FLASH_BlankCheck(FLASH, (uint8_t *)(ADDR), (uint8_t *)((ADDR) + (SZ) - 1))
#elif defined(CPU_QN908X)
#define FLASH_VERIFY_ERASE(ADDR, SZ)  SwFlashVerifyErase((ADDR), (SZ), ,kFLASH_MarginValueNormal)
#define FLASH_ERASE_AREA(ADDR, SZ) FLASH_Erase(&gFlashConfig, (ADDR), (SZ))
#define FLASH_PROGRAM_AREA(ADDR, SRC, SZ) FLASH_Program(&gFlashConfig, (uint32_t *)(ADDR), (uint32_t*)(SRC), (SZ))
#define FLASH_INIT() FLASH_Init(&gFlashConfig)
#define FLASH_VERIFY_ERASE(ADDR, SZ) SwFlashVerifyErase ((ADDR), (SZ))
#else
#define FLASH_VERIFY_ERASE(ADDR, SZ)  NV_FlashVerifyErase(ADDR, SZ)
#define FLASH_ERASE_AREA, SZ) FLASH_Erase(&gFlashConfig, (ADDR), (SZ), kFLASH_ApiEraseKey)
#define FLASH_PROGRAM_AREA(ADDR, SRC, SZ) FLASH_Program(&gFlashConfig, (uint32_t *)(ADDR), (uint32_t*)(SRC), (SZ))
#define FLASH_INIT() FLASH_Init(&gFlashConfig)
#define FLASH_VERIFY_ERASE(ADDR, SZ) FLASH_VerifyErase(&gFlashConfig, (ADDR), (SZ), kFLASH_MarginValueNormal)
#endif


uint32_t NV_ReadHWParameters(hardwareParameters_t *pHwParams);

uint32_t NV_WriteHWParameters(hardwareParameters_t *pHwParams);

void NV_Flash_SetCriticalSection(void);
void NV_Flash_ClearCriticalSection(void);

#endif /* __FLASH_ADAPTER_H__ */
