/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file implements the TCAT Agent service.
 */

#include "tcat_agent.hpp"
#include <openthread/tcat.h>

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/locator_getters.hpp"
#include "common/string.hpp"
#include "instance/instance.hpp"
#include "radio/radio.hpp"
#include "thread/thread_netif.hpp"
#include "thread/uri_paths.hpp"
#include "utils/otns.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("TcatAgent");

bool TcatAgent::VendorInfo::IsValid(void) const
{
    return mProvisioningUrl == nullptr || IsValidUtf8String(mProvisioningUrl) || mPskdString != nullptr;
}

TcatAgent::TcatAgent(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mVendorInfo(nullptr)
    , mCurrentApplicationProtocol(kApplicationProtocolNone)
    , mState(kStateDisabled)
    , mCommissionerHasNetworkName(false)
    , mCommissionerHasDomainName(false)
    , mCommissionerHasExtendedPanId(false)
{
    mJoinerPskd.Clear();
    mCurrentServiceName[0] = 0;
}

Error TcatAgent::Start(AppDataReceiveCallback aAppDataReceiveCallback, JoinCallback aHandler, void *aContext)
{
    Error error = kErrorNone;

    LogInfo("Starting");
    VerifyOrExit(mVendorInfo != nullptr, error = kErrorFailed);
    mAppDataReceiveCallback.Set(aAppDataReceiveCallback, aContext);
    mJoinCallback.Set(aHandler, aContext);

    mCurrentApplicationProtocol = kApplicationProtocolNone;
    mState                      = kStateEnabled;

exit:
    LogWarnOnError(error, "start TCAT agent");
    return error;
}

void TcatAgent::Stop(void)
{
    mCurrentApplicationProtocol = kApplicationProtocolNone;
    mState                      = kStateDisabled;
    mAppDataReceiveCallback.Clear();
    mJoinCallback.Clear();
    LogInfo("TCAT agent stopped");
}

Error TcatAgent::SetTcatVendorInfo(const VendorInfo &aVendorInfo)
{
    Error error = kErrorNone;

    VerifyOrExit(aVendorInfo.IsValid(), error = kErrorInvalidArgs);
    SuccessOrExit(error = mJoinerPskd.SetFrom(aVendorInfo.mPskdString));
    mVendorInfo = &aVendorInfo;

exit:
    return error;
}

Error TcatAgent::Connected(MeshCoP::SecureTransport &aTlsContext)
{
    size_t len;
    Error  error;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);
    len = sizeof(mCommissionerAuthorizationField);
    SuccessOrExit(
        error = aTlsContext.GetThreadAttributeFromPeerCertificate(
            kCertificateAuthorizationField, reinterpret_cast<uint8_t *>(&mCommissionerAuthorizationField), &len));
    VerifyOrExit(len == sizeof(mCommissionerAuthorizationField), error = kErrorParse);
    VerifyOrExit((mCommissionerAuthorizationField.mHeader & kCommissionerFlag) == 1, error = kErrorParse);

    len = sizeof(mDeviceAuthorizationField);
    SuccessOrExit(error = aTlsContext.GetThreadAttributeFromOwnCertificate(
                      kCertificateAuthorizationField, reinterpret_cast<uint8_t *>(&mDeviceAuthorizationField), &len));
    VerifyOrExit(len == sizeof(mDeviceAuthorizationField), error = kErrorParse);
    VerifyOrExit((mDeviceAuthorizationField.mHeader & kCommissionerFlag) == 0, error = kErrorParse);

    mCommissionerHasDomainName    = false;
    mCommissionerHasNetworkName   = false;
    mCommissionerHasExtendedPanId = false;

    len = sizeof(mCommissionerDomainName) - 1;
    if (aTlsContext.GetThreadAttributeFromPeerCertificate(
            kCertificateDomainName, reinterpret_cast<uint8_t *>(&mCommissionerDomainName), &len) == kErrorNone)
    {
        mCommissionerDomainName.m8[len] = '\0';
        mCommissionerHasDomainName      = true;
    }

    len = sizeof(mCommissionerNetworkName) - 1;
    if (aTlsContext.GetThreadAttributeFromPeerCertificate(
            kCertificateNetworkName, reinterpret_cast<uint8_t *>(&mCommissionerNetworkName), &len) == kErrorNone)
    {
        mCommissionerNetworkName.m8[len] = '\0';
        mCommissionerHasNetworkName      = true;
    }

    len = sizeof(mCommissionerExtendedPanId);
    if (aTlsContext.GetThreadAttributeFromPeerCertificate(
            kCertificateExtendedPanId, reinterpret_cast<uint8_t *>(&mCommissionerExtendedPanId), &len) == kErrorNone)
    {
        if (len == sizeof(mCommissionerExtendedPanId))
        {
            mCommissionerHasExtendedPanId = true;
        }
    }

    mCurrentApplicationProtocol = kApplicationProtocolNone;
    mCurrentServiceName[0]      = 0;
    mState                      = kStateConnected;
    LogInfo("TCAT agent connected");

exit:
    return error;
}

