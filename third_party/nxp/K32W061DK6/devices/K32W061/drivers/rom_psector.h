/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ROM_PSECTOR_H_
#define ROM_PSECTOR_H_

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

#include <stddef.h>
#include "rom_common.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/*! @brief PSECTOR_PAGE_WORDS number of 16 byte words available in page
 * A page is 512 bytes in size. That is 32x16 bytes.
 * The first 32 bytes contain the page header, which leaves 30x16bytes for storage.
 * Thence the 30.
 */
#define PSECTOR_PAGE_WORDS 30

/*! @brief PSECTOR_PAGE0_MAGIC magic word to identify PAGE0 page in header */
#define PSECTOR_PAGE0_MAGIC  0xc51d8ca9
/*! @brief PSECTOR_PFLASH_MAGIC magic word to identify PFLASH page in header */
#define PSECTOR_PFLASH_MAGIC 0xa7b4353d

#ifdef __GNUC__
#define PSECT_READ(partition, page_type, var)                                               \
( {                                                                                                \
    typeof((((page_type *)0)->var)) _a;                                                            \
    psector_ReadData(partition, 0, offsetof(page_type, var), sizeof((((page_type *)0)->var)), &_a);\
    _a;                                                                                            \
} )
#endif

#define ROM_SEC_BOOT_AUTH_ON_UPGRADE ((1 << 0) | ROM_SEC_BOOT_AUTH_ON_BOOT)
#define ROM_SEC_BOOT_AUTH_ON_BOOT (1 << 1)
#define ROM_SEC_BOOT_PREVENT_DOWNGRADE (1 << 2)
#define ROM_SEC_BOOT_USE_NXP_KEY (1 << 3)

/*! @brief IMG_DIRECTORY_MAX_SIZE max number of entries in image directory
 * Concerns Secondary Stage Bootloader only
*/
#define IMG_DIRECTORY_MAX_SIZE 8
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/*! @brief psector_partition_id_t describes the 2 partitions of psectors.
 * Note: PAGE0 is termed PSECT in the FlashProgrammer, whereas PFLASH remains PFLASH.
 *
 */
typedef enum {

    PSECTOR_PAGE0_PART,  /*!< Page0 partition : termed PSECT by FLashProgramemr tool
                         * Image related data */
    PSECTOR_PFLASH_PART, /*!< PFLASH : Custommer configuration data */
    MAX_PSECTOR_PARTITIONS,
} psector_partition_id_t;

/*! @brief psector_page_state_t describes the possible states of the psector partitions. */
typedef enum
{
    PAGE_STATE_BLANK,     /*!< Page has never been programmed or has been erased */
    PAGE_STATE_ERROR,     /*!< Both subpages constituting the psector contain
                           *unrecoverable errors that ECC/parity cannot mend */
    PAGE_STATE_DEGRADED,  /*!< One subpage contains unrecoverable errors or is blank  */
    PAGE_STATE_OK,        /*!< Both subpages are correct */
} psector_page_state_t;

/*! @brief psector_write_status_t status code of writes to update page.
 */
typedef enum
{
    WRITE_OK = 0x0,                    /*!< Succeded in writing page */
    WRITE_ERROR_BAD_MAGIC,             /*!< Magic word incorrect in page header */
    WRITE_ERROR_INVALID_PAGE_NUMBER,   /*!< Invalid page number (higher than partition size) */
    WRITE_ERROR_BAD_VERSION,           /*!< Invalid version number: must increment monotonically */
    WRITE_ERROR_BAD_CHECKSUM,          /*!< Invalid checksum */
    WRITE_ERROR_INCORRECT_UPDATE_MODE, /*!< Update mode incorrect */
    WRITE_ERROR_UPDATE_INVALID,        /*!< Update invalid */
    WRITE_ERROR_PAGE_ERROR             /*!< Failure to program page in flash */
} psector_write_status_t;

/*! @brief AuthMode_t authentication options.
 */
typedef enum {
    AUTH_NONE = 0,          /*!< no authentication is performed */
    AUTH_ON_FW_UPDATE = 1,  /*!< authentication is performed on firmware update */
    AUTH_ALWAYS = 2,        /*!< authentication is performed at each Cold boot*/
  AUTH_LEVEL_NB
} AuthMode_t;

