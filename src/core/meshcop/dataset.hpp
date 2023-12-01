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
 *
 */

#ifndef MESHCOP_DATASET_HPP_
#define MESHCOP_DATASET_HPP_

#include "openthread-core-config.h"

#include <openthread/dataset.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/const_cast.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "common/type_traits.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/mle_types.hpp"

namespace ot {
namespace MeshCoP {

/**
 * Represents MeshCop Dataset.
 *
 */
class Dataset
{
    friend class DatasetLocal;

public:
    static constexpr uint8_t kMaxSize      = OT_OPERATIONAL_DATASET_MAX_LENGTH; ///< Max size of MeshCoP Dataset (bytes)
    static constexpr uint8_t kMaxValueSize = 16;                                ///< Max size of a TLV value (bytes)
    static constexpr uint8_t kMaxGetTypes  = 64;                                ///< Max number of types in MGMT_GET.req

    /**
     * Represents the Dataset type (active or pending).
     *
     */
    enum Type : uint8_t
    {
        kActive,  ///< Active Dataset
        kPending, ///< Pending Dataset
    };

    /**
     * Represents presence of different components in Active or Pending Operational Dataset.
     *
     */
    class Components : public otOperationalDatasetComponents, public Clearable<Components>
    {
    public:
        /**
         * Indicates whether or not the Active Timestamp is present in the Dataset.
         *
         * @returns TRUE if Active Timestamp is present, FALSE otherwise.
         *
         */
        bool IsActiveTimestampPresent(void) const { return mIsActiveTimestampPresent; }

        /**
         * Indicates whether or not the Pending Timestamp is present in the Dataset.
         *
         * @returns TRUE if Pending Timestamp is present, FALSE otherwise.
         *
         */
        bool IsPendingTimestampPresent(void) const { return mIsPendingTimestampPresent; }

        /**
         * Indicates whether or not the Network Key is present in the Dataset.
         *
         * @returns TRUE if Network Key is present, FALSE otherwise.
         *
         */
        bool IsNetworkKeyPresent(void) const { return mIsNetworkKeyPresent; }

        /**
         * Indicates whether or not the Network Name is present in the Dataset.
         *
         * @returns TRUE if Network Name is present, FALSE otherwise.
         *
         */
        bool IsNetworkNamePresent(void) const { return mIsNetworkNamePresent; }

        /**
         * Indicates whether or not the Extended PAN ID is present in the Dataset.
         *
         * @returns TRUE if Extended PAN ID is present, FALSE otherwise.
         *
         */
        bool IsExtendedPanIdPresent(void) const { return mIsExtendedPanIdPresent; }

        /**
         * Indicates whether or not the Mesh Local Prefix is present in the Dataset.
         *
         * @returns TRUE if Mesh Local Prefix is present, FALSE otherwise.
         *
         */
        bool IsMeshLocalPrefixPresent(void) const { return mIsMeshLocalPrefixPresent; }

        /**
         * Indicates whether or not the Delay Timer is present in the Dataset.
         *
         * @returns TRUE if Delay Timer is present, FALSE otherwise.
         *
         */
        bool IsDelayPresent(void) const { return mIsDelayPresent; }

        /**
         * Indicates whether or not the PAN ID is present in the Dataset.
         *
         * @returns TRUE if PAN ID is present, FALSE otherwise.
         *
         */
        bool IsPanIdPresent(void) const { return mIsPanIdPresent; }

        /**
         * Indicates whether or not the Channel is present in the Dataset.
         *
         * @returns TRUE if Channel is present, FALSE otherwise.
         *
         */
        bool IsChannelPresent(void) const { return mIsChannelPresent; }

        /**
         * Indicates whether or not the PSKc is present in the Dataset.
         *
         * @returns TRUE if PSKc is present, FALSE otherwise.
         *
         */
        bool IsPskcPresent(void) const { return mIsPskcPresent; }

        /**
         * Indicates whether or not the Security Policy is present in the Dataset.
         *
         * @returns TRUE if Security Policy is present, FALSE otherwise.
         *
         */
        bool IsSecurityPolicyPresent(void) const { return mIsSecurityPolicyPresent; }

        /**
         * Indicates whether or not the Channel Mask is present in the Dataset.
         *
         * @returns TRUE if Channel Mask is present, FALSE otherwise.
         *
         */
        bool IsChannelMaskPresent(void) const { return mIsChannelMaskPresent; }
    };

