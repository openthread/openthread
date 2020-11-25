/**
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 * @defgroup nrf_cc3xx_platform_kmu nrf_cc3xx_platform kmu APIs
 * @ingroup nrf_cc3xx_platform
 * @{
 * @brief The nrf_cc3xx_platform_kmu APIs provides RTOS integration for storing
 *        keys in KMU hardware peripherals
 */
#ifndef NRF_CC3XX_PLATFORM_KMU__
#define NRF_CC3XX_PLATFORM_KMU__

#include <stdint.h>
#include "nrf.h"

#if defined(NRF9160_XXAA) || defined(NRF5340_XXAA_APPLICATION)

/** @brief Constant value representing the default permission to use
 *         when writing a key to KMU.
 *
 * @details This sets up the written key to be non-writable, non-readable
 *          and pushable.
 *
 * @warning Deviating from this mask when setting up permissions may allow
 *          reading the key from CPU, which has security implications.
 */
#define NRF_CC3XX_PLATFORM_KMU_DEFAULT_PERMISSIONS          (0xFFFFFFFCUL)

#if defined(NRF9160_XXAA)

    /** @brief Address of the AES key register in CryptoCell */
    #define NRF_CC3XX_PLATFORM_KMU_AES_ADDR                 (0x50841400UL)

#elif defined(NRF5340_XXAA_APPLICATION)

    /** Address of the AES key register in CryptoCell for 128 bit keys */
    #define NRF_CC3XX_PLATFORM_KMU_AES_ADDR                 (0x50845400UL)

    /** Address of the first 128 bits of AES key in CryptoCell */
    #define NRF_CC3XX_PLATFORM_KMU_AES_ADDR_1               (0x50845400UL)

    /** Address of the subsequent bits of AES key register in CryptoCell HW
     *
     * @note Used only when AES key is larger than 128 bits, in which case
     *       the AES key is split between two slots in KMU
     */
    #define NRF_CC3XX_PLATFORM_KMU_AES_ADDR_2               (0x50845410UL)
#endif

/** @brief Write a 128 bit key into a KMU slot
 *
 * @details This writes a key to KMU with the destination of the subsequent
 *          push operation set to the address of the AES key registers in
 *          Arm CryptoCell.
 *
 * @note    The default mask for permissions is recommended to use.
 *          Please see @ref NRF_CC3XX_PLATFORM_KMU_DEFAULT_PERMISSIONS.
 *
 * @note    Slot 0 and 1 is reserved for KDR use. See
 *          @ref nrf_cc3xx_platform_kmu_write_kdr_slot.
 *
 * @note nRF5340: Keys of 128 bits can use @ref NRF_CC3XX_PLATFORM_KMU_AES_ADDR,
 *       Keys larger than 128 bits must be split up to use two KMU slots.
 *       Use @ref NRF_CC3XX_PLATFORM_KMU_AES_ADDR_1 for the first 128 bits of
 *       the key.
 *       Use @ref NRF_CC3XX_PLATFORM_KMU_AES_ADDR_2 for the subsequent bits of
 *       the key.
 *
 * @param[in]   slot_id     KMU slot ID for the new key (2 - 127).
 * @param[in]   key_addr    Destination address in CryptoCell used for key push.
 * @param[in]   key_perm    Permissions to set for the KMU slot.
 * @param[in]   key         Array with the 128 bit key to put in the KMU slot.
 *
 * @return NRF_CC3XX_PLATFORM_SUCCESS on success, otherwise a negative value.
 */
int nrf_cc3xx_platform_kmu_write_key_slot(
    uint32_t slot_id,
    uint32_t key_addr,
    uint32_t key_perm,
    const uint8_t key[16]);

#endif /* defined(NRF9160_XXAA) || defined(NRF5340_XXAA_APPLICATION) */

#if defined(NRF9160_XXAA)

/** @brief Write a 128 bit AES key into the KMU slot 0 for KDR use
 *
 * @details This writes a key to KMU with the destination of the subsequent
 *          push operation set to the address of the KDR registers in
 *          Arm CryptoCell.
 *
 * @note    The permission set by this function is "non-writable, non-readable
 *          and pushable". Please see
 *          @ref NRF_CC3XX_PLATFORM_KMU_DEFAULT_PERMISSIONS.
 *
 * @param[in]   key         Array with the 128 bit key to put in the KMU slot.
 *
 * @return NRF_CC3XX_PLATFORM_SUCCESS on success, otherwise a negative value.
 */
int nrf_cc3xx_platform_kmu_write_kdr_slot(const uint8_t key[16]);


/** @brief Push the 128 bit AES key in KMU slot 0 (reserved for KDR) into
 *         CryptoCell KDR registers and set LCS state to secure
 *
 *  @note This function must be run once on every boot to load the KDR key
 *        and to set the LCS state to secure.
 *
 * @note The KDR key will be stored in the Always on Domain (AO) untill the next
 *       reset. It is not possible to set the KDR value once the LCS state is
 *       set to secure.
 *
 * @return NRF_CC3XX_PLATFORM_SUCCESS on success, otherwise a negative value.
 */
int nrf_cc3xx_platform_kmu_push_kdr_slot_and_lock(void);

#endif // defined(NRF9160_XXAA)


#if defined(NRF52840_XXAA)

/** @brief Load a unique 128 bit root key into CryptoCell KDR registers and set
 *         CryptoCell LCS state to secure
 *
 * @note This function must be run once on every boot do load an AES key into
 *       KDR. It is recommended that this is done in an immutable bootloader
 *       stage and the page holding the key is ACL read+write protected after
 *       it has been loaded into KDR with this API.
 *
 * @note The KDR key should be a randomly generated unique key.
 *
 * @note The KDR key will be stored in the Always on Domain (AO) until the next
 *       reset. It is not possible to set the KDR value once the LCS state is
 *       set to secure.
 *
 * @param[in]   key     Array with the AES 128 bit key.
 *
 * @return NRF_CC3XX_PLATFORM_SUCCESS on success, otherwise a negative value.
 */
int nrf_cc3xx_platform_kdr_load_key(uint8_t key[16]);

#endif // defined(NRF52840_XXAA)

#endif /* NRF_CC3XX_PLATFORM_KMU__ */

/** @} */
