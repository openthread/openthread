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

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "common/type_traits.hpp"
#include "meshcop/meshcop_tlvs.hpp"

namespace ot {
namespace MeshCoP {

/**
 * This class represents MeshCop Dataset.
 *
 */
class Dataset
{
    friend class DatasetLocal;

public:
    enum
    {
        kMaxSize      = OT_OPERATIONAL_DATASET_MAX_LENGTH, ///< Maximum size of MeshCoP Dataset (bytes)
        kMaxValueSize = 16,                                ///< Maximum size of each Dataset TLV value (bytes)
        kMaxGetTypes  = 64,                                ///< Maximum number of types in MGMT_GET.req
    };

    /**
     * This enumeration represents the Dataset type (active or pending).
     *
     */
    enum Type
    {
        kActive,  ///< Active Dataset
        kPending, ///< Pending Dataset
    };

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aType       The type of the dataset, active or pending.
     *
     */
    explicit Dataset(Type aType);

    /**
     * This method clears the Dataset.
     *
     */
    void Clear(void);

    /**
     * This method indicates whether or not the dataset appears to be well-formed.
     *
     * @returns TRUE if the dataset appears to be well-formed, FALSE otherwise.
     *
     */
    bool IsValid(void) const;

    /**
     * This method returns a pointer to the TLV with a given type.
     *
     * @param[in] aType  A TLV type.
     *
     * @returns A pointer to the TLV or nullptr if none is found.
     *
     */
    Tlv *GetTlv(Tlv::Type aType) { return const_cast<Tlv *>(const_cast<const Dataset *>(this)->GetTlv(aType)); }

    /**
     * This method returns a pointer to the TLV with a given type.
     *
     * @param[in] aType  The TLV type.
     *
     * @returns A pointer to the TLV or nullptr if none is found.
     *
     */
    const Tlv *GetTlv(Tlv::Type aType) const;

    /**
     * This template method returns a pointer to the TLV with a given template type `TlvType`
     *
     * @returns A pointer to the TLV or nullptr if none is found.
     *
     */
    template <typename TlvType> TlvType *GetTlv(void)
    {
        return static_cast<TlvType *>(GetTlv(static_cast<Tlv::Type>(TlvType::kType)));
    }

    /**
     * This template method returns a pointer to the TLV with a given template type `TlvType`
     *
     * @returns A pointer to the TLV or nullptr if none is found.
     *
     */
    template <typename TlvType> const TlvType *GetTlv(void) const
    {
        return static_cast<const TlvType *>(GetTlv(static_cast<Tlv::Type>(TlvType::kType)));
    }

    /**
     * This method returns a pointer to the byte representation of the Dataset.
     *
     * @returns A pointer to the byte representation of the Dataset.
     *
     */
    uint8_t *GetBytes(void) { return mTlvs; }

    /**
     * This method returns a pointer to the byte representation of the Dataset.
     *
     * @returns A pointer to the byte representation of the Dataset.
     *
     */
    const uint8_t *GetBytes(void) const { return mTlvs; }

    /**
     * This method converts the TLV representation to structure representation.
     *
     * @param[out] aDataset  A reference to `otOperationalDataset` to output the Dataset.
     *
     */
    void ConvertTo(otOperationalDataset &aDataset) const;

    /**
     * This method converts the TLV representation to structure representation.
     *
     * @param[out] aDataset  A reference to `otOperationalDatasetTlvs` to output the Dataset.
     *
     */
    void ConvertTo(otOperationalDatasetTlvs &aDataset) const;

    /**
     * This method returns the Dataset size in bytes.
     *
     * @returns The Dataset size in bytes.
     *
     */
    uint16_t GetSize(void) const { return mLength; }

    /**
     * This method sets the Dataset size in bytes.
     *
     * @param[in] aSize  The Dataset size in bytes.
     *
     */
    void SetSize(uint16_t aSize) { mLength = aSize; }

    /**
     * This method returns the local time the dataset was last updated.
     *
     * @returns The local time the dataset was last updated.
     *
     */
    TimeMilli GetUpdateTime(void) const { return mUpdateTime; }

