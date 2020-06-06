/***************************************************************************//**
 * @file
 * @brief Application interface to the storage plugin of the bootloader.
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

#ifndef BTL_INTERFACE_STORAGE_H
#define BTL_INTERFACE_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Get part series version.
#include "em_device.h"

/***************************************************************************//**
 * @addtogroup Interface
 * @{
 * @addtogroup StorageInterface Application Storage Interface
 * @brief Application interface for interfacing with the bootloader storage.
 * @details The Storage Interface is only available on bootloaders that declare
 *          they support @ref BOOTLOADER_CAPABILITY_STORAGE.
 * @{
 ******************************************************************************/

// -----------------------------------------------------------------------------
// Typedefs

/// Possible storage types
typedef enum {
  /// Storage backend is a SPI flash
  SPIFLASH,
  /// Storage backend is internal flash
  INTERNAL_FLASH,
  /// Storage backend is custom
  CUSTOM_STORAGE
} BootloaderStorageType_t;

/// Information about a storage slot
typedef struct {
  /// Address of the slot.
  uint32_t address;
  /// Size of the slot.
  uint32_t length;
} BootloaderStorageSlot_t;

/// Information about the bootloader storage implementation
typedef struct {
  /// The version of this data structure
  uint16_t version;
  /// A bitmask describing the capabilities of this particular storage
  uint16_t capabilitiesMask;
  /// Maximum time it takes to erase a page. (in milliseconds)
  uint32_t pageEraseMs;
  /// Maximum time it takes to erase the entire part. (in milliseconds)
  uint32_t partEraseMs;
  /// The size of a single erasable page in bytes
  uint32_t pageSize;
  /// The total size of the storage in bytes
  uint32_t partSize;
  /// Pointer to a string describing the attached storage
  const char * const partDescription;
  /// The number of bytes in a word for the storage
  uint8_t wordSizeBytes;
} BootloaderStorageImplementationInformation_t;

/// Information about the bootloader storage
typedef struct {
  /// The version of this data structure
  uint32_t version;
  /// The capabilities of the storage plugin
  uint32_t capabilities;
  /// Type of storage
  BootloaderStorageType_t storageType;
  /// Number of storage slots
  uint32_t numStorageSlots;
  /// Detailed information about the attached storage
  const BootloaderStorageImplementationInformation_t *info;
} BootloaderStorageInformation_t;

/// Erase status struct
typedef struct {
  /// Address of the current page to be erased
  uint32_t currentPageAddr;
  /// The size of a single erasable page in bytes
  uint32_t pageSize;
  /// Information about a storage slot
  BootloaderStorageSlot_t storageSlotInfo;
} BootloaderEraseStatus_t;

/// Storage API accessible from the application
typedef struct BootloaderStorageFunctions {
  /// Version of this struct
  uint32_t version;
  /// Get information about the storage -- capabilities, layout, configuration
  void (*getInfo)(BootloaderStorageInformation_t *info);
  /// Get information about storage slot -- size, location
  int32_t (*getSlotInfo)(uint32_t slotId, BootloaderStorageSlot_t *slot);
  /// Read bytes from slot into buffer
  int32_t (*read)(uint32_t slotId,
                  uint32_t offset,
                  uint8_t  *buffer,
                  size_t   length);
  /// Write bytes from buffer into slot
  int32_t (*write)(uint32_t slotId,
                   uint32_t offset,
                   uint8_t  *buffer,
                   size_t   length);
  /// Erase an entire slot
  int32_t (*erase)(uint32_t slotId);
  // ------------------------------
  /// Mark a list of slots for bootload
  int32_t (*setImagesToBootload)(int32_t *slotIds, size_t length);
  /// Mark a list of slots for bootload
  int32_t (*getImagesToBootload)(int32_t *slotIds, size_t length);
  /// Append a slot to bootload list
  int32_t (*appendImageToBootloadList)(int32_t slotId);
  // ------------------------------
  /// Start image parsing
  int32_t (*initParseImage)(uint32_t                  slotId,
                            BootloaderParserContext_t *context,
                            size_t                    contextSize);
  /// Continue image verification
  int32_t (*verifyImage)(BootloaderParserContext_t  *context,
                         BootloaderParserCallback_t metadataCallback);
  /// Get app and bootloader upgrade information from storage slot
  int32_t (*getImageInfo)(BootloaderParserContext_t *context,
                          ApplicationData_t         *appInfo,
                          uint32_t                  *bootloaderVersion);
  /// Check whether the bootloader storage is busy
  bool (*isBusy)(void);
  /// Read raw bytes from storage
  int32_t (*readRaw)(uint32_t address, uint8_t *buffer, size_t length);
  /// Write bytes to raw storage
  int32_t (*writeRaw)(uint32_t address, uint8_t *buffer, size_t length);
  /// Erase storage
  int32_t (*eraseRaw)(uint32_t address, size_t length);
} BootloaderStorageFunctions_t;

