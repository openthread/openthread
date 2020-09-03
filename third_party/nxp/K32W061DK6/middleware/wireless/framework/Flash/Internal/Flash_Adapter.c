/*! *********************************************************************************
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
* All rights reserved.
*
* \file
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */


/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "Flash_Adapter.h"
#include "FunctionLib.h"
#include "fsl_os_abstraction.h"
#if gNvStorageIncluded_d
#include "NVM_Interface.h"
#endif
#include "Panic.h"
#include "fsl_debug_console.h"
/*****************************************************************************
 *****************************************************************************
 * Private macros
 *****************************************************************************
 *****************************************************************************/
#if (PGM_SIZE_BYTE == 4)
#define mProgBuffSizeInPgmWrUnits_c  16
#elif (PGM_SIZE_BYTE == 8)
#define mProgBuffSizeInPgmWrUnits_c  8
#else
#define mProgBuffSizeInPgmWrUnits_c  4
#endif

/* Generator for CRC calculations. */
#define POLGEN  0x1021

#ifdef CPU_JN518X
#include "rom_psector.h"
void ResetMCU(void);
#endif
/*! *********************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Private prototypes
*************************************************************************************
********************************************************************************** */
static uint32_t NV_FlashProgramAdaptation(uint32_t dest, uint32_t size, uint8_t* pData);
#if !defined CPU_QN908X && ! defined CPU_JN518X
static uint8_t  NV_VerifyCrcOverHWParameters(hardwareParameters_t* pHwParams);
static uint16_t NV_ComputeCrcOverHWParameters(hardwareParameters_t* pHwParams);
#endif
static void NV_Flash_WaitForCSEndAndDisableInterrupts(void);
#ifdef CPU_QN908X
static uint32_t SwFlashVerifyErase (uint32_t start, uint32_t lengthInBytes);
#endif

/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */

/* Hardware parameters */
hardwareParameters_t gHardwareParameters;
extern flash_config_t gFlashConfig;


#if !defined CPU_QN908X && !defined CPU_JN518X
static const uint8_t mProdDataIdentifier[10] = {"PROD_DATA:"};
#endif
#if (USE_RTOS)
static osaSemaphoreId_t       mFlashAdapterSemaphoreId;
#endif
static volatile uint8_t mFA_CSFlag = 0;
static volatile uint8_t mFA_SemWaitCount = 0;
/*****************************************************************************
 *****************************************************************************
 * Private functions
 *****************************************************************************
 *****************************************************************************/
/*! *********************************************************************************
 * \brief This function ensures that there are no critical sections on and disables interrupts.
 *
 * \param[in] none
 * \return nothing
 *
********************************************************************************** */
static void NV_Flash_WaitForCSEndAndDisableInterrupts(void)
{
#if (USE_RTOS)
#if (gNvStorageIncluded_d)
    /*on freeRTOS NvIdle runs on a freeRTOS idle task hook which must never block. In this RTOS at least one task must be ready */
    /*since NvIdle runs on the task which has the least priority in the system we can wait in a loop for the end of the critical section */
    osaTaskId_t currentTaskId = OSA_TaskGetId();
    if(currentTaskId == (osaTaskId_t)NvGetNvIdleTaskId())
    {
        while(1)
        {
            while (mFA_CSFlag);
            OSA_InterruptDisable();
            if(mFA_CSFlag == 0)
            {
                break;
            }
            OSA_InterruptEnable();
        }
    }
    else
#endif
    {
        while(1)
        {
            OSA_InterruptDisable();
            if(mFA_CSFlag == 0)
            {
                break;
            }
            mFA_SemWaitCount++;
            OSA_InterruptEnable();
            OSA_SemaphoreWait(mFlashAdapterSemaphoreId, osaWaitForever_c);
        }
    }
#else
    OSA_InterruptDisable();
#endif
}
/*! *********************************************************************************
 * \brief  Write aligned data to FLASH
 *
 * \param[in] dest        The address of the Flash location
 * \param[in] size        The number of bytes to be programed
 * \param[in] pData       Pointer to the data to be programmed to Flash
 *
 * \return error code
 *
********************************************************************************** */
static uint32_t NV_FlashProgramAdaptation(uint32_t dest, uint32_t size, uint8_t* pData)
{
    uint32_t progBuf[PGM_SIZE_BYTE/sizeof(uint32_t)];
    uint32_t status = kStatus_FLASH_Success;
#if !defined (CPU_JN518X)
    if( (size & (PGM_SIZE_BYTE - 0x01U)) != 0 )
    {
        return kStatus_FLASH_AlignmentError;
    }
#if gFlashEraseDuringWrite
    {
        NV_FlashEraseSector(dest, size-1);
    }
#endif
#else
    if( (size & (PGM_SIZE_BYTE - 0x01U)) != 0 )
    {
        return kStatus_FLASH_AlignmentError;
    }
    if( (dest & MASK_LOG(PGM_SIZE_BYTE_LOG)) != 0 )
    {
        PRINTF("Unaligned flash address %08lx", dest);
        return kStatus_FLASH_AlignmentError;
    }

    NV_FlashEraseSector(dest, size-1);
#endif


    while(size)
    {
        FLib_MemCpy(progBuf, pData, PGM_SIZE_BYTE);

#if gNvDisableIntCmdSeq_c
        NV_Flash_WaitForCSEndAndDisableInterrupts();
#endif
        status = FLASH_PROGRAM_AREA(dest, progBuf, PGM_SIZE_BYTE);

#if gNvDisableIntCmdSeq_c
        OSA_InterruptEnable();
#endif

        if(status != kStatus_FLASH_Success)
        {
            break;
        }

        pData += PGM_SIZE_BYTE;
        dest += PGM_SIZE_BYTE;
        size -= PGM_SIZE_BYTE;
    }

    return status;
}