    /**
     * Represents the information about the fields contained an Active or Pending Operational Dataset.
     *
     */
    class Info : public otOperationalDataset, public Clearable<Info>
    {
    public:
        /**
         * Indicates whether or not the Active Timestamp is present in the Dataset.
         *
         * @returns TRUE if Active Timestamp is present, FALSE otherwise.
         *
         */
        bool IsActiveTimestampPresent(void) const { return mComponents.mIsActiveTimestampPresent; }

        /**
         * Gets the Active Timestamp in the Dataset.
         *
         * MUST be used when Active Timestamp component is present in the Dataset, otherwise its behavior is
         * undefined.
         *
         * @param[out] aTimestamp  A reference to output the Active Timestamp in the Dataset.
         *
         */
        void GetActiveTimestamp(Timestamp &aTimestamp) const { aTimestamp.SetFromTimestamp(mActiveTimestamp); }

        /**
         * Sets the Active Timestamp in the Dataset.
         *
         * @param[in] aTimestamp   A Timestamp value.
         *
         */
        void SetActiveTimestamp(const Timestamp &aTimestamp)
        {
            aTimestamp.ConvertTo(mActiveTimestamp);
            mComponents.mIsActiveTimestampPresent = true;
        }

        /**
         * Indicates whether or not the Pending Timestamp is present in the Dataset.
         *
         * @returns TRUE if Pending Timestamp is present, FALSE otherwise.
         *
         */
        bool IsPendingTimestampPresent(void) const { return mComponents.mIsPendingTimestampPresent; }

        /**
         * Gets the Pending Timestamp in the Dataset.
         *
         * MUST be used when Pending Timestamp component is present in the Dataset, otherwise its behavior
         * is undefined.
         *
         * @param[out] aTimestamp  A reference to output the Pending Timestamp in the Dataset.
         *
         */
        void GetPendingTimestamp(Timestamp &aTimestamp) const { aTimestamp.SetFromTimestamp(mPendingTimestamp); }

        /**
         * Sets the Pending Timestamp in the Dataset.
         *
         * @param[in] aTimestamp   A Timestamp value.
         *
         */
        void SetPendingTimestamp(const Timestamp &aTimestamp)
        {
            aTimestamp.ConvertTo(mPendingTimestamp);
            mComponents.mIsPendingTimestampPresent = true;
        }

        /**
         * Indicates whether or not the Network Key is present in the Dataset.
         *
         * @returns TRUE if Network Key is present, FALSE otherwise.
         *
         */
        bool IsNetworkKeyPresent(void) const { return mComponents.mIsNetworkKeyPresent; }

        /**
         * Gets the Network Key in the Dataset.
         *
         * MUST be used when Network Key component is present in the Dataset, otherwise its behavior
         * is undefined.
         *
         * @returns The Network Key in the Dataset.
         *
         */
        const NetworkKey &GetNetworkKey(void) const { return AsCoreType(&mNetworkKey); }

        /**
         * Sets the Network Key in the Dataset.
         *
         * @param[in] aNetworkKey  A Network Key.
         *
         */
        void SetNetworkKey(const NetworkKey &aNetworkKey)
        {
            mNetworkKey                      = aNetworkKey;
            mComponents.mIsNetworkKeyPresent = true;
        }

        /**
         * Returns a reference to the Network Key in the Dataset to be updated by caller.
         *
         * @returns A reference to the Network Key in the Dataset.
         *
         */
        NetworkKey &UpdateNetworkKey(void)
        {
            mComponents.mIsNetworkKeyPresent = true;
            return AsCoreType(&mNetworkKey);
        }

        /**
         * Indicates whether or not the Network Name is present in the Dataset.
         *
         * @returns TRUE if Network Name is present, FALSE otherwise.
         *
         */
        bool IsNetworkNamePresent(void) const { return mComponents.mIsNetworkNamePresent; }

        /**
         * Gets the Network Name in the Dataset.
         *
         * MUST be used when Network Name component is present in the Dataset, otherwise its behavior is
         * undefined.
         *
         * @returns The Network Name in the Dataset.
         *
         */
        const NetworkName &GetNetworkName(void) const { return AsCoreType(&mNetworkName); }

