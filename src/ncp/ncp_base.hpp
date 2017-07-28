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

#include <openthread/config.h>

#include <openthread/ip6.h>
#include <openthread/message.h>
#include <openthread/ncp.h>
#include <openthread/types.h>

#include "openthread-core-config.h"
#include "spinel.h"
#include "common/tasklet.hpp"
#include "ncp/ncp_buffer.hpp"

namespace ot {

#define NCP_COMMAND_HANDLER(name)                             \
            otError CommandHandler_##name(                    \
                        uint8_t aHeader,                      \
                        unsigned int aCommand,                \
                        const uint8_t *aArgPtr,               \
                        uint16_t aArgLen                      \
                    )


#define NCP_GET_PROP_HANDLER(name)                            \
            otError GetPropertyHandler_##name(                \
                        uint8_t aHeader,                      \
                        spinel_prop_key_t aKey                \
                    )

#define NCP_SET_PROP_HANDLER(name)                            \
            otError SetPropertyHandler_##name(                \
                        uint8_t aHeader,                      \
                        spinel_prop_key_t aKey,               \
                        const uint8_t *aValuePtr,             \
                        uint16_t aValueLen                    \
                    )

#define NCP_INSERT_PROP_HANDLER(name)                         \
            otError InsertPropertyHandler_##name(             \
                        uint8_t aHeader,                      \
                        spinel_prop_key_t aKey,               \
                        const uint8_t *aValuePtr,             \
                        uint16_t aValueLen                    \
                    )

#define NCP_REMOVE_PROP_HANDLER(name)                         \
            otError RemovePropertyHandler_##name(             \
                        uint8_t aHeader,                      \
                        spinel_prop_key_t aKey,               \
                        const uint8_t *aValuePtr,             \
                        uint16_t aValueLen                    \
                    )

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
     * param[in] aHeader           The spinel header byte
     *
     * @retval OT_ERROR_NONE       Successfully started a new frame.
     * @retval OT_ERROR_NO_BUFS    Insufficient buffer space available to start a new frame.
     *
     */
    otError OutboundFrameBegin(uint8_t aHeader);

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
    void HandleReceive(const uint8_t *aBuf, uint16_t aBufLength);

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

#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
    static void HandleTmfProxyStream(otMessage *aMessage, uint16_t aLocator, uint16_t aPort, void *aContext);
    void HandleTmfProxyStream(otMessage *aMessage, uint16_t aLocator, uint16_t aPort);
#endif // OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD

    static void HandleFrameRemovedFromNcpBuffer(void *aContext, NcpFrameBuffer::FrameTag aFrameTag,
                                                NcpFrameBuffer::Priority aPriority, NcpFrameBuffer *aNcpBuffer);
    void HandleFrameRemovedFromNcpBuffer(NcpFrameBuffer::FrameTag aFrameTag);

    static void HandleDatagramFromStack(otMessage *aMessage, void *aContext);
    void HandleDatagramFromStack(otMessage *aMessage);

    static void HandleRawFrame(const otRadioFrame *aFrame, void *aContext);
    void HandleRawFrame(const otRadioFrame *aFrame);

    static void HandleActiveScanResult_Jump(otActiveScanResult *aResult, void *aContext);
    void HandleActiveScanResult(otActiveScanResult *aResult);

    static void HandleEnergyScanResult_Jump(otEnergyScanResult *aResult, void *aContext);
    void HandleEnergyScanResult(otEnergyScanResult *aResult);

    static void HandleJamStateChange_Jump(bool aJamState, void *aContext);
    void HandleJamStateChange(bool aJamState);

    static void UpdateChangedProps(Tasklet &aTasklet);
    void UpdateChangedProps(void);
    void ProcessThreadChangedFlags(void);

    static void SendDoneTask(void *aContext);
    void SendDoneTask(void);

