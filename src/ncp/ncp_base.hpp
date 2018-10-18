/*
 *    Copyright (c) 2016, The OpenThread Authors.
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
 *   This file contains definitions a spinel interface to the OpenThread stack.
 */

#ifndef NCP_BASE_HPP_
#define NCP_BASE_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_MTD || OPENTHREAD_FTD
#include <openthread/ip6.h>
#else
#include <openthread/platform/radio.h>
#endif
#if OPENTHREAD_FTD
#include <openthread/thread_ftd.h>
#endif
#include <openthread/dhcp6_client.h>
#include <openthread/message.h>
#include <openthread/ncp.h>

#include "changed_props_set.hpp"
#include "common/instance.hpp"
#include "common/tasklet.hpp"
#include "ncp/ncp_buffer.hpp"
#include "ncp/spinel_decoder.hpp"
#include "ncp/spinel_encoder.hpp"

#include "spinel.h"

namespace ot {
namespace Ncp {

class NcpBase
{
public:
    /**
     * This constructor creates and initializes an NcpBase instance.
     *
     * @param[in]  aInstance  The OpenThread instance structure.
     *
     */
    NcpBase(Instance *aInstance);

    /**
     * This static method returns the pointer to the single NCP instance.
     *
     * @returns Pointer to the single NCP instance.
     *
     */
    static NcpBase *GetNcpInstance(void);

    /**
     * This method sends data to host via specific stream.
     *
     *
     * @param[in]  aStreamId  A numeric identifier for the stream to write to.
     *                        If set to '0', will default to the debug stream.
     * @param[in]  aDataPtr   A pointer to the data to send on the stream.
     *                        If aDataLen is non-zero, this param MUST NOT be NULL.
     * @param[in]  aDataLen   The number of bytes of data from aDataPtr to send.
     *
     * @retval OT_ERROR_NONE         The data was queued for delivery to the host.
     * @retval OT_ERROR_BUSY         There are not enough resources to complete this
     *                               request. This is usually a temporary condition.
     * @retval OT_ERROR_INVALID_ARGS The given aStreamId was invalid.
     *
     */
    otError StreamWrite(int aStreamId, const uint8_t *aDataPtr, int aDataLen);

    /**
     * This method send an OpenThread log message to host via `SPINEL_PROP_STREAM_LOG` property.
     *
     * @param[in] aLogLevel   The log level
     * @param[in] aLogRegion  The log region
     * @param[in] aLogString  The log string
     *
     */
    void Log(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aLogString);

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    /**
     * This method registers peek/poke delegate functions with NCP module.
     *
     * @param[in] aAllowPeekDelegate      Delegate function pointer for peek operation.
     * @param[in] aAllowPokeDelegate      Delegate function pointer for poke operation.
     *
     * @retval OT_ERROR_NONE              Successfully registered delegate functions.
     * @retval OT_ERROR_DISABLED_FEATURE  Peek/Poke feature is disabled (by a build-time configuration option).
     *
     */
    void RegisterPeekPokeDelagates(otNcpDelegateAllowPeekPoke aAllowPeekDelegate,
                                   otNcpDelegateAllowPeekPoke aAllowPokeDelegate);
#endif

#if OPENTHREAD_MTD || OPENTHREAD_FTD
#if OPENTHREAD_ENABLE_LEGACY
    /**
     * This callback is invoked by the legacy stack to notify that a new
     * legacy node did join the network.
     *
     * @param[in]   aExtAddr    The extended address of the joined node.
     *
     */
    void HandleLegacyNodeDidJoin(const otExtAddress *aExtAddr);

    /**
     * This callback is invoked by the legacy stack to notify that the
     * legacy ULA prefix has changed.
     *
     * param[in]    aUlaPrefix  The changed ULA prefix.
     *
     */
    void HandleDidReceiveNewLegacyUlaPrefix(const uint8_t *aUlaPrefix);

    /**
     * This method registers a set of legacy handlers with NCP.
     *
     * @param[in] aHandlers    A pointer to a handler struct.
     *
     */
    void RegisterLegacyHandlers(const otNcpLegacyHandlers *aHandlers);
#endif
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

