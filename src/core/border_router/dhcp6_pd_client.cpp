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
 *   This file implements DHCPv6 Prefix Delegation (PD) Client feature.
 */

#include "dhcp6_pd_client.hpp"

#if OT_CONFIG_DHCP6_PD_CLIENT_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace BorderRouter {

using namespace ot::Dhcp6;

RegisterLogModule("Dhcp6PdClient");

Dhcp6PdClient::Dhcp6PdClient(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStateStopped)
    , mPdPrefixCommited(false)
    , mMaxSolicitTimeout(kMaxSolicitTimeout)
    , mTimer(aInstance)
{
    mServerAddress.Clear();
}

void Dhcp6PdClient::Start(void)
{
    LogInfo("Starting");

    switch (mState)
    {
    case kStateStopped:
        Get<InfraIf>().SetDhcp6ListeningEnabled(true);
        OT_FALL_THROUGH;

    case kStateReleasing:
        EnterState(kStateToSolicit);
        break;

    case kStateToSolicit:
    case kStateSoliciting:
    case kStateRequesting:
    case kStateToRenew:
    case kStateRenewing:
    case kStateRebinding:
        ExitNow();
    }

exit:
    return;
}

void Dhcp6PdClient::Stop(void)
{
    LogInfo("Stopping");

    switch (mState)
    {
    case kStateStopped:
    case kStateReleasing:
        break;
    case kStateToSolicit:
    case kStateSoliciting:
    case kStateRequesting:
        EnterState(kStateStopped);
        break;
    case kStateToRenew:
    case kStateRenewing:
    case kStateRebinding:
        EnterState(kStateReleasing);
        break;
    }
}

void Dhcp6PdClient::EnterState(State aState)
{
    LogInfo("State: %s -> %s", StateToString(mState), StateToString(aState));

    mState = aState;

    switch (mState)
    {
    case kStateStopped:
        ClearServerDuid();
        ClearPdPrefix();
        Get<InfraIf>().SetDhcp6ListeningEnabled(false);
        break;

    case kStateToSolicit:
        ClearServerDuid();
        ClearPdPrefix();
        mTimer.Start(Random::NonCrypto::GetUint32InRange(0, kMaxDelayFirstSolicit));
        break;

    case kStateSoliciting:
        ClearServerDuid();
        ClearPdPrefix();
        // Per RFC 8415 section 18.2.1, the first timeout for Solicit message
        // must be selected to be strictly greater than the initial timeout
        // value, therefore `kPositiveJitter` is used.
        mRetxTracker.Start(kInitialSolicitTimeout, mMaxSolicitTimeout, kPositiveJitter);
        break;

    case kStateRequesting:
        mRetxTracker.Start(kInitialRequestTimeout, kMaxRequestTimeout, kFullJitter);
        mRetxTracker.SetMaxCount(kMaxRequestRetxCount);
        break;

    case kStateToRenew:
        mTimer.FireAt(mPdPrefix.DetermineT1Time());
        break;

    case kStateRenewing:
        mRetxTracker.Start(kInitialRenewTimeout, kMaxRenewTimeout, kFullJitter);
        mRetxTracker.SetRetxEndTime(mPdPrefix.DetermineT2Time());
        break;

    case kStateRebinding:
        ClearServerDuid();
        mRetxTracker.Start(kInitialRebindTimeout, kMaxRebindTimeout, kFullJitter);
        mRetxTracker.SetRetxEndTime(mPdPrefix.DeterminePreferredTime());
        break;

    case kStateReleasing:
        mRetxTracker.Start(kInitialReleaseTimeout, kInfiniteTimeout, kFullJitter);
        mRetxTracker.SetMaxCount(kMaxReleaseRetxCount);
        break;
    }

    SendMessage();
}

void Dhcp6PdClient::HandleTimer(void)
{
    switch (mState)
    {
    case kStateStopped:
        break;

    case kStateToSolicit:
        EnterState(kStateSoliciting);
        break;

    case kStateToRenew:
        EnterState(kStateRenewing);
        break;

    case kStateSoliciting:
        if (mPdPrefix.IsValid())
        {
            EnterState(kStateRequesting);
            break;
        }

        OT_FALL_THROUGH;

    case kStateRequesting:
    case kStateRenewing:
    case kStateRebinding:
    case kStateReleasing:
        SendMessage();
        break;
    }
}

