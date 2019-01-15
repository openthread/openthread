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

#include "openthread-core-config.h"

#include "coap/coap.hpp"
#include "common/locator.hpp"
#include "common/timer.hpp"
#include "mac/channel_mask.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/dataset_local.hpp"
#include "net/udp6.hpp"

namespace ot {

namespace MeshCoP {

class DatasetManager : public InstanceLocator
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
     * This method restores the Operational Dataset from non-volatile memory.
     *
     * @retval OT_ERROR_NONE       Successfully restore the dataset.
     * @retval OT_ERROR_NOT_FOUND  There is no corresponding dataset stored in non-volatile memory.
     *
     */
    otError Restore(void);

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
     * This method retrieves the dataset from non-volatile memory.
     *
     * @param[out]  aDataset  Where to place the dataset.
     *
     * @retval OT_ERROR_NONE       Successfully retrieved the dataset.
     * @retval OT_ERROR_NOT_FOUND  There is no corresponding dataset stored in non-volatile memory.
     *
     */
    otError Get(Dataset &aDataset) const { return mLocal.Get(aDataset); }

    /**
     * This method retrieves the dataset from non-volatile memory.
     *
     * @param[out]  aDataset  Where to place the dataset.
     *
     * @retval OT_ERROR_NONE       Successfully retrieved the dataset.
     * @retval OT_ERROR_NOT_FOUND  There is no corresponding dataset stored in non-volatile memory.
     *
     */
    otError Get(otOperationalDataset &aDataset) const { return mLocal.Get(aDataset); }

    /**
     * This method retrieves the channel mask from local dataset.
     *
     * @param[out]  aChannelMask  A reference to the channel mask.
     *
     * @retval OT_ERROR_NONE       Successfully retrieved the channel mask.
     * @retval OT_ERROR_NOT_FOUND  There is no valid channel mask stored in local dataset.
     *
     */
    otError GetChannelMask(Mac::ChannelMask &aChannelMask) const;

    /**
     * This method applies the Active or Pending Dataset to the Thread interface.
     *
     * @retval OT_ERROR_NONE   Successfully applied configuration.
     * @retval OT_ERROR_PARSE  The dataset has at least one TLV with invalid format.
     *
     */
    otError ApplyConfiguration(void) const;

    /**
     * This method updates the Operational Dataset when detaching from the network.
     *
     * On detach, the Operational Dataset is restored from non-volatile memory.
     *
     */
    void HandleDetach(void);

    /**
     * This method sends a MGMT_SET request to the Leader.
     *
     * @param[in]  aDataset  The Operational Datset.
     * @param[in]  aTlvs     Any additional raw TLVs to include.
     * @param[in]  aLength   Number of bytes in @p aTlvs.
     *
     * @retval OT_ERROR_NONE     Successfully send the meshcop dataset command.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space to send.
     *
     */
    otError SendSetRequest(const otOperationalDataset &aDataset, const uint8_t *aTlvs, uint8_t aLength);

    /**
     * This method sends a MGMT_GET request.
     *
     * @param[in]  aDatasetComponents  An Operational Dataset components structure specifying components to request.
     * @param[in]  aTlvTypes           A pointer to array containing additional raw TLV types to be requested.
     * @param[in]  aLength             Number of bytes in @p aTlvTypes.
     * @param[in]  aAddress            The IPv6 destination address for the MGMT_GET request.
     *
     * @retval OT_ERROR_NONE     Successfully send the meshcop dataset command.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space to send.
     *
     */
    otError SendGetRequest(const otOperationalDatasetComponents &aDatasetComponents,
                           const uint8_t *                       aTlvTypes,
                           uint8_t                               aLength,
                           const otIp6Address *                  aAddress) const;

protected:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance      A reference to the OpenThread instance.
     * @param[in]  aType          Identifies Active or Pending Operational Dataset.
     * @param[in]  aUriGet        The URI-PATH for getting the Operational Dataset.
     * @param[in]  aUriSet        The URI-PATH for setting the Operational Dataset.
     * @param[in]  aTimerHandler  The registration timer handler.
     *
     */
    DatasetManager(Instance &          aInstance,
                   const Tlv::Type     aType,
                   const char *        aUriGet,
                   const char *        aUriSet,
                   TimerMilli::Handler aTimerHandler);

    /**
     * This method clears the Operational Dataset.
     *
     */
    void Clear(void);

    /**
     * This method sets the Operational Dataset in non-volatile memory.
     *
     * @param[in]  aDataset  The Operational Dataset.
     *
     * @retval OT_ERROR_NONE   Successfully applied configuration.
     * @retval OT_ERROR_PARSE  The dataset has at least one TLV with invalid format.
     *
     */
    otError Set(const Dataset &aDataset);

