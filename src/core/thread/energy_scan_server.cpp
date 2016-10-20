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

#include <coap/coap_header.hpp>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <platform/random.h>
#include <thread/meshcop_tlvs.hpp>
#include <thread/energy_scan_server.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_uris.hpp>

namespace Thread {

EnergyScanServer::EnergyScanServer(ThreadNetif &aThreadNetif) :
    mActive(false),
    mEnergyScan(OPENTHREAD_URI_ENERGY_SCAN, &EnergyScanServer::HandleRequest, this),
    mSocket(aThreadNetif.GetIp6().mUdp),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, &EnergyScanServer::HandleTimer, this),
    mCoapServer(aThreadNetif.GetCoapServer()),
    mNetif(aThreadNetif)
{
    mNetifCallback.Set(&EnergyScanServer::HandleNetifStateChanged, this);
    mNetif.RegisterCallback(mNetifCallback);

    mCoapServer.AddResource(mEnergyScan);
    mSocket.Open(HandleUdpReceive, this);

    mCoapMessageId = static_cast<uint8_t>(otPlatRandomGet());
}

void EnergyScanServer::HandleRequest(void *aContext, Coap::Header &aHeader, Message &aMessage,
                                     const Ip6::MessageInfo &aMessageInfo)
{
    static_cast<EnergyScanServer *>(aContext)->HandleRequest(aHeader, aMessage, aMessageInfo);
}

void EnergyScanServer::HandleRequest(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    MeshCoP::CountTlv count;
    MeshCoP::PeriodTlv period;
    MeshCoP::ScanDurationTlv scanDuration;
    union
    {
        MeshCoP::ChannelMaskEntry channelMaskEntry;
        uint8_t channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + sizeof(uint32_t)];
    };
    uint16_t offset;
    uint16_t length;

    VerifyOrExit(aHeader.GetCode() == Coap::Header::kCodePost, ;);

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kCount, sizeof(count), count));
    VerifyOrExit(count.IsValid(), ;);

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kPeriod, sizeof(period), period));
    VerifyOrExit(period.IsValid(), ;);

    SuccessOrExit(MeshCoP::Tlv::GetTlv(aMessage, MeshCoP::Tlv::kScanDuration, sizeof(scanDuration), scanDuration));
    VerifyOrExit(scanDuration.IsValid(), ;);

    SuccessOrExit(MeshCoP::Tlv::GetValueOffset(aMessage, MeshCoP::Tlv::kChannelMask, offset, length));
    aMessage.Read(offset, sizeof(channelMaskBuf), channelMaskBuf);
    VerifyOrExit(channelMaskEntry.GetChannelPage() == 0 &&
                 channelMaskEntry.GetMaskLength() == sizeof(uint32_t), ;);

    mChannelMask =
        (static_cast<uint32_t>(channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + 0]) << 0) |
        (static_cast<uint32_t>(channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + 1]) << 8) |
        (static_cast<uint32_t>(channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + 2]) << 16) |
        (static_cast<uint32_t>(channelMaskBuf[sizeof(MeshCoP::ChannelMaskEntry) + 3]) << 24);

    mChannelMaskCurrent = mChannelMask;
    mCount = count.GetCount();
    mPeriod = period.GetPeriod();
    mScanDuration = scanDuration.GetScanDuration();
    mScanResultsLength = 0;
    mActive = true;
    mTimer.Start(kScanDelay);

    mCommissioner = aMessageInfo.GetPeerAddr();
    SendResponse(aHeader, aMessageInfo);

exit:
    return;
}

ThreadError EnergyScanServer::SendResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aRequestInfo)
{
    ThreadError error = kThreadError_None;
    Message *message = NULL;
    Coap::Header responseHeader;
    Ip6::MessageInfo responseInfo;

    VerifyOrExit(aRequestHeader.GetType() == Coap::Header::kTypeConfirmable, ;);

    VerifyOrExit((message = mCoapServer.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

    responseHeader.Init();
    responseHeader.SetType(Coap::Header::kTypeAcknowledgment);
    responseHeader.SetCode(Coap::Header::kCodeChanged);
    responseHeader.SetMessageId(aRequestHeader.GetMessageId());
    responseHeader.SetToken(aRequestHeader.GetToken(), aRequestHeader.GetTokenLength());
    responseHeader.Finalize();
    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

    memcpy(&responseInfo, &aRequestInfo, sizeof(responseInfo));
    memset(&responseInfo.mSockAddr, 0, sizeof(responseInfo.mSockAddr));
    SuccessOrExit(error = mCoapServer.SendMessage(*message, responseInfo));

    otLogInfoMeshCoP("sent energy scan query response\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void EnergyScanServer::HandleTimer(void *aContext)
{
    static_cast<EnergyScanServer *>(aContext)->HandleTimer();
}

void EnergyScanServer::HandleTimer(void)
{
    VerifyOrExit(mActive, ;);

    if (mCount)
    {
        // grab the lowest channel to scan
        uint32_t channelMask = mChannelMaskCurrent & ~(mChannelMaskCurrent - 1);
        otLogInfoMeshCoP("%x\n", channelMask);
        mNetif.GetMac().EnergyScan(channelMask, mScanDuration, HandleScanResult, this);
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
    VerifyOrExit(mActive, ;);

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

ThreadError EnergyScanServer::SendReport(void)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;

    OT_TOOL_PACKED_BEGIN
    struct
    {
        MeshCoP::ChannelMaskTlv tlv;
        MeshCoP::ChannelMaskEntry entry;
        uint32_t mask;
    } OT_TOOL_PACKED_END channelMask;

    MeshCoP::EnergyListTlv energyList;
    Ip6::MessageInfo messageInfo;
    Message *message;

    header.Init();
    header.SetType(Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodePost);
    header.SetMessageId(++mCoapMessageId);
    header.SetToken(mCoapToken, sizeof(mCoapToken));
    header.AppendUriPathOptions(OPENTHREAD_URI_ENERGY_REPORT);
    header.Finalize();

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    channelMask.tlv.Init();
    channelMask.tlv.SetLength(sizeof(channelMask) - sizeof(channelMask.tlv));
    channelMask.entry.SetChannelPage(0);
    channelMask.entry.SetMaskLength(sizeof(channelMask.mask));
    channelMask.mask = HostSwap32(mChannelMask);
    SuccessOrExit(error = message->Append(&channelMask, sizeof(channelMask)));

    energyList.Init();
    energyList.SetLength(mScanResultsLength);
    SuccessOrExit(error = message->Append(&energyList, sizeof(energyList)));
    SuccessOrExit(error = message->Append(mScanResults, mScanResultsLength));

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.GetPeerAddr() = mCommissioner;
    messageInfo.mPeerPort = kCoapUdpPort;
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoMeshCoP("sent scan results\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    mActive = false;

    return error;
}

void EnergyScanServer::HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    otLogInfoMeshCoP("received scan report response\n");
    (void)aContext;
    (void)aMessage;
    (void)aMessageInfo;
}

void EnergyScanServer::HandleNetifStateChanged(uint32_t aFlags, void *aContext)
{
    static_cast<EnergyScanServer *>(aContext)->HandleNetifStateChanged(aFlags);
}

void EnergyScanServer::HandleNetifStateChanged(uint32_t aFlags)
{
    uint8_t length;

    if ((aFlags & OT_THREAD_NETDATA_UPDATED) != 0 &&
        !mActive &&
        mNetif.GetNetworkDataLeader().GetCommissioningData(length) == NULL)
    {
        mActive = false;
        mTimer.Stop();
    }
}

}  // namespace Thread