#if OPENTHREAD_ENABLE_RAW_LINK_API

    static void LinkRawReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError);
    void LinkRawReceiveDone(otRadioFrame *aFrame, otError aError);

    static void LinkRawTransmitDone(otInstance *aInstance, otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError);
    void LinkRawTransmitDone(otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError);

    static void LinkRawEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi);
    void LinkRawEnergyScanDone(int8_t aEnergyScanMaxRssi);

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

    static void HandleNetifStateChanged(uint32_t aFlags, void *aContext);

private:

    otError OutboundFrameFeedPacked(const char *aPackFormat, ...);

    otError OutboundFrameFeedVPacked(const char *aPackFormat, va_list aArgs);

private:

    otError HandleCommand(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr, uint16_t aArgLen);

    otError HandleCommandPropertyGet(uint8_t aHeader, spinel_prop_key_t aKey);

    otError HandleCommandPropertySet(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                     uint16_t aValueLen);

    otError HandleCommandPropertyInsert(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                        uint16_t aValueLen);

    otError HandleCommandPropertyRemove(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                        uint16_t aValueLen);

    otError SendLastStatus(uint8_t aHeader, spinel_status_t aLastStatus);

    otError GetPropertyHandler_ChannelMaskHelper(uint8_t aHeader, spinel_prop_key_t aKey, uint32_t channel_mask);

private:

    otError SendPropertyUpdate(uint8_t aHeader, uint8_t aCommand, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                               uint16_t aValueLen);

    otError SendPropertyUpdate(uint8_t aHeader, uint8_t aCommand, spinel_prop_key_t aKey, otMessage *message);

    otError SendPropertyUpdate(uint8_t aHeader, uint8_t aCommand, spinel_prop_key_t aKey, const char *format, ...);

    otError SendSetPropertyResponse(uint8_t aHeader, spinel_prop_key_t aKey, otError aError);

private:

    typedef otError(NcpBase::*CommandHandlerType)(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr,
                                                  uint16_t aArgLen);

    typedef otError(NcpBase::*GetPropertyHandlerType)(uint8_t aHeader, spinel_prop_key_t aKey);

    typedef otError(NcpBase::*SetPropertyHandlerType)(uint8_t aHeader, spinel_prop_key_t aKey,
                                                      const uint8_t *aValuePtr, uint16_t aValueLen);

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

    // Command Handlers

    NCP_COMMAND_HANDLER(NOOP);
    NCP_COMMAND_HANDLER(RESET);
    NCP_COMMAND_HANDLER(PROP_VALUE_GET);
    NCP_COMMAND_HANDLER(PROP_VALUE_SET);
    NCP_COMMAND_HANDLER(PROP_VALUE_INSERT);
    NCP_COMMAND_HANDLER(PROP_VALUE_REMOVE);
    NCP_COMMAND_HANDLER(NET_SAVE);
    NCP_COMMAND_HANDLER(NET_CLEAR);
    NCP_COMMAND_HANDLER(NET_RECALL);
#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    NCP_COMMAND_HANDLER(PEEK);
    NCP_COMMAND_HANDLER(POKE);
