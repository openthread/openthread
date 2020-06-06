/***************************************************************************//**
 * @file
 * @brief Reset information for the Silicon Labs Gecko bootloader
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
#ifndef BTL_RESET_INFO_H
#define BTL_RESET_INFO_H

#include <stdint.h>

/***************************************************************************//**
 * @addtogroup Interface
 * @{
 * @addtogroup CommonInterface
 * @{
 * @addtogroup ResetInterface Reset Information
 * @brief Passing information when resetting into and out of the bootloader
 * @details
 *   To signal the bootloader to run, the
 *   application needs to write the @ref BootloaderResetCause_t structure
 *   to the first address of RAM, setting @ref BootloaderResetCause_t.reason
 *   to @ref BOOTLOADER_RESET_REASON_BOOTLOAD.
 *
 *   The reset cause is only valid if @ref BootloaderResetCause_t.signature
 *   is set to @ref BOOTLOADER_RESET_SIGNATURE_VALID.
 * @code BootloaderResetCause_t resetCause = {
 *    .reason = BOOTLOADER_RESET_REASON_BOOTLOAD,
 *    .signature = BOOTLOADER_RESET_SIGNATURE_VALID
 *  } @endcode
 *
 *   When the bootloader reboots back into the app, it sets the reset
 *   reason to @ref BOOTLOADER_RESET_REASON_GO if the bootload succeeded,
 *   or @ref BOOTLOADER_RESET_REASON_BADIMAGE if the bootload failed due
 *   to errors when parsing the upgrade image.
 *
 * @note
 *   The reset information is automatically filled out before reset if the
 *   @ref bootloader_rebootAndInstall function is called.
 * @{
 ******************************************************************************/

/// Reset cause of the bootloader
typedef struct {
  /// Reset reason as defined in the [reset information](@ref ResetInterface)
  uint16_t reason;
  /// Signature indicating whether the reset reason is valid
  uint16_t signature;
} BootloaderResetCause_t;

/// Unknown bootloader cause (should never occur)
#define BOOTLOADER_RESET_REASON_UNKNOWN       0x0200u
/// Bootloader caused reset telling app to run
#define BOOTLOADER_RESET_REASON_GO            0x0201u
/// Application requested that bootloader runs
#define BOOTLOADER_RESET_REASON_BOOTLOAD      0x0202u
/// Bootloader detected bad external upgrade image
#define BOOTLOADER_RESET_REASON_BADIMAGE      0x0203u
/// Fatal Error or assert in bootloader
#define BOOTLOADER_RESET_REASON_FATAL         0x0204u
/// Forced bootloader activation
#define BOOTLOADER_RESET_REASON_FORCE         0x0205u
/// OTA Bootloader mode activation
#define BOOTLOADER_RESET_REASON_OTAVALID      0x0206u
/// Bootloader initiated deep sleep
#define BOOTLOADER_RESET_REASON_DEEPSLEEP     0x0207u
/// Application verification failed
#define BOOTLOADER_RESET_REASON_BADAPP        0x0208u
/// Bootloader requested that first stage upgrades main bootloader
#define BOOTLOADER_RESET_REASON_UPGRADE       0x0209u
/// Bootloader timed out waiting for upgrade image
#define BOOTLOADER_RESET_REASON_TIMEOUT       0x020Au

/// Reset signature is valid
#define BOOTLOADER_RESET_SIGNATURE_VALID      0xF00Fu
/// Reset signature is invalid
#define BOOTLOADER_RESET_SIGNATURE_INVALID    0xC33Cu

/** @} (end addtogroup ResetInterface) */
/** @} (end addtogroup CommonInterface) */
/** @} (end addtogroup Interface) */

#endif // BTL_RESET_INFO_H
