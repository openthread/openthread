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
 * @file This file implements spinel property encoding and decoding functions.
 */

#include "lib/spinel/spinel_prop_codec.hpp"
#include "lib/utils/utils.hpp"

namespace ot {
namespace Spinel {

template <>
otError EncodeDnssd<otPlatDnssdHost>(Encoder                    &aEncoder,
                                     const otPlatDnssdHost      &aObj,
                                     otPlatDnssdRequestId        aRequestId,
                                     otPlatDnssdRegisterCallback aCallback)
{
    otError error = OT_ERROR_NONE;

    EXPECT_NO_ERROR(error = aEncoder.WriteUintPacked(SPINEL_PROP_DNSSD_HOST));
    EXPECT_NO_ERROR(error = aEncoder.WriteUtf8(aObj.mHostName == nullptr ? "" : aObj.mHostName));
    EXPECT_NO_ERROR(error = aEncoder.WriteUint16(aObj.mAddressesLength));
    for (uint16_t i = 0; i < aObj.mAddressesLength; i++)
    {
        EXPECT_NO_ERROR(error = aEncoder.WriteIp6Address(aObj.mAddresses[i]));
    }
    EXPECT_NO_ERROR(error = aEncoder.WriteUint32(aRequestId));
    EXPECT_NO_ERROR(error = aEncoder.WriteData(reinterpret_cast<const uint8_t *>(&aCallback), sizeof(aCallback)));

exit:
    return error;
}

template <>
otError EncodeDnssd<otPlatDnssdService>(Encoder                    &aEncoder,
                                        const otPlatDnssdService   &aObj,
                                        otPlatDnssdRequestId        aRequestId,
                                        otPlatDnssdRegisterCallback aCallback)
{
    otError error = OT_ERROR_NONE;

    EXPECT_NO_ERROR(error = aEncoder.WriteUintPacked(SPINEL_PROP_DNSSD_SERVICE));
    EXPECT_NO_ERROR(error = aEncoder.WriteUtf8(aObj.mHostName == nullptr ? "" : aObj.mHostName));
    EXPECT_NO_ERROR(error = aEncoder.WriteUtf8(aObj.mServiceInstance == nullptr ? "" : aObj.mServiceInstance));
    EXPECT_NO_ERROR(error = aEncoder.WriteUtf8(aObj.mServiceType == nullptr ? "" : aObj.mServiceType));
    EXPECT_NO_ERROR(error = aEncoder.OpenStruct());
    for (uint16_t i = 0; i < aObj.mSubTypeLabelsLength; i++)
    {
        EXPECT_NO_ERROR(error = aEncoder.WriteUtf8(aObj.mSubTypeLabels[i]));
    }
    EXPECT_NO_ERROR(error = aEncoder.CloseStruct());
    EXPECT_NO_ERROR(error = aEncoder.WriteDataWithLen(aObj.mTxtData, aObj.mTxtDataLength));
    EXPECT_NO_ERROR(error = aEncoder.WriteUint16(aObj.mPort));
    EXPECT_NO_ERROR(error = aEncoder.WriteUint16(aObj.mPriority));
    EXPECT_NO_ERROR(error = aEncoder.WriteUint16(aObj.mWeight));
    EXPECT_NO_ERROR(error = aEncoder.WriteUint32(aObj.mTtl));
    EXPECT_NO_ERROR(error = aEncoder.WriteUint32(aRequestId));
    EXPECT_NO_ERROR(error = aEncoder.WriteData(reinterpret_cast<const uint8_t *>(&aCallback), sizeof(aCallback)));

exit:
    return error;
}

template <>
otError EncodeDnssd<otPlatDnssdKey>(Encoder                    &aEncoder,
                                    const otPlatDnssdKey       &aObj,
                                    otPlatDnssdRequestId        aRequestId,
                                    otPlatDnssdRegisterCallback aCallback)
{
    otError error = OT_ERROR_NONE;

    EXPECT_NO_ERROR(error = aEncoder.WriteUintPacked(SPINEL_PROP_DNSSD_KEY_RECORD));
    EXPECT_NO_ERROR(error = aEncoder.WriteUtf8(aObj.mName == nullptr ? "" : aObj.mName));
    EXPECT_NO_ERROR(error = aEncoder.OpenStruct());
    if (aObj.mServiceType != nullptr)
    {
        EXPECT_NO_ERROR(error = aEncoder.WriteUtf8(aObj.mServiceType));
    }
    EXPECT_NO_ERROR(error = aEncoder.CloseStruct());
    EXPECT_NO_ERROR(error = aEncoder.WriteDataWithLen(aObj.mKeyData, aObj.mKeyDataLength));
    EXPECT_NO_ERROR(error = aEncoder.WriteUint16(aObj.mClass));
    EXPECT_NO_ERROR(error = aEncoder.WriteUint32(aObj.mTtl));
    EXPECT_NO_ERROR(error = aEncoder.WriteUint32(aRequestId));
    EXPECT_NO_ERROR(error = aEncoder.WriteData(reinterpret_cast<const uint8_t *>(&aCallback), sizeof(aCallback)));

exit:
    return error;
}

otError DecodeDnssdHost(Decoder              &aDecoder,
                        otPlatDnssdHost      &aHost,
                        otPlatDnssdRequestId &aRequestId,
                        const uint8_t       *&aCallbackData,
                        uint16_t             &aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;

    EXPECT_NO_ERROR(error = aDecoder.ReadUtf8(aHost.mHostName));
    EXPECT_NO_ERROR(error = aDecoder.ReadUint16(aHost.mAddressesLength));
    EXPECT_NO_ERROR(error = aDecoder.ReadIp6Address(aHost.mAddresses));
    EXPECT_NO_ERROR(error = aDecoder.ReadUint32(aRequestId));
    EXPECT_NO_ERROR(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

otError DecodeDnssdService(Decoder              &aDecoder,
                           otPlatDnssdService   &aService,
                           const char          **aSubTypeLabels,
                           uint16_t             &aSubTypeLabelsCount,
                           otPlatDnssdRequestId &aRequestId,
                           const uint8_t       *&aCallbackData,
                           uint16_t             &aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;
    uint8_t index = 0;

    EXPECT_NO_ERROR(error = aDecoder.ReadUtf8(aService.mHostName));
    EXPECT_NO_ERROR(error = aDecoder.ReadUtf8(aService.mServiceInstance));
    EXPECT_NO_ERROR(error = aDecoder.ReadUtf8(aService.mServiceType));
    EXPECT_NO_ERROR(error = aDecoder.OpenStruct());
    while (!aDecoder.IsAllReadInStruct())
    {
        EXPECT(index < aSubTypeLabelsCount, error = OT_ERROR_NO_BUFS);
        EXPECT_NO_ERROR(error = aDecoder.ReadUtf8(aSubTypeLabels[index]));
        index++;
    }
    aSubTypeLabelsCount = index;
    EXPECT_NO_ERROR(error = aDecoder.CloseStruct());
    EXPECT_NO_ERROR(error = aDecoder.ReadDataWithLen(aService.mTxtData, aService.mTxtDataLength));
    EXPECT_NO_ERROR(error = aDecoder.ReadUint16(aService.mPort));
    EXPECT_NO_ERROR(error = aDecoder.ReadUint16(aService.mPriority));
    EXPECT_NO_ERROR(error = aDecoder.ReadUint16(aService.mWeight));
    EXPECT_NO_ERROR(error = aDecoder.ReadUint32(aService.mTtl));
    EXPECT_NO_ERROR(error = aDecoder.ReadUint32(aRequestId));
    EXPECT_NO_ERROR(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

otError DecodeDnssdKey(Decoder              &aDecoder,
                       otPlatDnssdKey       &aKey,
                       otPlatDnssdRequestId &aRequestId,
                       const uint8_t       *&aCallbackData,
                       uint16_t             &aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;

    EXPECT_NO_ERROR(error = aDecoder.ReadUtf8(aKey.mName));
    EXPECT_NO_ERROR(error = aDecoder.OpenStruct());
    if (!aDecoder.IsAllReadInStruct())
    {
        EXPECT_NO_ERROR(error = aDecoder.ReadUtf8(aKey.mServiceType));
    }
    else
    {
        aKey.mServiceType = nullptr;
    }
    EXPECT_NO_ERROR(error = aDecoder.CloseStruct());
    EXPECT_NO_ERROR(error = aDecoder.ReadDataWithLen(aKey.mKeyData, aKey.mKeyDataLength));
    EXPECT_NO_ERROR(error = aDecoder.ReadUint16(aKey.mClass));
    EXPECT_NO_ERROR(error = aDecoder.ReadUint32(aKey.mTtl));
    EXPECT_NO_ERROR(error = aDecoder.ReadUint32(aRequestId));
    EXPECT_NO_ERROR(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

} // namespace Spinel
} // namespace ot
