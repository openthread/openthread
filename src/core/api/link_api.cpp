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
 *   This file implements the OpenThread Link API.
 */

#include "openthread-core-config.h"

#include <openthread/link.h>

#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "mac/mac.hpp"
#include "radio/radio.hpp"

using namespace ot;

uint8_t otLinkGetChannel(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);
    uint8_t   channel;

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    if (instance.Get<Mac::LinkRaw>().IsEnabled())
    {
        channel = instance.Get<Mac::LinkRaw>().GetChannel();
    }
    else
#endif
    {
        channel = instance.Get<Mac::Mac>().GetPanChannel();
    }

    return channel;
}

otError otLinkSetChannel(otInstance *aInstance, uint8_t aChannel)
{
    otError   error;
    Instance &instance = *static_cast<Instance *>(aInstance);

#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    if (instance.Get<Mac::LinkRaw>().IsEnabled())
    {
        error = instance.Get<Mac::LinkRaw>().SetChannel(aChannel);
        ExitNow();
    }
#endif

    VerifyOrExit(instance.Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = instance.Get<Mac::Mac>().SetPanChannel(aChannel));
    instance.Get<MeshCoP::ActiveDataset>().Clear();
    instance.Get<MeshCoP::PendingDataset>().Clear();

exit:
    return error;
}

uint32_t otLinkGetSupportedChannelMask(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().GetSupportedChannelMask().GetMask();
}

otError otLinkSetSupportedChannelMask(otInstance *aInstance, uint32_t aChannelMask)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(instance.Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);

    instance.Get<Mac::Mac>().SetSupportedChannelMask(static_cast<Mac::ChannelMask>(aChannelMask));

exit:
    return error;
}

const otExtAddress *otLinkGetExtendedAddress(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<Mac::Mac>().GetExtAddress();
}

otError otLinkSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    assert(aExtAddress != NULL);
    VerifyOrExit(instance.Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);

    instance.Get<Mac::Mac>().SetExtAddress(*static_cast<const Mac::ExtAddress *>(aExtAddress));

    instance.Get<Mle::MleRouter>().UpdateLinkLocalAddress();

exit:
    return error;
}

void otLinkGetFactoryAssignedIeeeEui64(otInstance *aInstance, otExtAddress *aEui64)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Radio>().GetIeeeEui64(*static_cast<Mac::ExtAddress *>(aEui64));
}

otPanId otLinkGetPanId(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().GetPanId();
}

otError otLinkSetPanId(otInstance *aInstance, otPanId aPanId)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(instance.Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);

    instance.Get<Mac::Mac>().SetPanId(aPanId);
    instance.Get<MeshCoP::ActiveDataset>().Clear();
    instance.Get<MeshCoP::PendingDataset>().Clear();

exit:
    return error;
}

uint32_t otLinkGetPollPeriod(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<DataPollSender>().GetKeepAlivePollPeriod();
}

otError otLinkSetPollPeriod(otInstance *aInstance, uint32_t aPollPeriod)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<DataPollSender>().SetExternalPollPeriod(aPollPeriod);
}

otError otLinkSendDataRequest(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<DataPollSender>().SendDataPoll();
}

otShortAddress otLinkGetShortAddress(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().GetShortAddress();
}

uint8_t otLinkGetMaxFrameRetriesDirect(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().GetMaxFrameRetriesDirect();
}

void otLinkSetMaxFrameRetriesDirect(otInstance *aInstance, uint8_t aMaxFrameRetriesDirect)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mac::Mac>().SetMaxFrameRetriesDirect(aMaxFrameRetriesDirect);
}

#if OPENTHREAD_FTD

uint8_t otLinkGetMaxFrameRetriesIndirect(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().GetMaxFrameRetriesIndirect();
}

void otLinkSetMaxFrameRetriesIndirect(otInstance *aInstance, uint8_t aMaxFrameRetriesIndirect)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mac::Mac>().SetMaxFrameRetriesIndirect(aMaxFrameRetriesIndirect);
}

#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

otMacFilterAddressMode otLinkFilterGetAddressMode(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Filter>().GetAddressMode();
}

otError otLinkFilterSetAddressMode(otInstance *aInstance, otMacFilterAddressMode aMode)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Filter>().SetAddressMode(aMode);
}

otError otLinkFilterAddAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    assert(aExtAddress != NULL);

    return instance.Get<Mac::Filter>().AddAddress(*static_cast<const Mac::ExtAddress *>(aExtAddress));
}

otError otLinkFilterRemoveAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    assert(aExtAddress != NULL);

    return instance.Get<Mac::Filter>().RemoveAddress(*static_cast<const Mac::ExtAddress *>(aExtAddress));
}

void otLinkFilterClearAddresses(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Filter>().ClearAddresses();
}

otError otLinkFilterGetNextAddress(otInstance *aInstance, otMacFilterIterator *aIterator, otMacFilterEntry *aEntry)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    assert(aIterator != NULL && aEntry != NULL);

    return instance.Get<Mac::Filter>().GetNextAddress(*aIterator, *aEntry);
}

