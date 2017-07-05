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

#include <openthread/types.h>

#include "coap/coap.hpp"
#include "common/locator.hpp"
#include "common/timer.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/dataset_local.hpp"
#include "net/udp6.hpp"
#include "thread/mle.hpp"
#include "thread/network_data_leader.hpp"

namespace ot {

class ThreadNetif;

namespace MeshCoP {

class DatasetManager: public ThreadNetifLocator
{
public:
    /**
     * This method returns a reference to the Timestamp.
     *
     * @returns A pointer to the Timestamp.
     *
     */
    const Timestamp *GetTimestamp(void) const;

    /**
     * This method compares @p aTimestamp to the dataset's timestamp value.
     *
     * @param[in]  aCompare  A reference to the timestamp to compare.
     *
     * @retval -1  if @p aCompare is older than this dataset.
     * @retval  0  if @p aCompare is equal to this dataset.
     * @retval  1  if @p aCompare is newer than this dataset.
     *
     */
    int Compare(const Timestamp &aTimestamp) const;

    /**
     * This method appends the MLE Dataset TLV but excluding MeshCoP Sub Timestamp TLV.
     *
     * @retval OT_ERROR_NONE     Successfully append MLE Dataset TLV without MeshCoP Sub Timestamp TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to append the message with MLE Dataset TLV.
     *
     */
    otError AppendMleDatasetTlv(Message &aMessage) const;

    /**
     * This method returns a pointer to the TLV.
     *
     * @returns A pointer to the TLV or NULL if none is found.
     *
     */
    const Tlv *GetTlv(Tlv::Type aType) const;

    /**
     * This method returns the Operational Dataset stored in non-volatile memory.
     *
     * @retval A reference to the Operational Dataset stored in non-volatile memory.
     *
     */
    DatasetLocal &GetLocal(void) { return mLocal; }

    /**
     * This method returns the Operational Dataset for the attached partition.
     *
     * When not attached to a partition, the returned dataset is the one stored in non-volatile memory.
     *
     * @retval A reference to the Operational Dataset for the attached partition.
     *
     */
    Dataset &GetNetwork(void) { return mNetwork; }

    /**
     * This method applies the Active or Pending Dataset to the Thread interface.
     *
     * @retval OT_ERROR_NONE  Successfully applied configuration.
     *
     */
    otError ApplyConfiguration(void);

protected:

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aThreadNetif   A reference to the Thread network interface.
     * @param[in]  aType          Identifies Active or Pending Operational Dataset.
     * @param[in]  aUriSet        The URI-PATH for setting the Operational Dataset.
     * @param[in]  aUriGet        The URI-PATH for getting the Operational Dataset.
     * @param[in]  aTimerHandler  The registration timer handler.
     *
     */
    DatasetManager(ThreadNetif &aThreadNetif, const Tlv::Type aType, const char *aUriSet, const char *aUriGet,
                   Timer::Handler aTimerHander);

    /**
     * This method restores the Operational Dataset from non-volatile memory.
     *
     * @retval OT_ERROR_NONE       Successfully restore the dataset.
     * @retval OT_ERROR_NOT_FOUND  There is no corresponding dataset stored in non-volatile memory.
     *
     */
    otError Restore(void);

    /**
     * This method clears the Operational Dataset.
     *
     */
    void Clear(void);

    /**
     * This method updates the Operational Dataset when detaching from the network.
     *
     * On detach, the Operational Dataset is restored from non-volatile memory.
     *
     */
    void HandleDetach(void);

    /**
     * This method sets the Operational Dataset in non-volatile memory.
     *
     * @param[in]  aDataset  The Operational Dataset.
     *
     */
    void Set(const Dataset &aDataset);

    /**
     * This method sets the Operational Dataset for the partition.
     *
     * This method also updates the non-volatile version if the partition's Operational Dataset is newer.
     *
     * @param[in]  aTimestamp  The timestamp for the Operational Dataset.
     * @param[in]  aMessage    The message buffer.
     * @param[in]  aOffset     The offset where the Operational Dataset begins.
     * @param[in]  aLength     The length of the Operational Dataset.
     *
     */
    otError Set(const Timestamp &aTimestamp, const Message &aMessage, uint16_t aOffset, uint8_t aLength);

