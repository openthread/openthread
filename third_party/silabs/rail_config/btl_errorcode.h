/***************************************************************************//**
 * @file
 * @brief Error codes used and exposed by the bootloader.
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
#ifndef BTL_ERRORCODE_H
#define BTL_ERRORCODE_H

/**
 * @addtogroup ErrorCodes Error Codes
 * @brief Bootloader error codes
 * @details
 * @{
 */

/// No error, operation OK
#define BOOTLOADER_OK                               0L

/**
 * @addtogroup ErrorBases Error Code Base Values
 * @brief Bootloader error code base values, per logical function
 * @details
 * @{
 */
/// Initialization errors
#define BOOTLOADER_ERROR_INIT_BASE                  0x0100L
/// Image verification errors
#define BOOTLOADER_ERROR_PARSE_BASE                 0x0200L
/// Storage errors
#define BOOTLOADER_ERROR_STORAGE_BASE               0x0400L
/// Bootload errors
#define BOOTLOADER_ERROR_BOOTLOAD_BASE              0x0500L
/// Security errors
#define BOOTLOADER_ERROR_SECURITY_BASE              0x0600L
/// Communication plugin errors
#define BOOTLOADER_ERROR_COMMUNICATION_BASE         0x0700L
/// XMODEM parser errors
#define BOOTLOADER_ERROR_XMODEM_BASE                0x0900L
/// Image file parser errors
#define BOOTLOADER_ERROR_PARSER_BASE                0x1000L
/// SPI slave driver errors
#define BOOTLOADER_ERROR_SPISLAVE_BASE              0x1100L
/// UART driver errors
#define BOOTLOADER_ERROR_UART_BASE                  0x1200L
/// Compression errors
#define BOOTLOADER_ERROR_COMPRESSION_BASE           0x1300L

/** @} addtogroup ErrorBases */

/**
 * @addtogroup InitError Initialization Error Codes
 * @brief Bootloader error codes returned by initialization code.
 * @details
 *    Offset from @ref BOOTLOADER_ERROR_INIT_BASE
 * @{
 */
/// Storage initialization error
#define BOOTLOADER_ERROR_INIT_STORAGE \
  (BOOTLOADER_ERROR_INIT_BASE | 0x01L)
/// Bootloader table invalid
#define BOOTLOADER_ERROR_INIT_TABLE \
  (BOOTLOADER_ERROR_INIT_BASE | 0x02L)

/** @} addtogroup InitError */

/**
 * @addtogroup ParseErrpr Parse Error Codes
 * @brief Bootloader error codes returned by image parsing.
 * @details
 *    Offset from @ref BOOTLOADER_ERROR_PARSE_BASE
 * @{
 */
/// Parse not complete, continue calling the
/// parsing function
#define BOOTLOADER_ERROR_PARSE_CONTINUE   (BOOTLOADER_ERROR_PARSE_BASE | 0x01L)
/// Verification failed
#define BOOTLOADER_ERROR_PARSE_FAILED     (BOOTLOADER_ERROR_PARSE_BASE | 0x02L)
/// Verification successfully completed. Image is valid.
#define BOOTLOADER_ERROR_PARSE_SUCCESS    (BOOTLOADER_ERROR_PARSE_BASE | 0x03L)
/// Bootloader has no storage, and cannot parse images.
#define BOOTLOADER_ERROR_PARSE_STORAGE    (BOOTLOADER_ERROR_PARSE_BASE | 0x04L)
/// Parse context incompatible with parse function
#define BOOTLOADER_ERROR_PARSE_CONTEXT    (BOOTLOADER_ERROR_PARSE_BASE | 0x05L)

/** @} addtogroup VerificationError */

/**
 * @addtogroup StorageError Storage Driver Error Codes
 * @brief Bootloader error codes returned by a storage driver.
 * @details
 *    Offset from @ref BOOTLOADER_ERROR_STORAGE_BASE
 * @{
 */
/// Invalid slot
#define BOOTLOADER_ERROR_STORAGE_INVALID_SLOT \
  (BOOTLOADER_ERROR_STORAGE_BASE | 0x01L)
/// Invalid address. Address not aligned/out of range
#define BOOTLOADER_ERROR_STORAGE_INVALID_ADDRESS \
  (BOOTLOADER_ERROR_STORAGE_BASE | 0x02L)
/// The storage area needs to be erased before it can be used
#define BOOTLOADER_ERROR_STORAGE_NEEDS_ERASE \
  (BOOTLOADER_ERROR_STORAGE_BASE | 0x03L)