void TcatAgent::Disconnected(void)
{
    mCurrentApplicationProtocol = kApplicationProtocolNone;

    if (mState != kStateDisabled)
    {
        mState = kStateEnabled;
    }

    LogInfo("TCAT agent disconnected");
}

bool TcatAgent::CheckCommandClassAuthorizationFlags(CommandClassFlags aCommissionerCommandClassFlags,
                                                    CommandClassFlags aDeviceCommandClassFlags,
                                                    Dataset          *aDataset) const
{
    bool authorized                     = false;
    bool additionalDeviceRequirementMet = false;
    bool domainNamesMatch               = false;
    bool networkNamesMatch              = false;
    bool extendedPanIdsMatch            = false;

    VerifyOrExit(IsConnected());
    VerifyOrExit(aCommissionerCommandClassFlags & kAccessFlag);

    if (aDeviceCommandClassFlags & kAccessFlag)
    {
        additionalDeviceRequirementMet = true;
    }

    if (aDeviceCommandClassFlags & kPskdFlag)
    {
        additionalDeviceRequirementMet = true;
    }

    if (aDeviceCommandClassFlags & kPskcFlag)
    {
        additionalDeviceRequirementMet = true;
    }

    if (mCommissionerHasNetworkName || mCommissionerHasExtendedPanId)
    {
        Dataset::Info datasetInfo;
        Error         datasetError = kErrorNone;

        if (aDataset == nullptr)
        {
            datasetError = Get<ActiveDatasetManager>().Read(datasetInfo);
        }
        else
        {
            aDataset->ConvertTo(datasetInfo);
        }

        if (datasetError == kErrorNone)
        {
            if (datasetInfo.IsPresent<Dataset::kNetworkName>() && mCommissionerHasNetworkName &&
                (datasetInfo.Get<Dataset::kNetworkName>() == mCommissionerNetworkName))
            {
                networkNamesMatch = true;
            }

            if (datasetInfo.IsPresent<Dataset::kExtendedPanId>() && mCommissionerHasExtendedPanId &&
                (datasetInfo.Get<Dataset::kExtendedPanId>() == mCommissionerExtendedPanId))
            {
                extendedPanIdsMatch = true;
            }
        }
    }

    if (!networkNamesMatch)
    {
        VerifyOrExit((aCommissionerCommandClassFlags & kNetworkNameFlag) == 0);
    }
    else if (aDeviceCommandClassFlags & kNetworkNameFlag)
    {
        additionalDeviceRequirementMet = true;
    }

    if (!extendedPanIdsMatch)
    {
        VerifyOrExit((aCommissionerCommandClassFlags & kExtendedPanIdFlag) == 0);
    }
    else if (aDeviceCommandClassFlags & kExtendedPanIdFlag)
    {
        additionalDeviceRequirementMet = true;
    }

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    VerifyOrExit((aCommissionerCommandClassFlags & kThreadDomainFlag) == 0);
#endif

    if (!domainNamesMatch)
    {
        VerifyOrExit((aCommissionerCommandClassFlags & kThreadDomainFlag) == 0);
    }
    else if (aDeviceCommandClassFlags & kThreadDomainFlag)
    {
        additionalDeviceRequirementMet = true;
    }

    if (additionalDeviceRequirementMet)
    {
        authorized = true;
    }

exit:
    return authorized;
}

