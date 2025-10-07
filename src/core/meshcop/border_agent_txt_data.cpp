/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements generation of Border Agent TXT data.
 */

#include "border_agent_txt_data.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace MeshCoP {
namespace BorderAgent {

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

const char TxtData::kRecordVersion[] = "1";

const char TxtData::Key::kRecordVersion[]   = "rv";
const char TxtData::Key::kAgentId[]         = "id";
const char TxtData::Key::kThreadVersion[]   = "tv";
const char TxtData::Key::kStateBitmap[]     = "sb";
const char TxtData::Key::kNetworkName[]     = "nn";
const char TxtData::Key::kExtendedPanId[]   = "xp";
const char TxtData::Key::kActiveTimestamp[] = "at";
const char TxtData::Key::kPartitionId[]     = "pt";
const char TxtData::Key::kDomainName[]      = "dn";
const char TxtData::Key::kBbrSeqNum[]       = "sq";
const char TxtData::Key::kBbrPort[]         = "bb";
const char TxtData::Key::kOmrPrefix[]       = "omr";
const char TxtData::Key::kExtAddress[]      = "xa";

TxtData::TxtData(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

Error TxtData::Prepare(uint8_t *aBuffer, uint16_t aBufferSize, uint16_t &aLength)
{
    Error                  error = kErrorNone;
    Dns::TxtDataEncoder    encoder(aBuffer, aBufferSize);
    MeshCoP::Dataset::Info datasetInfo;

    VerifyOrExit(aBuffer != nullptr, error = kErrorNoBufs);

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    {
        Id id;

        Get<Manager>().GetId(id);
        SuccessOrExit(error = encoder.AppendEntry(Key::kAgentId, id));
    }
#endif
    SuccessOrExit(error = encoder.AppendStringEntry(Key::kRecordVersion, kRecordVersion));
    SuccessOrExit(error = encoder.AppendBigEndianUintEntry(Key::kStateBitmap, StateBitmap::Determine(GetInstance())));
    SuccessOrExit(error = encoder.AppendStringEntry(Key::kThreadVersion, kThreadVersionString));
    SuccessOrExit(error = encoder.AppendEntry(Key::kExtAddress, Get<Mac::Mac>().GetExtAddress()));

    if (Get<MeshCoP::ActiveDatasetManager>().IsComplete() &&
        (Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo) == kErrorNone))
    {
        if (datasetInfo.IsPresent<Dataset::kExtendedPanId>())
        {
            SuccessOrExit(error = encoder.AppendEntry(Key::kExtendedPanId, datasetInfo.Get<Dataset::kExtendedPanId>()));
        }

        if (datasetInfo.IsPresent<Dataset::kNetworkName>())
        {
            SuccessOrExit(error = encoder.AppendNameEntry(Key::kNetworkName,
                                                          datasetInfo.Get<Dataset::kNetworkName>().GetAsData()));
        }
    }

    if (Get<Mle::Mle>().IsAttached())
    {
        SuccessOrExit(error = encoder.AppendBigEndianUintEntry(Key::kPartitionId,
                                                               Get<Mle::Mle>().GetLeaderData().GetPartitionId()));

        if (Get<MeshCoP::ActiveDatasetManager>().GetTimestamp().IsValid())
        {
            SuccessOrExit(error = encoder.AppendEntry(Key::kActiveTimestamp,
                                                      Get<MeshCoP::ActiveDatasetManager>().GetTimestamp()));
        }
    }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    if (Get<Mle::Mle>().IsAttached() && Get<BackboneRouter::Local>().IsEnabled())
    {
        BackboneRouter::Config bbrConfig;

        Get<BackboneRouter::Local>().GetConfig(bbrConfig);
        SuccessOrExit(error = encoder.AppendEntry(Key::kBbrSeqNum, bbrConfig.mSequenceNumber));
        SuccessOrExit(error = encoder.AppendBigEndianUintEntry(Key::kBbrPort, BackboneRouter::kBackboneUdpPort));
    }

    SuccessOrExit(error = encoder.AppendNameEntry(Key::kDomainName,
                                                  Get<MeshCoP::NetworkNameManager>().GetDomainName().GetAsData()));
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    {
        Ip6::Prefix                   prefix;
        BorderRouter::RoutePreference preference;

        if (Get<BorderRouter::RoutingManager>().GetFavoredOmrPrefix(prefix, preference) == kErrorNone &&
            prefix.GetLength() > 0)
        {
            uint8_t omrData[Ip6::NetworkPrefix::kSize + 1];

            omrData[0] = prefix.GetLength();
            memcpy(omrData + 1, prefix.GetBytes(), prefix.GetBytesSize());

            SuccessOrExit(error = encoder.AppendEntry(Key::kOmrPrefix, omrData));
        }
    }
#endif

    aLength = encoder.GetLength();

exit:
    return error;
}

Error TxtData::Prepare(ServiceTxtData &aTxtData)
{
    return Prepare(aTxtData.mData, sizeof(aTxtData.mData), aTxtData.mLength);
}

uint32_t TxtData::StateBitmap::Determine(Instance &aInstance)
{
    uint32_t bitmap = 0;

    bitmap |= (aInstance.Get<Manager>().IsRunning() ? kConnectionModePskc : kConnectionModeDisabled);
    bitmap |= kAvailabilityHigh;

    switch (aInstance.Get<Mle::Mle>().GetRole())
    {
    case Mle::DeviceRole::kRoleDisabled:
        bitmap |= (kThreadIfStatusNotInitialized | kThreadRoleDisabledOrDetached);
        break;
    case Mle::DeviceRole::kRoleDetached:
        bitmap |= (kThreadIfStatusInitialized | kThreadRoleDisabledOrDetached);
        break;
    case Mle::DeviceRole::kRoleChild:
        bitmap |= (kThreadIfStatusActive | kThreadRoleChild);
        break;
    case Mle::DeviceRole::kRoleRouter:
        bitmap |= (kThreadIfStatusActive | kThreadRoleRouter);
        break;
    case Mle::DeviceRole::kRoleLeader:
        bitmap |= (kThreadIfStatusActive | kThreadRoleLeader);
        break;
    }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    if (aInstance.Get<Mle::Mle>().IsAttached())
    {
        bitmap |= (aInstance.Get<BackboneRouter::Local>().IsEnabled() ? kFlagBbrIsActive : 0);
        bitmap |= (aInstance.Get<BackboneRouter::Local>().IsPrimary() ? kFlagBbrIsPrimary : 0);
    }
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if (aInstance.Get<EphemeralKeyManager>().GetState() != EphemeralKeyManager::kStateDisabled)
    {
        bitmap |= kFlagEpskcSupported;
    }
#endif

    return bitmap;
}

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

} // namespace BorderAgent
} // namespace MeshCoP
} // namespace ot