/// The address or length needs to be aligned
#define BOOTLOADER_ERROR_STORAGE_NEEDS_ALIGN \
  (BOOTLOADER_ERROR_STORAGE_BASE | 0x04L)
/// An error occured during bootload from storage
#define BOOTLOADER_ERROR_STORAGE_BOOTLOAD \
  (BOOTLOADER_ERROR_STORAGE_BASE | 0x05L)
/// There is no image in this storage slot
#define BOOTLOADER_ERROR_STORAGE_NO_IMAGE \
  (BOOTLOADER_ERROR_STORAGE_BASE | 0x06L)
/// Continue calling function
#define BOOTLOADER_ERROR_STORAGE_CONTINUE \
  (BOOTLOADER_ERROR_STORAGE_BASE | 0x07L)

/** @} addtogroup StorageError */

/**
 * @addtogroup BootloadError Bootloading Error Codes
 * @brief Bootloader error codes returned by the bootloading process.
 * @details
 *    Offset from @ref BOOTLOADER_ERROR_BOOTLOAD_BASE
 * @{
 */
/// No images marked for bootload
#define BOOTLOADER_ERROR_BOOTLOAD_LIST_EMPTY \
  (BOOTLOADER_ERROR_BOOTLOAD_BASE | 0x01L)
/// List of images marked for bootload is full
#define BOOTLOADER_ERROR_BOOTLOAD_LIST_FULL \
  (BOOTLOADER_ERROR_BOOTLOAD_BASE | 0x02L)
/// Image already marked for bootload
#define BOOTLOADER_ERROR_BOOTLOAD_LIST_ENTRY_EXISTS \
  (BOOTLOADER_ERROR_BOOTLOAD_BASE | 0x03L)
/// Bootload list overflowed, requested length too large
#define BOOTLOADER_ERROR_BOOTLOAD_LIST_OVERFLOW \
  (BOOTLOADER_ERROR_BOOTLOAD_BASE | 0x04L)
/// No bootload list found at the base of storage
#define BOOTLOADER_ERROR_BOOTLOAD_LIST_NO_LIST \
  (BOOTLOADER_ERROR_BOOTLOAD_BASE | 0x05L)

/** @} addtogroup BootloadError */

/**
 * @addtogroup SecurityError Security Error Codes
 * @brief Bootloader error codes returned by security algorithms.
 * @details
 *    Offset from @ref BOOTLOADER_ERROR_SECURITY_BASE
 * @{
 */
/// Invalid input parameter to security algorithm
#define BOOTLOADER_ERROR_SECURITY_INVALID_PARAM \
  (BOOTLOADER_ERROR_SECURITY_BASE | 0x01L)
/// Input parameter to security algorithm is out of range
#define BOOTLOADER_ERROR_SECURITY_PARAM_OUT_RANGE \
  (BOOTLOADER_ERROR_SECURITY_BASE | 0x02L)
/// Invalid option for security algorithm
#define BOOTLOADER_ERROR_SECURITY_INVALID_OPTION \
  (BOOTLOADER_ERROR_SECURITY_BASE | 0x03L)
/// Authentication did not check out
#define BOOTLOADER_ERROR_SECURITY_REJECTED \
  (BOOTLOADER_ERROR_SECURITY_BASE | 0x04L)

/** @} addtogroup SecurityError */

/**
 * @addtogroup CommunicationError Communication Plugin Error Codes
 * @brief Bootloader error codes returned by communication plugins.
 * @details
 *    Offset from @ref BOOTLOADER_ERROR_COMMUNICATION_BASE
 * @{
 */
/// Invalid input parameter to security algorithm
/// Could not initialize hardware resources for communication protocol
#define BOOTLOADER_ERROR_COMMUNICATION_INIT \
  (BOOTLOADER_ERROR_COMMUNICATION_BASE | 0x01L)
/// @brief Could not start communication with host (timeout, sync error,
///        version mismatch, ...)
#define BOOTLOADER_ERROR_COMMUNICATION_START \
  (BOOTLOADER_ERROR_COMMUNICATION_BASE | 0x02L)
/// Host closed communication, no image received
#define BOOTLOADER_ERROR_COMMUNICATION_DONE \
  (BOOTLOADER_ERROR_COMMUNICATION_BASE | 0x03L)
/// Unrecoverable error in host-bootloader communication
#define BOOTLOADER_ERROR_COMMUNICATION_ERROR \
  (BOOTLOADER_ERROR_COMMUNICATION_BASE | 0x04L)