#endif

    // Property Get Handlers

    NCP_GET_PROP_HANDLER(LAST_STATUS);
    NCP_GET_PROP_HANDLER(PROTOCOL_VERSION);
    NCP_GET_PROP_HANDLER(INTERFACE_TYPE);
    NCP_GET_PROP_HANDLER(VENDOR_ID);
    NCP_GET_PROP_HANDLER(CAPS);
    NCP_GET_PROP_HANDLER(NCP_VERSION);
    NCP_GET_PROP_HANDLER(INTERFACE_COUNT);
    NCP_GET_PROP_HANDLER(POWER_STATE);
    NCP_GET_PROP_HANDLER(HWADDR);
    NCP_GET_PROP_HANDLER(LOCK);
    NCP_GET_PROP_HANDLER(HOST_POWER_STATE);
    NCP_GET_PROP_HANDLER(UNSOL_UPDATE_FILTER);
    NCP_GET_PROP_HANDLER(UNSOL_UPDATE_LIST);
    NCP_GET_PROP_HANDLER(PHY_ENABLED);
    NCP_GET_PROP_HANDLER(PHY_FREQ);
    NCP_GET_PROP_HANDLER(PHY_CHAN_SUPPORTED);
    NCP_GET_PROP_HANDLER(PHY_CHAN);
    NCP_GET_PROP_HANDLER(PHY_RSSI);
    NCP_GET_PROP_HANDLER(PHY_TX_POWER);
    NCP_GET_PROP_HANDLER(PHY_RX_SENSITIVITY);
    NCP_GET_PROP_HANDLER(MAC_SCAN_STATE);
    NCP_GET_PROP_HANDLER(MAC_15_4_PANID);
    NCP_GET_PROP_HANDLER(MAC_15_4_LADDR);
    NCP_GET_PROP_HANDLER(MAC_15_4_SADDR);
    NCP_GET_PROP_HANDLER(MAC_RAW_STREAM_ENABLED);
    NCP_GET_PROP_HANDLER(MAC_EXTENDED_ADDR);
    NCP_GET_PROP_HANDLER(MAC_DATA_POLL_PERIOD);
    NCP_GET_PROP_HANDLER(NET_SAVED);
    NCP_GET_PROP_HANDLER(NET_IF_UP);
    NCP_GET_PROP_HANDLER(NET_STACK_UP);
    NCP_GET_PROP_HANDLER(NET_ROLE);
    NCP_GET_PROP_HANDLER(NET_NETWORK_NAME);
    NCP_GET_PROP_HANDLER(NET_XPANID);
    NCP_GET_PROP_HANDLER(NET_MASTER_KEY);
    NCP_GET_PROP_HANDLER(NET_KEY_SEQUENCE_COUNTER);
    NCP_GET_PROP_HANDLER(NET_PARTITION_ID);
    NCP_GET_PROP_HANDLER(NET_KEY_SWITCH_GUARDTIME);
    NCP_GET_PROP_HANDLER(THREAD_LEADER);
    NCP_GET_PROP_HANDLER(IPV6_ML_PREFIX);
    NCP_GET_PROP_HANDLER(IPV6_ML_ADDR);
    NCP_GET_PROP_HANDLER(IPV6_LL_ADDR);
    NCP_GET_PROP_HANDLER(IPV6_ADDRESS_TABLE);
    NCP_GET_PROP_HANDLER(IPV6_ROUTE_TABLE);
    NCP_GET_PROP_HANDLER(IPV6_ICMP_PING_OFFLOAD);
    NCP_GET_PROP_HANDLER(IPV6_MULTICAST_ADDRESS_TABLE);
    NCP_GET_PROP_HANDLER(THREAD_RLOC16_DEBUG_PASSTHRU);
    NCP_GET_PROP_HANDLER(THREAD_OFF_MESH_ROUTES);
    NCP_GET_PROP_HANDLER(STREAM_NET);
    NCP_GET_PROP_HANDLER(MAC_SCAN_MASK);
    NCP_GET_PROP_HANDLER(MAC_SCAN_PERIOD);
    NCP_GET_PROP_HANDLER(THREAD_LEADER_ADDR);
    NCP_GET_PROP_HANDLER(THREAD_PARENT);
    NCP_GET_PROP_HANDLER(THREAD_NEIGHBOR_TABLE);
    NCP_GET_PROP_HANDLER(THREAD_LEADER_RID);
    NCP_GET_PROP_HANDLER(THREAD_LEADER_WEIGHT);
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_GET_PROP_HANDLER(THREAD_NETWORK_DATA);
    NCP_GET_PROP_HANDLER(THREAD_STABLE_NETWORK_DATA);