void Dhcp6PdClient::SendMessage(void)
{
    static const uint16_t kRequestedOptions[] = {BigEndian::HostSwap16(Option::kSolMaxRt)};

    MsgType           msgType = kMsgTypeNone;
    OwnedPtr<Message> message;
    Header            header;
    Ip6::Address      dstAddr;

    switch (mState)
    {
    case kStateSoliciting:
        msgType = kMsgTypeSolicit;
        break;
    case kStateRequesting:
        msgType = kMsgTypeRequest;
        break;
    case kStateRenewing:
        msgType = kMsgTypeRenew;
        break;
    case kStateRebinding:
        msgType = kMsgTypeRebind;
        break;
    case kStateReleasing:
        msgType = kMsgTypeRelease;
        break;
    case kStateStopped:
    case kStateToSolicit:
    case kStateToRenew:
        ExitNow();
    }

    VerifyOrExit(msgType != kMsgTypeNone);

    if (!mRetxTracker.ShouldRetx())
    {
        // Message exchanges can optionally define limits: A maximum
        // retry count (e.g., for `Request` messages) and/or a
        // maximum retransmission duration which defines an end time
        // for retries (e.g., for `Renew` or `Rebind` messages). We skip
        // transmission and update the state once retransmissions are
        // exhausted.

        UpdateStateAfterRetxExhausted();
        ExitNow();
    }

    // In the unlikely event that message preparation fails (e.g., due to
    // out-of-buffer condition), it will be retried after a short
    // delay of `kRetxDelayOnFailedTx`.

    mTimer.Start(kRetxDelayOnFailedTx);

    message.Reset(Get<MessagePool>().Allocate(Message::kTypeOther));
    VerifyOrExit(message != nullptr);

    header.SetMsgType(msgType);
    header.SetTransactionId(mRetxTracker.GetTransactionId());
    SuccessOrExit(message->Append(header));

    SuccessOrExit(ClientIdOption::AppendWithEui64Duid(*message, Get<Mac::Mac>().GetExtAddress()));

    if (!mServerDuid.IsEmpty())
    {
        SuccessOrExit(ServerIdOption::AppendWithDuid(*message, mServerDuid.GetArrayBuffer(), mServerDuid.GetLength()));
    }

    SuccessOrExit(ElapsedTimeOption::AppendTo(*message, mRetxTracker.DetermineElapsedTime()));

    SuccessOrExit(
        Option::AppendOption(*message, Option::kOptionRequest, &kRequestedOptions, sizeof(kRequestedOptions)));

    SuccessOrExit(AppendIaPdOption(*message));

    LogInfo("Sending %s %s%s", MsgTypeToString(msgType),
            mServerAddress.IsUnspecified() ? "(multicast)" : "(unicast) to:",
            mServerAddress.IsUnspecified() ? "" : mServerAddress.ToString().AsCString());

    if (!mServerAddress.IsUnspecified())
    {
        dstAddr = mServerAddress;
    }
    else
    {
        GetAllRelayAgentsAndServersMulticastAddress(dstAddr);
    }

    Get<InfraIf>().SendDhcp6(*message.Release(), dstAddr);

    mRetxTracker.ScheduleTimeoutTimer(mTimer);
    mRetxTracker.UpdateTimeoutAndCountAfterTx();

exit:
    return;
}

void Dhcp6PdClient::GetAllRelayAgentsAndServersMulticastAddress(Ip6::Address &aAddress) const
{
    // `All_DHCP_Relay_Agents_and_Servers` address (`ff02::1:2`).

    aAddress.Clear();
    aAddress.mFields.m16[0] = BigEndian::HostSwap16(0xff02);
    aAddress.mFields.m16[6] = BigEndian::HostSwap16(1);
    aAddress.mFields.m16[7] = BigEndian::HostSwap16(2);
}

void Dhcp6PdClient::UpdateStateAfterRetxExhausted(void)
{
    switch (mState)
    {
    case kStateStopped:
    case kStateToSolicit:
    case kStateSoliciting:
    case kStateToRenew:
        break;

    case kStateRequesting:
        EnterState(kStateSoliciting);
        break;
    case kStateRenewing:
        EnterState(kStateRebinding);
        break;
    case kStateRebinding:
        EnterState(kStateSoliciting);
        break;
    case kStateReleasing:
        EnterState(kStateStopped);
        break;
    }
}

Error Dhcp6PdClient::AppendIaPdOption(Message &aMessage)
{
    Error      error;
    IaPdOption iaPdOption;

    uint16_t    optionOffset;
    Ip6::Prefix prefix;

    // RFC 8415 - Section 21.21: In message from client, the T1 and T2
    // field SHOULD be set to 0. Server MUST ignore any values in these
    // fields from a client.

    iaPdOption.Init();
    iaPdOption.SetIaid(kIaid);
    iaPdOption.SetT1(0);
    iaPdOption.SetT2(0);

    optionOffset = aMessage.GetLength();
    SuccessOrExit(error = aMessage.Append(iaPdOption));

    if (mPdPrefix.IsValid())
    {
        SuccessOrExit(error = AppendIaPrefixOption(aMessage, mPdPrefix.mPrefix));
    }
    else
    {
        // RFC 8415 Section 21.22: A client MAY send a non-zero value
        // in "prefix length" with unspecified (`::`) for the prefix to
        // indicates its preference for the size of the prefix to be
        // delegated.

        prefix.Clear();
        prefix.mLength = kDesiredPrefixLength;

        SuccessOrExit(error = AppendIaPrefixOption(aMessage, prefix));
    }

    Option::UpdateOptionLengthInMessage(aMessage, optionOffset);

exit:
    return error;
}

Error Dhcp6PdClient::AppendIaPrefixOption(Message &aMessage, const Ip6::Prefix &aPrefix)
{
    IaPrefixOption iaPrefixOption;

    // RFC 8415 Section 21.22: In a message sent by client, the
    // preferred and valid lifetimes fields SHOULD be set to 0. Server
    // MUST ignore any received values in these fields.

    iaPrefixOption.Init();
    iaPrefixOption.SetPreferredLifetime(0);
    iaPrefixOption.SetValidLifetime(0);
    iaPrefixOption.SetPrefix(aPrefix);

    return aMessage.Append(iaPrefixOption);
}

