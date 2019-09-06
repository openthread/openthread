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
 * @brief Module that contains frame parsing utilities for the 802.15.4 radio driver.
 *
 */

#ifndef NRF_802154_FRAME_PARSER_H
#define NRF_802154_FRAME_PARSER_H

#include <stdbool.h>
#include <stdint.h>

#define NRF_802154_FRAME_PARSER_INVALID_OFFSET 0xff

/**
 * @biref Structure that contains pointers to parts of MHR and details of MHR structure.
 */
typedef struct
{
    const uint8_t * p_dst_panid;           ///< Pointer to the destination PAN ID field, or NULL if missing.
    const uint8_t * p_dst_addr;            ///< Pointer to the destination address field, or NULL if missing.
    const uint8_t * p_src_panid;           ///< Pointer to the source PAN ID field, or NULL if missing.
    const uint8_t * p_src_addr;            ///< Pointer to the source address field, or NULL if missing.
    const uint8_t * p_sec_ctrl;            ///< Pointer to the security control field, or NULL if missing.
    uint8_t         dst_addr_size;         ///< Size of the destination address field.
    uint8_t         src_addr_size;         ///< Size of the source address field.
    uint8_t         addressing_end_offset; ///< Offset of the first byte following addressing fields.
} nrf_802154_frame_parser_mhr_data_t;

/**
 * @brief Determines if the destination address is extended.
 *
 * @param[in]   p_frame   Pointer to a frame to be checked.
 *
 * @retval  true   Destination address is extended.
 * @retval  false  Destination address is not extended.
 */
bool nrf_802154_frame_parser_dst_addr_is_extended(const uint8_t * p_frame);

/**
 * @brief Gets the destination address from the provided frame.
 *
 * @param[in]   p_frame             Pointer to a frame.
 * @param[out]  p_dst_addr_extended Pointer to a value, which is true if the destination address
 *                                  is extended. Otherwise, it is false.
 *
 * @returns  Pointer to the first byte of the destination address in @p p_frame.
 *           NULL if the destination address cannot be retrieved.
 */
const uint8_t * nrf_802154_frame_parser_dst_addr_get(const uint8_t * p_frame,
                                                     bool          * p_dst_addr_extended);

/**
 * @brief Gets the offset of the destination address field in the provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of the destination address field, including one byte
 *           of the frame length.
 * @returns  Zero if the destination address cannot be retrieved.
 *
 */
uint8_t nrf_802154_frame_parser_dst_addr_offset_get(const uint8_t * p_frame);

/**
 * @brief Gets the destination PAN ID from the provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Pointer to the first byte of the destination PAN ID in @p p_frame.
 * @returns  NULL if the destination PAN ID cannot be retrieved.
 *
 */
const uint8_t * nrf_802154_frame_parser_dst_panid_get(const uint8_t * p_frame);

/**
 * @brief Gets the offset of the destination PAN ID field in the provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of the destination PAN ID field, including one byte
 *           of the frame length.
 * @returns  Zero in case the destination PAN ID cannot be retrieved.
 *
 */
uint8_t nrf_802154_frame_parser_dst_panid_offset_get(const uint8_t * p_frame);

/**
 * @brief Gets the offset of the end of the destination address fields.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset of the first byte following the destination addressing fields in the MHR.
 */
uint8_t nrf_802154_frame_parser_dst_addr_end_offset_get(const uint8_t * p_frame);

/**
 * @brief Determines if the source address is extended.
 *
 * @param[in]   p_frame   Pointer to a frame to check.
 *
 * @retval  true   The source address is extended.
 * @retval  false  The source address is not extended.
 *
 */
bool nrf_802154_frame_parser_src_addr_is_extended(const uint8_t * p_frame);

/**
 * @brief Determines if the source address is short.
 *
 * @param[in]   p_frame   Pointer to a frame to check.
 *
 * @retval  true   The source address is short.
 * @retval  false  The source address is not short.
 *
 */
bool nrf_802154_frame_parser_src_addr_is_short(const uint8_t * p_frame);

/**
 * @brief Gets the source address from the provided frame.
 *
 * @param[in]   p_frame             Pointer to a frame.
 * @param[out]  p_src_addr_extended Pointer to a value, which is true if source address is extended.
 *                                  Otherwise, it is false.
 *
 * @returns  Pointer to the first byte of the source address in @p p_frame.
 * @returns  NULL if the source address cannot be retrieved.
 *
 */
const uint8_t * nrf_802154_frame_parser_src_addr_get(const uint8_t * p_frame,
                                                     bool          * p_src_addr_extended);

