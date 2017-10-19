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
#include <openthread/message.h>
#include <openthread/ncp.h>
#include <openthread/types.h>

#include "changed_props_set.hpp"
#include "common/tasklet.hpp"
#include "ncp/ncp_buffer.hpp"
#include "ncp/spinel_decoder.hpp"
#include "ncp/spinel_encoder.hpp"

#include "spinel.h"

namespace ot {
namespace Ncp {

#define NCP_GET_PROP_HANDLER(name)    otError GetPropertyHandler_##name(void)
#define NCP_SET_PROP_HANDLER(name)    otError SetPropertyHandler_##name(void)
#define NCP_INSERT_PROP_HANDLER(name) otError InsertPropertyHandler_##name(void)
#define NCP_REMOVE_PROP_HANDLER(name) otError RemovePropertyHandler_##name(void)

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

private:
    typedef otError (NcpBase::*PropertyHandler)(void);

    struct PropertyHandlerEntry
    {
        spinel_prop_key_t mPropKey;
        PropertyHandler mHandler;
    };

    NcpFrameBuffer::FrameTag GetLastOutboundFrameTag(void);

    otError HandleCommand(uint8_t aHeader, unsigned int aCommand);

    PropertyHandler FindPropertyHandler(spinel_prop_key_t aKey, const PropertyHandlerEntry *aTable, size_t aTableLen);
    PropertyHandler FindGetPropertyHandler(spinel_prop_key_t aKey);
    PropertyHandler FindSetPropertyHandler(spinel_prop_key_t aKey);
    PropertyHandler FindInsertPropertyHandler(spinel_prop_key_t aKey);
    PropertyHandler FindRemovePropertyHandler(spinel_prop_key_t aKey);

    otError HandleCommandPropertyGet(uint8_t aHeader, spinel_prop_key_t aKey);
    bool HandlePropertySetForSpecialProperties(uint8_t aHeader, spinel_prop_key_t aKey, otError &aError);
    otError HandleCommandPropertySet(uint8_t aHeader, spinel_prop_key_t aKey);
    otError HandleCommandPropertyInsertRemove(uint8_t aHeader, spinel_prop_key_t aKey, unsigned int aCommand);

    otError SendLastStatus(uint8_t aHeader, spinel_status_t aLastStatus);

    static void UpdateChangedProps(Tasklet &aTasklet);
    void UpdateChangedProps(void);

    static void HandleFrameRemovedFromNcpBuffer(void *aContext, NcpFrameBuffer::FrameTag aFrameTag,
                                                NcpFrameBuffer::Priority aPriority, NcpFrameBuffer *aNcpBuffer);
    void HandleFrameRemovedFromNcpBuffer(NcpFrameBuffer::FrameTag aFrameTag);

    static void HandleRawFrame(const otRadioFrame *aFrame, void *aContext);
    void HandleRawFrame(const otRadioFrame *aFrame);

#if OPENTHREAD_ENABLE_RAW_LINK_API

    static void LinkRawReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError);
    void LinkRawReceiveDone(otRadioFrame *aFrame, otError aError);

    static void LinkRawTransmitDone(otInstance *aInstance, otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError);
    void LinkRawTransmitDone(otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError);

    static void LinkRawEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi);
    void LinkRawEnergyScanDone(int8_t aEnergyScanMaxRssi);

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    static void HandleNetifStateChanged(uint32_t aFlags, void *aContext);
    void ProcessThreadChangedFlags(void);

    static void HandleDatagramFromStack(otMessage *aMessage, void *aContext);
    void HandleDatagramFromStack(otMessage *aMessage);

    otError SendQueuedDatagramMessages(void);
    otError SendDatagramMessage(otMessage *aMessage);

    static void HandleActiveScanResult_Jump(otActiveScanResult *aResult, void *aContext);
    void HandleActiveScanResult(otActiveScanResult *aResult);

    static void HandleEnergyScanResult_Jump(otEnergyScanResult *aResult, void *aContext);
    void HandleEnergyScanResult(otEnergyScanResult *aResult);

    static void HandleJamStateChange_Jump(bool aJamState, void *aContext);
    void HandleJamStateChange(bool aJamState);

    static void SendDoneTask(void *aContext);
    void SendDoneTask(void);

    otError GetPropertyHandler_ChannelMaskHelper(uint32_t channel_mask);
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_TMF_PROXY
    static void HandleTmfProxyStream(otMessage *aMessage, uint16_t aLocator, uint16_t aPort, void *aContext);
    void HandleTmfProxyStream(otMessage *aMessage, uint16_t aLocator, uint16_t aPort);