bool TcatAgent::IsCommandClassAuthorized(CommandClass aCommandClass) const
{
    bool authorized = false;

    switch (aCommandClass)
    {
    case kGeneral:
        authorized = true;
        break;

    case kCommissioning:
        authorized = CheckCommandClassAuthorizationFlags(mCommissionerAuthorizationField.mCommissioningFlags,
                                                         mDeviceAuthorizationField.mCommissioningFlags, nullptr);
        break;

    case kExtraction:
        authorized = CheckCommandClassAuthorizationFlags(mCommissionerAuthorizationField.mExtractionFlags,
                                                         mDeviceAuthorizationField.mExtractionFlags, nullptr);
        break;

    case kTlvDecommissioning:
        authorized = CheckCommandClassAuthorizationFlags(mCommissionerAuthorizationField.mDecommissioningFlags,
                                                         mDeviceAuthorizationField.mDecommissioningFlags, nullptr);
        break;

    case kApplication:
        authorized = CheckCommandClassAuthorizationFlags(mCommissionerAuthorizationField.mApplicationFlags,
                                                         mDeviceAuthorizationField.mApplicationFlags, nullptr);
        break;

    case kInvalid:
        authorized = false;
        break;
    }

    return authorized;
}

TcatAgent::CommandClass TcatAgent::GetCommandClass(uint8_t aTlvType) const
{
    static constexpr int kGeneralTlvs            = 0x1F;
    static constexpr int kCommissioningTlvs      = 0x3F;
    static constexpr int kExtractionTlvs         = 0x5F;
    static constexpr int kTlvDecommissioningTlvs = 0x7F;
    static constexpr int kApplicationTlvs        = 0x9F;

    if (aTlvType <= kGeneralTlvs)
    {
        return kGeneral;
    }
    else if (aTlvType <= kCommissioningTlvs)
    {
        return kCommissioning;
    }
    else if (aTlvType <= kExtractionTlvs)
    {
        return kExtraction;
    }
    else if (aTlvType <= kTlvDecommissioningTlvs)
    {
        return kTlvDecommissioning;
    }
    else if (aTlvType <= kApplicationTlvs)
    {
        return kApplication;
    }
    else
    {
        return kInvalid;
    }
}

bool TcatAgent::CanProcessTlv(uint8_t aTlvType) const
{
    CommandClass tlvCommandClass = GetCommandClass(aTlvType);
    return IsCommandClassAuthorized(tlvCommandClass);
}

Error TcatAgent::HandleSingleTlv(const Message &aIncomingMessage, Message &aOutgoingMessage)
{
    Error    error = kErrorParse;
    ot::Tlv  tlv;
    uint16_t offset = aIncomingMessage.GetOffset();
    uint16_t length;
    bool     response = false;

    VerifyOrExit(IsConnected(), error = kErrorInvalidState);
    SuccessOrExit(error = aIncomingMessage.Read(offset, tlv));

    if (tlv.IsExtended())
    {
        ot::ExtendedTlv extTlv;
        SuccessOrExit(error = aIncomingMessage.Read(offset, extTlv));
        length = extTlv.GetLength();
        offset += sizeof(ot::ExtendedTlv);
    }
    else
    {
        length = tlv.GetLength();
        offset += sizeof(ot::Tlv);
    }

    if (!CanProcessTlv(tlv.GetType()))
    {
        error = kErrorRejected;
    }
    else
    {
        switch (tlv.GetType())
        {
        case kTlvDisconnect:
            error = kErrorAbort;
            break;

        case kTlvSetActiveOperationalDataset:
            error = HandleSetActiveOperationalDataset(aIncomingMessage, offset, length);
            break;

        case kTlvStartThreadInterface:
            error = HandleStartThreadInterface();
            break;

        case kTlvStopThreadInterface:
            error = otThreadSetEnabled(&GetInstance(), false);
            break;

        case kTlvSendApplicationData:
            LogInfo("Application data len:%d, offset:%d", length, offset);
            mAppDataReceiveCallback.InvokeIfSet(&GetInstance(), &aIncomingMessage, offset,
                                                MapEnum(mCurrentApplicationProtocol), mCurrentServiceName);
            response = true;
            error    = kErrorNone;
            break;
        case kTlvDecommission:
            error = HandleDecomission();
            break;
        case kTlvPing:
            error = HandlePing(aIncomingMessage, aOutgoingMessage, offset, length);
            if (error == kErrorNone)
            {
                response = true;
            }
            break;
        default:
            error = kErrorInvalidCommand;
        }
    }

    if (!response)
    {
        StatusCode statusCode;

        switch (error)
        {
        case kErrorNone:
            statusCode = kStatusSuccess;
            break;

        case kErrorInvalidState:
            statusCode = kStatusUndefined;
            break;

        case kErrorParse:
            statusCode = kStatusParseError;
            break;

        case kErrorInvalidCommand:
            statusCode = kStatusUnsupported;
            break;

        case kErrorRejected:
            statusCode = kStatusUnauthorized;
            break;

        case kErrorNotImplemented:
            statusCode = kStatusUnsupported;
            break;

        default:
            statusCode = kStatusGeneralError;
            break;
        }

        SuccessOrExit(error = ot::Tlv::Append<ResponseWithStatusTlv>(aOutgoingMessage, statusCode));
    }

exit:
    return error;
}