    /**
     * This method returns a reference to the Timestamp.
     *
     * @returns A pointer to the Timestamp.
     *
     */
    const Timestamp *GetTimestamp(void) const;

    /**
     * This method sets the Timestamp value.
     *
     * @param[in] aTimestamp   A Timestamp.
     *
     */
    void SetTimestamp(const Timestamp &aTimestamp);

    /**
     * This method sets a TLV in the Dataset.
     *
     * @param[in]  aTlv  A reference to the TLV.
     *
     * @retval OT_ERROR_NONE     Successfully set the TLV.
     * @retval OT_ERROR_NO_BUFS  Could not set the TLV due to insufficient buffer space.
     *
     */
    otError SetTlv(const Tlv &aTlv);

    /**
     * This method sets a TLV with a given TLV Type and Value.
     *
     * @param[in] aType     The TLV Type.
     * @param[in] aValue    A pointer to TLV Value.
     * @param[in] aLength   The TLV Length in bytes (length of @p aValue).
     *
     * @retval OT_ERROR_NONE     Successfully set the TLV.
     * @retval OT_ERROR_NO_BUFS  Could not set the TLV due to insufficient buffer space.
     *
     */
    otError SetTlv(Tlv::Type aType, const void *aValue, uint8_t aLength);

    /**
     * This template method sets a TLV with a given TLV Type and Value.
     *
     * @tparam ValueType    The type of TLV's Value.
     *
     * @param[in] aType     The TLV Type.
     * @param[in] aValue    The TLV Value (of type `ValueType`).
     *
     * @retval OT_ERROR_NONE     Successfully set the TLV.
     * @retval OT_ERROR_NO_BUFS  Could not set the TLV due to insufficient buffer space.
     *
     */
    template <typename ValueType> otError SetTlv(Tlv::Type aType, const ValueType &aValue)
    {
        static_assert(!TypeTraits::IsPointer<ValueType>::kValue, "ValueType must not be a pointer");

        return SetTlv(aType, &aValue, sizeof(ValueType));
    }

    /**
     * This method sets the Dataset using TLVs stored in a message buffer.
     *
     * @param[in]  aMessage  The message buffer.
     * @param[in]  aOffset   The message buffer offset where the dataset starts.
     * @param[in]  aLength   The TLVs length in the message buffer in bytes.
     *
     * @retval OT_ERROR_NONE          Successfully set the Dataset.
     * @retval OT_ERROR_INVALID_ARGS  The values of @p aOffset and @p aLength are not valid for @p aMessage.
     *
     */
    otError Set(const Message &aMessage, uint16_t aOffset, uint8_t aLength);

    /**
     * This method sets the Dataset using an existing Dataset.
     *
     * If this Dataset is an Active Dataset, any Pending Timestamp and Delay Timer TLVs will be omitted in the copy
     * from @p aDataset.
     *
     * @param[in]  aDataset  The input Dataset.
     *
     */
    void Set(const Dataset &aDataset);

    /**
     * This method sets the Dataset from a given structure representation.
     *
     * @param[in]  aDataset  The input Dataset as otOperationalDataset.
     *
     * @retval OT_ERROR_NONE          Successfully set the Dataset.
     * @retval OT_ERROR_INVALID_ARGS  Dataset is missing Active and/or Pending Timestamp.
     *
     */
    otError SetFrom(const otOperationalDataset &aDataset);

    /**
     * This method sets the Dataset using @p aDataset.
     *
     * @param[in]  aDataset  The input Dataset as otOperationalDatasetTlvs.
     *
     */
    void SetFrom(const otOperationalDatasetTlvs &aDataset);

    /**
     * This method removes a TLV from the Dataset.
     *
     * @param[in] aType The type of a specific TLV.
     *
     */
    void RemoveTlv(Tlv::Type aType);

    /**
     * This method appends the MLE Dataset TLV but excluding MeshCoP Sub Timestamp TLV.
     *
     * @param[in] aMessage       A message to append to.
     *
     * @retval OT_ERROR_NONE     Successfully append MLE Dataset TLV without MeshCoP Sub Timestamp TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to append the message with MLE Dataset TLV.
     *
     */
    otError AppendMleDatasetTlv(Message &aMessage) const;