    /**
     * This method handles a MGMT_GET request message.
     *
     * @param[in]  aHeader       The CoAP header.
     * @param[in]  aMessage      The CoAP message buffer.
     * @parma[in]  aMessageInfo  The message info.
     *
     */
    void Get(const Coap::Header &aHeader, const Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const;

    /**
     * This method compares the partition's Operational Dataset with that stored in non-volatile memory.
     *
     * If the partition's Operational Dataset is newer, the non-volatile storage is updated.
     * If the partition's Operational Dataset is older, the registration process is started.
     *
     */
    void HandleNetworkUpdate(void);

    /**
     * This method initiates a network data registration message with the Leader.
     *
     */
    void HandleTimer(void);

    DatasetLocal mLocal;
    Dataset mNetwork;

private:
    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    otError Register(void);
    void SendGetResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                         uint8_t *aTlvs, uint8_t aLength) const;

    Timer mTimer;

    const char *mUriSet;
    const char *mUriGet;

#if OPENTHREAD_FTD
public:
    /**
     * This method sends a MGMT_SET request to the Leader.
     *
     * @parma[in]  aDataset  The Operational Datset.
     * @param[in]  aTlvs     Any additional raw TLVs to include.
     * @param[in]  aLength   Number of bytes in @p aTlvs.
     *
     * @retval OT_ERROR_NONE on success.
     *
     */
    otError SendSetRequest(const otOperationalDataset &aDataset, const uint8_t *aTlvs, uint8_t aLength);

    /**
     * This method sends a MGMT_GET request.
     *
     * @param[in]  aTlvTypes  The list of TLV types to request.
     * @param[in]  aLength    Number of bytes in @p aTlvTypes.
     * @parma[in]  aAddress   The IPv6 destination address for the MGMT_GET request.
     *
     * @retval OT_ERROR_NONE on success.
     *
     */
    otError SendGetRequest(const uint8_t *aTlvTypes, uint8_t aLength, const otIp6Address *aAddress);

protected:
    /**
     * This method sets the Operational Dataset in non-volatile memory.
     *
     * @parma[in]  aDataset  The Operational Dataset.
     *
     */
    otError Set(const otOperationalDataset &aDataset);

    /**
     * This method handles the MGMT_SET request message.
     *
     * @param[in]  aHeader       The CoAP header.
     * @param[in]  aMessage      The CoAP message buffer.
     * @parma[in]  aMessageInfo  The message info.
     *
     * @retval OT_ERROR_NONE  The MGMT_SET request message was handled successfully.
     * @retval OT_ERROR_DROP  The MGMT_SET request message was dropped.
     *
     */
    otError Set(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

private:
    void SendSetResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo, StateTlv::State aState);
#endif
};

class ActiveDatasetBase: public DatasetManager
{
public:
    /**
     * Constructor.
     *
     * @param[in]  aThreadNetif  The Thread network interface.
     *
     */
    ActiveDatasetBase(ThreadNetif &aThreadNetif);

    /**
     * This method restores the Active Operational Dataset from non-volatile memory.
     *
     * This method will also configure the Thread interface using the Active Operational Dataset.
     *
     * @retval OT_ERROR_NONE       Successfully restore the dataset.
     * @retval OT_ERROR_NOT_FOUND  There is no corresponding dataset stored in non-volatile memory.
     *
     */
    otError Restore(void);

    /**
     * This method clears the Active Operational Dataset.
     *
     */
    void Clear(void);

    /**
     * This method updates the Operational Dataset when detaching from the network.
     *
     * On detach, the Operational Dataset is restored from non-volatile memory and reconfigures the Thread
     * interface.
     *
     */
    void HandleDetach(void);

#if OPENTHREAD_FTD
    /**
     * This method sets the Operational Dataset in non-volatile memory.
     *
     * @parma[in]  aDataset  The Operational Dataset.
     *
     */
    otError Set(const otOperationalDataset &aDataset);
#endif

