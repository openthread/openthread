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
     * Represents a Dataset as a sequence of TLVs.
     *
     */
    typedef otOperationalDatasetTlvs Tlvs;

    /**
     * Represents a component in Dataset.
     *
     */
    enum Component : uint8_t
    {
        kActiveTimestamp,  ///< Active Timestamp
        kPendingTimestamp, ///< Pending Timestamp
        kNetworkKey,       ///< Network Key
        kNetworkName,      ///< Network Name
        kExtendedPanId,    ///< Extended PAN Identifier
        kMeshLocalPrefix,  ///< Mesh Local Prefix
        kDelay,            ///< Delay
        kPanId,            ///< PAN Identifier
        kChannel,          ///< Channel
        kPskc,             ///< PSKc
        kSecurityPolicy,   ///< Security Policy
        kChannelMask,      ///< Channel Mask
    };

    template <Component kComponent> struct TypeFor; ///< Specifies the associate type for a given `Component`.

    class Info;

    /**
     * Represents presence of different components in Active or Pending Operational Dataset.
     *
     */
    class Components : public otOperationalDatasetComponents, public Clearable<Components>
    {
        friend class Info;

    public:
        /**
         * Indicates whether or not the specified `kComponent` is present in the Dataset.
         *
         * @tparam kComponent  The component to check.
         *
         * @retval TRUE   The component is present in the Dataset.
         * @retval FALSE  The component is not present in the Dataset.
         *
         */
        template <Component kComponent> bool IsPresent(void) const;

    private:
        template <Component kComponent> void MarkAsPresent(void);
    };

    /**
     * Represents the information about the fields contained an Active or Pending Operational Dataset.
     *
     */
    class Info : public otOperationalDataset, public Clearable<Info>
    {
    public:
        /**
         * Indicates whether or not the specified component is present in the Dataset.
         *
         * @tparam kComponent  The component to check.
         *
         * @retval TRUE   The component is present in the Dataset.
         * @retval FALSE  The component is not present in the Dataset.
         *
         */
        template <Component kComponent> bool IsPresent(void) const { return GetComponents().IsPresent<kComponent>(); }

        /**
         * Gets the specified component in the Dataset.
         *
         * @tparam  kComponent  The component to check.
         *
         * MUST be used when component is present in the Dataset, otherwise its behavior is undefined.
         *
         * @returns The component value.
         *
         */
        template <Component kComponent> const typename TypeFor<kComponent>::Type &Get(void) const;

        /**
         * Gets the specified component in the Dataset.
         *
         * @tparam  kComponent  The component to check.
         *
         * MUST be used when component is present in the Dataset, otherwise its behavior is undefined.
         *
         * @pram[out] aComponent  A reference to output the component value.
         *
         */
        template <Component kComponent> void Get(typename TypeFor<kComponent>::Type &aComponent) const;

        /**
         * Sets the specified component in the Dataset.
         *
         * @tparam  kComponent  The component to set.
         *
         * @param[in] aComponent   The component value.
         *
         */
        template <Component kComponent> void Set(const typename TypeFor<kComponent>::Type &aComponent)
        {
            GetComponents().MarkAsPresent<kComponent>();
            AsNonConst(Get<kComponent>()) = aComponent;
        }

        /**
         * Returns a reference to the specified component in the Dataset to be updated by caller.
         *
         * @tparam  kComponent  The component to set.
         *
         * @returns A reference to the component in the Dataset.
         *
         */
        template <Component kComponent> typename TypeFor<kComponent>::Type &Update(void)
        {
            GetComponents().MarkAsPresent<kComponent>();
            return AsNonConst(Get<kComponent>());
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

    private:
        Components       &GetComponents(void) { return static_cast<Components &>(mComponents); }
        const Components &GetComponents(void) const { return static_cast<const Components &>(mComponents); }
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
     * @param[out] aTlvs  A reference to output the Dataset as a sequence of TLVs.
     *
     */
    void ConvertTo(Tlvs &aTlvs) const;

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
     * @param[in]  aDataset  The input Dataset as `Tlvs`.
     *
     */
    void SetFrom(const Tlvs &aTlvs);

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
     *
     * @retval kErrorNone   Successfully applied configuration.
     * @retval kErrorParse  The dataset has at least one TLV with invalid format.
     *
     */
    Error ApplyConfiguration(Instance &aInstance) const;

    /**
     * Applies the Active or Pending Dataset to the Thread interface.
     *
     * @param[in]  aInstance              A reference to the OpenThread instance.
     * @param[out] aIsNetworkKeyUpdated   Variable to return whether network key was updated.
     *
     * @retval kErrorNone   Successfully applied configuration, @p aIsNetworkKeyUpdated is changed.
     * @retval kErrorParse  The dataset has at least one TLV with invalid format.
     *
     */
    Error ApplyConfiguration(Instance &aInstance, bool &aIsNetworkKeyUpdated) const;

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

//---------------------------------------------------------------------------------------------------------------------
// Template specializations

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// `Dataset::Components::IsPresent()` and `Dataset::Components::MarkAsPresent()`

#define DefineIsPresentAndMarkAsPresent(Component)                                            \
    template <> inline bool Dataset::Components::IsPresent<Dataset::k##Component>(void) const \
    {                                                                                         \
        return mIs##Component##Present;                                                       \
    }                                                                                         \
                                                                                              \
    template <> inline void Dataset::Components::MarkAsPresent<Dataset::k##Component>(void)   \
    {                                                                                         \
        mIs##Component##Present = true;                                                       \
    }

// clang-format off

DefineIsPresentAndMarkAsPresent(ActiveTimestamp)
DefineIsPresentAndMarkAsPresent(PendingTimestamp)
DefineIsPresentAndMarkAsPresent(NetworkKey)
DefineIsPresentAndMarkAsPresent(NetworkName)
DefineIsPresentAndMarkAsPresent(ExtendedPanId)
DefineIsPresentAndMarkAsPresent(MeshLocalPrefix)
DefineIsPresentAndMarkAsPresent(Delay)
DefineIsPresentAndMarkAsPresent(PanId)
DefineIsPresentAndMarkAsPresent(Channel)
DefineIsPresentAndMarkAsPresent(Pskc)
DefineIsPresentAndMarkAsPresent(SecurityPolicy)
DefineIsPresentAndMarkAsPresent(ChannelMask)

#undef DefineIsPresentAndMarkAsPresent

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// `Dataset::TypeFor<>`

template <> struct Dataset::TypeFor<Dataset::kActiveTimestamp>  { using Type = Timestamp; };
template <> struct Dataset::TypeFor<Dataset::kPendingTimestamp> { using Type = Timestamp; };
template <> struct Dataset::TypeFor<Dataset::kNetworkKey>       { using Type = NetworkKey; };
template <> struct Dataset::TypeFor<Dataset::kNetworkName>      { using Type = NetworkName; };
template <> struct Dataset::TypeFor<Dataset::kExtendedPanId>    { using Type = ExtendedPanId; };
template <> struct Dataset::TypeFor<Dataset::kMeshLocalPrefix>  { using Type = Ip6::NetworkPrefix; };
template <> struct Dataset::TypeFor<Dataset::kDelay>            { using Type = uint32_t; };
template <> struct Dataset::TypeFor<Dataset::kPanId>            { using Type = Mac::PanId; };
template <> struct Dataset::TypeFor<Dataset::kChannel>          { using Type = uint16_t; };
template <> struct Dataset::TypeFor<Dataset::kPskc>             { using Type = Pskc; };
template <> struct Dataset::TypeFor<Dataset::kSecurityPolicy>   { using Type = SecurityPolicy; };
template <> struct Dataset::TypeFor<Dataset::kChannelMask>      { using Type = uint32_t; };

// clang-format on

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Dataset::Info::Get<>()

template <> inline const NetworkKey &Dataset::Info::Get<Dataset::kNetworkKey>(void) const
{
    return AsCoreType(&mNetworkKey);
}

template <> inline const NetworkName &Dataset::Info::Get<Dataset::kNetworkName>(void) const
{
    return AsCoreType(&mNetworkName);
}

template <> inline const ExtendedPanId &Dataset::Info::Get<Dataset::kExtendedPanId>(void) const
{
    return AsCoreType(&mExtendedPanId);
}

template <> inline const Ip6::NetworkPrefix &Dataset::Info::Get<Dataset::kMeshLocalPrefix>(void) const
{
    return static_cast<const Ip6::NetworkPrefix &>(mMeshLocalPrefix);
}

template <> inline const uint32_t &Dataset::Info::Get<Dataset::kDelay>(void) const { return mDelay; }

template <> inline const Mac::PanId &Dataset::Info::Get<Dataset::kPanId>(void) const { return mPanId; }

template <> inline const uint16_t &Dataset::Info::Get<Dataset::kChannel>(void) const { return mChannel; }

template <> inline const Pskc &Dataset::Info::Get<Dataset::kPskc>(void) const { return AsCoreType(&mPskc); }

template <> inline const SecurityPolicy &Dataset::Info::Get<Dataset::kSecurityPolicy>(void) const
{
    return AsCoreType(&mSecurityPolicy);
}

template <> inline const uint32_t &Dataset::Info::Get<Dataset::kChannelMask>(void) const { return mChannelMask; }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Active and Pending Timestamp

template <> inline void Dataset::Info::Get<Dataset::kActiveTimestamp>(Timestamp &aTimestamp) const
{
    aTimestamp.SetFromTimestamp(mActiveTimestamp);
}

template <> inline void Dataset::Info::Get<Dataset::kPendingTimestamp>(Timestamp &aTimestamp) const
{
    aTimestamp.SetFromTimestamp(mPendingTimestamp);
}

template <> inline void Dataset::Info::Set<Dataset::kActiveTimestamp>(const Timestamp &aTimestamp)
{
    GetComponents().MarkAsPresent<kActiveTimestamp>();
    aTimestamp.ConvertTo(mActiveTimestamp);
}

template <> inline void Dataset::Info::Set<Dataset::kPendingTimestamp>(const Timestamp &aTimestamp)
{
    GetComponents().MarkAsPresent<kPendingTimestamp>();
    aTimestamp.ConvertTo(mPendingTimestamp);
}

} // namespace MeshCoP

DefineCoreType(otOperationalDatasetComponents, MeshCoP::Dataset::Components);
DefineCoreType(otOperationalDataset, MeshCoP::Dataset::Info);

} // namespace ot

#endif // MESHCOP_DATASET_HPP_