// -----------------------------------------------------------------------------
// Defines

/// Context size for bootloader verification context
#if defined(_SILICON_LABS_32B_SERIES_2)
#define BOOTLOADER_STORAGE_VERIFICATION_CONTEXT_SIZE            (524)
#else
#define BOOTLOADER_STORAGE_VERIFICATION_CONTEXT_SIZE            (384)
#endif

/// Current version of the BootloaderStorageImplementationInformation_t struct
#define BOOTLOADER_STORAGE_IMPL_INFO_VERSION                    (0x0201)
/// Major version of the BootloaderStorageImplementationInformation_t struct
#define BOOTLOADER_STORAGE_IMPL_INFO_VERSION_MAJOR              (0x0200)
/// Major version mask for @ref BOOTLOADER_STORAGE_IMPL_INFO_VERSION
#define BOOTLOADER_STORAGE_IMPL_INFO_VERSION_MAJOR_MASK         (0xFF00)

/// Spiflash capability indicating that it supports erase
#define BOOTLOADER_STORAGE_IMPL_CAPABILITY_ERASE_SUPPORTED      (1 << 0)
/// @brief Spiflash capability indicating it requires full page erases before
/// new data can be written
#define BOOTLOADER_STORAGE_IMPL_CAPABILITY_PAGE_ERASE_REQUIRED  (1 << 1)
/// Spiflash capability indicating that the write function is blocking
#define BOOTLOADER_STORAGE_IMPL_CAPABILITY_BLOCKING_WRITE       (1 << 2)
/// Spiflash capability indicating that the erase function is blocking
#define BOOTLOADER_STORAGE_IMPL_CAPABILITY_BLOCKING_ERASE       (1 << 3)

// -----------------------------------------------------------------------------
// Functions

/***************************************************************************//**
 * Get information about the storage plugin.
 *
 * @param[out] info Information about the storage plugin.
 ******************************************************************************/
void bootloader_getStorageInfo(BootloaderStorageInformation_t *info);

/***************************************************************************//**
 * Get information about a storage slot.
 *
 * @param[in]  slotId ID of the slot to get information about
 * @param[out] slot Information about the storage slot
 *
 * @return @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_STORAGE_BASE range
 ******************************************************************************/
int32_t bootloader_getStorageSlotInfo(uint32_t                slotId,
                                      BootloaderStorageSlot_t *slot);

/***************************************************************************//**
 * Read data from a storage slot.
 *
 * @param[in]  slotId  ID of the slot
 * @param[in]  offset Offset into the slot to start reading from
 * @param[out] buffer Buffer to store the data
 * @param[in]  length Amount of data to read
 *
 * @return @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_STORAGE_BASE range
 ******************************************************************************/
int32_t bootloader_readStorage(uint32_t slotId,
                               uint32_t offset,
                               uint8_t  *buffer,
                               size_t   length);

/***************************************************************************//**
 * Write data to a storage slot.
 *
 * @param[in] slotId ID of the slot
 * @param[in] offset Offset into the slot to start writing to
 * @param[in] buffer Buffer to read data to write from
 * @param[in] length Amount of data to write
 *
 * @return @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_STORAGE_BASE range
 ******************************************************************************/
int32_t bootloader_writeStorage(uint32_t slotId,
                                uint32_t offset,
                                uint8_t  *buffer,
                                size_t   length);

/***************************************************************************//**
 * Erase and write data to a storage slot.
 *
 * @note This function automatically erases the following Flash page whenever
 *       the written data crosses a page boundary. In other words, the function
 *       cannot be used to perform multiple sequential writes to the same
 *       address range unless the range starts at a page boundary.
 *       For a sequential write, the first call to this function should have
 *       a start address at a page boundary. Otherwise, the corresponding page
 *       of the starting address needs to be erased explicitly.
 *
 * @param[in] slotId ID of the slot
 * @param[in] offset Offset into the slot to start writing to
 * @param[in] buffer Buffer to read data to write from
 * @param[in] length Amount of data to write
 *
 * @return @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_STORAGE_BASE range
 ******************************************************************************/
int32_t bootloader_eraseWriteStorage(uint32_t slotId,
                                     uint32_t offset,
                                     uint8_t  *buffer,
                                     size_t   length);

/***************************************************************************//**
 * Erase all contents of a storage slot.
 *
 * @param[in] slotId ID of the slot
 *
 * @return @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_STORAGE_BASE range
 ******************************************************************************/
