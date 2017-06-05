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

#include <openthread/openthread_enable_defines.h>

#include <openthread/ip6.h>
#include <openthread/message.h>
#include <openthread/ncp.h>
#include <openthread/types.h>

#include "openthread-core-config.h"
#include "spinel.h"
#include "common/tasklet.hpp"
#include "ncp/ncp_buffer.hpp"

namespace ot {

class NcpBase
{
public:

    /**
     * This constructor creates and initializes an NcpBase instance.
     *
     * @param[in]  aInstance  The OpenThread instance structure.
     *
     */
    NcpBase(otInstance *aInstance);

    /**
     * This static method returns the pointer to the single NCP instance.
     *
     @returns Pointer to the single NCP instance.
     *
     */
    static NcpBase *GetNcpInstance(void);

protected:

    /**
     * This method is called to start a new outbound frame.
     *
     * @retval OT_ERROR_NONE       Successfully started a new frame.
     * @retval OT_ERROR_NO_BUFS    Insufficient buffer space available to start a new frame.
     *
     */
    otError OutboundFrameBegin(void);

    /**
     * This method adds data to the current outbound frame being written.
     *
     * If no buffer space is available, this method should discard and clear the frame before returning an error status.
     *
     * @param[in]  aDataBuffer        A pointer to data buffer.
     * @param[in]  aDataBufferLength  The length of the data buffer.
     *
     * @retval OT_ERROR_NONE       Successfully added new data to the frame.
     * @retval OT_ERROR_NO_BUFS    Insufficient buffer space available to add data.
     *
     */
    otError OutboundFrameFeedData(const uint8_t *aDataBuffer, uint16_t aDataBufferLength);

    /**
     * This method adds a message to the current outbound frame being written.
     *
     * If no buffer space is available, this method should discard and clear the frame before returning an error status.
     * In case of success, the passed-in message @aMessage should be owned by outbound buffer and should be freed
     * when either the the frame is successfully sent and removed or if the frame is discarded.
     *
     * @param[in]  aMessage         A reference to the message to be added to current frame.
     *
     * @retval OT_ERROR_NONE     Successfully added the message to the frame.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space available to add message.
     *
     */
    otError OutboundFrameFeedMessage(otMessage *aMessage);

    /**
     * This method finalizes and sends the current outbound frame
     *
     * If no buffer space is available, this method should discard and clear the frame before returning an error status.
     *
     * @retval OT_ERROR_NONE     Successfully added the message to the frame.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space available to add message.
     *
     */
    otError OutboundFrameEnd(void);

    /**
     * This method is called by the framer whenever a framing error
     * is detected.
     */
    void IncrementFrameErrorCounter(void);

protected:

    /**
     * Called by the subclass to indicate when a frame has been received.
     */
    void HandleReceive(const uint8_t *buf, uint16_t bufLength);

    /**
     * Called by the subclass to learn when the host wake operation must be issued.
     */
    bool ShouldWakeHost(void);

    /**
     * Called by the subclass to learn when the transfer to the host should be deferred.
     */
    bool ShouldDeferHostSend(void);

private:

    otError OutboundFrameSend(void);

    NcpFrameBuffer::FrameTag GetLastOutboundFrameTag(void);

#if OPENTHREAD_ENABLE_BORDER_AGENT_PROXY && OPENTHREAD_FTD
    /**
     * Trampoline for HandleBorderAgentProxyStream().
     */
    static void HandleBorderAgentProxyStream(otMessage *aMessage, uint16_t aLocator, uint16_t aPort, void *aContext);

    void HandleBorderAgentProxyStream(otMessage *aMessage, uint16_t aLocator, uint16_t aPort);
#endif // OPENTHREAD_ENABLE_BORDER_AGENT_PROXY && OPENTHREAD_FTD

    /**
     * Trampoline for NcpFrameBuffer FrameRemoved Callback.
     */
    static void HandleFrameRemovedFromNcpBuffer(void *aContext, NcpFrameBuffer::FrameTag aFrameTag,
                                                NcpFrameBuffer *aNcpBuffer);

