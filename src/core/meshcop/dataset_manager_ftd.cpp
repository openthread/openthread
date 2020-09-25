/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements MeshCoP Datasets manager to process commands.
 *
 */
#if OPENTHREAD_FTD

#include <stdio.h>

#include <openthread/platform/radio.h>

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"
#include "common/timer.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/dataset_manager.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_leader.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {

otError DatasetManager::AppendMleDatasetTlv(Message &aMessage) const
{
    Dataset dataset(mLocal.GetType());

    IgnoreError(mLocal.Read(dataset));

    return dataset.AppendMleDatasetTlv(aMessage);
}

otError DatasetManager::HandleSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Tlv             tlv;
    Timestamp *     timestamp;
    uint16_t        offset = aMessage.GetOffset();
    Tlv::Type       type;
    bool            isUpdateFromCommissioner = false;
    bool            doesAffectConnectivity   = false;
    bool            doesAffectMasterKey      = false;
    bool            hasMasterKey             = false;
    StateTlv::State state                    = StateTlv::kReject;
    Dataset         dataset(mLocal.GetType());

    ActiveTimestampTlv   activeTimestamp;
    PendingTimestampTlv  pendingTimestamp;
    ChannelTlv           channel;
    uint16_t             sessionId;
    Mle::MeshLocalPrefix meshLocalPrefix;
    MasterKey            masterKey;
    uint16_t             panId;

    activeTimestamp.SetLength(0);
    pendingTimestamp.SetLength(0);
    channel.SetLength(0);
    pendingTimestamp.SetLength(0);

    VerifyOrExit(Get<Mle::MleRouter>().IsLeader(), OT_NOOP);

    // verify that TLV data size is less than maximum TLV value size
    while (offset < aMessage.GetLength())
    {
        aMessage.Read(offset, sizeof(tlv), &tlv);
        VerifyOrExit(tlv.GetLength() <= Dataset::kMaxValueSize, OT_NOOP);
        offset += sizeof(tlv) + tlv.GetLength();
    }

    // verify that does not overflow dataset buffer
    VerifyOrExit((offset - aMessage.GetOffset()) <= Dataset::kMaxSize, OT_NOOP);

    type = (strcmp(mUriSet, UriPath::kActiveSet) == 0 ? Tlv::kActiveTimestamp : Tlv::kPendingTimestamp);

    if (Tlv::FindTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) != OT_ERROR_NONE)
    {
        ExitNow();
    }

    VerifyOrExit(activeTimestamp.IsValid(), OT_NOOP);

    if (Tlv::FindTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), OT_NOOP);
    }

    // verify the request includes a timestamp that is ahead of the locally stored value
    timestamp = (type == Tlv::kActiveTimestamp) ? static_cast<Timestamp *>(&activeTimestamp)
                                                : static_cast<Timestamp *>(&pendingTimestamp);

    VerifyOrExit(mLocal.Compare(timestamp) > 0, OT_NOOP);

    // check channel
    if (Tlv::FindTlv(aMessage, Tlv::kChannel, sizeof(channel), channel) == OT_ERROR_NONE)
    {
        VerifyOrExit(channel.IsValid(), OT_NOOP);

        if (channel.GetChannel() != Get<Mac::Mac>().GetPanChannel())
        {
            doesAffectConnectivity = true;
        }
    }

    // check PAN ID
    if (Tlv::FindUint16Tlv(aMessage, Tlv::kPanId, panId) == OT_ERROR_NONE && panId != Get<Mac::Mac>().GetPanId())
    {
        doesAffectConnectivity = true;
    }

    // check mesh local prefix
    if (Tlv::FindTlv(aMessage, Tlv::kMeshLocalPrefix, &meshLocalPrefix, sizeof(meshLocalPrefix)) == OT_ERROR_NONE &&
        meshLocalPrefix != Get<Mle::MleRouter>().GetMeshLocalPrefix())
    {
        doesAffectConnectivity = true;
    }

    // check network master key
    if (Tlv::FindTlv(aMessage, Tlv::kNetworkMasterKey, &masterKey, sizeof(masterKey)) == OT_ERROR_NONE)
    {
        hasMasterKey = true;

        if (masterKey != Get<KeyManager>().GetMasterKey())
        {
            doesAffectConnectivity = true;
            doesAffectMasterKey    = true;
        }
    }

    // check active timestamp rollback
    if (type == Tlv::kPendingTimestamp && (!hasMasterKey || (masterKey == Get<KeyManager>().GetMasterKey())))
    {
        // no change to master key, active timestamp must be ahead
        const Timestamp *localActiveTimestamp = Get<ActiveDataset>().GetTimestamp();

        VerifyOrExit(localActiveTimestamp == nullptr || localActiveTimestamp->Compare(activeTimestamp) > 0, OT_NOOP);
    }

    // check commissioner session id
    if (Tlv::FindUint16Tlv(aMessage, Tlv::kCommissionerSessionId, sessionId) == OT_ERROR_NONE)
    {
        const CommissionerSessionIdTlv *localId;

        isUpdateFromCommissioner = true;

        localId = static_cast<const CommissionerSessionIdTlv *>(
            Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kCommissionerSessionId));

        VerifyOrExit(localId != nullptr && localId->GetCommissionerSessionId() == sessionId, OT_NOOP);
    }

    // verify an MGMT_ACTIVE_SET.req from a Commissioner does not affect connectivity
    VerifyOrExit(!isUpdateFromCommissioner || type == Tlv::kPendingTimestamp || !doesAffectConnectivity, OT_NOOP);

    if (isUpdateFromCommissioner)
    {
        // Thread specification allows partial dataset changes for MGMT_ACTIVE_SET.req/MGMT_PENDING_SET.req
        // from Commissioner based on existing active dataset.
        IgnoreError(Get<ActiveDataset>().Read(dataset));
    }

    if (type == Tlv::kPendingTimestamp || !doesAffectConnectivity)
    {
        offset = aMessage.GetOffset();

        while (offset < aMessage.GetLength())
        {
            DatasetTlv datasetTlv;

            SuccessOrExit(datasetTlv.ReadFromMessage(aMessage, offset));

            switch (datasetTlv.GetType())
            {
            case Tlv::kCommissionerSessionId:
                // do not store Commissioner Session ID TLV
                break;

            case Tlv::kDelayTimer:
            {
                DelayTimerTlv &delayTimerTlv = static_cast<DelayTimerTlv &>(static_cast<Tlv &>(datasetTlv));

                if (doesAffectMasterKey && delayTimerTlv.GetDelayTimer() < DelayTimerTlv::kDelayTimerDefault)
                {
                    delayTimerTlv.SetDelayTimer(DelayTimerTlv::kDelayTimerDefault);
                }
                else if (delayTimerTlv.GetDelayTimer() < Get<Leader>().GetDelayTimerMinimal())
                {
                    delayTimerTlv.SetDelayTimer(Get<Leader>().GetDelayTimerMinimal());
                }
            }

                // fall through

            default:
                SuccessOrExit(dataset.SetTlv(datasetTlv));
                break;
            }

            offset += static_cast<uint16_t>(datasetTlv.GetSize());
        }

        SuccessOrExit(Save(dataset));
        Get<NetworkData::Leader>().IncrementVersionAndStableVersion();
    }
    else
    {
        Get<PendingDataset>().ApplyActiveDataset(activeTimestamp, aMessage);
    }

    state = StateTlv::kAccept;

    // notify commissioner if update is from thread device
    if (!isUpdateFromCommissioner)
    {
        const CommissionerSessionIdTlv *localSessionId;
        Ip6::Address                    destination;

        localSessionId = static_cast<const CommissionerSessionIdTlv *>(
            Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kCommissionerSessionId));
        VerifyOrExit(localSessionId != nullptr, OT_NOOP);

        SuccessOrExit(
            Get<Mle::MleRouter>().GetCommissionerAloc(destination, localSessionId->GetCommissionerSessionId()));

        Get<Leader>().SendDatasetChanged(destination);
    }

