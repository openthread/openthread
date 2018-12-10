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
 *   This file implements the Energy Scan Server.
 */

#define WPP_NAME "energy_scan_server.tmh"

#include "energy_scan_server.hpp"

#include <openthread/platform/random.h>

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_uri_paths.hpp"

namespace ot {

EnergyScanServer::EnergyScanServer(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mChannelMask(0)
    , mChannelMaskCurrent(0)
    , mPeriod(0)
    , mScanDuration(0)
    , mCount(0)
    , mActive(false)
    , mScanResultsLength(0)
    , mTimer(aInstance, &EnergyScanServer::HandleTimer, this)
    , mNotifierCallback(&EnergyScanServer::HandleStateChanged, this)
    , mEnergyScan(OT_URI_PATH_ENERGY_SCAN, &EnergyScanServer::HandleRequest, this)
{
    aInstance.GetNotifier().RegisterCallback(mNotifierCallback);
    GetNetif().GetCoap().AddResource(mEnergyScan);
}

void EnergyScanServer::HandleRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<EnergyScanServer *>(aContext)->HandleRequest(*static_cast<Coap::Message *>(aMessage),
                                                             *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void EnergyScanServer::HandleRequest(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    MeshCoP::CountTlv        count;
    MeshCoP::PeriodTlv       period;
    MeshCoP::ScanDurationTlv scanDuration;
    MeshCoP::ChannelMaskTlv  channelMask;
    Ip6::MessageInfo         responseInfo(aMessageInfo);

    VerifyOrExit(aMessage.GetCode() == OT_COAP_CODE_POST);

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kCount, sizeof(count), count));
    VerifyOrExit(count.IsValid());

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kPeriod, sizeof(period), period));
    VerifyOrExit(period.IsValid());

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kScanDuration, sizeof(scanDuration), scanDuration));
    VerifyOrExit(scanDuration.IsValid());

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kChannelMask, sizeof(channelMask), channelMask));
    VerifyOrExit(channelMask.IsValid() && channelMask.GetChannelPage() == OT_RADIO_CHANNEL_PAGE);

    mChannelMask        = channelMask.GetMask();
    mChannelMaskCurrent = mChannelMask;
    mCount              = count.GetCount();
    mPeriod             = period.GetPeriod();
    mScanDuration       = scanDuration.GetScanDuration();
    mScanResultsLength  = 0;
    mActive             = true;
    mTimer.Start(kScanDelay);

    mCommissioner = aMessageInfo.GetPeerAddr();

    if (aMessage.IsConfirmable() && !aMessageInfo.GetSockAddr().IsMulticast())
    {
        SuccessOrExit(GetNetif().GetCoap().SendEmptyAck(aMessage, responseInfo));
        otLogInfoMeshCoP("sent energy scan query response");
    }

exit:
    return;
}

void EnergyScanServer::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<EnergyScanServer>().HandleTimer();
}

void EnergyScanServer::HandleTimer(void)
{
    VerifyOrExit(mActive);

    if (mCount)
    {
        // grab the lowest channel to scan
        uint32_t channelMask = mChannelMaskCurrent & ~(mChannelMaskCurrent - 1);
        GetNetif().GetMac().EnergyScan(channelMask, mScanDuration, HandleScanResult, this);
    }
    else
    {
        SendReport();
    }

exit:
    return;
}

void EnergyScanServer::HandleScanResult(void *aContext, otEnergyScanResult *aResult)
{
    static_cast<EnergyScanServer *>(aContext)->HandleScanResult(aResult);
}

void EnergyScanServer::HandleScanResult(otEnergyScanResult *aResult)
{
    VerifyOrExit(mActive);

    if (aResult)
    {
        mScanResults[mScanResultsLength++] = aResult->mMaxRssi;
    }
    else
    {
        // clear the lowest channel to scan
        mChannelMaskCurrent &= mChannelMaskCurrent - 1;

        if (mChannelMaskCurrent == 0)
        {
            mChannelMaskCurrent = mChannelMask;
            mCount--;
        }

        if (mCount)
        {
            mTimer.Start(mPeriod);
        }
        else
        {
            mTimer.Start(kReportDelay);
        }
    }

exit:
    return;
}

otError EnergyScanServer::SendReport(void)
{
    otError                 error = OT_ERROR_NONE;
    MeshCoP::ChannelMaskTlv channelMask;
    MeshCoP::EnergyListTlv  energyList;
    Ip6::MessageInfo        messageInfo;
    Coap::Message *         message;

    VerifyOrExit((message = MeshCoP::NewMeshCoPMessage(GetNetif().GetCoap())) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    message->SetToken(Coap::Message::kDefaultTokenLength);
    message->AppendUriPathOptions(OT_URI_PATH_ENERGY_REPORT);
    message->SetPayloadMarker();

    channelMask.Init();
    channelMask.SetChannelPage(OT_RADIO_CHANNEL_PAGE);
    channelMask.SetMask(mChannelMask);
    SuccessOrExit(error = message->Append(&channelMask, sizeof(channelMask)));

    energyList.Init();
    energyList.SetLength(mScanResultsLength);
    SuccessOrExit(error = message->Append(&energyList, sizeof(energyList)));
    SuccessOrExit(error = message->Append(mScanResults, mScanResultsLength));

    messageInfo.SetSockAddr(GetNetif().GetMle().GetMeshLocal16());
    messageInfo.SetPeerAddr(mCommissioner);
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = GetNetif().GetCoap().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("sent scan results");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    mActive = false;

    return error;
}

void EnergyScanServer::HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags)
{
    aCallback.GetOwner<EnergyScanServer>().HandleStateChanged(aFlags);
}

void EnergyScanServer::HandleStateChanged(otChangedFlags aFlags)
{
    if ((aFlags & OT_CHANGED_THREAD_NETDATA) != 0 && !mActive &&
        GetNetif().GetNetworkDataLeader().GetCommissioningData() == NULL)
    {
        mActive = false;
        mTimer.Stop();
    }
}

} // namespace ot
