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
 *   This file implements the vendor information.
 */

#include "vendor_info.hpp"

#include "instance/instance.hpp"

namespace ot {

//----------------------------------------------------------------------------------------------------------------------
// VendorInfo

const char VendorInfo::kName[]      = OPENTHREAD_CONFIG_NET_DIAG_VENDOR_NAME;
const char VendorInfo::kModel[]     = OPENTHREAD_CONFIG_NET_DIAG_VENDOR_MODEL;
const char VendorInfo::kSwVersion[] = OPENTHREAD_CONFIG_NET_DIAG_VENDOR_SW_VERSION;
const char VendorInfo::kAppUrl[]    = OPENTHREAD_CONFIG_NET_DIAG_VENDOR_APP_URL;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
static constexpr char kNamePrefix[] = "RD:";

static_assert(CheckConstStringPrefix(OPENTHREAD_CONFIG_NET_DIAG_VENDOR_NAME, kNamePrefix),
              "VENDOR_NAME MUST start with 'RD:' prefix for a reference device.");
#endif

static constexpr uint64_t kDefaultOuiConfig = OPENTHREAD_CONFIG_NET_DIAG_VENDOR_OUI;

const otThreadVendorOui VendorInfo::kOui = {
    OuiParser<kDefaultOuiConfig>::GetBitLength(),
    {
        OuiParser<kDefaultOuiConfig>::GetByte0(),
        OuiParser<kDefaultOuiConfig>::GetByte1(),
        OuiParser<kDefaultOuiConfig>::GetByte2(),
        OuiParser<kDefaultOuiConfig>::GetByte3(),
        OuiParser<kDefaultOuiConfig>::GetByte4(),
    },
};

VendorInfo::VendorInfo(Instance &aInstance)
    : InstanceLocator(aInstance)
{
    static_assert(sizeof(kName) <= sizeof(NameStringType), "VENDOR_NAME is too long");
    static_assert(sizeof(kModel) <= sizeof(ModelStringType), "VENDOR_MODEL is too long");
    static_assert(sizeof(kSwVersion) <= sizeof(SwVersionStringType), "VENDOR_SW_VERSION is too long");
    static_assert(sizeof(kAppUrl) <= sizeof(AppUrlStringType), "VENDOR_APP_URL is too long");

    static_assert(OuiParser<OPENTHREAD_CONFIG_NET_DIAG_VENDOR_OUI>::IsValid(),
                  "VENDOR_OUI format is not valid - see the config documentation");

#if OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE
    memcpy(mName, kName, sizeof(kName));
    memcpy(mModel, kModel, sizeof(kModel));
    memcpy(mSwVersion, kSwVersion, sizeof(kSwVersion));
    memcpy(mAppUrl, kAppUrl, sizeof(kAppUrl));
    mOui = AsCoreType(&kOui);
#endif
}

#if OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE

Error VendorInfo::SetName(const char *aName)
{
    Error error = kErrorNone;

    VerifyOrExit(!StringMatch(mName, (aName == nullptr) ? "" : aName));

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    VerifyOrExit(aName != nullptr && StringStartsWith(aName, kNamePrefix), error = kErrorInvalidArgs);
#endif

    SuccessOrExit(error = StringCopy(mName, aName, kStringCheckUtf8Encoding));

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    Get<MeshCoP::BorderAgent::TxtData>().HandleVendorNameChange();
#endif

exit:
    return error;
}

Error VendorInfo::SetModel(const char *aModel)
{
    Error error = kErrorNone;

    VerifyOrExit(!StringMatch(mModel, (aModel == nullptr) ? "" : aModel));

    SuccessOrExit(error = StringCopy(mModel, aModel, kStringCheckUtf8Encoding));

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    Get<MeshCoP::BorderAgent::TxtData>().HandleVendorModelChange();
#endif

exit:
    return error;
}

Error VendorInfo::SetSwVersion(const char *aSwVersion)
{
    return StringCopy(mSwVersion, aSwVersion, kStringCheckUtf8Encoding);
}

Error VendorInfo::SetAppUrl(const char *aAppUrl) { return StringCopy(mAppUrl, aAppUrl, kStringCheckUtf8Encoding); }

Error VendorInfo::SetOui(const Oui &aOui)
{
    Error error = kErrorNone;

    VerifyOrExit(aOui.IsValid(), error = kErrorInvalidArgs);

    VerifyOrExit(mOui != aOui);
    mOui.SetFrom(aOui);

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    Get<MeshCoP::BorderAgent::TxtData>().HandleVendorOuiChange();
#endif

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE

//----------------------------------------------------------------------------------------------------------------------
// VendorInfo::Oui

const uint8_t VendorInfo::Oui::kValidBitLengths[] = {24, 28, 36};

bool VendorInfo::Oui::IsValid(void) const { return DoesArrayContain(kValidBitLengths, GetBitLength()); }

void VendorInfo::Oui::SetFrom(const Oui &aOther)
{
    *this = aOther;
    Tidy();
}

Error VendorInfo::Oui::SetBitLengthFromSize(uint16_t aSize)
{
    Error error = kErrorNone;

    VerifyOrExit(IsValueInRange<uint16_t>(aSize, kMinSize, kMaxSize), error = kErrorInvalidArgs);

    Clear();
    mBitLength = kValidBitLengths[aSize - kMinSize];

exit:
    return error;
}

Error VendorInfo::Oui::SetFrom(const uint8_t *aData, uint16_t aSize)
{
    Error error;

    SuccessOrExit(error = SetBitLengthFromSize(aSize));
    memcpy(mBytes, aData, aSize);
    Tidy();

exit:
    return error;
}

Error VendorInfo::Oui::ParseFrom(const Message &aMessage, OffsetRange aOffsetRange)
{
    Error error = kErrorParse;

    aOffsetRange.ShrinkLength(kMaxSize);
    SuccessOrExit(SetBitLengthFromSize(aOffsetRange.GetLength()));

    SuccessOrExit(error = aMessage.Read(aOffsetRange.GetOffset(), mBytes, GetSize()));
    Tidy();

exit:
    return error;
}

void VendorInfo::Oui::Tidy(void)
{
    uint8_t index;

    if (!IsValid())
    {
        Clear();
        ExitNow();
    }

    index = GetBitLength() / kBitsPerByte;

    if (GetBitLength() > index * kBitsPerByte)
    {
        mBytes[index] &= kTopNibbleMask;
        index++;
    }

    for (; index < kMaxSize; index++)
    {
        mBytes[index] = 0;
    }

exit:
    return;
}

uint32_t VendorInfo::Oui::GetAsOui24(void) const
{
    uint32_t oui24;

    if (!IsValid())
    {
        oui24 = VendorInfo::Oui::kUnspecified;
        ExitNow();
    }

    oui24 = BigEndian::ReadUint24(GetBytes());

exit:
    return oui24;
}

bool VendorInfo::Oui::operator==(const Oui &aOther) const
{
    bool isEqual = false;
    Oui  tidyOther;

    VerifyOrExit(GetBitLength() == aOther.GetBitLength());

    tidyOther = aOther;
    tidyOther.Tidy();

    VerifyOrExit(memcmp(GetBytes(), tidyOther.GetBytes(), GetSize()) == 0);
    isEqual = true;

exit:
    return isEqual;
}

VendorInfo::Oui::InfoString VendorInfo::Oui::ToString(void) const
{
    InfoString string;

    ToString(string);

    return string;
}

void VendorInfo::Oui::ToString(char *aBuffer, uint16_t aSize) const
{
    StringWriter writer(aBuffer, aSize);
    ToString(writer);
}

void VendorInfo::Oui::ToString(StringWriter &aWriter) const
{
    uint8_t index        = 0;
    uint8_t numFullBytes = GetBitLength() / kBitsPerByte;

    if (!IsValid())
    {
        aWriter.Append("unspecified");
        ExitNow();
    }

    for (index = 0; index < numFullBytes; index++)
    {
        if (index != 0)
        {
            aWriter.Append("-");
        }

        aWriter.Append("%02X", mBytes[index]);
    }

    if (GetBitLength() > numFullBytes * kBitsPerByte)
    {
        aWriter.Append("-%1X", ReadBits<uint8_t, kTopNibbleMask>(mBytes[index]));
    }

exit:
    return;
}

} // namespace ot