#endif // OPENTHREAD_FTD && OPENTHREAD_ENABLE_TMF_PROXY

#if OPENTHREAD_ENABLE_SPINEL_VENDOR_SUPPORT
    otError VendorCommandHandler(uint8_t aHeader, unsigned int aCommand);
#endif // OPENTHREAD_ENABLE_SPINEL_VENDOR_SUPPORT

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

    // --------------------------------------------------------------------------
    // Common Properties

    NCP_GET_PROP_HANDLER(LAST_STATUS);
    NCP_GET_PROP_HANDLER(PROTOCOL_VERSION);
    NCP_GET_PROP_HANDLER(INTERFACE_TYPE);
    NCP_GET_PROP_HANDLER(VENDOR_ID);
    NCP_GET_PROP_HANDLER(CAPS);
    NCP_GET_PROP_HANDLER(DEBUG_TEST_ASSERT);
    NCP_GET_PROP_HANDLER(DEBUG_NCP_LOG_LEVEL);
    NCP_SET_PROP_HANDLER(DEBUG_NCP_LOG_LEVEL);
    NCP_GET_PROP_HANDLER(NCP_VERSION);
    NCP_GET_PROP_HANDLER(INTERFACE_COUNT);
    NCP_GET_PROP_HANDLER(POWER_STATE);
    NCP_SET_PROP_HANDLER(POWER_STATE);
    NCP_GET_PROP_HANDLER(HWADDR);
    NCP_GET_PROP_HANDLER(LOCK);
    NCP_GET_PROP_HANDLER(HOST_POWER_STATE);
    NCP_GET_PROP_HANDLER(UNSOL_UPDATE_FILTER);
    NCP_SET_PROP_HANDLER(UNSOL_UPDATE_FILTER);
    NCP_INSERT_PROP_HANDLER(UNSOL_UPDATE_FILTER);
    NCP_REMOVE_PROP_HANDLER(UNSOL_UPDATE_FILTER);
    NCP_GET_PROP_HANDLER(UNSOL_UPDATE_LIST);

    NCP_GET_PROP_HANDLER(PHY_RX_SENSITIVITY);
    NCP_GET_PROP_HANDLER(PHY_TX_POWER);
    NCP_SET_PROP_HANDLER(PHY_TX_POWER);
    NCP_GET_PROP_HANDLER(PHY_ENABLED);
    NCP_SET_PROP_HANDLER(PHY_CHAN);
    NCP_GET_PROP_HANDLER(PHY_CHAN);

    NCP_GET_PROP_HANDLER(MAC_15_4_PANID);
    NCP_SET_PROP_HANDLER(MAC_15_4_PANID);
    NCP_GET_PROP_HANDLER(MAC_15_4_LADDR);
    NCP_SET_PROP_HANDLER(MAC_15_4_LADDR);
    NCP_GET_PROP_HANDLER(MAC_15_4_SADDR);
    NCP_GET_PROP_HANDLER(MAC_PROMISCUOUS_MODE);
    NCP_SET_PROP_HANDLER(MAC_PROMISCUOUS_MODE);
    NCP_GET_PROP_HANDLER(MAC_RAW_STREAM_ENABLED);
    NCP_SET_PROP_HANDLER(MAC_RAW_STREAM_ENABLED);
    NCP_GET_PROP_HANDLER(MAC_DATA_POLL_PERIOD);
    NCP_SET_PROP_HANDLER(MAC_DATA_POLL_PERIOD);

    // --------------------------------------------------------------------------
    // Raw Link API Properties

