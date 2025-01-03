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
 *   This file includes definitions for managing MeshCoP Datasets.
 */

#ifndef MESHCOP_DATASET_MANAGER_HPP_
#define MESHCOP_DATASET_MANAGER_HPP_

#include "openthread-core-config.h"

#include "common/callback.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "mac/channel_mask.hpp"
#include "meshcop/dataset.hpp"
#include "net/udp6.hpp"
#include "thread/tmf.hpp"

namespace ot {

namespace MeshCoP {

class ActiveDatasetManager;
class PendingDatasetManager;

class DatasetManager : public InstanceLocator
{
    friend class ActiveDatasetManager;
    friend class PendingDatasetManager;

public:
    /**
     * Callback function pointer, invoked when a response to a MGMT_SET request is received or times out.
     */
    typedef otDatasetMgmtSetCallback MgmtSetCallback;

    /**
     * Indicates whether to check or ignore Security Policy flag when processing an MGMT_GET request message.
     *
     * This is used as input in `ProcessGetRequest().
     */
    enum SecurityPolicyCheckMode : uint8_t
    {
        kCheckSecurityPolicyFlags,  ///< Check Security Policy flags.
        kIgnoreSecurityPolicyFlags, ///< Ignore Security Policy flags.
    };

    /**
     * Returns the network Timestamp.
     *
     * @returns The network Timestamp.
     */
    const Timestamp &GetTimestamp(void) const { return mNetworkTimestamp; }

    /**
     * Clears the Operational Dataset.
     */
    void Clear(void);

    /**
     * Restores the Operational Dataset from non-volatile memory.
     *
     * @retval kErrorNone      Successfully restore the dataset.
     * @retval kErrorNotFound  There is no corresponding dataset stored in non-volatile memory.
     */
    Error Restore(void);

    /**
     * Retrieves the dataset from non-volatile memory.
     *
     * @param[out]  aDataset  Where to place the dataset.
     *
     * @retval kErrorNone      Successfully retrieved the dataset.
     * @retval kErrorNotFound  There is no corresponding dataset stored in non-volatile memory.
     */
    Error Read(Dataset &aDataset) const;

    /**
     * Retrieves the dataset from non-volatile memory.
     *
     * @param[out]  aDatasetInfo  Where to place the dataset (as `Dataset::Info`).
     *
     * @retval kErrorNone      Successfully retrieved the dataset.
     * @retval kErrorNotFound  There is no corresponding dataset stored in non-volatile memory.
     */
    Error Read(Dataset::Info &aDatasetInfo) const;

    /**
     * Retrieves the dataset from non-volatile memory.
     *
     * @param[out]  aDatasetTlvs  Where to place the dataset.
     *
     * @retval kErrorNone      Successfully retrieved the dataset.
     * @retval kErrorNotFound  There is no corresponding dataset stored in non-volatile memory.
     */
    Error Read(Dataset::Tlvs &aDatasetTlvs) const;

    /**
     * Saves the Operational Dataset in non-volatile memory.
     *
     * @param[in]  aDataset  The Operational Dataset.
     */
    void SaveLocal(const Dataset &aDataset);

    /**
     * Saves the Operational Dataset in non-volatile memory.
     *
     * @param[in]  aDatasetInfo  The Operational Dataset as `Dataset::Info`.
     */
    void SaveLocal(const Dataset::Info &aDatasetInfo);

    /**
     * Saves the Operational Dataset in non-volatile memory.
     *
     * @param[in]  aDatasetTlvs  The Operational Dataset as `Dataset::Tlvs`.
     *
     * @retval kErrorNone         Successfully saved the dataset.
     * @retval kErrorInvalidArgs  The @p aDatasetTlvs is invalid. It is too long or contains incorrect TLV formatting.
     */
    Error SaveLocal(const Dataset::Tlvs &aDatasetTlvs);