    /**
     * This method sets the Operational Dataset in non-volatile memory.
     *
     * This method also reconfigures the Thread interface.
     *
     * @param[in]  aDataset  The Operational Dataset.
     *
     */
    void Set(const Dataset &aDataset);

    /**
     * This method sets the Operational Dataset for the partition.
     *
     * This method also reconfigures the Thread interface.
     * This method also updates the non-volatile version if the partition's Operational Dataset is newer.
     *
     * @param[in]  aTimestamp  The timestamp for the Operational Dataset.
     * @param[in]  aMessage    The message buffer.
     * @param[in]  aOffset     The offset where the Operational Dataset begins.
     * @param[in]  aLength     The length of the Operational Dataset.
     *
     */
    otError Set(const Timestamp &aTimestamp, const Message &aMessage, uint16_t aOffset, uint8_t aLength);

private:
    static void HandleGet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                          const otMessageInfo *aMessageInfo);
    void HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleTimer(Timer &aTimer);
    void HandleTimer(void) { DatasetManager::HandleTimer(); }

    Coap::Resource mResourceGet;
};

class PendingDatasetBase: public DatasetManager
{
public:
    /**
     * Constructor.
     *
     * @param[in]  The Thread network interface.
     *
     */
    PendingDatasetBase(ThreadNetif &aThreadNetif);

    /**
     * This method restores the Operational Dataset from non-volatile memory.
     *
     * @retval OT_ERROR_NONE       Successfully restore the dataset.
     * @retval OT_ERROR_NOT_FOUND  There is no corresponding dataset stored in non-volatile memory.
     *
     */
    otError Restore(void);

    /**
     * This method clears the Pending Operational Dataset.
     *
     * This method also stops the Delay Timer if it was active.
     *
     */
    void Clear(void);

    /**
     * This method clears the network Pending Operational Dataset.
     *
     * This method also stops the Delay Timer if it was active.
     *
     */
    void ClearNetwork(void);

    /**
     * This method updates the Operational Dataset when detaching from the network.
     *
     * On detach, the Operational Dataset is restored from non-volatile memory.
     *
     * This method also stops the Delay Timer if it was active.
     *
     */
    void HandleDetach(void);

#if OPENTHREAD_FTD
    /**
     * This method sets the Operational Dataset in non-volatile memory.
     *
     * This method also starts the Delay Timer.
     *
     * @parma[in]  aDataset  The Operational Dataset.
     *
     */
    otError Set(const otOperationalDataset &aDataset);
#endif

    /**
     * This method sets the Operational Dataset in non-volatile memory.
     *
     * This method also starts the Delay Timer.
     *
     * @param[in]  aDataset  The Operational Dataset.
     *
     */
    void Set(const Dataset &aDataset);

    /**
     * This method sets the Operational Dataset for the partition.
     *
     * This method also updates the non-volatile version if the partition's Operational Dataset is newer.
     *
     * This method also starts the Delay Timer.
     *
     * @param[in]  aTimestamp  The timestamp for the Operational Dataset.
     * @param[in]  aMessage    The message buffer.
     * @param[in]  aOffset     The offset where the Operational Dataset begins.
     * @param[in]  aLength     The length of the Operational Dataset.
     *
     */
    otError Set(const Timestamp &aTimestamp, const Message &aMessage, uint16_t aOffset, uint8_t aLength);

protected:
    static void HandleDelayTimer(Timer &aTimer);
    void HandleDelayTimer(void);
    void StartDelayTimer(void);
    void HandleNetworkUpdate(void);

    Timer mDelayTimer;

private:
    static void HandleGet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                          const otMessageInfo *aMessageInfo);
    void HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleTimer(Timer &aTimer);
    void HandleTimer(void) { DatasetManager::HandleTimer(); }

    Coap::Resource mResourceGet;
};

}  // namespace MeshCoP
}  // namespace ot

#if OPENTHREAD_MTD
#include "dataset_manager_mtd.hpp"
#elif OPENTHREAD_FTD
#include "dataset_manager_ftd.hpp"
#else
#error Must define OPENTHREAD_MTD or OPENTHREAD_FTD
#endif

#endif  // MESHCOP_DATASET_MANAGER_HPP_