/// Host closed communication, no valid image received
#define BOOTLOADER_ERROR_COMMUNICATION_IMAGE_ERROR \
  (BOOTLOADER_ERROR_COMMUNICATION_BASE | 0x05L)
/// Communication aborted, no response from host
#define BOOTLOADER_ERROR_COMMUNICATION_TIMEOUT \
  (BOOTLOADER_ERROR_COMMUNICATION_BASE | 0x06L)

/** @} addtogroup CommunicationError */

/**
 * @addtogroup XmodemError XMODEM Error Codes
 * @brief Bootloader error codes returned by the XMODEM parser.
 * @details
 *    Offset from @ref BOOTLOADER_ERROR_XMODEM_BASE
 * @{
 */
/// Could not verify lower CRC byte
#define BOOTLOADER_ERROR_XMODEM_CRCL \
  (BOOTLOADER_ERROR_XMODEM_BASE | 0x01L)
/// Could not verify upper CRC byte
#define BOOTLOADER_ERROR_XMODEM_CRCH \
  (BOOTLOADER_ERROR_XMODEM_BASE | 0x02L)
/// No start of header found
#define BOOTLOADER_ERROR_XMODEM_NO_SOH \
  (BOOTLOADER_ERROR_XMODEM_BASE | 0x03L)
/// Packet number doesn't match its inverse
#define BOOTLOADER_ERROR_XMODEM_PKTNUM \
  (BOOTLOADER_ERROR_XMODEM_BASE | 0x04L)
/// Packet number error (unexpected sequence)
#define BOOTLOADER_ERROR_XMODEM_PKTSEQ \
  (BOOTLOADER_ERROR_XMODEM_BASE | 0x05L)
/// Packet number error (duplicate)
#define BOOTLOADER_ERROR_XMODEM_PKTDUP \
  (BOOTLOADER_ERROR_XMODEM_BASE | 0x06L)
/// Transfer is done (Technically not an error)
#define BOOTLOADER_ERROR_XMODEM_DONE \
  (BOOTLOADER_ERROR_XMODEM_BASE | 0x07L)
/// Transfer is canceled
#define BOOTLOADER_ERROR_XMODEM_CANCEL \
  (BOOTLOADER_ERROR_XMODEM_BASE | 0x08L)

/** @} addtogroup XmodemError */

/**
 * @addtogroup ParserError Image Parser Error Codes
 * @brief Bootloader error codes returned by the image file parser.
 * @details
 *    Offset from @ref BOOTLOADER_ERROR_PARSER_BASE
 * @{
 */
/// Encountered unexpected data/option
#define BOOTLOADER_ERROR_PARSER_UNEXPECTED \
  (BOOTLOADER_ERROR_PARSER_BASE | 0x01L)
/// Ran out of internal buffer space.
/// Please increase internal buffer size to match biggest header
#define BOOTLOADER_ERROR_PARSER_BUFFER \
  (BOOTLOADER_ERROR_PARSER_BASE | 0x02L)
/// Internal state: done parsing the current input buffer
#define BOOTLOADER_ERROR_PARSER_PARSED \
  (BOOTLOADER_ERROR_PARSER_BASE | 0x03L)
/// Invalid encryption key or no key not present
#define BOOTLOADER_ERROR_PARSER_KEYERROR \
  (BOOTLOADER_ERROR_PARSER_BASE | 0x04L)
/// Invalid checksum
#define BOOTLOADER_ERROR_PARSER_CRC \
  (BOOTLOADER_ERROR_PARSER_BASE | 0x05L)
/// Invalid signature
#define BOOTLOADER_ERROR_PARSER_SIGNATURE \
  (BOOTLOADER_ERROR_PARSER_BASE | 0x06L)
/// Image parsing is already done (or has previously errored out)
#define BOOTLOADER_ERROR_PARSER_EOF \
  (BOOTLOADER_ERROR_PARSER_BASE | 0x07L)
/// Unknown data type in image file
#define BOOTLOADER_ERROR_PARSER_UNKNOWN_TAG \
  (BOOTLOADER_ERROR_PARSER_BASE | 0x08L)
/// Image file version doesn't match with parser
#define BOOTLOADER_ERROR_PARSER_VERSION \
  (BOOTLOADER_ERROR_PARSER_BASE | 0x09L)
/// Image file type doesn't match with parser
#define BOOTLOADER_ERROR_PARSER_FILETYPE \
  (BOOTLOADER_ERROR_PARSER_BASE | 0x0AL)