#if OPENTHREAD_ENABLE_RAW_LINK_API

    NCP_SET_PROP_HANDLER(PHY_ENABLED);

    NCP_SET_PROP_HANDLER(MAC_15_4_SADDR);
    NCP_SET_PROP_HANDLER(MAC_SRC_MATCH_ENABLED);
    NCP_SET_PROP_HANDLER(MAC_SRC_MATCH_SHORT_ADDRESSES);
    NCP_INSERT_PROP_HANDLER(MAC_SRC_MATCH_SHORT_ADDRESSES);
    NCP_REMOVE_PROP_HANDLER(MAC_SRC_MATCH_SHORT_ADDRESSES);
    NCP_SET_PROP_HANDLER(MAC_SRC_MATCH_EXTENDED_ADDRESSES);
    NCP_INSERT_PROP_HANDLER(MAC_SRC_MATCH_EXTENDED_ADDRESSES);
    NCP_REMOVE_PROP_HANDLER(MAC_SRC_MATCH_EXTENDED_ADDRESSES);

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

    // --------------------------------------------------------------------------
    // MTD (or FTD) Properties

#if OPENTHREAD_MTD || OPENTHREAD_FTD

    NCP_SET_PROP_HANDLER(STREAM_NET);
    NCP_SET_PROP_HANDLER(STREAM_NET_INSECURE);

    NCP_GET_PROP_HANDLER(PHY_FREQ);
    NCP_GET_PROP_HANDLER(PHY_CHAN_SUPPORTED);
    NCP_GET_PROP_HANDLER(PHY_RSSI);

    NCP_GET_PROP_HANDLER(MAC_EXTENDED_ADDR);
    NCP_GET_PROP_HANDLER(MAC_SCAN_MASK);
    NCP_SET_PROP_HANDLER(MAC_SCAN_MASK);
    NCP_GET_PROP_HANDLER(MAC_SCAN_PERIOD);
    NCP_SET_PROP_HANDLER(MAC_SCAN_PERIOD);
    NCP_GET_PROP_HANDLER(MAC_SCAN_STATE);
    NCP_SET_PROP_HANDLER(MAC_SCAN_STATE);
#if OPENTHREAD_ENABLE_MAC_FILTER
    NCP_GET_PROP_HANDLER(MAC_WHITELIST_ENABLED);
    NCP_SET_PROP_HANDLER(MAC_WHITELIST_ENABLED);
    NCP_GET_PROP_HANDLER(MAC_WHITELIST);
    NCP_SET_PROP_HANDLER(MAC_WHITELIST);
    NCP_INSERT_PROP_HANDLER(MAC_WHITELIST);
    NCP_REMOVE_PROP_HANDLER(MAC_WHITELIST);
    NCP_GET_PROP_HANDLER(MAC_BLACKLIST_ENABLED);
    NCP_SET_PROP_HANDLER(MAC_BLACKLIST_ENABLED);
    NCP_GET_PROP_HANDLER(MAC_BLACKLIST);
    NCP_SET_PROP_HANDLER(MAC_BLACKLIST);
    NCP_INSERT_PROP_HANDLER(MAC_BLACKLIST);
    NCP_REMOVE_PROP_HANDLER(MAC_BLACKLIST);
    NCP_GET_PROP_HANDLER(MAC_FIXED_RSS);
    NCP_SET_PROP_HANDLER(MAC_FIXED_RSS);
    NCP_INSERT_PROP_HANDLER(MAC_FIXED_RSS);
    NCP_REMOVE_PROP_HANDLER(MAC_FIXED_RSS);