#if !defined CPU_QN908X && ! defined CPU_JN518X
/*! *********************************************************************************
 * \brief  Verifies if the CRC field matches computed CRC over stored values
 *
 * \param[in] pHwParams  pointer to a structure containing HW parameters
 *
 * \return 1 on success, 0 otherwise
 *
********************************************************************************** */
static uint8_t NV_VerifyCrcOverHWParameters(hardwareParameters_t* pHwParams)
{
    uint8_t status = 0;

    if(NV_ComputeCrcOverHWParameters(pHwParams) == pHwParams->hardwareParamsCrc)
    {
        status = 1;
    }
    else
    {
        status = 0;
    }
    return status;
}

/*! *********************************************************************************
 * \brief  Computes the CRC for the hardware parameters and stores it
 *
 * \param[in] pHwParams  pointer to a structure containing HW parameters
 *
 * \return Computed CRC value.
 *
********************************************************************************** */
static uint16_t NV_ComputeCrcOverHWParameters(hardwareParameters_t* pHwParams)
{
    uint16_t  computedCRC = 0;
    uint8_t crcA;
    uint8_t byte = 0;
    if(NULL != pHwParams)
    {
        uint8_t *ptr = (uint8_t *)(&pHwParams->reserved);
        uint16_t len = (uint8_t *)(&pHwParams->hardwareParamsCrc) -
                           (uint8_t *)(&pHwParams->reserved);
        while(len)
        {
            byte = *ptr;
            computedCRC ^= ((uint16_t)byte << 8);
            for(crcA = 8; crcA; crcA--)
            {
                if(computedCRC & 0x8000) {
                    computedCRC <<= 1;
                    computedCRC ^= POLGEN;
                }
                else computedCRC <<= 1;
            }
            --len;
            ++ptr;
        }
    }
    return computedCRC;
}
#endif

#ifdef CPU_QN908X
/*! *********************************************************************************
 * \brief  Flash verify erase software implementation
 *
 * \param[in] start - start address of the memory block to be verified
 * \param[in] lengthInBytes - memory block length
 *
 * \return TRUE if the flash memory area is blank (erased), FALSE otherwise
 *
********************************************************************************** */
uint32_t SwFlashVerifyErase (uint32_t start, uint32_t lengthInBytes)
{
    uint8_t* pAddress = (uint8_t*)start;
    flash_status_t status = kStatus_FLASH_Success;

    do
    {
        if(*pAddress++ != 0xff)
        {
            status = kStatus_FLASH_EraseError;
            break;
        }
    } while(--lengthInBytes);
    
    return (uint32_t)status;
}
#endif
#ifdef CPU_JN518X
#define FLASH_WORD_SZ 16

