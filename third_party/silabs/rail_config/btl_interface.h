/***************************************************************************//**
 * @file
 * @brief Application interface to the bootloader.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc.  Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.  This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#ifndef BTL_INTERFACE_H
#define BTL_INTERFACE_H

#include <stddef.h>

#include "btl_errorcode.h"
#include "btl_reset_info.h"
#include "application_properties.h"

// Include plugin-specific interfaces
#include "btl_interface_parser.h"
#include "btl_interface_storage.h"

// Get flash page size
#include "em_device.h"

/***************************************************************************//**
 * @addtogroup Interface Application Interface
 * @brief Application interface to the bootloader
 * @details
 *   The application interface consists of functions that can be included
 *   in the customer application that and will communicate with the
 *   bootloader through the @ref MainBootloaderTable_t. This table
 *   contains function pointers to the bootloader. The 10th word of the
 *   bootloader contains a pointer to this struct, allowing any application to
 *   easily locate it.
 *   To access the bootloader table, use wrapper functions. Avoid
 *   accessing the bootloader table directly.
 *
 * @{
 * @addtogroup CommonInterface Common Application Interface
 * @brief Generic application interface available on all versions of the
 *        bootloader, regardless of the available plugins.
 * @details
 * @{
 ******************************************************************************/

/// Bare boot table. Can be mapped on top of vector table to access contents.
typedef struct {
  /// Pointer to top of stack
  uint32_t *stackTop;
  /// Pointer to reset vector
  void     (*resetVector)(void);
  /// Reserved pointers to fault handlers
  uint32_t reserved0[5];
  /// Reserved pointers to RESERVED fields
  uint32_t reserved1[3];
  /// Pointer to bootloader table
  void     *table;
  /// Reserved pointers to SVC and DebugMon interrupts
  uint32_t reserved2[2];
  /// Pointer to application signature
  void     *signature;
} BareBootTable_t;

// --------------------------------
// Bootloader information typedefs

/// Type of bootloader
typedef enum {
  /// No bootloader present.
  NO_BOOTLOADER = 0,
  /// Bootloader is a Silicon Labs bootloader.
  SL_BOOTLOADER = 1
} BootloaderType_t;

/// Information about the current bootloader
typedef struct {
  /// The type of bootloader.
  BootloaderType_t type;
  /// Version number of the bootloader
  uint32_t version;
  /// Capability mask for the bootloader.
  uint32_t capabilities;
} BootloaderInformation_t;

/// Common header for bootloader tables
typedef struct {
  /// Type of image
  uint32_t type;
  /// Version number of the bootloader/application table
  uint32_t layout;
  /// Version number of the image
  uint32_t version;
} BootloaderHeader_t;

/// Address table for the First Stage Bootloader
typedef struct {
  /// Header of the First Stage Bootloader table
  BootloaderHeader_t header;
  /// Start address of the Main Bootloader
  BareBootTable_t *mainBootloader;
  /// Location of the Main Bootloader upgrade image
  BareBootTable_t *upgradeLocation;
} FirstBootloaderTable_t;

/// Address table for the Main Bootloader
typedef struct {
  /// Header of the Main Bootloader table
  BootloaderHeader_t header;
  /// Size of the Main Bootloader
  uint32_t size;
  /// Start address of the application
  BareBootTable_t *startOfAppSpace;
  /// End address of the allocated application space
  uint32_t *endOfAppSpace;
  /// Capabilities of the bootloader
  uint32_t capabilities;
  // ------------------------------
  /// Initialize bootloader for use from application
  int32_t (*init)(void);
  /// Deinitialize bootloader after use from application
  int32_t (*deinit)(void);
  // ------------------------------
  /// Verify application
  bool (*verifyApplication)(uint32_t startAddress);
  // ------------------------------
  /// Initialize parser
  int32_t (*initParser)(BootloaderParserContext_t *context, size_t contextSize);
  /// Parse a buffer
  int32_t (*parseBuffer)(BootloaderParserContext_t         *context,
                         const BootloaderParserCallbacks_t *callbacks,
                         uint8_t                           data[],
                         size_t                            numBytes);

  // ------------------------------
  /// Function table for storage plugin
  const BootloaderStorageFunctions_t *storage;
} MainBootloaderTable_t;

// --------------------------------
// Bootloader capabilities

/// Bootloader enforces signed application upgrade images
#define BOOTLOADER_CAPABILITY_ENFORCE_UPGRADE_SIGNATURE   (1 << 0)
/// Bootloader enforces encrypted application upgrade images
#define BOOTLOADER_CAPABILITY_ENFORCE_UPGRADE_ENCRYPTION  (1 << 1)
/// @brief Bootloader enforces signature verification of the application image
///        before every boot
#define BOOTLOADER_CAPABILITY_ENFORCE_SECURE_BOOT         (1 << 2)

