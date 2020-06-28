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
 * @brief Module that implements the incoming frame filter API.
 *
 */

#ifndef NRF_802154_FILTER_H_
#define NRF_802154_FILTER_H_

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_types.h"

/**
 * @defgroup nrf_802154_filter Incoming frame filter API
 * @{
 * @ingroup nrf_802154
 * @brief Procedures used to discard the incoming frames that contain unexpected data in PHR or MHR.
 */

/**
 * @brief Verifies if the given part of the frame is valid.
 *
 * This function is called a few times for each received frame. The first call is after the FCF
 * is received (PSDU length is 2 and @p p_num_bytes value is 3). The subsequent calls are performed
 * when the number of bytes requested by the previous call is available. The iteration ends
 * when the function does not request any more bytes to check.
 * If the verified part of the function is correct, this function returns true and sets
 * @p p_num_bytes to the number of bytes that should be available in PSDU during the next iteration.
 * If the frame is correct and there is nothing more to check, this function returns true
 * and does not modify the @p p_num_bytes value. If the verified frame is incorrect, this function
 * returns false and the @p p_num_bytes value is undefined.
 *
 * @param[in]    p_data       Pointer to a buffer that contains PHR and PSDU of the incoming frame.
 * @param[inout] p_num_bytes  Number of bytes available in @p p_data buffer. This value is either
 *                            set to the requested number of bytes for the next iteration or remains
 *                            unchanged if no more iterations are to be performed during
 *                            the filtering of the given frame.
 *
 * @retval NRF_802154_RX_ERROR_NONE               Verified part of the incoming frame is valid.
 * @retval NRF_802154_RX_ERROR_INVALID_FRAME      Verified part of the incoming frame is invalid.
 * @retval NRF_802154_RX_ERROR_INVALID_DEST_ADDR  Incoming frame has destination address that
 *                                                mismatches the address of this node.
 */
nrf_802154_rx_error_t nrf_802154_filter_frame_part(const uint8_t * p_data, uint8_t * p_num_bytes);

#endif /* NRF_802154_FILTER_H_ */