#define IMG_FLAG_BOOTABLE 1

/*! @brief image_directory_entry_t image directory found in PAGE0 (PSECT)
 * when SSBL is involved in the loading process
 */
typedef struct {
    uint32_t img_base_addr;    /*!< image start address in internal Flash or QSPI flash  */
    uint16_t img_nb_pages;     /*!< image number of 512 byte pages */
    uint8_t  flags;            /*!< IMG_FLAG_BOOTABLE : bit 0, other TBD */
    uint8_t  img_type;         /*!< image type */

} image_directory_entry_t;

typedef union
{
    uint8_t data_8[16];
    uint32_t data_32[4];
    uint64_t data_64[2];
} psector_page_word_t;

/*! @brief psector_header_t psector header.
 */
typedef struct
{
    uint32_t checksum;     /*!< page checksum */
    uint32_t magic;        /*!< magic:  PSECTOR_PAGE0_MAGIC or PSECTOR_PFLASH_MAGIC*/
    uint16_t psector_size;
    uint16_t page_number;  /*!< should be 0 because both partitions contain a single page */
    uint32_t version;
    uint8_t update_modes[16];

} psector_header_t;

typedef struct
{
    psector_header_t hdr;
    psector_page_word_t page_word[PSECTOR_PAGE_WORDS];
} psector_page_t;