Error TcatAgent::HandleSetActiveOperationalDataset(const Message &aIncomingMessage, uint16_t aOffset, uint16_t aLength)
{
    Dataset     dataset;
    OffsetRange offsetRange;
    Error       error;

    offsetRange.Init(aOffset, aLength);
    SuccessOrExit(error = dataset.SetFrom(aIncomingMessage, offsetRange));
    SuccessOrExit(error = dataset.ValidateTlvs());

    if (!CheckCommandClassAuthorizationFlags(mCommissionerAuthorizationField.mApplicationFlags,
                                             mDeviceAuthorizationField.mApplicationFlags, &dataset))
    {
        error = kErrorRejected;
        ExitNow();
    }

    Get<ActiveDatasetManager>().SaveLocal(dataset);

exit:
    return error;
}

Error TcatAgent::HandleDecomission(void)
{
    Error error = kErrorNone;

    IgnoreReturnValue(otThreadSetEnabled(&GetInstance(), false));
    Get<ActiveDatasetManager>().Clear();
    Get<PendingDatasetManager>().Clear();

    error = Get<Instance>().ErasePersistentInfo();

#if !OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    {
        NetworkKey networkKey;
        networkKey.Clear();
        Get<KeyManager>().SetNetworkKey(networkKey);
    }
#endif

    return error;
}

Error TcatAgent::HandlePing(const Message &aIncomingMessage,
                            Message       &aOutgoingMessage,
                            uint16_t       aOffset,
                            uint16_t       aLength)
{
    Error           error = kErrorNone;
    ot::ExtendedTlv extTlv;
    ot::Tlv         tlv;

    VerifyOrExit(aLength <= kPingPayloadMaxLength, error = kErrorParse);
    if (aLength > ot::Tlv::kBaseTlvMaxLength)
    {
        extTlv.SetType(kTlvResponseWithPayload);
        extTlv.SetLength(aLength);
        SuccessOrExit(error = aOutgoingMessage.Append(extTlv));
    }
    else
    {
        tlv.SetType(kTlvResponseWithPayload);
        tlv.SetLength(static_cast<uint8_t>(aLength));
        SuccessOrExit(error = aOutgoingMessage.Append(tlv));
    }

    SuccessOrExit(error = aOutgoingMessage.AppendBytesFromMessage(aIncomingMessage, aOffset, aLength));

exit:
    return error;
}

Error TcatAgent::HandleStartThreadInterface(void)
{
    Error         error;
    Dataset::Info datasetInfo;

    VerifyOrExit(Get<ActiveDatasetManager>().Read(datasetInfo) == kErrorNone, error = kErrorInvalidState);
    VerifyOrExit(datasetInfo.IsPresent<Dataset::kNetworkKey>(), error = kErrorInvalidState);

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    VerifyOrExit(!Get<Mac::LinkRaw>().IsEnabled(), error = kErrorInvalidState);
#endif

    Get<ThreadNetif>().Up();
    error = Get<Mle::MleRouter>().Start();

exit:
    return error;
}

