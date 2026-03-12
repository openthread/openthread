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

#define UriEntryMapList(_)                                        \
    _("a/ae", kUriAddressError, "AddrError")                      \
    _("a/an", kUriAddressNotify, "AddrNotify")                    \
    _("a/aq", kUriAddressQuery, "AddrQuery")                      \
    _("a/ar", kUriAddressRelease, "AddrRelease")                  \
    _("a/as", kUriAddressSolicit, "AddrSolicit")                  \
    _("a/sd", kUriServerData, "ServerData")                       \
    _("a/yl", kUriAnycastLocate, "AnycastLocate")                 \
    _("b/ba", kUriBackboneAnswer, "BbAnswer")                     \
    _("b/bmr", kUriBackboneMlr, "BbMlr")                          \
    _("b/bq", kUriBackboneQuery, "BbQuery")                       \
    _("c/ab", kUriAnnounceBegin, "AnnounceBegin")                 \
    _("c/ag", kUriActiveGet, "ActiveGet")                         \
    _("c/ar", kUriActiveReplace, "ActiveReplace")                 \
    _("c/as", kUriActiveSet, "ActiveSet")                         \
    _("c/ca", kUriCommissionerKeepAlive, "CommrKeepAlive")        \
    _("c/cg", kUriCommissionerGet, "CommrGet")                    \
    _("c/cp", kUriCommissionerPetition, "CommrPetition")          \
    _("c/cs", kUriCommissionerSet, "CommrSet")                    \
    _("c/dc", kUriDatasetChanged, "DatasetChanged")               \
    _("c/er", kUriEnergyReport, "EnergyReport")                   \
    _("c/es", kUriEnergyScan, "EnergyScan")                       \
    _("c/je", kUriJoinerEntrust, "JoinerEntrust")                 \
    _("c/jf", kUriJoinerFinalize, "JoinerFinalize")               \
    _("c/la", kUriLeaderKeepAlive, "LeaderKeepAlive")             \
    _("c/lp", kUriLeaderPetition, "LeaderPetition")               \
    _("c/nj", kUriEnrollerJoinerAccept, "EnrollerJoinerAccept")   \
    _("c/nk", kUriEnrollerKeepAlive, "EnrollerKeepAlive")         \
    _("c/nl", kUriEnrollerJoinerRelease, "EnrollerJoinerRelease") \
    _("c/nr", kUriEnrollerRegister, "EnrollerRegister")           \
    _("c/ns", kUriEnrollerReportState, "EnrollerReportState")     \
    _("c/pc", kUriPanIdConflict, "PanIdConflict")                 \
    _("c/pg", kUriPendingGet, "PendingGet")                       \
    _("c/pq", kUriPanIdQuery, "PanIdQuery")                       \
    _("c/ps", kUriPendingSet, "PendingSet")                       \
    _("c/rx", kUriRelayRx, "RelayRx")                             \
    _("c/te", kUriTcatEnable, "TcatEnable")                       \
    _("c/tx", kUriRelayTx, "RelayTx")                             \
    _("c/ur", kUriProxyRx, "ProxyRx")                             \
    _("c/ut", kUriProxyTx, "ProxyTx")                             \
    _("d/da", kUriDiagnosticGetAnswer, "DiagGetAnswer")           \
    _("d/dg", kUriDiagnosticGetRequest, "DiagGetRequest")         \
    _("d/dq", kUriDiagnosticGetQuery, "DiagGetQuery")             \
    _("d/dr", kUriDiagnosticReset, "DiagReset")                   \
    _("h/an", kUriHistoryAnswer, "HistAnswer")                    \
    _("h/qy", kUriHistoryQuery, "HistQuery")                      \
    _("n/dn", kUriDuaRegistrationNotify, "DuaRegNotify")          \
    _("n/dr", kUriDuaRegistrationRequest, "DuaRegRequest")        \
    _("n/mr", kUriMlr, "Mlr")

// We use the X-Macro pattern here. The `UriEntryMapList` macro defines the
// mapping between URI path string, its `kUri*` enum and its name string (for
// `UriToString`).
//
// The `UriEntryMapList` macro accepts a single parameter, which is a "visitor"
// macro (`_`). The visitor macro is called for each entry in the list. We define
// different visitor macros: one to define the `kEntries[]` array, another to
// validate the entries in the array (ensuring the URI paths match the enum
// values and are sorted properly), and a third to define the `UriToString()`
// template specializations.

#define _EntryArrayElement(kPathString, kUri, kName) {kPathString},

static constexpr const Entry kEntries[] = {UriEntryMapList(_EntryArrayElement)};

// The URI entries MUST be sorted based on their path string (e.g. `c/ut`)
static_assert(BinarySearch::IsSorted(kEntries), "kEntries is not sorted");

#define _ValidateEntryElement(kPathString, kUri, kName)                    \
    static_assert(AreConstStringsEqual(kEntries[kUri].mPath, kPathString), \
                  #kUri " value is incorrect. list is not sorted");

UriEntryMapList(_ValidateEntryElement)

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

#define _DefineUriToString(kPathString, kUri, kName) \
    template <> const char *UriToString<kUri>(void) { return kName; }

UriEntryMapList(_DefineUriToString)

} // namespace ot
