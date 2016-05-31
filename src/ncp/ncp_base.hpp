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

    NcpBase();

    ThreadError Start();
    ThreadError Stop();

protected:

    virtual ThreadError OutboundFrameBegin(void) = 0;

    virtual uint16_t OutboundFrameGetRemaining(void) = 0;

    virtual ThreadError OutboundFrameFeedData(const uint8_t *frame, uint16_t frameLength) = 0;

    virtual ThreadError OutboundFrameFeedMessage(Message &message) = 0;

    virtual ThreadError OutboundFrameSend(void) = 0;

protected:

    /**
     * Called by the superclass to indicate when a frame has been received.
     */
    void HandleReceive(const uint8_t *buf, uint16_t bufLength);

    /**
     * Called by the superclass to indicate when a send has been completed.
     */
    void HandleSendDone(void);

private:

    /**
     * Trampoline for HandleDatagramFromStack().
     */
    static void HandleDatagramFromStack(void *context, Message &message);

    void HandleDatagramFromStack(Message &message);

    /**
     * Trampoline for HandleActiveScanResult().
     */
    static void HandleActiveScanResult_Jump(otActiveScanResult *result);

    void HandleActiveScanResult(otActiveScanResult *result);

    /**
     * Trampoline for RunUpdateAddressesTask().
     */
    static void RunUpdateAddressesTask(void *context);

    void RunUpdateAddressesTask(void);


    static void HandleUnicastAddressesChanged(void *context);

private:

    ThreadError OutboundFrameFeedPacked(const char *pack_format, ...);

    ThreadError OutboundFrameFeedVPacked(const char *pack_format, va_list args);

private:

    void HandleCommand(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);

    void HandleCommandPropertyGet(uint8_t header, spinel_prop_key_t key);

    void HandleCommandPropertySet(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);

    void HandleCommandPropertyInsert(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);

    void HandleCommandPropertyRemove(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);

    void SendLastStatus(uint8_t header, spinel_status_t lastStatus);

    void SendPropteryUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, const uint8_t *value_ptr,
                            uint16_t value_len);

    void SendPropteryUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, Message &message);

    void SendPropteryUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, const char *format, ...);

private:

    typedef void (NcpBase::*CommandHandlerType)(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                                uint16_t arg_len);

    typedef void (NcpBase::*GetPropertyHandlerType)(uint8_t header, spinel_prop_key_t key);

    typedef void (NcpBase::*SetPropertyHandlerType)(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                    uint16_t value_len);

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

    void CommandHandler_NOOP(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);
    void CommandHandler_RESET(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);
    void CommandHandler_PROP_VALUE_GET(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);
    void CommandHandler_PROP_VALUE_SET(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);
    void CommandHandler_PROP_VALUE_INSERT(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);
    void CommandHandler_PROP_VALUE_REMOVE(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);

    void GetPropertyHandler_LAST_STATUS(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_PROTOCOL_VERSION(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_INTERFACE_TYPE(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_VENDOR_ID(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_CAPS(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_NCP_VERSION(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_INTERFACE_COUNT(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_POWER_STATE(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_HWADDR(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_LOCK(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_PHY_ENABLED(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_PHY_FREQ(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_PHY_CHAN_SUPPORTED(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_PHY_CHAN(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_PHY_RSSI(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_MAC_SCAN_STATE(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_MAC_15_4_PANID(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_MAC_15_4_LADDR(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_MAC_15_4_SADDR(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_NET_ENABLED(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_NET_STATE(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_NET_ROLE(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_NET_NETWORK_NAME(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_NET_XPANID(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_NET_MASTER_KEY(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_NET_KEY_SEQUENCE(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_NET_PARTITION_ID(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_THREAD_LEADER(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_IPV6_ML_PREFIX(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_IPV6_ML_ADDR(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_IPV6_LL_ADDR(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_IPV6_ROUTE_TABLE(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_THREAD_LOCAL_ROUTES(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_STREAM_NET(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_MAC_SCAN_MASK(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_MAC_SCAN_PERIOD(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_THREAD_LEADER_ADDR(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_THREAD_LEADER_RID(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_THREAD_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_THREAD_NETWORK_DATA(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_THREAD_NETWORK_DATA_VERSION(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_THREAD_STABLE_NETWORK_DATA(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_THREAD_STABLE_NETWORK_DATA_VERSION(uint8_t header, spinel_prop_key_t key);
    void GetPropertyHandler_MAC_FILTER_MODE(uint8_t header, spinel_prop_key_t key);

    void SetPropertyHandler_POWER_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                        uint16_t value_len);
    void SetPropertyHandler_PHY_TX_POWER(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                         uint16_t value_len);
    void SetPropertyHandler_PHY_CHAN(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);
    void SetPropertyHandler_MAC_SCAN_MASK(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                          uint16_t value_len);
    void SetPropertyHandler_MAC_SCAN_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                           uint16_t value_len);
    void SetPropertyHandler_MAC_15_4_PANID(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                           uint16_t value_len);
    void SetPropertyHandler_NET_ENABLED(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                        uint16_t value_len);
    void SetPropertyHandler_NET_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);
    void SetPropertyHandler_NET_ROLE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);
    void SetPropertyHandler_NET_NETWORK_NAME(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                             uint16_t value_len);
    void SetPropertyHandler_NET_XPANID(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);
    void SetPropertyHandler_NET_MASTER_KEY(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                           uint16_t value_len);
    void SetPropertyHandler_NET_KEY_SEQUENCE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                             uint16_t value_len);
    void SetPropertyHandler_STREAM_NET_INSECURE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len);
    void SetPropertyHandler_STREAM_NET(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);
    void SetPropertyHandler_IPV6_ML_PREFIX(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                           uint16_t value_len);
    void SetPropertyHandler_PHY_ENABLED(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                        uint16_t value_len);
    void SetPropertyHandler_MAC_FILTER_MODE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                     uint16_t value_len);
    void SetPropertyHandler_MAC_SCAN_PERIOD(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                            uint16_t value_len);
    void SetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                       uint16_t value_len);

    void InsertPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                  uint16_t value_len);
    void InsertPropertyHandler_THREAD_LOCAL_ROUTES(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len);
    void InsertPropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len);

    void RemovePropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                  uint16_t value_len);
    void RemovePropertyHandler_THREAD_LOCAL_ROUTES(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len);
    void RemovePropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len);

private:

    Ip6::NetifHandler mNetifHandler;

    spinel_status_t mLastStatus;

    uint32_t mChannelMask;

    uint8_t mQueuedGetHeader;

    uint16_t mScanPeriod;

    spinel_prop_key_t mQueuedGetKey;

    Tasklet mUpdateAddressesTask;

    MessageQueue mSendQueue;

protected:
    /**
     * Set to true when there is a send in progress. Set and cleared
     * by the superclass. Should be considered read-only by everyone
     * except the superclass!
     */
    bool mSending;

};

}  // namespace Thread

#endif  // NCP_BASE_HPP_
