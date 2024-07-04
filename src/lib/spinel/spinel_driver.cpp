/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "spinel_driver.hpp"

#include <assert.h>

#include <openthread/platform/time.h>

#include "lib/platform/exit_code.h"
#include "lib/spinel/spinel.h"
#include "lib/utils/math.hpp"
#include "lib/utils/utils.hpp"

namespace ot {
namespace Spinel {

constexpr spinel_tid_t sTid = 1; ///< In Spinel Driver, only use Tid as 1.

SpinelDriver::SpinelDriver(void)
    : Logger("SpinelDriver")
    , mSpinelInterface(nullptr)
    , mWaitingKey(SPINEL_PROP_LAST_STATUS)
    , mIsWaitingForResponse(false)
    , mIid(SPINEL_HEADER_INVALID_IID)
    , mSpinelVersionMajor(-1)
    , mSpinelVersionMinor(-1)
    , mIsCoprocessorReady(false)
{
    memset(mVersion, 0, sizeof(mVersion));

    mReceivedFrameHandler = &HandleInitialFrame;
    mFrameHandlerContext  = this;
}

CoprocessorType SpinelDriver::Init(SpinelInterface    &aSpinelInterface,
                                   bool                aSoftwareReset,
                                   const spinel_iid_t *aIidList,
                                   uint8_t             aIidListLength)
{
    CoprocessorType coprocessorType;

    mSpinelInterface = &aSpinelInterface;
    mRxFrameBuffer.Clear();
    SuccessOrDie(mSpinelInterface->Init(HandleReceivedFrame, this, mRxFrameBuffer));

    VerifyOrDie(aIidList != nullptr, OT_EXIT_INVALID_ARGUMENTS);
    VerifyOrDie(aIidListLength != 0 && aIidListLength <= mIidList.GetMaxSize(), OT_EXIT_INVALID_ARGUMENTS);

    for (uint8_t i = 0; i < aIidListLength; i++)
    {
        SuccessOrDie(mIidList.PushBack(aIidList[i]));
    }
    mIid = aIidList[0];

    ResetCoprocessor(aSoftwareReset);
    SuccessOrDie(CheckSpinelVersion());
    SuccessOrDie(GetCoprocessorVersion());
    SuccessOrDie(GetCoprocessorCaps());

    coprocessorType = GetCoprocessorType();
    if (coprocessorType == OT_COPROCESSOR_UNKNOWN)
    {
        LogCrit("The coprocessor mode is unknown!");
        DieNow(OT_EXIT_FAILURE);
    }

    return coprocessorType;
}

void SpinelDriver::Deinit(void)
{
    // This allows implementing pseudo reset.
    new (this) SpinelDriver();
}

otError SpinelDriver::SendReset(uint8_t aResetType)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        buffer[kMaxSpinelFrame];
    spinel_ssize_t packed;

    // Pack the header, command and key
    packed = spinel_datatype_pack(buffer, sizeof(buffer), SPINEL_DATATYPE_COMMAND_S SPINEL_DATATYPE_UINT8_S,
                                  SPINEL_HEADER_FLAG | SPINEL_HEADER_IID(mIid), SPINEL_CMD_RESET, aResetType);