    /**
     * Sets the Operational Dataset for the partition.
     *
     * Also updates the non-volatile local version if the partition's Operational Dataset is newer. If Active
     * Operational Dataset is changed, applies the configuration to to Thread interface.
     *
     * @param[in]  aDataset  The Operational Dataset.
     *
     * @retval kErrorNone   Successfully applied configuration.
     * @retval kErrorParse  The dataset has at least one TLV with invalid format.
     */
    Error Save(const Dataset &aDataset) { return Save(aDataset, /* aAllowOlderTimestamp */ false); }

    /**
     * Retrieves the channel mask from local dataset.
     *
     * @param[out]  aChannelMask  A reference to the channel mask.
     *
     * @retval kErrorNone      Successfully retrieved the channel mask.
     * @retval kErrorNotFound  There is no valid channel mask stored in local dataset.
     */
    Error GetChannelMask(Mac::ChannelMask &aChannelMask) const;

    /**
     * Applies the Active or Pending Dataset to the Thread interface.
     *
     * @retval kErrorNone   Successfully applied configuration.
     * @retval kErrorParse  The dataset has at least one TLV with invalid format.
     */
    Error ApplyConfiguration(void) const;

    /**
     * Sends a MGMT_SET request to the Leader.
     *
     * @param[in]  aDatasetInfo  The Operational Dataset.
     * @param[in]  aTlvs         Any additional raw TLVs to include.
     * @param[in]  aLength       Number of bytes in @p aTlvs.
     * @param[in]  aCallback     A pointer to a function that is called on response reception or timeout.
     * @param[in]  aContext      A pointer to application-specific context for @p aCallback.
     *
     * @retval kErrorNone    Successfully send the meshcop dataset command.
     * @retval kErrorNoBufs  Insufficient buffer space to send.
     * @retval kErrorBusy    A previous request is ongoing.
     */
    Error SendSetRequest(const Dataset::Info &aDatasetInfo,
                         const uint8_t       *aTlvs,
                         uint8_t              aLength,
                         MgmtSetCallback      aCallback,
                         void                *aContext);

    /**
     * Sends a MGMT_GET request.
     *
     * @param[in]  aDatasetComponents  An Operational Dataset components structure specifying components to request.
     * @param[in]  aTlvTypes           A pointer to array containing additional raw TLV types to be requested.
     * @param[in]  aLength             Number of bytes in @p aTlvTypes.
     * @param[in]  aAddress            The IPv6 destination address for the MGMT_GET request.
     *
     * @retval kErrorNone     Successfully send the meshcop dataset command.
     * @retval kErrorNoBufs   Insufficient buffer space to send.
     */
    Error SendGetRequest(const Dataset::Components &aDatasetComponents,
                         const uint8_t             *aTlvTypes,
                         uint8_t                    aLength,
                         const otIp6Address        *aAddress) const;

    /**
     * Processes a MGMT_GET request message and prepares the response.
     *
     * @param[in] aRequest   The MGMT_GET request message.
     * @param[in] aCheckMode Indicates whether to check or ignore the Security Policy flags.
     *
     * @returns The prepared response, or `nullptr` if fails to parse the request or cannot allocate message.
     */
    Coap::Message *ProcessGetRequest(const Coap::Message &aRequest, SecurityPolicyCheckMode aCheckMode) const;

private:
    static constexpr uint8_t  kMaxGetTypes  = 64;   // Max number of types in MGMT_GET.req
    static constexpr uint32_t kSendSetDelay = 5000; // in msec.

    using Type = Dataset::Type;

    class TlvList : public Array<uint8_t, kMaxGetTypes>
    {
    public:
        TlvList(void) = default;
        void Add(uint8_t aTlvType);
    };

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    using KeyRef = Crypto::Storage::KeyRef;

    struct SecurelyStoredTlv
    {
        KeyRef GetKeyRef(Dataset::Type aType) const
        {
            return (aType == Dataset::kActive) ? mActiveKeyRef : mPendingKeyRef;
        }

