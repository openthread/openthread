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
 *   This file includes definitions for Dataset Updater.
 */

#ifndef DATASET_UPDATER_HPP_
#define DATASET_UPDATER_HPP_

#include "openthread-core-config.h"

#include <openthread/dataset_updater.h>

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/meshcop_tlvs.hpp"

namespace ot {
namespace Utils {

#if (OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE || OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE) && OPENTHREAD_FTD

/**
 * This class implements the Dataset Updater.
 *
 */
class DatasetUpdater : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;

public:
    /**
     * This constructor initializes a `DatasetUpdater` object.
     *
     * @param[in]   aInstance  A reference to the OpenThread instance.
     *
     */
    explicit DatasetUpdater(Instance &aInstance);

    /**
     * This type represents the callback function pointer which is called when a Dataset update request finishes,
     * reporting success or failure status of the request.
     *
     * The function pointer has the syntax `void (*Callback)(otError aError, void *aContext)`.
     *
     * @param[in] aError   The error status.
     *                     OT_ERROR_NONE           indicates Dataset update successfully finished.
     *                     OT_ERROR_INVALID_STATE  indicates failure due invalid state (MLE being disabled).
     *                     OT_ERROR_ALREADY        indicates failure due to another device within network requesting
     *                                             a conflicting Dataset update.
     * @param[in] aContext A pointer to the arbitrary context provided by the user.
     *
     */
    typedef otDatasetUpdaterCallback Callback;

    /**
     * This method requests an update to Operational Dataset.
     *
     * @p aDataset should contain the fields to be updated and their new value. It must not contain Active or Pending
     * Timestamp fields. The Delay field is optional, if not provided a default value (`kDefaultDelay`) would be used.
     *
     * @param[in]  aDataset                Dataset info containing fields to change.
     * @param[in]  aCallback               A callback to indicate when Dataset update request finishes.
     * @param[in]  aContext                An arbitrary context passed to callback.
     * @param[in]  aRetryWaitInterval      The wait time after sending Pending dataset before retrying (interval in ms).
     *
     * @retval OT_ERROR_NONE           Dataset update started successfully (@p aCallback will be invoked on completion).
     * @retval OT_ERROR_INVALID_STATE  Device is disabled (MLE is disabled).
     * @retval OT_ERROR_INVALID_ARGS   The @p aDataset is not valid (contains Active or Pending Timestamp).
     * @retval OT_ERROR_BUSY           Cannot start update, a previous one is ongoing.
     * @retval OT_ERROR_NO_BUFS        Could not allocated buffer to save Dataset.
     *
     */
    otError RequestUpdate(const MeshCoP::Dataset::Info &aDataset,
                          Callback                      aCallback,
                          void *                        aContext,
                          uint32_t                      aReryWaitInterval = kWaitInterval);

    /**
     * This method cancels an ongoing (if any) Operational Dataset update request.
     *
     */
    void CancelUpdate(void);

    /**
     * This method indicates whether there is an ongoing Operation Dataset update request.
     *
     * @retval TRUE    There is an ongoing update.
     * @retval FALSE   There is no ongoing update.
     *
     */
    bool IsUpdateOngoing(void) const { return (mState != kStateIdle); }

private:
    enum State : uint8_t
    {
        kStateIdle,
        kStateUpdateRequested,
        kStateSentMgmtPendingDataset,
    };

    enum : uint32_t
    {
        // Default delay (in ms) in Pending Dataset.
        kDefaultDelay = OPENTHREAD_CONFIG_DATASET_UPDATER_DEFAULT_DELAY,

        // Default wait interval (in ms) after sending Pending Dataset to retry (in addition Dataset Delay)
        kWaitInterval = OPENTHREAD_CONFIG_DATASET_UPDATER_DEFAULT_RETRY_WAIT_INTERVAL,

        kRetryInterval        = 1000, // In ms. Retry interval when preparing and/or sending Pending Dataset fails.
        kMaxTimestampIncrease = 128,  // Maximum increase of Pending/Active Timestamp during Dataset Update.
    };

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);
    void        PreparePendingDataset(void);
    void        Finish(otError aError);
    void        HandleNotifierEvents(Events aEvents);

    State      mState;
    uint32_t   mWaitInterval;
    Callback   mCallback;
    void *     mCallbackContext;
    TimerMilli mTimer;
    Message *  mDataset;
};

#endif // (OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE || OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE) && OPENTHREAD_FTD

} // namespace Utils
} // namespace ot

#endif // DATASET_UPDATER_HPP_