    EXPECT(packed > 0 && static_cast<size_t>(packed) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

    EXPECT_NO_ERROR(error = mSpinelInterface->SendFrame(buffer, static_cast<uint16_t>(packed)));
    LogSpinelFrame(buffer, static_cast<uint16_t>(packed), true /* aTx */);

exit:
    return error;
}

void SpinelDriver::ResetCoprocessor(bool aSoftwareReset)
{
    bool hardwareReset;
    bool resetDone = false;

    // Avoid resetting the device twice in a row in Multipan RCP architecture
    EXPECT(!mIsCoprocessorReady, resetDone = true);

    mWaitingKey = SPINEL_PROP_LAST_STATUS;

    if (aSoftwareReset && (SendReset(SPINEL_RESET_STACK) == OT_ERROR_NONE) && (WaitResponse() == OT_ERROR_NONE))
    {
        EXPECT(mIsCoprocessorReady, resetDone = false);
        LogCrit("Software reset co-processor successfully");
        EXIT_NOW(resetDone = true);
    }

    hardwareReset = (mSpinelInterface->HardwareReset() == OT_ERROR_NONE);

    if (hardwareReset)
    {
        EXPECT_NO_ERROR(WaitResponse());
    }

    resetDone = true;

    if (hardwareReset)
    {
        LogInfo("Hardware reset co-processor successfully");
    }
    else
    {
        LogInfo("co-processor self reset successfully");
    }

exit:
    if (!resetDone)
    {
        LogCrit("Failed to reset co-processor!");
        DieNow(OT_EXIT_FAILURE);
    }
}

void SpinelDriver::Process(const void *aContext)
{
    if (mRxFrameBuffer.HasSavedFrame())
    {
        ProcessFrameQueue();
    }

    mSpinelInterface->Process(aContext);

    if (mRxFrameBuffer.HasSavedFrame())
    {
        ProcessFrameQueue();
    }
}

otError SpinelDriver::SendCommand(uint32_t aCommand, spinel_prop_key_t aKey, spinel_tid_t aTid)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        buffer[kMaxSpinelFrame];
    spinel_ssize_t packed;
    uint16_t       offset;

    // Pack the header, command and key
    packed = spinel_datatype_pack(buffer, sizeof(buffer), "Cii", SPINEL_HEADER_FLAG | SPINEL_HEADER_IID(mIid) | aTid,
                                  aCommand, aKey);

    EXPECT(packed > 0 && static_cast<size_t>(packed) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

    offset = static_cast<uint16_t>(packed);

    EXPECT_NO_ERROR(error = mSpinelInterface->SendFrame(buffer, offset));
    LogSpinelFrame(buffer, offset, true /* aTx */);

exit:
    return error;
}

otError SpinelDriver::SendCommand(uint32_t          aCommand,
                                  spinel_prop_key_t aKey,
                                  spinel_tid_t      aTid,
                                  const char       *aFormat,
                                  va_list           aArgs)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        buffer[kMaxSpinelFrame];
    spinel_ssize_t packed;
    uint16_t       offset;

    // Pack the header, command and key
    packed = spinel_datatype_pack(buffer, sizeof(buffer), "Cii", SPINEL_HEADER_FLAG | SPINEL_HEADER_IID(mIid) | aTid,
                                  aCommand, aKey);