#endif

    NCP_GET_PROP_HANDLER(NET_SAVED);
    NCP_GET_PROP_HANDLER(NET_IF_UP);
    NCP_SET_PROP_HANDLER(NET_IF_UP);
    NCP_GET_PROP_HANDLER(NET_STACK_UP);
    NCP_SET_PROP_HANDLER(NET_STACK_UP);
    NCP_GET_PROP_HANDLER(NET_ROLE);
    NCP_SET_PROP_HANDLER(NET_ROLE);
    NCP_GET_PROP_HANDLER(NET_NETWORK_NAME);
    NCP_SET_PROP_HANDLER(NET_NETWORK_NAME);
    NCP_GET_PROP_HANDLER(NET_XPANID);
    NCP_SET_PROP_HANDLER(NET_XPANID);
    NCP_GET_PROP_HANDLER(NET_MASTER_KEY);
    NCP_SET_PROP_HANDLER(NET_MASTER_KEY);
    NCP_GET_PROP_HANDLER(NET_KEY_SEQUENCE_COUNTER);
    NCP_SET_PROP_HANDLER(NET_KEY_SEQUENCE_COUNTER);
    NCP_GET_PROP_HANDLER(NET_PARTITION_ID);
    NCP_GET_PROP_HANDLER(NET_KEY_SWITCH_GUARDTIME);
    NCP_SET_PROP_HANDLER(NET_KEY_SWITCH_GUARDTIME);

    NCP_GET_PROP_HANDLER(IPV6_ML_PREFIX);
    NCP_SET_PROP_HANDLER(IPV6_ML_PREFIX);
    NCP_GET_PROP_HANDLER(IPV6_ML_ADDR);
    NCP_GET_PROP_HANDLER(IPV6_LL_ADDR);
    NCP_GET_PROP_HANDLER(IPV6_ADDRESS_TABLE);
    NCP_INSERT_PROP_HANDLER(IPV6_ADDRESS_TABLE);
    NCP_REMOVE_PROP_HANDLER(IPV6_ADDRESS_TABLE);
    NCP_GET_PROP_HANDLER(IPV6_ROUTE_TABLE);
    NCP_GET_PROP_HANDLER(IPV6_ICMP_PING_OFFLOAD);
    NCP_SET_PROP_HANDLER(IPV6_ICMP_PING_OFFLOAD);
    NCP_GET_PROP_HANDLER(IPV6_MULTICAST_ADDRESS_TABLE);
    NCP_INSERT_PROP_HANDLER(IPV6_MULTICAST_ADDRESS_TABLE);
    NCP_REMOVE_PROP_HANDLER(IPV6_MULTICAST_ADDRESS_TABLE);

    NCP_GET_PROP_HANDLER(THREAD_LEADER);
    NCP_GET_PROP_HANDLER(THREAD_RLOC16_DEBUG_PASSTHRU);
    NCP_SET_PROP_HANDLER(THREAD_RLOC16_DEBUG_PASSTHRU);
    NCP_GET_PROP_HANDLER(THREAD_OFF_MESH_ROUTES);
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_INSERT_PROP_HANDLER(THREAD_OFF_MESH_ROUTES);
    NCP_REMOVE_PROP_HANDLER(THREAD_OFF_MESH_ROUTES);
#endif
    NCP_GET_PROP_HANDLER(THREAD_ON_MESH_NETS);
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_INSERT_PROP_HANDLER(THREAD_ON_MESH_NETS);
    NCP_REMOVE_PROP_HANDLER(THREAD_ON_MESH_NETS);
