/* Copyright (c) 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @brief Module that contains an ACK data generator for the nRF 802.15.4 radio driver.
 *
 * @note  The current implementation supports setting pending bit and IEs in 802.15.4-2015 Enh-Ack frames.
 */

#ifndef NRF_802154_ACK_DATA_H
#define NRF_802154_ACK_DATA_H

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_types.h"

/**
 * @brief Initializes the ACK data generator module.
 */
void nrf_802154_ack_data_init(void);

/**
 * @brief Enables or disables the ACK data generator module.
 *
 * @param[in]  enabled  True if the module is to be enabled. False otherwise.
 */
void nrf_802154_ack_data_enable(bool enabled);

/**
 * @brief Adds an address to the ACK data list.
 *
 * ACK frames sent in response to frames with the source address matching any address from the ACK data list
 * will have the appropriate data set. If the source address does not match any of the addresses in the
 * list, the ACK frame will not have the data set.
 *
 * @param[in]  p_addr    Pointer to the address that is to be added to the list.
 * @param[in]  extended  Indication if @p p_addr is an extended address or a short address.
 * @param[in]  data_type Type of data to be set. Refer to the @ref nrf_802154_ack_data_t type.
 * @param[in]  p_data    Pointer to the data to be set.
 * @param[in]  data_len  Length of the @p p_data buffer.
 *
 * @retval true   Address successfully added to the list.
 * @retval false  Address not added to the list (list is full).
 */
bool nrf_802154_ack_data_for_addr_set(const uint8_t * p_addr,
                                      bool            extended,
                                      uint8_t         data_type,
                                      const void    * p_data,
                                      uint8_t         data_len);

/**
 * @brief Removes an address from the ACK data list.
 *
 * ACK frames sent in response to frames with the source address matching any address from
 * the ACK data list will have the appropriate data set. If the source address does not match
 * any of the addresses in the list, the ACK frame will not have the data set.
 *
 * @param[in]  p_addr    Pointer to the address that is to be removed from the list.
 * @param[in]  extended  Indication if @p p_addr is an extended address or a short address.
 * @param[in]  data_type Type of data that is to be cleared for @p p_addr.
 *
 * @retval true   Address successfully removed from the list.
 * @retval false  Address not removed from the list (address is missing from the list).
 */
bool nrf_802154_ack_data_for_addr_clear(const uint8_t * p_addr, bool extended, uint8_t data_type);

/**
 * @brief Removes all addresses of a given length from the ACK data list.
 *
 * @param[in]  extended  Indication if all extended addresses or all short addresses are
 *                       to be removed from the list.
 * @param[in]  data_type Type of data that is to be cleared for all addresses of a given length.
 */
void nrf_802154_ack_data_reset(bool extended, uint8_t data_type);

/**
 * @brief Select the source matching algorithm.
 *
 * @note This function is to be called after the driver initialization, but before the transceiver is enabled.
 *
 * When calling @ref nrf_802154_ack_data_pending_bit_should_be_set, one of several algorithms
 * for source address matching will be chosen. To ensure a specific algorithm is selected,
 * call this function before @ref rf_802154_ack_data_pending_bit_should_be_set.
 *
 * @param[in]  match_method Source matching method to be used.
 */
void nrf_802154_ack_data_src_addr_matching_method_set(nrf_802154_src_addr_match_t match_method);

/**
 * @brief Checks if a pending bit is to be set in the ACK frame sent in response to a given frame.
 *
 * @param[in]  p_frame  Pointer to the frame for which the ACK frame is being prepared.
 *
 * @retval true   Pending bit is to be set.
 * @retval false  Pending bit is to be cleared.
 */
bool nrf_802154_ack_data_pending_bit_should_be_set(const uint8_t * p_frame);

/**
 * @brief Gets the IE data stored in the list for the source address of the provided frame.
 *
 * @param[in]  p_src_addr    Pointer to the source address to search for in the list.
 * @param[in]  src_addr_ext  If the source address is extended.
 * @param[out] p_ie_length   Length of the IE data.
 *
 * @returns  Either pointer to the stored IE data or NULL if the IE data is not to be set.
 */
const uint8_t * nrf_802154_ack_data_ie_get(const uint8_t * p_src_addr,
                                           bool            src_addr_ext,
                                           uint8_t       * p_ie_length);

#endif // NRF_802154_ACK_DATA_H
