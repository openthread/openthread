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
#include "common/code_utils.hpp"

namespace ot {
namespace Spinel {

template <>
otError EncodeDnssd<otPlatDnssdHost>(Encoder                    &aEncoder,
                                     const otPlatDnssdHost      &aObj,
                                     otPlatDnssdRequestId        aRequestId,
                                     otPlatDnssdRegisterCallback aCallback)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aEncoder.WriteUintPacked(SPINEL_PROP_DNSSD_HOST));
    SuccessOrExit(error = aEncoder.WriteUtf8(aObj.mHostName == nullptr ? "" : aObj.mHostName));
    SuccessOrExit(error = aEncoder.WriteUint16(aObj.mAddressesLength));
    for (uint16_t i = 0; i < aObj.mAddressesLength; i++)
    {
        SuccessOrExit(error = aEncoder.WriteIp6Address(aObj.mAddresses[i]));
    }
    SuccessOrExit(error = aEncoder.WriteUint32(aRequestId));
    SuccessOrExit(error = aEncoder.WriteData(reinterpret_cast<const uint8_t *>(&aCallback), sizeof(aCallback)));

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

    SuccessOrExit(error = aEncoder.WriteUintPacked(SPINEL_PROP_DNSSD_SERVICE));
    SuccessOrExit(error = aEncoder.WriteUtf8(aObj.mHostName == nullptr ? "" : aObj.mHostName));
    SuccessOrExit(error = aEncoder.WriteUtf8(aObj.mServiceInstance == nullptr ? "" : aObj.mServiceInstance));
    SuccessOrExit(error = aEncoder.WriteUtf8(aObj.mServiceType == nullptr ? "" : aObj.mServiceType));
    SuccessOrExit(error = aEncoder.OpenStruct());
    for (uint16_t i = 0; i < aObj.mSubTypeLabelsLength; i++)
    {
        SuccessOrExit(error = aEncoder.WriteUtf8(aObj.mSubTypeLabels[i]));
    }
    SuccessOrExit(error = aEncoder.CloseStruct());
    SuccessOrExit(error = aEncoder.WriteDataWithLen(aObj.mTxtData, aObj.mTxtDataLength));
    SuccessOrExit(error = aEncoder.WriteUint16(aObj.mPort));
    SuccessOrExit(error = aEncoder.WriteUint16(aObj.mPriority));
    SuccessOrExit(error = aEncoder.WriteUint16(aObj.mWeight));
    SuccessOrExit(error = aEncoder.WriteUint32(aObj.mTtl));
    SuccessOrExit(error = aEncoder.WriteUint32(aRequestId));
    SuccessOrExit(error = aEncoder.WriteData(reinterpret_cast<const uint8_t *>(&aCallback), sizeof(aCallback)));

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

    SuccessOrExit(error = aEncoder.WriteUintPacked(SPINEL_PROP_DNSSD_KEY_RECORD));
    SuccessOrExit(error = aEncoder.WriteUtf8(aObj.mName == nullptr ? "" : aObj.mName));
    SuccessOrExit(error = aEncoder.OpenStruct());
    if (aObj.mServiceType != nullptr)
    {
        SuccessOrExit(error = aEncoder.WriteUtf8(aObj.mServiceType));
    }
    SuccessOrExit(error = aEncoder.CloseStruct());
    SuccessOrExit(error = aEncoder.WriteDataWithLen(aObj.mKeyData, aObj.mKeyDataLength));
    SuccessOrExit(error = aEncoder.WriteUint16(aObj.mClass));
    SuccessOrExit(error = aEncoder.WriteUint32(aObj.mTtl));
    SuccessOrExit(error = aEncoder.WriteUint32(aRequestId));
    SuccessOrExit(error = aEncoder.WriteData(reinterpret_cast<const uint8_t *>(&aCallback), sizeof(aCallback)));

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