    /**
     * This method sets the Operational Dataset in non-volatile memory.
     *
     * @param[in]  aDataset  The Operational Dataset.
     *
     */
    otError Set(const otOperationalDataset &aDataset);

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
     * @param[in]  aMessageInfo  The message info.
     *
     */
    void Get(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const;

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
    Timestamp    mTimestamp;
    bool         mTimestampValid : 1;

private:
    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleUdpReceive(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    otError Register(void);
    void    SendGetResponse(const Coap::Message &   aRequest,
                            const Ip6::MessageInfo &aMessageInfo,
                            uint8_t *               aTlvs,
                            uint8_t                 aLength) const;

    enum
    {
        kMaxDatasetTlvs = 16, // Maximum number of TLVs in an `otOperationalDataset`.
    };

    TimerMilli mTimer;

    const char *mUriGet;
    const char *mUriSet;

#if OPENTHREAD_FTD
public:
    /**
     * This method appends the MLE Dataset TLV but excluding MeshCoP Sub Timestamp TLV.
     *
     * @retval OT_ERROR_NONE     Successfully append MLE Dataset TLV without MeshCoP Sub Timestamp TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to append the message with MLE Dataset TLV.
     *
     */
    otError AppendMleDatasetTlv(Message &aMessage) const;

protected:
    /**
     * This method handles the MGMT_SET request message.
     *
     * @param[in]  aHeader       The CoAP header.
     * @param[in]  aMessage      The CoAP message buffer.
     * @param[in]  aMessageInfo  The message info.
     *
     * @retval OT_ERROR_NONE  The MGMT_SET request message was handled successfully.
     * @retval OT_ERROR_DROP  The MGMT_SET request message was dropped.
     *
     */
    otError Set(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

private:
    void SendSetResponse(const Coap::Message &aRequest, const Ip6::MessageInfo &aMessageInfo, StateTlv::State aState);
#endif // OPENTHREAD_FTD
};

class ActiveDataset : public DatasetManager
{
public:
    /**
     * Constructor.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit ActiveDataset(Instance &aInstance);

    /**
     * This method clears the Active Operational Dataset.
     *
     */
    void Clear(void);

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

    /**
     * This method sets the Operational Dataset in non-volatile memory.
     *
     * @param[in]  aDataset  The Operational Dataset.
     *
     */
    otError Set(const otOperationalDataset &aDataset);

#if OPENTHREAD_FTD

    /**
     * This method starts the Leader functions for maintaining the Active Operational Dataset.
     *
     */
    void StartLeader(void);

    /**
     * This method stops the Leader functions for maintaining the Active Operational Dataset.
     *
     */
    void StopLeader(void);

    /**
     * This method generate a default Active Operational Dataset.
     *
     * @retval OT_ERROR_NONE  Successfully generated an Active Operational Dataset.
     * @retval OT_ERROR_INVALID_STATE  Device is not currently attached to a network.
     *
     */
    otError GenerateLocal(void);
#endif

private:
    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void) { DatasetManager::HandleTimer(); }

    static void HandleGet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleGet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const;

#if OPENTHREAD_FTD
    static void HandleSet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    bool IsTlvInitialized(Tlv::Type aType);
#endif

    Coap::Resource mResourceGet;

#if OPENTHREAD_FTD
    Coap::Resource mResourceSet;
#endif
};

class PendingDataset : public DatasetManager
{
public:
    /**
     * Constructor.
     *
     * @param[in]  The Thread network interface.
     *
     */
    explicit PendingDataset(Instance &aInstance);

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
     * This method sets the Operational Dataset in non-volatile memory.
     *
     * This method also starts the Delay Timer.
     *
     * @param[in]  aDataset  The Operational Dataset.
     *
     */
    otError Set(const otOperationalDataset &aDataset);

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

#if OPENTHREAD_FTD

    /**
     * This method starts the Leader functions for maintaining the Active Operational Dataset.
     *
     */
    void StartLeader(void);

    /**
     * This method stops the Leader functions for maintaining the Active Operational Dataset.
     *
     */
    void StopLeader(void);

    /**
     * This method generates a Pending Dataset from an Active Dataset.
     *
     * @param[in]  aTimestamp  The Active Dataset Timestamp.
     * @param[in]  aMessage    The MGMT_SET message that contains an Active Dataset.
     *
     */
    void ApplyActiveDataset(const Timestamp &aTimestamp, Coap::Message &aMessage);
#endif

private:
    void StartDelayTimer(void);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void) { DatasetManager::HandleTimer(); }

    static void HandleDelayTimer(Timer &aTimer);
    void        HandleDelayTimer(void);

    static void HandleGet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleGet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const;

#if OPENTHREAD_FTD
    static void HandleSet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
#endif

    TimerMilli mDelayTimer;

    Coap::Resource mResourceGet;

#if OPENTHREAD_FTD
    Coap::Resource mResourceSet;
#endif
};

} // namespace MeshCoP
} // namespace ot

#endif // MESHCOP_DATASET_MANAGER_HPP_