typedef struct
{
    psector_header_t hdr;

    union
    {
        psector_page_word_t page_word[PSECTOR_PAGE_WORDS];

        struct
        {
            /* Word 0 - Any */
            uint32_t SelectedImageAddress;
            uint32_t RESERVED0[3];
            /* Word 1 - Increment */
            uint32_t MinVersion;
            uint32_t img_pk_valid;
            uint32_t RESERVED1[2];
            /* Word [2:17] - OTP */
            uint8_t image_pubkey[256];
        } page0_v2;                                   /*!< Deprecated form kept for backward compatibility */
        struct
        {
            /* Word 0 - Any */
            uint32_t SelectedImageAddress;            /*!< Address of image to be loaded by boot ROM
                                                       * offset 0x20 */
            uint32_t preferred_app_index;             /*!< for use with SSBL: index of application to select from image directory value 0..8
                                                       * offset 0x24 */
            image_directory_entry_t ota_entry;        /*!< New image written by OTA : SSBL to check validity and authentication
                                                       * offset 0x28 */
            /* Word 1 - Increment */
            uint32_t MinVersion;                      /*!< Minimum version accepted : application's version number must be greater than this one to be accepted.
                                                       * offset 0x30 */
            uint32_t img_pk_valid;                    /*!< Image public key valid
                                                       * offset 0x34 */
            uint32_t flash_audit_done;                /*!< Flash audit done: already sought for wrongly initialized pages
                                                       * offset 0x38 */
            uint32_t RESERVED1;                       /*!< padding reserved word */
            /* Word [2:17] - OTP */
            uint8_t image_pubkey[256];                /*!< RSA Public Key to be used to verify authenticity
                                                       * offset 0x40 */
            /* Word [18:20] */
            uint8_t zigbee_install_code[36];          /*!< Zigbee install code
                                                       * offset 0x140 */
            uint32_t RESERVED3[3];                    /*!< padding reserved wordes */
            /* Word 21 */
            uint8_t zigbee_password[16];              /*!< Zigbee password
                                                       * offset 0x170 */
            /*Word 22 */                              /*!< Image directory entries array, used by OTA process to locate images and/or blobs
                                                       * offset 0x180 */
            image_directory_entry_t
                      img_directory[IMG_DIRECTORY_MAX_SIZE];

        } page0_v3;
        struct
        {
            uint32_t rom_patch_region_sz;
            uint32_t rom_patch_region_addr;    /*!< ROM patch entry point address.
                                                * A value outside of the address range used to store
                                                * the ROM patch binary shall be deemed invalid
                                                */
            uint32_t rom_patch_checksum;
            uint32_t rom_patch_checksum_valid; /*!< ROM patch checksum valid:
                                                * 0 means invalid
                                                * Any other value means valid
                                                */
            uint32_t hwtestmode_disable;       /*< HW test mode control:
                                                *  0 means enabled
                                                *  Any other value means disabled
                                                */

            uint32_t ISP_access_level;         /*!< ISP access level:
                                                * 0 means full access, unsecure
                                                * 0x01010101 means full access, secure
                                                * 0x02020202 means write only, unsecure
                                                * 0x03030303 means write only, secure
                                                * 0x04040404 means locked
                                                * Any other value means disabled
                                                */

            uint16_t application_flash_sz;     /*!< Application flash size, in kilobytes.
                                                * 0 is interpreted as maximum (640).
                                                * This is intended to provide an alternative way of
                                                * restricting the flash size on a device, and to greater
                                                * granularity, than the eFuse bit.
                                                * The actual level of granularity that can be obtained is
                                                * dependent upon the MPU region configuration
                                                */
            uint16_t image_authentication_level; /*!< Image authentication level:
                                                  * 0 means check only header validity
                                                  * 1 means check signature of whole image if image has changed
                                                  * 2 means check signature of whole image on every cold start
                                                  */
            uint16_t unlock_key_valid;          /*!<  0: unlock key is not valid, >= 1: is present  */
            uint16_t ram1_bank_sz;               /*!< RAM bank 1 size, in kilobytes.
                                                  * This is intended to provide an alternative way of restricting
                                                  * the RAM size on a device, and to greater granularity,
                                                  * than the eFuse bit. The actual level of granularity that can
                                                  * be obtained is dependent upon the MPU region configuration */

            uint32_t app_search_granularity;     /*!< Application search granularity (increment), in bytes.
                                                  * Value of 0 shall be equated to 4096.
                                                  * Other values are to be used directly;
                                                  * configurations that are not using hardware remapping do not
                                                  * require hard restrictions
                                                  */
            uint32_t qspi_app_search_granularity;

            uint32_t reserved1[2];

            uint8_t ISP_protocol_key[16];        /*!< ISP protocol key: key used to encrypt messages over ISP UART
                                                  * with secure access level
                                                  */
            uint64_t ieee_mac_id1;               /*!< IEEE_MAC_ID_1 (Used to over-ride MAC ID_1 in N-2 page) */
            uint64_t ieee_mac_id2;               /*!< IEEE_MAC_ID_2 if second MAC iID is required */

            uint64_t ble_mac_id;                 /*!< BLE device address : only 6 LSB bytes are significant */
            uint8_t  reserved2[104];             /*!< Reserved for future use  */

            uint64_t customer_id;                /*!< Customer ID, used for secure handshake */
            uint64_t min_device_id;              /*!< Min Device ID, used for secure handshake - Certificate compatibility  */
            uint64_t device_id;                  /*!< Device ID, used for secure handshake */
            uint64_t max_device_id;              /*!< Max Device ID, used for secure handshake - Certificate compatibility  */
            uint8_t unlock_key[256];             /*!< 2048-bit public key for secure handshake
                                                  * (equivalent to ‘unlock’ key). Stored encrypted,
                                                  * using the AES key in eFuse*/
        } pFlash;

    };
} psector_page_data_t;

STATIC_ASSERT(sizeof(psector_page_t) == 512, "Psector page size not equal to flash page");
STATIC_ASSERT(sizeof(psector_page_data_t) == 512, "Psector data size not equal to flash page");

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/* General access functions */

/*!
 * @brief This function is used to validate a page content and write it to the update page.
 *
 * The actual write to the partition will be effective after a reset only.
 * Among other checks, the page must have a correct magic, a correct checksum
 *
 * @param  part_index: PSECTOR_PAGE0_PART or PFLASH_PAGE0_PART.
 * @param  page: psector_page_t RAM buffer to be written to update page
 *
 * @return  status code see @ psector_write_status_t.
 *
 */
static inline psector_write_status_t psector_WriteUpdatePage(psector_partition_id_t part_index, psector_page_t *page)
{
    psector_write_status_t (*p_psector_WriteUpdatePage)(psector_partition_id_t part_index, psector_page_t *page);
    p_psector_WriteUpdatePage =
        (psector_write_status_t (*)(psector_partition_id_t part_index, psector_page_t *page))0x03004e11U;

    return p_psector_WriteUpdatePage(part_index, page);
}

