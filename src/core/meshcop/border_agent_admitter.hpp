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
 *   This file includes definitions for the Border Agent Admitter.
 */

#ifndef OT_CORE_MESHCOP_BORDER_AGENT_ADMITTER_HPP_
#define OT_CORE_MESHCOP_BORDER_AGENT_ADMITTER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLE

#if !OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
#error "Border Admitter requires OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE"
#endif

#include <openthread/border_agent_admitter.h>

#include "common/error.hpp"
#include "common/heap_allocatable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/owning_list.hpp"
#include "common/tasklet.hpp"
#include "common/time.hpp"
#include "common/timer.hpp"
#include "common/uptime.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "net/ip6_address.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"
#include "thread/network_data_publisher.hpp"
#include "thread/tmf.hpp"

namespace ot {
namespace MeshCoP {
namespace BorderAgent {

class Manager;

/**
 * Implements a Border Agent Admitter.
 */
class Admitter : public InstanceLocator, private NonCopyable
{
    friend class Manager;
    friend class ot::Notifier;
    friend class NetworkData::Publisher;

    struct Joiner;

public:
    typedef otBorderAdmitterEnrollerInfo EnrollerInfo; ///< Information about an enroller.
    typedef otBorderAdmitterJoinerInfo   JoinerInfo;   ///< Information about a joiner accepted by an enroller.

    /**
     * Represents an iterator for enrollers and joiners accepted by an enroller.
     */
    class Iterator : public otBorderAdmitterIterator
    {
        friend class Admitter;

    public:
        /**
         * Initializes the `Iterator`.
         *
         * @param[in] aInstance  The OpenThread instance.
         */
        void Init(Instance &aInstance);

        /**
         * Retrieves the next enroller information.
         *
         * @param[out] aEnrollerInfo  A `EnrollerInfo` to populate.
         *
         * @retval kErrorNone         Successfully retrieved the next session. @p aEnrollerInfo is updated.
         * @retval kErrorNotFound     No more entries are available. The end of the list has been reached.
         */
        Error GetNextEnrollerInfo(EnrollerInfo &aEnrollerInfo);

        /**
         * Retrieves the information about next accepted joiner by the latest retrieved enroller during iteration.
         *
         * @param[out] aJoinerInfo    A `JoinerInfo` to populate.
         *
         * @retval kErrorNone         Successfully retrieved the next session. @p aJoinerInfo is updated.
         * @retval kErrorNotFound     No more entries are available. The end of the list has been reached.
         */
        Error GetNextJoinerInfo(JoinerInfo &aJoinerInfo);

    private:
        void           SetSession(SecureSession *aSession) { mPtr1 = aSession; }
        SecureSession *GetSession(void) const { return static_cast<SecureSession *>(mPtr1); }
        void           SetJoiner(Joiner *aJoiner) { mPtr2 = aJoiner; }
        Joiner        *GetJoiner(void) const { return static_cast<Joiner *>(mPtr2); }
        UptimeMsec     GetInitUptime(void) const { return mData1; }
        void           SetInitUptime(UptimeMsec aUptime) { mData1 = aUptime; }
        void           SetInitTime(TimeMilli aNow) { mData2 = aNow.GetValue(); }
        TimeMilli      GetInitTime(void) const { return TimeMilli(mData2); }

        static void SkipToNextEnrollerSession(SecureSession *&aSession);
    };

    /**
     * Initializes the `Admitter`.
     *
     * @param[in] aInstance    The OpenThread instance.
     */
    explicit Admitter(Instance &aInstance);

    /**
     * Enables/Disables the Admitter functionality.
     *
     * @param[in] aEnable   TRUE to enable, FALSE to disable.
     */
    void SetEnabled(bool aEnable);

    /**
     * Indicates whether or not the Admitter functionality is enabled.
     *
     * @retval TRUE    The Admitter functionality is enabled.
     * @retval FALSE   The Admitter functionality is disabled.
     */
    bool IsEnabled(void) const { return mEnabled; }

    /**
     * Indicates whether or not the Admitter is selected and as the prime Admitter within the Thread mesh.
     *
     * @retval TRUE    The Admitter is the prime Admitter.
     * @retval FALSE   The Admitter is not the prime Admitter.
     */
    bool IsPrimeAdmitter(void) const { return mArbitrator.IsPrimeAdmitter(); }