exit:

    if (Get<Mle::MleRouter>().IsLeader())
    {
        SendSetResponse(aMessage, aMessageInfo, state);
    }

    return (state == StateTlv::kAccept) ? OT_ERROR_NONE : OT_ERROR_DROP;
}

void DatasetManager::SendSetResponse(const Coap::Message &   aRequest,
                                     const Ip6::MessageInfo &aMessageInfo,
                                     StateTlv::State         aState)
{
    otError        error = OT_ERROR_NONE;
    Coap::Message *message;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Tmf::TmfAgent>())) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetDefaultResponseHeader(aRequest));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendUint8Tlv(*message, Tlv::kState, static_cast<uint8_t>(aState)));

    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent dataset set response");

exit:
    FreeMessageOnError(message, error);
}

otError DatasetManager::DatasetTlv::ReadFromMessage(const Message &aMessage, uint16_t aOffset)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aMessage.Read(aOffset, sizeof(Tlv), this) == sizeof(Tlv), error = OT_ERROR_PARSE);
    VerifyOrExit(GetLength() <= kMaxValueSize, error = OT_ERROR_PARSE);
    VerifyOrExit(aMessage.Read(aOffset + sizeof(Tlv), GetLength(), mValue) == GetLength(), error = OT_ERROR_PARSE);
    VerifyOrExit(Tlv::IsValid(*this), error = OT_ERROR_PARSE);

