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

#include "cli.hpp"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "power.hpp"
#include "common/code_utils.hpp"

namespace ot {
namespace Fct {

const struct Cli::Command Cli::sCommands[] = {
    {"powercalibrationtable", &Cli::ProcessCalibrationTable},
    {"targetpowertable", &Cli::ProcessTargetPowerTable},
    {"regiondomaintable", &Cli::ProcessRegionDomainTable},
};

otError Cli::GetNextTargetPower(const Power::Domain &aDomain, int &aIterator, Power::TargetPower &aTargetPower)
{
    otError error = OT_ERROR_NOT_FOUND;
    char    value[kMaxValueSize];
    char   *domain;
    char   *psave;

    while (mProductConfigFile.Get(kKeyTargetPower, aIterator, value, sizeof(value)) == OT_ERROR_NONE)
    {
        if (((domain = strtok_r(value, kCommaDelimiter, &psave)) == nullptr) || (aDomain != domain))
        {
            continue;
        }

        error = aTargetPower.FromString(psave);
        break;
    }

    return error;
}

otError Cli::GetNextDomain(int &aIterator, Power::Domain &aDomain)
{
    otError error = OT_ERROR_NOT_FOUND;
    char    value[kMaxValueSize];
    char   *str;

    while (mProductConfigFile.Get(kKeyRegionDomainMapping, aIterator, value, sizeof(value)) == OT_ERROR_NONE)
    {
        if ((str = strtok(value, kCommaDelimiter)) == nullptr)
        {
            continue;
        }

        error = aDomain.Set(str);
        break;
    }

exit:
    return error;
}

otError Cli::ProcessTargetPowerTable(Utils::CmdLineParser::Arg aArgs[])
{
    otError       error    = OT_ERROR_NONE;
    int           iterator = 0;
    Power::Domain domain;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    printf("|  Domain  | ChStart |  ChEnd  | TargetPower(0.01dBm) |\r\n");
    printf("+----------+---------+---------+----------------------+\r\n");
    while (GetNextDomain(iterator, domain) == OT_ERROR_NONE)
    {
        int                iter = 0;
        Power::TargetPower targetPower;

        while (GetNextTargetPower(domain, iter, targetPower) == OT_ERROR_NONE)
        {
            printf("| %-8s | %-7d | %-7d | %-20d |\r\n", domain.AsCString(), targetPower.GetChannelStart(),
                   targetPower.GetChannelEnd(), targetPower.GetTargetPower());
        }
    }

exit:
    return error;
}

otError Cli::ProcessRegionDomainTable(Utils::CmdLineParser::Arg aArgs[])
{
    otError error    = OT_ERROR_NONE;
    int     iterator = 0;
    char    value[kMaxValueSize];
    char   *domain;
    char   *psave;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    while (mProductConfigFile.Get(kKeyRegionDomainMapping, iterator, value, sizeof(value)) == OT_ERROR_NONE)
    {
        printf("%s\r\n", value);
    }

exit:
    return error;
}

otError Cli::ParseNextCalibratedPower(char                   *aCalibratedPowerString,
                                      uint16_t                aLength,
                                      uint16_t               &aIterator,
                                      Power::CalibratedPower &aCalibratedPower)
{
    otError                    error = OT_ERROR_NONE;
    char                      *start = aCalibratedPowerString + aIterator;
    char                      *end;
    char                      *subString;
    int16_t                    actualPower;
    ot::Power::RawPowerSetting rawPowerSetting;

    VerifyOrExit(aIterator < aLength, error = OT_ERROR_PARSE);

    end = strstr(start, "/");
    if (end != nullptr)
    {
        aIterator = end - aCalibratedPowerString + 1; // +1 to skip '/'
        *end      = '\0';
    }
    else
    {
        aIterator = aLength;
        end       = aCalibratedPowerString + aLength;
    }

    subString = strstr(start, kCommaDelimiter);
    VerifyOrExit(subString != nullptr, error = OT_ERROR_PARSE);
    *subString = '\0';
    subString++;

    SuccessOrExit(error = Utils::CmdLineParser::ParseAsInt16(start, actualPower));
    aCalibratedPower.SetActualPower(actualPower);

    VerifyOrExit(subString < end, error = OT_ERROR_PARSE);
    SuccessOrExit(error = rawPowerSetting.Set(subString));
    aCalibratedPower.SetRawPowerSetting(rawPowerSetting);

exit:
    return error;
}

otError Cli::ProcessCalibrationTable(Utils::CmdLineParser::Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        int  iterator = 0;
        char value[kMaxValueSize];

        ot::Power::CalibratedPower calibratedPower;

        printf("| ChStart |  ChEnd  | ActualPower(0.01dBm) | RawPowerSetting |\r\n");
        printf("+---------+---------+----------------------+-----------------+\r\n");

        while (mFactoryConfigFile.Get(kKeyCalibratedPower, iterator, value, sizeof(value)) == OT_ERROR_NONE)
        {
            SuccessOrExit(error = calibratedPower.FromString(value));
            printf("| %-7d | %-7d | %-20d | %-15s |\r\n", calibratedPower.GetChannelStart(),
                   calibratedPower.GetChannelEnd(), calibratedPower.GetActualPower(),
                   calibratedPower.GetRawPowerSetting().ToString().AsCString());
        }
    }
    else if (aArgs[0] == "add")
    {
        constexpr uint16_t kStateSearchDomain = 0;
        constexpr uint16_t kStateSearchPower  = 1;

        uint8_t                state = kStateSearchDomain;
        char                  *subString;
        uint8_t                channel;
        Power::CalibratedPower calibratedPower;

        for (Utils::CmdLineParser::Arg *arg = &aArgs[1]; !arg->IsEmpty(); arg++)
        {
            if ((state == kStateSearchDomain) && (*arg == "-b"))
            {
                arg++;
                VerifyOrExit(!arg->IsEmpty(), error = OT_ERROR_INVALID_ARGS);

                subString = strtok(arg->GetCString(), kCommaDelimiter);
                VerifyOrExit(subString != nullptr, error = OT_ERROR_PARSE);
                SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(subString, channel));
                calibratedPower.SetChannelStart(channel);

                subString = strtok(NULL, kCommaDelimiter);
                VerifyOrExit(subString != nullptr, error = OT_ERROR_PARSE);
                SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(subString, channel));
                calibratedPower.SetChannelEnd(channel);
                VerifyOrExit(calibratedPower.GetChannelStart() <= calibratedPower.GetChannelEnd(),
                             error = OT_ERROR_INVALID_ARGS);

                state = kStateSearchPower;
            }
            else if ((state == kStateSearchPower) && (*arg == "-c"))
            {
                uint16_t length;
                uint16_t iterator = 0;

                arg++;
                VerifyOrExit(!arg->IsEmpty(), error = OT_ERROR_INVALID_ARGS);

                length = strlen(arg->GetCString());
                while (ParseNextCalibratedPower(arg->GetCString(), length, iterator, calibratedPower) == OT_ERROR_NONE)
                {
                    SuccessOrExit(
                        error = mFactoryConfigFile.Add(kKeyCalibratedPower, calibratedPower.ToString().AsCString()));
                }

                state = kStateSearchDomain;
            }
            else
            {
                error = OT_ERROR_INVALID_ARGS;
                break;
            }
        }