    /**
     * Indicates whether or not the Admitter is currently acting as the native Commissioner within the Thread mesh.
     *
     * @retval TRUE    The Admitter is currently native Commissioner.
     * @retval FALSE   The Admitter is not a native Commissioner.
     */
    bool IsActiveCommissioner(void) const { return mCommissionerPetitioner.IsActiveCommissioner(); }

    /**
     * Indicates whether the Admitter's petition to become the native commissioner within mesh was rejected.
     *
     * A rejection typically occurs if there is already another active commissioner in the Thread network.
     *
     * @retval TRUE   The petition was rejected.
     * @retval FALSE  The petition was not rejected.
     */
    bool IsPetitionRejected(void) const { return mCommissionerPetitioner.IsPetitionRejected(); }

    /**
     * Gets the Joiner UDP port.
     *
     * Zero value indicates the Joiner UDP port is not specified/fixed by the Admitter (Joiner Routers can pick).
     *
     * @returns The joiner UDP port number.
     */
    uint16_t GetJoinerUdpPort(void) const { return mCommissionerPetitioner.GetJoinerUdpPort(); }

    /**
     * Sets the joiner UDP port.
     *
     * Zero value indicates the Joiner UDP port is not specified/fixed by the Admitter (Joiner Routers can pick).
     *
     * @param[in] aInstance  The OpenThread instance.
     * @param[in] aUdpPort   The UDP port number.
     */
    void SetJoinerUdpPort(uint16_t aUdpPort) { mCommissionerPetitioner.SetJoinerUdpPort(aUdpPort); }

    /**
     * Gets the commissioner session ID.
     *
     * @returns The commissioner session ID when `IsActiveCommissioner()`, zero otherwise.
     */
    uint16_t GetCommissionerSessionId(void) const { return mCommissionerPetitioner.GetSessionId(); }

private:
    //-----------------------------------------------------------------------------------------------------------------
    // Constants and enumerations

    static constexpr bool     kEnabledByDefault     = OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLED_BY_DEFAULT;
    static constexpr uint16_t kDefaultJoinerUdpPort = OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_DEFAULT_JOINER_UDP_PORT;

    static constexpr uint32_t kEnrollerKeepAliveTimeout = 50 * Time::kOneSecondInMsec;

    enum State : uint8_t
    {
        kStateUnavailable   = 0, // Cannot act as Admitter (e.g. disabled/stopped, or there is another Admitter).
        kStateReady         = 1, // Admitter is ready to accept Enroller registrations but not yet active.
        kStateActive        = 2, // Admitter is fully active (it is the native mesh commissioner).
        kStateConflictError = 3, // Admitter could not become active (e.g., another commissioner is active).
    };

    //-----------------------------------------------------------------------------------------------------------------
    // Nested Types

    struct Joiner : public InstanceLocator, LinkedListEntry<Joiner>, Heap::Allocatable<Joiner>
    {
        // Track information about a Joiner which is accepted by
        // an Enroller.

        static constexpr uint32_t kTimeout = 7 * Time::kOneMinuteInMsec;

        Joiner(Instance &aInstance, const Ip6::InterfaceIdentifier &aIid);

        void UpdateExpirationTime(void);
        bool Matches(const Ip6::InterfaceIdentifier &aIid) const;
        bool Matches(const ExpirationChecker &aChecker) const;

