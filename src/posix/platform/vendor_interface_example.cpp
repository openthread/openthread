/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file provides an example on how to implement an OpenThread vendor interface to RCP.
 */

#include "openthread-posix-config.h"

#if OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE

#include "vendor_interface.hpp"
#include "common/code_utils.hpp"
#include "common/new.hpp"

namespace ot {
namespace Posix {
using ot::Spinel::SpinelInterface;

/**
 * Defines the vendor implementation object.
 */
class VendorInterfaceImpl
{
public:
    explicit VendorInterfaceImpl(const Url::Url &aRadioUrl)
        : mRadioUrl(aRadioUrl)
    {
    }

    // TODO: Add vendor code (add methods and/or member variables).

private:
    const Url::Url &mRadioUrl;
};

// ----------------------------------------------------------------------------
// `VendorInterface` API
// ----------------------------------------------------------------------------

static OT_DEFINE_ALIGNED_VAR(sVendorInterfaceImplRaw, sizeof(VendorInterfaceImpl), uint64_t);

VendorInterface::VendorInterface(const Url::Url &aRadioUrl)
{
    new (&sVendorInterfaceImplRaw) VendorInterfaceImpl(aRadioUrl);
    OT_UNUSED_VARIABLE(sVendorInterfaceImplRaw);
}

VendorInterface::~VendorInterface(void) { Deinit(); }

otError VendorInterface::Init(ReceiveFrameCallback aCallback, void *aCallbackContext, RxFrameBuffer &aFrameBuffer)
{
    OT_UNUSED_VARIABLE(aCallback);
    OT_UNUSED_VARIABLE(aCallbackContext);
    OT_UNUSED_VARIABLE(aFrameBuffer);

    // TODO: Implement vendor code here.

    return OT_ERROR_NONE;
}

void VendorInterface::Deinit(void)
{
    // TODO: Implement vendor code here.
}

uint32_t VendorInterface::GetBusSpeed(void) const { return 1000000; }

otError VendorInterface::HardwareReset(void)
{
    // TODO: Implement vendor code here.

    return OT_ERROR_NOT_IMPLEMENTED;
}

void VendorInterface::UpdateFdSet(void *aMainloopContext)
{
    OT_UNUSED_VARIABLE(aMainloopContext);

    // TODO: Implement vendor code here.
}

void VendorInterface::Process(const void *aMainloopContext)
{
    OT_UNUSED_VARIABLE(aMainloopContext);

    // TODO: Implement vendor code here.
}

otError VendorInterface::WaitForFrame(uint64_t aTimeoutUs)
{
    OT_UNUSED_VARIABLE(aTimeoutUs);

    // TODO: Implement vendor code here.

    return OT_ERROR_NONE;
}

otError VendorInterface::SendFrame(const uint8_t *aFrame, uint16_t aLength)
{
    OT_UNUSED_VARIABLE(aFrame);
    OT_UNUSED_VARIABLE(aLength);

    // TODO: Implement vendor code here.

    return OT_ERROR_NONE;
}

const otRcpInterfaceMetrics *VendorInterface::GetRcpInterfaceMetrics(void) const
{
    // TODO: Implement vendor code here.

    return nullptr;
}
} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE
