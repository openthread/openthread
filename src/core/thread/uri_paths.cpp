/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for Thread URIs.
 */

#include "uri_paths.hpp"

#include "common/binary_search.hpp"
#include "common/debug.hpp"
#include "common/string.hpp"

#include <string.h>

namespace ot {

namespace UriList {

struct Entry
{
    const char *mPath;

    constexpr static bool AreInOrder(const Entry &aFirst, const Entry &aSecond)
    {
        return AreStringsInOrder(aFirst.mPath, aSecond.mPath);
    }

    int Compare(const char *aPath) const { return strcmp(aPath, mPath); }
};

// The list of URI paths (MUST be sorted alphabetically)
static constexpr Entry kEntries[] = {
    {"a/ae"},  // (0) kUriAddressError
    {"a/an"},  // (1) kUriAddressNotify
    {"a/aq"},  // (2) kUriAddressQuery
    {"a/ar"},  // (3) kUriAddressRelease
    {"a/as"},  // (4) kUriAddressSolicit
    {"a/sd"},  // (5) kUriServerData
    {"a/yl"},  // (6) kUriAnycastLocate
    {"b/ba"},  // (7) kUriBackboneAnswer
    {"b/bmr"}, // (8) kUriBackboneMlr
    {"b/bq"},  // (9) kUriBackboneQuery
    {"c/ab"},  // (10) kUriAnnounceBegin
    {"c/ag"},  // (11) kUriActiveGet
    {"c/ar"},  // (12) kUriActiveReplace
    {"c/as"},  // (13) kUriActiveSet
    {"c/ca"},  // (14) kUriCommissionerKeepAlive
    {"c/cg"},  // (15) kUriCommissionerGet
    {"c/cp"},  // (16) kUriCommissionerPetition
    {"c/cs"},  // (17) kUriCommissionerSet
    {"c/dc"},  // (18) kUriDatasetChanged
    {"c/er"},  // (19) kUriEnergyReport
    {"c/es"},  // (20) kUriEnergyScan
    {"c/je"},  // (21) kUriJoinerEntrust
    {"c/jf"},  // (22) kUriJoinerFinalize
    {"c/la"},  // (23) kUriLeaderKeepAlive
    {"c/lp"},  // (24) kUriLeaderPetition
    {"c/pc"},  // (25) kUriPanIdConflict
    {"c/pg"},  // (26) kUriPendingGet
    {"c/pq"},  // (27) kUriPanIdQuery
    {"c/ps"},  // (28) kUriPendingSet
    {"c/rx"},  // (29) kUriRelayRx
    {"c/tx"},  // (30) kUriRelayTx
    {"c/ur"},  // (31) kUriProxyRx
    {"c/ut"},  // (32) kUriProxyTx
    {"d/da"},  // (33) kUriDiagnosticGetAnswer
    {"d/dg"},  // (34) kUriDiagnosticGetRequest
    {"d/dq"},  // (35) kUriDiagnosticGetQuery
    {"d/dr"},  // (36) kUriDiagnosticReset
    {"n/dn"},  // (37) kUriDuaRegistrationNotify
    {"n/dr"},  // (38) kUriDuaRegistrationRequest
    {"n/mr"},  // (39) kUriMlr
};

static_assert(BinarySearch::IsSorted(kEntries), "kEntries is not sorted");

static_assert(0 == kUriAddressError, "kUriAddressError (`a/ae`) is invalid");
static_assert(1 == kUriAddressNotify, "kUriAddressNotify (`a/an`) is invalid");
static_assert(2 == kUriAddressQuery, "kUriAddressQuery (`a/aq`) is invalid");
static_assert(3 == kUriAddressRelease, "kUriAddressRelease (`a/ar`) is invalid");
static_assert(4 == kUriAddressSolicit, "kUriAddressSolicit (`a/as`) is invalid");
static_assert(5 == kUriServerData, "kUriServerData (`a/sd`) is invalid");
static_assert(6 == kUriAnycastLocate, "kUriAnycastLocate (`a/yl`) is invalid");
static_assert(7 == kUriBackboneAnswer, "kUriBackboneAnswer (`b/ba`) is invalid");
static_assert(8 == kUriBackboneMlr, "kUriBackboneMlr (`b/bmr`) is invalid");
static_assert(9 == kUriBackboneQuery, "kUriBackboneQuery (`b/bq`) is invalid");
static_assert(10 == kUriAnnounceBegin, "kUriAnnounceBegin (`c/ab`) is invalid");
static_assert(11 == kUriActiveGet, "kUriActiveGet (`c/ag`) is invalid");
static_assert(12 == kUriActiveReplace, "kUriActiveReplace (`c/ar`) is invalid");
static_assert(13 == kUriActiveSet, "kUriActiveSet (`c/as`) is invalid");
static_assert(14 == kUriCommissionerKeepAlive, "kUriCommissionerKeepAlive (`c/ca`) is invalid");
static_assert(15 == kUriCommissionerGet, "kUriCommissionerGet (`c/cg`) is invalid");
static_assert(16 == kUriCommissionerPetition, "kUriCommissionerPetition (`c/cp`) is invalid");
static_assert(17 == kUriCommissionerSet, "kUriCommissionerSet (`c/cs`) is invalid");
static_assert(18 == kUriDatasetChanged, "kUriDatasetChanged (`c/dc`) is invalid");
static_assert(19 == kUriEnergyReport, "kUriEnergyReport (`c/er`) is invalid");
static_assert(20 == kUriEnergyScan, "kUriEnergyScan (`c/es`) is invalid");
static_assert(21 == kUriJoinerEntrust, "kUriJoinerEntrust (`c/je`) is invalid");
static_assert(22 == kUriJoinerFinalize, "kUriJoinerFinalize (`c/jf`) is invalid");
static_assert(23 == kUriLeaderKeepAlive, "kUriLeaderKeepAlive (`c/la`) is invalid");
static_assert(24 == kUriLeaderPetition, "kUriLeaderPetition (`c/lp`) is invalid");
static_assert(25 == kUriPanIdConflict, "kUriPanIdConflict (`c/pc`) is invalid");
static_assert(26 == kUriPendingGet, "kUriPendingGet (`c/pg`) is invalid");
static_assert(27 == kUriPanIdQuery, "kUriPanIdQuery (`c/pq`) is invalid");
static_assert(28 == kUriPendingSet, "kUriPendingSet (`c/ps`) is invalid");
static_assert(29 == kUriRelayRx, "kUriRelayRx (`c/rx`) is invalid");
static_assert(30 == kUriRelayTx, "kUriRelayTx (`c/tx`) is invalid");
static_assert(31 == kUriProxyRx, "kUriProxyRx (`c/ur`) is invalid");
static_assert(32 == kUriProxyTx, "kUriProxyTx (`c/ut`) is invalid");
static_assert(33 == kUriDiagnosticGetAnswer, "kUriDiagnosticGetAnswer (`d/da`) is invalid");
static_assert(34 == kUriDiagnosticGetRequest, "kUriDiagnosticGetRequest (`d/dg`) is invalid");
static_assert(35 == kUriDiagnosticGetQuery, "kUriDiagnosticGetQuery (`d/dq`) is invalid");
static_assert(36 == kUriDiagnosticReset, "kUriDiagnosticReset (`d/dr`) is invalid");
static_assert(37 == kUriDuaRegistrationNotify, "kUriDuaRegistrationNotify (`n/dn`) is invalid");
static_assert(38 == kUriDuaRegistrationRequest, "kUriDuaRegistrationRequest (`n/dr`) is invalid");
static_assert(39 == kUriMlr, "kUriMlr (`n/mr`) is invalid");

} // namespace UriList

const char *PathForUri(Uri aUri)
{
    OT_ASSERT(aUri != kUriUnknown);

    return UriList::kEntries[aUri].mPath;
}

Uri UriFromPath(const char *aPath)
{
    Uri                   uri   = kUriUnknown;
    const UriList::Entry *entry = BinarySearch::Find(aPath, UriList::kEntries);

    VerifyOrExit(entry != nullptr);
    uri = static_cast<Uri>(entry - UriList::kEntries);

exit:
    return uri;
}

template <> const char *UriToString<kUriAddressError>(void) { return "AddressError"; }
template <> const char *UriToString<kUriAddressNotify>(void) { return "AddressNotify"; }
template <> const char *UriToString<kUriAddressQuery>(void) { return "AddressQuery"; }
template <> const char *UriToString<kUriAddressRelease>(void) { return "AddressRelease"; }
template <> const char *UriToString<kUriAddressSolicit>(void) { return "AddressSolicit"; }
template <> const char *UriToString<kUriServerData>(void) { return "ServerData"; }
template <> const char *UriToString<kUriAnycastLocate>(void) { return "AnycastLocate"; }
template <> const char *UriToString<kUriBackboneAnswer>(void) { return "BackboneAnswer"; }
template <> const char *UriToString<kUriBackboneMlr>(void) { return "BackboneMlr"; }
template <> const char *UriToString<kUriBackboneQuery>(void) { return "BackboneQuery"; }
template <> const char *UriToString<kUriAnnounceBegin>(void) { return "AnnounceBegin"; }
template <> const char *UriToString<kUriActiveGet>(void) { return "ActiveGet"; }
template <> const char *UriToString<kUriActiveReplace>(void) { return "ActiveReplace"; }
template <> const char *UriToString<kUriActiveSet>(void) { return "ActiveSet"; }
template <> const char *UriToString<kUriCommissionerKeepAlive>(void) { return "CommissionerKeepAlive"; }
template <> const char *UriToString<kUriCommissionerGet>(void) { return "CommissionerGet"; }
template <> const char *UriToString<kUriCommissionerPetition>(void) { return "CommissionerPetition"; }
template <> const char *UriToString<kUriCommissionerSet>(void) { return "CommissionerSet"; }
template <> const char *UriToString<kUriDatasetChanged>(void) { return "DatasetChanged"; }
template <> const char *UriToString<kUriEnergyReport>(void) { return "EnergyReport"; }
template <> const char *UriToString<kUriEnergyScan>(void) { return "EnergyScan"; }
template <> const char *UriToString<kUriJoinerEntrust>(void) { return "JoinerEntrust"; }
template <> const char *UriToString<kUriJoinerFinalize>(void) { return "JoinerFinalize"; }
template <> const char *UriToString<kUriLeaderKeepAlive>(void) { return "LeaderKeepAlive"; }
template <> const char *UriToString<kUriLeaderPetition>(void) { return "LeaderPetition"; }
template <> const char *UriToString<kUriPanIdConflict>(void) { return "PanIdConflict"; }
template <> const char *UriToString<kUriPendingGet>(void) { return "PendingGet"; }
template <> const char *UriToString<kUriPanIdQuery>(void) { return "PanIdQuery"; }
template <> const char *UriToString<kUriPendingSet>(void) { return "PendingSet"; }
template <> const char *UriToString<kUriRelayRx>(void) { return "RelayRx"; }
template <> const char *UriToString<kUriRelayTx>(void) { return "RelayTx"; }
template <> const char *UriToString<kUriProxyRx>(void) { return "ProxyRx"; }
template <> const char *UriToString<kUriProxyTx>(void) { return "ProxyTx"; }
template <> const char *UriToString<kUriDiagnosticGetAnswer>(void) { return "DiagGetAnswer"; }
template <> const char *UriToString<kUriDiagnosticGetRequest>(void) { return "DiagGetRequest"; }
template <> const char *UriToString<kUriDiagnosticGetQuery>(void) { return "DiagGetQuery"; }
template <> const char *UriToString<kUriDiagnosticReset>(void) { return "DiagReset"; }
template <> const char *UriToString<kUriDuaRegistrationNotify>(void) { return "DuaRegNotify"; }
template <> const char *UriToString<kUriDuaRegistrationRequest>(void) { return "DuaRegRequest"; }
template <> const char *UriToString<kUriMlr>(void) { return "Mlr"; }

} // namespace ot