#endif
    NCP_GET_PROP_HANDLER(THREAD_NETWORK_DATA_VERSION);
    NCP_GET_PROP_HANDLER(THREAD_STABLE_NETWORK_DATA_VERSION);
    NCP_GET_PROP_HANDLER(THREAD_LEADER_NETWORK_DATA);
    NCP_GET_PROP_HANDLER(THREAD_STABLE_LEADER_NETWORK_DATA);
    NCP_GET_PROP_HANDLER(MAC_PROMISCUOUS_MODE);
    NCP_GET_PROP_HANDLER(THREAD_ASSISTING_PORTS);
    NCP_GET_PROP_HANDLER(THREAD_ALLOW_LOCAL_NET_DATA_CHANGE);
    NCP_GET_PROP_HANDLER(MAC_CNTR);
    NCP_GET_PROP_HANDLER(NCP_CNTR);
    NCP_GET_PROP_HANDLER(IP_CNTR);
    NCP_GET_PROP_HANDLER(MSG_BUFFER_COUNTERS);
#if OPENTHREAD_ENABLE_MAC_FILTER
    NCP_GET_PROP_HANDLER(MAC_WHITELIST);
    NCP_GET_PROP_HANDLER(MAC_WHITELIST_ENABLED);
    NCP_GET_PROP_HANDLER(MAC_BLACKLIST);
    NCP_GET_PROP_HANDLER(MAC_BLACKLIST_ENABLED);
    NCP_GET_PROP_HANDLER(MAC_FIXED_RSS);
#endif
    NCP_GET_PROP_HANDLER(THREAD_MODE);
    NCP_GET_PROP_HANDLER(THREAD_CHILD_TIMEOUT);
    NCP_GET_PROP_HANDLER(THREAD_RLOC16);
    NCP_GET_PROP_HANDLER(THREAD_ON_MESH_NETS);
    NCP_GET_PROP_HANDLER(NET_REQUIRE_JOIN_EXISTING);
    NCP_GET_PROP_HANDLER(DEBUG_TEST_ASSERT);
    NCP_GET_PROP_HANDLER(DEBUG_NCP_LOG_LEVEL);
    NCP_GET_PROP_HANDLER(THREAD_DISCOVERY_SCAN_JOINER_FLAG);
    NCP_GET_PROP_HANDLER(THREAD_DISCOVERY_SCAN_ENABLE_FILTERING);
    NCP_GET_PROP_HANDLER(THREAD_DISCOVERY_SCAN_PANID);
#if OPENTHREAD_FTD
    NCP_GET_PROP_HANDLER(THREAD_CHILD_TABLE);
    NCP_GET_PROP_HANDLER(THREAD_LOCAL_LEADER_WEIGHT);
    NCP_GET_PROP_HANDLER(THREAD_ROUTER_ROLE_ENABLED);
    NCP_GET_PROP_HANDLER(NET_PSKC);
    NCP_GET_PROP_HANDLER(THREAD_CHILD_COUNT_MAX);
    NCP_GET_PROP_HANDLER(THREAD_ROUTER_UPGRADE_THRESHOLD);
    NCP_GET_PROP_HANDLER(THREAD_ROUTER_DOWNGRADE_THRESHOLD);
    NCP_GET_PROP_HANDLER(THREAD_ROUTER_SELECTION_JITTER);
    NCP_GET_PROP_HANDLER(THREAD_CONTEXT_REUSE_DELAY);
    NCP_GET_PROP_HANDLER(THREAD_NETWORK_ID_TIMEOUT);
#endif // #if OPENTHREAD_FTD
#if OPENTHREAD_ENABLE_COMMISSIONER
    NCP_GET_PROP_HANDLER(THREAD_COMMISSIONER_ENABLED);
#endif
    NCP_GET_PROP_HANDLER(THREAD_TMF_PROXY_ENABLED);
#if OPENTHREAD_ENABLE_JAM_DETECTION
    NCP_GET_PROP_HANDLER(JAM_DETECT_ENABLE);
    NCP_GET_PROP_HANDLER(JAM_DETECTED);
    NCP_GET_PROP_HANDLER(JAM_DETECT_RSSI_THRESHOLD);
    NCP_GET_PROP_HANDLER(JAM_DETECT_WINDOW);
    NCP_GET_PROP_HANDLER(JAM_DETECT_BUSY);
    NCP_GET_PROP_HANDLER(JAM_DETECT_HISTORY_BITMAP);
#endif
#if OPENTHREAD_ENABLE_LEGACY
    NCP_GET_PROP_HANDLER(NEST_LEGACY_ULA_PREFIX);
    NCP_GET_PROP_HANDLER(NEST_LEGACY_LAST_NODE_JOINED);
#endif

    // Property Set Handlers

    NCP_SET_PROP_HANDLER(POWER_STATE);
    NCP_SET_PROP_HANDLER(HOST_POWER_STATE);
    NCP_SET_PROP_HANDLER(PHY_TX_POWER);
    NCP_SET_PROP_HANDLER(PHY_CHAN);
    NCP_SET_PROP_HANDLER(UNSOL_UPDATE_FILTER);
    NCP_SET_PROP_HANDLER(MAC_SCAN_MASK);
    NCP_SET_PROP_HANDLER(MAC_SCAN_STATE);
    NCP_SET_PROP_HANDLER(MAC_15_4_PANID);
    NCP_SET_PROP_HANDLER(MAC_15_4_LADDR);
    NCP_SET_PROP_HANDLER(MAC_DATA_POLL_PERIOD);
    NCP_SET_PROP_HANDLER(MAC_RAW_STREAM_ENABLED);
#if OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_SET_PROP_HANDLER(MAC_15_4_SADDR);
    NCP_SET_PROP_HANDLER(STREAM_RAW);
#endif
    NCP_SET_PROP_HANDLER(NET_IF_UP);
    NCP_SET_PROP_HANDLER(NET_STACK_UP);
    NCP_SET_PROP_HANDLER(NET_ROLE);
    NCP_SET_PROP_HANDLER(NET_NETWORK_NAME);
    NCP_SET_PROP_HANDLER(NET_XPANID);
    NCP_SET_PROP_HANDLER(NET_MASTER_KEY);
    NCP_SET_PROP_HANDLER(NET_KEY_SEQUENCE_COUNTER);
    NCP_SET_PROP_HANDLER(NET_KEY_SWITCH_GUARDTIME);
    NCP_SET_PROP_HANDLER(STREAM_NET_INSECURE);
    NCP_SET_PROP_HANDLER(STREAM_NET);
    NCP_SET_PROP_HANDLER(IPV6_ML_PREFIX);
    NCP_SET_PROP_HANDLER(IPV6_ICMP_PING_OFFLOAD);
    NCP_SET_PROP_HANDLER(THREAD_RLOC16_DEBUG_PASSTHRU);
    NCP_SET_PROP_HANDLER(THREAD_TMF_PROXY_STREAM);
#if OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_SET_PROP_HANDLER(PHY_ENABLED);
#endif
    NCP_SET_PROP_HANDLER(MAC_PROMISCUOUS_MODE);
    NCP_SET_PROP_HANDLER(MAC_SCAN_PERIOD);
#if OPENTHREAD_ENABLE_MAC_FILTER
    NCP_SET_PROP_HANDLER(MAC_WHITELIST);
    NCP_SET_PROP_HANDLER(MAC_WHITELIST_ENABLED);
    NCP_SET_PROP_HANDLER(MAC_BLACKLIST);
    NCP_SET_PROP_HANDLER(MAC_BLACKLIST_ENABLED);
    NCP_SET_PROP_HANDLER(MAC_FIXED_RSS);
#endif
#if OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_SET_PROP_HANDLER(MAC_SRC_MATCH_ENABLED);
    NCP_SET_PROP_HANDLER(MAC_SRC_MATCH_SHORT_ADDRESSES);
    NCP_SET_PROP_HANDLER(MAC_SRC_MATCH_EXTENDED_ADDRESSES);
#endif
    NCP_SET_PROP_HANDLER(THREAD_MODE);
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_SET_PROP_HANDLER(THREAD_ALLOW_LOCAL_NET_DATA_CHANGE);
#endif
#if OPENTHREAD_FTD
    NCP_SET_PROP_HANDLER(NET_PSKC);
    NCP_SET_PROP_HANDLER(THREAD_LOCAL_LEADER_WEIGHT);
    NCP_SET_PROP_HANDLER(THREAD_CHILD_COUNT_MAX);
    NCP_SET_PROP_HANDLER(THREAD_CHILD_TIMEOUT);
    NCP_SET_PROP_HANDLER(THREAD_ROUTER_UPGRADE_THRESHOLD);
    NCP_SET_PROP_HANDLER(THREAD_ROUTER_DOWNGRADE_THRESHOLD);
    NCP_SET_PROP_HANDLER(THREAD_ROUTER_SELECTION_JITTER);
    NCP_SET_PROP_HANDLER(THREAD_CONTEXT_REUSE_DELAY);
    NCP_SET_PROP_HANDLER(THREAD_NETWORK_ID_TIMEOUT);
    NCP_SET_PROP_HANDLER(THREAD_PREFERRED_ROUTER_ID);
    NCP_SET_PROP_HANDLER(THREAD_ROUTER_ROLE_ENABLED);
#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    NCP_SET_PROP_HANDLER(THREAD_STEERING_DATA);
#endif
#endif // #if OPENTHREAD_FTD
    NCP_SET_PROP_HANDLER(THREAD_DISCOVERY_SCAN_JOINER_FLAG);
    NCP_SET_PROP_HANDLER(THREAD_DISCOVERY_SCAN_ENABLE_FILTERING);
    NCP_SET_PROP_HANDLER(THREAD_DISCOVERY_SCAN_PANID);
    NCP_SET_PROP_HANDLER(THREAD_ASSISTING_PORTS);
    NCP_SET_PROP_HANDLER(NET_REQUIRE_JOIN_EXISTING);
    NCP_SET_PROP_HANDLER(CNTR_RESET);
    NCP_SET_PROP_HANDLER(DEBUG_NCP_LOG_LEVEL);
#if OPENTHREAD_ENABLE_COMMISSIONER
    NCP_SET_PROP_HANDLER(THREAD_COMMISSIONER_ENABLED);
#endif
    NCP_SET_PROP_HANDLER(THREAD_TMF_PROXY_ENABLED);
#if OPENTHREAD_ENABLE_JAM_DETECTION
    NCP_SET_PROP_HANDLER(JAM_DETECT_ENABLE);
    NCP_SET_PROP_HANDLER(JAM_DETECT_RSSI_THRESHOLD);
    NCP_SET_PROP_HANDLER(JAM_DETECT_WINDOW);
    NCP_SET_PROP_HANDLER(JAM_DETECT_BUSY);
#endif
#if OPENTHREAD_ENABLE_DIAG
    NCP_SET_PROP_HANDLER(NEST_STREAM_MFG);
#endif
#if OPENTHREAD_ENABLE_LEGACY
    NCP_SET_PROP_HANDLER(NEST_LEGACY_ULA_PREFIX);
#endif

    // Property Insert Handlers

    NCP_INSERT_PROP_HANDLER(UNSOL_UPDATE_FILTER);
#if OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_INSERT_PROP_HANDLER(MAC_SRC_MATCH_SHORT_ADDRESSES);
    NCP_INSERT_PROP_HANDLER(MAC_SRC_MATCH_EXTENDED_ADDRESSES);
#endif
    NCP_INSERT_PROP_HANDLER(IPV6_ADDRESS_TABLE);
    NCP_INSERT_PROP_HANDLER(IPV6_MULTICAST_ADDRESS_TABLE);
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_INSERT_PROP_HANDLER(THREAD_OFF_MESH_ROUTES);
    NCP_INSERT_PROP_HANDLER(THREAD_ON_MESH_NETS);
#endif
    NCP_INSERT_PROP_HANDLER(THREAD_ASSISTING_PORTS);
#if OPENTHREAD_ENABLE_MAC_FILTER
    NCP_INSERT_PROP_HANDLER(MAC_WHITELIST);
    NCP_INSERT_PROP_HANDLER(MAC_BLACKLIST);
    NCP_INSERT_PROP_HANDLER(MAC_FIXED_RSS);
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER
    NCP_INSERT_PROP_HANDLER(THREAD_JOINERS);
#endif

    // Property Remove Handlers

    NCP_REMOVE_PROP_HANDLER(UNSOL_UPDATE_FILTER);
#if OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_REMOVE_PROP_HANDLER(MAC_SRC_MATCH_SHORT_ADDRESSES);
    NCP_REMOVE_PROP_HANDLER(MAC_SRC_MATCH_EXTENDED_ADDRESSES);
#endif
    NCP_REMOVE_PROP_HANDLER(IPV6_ADDRESS_TABLE);
    NCP_REMOVE_PROP_HANDLER(IPV6_MULTICAST_ADDRESS_TABLE);
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_REMOVE_PROP_HANDLER(THREAD_OFF_MESH_ROUTES);
    NCP_REMOVE_PROP_HANDLER(THREAD_ON_MESH_NETS);
#endif
    NCP_REMOVE_PROP_HANDLER(THREAD_ASSISTING_PORTS);
#if OPENTHREAD_ENABLE_MAC_FILTER
    NCP_REMOVE_PROP_HANDLER(MAC_WHITELIST);
    NCP_REMOVE_PROP_HANDLER(MAC_BLACKLIST);
    NCP_REMOVE_PROP_HANDLER(MAC_FIXED_RSS);
#endif
#if OPENTHREAD_FTD
    NCP_REMOVE_PROP_HANDLER(THREAD_ACTIVE_ROUTER_IDS);
#endif

public:
    otError StreamWrite(int aStreamId, const uint8_t *aDataPtr, int aDataLen);

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    void RegisterPeekPokeDelagates(otNcpDelegateAllowPeekPoke aAllowPeekDelegate,
                                   otNcpDelegateAllowPeekPoke aAllowPokeDelegate);