/*!
 * @brief This function is used to validate a page content and write it to the update page.
 *
 * The actual write to the partition will be effective after a reset only.
 *
 * @param  part_index: PSECTOR_PAGE0_PART or PFLASH_PAGE0_PART.
 * @param  page: psector_page_t RAM buffer to be written to update page
 *
 * @return  status code see @ psector_write_status_t.
 *
 */
static inline void psector_EraseUpdate(void)
{
    void (*p_psector_EraseUpdate)(void);
    p_psector_EraseUpdate = (void (*)(void))0x03004d59U;

    p_psector_EraseUpdate();
}

/*!
 * @brief This function is used to read data from a psector partition.
 *
 *
 * @param  part_index: PSECTOR_PAGE0_PART or PFLASH_PAGE0_PART.
 * @param  page_number: necessarily 0 since partitions now contain 1 single page.
 * @param  offset: offset of data from which data is to be read
 * @param  size: number of bytes to be read
 * @param  data: pointer on RAM buffer used to copy retrived data.
 *
 * @return  status code see @ psector_page_state_t
 *          if PAGE_STATE_DEGRADED or PAGE_STATE_OK, data is available.
 *          if PAGE_STATE_ERROR or PAGE_STATE_BLANK, no data was read
 */
static inline psector_page_state_t psector_ReadData(
    psector_partition_id_t part_index, int page_number, uint32_t offset, uint32_t size, void *data)
{
    psector_page_state_t (*p_psector_ReadData)(psector_partition_id_t part_index, int page_number, uint32_t offset,
                                               uint32_t size, void *data);
    p_psector_ReadData = (psector_page_state_t (*)(psector_partition_id_t part_index, int page_number, uint32_t offset,
                                                   uint32_t size, void *data))0x03004ef1U;

    return p_psector_ReadData(part_index, page_number, offset, size, data);
}

/*!
 * @brief This function is used to calculate a page checksum.
 *
 * It is essential to recalculate the checksum when performing a psector page update,
 * failing to update this field, the write operation would be rejected.
 *
 * @param  psector_page: pointer on page over which computation is required.
 *
 * @return  checksum value to be checked or to replace checksum field of psector header
 */
static inline uint32_t psector_CalculateChecksum(psector_page_t *psector_page)
{
    uint32_t (*p_psector_CalculateChecksum)(psector_page_t *psector_page);
    p_psector_CalculateChecksum = (uint32_t (*)(psector_page_t *psector_page))0x030050bdU;

    return p_psector_CalculateChecksum(psector_page);
}

/* Access helper functions */
/*!
 * @brief This function returns the CustomerId field.
 *
 * @param  none
 *
 * @return  CustomerId on 64 bit word
 */
static inline uint64_t psector_Read_CustomerId(void)
{
    uint64_t (*p_psector_Read_CustomerId)(void);
    p_psector_Read_CustomerId = (uint64_t (*)(void))0x030051ddU;

    return p_psector_Read_CustomerId();
}

/*!
 * @brief This function returns the ROM patch information read from the PFLASH.
 *
 * @param  patch_region_sz: pointer on unsigned long to return ROM patch size
 * @param  patch_region_addr: pointer on unsigned long to return ROM patch address
 * @param  patch_checksum: pointer on unsigned long to return ROM patch checksum value
 * @param  patch_checksum_valid: pointer on unsigned long to return ROM patch checksum validity (0..1)
 *
 * @return -1 if erro is found (any of the input parameters is NULL) or PFLASH is unreadable.
 */
static inline int psector_Read_RomPatchInfo(uint32_t *patch_region_sz,
                                            uint32_t *patch_region_addr,
                                            uint32_t *patch_checksum,
                                            uint32_t *patch_checksum_valid)
{
    int (*p_psector_Read_RomPatchInfo)(uint32_t *patch_region_sz, uint32_t *patch_region_addr, uint32_t *patch_checksum,
                                       uint32_t *patch_checksum_valid);
    p_psector_Read_RomPatchInfo = (int (*)(uint32_t *patch_region_sz, uint32_t *patch_region_addr,
                                           uint32_t *patch_checksum, uint32_t *patch_checksum_valid))0x03005209U;

    return p_psector_Read_RomPatchInfo(patch_region_sz, patch_region_addr, patch_checksum, patch_checksum_valid);
}

