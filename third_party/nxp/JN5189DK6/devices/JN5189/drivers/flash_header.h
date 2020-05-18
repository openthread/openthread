/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FLASH_HEADER_H_
#define FLASH_HEADER_H_

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <stdint.h>

/*!
 * @addtogroup jn_flash
 * @{
 */

/*! @file */

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define NUMBER_CCSUM_VECTORS (7)

#define IMAGE_SIGNATURE (0x98447902)
#define IMAGE_HEADER_SIGNATURE_V3_ZB (IMAGE_SIGNATURE + 1)
#define IMAGE_HEADER_SIGNATURE_V3_BLE (IMAGE_SIGNATURE + 2)
#define BOOT_BLOCK_HDR_MARKER 0xbb0110bb

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/*!
 * @brief Image header.
 *
 * Be very cautious when modifying the IMG_HEADER_T and the BOOT_BLOCK_T structures (alignment) as
 * these structures are used in the image_tool.py (which does not take care of alignment).
 */
typedef struct
{
    uint32_t vectors[NUMBER_CCSUM_VECTORS]; /*!< critical vectors protected by csum */
    uint32_t vectorCsum;                    /*!< csum of vectors 0-7 */
    uint32_t imageSignature;                /*!< image signature */
    uint32_t bootBlockOffset;               /*!< offset of boot block structure */
    uint32_t header_crc;                    /*!< the CRC of header */
} IMG_HEADER_T;

/*!
 * @brief Boot block.
 *
 * For some ADC16 channels, there are two pin selections in channel multiplexer. For example, ADC0_SE4a and ADC0_SE4b
 * are the different channels that share the same channel number.
 */
typedef struct
{
    uint32_t header_marker;        /*!< Image header marker should always be set to 0xbb0110bb+/-2 */
    uint32_t img_type;             /*!< Image check type, with or without optional CRC */
    uint32_t target_addr;          /*!< Target address */
    uint32_t img_len;              /*!< Image length or the length of image CRC check should be done.
                           For faster boot application could set a smaller length than actual image.
                           For Secure boot images, this MUST be the entire image length */
    uint32_t stated_size;          /*!< max size of any subsequent image : AppSize0 = 2 x stated_size */
    uint32_t certificate_offset;   /*!< Offset of the certificate list */
    uint32_t compatibility_offset; /*!< Offset of the compatibility list */
    uint32_t version;              /*!< Image version for multi-image support */
} BOOT_BLOCK_T;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif /* FLASH_HEADER_H_ */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