/// Initialization failed
#define BOOTLOADER_ERROR_PARSER_INIT \
  (BOOTLOADER_ERROR_PARSER_BASE | 0x0BL)
/// Upgrade file was rejected
#define BOOTLOADER_ERROR_PARSER_REJECTED \
  (BOOTLOADER_ERROR_PARSER_BASE | 0x0CL)

/** @} addtogroup ParserError */

/**
 * @addtogroup SpiSlaveError SPI Slave Driver Error Codes
 * @brief Bootloader error codes returned by the SPI slave driver.
 * @details
 *    Offset from @ref BOOTLOADER_ERROR_SPISLAVE_BASE
 * @{
 */
/// Operation not allowed because hardware has not been initialized
#define BOOTLOADER_ERROR_SPISLAVE_UNINIT \
  (BOOTLOADER_ERROR_SPISLAVE_BASE | 0x01)
/// Hardware fail during initialization
#define BOOTLOADER_ERROR_SPISLAVE_INIT \
  (BOOTLOADER_ERROR_SPISLAVE_BASE | 0x02)
/// Invalid argument
#define BOOTLOADER_ERROR_SPISLAVE_ARGUMENT \
  (BOOTLOADER_ERROR_SPISLAVE_BASE | 0x03)
/// Timeout
#define BOOTLOADER_ERROR_SPISLAVE_TIMEOUT \
  (BOOTLOADER_ERROR_SPISLAVE_BASE | 0x04)
/// Buffer overflow condition
#define BOOTLOADER_ERROR_SPISLAVE_OVERFLOW \
  (BOOTLOADER_ERROR_SPISLAVE_BASE | 0x05)
/// Busy condition
#define BOOTLOADER_ERROR_SPISLAVE_BUSY \
  (BOOTLOADER_ERROR_SPISLAVE_BASE | 0x06)

/** @} addtogroup SpiSlaveError */

/**
 * @addtogroup UartError UART Driver Error Codes
 * @brief Bootloader error codes returned by the UART driver.
 * @details
 *    Offset from @ref BOOTLOADER_ERROR_UART_BASE
 * @{
 */
/// Operation not allowed because hardware has not been initialized
#define BOOTLOADER_ERROR_UART_UNINIT        (BOOTLOADER_ERROR_UART_BASE | 0x01)
/// Hardware fail during initialization
#define BOOTLOADER_ERROR_UART_INIT          (BOOTLOADER_ERROR_UART_BASE | 0x02)
/// Invalid argument
#define BOOTLOADER_ERROR_UART_ARGUMENT      (BOOTLOADER_ERROR_UART_BASE | 0x03)
/// Operation timed out
#define BOOTLOADER_ERROR_UART_TIMEOUT       (BOOTLOADER_ERROR_UART_BASE | 0x04)
/// Buffer overflow condition
#define BOOTLOADER_ERROR_UART_OVERFLOW      (BOOTLOADER_ERROR_UART_BASE | 0x05)
/// Busy condition
#define BOOTLOADER_ERROR_UART_BUSY          (BOOTLOADER_ERROR_UART_BASE | 0x06)

/** @} addtogroup UartError */

/**
 * @addtogroup CompressionError Compression Error Codes
 * @brief Bootloader error codes returned by the decompressor
 * @details
 *    Offset from @ref BOOTLOADER_ERROR_COMPRESSION_BASE
 * @{
 */
/// Could not initialize decompressor
#define BOOTLOADER_ERROR_COMPRESSION_INIT \
  (BOOTLOADER_ERROR_COMPRESSION_BASE | 0x01)
/// Invalid decompressor state -- possible invalid input
#define BOOTLOADER_ERROR_COMPRESSION_STATE \
  (BOOTLOADER_ERROR_COMPRESSION_BASE | 0x02)
/// Data error
#define BOOTLOADER_ERROR_COMPRESSION_DATA \
  (BOOTLOADER_ERROR_COMPRESSION_BASE | 0x03)
/// Data length error
#define BOOTLOADER_ERROR_COMPRESSION_DATALEN \
  (BOOTLOADER_ERROR_COMPRESSION_BASE | 0x04)
/// Memory error
#define BOOTLOADER_ERROR_COMPRESSION_MEM \
  (BOOTLOADER_ERROR_COMPRESSION_BASE | 0x05)

/** @} addtogroup CompressionError */

/** @} addtogroup ErrorCodes */

#endif // BTL_ERRORCODE_H
