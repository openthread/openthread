/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes definitions for scan result (discover scan or MAC active scan).
 */

#ifndef OT_CORE_MAC_SCAN_RESULT_HPP_
#define OT_CORE_MAC_SCAN_RESULT_HPP_

#include "openthread-core-config.h"

#include <openthread/link.h>

#include "common/as_core_type.hpp"
#include "common/callback.hpp"
#include "common/clearable.hpp"
#include "common/error.hpp"
#include "mac/mac_frame.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/extended_panid.hpp"
#include "meshcop/network_name.hpp"
#include "meshcop/steering_data.hpp"

namespace ot {

namespace Mac {
class Mac;
}

namespace Mle {
class DiscoverScanner;
}

/**
 * Represents a discover or active scan result.
 */
class ScanResult : public otActiveScanResult, public Clearable<ScanResult>
{
    friend class ot::Mac::Mac;
    friend class ot::Mle::DiscoverScanner;

public:
    /**
     * Represents the function pointer callback to notify scan result.
     */
    typedef otHandleActiveScanResult Handler;

    /**
     * Gets the IEEE 802.15.4 Extended Address.
     *
     * @returns A constant reference to the IEEE 802.15.4 Extended Address.
     */
    const Mac::ExtAddress &GetExtAddress(void) const { return AsCoreType(&mExtAddress); }

    /**
     * Gets the Thread Network Name.
     *
     * @returns A constant reference to the Thread Network Name.
     */
    const MeshCoP::NetworkName &GetNetworkName(void) const { return AsCoreType(&mNetworkName); }

    /**
     * Gets the Thread Extended PAN ID.
     *
     * @returns A constant reference to the Thread Extended PAN ID.
     */
    const MeshCoP::ExtendedPanId &GetExtendedPanId(void) const { return AsCoreType(&mExtendedPanId); }

    /**
     * Gets the Steering Data.
     *
     * @returns A constant reference to the Steering Data.
     */
    const MeshCoP::SteeringData &GetSteeringData(void) const { return AsCoreType(&mSteeringData); }

    /**
     * Gets the IEEE 802.15.4 PAN ID.
     *
     * @returns The IEEE 802.15.4 PAN ID.
     */
    Mac::PanId GetPanId(void) const { return mPanId; }

    /**
     * Gets the Joiner UDP Port.
     *
     * @returns The Joiner UDP Port.
     */
    uint16_t GetJoinerUdpPort(void) const { return mJoinerUdpPort; }

    /**
     * Gets the IEEE 802.15.4 Channel.
     *
     * @returns The IEEE 802.15.4 Channel.
     */
    uint8_t GetChannel(void) const { return mChannel; }

    /**
     * Gets the RSSI (dBm).
     *
     * @returns The RSSI.
     */
    int8_t GetRssi(void) const { return mRssi; }

    /**
     * Gets the LQI.
     *
     * @returns The LQI.
     */
    uint8_t GetLqi(void) const { return mLqi; }

    /**
     * Gets the Version.
     *
     * @returns The Version.
     */
    uint8_t GetVersion(void) const { return mVersion; }

    /**
     * Indicates whether the Native Commissioner flag is set.
     *
     * @retval TRUE   If the Native Commissioner flag is set.
     * @retval FALSE  If the Native Commissioner flag is not set.
     */
    bool IsNative(void) const { return mIsNative; }

    /**
     * Indicates whether the result is from MLE Discovery.
     *
     * @retval TRUE   If the result is from MLE Discovery.
     * @retval FALSE  If the result is not from MLE Discovery.
     */
    bool IsDiscover(void) const { return mDiscover; }

    /**
     * Indicates whether the Joining Permitted flag is set.
     *
     * @retval TRUE   If the Joining Permitted flag is set.
     * @retval FALSE  If the Joining Permitted flag is not set.
     */
    bool IsJoinable(void) const { return mIsJoinable; }

private:
    typedef Callback<Handler> ScanCallback;

    Error PopulateFromBeacon(const Mac::RxFrame *aBeaconFrame);
};

/**
 * Declares a Scan handler method in a given class `Type`.
 *
 * This macro simplifies the definition of scan handler callback. It defines a `static` handler method which can be
 * used as a `Handler `callback function pointer. The `static` handler acts as a wrapper, casting the `aContext`
 * pointer back to a `Type` object and invoking its member method with the same `MethodName`.
 *
 * The `Type` class MUST implement the following member method which will be invoked by the `static` handler:
 *
 *   void MethodName(const ScanResult *aScanResult);
 *
 * @param[in] Type        The class `Type` in which scan result handler is declared.
 * @param[in] MethodName  The handler method name.
 */
#define DeclareScanResultHandlerIn(Type, MethodName)                           \
    static void MethodName(otActiveScanResult *aScanResult, void *aContext)    \
    {                                                                          \
        static_cast<Type *>(aContext)->MethodName(AsCoreTypePtr(aScanResult)); \
    }                                                                          \
                                                                               \
    void MethodName(const ScanResult *aScanResult)

DefineCoreType(otActiveScanResult, ScanResult);

} // namespace ot

#endif // OT_CORE_MAC_SCAN_RESULT_HPP_