/* This is kept as an example of code using the TRY .. CATCH for exceptions
 * Using it raises some execution issues with a debugger
*/
uint32_t NV_ReadSweep(uint32_t start_addr, uint32_t end_addr)
{
    uint32_t addr = start_addr;
    TRY
    {
        while (addr < end_addr)
        {
            volatile uint32_t read_val;
            read_val = *(uint32_t*)addr;
            read_val = read_val;
            addr += FLASH_WORD_SZ;
        }
    }
    CATCH (BUS_EXCEPTION)
    {
        /* return page address if an error is detected */
        return ROUND_FLOOR(addr, 9);
    }
    YRT;
    return 0;
}


uint32_t NV_SafeReadFromFlash(uint8_t * ram_dst, uint8_t * flash_src, size_t size)
{
    uint32_t au32Data[FLASH_WORD_SZ/sizeof(uint32_t)];
    uint32_t st = kStatus_FLASH_Fail;
    uint32_t nb_flash_words = (size + (FLASH_WORD_SZ-1)) / FLASH_WORD_SZ;
    while (nb_flash_words-- > 0)
    {
        size_t sz;
        sz = (size >= FLASH_WORD_SZ) ? FLASH_WORD_SZ : size;
        st = FLASH_Read(FLASH, flash_src, 0, au32Data);
        if (st != kStatus_FLASH_Success) break;
        memcpy(ram_dst, (uint8_t*)&au32Data[0], sz);
        ram_dst += sz;
        flash_src +=  sz;
        size -=  sz;
    }
    return st;
}
void NV_FlashPerformAudit(void)
{
    int status;

    uint32_t addr = INT_STORAGE_START_OFFSET;
    if (INT_STORAGE_END_OFFSET == INT_STORAGE_START_OFFSET) addr++;
    uint32_t end =  (uint32_t)0x9ddff;

    uint32_t  failed_addr;
    uint8_t buf[FLASH_PAGE_SIZE];

    while (addr <= ROUND_FLOOR(end, 9))
    {
        status = FLASH_BlankCheck(FLASH, (uint8_t*)addr, (uint8_t*)end);
        if (kStatus_FLASH_Fail == status)
        {
            failed_addr = FLASH->DATAW[0]; /* is a flash word address */
            failed_addr <<= 4;
            addr = ROUND_FLOOR(failed_addr, 9);
            status =  NV_SafeReadFromFlash(buf, (uint8_t*)addr, FLASH_PAGE_SIZE);
            if (kStatus_FLASH_Success != status)
            {
                status =  FLASH_Erase(FLASH,
                            (uint8_t*)addr,
                            (uint8_t*)(addr+FLASH_PAGE_SIZE-1));
                if (status != kStatus_FLASH_Success)
                {
                    PRINTF("NV Audit Erase failed at addr=%08lx\r\n", addr);
                    break;
                }

            }
            addr += FLASH_PAGE_SIZE;
        }
        else
        {
            addr = end;
        }
    }
}
#endif



/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */
#ifdef CPU_JN518X
int PsectorUpdateFlashAudit(uint32_t new_value)
{
	int res = 1;
	psector_page_data_t page_buf;
	psector_page_data_t  *page0 = (psector_page_data_t*)0x9e800;

	memcpy(&page_buf, page0,  FLASH_PAGE_SIZE);

	((psector_page_data_t*)&page_buf)->page0_v3.flash_audit_done = new_value;
	page_buf.hdr.version ++;
	page_buf.hdr.checksum = psector_CalculateChecksum((psector_page_t *)&page_buf);
	if (psector_WriteUpdatePage(PSECTOR_PAGE0_PART, (psector_page_t *)&page_buf) != WRITE_OK)
	{
		res = -1;
	}
	else
	{
		ResetMCU(); /* Commit now by forcing reset */
	}
	return res;
}

int psector_FlashAudit(void)
{
	int res = -1;
	do {
		psector_page_data_t  *page0 = (psector_page_data_t*)0x9e800;
		uint32_t flash_audit_word = page0->page0_v3.flash_audit_done;

		if ((flash_audit_word & 0xffff) == FLASH_AUDIT_DONE)
		{
			res = 0;
			break;
		}
		else
		{
			/* Need to verify internal flash */
			NV_FlashPerformAudit();
			uint32_t flash_audit_code = (flash_audit_word & 0xffff0000u) | FLASH_AUDIT_DONE;
			res = PsectorUpdateFlashAudit(flash_audit_code);
			if (res < 0)
				break;
		}

	} while (0);

	return res;
}
#endif

