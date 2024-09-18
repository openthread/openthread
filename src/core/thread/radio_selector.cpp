/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes implementation of radio selector (for multi radio links).
 */

#include "radio_selector.hpp"

#if OPENTHREAD_CONFIG_MULTI_RADIO

#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("RadioSelector");

// This array defines the order in which different radio link types are
// selected for message tx (direct message).
const Mac::RadioType RadioSelector::sRadioSelectionOrder[Mac::kNumRadioTypes] = {
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    Mac::kRadioTypeTrel,
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    Mac::kRadioTypeIeee802154,
#endif
};

RadioSelector::RadioSelector(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

void RadioSelector::NeighborInfo::PopulateMultiRadioInfo(MultiRadioInfo &aInfo)
{
    ClearAllBytes(aInfo);

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    if (GetSupportedRadioTypes().Contains(Mac::kRadioTypeIeee802154))
    {
        aInfo.mSupportsIeee802154         = true;
        aInfo.mIeee802154Info.mPreference = GetRadioPreference(Mac::kRadioTypeIeee802154);
    }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    if (GetSupportedRadioTypes().Contains(Mac::kRadioTypeTrel))
    {
        aInfo.mSupportsTrelUdp6         = true;
        aInfo.mTrelUdp6Info.mPreference = GetRadioPreference(Mac::kRadioTypeTrel);
    }
#endif
}

LogLevel RadioSelector::UpdatePreference(Neighbor &aNeighbor, Mac::RadioType aRadioType, int16_t aDifference)
{
    uint8_t old        = aNeighbor.GetRadioPreference(aRadioType);
    int16_t preference = static_cast<int16_t>(old);

    preference += aDifference;

    if (preference > kMaxPreference)
    {
        preference = kMaxPreference;
    }

    if (preference < kMinPreference)
    {
        preference = kMinPreference;
    }

    aNeighbor.SetRadioPreference(aRadioType, static_cast<uint8_t>(preference));

    // We check whether the update to the preference value caused it
    // to cross the threshold `kHighPreference`. Based on this we
    // return a suggested log level. If there is cross, suggest info
    // log level, otherwise debug log level.

    return ((old >= kHighPreference) != (preference >= kHighPreference)) ? kLogLevelInfo : kLogLevelDebg;
}

void RadioSelector::UpdateOnReceive(Neighbor &aNeighbor, Mac::RadioType aRadioType, bool aIsDuplicate)
{
    LogLevel logLevel = kLogLevelInfo;

    if (aNeighbor.GetSupportedRadioTypes().Contains(aRadioType))
    {
        logLevel = UpdatePreference(aNeighbor, aRadioType,
                                    aIsDuplicate ? kPreferenceChangeOnRxDuplicate : kPreferenceChangeOnRx);

        Log(logLevel, aIsDuplicate ? "UpdateOnDupRx" : "UpdateOnRx", aRadioType, aNeighbor);
    }
    else
    {
        aNeighbor.AddSupportedRadioType(aRadioType);
        aNeighbor.SetRadioPreference(aRadioType, kInitPreference);

        Log(logLevel, "NewRadio(OnRx)", aRadioType, aNeighbor);
    }
}

void RadioSelector::UpdateOnSendDone(Mac::TxFrame &aFrame, Error aTxError)
{
    LogLevel       logLevel  = kLogLevelInfo;
    Mac::RadioType radioType = aFrame.GetRadioType();
    Mac::Address   macDest;
    Neighbor      *neighbor;

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    if (radioType == Mac::kRadioTypeTrel)
    {
        // TREL radio link uses deferred ack model. We ignore
        // `SendDone` event from `Mac` layer with success status and
        // wait for deferred ack callback.
        VerifyOrExit(aTxError != kErrorNone);
    }
#endif

    VerifyOrExit(aFrame.GetAckRequest());

    IgnoreError(aFrame.GetDstAddr(macDest));
    neighbor = Get<NeighborTable>().FindNeighbor(macDest, Neighbor::kInStateAnyExceptInvalid);
    VerifyOrExit(neighbor != nullptr);

    if (neighbor->GetSupportedRadioTypes().Contains(radioType))
    {
        logLevel = UpdatePreference(
            *neighbor, radioType, (aTxError == kErrorNone) ? kPreferenceChangeOnTxSuccess : kPreferenceChangeOnTxError);

        Log(logLevel, (aTxError == kErrorNone) ? "UpdateOnTxSucc" : "UpdateOnTxErr", radioType, *neighbor);
    }
    else
    {
        VerifyOrExit(aTxError == kErrorNone);
        neighbor->AddSupportedRadioType(radioType);
        neighbor->SetRadioPreference(radioType, kInitPreference);

        Log(logLevel, "NewRadio(OnTx)", radioType, *neighbor);
    }

exit:
    return;
}

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
void RadioSelector::UpdateOnDeferredAck(Neighbor &aNeighbor, Error aTxError, bool &aAllowNeighborRemove)
{
    LogLevel logLevel = kLogLevelInfo;

    aAllowNeighborRemove = true;

    if (aNeighbor.GetSupportedRadioTypes().Contains(Mac::kRadioTypeTrel))
    {
        logLevel = UpdatePreference(aNeighbor, Mac::kRadioTypeTrel,
                                    (aTxError == kErrorNone) ? kPreferenceChangeOnDeferredAckSuccess
                                                             : kPreferenceChangeOnDeferredAckTimeout);

        Log(logLevel, (aTxError == kErrorNone) ? "UpdateOnDefAckSucc" : "UpdateOnDefAckFail", Mac::kRadioTypeTrel,
            aNeighbor);

        // In case of deferred ack timeout, we check if the neighbor
        // has any other radio link (with high preference) for future
        // tx. If it it does, we set `aAllowNeighborRemove` to `false`
        // to ensure neighbor is not removed yet.

        VerifyOrExit(aTxError != kErrorNone);

        for (Mac::RadioType radio : sRadioSelectionOrder)
        {
            if ((radio != Mac::kRadioTypeTrel) && aNeighbor.GetSupportedRadioTypes().Contains(radio) &&
                aNeighbor.GetRadioPreference(radio) >= kHighPreference)
            {
                aAllowNeighborRemove = false;
                ExitNow();
            }
        }
    }
    else
    {
        VerifyOrExit(aTxError == kErrorNone);
        aNeighbor.AddSupportedRadioType(Mac::kRadioTypeTrel);
        aNeighbor.SetRadioPreference(Mac::kRadioTypeTrel, kInitPreference);

        Log(logLevel, "NewRadio(OnDefAckSucc)", Mac::kRadioTypeTrel, aNeighbor);
    }

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

Mac::RadioType RadioSelector::Select(Mac::RadioTypes aRadioOptions, const Neighbor &aNeighbor)
{
    Mac::RadioType selectedRadio      = sRadioSelectionOrder[0];
    uint8_t        selectedPreference = 0;
    bool           found              = false;

    // Select the first radio links with preference higher than
    // threshold `kHighPreference`. The radio links are checked in the
    // order defined by the `sRadioSelectionOrder` array. If no radio
    // link has preference higher then threshold, select the one with
    // highest preference.

    for (Mac::RadioType radio : sRadioSelectionOrder)
    {
        if (aRadioOptions.Contains(radio))
        {
            uint8_t preference = aNeighbor.GetRadioPreference(radio);

            if (preference >= kHighPreference)
            {
                selectedRadio = radio;
                break;
            }

            if (!found || (selectedPreference < preference))
            {
                found              = true;
                selectedRadio      = radio;
                selectedPreference = preference;
            }
        }
    }

    return selectedRadio;
}

Mac::TxFrame &RadioSelector::SelectRadio(Message &aMessage, const Mac::Address &aMacDest, Mac::TxFrames &aTxFrames)
{
    Neighbor       *neighbor;
    Mac::RadioType  selectedRadio;
    Mac::RadioTypes selections;

    if (aMacDest.IsBroadcast() || aMacDest.IsNone())
    {
        aMessage.ClearRadioType();
        ExitNow(selections.AddAll());
    }

    // If the radio type is already set when message was created we
    // use the selected radio type. (e.g., MLE Discovery Response
    // selects the radio link from which MLE Discovery Request is
    // received.

    if (aMessage.IsRadioTypeSet())
    {
        ExitNow(selections.Add(aMessage.GetRadioType()));
    }

    neighbor = Get<NeighborTable>().FindNeighbor(aMacDest, Neighbor::kInStateAnyExceptInvalid);

    if ((neighbor == nullptr) || neighbor->GetSupportedRadioTypes().IsEmpty())
    {
        // If we do not have a corresponding neighbor or do not yet
        // know the supported radio types, we try sending on all radio
        // links in parallel. As an example, such a situation can
        // happen when recovering a non-sleepy child (sending MLE
        // Child Update Request to it) after device itself was reset.

        aMessage.ClearRadioType();
        ExitNow(selections.AddAll());
    }

    selectedRadio = Select(neighbor->GetSupportedRadioTypes(), *neighbor);
    selections.Add(selectedRadio);

    Log(kLogLevelDebg, "SelectRadio", selectedRadio, *neighbor);

    aMessage.SetRadioType(selectedRadio);

    // We (probabilistically) decide whether to probe on another radio
    // link for the current frame tx. When probing we allow the same
    // frame to be sent in parallel over multiple radio links but only
    // care about the tx outcome (ack status) on the main selected
    // radio link. This is done by setting the "required radio types"
    // (`SetRequiredRadioTypes()`) to match the main selection on
    // `aTxFrames`. We allow probe on TREL radio link if it is not
    // currently usable (thus not selected) but is/was supported by
    // the neighbor (i.e., we did rx/tx on TREL link from/to this
    // neighbor in the past). The probe process helps detect whether
    // the TREL link is usable again allowing us to switch over
    // faster.

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    if (!selections.Contains(Mac::kRadioTypeTrel) && neighbor->GetSupportedRadioTypes().Contains(Mac::kRadioTypeTrel) &&
        (Random::NonCrypto::GetUint8InRange(0, 100) < kTrelProbeProbability))
    {
        aTxFrames.SetRequiredRadioTypes(selections);
        selections.Add(Mac::kRadioTypeTrel);

        Log(kLogLevelDebg, "Probe", Mac::kRadioTypeTrel, *neighbor);
    }
#endif

exit:
    return aTxFrames.GetTxFrame(selections);
}

Mac::RadioType RadioSelector::SelectPollFrameRadio(const Neighbor &aParent)
{
    // This array defines the order in which different radio link types
    // are selected for data poll frame tx.
    static const Mac::RadioType selectionOrder[Mac::kNumRadioTypes] = {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        Mac::kRadioTypeIeee802154,
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        Mac::kRadioTypeTrel,
#endif
    };

    Mac::RadioType selection = selectionOrder[0];

    for (Mac::RadioType radio : selectionOrder)
    {
        if (aParent.GetSupportedRadioTypes().Contains(radio))
        {
            selection = radio;
            break;
        }
    }

    return selection;
}

// LCOV_EXCL_START

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void RadioSelector::Log(LogLevel        aLogLevel,
                        const char     *aActionText,
                        Mac::RadioType  aRadioType,
                        const Neighbor &aNeighbor)
{
    String<kRadioPreferenceStringSize> preferenceString;
    bool                               isFirstEntry = true;

    VerifyOrExit(Instance::GetLogLevel() >= aLogLevel);

    for (Mac::RadioType radio : sRadioSelectionOrder)
    {
        if (aNeighbor.GetSupportedRadioTypes().Contains(radio))
        {
            preferenceString.Append("%s%s:%d", isFirstEntry ? "" : " ", RadioTypeToString(radio),
                                    aNeighbor.GetRadioPreference(radio));
            isFirstEntry = false;
        }
    }

    LogAt(aLogLevel, "RadioSelector: %s %s - neighbor:[%s rloc16:0x%04x radio-pref:{%s} state:%s]", aActionText,
          RadioTypeToString(aRadioType), aNeighbor.GetExtAddress().ToString().AsCString(), aNeighbor.GetRloc16(),
          preferenceString.AsCString(), Neighbor::StateToString(aNeighbor.GetState()));

exit:
    return;
}

#else // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void RadioSelector::Log(LogLevel, const char *, Mac::RadioType, const Neighbor &) {}

#endif // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

// LCOV_EXCL_STOP

} // namespace ot

#endif // #if OPENTHREAD_CONFIG_MULTI_RADIO