    EXPECT(packed > 0 && static_cast<size_t>(packed) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

    offset = static_cast<uint16_t>(packed);

    // Pack the data (if any)
    if (aFormat)
    {
        packed = spinel_datatype_vpack(buffer + offset, sizeof(buffer) - offset, aFormat, aArgs);
        EXPECT(packed > 0 && static_cast<size_t>(packed + offset) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

        offset += static_cast<uint16_t>(packed);
    }

    EXPECT_NO_ERROR(error = mSpinelInterface->SendFrame(buffer, offset));
    LogSpinelFrame(buffer, offset, true /* aTx */);

exit:
    return error;
}

void SpinelDriver::SetFrameHandler(ReceivedFrameHandler aReceivedFrameHandler,
                                   SavedFrameHandler    aSavedFrameHandler,
                                   void                *aContext)
{
    mReceivedFrameHandler = aReceivedFrameHandler;
    mSavedFrameHandler    = aSavedFrameHandler;
    mFrameHandlerContext  = aContext;
}

otError SpinelDriver::WaitResponse(void)
{
    otError  error = OT_ERROR_NONE;
    uint64_t end   = otPlatTimeGet() + kMaxWaitTime * kUsPerMs;

    LogDebg("Waiting response: key=%lu", Lib::Utils::ToUlong(mWaitingKey));

    do
    {
        uint64_t now = otPlatTimeGet();

        if ((end <= now) || (mSpinelInterface->WaitForFrame(end - now) != OT_ERROR_NONE))
        {
            LogWarn("Wait for response timeout");
            EXIT_NOW(error = OT_ERROR_RESPONSE_TIMEOUT);
        }
    } while (mIsWaitingForResponse || !mIsCoprocessorReady);

    mWaitingKey = SPINEL_PROP_LAST_STATUS;

exit:
    return error;
}

void SpinelDriver::HandleReceivedFrame(void *aContext) { static_cast<SpinelDriver *>(aContext)->HandleReceivedFrame(); }

void SpinelDriver::HandleReceivedFrame(void)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        header;
    spinel_ssize_t unpacked;
    bool           shouldSave = true;
    spinel_iid_t   iid;

    LogSpinelFrame(mRxFrameBuffer.GetFrame(), mRxFrameBuffer.GetLength(), false);
    unpacked = spinel_datatype_unpack(mRxFrameBuffer.GetFrame(), mRxFrameBuffer.GetLength(), "C", &header);

    // Accept spinel messages with the correct IID or broadcast IID.
    iid = SPINEL_HEADER_GET_IID(header);

    if (!mIidList.Contains(iid))
    {
        mRxFrameBuffer.DiscardFrame();
        EXIT_NOW();
    }

    EXPECT(unpacked > 0 && (header & SPINEL_HEADER_FLAG) == SPINEL_HEADER_FLAG, error = OT_ERROR_PARSE);

    assert(mReceivedFrameHandler != nullptr && mFrameHandlerContext != nullptr);
    mReceivedFrameHandler(mRxFrameBuffer.GetFrame(), mRxFrameBuffer.GetLength(), header, shouldSave,
                          mFrameHandlerContext);

    if (shouldSave)
    {
        error = mRxFrameBuffer.SaveFrame();
    }
    else
    {
        mRxFrameBuffer.DiscardFrame();
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        mRxFrameBuffer.DiscardFrame();
        LogWarn("Error handling spinel frame: %s", otThreadErrorToString(error));
    }
}

void SpinelDriver::HandleInitialFrame(const uint8_t *aFrame,
                                      uint16_t       aLength,
                                      uint8_t        aHeader,
                                      bool          &aSave,
                                      void          *aContext)
{
    static_cast<SpinelDriver *>(aContext)->HandleInitialFrame(aFrame, aLength, aHeader, aSave);
}

void SpinelDriver::HandleInitialFrame(const uint8_t *aFrame, uint16_t aLength, uint8_t aHeader, bool &aSave)
{
    spinel_prop_key_t key;
    uint8_t          *data   = nullptr;
    spinel_size_t     len    = 0;
    uint8_t           header = 0;
    uint32_t          cmd    = 0;
    spinel_ssize_t    rval   = 0;
    spinel_ssize_t    unpacked;
    otError           error = OT_ERROR_NONE;

    OT_UNUSED_VARIABLE(aHeader);

    rval = spinel_datatype_unpack(aFrame, aLength, "CiiD", &header, &cmd, &key, &data, &len);
    EXPECT(rval > 0 && cmd >= SPINEL_CMD_PROP_VALUE_IS && cmd <= SPINEL_CMD_PROP_VALUE_REMOVED, error = OT_ERROR_PARSE);

    EXPECT(cmd == SPINEL_CMD_PROP_VALUE_IS, error = OT_ERROR_DROP);

    if (key == SPINEL_PROP_LAST_STATUS)
    {
        spinel_status_t status = SPINEL_STATUS_OK;

        unpacked = spinel_datatype_unpack(data, len, "i", &status);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

        if (status >= SPINEL_STATUS_RESET__BEGIN && status <= SPINEL_STATUS_RESET__END)
        {
            // this clear is necessary in case the RCP has sent messages between disable and reset
            mRxFrameBuffer.Clear();

            LogInfo("co-processor reset: %s", spinel_status_to_cstr(status));
            mIsCoprocessorReady = true;
        }
        else
        {
            LogInfo("co-processor last status: %s", spinel_status_to_cstr(status));
            EXIT_NOW();
        }
    }
    else
    {
        // Drop other frames when the key isn't waiting key.
        EXPECT(mWaitingKey == key, error = OT_ERROR_DROP);

        if (key == SPINEL_PROP_PROTOCOL_VERSION)
        {
            unpacked = spinel_datatype_unpack(data, len, (SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S),
                                              &mSpinelVersionMajor, &mSpinelVersionMinor);

            EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
        }
        else if (key == SPINEL_PROP_NCP_VERSION)
        {
            unpacked = spinel_datatype_unpack_in_place(data, len, SPINEL_DATATYPE_UTF8_S, mVersion, sizeof(mVersion));

            EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
        }
        else if (key == SPINEL_PROP_CAPS)
        {
            uint8_t        capsBuffer[kCapsBufferSize];
            spinel_size_t  capsLength = sizeof(capsBuffer);
            const uint8_t *capsData   = capsBuffer;

            unpacked = spinel_datatype_unpack_in_place(data, len, SPINEL_DATATYPE_DATA_S, capsBuffer, &capsLength);

            EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

            while (capsLength > 0)
            {
                unsigned int capability;

                unpacked = spinel_datatype_unpack(capsData, capsLength, SPINEL_DATATYPE_UINT_PACKED_S, &capability);
                EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

                EXPECT_NO_ERROR(error = mCoprocessorCaps.PushBack(capability));

                capsData += unpacked;
                capsLength -= static_cast<spinel_size_t>(unpacked);
            }
        }

        mIsWaitingForResponse = false;
    }

exit:
    aSave = false;
    LogIfFail("Error processing frame", error);
}

otError SpinelDriver::CheckSpinelVersion(void)
{
    otError error = OT_ERROR_NONE;

    EXPECT_NO_ERROR(error = SendCommand(SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PROTOCOL_VERSION, sTid));
    mIsWaitingForResponse = true;
    mWaitingKey           = SPINEL_PROP_PROTOCOL_VERSION;

    EXPECT_NO_ERROR(error = WaitResponse());

    if ((mSpinelVersionMajor != SPINEL_PROTOCOL_VERSION_THREAD_MAJOR) ||
        (mSpinelVersionMinor != SPINEL_PROTOCOL_VERSION_THREAD_MINOR))
    {
        LogCrit("Spinel version mismatch - Posix:%d.%d, co-processor:%d.%d", SPINEL_PROTOCOL_VERSION_THREAD_MAJOR,
                SPINEL_PROTOCOL_VERSION_THREAD_MINOR, mSpinelVersionMajor, mSpinelVersionMinor);
        DieNow(OT_EXIT_RADIO_SPINEL_INCOMPATIBLE);
    }

exit:
    return error;
}

otError SpinelDriver::GetCoprocessorVersion(void)
{
    otError error = OT_ERROR_NONE;

    EXPECT_NO_ERROR(error = SendCommand(SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_NCP_VERSION, sTid));
    mIsWaitingForResponse = true;
    mWaitingKey           = SPINEL_PROP_NCP_VERSION;

    EXPECT_NO_ERROR(error = WaitResponse());
exit:
    return error;
}

otError SpinelDriver::GetCoprocessorCaps(void)
{
    otError error = OT_ERROR_NONE;

    EXPECT_NO_ERROR(error = SendCommand(SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_CAPS, sTid));
    mIsWaitingForResponse = true;
    mWaitingKey           = SPINEL_PROP_CAPS;

    EXPECT_NO_ERROR(error = WaitResponse());
exit:
    return error;
}

CoprocessorType SpinelDriver::GetCoprocessorType(void)
{
    CoprocessorType type = OT_COPROCESSOR_UNKNOWN;

    if (CoprocessorHasCap(SPINEL_CAP_CONFIG_RADIO))
    {
        type = OT_COPROCESSOR_RCP;
    }
    else if (CoprocessorHasCap(SPINEL_CAP_CONFIG_FTD) || CoprocessorHasCap(SPINEL_CAP_CONFIG_MTD))
    {
        type = OT_COPROCESSOR_NCP;
    }

    return type;
}

void SpinelDriver::ProcessFrameQueue(void)
{
    uint8_t *frame = nullptr;
    uint16_t length;

    assert(mSavedFrameHandler != nullptr && mFrameHandlerContext != nullptr);

    while (mRxFrameBuffer.GetNextSavedFrame(frame, length) == OT_ERROR_NONE)
    {
        mSavedFrameHandler(frame, length, mFrameHandlerContext);
    }

    mRxFrameBuffer.ClearSavedFrames();
}

} // namespace Spinel
} // namespace ot