void Dhcp6PdClient::HandleReceived(Message &aMessage)
{
    Header      header;
    OffsetRange serverDuidOffsetRange;

    switch (mState)
    {
    case kStateSoliciting:
    case kStateRequesting:
    case kStateRenewing:
    case kStateRebinding:
    case kStateReleasing:
        break;

    case kStateStopped:
    case kStateToSolicit:
    case kStateToRenew:
        ExitNow();
    }

    SuccessOrExit(ParseHeaderAndValidateMessage(aMessage, header));

    switch (header.GetMsgType())
    {
    case kMsgTypeAdvertise:
        VerifyOrExit(mState == kStateSoliciting);
        break;

    case kMsgTypeReply:
        VerifyOrExit(mState != kStateSoliciting);
        break;

    default:
        ExitNow();
    }

    // Per RFC 8415, Section 16, a client MUST discard any received
    // Advertise/Reply messages that meet any of the following
    // conditions:
    // - The "Transaction ID" does not match the value used in the
    //   Solicit/Request message.
    // - The message does not include a Client ID Option, or its content
    //   does not match the client's DUID.
    // - The message does not include a Server ID option.

    VerifyOrExit(header.GetTransactionId() == mRetxTracker.GetTransactionId());
    SuccessOrExit(ClientIdOption::MatchesEui64Duid(aMessage, Get<Mac::Mac>().GetExtAddress()));
    SuccessOrExit(ServerIdOption::ReadDuid(aMessage, serverDuidOffsetRange));

    // If we have selected a server, ensure the received server DUID
    // matches the one saved in `mServerDuid`. However, during the
    // initial solicitation attempt, we wait until the first timeout
    // expires to collect Solicits from all servers. During this
    // period, the favored server/prefix is tracked, so `mServerDuid`
    // may be set, but we still process Solicits from other servers.

    if (!mServerDuid.IsEmpty() && (mState != kStateSoliciting))
    {
        VerifyOrExit(serverDuidOffsetRange.GetLength() == mServerDuid.GetLength());
        VerifyOrExit(aMessage.CompareBytes(serverDuidOffsetRange, mServerDuid.GetArrayBuffer()));
    }

    // The client MUST process any SOL_MAX_RT option in an Advertise
    // or Reply message, even if the message contains a Status Code
    // option indicating a failure and will be discarded by the client

    ProcessSolMaxRtOption(aMessage);

    switch (header.GetMsgType())
    {
    case kMsgTypeAdvertise:
        HandleAdvertise(aMessage);
        break;

    case kMsgTypeReply:
        HandleReply(aMessage);
        break;
    }

exit:
    aMessage.Free();
}

Error Dhcp6PdClient::ParseHeaderAndValidateMessage(Message &aMessage, Header &aHeader)
{
    Error       error;
    OffsetRange offsetRange;
    Option      option;

    offsetRange.InitFromMessageOffsetToEnd(aMessage);

    SuccessOrExit(error = aMessage.Read(offsetRange, aHeader));
    offsetRange.AdvanceOffset(sizeof(Header));

    aMessage.SetOffset(offsetRange.GetOffset());

    // Validate all top-level options.

    while (!offsetRange.IsEmpty())
    {
        SuccessOrExit(error = aMessage.Read(offsetRange, option));
        VerifyOrExit(offsetRange.Contains(option.GetSize()), error = kErrorParse);
        offsetRange.AdvanceOffset(option.GetSize());
    }

exit:
    return error;
}

void Dhcp6PdClient::HandleAdvertise(const Message &aMessage)
{
    PdPrefixArray            pdPrefixes;
    PdPrefix                *favoredPdPrefix;
    uint8_t                  preference;
    StatusCodeOption::Status status;

    VerifyOrExit(StatusCodeOption::ReadStatusFrom(aMessage) == StatusCodeOption::kSuccess);

    LogInfo("Received %s", MsgTypeToString(kMsgTypeAdvertise));

    SuccessOrExit(ProcessIaPd(aMessage, pdPrefixes, status));

    VerifyOrExit(status != StatusCodeOption::kNoPrefixAvail);

    favoredPdPrefix = SelectFavoredPrefix(pdPrefixes);
    VerifyOrExit(favoredPdPrefix != nullptr);

    ProcessPreferenceOption(aMessage, preference);

    // Per RFC 8415 Section 18.2.1, for the first Solicit message, a
    // client MUST process all valid Advertise messages for the full
    // timeout duration, unless it receives a valid Advertise message
    // with a preference value of 255. For subsequent Solicit
    // retransmissions, the client terminates the retransmission
    // process upon receiving any valid Advertise message and acts on
    // it immediately, without waiting for additional Advertise
    // messages.
    //
    // This is implemented by tracking the overall favored prefix and
    // its corresponding server DUID across all received Advertise
    // messages. If this is the first Solicit and a favored prefix from
    // a valid server DUID is already being tracked, we ensure the
    // newly received favored prefix is indeed more preferred before
    // updating the stored information.

    if (mRetxTracker.IsFirstAttempt() && !mServerDuid.IsEmpty())
    {
        VerifyOrExit(favoredPdPrefix->mAdjustedPrefix < mPdPrefix.mAdjustedPrefix);
    }

    mPdPrefix = *favoredPdPrefix;

    SaveServerDuidAndAddress(aMessage);

    if (!mRetxTracker.IsFirstAttempt() || (preference == 255))
    {
        EnterState(kStateRequesting);
    }

exit:
    return;
}

