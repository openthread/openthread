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

#include "common/locator.hpp"
#include "common/message.hpp"
#include "meshcop/meshcop_tlvs.hpp"

namespace ot {
namespace MeshCoP {

class Dataset: public InstanceLocator
{
public:
    enum
    {
        kMaxSize = 256,      ///< Maximum size of MeshCoP Dataset (bytes)
        kMaxValueSize = 16,  /// < Maximum size of each Dataset TLV value (bytes)
    };

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A pointer to an OpenThread instance.
     * @param[in]  aType       The type of the dataset, active or pending.
     *
     */
    Dataset(otInstance *aInstance, const Tlv::Type aType);

    /**
     * This method clears the Dataset.
     *
     * @param[in]  isLocal  TRUE to delete the local dataset from non-volatile memory, FALSE not.
     *
     */
    void Clear(bool isLocal);

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
     * This method compares this dataset to another based on the timestamp.
     *
     * @param[in]  aCompare  A reference to the timestamp to compare.
     *
     * @retval -1  if @p aCompare is older than this dataset.
     * @retval  0  if @p aCompare is equal to this dataset.
     * @retval  1  if @p aCompare is newer than this dataset.
     *
     */
    int Compare(const Dataset &aCompare) const;

    /**
     * This method restores dataset from non-volatile memory.
     *
     * @retval OT_ERROR_NONE       Successfully restore the dataset.
     * @retval OT_ERROR_NOT_FOUND  There is no corresponding dataset stored in non-volatile memory.
     *
     */
    otError Restore(void);

    /**
     * This method stores dataset into non-volatile memory.
     *
     * @retval OT_ERROR_NONE       Successfully store the dataset.
     * @retval OT_ERROR_NO_BUFS    Could not store the dataset due to insufficient memory space.
     *
     */
    otError Store(void);

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

    otError Set(const Message &aMessage, uint16_t aOffset, uint8_t aLength);

    otError Set(const Dataset &aDataset);

#if OPENTHREAD_FTD
    otError Set(const otOperationalDataset &aDataset);
#endif

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
    otError AppendMleDatasetTlv(Message &aMessage);

private:
    uint16_t GetSettingsKey(void);

    void Remove(uint8_t *aStart, uint8_t aLength);

    Tlv::Type  mType;            ///< Active or Pending
    uint8_t    mTlvs[kMaxSize];  ///< The Dataset buffer
    uint16_t   mLength;          ///< The number of valid bytes in @var mTlvs
};

}  // namespace MeshCoP
}  // namespace ot

#endif  // MESHCOP_DATASET_HPP_