/// Bootloader has the capability of being upgraded
#define BOOTLOADER_CAPABILITY_BOOTLOADER_UPGRADE          (1 << 4)

/// Bootloader has the capability of parsing EBL files
#define BOOTLOADER_CAPABILITY_EBL                         (1 << 5)
/// Bootloader has the capability of parsing signed EBL files
#define BOOTLOADER_CAPABILITY_EBL_SIGNATURE               (1 << 6)
/// Bootloader has the capability of parsing encrypted EBL files
#define BOOTLOADER_CAPABILITY_EBL_ENCRYPTION              (1 << 7)

/// @brief Bootloader has the capability of storing data in an internal or
/// external storage medium
#define BOOTLOADER_CAPABILITY_STORAGE                     (1 << 16)
/// @brief Bootloader has the capability of communicating with host processors
/// using a communication interface
#define BOOTLOADER_CAPABILITY_COMMUNICATION               (1 << 20)

// --------------------------------
// Magic constants for bootloader tables

/// Magic word indicating first stage bootloader table
#define BOOTLOADER_MAGIC_FIRST_STAGE                      (0xB00710ADUL)
/// Magic word indicating main bootloader table
#define BOOTLOADER_MAGIC_MAIN                             (0x5ECDB007UL)

/// @cond DO_NOT_INCLUDE_WITH_DOXYGEN
#define BOOTLOADER_HEADER_VERSION_FIRST_STAGE             (0x00000001UL)
#define BOOTLOADER_HEADER_VERSION_MAIN                    (0x00000001UL)
/// @endcond

// --------------------------------
// Bootloader table access

/// @cond DO_NOT_INCLUDE_WITH_DOXYGEN

#if defined(SEMAILBOX_PRESENT) || defined(CRYPTOACC_PRESENT) || defined(MAIN_BOOTLOADER_TEST)
// No first stage on devices with SE
#define BTL_FIRST_STAGE_SIZE              (0UL)
#else
// First stage takes a single flash page
#define BTL_FIRST_STAGE_SIZE              (FLASH_PAGE_SIZE)
#define BOOTLOADER_HAS_FIRST_STAGE
#endif

#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_80)
// No writeable bootloader area: Place the bootloader in main flash
#define BTL_FIRST_STAGE_BASE              0x00000000UL
#define BTL_APPLICATION_BASE              0x00004000UL
#define BTL_MAIN_STAGE_MAX_SIZE           (BTL_APPLICATION_BASE \
                                           - BTL_FIRST_STAGE_SIZE)
#elif defined(_SILICON_LABS_GECKO_INTERNAL_SDID_84)
// Dedicated bootloader area of 38k
// Place the bootloader in the dedicated bootloader area of the
// information block
#define BTL_FIRST_STAGE_BASE              0x0FE10000UL
#define BTL_APPLICATION_BASE              0x00000000UL
#define BTL_MAIN_STAGE_MAX_SIZE           (0x00009800UL - BTL_FIRST_STAGE_SIZE)
#elif defined(_SILICON_LABS_GECKO_INTERNAL_SDID_89)
#define BTL_FIRST_STAGE_BASE              0x0FE10000UL
#if defined(MAIN_BOOTLOADER_IN_MAIN_FLASH)
#define BTL_APPLICATION_BASE              0x00004800UL
#define BTL_MAIN_STAGE_MAX_SIZE           BTL_APPLICATION_BASE
#else
// Dedicated bootloader area of 16k
// Place the bootloader in the dedicated bootloader area of the
// information block
#define BTL_APPLICATION_BASE              0x00000000UL
#define BTL_MAIN_STAGE_MAX_SIZE           (0x00004000UL - BTL_FIRST_STAGE_SIZE)
#endif
#elif defined(_SILICON_LABS_GECKO_INTERNAL_SDID_95)
#define BTL_FIRST_STAGE_BASE              0x0FE10000UL
#if defined(MAIN_BOOTLOADER_IN_MAIN_FLASH)
#define BTL_APPLICATION_BASE              0x00004800UL
#define BTL_MAIN_STAGE_MAX_SIZE           BTL_APPLICATION_BASE
#else
// Dedicated bootloader area of 18k
// Place the bootloader in the dedicated bootloader area of the
// information block
#define BTL_APPLICATION_BASE              0x00000000UL
#define BTL_MAIN_STAGE_MAX_SIZE           (0x00004800UL - BTL_FIRST_STAGE_SIZE)
#endif
#elif defined(_SILICON_LABS_GECKO_INTERNAL_SDID_100) || defined(_SILICON_LABS_GECKO_INTERNAL_SDID_106)
// Dedicated bootloader area of 32k
// Place the bootloader in the dedicated bootloader area of the
// information block
#define BTL_FIRST_STAGE_BASE              0x0FE10000UL
#define BTL_APPLICATION_BASE              0x00000000UL
#define BTL_MAIN_STAGE_MAX_SIZE           (0x00008000UL - BTL_FIRST_STAGE_SIZE)
#elif defined(_SILICON_LABS_GECKO_INTERNAL_SDID_103)
// Dedicated bootloader area of 18k
// Place the bootloader in the dedicated bootloader area of the
// information block
#define BTL_FIRST_STAGE_BASE              0x0FE10000UL
#define BTL_APPLICATION_BASE              0x00000000UL
#define BTL_MAIN_STAGE_MAX_SIZE           (0x00004800UL - BTL_FIRST_STAGE_SIZE)
#elif defined(_SILICON_LABS_GECKO_INTERNAL_SDID_200)
// No bootloader area: Place the bootloader in main flash
#define BTL_FIRST_STAGE_BASE              0x00000000UL
#define BTL_APPLICATION_BASE              0x00004000UL
#define BTL_MAIN_STAGE_MAX_SIZE           (BTL_APPLICATION_BASE \
                                           - BTL_FIRST_STAGE_SIZE)
