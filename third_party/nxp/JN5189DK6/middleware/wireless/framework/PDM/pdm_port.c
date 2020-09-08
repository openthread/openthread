/*****************************************************************************
 *
 * MODULE:
 *
 * COMPONENT:
 *
 * DESCRIPTION:
 *
 *****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5179].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2016. All rights reserved
 *
 ****************************************************************************/

/* Standard includes. */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#if defined FSL_RTOS_FREE_RTOS && (FSL_RTOS_FREE_RTOS != 0)
#include "FreeRTOSConfig.h"
#include "projdefs.h"
#include "portable.h"
#else
#include "MemManager.h"
#endif

#include "FunctionLib.h"
#include "fsl_debug_console.h"
#include "Flash_Adapter.h"
#include "PDM.h"

typedef struct
{
    uint8_t u8Level;
} tsMicroIntStorage;

#if defined(__GNUC__)
#define __WEAK_FUNC __attribute__((weak))
#elif defined(__ICCARM__)
#define __WEAK_FUNC __weak
#elif defined( __CC_ARM )
#define __WEAK_FUNC __weak
#endif

#if gUsePdm_d

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/


/* In this module pvHeap_Alloc, vHeap_Free, vHeap_ResetHeap,
 * vMicroIntEnableOnly, vMicroIntRestoreState
 * are default weak implementations that can be overridden
 * for USE_RTOS or DUAL_MODE_APP builds */


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Imported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


typedef enum
{
    E_HEAP_ALLOC = 0,
    E_HEAP_RESET,
    E_FUNCTION_MAX
} eFunctionId;



/* Nested interrupt control */
#ifndef MICRO_SET_PRIMASK_LEVEL
#define MICRO_SET_PRIMASK_LEVEL(A) __set_PRIMASK(A)
#endif
// read back PRIMASK status into u32Store variable then disable the interrupts
#ifndef MICRO_DISABLE_AND_SAVE_INTERRUPTS
#define MICRO_DISABLE_AND_SAVE_INTERRUPTS(u32Store) \
    u32Store = __get_PRIMASK(); __disable_irq();
#endif
#ifndef MICRO_GET_ACTIVE_INT_LEVEL
#define MICRO_GET_ACTIVE_INT_LEVEL  __get_BASEPRI
#endif
#ifndef MICRO_SET_ACTIVE_INT_LEVEL_MAX
#define MICRO_SET_ACTIVE_INT_LEVEL_MAX(A) __set_BASEPRI_MAX(A)
#endif
#ifndef MICRO_SET_ACTIVE_INT_LEVEL
#define MICRO_SET_ACTIVE_INT_LEVEL(A) __set_BASEPRI(A)
#endif

/****************************************************************************
 *
 * NAME:        pvHeap_Alloc
 *
 * DESCRIPTION:
 * Allocates a block of memory from the heap.
 *
 * RETURNS:
 * Pointer to block, or NULL if the heap didn't have enough space. If block
 * was already assigned, returns original value and doesn't take anything
 * from heap.
 *
 * NOTES:
 * If buffer has already been allocated, it is not cleared.
 * If it is a fresh allocation, it is cleared on request.
 *
 ****************************************************************************/
__WEAK_FUNC void *pvHeap_Alloc(void *pvPointer, uint32_t u32BytesNeeded, bool_t bClear)
{
    do {
        if (pvPointer != NULL) break;
#if defined FSL_RTOS_FREE_RTOS && (FSL_RTOS_FREE_RTOS != 0)
        pvPointer = pvPortMalloc(u32BytesNeeded);
#else
        pvPointer = MEM_BufferAllocWithId(u32BytesNeeded, gPdmMemPoolId_c, (void*)__get_LR());
#endif
        if (pvPointer == NULL) break;
        if (bClear)
        {
            FLib_MemSet(pvPointer, 0, u32BytesNeeded);
        }
    } while (0);
    return pvPointer;
}