    /**
     * This method is called by the framer whenever a framing error is detected.
     */
    void IncrementFrameErrorCounter(void);

    /**
     * Called by the subclass to indicate when a frame has been received.
     */
    void HandleReceive(const uint8_t *aBuf, uint16_t aBufLength);

    /**
     * Called by the subclass to learn when the host wake operation must be issued.
     */
    bool ShouldWakeHost(void);

    /**
     * Called by the subclass to learn when the transfer to the host should be deferred.
     */
    bool ShouldDeferHostSend(void);

protected:
    typedef otError (NcpBase::*PropertyHandler)(void);

    /**
     * This struct represents a spinel response entry.
     *
     * It can be either a `VALUE_IS` property update for a specific property, or a `VALUE_IS(LAST_STATUS)` with a given
     * spinel status error.
     *
     */
    struct ResponseEntry
    {
        uint8_t  mTid : 4;              ///< Spinel transaction id.
        bool     mIsInUse : 1;          ///< `true` if this entry is in use, `false` otherwise.
        bool     mIsLastStatus : 1;     ///< `true` if entry is a `LAST_STATUS`, otherwise it is a property update.
        bool     mIsGetResponse : 1;    ///< `true` if response is for `VALUE_GET`, 'false` otherwise.
        uint32_t mPropKeyOrStatus : 24; ///< 3 bytes for either property key or spinel status.
    };

    NcpFrameBuffer::FrameTag GetLastOutboundFrameTag(void);

    otError HandleCommand(uint8_t aHeader);

    PropertyHandler FindGetPropertyHandler(spinel_prop_key_t aKey);
    PropertyHandler FindSetPropertyHandler(spinel_prop_key_t aKey);
    PropertyHandler FindInsertPropertyHandler(spinel_prop_key_t aKey);
    PropertyHandler FindRemovePropertyHandler(spinel_prop_key_t aKey);

    bool    HandlePropertySetForSpecialProperties(uint8_t aHeader, spinel_prop_key_t aKey, otError &aError);
    otError HandleCommandPropertySet(uint8_t aHeader, spinel_prop_key_t aKey);
    otError HandleCommandPropertyInsertRemove(uint8_t aHeader, spinel_prop_key_t aKey, unsigned int aCommand);

    otError WriteLastStatusFrame(uint8_t aHeader, spinel_status_t aLastStatus);
    otError WritePropertyValueIsFrame(uint8_t aHeader, spinel_prop_key_t aPropKey, bool aIsGetResponse = true);
    otError WritePropertyValueInsertedRemovedFrame(uint8_t           aHeader,
                                                   unsigned int      aResponseCommand,
                                                   spinel_prop_key_t aPropKey,
                                                   const uint8_t *   aValuePtr,
                                                   uint16_t          aValueLen);

    otError SendQueuedResponses(void);
    bool    IsResponseQueueEmpty(void) { return (mResponseQueueHead == mResponseQueueTail); }
    otError PrepareResponse(uint8_t aHeader, bool aIsLastStatus, bool aIsGetResponse, unsigned int aPropKeyOrStatus);
    otError PrepareGetResponse(uint8_t aHeader, spinel_prop_key_t aPropKey)
    {
        return PrepareResponse(aHeader, false /* aIsLastStatus */, true /* aIsGetResponse*/, aPropKey);
    }
    otError PrepareSetResponse(uint8_t aHeader, spinel_prop_key_t aPropKey)
    {
        return PrepareResponse(aHeader, false /* aIsLastStatus */, false /* aIsGetResponse*/, aPropKey);
    }
    otError PrepareLastStatusResponse(uint8_t aHeader, spinel_status_t aStatus)
    {
        return PrepareResponse(aHeader, true /*aIsLastStatus */, false, aStatus);
    }
    static uint8_t GetWrappedResponseQueueIndex(uint8_t aPosition);

    static void UpdateChangedProps(Tasklet &aTasklet);
    void        UpdateChangedProps(void);