        /**
         * Sets the Network Name in the Dataset.
         *
         * @param[in] aNetworkNameData   A Network Name Data.
         *
         */
        void SetNetworkName(const NameData &aNetworkNameData)
        {
            IgnoreError(AsCoreType(&mNetworkName).Set(aNetworkNameData));
            mComponents.mIsNetworkNamePresent = true;
        }

        /**
         * Indicates whether or not the Extended PAN ID is present in the Dataset.
         *
         * @returns TRUE if Extended PAN ID is present, FALSE otherwise.
         *
         */
        bool IsExtendedPanIdPresent(void) const { return mComponents.mIsExtendedPanIdPresent; }

        /**
         * Gets the Extended PAN ID in the Dataset.
         *
         * MUST be used when Extended PAN ID component is present in the Dataset, otherwise its behavior is
         * undefined.
         *
         * @returns The Extended PAN ID in the Dataset.
         *
         */
        const ExtendedPanId &GetExtendedPanId(void) const { return AsCoreType(&mExtendedPanId); }

        /**
         * Sets the Extended PAN ID in the Dataset.
         *
         * @param[in] aExtendedPanId   An Extended PAN ID.
         *
         */
        void SetExtendedPanId(const ExtendedPanId &aExtendedPanId)
        {
            mExtendedPanId                      = aExtendedPanId;
            mComponents.mIsExtendedPanIdPresent = true;
        }

        /**
         * Indicates whether or not the Mesh Local Prefix is present in the Dataset.
         *
         * @returns TRUE if Mesh Local Prefix is present, FALSE otherwise.
         *
         */
        bool IsMeshLocalPrefixPresent(void) const { return mComponents.mIsMeshLocalPrefixPresent; }

        /**
         * Gets the Mesh Local Prefix in the Dataset.
         *
         * MUST be used when Mesh Local Prefix component is present in the Dataset, otherwise its behavior
         * is undefined.
         *
         * @returns The Mesh Local Prefix in the Dataset.
         *
         */
        const Ip6::NetworkPrefix &GetMeshLocalPrefix(void) const
        {
            return static_cast<const Ip6::NetworkPrefix &>(mMeshLocalPrefix);
        }

        /**
         * Sets the Mesh Local Prefix in the Dataset.
         *
         * @param[in] aMeshLocalPrefix   A Mesh Local Prefix.
         *
         */
        void SetMeshLocalPrefix(const Ip6::NetworkPrefix &aMeshLocalPrefix)
        {
            mMeshLocalPrefix                      = aMeshLocalPrefix;
            mComponents.mIsMeshLocalPrefixPresent = true;
        }

        /**
         * Indicates whether or not the Delay Timer is present in the Dataset.
         *
         * @returns TRUE if Delay Timer is present, FALSE otherwise.
         *
         */
        bool IsDelayPresent(void) const { return mComponents.mIsDelayPresent; }

        /**
         * Gets the Delay Timer in the Dataset.
         *
         * MUST be used when Delay Timer component is present in the Dataset, otherwise its behavior is
         * undefined.
         *
         * @returns The Delay Timer in the Dataset.
         *
         */
        uint32_t GetDelay(void) const { return mDelay; }

        /**
         * Sets the Delay Timer in the Dataset.
         *
         * @param[in] aDelay  A Delay value.
         *
         */
        void SetDelay(uint32_t aDelay)
        {
            mDelay                      = aDelay;
            mComponents.mIsDelayPresent = true;
        }

        /**
         * Indicates whether or not the PAN ID is present in the Dataset.
         *
         * @returns TRUE if PAN ID is present, FALSE otherwise.
         *
         */
        bool IsPanIdPresent(void) const { return mComponents.mIsPanIdPresent; }

        /**
         * Gets the PAN ID in the Dataset.
         *
         * MUST be used when PAN ID component is present in the Dataset, otherwise its behavior is
         * undefined.
         *
         * @returns The PAN ID in the Dataset.
         *
         */
        Mac::PanId GetPanId(void) const { return mPanId; }

        /**
         * Sets the PAN ID in the Dataset.
         *
         * @param[in] aPanId  A PAN ID.
         *
         */
        void SetPanId(Mac::PanId aPanId)
        {
            mPanId                      = aPanId;
            mComponents.mIsPanIdPresent = true;
        }

        /**
         * Indicates whether or not the Channel is present in the Dataset.
         *
         * @returns TRUE if Channel is present, FALSE otherwise.
         *
         */
        bool IsChannelPresent(void) const { return mComponents.mIsChannelPresent; }