/*! *********************************************************************************
 * \brief  Initialize the FLASH driver
 *
********************************************************************************** */
void NV_Init(void)
{
    static bool_t nvmInit = 0;
    if(nvmInit == 0)
    {
#ifdef CPU_QN908X
        /* Configure Flash  */
        FLASH_GetDefaultConfig(&gFlashConfig);
        gFlashConfig.blockBase = 0x0U;
#endif /* CPU_QN908X */
#ifdef gNVM_MULTICORE_SUPPORT_d
        FLASH_SetProperty(&gFlashConfig, kFLASH_PropertyFlashMemoryIndex, 1);
#endif
        /* Init Flash */
        FLASH_INIT();
#if (defined(CPU_MKW36Z512VFP4) || defined(CPU_MKW36Z512VHT4))
        /* KW36 has 256KB of FlexNVM mapped at adress 0x1000 0000 which also has an alias starting from address 0x0004 0000.
         * Configured the Flash driver to treat the PFLASH bloxk and FlexNVM block as a continuous memory block. */
        gFlashConfig.DFlashBlockBase = FSL_FEATURE_FLASH_PFLASH_BLOCK_SIZE * FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT;
#endif
#if (USE_RTOS)
        if( (mFlashAdapterSemaphoreId = OSA_SemaphoreCreate(0)) == NULL )
        {
            panic( ID_PANIC(0,0), (uint32_t)NV_Init, 0, 0 );
        }
#endif

#ifdef CPU_JN518X
        if (!nvmInit)
        {
        	if (psector_FlashAudit() == 0)
                nvmInit = 1;
        }
#else
         nvmInit = 1;
#endif
    }
}
/******************************************************************************
 * Name: NvSetCriticalSection
 * Description: enter critical section
 * Parameters: -
 * Return: -
 ******************************************************************************/
/*! *********************************************************************************
 * \brief  Start a critical section during which flash operations are not allowed
 *
 * \param[in] none
 * \return nothing
 *
********************************************************************************** */
void NV_Flash_SetCriticalSection
(
void
)
{
#if (USE_RTOS)
    OSA_InterruptDisable();
    ++mFA_CSFlag;
    OSA_InterruptEnable();
#endif
}

/*! *********************************************************************************
 * \brief  Start a critical section during which flash operations are not allowed
 *
 * \param[in] none
 * \return nothing
 *
********************************************************************************** */
void NV_Flash_ClearCriticalSection
(
void
)
{
#if (USE_RTOS)
    OSA_InterruptDisable();
    if(mFA_CSFlag)
    {
        mFA_CSFlag--;
    }
    OSA_InterruptEnable();
    while(1)
    {
        OSA_InterruptDisable();

        if(mFA_CSFlag)
        {
            break;
        }
        if(mFA_SemWaitCount == 0)
        {
            break;
        }
        mFA_SemWaitCount--;
        OSA_InterruptEnable();
        OSA_SemaphorePost(mFlashAdapterSemaphoreId);
    }
    OSA_InterruptEnable();
#endif
}

/*! *********************************************************************************
 * \brief  Verify erase data in Flash
 *
 * \param[in] start                The address of the Flash location
 * \param[in] lengthInBytes        The number of bytes to be checked
 * \param[in] margin
  * \return error code
 *
********************************************************************************** */
uint32_t NV_FlashVerifyErase ( uint32_t start, uint32_t lengthInBytes)
{
  uint32_t status;
#if gNvDisableIntCmdSeq_c
    NV_Flash_WaitForCSEndAndDisableInterrupts();
#endif

    status = FLASH_VERIFY_ERASE(start, lengthInBytes);

#if gNvDisableIntCmdSeq_c
    OSA_InterruptEnable();
#endif
  return status;
}
/*! *********************************************************************************
 * \brief  Write aligned data to FLASH
 *
 * \param[in] pSSDConfig  Pointer to a flash config structure
 * \param[in] dest        The address of the Flash location
 * \param[in] size        The number of bytes to be programed
 * \param[in] pData       Pointer to the data to be programmed to Flash
 * \param[in] pFlashCommandSequence  Pointer to the Flash RAM function
 *
 * \return error code
 *
********************************************************************************** */
uint32_t NV_FlashProgram(uint32_t dest,
                           uint32_t size,
                           uint8_t* pData)
{
    return NV_FlashProgramAdaptation(dest, size, pData);
}