    static void HandleFrameRemovedFromNcpBuffer(void *                   aContext,
                                                NcpFrameBuffer::FrameTag aFrameTag,
                                                NcpFrameBuffer::Priority aPriority,
                                                NcpFrameBuffer *         aNcpBuffer);
    void        HandleFrameRemovedFromNcpBuffer(NcpFrameBuffer::FrameTag aFrameTag);

    static void HandleRawFrame(const otRadioFrame *aFrame, void *aContext);
    void        HandleRawFrame(const otRadioFrame *aFrame);

    otError EncodeChannelMask(uint32_t aChannelMask);
    otError DecodeChannelMask(uint32_t &aChannelMask);

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

    static void LinkRawReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError);
    void        LinkRawReceiveDone(otRadioFrame *aFrame, otError aError);

    static void LinkRawTransmitDone(otInstance *  aInstance,
                                    otRadioFrame *aFrame,
                                    otRadioFrame *aAckFrame,
                                    otError       aError);
    void        LinkRawTransmitDone(otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError);

    static void LinkRawEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi);
    void        LinkRawEnergyScanDone(int8_t aEnergyScanMaxRssi);

#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    static void HandleStateChanged(otChangedFlags aFlags, void *aContext);
    void        ProcessThreadChangedFlags(void);

#if OPENTHREAD_FTD
    static void HandleChildTableChanged(otThreadChildTableEvent aEvent, const otChildInfo *aChildInfo);
    void        HandleChildTableChanged(otThreadChildTableEvent aEvent, const otChildInfo &aChildInfo);

    static void HandleParentResponseInfo(otThreadParentResponseInfo *aInfo, void *aContext);
    void        HandleParentResponseInfo(const otThreadParentResponseInfo *aInfo);
#endif

    static void HandleDatagramFromStack(otMessage *aMessage, void *aContext);
    void        HandleDatagramFromStack(otMessage *aMessage);

    otError SendQueuedDatagramMessages(void);
    otError SendDatagramMessage(otMessage *aMessage);

    static void HandleActiveScanResult_Jump(otActiveScanResult *aResult, void *aContext);
    void        HandleActiveScanResult(otActiveScanResult *aResult);

    static void HandleEnergyScanResult_Jump(otEnergyScanResult *aResult, void *aContext);
    void        HandleEnergyScanResult(otEnergyScanResult *aResult);

    static void HandleJamStateChange_Jump(bool aJamState, void *aContext);
    void        HandleJamStateChange(bool aJamState);

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    static void HandleCommissionerEnergyReport_Jump(uint32_t       aChannelMask,
                                                    const uint8_t *aEnergyData,
                                                    uint8_t        aLength,
                                                    void *         aContext);
    void        HandleCommissionerEnergyReport(uint32_t aChannelMask, const uint8_t *aEnergyData, uint8_t aLength);

    static void HandleCommissionerPanIdConflict_Jump(uint16_t aPanId, uint32_t aChannelMask, void *aContext);
    void        HandleCommissionerPanIdConflict(uint16_t aPanId, uint32_t aChannelMask);
#endif

#if OPENTHREAD_ENABLE_JOINER
    static void HandleJoinerCallback_Jump(otError aError, void *aContext);
    void        HandleJoinerCallback(otError aError);
#endif

    static void SendDoneTask(void *aContext);
    void        SendDoneTask(void);

    otError EncodeOperationalDataset(const otOperationalDataset &aDataset);

#if OPENTHREAD_FTD
    otError EncodeChildInfo(const otChildInfo &aChildInfo);
    otError DecodeOperationalDataset(otOperationalDataset &aDataset,
                                     const uint8_t **      aTlvs             = NULL,
                                     uint8_t *             aTlvsLength       = NULL,
                                     const otIp6Address ** aDestIpAddress    = NULL,
                                     bool                  aAllowEmptyValues = false);
#endif

#if OPENTHREAD_ENABLE_UDP_PROXY
    static void HandleUdpProxyStream(otMessage *   aMessage,
                                     uint16_t      aPeerPort,
                                     otIp6Address *aPeerAddr,
                                     uint16_t      aSockPort,
                                     void *        aContext);
    void        HandleUdpProxyStream(otMessage *aMessage, uint16_t aPeerPort, otIp6Address &aPeerAddr, uint16_t aPort);
#endif // OPENTHREAD_ENABLE_UDP_PROXY
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

