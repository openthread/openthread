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
 *   This file implements format helpers for the Vendor Specific Data MLE TLV (type 92).
 */

#include "mle_vendor_specific_tlv.hpp"

#if OPENTHREAD_CONFIG_VENDOR_SPECIFIC_EXTENSIONS_ENABLE

#include "common/code_utils.hpp"
#include "common/numeric_limits.hpp"

namespace ot {
namespace Mle {

Error VendorSpecificDataTlv::AppendTo(Message       &aMessage,
                                      Command        aCommand,
                                      const uint8_t *aOuiBytes,
                                      uint8_t        aOuiLength,
                                      const void    *aVendorData,
                                      uint16_t       aVendorDataLength,
                                      uint16_t       aSingleFrameMaxLength)
{
    Error    error;
    uint16_t valueLength = static_cast<uint16_t>(aOuiLength) + aVendorDataLength;
    uint16_t totalAdded  = sizeof(Tlv) + valueLength;

    VerifyOrExit(aCommand != kCommandAdvertisement, error = kErrorInvalidArgs);

    VerifyOrExit(aOuiLength == kOuiMaLBytes || aOuiLength == kOuiMaMBytes || aOuiLength == kOuiMaSBytes,
                 error = kErrorInvalidArgs);
    VerifyOrExit(valueLength <= NumericLimits<uint8_t>::kMax, error = kErrorInvalidArgs);

    if ((aCommand == kCommandChildIdRequest || aCommand == kCommandChildIdResponse) && aSingleFrameMaxLength != 0)
    {
        VerifyOrExit(aMessage.GetLength() + totalAdded <= aSingleFrameMaxLength, error = kErrorNoBufs);
    }

    SuccessOrExit(error = Tlv::AppendTlvHeader(aMessage, kType, static_cast<uint8_t>(valueLength)));
    SuccessOrExit(error = aMessage.AppendBytes(aOuiBytes, aOuiLength));

    if (aVendorDataLength > 0)
    {
        SuccessOrExit(error = aMessage.AppendBytes(aVendorData, aVendorDataLength));
    }

exit:
    return error;
}

Error VendorSpecificDataTlv::AppendTo(Message    &aMessage,
                                      Command     aCommand,
                                      uint32_t    aOui24,
                                      const void *aVendorData,
                                      uint16_t    aVendorDataLength,
                                      uint16_t    aSingleFrameMaxLength)
{
    uint8_t ouiBytes[kOuiMaLBytes];

    WriteOui24(ouiBytes, aOui24);
    return AppendTo(aMessage, aCommand, ouiBytes, kOuiMaLBytes, aVendorData, aVendorDataLength, aSingleFrameMaxLength);
}

Error VendorSpecificDataTlv::FindValue(const Message &aMessage, OffsetRange &aValueRange)
{
    Error error;

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aMessage, kType, aValueRange));
    VerifyOrExit(aValueRange.GetLength() >= kMinLength, error = kErrorParse);

exit:
    return error;
}

Error VendorSpecificDataTlv::ReadOui24(const Message &aMessage, const OffsetRange &aValueRange, uint32_t &aOui24)
{
    Error   error;
    uint8_t bytes[kOuiMaLBytes];

    VerifyOrExit(aValueRange.GetLength() >= kOuiMaLBytes, error = kErrorParse);
    SuccessOrExit(error = aMessage.Read(aValueRange.GetOffset(), bytes, kOuiMaLBytes));
    aOui24 = ReadOui24(bytes);

exit:
    return error;
}

} // namespace Mle
} // namespace ot

#endif // OPENTHREAD_CONFIG_VENDOR_SPECIFIC_EXTENSIONS_ENABLE