void Dhcp6PdClient::HandleReply(const Message &aMessage)
{
    PdPrefixArray            pdPrefixes;
    PdPrefix                *favoredPdPrefix;
    PdPrefix                *matchedPdPrefix;
    StatusCodeOption::Status status;

    status = StatusCodeOption::ReadStatusFrom(aMessage);

    LogInfo("Received %s, status:%u", MsgTypeToString(kMsgTypeReply), status);

    if (status == StatusCodeOption::kUnspecFail)
    {
        ExitNow();
    }

    if (status == StatusCodeOption::kUseMulticast)
    {
        // Per RFC 8514 Section 18.2.10, if the client receives a Reply
        // message with a status code of UseMulticast, the client
        // records the receipt of the message and sends subsequent
        // messages to the server using multicast. The client re-sends the
        // original message using multicast.

        VerifyOrExit(!mServerAddress.IsUnspecified());
        mServerAddress.Clear();
        SendMessage();
        ExitNow();
    }

    if (mState == kStateReleasing)
    {
        // Per RFC 8415 Section 18.2.10.2: When the client receives a
        // valid Reply message in response to a Release message, the
        // client considers the Release event completed, regardless of
        // the Status Code option returned by the server.

        mState = kStateStopped;
        ExitNow();
    }

    SuccessOrExit(ProcessIaPd(aMessage, pdPrefixes, status));

    if (mState == kStateRequesting)
    {
        favoredPdPrefix = SelectFavoredPrefix(pdPrefixes);

        if ((status == StatusCodeOption::kNoPrefixAvail) || (favoredPdPrefix == nullptr))
        {
            EnterState(kStateToSolicit);
            ExitNow();
        }

        SaveServerDuidAndAddress(aMessage);
        CommitPdPrefix(*favoredPdPrefix);

        ExitNow();
    }

    if ((mState == kStateRenewing) || (mState == kStateRebinding))
    {
        VerifyOrExit(status != StatusCodeOption::kNoPrefixAvail);

        matchedPdPrefix = pdPrefixes.FindMatching(mPdPrefix);

        if (matchedPdPrefix != nullptr)
        {
            SaveServerDuidAndAddress(aMessage);
            CommitPdPrefix(*matchedPdPrefix);

            if (mPdPrefix.mPreferredLifetime >= kMinPreferredLifetime)
            {
                ExitNow();
            }
        }

        // The previously delegated prefix does not appear in IA or
        // it is included with a unacceptably short lifetime.
        // Check if server provided any other prefixes which we
        // can use instead.

        favoredPdPrefix = SelectFavoredPrefix(pdPrefixes);

        if (favoredPdPrefix != nullptr)
        {
            SaveServerDuidAndAddress(aMessage);
            CommitPdPrefix(*favoredPdPrefix);
            ExitNow();
        }

        if ((status == StatusCodeOption::kNoBinding) && (mState == kStateRenewing))
        {
            EnterState(kStateRebinding);
        }

        ExitNow();
    }

exit:
    return;
}

Dhcp6PdClient::PdPrefix *Dhcp6PdClient::SelectFavoredPrefix(PdPrefixArray &aPdPrefixes)
{
    // Select the favored prefix from the list of delegated prefixes.
    // While we request a single prefix in an `IaPdOption` and servers
    // typically provide one, if multiple prefixes are present, we
    // select the numerically smaller (favored) one. We also validate
    // that the delegated prefix meets a minimum lifetime.
    //
    // We request a /64 prefix length, but the server might assign a
    // shorter length. In such cases, we adjust it by adding zero bits
    // to create a /64 prefix.

    PdPrefix *favoredPdPrefix = nullptr;

    for (PdPrefix &pdPrefix : aPdPrefixes)
    {
        if (pdPrefix.mPrefix.GetLength() > kDesiredPrefixLength)
        {
            continue;
        }

        if (pdPrefix.mPreferredLifetime < kMinPreferredLifetime)
        {
            continue;
        }

        // Determine the favored prefix

        if ((favoredPdPrefix == nullptr) || (pdPrefix.mAdjustedPrefix < favoredPdPrefix->mAdjustedPrefix))
        {
            favoredPdPrefix = &pdPrefix;
        }
    }

    return favoredPdPrefix;
}

void Dhcp6PdClient::SaveServerDuidAndAddress(const Message &aMessage)
{
    // Reads the server DUID and Server Unicast option from the given
    // message and saves them. The message is assumed to have already
    // been validated to contain a Server ID option.

    OffsetRange  serverDuidOffsetRange;
    Ip6::Address serverAddress;

    SuccessOrAssert(ServerIdOption::ReadDuid(aMessage, serverDuidOffsetRange));
    mServerDuid.SetLength(static_cast<uint8_t>(serverDuidOffsetRange.GetLength()));
    aMessage.ReadBytes(serverDuidOffsetRange, mServerDuid.GetArrayBuffer());

    ProcessServerUnicastOption(aMessage, serverAddress);

    if (!serverAddress.IsUnspecified())
    {
        mServerAddress = serverAddress;
    }
}

void Dhcp6PdClient::ClearServerDuid(void)
{
    mServerDuid.Clear();
    mServerAddress.Clear();
}

void Dhcp6PdClient::ClearPdPrefix(void)
{
    if (mPdPrefixCommited)
    {
        mPdPrefix.mPreferredLifetime = 0;
        mPdPrefix.mValidLifetime     = 0;
        mPdPrefix.mT1                = 0;
        mPdPrefix.mT2                = 0;
        ReportPdPrefixToRoutingManager();
    }

    mPdPrefix.Clear();
    mPdPrefixCommited = false;
}

void Dhcp6PdClient::CommitPdPrefix(const PdPrefix &aPdPrefix)
{
    if (!mPdPrefix.Matches(aPdPrefix))
    {
        // Clear to ensure we report the prefix's removal if it
        // was previously committed.
        ClearPdPrefix();
    }

    mPdPrefix = aPdPrefix;
    mPdPrefix.AdjustLifetimesT1AndT2();
    mPdPrefixCommited = true;

    EnterState(kStateToRenew);
    ReportPdPrefixToRoutingManager();
}

