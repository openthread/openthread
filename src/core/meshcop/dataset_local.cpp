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
 *   This file implements common methods for manipulating MeshCoP Datasets.
 *
 */

#define WPP_NAME "dataset_local.tmh"

#include "dataset_local.hpp"

#include <stdio.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/settings.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/mle_tlvs.hpp"

namespace ot {
namespace MeshCoP {

DatasetLocal::DatasetLocal(Instance &aInstance, const Tlv::Type aType)
    : InstanceLocator(aInstance)
    , mUpdateTime(0)
    , mType(aType)
    , mTimestampPresent(false)
{
    mTimestamp.Init();
}

void DatasetLocal::Clear(void)
{
    mTimestamp.Init();
    mTimestampPresent = false;
    GetInstance().GetSettings().DeleteOperationalDataset(IsActive());
}

otError DatasetLocal::Restore(Dataset &aDataset)
{
    const Timestamp *timestamp;
    otError          error;

    error = Get(aDataset);
    SuccessOrExit(error);

    timestamp = aDataset.GetTimestamp();

    if (timestamp != NULL)
    {
        mTimestamp        = *timestamp;
        mTimestampPresent = true;
    }
    else
    {
        mTimestampPresent = false;
    }

exit:
    return error;
}

otError DatasetLocal::Get(Dataset &aDataset) const
{
    DelayTimerTlv *delayTimer;
    uint32_t       elapsed;
    otError        error;

    error = GetInstance().GetSettings().ReadOperationalDataset(IsActive(), aDataset);
    VerifyOrExit(error == OT_ERROR_NONE, aDataset.mLength = 0);

    if (mType == Tlv::kActiveTimestamp)
    {
        aDataset.Remove(Tlv::kPendingTimestamp);
        aDataset.Remove(Tlv::kDelayTimer);
    }
    else
    {
        delayTimer = static_cast<DelayTimerTlv *>(aDataset.Get(Tlv::kDelayTimer));
        VerifyOrExit(delayTimer);

        elapsed = TimerMilli::GetNow() - mUpdateTime;

        if (delayTimer->GetDelayTimer() > elapsed)
        {
            delayTimer->SetDelayTimer(delayTimer->GetDelayTimer() - elapsed);
        }
        else
        {
            delayTimer->SetDelayTimer(0);
        }
    }

    aDataset.mUpdateTime = TimerMilli::GetNow();

exit:
    return error;
}

otError DatasetLocal::Get(otOperationalDataset &aDataset) const
{
    Dataset dataset(mType);
    otError error;

    memset(&aDataset, 0, sizeof(aDataset));

    error = Get(dataset);
    SuccessOrExit(error);

    dataset.Get(aDataset);

exit:
    return error;
}

otError DatasetLocal::Set(const otOperationalDataset &aDataset)
{
    otError error = OT_ERROR_NONE;
    Dataset dataset(mType);

    error = dataset.Set(aDataset);
    SuccessOrExit(error);

    error = Set(dataset);
    SuccessOrExit(error);

exit:
    return error;
}

otError DatasetLocal::Set(const Dataset &aDataset)
{
    const Timestamp *timestamp;
    otError          error;

    if (aDataset.GetSize() == 0)
    {
        error = GetInstance().GetSettings().DeleteOperationalDataset(IsActive());
        otLogInfoMeshCoP("%s dataset deleted", mType == Tlv::kActiveTimestamp ? "Active" : "Pending");
    }
    else
    {
        error = GetInstance().GetSettings().SaveOperationalDataset(IsActive(), aDataset);
        otLogInfoMeshCoP("%s dataset set", mType == Tlv::kActiveTimestamp ? "Active" : "Pending");
    }

    SuccessOrExit(error);

    timestamp = aDataset.GetTimestamp();

    if (timestamp != NULL)
    {
        mTimestamp        = *timestamp;
        mTimestampPresent = true;
    }
    else
    {
        mTimestampPresent = false;
    }

    mUpdateTime = TimerMilli::GetNow();

exit:
    return error;
}

int DatasetLocal::Compare(const Timestamp *aCompareTimestamp)
{
    int rval = 1;

    if (aCompareTimestamp == NULL)
    {
        if (!mTimestampPresent)
        {
            rval = 0;
        }
        else
        {
            rval = -1;
        }
    }
    else
    {
        if (!mTimestampPresent)
        {
            rval = 1;
        }
        else
        {
            rval = mTimestamp.Compare(*aCompareTimestamp);
        }
    }

    return rval;
}

} // namespace MeshCoP
} // namespace ot