    void HandleFrameRemovedFromNcpBuffer(NcpFrameBuffer::FrameTag aFrameTag);

    /**
     * Trampoline for HandleDatagramFromStack().
     */
    static void HandleDatagramFromStack(otMessage *aMessage, void *aContext);

    void HandleDatagramFromStack(otMessage *aMessage);

    /**
     * Trampoline for HandleRawFrame().
     */
    static void HandleRawFrame(const otRadioFrame *aFrame, void *aContext);

    void HandleRawFrame(const otRadioFrame *aFrame);

    /**
     * Trampoline for HandleActiveScanResult().
     */
    static void HandleActiveScanResult_Jump(otActiveScanResult *result, void *aContext);

    void HandleActiveScanResult(otActiveScanResult *result);

    /**
     * Trampoline for HandleEnergyScanResult().
     */
    static void HandleEnergyScanResult_Jump(otEnergyScanResult *aResult, void *aContext);

    void HandleEnergyScanResult(otEnergyScanResult *result);

    /**
     * Trampoline for HandleJamStateChange().
     */
    static void HandleJamStateChange_Jump(bool aJamState, void *aContext);

    void HandleJamStateChange(bool aJamState);

    /**
     * Trampoline for UpdateChangedProps().
     */
    static void UpdateChangedProps(void *context);

    void UpdateChangedProps(void);

    /**
     * Trampoline for SendDoneTask().
     */
    static void SendDoneTask(void *context);

    void SendDoneTask(void);

#if OPENTHREAD_ENABLE_RAW_LINK_API

    /**
     * Trampoline for LinkRawReceiveDone().
     */
    static void LinkRawReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError);

    void LinkRawReceiveDone(otRadioFrame *aFrame, otError aError);

    /**
     * Trampoline for LinkRawTransmitDone().
     */
    static void LinkRawTransmitDone(otInstance *aInstance, otRadioFrame *aFrame, bool aFramePending,
                                    otError aError);

    void LinkRawTransmitDone(otRadioFrame *aFrame, bool aFramePending, otError aError);

    /**
     * Trampoline for LinkRawEnergyScanDone().
     */
    static void LinkRawEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi);

    void LinkRawEnergyScanDone(int8_t aEnergyScanMaxRssi);

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

    static void HandleNetifStateChanged(uint32_t flags, void *context);

private:

    otError OutboundFrameFeedPacked(const char *pack_format, ...);

    otError OutboundFrameFeedVPacked(const char *pack_format, va_list args);

private:

    otError HandleCommand(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);

    otError HandleCommandPropertyGet(uint8_t header, spinel_prop_key_t key);

    otError HandleCommandPropertySet(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                     uint16_t value_len);

    otError HandleCommandPropertyInsert(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                        uint16_t value_len);

    otError HandleCommandPropertyRemove(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                        uint16_t value_len);

    otError SendLastStatus(uint8_t header, spinel_status_t lastStatus);

private:

    otError SendPropertyUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, const uint8_t *value_ptr,
                               uint16_t value_len);

    otError SendPropertyUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, otMessage *message);

    otError SendPropertyUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, const char *format, ...);

