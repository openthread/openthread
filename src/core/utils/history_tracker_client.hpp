/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes definitions to support History Tracker Client.
 */

#ifndef OT_CORE_UTILS_HISTORY_TRACKER_CLIENT_HPP_
#define OT_CORE_UTILS_HISTORY_TRACKER_CLIENT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_CLIENT_ENABLE

#include <openthread/history_tracker.h>

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "thread/tmf.hpp"
#include "utils/history_tracker.hpp"
#include "utils/history_tracker_tlvs.hpp"

namespace ot {
namespace HistoryTracker {

/**
 * Implements a History Tracker Client
 */
class Client : public InstanceLocator
{
    friend class Tmf::Agent;

public:
    typedef otHistoryTrackerNetInfoCallback NetInfoCallback; ///< Callback function type for Network Info queries.

    /**
     * Constructor for the Client.
     *
     * @param[in] aInstance The OpenThread instance.
     */
    explicit Client(Instance &aInstance);

    /**
     * Cancels any ongoing query.
     */
    void CancelQuery(void);

    /**
     * Queries for Network Info entries from a specified RLOC16.
     *
     * Upon successful initiation of the query, the provided @p aCallback will be invoked to report the requested
     * retrieved entries (parsing the answer).  The callback parameter `aError` indicates if any error occurs. If there
     * are more entries to be provided, `aError` will be set to `kErrorPending`. The end of the list is indicated by
     * `aError` being set to `kErrorNone` with a null entry pointer. Any other errors, such as `kErrorResponseTimeout`
     * or `kErrorParse` (if the received response has an invalid format), will also be indicated by `aError`.
     *
     * @param[in] aRloc16       The RLOC16 of the device to query.
     * @param[in] aMaxEntries   The maximum number of entries to request (0 indicates all available entries).
     * @param[in] aMaxEntryAge  The maximum age (in milliseconds) of entries to request (0 indicates no age limit).
     * @param[in] aCallback     A pointer to a callback function to be called when the query response is received.
     * @param[in] aContext      A user-defined context pointer to be passed to the callback function.
     *
     * @retval kErrorNone             If the query was successfully sent.
     * @retval kErrorBusy             If a query is already in progress.
     * @retval kErrorNoBufs           If there are insufficient message buffers to send the query.
     * @retval kErrorInvalidState     Device is not attached.
     */
    Error QueryNetInfo(uint16_t        aRloc16,
                       uint16_t        aMaxEntries,
                       uint32_t        aMaxEntryAge,
                       NetInfoCallback aCallback,
                       void           *aContext);

private:
    static constexpr uint16_t kResponseTimeout = 5000;

    Error SendQuery(Tlv::Type aTlvType, uint16_t aMaxEntries, uint32_t aMaxEntryAge, uint16_t aRloc16);
    Error ProcessAnswer(const Coap::Msg &aMsg);
    void  ProcessNetInfoAnswer(const Coap::Message &aMessage);
    void  Finalize(Error aError);
    void  HandleTimer(void);

    template <Uri kUri> void HandleTmf(Coap::Msg &aMsg);

    union Callbacks
    {
        Callbacks(void) {}

        Callback<NetInfoCallback> mNetInfo;
    };

    using TimeoutTimer = TimerMilliIn<Client, &Client::HandleTimer>;

    bool         mActive;
    Tlv::Type    mTlvType;
    Callbacks    mCallbacks;
    uint16_t     mQueryRloc16;
    uint16_t     mQueryId;
    uint16_t     mAnswerIndex;
    TimeoutTimer mTimer;
};

DeclareTmfHandler(Client, kUriHistoryAnswer);

} // namespace HistoryTracker
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_CLIENT_ENABLE

#endif // OT_CORE_UTILS_HISTORY_TRACKER_CLIENT_HPP_