        /**
         * Gets the Channel in the Dataset.
         *
         * MUST be used when Channel component is present in the Dataset, otherwise its behavior is
         * undefined.
         *
         * @returns The Channel in the Dataset.
         *
         */
        uint16_t GetChannel(void) const { return mChannel; }

        /**
         * Sets the Channel in the Dataset.
         *
         * @param[in] aChannel  A Channel.
         *
         */
        void SetChannel(uint16_t aChannel)
        {
            mChannel                      = aChannel;
            mComponents.mIsChannelPresent = true;
        }

        /**
         * Indicates whether or not the PSKc is present in the Dataset.
         *
         * @returns TRUE if PSKc is present, FALSE otherwise.
         *
         */
        bool IsPskcPresent(void) const { return mComponents.mIsPskcPresent; }

        /**
         * Gets the PSKc in the Dataset.
         *
         * MUST be used when PSKc component is present in the Dataset, otherwise its behavior is undefined.
         *
         * @returns The PSKc in the Dataset.
         *
         */
        const Pskc &GetPskc(void) const { return AsCoreType(&mPskc); }

        /**
         * Set the PSKc in the Dataset.
         *
         * @param[in] aPskc  A PSKc value.
         *
         */
        void SetPskc(const Pskc &aPskc)
        {
            mPskc                      = aPskc;
            mComponents.mIsPskcPresent = true;
        }

        /**
         * Indicates whether or not the Security Policy is present in the Dataset.
         *
         * @returns TRUE if Security Policy is present, FALSE otherwise.
         *
         */
        bool IsSecurityPolicyPresent(void) const { return mComponents.mIsSecurityPolicyPresent; }

        /**
         * Gets the Security Policy in the Dataset.
         *
         * MUST be used when Security Policy component is present in the Dataset, otherwise its behavior is
         * undefined.
         *
         * @returns The Security Policy in the Dataset.
         *
         */
        const SecurityPolicy &GetSecurityPolicy(void) const { return AsCoreType(&mSecurityPolicy); }

        /**
         * Sets the Security Policy in the Dataset.
         *
         * @param[in] aSecurityPolicy  A Security Policy to set in Dataset.
         *
         */
        void SetSecurityPolicy(const SecurityPolicy &aSecurityPolicy)
        {
            mSecurityPolicy                      = aSecurityPolicy;
            mComponents.mIsSecurityPolicyPresent = true;
        }

        /**
         * Indicates whether or not the Channel Mask is present in the Dataset.
         *
         * @returns TRUE if Channel Mask is present, FALSE otherwise.
         *
         */
        bool IsChannelMaskPresent(void) const { return mComponents.mIsChannelMaskPresent; }

        /**
         * Gets the Channel Mask in the Dataset.
         *
         * MUST be used when Channel Mask component is present in the Dataset, otherwise its behavior is
         * undefined.
         *
         * @returns The Channel Mask in the Dataset.
         *
         */
        otChannelMask GetChannelMask(void) const { return mChannelMask; }

        /**
         * Sets the Channel Mask in the Dataset.
         *
         * @param[in] aChannelMask   A Channel Mask value.
         *
         */
        void SetChannelMask(otChannelMask aChannelMask)
        {
            mChannelMask                      = aChannelMask;
            mComponents.mIsChannelMaskPresent = true;
        }

        /**
         * Populates the Dataset with random fields.
         *
         * The Network Key, PSKc, Mesh Local Prefix, PAN ID, and Extended PAN ID are generated randomly (crypto-secure)
         * with Network Name set to "OpenThread-%04x" with PAN ID appended as hex. The Channel is chosen randomly from
         * radio's preferred channel mask, Channel Mask is set from radio's supported mask, and Security Policy Flags
         * from current `KeyManager` value.
         *
         * @param[in] aInstance    The OpenThread instance.
         *
         * @retval kErrorNone If the Dataset was generated successfully.
         *
         */
        Error GenerateRandom(Instance &aInstance);

        /**
         * Checks whether the Dataset is a subset of another one, i.e., all the components in the current
         * Dataset are also present in the @p aOther and the component values fully match.
         *
         * The matching of components in the two Datasets excludes Active/Pending Timestamp and Delay components.
         *
         * @param[in] aOther   The other Dataset to check against.
         *
         * @retval TRUE   The current dataset is a subset of @p aOther.
         * @retval FALSE  The current Dataset is not a subset of @p aOther.
         *
         */
        bool IsSubsetOf(const Info &aOther) const;
    };