        Joiner                  *mNext;
        Ip6::InterfaceIdentifier mIid;
        UptimeMsec               mAcceptUptime;
        TimeMilli                mExpirationTime;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    struct Enroller : Heap::Allocatable<Enroller>
    {
        // Tracks information for a registered enroller. This is
        // included in `CoapDtlsSession` as the member variable
        // `mEnroller` of type `OwnedPtr<Enroller>`. If the
        // session is not for an enroller, the `OwnedPtr` will
        // be null.

        bool ShouldForwardJoinerRelay(void) const { return (mMode & EnrollerModeTlv::kForwardJoinerRelayRx); }
        bool ShouldForwardUdpProxy(void) const { return (mMode & EnrollerModeTlv::kForwardUdpProxyRx); }

        EnrollerIdTlv::StringType mId;
        SteeringData              mSteeringData;
        OwningList<Joiner>        mJoiners;
        UptimeMsec                mRegisterUptime;
        uint8_t                   mMode;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class EnrollerIterator : private NonCopyable
    {
        // Iterates over Border Agent sessions and returns only the
        // ones acting as Enroller.

    public:
        explicit EnrollerIterator(Instance &aInstance);

        bool      IsDone(void) const { return (mSession == nullptr); }
        void      Advance(void);
        Enroller *GetEnroller(void) const;
        uint16_t  GetSessionIndex(void) const;

        template <typename Type> Type *GetSessionAs(void) const { return static_cast<Type *>(mSession); }

    private:
        void FindNextEnroller(void) { Iterator::SkipToNextEnrollerSession(mSession); }

        SecureSession *mSession;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleNetDataPublisherEvent(NetworkData::Publisher::Event aEvent) { mArbitrator.HandlePublisherEvent(aEvent); }
    void HandleArbitratorTimer(void) { mArbitrator.HandleTimer(); }

    class Arbitrator : public InstanceLocator, private NonCopyable
    {
        // Coordinates between Border Agents within mesh to decide which
        // one can take the prime Admitter role.

    public:
        explicit Arbitrator(Instance &aInstance);

        void Start(void);
        void Stop(void);
        bool IsPrimeAdmitter(void) const { return mState == kPrime; }

        void HandlePublisherEvent(NetworkData::Publisher::Event aEvent);
        void HandleTimer(void);

    private:
        static constexpr uint32_t kDelayToBecomePrime = 18 * Time::kOneSecondInMsec;

        enum State : uint8_t
        {
            kStopped,   // Stopped.
            kClaiming,  // Actively trying to claim the prime role (publishing netdata service).
            kCandidate, // Is a candidate (netdata service is added - delaying before becoming prime).
            kPrime,     // Is the prime admitter.
        };

        void SetState(State aState);

        static const char *StateToString(State aState);

        using DelayTimer = TimerMilliIn<Admitter, &Admitter::HandleArbitratorTimer>;

        State      mState;
        DelayTimer mTimer;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleCommissionerPetitionerRetryTimer(void) { mCommissionerPetitioner.HandleRetryTimer(); }
    void HandleCommissionerPetitionerKeepAliveTimer(void) { mCommissionerPetitioner.HandleKeepAliveTimer(); }

    class CommissionerPetitioner : public InstanceLocator, private NonCopyable
    {
        // Manages becoming the native commissioner within mesh.

    public:
        enum State : uint8_t
        {
            kStopped,             // Stopped.
            kToPetition,          // To send petition to leader for becoming commissioner.
            kPetitioning,         // Petition request sent, waiting for response from leader.
            kAcceptedToSyncData,  // Petition accepted - need to sync commissioner data with leader.
            kAcceptedSyncingData, // Petition accepted - commissioner data sync in progress, waiting for response.
            kAcceptedDataSynced,  // Petition accepted - commissioner data sync done.
            kRejected,            // Petition rejected (another commissioner is active).
        };

        explicit CommissionerPetitioner(Instance &aInstance);

        void                Start(void);
        void                Stop(void);
        State               GetState(void) const { return mState; }
        bool                IsActiveCommissioner(void) const;
        bool                IsPetitionRejected(void) const { return (mState == kRejected); }
        uint16_t            GetJoinerUdpPort(void) const { return mJoinerUdpPort; }
        void                SetJoinerUdpPort(uint16_t aUdpPort);
        uint16_t            GetSessionId(void) const { return mSessionId; }
        const Ip6::Address &GetAloc(void) const { return mAloc.GetAddress(); }

        void HandleEnrollerChange(void);
        void HandleNetDataChange(void);
        void HandleRetryTimer(void);
        void HandleKeepAliveTimer(void);

    private:
        // All intervals are in msec
        static constexpr uint32_t kPetitionRetryDelay   = Time::kOneSecondInMsec;
        static constexpr uint16_t kPetitionRetryJitter  = 100;
        static constexpr uint32_t kKeepAliveTxInterval  = 25 * Time::kOneSecondInMsec; // Half of timeout used by leader
        static constexpr uint16_t kKeepAliveTxJitter    = 500;
        static constexpr uint32_t kKeepAliveRetryDelay  = Time::kOneSecondInMsec;
        static constexpr uint16_t kKeepAliveRetryJitter = 100;
        static constexpr uint32_t kDataSyncRetryDelay   = Time::kOneSecondInMsec;
        static constexpr uint16_t kDataSyncRetryJitter  = 100;

        void  SetState(State aState);
        void  SendPetitionIfNoOtherCommissioner(void);
        void  SchedulePetitionRetry(void);
        Error ProcessPetitionResponse(const Coap::Msg *aMsg, Error aResult);
        void  AddAlocAndUdpReceiver(void);
        void  RemoveAlocAndUdpReceiver(void);
        void  ScheduleNextKeepAlive(void);
        void  ScheduleKeepAliveRetry(void);
        Error SendKeepAlive(StateTlv::State aState);
        void  SendDataSet(void);
        Error ProcessDataSetResponse(const Coap::Msg *aMsg, Error aResult);
        void  ScheduleDataSyncRetry(void);
        void  ScheduleImmediateDataSync(void);
        Error SendToLeader(OwnedPtr<Coap::Message> aMessage, Coap::ResponseHandler aHandler);
        bool  HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

        DeclareTmfResponseHandlerIn(CommissionerPetitioner, HandlePetitionResponse);
        DeclareTmfResponseHandlerIn(CommissionerPetitioner, HandleKeepAliveResponse);
        DeclareTmfResponseHandlerIn(CommissionerPetitioner, HandleDataSetResponse);

        static bool HandleUdpReceive(void *aContext, const otMessage *aMessage, const otMessageInfo *aMessageInfo);
        static const char *StateToString(State aState);

        using RetryTimer     = TimerMilliIn<Admitter, &Admitter::HandleCommissionerPetitionerRetryTimer>;
        using KeepAliveTimer = TimerMilliIn<Admitter, &Admitter::HandleCommissionerPetitionerKeepAliveTimer>;

        State                      mState;
        SteeringData               mSteeringData;
        uint16_t                   mJoinerUdpPort;
        uint16_t                   mSessionId;
        RetryTimer                 mRetryTimer;
        KeepAliveTimer             mKeepAliveTimer;
        Ip6::Udp::Receiver         mUdpReceiver;
        Ip6::Netif::UnicastAddress mAloc;
    };

    //-----------------------------------------------------------------------------------------------------------------
    // Methods

    State DetermineState(void) const;
    void  EvaluateOperation(void);
    void  PostReportStateTask(void);
    void  HandleReportStateTask(void);
    void  DetermineSteeringData(SteeringData &aSteeringData) const;
    void  ForwardJoinerRelayToEnrollers(const Coap::Msg &aMsg);
    void  ForwardUdpProxyToEnrollers(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void  HandleJoinerTimer(void);
    void  HandleEnrollerChange(void);
    void  HandleNotifierEvents(Events aEvents);

    static const char *EnrollerUriToString(Uri aUri);

    //-----------------------------------------------------------------------------------------------------------------
    // Variables and Constants

    using JoinerTimer     = TimerMilliIn<Admitter, &Admitter::HandleJoinerTimer>;
    using ReportStateTask = TaskletIn<Admitter, &Admitter::HandleReportStateTask>;

    static const uint8_t kEnrollerValidSteeringDataLengths[];

    bool                   mEnabled;
    bool                   mHasAnyEnroller;
    Arbitrator             mArbitrator;
    CommissionerPetitioner mCommissionerPetitioner;
    JoinerTimer            mJoinerTimer;
    ReportStateTask        mReportStateTask;
    State                  mLastSyncedState;
};

} // namespace BorderAgent
} // namespace MeshCoP

DefineCoreType(otBorderAdmitterIterator, MeshCoP::BorderAgent::Admitter::Iterator);

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLE

#endif // OT_CORE_MESHCOP_BORDER_AGENT_ADMITTER_HPP_