#endif
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
    NCP_GET_PROP_HANDLER(THREAD_ASSISTING_PORTS);
    NCP_SET_PROP_HANDLER(THREAD_ASSISTING_PORTS);
    NCP_INSERT_PROP_HANDLER(THREAD_ASSISTING_PORTS);
    NCP_REMOVE_PROP_HANDLER(THREAD_ASSISTING_PORTS);
    NCP_GET_PROP_HANDLER(THREAD_ALLOW_LOCAL_NET_DATA_CHANGE);
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_SET_PROP_HANDLER(THREAD_ALLOW_LOCAL_NET_DATA_CHANGE);
#endif
    NCP_GET_PROP_HANDLER(THREAD_MODE);
    NCP_SET_PROP_HANDLER(THREAD_MODE);
    NCP_GET_PROP_HANDLER(THREAD_CHILD_TIMEOUT);
    NCP_GET_PROP_HANDLER(THREAD_RLOC16);
    NCP_GET_PROP_HANDLER(NET_REQUIRE_JOIN_EXISTING);
    NCP_SET_PROP_HANDLER(NET_REQUIRE_JOIN_EXISTING);
    NCP_GET_PROP_HANDLER(THREAD_DISCOVERY_SCAN_JOINER_FLAG);
    NCP_SET_PROP_HANDLER(THREAD_DISCOVERY_SCAN_JOINER_FLAG);
    NCP_GET_PROP_HANDLER(THREAD_DISCOVERY_SCAN_ENABLE_FILTERING);
    NCP_SET_PROP_HANDLER(THREAD_DISCOVERY_SCAN_ENABLE_FILTERING);
    NCP_GET_PROP_HANDLER(THREAD_DISCOVERY_SCAN_PANID);
    NCP_SET_PROP_HANDLER(THREAD_DISCOVERY_SCAN_PANID);

    NCP_GET_PROP_HANDLER(CNTR_TX_PKT_TOTAL);
    NCP_GET_PROP_HANDLER(CNTR_TX_PKT_ACK_REQ);
    NCP_GET_PROP_HANDLER(CNTR_TX_PKT_ACKED);
    NCP_GET_PROP_HANDLER(CNTR_TX_PKT_NO_ACK_REQ);
    NCP_GET_PROP_HANDLER(CNTR_TX_PKT_DATA);
    NCP_GET_PROP_HANDLER(CNTR_TX_PKT_DATA_POLL);
    NCP_GET_PROP_HANDLER(CNTR_TX_PKT_BEACON);
    NCP_GET_PROP_HANDLER(CNTR_TX_PKT_BEACON_REQ);
    NCP_GET_PROP_HANDLER(CNTR_TX_PKT_OTHER);
    NCP_GET_PROP_HANDLER(CNTR_TX_PKT_RETRY);
    NCP_GET_PROP_HANDLER(CNTR_TX_PKT_UNICAST);
    NCP_GET_PROP_HANDLER(CNTR_TX_PKT_BROADCAST);
    NCP_GET_PROP_HANDLER(CNTR_TX_ERR_CCA);
    NCP_GET_PROP_HANDLER(CNTR_TX_ERR_ABORT);
    NCP_GET_PROP_HANDLER(CNTR_RX_PKT_TOTAL);
    NCP_GET_PROP_HANDLER(CNTR_RX_PKT_DATA);
    NCP_GET_PROP_HANDLER(CNTR_RX_PKT_DATA_POLL);
    NCP_GET_PROP_HANDLER(CNTR_RX_PKT_BEACON);
    NCP_GET_PROP_HANDLER(CNTR_RX_PKT_BEACON_REQ);
    NCP_GET_PROP_HANDLER(CNTR_RX_PKT_OTHER);
    NCP_GET_PROP_HANDLER(CNTR_RX_PKT_FILT_WL);
    NCP_GET_PROP_HANDLER(CNTR_RX_PKT_FILT_DA);
    NCP_GET_PROP_HANDLER(CNTR_RX_PKT_UNICAST);
    NCP_GET_PROP_HANDLER(CNTR_RX_PKT_BROADCAST);
    NCP_GET_PROP_HANDLER(CNTR_RX_ERR_EMPTY);
    NCP_GET_PROP_HANDLER(CNTR_RX_ERR_UKWN_NBR);
    NCP_GET_PROP_HANDLER(CNTR_RX_ERR_NVLD_SADDR);
    NCP_GET_PROP_HANDLER(CNTR_RX_ERR_SECURITY);
    NCP_GET_PROP_HANDLER(CNTR_RX_ERR_BAD_FCS);
    NCP_GET_PROP_HANDLER(CNTR_RX_ERR_OTHER);
    NCP_GET_PROP_HANDLER(CNTR_RX_PKT_DUP);
    NCP_GET_PROP_HANDLER(CNTR_TX_IP_SEC_TOTAL);
    NCP_GET_PROP_HANDLER(CNTR_TX_IP_INSEC_TOTAL);
    NCP_GET_PROP_HANDLER(CNTR_TX_IP_DROPPED);
    NCP_GET_PROP_HANDLER(CNTR_RX_IP_SEC_TOTAL);
    NCP_GET_PROP_HANDLER(CNTR_RX_IP_INSEC_TOTAL);
    NCP_GET_PROP_HANDLER(CNTR_RX_IP_DROPPED);
    NCP_GET_PROP_HANDLER(CNTR_TX_SPINEL_TOTAL);
    NCP_GET_PROP_HANDLER(CNTR_RX_SPINEL_TOTAL);
    NCP_GET_PROP_HANDLER(CNTR_RX_SPINEL_OUT_OF_ORDER_TID);
    NCP_GET_PROP_HANDLER(CNTR_RX_SPINEL_ERR);
    NCP_GET_PROP_HANDLER(CNTR_IP_TX_SUCCESS);
    NCP_GET_PROP_HANDLER(CNTR_IP_RX_SUCCESS);
    NCP_GET_PROP_HANDLER(CNTR_IP_TX_FAILURE);
    NCP_GET_PROP_HANDLER(CNTR_IP_RX_FAILURE);
    NCP_SET_PROP_HANDLER(CNTR_RESET);
    NCP_GET_PROP_HANDLER(MSG_BUFFER_COUNTERS);

