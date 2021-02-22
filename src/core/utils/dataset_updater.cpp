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
namespace Utils {

DatasetUpdater::DatasetUpdater(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStateIdle)
    , mWaitInterval(kWaitInterval)
    , mCallback(nullptr)
    , mCallbackContext(nullptr)
    , mTimer(aInstance, DatasetUpdater::HandleTimer)
    , mDataset(nullptr)
{
}

otError DatasetUpdater::RequestUpdate(const MeshCoP::Dataset::Info &aDataset,
                                      Callback                      aCallback,
                                      void *                        aContext,
                                      uint32_t                      aReryWaitInterval)
{
    otError  error   = OT_ERROR_NONE;
    Message *message = nullptr;

    VerifyOrExit(!Get<Mle::Mle>().IsDisabled(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mState == kStateIdle, error = OT_ERROR_BUSY);

    VerifyOrExit(!aDataset.IsActiveTimestampPresent() && !aDataset.IsPendingTimestampPresent(),
                 error = OT_ERROR_INVALID_ARGS);

    message = Get<MessagePool>().New(Message::kTypeOther, 0);
    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Append(aDataset));

    mCallback        = aCallback;
    mCallbackContext = aContext;
    mWaitInterval    = aReryWaitInterval;
    mDataset         = message;
    mState           = kStateUpdateRequested;

    PreparePendingDataset();

exit:
    FreeMessageOnError(message, error);
    return error;
}

void DatasetUpdater::CancelUpdate(void)
{
    if (mState != kStateIdle)
    {
        FreeMessage(mDataset);
        mState = kStateIdle;
        mTimer.Stop();
    }
}

void DatasetUpdater::HandleTimer(Timer &aTimer)
{
    aTimer.Get<DatasetUpdater>().HandleTimer();
}

void DatasetUpdater::HandleTimer(void)
{
    switch (mState)
    {
    case kStateIdle:
        break;
    case kStateUpdateRequested:
    case kStateSentMgmtPendingDataset:
        PreparePendingDataset();
        break;
    }
}

void DatasetUpdater::PreparePendingDataset(void)
{
    otError                error;
    MeshCoP::Dataset::Info newDataset;
    MeshCoP::Dataset::Info curDataset;

    VerifyOrExit(mState != kStateIdle);

    VerifyOrExit(!Get<Mle::Mle>().IsDisabled(), Finish(OT_ERROR_INVALID_STATE));

    error = Get<MeshCoP::ActiveDataset>().Read(curDataset);

    if (error != OT_ERROR_NONE)
    {
        // If there is no valid Active Dataset but MLE is not disabled,
        // set the timer to try again after the retry interval. This
        // handles the situation where a dataset update request comes
        // right after the network is formed but before the active
        // dataset is created.

        mState = kStateUpdateRequested;
        mTimer.Start(kRetryInterval);
        ExitNow();
    }

    IgnoreError(mDataset->Read(0, newDataset));

    if (newDataset.IsSubsetOf(curDataset))
    {
        // If new requested Dataset is already contained in the current
        // Active Dataset, no change is required, and we can report the
        // update to be successful.

        Finish(OT_ERROR_NONE);
        ExitNow();
    }

    if (newDataset.IsActiveTimestampPresent())
    {
        // Presence of the active timestamp in the new Dataset
        // indicates that it is a retry. In this case, we ensure
        // that the timestamp is ahead of current active dataset.
        // This covers the case where another device in network
        // requested a Dataset update after this device.

        VerifyOrExit(newDataset.GetActiveTimestamp() > curDataset.GetActiveTimestamp(), Finish(OT_ERROR_ALREADY));
    }
    else
    {
        newDataset.SetActiveTimestamp(curDataset.GetActiveTimestamp() +
                                      Random::NonCrypto::GetUint32InRange(1, kMaxTimestampIncrease));
    }

    if (!newDataset.IsDelayPresent())
    {
        newDataset.SetDelay(kDefaultDelay);
    }

    if (!newDataset.IsPendingTimestampPresent())
    {
        uint32_t timestampIncrease = Random::NonCrypto::GetUint32InRange(1, kMaxTimestampIncrease);

        if (Get<MeshCoP::PendingDataset>().Read(curDataset) == OT_ERROR_NONE)
        {
            newDataset.SetPendingTimestamp(curDataset.GetPendingTimestamp() + timestampIncrease);
        }
        else
        {
            newDataset.SetPendingTimestamp(timestampIncrease);
        }

        mDataset->Write(0, newDataset);
    }

    error = Get<MeshCoP::PendingDataset>().SendSetRequest(newDataset, nullptr, 0);

    if (error == OT_ERROR_NONE)
    {
        mState = kStateSentMgmtPendingDataset;
        mTimer.Start(newDataset.GetDelay() + mWaitInterval);
    }
    else
    {
        mTimer.Start(kRetryInterval);
    }

exit:
    return;
}

void DatasetUpdater::Finish(otError aError)
{
    FreeMessage(mDataset);
    mState = kStateIdle;

    if (mCallback != nullptr)
    {
        mCallback(aError, mCallbackContext);
    }
}

void DatasetUpdater::HandleNotifierEvents(Events aEvents)
{
    VerifyOrExit(mState == kStateSentMgmtPendingDataset);

    if (aEvents.Contains(kEventActiveDatasetChanged))
    {
        MeshCoP::Dataset::Info requestedDataset;
        MeshCoP::Dataset::Info activeDataset;

        SuccessOrExit(Get<MeshCoP::ActiveDataset>().Read(activeDataset));
        IgnoreError(mDataset->Read(0, requestedDataset));

        if (requestedDataset.IsSubsetOf(activeDataset))
        {
            Finish(OT_ERROR_NONE);
        }
        else if (requestedDataset.GetActiveTimestamp() <= activeDataset.GetActiveTimestamp())
        {
            Finish(OT_ERROR_ALREADY);
        }
    }

exit:
    return;
}

} // namespace Utils
} // namespace ot

#endif // #if (OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE || OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE) && OPENTHREAD_FTD
