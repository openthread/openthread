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

#include "instance/instance.hpp"

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
    {"a/ae"},  // kUriAddressError
    {"a/an"},  // kUriAddressNotify
    {"a/aq"},  // kUriAddressQuery
    {"a/ar"},  // kUriAddressRelease
    {"a/as"},  // kUriAddressSolicit
    {"a/sd"},  // kUriServerData
    {"a/yl"},  // kUriAnycastLocate
    {"b/ba"},  // kUriBackboneAnswer
    {"b/bmr"}, // kUriBackboneMlr
    {"b/bq"},  // kUriBackboneQuery
    {"c/ab"},  // kUriAnnounceBegin
    {"c/ag"},  // kUriActiveGet
    {"c/ar"},  // kUriActiveReplace
    {"c/as"},  // kUriActiveSet
    {"c/ca"},  // kUriCommissionerKeepAlive
    {"c/cg"},  // kUriCommissionerGet
    {"c/cp"},  // kUriCommissionerPetition
    {"c/cs"},  // kUriCommissionerSet
    {"c/dc"},  // kUriDatasetChanged
    {"c/er"},  // kUriEnergyReport
    {"c/es"},  // kUriEnergyScan
    {"c/je"},  // kUriJoinerEntrust
    {"c/jf"},  // kUriJoinerFinalize
    {"c/la"},  // kUriLeaderKeepAlive
    {"c/lp"},  // kUriLeaderPetition
    {"c/pc"},  // kUriPanIdConflict
    {"c/pg"},  // kUriPendingGet
    {"c/pq"},  // kUriPanIdQuery
    {"c/ps"},  // kUriPendingSet
    {"c/rx"},  // kUriRelayRx
    {"c/tx"},  // kUriRelayTx
    {"c/ur"},  // kUriProxyRx
    {"c/ut"},  // kUriProxyTx
    {"d/da"},  // kUriDiagnosticGetAnswer
    {"d/dg"},  // kUriDiagnosticGetRequest
    {"d/dq"},  // kUriDiagnosticGetQuery
    {"d/dr"},  // kUriDiagnosticReset
    {"n/dn"},  // kUriDuaRegistrationNotify
    {"n/dr"},  // kUriDuaRegistrationRequest
    {"n/mr"},  // kUriMlr
};

static_assert(BinarySearch::IsSorted(kEntries), "kEntries is not sorted");

// Validate that the `kEntries[]` order match the `kUri*` enum constants
static_assert(AreConstStringsEqual(kEntries[kUriAddressError].mPath, "a/ae"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriAddressNotify].mPath, "a/an"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriAddressQuery].mPath, "a/aq"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriAddressRelease].mPath, "a/ar"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriAddressSolicit].mPath, "a/as"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriServerData].mPath, "a/sd"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriAnycastLocate].mPath, "a/yl"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriBackboneAnswer].mPath, "b/ba"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriBackboneMlr].mPath, "b/bmr"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriBackboneQuery].mPath, "b/bq"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriAnnounceBegin].mPath, "c/ab"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriActiveGet].mPath, "c/ag"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriActiveReplace].mPath, "c/ar"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriActiveSet].mPath, "c/as"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriCommissionerKeepAlive].mPath, "c/ca"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriCommissionerGet].mPath, "c/cg"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriCommissionerPetition].mPath, "c/cp"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriCommissionerSet].mPath, "c/cs"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriDatasetChanged].mPath, "c/dc"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriEnergyReport].mPath, "c/er"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriEnergyScan].mPath, "c/es"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriJoinerEntrust].mPath, "c/je"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriJoinerFinalize].mPath, "c/jf"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriLeaderKeepAlive].mPath, "c/la"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriLeaderPetition].mPath, "c/lp"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriPanIdConflict].mPath, "c/pc"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriPendingGet].mPath, "c/pg"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriPanIdQuery].mPath, "c/pq"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriPendingSet].mPath, "c/ps"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriRelayRx].mPath, "c/rx"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriRelayTx].mPath, "c/tx"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriProxyRx].mPath, "c/ur"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriProxyTx].mPath, "c/ut"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriDiagnosticGetAnswer].mPath, "d/da"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriDiagnosticGetRequest].mPath, "d/dg"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriDiagnosticGetQuery].mPath, "d/dq"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriDiagnosticReset].mPath, "d/dr"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriDuaRegistrationNotify].mPath, "n/dn"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriDuaRegistrationRequest].mPath, "n/dr"), "kEntries is invalid");
static_assert(AreConstStringsEqual(kEntries[kUriMlr].mPath, "n/mr"), "kEntries is invalid");

struct UriEnumCheck
{
    InitEnumValidatorCounter();
    ValidateNextEnum(kUriAddressError);
    ValidateNextEnum(kUriAddressNotify);
    ValidateNextEnum(kUriAddressQuery);
    ValidateNextEnum(kUriAddressRelease);
    ValidateNextEnum(kUriAddressSolicit);
    ValidateNextEnum(kUriServerData);
    ValidateNextEnum(kUriAnycastLocate);
    ValidateNextEnum(kUriBackboneAnswer);
    ValidateNextEnum(kUriBackboneMlr);
    ValidateNextEnum(kUriBackboneQuery);
    ValidateNextEnum(kUriAnnounceBegin);
    ValidateNextEnum(kUriActiveGet);
    ValidateNextEnum(kUriActiveReplace);
    ValidateNextEnum(kUriActiveSet);
    ValidateNextEnum(kUriCommissionerKeepAlive);
    ValidateNextEnum(kUriCommissionerGet);
    ValidateNextEnum(kUriCommissionerPetition);
    ValidateNextEnum(kUriCommissionerSet);
    ValidateNextEnum(kUriDatasetChanged);
    ValidateNextEnum(kUriEnergyReport);
    ValidateNextEnum(kUriEnergyScan);
    ValidateNextEnum(kUriJoinerEntrust);
    ValidateNextEnum(kUriJoinerFinalize);
    ValidateNextEnum(kUriLeaderKeepAlive);
    ValidateNextEnum(kUriLeaderPetition);
    ValidateNextEnum(kUriPanIdConflict);
    ValidateNextEnum(kUriPendingGet);
    ValidateNextEnum(kUriPanIdQuery);
    ValidateNextEnum(kUriPendingSet);
    ValidateNextEnum(kUriRelayRx);
    ValidateNextEnum(kUriRelayTx);
    ValidateNextEnum(kUriProxyRx);
    ValidateNextEnum(kUriProxyTx);
    ValidateNextEnum(kUriDiagnosticGetAnswer);
    ValidateNextEnum(kUriDiagnosticGetRequest);
    ValidateNextEnum(kUriDiagnosticGetQuery);
    ValidateNextEnum(kUriDiagnosticReset);
    ValidateNextEnum(kUriDuaRegistrationNotify);
    ValidateNextEnum(kUriDuaRegistrationRequest);
    ValidateNextEnum(kUriMlr);
};

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