#endif

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

    /**
     * Defines a class to track a set of property/status changes that require update to host. The properties that can
     * be added to this set must support sending unsolicited updates. This class also provides mechanism for user
     * to block certain filterable properties disallowing the unsolicited update from them.
     *
     */
    class ChangedPropsSet
    {
    public:
        /**
         * Defines an entry in the set/list.
         *
         */
        struct Entry
        {
            spinel_prop_key_t mPropKey;    ///< The spinel property key.
            spinel_status_t   mStatus;     ///< The spinel status (used only if prop key is `LAST_STATUS`).
            bool              mFilterable; ///< Indicates whether the entry can be filtered
        };

        /**
         * This constructor initializes the set.
         *
         */
        ChangedPropsSet(void):
            mChangedSet(0),
            mFilterSet(0)
        { }

        /**
         * This method clears the set.
         *
         */
        void Clear(void)  { mChangedSet = 0; }

        /**
         * This method indicates if the set is empty or not.
         *
         * @returns TRUE if the set if empty, FALSE otherwise.
         *
         */
        bool IsEmpty(void) const { return (mChangedSet == 0); }

        /**
         * This method adds a property to the set. The property added must be in the list of supported properties
         * capable of sending unsolicited update, otherwise the input is ignored.
         *
         * Note that if the property is already in the set, adding it again does not change the set.
         *
         * @param[in] aPropKey    The spinel property key to be added to the set
         *
         */
        void AddProperty(spinel_prop_key_t aPropKey) { Add(aPropKey, SPINEL_STATUS_OK); }

        /**
         * This method adds a `LAST_STATUS` update to the set. The update must be in list of supported entries.
         *
         * @param[in] aStatus     The spinel status update to be added to set.
         *
         */
        void AddLastStatus(spinel_status_t aStatus) {  Add(SPINEL_PROP_LAST_STATUS, aStatus); }

        /**
         * This method returns a pointer to array of entries of supported property/status updates. The list includes
         * all properties that can generate unsolicited update.
         *
         * @param[out]  aNumEntries  A reference to output the number of entries in the list.
         *
         * @returns A pointer to the supported entries array.
         *
         */
        const Entry *GetSupportedEntries(uint8_t &aNumEntries) const {
            aNumEntries = GetNumEntries();
            return &mSupportedProps[0];
        }

        /**
         * This method returns a pointer to the entry associated with a given index.
         *
         * @param[in] aIndex     The index to an entry.
         *
         * @returns A pointer to the entry associated with @p aIndex, or NULL if the index is beyond end of array.
         *
         */
        const Entry *GetEntry(uint8_t aIndex) const {
            return (aIndex < GetNumEntries()) ? &mSupportedProps[aIndex] : NULL;
        }

        /**
         * This method indicates if the entry associated with an index is in the set (i.e., it has been changed and
         * requires an unsolicited update).
         *
         * @param[in] aIndex     The index to an entry.
         *
         * @returns TRUE if the entry is in the set, FALSE otherwise.
         *
         */
        bool IsEntryChanged(uint8_t aIndex) const { return IsBitSet(mChangedSet, aIndex); }

        /**
         * This method removes an entry associated with an index in the set.
         *
         * Note that if the property/entry is not in the set, removing it simply does nothing.
         *
         * @param[in] aIndex               Index of entry to be removed.
         *
         */
        void RemoveEntry(uint8_t aIndex) { ClearBit(mChangedSet, aIndex); }

        /**
         * This method enables/disables filtering of a given property.
         *
         * @param[in] aPropKey             The property key to filter.
         * @param[in] aEnable              TRUE to enable filtering, FALSE to disable.
         *
         * @retval OT_ERROR_NONE           Filter state for given property updated successfully.
         * @retval OT_ERROR_INVALID_ARGS   The given property is not valid (i.e., not capable of unsolicited update).
         *
         */
        otError EnablePropertyFilter(spinel_prop_key_t aPropKey, bool aEnable);

        /**
         * This method determines whether filtering is enabled for an entry associated with an index.
         *
         * @param[in] aIndex               Index of entry to be checked.
         *
         * @returns TRUE if the filter is enabled for the given entry, FALSE otherwise.
         *
         */
        bool IsEntryFiltered(uint8_t aIndex) const { return IsBitSet(mFilterSet, aIndex); }

        /**
         * This method determines whether filtering is enabled for a given property key.
         *
         * @param[in] aPropKey             The property key to check.
         *
         * @returns TRUE if the filter is enabled for the given property, FALSE if the property is not filtered or if
         *          it is not filterable.
         *
         */
        bool IsPropertyFiltered(spinel_prop_key_t aPropKey) const;

        /**
         * This method clears the filter.
         *
         */
        void ClearFilter(void) { mFilterSet = 0; }

    private:
        uint8_t GetNumEntries(void) const;
        void Add(spinel_prop_key_t aPropKey, spinel_status_t aStatus);

        static void SetBit  (uint32_t &aBitset, uint8_t aBitIndex)   { aBitset |= (1U << aBitIndex);                }
        static void ClearBit(uint32_t &aBitset, uint8_t aBitIndex)   { aBitset &= ~(1U << aBitIndex);               }
        static bool IsBitSet(uint32_t aBitset, uint8_t aBitIndex)    { return (aBitset & (1U << aBitIndex)) != 0;   }

        static const Entry mSupportedProps[];

        uint32_t mChangedSet;
        uint32_t mFilterSet;
    };

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
    uint32_t mThreadChangedFlags;
    ChangedPropsSet mChangedPropsSet;

    spinel_host_power_state_t mHostPowerState;
    bool mHostPowerStateInProgress;
    NcpFrameBuffer::FrameTag mHostPowerReplyFrameTag;
    uint8_t mHostPowerStateHeader;

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    otNcpDelegateAllowPeekPoke mAllowPeekDelegate;
    otNcpDelegateAllowPeekPoke mAllowPokeDelegate;
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
    otExtAddress mLegacyLastJoinedNode;
    bool mLegacyNodeDidJoin;
#endif

};

}  // namespace ot

#endif  // NCP_BASE_HPP_