    otError CommandHandler_NOOP(uint8_t aHeader);
    otError CommandHandler_RESET(uint8_t aHeader);
    // Combined command handler for `VALUE_GET`, `VALUE_SET`, `VALUE_INSERT` and `VALUE_REMOVE`.
    otError CommandHandler_PROP_VALUE_update(uint8_t aHeader, unsigned int aCommand);
#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    otError CommandHandler_PEEK(uint8_t aHeader);
    otError CommandHandler_POKE(uint8_t aHeader);
#endif
#if OPENTHREAD_MTD || OPENTHREAD_FTD
    otError CommandHandler_NET_SAVE(uint8_t aHeader);
    otError CommandHandler_NET_CLEAR(uint8_t aHeader);
    otError CommandHandler_NET_RECALL(uint8_t aHeader);
#endif

    // ----------------------------------------------------------------------------
    // Property Handlers
    // ----------------------------------------------------------------------------
    //
    // There are 4 types of property handlers for "get", "set", "insert", and
    // "remove" commands.
    //
    // "Get" handlers should get/retrieve the property value and then encode and
    // write the value into the NCP buffer. If the "get" operation itself fails,
    // "get" handler should write a `LAST_STATUS` with the error status into the NCP
    // buffer. The `otError` returned from a "get" handler is the error of writing
    // into the NCP buffer (e.g., running out buffer), and not of the "get" operation
    // itself.
    //
    // "Set/Insert/Remove" handlers should first decode/parse the value from the
    // input Spinel frame and then perform the corresponding set/insert/remove
    // operation. They are not responsible for preparing the Spinel response and
    // therefore should not write anything to the NCP buffer. The `otError` returned
    // from a "set/insert/remove" handler indicates the error in either parsing of
    // the input or the error of set/insert/remove operation.
    //
    // The corresponding command handler (e.g., `HandleCommandPropertySet()` for
    // `VALUE_SET` command) will take care of preparing the Spinel response after
    // invoking the "set/insert/remove" handler for a given property. For example,
    // for a `VALUE_SET` command, if the "set" handler returns an error, then a
    // `LAST_STATUS` update response is prepared, otherwise on success the "get"
    // handler for the property is used to prepare a `VALUE_IS` Spinel response (in
    // cases where there is no "get" handler for the property, the input value is
    // echoed in the response).
    //
    // Few properties require special treatment where the response needs to be
    // prepared directly in the  "set"  handler (e.g., `HOST_POWER_STATE` or
    // `NEST_STREAM_MFG`). These properties have a different handler method format
    // (they expect `aHeader` as an input argument) and are processed separately in
    // `HandleCommandPropertySet()`.

    template <spinel_prop_key_t aKey> otError HandlePropertyGet(void);
    template <spinel_prop_key_t aKey> otError HandlePropertySet(void);
    template <spinel_prop_key_t aKey> otError HandlePropertyInsert(void);
    template <spinel_prop_key_t aKey> otError HandlePropertyRemove(void);

    // --------------------------------------------------------------------------
    // Property "set" handlers for special properties for which the spinel
    // response needs to be created from within the set handler.

    otError HandlePropertySet_SPINEL_PROP_HOST_POWER_STATE(uint8_t aHeader);

#if OPENTHREAD_ENABLE_DIAG
    otError HandlePropertySet_SPINEL_PROP_NEST_STREAM_MFG(uint8_t aHeader);
#endif

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    otError HandlePropertySet_SPINEL_PROP_THREAD_COMMISSIONER_ENABLED(uint8_t aHeader);
#endif // OPENTHREAD_FTD

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    otError HandlePropertySet_SPINEL_PROP_STREAM_RAW(uint8_t aHeader);
#endif

#if OPENTHREAD_ENABLE_LEGACY
    void StartLegacy(void);
    void StopLegacy(void);
#else
    void StartLegacy(void) {}
    void StopLegacy(void) {}
#endif