/****************************************************************************
 *
 * NAME:        vHeap_Free
 *
 * DESCRIPTION:
 * Release a block of memory from the heap.
 *
 * RETURNS:
 * none
 * NOTES:
 *
 ****************************************************************************/
__WEAK_FUNC void vHeap_Free(void *pvPointer)
{
#if defined FSL_RTOS_FREE_RTOS && (FSL_RTOS_FREE_RTOS != 0)
    vPortFree(pvPointer);
#else
    memStatus_t status;
    status = MEM_BufferFree(pvPointer);
    if (status != MEM_SUCCESS_c)
    {
#ifdef DEBUG
        PRINTF("MemMngt error freeing %08lx code=%d\r\n", (uint32_t)pvPointer, status);
#endif
    }
#endif
}

__WEAK_FUNC void vHeap_ResetHeap(void)
{
}
///////////////////////////////////////////////////////////////////////////////////////////////////////

__WEAK_FUNC void vMicroIntEnableOnly(tsMicroIntStorage *int_storage, uint32_t u32EnableMask)
{
    uint32_t primask_lvl;
    MICRO_DISABLE_AND_SAVE_INTERRUPTS(primask_lvl);
    int_storage->u8Level = (uint8_t)MICRO_GET_ACTIVE_INT_LEVEL();
    MICRO_SET_ACTIVE_INT_LEVEL_MAX( 96 );
    MICRO_SET_PRIMASK_LEVEL(primask_lvl);
}

__WEAK_FUNC void vMicroIntRestoreState(tsMicroIntStorage * int_storage)
{
    MICRO_SET_ACTIVE_INT_LEVEL(int_storage->u8Level);
}
#endif

/****************************************************************************
 *
 * NAME:        PDM_Init
 *
 * DESCRIPTION:
 * Wrapper to call PDM Initialization with the right Flash configuration parameters
 *
 * Note: FLASH_Init must have been called beforehand in order to initialize gFlashConfig.
 *
 * RETURNS:
 * 0: if no error, negative value otherwise
 * NOTES:
 *
 ****************************************************************************/
int PDM_Init(void)
{
    int status = -1;

#if gUsePdm_d
    static bool pdm_init_done = false;

    do {
        if (pdm_init_done)
        {
            status = 0;
            break;
        }

        NV_Init(); /* used to setup the gFlashConfig.PFlashTotalSize value */

        PDM_teStatus st = PDM_E_STATUS_INTERNAL_ERROR;
        uint8_t *base = (uint8_t*)NV_STORAGE_END_ADDRESS;
        size_t len = (size_t)((uint32_t)NV_STORAGE_START_ADDRESS + 1 - (uint32_t)NV_STORAGE_END_ADDRESS);
#ifdef DEBUG
        uint8_t * flash_top = (uint8_t*)(gFlashConfig.PFlashTotalSize + gFlashConfig.PFlashBlockBase);
        uint8_t * flash_base = (uint8_t*)gFlashConfig.PFlashBlockBase;
        assert(base >= flash_base);
        assert(base + len <= flash_top);
        assert(len > FLASH_PAGE_SIZE*2);
#endif
#if !defined gPdmNbSegments || (gPdmNbSegments < NV_STORAGE_MAX_SECTORS)
#error "gPdmNbSegments should match NV_STORAGE_MAX_SECTORS"
#endif
        uint32_t sect_sz_log = Flib_Log2(gFlashConfig.PFlashSectorSize);
        st = PDM_eInitialise(ADDR2SEG(base, sect_sz_log),
                             SIZE2SEGNB(len, sect_sz_log), NULL);

        if (st != PDM_E_STATUS_OK)
        {
#ifdef DEBUG
            PRINTF("PDM/NVM misconfiguration\r\n");
#endif
            assert(st != PDM_E_STATUS_OK);
            status = -st;
            break;
        }
        pdm_init_done = true;
        status = 0;
    } while (0);
#endif
    return status;
}
/*-----------------------------------------------------------*/