/*! *********************************************************************************
 * \brief  Write data to FLASH
 *
 * \param[in] pSSDConfig  Pointer to a flash config structure
 * \param[in] dest        The address of the Flash location
 * \param[in] size        The number of bytes to be programed
 * \param[in] pData       Pointer to the data to be programmed to Flash
 * \param[in] pFlashCommandSequence  Pointer to the Flash RAM function
 *
 * \return error code
 *
********************************************************************************** */
uint32_t NV_FlashProgramUnaligned(uint32_t dest,
                                     uint32_t size,
                                     uint8_t* pData)
{
    uint8_t  buffer[PGM_SIZE_BYTE];
    uint32_t status;
    do {
        uint32_t align_addr = ROUND_FLOOR(dest, PGM_SIZE_BYTE_LOG);
        uint16_t bytes = (dest - align_addr);

        if( bytes )
        {
            uint16_t unalignedBytes = PGM_SIZE_BYTE - bytes;

            if( unalignedBytes > size )
            {
                unalignedBytes = size;
            }

            if (FLib_CopyFromFlash(buffer, (void*)align_addr, PGM_SIZE_BYTE))
            {
                PRINTF("Raised error while reading from %08lx\r\n", align_addr);
            }


            FLib_MemCpy(&buffer[bytes], pData, unalignedBytes);

            status = NV_FlashProgramAdaptation(align_addr, PGM_SIZE_BYTE, buffer);

            if(status != kStatus_FLASH_Success) break;

            dest += unalignedBytes;
            /* if size is less than the distance to the end of program block
               unalignedBytes has been shrunk , after size is decremented it will become 0 */
            pData += unalignedBytes;
            size -= unalignedBytes;
        }

        bytes = size & ~(PGM_SIZE_BYTE - 1U);

        /* Now dest is on an aligned boundary */
        /* bytes is an integer number of program blocks (pages) */
        if( bytes )
        {
            status = NV_FlashProgramAdaptation(dest, bytes, pData);

            if(status != kStatus_FLASH_Success) break;

            dest  += bytes;
            pData += bytes;
            size  -= bytes;
        }

        /* dest is still aligned because we have increased it by a multiple of the program block (page) */
        if( size )
        {
            if (
#ifdef CPU_JN518X
                /* Avoid reading page if it is blank */
                (FLASH_VERIFY_ERASE(dest,  PGM_SIZE_BYTE) == kStatus_FLASH_Fail) &&
#endif
                (FLib_CopyFromFlash(buffer, (void*)dest, PGM_SIZE_BYTE))
               )
            {
                PRINTF("Raised error while reading from %08lx\r\n", (dest - bytes));
            }

            FLib_MemCpy(buffer, pData, size);
            status = NV_FlashProgramAdaptation(dest, PGM_SIZE_BYTE, buffer);

            if(status != kStatus_FLASH_Success) break;
        }
        status = kStatus_FLASH_Success;

    } while (0);

    return status;
}

/*! *********************************************************************************
 * \brief  Erase to 0xFF one ore more FLASH sectors.
 *
 * \param[in] pSSDConfig  Pointer to a flash config structure
 * \param[in] dest        The start address of the first sector to be erased
 * \param[in] size        The amount of flash to be erased (multiple of sector size)
 * \param[in] pFlashCommandSequence  Pointer to the Flash RAM function
 *
 * \return error code
 *
********************************************************************************** */
uint32_t NV_FlashEraseSector(uint32_t dest, uint32_t size)
{
    uint32_t status;
#ifdef CPU_QN908X
    uint32_t status_flags;
#endif

#if gNvDisableIntCmdSeq_c
    NV_Flash_WaitForCSEndAndDisableInterrupts();
#endif
#if defined(CPU_QN908X)
    status_flags = FLASH_GetStatusFlags();
    if(status_flags & FLASH_INT_STAT_AHBL_INT_MASK)
    {
        FLASH_ClearStatusFlags(FLASH_INTCLR_AHBL_INTCLR_MASK);
    }
    if(status_flags & FLASH_INT_STAT_AHBH_INT_MASK)
    {
        FLASH_ClearStatusFlags(FLASH_INTCLR_AHBH_INTCLR_MASK);
    }
#endif
    status = FLASH_ERASE_AREA(dest, size);

#if gNvDisableIntCmdSeq_c
    OSA_InterruptEnable();
#endif
    return status;
}

