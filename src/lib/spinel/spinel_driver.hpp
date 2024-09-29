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

#ifndef SPINEL_DRIVER_HPP_
#define SPINEL_DRIVER_HPP_

#include <openthread/instance.h>

#include "lib/spinel/coprocessor_type.h"
#include "lib/spinel/logger.hpp"
#include "lib/spinel/spinel.h"
#include "lib/spinel/spinel_interface.hpp"

/**
 * Represents an opaque (and empty) type corresponding to a SpinelDriver object.
 */
struct otSpinelDriver
{
};

namespace ot {
namespace Spinel {

/**
 * Maximum number of Spinel Interface IDs.
 */
#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
static constexpr uint8_t kSpinelHeaderMaxNumIid = 4;
#else
static constexpr uint8_t kSpinelHeaderMaxNumIid = 1;
#endif

class SpinelDriver : public otSpinelDriver, public Logger
{
public:
    typedef void (
        *ReceivedFrameHandler)(const uint8_t *aFrame, uint16_t aLength, uint8_t aHeader, bool &aSave, void *aContext);
    typedef void (*SavedFrameHandler)(const uint8_t *aFrame, uint16_t aLength, void *aContext);

    /**
     * Constructor of the SpinelDriver.
     */
    SpinelDriver(void);

    /**
     * Initialize this SpinelDriver Instance.
     *
     * @param[in]  aSpinelInterface            A reference to the Spinel interface.
     * @param[in]  aSoftwareReset              TRUE to reset on init, FALSE to not reset on init.
     * @param[in]  aIidList                    A Pointer to the list of IIDs to receive spinel frame from.
     *                                         First entry must be the IID of the Host Application.
     * @param[in]  aIidListLength              The Length of the @p aIidList.
     *
     * @retval  OT_COPROCESSOR_UNKNOWN  The initialization fails.
     * @retval  OT_COPROCESSOR_RCP      The Co-processor is a RCP.
     * @retval  OT_COPROCESSOR_NCP      The Co-processor is a NCP.
     */
    CoprocessorType Init(SpinelInterface    &aSpinelInterface,
                         bool                aSoftwareReset,
                         const spinel_iid_t *aIidList,
                         uint8_t             aIidListLength);

    /**
     * Deinitialize this SpinelDriver Instance.
     */
    void Deinit(void);

    /**
     * Clear the rx frame buffer.
     */
    void ClearRxBuffer(void) { mRxFrameBuffer.Clear(); }

    /**
     * Set the internal state of co-processor as ready.
     *
     * This method is used to skip a reset.
     */
    void SetCoprocessorReady(void) { mIsCoprocessorReady = true; }

    /**
     * Send a reset command to the co-processor.
     *
     * @prarm[in] aResetType    The reset type, SPINEL_RESET_PLATFORM, SPINEL_RESET_STACK, or SPINEL_RESET_BOOTLOADER.
     *
     * @retval  OT_ERROR_NONE               Successfully removed item from the property.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     */
    otError SendReset(uint8_t aResetType);

    /**
     * Reset the co-processor.
     *
     * This method will reset the co-processor and wait until the co-process is ready (receiving SPINEL_PROP_LAST_STATUS
     * from the it). The reset will be either a software or hardware reset. If `aSoftwareReset` is `true`, then the
     * method will first try a software reset. If the software reset succeeds, the method exits. Otherwise the method
     * will then try a hardware reset. If `aSoftwareReset` is `false`, then method will directly try a hardware reset.
     *
     * @param[in]  aSoftwareReset                 TRUE to try SW reset first, FALSE to directly try HW reset.
     */
    void ResetCoprocessor(bool aSoftwareReset);

    /**
     * Processes any pending the I/O data.
     *
     * The method should be called by the system loop to process received spinel frames.
     *
     * @param[in]  aContext   The process context.
     */
    void Process(const void *aContext);

    /**
     * Checks whether there is pending frame in the buffer.
     *
     * The method is required by the system loop to update timer fd.
     *
     * @returns Whether there is pending frame in the buffer.
     */
    bool HasPendingFrame(void) const { return mRxFrameBuffer.HasSavedFrame(); }

    /**
     * Returns the co-processor sw version string.
     *
     * @returns A pointer to the co-processor version string.
     */
    const char *GetVersion(void) const { return mVersion; }

    /*
     * Sends a spinel command to the co-processor.
     *
     * @param[in] aCommand    The spinel command.
     * @param[in] aKey        The spinel property key.
     * @param[in] aTid        The spinel transaction id.
     * @param[in] aFormat     The format string of the arguments to send.
     * @param[in] aArgs       The argument list.
     *
     * @retval  OT_ERROR_NONE           Successfully sent the command through spinel interface.
     * @retval  OT_ERROR_INVALID_STATE  The spinel interface is in an invalid state.
     * @retval  OT_ERROR_NO_BUFS        The spinel interface doesn't have enough buffer.
     */
    otError SendCommand(uint32_t          aCommand,
                        spinel_prop_key_t aKey,
                        spinel_tid_t      aTid,
                        const char       *aFormat,
                        va_list           aArgs);

