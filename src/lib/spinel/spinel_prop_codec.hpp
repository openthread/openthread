/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 * @file This file includes definitions for spinel property encoding and decoding functions.
 */

#ifndef SPINEL_PROP_CODEC_HPP_
#define SPINEL_PROP_CODEC_HPP_

#include <openthread/platform/dnssd.h>

#include "lib/spinel/spinel.h"
#include "lib/spinel/spinel_decoder.hpp"
#include "lib/spinel/spinel_encoder.hpp"

namespace ot {
namespace Spinel {

/**
 * Use Spinel::Encoder to encode a Dnssd object.
 *
 * A Spinel header and command MUST have been encoded by the encoder.
 *
 * @param[in] aEncoder    A reference to the encoder object.
 * @param[in] aObj        A reference to the dnssd object. (otPlatDnssdHost, otPlatDnssdService or otPlatDnssdKey).
 * @param[in] aRequestId  The request id.
 * @param[in] aCallback   A callback to receive the dnssd update result.
 */
template <typename DnssdObjType>
otError EncodeDnssd(Encoder                    &aEncoder,
                    const DnssdObjType         &aObj,
                    otPlatDnssdRequestId        aRequestId,
                    otPlatDnssdRegisterCallback aCallback);

/**
 * Use Spinel::Decoder to decode a SPINEL_PROP_DNSSD_HOST message to a otPlatDnssdHost.
 *
 * The decoder MUST have read the header, command and property key of the frame.
 *
 * @param[in]  aDecoder      A reference to the decoder object.
 * @param[out] aHost         A reference to the dnssd host.
 * @param[out] aRequestId    A reference to the request id.
 * @param[out] aCallback     A reference to the pointer to the callback data.
 * @param[out] aCallbackLen  A reference to the callback data length.
 */
otError DecodeDnssdHost(Decoder              &aDecoder,
                        otPlatDnssdHost      &aHost,
                        otPlatDnssdRequestId &aRequestId,
                        const uint8_t       *&aCallbackData,
                        uint16_t             &aCallbackDataLen);

/**
 * Use Spinel::Decoder to decode a SPINEL_PROP_DNSSD_SERVICE message to a otPlatDnssdService.
 *
 * The decoder MUST have read the header, command and property key of the frame.
 *
 * @param[in]  aDecoder        A reference to the decoder object.
 * @param[out] aService        A reference to the dnssd service.
 * @param[out] aRequestId      A reference to the request id.
 * @param[out] aSubTypeLabels  A pointer to the array of sub-type labels.
 * @param[out] aSubTypeCount   The number of sub-type labels.
 * @param[out] aCallback       A reference to the pointer to the callback data.
 * @param[out] aCallbackLen    A reference to the callback data length.
 */
otError DecodeDnssdService(Decoder              &aDecoder,
                           otPlatDnssdService   &aService,
                           const char          **aSubTypeLabels,
                           uint16_t             &aSubTypeLabelsCount,
                           otPlatDnssdRequestId &aRequestId,
                           const uint8_t       *&aCallbackData,
                           uint16_t             &aCallbackDataLen);

/**
 * Use Spinel::Decoder to decode a SPINEL_PROP_DNSSD_KEY_RECORD message to a otPlatDnssdKey.
 *
 * The decoder MUST have read the header, command and property key of the frame.
 *
 * @param[in]  aDecoder      A reference to the decoder object.
 * @param[out] aKey          A reference to the dnssd key.
 * @param[out] aRequestId    A reference to the request id.
 * @param[out] aCallback     A reference to the pointer to the callback data.
 * @param[out] aCallbackLen  A reference to the callback data length.
 */
otError DecodeDnssdKey(Decoder              &aDecoder,
                       otPlatDnssdKey       &aKey,
                       otPlatDnssdRequestId &aRequestId,
                       const uint8_t       *&aCallbackData,
                       uint16_t             &aCallbackDataLen);

} // namespace Spinel
} // namespace ot

#endif // SPINEL_PROP_CODEC_HPP_
