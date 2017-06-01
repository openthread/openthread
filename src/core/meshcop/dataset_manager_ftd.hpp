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

#ifndef MESHCOP_DATASET_MANAGER_FTD_HPP_
#define MESHCOP_DATASET_MANAGER_FTD_HPP_

#include <openthread/types.h>

#include "coap/coap.hpp"

namespace ot {

class ThreadNetif;

namespace MeshCoP {

class ActiveDataset: public ActiveDatasetBase
{
public:
    ActiveDataset(ThreadNetif &aThreadNetif);

    otError GenerateLocal(void);

    void StartLeader(void);

    void StopLeader(void);

private:
    static void HandleSet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                          const otMessageInfo *aMessageInfo);
    void HandleSet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    bool IsTlvInitialized(Tlv::Type aType);

    Coap::Resource mResourceSet;
};

class PendingDataset: public PendingDatasetBase
{
public:
    PendingDataset(ThreadNetif &aThreadNetif);

    void StartLeader(void);

    void StopLeader(void);

    void ApplyActiveDataset(const Timestamp &aTimestamp, Message &aMessage);

private:
    static void HandleSet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                          const otMessageInfo *aMessageInfo);
    void HandleSet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    Coap::Resource mResourceSet;
};

}  // namespace MeshCoP
}  // namespace ot

#endif  // MESHCOP_DATASET_MANAGER_HPP_
