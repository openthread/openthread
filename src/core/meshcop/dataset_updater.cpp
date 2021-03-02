/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements Dataset Updater.
 *
 */

#include "dataset_updater.hpp"

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"

#if (OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE || OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE) && OPENTHREAD_FTD

namespace ot {
namespace MeshCoP {

DatasetUpdater::DatasetUpdater(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mCallback(nullptr)
    , mCallbackContext(nullptr)
    , mTimer(aInstance, DatasetUpdater::HandleTimer)
    , mDataset(nullptr)
{
}

otError DatasetUpdater::RequestUpdate(const MeshCoP::Dataset::Info &aDataset, Callback aCallback, void *aContext)
{
    otError  error   = OT_ERROR_NONE;
    Message *message = nullptr;

    VerifyOrExit(!Get<Mle::Mle>().IsDisabled(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mDataset == nullptr, error = OT_ERROR_BUSY);

    VerifyOrExit(!aDataset.IsActiveTimestampPresent() && !aDataset.IsPendingTimestampPresent(),
                 error = OT_ERROR_INVALID_ARGS);

    message = Get<MessagePool>().New(Message::kTypeOther, 0);
    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Append(aDataset));

    mCallback        = aCallback;
    mCallbackContext = aContext;
    mDataset         = message;

    mTimer.Start(1);

exit:
    FreeMessageOnError(message, error);
    return error;
}

void DatasetUpdater::CancelUpdate(void)
{
    VerifyOrExit(mDataset != nullptr);

    FreeMessage(mDataset);
    mTimer.Stop();

exit:
    return;
}

void DatasetUpdater::HandleTimer(Timer &aTimer)
{
    aTimer.Get<DatasetUpdater>().HandleTimer();
}

void DatasetUpdater::HandleTimer(void)
{
    PreparePendingDataset();
}

void DatasetUpdater::PreparePendingDataset(void)
{
    Dataset                dataset(Dataset::kPending);
    MeshCoP::Dataset::Info requestedDataset;
    otError                error;

    VerifyOrExit(!Get<Mle::Mle>().IsDisabled(), error = OT_ERROR_INVALID_STATE);

    IgnoreError(mDataset->Read(0, requestedDataset));

    error = Get<ActiveDataset>().Read(dataset);

    if (error != OT_ERROR_NONE)
    {
        // If there is no valid Active Dataset but MLE is not disabled,
        // set the timer to try again after the retry interval. This
        // handles the situation where a dataset update request comes
        // right after the network is formed but before the active
        // dataset is created.

        mTimer.Start(kRetryInterval);
        ExitNow(error = OT_ERROR_NONE);
    }

    IgnoreError(dataset.SetFrom(requestedDataset));

    if (!requestedDataset.IsDelayPresent())
    {
        uint32_t delay = kDefaultDelay;

        SuccessOrExit(error = dataset.SetTlv(Tlv::kDelayTimer, delay));
    }

    {
        Timestamp timestamp;

        if (Get<PendingDataset>().GetTimestamp() != nullptr)
        {
            timestamp = *Get<PendingDataset>().GetTimestamp();
        }

        timestamp.AdvanceRandomTicks();
        dataset.SetTimestamp(timestamp);
    }

    {
        ActiveTimestampTlv *tlv = dataset.GetTlv<ActiveTimestampTlv>();
        tlv->AdvanceRandomTicks();
    }

    SuccessOrExit(error = Get<PendingDataset>().Save(dataset));

exit:
    if (error != OT_ERROR_NONE)
    {
        Finish(error);
    }
}

void DatasetUpdater::Finish(otError aError)
{
    OT_ASSERT(mDataset != nullptr);

    FreeMessage(mDataset);
    mDataset = nullptr;

    if (mCallback != nullptr)
    {
        mCallback(aError, mCallbackContext);
    }
}

void DatasetUpdater::HandleNotifierEvents(Events aEvents)
{
    MeshCoP::Dataset::Info requestedDataset;
    MeshCoP::Dataset::Info dataset;

    VerifyOrExit(mDataset != nullptr);

    VerifyOrExit(aEvents.ContainsAny(kEventActiveDatasetChanged | kEventPendingDatasetChanged));

    IgnoreError(mDataset->Read(0, requestedDataset));

    if (aEvents.Contains(kEventActiveDatasetChanged) && Get<MeshCoP::ActiveDataset>().Read(dataset) == OT_ERROR_NONE)
    {
        if (requestedDataset.IsSubsetOf(dataset))
        {
            Finish(OT_ERROR_NONE);
        }
        else if (requestedDataset.GetActiveTimestamp() <= dataset.GetActiveTimestamp())
        {
            Finish(OT_ERROR_ALREADY);
        }
    }

    if (aEvents.Contains(kEventPendingDatasetChanged) && Get<MeshCoP::PendingDataset>().Read(dataset) == OT_ERROR_NONE)
    {
        if (!requestedDataset.IsSubsetOf(dataset))
        {
            Finish(OT_ERROR_ALREADY);
        }
    }

exit:
    return;
}

} // namespace MeshCoP
} // namespace ot

#endif // #if (OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE || OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE) && OPENTHREAD_FTD