        Tlv::Type mTlvType;
        KeyRef    mActiveKeyRef;
        KeyRef    mPendingKeyRef;
    };

    static const SecurelyStoredTlv kSecurelyStoredTlvs[];
#endif

#if OPENTHREAD_FTD
    enum MgmtCommand : uint8_t
    {
        kMgmtSet,
        kMgmtReplace,
    };

    struct RequestInfo : Clearable<RequestInfo> // Info from a MGMT_SET or MGMT_REPLACE request.
    {
        Dataset mDataset;
        bool    mIsFromCommissioner;
        bool    mAffectsConnectivity;
        bool    mAffectsNetworkKey;
    };
#endif

    DatasetManager(Instance &aInstance, Type aType, TimerMilli::Handler aTimerHandler);

    bool  IsActiveDataset(void) const { return (mType == Dataset::kActive); }
    bool  IsPendingDataset(void) const { return (mType == Dataset::kPending); }
    void  Restore(const Dataset &aDataset);
    Error ApplyConfiguration(const Dataset &aDataset) const;
    void  HandleGet(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const;
    void  HandleTimer(void);
    Error Save(const Dataset &aDataset, bool aAllowOlderTimestamp);
    void  LocalSave(const Dataset &aDataset);
    void  SignalDatasetChange(void) const;
    void  SyncLocalWithLeader(const Dataset &aDataset);
    Error SendSetRequest(const Dataset &aDataset);
    void  HandleMgmtSetResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aError);

    static void HandleMgmtSetResponse(void                *aContext,
                                      otMessage           *aMessage,
                                      const otMessageInfo *aMessageInfo,
                                      Error                aError);

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    void  MoveKeysToSecureStorage(Dataset &aDataset) const;
    void  DestroySecurelyStoredKeys(void) const;
    void  EmplaceSecurelyStoredKeys(Dataset &aDataset) const;
    void  SaveTlvInSecureStorageAndClearValue(Dataset &aDataset, Tlv::Type aTlvType, KeyRef aKeyRef) const;
    Error ReadTlvFromSecureStorage(Dataset &aDataset, Tlv::Type aTlvType, KeyRef aKeyRef) const;
#endif

#if OPENTHREAD_FTD
    Error HandleSetOrReplace(MgmtCommand aCommand, const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    Error ProcessSetOrReplaceRequest(MgmtCommand aCommand, const Coap::Message &aMessage, RequestInfo &aInfo) const;
    void  SendSetOrReplaceResponse(const Coap::Message    &aRequest,
                                   const Ip6::MessageInfo &aMessageInfo,
                                   StateTlv::State         aState);
#endif

    Type                      mType;
    bool                      mLocalSaved : 1;
    bool                      mMgmtPending : 1;
    TimeMilli                 mLocalUpdateTime;
    Timestamp                 mLocalTimestamp;
    Timestamp                 mNetworkTimestamp;
    TimerMilli                mTimer;
    Callback<MgmtSetCallback> mMgmtSetCallback;
};

//----------------------------------------------------------------------------------------------------------------------

class ActiveDatasetManager : public DatasetManager, private NonCopyable
{
    friend class Tmf::Agent;

public:
    /**
     * Initializes the ActiveDatasetManager object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit ActiveDatasetManager(Instance &aInstance);

    /**
     * Indicates whether the Active Dataset is partially complete.
     *
     * Is primarily used to determine whether a user has supplied a partial Active Dataset for use
     * with joining a network.
     *
     * @retval TRUE   If an Active Dataset is saved but does not include an Active Timestamp.
     * @retval FALSE  If an Active Dataset is not saved or does include an Active Timestamp.
     */
    bool IsPartiallyComplete(void) const;

