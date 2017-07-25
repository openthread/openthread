/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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

#ifndef MESHCOP_DATASET_LOCAL_HPP_
#define MESHCOP_DATASET_LOCAL_HPP_

#include "common/locator.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/meshcop_tlvs.hpp"

namespace ot {
namespace MeshCoP {

class DatasetLocal: public InstanceLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A reference to an OpenThread instance.
     * @param[in]  aType      The type of the dataset, active or pending.
     *
     */
    DatasetLocal(otInstance &aInstance, const Tlv::Type aType);

    /**
     * This method indicates whether this is an Active or Pending Dataset.
     *
     * @retval Tlv::kActiveTimestamp when this is an Active Dataset.
     * @retval Tlv::kPendingTimetamp when this is a Pending Dataset.
     *
     */
    Tlv::Type GetType(void) const { return mType; }

    /**
     * This method clears the Dataset.
     *
     */
    void Clear(void);

    /**
     * This method indicates whether or not the dataset is present in non-volatile memory.
     *
     * @retval TRUE   if the dataset is present in non-volatile memory.
     * @retval FALSE  if the dataset is not present in non-volatile memory.
     *
     */
    bool IsPresent(void) const;

    /**
     * This method returns a pointer to the Active or Pending Timestamp value.
     *
     * @returns  A pointer to the Active or Pending Timestamp value or NULL if the dataset is invalid.
     *
     */
    const Timestamp *GetTimestamp(void) const;

    /**
     * This method retrieves the dataset from non-volatile memory.
     *
     * @param[out]  aDataset  Where to place the dataset.
     *
     * @retval OT_ERROR_NONE       Successfully retrieved the dataset.
     * @retval OT_ERROR_NOT_FOUND  There is no corresponding dataset stored in non-volatile memory.
     *
     */
    otError Get(Dataset &aDataset);

    /**
     * This method retrieves the dataset from non-volatile memory.
     *
     * @param[out]  aDataset  Where to place the dataset.
     *
     * @retval OT_ERROR_NONE       Successfully retrieved the dataset.
     * @retval OT_ERROR_NOT_FOUND  There is no corresponding dataset stored in non-volatile memory.
     *
     */
    otError Get(otOperationalDataset &aDataset) const;

    /**
     * This method returns the local time this dataset was last updated or restored.
     *
     * @returns The local time this dataset was last updated or restored.
     *
     */
    uint32_t GetUpdateTime(void) const { return mUpdateTime; }

#if OPENTHREAD_FTD
    /**
     * This method stores the dataset into non-volatile memory.
     *
     * @retval OT_ERROR_NONE  Successfully stored the dataset.
     *
     */
    otError Set(const otOperationalDataset &aDataset);
#endif

    /**
     * This method stores the dataset into non-volatile memory.
     *
     * @retval OT_ERROR_NONE  Successfully stored the dataset.
     *
     */
    otError Set(const Dataset &aDataset);

    /**
     * This method restores dataset from non-volatile memory.
     *
     * @retval OT_ERROR_NONE       Successfully restore the dataset.
     * @retval OT_ERROR_NOT_FOUND  There is no corresponding dataset stored in non-volatile memory.
     *
     */
    otError Restore(void);

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
    int Compare(const Timestamp *aCompare);

private:
    uint16_t GetSettingsKey(void) const;
    void SetTimestamp(const Dataset &aDataset);

    uint32_t    mUpdateTime;      ///< Local time last updated
    Tlv::Type   mType;            ///< Active or Pending
};

}  // namespace MeshCoP
}  // namespace ot

#endif  // MESHCOP_DATASET_LOCAL_HPP_
