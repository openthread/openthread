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
 *   This file implements the vendor information.
 */

#include "vendor_info.hpp"

#include "instance/instance.hpp"

namespace ot {

const char VendorInfo::kName[]      = OPENTHREAD_CONFIG_NET_DIAG_VENDOR_NAME;
const char VendorInfo::kModel[]     = OPENTHREAD_CONFIG_NET_DIAG_VENDOR_MODEL;
const char VendorInfo::kSwVersion[] = OPENTHREAD_CONFIG_NET_DIAG_VENDOR_SW_VERSION;
const char VendorInfo::kAppUrl[]    = OPENTHREAD_CONFIG_NET_DIAG_VENDOR_APP_URL;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
static constexpr char kNamePrefix[] = "RD:";

static_assert(CheckConstStringPrefix(OPENTHREAD_CONFIG_NET_DIAG_VENDOR_NAME, kNamePrefix),
              "VENDOR_NAME MUST start with 'RD:' prefix for a reference device.");
#endif

VendorInfo::VendorInfo(Instance &aInstance)
    : InstanceLocator(aInstance)
{
    static_assert(sizeof(kName) <= sizeof(NameStringType), "VENDOR_NAME is too long");
    static_assert(sizeof(kModel) <= sizeof(ModelStringType), "VENDOR_MODEL is too long");
    static_assert(sizeof(kSwVersion) <= sizeof(SwVersionStringType), "VENDOR_SW_VERSION is too long");
    static_assert(sizeof(kAppUrl) <= sizeof(AppUrlStringType), "VENDOR_APP_URL is too long");

#if OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE
    memcpy(mName, kName, sizeof(kName));
    memcpy(mModel, kModel, sizeof(kModel));
    memcpy(mSwVersion, kSwVersion, sizeof(kSwVersion));
    memcpy(mAppUrl, kAppUrl, sizeof(kAppUrl));
#endif
}

#if OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE

Error VendorInfo::SetName(const char *aName)
{
    Error error;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    VerifyOrExit(aName != nullptr && StringStartsWith(aName, kNamePrefix), error = kErrorInvalidArgs);
#endif

    SuccessOrExit(error = StringCopy(mName, aName, kStringCheckUtf8Encoding));

exit:
    return error;
}

Error VendorInfo::SetModel(const char *aModel) { return StringCopy(mModel, aModel, kStringCheckUtf8Encoding); }

Error VendorInfo::SetSwVersion(const char *aSwVersion)
{
    return StringCopy(mSwVersion, aSwVersion, kStringCheckUtf8Encoding);
}

Error VendorInfo::SetAppUrl(const char *aAppUrl) { return StringCopy(mAppUrl, aAppUrl, kStringCheckUtf8Encoding); }

#endif // OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE

} // namespace ot