private:

    typedef otError(NcpBase::*CommandHandlerType)(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                                  uint16_t arg_len);

    typedef otError(NcpBase::*GetPropertyHandlerType)(uint8_t header, spinel_prop_key_t key);

    typedef otError(NcpBase::*SetPropertyHandlerType)(uint8_t header, spinel_prop_key_t key,
                                                      const uint8_t *value_ptr, uint16_t value_len);

    struct CommandHandlerEntry
    {
        spinel_cid_t mCommand;
        CommandHandlerType mHandler;
    };

    struct GetPropertyHandlerEntry
    {
        spinel_prop_key_t mPropKey;
        GetPropertyHandlerType mHandler;
    };

    struct SetPropertyHandlerEntry
    {
        spinel_prop_key_t mPropKey;
        SetPropertyHandlerType mHandler;
    };

    struct InsertPropertyHandlerEntry
    {
        spinel_prop_key_t mPropKey;
        SetPropertyHandlerType mHandler;
    };

    struct RemovePropertyHandlerEntry
    {
        spinel_prop_key_t mPropKey;
        SetPropertyHandlerType mHandler;
    };

    static const CommandHandlerEntry mCommandHandlerTable[];
    static const GetPropertyHandlerEntry mGetPropertyHandlerTable[];
    static const SetPropertyHandlerEntry mSetPropertyHandlerTable[];
    static const InsertPropertyHandlerEntry mInsertPropertyHandlerTable[];
    static const RemovePropertyHandlerEntry mRemovePropertyHandlerTable[];

    otError CommandHandler_NOOP(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);
    otError CommandHandler_RESET(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);
    otError CommandHandler_PROP_VALUE_GET(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                          uint16_t arg_len);
    otError CommandHandler_PROP_VALUE_SET(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                          uint16_t arg_len);
    otError CommandHandler_PROP_VALUE_INSERT(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                             uint16_t arg_len);
    otError CommandHandler_PROP_VALUE_REMOVE(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                             uint16_t arg_len);
    otError CommandHandler_NET_SAVE(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);
    otError CommandHandler_NET_CLEAR(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                     uint16_t arg_len);
    otError CommandHandler_NET_RECALL(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                      uint16_t arg_len);

    otError GetPropertyHandler_ChannelMaskHelper(uint8_t header, spinel_prop_key_t key, uint32_t channel_mask);

    otError GetPropertyHandler_LAST_STATUS(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_PROTOCOL_VERSION(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_INTERFACE_TYPE(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_VENDOR_ID(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_CAPS(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NCP_VERSION(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_INTERFACE_COUNT(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_POWER_STATE(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_HWADDR(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_LOCK(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_HOST_POWER_STATE(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_PHY_ENABLED(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_PHY_FREQ(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_PHY_CHAN_SUPPORTED(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_PHY_CHAN(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_PHY_RSSI(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_PHY_TX_POWER(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_PHY_RX_SENSITIVITY(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_MAC_SCAN_STATE(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_MAC_15_4_PANID(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_MAC_15_4_LADDR(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_MAC_15_4_SADDR(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_MAC_RAW_STREAM_ENABLED(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_MAC_EXTENDED_ADDR(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NET_SAVED(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NET_IF_UP(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NET_STACK_UP(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NET_ROLE(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NET_NETWORK_NAME(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NET_XPANID(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NET_MASTER_KEY(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NET_KEY_SEQUENCE_COUNTER(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NET_PARTITION_ID(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NET_KEY_SWITCH_GUARDTIME(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_LEADER(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_IPV6_ML_PREFIX(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_IPV6_ML_ADDR(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_IPV6_LL_ADDR(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_IPV6_ROUTE_TABLE(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_IPV6_ICMP_PING_OFFLOAD(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_RLOC16_DEBUG_PASSTHRU(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_OFF_MESH_ROUTES(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_STREAM_NET(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_MAC_SCAN_MASK(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_MAC_SCAN_PERIOD(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_LEADER_ADDR(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_PARENT(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_NEIGHBOR_TABLE(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_LEADER_RID(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key);
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    otError GetPropertyHandler_THREAD_NETWORK_DATA(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_STABLE_NETWORK_DATA(uint8_t header, spinel_prop_key_t key);
#endif
    otError GetPropertyHandler_THREAD_NETWORK_DATA_VERSION(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_STABLE_NETWORK_DATA_VERSION(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_LEADER_NETWORK_DATA(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_STABLE_LEADER_NETWORK_DATA(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_MAC_PROMISCUOUS_MODE(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_MAC_CNTR(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NCP_CNTR(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_MSG_BUFFER_COUNTERS(uint8_t header, spinel_prop_key_t key);
#if OPENTHREAD_ENABLE_MAC_WHITELIST
    otError GetPropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_MAC_WHITELIST_ENABLED(uint8_t header, spinel_prop_key_t key);
#endif
    otError GetPropertyHandler_THREAD_MODE(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_CHILD_TIMEOUT(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_RLOC16(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NET_REQUIRE_JOIN_EXISTING(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_DEBUG_TEST_ASSERT(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_DEBUG_NCP_LOG_LEVEL(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_DISCOVERY_SCAN_JOINER_FLAG(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_DISCOVERY_SCAN_PANID(uint8_t header, spinel_prop_key_t key);

#if OPENTHREAD_FTD
    otError GetPropertyHandler_THREAD_CHILD_TABLE(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_ROUTER_ROLE_ENABLED(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_NET_PSKC(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_CHILD_COUNT_MAX(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_ROUTER_DOWNGRADE_THRESHOLD(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_ROUTER_SELECTION_JITTER(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT(uint8_t header, spinel_prop_key_t key);
#endif // #if OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_COMMISSIONER
    otError GetPropertyHandler_THREAD_COMMISSIONER_ENABLED(uint8_t header, spinel_prop_key_t key);
#endif

    otError GetPropertyHandler_BA_PROXY_ENABLED(uint8_t header, spinel_prop_key_t key);

#if OPENTHREAD_ENABLE_JAM_DETECTION
    otError GetPropertyHandler_JAM_DETECT_ENABLE(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_JAM_DETECTED(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_JAM_DETECT_RSSI_THRESHOLD(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_JAM_DETECT_WINDOW(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_JAM_DETECT_BUSY(uint8_t header, spinel_prop_key_t key);
    otError GetPropertyHandler_JAM_DETECT_HISTORY_BITMAP(uint8_t header, spinel_prop_key_t key);
#endif

#if OPENTHREAD_ENABLE_LEGACY
    otError GetPropertyHandler_NEST_LEGACY_ULA_PREFIX(uint8_t header, spinel_prop_key_t key);
#endif


    otError SetPropertyHandler_POWER_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                           uint16_t value_len);
    otError SetPropertyHandler_HOST_POWER_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len);
    otError SetPropertyHandler_PHY_TX_POWER(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                            uint16_t value_len);
    otError SetPropertyHandler_PHY_CHAN(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                        uint16_t value_len);
    otError SetPropertyHandler_MAC_SCAN_MASK(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                             uint16_t value_len);
    otError SetPropertyHandler_MAC_SCAN_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                              uint16_t value_len);
    otError SetPropertyHandler_MAC_15_4_PANID(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                              uint16_t value_len);
    otError SetPropertyHandler_MAC_15_4_LADDR(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                              uint16_t value_len);
    otError SetPropertyHandler_MAC_RAW_STREAM_ENABLED(uint8_t header, spinel_prop_key_t key,
                                                      const uint8_t *value_ptr, uint16_t value_len);

#if OPENTHREAD_ENABLE_RAW_LINK_API
    otError SetPropertyHandler_MAC_15_4_SADDR(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                              uint16_t value_len);
    otError SetPropertyHandler_STREAM_RAW(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                          uint16_t value_len);
#endif // OPENTHREAD_ENABLE_RAW_LINK_API

    otError SetPropertyHandler_NET_IF_UP(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                         uint16_t value_len);
    otError SetPropertyHandler_NET_STACK_UP(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                            uint16_t value_len);
    otError SetPropertyHandler_NET_ROLE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                        uint16_t value_len);
    otError SetPropertyHandler_NET_NETWORK_NAME(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len);
    otError SetPropertyHandler_NET_XPANID(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                          uint16_t value_len);
    otError SetPropertyHandler_NET_MASTER_KEY(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                              uint16_t value_len);
    otError SetPropertyHandler_NET_KEY_SEQUENCE_COUNTER(uint8_t header, spinel_prop_key_t key,
                                                        const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_NET_KEY_SWITCH_GUARDTIME(uint8_t header, spinel_prop_key_t key,
                                                        const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_STREAM_NET_INSECURE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len);
    otError SetPropertyHandler_STREAM_NET(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                          uint16_t value_len);
    otError SetPropertyHandler_IPV6_ML_PREFIX(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                              uint16_t value_len);
    otError SetPropertyHandler_IPV6_ICMP_PING_OFFLOAD(uint8_t header, spinel_prop_key_t key,
                                                      const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_THREAD_RLOC16_DEBUG_PASSTHRU(uint8_t header, spinel_prop_key_t key,
                                                            const uint8_t *value_ptr, uint16_t value_len);

    otError SetPropertyHandler_THREAD_BA_PROXY_STREAM(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                      uint16_t value_len);
#if OPENTHREAD_ENABLE_RAW_LINK_API
    otError SetPropertyHandler_PHY_ENABLED(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                           uint16_t value_len);
#endif // OPENTHREAD_ENABLE_RAW_LINK_API

    otError SetPropertyHandler_MAC_PROMISCUOUS_MODE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                    uint16_t value_len);
    otError SetPropertyHandler_MAC_SCAN_PERIOD(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                               uint16_t value_len);
#if OPENTHREAD_ENABLE_MAC_WHITELIST
    otError SetPropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                             uint16_t value_len);
    otError SetPropertyHandler_MAC_WHITELIST_ENABLED(uint8_t header, spinel_prop_key_t key,
                                                     const uint8_t *value_ptr, uint16_t value_len);
#endif
#if OPENTHREAD_ENABLE_RAW_LINK_API
    otError SetPropertyHandler_MAC_SRC_MATCH_ENABLED(uint8_t header, spinel_prop_key_t key,
                                                     const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(uint8_t header, spinel_prop_key_t key,
                                                             const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(uint8_t header, spinel_prop_key_t key,
                                                                const uint8_t *value_ptr, uint16_t value_len);
#endif
#if OPENTHREAD_FTD
    otError SetPropertyHandler_NET_PSKC(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                        uint16_t value_len);
#endif
    otError SetPropertyHandler_THREAD_MODE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                           uint16_t value_len);

#if OPENTHREAD_ENABLE_BORDER_ROUTER
    otError SetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(uint8_t header, spinel_prop_key_t key,
                                                                  const uint8_t *value_ptr, uint16_t value_len);
#endif

#if OPENTHREAD_FTD
    otError SetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key,
                                                          const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_THREAD_CHILD_COUNT_MAX(uint8_t header, spinel_prop_key_t key,
                                                      const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_THREAD_CHILD_TIMEOUT(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                    uint16_t value_len);
    otError SetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD(uint8_t header, spinel_prop_key_t key,
                                                               const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_THREAD_ROUTER_DOWNGRADE_THRESHOLD(uint8_t header, spinel_prop_key_t key,
                                                                 const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_THREAD_ROUTER_SELECTION_JITTER(uint8_t header, spinel_prop_key_t key,
                                                              const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY(uint8_t header, spinel_prop_key_t key,
                                                          const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT(uint8_t header, spinel_prop_key_t key,
                                                         const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_THREAD_PREFERRED_ROUTER_ID(uint8_t header, spinel_prop_key_t key,
                                                          const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_THREAD_ROUTER_ROLE_ENABLED(uint8_t header, spinel_prop_key_t key,
                                                          const uint8_t *value_ptr, uint16_t value_len);
#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    otError SetPropertyHandler_THREAD_THREAD_STEERING_DATA(uint8_t header, spinel_prop_key_t key,
                                                           const uint8_t *value_ptr, uint16_t value_len);
#endif

#endif // #if OPENTHREAD_FTD

    otError SetPropertyHandler_THREAD_DISCOVERY_SCAN_JOINER_FLAG(uint8_t header, spinel_prop_key_t key,
                                                                 const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING(uint8_t header, spinel_prop_key_t key,
                                                                      const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_THREAD_DISCOVERY_SCAN_PANID(uint8_t header, spinel_prop_key_t key,
                                                           const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key,
                                                      const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_NET_REQUIRE_JOIN_EXISTING(uint8_t header, spinel_prop_key_t key,
                                                         const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_CNTR_RESET(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                          uint16_t value_len);
    otError SetPropertyHandler_DEBUG_NCP_LOG_LEVEL(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len);
#if OPENTHREAD_ENABLE_COMMISSIONER
    otError SetPropertyHandler_THREAD_COMMISSIONER_ENABLED(uint8_t header, spinel_prop_key_t key,
                                                           const uint8_t *value_ptr, uint16_t value_len);
#endif

    otError SetPropertyHandler_BA_PROXY_ENABLED(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len);
#if OPENTHREAD_ENABLE_JAM_DETECTION
    otError SetPropertyHandler_JAM_DETECT_ENABLE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                 uint16_t value_len);
    otError SetPropertyHandler_JAM_DETECT_RSSI_THRESHOLD(uint8_t header, spinel_prop_key_t key,
                                                         const uint8_t *value_ptr, uint16_t value_len);
    otError SetPropertyHandler_JAM_DETECT_WINDOW(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                 uint16_t value_len);
    otError SetPropertyHandler_JAM_DETECT_BUSY(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                               uint16_t value_len);
#endif

#if OPENTHREAD_ENABLE_DIAG
    otError SetPropertyHandler_NEST_STREAM_MFG(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                               uint16_t value_len);
#endif

#if OPENTHREAD_ENABLE_LEGACY
    otError SetPropertyHandler_NEST_LEGACY_ULA_PREFIX(uint8_t header, spinel_prop_key_t key,
                                                      const uint8_t *value_ptr, uint16_t value_len);
#endif

#if OPENTHREAD_ENABLE_RAW_LINK_API
    otError InsertPropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(uint8_t header, spinel_prop_key_t key,
                                                                const uint8_t *value_ptr, uint16_t value_len);
    otError InsertPropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(uint8_t header, spinel_prop_key_t key,
                                                                   const uint8_t *value_ptr, uint16_t value_len);
#endif
    otError InsertPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key,
                                                     const uint8_t *value_ptr, uint16_t value_len);
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    otError InsertPropertyHandler_THREAD_OFF_MESH_ROUTES(uint8_t header, spinel_prop_key_t key,
                                                         const uint8_t *value_ptr, uint16_t value_len);
    otError InsertPropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key,
                                                      const uint8_t *value_ptr, uint16_t value_len);
#endif
    otError InsertPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key,
                                                         const uint8_t *value_ptr, uint16_t value_len);
#if OPENTHREAD_ENABLE_MAC_WHITELIST
    otError InsertPropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len);
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER
    otError InsertPropertyHandler_THREAD_JOINERS(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                 uint16_t value_len);
#endif

#if OPENTHREAD_ENABLE_RAW_LINK_API
    otError RemovePropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(uint8_t header, spinel_prop_key_t key,
                                                                const uint8_t *value_ptr, uint16_t value_len);
    otError RemovePropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(uint8_t header, spinel_prop_key_t key,
                                                                   const uint8_t *value_ptr, uint16_t value_len);
#endif
    otError RemovePropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key,
                                                     const uint8_t *value_ptr, uint16_t value_len);
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    otError RemovePropertyHandler_THREAD_OFF_MESH_ROUTES(uint8_t header, spinel_prop_key_t key,
                                                         const uint8_t *value_ptr, uint16_t value_len);
    otError RemovePropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key,
                                                      const uint8_t *value_ptr, uint16_t value_len);
#endif
    otError RemovePropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key,
                                                         const uint8_t *value_ptr, uint16_t value_len);
#if OPENTHREAD_ENABLE_MAC_WHITELIST
    otError RemovePropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len);
#endif
#if OPENTHREAD_FTD
    otError RemovePropertyHandler_THREAD_ACTIVE_ROUTER_IDS(uint8_t header, spinel_prop_key_t key,
                                                           const uint8_t *value_ptr, uint16_t value_len);
#endif

public:
    otError StreamWrite(int aStreamId, const uint8_t *aDataPtr, int aDataLen);

#if OPENTHREAD_ENABLE_LEGACY
public:
    void HandleLegacyNodeDidJoin(const otExtAddress *aExtAddr);
    void HandleDidReceiveNewLegacyUlaPrefix(const uint8_t *aUlaPrefix);
    void RegisterLegacyHandlers(const otNcpLegacyHandlers *aHandlers);
#endif

protected:
    static NcpBase *sNcpInstance;
    otInstance *mInstance;
    NcpFrameBuffer  mTxFrameBuffer;

private:
    enum
    {
        kTxBufferSize = OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE,  // Tx Buffer size (used by mTxFrameBuffer).
    };

    spinel_status_t mLastStatus;
    uint32_t mSupportedChannelMask;
    uint32_t mChannelMask;
    uint16_t mScanPeriod;
    bool mDiscoveryScanJoinerFlag;
    bool mDiscoveryScanEnableFiltering;
    uint16_t mDiscoveryScanPanId;
    Tasklet mUpdateChangedPropsTask;
    uint32_t mChangedFlags;
    bool mShouldSignalEndOfScan;
    spinel_host_power_state_t mHostPowerState;
    bool mHostPowerStateInProgress;
    NcpFrameBuffer::FrameTag mHostPowerReplyFrameTag;
    uint8_t mHostPowerStateHeader;

#if OPENTHREAD_ENABLE_JAM_DETECTION
    bool mShouldSignalJamStateChange;
#endif

    spinel_tid_t mDroppedReplyTid;
    uint16_t mDroppedReplyTidBitSet;
    spinel_tid_t mNextExpectedTid;
    uint8_t mTxBuffer[kTxBufferSize];

    bool mAllowLocalNetworkDataChange;
    bool mRequireJoinExistingNetwork;
    bool mIsRawStreamEnabled;
    bool mDisableStreamWrite;

#if OPENTHREAD_ENABLE_RAW_LINK_API
    uint8_t mCurTransmitTID;
    uint8_t mCurReceiveChannel;
    int8_t  mCurScanChannel;
#endif // OPENTHREAD_ENABLE_RAW_LINK_API

    uint32_t mFramingErrorCounter;             // Number of improperly formed received spinel frames.
    uint32_t mRxSpinelFrameCounter;            // Number of received (inbound) spinel frames.
    uint32_t mRxSpinelOutOfOrderTidCounter;    // Number of out of order received spinel frames (tid increase > 1).
    uint32_t mTxSpinelFrameCounter;            // Number of sent (outbound) spinel frames.
    uint32_t mInboundSecureIpFrameCounter;     // Number of secure inbound data/IP frames.
    uint32_t mInboundInsecureIpFrameCounter;   // Number of insecure inbound data/IP frames.
    uint32_t mOutboundSecureIpFrameCounter;    // Number of secure outbound data/IP frames.
    uint32_t mOutboundInsecureIpFrameCounter;  // Number of insecure outbound data/IP frames.
    uint32_t mDroppedOutboundIpFrameCounter;   // Number of dropped outbound data/IP frames.
    uint32_t mDroppedInboundIpFrameCounter;    // Number of dropped inbound data/IP frames.

#if OPENTHREAD_ENABLE_LEGACY
    const otNcpLegacyHandlers *mLegacyHandlers;
    uint8_t mLegacyUlaPrefix[OT_NCP_LEGACY_ULA_PREFIX_LENGTH];
    bool mLegacyNodeDidJoin;
#endif

};

}  // namespace ot

#endif  // NCP_BASE_HPP_
