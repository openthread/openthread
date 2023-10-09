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

#ifndef CLI_H
#define CLI_H

#include "openthread-posix-config.h"

#include <stdint.h>
#include <stdio.h>

#include "config_file.hpp"
#include "power.hpp"
#include "utils/parse_cmdline.hpp"

#include <openthread/error.h>
#include <openthread/platform/radio.h>

namespace ot {
namespace Fct {

class Cli;

/**
 * Implements the factory CLI.
 *
 */
class Cli
{
public:
    Cli(void)
        : mFactoryConfigFile(OPENTHREAD_POSIX_CONFIG_FACTORY_CONFIG_FILE)
        , mProductConfigFile(OPENTHREAD_POSIX_CONFIG_PRODUCT_CONFIG_FILE)
    {
    }

    /**
     * Processes a factory command.
     *
     * @param[in]   aArgs          The arguments of command line.
     * @param[in]   aArgsLength    The number of args in @p aArgs.
     *
     */
    void ProcessCommand(Utils::CmdLineParser::Arg aArgs[]);

    /**
     * Processes the command line.
     *
     * @param[in]  aLine   A pointer to a command line string.
     *
     */
    void ProcessLine(char *aLine);

    /**
     * Outputs the prompt.
     *
     */
    void OutputPrompt(void);

private:
    static constexpr uint16_t kMaxValueSize           = 512;
    const char               *kKeyCalibratedPower     = "calibrated_power";
    const char               *kKeyTargetPower         = "target_power";
    const char               *kKeyRegionDomainMapping = "region_domain_mapping";
    const char               *kCommaDelimiter         = ",";

    struct Command
    {
        const char *mName;
        otError (Cli::*mCommand)(Utils::CmdLineParser::Arg aArgs[]);
    };

    otError ParseNextCalibratedPower(char                   *aCalibratedPowerString,
                                     uint16_t                aLength,
                                     uint16_t               &aIterator,
                                     Power::CalibratedPower &aCalibratedPower);
    otError ProcessCalibrationTable(Utils::CmdLineParser::Arg aArgs[]);
    otError ProcessTargetPowerTable(Utils::CmdLineParser::Arg aArgs[]);
    otError ProcessRegionDomainTable(Utils::CmdLineParser::Arg aArgs[]);
    otError GetNextDomain(int &aIterator, Power::Domain &aDomain);
    otError GetNextTargetPower(const Power::Domain &aDomain, int &aIterator, Power::TargetPower &aTargetPower);

    void AppendErrorResult(otError aError);

    static const struct Command sCommands[];

    ot::Posix::ConfigFile mFactoryConfigFile;
    ot::Posix::ConfigFile mProductConfigFile;
};
} // namespace Fct
} // namespace ot
#endif
