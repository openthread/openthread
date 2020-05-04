/*
 *    Copyright (c) 2019-2020, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file shows how to implement the NCP vendor hook.
 */

#if OPENTHREAD_ENABLE_NCP_VENDOR_HOOK

#include "ncp_base.hpp"

#include "../../third_party/nxp/JN5189DK6/middleware/wireless/openthread/examples/posix_ota_server/app_ota.h"
namespace ot {
namespace Ncp {

otError NcpBase::VendorCommandHandler(uint8_t aHeader, unsigned int aCommand)
{
    otError error = OT_ERROR_NONE;

    switch (aCommand)
    {

    // TODO: Implement your command handlers here.
    case SPINEL_CMD_VENDOR_NXP_OTA_START:
        uint8_t        otaType, start_result;
        const char *   file_path;

        SuccessOrExit(error = mDecoder.ReadUint8(otaType));
        SuccessOrExit(error = mDecoder.ReadUtf8(file_path));

        //Start OTA process
        start_result = OtaServer_StartOta(otaType, file_path);

        SuccessOrExit(error = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_NXP_OTA_START_RET));
        SuccessOrExit(error = mEncoder.WriteUint8(start_result));
        SuccessOrExit(error = mEncoder.EndFrame());

        break;

    case SPINEL_CMD_VENDOR_NXP_OTA_STOP:
        //Stop OTA process
        SuccessOrExit(error = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_NXP_OTA_STOP_RET));
        SuccessOrExit(error = mEncoder.WriteUint8(OtaServer_StopOta()));
        SuccessOrExit(error = mEncoder.EndFrame());

        break;

    case SPINEL_CMD_VENDOR_NXP_OTA_STATUS:
        //Get OTA process status
        otaServerPercentageInfo_t otaInfo;
        OtaServer_GetOtaStatus(&otaInfo);

        SuccessOrExit(error = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_NXP_OTA_STATUS_RET));
        SuccessOrExit(error = mEncoder.WriteData((uint8_t *)&otaInfo, sizeof(otaInfo)));
        SuccessOrExit(error = mEncoder.EndFrame());

        break;

    default:
        error = PrepareLastStatusResponse(aHeader, SPINEL_STATUS_INVALID_COMMAND);
    }

exit:

    return error;
}

void NcpBase::VendorHandleFrameRemovedFromNcpBuffer(NcpFrameBuffer::FrameTag aFrameTag)
{
    // This method is a callback which mirrors `NcpBase::HandleFrameRemovedFromNcpBuffer()`.
    // It is called when a spinel frame is sent and removed from NCP buffer.
    //
    // (a) This can be used to track and verify that a vendor spinel frame response is
    //     delivered to the host (tracking the frame using its tag).
    //
    // (b) It indicates that NCP buffer space is now available (since a spinel frame is
    //     removed). This can be used to implement reliability mechanisms to re-send
    //     a failed spinel command response (or an async spinel frame) transmission
    //     (failed earlier due to NCP buffer being full).

    OT_UNUSED_VARIABLE(aFrameTag);
}

otError NcpBase::VendorGetPropertyHandler(spinel_prop_key_t aPropKey)
{
    otError error = OT_ERROR_NONE;

    switch (aPropKey)
    {

    // TODO: Implement your property get handlers here.
    //
    // Get handler should retrieve the property value and then encode and write the
    // value into the NCP buffer. If the "get" operation itself fails, handler should
    // write a `LAST_STATUS` with the error status into the NCP buffer. `OT_ERROR_NO_BUFS`
    // should be returned if NCP buffer is full and response cannot be written.

    default:
        error = OT_ERROR_NOT_FOUND;
        break;
    }

    return error;
}

otError NcpBase::VendorSetPropertyHandler(spinel_prop_key_t aPropKey)
{
    otError error = OT_ERROR_NONE;

    switch (aPropKey)
    {

    // TODO: Implement your property set handlers here.
    //
    // Set handler should first decode the value from the input Spinel frame and then
    // perform the corresponding set operation. The handler should not prepare the
    // spinel response and therefore should not write anything to the NCP buffer.
    // The error returned from handler (other than `OT_ERROR_NOT_FOUND`) indicates the
    // error in either parsing of the input or the error of the set operation. In case
    // of a successful "set", `NcpBase` set command handler will invoke the
    // `VendorGetPropertyHandler()` for the same property key to prepare the response.

    default:
        error = OT_ERROR_NOT_FOUND;
        break;
    }

    return error;
}

}  // namespace Ncp
}  // namespace ot

//-------------------------------------------------------------------------------------------------------------------
// When OPENTHREAD_ENABLE_NCP_VENDOR_HOOK is enabled, vendor code is
// expected to provide the `otNcpInit()` function. The reason behind
// this is to enable vendor code to define its own sub-class of
// `NcpBase` or `NcpUart`/`NcpSpi`.
//
// Example below show how to add a vendor sub-class over `NcpUart`.

#include "ncp_uart.hpp"
#include "common/new.hpp"

class NcpVendorUart : public ot::Ncp::NcpUart
{
public:
    NcpVendorUart(ot::Instance *aInstance)
        : ot::Ncp::NcpUart(aInstance)
    {}

    // Add public/private methods or member variables
};

static otDEFINE_ALIGNED_VAR(sNcpVendorRaw, sizeof(NcpVendorUart), uint64_t);

extern "C" void otNcpInit(otInstance *aInstance)
{
    NcpVendorUart *ncpVendor = NULL;
    ot::Instance * instance  = static_cast<ot::Instance *>(aInstance);

    ncpVendor = new (&sNcpVendorRaw) NcpVendorUart(instance);

    if (ncpVendor == NULL || ncpVendor != ot::Ncp::NcpBase::GetNcpInstance())
    {
        assert(false);
    }
}

#endif // #if OPENTHREAD_ENABLE_NCP_VENDOR_HOOK