void Dhcp6PdClient::ReportPdPrefixToRoutingManager(void)
{
    RoutingManager::Dhcp6PdPrefix prefix;

    LogInfo("Delegated prefix:%s, adj:%s, T1:%lu, T2:%lu, prf:%lu, valid:%lu", mPdPrefix.mPrefix.ToString().AsCString(),
            mPdPrefix.mAdjustedPrefix.ToString().AsCString(), ToUlong(mPdPrefix.mT1), ToUlong(mPdPrefix.mT2),
            ToUlong(mPdPrefix.mPreferredLifetime), ToUlong(mPdPrefix.mValidLifetime));

    switch (mState)
    {
    case kStateToSolicit:
    case kStateSoliciting:
    case kStateRequesting:
    case kStateToRenew:
    case kStateRenewing:
    case kStateRebinding:
        break;

    case kStateStopped:
    case kStateReleasing:
        ExitNow();
    }

    ClearAllBytes(prefix);
    prefix.mPrefix            = mPdPrefix.mPrefix;
    prefix.mValidLifetime     = mPdPrefix.mValidLifetime;
    prefix.mPreferredLifetime = mPdPrefix.mPreferredLifetime;

    Get<RoutingManager>().ProcessDhcp6PdPrefix(prefix);

exit:
    return;
}

void Dhcp6PdClient::ProcessSolMaxRtOption(const Message &aMessage)
{
    // RFC 8415 - Section 18.2.9: The client MUST process any SOL_MAX_RT
    // option present in an Advertise message, even if the message
    // contains a Status Code option indicating a failure, and the
    // Advertise message will be discarded by the client. A client
    // SHOULD only update its SOL_MAX_RT values if all received
    // Advertise messages that contained the corresponding option
    // specified the same value; otherwise, it should use the default
    // value.

    OffsetRange    offsetRange;
    SolMaxRtOption solMaxRtOption;
    uint32_t       solMaxRt;

    SuccessOrExit(Option::FindOption(aMessage, Option::kSolMaxRt, offsetRange));
    SuccessOrExit(aMessage.Read(offsetRange, solMaxRtOption));

    // A DHCP client MUST ignore any SOL_MAX_RT option values that are
    // less than 60 `kMinSolMaxRt` or more than 86400 `kMaxSolMaxRt`.

    solMaxRt = solMaxRtOption.GetSolMaxRt();

    VerifyOrExit(solMaxRt >= SolMaxRtOption::kMinSolMaxRt);
    VerifyOrExit(solMaxRt <= SolMaxRtOption::kMaxSolMaxRt);

    solMaxRt *= Time::kOneSecondInMsec;

    // If we are using default, adopt the new received value. If we
    // have previously updated the value (from another advertisement)
    // and the value does not match the newly received one, revert
    // back to the default value.

    if (mMaxSolicitTimeout == kMaxSolicitTimeout)
    {
        mMaxSolicitTimeout = solMaxRt;
    }
    else if (solMaxRt != mMaxSolicitTimeout)
    {
        mMaxSolicitTimeout = kMaxSolicitTimeout;
    }

    if (mState == kStateSoliciting)
    {
        mRetxTracker.SetMaxTimeout(mMaxSolicitTimeout);
    }

exit:
    return;
}

Error Dhcp6PdClient::ProcessIaPd(const Message            &aMessage,
                                 PdPrefixArray            &aPdPrefixes,
                                 StatusCodeOption::Status &aStatus) const
{
    Error            error;
    Option::Iterator iterator;

    aStatus = StatusCodeOption::kSuccess;

    // Iterate over all `IaPdOption` to find one matching `kIaid`.

    aPdPrefixes.Free();

    for (iterator.Init(aMessage, Option::kIaPd); !iterator.IsDone(); iterator.Advance())
    {
        IaPdOption  iaPdOption;
        OffsetRange offsetRange = iterator.GetOptionOffsetRange();

        if ((aMessage.Read(offsetRange, iaPdOption) != kErrorNone) || ShouldSkipPdOption(iaPdOption))
        {
            continue;
        }

        offsetRange.AdvanceOffset(sizeof(IaPdOption));
        error = ProcessIaPdPrefixes(aMessage, iaPdOption, offsetRange, aPdPrefixes, aStatus);
        ExitNow();
    }

    error = iterator.GetError();

exit:
    return error;
}

bool Dhcp6PdClient::ShouldSkipPdOption(const IaPdOption &aIaPdOption) const
{
    bool shouldSkip = true;

    VerifyOrExit(aIaPdOption.GetIaid() == kIaid);

    // RFC 8415 Section 21.21: If T1 is greater than T2 and both T1 and
    // T2 are non-zero, we discard the `IaPdOption` and processes the
    // remainder of the message as though this option had not been
    // included.

    if (aIaPdOption.GetT1() != 0 && aIaPdOption.GetT2() != 0)
    {
        VerifyOrExit(aIaPdOption.GetT1() <= aIaPdOption.GetT2());
    }

    shouldSkip = false;

exit:
    return shouldSkip;
}

Error Dhcp6PdClient::ProcessIaPdPrefixes(const Message            &aMessage,
                                         const IaPdOption         &aIaPdOption,
                                         const OffsetRange        &aIaPdOptionsOffseRange,
                                         PdPrefixArray            &aPdPrefixes,
                                         StatusCodeOption::Status &aStatus) const
{
    Error            error;
    Option::Iterator iterator;

    aStatus = StatusCodeOption::ReadStatusFrom(aMessage, aIaPdOptionsOffseRange);

    LogInfo("Processing IA prefix options, status:%u", aStatus);

    for (iterator.Init(aMessage, aIaPdOptionsOffseRange, Option::kIaPrefix); !iterator.IsDone(); iterator.Advance())
    {
        IaPrefixOption prefixOption;
        Ip6::Prefix    prefix;
        PdPrefix      *newEntry;
        bool           shouldSkip;

        if (aMessage.Read(iterator.GetOptionOffsetRange(), prefixOption) != kErrorNone)
        {
            continue;
        }

        shouldSkip = ShouldSkipPrefixOption(prefixOption);
        prefixOption.GetPrefix(prefix);

        LogInfo("   Prefix:%s, T1:%lu, T2:%lu, prf:%lu, valid:%lu, skip:%s", prefix.ToString().AsCString(),
                ToUlong(aIaPdOption.GetT1()), ToUlong(aIaPdOption.GetT2()),
                ToUlong(prefixOption.GetPreferredLifetime()), ToUlong(prefixOption.GetValidLifetime()),
                ToYesNo(shouldSkip));

        if (shouldSkip)
        {
            continue;
        }

        newEntry = aPdPrefixes.PushBack();
        VerifyOrExit(newEntry != nullptr, error = kErrorNoBufs);

        newEntry->mPrefix            = prefix;
        newEntry->mT1                = aIaPdOption.GetT1();
        newEntry->mT2                = aIaPdOption.GetT2();
        newEntry->mPreferredLifetime = prefixOption.GetPreferredLifetime();
        newEntry->mValidLifetime     = prefixOption.GetValidLifetime();
        newEntry->mUpdateTime        = TimerMilli::GetNow();

        // Adjust a shorter prefix to the desired /64 length.

        if (prefix.GetLength() <= kDesiredPrefixLength)
        {
            newEntry->mAdjustedPrefix = prefix;
            newEntry->mAdjustedPrefix.Tidy();
            newEntry->mAdjustedPrefix.SetLength(kDesiredPrefixLength);
        }
        else
        {
            newEntry->mAdjustedPrefix.Clear();
        }
    }

    error = iterator.GetError();

exit:
    return error;
}

bool Dhcp6PdClient::ShouldSkipPrefixOption(const IaPrefixOption &aPrefixOption) const
{
    bool shouldSkip = true;

    VerifyOrExit(aPrefixOption.GetPrefixLength() <= Ip6::Prefix::kMaxLength);

    // The client MUST discard any prefixes for which the preferred lifetime
    // is greater than the valid lifetime.

    VerifyOrExit(aPrefixOption.GetPreferredLifetime() <= aPrefixOption.GetValidLifetime());

    shouldSkip = false;

exit:
    return shouldSkip;
}

void Dhcp6PdClient::ProcessServerUnicastOption(const Message &aMessage, Ip6::Address &aServerAddress) const
{
    // Searches the message for a `ServerUnicastOption`. If found, the
    // server address is retrieved from it. Otherwise, `aServerAddress`
    // is set to `::` (unspecified address).

    OffsetRange         offsetRange;
    ServerUnicastOption serverUnicastOption;

    aServerAddress.Clear();

    SuccessOrExit(Option::FindOption(aMessage, Option::kServerUnicast, offsetRange));
    SuccessOrExit(aMessage.Read(offsetRange, serverUnicastOption));

    aServerAddress = serverUnicastOption.GetServerAddress();

    LogInfo("Processed Sever Unicast Option, serverAddr:%s", aServerAddress.ToString().AsCString());

exit:
    return;
}

void Dhcp6PdClient::ProcessPreferenceOption(const Message &aMessage, uint8_t &aPreference) const
{
    // Searches for `PreferenceOption` in the message. If it is not
    // found, assumes default preference value of zero.

    OffsetRange      offsetRange;
    PreferenceOption preferenceOption;

    aPreference = kDefaultPreference;

    SuccessOrExit(Option::FindOption(aMessage, Option::kPreference, offsetRange));
    SuccessOrExit(aMessage.Read(offsetRange, preferenceOption));

    aPreference = preferenceOption.GetPreference();

    LogInfo("Processed Preference Option, prf:%u", aPreference);

exit:
    return;
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *Dhcp6PdClient::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Stopped",    // (0) kStateStopped
        "ToSolicit",  // (1) kStateToSolicit
        "Soliciting", // (2) kStateSoliciting
        "Requesting", // (3) kStateRequesting
        "ToRenew",    // (4) kStateToRenew
        "Renewing",   // (5) kStateRenewing
        "Rebinding",  // (6) kStateRebinding
        "Releasing",  // (7) kStateReleasing
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kStateStopped);
        ValidateNextEnum(kStateToSolicit);
        ValidateNextEnum(kStateSoliciting);
        ValidateNextEnum(kStateRequesting);
        ValidateNextEnum(kStateToRenew);
        ValidateNextEnum(kStateRenewing);
        ValidateNextEnum(kStateRebinding);
        ValidateNextEnum(kStateReleasing);
    };

    return kStateStrings[aState];
}

