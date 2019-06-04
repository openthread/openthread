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
 * @brief This module contatins frame parsing utilities for 802.15.4 radio driver.
 *
 */

#ifndef NRF_802154_FRAME_PARSER_H
#define NRF_802154_FRAME_PARSER_H

#include <stdbool.h>
#include <stdint.h>

#define NRF_802154_FRAME_PARSER_INVALID_OFFSET 0xff

/**
 * @biref Structure containing pointers to parts of MHR and details of MHR structure.
 */
typedef struct
{
    const uint8_t * p_dst_panid;           ///< Pointer to the destination PAN ID field or NULL if missing.
    const uint8_t * p_dst_addr;            ///< Pointer to the destination address field or NULL if missing.
    const uint8_t * p_src_panid;           ///< Pointer to the source PAN ID field or NULL if missing.
    const uint8_t * p_src_addr;            ///< Pointer to the source address field or NULL if missing.
    const uint8_t * p_sec_ctrl;            ///< Pointer to the security control field or NULL if missing.
    uint8_t         dst_addr_size;         ///< Size of destination address field.
    uint8_t         src_addr_size;         ///< Size of source address field.
    uint8_t         addressing_end_offset; ///< Offset of the first byte following addressing fields.
} nrf_802154_frame_parser_mhr_data_t;

/**
 * @brief Determine if destination address is extended.
 *
 * @param[in]   p_frame   Pointer to a frame to check.
 *
 * @retval  true   If destination address is extended.
 * @retval  false  Otherwise.
 */
bool nrf_802154_frame_parser_dst_addr_is_extended(const uint8_t * p_frame);

/**
 * @brief Get destination address from provided frame.
 *
 * @param[in]   p_frame             Pointer to a frame.
 * @param[out]  p_dst_addr_extended Pointer to a value which is true if destination address is extended.
 *                                  Otherwise it is false.
 *
 * @returns  Pointer to the first byte of destination address in @p p_frame.
 *           NULL if destination address cannot be retrieved.
 */
const uint8_t * nrf_802154_frame_parser_dst_addr_get(const uint8_t * p_frame,
                                                     bool          * p_dst_addr_extended);

/**
 * @brief Get offset of destination address field in provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of destination address field including one byte of frame length.
 *           Zero if destination address cannot be retrieved.
 */
uint8_t nrf_802154_frame_parser_dst_addr_offset_get(const uint8_t * p_frame);

/**
 * @brief Get destination PAN ID from provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Pointer to the first byte of destination PAN ID in @p p_frame.
 *           NULL if destination PAN ID cannot be retrieved.
 */
const uint8_t * nrf_802154_frame_parser_dst_panid_get(const uint8_t * p_frame);

/**
 * @brief Get offset of destination PAN ID field in provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of destination PAN ID field including one byte of frame length.
 *           Zero in case the destination PAN ID cannot be retrieved.
 */
uint8_t nrf_802154_frame_parser_dst_panid_offset_get(const uint8_t * p_frame);

/**
 * @brief Get offset of the end of destination address fields.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset of the first byte following destination addressing fields in the MHR.
 */
uint8_t nrf_802154_frame_parser_dst_addr_end_offset_get(const uint8_t * p_frame);

/**
 * @brief Determine if source address is extended.
 *
 * @param[in]   p_frame   Pointer to a frame to check.
 *
 * @retval  true   If source address is extended.
 * @retval  false  Otherwise.
 */
bool nrf_802154_frame_parser_src_addr_is_extended(const uint8_t * p_frame);

/**
 * @brief Determine if source address is short.
 *
 * @param[in]   p_frame   Pointer to a frame to check.
 *
 * @retval  true   If source address is short..
 * @retval  false  Otherwise.
 */
bool nrf_802154_frame_parser_src_addr_is_short(const uint8_t * p_frame);

/**
 * @brief Get source address from provided frame.
 *
 * @param[in]   p_frame             Pointer to a frame.
 * @param[out]  p_src_addr_extended Pointer to a value which is true if source address is extended.
 *                                  Otherwise it is false.
 *
 * @returns  Pointer to the first byte of source address in @p p_frame.
 *           NULL if source address cannot be retrieved.
 */