    static uint8_t      ConvertLogLevel(otLogLevel aLogLevel);
    static unsigned int ConvertLogRegion(otLogRegion aLogRegion);

#if OPENTHREAD_ENABLE_NCP_VENDOR_HOOK
    /**
     * This method defines a vendor "command handler" hook to process vendor-specific spinel commands.
     *
     * @param[in] aHeader   The spinel frame header.
     * @param[in] aCommand  The spinel command key.
     *
     * @retval OT_ERROR_NONE     The response is prepared.
     * @retval OT_ERROR_NO_BUFS  Out of buffer while preparing the response.
     *
     */
    otError VendorCommandHandler(uint8_t aHeader, unsigned int aCommand);

    /**
     * This method is a callback which mirrors `NcpBase::HandleFrameRemovedFromNcpBuffer()`. It is called when a
     * spinel frame is sent and removed from NCP buffer.
     *
     * (a) This can be used to track and verify that a vendor spinel frame response is delivered to the host (tracking
     *     the frame using its tag).
     *
     * (b) It indicates that NCP buffer space is now available (since a spinel frame is removed). This can be used to
     *     implement mechanisms to re-send a failed/pending response or an async spinel frame.
     *
     * @param[in] aFrameTag    The tag of the frame removed from NCP buffer.
     *
     */
    void VendorHandleFrameRemovedFromNcpBuffer(NcpFrameBuffer::FrameTag aFrameTag);

    /**
     * This method defines a vendor "get property handler" hook to process vendor spinel properties.
     *
     * The vendor handler should return `OT_ERROR_NOT_FOUND` status if it does not support "get" operation for the
     * given property key. Otherwise, the vendor handler should behave like other property get handlers, i.e., it
     * should retrieve the property value and then encode and write the value into the NCP buffer. If the "get"
     * operation itself fails, handler should write a `LAST_STATUS` with the error status into the NCP buffer.
     *
     * @param[in] aPropKey            The spinel property key.
     *
     * @retval OT_ERROR_NONE          Successfully retrieved the property value and prepared the response.
     * @retval OT_ERROR_NOT_FOUND     Does not support the given property key.
     * @retval OT_ERROR_NO_BUFS       Out of buffer while preparing the response.
     *
     */
    otError VendorGetPropertyHandler(spinel_prop_key_t aPropKey);

    /**
     * This method defines a vendor "set property handler" hook to process vendor spinel properties.
     *
     * The vendor handler should return `OT_ERROR_NOT_FOUND` status if it does not support "set" operation for the
     * given property key. Otherwise, the vendor handler should behave like other property set handlers, i.e., it
     * should first decode the value from the input spinel frame and then perform the corresponding set operation. The
     * handler should not prepare the spinel response and therefore should not write anything to the NCP buffer. The
     * `otError` returned from handler (other than `OT_ERROR_NOT_FOUND`) indicates the error in either parsing of the
     * input or the error of the set operation. In case of a successful "set", `NcpBase` set command handler will call
     * the `VendorGetPropertyHandler()` for the same property key to prepare the response.
     *
     * @param[in] aPropKey  The spinel property key.
     *
     * @returns OT_ERROR_NOT_FOUND if it does not support the given property key, otherwise the error in either parsing
     *          of the input or the "set" operation.
     *
     */
    otError VendorSetPropertyHandler(spinel_prop_key_t aPropKey);

#endif // OPENTHREAD_ENABLE_NCP_VENDOR_HOOK

protected:
    static NcpBase *       sNcpInstance;
    static spinel_status_t ThreadErrorToSpinelStatus(otError aError);
    static uint8_t         LinkFlagsToFlagByte(bool aRxOnWhenIdle,
                                               bool aSecureDataRequests,
                                               bool aDeviceType,
                                               bool aNetworkData);
    Instance *             mInstance;
    NcpFrameBuffer         mTxFrameBuffer;
    SpinelEncoder          mEncoder;
    SpinelDecoder          mDecoder;
    bool                   mHostPowerStateInProgress;