#elif defined(_SILICON_LABS_GECKO_INTERNAL_SDID_205)
// No bootloader area: Place the bootloader in main flash
#define BTL_FIRST_STAGE_BASE              0x00000000UL
#define BTL_APPLICATION_BASE              0x00006000UL
#define BTL_MAIN_STAGE_MAX_SIZE           (BTL_APPLICATION_BASE \
                                           - BTL_FIRST_STAGE_SIZE)
#else
#error "This part is not supported in this bootloader version."
#endif

#if defined(MAIN_BOOTLOADER_TEST) || defined(MAIN_BOOTLOADER_IN_MAIN_FLASH)
#define BTL_MAIN_STAGE_BASE               (0UL)
#else
#define BTL_MAIN_STAGE_BASE               (BTL_FIRST_STAGE_BASE \
                                           + BTL_FIRST_STAGE_SIZE)
#endif

#if defined(BOOTLOADER_HAS_FIRST_STAGE)
#define BTL_FIRST_BOOTLOADER_TABLE_BASE   (BTL_FIRST_STAGE_BASE \
                                           + offsetof(BareBootTable_t, table))
#endif
#define BTL_MAIN_BOOTLOADER_TABLE_BASE    (BTL_MAIN_STAGE_BASE \
                                           + offsetof(BareBootTable_t, table))

/// @endcond // DO_NOT_INCLUDE_WITH_DOXYGEN

#if defined(MAIN_BOOTLOADER_TEST)
#if defined(BOOTLOADER_HAS_FIRST_STAGE)
extern FirstBootloaderTable_t *firstBootloaderTable;
#endif
extern MainBootloaderTable_t *mainBootloaderTable;
#else
#if defined(BOOTLOADER_HAS_FIRST_STAGE)
/// Pointer to first stage bootloader table
#define firstBootloaderTable              (*(FirstBootloaderTable_t **) \
                                           (BTL_FIRST_BOOTLOADER_TABLE_BASE))
#endif // BOOTLOADER_HAS_FIRST_STAGE
/// Pointer to main bootloader table
#define mainBootloaderTable               (*(MainBootloaderTable_t **) \
                                           (BTL_MAIN_BOOTLOADER_TABLE_BASE))
#endif // MAIN_BOOTLOADER_TEST
// --------------------------------
// Functions

/***************************************************************************//**
 * Get information about the bootloader on this device.
 *
 * The information returned is fetched from the main bootloader
 * information table.
 *
 * @param[out] info Pointer to the bootloader information struct.
 ******************************************************************************/
void bootloader_getInfo(BootloaderInformation_t *info);

/***************************************************************************//**
 * Initialize components of the bootloader
 * so the app can use the interface. This typically includes initializing
 * serial peripherals for communication with external SPI flashes, and so on.
 *
 * @return Error code. @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_INIT_BASE range.
 ******************************************************************************/
int32_t bootloader_init(void);

/***************************************************************************//**
 * De-initialize components of the bootloader that were previously initialized.
 * This typically includes powering down external SPI flashes and
 * de-initializing the serial peripheral used for communication with the
 * external flash.
 *
 * @return Error code. @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_INIT_BASE range.
 ******************************************************************************/