exit:
    return error;
}

otError ActiveDataset::CreateNewNetwork(otOperationalDataset &aDataset)
{
    otError          error             = OT_ERROR_NONE;
    Mac::ChannelMask supportedChannels = Get<Mac::Mac>().GetSupportedChannelMask();
    Mac::ChannelMask preferredChannels(Get<Radio>().GetPreferredChannelMask());

    memset(&aDataset, 0, sizeof(aDataset));

    aDataset.mActiveTimestamp = 1;

    SuccessOrExit(error = static_cast<MasterKey &>(aDataset.mMasterKey).GenerateRandom());
    SuccessOrExit(error = static_cast<Pskc &>(aDataset.mPskc).GenerateRandom());
    SuccessOrExit(error = Random::Crypto::FillBuffer(aDataset.mExtendedPanId.m8, sizeof(aDataset.mExtendedPanId)));

    SuccessOrExit(error = static_cast<Ip6::NetworkPrefix &>(aDataset.mMeshLocalPrefix).GenerateRandomUla());

    aDataset.mSecurityPolicy.mFlags = Get<KeyManager>().GetSecurityPolicyFlags();

    // If the preferred channel mask is not empty, select a random
    // channel from it, otherwise choose one from the supported
    // channel mask.

    preferredChannels.Intersect(supportedChannels);

    if (preferredChannels.IsEmpty())
    {
        preferredChannels = supportedChannels;
    }

    aDataset.mChannel     = preferredChannels.ChooseRandomChannel();
    aDataset.mChannelMask = supportedChannels.GetMask();

    aDataset.mPanId = Mac::GenerateRandomPanId();

    snprintf(aDataset.mNetworkName.m8, sizeof(aDataset.mNetworkName), "OpenThread-%04x", aDataset.mPanId);

    aDataset.mComponents.mIsActiveTimestampPresent = true;
    aDataset.mComponents.mIsMasterKeyPresent       = true;
    aDataset.mComponents.mIsNetworkNamePresent     = true;
    aDataset.mComponents.mIsExtendedPanIdPresent   = true;
    aDataset.mComponents.mIsMeshLocalPrefixPresent = true;
    aDataset.mComponents.mIsPanIdPresent           = true;
    aDataset.mComponents.mIsChannelPresent         = true;
    aDataset.mComponents.mIsPskcPresent            = true;
    aDataset.mComponents.mIsSecurityPolicyPresent  = true;
    aDataset.mComponents.mIsChannelMaskPresent     = true;

exit:
    return error;
}

