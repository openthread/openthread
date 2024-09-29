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
 */

#include "dataset_updater.hpp"

#if (OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE || OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE) && OPENTHREAD_FTD

#include "instance/instance.hpp"

namespace ot {
namespace MeshCoP {

DatasetUpdater::DatasetUpdater(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mDataset(nullptr)
{
}

Error DatasetUpdater::RequestUpdate(const Dataset::Info &aDataset, UpdaterCallback aCallback, void *aContext)
{
    Dataset dataset;

    dataset.SetFrom(aDataset);
    return RequestUpdate(dataset, aCallback, aContext);
}

Error DatasetUpdater::RequestUpdate(Dataset &aDataset, UpdaterCallback aCallback, void *aContext)
{
    Error     error;
    Message  *message = nullptr;
    Dataset   activeDataset;
    Timestamp activeTimestamp;
    Timestamp pendingTimestamp;

    error = kErrorInvalidState;
    VerifyOrExit(!Get<Mle::Mle>().IsDisabled());
    SuccessOrExit(Get<ActiveDatasetManager>().Read(activeDataset));
    SuccessOrExit(activeDataset.Read<ActiveTimestampTlv>(activeTimestamp));

    error = kErrorInvalidArgs;
    SuccessOrExit(aDataset.ValidateTlvs());
    VerifyOrExit(!aDataset.ContainsTlv(Tlv::kActiveTimestamp));
    VerifyOrExit(!aDataset.ContainsTlv(Tlv::kPendingTimestamp));

    VerifyOrExit(!IsUpdateOngoing(), error = kErrorBusy);

    VerifyOrExit(!aDataset.IsSubsetOf(activeDataset), error = kErrorAlready);

    // Set the Active and Pending Timestamps for new requested Dataset
    // by advancing ticks on the current timestamp values.

    activeTimestamp.AdvanceRandomTicks();
    SuccessOrExit(error = aDataset.Write<ActiveTimestampTlv>(activeTimestamp));

    pendingTimestamp = Get<PendingDatasetManager>().GetTimestamp();

    if (!pendingTimestamp.IsValid())
    {
        pendingTimestamp.Clear();
    }

    pendingTimestamp.AdvanceRandomTicks();
    SuccessOrExit(error = aDataset.Write<PendingTimestampTlv>(pendingTimestamp));

    if (!aDataset.ContainsTlv(Tlv::kDelayTimer))
    {
        SuccessOrExit(error = aDataset.Write<DelayTimerTlv>(kDefaultDelay));
    }

    SuccessOrExit(error = activeDataset.WriteTlvsFrom(aDataset));

    // Store the dataset in an allocated message to track update
    // status and report the outcome via the callback.

    message = Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = message->AppendBytes(aDataset.GetBytes(), aDataset.GetLength()));

    Get<PendingDatasetManager>().SaveLocal(activeDataset);

    mCallback.Set(aCallback, aContext);
    mDataset = message;

exit:
    FreeMessageOnError(message, error);
    return error;
}

void DatasetUpdater::CancelUpdate(void)
{
    VerifyOrExit(IsUpdateOngoing());

    FreeMessage(mDataset);
    mDataset = nullptr;

exit:
    return;
}

void DatasetUpdater::Finish(Error aError)
{
    VerifyOrExit(IsUpdateOngoing());

    FreeMessage(mDataset);
    mDataset = nullptr;
    mCallback.InvokeIfSet(aError);

exit:
    return;
}

void DatasetUpdater::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventActiveDatasetChanged))
    {
        HandleDatasetChanged(Dataset::kActive);
    }

    if (aEvents.Contains(kEventPendingDatasetChanged))
    {
        HandleDatasetChanged(Dataset::kPending);
    }
}

void DatasetUpdater::HandleDatasetChanged(Dataset::Type aType)
{
    Dataset     requestedDataset;
    Dataset     newDataset;
    Timestamp   newTimestamp;
    Timestamp   requestedTimestamp;
    OffsetRange offsetRange;

    VerifyOrExit(IsUpdateOngoing());

    offsetRange.InitFromMessageFullLength(*mDataset);
    SuccessOrExit(requestedDataset.SetFrom(*mDataset, offsetRange));

    if (aType == Dataset::kActive)
    {
        SuccessOrExit(Get<ActiveDatasetManager>().Read(newDataset));
    }
    else
    {
        SuccessOrExit(Get<PendingDatasetManager>().Read(newDataset));
    }

    // Check if the new dataset includes the requested changes. If
    // found in the Active Dataset, report success and finish. If
    // found in the Pending Dataset, wait for it to be applied as
    // Active.

    if (requestedDataset.IsSubsetOf(newDataset))
    {
        if (aType == Dataset::kActive)
        {
            Finish(kErrorNone);
        }

        ExitNow();
    }

    // If the new timestamp is ahead of the requested timestamp, it
    // means there was a conflicting update (possibly from another
    // device). In this case, report the update as a failure.

    SuccessOrExit(newDataset.ReadTimestamp(aType, newTimestamp));
    SuccessOrExit(requestedDataset.ReadTimestamp(aType, requestedTimestamp));

    if (newTimestamp >= requestedTimestamp)
    {
        Finish(kErrorAlready);
    }

exit:
    return;
}

} // namespace MeshCoP
} // namespace ot

#endif // #if (OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE || OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE) && OPENTHREAD_FTD