        if (state == kStateSearchPower)
        {
            error = OT_ERROR_INVALID_ARGS;
        }
    }
    else if (aArgs[0] == "clear")
    {
        error = mFactoryConfigFile.Clear(kKeyCalibratedPower);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

void Cli::ProcessCommand(Utils::CmdLineParser::Arg aArgs[])
{
    otError error = OT_ERROR_NOT_FOUND;
    int     i;

    for (i = 0; i < (sizeof(sCommands) / sizeof(sCommands[0])); i++)
    {
        if (strcmp(aArgs[0].GetCString(), sCommands[i].mName) == 0)
        {
            error = (this->*sCommands[i].mCommand)(aArgs + 1);
            break;
        }
    }

exit:
    AppendErrorResult(error);
}

void Cli::ProcessLine(char *aLine)
{
    const int                 kMaxArgs = 20;
    Utils::CmdLineParser::Arg args[kMaxArgs + 1];

    SuccessOrExit(ot::Utils::CmdLineParser::ParseCmd(aLine, args, kMaxArgs));
    VerifyOrExit(!args[0].IsEmpty());

    ProcessCommand(args);

exit:
    OutputPrompt();
}

void Cli::OutputPrompt(void)
{
    printf("> ");
    fflush(stdout);
}

void Cli::AppendErrorResult(otError aError)
{
    if (aError != OT_ERROR_NONE)
    {
        printf("failed\r\nstatus %#x\r\n", aError);
    }
    else
    {
        printf("Done\r\n");
    }

    fflush(stdout);
}
} // namespace Fct
} // namespace ot