otError ActiveDataset::GenerateLocal(void)
{
    otError error = OT_ERROR_NONE;
    Dataset dataset(mLocal.GetType());

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!mLocal.IsTimestampPresent(), error = OT_ERROR_ALREADY);

    IgnoreError(mLocal.Read(dataset));

    if (dataset.GetTlv<ActiveTimestampTlv>() == nullptr)
    {
        ActiveTimestampTlv activeTimestampTlv;
        activeTimestampTlv.Init();
        activeTimestampTlv.SetSeconds(0);
        activeTimestampTlv.SetTicks(0);
        IgnoreError(dataset.SetTlv(activeTimestampTlv));
    }

    if (dataset.GetTlv<ChannelTlv>() == nullptr)
    {
        ChannelTlv tlv;
        tlv.Init();
        tlv.SetChannel(Get<Mac::Mac>().GetPanChannel());
        IgnoreError(dataset.SetTlv(tlv));
    }

    if (dataset.GetTlv<ChannelMaskTlv>() == nullptr)
    {
        ChannelMaskTlv tlv;
        tlv.Init();
        tlv.SetChannelMask(Get<Mac::Mac>().GetSupportedChannelMask().GetMask());
        IgnoreError(dataset.SetTlv(tlv));
    }

    if (dataset.GetTlv<ExtendedPanIdTlv>() == nullptr)
    {
        IgnoreError(
            dataset.SetTlv(Tlv::kExtendedPanId, &Get<Mac::Mac>().GetExtendedPanId(), sizeof(Mac::ExtendedPanId)));
    }

    if (dataset.GetTlv<MeshLocalPrefixTlv>() == nullptr)
    {
        IgnoreError(dataset.SetTlv(Tlv::kMeshLocalPrefix, &Get<Mle::MleRouter>().GetMeshLocalPrefix(),
                                   sizeof(Mle::MeshLocalPrefix)));
    }

    if (dataset.GetTlv<NetworkMasterKeyTlv>() == nullptr)
    {
        IgnoreError(dataset.SetTlv(Tlv::kNetworkMasterKey, &Get<KeyManager>().GetMasterKey(), sizeof(MasterKey)));
    }

    if (dataset.GetTlv<NetworkNameTlv>() == nullptr)
    {
        Mac::NameData nameData = Get<Mac::Mac>().GetNetworkName().GetAsData();

        IgnoreError(dataset.SetTlv(Tlv::kNetworkName, nameData.GetBuffer(), nameData.GetLength()));
    }

    if (dataset.GetTlv<PanIdTlv>() == nullptr)
    {
        IgnoreError(dataset.SetUint16Tlv(Tlv::kPanId, Get<Mac::Mac>().GetPanId()));
    }

    if (dataset.GetTlv<PskcTlv>() == nullptr)
    {
        if (Get<KeyManager>().IsPskcSet())
        {
            IgnoreError(dataset.SetTlv(Tlv::kPskc, &Get<KeyManager>().GetPskc(), sizeof(Pskc)));
        }
        else
        {
            // PSKc has not yet been configured, generate new PSKc at random
            Pskc pskc;

            SuccessOrExit(error = pskc.GenerateRandom());
            IgnoreError(dataset.SetTlv(Tlv::kPskc, &pskc, sizeof(Pskc)));
        }
    }

    if (dataset.GetTlv<SecurityPolicyTlv>() == nullptr)
    {
        SecurityPolicyTlv tlv;
        tlv.Init();
        tlv.SetRotationTime(static_cast<uint16_t>(Get<KeyManager>().GetKeyRotation()));
        tlv.SetFlags(Get<KeyManager>().GetSecurityPolicyFlags());
        IgnoreError(dataset.SetTlv(tlv));
    }

    SuccessOrExit(error = mLocal.Save(dataset));
    IgnoreError(Restore());

    otLogInfoMeshCoP("Generated local dataset");

exit:
    return error;
}

void ActiveDataset::StartLeader(void)
{
    IgnoreError(GenerateLocal());
    Get<Tmf::TmfAgent>().AddResource(mResourceSet);
}

void ActiveDataset::StopLeader(void)
{
    Get<Tmf::TmfAgent>().RemoveResource(mResourceSet);
}

void ActiveDataset::HandleSet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDataset *>(aContext)->HandleSet(*static_cast<Coap::Message *>(aMessage),
                                                      *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void ActiveDataset::HandleSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SuccessOrExit(DatasetManager::HandleSet(aMessage, aMessageInfo));
    IgnoreError(ApplyConfiguration());

exit:
    return;
}

void PendingDataset::StartLeader(void)
{
    StartDelayTimer();
    Get<Tmf::TmfAgent>().AddResource(mResourceSet);
}

void PendingDataset::StopLeader(void)
{
    Get<Tmf::TmfAgent>().RemoveResource(mResourceSet);
}

void PendingDataset::HandleSet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<PendingDataset *>(aContext)->HandleSet(*static_cast<Coap::Message *>(aMessage),
                                                       *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PendingDataset::HandleSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SuccessOrExit(DatasetManager::HandleSet(aMessage, aMessageInfo));
    StartDelayTimer();

exit:
    return;
}

void PendingDataset::ApplyActiveDataset(const Timestamp &aTimestamp, Coap::Message &aMessage)
{
    uint16_t offset = aMessage.GetOffset();
    Dataset  dataset(mLocal.GetType());

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached(), OT_NOOP);

    while (offset < aMessage.GetLength())
    {
        DatasetTlv datasetTlv;

        SuccessOrExit(datasetTlv.ReadFromMessage(aMessage, offset));
        offset += static_cast<uint16_t>(datasetTlv.GetSize());
        IgnoreError(dataset.SetTlv(datasetTlv));
    }

    // add delay timer tlv
    IgnoreError(dataset.SetUint32Tlv(Tlv::kDelayTimer, Get<Leader>().GetDelayTimerMinimal()));

    // add pending timestamp tlv
    dataset.SetTimestamp(aTimestamp);
    IgnoreError(DatasetManager::Save(dataset));

    // reset delay timer
    StartDelayTimer();

exit:
    return;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD
