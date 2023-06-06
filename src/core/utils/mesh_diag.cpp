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
 *   This file implements the Mesh Diag module.
 */

#include "mesh_diag.hpp"

#if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"

namespace ot {
namespace Utils {

using namespace ot::NetworkDiagnostic;

RegisterLogModule("MeshDiag");

//---------------------------------------------------------------------------------------------------------------------
// MeshDiag

MeshDiag::MeshDiag(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance)
{
}

Error MeshDiag::DiscoverTopology(const DiscoverConfig &aConfig, DiscoverCallback aCallback, void *aContext)
{
    Error error = kErrorNone;

    VerifyOrExit(Get<Mle::Mle>().IsAttached(), error = kErrorInvalidState);
    VerifyOrExit(!mTimer.IsRunning(), error = kErrorBusy);

    Get<RouterTable>().GetRouterIdSet(mExpectedRouterIdSet);

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        if (mExpectedRouterIdSet.Contains(routerId))
        {
            SuccessOrExit(error = SendDiagGetTo(Mle::Rloc16FromRouterId(routerId), aConfig));
        }
    }

    mDiscoverCallback.Set(aCallback, aContext);
    mTimer.Start(kResponseTimeout);

exit:
    return error;
}

void MeshDiag::Cancel(void)
{
    mTimer.Stop();
    IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleDiagGetResponse, this));
}

Error MeshDiag::SendDiagGetTo(uint16_t aRloc16, const DiscoverConfig &aConfig)
{
    static constexpr uint8_t kMaxTlvsToRequest = 6;

    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());
    uint8_t          tlvs[kMaxTlvsToRequest];
    uint8_t          tlvsLength = 0;

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriDiagnosticGetRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    IgnoreError(message->SetPriority(Message::kPriorityLow));

    tlvs[tlvsLength++] = Address16Tlv::kType;
    tlvs[tlvsLength++] = ExtMacAddressTlv::kType;
    tlvs[tlvsLength++] = RouteTlv::kType;
    tlvs[tlvsLength++] = VersionTlv::kType;

    if (aConfig.mDiscoverIp6Addresses)
    {
        tlvs[tlvsLength++] = Ip6AddressListTlv::kType;
    }

    if (aConfig.mDiscoverChildTable)
    {
        tlvs[tlvsLength++] = ChildTableTlv::kType;
    }

    SuccessOrExit(error = Tlv::Append<TypeListTlv>(*message, tlvs, tlvsLength));

    messageInfo.SetSockAddrToRlocPeerAddrTo(aRloc16);
    error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleDiagGetResponse, this);

exit:
    FreeMessageOnError(message, error);
    return error;
}

void MeshDiag::HandleDiagGetResponse(void                *aContext,
                                     otMessage           *aMessage,
                                     const otMessageInfo *aMessageInfo,
                                     Error                aResult)
{
    static_cast<MeshDiag *>(aContext)->HandleDiagGetResponse(AsCoapMessagePtr(aMessage), AsCoreTypePtr(aMessageInfo),
                                                             aResult);
}

void MeshDiag::HandleDiagGetResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error           error;
    RouterInfo      routerInfo;
    Ip6AddrIterator ip6AddrIterator;
    ChildIterator   childIterator;

    SuccessOrExit(aResult);
    VerifyOrExit((aMessage != nullptr) && mTimer.IsRunning());

    SuccessOrExit(routerInfo.ParseFrom(*aMessage));

    if (ip6AddrIterator.InitFrom(*aMessage) == kErrorNone)
    {
        routerInfo.mIp6AddrIterator = &ip6AddrIterator;
    }

    if (childIterator.InitFrom(*aMessage, routerInfo.mRloc16) == kErrorNone)
    {
        routerInfo.mChildIterator = &childIterator;
    }

    mExpectedRouterIdSet.Remove(routerInfo.mRouterId);

    if (mExpectedRouterIdSet.GetNumberOfAllocatedIds() == 0)
    {
        error = kErrorNone;
        mTimer.Stop();
    }
    else
    {
        error = kErrorPending;
    }

    mDiscoverCallback.InvokeIfSet(error, &routerInfo);

exit:
    return;
}