#if OPENTHREAD_ENABLE_JAM_DETECTION
    NCP_GET_PROP_HANDLER(JAM_DETECTED);
    NCP_GET_PROP_HANDLER(JAM_DETECT_ENABLE);
    NCP_SET_PROP_HANDLER(JAM_DETECT_ENABLE);
    NCP_GET_PROP_HANDLER(JAM_DETECT_RSSI_THRESHOLD);
    NCP_SET_PROP_HANDLER(JAM_DETECT_RSSI_THRESHOLD);
    NCP_GET_PROP_HANDLER(JAM_DETECT_WINDOW);
    NCP_SET_PROP_HANDLER(JAM_DETECT_WINDOW);
    NCP_GET_PROP_HANDLER(JAM_DETECT_BUSY);
    NCP_SET_PROP_HANDLER(JAM_DETECT_BUSY);
    NCP_GET_PROP_HANDLER(JAM_DETECT_HISTORY_BITMAP);
#endif
#if OPENTHREAD_ENABLE_LEGACY
    NCP_GET_PROP_HANDLER(NEST_LEGACY_ULA_PREFIX);
    NCP_SET_PROP_HANDLER(NEST_LEGACY_ULA_PREFIX);
    NCP_GET_PROP_HANDLER(NEST_LEGACY_LAST_NODE_JOINED);
#endif

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

    // --------------------------------------------------------------------------
    // FTD Only Properties

#if OPENTHREAD_FTD

    NCP_GET_PROP_HANDLER(NET_PSKC);
    NCP_SET_PROP_HANDLER(NET_PSKC);

    NCP_GET_PROP_HANDLER(THREAD_CHILD_TABLE);
    NCP_GET_PROP_HANDLER(THREAD_CHILD_COUNT_MAX);
    NCP_SET_PROP_HANDLER(THREAD_CHILD_COUNT_MAX);
    NCP_SET_PROP_HANDLER(THREAD_CHILD_TIMEOUT);
    NCP_GET_PROP_HANDLER(THREAD_CONTEXT_REUSE_DELAY);
    NCP_SET_PROP_HANDLER(THREAD_CONTEXT_REUSE_DELAY);
    NCP_GET_PROP_HANDLER(THREAD_LOCAL_LEADER_WEIGHT);
    NCP_SET_PROP_HANDLER(THREAD_LOCAL_LEADER_WEIGHT);
    NCP_GET_PROP_HANDLER(THREAD_NETWORK_ID_TIMEOUT);
    NCP_SET_PROP_HANDLER(THREAD_NETWORK_ID_TIMEOUT);
    NCP_GET_PROP_HANDLER(THREAD_ROUTER_ROLE_ENABLED);
    NCP_SET_PROP_HANDLER(THREAD_ROUTER_ROLE_ENABLED);
    NCP_GET_PROP_HANDLER(THREAD_ROUTER_UPGRADE_THRESHOLD);
    NCP_SET_PROP_HANDLER(THREAD_ROUTER_UPGRADE_THRESHOLD);
    NCP_GET_PROP_HANDLER(THREAD_ROUTER_DOWNGRADE_THRESHOLD);
    NCP_SET_PROP_HANDLER(THREAD_ROUTER_DOWNGRADE_THRESHOLD);
    NCP_GET_PROP_HANDLER(THREAD_ROUTER_SELECTION_JITTER);
    NCP_SET_PROP_HANDLER(THREAD_ROUTER_SELECTION_JITTER);
    NCP_SET_PROP_HANDLER(THREAD_PREFERRED_ROUTER_ID);
