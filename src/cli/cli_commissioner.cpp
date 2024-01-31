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
 *   This file implements a simple CLI for the Commissioner role.
 */

#include "cli_commissioner.hpp"

#include "cli/cli.hpp"

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD

namespace ot {
namespace Cli {

/**
 * @cli commissioner announce
 * @code
 * commissioner announce 0x00050000 2 32 fdde:ad00:beef:0:0:ff:fe00:c00
 * Done
 * @endcode
 * @cparam commissioner announce @ca{mask} @ca{count} @ca{period} @ca{destination}
 *   * `mask`: Bitmask that identifies channels for sending MLE `Announce` messages.
 *   * `count`: Number of MLE `Announce` transmissions per channel.
 *   * `period`: Number of milliseconds between successive MLE `Announce` transmissions.
 *   * `destination`: Destination IPv6 address for the message. The message may be multicast.
 * @par
 * Sends an Announce Begin message.
 * @note Use this command only after successfully starting the %Commissioner role
 * with the `commissioner start` command.
 * @csa{commissioner start}
 * @sa otCommissionerAnnounceBegin
 */
template <> otError Commissioner::Process<Cmd("announce")>(Arg aArgs[])
{
    otError      error;
    uint32_t     mask;
    uint8_t      count;
    uint16_t     period;
    otIp6Address address;

    SuccessOrExit(error = aArgs[0].ParseAsUint32(mask));
    SuccessOrExit(error = aArgs[1].ParseAsUint8(count));
    SuccessOrExit(error = aArgs[2].ParseAsUint16(period));
    SuccessOrExit(error = aArgs[3].ParseAsIp6Address(address));

    error = otCommissionerAnnounceBegin(GetInstancePtr(), mask, count, period, &address);

exit:
    return error;
}

/**
 * @cli commissioner energy
 * @code
 * commissioner energy 0x00050000 2 32 1000 fdde:ad00:beef:0:0:ff:fe00:c00
 * Done
 * Energy: 00050000 0 0 0 0
 * @endcode
 * @cparam commissioner energy @ca{mask} @ca{count} @ca{period} @ca{scanDuration} @ca{destination}
 *   * `mask`: Bitmask that identifies channels for performing IEEE 802.15.4 energy scans.
 *   * `count`: Number of IEEE 802.15.4 energy scans per channel.
 *   * `period`: Number of milliseconds between successive IEEE 802.15.4 energy scans.
 *   * `scanDuration`: Scan duration in milliseconds to use when
 *     performing an IEEE 802.15.4 energy scan.
 *   * `destination`: Destination IPv6 address for the message. The message may be multicast.
 * @par
 * Sends an Energy Scan Query message. Command output is printed as it is received.
 * @note Use this command only after successfully starting the %Commissioner role
 * with the `commissioner start` command.
 * @csa{commissioner start}
 * @sa otCommissionerEnergyScan
 */
template <> otError Commissioner::Process<Cmd("energy")>(Arg aArgs[])
{
    otError      error;
    uint32_t     mask;
    uint8_t      count;
    uint16_t     period;
    uint16_t     scanDuration;
    otIp6Address address;

    SuccessOrExit(error = aArgs[0].ParseAsUint32(mask));
    SuccessOrExit(error = aArgs[1].ParseAsUint8(count));
    SuccessOrExit(error = aArgs[2].ParseAsUint16(period));
    SuccessOrExit(error = aArgs[3].ParseAsUint16(scanDuration));
    SuccessOrExit(error = aArgs[4].ParseAsIp6Address(address));

    error = otCommissionerEnergyScan(GetInstancePtr(), mask, count, period, scanDuration, &address,
                                     &Commissioner::HandleEnergyReport, this);

exit:
    return error;
}

template <> otError Commissioner::Process<Cmd("joiner")>(Arg aArgs[])
{
    otError             error = OT_ERROR_NONE;
    otExtAddress        addr;
    const otExtAddress *addrPtr = nullptr;
    otJoinerDiscerner   discerner;

    /**
     * @cli commissioner joiner table
     * @code
     * commissioner joiner table
     * | ID                    | PSKd                             | Expiration |
     * +-----------------------+----------------------------------+------------+
     * |                     * |                           J01NME |      81015 |
     * |      d45e64fa83f81cf7 |                           J01NME |     101204 |
     * | 0x0000000000000abc/12 |                           J01NME |     114360 |
     * Done
     * @endcode
     * @par
     * Lists all %Joiner entries in table format.
     */
    if (aArgs[0] == "table")
    {
        uint16_t     iter = 0;
        otJoinerInfo joinerInfo;

        static const char *const kJoinerTableTitles[] = {"ID", "PSKd", "Expiration"};

        static const uint8_t kJoinerTableColumnWidths[] = {
            23,
            34,
            12,
        };

        OutputTableHeader(kJoinerTableTitles, kJoinerTableColumnWidths);

        while (otCommissionerGetNextJoinerInfo(GetInstancePtr(), &iter, &joinerInfo) == OT_ERROR_NONE)
        {
            switch (joinerInfo.mType)
            {
            case OT_JOINER_INFO_TYPE_ANY:
                OutputFormat("| %21s", "*");
                break;

            case OT_JOINER_INFO_TYPE_EUI64:
                OutputFormat("|      ");
                OutputExtAddress(joinerInfo.mSharedId.mEui64);
                break;

            case OT_JOINER_INFO_TYPE_DISCERNER:
                OutputFormat("| 0x%08lx%08lx/%2u",
                             static_cast<unsigned long>(joinerInfo.mSharedId.mDiscerner.mValue >> 32),
                             static_cast<unsigned long>(joinerInfo.mSharedId.mDiscerner.mValue & 0xffffffff),
                             joinerInfo.mSharedId.mDiscerner.mLength);
                break;
            }

            OutputFormat(" | %32s | %10lu |", joinerInfo.mPskd.m8, ToUlong(joinerInfo.mExpirationTime));
            OutputNewLine();
        }

        ExitNow(error = OT_ERROR_NONE);
    }

    VerifyOrExit(!aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    memset(&discerner, 0, sizeof(discerner));

    if (aArgs[1] == "*")
    {
        // Intentionally empty
    }
    else
    {
        error = Interpreter::ParseJoinerDiscerner(aArgs[1], discerner);

        if (error == OT_ERROR_NOT_FOUND)
        {
            error   = aArgs[1].ParseAsHexString(addr.m8);
            addrPtr = &addr;
        }

        SuccessOrExit(error);
    }

    /**
     * @cli commissioner joiner add
     * @code
     * commissioner joiner add d45e64fa83f81cf7 J01NME
     * Done
     * @endcode
     * @code
     * commissioner joiner add 0xabc/12 J01NME
     * Done
     * @endcode
     * @cparam commissioner joiner add @ca{eui64}|@ca{discerner pksd} [@ca{timeout}]
     *   * `eui64`: IEEE EUI-64 of the %Joiner. To match any joiner, use `*`.
     *   * `discerner`: The %Joiner discerner in the format `number/length`.
     *   * `pksd`: Pre-Shared Key for the joiner.
     *   * `timeout`: The %Joiner timeout in seconds.
     * @par
     * Adds a joiner entry.
     * @note Use this command only after successfully starting the %Commissioner role
     * with the `commissioner start` command.
     * @csa{commissioner start}
     * @sa otCommissionerAddJoiner
     * @sa otCommissionerAddJoinerWithDiscerner
     */
    if (aArgs[0] == "add")
    {
        uint32_t timeout = kDefaultJoinerTimeout;

        VerifyOrExit(!aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        if (!aArgs[3].IsEmpty())
        {
            SuccessOrExit(error = aArgs[3].ParseAsUint32(timeout));
        }

        if (discerner.mLength)
        {
            error = otCommissionerAddJoinerWithDiscerner(GetInstancePtr(), &discerner, aArgs[2].GetCString(), timeout);
        }
        else
        {
            error = otCommissionerAddJoiner(GetInstancePtr(), addrPtr, aArgs[2].GetCString(), timeout);
        }
        /**
         * @cli commissioner joiner remove
         * @code
         * commissioner joiner remove d45e64fa83f81cf7
         * Done
         * @endcode
         * @code
         * commissioner joiner remove 0xabc/12
         * Done
         * @endcode
         * @cparam commissioner joiner remove @ca{eui64}|@ca{discerner}
         *   * `eui64`: IEEE EUI-64 of the joiner. To match any joiner, use `*`.
         *   * `discerner`: The joiner discerner in the format `number/length`.
         * @par
         * Removes a %Joiner entry.
         * @note Use this command only after successfully starting the %Commissioner role
         * with the `commissioner start` command.
         * @csa{commissioner start}
         * @sa otCommissionerRemoveJoiner
         * @sa otCommissionerRemoveJoinerWithDiscerner
         */
    }
    else if (aArgs[0] == "remove")
    {
        if (discerner.mLength)
        {
            error = otCommissionerRemoveJoinerWithDiscerner(GetInstancePtr(), &discerner);
        }
        else
        {
            error = otCommissionerRemoveJoiner(GetInstancePtr(), addrPtr);
        }
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

/**
 * @cli commissioner mgmtget
 * @code
 * commissioner mgmtget locator sessionid
 * Done
 * @endcode
 * @cparam commissioner mgmtget [locator] [sessionid] <!--
 * -->                          [steeringdata] [joinerudpport] <!--
 * -->                          [-x @ca{TLVs}]
 *   * `locator`: Border Router RLOC16.
 *   * `sessionid`: Session ID of the %Commissioner.
 *   * `steeringdata`: Steering data.
 *   * `joinerudpport`: %Joiner UDP port.
 *   * `TLVs`: The set of TLVs to be retrieved.
 * @par
 * Sends a `MGMT_GET` (Management Get) message to the Leader.
 * Variable values that have been set using the `commissioner mgmtset` command are returned.
 * @sa otCommissionerSendMgmtGet
 */
template <> otError Commissioner::Process<Cmd("mgmtget")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    uint8_t tlvs[32];
    uint8_t length = 0;

    for (; !aArgs->IsEmpty(); aArgs++)
    {
        VerifyOrExit(static_cast<size_t>(length) < sizeof(tlvs), error = OT_ERROR_NO_BUFS);

        if (*aArgs == "locator")
        {
            tlvs[length++] = OT_MESHCOP_TLV_BORDER_AGENT_RLOC;
        }
        else if (*aArgs == "sessionid")
        {
            tlvs[length++] = OT_MESHCOP_TLV_COMM_SESSION_ID;
        }
        else if (*aArgs == "steeringdata")
        {
            tlvs[length++] = OT_MESHCOP_TLV_STEERING_DATA;
        }
        else if (*aArgs == "joinerudpport")
        {
            tlvs[length++] = OT_MESHCOP_TLV_JOINER_UDP_PORT;
        }
        else if (*aArgs == "-x")
        {
            uint16_t readLength;

            aArgs++;
            readLength = static_cast<uint16_t>(sizeof(tlvs) - length);
            SuccessOrExit(error = aArgs->ParseAsHexString(readLength, tlvs + length));
            length += static_cast<uint8_t>(readLength);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    error = otCommissionerSendMgmtGet(GetInstancePtr(), tlvs, static_cast<uint8_t>(length));

exit:
    return error;
}

/**
 * @cli commissioner mgmtset
 * @code
 * commissioner mgmtset joinerudpport 9988
 * Done
 * @endcode
 * @cparam commissioner mgmtset [locator @ca{locator}] [sessionid @ca{sessionid}] <!--
 * -->                          [steeringdata @ca{steeringdata}] [joinerudpport @ca{joinerudpport}] <!--
 * -->                          [-x @ca{TLVs}]
 *   * `locator`: Border Router RLOC16.
 *   * `sessionid`: Session ID of the %Commissioner.
 *   * `steeringdata`: Steering data.
 *   * `joinerudpport`: %Joiner UDP port.
 *   * `TLVs`: The set of TLVs to be retrieved.
 * @par
 * Sends a `MGMT_SET` (Management Set) message to the Leader, and sets the
 * variables to the values specified.
 * @sa otCommissionerSendMgmtSet
 */
template <> otError Commissioner::Process<Cmd("mgmtset")>(Arg aArgs[])
{
    otError                error;
    otCommissioningDataset dataset;
    uint8_t                tlvs[32];
    uint8_t                tlvsLength = 0;

    VerifyOrExit(!aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    memset(&dataset, 0, sizeof(dataset));

    for (; !aArgs->IsEmpty(); aArgs++)
    {
        if (*aArgs == "locator")
        {
            aArgs++;
            dataset.mIsLocatorSet = true;
            SuccessOrExit(error = aArgs->ParseAsUint16(dataset.mLocator));
        }
        else if (*aArgs == "sessionid")
        {
            aArgs++;
            dataset.mIsSessionIdSet = true;
            SuccessOrExit(error = aArgs->ParseAsUint16(dataset.mSessionId));
        }
        else if (*aArgs == "steeringdata")
        {
            uint16_t length;

            aArgs++;
            dataset.mIsSteeringDataSet = true;
            length                     = sizeof(dataset.mSteeringData.m8);
            SuccessOrExit(error = aArgs->ParseAsHexString(length, dataset.mSteeringData.m8));
            dataset.mSteeringData.mLength = static_cast<uint8_t>(length);
        }
        else if (*aArgs == "joinerudpport")
        {
            aArgs++;
            dataset.mIsJoinerUdpPortSet = true;
            SuccessOrExit(error = aArgs->ParseAsUint16(dataset.mJoinerUdpPort));
        }
        else if (*aArgs == "-x")
        {
            uint16_t length;

            aArgs++;
            length = sizeof(tlvs);
            SuccessOrExit(error = aArgs->ParseAsHexString(length, tlvs));
            tlvsLength = static_cast<uint8_t>(length);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    error = otCommissionerSendMgmtSet(GetInstancePtr(), &dataset, tlvs, tlvsLength);

exit:
    return error;
}

/**
 * @cli commissioner panid
 * @code
 * commissioner panid 0xdead 0x7fff800 fdde:ad00:beef:0:0:ff:fe00:c00
 * Done
 * Conflict: dead, 00000800
 * @endcode
 * @cparam commissioner panid @ca{panid} @ca{mask} @ca{destination}
 *   * `paind`: PAN ID to use to check for conflicts.
 *   * `mask`; Bitmask that identifies channels to perform IEEE 802.15.4
 *     Active Scans.
 *   * `destination`: IPv6 destination address for the message. The message may be multicast.
 * @par
 * Sends a PAN ID query. Command output is returned as it is received.
 * @note Use this command only after successfully starting the %Commissioner role
 * with the `commissioner start` command.
 * @csa{commissioner start}
 * @sa otCommissionerPanIdQuery
 */
template <> otError Commissioner::Process<Cmd("panid")>(Arg aArgs[])
{
    otError      error;
    uint16_t     panId;
    uint32_t     mask;
    otIp6Address address;

    SuccessOrExit(error = aArgs[0].ParseAsUint16(panId));
    SuccessOrExit(error = aArgs[1].ParseAsUint32(mask));
    SuccessOrExit(error = aArgs[2].ParseAsIp6Address(address));

    error = otCommissionerPanIdQuery(GetInstancePtr(), panId, mask, &address, &Commissioner::HandlePanIdConflict, this);

exit:
    return error;
}

/**
 * @cli commissioner provisioningurl
 * @code
 * commissioner provisioningurl http://github.com/openthread/openthread
 * Done
 * @endcode
 * @cparam commissioner provisioningurl @ca{provisioningurl}
 * @par
 * Sets the %Commissioner provisioning URL.
 * @sa otCommissionerSetProvisioningUrl
 */
template <> otError Commissioner::Process<Cmd("provisioningurl")>(Arg aArgs[])
{
    // If aArgs[0] is empty, `GetCString() will return `nullptr`
    /// which will correctly clear the provisioning URL.
    return otCommissionerSetProvisioningUrl(GetInstancePtr(), aArgs[0].GetCString());
}

/**
 * @cli commissioner sessionid
 * @code
 * commissioner sessionid
 * 0
 * Done
 * @endcode
 * @par
 * Gets the current %Commissioner session ID.
 * @sa otCommissionerGetSessionId
 */
template <> otError Commissioner::Process<Cmd("sessionid")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    OutputLine("%d", otCommissionerGetSessionId(GetInstancePtr()));

    return OT_ERROR_NONE;
}

/**
 * @cli commissioner id (get,set)
 * @code
 * commissioner id OpenThread Commissioner
 * Done
 * @endcode
 * @code
 * commissioner id
 * OpenThread Commissioner
 * Done
 * @endcode
 * @cparam commissioner id @ca{name}
 * @par
 * Gets or sets the OpenThread %Commissioner ID name.
 * @sa otCommissionerSetId
 */
template <> otError Commissioner::Process<Cmd("id")>(Arg aArgs[])
{
    otError error;

    if (aArgs[0].IsEmpty())
    {
        OutputLine("%s", otCommissionerGetId(GetInstancePtr()));
        error = OT_ERROR_NONE;
    }
    else
    {
        error = otCommissionerSetId(GetInstancePtr(), aArgs[0].GetCString());
    }

    return error;
}

/**
 * @cli commissioner start
 * @code
 * commissioner start
 * Commissioner: petitioning
 * Done
 * Commissioner: active
 * @endcode
 * @par
 * Starts the Thread %Commissioner role.
 * @note The `commissioner` commands are available only when
 * `OPENTHREAD_CONFIG_COMMISSIONER_ENABLE` and `OPENTHREAD_FTD` are set.
 * @sa otCommissionerStart
 */
template <> otError Commissioner::Process<Cmd("start")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    return otCommissionerStart(GetInstancePtr(), &Commissioner::HandleStateChanged, &Commissioner::HandleJoinerEvent,
                               this);
}

void Commissioner::HandleStateChanged(otCommissionerState aState, void *aContext)
{
    static_cast<Commissioner *>(aContext)->HandleStateChanged(aState);
}

void Commissioner::HandleStateChanged(otCommissionerState aState)
{
    OutputLine("Commissioner: %s", StateToString(aState));
}

const char *Commissioner::StateToString(otCommissionerState aState)
{
    static const char *const kStateString[] = {
        "disabled",    // (0) OT_COMMISSIONER_STATE_DISABLED
        "petitioning", // (1) OT_COMMISSIONER_STATE_PETITION
        "active",      // (2) OT_COMMISSIONER_STATE_ACTIVE
    };

    static_assert(0 == OT_COMMISSIONER_STATE_DISABLED, "OT_COMMISSIONER_STATE_DISABLED value is incorrect");
    static_assert(1 == OT_COMMISSIONER_STATE_PETITION, "OT_COMMISSIONER_STATE_PETITION value is incorrect");
    static_assert(2 == OT_COMMISSIONER_STATE_ACTIVE, "OT_COMMISSIONER_STATE_ACTIVE value is incorrect");

    return Stringify(aState, kStateString);
}

void Commissioner::HandleJoinerEvent(otCommissionerJoinerEvent aEvent,
                                     const otJoinerInfo       *aJoinerInfo,
                                     const otExtAddress       *aJoinerId,
                                     void                     *aContext)
{
    static_cast<Commissioner *>(aContext)->HandleJoinerEvent(aEvent, aJoinerInfo, aJoinerId);
}

void Commissioner::HandleJoinerEvent(otCommissionerJoinerEvent aEvent,
                                     const otJoinerInfo       *aJoinerInfo,
                                     const otExtAddress       *aJoinerId)
{
    static const char *const kEventStrings[] = {
        "start",    // (0) OT_COMMISSIONER_JOINER_START
        "connect",  // (1) OT_COMMISSIONER_JOINER_CONNECTED
        "finalize", // (2) OT_COMMISSIONER_JOINER_FINALIZE
        "end",      // (3) OT_COMMISSIONER_JOINER_END
        "remove",   // (4) OT_COMMISSIONER_JOINER_REMOVED
    };

    static_assert(0 == OT_COMMISSIONER_JOINER_START, "OT_COMMISSIONER_JOINER_START value is incorrect");
    static_assert(1 == OT_COMMISSIONER_JOINER_CONNECTED, "OT_COMMISSIONER_JOINER_CONNECTED value is incorrect");
    static_assert(2 == OT_COMMISSIONER_JOINER_FINALIZE, "OT_COMMISSIONER_JOINER_FINALIZE value is incorrect");
    static_assert(3 == OT_COMMISSIONER_JOINER_END, "OT_COMMISSIONER_JOINER_END value is incorrect");
    static_assert(4 == OT_COMMISSIONER_JOINER_REMOVED, "OT_COMMISSIONER_JOINER_REMOVED value is incorrect");

    OT_UNUSED_VARIABLE(aJoinerInfo);

    OutputFormat("Commissioner: Joiner %s ", Stringify(aEvent, kEventStrings));

    if (aJoinerId != nullptr)
    {
        OutputExtAddress(*aJoinerId);
    }

    OutputNewLine();
}

/**
 * @cli commissioner stop
 * @code
 * commissioner stop
 * Done
 * @endcode
 * @par
 * Stops the Thread %Commissioner role.
 * @sa otCommissionerStop
 */
template <> otError Commissioner::Process<Cmd("stop")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    return otCommissionerStop(GetInstancePtr());
}

/**
 * @cli commissioner state
 * @code
 * commissioner state
 * active
 * Done
 * @endcode
 * @par
 * Returns the current state of the %Commissioner. Possible values are
 * `active`, `disabled`, or `petition` (petitioning to become %Commissioner).
 * @sa otCommissionerState
 */
template <> otError Commissioner::Process<Cmd("state")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    OutputLine("%s", StateToString(otCommissionerGetState(GetInstancePtr())));

    return OT_ERROR_NONE;
}

otError Commissioner::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                                    \
    {                                                               \
        aCommandString, &Commissioner::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("announce"),  CmdEntry("energy"),  CmdEntry("id"),    CmdEntry("joiner"),
        CmdEntry("mgmtget"),   CmdEntry("mgmtset"), CmdEntry("panid"), CmdEntry("provisioningurl"),
        CmdEntry("sessionid"), CmdEntry("start"),   CmdEntry("state"), CmdEntry("stop"),
    };

#undef CmdEntry

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty() || (aArgs[0] == "help"))
    {
        OutputCommandTable(kCommands);
        ExitNow(error = aArgs[0].IsEmpty() ? error : OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

void Commissioner::HandleEnergyReport(uint32_t       aChannelMask,
                                      const uint8_t *aEnergyList,
                                      uint8_t        aEnergyListLength,
                                      void          *aContext)
{
    static_cast<Commissioner *>(aContext)->HandleEnergyReport(aChannelMask, aEnergyList, aEnergyListLength);
}

void Commissioner::HandleEnergyReport(uint32_t aChannelMask, const uint8_t *aEnergyList, uint8_t aEnergyListLength)
{
    OutputFormat("Energy: %08lx ", ToUlong(aChannelMask));

    for (uint8_t i = 0; i < aEnergyListLength; i++)
    {
        OutputFormat("%d ", static_cast<int8_t>(aEnergyList[i]));
    }

    OutputNewLine();
}

void Commissioner::HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask, void *aContext)
{
    static_cast<Commissioner *>(aContext)->HandlePanIdConflict(aPanId, aChannelMask);
}

void Commissioner::HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask)
{
    OutputLine("Conflict: %04x, %08lx", aPanId, ToUlong(aChannelMask));
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
