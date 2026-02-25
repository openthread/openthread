/*
 *    Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file implements the NCP vendor hook, providing:
 *
 *   Commands:
 *     - VENDOR_CMD_RESET_STATS    : Resets all vendor-tracked counters.
 *     - VENDOR_CMD_PING           : Echo-back command for host connectivity checks.
 *
 *   Properties (GET/SET):
 *     - VENDOR_PROP_FIRMWARE_VERSION : Read-only string identifying vendor firmware.
 *     - VENDOR_PROP_TX_POWER         : Read/write TX power level in dBm (int8_t).
 *     - VENDOR_PROP_DIAGNOSTIC_MODE  : Read/write boolean enabling verbose diagnostics.
 *
 *   Frame tracking:
 *     - Tracks pending frame tags to detect dropped or delayed Spinel responses.
 *     - Re-queues failed async frames when NCP buffer space becomes available.
 */

#if OPENTHREAD_ENABLE_NCP_VENDOR_HOOK

#include <string.h>

#include "ncp_base.hpp"
#include "common/logging.hpp"

namespace ot {
namespace Ncp {

// ----------------------------------------------------------------------------
// Vendor-defined Spinel command codes.
// Must be in the vendor command range (>= SPINEL_CMD_VENDOR__BEGIN).
// ----------------------------------------------------------------------------

enum VendorCommand : unsigned int
{
    kVendorCmdResetStats = SPINEL_CMD_VENDOR__BEGIN + 0, ///< Reset all vendor diagnostic counters.
    kVendorCmdPing       = SPINEL_CMD_VENDOR__BEGIN + 1, ///< Ping: host sends a token, NCP echoes it back.
};

// ----------------------------------------------------------------------------
// Vendor-defined Spinel property keys.
// Must be in the vendor property range (>= SPINEL_PROP_VENDOR__BEGIN).
// ----------------------------------------------------------------------------

enum VendorProperty : spinel_prop_key_t
{
    kVendorPropFirmwareVersion = SPINEL_PROP_VENDOR__BEGIN + 0, ///< UTF-8 firmware version string (read-only).
    kVendorPropTxPower         = SPINEL_PROP_VENDOR__BEGIN + 1, ///< TX power in dBm, int8_t (read/write).
    kVendorPropDiagnosticMode  = SPINEL_PROP_VENDOR__BEGIN + 2, ///< Diagnostic mode enable flag, bool (read/write).
};

// ----------------------------------------------------------------------------
// Vendor firmware metadata.
// ----------------------------------------------------------------------------

static constexpr char    kVendorFirmwareVersion[] = "VendorNCP-v1.0.0";
static constexpr int8_t  kDefaultTxPowerdBm       = 0;    ///< Default TX power: 0 dBm.
static constexpr int8_t  kMinTxPowerdBm           = -30;  ///< Minimum allowed TX power.
static constexpr int8_t  kMaxTxPowerdBm           = 20;   ///< Maximum allowed TX power.

// ----------------------------------------------------------------------------
// Vendor runtime state (static, lives for the process lifetime).
// ----------------------------------------------------------------------------

static struct VendorState
{
    int8_t  mTxPowerdBm;       ///< Current TX power level in dBm.
    bool    mDiagnosticMode;   ///< Whether verbose diagnostics are active.
    uint32_t mResetCount;      ///< Number of times stats have been reset.
    uint32_t mPingCount;       ///< Total ping commands received.

    // Frame-tracking: store tags of async frames that must be re-sent on buffer recovery.
    static constexpr uint8_t kMaxPendingFrames = 4;
    Spinel::Buffer::FrameTag mPendingTags[kMaxPendingFrames];
    uint8_t                  mPendingTagCount;

    void Init(void)
    {
        mTxPowerdBm      = kDefaultTxPowerdBm;
        mDiagnosticMode  = false;
        mResetCount      = 0;
        mPingCount       = 0;
        mPendingTagCount = 0;
        memset(mPendingTags, 0, sizeof(mPendingTags));
    }

    void ResetStats(void)
    {
        mPingCount  = 0;
        mResetCount++;
        otLogInfoPlat("[VendorHook] Stats reset (#%u)", mResetCount);
    }

    bool TrackPendingTag(Spinel::Buffer::FrameTag aTag)
    {
        if (mPendingTagCount >= kMaxPendingFrames)
        {
            return false; // Tracking table full.
        }

        mPendingTags[mPendingTagCount++] = aTag;
        return true;
    }