void MeshDiag::HandleTimer(void)
{
    // Timed out waiting for response from one or more routers.

    IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleDiagGetResponse, this));

    mDiscoverCallback.InvokeIfSet(kErrorResponseTimeout, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
// MeshDiag::RouterInfo

Error MeshDiag::RouterInfo::ParseFrom(const Message &aMessage)
{
    Error     error = kErrorNone;
    Mle::Mle &mle   = aMessage.Get<Mle::Mle>();
    RouteTlv  routeTlv;

    Clear();

    SuccessOrExit(error = Tlv::Find<Address16Tlv>(aMessage, mRloc16));
    SuccessOrExit(error = Tlv::Find<ExtMacAddressTlv>(aMessage, AsCoreType(&mExtAddress)));
    SuccessOrExit(error = Tlv::FindTlv(aMessage, routeTlv));

    switch (error = Tlv::Find<VersionTlv>(aMessage, mVersion))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        mVersion = kVersionUnknown;
        error    = kErrorNone;
        break;
    default:
        ExitNow();
    }

    mRouterId           = Mle::RouterIdFromRloc16(mRloc16);
    mIsThisDevice       = (mRloc16 == mle.GetRloc16());
    mIsThisDeviceParent = mle.IsChild() && (mRloc16 == mle.GetParent().GetRloc16());
    mIsLeader           = (mRouterId == mle.GetLeaderId());
    mIsBorderRouter     = aMessage.Get<NetworkData::Leader>().ContainsBorderRouterWithRloc(mRloc16);

    for (uint8_t id = 0, index = 0; id <= Mle::kMaxRouterId; id++)
    {
        if (routeTlv.IsRouterIdSet(id))
        {
            mLinkQualities[id] = routeTlv.GetLinkQualityIn(index);
            index++;
        }
    }

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// MeshDiag::Ip6AddrIterator

Error MeshDiag::Ip6AddrIterator::InitFrom(const Message &aMessage)
{
    Error error;

    SuccessOrExit(error = Tlv::FindTlvValueStartEndOffsets(aMessage, Ip6AddressListTlv::kType, mCurOffset, mEndOffset));
    mMessage = &aMessage;

exit:
    return error;
}

Error MeshDiag::Ip6AddrIterator::GetNextAddress(Ip6::Address &aAddress)
{
    Error error = kErrorNone;

    VerifyOrExit(mMessage != nullptr, error = kErrorNotFound);
    VerifyOrExit(mCurOffset + sizeof(Ip6::Address) <= mEndOffset, error = kErrorNotFound);

    IgnoreError(mMessage->Read(mCurOffset, aAddress));
    mCurOffset += sizeof(Ip6::Address);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// MeshDiag::ChildIterator

Error MeshDiag::ChildIterator::InitFrom(const Message &aMessage, uint16_t aParentRloc16)
{
    Error error;

    SuccessOrExit(error = Tlv::FindTlvValueStartEndOffsets(aMessage, ChildTableTlv::kType, mCurOffset, mCurOffset));
    mMessage      = &aMessage;
    mParentRloc16 = aParentRloc16;

exit:
    return error;
}

Error MeshDiag::ChildIterator::GetNextChildInfo(ChildInfo &aChildInfo)
{
    Error           error = kErrorNone;
    ChildTableEntry entry;

    VerifyOrExit(mMessage != nullptr, error = kErrorNotFound);
    VerifyOrExit(mCurOffset + sizeof(ChildTableEntry) <= mEndOffset, error = kErrorNotFound);

    IgnoreError(mMessage->Read(mCurOffset, entry));
    mCurOffset += sizeof(ChildTableEntry);

    aChildInfo.mRloc16 = mParentRloc16 + entry.GetChildId();
    entry.GetMode().Get(aChildInfo.mMode);
    aChildInfo.mLinkQuality = entry.GetLinkQuality();

    aChildInfo.mIsThisDevice   = (aChildInfo.mRloc16 == mMessage->Get<Mle::Mle>().GetRloc16());
    aChildInfo.mIsBorderRouter = mMessage->Get<NetworkData::Leader>().ContainsBorderRouterWithRloc(aChildInfo.mRloc16);

exit:
    return error;
}

} // namespace Utils
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD
