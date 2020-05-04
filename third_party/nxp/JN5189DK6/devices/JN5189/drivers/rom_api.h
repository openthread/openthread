/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ROM_API_H_
#define ROM_API_H_

#if defined __cplusplus
extern "C" {
#endif

/*!
 * @addtogroup ROM_API
 * @{
 */

/*! @file */

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <stdint.h>
#include "rom_common.h"
#include "rom_psector.h"
#include "flash_header.h"
#include "rom_aes.h"
#include "rom_efuse.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.jn_romapi"
#endif
/*! @name Driver version */
/*@{*/
/*! @brief JN_ROMAPI driver version 2.0.0. */
#define FSL_JN_ROMAPI_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/*!
 * @brief Convert logical address into physical address, based on SYSCOM MEMORYREMAP register
 *
 * The chip has a remapping capability that allows to remap internal flash areas.
 * This feature is part of the firmware update mechanism (OTA).
 *
 * @param  address logical address to convert
 *
 * @return  physical address
 *
 */
static inline uint32_t BOOT_RemapAddress(uint32_t address)
{
    uint32_t (*p_BOOT_RemapAddress)(uint32_t address);
    p_BOOT_RemapAddress = (uint32_t(*)(uint32_t address))0x03000dc9U;

    return p_BOOT_RemapAddress(address);
}

/*! @brief IMAGE_DATA_T image node : element of single link chained list of images found in flash */
typedef struct _image_data_t
{
    uint32_t version;           /*!< version number found in image  */
    uint32_t address;           /*!< start address of image */
    struct _image_data_t *next; /*!< pointer on next IMAGE_DATA_T in list */
} IMAGE_DATA_T;

/*! @brief IMAGE_VERIFY_T function pointer : verification function  e.g. boot_Verify_eScoreImageList */
typedef uint32_t (*IMAGE_VERIFY_T)(IMAGE_DATA_T *list_head);

/*!
 * @brief Parse the image chained list and select the first valid entry.
 *
 * The image list is already sorted by version number. Compare image version against
 * Min version read from PSECT. If it is greater than or equal to Min version, perform the
 * RSA authentication over the image using the ket found in PFLASH if any.
 * see IMAGE_VERIFY_T
 * @param  list_head sorted list of images
 *
 * @return  selected image start address
 *
 */
static inline uint32_t boot_Verify_eScoreImageList(IMAGE_DATA_T *list_head)
{
    uint32_t (*p_boot_Verify_eScoreImageList)(IMAGE_DATA_T * list_head);
    p_boot_Verify_eScoreImageList = (uint32_t(*)(IMAGE_DATA_T * list_head))0x030003e5U;

    return p_boot_Verify_eScoreImageList(list_head);
}

/*!
 * @brief Search for a valid executable image between boundaries in internal flash.
 *
 * This function is involved in the search of a bootable image.
 * It is called by the boot ROM on Cold boot but can be called by the Selective OTA.
 *
 * The application granularity parameter is read from the PSECT, this is used as the
 * increment used to hop to next position in case of failure.
 * The function builds up a chained list of image descriptors that it sorts by version number.
 * The intent is that the most recent version is at the head of the list.
 *
 * @param  start_addr address from which to start search
 *
 * @param  end_addr  address from which to start search
 *
 * @param  signature  magic identifier : constant 0x98447902
 *
 * @param  IMAGE_VERIFY_T verification function pointer (see @boot_Verify_eScoreImageList)
 *         This parameter cannot be NULL.
 *         The implementer may opt for a version that simply returns the head of the chained list.
 *
 * @return  image address if valid, IMAGE_INVALID_ADDR (0xffffffff) otherwise
 *
 */
static inline uint32_t BOOT_FindImage(uint32_t start_addr, uint32_t end_addr, uint32_t signature, IMAGE_VERIFY_T verify)
{
    uint32_t (*p_BOOT_FindImage)(uint32_t start_addr, uint32_t end_addr, uint32_t signature, IMAGE_VERIFY_T verify);
    p_BOOT_FindImage =
        (uint32_t(*)(uint32_t start_addr, uint32_t end_addr, uint32_t signature, IMAGE_VERIFY_T verify))0x03000519U;

    return p_BOOT_FindImage(start_addr, end_addr, signature, verify);
}

/*!
 * @brief Retrieve LPMode value that has been saved previously in retained RAM bank.
 *
 * This is mostly used to determine in which power mode the PMC was before reset,
 * i.e. whether is is a cold or warm reset. This is to be invoked from ResetISR2
 *
 * @param  none
 *
 * @return  LPMode
 *
 */
static inline uint32_t BOOT_GetStartPowerMode(void)
{
    uint32_t (*p_BOOT_GetStartPowerMode)(void);
    p_BOOT_GetStartPowerMode = (uint32_t(*)(void))0x03000e9dU;

    return p_BOOT_GetStartPowerMode();
}

/*!
 * @brief Sets the value of stack pointer to be restored on warm boot.
 *
 * @param  stack_pointer address to be written in retained RAM bank so that
 * boot ROM restores value on warm start
 *
 * @return  none
 *
 */
static inline void BOOT_SetResumeStackPointer(uint32_t stack_pointer)
{
    void (*p_BOOT_SetResumeStackPointer)(uint32_t stack_pointer);
    p_BOOT_SetResumeStackPointer = (void (*)(uint32_t stack_pointer))0x03000ea9U;

    p_BOOT_SetResumeStackPointer(stack_pointer);
}

/*!
 * @brief Retrieve Internal flash address and size
 *
 * The internal flash start address is necessarily 0.
 * Its size may vary depending on chip options.
 * The size returned is the number of bytes usable for program and data.
 * The maximum possible value is 0x9dc00.
 *
 * @param address pointer on location to store returned address
 *
 * @param size pointer on location to store returned size
 *
 * @return  *address is 0x00000000UL and *size is up to 0x9dc00
 *
 */
static inline void ROM_GetFlash(uint32_t *address, uint32_t *size)
{
    void (*p_ROM_GetFlash)(uint32_t * address, uint32_t * size);
    p_ROM_GetFlash = (void (*)(uint32_t * address, uint32_t * size))0x03000e0dU;

    p_ROM_GetFlash(address, size);
}

/*!
 * @brief Retrieve SRAM0 address and size
 *
 * @param address pointer on location to store returned address
 *
 * @param size pointer on location to store returned size
 *
 * @return  *address is 0x04000000UL and *size is 88k (0x16000)
 *
 */
static inline void ROM_GetSRAM0(uint32_t *address, uint32_t *size)
{
    void (*p_ROM_GetSRAM0)(uint32_t * address, uint32_t * size);
    p_ROM_GetSRAM0 = (void (*)(uint32_t * address, uint32_t * size))0x03000e21U;

    p_ROM_GetSRAM0(address, size);
}

/*!
 * @brief Retrieve SRAM1 address and size.
 *        SRAM1 presence is optional depending on chip variant
 *
 * @param address pointer on location to store returned address
 *
 * @param size pointer on location to store returned size
 *
 * @return  if SRAM1 not present *address is 0 and *size is 0,
 *          otherwise *address is  0x04020000UL and *size is up to 64k (0x10000)
 */
static inline void ROM_GetSRAM1(uint32_t *address, uint32_t *size)
{
    void (*p_ROM_GetSRAM1)(uint32_t * address, uint32_t * size);
    p_ROM_GetSRAM1 = (void (*)(uint32_t * address, uint32_t * size))0x03000e35U;

    p_ROM_GetSRAM1(address, size);
}

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif /* ROM_API_H_ */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