    /**
     * Initializes the object.
     *
     */
    Dataset(void);

    /**
     * Clears the Dataset.
     *
     */
    void Clear(void);

    /**
     * Indicates whether or not the dataset appears to be well-formed.
     *
     * @returns TRUE if the dataset appears to be well-formed, FALSE otherwise.
     *
     */
    bool IsValid(void) const;

    /**
     * Indicates whether or not a given TLV type is present in the Dataset.
     *
     * @param[in] aType  The TLV type to check.
     *
     * @retval TRUE    TLV with @p aType is present in the Dataset.
     * @retval FALSE   TLV with @p aType is not present in the Dataset.
     *
     */
    bool ContainsTlv(Tlv::Type aType) const { return (FindTlv(aType) != nullptr); }

    /**
     * Indicates whether or not a given TLV type is present in the Dataset.
     *
     * @tparam  aTlvType  The TLV type to check.
     *
     * @retval TRUE    TLV of @p aTlvType is present in the Dataset.
     * @retval FALSE   TLV of @p aTlvType is not present in the Dataset.
     *
     */
    template <typename TlvType> bool Contains(void) const
    {
        return ContainsTlv(static_cast<Tlv::Type>(TlvType::kType));
    }

    /**
     * Searches for a given TLV type in the Dataset.
     *
     * @param[in] aType  The TLV type to find.
     *
     * @returns A pointer to the TLV or `nullptr` if not found.
     *
     */
    Tlv *FindTlv(Tlv::Type aType) { return AsNonConst(AsConst(this)->FindTlv(aType)); }

    /**
     * Searches for a given TLV type in the Dataset.
     *
     * @param[in] aType  The TLV type to find.
     *
     * @returns A pointer to the TLV or `nullptr` if not found.
     *
     */
    const Tlv *FindTlv(Tlv::Type aType) const;

    /**
     * Writes a TLV to the Dataset.
     *
     * If the specified TLV type already exists, it will be replaced. Otherwise, the TLV will be appended.
     *
     * @param[in] aTlv     A reference to the TLV.
     *
     * @retval kErrorNone    Successfully updated the TLV.
     * @retval kErrorNoBufs  Could not add the TLV due to insufficient buffer space.
     *
     */
    Error WriteTlv(const Tlv &aTlv);

    /**
     * Writes a TLV in the Dataset.
     *
     * If the specified TLV type already exists, it will be replaced. Otherwise, the TLV will be appended.
     *
     * @param[in]  aType     The TLV type.
     * @param[in]  aValue    A pointer to a buffer containing the TLV value.
     * @param[in]  aLength   The TLV length.
     *
     * @retval kErrorNone    Successfully updated the TLV.
     * @retval kErrorNoBufs  Could not add the TLV due to insufficient buffer space.
     *
     */
    Error WriteTlv(Tlv::Type aType, const void *aValue, uint8_t aLength);

    /**
     * Writes a simple TLV in the Dataset.
     *
     * If the specified TLV type already exists, it will be replaced. Otherwise, the TLV will be appended.
     *
     * @tparam  SimpleTlvType   The simple TLV type (must be a sub-class of `SimpleTlvInfo`).
     *
     * @param[in] aValue   The TLV value.
     *
     * @retval kErrorNone    Successfully updated the TLV.
     * @retval kErrorNoBufs  Could not add the TLV due to insufficient buffer space.
     *
     */
    template <typename SimpleTlvType> Error Write(const typename SimpleTlvType::ValueType &aValue)
    {
        return WriteTlv(static_cast<Tlv::Type>(SimpleTlvType::kType), &aValue, sizeof(aValue));
    }

    /**
     * Writes a `uint` TLV in the Dataset.
     *
     * If the specified TLV type already exists, it will be replaced. Otherwise, the TLV will be appended.
     *
     * @tparam  UintTlvType     The integer simple TLV type (must be a sub-class of `UintTlvInfo`).
     *
     * @param[in]  aValue   The TLV value.
     *
     * @retval kErrorNone    Successfully updated the TLV.
     * @retval kErrorNoBufs  Could not add the TLV due to insufficient buffer space.
     *
     */
    template <typename UintTlvType> Error Write(typename UintTlvType::UintValueType aValue)
    {
        typename UintTlvType::UintValueType value = BigEndian::HostSwap(aValue);

        return WriteTlv(static_cast<Tlv::Type>(UintTlvType::kType), &value, sizeof(value));
    }