    SuccessOrExit(error = aDecoder.ReadUtf8(aHost.mHostName));
    SuccessOrExit(error = aDecoder.ReadUint16(aHost.mAddressesLength));
    SuccessOrExit(error = aDecoder.ReadIp6Address(aHost.mAddresses));
    SuccessOrExit(error = aDecoder.ReadUint32(aRequestId));
    SuccessOrExit(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

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

    SuccessOrExit(error = aDecoder.ReadUtf8(aService.mHostName));
    SuccessOrExit(error = aDecoder.ReadUtf8(aService.mServiceInstance));
    SuccessOrExit(error = aDecoder.ReadUtf8(aService.mServiceType));
    SuccessOrExit(error = aDecoder.OpenStruct());
    while (!aDecoder.IsAllReadInStruct())
    {
        VerifyOrExit(index < aSubTypeLabelsCount, error = OT_ERROR_NO_BUFS);
        SuccessOrExit(error = aDecoder.ReadUtf8(aSubTypeLabels[index]));
        index++;
    }
    aSubTypeLabelsCount = index;
    SuccessOrExit(error = aDecoder.CloseStruct());
    SuccessOrExit(error = aDecoder.ReadDataWithLen(aService.mTxtData, aService.mTxtDataLength));
    SuccessOrExit(error = aDecoder.ReadUint16(aService.mPort));
    SuccessOrExit(error = aDecoder.ReadUint16(aService.mPriority));
    SuccessOrExit(error = aDecoder.ReadUint16(aService.mWeight));
    SuccessOrExit(error = aDecoder.ReadUint32(aService.mTtl));
    SuccessOrExit(error = aDecoder.ReadUint32(aRequestId));
    SuccessOrExit(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

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

    SuccessOrExit(error = aDecoder.ReadUtf8(aKey.mName));
    SuccessOrExit(error = aDecoder.OpenStruct());
    if (!aDecoder.IsAllReadInStruct())
    {
        SuccessOrExit(error = aDecoder.ReadUtf8(aKey.mServiceType));
    }
    else
    {
        aKey.mServiceType = nullptr;
    }
    SuccessOrExit(error = aDecoder.CloseStruct());
    SuccessOrExit(error = aDecoder.ReadDataWithLen(aKey.mKeyData, aKey.mKeyDataLength));
    SuccessOrExit(error = aDecoder.ReadUint16(aKey.mClass));
    SuccessOrExit(error = aDecoder.ReadUint32(aKey.mTtl));
    SuccessOrExit(error = aDecoder.ReadUint32(aRequestId));
    SuccessOrExit(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

template <> otError EncodeDnssdDiscovery<otPlatDnssdBrowser>(Encoder &aEncoder, const otPlatDnssdBrowser &aDiscovery)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aEncoder.WriteUtf8(aDiscovery.mServiceType));
    SuccessOrExit(error = aEncoder.OpenStruct());
    if (aDiscovery.mSubTypeLabel != nullptr)
    {
        SuccessOrExit(error = aEncoder.WriteUtf8(aDiscovery.mSubTypeLabel));
    }
    SuccessOrExit(error = aEncoder.CloseStruct());
    SuccessOrExit(error = aEncoder.WriteUint32(aDiscovery.mInfraIfIndex));
    SuccessOrExit(error = aEncoder.WriteData(reinterpret_cast<const uint8_t *>(&aDiscovery.mCallback),
                                             sizeof(aDiscovery.mCallback)));

exit:
    return error;
}

otError EncodeDnssdBrowseResult(Encoder                       &aEncoder,
                                const otPlatDnssdBrowseResult &aBrowseResult,
                                const uint8_t                 *aCallbackData,
                                uint16_t                       aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aEncoder.WriteUtf8(aBrowseResult.mServiceType));
    SuccessOrExit(error = aEncoder.OpenStruct());
    if (aBrowseResult.mSubTypeLabel != nullptr)
    {
        SuccessOrExit(error = aEncoder.WriteUtf8(aBrowseResult.mSubTypeLabel));
    }
    SuccessOrExit(error = aEncoder.CloseStruct());
    SuccessOrExit(error = aEncoder.WriteUtf8(aBrowseResult.mServiceInstance));
    SuccessOrExit(error = aEncoder.WriteUint32(aBrowseResult.mTtl));
    SuccessOrExit(error = aEncoder.WriteUint32(aBrowseResult.mInfraIfIndex));
    SuccessOrExit(error = aEncoder.WriteData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

otError DecodeDnssdBrowser(Decoder            &aDecoder,
                           otPlatDnssdBrowser &aBrowser,
                           const uint8_t     *&aCallbackData,
                           uint16_t           &aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aDecoder.ReadUtf8(aBrowser.mServiceType));
    SuccessOrExit(error = aDecoder.OpenStruct());
    if (!aDecoder.IsAllReadInStruct())
    {
        SuccessOrExit(error = aDecoder.ReadUtf8(aBrowser.mSubTypeLabel));
    }
    else
    {
        aBrowser.mSubTypeLabel = nullptr;
    }
    SuccessOrExit(error = aDecoder.CloseStruct());
    SuccessOrExit(error = aDecoder.ReadUint32(aBrowser.mInfraIfIndex));
    SuccessOrExit(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

otError DecodeDnssdBrowseResult(Decoder                 &aDecoder,
                                otPlatDnssdBrowseResult &aBrowseResult,
                                const uint8_t          *&aCallbackData,
                                uint16_t                &aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aDecoder.ReadUtf8(aBrowseResult.mServiceType));
    SuccessOrExit(error = aDecoder.OpenStruct());
    if (!aDecoder.IsAllReadInStruct())
    {
        SuccessOrExit(error = aDecoder.ReadUtf8(aBrowseResult.mSubTypeLabel));
    }
    else
    {
        aBrowseResult.mSubTypeLabel = nullptr;
    }
    SuccessOrExit(error = aDecoder.CloseStruct());
    SuccessOrExit(error = aDecoder.ReadUtf8(aBrowseResult.mServiceInstance));
    SuccessOrExit(error = aDecoder.ReadUint32(aBrowseResult.mTtl));
    SuccessOrExit(error = aDecoder.ReadUint32(aBrowseResult.mInfraIfIndex));
    SuccessOrExit(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

template <>
otError EncodeDnssdDiscovery<otPlatDnssdSrvResolver>(Encoder &aEncoder, const otPlatDnssdSrvResolver &aDiscovery)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aEncoder.WriteUtf8(aDiscovery.mServiceInstance));
    SuccessOrExit(error = aEncoder.WriteUtf8(aDiscovery.mServiceType));
    SuccessOrExit(error = aEncoder.WriteUint32(aDiscovery.mInfraIfIndex));
    SuccessOrExit(error = aEncoder.WriteData(reinterpret_cast<const uint8_t *>(&aDiscovery.mCallback),
                                             sizeof(aDiscovery.mCallback)));

exit:
    return error;
}

otError EncodeDnssdSrvResult(Encoder                    &aEncoder,
                             const otPlatDnssdSrvResult &aSrvResult,
                             const uint8_t              *aCallbackData,
                             uint16_t                    aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aEncoder.WriteUtf8(aSrvResult.mServiceInstance));
    SuccessOrExit(error = aEncoder.WriteUtf8(aSrvResult.mServiceType));
    SuccessOrExit(error = aEncoder.OpenStruct());
    if (aSrvResult.mHostName != nullptr)
    {
        SuccessOrExit(error = aEncoder.WriteUtf8(aSrvResult.mHostName));
    }
    SuccessOrExit(error = aEncoder.CloseStruct());
    SuccessOrExit(error = aEncoder.WriteUint16(aSrvResult.mPort));
    SuccessOrExit(error = aEncoder.WriteUint16(aSrvResult.mPriority));
    SuccessOrExit(error = aEncoder.WriteUint16(aSrvResult.mWeight));
    SuccessOrExit(error = aEncoder.WriteUint32(aSrvResult.mTtl));
    SuccessOrExit(error = aEncoder.WriteUint32(aSrvResult.mInfraIfIndex));
    SuccessOrExit(error = aEncoder.WriteData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

otError DecodeDnssdSrvResolver(Decoder                &aDecoder,
                               otPlatDnssdSrvResolver &aSrvResolver,
                               const uint8_t         *&aCallbackData,
                               uint16_t               &aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aDecoder.ReadUtf8(aSrvResolver.mServiceInstance));
    SuccessOrExit(error = aDecoder.ReadUtf8(aSrvResolver.mServiceType));
    SuccessOrExit(error = aDecoder.ReadUint32(aSrvResolver.mInfraIfIndex));
    SuccessOrExit(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

otError DecodeDnssdSrvResult(Decoder              &aDecoder,
                             otPlatDnssdSrvResult &aSrvResult,
                             const uint8_t       *&aCallbackData,
                             uint16_t             &aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aDecoder.ReadUtf8(aSrvResult.mServiceInstance));
    SuccessOrExit(error = aDecoder.ReadUtf8(aSrvResult.mServiceType));
    SuccessOrExit(error = aDecoder.OpenStruct());
    if (!aDecoder.IsAllReadInStruct())
    {
        SuccessOrExit(error = aDecoder.ReadUtf8(aSrvResult.mHostName));
    }
    else
    {
        aSrvResult.mHostName = nullptr;
    }
    SuccessOrExit(error = aDecoder.CloseStruct());
    SuccessOrExit(error = aDecoder.ReadUint16(aSrvResult.mPort));
    SuccessOrExit(error = aDecoder.ReadUint16(aSrvResult.mPriority));
    SuccessOrExit(error = aDecoder.ReadUint16(aSrvResult.mWeight));
    SuccessOrExit(error = aDecoder.ReadUint32(aSrvResult.mTtl));
    SuccessOrExit(error = aDecoder.ReadUint32(aSrvResult.mInfraIfIndex));
    SuccessOrExit(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

template <>
otError EncodeDnssdDiscovery<otPlatDnssdTxtResolver>(Encoder &aEncoder, const otPlatDnssdTxtResolver &aDiscovery)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aEncoder.WriteUtf8(aDiscovery.mServiceInstance));
    SuccessOrExit(error = aEncoder.WriteUtf8(aDiscovery.mServiceType));
    SuccessOrExit(error = aEncoder.WriteUint32(aDiscovery.mInfraIfIndex));
    SuccessOrExit(error = aEncoder.WriteData(reinterpret_cast<const uint8_t *>(&aDiscovery.mCallback),
                                             sizeof(aDiscovery.mCallback)));

exit:
    return error;
}

template <>
otError EncodeDnssdDiscovery<otPlatDnssdAddressResolver>(Encoder &aEncoder, const otPlatDnssdAddressResolver &aDiscovery)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aEncoder.WriteUtf8(aDiscovery.mHostName));
    SuccessOrExit(error = aEncoder.WriteUint32(aDiscovery.mInfraIfIndex));
    SuccessOrExit(error = aEncoder.WriteData(reinterpret_cast<const uint8_t *>(&aDiscovery.mCallback),
                                             sizeof(aDiscovery.mCallback)));

exit:
    return error;
}

otError EncodeDnssdTxtResult(Encoder                    &aEncoder,
                             const otPlatDnssdTxtResult &aTxtResult,
                             const uint8_t              *aCallbackData,
                             uint16_t                    aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aEncoder.WriteUtf8(aTxtResult.mServiceInstance));
    SuccessOrExit(error = aEncoder.WriteUtf8(aTxtResult.mServiceType));
    if (aTxtResult.mTxtData != nullptr && aTxtResult.mTxtDataLength > 0)
    {
        SuccessOrExit(error = aEncoder.WriteDataWithLen(aTxtResult.mTxtData, aTxtResult.mTxtDataLength));
    }
    else
    {
        SuccessOrExit(error = aEncoder.WriteDataWithLen(nullptr, 0));
    }
    SuccessOrExit(error = aEncoder.WriteUint32(aTxtResult.mTtl));
    SuccessOrExit(error = aEncoder.WriteUint32(aTxtResult.mInfraIfIndex));
    SuccessOrExit(error = aEncoder.WriteData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

otError DecodeDnssdTxtResolver(Decoder                &aDecoder,
                               otPlatDnssdTxtResolver &aTxtResolver,
                               const uint8_t         *&aCallbackData,
                               uint16_t               &aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aDecoder.ReadUtf8(aTxtResolver.mServiceInstance));
    SuccessOrExit(error = aDecoder.ReadUtf8(aTxtResolver.mServiceType));
    SuccessOrExit(error = aDecoder.ReadUint32(aTxtResolver.mInfraIfIndex));
    SuccessOrExit(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

otError DecodeDnssdTxtResult(Decoder              &aDecoder,
                             otPlatDnssdTxtResult &aTxtResult,
                             const uint8_t       *&aCallbackData,
                             uint16_t             &aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aDecoder.ReadUtf8(aTxtResult.mServiceInstance));
    SuccessOrExit(error = aDecoder.ReadUtf8(aTxtResult.mServiceType));
    SuccessOrExit(error = aDecoder.ReadDataWithLen(aTxtResult.mTxtData, aTxtResult.mTxtDataLength));
    SuccessOrExit(error = aDecoder.ReadUint32(aTxtResult.mTtl));
    SuccessOrExit(error = aDecoder.ReadUint32(aTxtResult.mInfraIfIndex));
    SuccessOrExit(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

otError EncodeDnssdAddressResult(Encoder                        &aEncoder,
                                 const otPlatDnssdAddressResult &aAddressResult,
                                 const uint8_t                  *aCallbackData,
                                 uint16_t                        aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aEncoder.WriteUtf8(aAddressResult.mHostName));
    SuccessOrExit(error = aEncoder.WriteUint32(aAddressResult.mInfraIfIndex));
    SuccessOrExit(error = aEncoder.WriteUint16(aAddressResult.mAddressesLength));
    for (uint16_t i = 0; i < aAddressResult.mAddressesLength; i++)
    {
        SuccessOrExit(error = aEncoder.WriteIp6Address(aAddressResult.mAddresses[i].mAddress));
        SuccessOrExit(error = aEncoder.WriteUint32(aAddressResult.mAddresses[i].mTtl));
    }
    SuccessOrExit(error = aEncoder.WriteData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

otError DecodeDnssdAddressResolver(Decoder                    &aDecoder,
                                   otPlatDnssdAddressResolver &aAddressResolver,
                                   const uint8_t             *&aCallbackData,
                                   uint16_t                   &aCallbackDataLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aDecoder.ReadUtf8(aAddressResolver.mHostName));
    SuccessOrExit(error = aDecoder.ReadUint32(aAddressResolver.mInfraIfIndex));
    SuccessOrExit(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

otError DecodeDnssdAddressResult(Decoder                  &aDecoder,
                                 otPlatDnssdAddressResult &aAddressResult,
                                 otPlatDnssdAddressAndTtl *aAddressArray,
                                 uint16_t                  aMaxAddresses,
                                 const uint8_t           *&aCallbackData,
                                 uint16_t                 &aCallbackDataLen)
{
    otError   error = OT_ERROR_NONE;
    uint16_t  count;

    SuccessOrExit(error = aDecoder.ReadUtf8(aAddressResult.mHostName));
    SuccessOrExit(error = aDecoder.ReadUint32(aAddressResult.mInfraIfIndex));
    SuccessOrExit(error = aDecoder.ReadUint16(count));
    VerifyOrExit(count <= aMaxAddresses, error = OT_ERROR_PARSE);

    for (uint16_t i = 0; i < count; i++)
    {
        SuccessOrExit(error = aDecoder.ReadIp6Address(aAddressArray[i].mAddress));
        SuccessOrExit(error = aDecoder.ReadUint32(aAddressArray[i].mTtl));
    }

    aAddressResult.mAddresses       = aAddressArray;
    aAddressResult.mAddressesLength = count;
    SuccessOrExit(error = aDecoder.ReadData(aCallbackData, aCallbackDataLen));

exit:
    return error;
}

} // namespace Spinel
} // namespace ot
