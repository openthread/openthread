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

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

#include <stdio.h>

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
    , mAlreadyCommissioned(false)
    , mCommissionerHasNetworkName(false)
    , mCommissionerHasDomainName(false)
    , mCommissionerHasExtendedPanId(false)
{
    mJoinerPskd.Clear();
    mCurrentServiceName[0] = 0;
}

Error TcatAgent::Start(const TcatAgent::VendorInfo &aVendorInfo,
                       AppDataReceiveCallback       aAppDataReceiveCallback,
                       JoinCallback                 aHandler,
                       void                        *aContext)
{
    Error error = kErrorNone;

    LogInfo("Starting");

    VerifyOrExit(aVendorInfo.IsValid(), error = kErrorInvalidArgs);
    SuccessOrExit(error = mJoinerPskd.SetFrom(aVendorInfo.mPskdString));

    mAppDataReceiveCallback.Set(aAppDataReceiveCallback, aContext);
    mJoinCallback.Set(aHandler, aContext);

    mVendorInfo                 = &aVendorInfo;
    mCurrentApplicationProtocol = kApplicationProtocolNone;
    mState                      = kStateEnabled;
    mAlreadyCommissioned        = false;

exit:
    LogError("start TCAT agent", error);
    return error;
}

void TcatAgent::Stop(void)
{
    mCurrentApplicationProtocol = kApplicationProtocolNone;
    mState                      = kStateDisabled;
    mAlreadyCommissioned        = false;
    mAppDataReceiveCallback.Clear();
    mJoinCallback.Clear();
    LogInfo("TCAT agent stopped");
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
    mAlreadyCommissioned        = Get<ActiveDatasetManager>().IsCommissioned();
    LogInfo("TCAT agent connected");

exit:
    return error;
}

void TcatAgent::Disconnected(void)
{
    mCurrentApplicationProtocol = kApplicationProtocolNone;
    mAlreadyCommissioned        = false;

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
            if (datasetInfo.IsNetworkNamePresent() && mCommissionerHasNetworkName &&
                (datasetInfo.GetNetworkName() == mCommissionerNetworkName))
            {
                networkNamesMatch = true;
            }

            if (datasetInfo.IsExtendedPanIdPresent() && mCommissionerHasExtendedPanId &&
                (datasetInfo.GetExtendedPanId() == mCommissionerExtendedPanId))
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

Error TcatAgent::HandleSingleTlv(const Message &aIncommingMessage, Message &aOutgoingMessage)
{
    Error    error = kErrorParse;
    ot::Tlv  tlv;
    uint16_t offset = aIncommingMessage.GetOffset();
    uint16_t length;
    bool     response = false;

    VerifyOrExit(IsConnected(), error = kErrorInvalidState);
    SuccessOrExit(error = aIncommingMessage.Read(offset, tlv));

    if (tlv.IsExtended())
    {
        ot::ExtendedTlv extTlv;
        SuccessOrExit(error = aIncommingMessage.Read(offset, extTlv));
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
            error = HandleSetActiveOperationalDataset(aIncommingMessage, offset, length);
            break;

        case kTlvStartThreadInterface:
            error = HandleStartThreadInterface();
            break;

        case kTlvStopThreadInterface:
            error = otThreadSetEnabled(&GetInstance(), false);
            break;

        case kTlvSendApplicationData:
            LogInfo("Application data len:%d, offset:%d", length, offset);
            mAppDataReceiveCallback.InvokeIfSet(&GetInstance(), &aIncommingMessage, offset,
                                                MapEnum(mCurrentApplicationProtocol), mCurrentServiceName);
            response = true;
            error    = kErrorNone;
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

Error TcatAgent::HandleSetActiveOperationalDataset(const Message &aIncommingMessage, uint16_t aOffset, uint16_t aLength)
{
    Dataset                  dataset;
    otOperationalDatasetTlvs datasetTlvs;
    Error                    error;

    SuccessOrExit(error = dataset.ReadFromMessage(aIncommingMessage, aOffset, aLength));

    if (!CheckCommandClassAuthorizationFlags(mCommissionerAuthorizationField.mApplicationFlags,
                                             mDeviceAuthorizationField.mApplicationFlags, &dataset))
    {
        error = kErrorRejected;
        ExitNow();
    }

    dataset.ConvertTo(datasetTlvs);
    error = Get<ActiveDatasetManager>().Save(datasetTlvs);

exit:
    return error;
}

Error TcatAgent::HandleStartThreadInterface(void)
{
    Error         error;
    Dataset::Info datasetInfo;

    VerifyOrExit(Get<ActiveDatasetManager>().Read(datasetInfo) == kErrorNone, error = kErrorInvalidState);
    VerifyOrExit(datasetInfo.IsNetworkKeyPresent(), error = kErrorInvalidState);

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    VerifyOrExit(!Get<Mac::LinkRaw>().IsEnabled(), error = kErrorInvalidState);
#endif

    Get<ThreadNetif>().Up();
    error = Get<Mle::MleRouter>().Start();

exit:
    return error;
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
void TcatAgent::LogError(const char *aActionText, Error aError)
{
    if (aError != kErrorNone)
    {
        LogWarn("Failed to %s: %s", aActionText, ErrorToString(aError));
    }
}
#endif

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
