/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

/**
 * @file
 *   This file includes definitions for managing MeshCoP Datasets.
 *
 */

#ifndef MESHCOP_DATASET_MANAGER_HPP_
#define MESHCOP_DATASET_MANAGER_HPP_

#include <openthread-types.h>

#include <coap/coap_server.hpp>
#include <common/timer.hpp>
#include <net/udp6.hpp>
#include <thread/meshcop_dataset.hpp>
#include <thread/mle.hpp>
#include <thread/network_data_leader.hpp>

namespace Thread {

class ThreadNetif;

namespace MeshCoP {

class DatasetManager
{
public:
    void StartLeader(void);
    void StopLeader(void);

    Dataset &GetLocal(void) { return mLocal; }

    Dataset &GetNetwork(void) { return mNetwork; }

    ThreadError SendSetRequest(const otOperationalDataset &aDataset, const uint8_t *aTlvs, uint8_t aLength);
    ThreadError SendGetRequest(const uint8_t *aTlvTypes, uint8_t aLength);

protected:
    enum
    {
        kFlagLocalUpdated   = 1 << 0,
        kFlagNetworkUpdated = 1 << 1,
    };

    DatasetManager(ThreadNetif &aThreadNetif, const Tlv::Type aType, const char *aUriSet, const char *aUriGet);

    ThreadError Set(const Dataset &aDataset, uint8_t &aFlags);

    ThreadError Set(const Timestamp &aTimestamp, const Message &aMessage, uint16_t aOffset, uint8_t aLength,
                    uint8_t &aFlags);

    Dataset mLocal;
    Dataset mNetwork;

    Mle::Mle &mMle;
    ThreadNetif &mNetif;
    NetworkData::Leader &mNetworkDataLeader;

private:
    static void HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo);
    void HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleSet(void *aContext, Coap::Header &aHeader, Message &aMessage,
                          const Ip6::MessageInfo &aMessageInfo);
    void HandleSet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleGet(void *aContext, Coap::Header &aHeader, Message &aMessage,
                          const Ip6::MessageInfo &aMessageInfo);
    void HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleTimer(void *aContext);
    void HandleTimer(void);

    ThreadError Register(void);
    void SendSetResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo, StateTlv::State aState);
    void SendGetResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                         uint8_t *aTlvs, uint8_t aLength);

    Coap::Resource mResourceSet;
    Coap::Resource mResourceGet;
    Timer mTimer;

    Ip6::UdpSocket mSocket;
    uint8_t        mCoapToken[2];
    uint16_t       mCoapMessageId;

    const char *mUriSet;
    const char *mUriGet;

    Coap::Server &mCoapServer;
};

class ActiveDataset: public DatasetManager
{
public:
    ActiveDataset(ThreadNetif &aThreadNetif);

    void Get(otOperationalDataset &aDataset);

    ThreadError Set(const Dataset &aDataset);

    ThreadError Set(const otOperationalDataset &aDataset);

    ThreadError Set(const Timestamp &aTimestamp, const Message &aMessage, uint16_t aOffset, uint8_t aLength);

    ThreadError ApplyConfiguration(void);
};

class PendingDataset: public DatasetManager
{
public:
    PendingDataset(ThreadNetif &aThreadNetif);

    void StartLeader(void);

    void Get(otOperationalDataset &aDataset);

    ThreadError Set(const Dataset &aDataset);

    ThreadError Set(const otOperationalDataset &aDataset);

    ThreadError Set(const Timestamp &aTimestamp, const Message &aMessage, uint16_t aOffset, uint8_t aLength);

    void UpdateDelayTimer(void);

private:
    static void HandleTimer(void *aContext);
    void HandleTimer(void);

    void ResetDelayTimer(uint8_t aFlags);
    void UpdateDelayTimer(Dataset &aDataset, uint32_t &aStartTime);

    Timer mTimer;
    uint32_t mLocalTime;
    uint32_t mNetworkTime;
};

}  // namespace MeshCoP
}  // namespace Thread

#endif  // MESHCOP_DATASET_MANAGER_HPP_