    /**
     * Removes a TLV from the Dataset.
     *
     * If the Dataset does not contain the given TLV type, no action is performed.
     *
     * @param[in] aType  The TLV type to remove.
     *
     */
    void RemoveTlv(Tlv::Type aType);

    /**
     * Returns a pointer to the byte representation of the Dataset.
     *
     * @returns A pointer to the byte representation of the Dataset.
     *
     */
    uint8_t *GetBytes(void) { return mTlvs; }

    /**
     * Returns a pointer to the byte representation of the Dataset.
     *
     * @returns A pointer to the byte representation of the Dataset.
     *
     */
    const uint8_t *GetBytes(void) const { return mTlvs; }

    /**
     * Converts the TLV representation to structure representation.
     *
     * @param[out] aDatasetInfo  A reference to `Info` object to output the Dataset.
     *
     */
    void ConvertTo(Info &aDatasetInfo) const;

    /**
     * Converts the TLV representation to structure representation.
     *
     * @param[out] aDataset  A reference to `otOperationalDatasetTlvs` to output the Dataset.
     *
     */
    void ConvertTo(otOperationalDatasetTlvs &aDataset) const;

    /**
     * Returns the Dataset size in bytes.
     *
     * @returns The Dataset size in bytes.
     *
     */
    uint16_t GetSize(void) const { return mLength; }

    /**
     * Sets the Dataset size in bytes.
     *
     * @param[in] aSize  The Dataset size in bytes.
     *
     */
    void SetSize(uint16_t aSize) { mLength = aSize; }

    /**
     * Returns the local time the dataset was last updated.
     *
     * @returns The local time the dataset was last updated.
     *
     */
    TimeMilli GetUpdateTime(void) const { return mUpdateTime; }

    /**
     * Gets the Timestamp (Active or Pending).
     *
     * @param[in]  aType       The type: active or pending.
     * @param[out] aTimestamp  A reference to a `Timestamp` to output the value.
     *
     * @retval kErrorNone      Timestamp was read successfully. @p aTimestamp is updated.
     * @retval kErrorNotFound  Could not find the requested Timestamp TLV.
     *
     */
    Error GetTimestamp(Type aType, Timestamp &aTimestamp) const;

    /**
     * Sets the Timestamp value.
     *
     * @param[in] aType        The type: active or pending.
     * @param[in] aTimestamp   A Timestamp.
     *
     */
    void SetTimestamp(Type aType, const Timestamp &aTimestamp);

    /**
     * Reads the Dataset from a given message and checks that it is well-formed and valid.
     *
     * @param[in]  aMessage  The message to read from.
     * @param[in]  aOffset   The offset in @p aMessage to start reading the Dataset TLVs.
     * @param[in]  aLength   The dataset length in bytes.
     *
     * @retval kErrorNone    Successfully read and validated the Dataset.
     * @retval kErrorParse   Could not read or parse the dataset from @p aMessage.
     *
     */
    Error ReadFromMessage(const Message &aMessage, uint16_t aOffset, uint16_t aLength);

    /**
     * Sets the Dataset using an existing Dataset.
     *
     * If this Dataset is an Active Dataset, any Pending Timestamp and Delay Timer TLVs will be omitted in the copy
     * from @p aDataset.
     *
     * @param[in]  aType     The type of the dataset, active or pending.
     * @param[in]  aDataset  The input Dataset.
     *
     */
    void Set(Type aType, const Dataset &aDataset);

    /**
     * Sets the Dataset from a given structure representation.
     *
     * @param[in]  aDatasetInfo  The input Dataset as `Dataset::Info`.
     *
     * @retval kErrorNone         Successfully set the Dataset.
     * @retval kErrorInvalidArgs  Dataset is missing Active and/or Pending Timestamp.
     *
     */
    Error SetFrom(const Info &aDatasetInfo);

    /**
     * Sets the Dataset using @p aDataset.
     *
     * @param[in]  aDataset  The input Dataset as otOperationalDatasetTlvs.
     *
     */
    void SetFrom(const otOperationalDatasetTlvs &aDataset);