/*!
 * @brief This function returns the image authentication level from the PFLASH.
 *
 * @param  none.
 *
 * @return AUH_NONE if PFLASH unreadable, or the image_authentication_level field value if readable.
 */
static inline uint16_t psector_Read_ImgAuthLevel(void)
{
    uint16_t (*p_psector_Read_ImgAuthLevel)(void);
    p_psector_Read_ImgAuthLevel = (uint16_t (*)(void))0x03005299U;

    return p_psector_Read_ImgAuthLevel();
}

/*!
 * @brief This function returns the app search granularity value from the PFLASH.
 *
 * @param  none.
 *
 * @return 0 if PFLASH unreadable, or the app_search_granularity field value if not 0 or 4096 if 0.
 */
static inline uint32_t psector_Read_AppSearchGranularity(void)
{
    uint32_t (*p_psector_Read_AppSearchGranularity)(void);
    p_psector_Read_AppSearchGranularity = (uint32_t (*)(void))0x030052d5U;

    return p_psector_Read_AppSearchGranularity();
}

/*!
 * @brief This function returns the Qspi app search granularity value from the PFLASH.
 *
 * @param  none.
 *
 * @return 0 if PFLASH unreadable, or the qspi_app_search_granularity field value.
 */
static inline uint32_t psector_Read_QspiAppSearchGranularity(void)
{
    uint32_t (*p_psector_Read_QspiAppSearchGranularity)(void);
    p_psector_Read_QspiAppSearchGranularity = (uint32_t (*)(void))0x03005305U;

    return p_psector_Read_QspiAppSearchGranularity();
}

/*!
 * @brief This function returns the DeviceId value from the PFLASH.
 *
 * @param  none.
 *
 * @return 0 if PFLASH unreadable, or the device_id field value.
 */
static inline uint64_t psector_Read_DeviceId(void)
{
    uint64_t (*p_psector_Read_DeviceId)(void);
    p_psector_Read_DeviceId = (uint64_t (*)(void))0x03005329U;

    return p_psector_Read_DeviceId();
}

/*!
 * @brief This function returns the unlock key value from the PFLASH.
 *
 * @param  valid:  pointer on int to store validity of key (unlock_key_valid field)
 * @param  key:   pointer on 256 byte storage to receive the key read from PFLASH
 * @param  raw:    raw  if raw is not requested (0), the key is deciphered
 *                using the internal AES fused key.
 *
 * @return -1 if read error occurred, 0 otherwise
 */
static inline int psector_Read_UnlockKey(int *valid, uint8_t key[256], bool raw)
{
    int (*p_psector_Read_UnlockKey)(int *valid, uint8_t key[256], bool raw);
    p_psector_Read_UnlockKey = (int (*)(int *valid, uint8_t key[256], bool raw))0x03005519U;

    return p_psector_Read_UnlockKey(valid, key, raw);
}

/*!
 * @brief This function returns the ISP protocol AES key from PFLASH.
 *
 * @param  key:   pointer on 16 byte storage to receive the key read from PFLASH.
 *
 * @return -1 if read error occurred, 0 otherwise
 */
static inline int psector_Read_ISP_protocol_key(uint8_t key[16])
{
    int (*p_psector_Read_ISP_protocol_key)(uint8_t key[16]);
    p_psector_Read_ISP_protocol_key = (int (*)(uint8_t key[16]))0x03005355U;

    return p_psector_Read_ISP_protocol_key(key);
}

/*!
 * @brief This function returns the IEEE-802.15.4 Mac address first instance from PFLASH.
 *
 * @param  none.
 *
 * @return 64 bit word 0 if field unreadable, otherwise MAC address contained in ieee_mac_id1 field.
 */
static inline uint64_t psector_ReadIeee802_15_4_MacId1(void)
{
    uint64_t (*p_psector_ReadIeee802_15_4_MacId1)(void);
    p_psector_ReadIeee802_15_4_MacId1 = (uint64_t (*)(void))0x030053b1U;

    return p_psector_ReadIeee802_15_4_MacId1();
}

/*!
 * @brief This function returns the IEEE-802.15.4 Mac address second instance from PFLASH.
 *
 * @param  none.
 *
 * @return 64 bit word 0 if field unreadable, otherwise MAC address contained in ieee_mac_id2 field.
 */