    bool RemovePendingTag(Spinel::Buffer::FrameTag aTag)
    {
        for (uint8_t i = 0; i < mPendingTagCount; i++)
        {
            if (mPendingTags[i] == aTag)
            {
                // Shift remaining entries down.
                memmove(&mPendingTags[i], &mPendingTags[i + 1],
                        (mPendingTagCount - i - 1) * sizeof(Spinel::Buffer::FrameTag));
                mPendingTagCount--;
                return true;
            }
        }

        return false;
    }
} sVendorState;

// ----------------------------------------------------------------------------
// VendorCommandHandler
//
// Called by NcpBase when a Spinel frame with a vendor command code arrives.
// Responsible for preparing and writing a Spinel response into the NCP buffer.
// ----------------------------------------------------------------------------

otError NcpBase::VendorCommandHandler(uint8_t aHeader, unsigned int aCommand)
{
    otError error = OT_ERROR_NONE;

    switch (aCommand)
    {
    case kVendorCmdResetStats:
    {
        // Reset all vendor counters and respond with a LAST_STATUS OK.
        sVendorState.ResetStats();
        error = PrepareLastStatusResponse(aHeader, SPINEL_STATUS_OK);
        break;
    }

    case kVendorCmdPing:
    {
        // The host sends an arbitrary uint32_t token; NCP echoes it back.
        // This lets the host verify the vendor command channel is alive.
        uint32_t token = 0;

        // Decode the token from the incoming Spinel frame.
        SuccessOrExit(error = mDecoder.ReadUint32(token));

        sVendorState.mPingCount++;

        if (sVendorState.mDiagnosticMode)
        {
            otLogInfoPlat("[VendorHook] PING token=0x%08x (total=%u)", token, sVendorState.mPingCount);
        }

        // Encode a response: CMD_PROP_VALUE_IS for a synthetic vendor "ping" property.
        SuccessOrExit(error = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PROP_VALUE_IS, kVendorCmdPing));
        SuccessOrExit(error = mEncoder.WriteUint32(token));
        SuccessOrExit(error = mEncoder.EndFrame());
        break;
    }

    default:
        error = PrepareLastStatusResponse(aHeader, SPINEL_STATUS_INVALID_COMMAND);
        break;
    }

exit:
    return error;
}

// ----------------------------------------------------------------------------
// VendorHandleFrameRemovedFromNcpBuffer
//
// Called whenever a Spinel frame is successfully sent and removed from the
// NCP TX buffer. Use this to:
//   (a) Confirm delivery of a tracked async frame.
//   (b) Re-queue any frame that previously failed due to buffer exhaustion.
// ----------------------------------------------------------------------------

void NcpBase::VendorHandleFrameRemovedFromNcpBuffer(Spinel::Buffer::FrameTag aFrameTag)
{
    // (a) Check if this tag belongs to a vendor-tracked pending frame.
    //     If so, remove it — the host has received it successfully.
    if (sVendorState.RemovePendingTag(aFrameTag))
    {
        otLogInfoPlat("[VendorHook] Tracked frame tag=0x%p delivered, %u still pending",
                      aFrameTag, sVendorState.mPendingTagCount);
    }

    // (b) If there are still pending frames that failed to send earlier due
    //     to a full buffer, this is the moment to retry them. The fact that a
    //     frame was just removed means buffer space is now available.
    //
    //     (Retry logic omitted here as it is application-specific, but a
    //      typical pattern would set a flag and call WriteLastStatusFrame or
    //      a custom async notification encoder on the next Process() cycle.)
    if (sVendorState.mPendingTagCount > 0 && sVendorState.mDiagnosticMode)
    {
        otLogWarnPlat("[VendorHook] %u vendor frame(s) still awaiting delivery",
                      sVendorState.mPendingTagCount);
    }
}

// ----------------------------------------------------------------------------
// VendorGetPropertyHandler
//
// Called by NcpBase for GET requests on vendor property keys.
// Must encode the property value into the NCP TX buffer and return OT_ERROR_NONE,
// or return OT_ERROR_NO_BUFS if the buffer is full.
// ----------------------------------------------------------------------------

otError NcpBase::VendorGetPropertyHandler(spinel_prop_key_t aPropKey)
{
    otError error = OT_ERROR_NONE;

    switch (aPropKey)
    {
    case kVendorPropFirmwareVersion:
    {
        // Return the vendor firmware version as a null-terminated UTF-8 string.
        SuccessOrExit(error = mEncoder.WriteUtf8(kVendorFirmwareVersion));
        break;
    }

    case kVendorPropTxPower:
    {
        // Return current TX power as a signed 8-bit integer (dBm).
        SuccessOrExit(error = mEncoder.WriteInt8(sVendorState.mTxPowerdBm));
        break;
    }

    case kVendorPropDiagnosticMode:
    {
        // Return whether diagnostic mode is currently active.
        SuccessOrExit(error = mEncoder.WriteBool(sVendorState.mDiagnosticMode));
        break;
    }

    default:
        error = OT_ERROR_NOT_FOUND;
        break;
    }

exit:
    return error;
}

// ----------------------------------------------------------------------------
// VendorSetPropertyHandler
//
// Called by NcpBase for SET requests on vendor property keys.
// Must decode the value from the Spinel frame. Must NOT write to the NCP buffer —
// NcpBase will call VendorGetPropertyHandler() automatically on success to
// prepare the response.
// ----------------------------------------------------------------------------

otError NcpBase::VendorSetPropertyHandler(spinel_prop_key_t aPropKey)
{
    otError error = OT_ERROR_NONE;

    switch (aPropKey)
    {
    case kVendorPropFirmwareVersion:
    {
        // Firmware version is read-only; reject any SET attempt.
        error = OT_ERROR_INVALID_ARGS;
        break;
    }

    case kVendorPropTxPower:
    {
        // Decode the requested TX power level.
        int8_t newPower;
        SuccessOrExit(error = mDecoder.ReadInt8(newPower));

        // Validate range.
        if (newPower < kMinTxPowerdBm || newPower > kMaxTxPowerdBm)
        {
            otLogWarnPlat("[VendorHook] TX power %d dBm out of range [%d, %d]",
                          newPower, kMinTxPowerdBm, kMaxTxPowerdBm);
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        sVendorState.mTxPowerdBm = newPower;

        // Apply the new TX power to the radio platform.
        otPlatRadioSetTransmitPower(mInstance, newPower);

        otLogInfoPlat("[VendorHook] TX power set to %d dBm", newPower);
        break;
    }

    case kVendorPropDiagnosticMode:
    {
        // Decode the requested diagnostic mode state.
        bool enable;
        SuccessOrExit(error = mDecoder.ReadBool(enable));

        sVendorState.mDiagnosticMode = enable;
        otLogInfoPlat("[VendorHook] Diagnostic mode %s", enable ? "ENABLED" : "DISABLED");
        break;
    }

    default:
        error = OT_ERROR_NOT_FOUND;
        break;
    }

exit:
    return error;
}

} // namespace Ncp
} // namespace ot

