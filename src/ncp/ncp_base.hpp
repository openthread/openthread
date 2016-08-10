/*
 *    Copyright (c) 2016, Nest Labs, Inc.
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

#include <openthread-types.h>
#include <common/message.hpp>
#include <thread/thread_netif.hpp>

#include "spinel.h"

namespace Thread {

class NcpBase
{
public:

    /**
     * This constructor creates and initializes an NcpBase instance.
     *
     */
    NcpBase(void);

protected:

    /**
     * This method is called to start a new outbound frame.
     *
     * @retval kThreadError_None      Successfully started a new frame.
     * @retval kThreadError_NoBufs    Insufficient buffer space available to start a new frame.
     *
     */
    virtual ThreadError OutboundFrameBegin(void) = 0;

    /**
     * This method adds data to the current outbound frame being written.
     *
     * If no buffer space is available, this method should discard and clear the frame before returning an error status.
     *
     * @param[in]  aDataBuffer        A pointer to data buffer.
     * @param[in]  aDataBufferLength  The length of the data buffer.
     *
     * @retval kThreadError_None      Successfully added new data to the frame.
     * @retval kThreadError_NoBufs    Insufficient buffer space available to add data.
     *
     */
    virtual ThreadError OutboundFrameFeedData(const uint8_t *aDataBuffer, uint16_t aDataBufferLength) = 0;

    /**
     * This method adds a message to the current outbound frame being written.
     *
     * If no buffer space is available, this method should discard and clear the frame before returning an error status.
     * In case of success, the passed-in message @aMessage should be owned by outbound buffer and should be freed
     * when either the the frame is successfully sent and removed or if the frame is discarded.
     *
     * @param[in]  aMessage         A reference to the message to be added to current frame.
     *
     * @retval kThreadError_None    Successfully added the message to the frame.
     * @retval kThreadError_NoBufs  Insufficient buffer space available to add message.
     *
     */
    virtual ThreadError OutboundFrameFeedMessage(Message &aMessage) = 0;

    /**
     * This method finalizes and sends the current outbound frame
     *
     * If no buffer space is available, this method should discard and clear the frame before returning an error status.
     *
     * @retval kThreadError_None    Successfully added the message to the frame.
     * @retval kThreadError_NoBufs  Insufficient buffer space available to add message.
     *
     */
    virtual ThreadError OutboundFrameSend(void) = 0;

protected:

    /**
     * Called by the subclass to indicate when a frame has been received.
     */
    void HandleReceive(const uint8_t *buf, uint16_t bufLength);

    /**
     * Called by the subclass to indicate when a frame was removed and some space in tx buffer is available.
     */
    void HandleSpaceAvailableInTxBuffer(void);

private:

    /**
     * Trampoline for HandleDatagramFromStack().
     */
    static void HandleDatagramFromStack(otMessage aMessage);

    void HandleDatagramFromStack(Message &aMessage);

    /**
     * Trampoline for HandleActiveScanResult().
     */
    static void HandleActiveScanResult_Jump(otActiveScanResult *result);

    void HandleActiveScanResult(otActiveScanResult *result);

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

    static void HandleNetifStateChanged(uint32_t flags, void *context);

private:

    ThreadError OutboundFrameFeedPacked(const char *pack_format, ...);

    ThreadError OutboundFrameFeedVPacked(const char *pack_format, va_list args);

private:

    ThreadError HandleCommand(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);

    ThreadError HandleCommandPropertyGet(uint8_t header, spinel_prop_key_t key);

    ThreadError HandleCommandPropertySet(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                         uint16_t value_len);

    ThreadError HandleCommandPropertyInsert(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                            uint16_t value_len);

    ThreadError HandleCommandPropertyRemove(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                            uint16_t value_len);

    ThreadError SendLastStatus(uint8_t header, spinel_status_t lastStatus);

public:

    ThreadError SendPropertyUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, const uint8_t *value_ptr,
                            uint16_t value_len);

    ThreadError SendPropertyUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, Message &message);

    ThreadError SendPropertyUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, const char *format, ...);