void SeralizeTcatAdvertisementTlv(uint8_t                 *aBuffer,
                                  uint16_t                &aOffset,
                                  TcatAdvertisementTlvType aType,
                                  uint16_t                 aLength,
                                  const uint8_t           *aValue)
{
    aBuffer[aOffset++] = static_cast<uint8_t>(aType << 4 | (aLength & 0xf));
    memcpy(aBuffer + aOffset, aValue, aLength);
    aOffset += aLength;
}

Error TcatAgent::GetAdvertisementData(uint16_t &aLen, uint8_t *aAdvertisementData)
{
    Error                 error = kErrorNone;
    DeviceTypeAndStatus   tas;
    otBleLinkCapabilities caps;

    VerifyOrExit(mVendorInfo != nullptr && aAdvertisementData != nullptr, error = kErrorInvalidArgs);

    aLen = 0;

    LittleEndian::WriteUint16(OT_TOBLE_SERVICE_UUID, aAdvertisementData);
    aLen += sizeof(uint16_t);
    aAdvertisementData[2] = OPENTHREAD_CONFIG_THREAD_VERSION << 4 | OT_TCAT_OPCODE;
    aLen++;

    if (mVendorInfo->mDeviceIds != nullptr)
    {
        for (uint8_t i = 0; mVendorInfo->mDeviceIds[i].mDeviceIdType != OT_TCAT_DEVICE_ID_EMPTY; i++)
        {
            switch (MapEnum(mVendorInfo->mDeviceIds[i].mDeviceIdType))
            {
            case kTcatDeviceIdOui24:
                SeralizeTcatAdvertisementTlv(aAdvertisementData, aLen, kTlvVendorOui24,
                                             mVendorInfo->mDeviceIds[i].mDeviceIdLen,
                                             mVendorInfo->mDeviceIds[i].mDeviceId);
                break;
            case kTcatDeviceIdOui36:
                SeralizeTcatAdvertisementTlv(aAdvertisementData, aLen, kTlvVendorOui36,
                                             mVendorInfo->mDeviceIds[i].mDeviceIdLen,
                                             mVendorInfo->mDeviceIds[i].mDeviceId);
                break;
            case kTcatDeviceIdDiscriminator:
                SeralizeTcatAdvertisementTlv(aAdvertisementData, aLen, kTlvDeviceDiscriminator,
                                             mVendorInfo->mDeviceIds[i].mDeviceIdLen,
                                             mVendorInfo->mDeviceIds[i].mDeviceId);
                break;
            case kTcatDeviceIdIanaPen:
                SeralizeTcatAdvertisementTlv(aAdvertisementData, aLen, kTlvVendorIanaPen,
                                             mVendorInfo->mDeviceIds[i].mDeviceIdLen,
                                             mVendorInfo->mDeviceIds[i].mDeviceId);
                break;
            default:
                break;
            }
        }
    }

    otPlatBleGetLinkCapabilities(&GetInstance(), &caps);

    if (caps.mGattNotifications || caps.mL2CapDirect)
    {
        SeralizeTcatAdvertisementTlv(aAdvertisementData, aLen, kTlvBleLinkCapabilities, kTlvBleLinkCapabilitiesLength,
                                     reinterpret_cast<uint8_t *>(&caps));
    }

    tas.mRsv                 = 0;
    tas.mMultiradioSupport   = otPlatBleSupportsMultiRadio(&GetInstance());
    tas.mIsCommisionned      = Get<ActiveDatasetManager>().IsCommissioned();
    tas.mThreadNetworkActive = Get<Mle::Mle>().IsAttached();
    tas.mDeviceType          = Get<Mle::Mle>().GetDeviceMode().IsFullThreadDevice();
    tas.mRxOnWhenIdle        = Get<Mle::Mle>().GetDeviceMode().IsRxOnWhenIdle();

#if OPENTHREAD_FTD && (OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE || OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE || \
                       OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE)
    tas.mIsBorderRouter = true;
#else
    tas.mIsBorderRouter = false;
#endif

    SeralizeTcatAdvertisementTlv(aAdvertisementData, aLen, kTlvDeviceTypeAndStatus, kTlvDeviceTypeAndStatusLength,
                                 reinterpret_cast<uint8_t *>(&tas));
    OT_ASSERT(aLen <= OT_TCAT_ADVERTISEMENT_MAX_LEN);

exit:
    return error;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