    enum
    {
        kTxBufferSize       = OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE, // Tx Buffer size (used by mTxFrameBuffer).
        kResponseQueueSize  = OPENTHREAD_CONFIG_NCP_SPINEL_RESPONSE_QUEUE_SIZE,
        kInvalidScanChannel = -1, // Invalid scan channel.
    };

    spinel_status_t mLastStatus;
    uint32_t        mScanChannelMask;
    uint16_t        mScanPeriod;
    bool            mDiscoveryScanJoinerFlag;
    bool            mDiscoveryScanEnableFiltering;
    uint16_t        mDiscoveryScanPanId;

    Tasklet         mUpdateChangedPropsTask;
    uint32_t        mThreadChangedFlags;
    ChangedPropsSet mChangedPropsSet;

    spinel_host_power_state_t mHostPowerState;
    NcpFrameBuffer::FrameTag  mHostPowerReplyFrameTag;
    uint8_t                   mHostPowerStateHeader;

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    otNcpDelegateAllowPeekPoke mAllowPeekDelegate;
    otNcpDelegateAllowPeekPoke mAllowPokeDelegate;
#endif

    uint8_t mTxBuffer[kTxBufferSize];

    spinel_tid_t mNextExpectedTid;

    uint8_t       mResponseQueueHead;
    uint8_t       mResponseQueueTail;
    ResponseEntry mResponseQueue[kResponseQueueSize];

    bool mAllowLocalNetworkDataChange;
    bool mRequireJoinExistingNetwork;
    bool mIsRawStreamEnabled;
    bool mDisableStreamWrite;
    bool mShouldEmitChildTableUpdate;

#if OPENTHREAD_ENABLE_DHCP6_CLIENT
    otDhcpAddress mDhcpAddresses[OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES];
#endif

#if OPENTHREAD_FTD
#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    otExtAddress mSteeringDataAddress;
#endif
    uint8_t mPreferredRouteId;
#endif

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    uint8_t mCurTransmitTID;
    int8_t  mCurScanChannel;
    bool    mSrcMatchEnabled;
#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    otMessageQueue mMessageQueue;

    uint32_t mInboundSecureIpFrameCounter;    // Number of secure inbound data/IP frames.
    uint32_t mInboundInsecureIpFrameCounter;  // Number of insecure inbound data/IP frames.
    uint32_t mOutboundSecureIpFrameCounter;   // Number of secure outbound data/IP frames.
    uint32_t mOutboundInsecureIpFrameCounter; // Number of insecure outbound data/IP frames.
    uint32_t mDroppedOutboundIpFrameCounter;  // Number of dropped outbound data/IP frames.
    uint32_t mDroppedInboundIpFrameCounter;   // Number of dropped inbound data/IP frames.
#endif                                        // OPENTHREAD_MTD || OPENTHREAD_FTD
    uint32_t mFramingErrorCounter;            // Number of improperly formed received spinel frames.
    uint32_t mRxSpinelFrameCounter;           // Number of received (inbound) spinel frames.
    uint32_t mRxSpinelOutOfOrderTidCounter;   // Number of out of order received spinel frames (tid increase > 1).
    uint32_t mTxSpinelFrameCounter;           // Number of sent (outbound) spinel frames.

#if OPENTHREAD_ENABLE_LEGACY
    const otNcpLegacyHandlers *mLegacyHandlers;
    uint8_t                    mLegacyUlaPrefix[OT_NCP_LEGACY_ULA_PREFIX_LENGTH];
    otExtAddress               mLegacyLastJoinedNode;
    bool                       mLegacyNodeDidJoin;
#endif

    bool mDidInitialUpdates;
};

} // namespace Ncp
} // namespace ot

#endif // NCP_BASE_HPP_
