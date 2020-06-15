/*
 *  Copyright (c) 2016-2020, The OpenThread Authors.
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
 *   This file includes definitions for MLE Discover Scan process.
 */

#ifndef DISCOVER_SCANNER_HPP_
#define DISCOVER_SCANNER_HPP_

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "common/timer.hpp"
#include "mac/channel_mask.hpp"
#include "mac/mac.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop.hpp"
#include "thread/mle.hpp"

namespace ot {

class MeshForwarder;

namespace Mle {

/**
 * This class implements MLE Discover Scan.
 *
 */
class DiscoverScanner : public InstanceLocator
{
    friend class ot::Instance;
    friend class ot::MeshForwarder;
    friend class Mle;

public:
    enum
    {
        kDefaultScanDuration = Mac::kScanDurationDefault, ///< Default scan duration (per channel), in milliseconds.
    };

    /**
     * This type represents Discover Scan result.
     *
     */
    typedef otActiveScanResult ScanResult;

    /**
     * This type represents the handler function pointer called with any Discover Scan result or when the scan
     * completes.
     *
     * The handler function format is `void (*oHandler)(ScanResult *aResult, void *aContext);`. End of scan is
     * indicated by `aResult` pointer being set to NULL.
     *
     */
    typedef otHandleActiveScanResult Handler;

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit DiscoverScanner(Instance &aInstance);

    /**
     * This method starts a Thread Discovery Scan.
     *
     * @param[in]  aScanChannels      Channel mask listing channels to scan (if empty, use all supported channels).
     * @param[in]  aPanId             The PAN ID filter (set to Broadcast PAN to disable filter).
     * @param[in]  aJoiner            Value of the Joiner Flag in the Discovery Request TLV.
     * @param[in]  aEnableFiltering   Enable filtering MLE Discovery Responses that don't match our factory EUI64.
     * @param[in]  aHandler           A pointer to a function that is called on receiving an MLE Discovery Response.
     * @param[in]  aContext           A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE       Successfully started a Thread Discovery Scan.
     * @retval OT_ERROR_NO_BUFS    Could not allocate message for Discovery Request.
     * @retval OT_ERROR_BUSY       Thread Discovery Scan is already in progress.
     *
     */
    otError Discover(const Mac::ChannelMask &aScanChannels,
                     Mac::PanId              aPanId,
                     bool                    aJoiner,
                     bool                    aEnableFiltering,
                     Handler                 aHandler,
                     void *                  aContext);

    /**
     * This method indicates whether or not an MLE Thread Discovery Scan is currently in progress.
     *
     * @returns true if an MLE Thread Discovery Scan is in progress, false otherwise.
     *
     */
    bool IsInProgress(void) const { return (mState != kStateIdle); }

private:
    enum State
    {
        kStateIdle,
        kStateScanning,
        kStateScanDone,
    };

    // Methods used by `MeshForwarder`
    otError PrepareDiscoveryRequestFrame(Mac::TxFrame &aFrame);
    void    HandleDiscoveryRequestFrameTxDone(Message &aMessage);
    void    Stop(void) { HandleDiscoverComplete(); }

    // Methods used from `Mle`
    void HandleDiscoveryResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const;

    void        HandleDiscoverComplete(void);
    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    Handler                               mHandler;
    void *                                mHandlerContext;
    TimerMilli                            mTimer;
    MeshCoP::SteeringData::HashBitIndexes mFilterIndexes;
    Mac::ChannelMask                      mScanChannels;
    State                                 mState;
    uint8_t                               mScanChannel;
    bool                                  mEnableFiltering : 1;
    bool                                  mShouldRestorePanId : 1;
};

} // namespace Mle
} // namespace ot

#endif // DISCOVER_SCANNER_HPP_
