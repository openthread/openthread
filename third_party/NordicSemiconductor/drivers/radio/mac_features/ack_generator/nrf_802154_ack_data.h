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
 * @brief This module contains an ACK data generator for nRF 802.15.4 radio driver.
 *
 * @note  Current implementation supports setting pending bit and IEs in 802.15.4-2015 Enh-Ack frames.
 */

#ifndef NRF_802154_ACK_DATA_H
#define NRF_802154_ACK_DATA_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize this module.
 */
void nrf_802154_ack_data_init(void);

/**
 * @brief Enable or disable this module.
 *
 * @param[in]  enabled  True if module should be enabled, false otherwise.
 */
void nrf_802154_ack_data_enable(bool enabled);

/**
 * @brief Add address to ACK data list.
 *
 * ACK frames sent in response to frames with source address matching any address from ACK data list
 * will have appropriate data set. If source address does not match any of the addresses in the
 * list the ACK frame will not have the data set.
 *
 * @param[in]  p_addr    Pointer to address that should be added to the list.
 * @param[in]  extended  Indication if @p p_addr is extended or short address.
 * @param[in]  p_data    Pointer to data to set.
 * @param[in]  data_len  Length of @p p_data buffer.
 * @param[in]  data_type Type of data to set. Please refer to nrf_802154_ack_data_t type.
 *
 * @retval true   Address successfully added to the list.
 * @retval false  Address was not added to the list (list is full).
 */
bool nrf_802154_ack_data_for_addr_set(const uint8_t * p_addr,
                                      bool            extended,
                                      const void    * p_data,
                                      uint8_t         data_len,
                                      uint8_t         data_type);

/**
 * @brief Remove address from ACK data list.
 *
 * ACK frames sent in response to frames with source address matching any address from ACK data list
 * will have appropriate data set. If source address does not match any of the addresses in the
 * list the ACK frame will not have the data set.
 *
 * @param[in]  p_addr    Pointer to address that should be removed from the list.
 * @param[in]  extended  Indication if @p p_addr is extended or short address.
 * @param[in]  data_type Type of data that should be cleared for @p p_addr.
 *
 * @retval true   Address successfully removed from the list.
 * @retval false  Address was not removed from the list (address is missing in the list).
 */
bool nrf_802154_ack_data_for_addr_clear(const uint8_t * p_addr, bool extended, uint8_t data_type);

/**
 * @brief Remove all addresses of given length from ACK data list.
 *
 * @param[in]  extended  Indication if all extended or all short addresses should be removed
 *                       from the list.
 * @param[in]  data_type Type of data that should be cleared for all addresses of given length.
 */
void nrf_802154_ack_data_for_addr_reset(bool extended, uint8_t data_type);

/**
 * @brief Check if pending bit should be set in ACK sent in response to given frame.
 *
 * @param[in]  p_frame  Pointer to a frame to which ACK frame is being prepared.
 *
 * @retval true   Pending bit should be set.
 * @retval false  Pending bit should be cleared.
 */
bool nrf_802154_ack_data_pending_bit_should_be_set(const uint8_t * p_frame);

/**
 * @brief Get IE data stored in the list for source address of provided frame.
 *
 * @param[in]  p_src_addr    Pointer to a source address that is searched for in the list.
 * @param[in]  src_addr_ext  If the source address is extended.
 * @param[out] p_ie_length   Length of the IE data.
 *
 * @returns  Pointer to stored IE data or NULL if IE data should not be set.
 */
const uint8_t * nrf_802154_ack_data_ie_get(const uint8_t * p_src_addr,
                                           bool            src_addr_ext,
                                           uint8_t       * p_ie_length);

#endif // NRF_802154_ACK_DATA_H
