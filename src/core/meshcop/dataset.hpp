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
 */
class Dataset
{
    friend class DatasetManager;

public:
    static constexpr uint8_t kMaxLength = OT_OPERATIONAL_DATASET_MAX_LENGTH; ///< Max length of Dataset (bytes)

    /**
     * Represents the Dataset type (active or pending).
     */
    enum Type : uint8_t
    {
        kActive,  ///< Active Dataset
        kPending, ///< Pending Dataset
    };

    /**
     * Represents a Dataset as a sequence of TLVs.
     */
    typedef otOperationalDatasetTlvs Tlvs;

    /**
     * Represents a component in Dataset.
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
        kWakeupChannel,    ///< Wakeup Channel
        kPskc,             ///< PSKc
        kSecurityPolicy,   ///< Security Policy
        kChannelMask,      ///< Channel Mask
    };

    template <Component kComponent> struct TypeFor; ///< Specifies the associate type for a given `Component`.

    class Info;

    /**
     * Represents presence of different components in Active or Pending Operational Dataset.
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
         */
        template <Component kComponent> bool IsPresent(void) const;

    private:
        template <Component kComponent> void MarkAsPresent(void);
    };

    /**
     * Represents the information about the fields contained an Active or Pending Operational Dataset.
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
         */
        template <Component kComponent> void Get(typename TypeFor<kComponent>::Type &aComponent) const;

        /**
         * Sets the specified component in the Dataset.
         *
         * @tparam  kComponent  The component to set.
         *
         * @param[in] aComponent   The component value.
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
         */
        Error GenerateRandom(Instance &aInstance);

    private:
        Components       &GetComponents(void) { return static_cast<Components &>(mComponents); }
        const Components &GetComponents(void) const { return static_cast<const Components &>(mComponents); }
    };

    /**
     * Initializes the object.
     */
    Dataset(void);

    /**
     * Clears the Dataset.
     */
    void Clear(void) { mLength = 0; }

    /**
     * Parses and validates all TLVs contained within the Dataset.
     *
     * Performs the following checks all TLVs in the Dataset:
     *  - Ensures correct TLV format and expected minimum length for known TLV types that may appear in a Dataset.
     *  - Validates TLV value when applicable (e.g., Channel TLV using a supported channel).
     *  - Ensures no duplicate occurrence of same TLV type.
     *
     * @retval kErrorNone   Successfully validated all the TLVs in the Dataset.
     * @retval kErrorParse  Dataset TLVs is not well-formed.
     */
    Error ValidateTlvs(void) const;

    /**
     * Validates the format and value of a given MeshCoP TLV used in Dataset.
     *
     * TLV types that can appear in an Active or Pending Operational Dataset are validated. Other TLV types including
     * unknown TLV types are considered as valid.
     *
     * @param[in]  aTlv    The TLV to validate.
     *
     * @retval  TRUE       The TLV format and value is valid, or TLV type is unknown (not supported in Dataset).
     * @retval  FALSE      The TLV format or value is invalid.
     */
    static bool IsTlvValid(const Tlv &aTlv);

    /**
     * Indicates whether or not a given TLV type is present in the Dataset.
     *
     * @param[in] aType  The TLV type to check.
     *
     * @retval TRUE    TLV with @p aType is present in the Dataset.
     * @retval FALSE   TLV with @p aType is not present in the Dataset.
     */
    bool ContainsTlv(Tlv::Type aType) const { return (FindTlv(aType) != nullptr); }

    /**
     * Indicates whether or not a given TLV type is present in the Dataset.
     *
     * @tparam  aTlvType  The TLV type to check.
     *
     * @retval TRUE    TLV of @p aTlvType is present in the Dataset.
     * @retval FALSE   TLV of @p aTlvType is not present in the Dataset.
     */
    template <typename TlvType> bool Contains(void) const
    {
        return ContainsTlv(static_cast<Tlv::Type>(TlvType::kType));
    }

    /**
     * Indicates whether or not the Dataset contains all the TLVs from a given array.
     *
     * @param[in] aTlvTypes    An array of TLV types.
     * @param[in] aLength      Length of @p aTlvTypes array.
     *
     * @retval TRUE    The Dataset contains all the TLVs in @p aTlvTypes array.
     * @retval FALSE   The Dataset does not contain all the TLVs in @p aTlvTypes array.
     */
    bool ContainsAllTlvs(const Tlv::Type aTlvTypes[], uint8_t aLength) const;

    /**
     * Indicates whether or not the Dataset contains all the required TLVs for an Active or Pending Dataset.
     *
     * @param[in] aType  The Dataset type, Active or Pending.
     *
     * @retval TRUE    The Dataset contains all the required TLVs for @p aType.
     * @retval FALSE   The Dataset does not contain all the required TLVs for @p aType.
     */
    bool ContainsAllRequiredTlvsFor(Type aType) const;

    /**
     * Searches for a given TLV type in the Dataset.
     *
     * @param[in] aType  The TLV type to find.
     *
     * @returns A pointer to the TLV or `nullptr` if not found.
     */
    Tlv *FindTlv(Tlv::Type aType) { return AsNonConst(AsConst(this)->FindTlv(aType)); }

    /**
     * Searches for a given TLV type in the Dataset.
     *
     * @param[in] aType  The TLV type to find.
     *
     * @returns A pointer to the TLV or `nullptr` if not found.
     */
    const Tlv *FindTlv(Tlv::Type aType) const;

    /**
     * Finds and reads a simple TLV in the Dataset.
     *
     * If the specified TLV type is not found, `kErrorNotFound` is reported.
     *
     * @tparam  SimpleTlvType   The simple TLV type (must be a sub-class of `SimpleTlvInfo`).
     *
     * @param[out] aValue       A reference to return the read TLV value.
     *
     * @retval kErrorNone      Successfully found and read the TLV value. @p aValue is updated.
     * @retval kErrorNotFound  Could not find the TLV in the Dataset.
     */
    template <typename SimpleTlvType> Error Read(typename SimpleTlvType::ValueType &aValue) const
    {
        const Tlv *tlv = FindTlv(static_cast<Tlv::Type>(SimpleTlvType::kType));

        return (tlv == nullptr) ? kErrorNotFound : (aValue = tlv->ReadValueAs<SimpleTlvType>(), kErrorNone);
    }

    /**
     * Finds and reads an `uint` TLV in the Dataset.
     *
     * If the specified TLV type is not found, `kErrorNotFound` is reported.
     *
     * @tparam  UintTlvType     The integer simple TLV type (must be a sub-class of `UintTlvInfo`).
     *
     * @param[out] aValue       A reference to return the read TLV value.
     *
     * @retval kErrorNone      Successfully found and read the TLV value. @p aValue is updated.
     * @retval kErrorNotFound  Could not find the TLV in the Dataset.
     */
    template <typename UintTlvType> Error Read(typename UintTlvType::UintValueType &aValue) const
    {
        const Tlv *tlv = FindTlv(static_cast<Tlv::Type>(UintTlvType::kType));

        return (tlv == nullptr) ? kErrorNotFound : (aValue = tlv->ReadValueAs<UintTlvType>(), kErrorNone);
    }

    /**
     * Writes a TLV to the Dataset.
     *
     * If the specified TLV type already exists, it will be replaced. Otherwise, the TLV will be appended.
     *
     * @param[in] aTlv     A reference to the TLV.
     *
     * @retval kErrorNone    Successfully updated the TLV.
     * @retval kErrorNoBufs  Could not add the TLV due to insufficient buffer space.
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
     */
    template <typename UintTlvType> Error Write(typename UintTlvType::UintValueType aValue)
    {
        typename UintTlvType::UintValueType value = BigEndian::HostSwap(aValue);

        return WriteTlv(static_cast<Tlv::Type>(UintTlvType::kType), &value, sizeof(value));
    }

    /**
     * Writes TLVs parsed from a given Dataset into this Dataset.
     *
     * TLVs from @p aDataset are parsed and written in the current Dataset. If the same TLV already exists, it will be
     * replaced. Otherwise, the TLV will be appended.
     *
     * @param[in] aDataset   A Dataset.
     *
     * @retval kErrorNone    Successfully merged TLVs from @p Dataset into this Dataset.
     * @retval kErrorParse   The @p aDataset is not valid.
     * @retval kErrorNoBufs  Could not add the TLVs due to insufficient buffer space.
     */
    Error WriteTlvsFrom(const Dataset &aDataset);

    /**
     * Writes TLVs parsed from a given buffer containing a sequence of TLVs into this Dataset.
     *
     * TLVs from @p aTlvs buffer are parsed and written in the current Dataset. If the same TLV already exists, it will
     * be replaced. Otherwise, the TLV will be appended.
     *
     * @param[in] aTlvs     A pointer to a buffer containing TLVs.
     * @param[in] aLength   Number of bytes in @p aTlvs buffer.
     *
     * @retval kErrorNone    Successfully merged TLVs from @p Dataset into this Dataset.
     * @retval kErrorParse   The @p aTlvs is not valid.
     * @retval kErrorNoBufs  Could not add the TLVs due to insufficient buffer space.
     */
    Error WriteTlvsFrom(const uint8_t *aTlvs, uint8_t aLength);

    /**
     * Writes TLVs corresponding to the components in a given `Dataset::Info` into this Dataset.
     *
     * If the same TLV already exists, it will be replaced. Otherwise the TLV will be appended.
     *
     * @param[in] aDataseInfo     A `Dataset::Info`.
     *
     * @retval kErrorNone    Successfully merged TLVs from @p aDataseInfo into this Dataset.
     * @retval kErrorNoBufs  Could not add the TLVs due to insufficient buffer space.
     */
    Error WriteTlvsFrom(const Dataset::Info &aDatasetInfo);

    /**
     * Appends a given sequence of TLVs to the Dataset.
     *
     * @note Unlike `WriteTlvsFrom()`, this method does not validate the @p aTlvs to be well-formed or check that there
     * are no duplicates. It is up to caller to validate the resulting `Dataset` (e.g., using `ValidateTlvs()`) if
     * desired.
     *
     * @param[in] aTlvs     A pointer to a buffer containing TLVs.
     * @param[in] aLength   Number of bytes in @p aTlvs buffer.
     *
     * @retval kErrorNone    Successfully merged TLVs from @p Dataset into this Dataset.
     * @retval kErrorNoBufs  Could not append the TLVs due to insufficient buffer space.
     */
    Error AppendTlvsFrom(const uint8_t *aTlvs, uint8_t aLength);

    /**
     * Removes a TLV from the Dataset.
     *
     * If the Dataset does not contain the given TLV type, no action is performed.
     *
     * @param[in] aType  The TLV type to remove.
     */
    void RemoveTlv(Tlv::Type aType);

    /**
     * Reads the Timestamp TLV (Active or Pending).
     *
     * @param[in]  aType       The timestamp type, active or pending.
     * @param[out] aTimestamp  A reference to a `Timestamp` to output the value.
     *
     * @retval kErrorNone      Timestamp was read successfully. @p aTimestamp is updated.
     * @retval kErrorNotFound  Could not find the requested Timestamp TLV.
     */
    Error ReadTimestamp(Type aType, Timestamp &aTimestamp) const;

    /**
     * Writes the Timestamp TLV (Active or Pending).
     *
     * If the TLV already exists, it will be replaced. Otherwise, the TLV will be appended.
     *
     * @param[in] aType       The timestamp type, active or pending.
     * @param[in] aTimestamp  The timestamp value.
     *
     * @retval kErrorNone    Successfully updated the Timestamp TLV.
     * @retval kErrorNoBufs  Could not append the Timestamp TLV due to insufficient buffer space.
     */
    Error WriteTimestamp(Type aType, const Timestamp &aTimestamp);

    /**
     * Removes the Timestamp TLV (Active or Pending) from the Dataset.
     *
     * @param[in] aType       The timestamp type, active or pending.
     */
    void RemoveTimestamp(Type aType);

    /**
     * Returns a pointer to the byte representation of the Dataset.
     *
     * @returns A pointer to the byte representation of the Dataset.
     */
    uint8_t *GetBytes(void) { return mTlvs; }

    /**
     * Returns a pointer to the byte representation of the Dataset.
     *
     * @returns A pointer to the byte representation of the Dataset.
     */
    const uint8_t *GetBytes(void) const { return mTlvs; }

    /**
     * Converts the TLV representation to structure representation.
     *
     * @param[out] aDatasetInfo  A reference to `Info` object to output the Dataset.
     */
    void ConvertTo(Info &aDatasetInfo) const;

    /**
     * Converts the TLV representation to structure representation.
     *
     * @param[out] aTlvs  A reference to output the Dataset as a sequence of TLVs.
     */
    void ConvertTo(Tlvs &aTlvs) const;

    /**
     * Returns the Dataset length in bytes.
     *
     * @returns The Dataset length in bytes.
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * Sets the Dataset size in bytes.
     *
     * @param[in] aSize  The Dataset size in bytes.
     */
    void SetLength(uint8_t aLength) { mLength = aLength; }

    /**
     * Returns the local time the dataset was last updated.
     *
     * @returns The local time the dataset was last updated.
     */
    TimeMilli GetUpdateTime(void) const { return mUpdateTime; }

    /**
     * Sets this Dataset using an existing Dataset.
     *
     * @param[in]  aDataset  The input Dataset.
     */
    void SetFrom(const Dataset &aDataset);

    /**
     * Sets the Dataset from a given structure representation.
     *
     * @param[in]  aDatasetInfo  The input Dataset as `Dataset::Info`.
     */
    void SetFrom(const Info &aDatasetInfo);

    /**
     * Sets the Dataset from a given sequence of TLVs.
     *
     * @param[in]  aTlvs          The input Dataset as `Tlvs`.
     *
     * @retval kErrorNone         Successfully set the Dataset.
     * @retval kErrorInvalidArgs  The @p aTlvs is invalid and its length is longer than `kMaxLength`.
     */
    Error SetFrom(const Tlvs &aTlvs);

    /**
     * Sets the Dataset from a buffer containing a sequence of TLVs.
     *
     * @param[in] aTlvs     A pointer to a buffer containing TLVs.
     * @param[in] aLength   Number of bytes in @p aTlvs buffer.
     *
     * @retval kErrorNone         Successfully set the Dataset.
     * @retval kErrorInvalidArgs  @p aLength is longer than `kMaxLength`.
     */
    Error SetFrom(const uint8_t *aTlvs, uint8_t aLength);

    /**
     * Sets the Dataset by reading the TLVs bytes from given message.
     *
     * @param[in] aMessage       The message to read from.
     * @param[in] aOffsetRange   The offset range in @p aMessage to read the Dataset TLVs.
     *
     * @retval kErrorNone    Successfully set the Dataset.
     * @retval kInvalidArgs  The given offset range length is longer than `kMaxLength`.
     * @retval kErrorParse   Could not read or parse the dataset from @p aMessage.
     */
    Error SetFrom(const Message &aMessage, const OffsetRange &aOffsetRange);

    /**
     * Returns a pointer to the start of Dataset TLVs sequence.
     *
     * @return  A pointer to the start of Dataset TLVs sequence.
     */
    Tlv *GetTlvsStart(void) { return reinterpret_cast<Tlv *>(mTlvs); }

    /**
     * Returns a pointer to the start of Dataset TLVs sequence.
     *
     * @return  A pointer to start of Dataset TLVs sequence.
     */
    const Tlv *GetTlvsStart(void) const { return reinterpret_cast<const Tlv *>(mTlvs); }

    /**
     * Returns a pointer to the past-the-end of Dataset TLVs sequence.
     *
     * Note that past-the-end points to the byte after the end of the last TLV in Dataset TLVs sequence.
     *
     * @return  A pointer to past-the-end of Dataset TLVs sequence.
     */
    Tlv *GetTlvsEnd(void) { return reinterpret_cast<Tlv *>(mTlvs + mLength); }

    /**
     * Returns a pointer to the past-the-end of Dataset TLVs sequence.
     *
     * Note that past-the-end points to the byte after the end of the last TLV in Dataset TLVs sequence.
     *
     * @return  A pointer to past-the-end of Dataset TLVs sequence.
     */
    const Tlv *GetTlvsEnd(void) const { return reinterpret_cast<const Tlv *>(mTlvs + mLength); }

    /**
     * Determines whether this Dataset is a subset of another Dataset.
     *
     * The Dataset is considered a subset if all of its TLVs, excluding Active/Pending Timestamp and Delay Timer TLVs,
     * are present in the @p aOther Dataset and the TLV values match exactly.
     *
     * @param[in] aOther   The other Dataset to check against.
     *
     * @retval TRUE   The current Dataset is a subset of @p aOther.
     * @retval FALSE  The current Dataset is not a subset of @p aOther.
     */
    bool IsSubsetOf(const Dataset &aOther) const;

    /**
     * Converts a Dataset Type to a string.
     *
     * @param[in]  aType   A Dataset type.
     */
    static const char *TypeToString(Type aType);

private:
    void RemoveTlv(Tlv *aTlv);

    static Tlv::Type TimestampTlvFor(Type aType)
    {
        return (aType == kActive) ? Tlv::kActiveTimestamp : Tlv::kPendingTimestamp;
    }

    uint8_t   mTlvs[kMaxLength];
    uint8_t   mLength;
    TimeMilli mUpdateTime; // Local time last updated
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
DefineIsPresentAndMarkAsPresent(WakeupChannel)
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
template <> struct Dataset::TypeFor<Dataset::kWakeupChannel>    { using Type = uint16_t; };
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

template <> inline const uint16_t &Dataset::Info::Get<Dataset::kWakeupChannel>(void) const { return mWakeupChannel; }

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
    aTimestamp.SetFrom(mActiveTimestamp);
}

template <> inline void Dataset::Info::Get<Dataset::kPendingTimestamp>(Timestamp &aTimestamp) const
{
    aTimestamp.SetFrom(mPendingTimestamp);
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
