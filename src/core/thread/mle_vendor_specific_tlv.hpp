/*
 *  Copyright (c) 2026, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes format helpers for the Vendor Specific Data MLE TLV (type 92).
 *
 *   Vendor-specific extensions live entirely outside this file: each vendor builds its own data layout on top of
 *   the OUI-prefixed TLV, links against this format helper, and dispatches by OUI on receive. No vendor-specific
 *   logic should be added here, and core OT files should not reference vendors directly. This isolation lets
 *   multiple independent vendor extensions coexist on the same device and decode each other's TLVs by OUI.
 */

#ifndef OT_CORE_THREAD_MLE_VENDOR_SPECIFIC_TLV_HPP_
#define OT_CORE_THREAD_MLE_VENDOR_SPECIFIC_TLV_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_VENDOR_SPECIFIC_EXTENSIONS_ENABLE

#include "common/error.hpp"
#include "common/message.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"

namespace ot {
namespace Mle {

/**
 * Format helpers for the Vendor Specific Data TLV (MLE TLV type 92).
 */
class VendorSpecificDataTlv
{
public:
    static constexpr uint8_t kType        = Tlv::kVendorSpecificData; ///< MLE TLV type (92).
    static constexpr uint8_t kMinLength   = 3;                        ///< Minimum TLV value length (MA-L OUI only).
    static constexpr uint8_t kOuiMaLBytes = 3;                        ///< OUI bytes for MA-L assignment.
    static constexpr uint8_t kOuiMaMBytes = 4;                        ///< OUI bytes for MA-M assignment.
    static constexpr uint8_t kOuiMaSBytes = 5;                        ///< OUI bytes for MA-S assignment.

    /**
     * Appends a Vendor Specific Data TLV to a message.
     *
     * @param[in,out] aMessage              The message to append the TLV to.
     * @param[in]     aCommand              The MLE command type the message is being constructed for.
     * @param[in]     aOuiBytes             OUI bytes in network byte order (3/4/5 for MA-L/MA-M/MA-S).
     * @param[in]     aOuiLength            OUI length in octets: 3, 4, or 5.
     * @param[in]     aVendorData           Pointer to vendor-defined data (may be `nullptr` if length is 0).
     * @param[in]     aVendorDataLength     Length of vendor-defined data in octets.
     * @param[in]     aSingleFrameMaxLength Optional total-message-length budget for the single-frame
     *                                      fragmentation check on Child ID Request / Response. A value of 0
     *                                      disables the check.
     *
     * @retval kErrorNone         Successfully appended the TLV.
     * @retval kErrorInvalidArgs  @p aOuiLength is not 3/4/5, or @p aCommand is `kCommandAdvertisement`.
     * @retval kErrorNoBufs       The message buffer is full, or the resulting message would exceed
     *                            @p aSingleFrameMaxLength on a Child ID Request / Response.
     */
    static Error AppendTo(Message       &aMessage,
                          Command        aCommand,
                          const uint8_t *aOuiBytes,
                          uint8_t        aOuiLength,
                          const void    *aVendorData,
                          uint16_t       aVendorDataLength,
                          uint16_t       aSingleFrameMaxLength = 0);

    /**
     * Appends a Vendor Specific Data TLV with a 24-bit MA-L OUI and vendor-defined data.
     *
     * @param[in,out] aMessage              The message to append the TLV to.
     * @param[in]     aCommand              The MLE command type the message is being constructed for.
     * @param[in]     aOui24                The 24-bit MA-L OUI value (e.g., 0x641666).
     * @param[in]     aVendorData           Pointer to vendor-defined data.
     * @param[in]     aVendorDataLength     Length of vendor-defined data in octets.
     * @param[in]     aSingleFrameMaxLength Optional single-frame budget for Child ID Request / Response.
     *
     * @retval kErrorNone         Successfully appended the TLV.
     * @retval kErrorInvalidArgs  @p aCommand is `kCommandAdvertisement`.
     * @retval kErrorNoBufs       Insufficient buffer or single-frame budget exceeded.
     */
    static Error AppendTo(Message    &aMessage,
                          Command     aCommand,
                          uint32_t    aOui24,
                          const void *aVendorData,
                          uint16_t    aVendorDataLength,
                          uint16_t    aSingleFrameMaxLength = 0);

    /**
     * Locates the Vendor Specific Data TLV in a message and returns the value offset range.
     *
     * @param[in]  aMessage     The message to search.
     * @param[out] aValueRange  Receives the value offset range of the TLV (OUI bytes + vendor data).
     *
     * @retval kErrorNone      Successfully located the TLV.
     * @retval kErrorNotFound  No Vendor Specific Data TLV in the message.
     * @retval kErrorParse     The TLV value is shorter than the minimum (3 octets).
     */
    static Error FindValue(const Message &aMessage, OffsetRange &aValueRange);

    /**
     * Writes the 3-octet MA-L OUI prefix to a buffer in network byte order.
     *
     * @param[out] aBuffer  Buffer of at least 3 octets.
     * @param[in]  aOui24   The 24-bit OUI value (e.g., 0x641666).
     */
    static void WriteOui24(uint8_t *aBuffer, uint32_t aOui24)
    {
        aBuffer[0] = static_cast<uint8_t>((aOui24 >> 16) & 0xff);
        aBuffer[1] = static_cast<uint8_t>((aOui24 >> 8) & 0xff);
        aBuffer[2] = static_cast<uint8_t>(aOui24 & 0xff);
    }

    /**
     * Reads the 3-octet OUI prefix from a buffer in network byte order.
     *
     * @param[in] aBuffer  Buffer of at least 3 octets.
     *
     * @returns The 24-bit OUI prefix.
     */
    static uint32_t ReadOui24(const uint8_t *aBuffer)
    {
        return (static_cast<uint32_t>(aBuffer[0]) << 16) | (static_cast<uint32_t>(aBuffer[1]) << 8) |
               static_cast<uint32_t>(aBuffer[2]);
    }

    /**
     * Reads the 24-bit OUI prefix from the start of a TLV value range in the message.
     *
     * @param[in]  aMessage     The message containing the TLV.
     * @param[in]  aValueRange  The TLV value offset range.
     * @param[out] aOui24       The 24-bit OUI prefix (network byte order).
     *
     * @retval kErrorNone   Successfully read the OUI prefix.
     * @retval kErrorParse  Value range is shorter than 3 octets.
     */
    static Error ReadOui24(const Message &aMessage, const OffsetRange &aValueRange, uint32_t &aOui24);
};

} // namespace Mle
} // namespace ot

#endif // OPENTHREAD_CONFIG_VENDOR_SPECIFIC_EXTENSIONS_ENABLE

#endif // OT_CORE_THREAD_MLE_VENDOR_SPECIFIC_TLV_HPP_