#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    NCP_SET_PROP_HANDLER(THREAD_STEERING_DATA);
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER
    NCP_GET_PROP_HANDLER(THREAD_COMMISSIONER_ENABLED);
    NCP_INSERT_PROP_HANDLER(THREAD_JOINERS);
#endif
#if OPENTHREAD_ENABLE_TMF_PROXY
    NCP_GET_PROP_HANDLER(THREAD_TMF_PROXY_ENABLED);
    NCP_SET_PROP_HANDLER(THREAD_TMF_PROXY_ENABLED);
    NCP_SET_PROP_HANDLER(THREAD_TMF_PROXY_STREAM);
#endif
    NCP_REMOVE_PROP_HANDLER(THREAD_ACTIVE_ROUTER_IDS);

#endif // OPENTHREAD_FTD

    // --------------------------------------------------------------------------
    // Property "set" handlers for special properties for which the spinel
    // response needs to be created from within the set handler.

    otError SetPropertyHandler_HOST_POWER_STATE(uint8_t aHeader);

#if OPENTHREAD_ENABLE_DIAG
    otError SetPropertyHandler_NEST_STREAM_MFG(uint8_t aHeader);
#endif

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    otError SetPropertyHandler_THREAD_COMMISSIONER_ENABLED(uint8_t aHeader);
#endif // OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_RAW_LINK_API
    otError SetPropertyHandler_STREAM_RAW(uint8_t aHeader);
#endif

protected:
    static NcpBase *sNcpInstance;
    static spinel_status_t ThreadErrorToSpinelStatus(otError aError);
    static uint8_t LinkFlagsToFlagByte(bool aRxOnWhenIdle, bool aSecureDataRequests, bool aDeviceType, bool aNetworkData);
    otInstance *mInstance;
    NcpFrameBuffer  mTxFrameBuffer;
    SpinelEncoder mEncoder;
    SpinelDecoder mDecoder;
    bool mHostPowerStateInProgress;

private:
    enum
    {
        kTxBufferSize = OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE,  // Tx Buffer size (used by mTxFrameBuffer).
        kInvalidScanChannel = -1,                              // Invalid scan channel.
    };

    // Command Handlers
    static const PropertyHandlerEntry mGetPropertyHandlerTable[];
    static const PropertyHandlerEntry mSetPropertyHandlerTable[];
    static const PropertyHandlerEntry mInsertPropertyHandlerTable[];
    static const PropertyHandlerEntry mRemovePropertyHandlerTable[];

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

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    otMessageQueue mMessageQueue;

    uint32_t mInboundSecureIpFrameCounter;     // Number of secure inbound data/IP frames.
    uint32_t mInboundInsecureIpFrameCounter;   // Number of insecure inbound data/IP frames.
    uint32_t mOutboundSecureIpFrameCounter;    // Number of secure outbound data/IP frames.
    uint32_t mOutboundInsecureIpFrameCounter;  // Number of insecure outbound data/IP frames.
    uint32_t mDroppedOutboundIpFrameCounter;   // Number of dropped outbound data/IP frames.
    uint32_t mDroppedInboundIpFrameCounter;    // Number of dropped inbound data/IP frames.
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
    uint32_t mFramingErrorCounter;             // Number of improperly formed received spinel frames.
    uint32_t mRxSpinelFrameCounter;            // Number of received (inbound) spinel frames.
    uint32_t mRxSpinelOutOfOrderTidCounter;    // Number of out of order received spinel frames (tid increase > 1).
    uint32_t mTxSpinelFrameCounter;            // Number of sent (outbound) spinel frames.


#if OPENTHREAD_ENABLE_LEGACY
    const otNcpLegacyHandlers *mLegacyHandlers;
    uint8_t mLegacyUlaPrefix[OT_NCP_LEGACY_ULA_PREFIX_LENGTH];
    otExtAddress mLegacyLastJoinedNode;
    bool mLegacyNodeDidJoin;
#endif

};

}  // namespace Ncp
}  // namespace ot

#endif  // NCP_BASE_HPP_
