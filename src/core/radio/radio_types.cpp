/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes implementation of OpenThread radio types.
 */

#include "radio_types.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace Radio {

bool IsTimeStrictlyBefore(Time32 aFirstTime, Time32 aSecondTime)
{
    Time firstTime(aFirstTime);
    Time secondTime(aSecondTime);

    return (firstTime < secondTime);
}

//---------------------------------------------------------------------------------------------------------------------
// SyncedTime

#if OT_CONFIG_RADIO_TIME_ENABLE && OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE

void SyncedTime::SetToNow(Radio &aRadio)
{
    mRadioTime = aRadio.GetNow();
    mLocalTime = TimerMicro::GetNow();
}

#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO

//---------------------------------------------------------------------------------------------------------------------
// Types

const Type Types::kAllTypes[kNumTypes] = {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    kTypeIeee802154,
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    kTypeTrel,
#endif
};

void Types::AddAll(void)
{
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    Add(kTypeIeee802154);
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    Add(kTypeTrel);
#endif
}

Types::InfoString Types::ToString(void) const
{
    InfoString string;
    bool       addComma = false;

    string.Append("{");
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    if (Contains(kTypeIeee802154))
    {
        string.Append("%s%s", addComma ? ", " : " ", TypeToString(kTypeIeee802154));
        addComma = true;
    }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    if (Contains(kTypeTrel))
    {
        string.Append("%s%s", addComma ? ", " : " ", TypeToString(kTypeTrel));
        addComma = true;
    }
#endif

    OT_UNUSED_VARIABLE(addComma);

    string.Append(" }");

    return string;
}

//---------------------------------------------------------------------------------------------------------------------

const char *TypeToString(Type aType)
{
    const char *str = "unknown";

    switch (aType)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    case kTypeIeee802154:
        str = "15.4";
        break;
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    case kTypeTrel:
        str = "trel";
        break;
#endif
    }

    return str;
}

#endif // OPENTHREAD_CONFIG_MULTI_RADIO

} // namespace Radio
} // namespace ot