    /**
     * Appends the MLE Dataset TLV but excluding MeshCoP Sub Timestamp TLV.
     *
     * @param[in] aType          The type of the dataset, active or pending.
     * @param[in] aMessage       A message to append to.
     *
     * @retval kErrorNone    Successfully append MLE Dataset TLV without MeshCoP Sub Timestamp TLV.
     * @retval kErrorNoBufs  Insufficient available buffers to append the message with MLE Dataset TLV.
     *
     */
    Error AppendMleDatasetTlv(Type aType, Message &aMessage) const;

    /**
     * Applies the Active or Pending Dataset to the Thread interface.
     *
     * @param[in]  aInstance            A reference to the OpenThread instance.
     * @param[out] aIsNetworkKeyUpdated A pointer to where to place whether network key was updated.
     *
     * @retval kErrorNone   Successfully applied configuration.
     * @retval kErrorParse  The dataset has at least one TLV with invalid format.
     *
     */
    Error ApplyConfiguration(Instance &aInstance, bool *aIsNetworkKeyUpdated = nullptr) const;

    /**
     * Converts a Pending Dataset to an Active Dataset.
     *
     * Removes the Delay Timer and Pending Timestamp TLVs
     *
     */
    void ConvertToActive(void);

    /**
     * Returns a pointer to the start of Dataset TLVs sequence.
     *
     * @return  A pointer to the start of Dataset TLVs sequence.
     *
     */
    Tlv *GetTlvsStart(void) { return reinterpret_cast<Tlv *>(mTlvs); }

    /**
     * Returns a pointer to the start of Dataset TLVs sequence.
     *
     * @return  A pointer to start of Dataset TLVs sequence.
     *
     */
    const Tlv *GetTlvsStart(void) const { return reinterpret_cast<const Tlv *>(mTlvs); }

    /**
     * Returns a pointer to the past-the-end of Dataset TLVs sequence.
     *
     * Note that past-the-end points to the byte after the end of the last TLV in Dataset TLVs sequence.
     *
     * @return  A pointer to past-the-end of Dataset TLVs sequence.
     *
     */
    Tlv *GetTlvsEnd(void) { return reinterpret_cast<Tlv *>(mTlvs + mLength); }

    /**
     * Returns a pointer to the past-the-end of Dataset TLVs sequence.
     *
     * Note that past-the-end points to the byte after the end of the last TLV in Dataset TLVs sequence.
     *
     * @return  A pointer to past-the-end of Dataset TLVs sequence.
     *
     */
    const Tlv *GetTlvsEnd(void) const { return reinterpret_cast<const Tlv *>(mTlvs + mLength); }

    /**
     * Converts a Dataset Type to a string.
     *
     * @param[in]  aType   A Dataset type.
     *
     */
    static const char *TypeToString(Type aType);

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE

    /**
     * Saves a given TLV value in secure storage and clears the TLV value by setting all value bytes to zero.
     *
     * If the Dataset does not contain the @p aTlvType, no action is performed.
     *
     * @param[in] aTlvType    The TLV type.
     * @param[in] aKeyRef     The `KeyRef` to use with secure storage.
     *
     */
    void SaveTlvInSecureStorageAndClearValue(Tlv::Type aTlvType, Crypto::Storage::KeyRef aKeyRef);

    /**
     * Reads and updates a given TLV value in Dataset from secure storage.
     *
     * If the Dataset does not contain the @p aTlvType, no action is performed and `kErrorNone` is returned.
     *
     * @param[in] aTlvType    The TLV type.
     * @param[in] aKeyRef     The `KeyRef` to use with secure storage.
     *
     * @retval kErrorNone    Successfully read the TLV value from secure storage and updated the Dataset.
     * @retval KErrorFailed  Could not read the @aKeyRef from secure storage.
     *
     */
    Error ReadTlvFromSecureStorage(Tlv::Type aTlvType, Crypto::Storage::KeyRef aKeyRef);

#endif // OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE

private:
    void RemoveTlv(Tlv *aTlv);

    uint8_t   mTlvs[kMaxSize]; ///< The Dataset buffer
    TimeMilli mUpdateTime;     ///< Local time last updated
    uint16_t  mLength;         ///< The number of valid bytes in @var mTlvs
};

} // namespace MeshCoP

DefineCoreType(otOperationalDatasetComponents, MeshCoP::Dataset::Components);
DefineCoreType(otOperationalDataset, MeshCoP::Dataset::Info);

} // namespace ot

#endif // MESHCOP_DATASET_HPP_