    /*
     * Sends a spinel command without arguments to the co-processor.
     *
     * @param[in] aCommand    The spinel command.
     * @param[in] aKey        The spinel property key.
     * @param[in] aTid        The spinel transaction id.
     *
     * @retval  OT_ERROR_NONE           Successfully sent the command through spinel interface.
     * @retval  OT_ERROR_INVALID_STATE  The spinel interface is in an invalid state.
     * @retval  OT_ERROR_NO_BUFS        The spinel interface doesn't have enough buffer.
     */
    otError SendCommand(uint32_t aCommand, spinel_prop_key_t aKey, spinel_tid_t aTid);

    /*
     * Sets the handler to process the received spinel frame.
     *
     * @param[in] aReceivedFrameHandler  The handler to process received spinel frames.
     * @param[in] aSavedFrameHandler     The handler to process saved spinel frames.
     * @param[in] aContext               The context to call the handler.
     */
    void SetFrameHandler(ReceivedFrameHandler aReceivedFrameHandler,
                         SavedFrameHandler    aSavedFrameHandler,
                         void                *aContext);

    /*
     * Returns the spinel interface.
     *
     * @returns A pointer to the spinel interface object.
     */
    SpinelInterface *GetSpinelInterface(void) const { return mSpinelInterface; }

    /**
     * Returns if the co-processor has some capability
     *
     * @param[in] aCapability  The capability queried.
     *
     * @returns `true` if the co-processor has the capability. `false` otherwise.
     */
    bool CoprocessorHasCap(unsigned int aCapability) { return mCoprocessorCaps.Contains(aCapability); }

    /**
     * Returns the spinel interface id.
     *
     * @returns the spinel interface id.
     */
    spinel_iid_t GetIid(void) { return mIid; }

private:
    static constexpr uint16_t kMaxSpinelFrame    = SPINEL_FRAME_MAX_SIZE;
    static constexpr uint16_t kVersionStringSize = 128;
    static constexpr uint32_t kUsPerMs           = 1000; ///< Microseconds per millisecond.
    static constexpr uint32_t kMaxWaitTime       = 2000; ///< Max time to wait for response in milliseconds.
    static constexpr uint16_t kCapsBufferSize    = 100;  ///< Max buffer size used to store `SPINEL_PROP_CAPS` value.

    /**
     * Represents an array of elements with a fixed max size.
     *
     * @tparam Type        The array element type.
     * @tparam kMaxSize    Specifies the max array size (maximum number of elements in the array).
     */
    template <typename Type, uint16_t kMaxSize> class Array
    {
        static_assert(kMaxSize != 0, "Array `kMaxSize` cannot be zero");

    public:
        Array(void)
            : mLength(0)
        {
        }

        uint16_t GetMaxSize(void) const { return kMaxSize; }

        bool IsFull(void) const { return (mLength == GetMaxSize()); }

        otError PushBack(const Type &aEntry)
        {
            return IsFull() ? OT_ERROR_NO_BUFS : (mElements[mLength++] = aEntry, OT_ERROR_NONE);
        }

        const Type *Find(const Type &aEntry) const
        {
            const Type *matched = nullptr;

            for (const Type &element : *this)
            {
                if (element == aEntry)
                {
                    matched = &element;
                    break;
                }
            }

            return matched;
        }

        bool Contains(const Type &aEntry) const { return Find(aEntry) != nullptr; }

        Type       *begin(void) { return &mElements[0]; }
        Type       *end(void) { return &mElements[mLength]; }
        const Type *begin(void) const { return &mElements[0]; }
        const Type *end(void) const { return &mElements[mLength]; }

    private:
        Type     mElements[kMaxSize];
        uint16_t mLength;
    };

    otError WaitResponse(void);

    static void HandleReceivedFrame(void *aContext);
    void        HandleReceivedFrame(void);

    static void HandleInitialFrame(const uint8_t *aFrame,
                                   uint16_t       aLength,
                                   uint8_t        aHeader,
                                   bool          &aSave,
                                   void          *aContext);
    void        HandleInitialFrame(const uint8_t *aFrame, uint16_t aLength, uint8_t aHeader, bool &aSave);

    otError         CheckSpinelVersion(void);
    otError         GetCoprocessorVersion(void);
    otError         GetCoprocessorCaps(void);
    CoprocessorType GetCoprocessorType(void);

    void ProcessFrameQueue(void);

    SpinelInterface::RxFrameBuffer mRxFrameBuffer;
    SpinelInterface               *mSpinelInterface;

    spinel_prop_key_t mWaitingKey; ///< The property key of current transaction.
    bool              mIsWaitingForResponse;

    spinel_iid_t                                mIid;
    Array<spinel_iid_t, kSpinelHeaderMaxNumIid> mIidList;

    ReceivedFrameHandler mReceivedFrameHandler;
    SavedFrameHandler    mSavedFrameHandler;
    void                *mFrameHandlerContext;

    int mSpinelVersionMajor;
    int mSpinelVersionMinor;

    bool mIsCoprocessorReady;
    char mVersion[kVersionStringSize];

    Array<unsigned int, kCapsBufferSize> mCoprocessorCaps;
};

} // namespace Spinel
} // namespace ot

#endif // SPINEL_DRIVER_HPP_
