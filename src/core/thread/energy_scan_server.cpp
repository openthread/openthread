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

#include "energy_scan_server.hpp"

#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("EnergyScanSrv");

EnergyScanServer::EnergyScanServer(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mChannelMask(0)
    , mChannelMaskCurrent(0)
    , mPeriod(0)
    , mScanDuration(0)
    , mCount(0)
    , mReportMessage(nullptr)
    , mTimer(aInstance)
{
}

template <>
void EnergyScanServer::HandleTmf<kUriEnergyScan>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint8_t      count;
    uint16_t     period;
    uint16_t     scanDuration;
    uint32_t     mask;
    MeshCoP::Tlv tlv;

    VerifyOrExit(aMessage.IsPostRequest());

    SuccessOrExit(Tlv::Find<MeshCoP::CountTlv>(aMessage, count));
    SuccessOrExit(Tlv::Find<MeshCoP::PeriodTlv>(aMessage, period));
    SuccessOrExit(Tlv::Find<MeshCoP::ScanDurationTlv>(aMessage, scanDuration));

    SuccessOrExit(MeshCoP::ChannelMaskTlv::FindIn(aMessage, mask));

    FreeMessage(mReportMessage);
    mReportMessage = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriEnergyReport);
    VerifyOrExit(mReportMessage != nullptr);

    SuccessOrExit(MeshCoP::ChannelMaskTlv::AppendTo(*mReportMessage, mask));

    tlv.SetType(MeshCoP::Tlv::kEnergyList);
    SuccessOrExit(mReportMessage->Append(tlv));

    mNumScanResults     = 0;
    mChannelMask        = mask;
    mChannelMaskCurrent = mChannelMask;
    mCount              = count;
    mPeriod             = period;
    mScanDuration       = scanDuration;
    mTimer.Start(kScanDelay);

    mCommissioner = aMessageInfo.GetPeerAddr();

    if (aMessage.IsConfirmable() && !aMessageInfo.GetSockAddr().IsMulticast())
    {
        SuccessOrExit(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));
        LogInfo("Sent %s ack", UriToString<kUriEnergyScan>());
    }

exit:
    return;
}

void EnergyScanServer::HandleTimer(void)
{
    VerifyOrExit(mReportMessage != nullptr);

    if (mCount)
    {
        // grab the lowest channel to scan
        uint32_t channelMask = mChannelMaskCurrent & ~(mChannelMaskCurrent - 1);

        IgnoreError(Get<Mac::Mac>().EnergyScan(channelMask, mScanDuration, HandleScanResult, this));
    }
    else
    {
        SendReport();
    }

exit:
    return;
}

void EnergyScanServer::HandleScanResult(Mac::EnergyScanResult *aResult, void *aContext)
{
    static_cast<EnergyScanServer *>(aContext)->HandleScanResult(aResult);
}

void EnergyScanServer::HandleScanResult(Mac::EnergyScanResult *aResult)
{
    VerifyOrExit(mReportMessage != nullptr);

    if (aResult)
    {
        if (mReportMessage->Append<int8_t>(aResult->mMaxRssi) != kErrorNone)
        {
            FreeMessage(mReportMessage);
            mReportMessage = nullptr;
            ExitNow();
        }

        mNumScanResults++;

        if (mNumScanResults == NumericLimits<uint8_t>::kMax)
        {
            // If we reach the max length that fit in the Energy List
            // TLV we send the current set of energy scan data.

            mCount = 0;
            mTimer.Start(kReportDelay);
        }
    }
    else
    {
        // Clear the lowest channel to scan
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

void EnergyScanServer::SendReport(void)
{
    Error            error = kErrorNone;
    Tmf::MessageInfo messageInfo(GetInstance());
    uint16_t         offset;

    // Update the Energy List TLV length in Report message
    offset = mReportMessage->GetLength() - mNumScanResults - sizeof(uint8_t);
    mReportMessage->Write(offset, mNumScanResults);

    messageInfo.SetSockAddrToRlocPeerAddrTo(mCommissioner);

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*mReportMessage, messageInfo));

    LogInfo("Sent %s", UriToString<kUriEnergyReport>());

exit:
    FreeMessageOnError(mReportMessage, error);
    LogWarnOnError(error, "send scan results");
    mReportMessage = nullptr;
}

void EnergyScanServer::HandleNotifierEvents(Events aEvents)
{
    uint16_t borderAgentRloc;

    if (aEvents.Contains(kEventThreadNetdataChanged) && (mReportMessage != nullptr) &&
        Get<NetworkData::Leader>().FindBorderAgentRloc(borderAgentRloc) != kErrorNone)
    {
        mReportMessage->Free();
        mReportMessage = nullptr;
        mTimer.Stop();
    }
}

} // namespace ot
