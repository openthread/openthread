/*
 *  Copyright (c) 2022, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must strain the above copyright
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

#include "power.hpp"
#include "common/code_utils.hpp"
#include "utils/parse_cmdline.hpp"

namespace ot {
namespace Power {

otError Domain::Set(const char *aDomain)
{
    otError  error  = OT_ERROR_NONE;
    uint16_t length = static_cast<uint16_t>(strlen(aDomain));

    VerifyOrExit(length <= kDomainSize, error = OT_ERROR_INVALID_ARGS);
    memcpy(m8, aDomain, length);
    m8[length] = '\0';

exit:
    return error;
}

otError TargetPower::FromString(char *aString)
{
    otError error = OT_ERROR_NONE;
    char   *str;
    char   *psave;

    VerifyOrExit((str = strtok_r(aString, ",", &psave)) != nullptr, error = OT_ERROR_PARSE);
    SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(str, mChannelStart));

    VerifyOrExit((str = strtok_r(nullptr, ",", &psave)) != nullptr, error = OT_ERROR_PARSE);
    SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(str, mChannelEnd));

    VerifyOrExit((str = strtok_r(nullptr, ",", &psave)) != nullptr, error = OT_ERROR_PARSE);
    SuccessOrExit(error = Utils::CmdLineParser::ParseAsInt16(str, mTargetPower));

exit:
    return error;
}

TargetPower::InfoString TargetPower::ToString(void) const
{
    InfoString string;

    string.Append("%u,%u,%d", mChannelStart, mChannelEnd, mTargetPower);

    return string;
}

otError RawPowerSetting::Set(const char *aRawPowerSetting)
{
    otError  error;
    uint16_t length = sizeof(mData);

    SuccessOrExit(error = ot::Utils::CmdLineParser::ParseAsHexString(aRawPowerSetting, length, mData));
    mLength = static_cast<uint8_t>(length);

exit:
    return error;
}

RawPowerSetting::InfoString RawPowerSetting::ToString(void) const
{
    InfoString string;

    string.AppendHexBytes(mData, mLength);

    return string;
}

otError CalibratedPower::FromString(char *aString)
{
    otError error = OT_ERROR_NONE;
    char   *str;
    char   *pSave;

    VerifyOrExit((str = strtok_r(aString, ",", &pSave)) != nullptr, error = OT_ERROR_PARSE);
    SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(str, mChannelStart));

    VerifyOrExit((str = strtok_r(nullptr, ",", &pSave)) != nullptr, error = OT_ERROR_PARSE);
    SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(str, mChannelEnd));

    VerifyOrExit((str = strtok_r(nullptr, ",", &pSave)) != nullptr, error = OT_ERROR_PARSE);
    SuccessOrExit(error = Utils::CmdLineParser::ParseAsInt16(str, mActualPower));
    SuccessOrExit(error = mRawPowerSetting.Set(pSave));

exit:
    return error;
}

CalibratedPower::InfoString CalibratedPower::ToString(void) const
{
    InfoString string;

    string.Append("%u,%u,%d,%s", mChannelStart, mChannelEnd, mActualPower, mRawPowerSetting.ToString().AsCString());

    return string;
}
} // namespace Power
} // namespace ot
