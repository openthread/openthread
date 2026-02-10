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
 *   This file includes definitions for maintaining vendor information (name, model, etc).
 */

#ifndef OT_CORE_THREAD_VENDOR_INFO_HPP_
#define OT_CORE_THREAD_VENDOR_INFO_HPP_

#include "openthread-core-config.h"

#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "thread/network_diagnostic_tlvs.hpp"

namespace ot {

/**
 * Represents the vendor information.
 */
class VendorInfo : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Initializes the `VendorInfo`.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    explicit VendorInfo(Instance &aInstance);

#if OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE
    /**
     * Returns the vendor name string.
     *
     * @returns The vendor name string.
     */
    const char *GetName(void) const { return mName; }

    /**
     * Sets the vendor name string.
     *
     * @param[in] aName     The vendor name string.
     *
     * If `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled, @p aName must start with the "RD:" prefix.
     * This is enforced to ensure reference devices are identifiable. If @p aName does not follow this pattern,
     * the name is rejected, and `kErrorInvalidArgs` is returned.
     *
     * @retval kErrorNone         Successfully set the vendor name.
     * @retval kErrorInvalidArgs  @p aName is not valid. It is too long, is not UTF-8, or does not start with
     *                            the "RD:" prefix when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.
     */
    Error SetName(const char *aName);

    /**
     * Returns the vendor model string.
     *
     * @returns The vendor model string.
     */
    const char *GetModel(void) const { return mModel; }

    /**
     * Sets the vendor model string.
     *
     * @param[in] aModel     The vendor model string.
     *
     * @retval kErrorNone         Successfully set the vendor model.
     * @retval kErrorInvalidArgs  @p aModel is not valid (too long or not UTF8).
     */
    Error SetModel(const char *aModel);

    /**
     * Returns the vendor software version string.
     *
     * @returns The vendor software version string.
     */
    const char *GetSwVersion(void) const { return mSwVersion; }

    /**
     * Sets the vendor sw version string
     *
     * @param[in] aSwVersion     The vendor sw version string.
     *
     * @retval kErrorNone         Successfully set the vendor sw version.
     * @retval kErrorInvalidArgs  @p aSwVersion is not valid (too long or not UTF8).
     */
    Error SetSwVersion(const char *aSwVersion);

    /**
     * Returns the vendor app URL string.
     *
     * @returns the vendor app URL string.
     */
    const char *GetAppUrl(void) const { return mAppUrl; }

    /**
     * Sets the vendor app URL string.
     *
     * @param[in] aAppUrl     The vendor app URL string
     *
     * @retval kErrorNone         Successfully set the vendor app URL.
     * @retval kErrorInvalidArgs  @p aAppUrl is not valid (too long or not UTF8).
     */
    Error SetAppUrl(const char *aAppUrl);

#else
    const char *GetName(void) const { return kName; }
    const char *GetModel(void) const { return kModel; }
    const char *GetSwVersion(void) const { return kSwVersion; }
    const char *GetAppUrl(void) const { return kAppUrl; }
#endif // OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE

private:
    // String buffer types (max size specified by corresponding TLV)
    typedef NetworkDiagnostic::VendorNameTlv::StringType      NameStringType;
    typedef NetworkDiagnostic::VendorModelTlv::StringType     ModelStringType;
    typedef NetworkDiagnostic::VendorSwVersionTlv::StringType SwVersionStringType;
    typedef NetworkDiagnostic::VendorAppUrlTlv::StringType    AppUrlStringType;

    static const char kName[];
    static const char kModel[];
    static const char kSwVersion[];
    static const char kAppUrl[];

#if OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE
    NameStringType      mName;
    ModelStringType     mModel;
    SwVersionStringType mSwVersion;
    AppUrlStringType    mAppUrl;
#endif
};

} // namespace ot

#endif // OT_CORE_THREAD_VENDOR_INFO_HPP_