const char *Dhcp6PdClient::MsgTypeToString(MsgType aMsgType)
{
    static const char *const kMsgTypeStrings[]{
        "Solicit",            // (1) kMsgTypeSolicit
        "Advertise",          // (2) kMsgTypeAdvertise
        "Request",            // (3) kMsgTypeRequest
        "Confirm",            // (4) kMsgTypeConfirm
        "Renew",              // (5) kMsgTypeRenew
        "Rebind",             // (6) kMsgTypeRebind
        "Reply",              // (7) kMsgTypeReply
        "Release",            // (8) kMsgTypeRelease
        "Decline",            // (9) kMsgTypeDecline
        "Reconfigure",        // (10) kMsgTypeReconfigure
        "InformationRequest", // (11) kMsgTypeInformationRequest
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        SkipNextEnum();
        ValidateNextEnum(kMsgTypeSolicit);
        ValidateNextEnum(kMsgTypeAdvertise);
        ValidateNextEnum(kMsgTypeRequest);
        ValidateNextEnum(kMsgTypeConfirm);
        ValidateNextEnum(kMsgTypeRenew);
        ValidateNextEnum(kMsgTypeRebind);
        ValidateNextEnum(kMsgTypeReply);
        ValidateNextEnum(kMsgTypeRelease);
        ValidateNextEnum(kMsgTypeDecline);
        ValidateNextEnum(kMsgTypeReconfigure);
        ValidateNextEnum(kMsgTypeInformationRequest);
    };

    const char *string = "UnknownMsg";

    VerifyOrExit(aMsgType != 0);
    VerifyOrExit(aMsgType <= kMsgTypeInformationRequest);
    string = kMsgTypeStrings[aMsgType - kMsgTypeSolicit];

exit:
    return string;
}

#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

//---------------------------------------------------------------------------------------------------------------------
// PdPrefix

TimeMilli Dhcp6PdClient::PdPrefix::DetermineT1Time(void) const { return mUpdateTime + TimeMilli::SecToMsec(mT1); }

TimeMilli Dhcp6PdClient::PdPrefix::DetermineT2Time(void) const { return mUpdateTime + TimeMilli::SecToMsec(mT2); }

TimeMilli Dhcp6PdClient::PdPrefix::DeterminePreferredTime(void) const
{
    return mUpdateTime + TimeMilli::SecToMsec(mPreferredLifetime);
}

void Dhcp6PdClient::PdPrefix::AdjustLifetimesT1AndT2(void)
{
    // We limit the preferred lifetime to `kMaxPreferredLifetime`
    // (4 hours). This ensures renewals occur within a reasonable
    // timeframe, preventing a delegated prefix from being used for an
    // excessively long duration.
    //
    // The `RoutingManager` publishes the delegated PD prefix as an OMR
    // in Network Data for its preferred lifetime (which is ensured
    // to be at least `kMinPreferredLifetime` (30 minutes)). There's no
    // benefit to maintaining the delegated prefix for much longer.
    // Therefore, the valid lifetime is adjusted to be at most
    // `kMaxValidMarginAfterPreferredLifetime` (2 minutes) longer than
    // the preferred lifetime.

    mPreferredLifetime = Min(mPreferredLifetime, kMaxPreferredLifetime);
    mValidLifetime     = Min(mValidLifetime, mPreferredLifetime + kMaxValidMarginAfterPreferredLifetime);

    // If T1 or T2 not specified, use 0.5 and 0.8 times of the
    // preferred lifetime.

    if (mT1 == 0)
    {
        mT1 = mPreferredLifetime * kDefaultT1FactorNumerator / kDefaultT1FactorDenominator;
    }

    if (mT2 == 0)
    {
        mT2 = mPreferredLifetime * kDefaultT2FactorNumerator / kDefaultT2FactorDenominator;
    }

    if (mPreferredLifetime >= kMinPreferredLifetime)
    {
        // This block handles the common scenario where the prefix has a
        // preferred lifetime of at least `kMinPreferredLifetime`
        // (30 minutes). During prefix lease renewal or rebind, the
        // server might choose not to extend the lease, returning a
        // remaining preferred lifetime shorter than 30 minutes. This
        // specific case is handled in the `else if` and `else` blocks
        // below.

        // We ensure T1 (renewal time) is at least `kMinT1` (5 minutes)
        // to prevent frequent renewals. T1 is clamped between `kMinT1`
        // and `mPreferredLifetime - kMinT1MarginBeforePreferredLifetime`
        // (15 minutes). This margin ensures sufficient time for lease
        // renewal before expiration. Similarly, T2 (rebind time) is
        // clamped between T1 and `mPreferredLifetime` minus a margin
        // of `kMinT2MarginBeforePreferredLifetime`(6 minutes).

        // Since `mPreferredLifetime` is at least 30 minutes, we know
        // that `mPreferredLifetime - 15 min` will be at least 15
        // minutes, guaranteeing it is always greater than `kMinT1`
        // (5 minutes). Additionally, `mT1` will be at most
        // `mPreferredLifetime - 15 min`, ensuring `mT1` is less than
        // `mPreferredLifetime - 6 min`. In both `Clamp` function
        // calls, the minimum value is thus guaranteed to be less than
        // the maximum value.

        mT1 = Clamp(mT1, kMinT1, mPreferredLifetime - kMinT1MarginBeforePreferredLifetime);
        mT2 = Clamp(mT2, mT1, mPreferredLifetime - kMinT2MarginBeforePreferredLifetime);
    }
    else if (mPreferredLifetime >= kMinT1)
    {
        // This block handles cases where the server cannot extend the
        // lease during renewal or rebind. In such scenarios, the
        // server may return the remaining preferred lifetime as T1 and
        // T2. This signals to the client that the leases associated
        // with the prefix will not be extended, eliminating the need
        // for further renewal attempts and preventing unnecessary
        // network traffic as the remaining lifetime approaches zero.

        mT1 = Clamp(mT1, kMinT1, mPreferredLifetime);
        mT2 = Clamp(mT2, mT1, mPreferredLifetime);
    }
    else
    {
        // If the preferred lifetime is very short (less than kMinT1),
        // set T1 and T2 directly to the remaining preferred
        // lifetime. This indicates that the lease is expiring soon
        // and no further renewal or rebind attempts are productive.

        mT1 = mPreferredLifetime;
        mT2 = mPreferredLifetime;
    }
}

//---------------------------------------------------------------------------------------------------------------------
// RetxTracker

