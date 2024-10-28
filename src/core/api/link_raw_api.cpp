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
 *   This file implements the OpenThread Link Raw API.
 */

#include "openthread-core-config.h"

#include <openthread/platform/time.h>

#include "instance/instance.hpp"
#include "openthread/platform/radio.h"

using namespace ot;

#if OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE

otError otLinkRawSetReceiveDone(otInstance *aInstance, otLinkRawReceiveDone aCallback)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);
#if OPENTHREAD_MTD || OPENTHREAD_FTD
    bool enable = aCallback != nullptr;

    VerifyOrExit(!instance.Get<ThreadNetif>().IsUp(), error = kErrorInvalidState);

    // In MTD/FTD build, `Mac` has already enabled sub-mac. We ensure to
    // disable/enable MAC layer when link-raw is being enabled/disabled to
    // avoid any conflict in control of radio and sub-mac between `Mac` and
    // `LinkRaw`. in RADIO build, we directly enable/disable sub-mac.

    if (!enable)
    {
        // When disabling link-raw, make sure there is no ongoing
        // transmit or scan operation. Otherwise Mac will attempt to
        // handle an unexpected "done" callback.
        VerifyOrExit(!instance.Get<Mac::SubMac>().IsTransmittingOrScanning(), error = kErrorBusy);
    }

    instance.Get<Mac::Mac>().SetEnabled(!enable);
#endif

    instance.Get<Mac::Links>().SetReceiveDone(aCallback);

#if OPENTHREAD_MTD || OPENTHREAD_FTD
exit:
#endif
    return error;
}

bool otLinkRawIsEnabled(otInstance *aInstance) { return AsCoreType(aInstance).Get<Mac::Links>().IsLinkRawEnabled(); }

otError otLinkRawSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);
    instance.Get<Mac::Links>().SetShortAddress(aShortAddress);

exit:
    return error;
}

otError otLinkRawSetAlternateShortAddress(otInstance *aInstance, otShortAddress aShortAddress)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);
    instance.Get<Mac::Links>().SetAlternateShortAddress(aShortAddress);

exit:
    return error;
}

bool otLinkRawGetPromiscuous(otInstance *aInstance) { return AsCoreType(aInstance).Get<Radio>().GetPromiscuous(); }

otError otLinkRawSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);
    instance.Get<Radio>().SetPromiscuous(aEnable);

exit:
    return error;
}

otError otLinkRawSleep(otInstance *aInstance)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);
    instance.Get<Mac::Links>().Sleep();

exit:
    return error;
}

otError otLinkRawReceive(otInstance *aInstance)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);
    instance.Get<Mac::Links>().Receive();

exit:
    return error;
}

otRadioFrame *otLinkRawGetTransmitBuffer(otInstance *aInstance)
{
    otRadioFrame *frame;
    Instance     &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), frame = nullptr);
    frame = &AsCoreType(aInstance).Get<Mac::Links>().GetTxFrame802154();

exit:
    return frame;
}

otError otLinkRawTransmit(otInstance *aInstance, otLinkRawTransmitDone aCallback)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);

    error = instance.Get<Mac::Links>().Transmit(aCallback);
exit:
    return error;
}

int8_t otLinkRawGetRssi(otInstance *aInstance) { return AsCoreType(aInstance).Get<Radio>().GetRssi(); }

otRadioCaps otLinkRawGetCaps(otInstance *aInstance) { return AsCoreType(aInstance).Get<Mac::Links>().GetCaps802154(); }

otError otLinkRawEnergyScan(otInstance             *aInstance,
                            uint8_t                 aScanChannel,
                            uint16_t                aScanDuration,
                            otLinkRawEnergyScanDone aCallback)
{
    return AsCoreType(aInstance).Get<Mac::Links>().EnergyScan(aScanChannel, aScanDuration, aCallback);
}

otError otLinkRawSrcMatchEnable(otInstance *aInstance, bool aEnable)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);

    instance.Get<Radio>().EnableSrcMatch(aEnable);

exit:
    return error;
}

otError otLinkRawSrcMatchAddShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);

    error = instance.Get<Radio>().AddSrcMatchShortEntry(aShortAddress);

exit:
    return error;
}

otError otLinkRawSrcMatchAddExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    Mac::ExtAddress address;
    Error           error    = kErrorNone;
    Instance       &instance = AsCoreType(aInstance);

    AssertPointerIsNotNull(aExtAddress);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);

    address.Set(aExtAddress->m8, Mac::ExtAddress::kReverseByteOrder);
    error = instance.Get<Radio>().AddSrcMatchExtEntry(address);

