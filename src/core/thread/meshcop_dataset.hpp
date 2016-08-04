/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
        kMaxSize = 256,  ///< Maximum size of MeshCoP Dataset (bytes)
    };

    /**
     * This constructor initializes the object.
     *
     */
    Dataset(void);

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
    void Get(otOperationalDataset &aDataset);

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
     * @returns A reference to the Timestamp.
     *
     */
    const Timestamp &GetTimestamp(void) const;

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
     * @retval kThreadError_None    Successfully set the TLV.
     * @retval kThreadError_NoBufs  Could not set the TLV due to insufficient buffer space.
     *
     */
    ThreadError Set(const Tlv &aTlv);

    ThreadError Set(const Message &aMessage, uint16_t aOffset, uint16_t aLength);

    ThreadError Set(const otOperationalDataset &aDataset, bool aActive);

    void Remove(Tlv::Type aType);

private:
    void Remove(uint8_t *aStart, uint8_t aLength);

    Timestamp mTimestamp;       ///< Active or Pending Timestamp
    uint8_t   mTlvs[kMaxSize];  ///< The Dataset buffer
    uint16_t  mLength;          ///< The number of valid bytes in @var mTlvs
};

}  // namespace MeshCoP
}  // namespace Thread

#endif  // MESHCOP_DATASET_HPP_