const uint8_t * nrf_802154_frame_parser_src_addr_get(const uint8_t * p_frame,
                                                     bool          * p_src_addr_extended);

/**
 * @brief Get offset of source address field in provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of source address field including one byte of frame length.
 *           Zero if source address cannot be retrieved.
 */
uint8_t nrf_802154_frame_parser_src_addr_offset_get(const uint8_t * p_frame);

/**
 * @brief Get source PAN ID from provided frame.
 *
 * @param[in]   p_frame   Pointer to a frame.
 *
 * @returns  Pointer to the first byte of source PAN ID in @p p_frame.
 *           NULL if source PAN ID cannot be retrieved or if it is compressed.
 */
const uint8_t * nrf_802154_frame_parser_src_panid_get(const uint8_t * p_frame);

/**
 * @brief Get offset of source PAN ID field in provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of source PAN ID field including one byte of frame length.
 *           Zero in case the source PAN ID cannot be retrieved or it is compressed.
 */
uint8_t nrf_802154_frame_parser_src_panid_offset_get(const uint8_t * p_frame);

/**
 * @brief Get pointer and details of MHR parts of given frame
 *
 * @param[in]  p_frame   Pointer to a frame to parse.
 * @param[out] p_fields  Pointer to a structure containing pointers and details of the parsed frame.
 *
 * @retval true   Frame parsed correctly.
 * @retval false  Parse error. @p p_fields values are invalid.
 */
bool nrf_802154_frame_parser_mhr_parse(const uint8_t                      * p_frame,
                                       nrf_802154_frame_parser_mhr_data_t * p_fields);

/**
 * @brief Get security control field in provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Pointer to the first byte of security control field in @p p_frame.
 *           NULL if security control cannot be retrieved (security not enabled).
 */
const uint8_t * nrf_802154_frame_parser_sec_ctrl_get(const uint8_t * p_frame);

/**
 * @brief Get offset of the first byte after addressing fields in MHR.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of the first byte after addressing fields in MHR.
 */
uint8_t nrf_802154_frame_parser_addressing_end_offset_get(const uint8_t * p_frame);

/**
 * @brief Get offset of security control field in provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of security control field including one byte of frame length.
 *           Zero if security control cannot be retrieved (security not enabled).
 */
uint8_t nrf_802154_frame_parser_sec_ctrl_offset_get(const uint8_t * p_frame);

/**
 * @brief Get key identifier field in provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Pointer to the first byte of key identifier field in @p p_frame.
 *           NULL if key identifier cannot be retrieved (security not enabled).
 */
const uint8_t * nrf_802154_frame_parser_key_id_get(const uint8_t * p_frame);

/**
 * @brief Get offset of key identifier field in provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of key identifier field including one byte of frame length.
 *           Zero if key identifier cannot be retrieved (security not enabled).
 */
uint8_t nrf_802154_frame_parser_key_id_offset_get(const uint8_t * p_frame);

/**
 * @brief Determine if sequence number suppression bit is set.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @retval  true   If sequence number suppression bit is set.
 * @retval  false  Otherwise.
 */
bool nrf_802154_frame_parser_dsn_suppress_bit_is_set(const uint8_t * p_frame);

/**
 * @brief Determine if IE present bit is set.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @retval  true   If IE present bit is set.
 * @retval  false  Otherwise.
 */
bool nrf_802154_frame_parser_ie_present_bit_is_set(const uint8_t * p_frame);

/**
 * @brief Get IE header field in provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Pointer to the first byte of IE header field in @p p_frame.
 *           NULL if IE header cannot be retrieved (IE not present).
 */
const uint8_t * nrf_802154_frame_parser_ie_header_get(const uint8_t * p_frame);

/**
 * @brief Get offset of IE header field in provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of IE header field including one byte of frame length.
 *           Zero if IE header cannot be retrieved (IE not present).
 */
uint8_t nrf_802154_frame_parser_ie_header_offset_get(const uint8_t * p_frame);

#endif // NRF_802154_FRAME_PARSER_H
