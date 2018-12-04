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
#include "meshcop/meshcop_tlvs.hpp"

namespace ot {
namespace MeshCoP {

class Dataset
{
    friend class DatasetLocal;

public:
    enum
    {
        kMaxSize      = 256, ///< Maximum size of MeshCoP Dataset (bytes)
        kMaxValueSize = 16,  /// < Maximum size of each Dataset TLV value (bytes)
    };

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aType       The type of the dataset, active or pending.
     *
     */
    Dataset(const Tlv::Type aType);

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
     * This method returns a pointer to the TLV.
     *
     * @returns A pointer to the TLV or NULL if none is found.
     *
     */
    Tlv *Get(Tlv::Type aType);

    /**
     * This method returns a pointer to the TLV.
     *
     * @returns A pointer to the TLV or NULL if none is found.
     *
     */
    const Tlv *Get(Tlv::Type aType) const;

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
     */
    void Get(otOperationalDataset &aDataset) const;

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
    uint32_t GetUpdateTime(void) const { return mUpdateTime; }

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
    otError Set(const Tlv &aTlv);

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
     * @retval OT_ERROR_NONE  Successfully set the Dataset.
     *
     */
    otError Set(const Dataset &aDataset);

    /**
     * This method sets the Dataset.
     *
     * @param[in]  aDataset  The input Dataset.
     *
     * @retval OT_ERROR_NONE  Successfully set the Dataset.
     *
     */
    otError Set(const otOperationalDataset &aDataset);

    /**
     * This method removes a TLV from the Dataset.
     *
     * @param[in] aType The type of a specific TLV.
     *
     */
    void Remove(Tlv::Type aType);

    /**
     * This method appends the MLE Dataset TLV but excluding MeshCoP Sub Timestamp TLV.
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
    otError ApplyConfiguration(Instance &aInstance, bool *aIsMasterKeyUpdated = NULL) const;

    /**
     * This method converts a Pending Dataset to an Active Dataset.
     *
     * This method removes the Delay Timer and Pending Timestamp TLVs
     *
     * @retval OT_ERROR_NONE  Successfully converted to Active Dataset.
     *
     */
    otError ConvertToActive(void);

private:
    void Remove(uint8_t *aStart, uint8_t aLength);

    uint8_t   mTlvs[kMaxSize]; ///< The Dataset buffer
    uint32_t  mUpdateTime;     ///< Local time last updated
    uint16_t  mLength;         ///< The number of valid bytes in @var mTlvs
    Tlv::Type mType;           ///< Active or Pending
};

} // namespace MeshCoP
} // namespace ot

#endif // MESHCOP_DATASET_HPP_
