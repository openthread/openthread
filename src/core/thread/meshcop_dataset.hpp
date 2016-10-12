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

#include <common/message.hpp>
#include <thread/meshcop_tlvs.hpp>

namespace Thread {
namespace MeshCoP {

class Dataset
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
     * @param[in]  aType      The type of the dataset, active or pending.
     *
     */
    Dataset(const Tlv::Type aType);

    /**
     * This method clears the Dataset.
     *
     */
    void Clear(void);

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
    uint8_t GetSize(void) const { return mLength; }

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
     * This method sets a TLV in the Dataset.
     *
     * @param[in]  aTlv  A reference to the TLV.
     *
     * @retval kThreadError_None    Successfully set the TLV.
     * @retval kThreadError_NoBufs  Could not set the TLV due to insufficient buffer space.
     *
     */
    ThreadError Set(const Tlv &aTlv);

    ThreadError Set(const Message &aMessage, uint16_t aOffset, uint8_t aLength);

    ThreadError Set(const Dataset &aDataset);

    ThreadError Set(const otOperationalDataset &aDataset);

    void Remove(Tlv::Type aType);

private:
    void Remove(uint8_t *aStart, uint8_t aLength);

    Tlv::Type  mType;            ///< Active or Pending
    uint8_t    mTlvs[kMaxSize];  ///< The Dataset buffer
    uint8_t    mLength;          ///< The number of valid bytes in @var mTlvs
};

}  // namespace MeshCoP
}  // namespace Thread

#endif  // MESHCOP_DATASET_HPP_