/**
 * @brief Gets the offset of the source address field in the provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of the source address field, including one byte of the frame length.
 * @returns  Zero if the source address cannot be retrieved.
 *
 */
uint8_t nrf_802154_frame_parser_src_addr_offset_get(const uint8_t * p_frame);

/**
 * @brief Gets the source PAN ID from the provided frame.
 *
 * @param[in]   p_frame   Pointer to a frame.
 *
 * @returns  Pointer to the first byte of the source PAN ID in @p p_frame.
 * @returns  NULL if the source PAN ID cannot be retrieved or if it is compressed.
 *
 */
const uint8_t * nrf_802154_frame_parser_src_panid_get(const uint8_t * p_frame);

/**
 * @brief Gets the offset of the source PAN ID field in the provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of the source PAN ID field, including one byte of the frame length.
 * @returns  Zero if the source PAN ID cannot be retrieved or is compressed.
 *
 */
uint8_t nrf_802154_frame_parser_src_panid_offset_get(const uint8_t * p_frame);

/**
 * @brief Gets the pointer and the details of MHR parts of a given frame.
 *
 * @param[in]  p_frame   Pointer to a frame to parse.
 * @param[out] p_fields  Pointer to a structure that contains pointers and details
 *                       of the parsed frame.
 *
 * @retval true   Frame parsed correctly.
 * @retval false  Parse error. @p p_fields values are invalid.
 *
 */
bool nrf_802154_frame_parser_mhr_parse(const uint8_t                      * p_frame,
                                       nrf_802154_frame_parser_mhr_data_t * p_fields);

/**
 * @brief Gets the security control field in the provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Pointer to the first byte of the security control field in @p p_frame.
 * @returns  NULL if the security control cannot be retrieved (that is, security is not enabled).
 *
 */
const uint8_t * nrf_802154_frame_parser_sec_ctrl_get(const uint8_t * p_frame);

/**
 * @brief Gets the offset of the first byte after the addressing fields in MHR.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of the first byte after the addressing fields in MHR.
 */
uint8_t nrf_802154_frame_parser_addressing_end_offset_get(const uint8_t * p_frame);

/**
 * @brief Gets the offset of the security control field in the provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of the security control field, including one byte of the frame length.
 * @returns  Zero if the security control cannot be retrieved (that is, security is not enabled).
 *
 */
uint8_t nrf_802154_frame_parser_sec_ctrl_offset_get(const uint8_t * p_frame);

/**
 * @brief Gets the key identifier field in the provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Pointer to the first byte of the key identifier field in @p p_frame.
 * @returns  NULL if the key identifier cannot be retrieved (that is, security is not enabled).
 *
 */
const uint8_t * nrf_802154_frame_parser_key_id_get(const uint8_t * p_frame);

/**
 * @brief Gets the offset of the key identifier field in the provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of the key identifier field, including one byte of the frame length.
 * @returns  Zero if the key identifier cannot be retrieved (that is, security is not enabled).
 *
 */
uint8_t nrf_802154_frame_parser_key_id_offset_get(const uint8_t * p_frame);

/**
 * @brief Determines if the sequence number suppression bit is set.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @retval  true   Sequence number suppression bit is set.
 * @retval  false  Sequence number suppression bit is not set.
 *
 */
bool nrf_802154_frame_parser_dsn_suppress_bit_is_set(const uint8_t * p_frame);

/**
 * @brief Determines if the IE present bit is set.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @retval  true   IE present bit is set.
 * @retval  false  IE present bit is not set.
 *
 */
bool nrf_802154_frame_parser_ie_present_bit_is_set(const uint8_t * p_frame);

/**
 * @brief Determines if the Ack Request (AR) bit is set.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @retval  true   AR bit is set.
 * @retval  false  AR bit is not set.
 *
 */
bool nrf_802154_frame_parser_ar_bit_is_set(const uint8_t * p_frame);

/**
 * @brief Gets the IE header field in the provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Pointer to the first byte of the IE header field in @p p_frame.
 * @returns  NULL if the IE header cannot be retrieved (that is, the IE header is not present).
 *
 */
const uint8_t * nrf_802154_frame_parser_ie_header_get(const uint8_t * p_frame);

/**
 * @brief Gets the offset of the IE header field in the provided frame.
 *
 * @param[in]   p_frame  Pointer to a frame.
 *
 * @returns  Offset in bytes of the IE header field, including one byte of the frame length.
 * @returns  Zero if the IE header cannot be retrieved (that is, the IE header is not present).
 *
 */
uint8_t nrf_802154_frame_parser_ie_header_offset_get(const uint8_t * p_frame);

#endif // NRF_802154_FRAME_PARSER_H