    /**
     * This method applies the Active or Pending Dataset to the Thread interface.
     *
     * @param[in]  aInstance           A reference to the OpenThread instance.
     * @param[out] aIsMasterKeyUpdated A pointer to where to place whether master key was updated.
     *
     * @retval OT_ERROR_NONE   Successfully applied configuration.
     * @retval OT_ERROR_PARSE  The dataset has at least one TLV with invalid format.
     *
     */
    otError ApplyConfiguration(Instance &aInstance, bool *aIsMasterKeyUpdated = nullptr) const;

    /**
     * This method converts a Pending Dataset to an Active Dataset.
     *
     * This method removes the Delay Timer and Pending Timestamp TLVs
     *
     */
    void ConvertToActive(void);

    /**
     * This method returns a pointer to the start of Dataset TLVs sequence.
     *
     * @return  A pointer to the start of Dataset TLVs sequence.
     *
     */
    Tlv *GetTlvsStart(void) { return reinterpret_cast<Tlv *>(mTlvs); }

    /**
     * This method returns a pointer to the start of Dataset TLVs sequence.
     *
     * @return  A pointer to start of Dataset TLVs sequence.
     *
     */
    const Tlv *GetTlvsStart(void) const { return reinterpret_cast<const Tlv *>(mTlvs); }

    /**
     * This method returns a pointer to the past-the-end of Dataset TLVs sequence.
     *
     * Note that past-the-end points to the byte after the end of the last TLV in Dataset TLVs sequence.
     *
     * @return  A pointer to past-the-end of Dataset TLVs sequence.
     *
     */
    Tlv *GetTlvsEnd(void) { return reinterpret_cast<Tlv *>(mTlvs + mLength); }

    /**
     * This method returns a pointer to the past-the-end of Dataset TLVs sequence.
     *
     * Note that past-the-end points to the byte after the end of the last TLV in Dataset TLVs sequence.
     *
     * @return  A pointer to past-the-end of Dataset TLVs sequence.
     *
     */
    const Tlv *GetTlvsEnd(void) const { return reinterpret_cast<const Tlv *>(mTlvs + mLength); }

    /**
     * This static method converts a Dataset Type to a string.
     *
     * @param[in]  aType   A Dataset type.
     *
     */
    static const char *TypeToString(Type aType);

private:
    void RemoveTlv(Tlv *aTlv);

    uint8_t   mTlvs[kMaxSize]; ///< The Dataset buffer
    TimeMilli mUpdateTime;     ///< Local time last updated
    uint16_t  mLength;         ///< The number of valid bytes in @var mTlvs
    Type      mType;           ///< Active or Pending
};

/**
 * This is a template specialization of `SetTlv<ValueType>` with a `uint16_t` value type.
 *
 * @param[in] aType     The TLV Type.
 * @param[in] aValue    The TLV value (as `uint16_t`).
 *
 * @retval OT_ERROR_NONE     Successfully set the TLV.
 * @retval OT_ERROR_NO_BUFS  Could not set the TLV due to insufficient buffer space.
 *
 */
template <> inline otError Dataset::SetTlv(Tlv::Type aType, const uint16_t &aValue)
{
    uint16_t value = Encoding::BigEndian::HostSwap16(aValue);

    return SetTlv(aType, &value, sizeof(uint16_t));
}

/**
 * This is a template specialization of `SetTlv<ValueType>` with a `uint32_t` value type
 *
 * @param[in] aType     The TLV Type.
 * @param[in] aValue    The TLV value (as `uint32_t`).
 *
 * @retval OT_ERROR_NONE     Successfully set the TLV.
 * @retval OT_ERROR_NO_BUFS  Could not set the TLV due to insufficient buffer space.
 *
 */
template <> inline otError Dataset::SetTlv(Tlv::Type aType, const uint32_t &aValue)
{
    uint32_t value = Encoding::BigEndian::HostSwap32(aValue);

    return SetTlv(aType, &value, sizeof(uint32_t));
}

} // namespace MeshCoP
} // namespace ot

#endif // MESHCOP_DATASET_HPP_