private:

    typedef ThreadError(NcpBase::*CommandHandlerType)(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                                      uint16_t arg_len);

    typedef ThreadError(NcpBase::*GetPropertyHandlerType)(uint8_t header, spinel_prop_key_t key);

    typedef ThreadError(NcpBase::*SetPropertyHandlerType)(uint8_t header, spinel_prop_key_t key,
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

    ThreadError CommandHandler_NOOP(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);
    ThreadError CommandHandler_RESET(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);
    ThreadError CommandHandler_PROP_VALUE_GET(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                              uint16_t arg_len);
    ThreadError CommandHandler_PROP_VALUE_SET(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                              uint16_t arg_len);
    ThreadError CommandHandler_PROP_VALUE_INSERT(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                                 uint16_t arg_len);
    ThreadError CommandHandler_PROP_VALUE_REMOVE(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                                 uint16_t arg_len);

    ThreadError GetPropertyHandler_ChannelMaskHelper(uint8_t header, spinel_prop_key_t key, uint32_t channel_mask);

    ThreadError GetPropertyHandler_LAST_STATUS(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_PROTOCOL_VERSION(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_INTERFACE_TYPE(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_VENDOR_ID(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_CAPS(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_NCP_VERSION(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_INTERFACE_COUNT(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_POWER_STATE(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_HWADDR(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_LOCK(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_PHY_ENABLED(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_PHY_FREQ(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_PHY_CHAN_SUPPORTED(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_PHY_CHAN(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_PHY_RSSI(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_MAC_SCAN_STATE(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_MAC_15_4_PANID(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_MAC_15_4_LADDR(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_MAC_15_4_SADDR(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_NET_ENABLED(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_NET_STATE(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_NET_ROLE(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_NET_NETWORK_NAME(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_NET_XPANID(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_NET_MASTER_KEY(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_NET_KEY_SEQUENCE(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_NET_PARTITION_ID(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_LEADER(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_IPV6_ML_PREFIX(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_IPV6_ML_ADDR(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_IPV6_LL_ADDR(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_IPV6_ROUTE_TABLE(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_LOCAL_ROUTES(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_STREAM_NET(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_MAC_SCAN_MASK(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_MAC_SCAN_PERIOD(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_LEADER_ADDR(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_LEADER_RID(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_NETWORK_DATA(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_NETWORK_DATA_VERSION(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_STABLE_NETWORK_DATA(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_STABLE_NETWORK_DATA_VERSION(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_MAC_FILTER_MODE(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_CNTR(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_MAC_WHITELIST_ENABLED(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_MODE(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_CHILD_TIMEOUT(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_RLOC16(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD(uint8_t header, spinel_prop_key_t key);
    ThreadError GetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY(uint8_t header, spinel_prop_key_t key);

    ThreadError SetPropertyHandler_POWER_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                               uint16_t value_len);
    ThreadError SetPropertyHandler_PHY_TX_POWER(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len);
    ThreadError SetPropertyHandler_PHY_CHAN(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                            uint16_t value_len);
    ThreadError SetPropertyHandler_MAC_SCAN_MASK(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                 uint16_t value_len);
    ThreadError SetPropertyHandler_MAC_SCAN_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                  uint16_t value_len);
    ThreadError SetPropertyHandler_MAC_15_4_PANID(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                  uint16_t value_len);
    ThreadError SetPropertyHandler_NET_ENABLED(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                               uint16_t value_len);
    ThreadError SetPropertyHandler_NET_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                             uint16_t value_len);
    ThreadError SetPropertyHandler_NET_ROLE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                            uint16_t value_len);
    ThreadError SetPropertyHandler_NET_NETWORK_NAME(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                    uint16_t value_len);
    ThreadError SetPropertyHandler_NET_XPANID(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                              uint16_t value_len);
    ThreadError SetPropertyHandler_NET_MASTER_KEY(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                  uint16_t value_len);
    ThreadError SetPropertyHandler_NET_KEY_SEQUENCE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                    uint16_t value_len);
    ThreadError SetPropertyHandler_STREAM_NET_INSECURE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                       uint16_t value_len);
    ThreadError SetPropertyHandler_STREAM_NET(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                              uint16_t value_len);
    ThreadError SetPropertyHandler_IPV6_ML_PREFIX(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                  uint16_t value_len);
    ThreadError SetPropertyHandler_PHY_ENABLED(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                               uint16_t value_len);
    ThreadError SetPropertyHandler_MAC_FILTER_MODE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len);
    ThreadError SetPropertyHandler_MAC_SCAN_PERIOD(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len);
    ThreadError SetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key,
                                                              const uint8_t *value_ptr,
                                                              uint16_t value_len);
    ThreadError SetPropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);
    ThreadError SetPropertyHandler_MAC_WHITELIST_ENABLED(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);
    ThreadError SetPropertyHandler_THREAD_MODE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);
    ThreadError SetPropertyHandler_THREAD_CHILD_TIMEOUT(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);
    ThreadError SetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);
    ThreadError SetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);


    ThreadError SetPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key,
                                                    const uint8_t *value_ptr, uint16_t value_len);
    ThreadError SetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(uint8_t header, spinel_prop_key_t key,
                                                   const uint8_t *value_ptr, uint16_t value_len);
    ThreadError SetPropertyHandler_CNTR_RESET(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                              uint16_t value_len);

    ThreadError InsertPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                         uint16_t value_len);
    ThreadError InsertPropertyHandler_THREAD_LOCAL_ROUTES(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                          uint16_t value_len);
    ThreadError InsertPropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                          uint16_t value_len);
    ThreadError InsertPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key,
                                                             const uint8_t *value_ptr,
                                                             uint16_t value_len);
    ThreadError InsertPropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);

    ThreadError RemovePropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                         uint16_t value_len);
    ThreadError RemovePropertyHandler_THREAD_LOCAL_ROUTES(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                          uint16_t value_len);
    ThreadError RemovePropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                          uint16_t value_len);
    ThreadError RemovePropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key,
                                                             const uint8_t *value_ptr,
                                                             uint16_t value_len);
    ThreadError RemovePropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);

private:
    enum
    {
        kNetifAddressListSize = 8,           // Size of mNetifAddresses array.
        kUnusedNetifAddressPrefixLen = 0xff, // Prefix len value to indicate an unused/available NetifAddress element.
    };

    spinel_status_t mLastStatus;

    uint32_t mSupportedChannelMask;

    uint32_t mChannelMask;

    uint16_t mScanPeriod;

    Tasklet mUpdateChangedPropsTask;

    uint32_t mChangedFlags;

    spinel_tid_t mDroppedReplyTid;

    uint16_t mDroppedReplyTidBitSet;

    otNetifAddress mNetifAddresses[kNetifAddressListSize];

    bool mAllowLocalNetworkDataChange;
};

}  // namespace Thread

#endif  // NCP_BASE_HPP_