#if !defined CPU_QN908X && !defined CPU_JN518X
/*! *********************************************************************************
 * \brief  Load the HW parameters from Flash to RAM
 *
 * \param[in] pHwParams  pointer to a structure where HW parameters will be stored
 *
 * \return error code
 *
********************************************************************************** */
uint32_t NV_ReadHWParameters(hardwareParameters_t *pHwParams)
{
    hardwareParameters_t* pLocalParams = (hardwareParameters_t*)FREESCALE_PROD_DATA_BASE_ADDR;
    uint32_t status = 0;
    do {

        if(NULL == pHwParams)
        {
            /* Invalid parameter */
            status = 2;
            break;
        }
#ifdef CPU_JN518X
   #if 0
        if (NV_SafeReadFromFlash((uint8_t*)pHwParams,
                                   (uint8_t*)pLocalParams,
                                   sizeof(hardwareParameters_t)) == kStatus_FLASH_Success)
        {
            if (FLib_MemCmp(pHwParams,
                            (void*)mProdDataIdentifier,
                            sizeof(mProdDataIdentifier)) &&
                NV_VerifyCrcOverHWParameters(pHwParams))
            {
                status = 0;
                break;
            }
        }
        else
        {
            NV_FlashEraseSector((uint32_t)pLocalParams, FLASH_PAGE_SIZE-1);
            FLib_MemSet(pHwParams, 0xFF, sizeof(hardwareParameters_t));
            status = 1;
        }
    #endif
#else
        if (FLib_MemCmp(FREESCALE_PROD_DATA_BASE_ADDR, (void*)mProdDataIdentifier, sizeof(mProdDataIdentifier)) &&
           NV_VerifyCrcOverHWParameters(pLocalParams))
        {
            FLib_MemCpy(pHwParams, FREESCALE_PROD_DATA_BASE_ADDR, sizeof(hardwareParameters_t));
            status = 0;
        }
        else
        {
            FLib_MemSet(pHwParams, 0xFF, sizeof(hardwareParameters_t));
            status = 1;
            break;
        }
 #endif
    } while (0);

    return status;
}
/*! *********************************************************************************
 * \brief  Store the HW parameters to Flash
 *
 * \param[in] pHwParams  pointer to a structure containing HW parameters
 *
 * \return error code of the Flash erase/write functions
 *
********************************************************************************** */
uint32_t NV_WriteHWParameters(hardwareParameters_t *pHwParams)
{
    uint32_t status = 0;
    NV_Init();

#ifndef CPU_JN518X
#ifdef CPU_JN518X

    status = FLASH_BlankCheck(FLASH, (uint8_t *)FREESCALE_PROD_DATA_BASE_ADDR,
                        (uint8_t *)(FREESCALE_PROD_DATA_BASE_ADDR + sizeof(hardwareParameters_t)));
#endif

    if(
#ifdef CPU_JN518X
       (status == kStatus_FLASH_Success) ||
#endif
       !FLib_MemCmp(pHwParams, (void*)FREESCALE_PROD_DATA_BASE_ADDR, sizeof(hardwareParameters_t)))
    {
        pHwParams->hardwareParamsCrc = NV_ComputeCrcOverHWParameters(pHwParams);
        FLib_MemCpy(pHwParams->identificationWord, (void*)mProdDataIdentifier, sizeof(mProdDataIdentifier));

#if defined(FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE)
        status = NV_FlashEraseSector((uint32_t)FREESCALE_PROD_DATA_BASE_ADDR, FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE);
#elif defined(FSL_FEATURE_FLASH_PAGE_SIZE_BYTES)
        status = NV_FlashEraseSector((uint32_t)FREESCALE_PROD_DATA_BASE_ADDR, FSL_FEATURE_FLASH_PAGE_SIZE_BYTES);
#endif

        if( kStatus_FLASH_Success == status )
        {
            status = NV_FlashProgramUnaligned((uint32_t)FREESCALE_PROD_DATA_BASE_ADDR,
                                              sizeof(hardwareParameters_t),
                                              (uint8_t*)pHwParams);
        }
    }
#endif
    return status;
}
#endif /* neither CPU_QN908X not CPU_JN518X */