uint32_t Dhcp6PdClient::RetxTracker::AddJitter(uint32_t aValue, JitterMode aJitterMode)
{
    // Applies a random jitter to a given value based on the selected
    // jitter mode. For `kPositiveJitter`, the returned value is in
    // the range `[aValue, aValue * 1.1]`. For `kFullJitter`, the
    // range is `[aValue * 0.9, aValue * 1.1]`.
    //
    // Follows the RFC 8415 Section 15 rules regarding retransmission
    // timeout (RT) calculation, where the timeout includes a
    // randomization factor (RAND), which is a random number chosen
    // with a uniform distribution between -0.1 and +0.1, calculating
    // `RT + RAND * RT`.
    //
    // For a Solicit message's initial timeout, the randomization is
    // required to be positive, for which `kPositiveJitter` is used.

    uint32_t randomizedValue = aValue;
    uint32_t jitter          = aValue / kJitterDivisor;

    switch (aJitterMode)
    {
    case kPositiveJitter:
        randomizedValue += Random::NonCrypto::GetUint32InRange(0, jitter);
        break;

    case kFullJitter:
        randomizedValue += Random::NonCrypto::GetUint32InRange(0, 2 * jitter) - jitter;
        break;
    }

    return randomizedValue;
}

void Dhcp6PdClient::RetxTracker::Start(uint32_t aInitialTimeout, uint32_t aMaxTimeout, JitterMode aJitterMode)
{
    TxnId oldId = mTransactionId;

    do
    {
        mTransactionId.GenerateRandom();
    } while (mTransactionId == oldId);

    mStartTime       = TimerMilli::GetNow();
    mTimeout         = AddJitter(aInitialTimeout, aJitterMode);
    mMaxTimeout      = aMaxTimeout;
    mCount           = 0;
    mMaxCount        = kInfiniteRetxCount;
    mHasEndTime      = false;
    mLongElapsedTime = false;
}

void Dhcp6PdClient::RetxTracker::ScheduleTimeoutTimer(TimerMilli &aTimer) const
{
    TimeMilli now = TimerMilli::GetNow();

    aTimer.FireAt(now + mTimeout);

    if (mHasEndTime)
    {
        aTimer.FireAtIfEarlier(mEndTime);
    }

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    {
        uint32_t delay = aTimer.GetFireTime() - now;

        LogInfo("Scheduled timer for %lu.%03u sec", ToUlong(delay / 1000), static_cast<uint16_t>(delay % 1000));
    }
#endif
}

void Dhcp6PdClient::RetxTracker::UpdateTimeoutAndCountAfterTx(void)
{
    // RFC 8415 Section 15 required retransmission timeout (RT) to be updated
    // for each subsequent tx based on the previous value using `RT =
    // 2*RTprev + RAND*RTprev` This is interpreted as `RT = RTPrev +
    // (RTprev + RAND*RTprev)`:

    mTimeout = mTimeout + AddJitter(mTimeout, kFullJitter);

    if ((mMaxTimeout != kInfiniteTimeout) && (mTimeout > mMaxTimeout))
    {
        mTimeout = AddJitter(mMaxTimeout, kFullJitter);
    }

    if (mCount != NumericLimits<uint16_t>::kMax)
    {
        mCount++;
    }
}

bool Dhcp6PdClient::RetxTracker::ShouldRetx(void) const
{
    // Determines whether we have reached max retx count or
    // retx end time has passed.

    bool shouldRetx = false;

    if (mMaxCount != kInfiniteRetxCount)
    {
        VerifyOrExit(mCount <= mMaxCount);
    }

    if (mHasEndTime)
    {
        VerifyOrExit(TimerMilli::GetNow() < mEndTime);
    }

    shouldRetx = true;

exit:
    return shouldRetx;
}

uint16_t Dhcp6PdClient::RetxTracker::DetermineElapsedTime(void)
{
    // Determine the elapsed time for `ElapsedTimeOption`. This is
    // measured from the time at which the client sent the first
    // message in the current message exchange. It is set to 0 in the
    // first message in the message exchange. Elapsed time is
    // expressed in the unit of hundredths of a second. Value
    // `0xffff` is used to represent any elapsed-time values greater
    // than the largest time value that can be represented as
    // `uint16_t`.

    uint16_t elapsed = 0;

    if (mCount == 0)
    {
        elapsed = 0;
        ExitNow();
    }

    // `mLongElapsedTime` tracks whether the elapsed time has previously
    // reached the `0xffff` limit, which occurs after approximately 656
    // seconds. Once this flag is set, we'll consistently return
    // `0xffff`, ensuring correct behavior for very long retx runs
    // (e.g., during solicit). The `mStartTime` (a `uint32_t`
    // `TimeMilli` value) tracks the transmission time of the first
    // message in the exchange. While `TimeMilli::GetNow()` can roll
    // over after/ more than 49 days, `mLongElapsedTime` is marked
    // much earlier (around 656 seconds), so we stop using `mStartTime`
    // once the limit is hit.

    if (mLongElapsedTime)
    {
        elapsed = NumericLimits<uint16_t>::kMax;
        ExitNow();
    }

    // Calculate the duration in msec, divide by 10 to convert to unit
    // of hundredths of a second. Clamp to `uint16`.

    elapsed = ClampToUint16((TimerMilli::GetNow() - mStartTime) / 10);

    if (elapsed == NumericLimits<uint16_t>::kMax)
    {
        mLongElapsedTime = true;
    }

exit:
    return elapsed;
}

} // namespace BorderRouter
} // namespace ot

#endif // OT_CONFIG_DHCP6_PD_CLIENT_ENABLE