int32_t bootloader_eraseStorageSlot(uint32_t slotId);

/***************************************************************************//**
 * Initialize chunked erase of a storage slot.
 *
 * @note This function must be called before calling
 *       @ref bootloader_chunkedEraseStorageSlot in a loop.
 *
 * @param[in] slotId    ID of the slot
 * @param[in] eraseStat Erase status struct
 *
 * @return @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_STORAGE_BASE range
 ******************************************************************************/
int32_t bootloader_initChunkedEraseStorageSlot(uint32_t                slotId,
                                               BootloaderEraseStatus_t *eraseStat);

/***************************************************************************//**
 * Erase one page from a storage slot according to
 * the struct BootloaderEraseStatus_t.
 *
 * @note @ref bootloader_initChunkedEraseStorageSlot must be called
 *       before calling this function, in order to prepare
 *       BootloaderEraseStatus_t.
 *
 * @note This can be called sequentially to, for example, erase all contents
 *       of a storage slot.
 *
 * @param[in] eraseStat Erase status struct
 *
 * @return @ref BOOTLOADER_ERROR_STORAGE_CONTINUE if erasing a page was
 *         successful. Erase can be continued by calling this function again.
 *         @ref BOOTLOADER_OK if the entire slot has been erased,
 *         else error code in @ref BOOTLOADER_ERROR_STORAGE_BASE range
 ******************************************************************************/
int32_t bootloader_chunkedEraseStorageSlot(BootloaderEraseStatus_t *eraseStat);

/***************************************************************************//**
 * Set a prioritized list of images to attempt to bootload. The last call to
 * this function determines which slot will be installed when
 * @ref bootloader_rebootAndInstall is called.
 *
 * @param[in] slotIds Prioritized list of slot IDs to attempt to bootload. The
 *                    first image to pass verification will be bootloaded.
 * @param[in] length  Length of the slotIds array
 *
 * @return @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_BOOTLOAD_BASE range
 ******************************************************************************/
int32_t bootloader_setImagesToBootload(int32_t *slotIds, size_t length);

/***************************************************************************//**
 * Get the prioritized list of images the bootloader will attempt to bootload.
 *
 * @param[out] slotIds Prioritized list of slot IDs to attempt to bootload. The
 *                     first image to pass verification will be bootloaded.
 * @param[in] length   Length of the slotIds array
 *
 * @return @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_BOOTLOAD_BASE range
 ******************************************************************************/
int32_t bootloader_getImagesToBootload(int32_t *slotIds, size_t length);

/***************************************************************************//**
 * Append a single image to the list of images to attempt to bootload.
 *
 * @param[in] slotId ID of the slot to append
 *
 * @return @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_BOOTLOAD_BASE range
 ******************************************************************************/
int32_t bootloader_appendImageToBootloadList(int32_t slotId);

/***************************************************************************//**
 * Set a single image to attempt to bootload.
 *
 * @param[in] slotId ID of the slot
 *
 * @return @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_BOOTLOAD_BASE range
 ******************************************************************************/
int32_t bootloader_setImageToBootload(int32_t slotId);

/***************************************************************************//**
 * Initialize image verification.
 *
 * Initialize verification of an upgrade image stored in a bootloader storage
 * slot.
 *
 * @note This function must be called before calling
 *       @ref bootloader_continueVerifyImage in a loop.
 *
 * @note The context pointer must point to memory allocated by the caller.
 *       The required size of this context may depend on the version
 *       of the bootloader. The required size for the bootloader associated with
 *       this version of the application interface is given by the define
 *       @ref BOOTLOADER_STORAGE_VERIFICATION_CONTEXT_SIZE.
 *
 * @note Instead of calling @ref bootloader_initVerifyImage followed by
 *       @ref bootloader_continueVerifyImage, call
 *       @ref bootloader_verifyImage if no
 *       time-critical tasks are needed and sufficient stack space is
 *       available for the automatically allocated context. The purpose of the
 *       init-and-continue functions is to allow the caller to service system
 *       needs during verification.
 *
 * @param[in] slotId      ID of the slot to check.
 * @param     context     Pointer to memory used to hold the parser context.
 * @param[in] contextSize Size of the context. An error is returned if the
 *                        supplied context is too small.
 *
 * @return @ref BOOTLOADER_OK if the image parser was initialized, else error
 *         code.
 ******************************************************************************/
int32_t bootloader_initVerifyImage(uint32_t slotId,
                                   void     *context,
                                   size_t   contextSize);