static inline uint64_t psector_ReadIeee802_15_4_MacId2(void)
{
    uint64_t (*p_psector_ReadIeee802_15_4_MacId2)(void);
    p_psector_ReadIeee802_15_4_MacId2 = (uint64_t (*)(void))0x03005385U;

    return p_psector_ReadIeee802_15_4_MacId2();
}

/*!
 * @brief This function returns the Min Device id from PFLASH.
 *
 * @param  none.
 *
 * @return 0 if PFLASH unreadable, otherwise min_device_id field content.
 */
static inline uint64_t psector_Read_MinDeviceId(void)
{
    uint64_t (*p_psector_Read_MinDeviceId)(void);
    p_psector_Read_MinDeviceId = (uint64_t (*)(void))0x030053ddU;

    return p_psector_Read_MinDeviceId();
}

/*!
 * @brief This function returns the Max Device id from PFLASH.
 *
 * @param  none.
 *
 * @return 0 if PFLASH unreadable, otherwise max_device_id field content.
 */
static inline uint64_t psector_Read_MaxDeviceId(void)
{
    uint64_t (*p_psector_Read_MaxDeviceId)(void);
    p_psector_Read_MaxDeviceId = (uint64_t (*)(void))0x03005409U;

    return p_psector_Read_MaxDeviceId();
}

/* Helper functions for reading and writing image data */

/*!
 * @brief This function returns the Min Version from PAGE0.
 *
 * @param  none.
 *
 * @return if PAGE0 unreadable, otherwise MinVersion field content.
 */
static inline uint32_t psector_Read_MinVersion(void)
{
    uint32_t (*p_psector_Read_MinVersion)(void);
    p_psector_Read_MinVersion = (uint32_t (*)(void))0x03005439U;

    return p_psector_Read_MinVersion();
}

/*!
 * @brief This function is used to set the selected image address and MinVersion into PAGE0.
 *
 * @param  image_addr:   32 bit value to be written to SelectImageAddress.
 * @param  min_version:   32 bit value to be written to MinVersion.
 *
 * @return psector_write_status_t status of operation see @psector_WriteUpdatePage
 */
static inline psector_write_status_t psector_SetEscoreImageData(uint32_t image_addr, uint32_t min_version)
{
    psector_write_status_t (*p_psector_SetEscoreImageData)(uint32_t image_addr, uint32_t min_version);
    p_psector_SetEscoreImageData = (psector_write_status_t (*)(uint32_t image_addr, uint32_t min_version))0x0300545dU;

    return p_psector_SetEscoreImageData(image_addr, min_version);
}

/*!
 * @brief This function returns the image address and min version value from PAGE0.
 *
 * @param  image_addr:   pointer on 32 bit word to receive SelectImageAddress value.
 * @param  min_versionm:   pointer on 32 bit word to receive SelectImageAddress value.
 *
 * @return -1 if read error occurred, 0 otherwise
 */
static inline psector_page_state_t psector_ReadEscoreImageData(uint32_t *image_addr, uint32_t *min_version)
{
    psector_page_state_t (*p_psector_ReadEscoreImageData)(uint32_t *image_addr, uint32_t *min_version);
    p_psector_ReadEscoreImageData = (psector_page_state_t (*)(uint32_t *image_addr, uint32_t *min_version))0x03005491U;

    return p_psector_ReadEscoreImageData(image_addr, min_version);
}

/*!
 * @brief This function returns the unlock key value from PAGE0.
 *
 * @param  valid:  pointer on int to store validity of key (img_pk_valid field)
 * @param  key:   pointer on 256 byte storage to receive the key read from PAGE0
 * @param  raw:   raw  if raw is not requested (0), the key is deciphered
 *                using the internal AES fused key.
 *
 * @return -1 if read error occurred, 0 otherwise
 */
static inline int psector_Read_ImagePubKey(int *valid, uint8_t key[256], bool raw)
{
    int (*p_psector_Read_ImagePubKey)(int *valid, uint8_t key[256], bool raw);
    p_psector_Read_ImagePubKey = (int (*)(int *valid, uint8_t key[256], bool raw))0x03005531U;

    return p_psector_Read_ImagePubKey(valid, key, raw);
}

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif /* ROM_PSECTOR_H_ */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