exit:
    return error;
}

otError otLinkRawSrcMatchClearShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);
    error = instance.Get<Radio>().ClearSrcMatchShortEntry(aShortAddress);

exit:
    return error;
}

otError otLinkRawSrcMatchClearExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    Mac::ExtAddress address;
    Error           error    = kErrorNone;
    Instance       &instance = AsCoreType(aInstance);

    AssertPointerIsNotNull(aExtAddress);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);

    address.Set(aExtAddress->m8, Mac::ExtAddress::kReverseByteOrder);
    error = instance.Get<Radio>().ClearSrcMatchExtEntry(address);

exit:
    return error;
}

otError otLinkRawSrcMatchClearShortEntries(otInstance *aInstance)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);

    instance.Get<Radio>().ClearSrcMatchShortEntries();

exit:
    return error;
}

otError otLinkRawSrcMatchClearExtEntries(otInstance *aInstance)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);

    instance.Get<Radio>().ClearSrcMatchExtEntries();

exit:
    return error;
}

otError otLinkRawSetMacKey(otInstance     *aInstance,
                           uint8_t         aKeyIdMode,
                           uint8_t         aKeyId,
                           const otMacKey *aPrevKey,
                           const otMacKey *aCurrKey,
                           const otMacKey *aNextKey)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);
    instance.Get<Mac::Links>().SetMacKey(aKeyIdMode, aKeyId, AsCoreType(aPrevKey), AsCoreType(aCurrKey),
                                         AsCoreType(aNextKey));
exit:
    return error;
}

otError otLinkRawSetMacFrameCounter(otInstance *aInstance, uint32_t aMacFrameCounter)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);
    instance.Get<Mac::Links>().SetMacFrameCounter(aMacFrameCounter, /* aSetIfLarger */ false);

exit:
    return error;
}

otError otLinkRawSetMacFrameCounterIfLarger(otInstance *aInstance, uint32_t aMacFrameCounter)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);
    instance.Get<Mac::Links>().SetMacFrameCounter(aMacFrameCounter, /* aSetIfLarger */ true);
exit:
    return error;
}

uint64_t otLinkRawGetRadioTime(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return otPlatTimeGet();
}

#if OPENTHREAD_RADIO

otDeviceRole otThreadGetDeviceRole(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_DEVICE_ROLE_DISABLED;
}

uint8_t otLinkGetChannel(otInstance *aInstance) { return AsCoreType(aInstance).Get<Mac::Links>().GetChannel(); }

otError otLinkSetChannel(otInstance *aInstance, uint8_t aChannel)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);
    instance.Get<Mac::Links>().SetChannel(aChannel);

exit:
    return error;
}

otPanId otLinkGetPanId(otInstance *aInstance) { return AsCoreType(aInstance).Get<Mac::Links>().GetPanId(); }

otError otLinkSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);
    instance.Get<Mac::Links>().SetPanId(aPanId);

exit:
    return error;
}

const otExtAddress *otLinkGetExtendedAddress(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<Mac::Links>().GetExtAddress();
}

otError otLinkSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);
    instance.Get<Mac::Links>().SetExtAddress(AsCoreType(aExtAddress));

exit:
    return error;
}

uint16_t otLinkGetShortAddress(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mac::Links>().GetShortAddress();
}

void otLinkGetFactoryAssignedIeeeEui64(otInstance *aInstance, otExtAddress *aEui64)
{
    AssertPointerIsNotNull(aEui64);

    otPlatRadioGetIeeeEui64(aInstance, aEui64->m8);
}

#endif // OPENTHREAD_RADIO

#endif // OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE

otError otLinkSetRxOnWhenIdle(otInstance *aInstance, bool aRxOnWhenIdle)
{
    Error error = OT_ERROR_NONE;

#if OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    VerifyOrExit(AsCoreType(aInstance).Get<Mac::Links>().IsLinkRawEnabled(), error = kErrorInvalidState);
#else
    VerifyOrExit(AsCoreType(aInstance).Get<Mac::Mac>().IsEnabled() &&
                     AsCoreType(aInstance).Get<Mle::Mle>().IsDisabled(),
                 error = kErrorInvalidState);
#endif

    AsCoreType(aInstance).Get<Mac::SubMac>().SetRxOnWhenIdle(aRxOnWhenIdle);

exit:
    return error;
}