/***************************************************************************//**
 * Continue image verification.
 *
 * Continue verification of an upgrade image stored in a bootloader storage
 * slot.
 *
 * @note This function must be called in a loop until anything other than
 *       @ref BOOTLOADER_ERROR_PARSE_CONTINUE is returned.
 *
 * @note @ref bootloader_initVerifyImage must be called before calling this
 *       function to reset the parser.
 *
 * @note Instead of calling @ref bootloader_initVerifyImage followed by
 *       @ref bootloader_continueVerifyImage, call
 *       @ref bootloader_verifyImage if no
 *       time-critical tasks are needed. The purpose of the
 *       init-and-continue functions is to allow the caller to service system
 *       needs during verification.
 *
 * @param     context          Pointer to a context structure that has
 *                             been initialized by calling
 *                             @ref bootloader_initVerifyImage()
 * @param[in] metadataCallback Function pointer which gets called when
 *                             the binary metadata in the image is currently
 *                             verified. Set to NULL if not required.
 *
 * @return @ref BOOTLOADER_ERROR_PARSE_CONTINUE if parsing was successful, and
 *         the parser expects more data. @ref BOOTLOADER_ERROR_PARSE_SUCCESS if
 *         the parser has successfully parsed the image and it passes
 *         verification. Else error code.
 ******************************************************************************/
int32_t bootloader_continueVerifyImage(void                       *context,
                                       BootloaderParserCallback_t metadataCallback);

/***************************************************************************//**
 * Verify that the image in the given storage slot is valid.
 *
 * @param[in] slotId ID of the slot to check
 * @param[in] metadataCallback Function pointer which gets called when
 *                             binary metadata is present in the storage slot.
 *                             Set to NULL if not required.
 *
 * @note This function allocates a context structure of the size given by
 *       @ref BOOTLOADER_STORAGE_VERIFICATION_CONTEXT_SIZE on the caller's
 *       stack. To manage memory and allocate the
 *       context elsewhere (on the heap, as a global variable, and so on), use
 *       @ref bootloader_initVerifyImage and @ref bootloader_continueVerifyImage
 *       functions instead.
 *
 * @return @ref BOOTLOADER_OK if the image is valid, else error code.
 ******************************************************************************/
int32_t bootloader_verifyImage(uint32_t                   slotId,
                               BootloaderParserCallback_t metadataCallback);

/***************************************************************************//**
 * Get application and bootloader upgrade metadata from the storage slot.
 *
 * @param[in]  slotId            ID of the slot to check
 * @param[out] appInfo           Pointer to @ref ApplicationData_t struct
 * @param[out] bootloaderVersion Pointer to an integer representing bootloader
 *                               version
 *
 * @return @ref BOOTLOADER_OK if metadata was filled successfully
 ******************************************************************************/
int32_t bootloader_getImageInfo(uint32_t          slotId,
                                ApplicationData_t *appInfo,
                                uint32_t          *bootloaderVersion);

/***************************************************************************//**
 * Check whether the bootloader storage is busy.
 *
 * @return True if the storage is busy
 ******************************************************************************/
bool bootloader_storageIsBusy(void);

/***************************************************************************//**
 * Read raw data from storage.
 *
 * @param[in]  address Address to start reading from
 * @param[out] buffer  Buffer to store the data
 * @param[in]  length  Amount of data to read
 *
 * @return @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_STORAGE_BASE range
 ******************************************************************************/
int32_t bootloader_readRawStorage(uint32_t address,
                                  uint8_t  *buffer,
                                  size_t   length);

/***************************************************************************//**
 * Write data to storage.
 *
 * @param[in] address Address to start writing to
 * @param[in] buffer  Buffer to read data to write from
 * @param[in] length  Amount of data to write
 *
 * @return @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_STORAGE_BASE range
 ******************************************************************************/
int32_t bootloader_writeRawStorage(uint32_t address,
                                   uint8_t  *buffer,
                                   size_t   length);

/***************************************************************************//**
 * Erase data from storage.
 *
 * @note Erasing storage must adhere to the limitations of the underlying
 *       storage medium, such as requiring full page erases. Use
 *       @ref bootloader_getStorageInfo to learn the limitations of the
 *       configured storage medium.
 *
 * @param[in] address Address to start erasing from
 * @param[in] length  Length of data to erase
 *
 * @return @ref BOOTLOADER_OK on success, else error code in
 *         @ref BOOTLOADER_ERROR_STORAGE_BASE range
 ******************************************************************************/
int32_t bootloader_eraseRawStorage(uint32_t address, size_t length);

/** @} (end addtogroup StorageInterface) */
/** @} (end addtogroup Interface) */

#endif // BTL_INTERFACE_STORAGE_H