    /**
     * Indicates whether the Active Dataset is complete.
     *
     * @retval TRUE   If an Active Dataset is saved and includes an Active Timestamp.
     * @retval FALSE  If an Active Dataset is not saved or does include an Active Timestamp.
     */
    bool IsComplete(void) const;

    /**
     * Indicates whether or not a valid network is present in the Active Operational Dataset.
     *
     * @retval TRUE if a valid network is present in the Active Dataset.
     * @retval FALSE if a valid network is not present in the Active Dataset.
     */
    bool IsCommissioned(void) const;

#if OPENTHREAD_FTD

    /**
     * Creates a new Operational Dataset to use when forming a new network.
     *
     * @param[out]  aDatasetInfo  The Operational Dataset as `Dataset::Info`.
     *
     * @retval kErrorNone    Successfully created a new Operational Dataset.
     * @retval kErrorFailed  Failed to generate random values for new parameters.
     */
    Error CreateNewNetwork(Dataset::Info &aDatasetInfo) { return aDatasetInfo.GenerateRandom(GetInstance()); }

    /**
     * Starts the Leader functions for maintaining the Active Operational Dataset.
     */
    void StartLeader(void);

#if OPENTHREAD_CONFIG_OPERATIONAL_DATASET_AUTO_INIT
    /**
     * Generate a default Active Operational Dataset.
     *
     * @retval kErrorNone          Successfully generated an Active Operational Dataset.
     * @retval kErrorAlready       A valid Active Operational Dataset already exists.
     * @retval kErrorInvalidState  Device is not currently attached to a network.
     */
    Error GenerateLocal(void);
#endif
#endif

private:
    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void) { DatasetManager::HandleTimer(); }
};

DeclareTmfHandler(ActiveDatasetManager, kUriActiveGet);
#if OPENTHREAD_FTD
DeclareTmfHandler(ActiveDatasetManager, kUriActiveSet);
DeclareTmfHandler(ActiveDatasetManager, kUriActiveReplace);
#endif

//----------------------------------------------------------------------------------------------------------------------

class PendingDatasetManager : public DatasetManager, private NonCopyable
{
    friend class Tmf::Agent;
    friend class DatasetManager;

public:
    /**
     * Initializes the PendingDatasetManager object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit PendingDatasetManager(Instance &aInstance);

    /**
     * Reads the Active Timestamp in the Pending Operational Dataset.
     *
     * @param[out] aTimestamp A reference to return the read timestamp.
     *
     * @retval kErrorNone     The active timestamp was successfully fetched.
     * @retval kErrorNotFound The pending dataset is not currently valid.
     */
    Error ReadActiveTimestamp(Timestamp &aTimestamp) const;

    /**
     * Reads the remaining delay time in ms.
     *
     * @param[out] aRemainingDelay A reference to return the remaining delay time.
     *
     * @retval kErrorNone     The remaining delay time was successfully fetched.
     * @retval kErrorNotFound The pending dataset is not currently valid.
     */
    Error ReadRemainingDelay(uint32_t &aRemainingDelay) const;

#if OPENTHREAD_FTD
    /**
     * Starts the Leader functions for maintaining the Active Operational Dataset.
     */
    void StartLeader(void);
#endif

private:
#if OPENTHREAD_FTD
    void ApplyActiveDataset(Dataset &aDataset);
#endif

    void StartDelayTimer(void);
    void StartDelayTimer(const Dataset &aDataset);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void) { DatasetManager::HandleTimer(); }

    void                     HandleDelayTimer(void);
    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    using DelayTimer = TimerMilliIn<PendingDatasetManager, &PendingDatasetManager::HandleDelayTimer>;

    DelayTimer mDelayTimer;
};

DeclareTmfHandler(PendingDatasetManager, kUriPendingGet);
#if OPENTHREAD_FTD
DeclareTmfHandler(PendingDatasetManager, kUriPendingSet);
#endif

} // namespace MeshCoP
} // namespace ot

#endif // MESHCOP_DATASET_MANAGER_HPP_
