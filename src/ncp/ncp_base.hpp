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

    virtual ThreadError Start();
    virtual ThreadError Stop();

    virtual ThreadError OutboundFrameBegin(void) = 0;
    virtual uint16_t OutboundFrameGetRemaining(void) = 0;
    virtual ThreadError OutboundFrameFeedData(const uint8_t *frame, uint16_t frameLength) = 0;
    virtual ThreadError OutboundFrameFeedMessage(Message &message) = 0;
    virtual ThreadError OutboundFrameFeedPacked(const char *pack_format, ...);
    virtual ThreadError OutboundFrameFeedVPacked(const char *pack_format, va_list args);
    virtual ThreadError OutboundFrameSend(void) = 0;

    static void HandleReceivedDatagram(void *context, Message &message);
    void HandleReceivedDatagram(Message &message);

protected:
    static void HandleReceive(void *context, const uint8_t *buf, uint16_t bufLength);
    void HandleReceive(const uint8_t *buf, uint16_t bufLength);

    static void HandleSendDone(void *context);
    void HandleSendDone();

    bool mSending;

private:
    void HandleCommand(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len);
    void HandleCommandReset(uint8_t header);
    void HandleCommandNoop(uint8_t header);
    void HandleCommandPropertyGet(uint8_t header, spinel_prop_key_t key);
    void HandleCommandPropertySet(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);
    void HandleCommandPropertyInsert(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);
    void HandleCommandPropertyRemove(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len);

    void HandleCommandPropertyGetAddressList(uint8_t header);
    void HandleCommandPropertyGetRoutingTable(uint8_t header);

    void SendLastStatus(uint8_t header, spinel_status_t lastStatus);

    void SendPropteryUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, const uint8_t *value_ptr,
                            uint16_t value_len);
    void SendPropteryUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, Message &message);
    void SendPropteryUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, const char *format, ...);


    static void HandleActiveScanResult_Jump(otActiveScanResult *result);
    void HandleActiveScanResult(otActiveScanResult *result);
    static void HandleUnicastAddressesChanged(void *context);

    static void RunUpdateAddressesTask(void *context);

    Ip6::NetifHandler mNetifHandler;

    spinel_status_t mLastStatus;
    uint32_t mChannelMask;

    uint8_t mQueuedGetHeader;
    spinel_prop_key_t mQueuedGetKey;

    void RunUpdateAddressesTask();
    Tasklet mUpdateAddressesTask;

    void RunSendScanResultsTask();

protected:
    MessageQueue mSendQueue;
};

}  // namespace Thread

#endif  // NCP_BASE_HPP_