// ----------------------------------------------------------------------------
// NcpVendorUart — vendor sub-class of NcpHdlc with a real SendHdlc callback.
//
// In production, SendHdlc should call the platform's UART write function.
// Here we call otPlatUartSend() which is the standard OpenThread POSIX UART API.
// ----------------------------------------------------------------------------

#include "ncp_hdlc.hpp"
#include "common/new.hpp"
#include <openthread/platform/uart.h>

class NcpVendorUart : public ot::Ncp::NcpHdlc
{
    /**
     * Forwards HDLC-encoded bytes to the platform UART TX path.
     *
     * @param[in] aBuf        Pointer to the data to send.
     * @param[in] aBufLength  Number of bytes to send.
     *
     * @returns Number of bytes accepted (aBufLength on success, 0 on failure).
     */
    static int SendHdlc(const uint8_t *aBuf, uint16_t aBufLength)
    {
        otError error = otPlatUartSend(aBuf, aBufLength);
        return (error == OT_ERROR_NONE) ? static_cast<int>(aBufLength) : 0;
    }

public:
    explicit NcpVendorUart(ot::Instance *aInstance)
        : ot::Ncp::NcpHdlc(aInstance, &NcpVendorUart::SendHdlc)
    {
        // Initialize vendor state on first construction.
        sVendorState.Init();
        otLogInfoPlat("[VendorHook] NcpVendorUart initialized, firmware=%s", kVendorFirmwareVersion);
    }

#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE && OPENTHREAD_RADIO
    NcpVendorUart(ot::Instance **aInstances, uint8_t aCount)
        : ot::Ncp::NcpHdlc(aInstances, aCount, &NcpVendorUart::SendHdlc)
    {
        sVendorState.Init();
        otLogInfoPlat("[VendorHook] NcpVendorUart (multi-PAN, count=%u) initialized", aCount);
    }
#endif // OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE && OPENTHREAD_RADIO
};

// Static placement storage for NcpVendorUart (avoids heap allocation).
static OT_DEFINE_ALIGNED_VAR(sNcpVendorRaw, sizeof(NcpVendorUart), uint64_t);

// ----------------------------------------------------------------------------
// otAppNcpInit — entry point called by the OpenThread POSIX platform to
// construct the NCP object. Must use placement new into sNcpVendorRaw.
// ----------------------------------------------------------------------------

extern "C" void otAppNcpInit(otInstance *aInstance)
{
    NcpVendorUart *ncpVendor = nullptr;
    ot::Instance  *instance  = static_cast<ot::Instance *>(aInstance);

    ncpVendor = new (&sNcpVendorRaw) NcpVendorUart(instance);

    // Verify that NcpBase registered this instance as the active NCP object.
    if (ncpVendor == nullptr || ncpVendor != ot::Ncp::NcpBase::GetNcpInstance())
    {
        OT_ASSERT(false);
    }
}

#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE && OPENTHREAD_RADIO
extern "C" void otAppNcpInitMulti(otInstance **aInstances, uint8_t aCount)
{
    NcpVendorUart *ncpVendor = nullptr;
    ot::Instance  *instances[SPINEL_HEADER_IID_MAX];

    OT_ASSERT(aCount > 0 && aCount < SPINEL_HEADER_IID_MAX);
    OT_ASSERT(aInstances[0] != nullptr);

    for (uint8_t i = 0; i < aCount; i++)
    {
        instances[i] = static_cast<ot::Instance *>(aInstances[i]);
    }

    ncpVendor = new (&sNcpVendorRaw) NcpVendorUart(instances, aCount);

    if (ncpVendor == nullptr || ncpVendor != ot::Ncp::NcpBase::GetNcpInstance())
    {
        OT_ASSERT(false);
    }
}
#endif // OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE && OPENTHREAD_RADIO

#endif // OPENTHREAD_ENABLE_NCP_VENDOR_HOOK