otError otLinkFilterAddRssIn(otInstance *aInstance, const otExtAddress *aExtAddress, int8_t aRss)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Filter>().AddRssIn(static_cast<const Mac::ExtAddress *>(aExtAddress), aRss);
}

otError otLinkFilterRemoveRssIn(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Filter>().RemoveRssIn(static_cast<const Mac::ExtAddress *>(aExtAddress));
}

void otLinkFilterClearRssIn(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mac::Filter>().ClearRssIn();
}

otError otLinkFilterGetNextRssIn(otInstance *aInstance, otMacFilterIterator *aIterator, otMacFilterEntry *aEntry)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    assert(aIterator != NULL && aEntry != NULL);

    return instance.Get<Mac::Filter>().GetNextRssIn(*aIterator, *aEntry);
}

uint8_t otLinkConvertRssToLinkQuality(otInstance *aInstance, int8_t aRss)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return LinkQualityInfo::ConvertRssToLinkQuality(instance.Get<Mac::Mac>().GetNoiseFloor(), aRss);
}

int8_t otLinkConvertLinkQualityToRss(otInstance *aInstance, uint8_t aLinkQuality)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return LinkQualityInfo::ConvertLinkQualityToRss(instance.Get<Mac::Mac>().GetNoiseFloor(), aLinkQuality);
}

#endif // OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

#if OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
const uint32_t *otLinkGetTxDirectRetrySuccessHistogram(otInstance *aInstance, uint8_t *aNumberOfEntries)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().GetDirectRetrySuccessHistogram(*aNumberOfEntries);
}

const uint32_t *otLinkGetTxIndirectRetrySuccessHistogram(otInstance *aInstance, uint8_t *aNumberOfEntries)
{
    const uint32_t *histogram = NULL;

#if OPENTHREAD_FTD
    Instance &instance = *static_cast<Instance *>(aInstance);

    histogram = instance.Get<Mac::Mac>().GetIndirectRetrySuccessHistogram(*aNumberOfEntries);
#else
    OT_UNUSED_VARIABLE(aInstance);
    *aNumberOfEntries = 0;
#endif

    return histogram;
}

void otLinkResetTxRetrySuccessHistogram(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mac::Mac>().ResetRetrySuccessHistogram();
}
#endif // OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE

void otLinkSetPcapCallback(otInstance *aInstance, otLinkPcapCallback aPcapCallback, void *aCallbackContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mac::Mac>().SetPcapCallback(aPcapCallback, aCallbackContext);
}

bool otLinkIsPromiscuous(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().IsPromiscuous();
}

otError otLinkSetPromiscuous(otInstance *aInstance, bool aPromiscuous)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    // cannot enable IEEE 802.15.4 promiscuous mode if the Thread interface is enabled
    VerifyOrExit(!instance.Get<ThreadNetif>().IsUp(), error = OT_ERROR_INVALID_STATE);

    instance.Get<Mac::Mac>().SetPromiscuous(aPromiscuous);

exit:
    return error;
}

otError otLinkSetEnabled(otInstance *aInstance, bool aEnable)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    // cannot disable the link layer if the Thread interface is enabled
    VerifyOrExit(!instance.Get<ThreadNetif>().IsUp(), error = OT_ERROR_INVALID_STATE);

    instance.Get<Mac::Mac>().SetEnabled(aEnable);

exit:
    return error;
}

bool otLinkIsEnabled(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().IsEnabled();
}

const otMacCounters *otLinkGetCounters(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.Get<Mac::Mac>().GetCounters();
}

void otLinkResetCounters(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Mac::Mac>().ResetCounters();
}

otError otLinkActiveScan(otInstance *             aInstance,
                         uint32_t                 aScanChannels,
                         uint16_t                 aScanDuration,
                         otHandleActiveScanResult aCallback,
                         void *                   aCallbackContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().ActiveScan(aScanChannels, aScanDuration, aCallback, aCallbackContext);
}

bool otLinkIsActiveScanInProgress(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().IsActiveScanInProgress();
}

otError otLinkEnergyScan(otInstance *             aInstance,
                         uint32_t                 aScanChannels,
                         uint16_t                 aScanDuration,
                         otHandleEnergyScanResult aCallback,
                         void *                   aCallbackContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().EnergyScan(aScanChannels, aScanDuration, aCallback, aCallbackContext);
}

bool otLinkIsEnergyScanInProgress(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().IsEnergyScanInProgress();
}

bool otLinkIsInTransmitState(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().IsInTransmitState();
}

otError otLinkOutOfBandTransmitRequest(otInstance *aInstance, otRadioFrame *aOobFrame)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().RequestOutOfBandFrameTransmission(aOobFrame);
}

uint16_t otLinkGetCcaFailureRate(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Mac::Mac>().GetCcaFailureRate();
}