int32_t bootloader_deinit(void);

/***************************************************************************//**
 * Reboot into the bootloader to install something.
 *
 * If there is a storage plugin and a slot is marked for bootload, install
 * the image in that slot after verifying it.
 *
 * If a communication plugin is present, open the communication channel and
 * receive an image to be installed.
 ******************************************************************************/
void bootloader_rebootAndInstall(void);

/***************************************************************************//**
 * Verify the application image stored in the Flash memory starting at
 * the address startAddress.
 *
 * If the secure boot is enforced, the function will only return true if the
 * cryptographic signature of the application is valid. Otherwise, the
 * application is verified according to the signature type defined in the
 * ApplicationProperties_t structure embedded in the application. Silicon Labs
 * wireless stacks include a declaration of this structure. However,
 * applications not using a full wireless stack may need to instantiate
 * the structure.
 *
 * Examples of results when the secure boot is not enforced:
 * - App has no signature: Valid if initial stack pointer and program counter
 *                         have reasonable values.
 * - App has CRC checksum: Valid if checksum is valid.
 * - App has ECDSA signature: Valid if ECDSA signature is valid.
 *
 * When secure boot is enforced, only ECDSA signed applications with
 * a valid signature are considered valid.
 *
 * @param[in] startAddress Starting address of the application.
 *
 * @return True if the application is valid, else false.
 ******************************************************************************/
bool bootloader_verifyApplication(uint32_t startAddress);

#if defined(_SILICON_LABS_32B_SERIES_2)
/***************************************************************************//**
 * Get bootloader certificate version.
 *
 * @param[out] version           Bootloader certificate version
 *
 * @return True if a valid certificate version is found.
 ******************************************************************************/
bool bootloader_getCertificateVersion(uint32_t *version);
#endif // _SILICON_LABS_32B_SERIES_2

#if defined(BOOTLOADER_HAS_FIRST_STAGE)
/***************************************************************************//**
 * Check if a pointer is valid and if it points to the bootloader first stage.
 *
 * This function can be used to check pointers to bootloader
 * jump tables.
 *
 * @param[in] ptr The pointer to check
 *
 * @return True if the pointer points to the bootloader first stage,
 *         false if not.
 ******************************************************************************/
__STATIC_INLINE bool bootloader_pointerToFirstStageValid(const void *ptr);
__STATIC_INLINE bool bootloader_pointerToFirstStageValid(const void *ptr)
{
#if defined(MAIN_BOOTLOADER_TEST)
  // In main bootloader tests, no first stage is present
  (void) ptr;
  return false;
#elif BTL_FIRST_STAGE_BASE > 0U
  if (((size_t)(ptr) >= BTL_FIRST_STAGE_BASE)
      && ((size_t)(ptr) < (BTL_FIRST_STAGE_BASE + BTL_FIRST_STAGE_SIZE))) {
    return true;
  } else {
    return false;
  }
#else
  // First stage starts at address 0, don't need to check lower bound
  if ((size_t)(ptr) < (BTL_FIRST_STAGE_BASE + BTL_FIRST_STAGE_SIZE)) {
    return true;
  } else {
    return false;
  }
#endif
}
#endif // BOOTLOADER_HAS_FIRST_STAGE

/***************************************************************************//**
 * Check if a pointer is valid and if it points to the bootloader main stage.
 *
 * This function can be used to check pointers to bootloader
 * jump tables.
 *
 * @param[in] ptr The pointer to check
 *
 * @return True if the pointer points into the bootloader main stage,
 *         false if not.
 ******************************************************************************/
__STATIC_INLINE bool bootloader_pointerValid(const void *ptr);
__STATIC_INLINE bool bootloader_pointerValid(const void *ptr)
{
#if defined(MAIN_BOOTLOADER_TEST)
  // In main bootloader tests, all of memory is considered part of the bootloader
  (void) ptr;
  return true;
#elif BTL_MAIN_STAGE_BASE > 0U
  if (((size_t)ptr >= BTL_MAIN_STAGE_BASE)
      && ((size_t)ptr < (BTL_MAIN_STAGE_BASE + BTL_MAIN_STAGE_MAX_SIZE))) {
    return true;
  } else {
    return false;
  }
#else
  if ((size_t)ptr < (BTL_MAIN_STAGE_BASE + BTL_MAIN_STAGE_MAX_SIZE)) {
    return true;
  } else {
    return false;
  }
#endif
}

/** @} (end addtogroup CommonInterface) */
/** @} (end addtogroup Interface) */

#endif // BTL_INTERFACE_H
