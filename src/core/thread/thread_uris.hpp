/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#ifndef THREAD_URIS_HPP_
#define THREAD_URIS_HPP_

namespace Thread {

/**
 * The URI Path for Address Query.
 *
 */
#define OPENTHREAD_URI_ADDRESS_QUERY    "a/aq"

/**
 * @def OPENTHREAD_URI_ADDRESS_NOTIFY
 *
 * The URI Path for Address Notify.
 *
 */
#define OPENTHREAD_URI_ADDRESS_NOTIFY   "a/an"

/**
 * @def OPENTHREAD_URI_ADDRESS_ERROR
 *
 * The URI Path for Address Error.
 *
 */
#define OPENTHREAD_URI_ADDRESS_ERROR    "a/ae"

/**
 * @def OPENTHREAD_URI_ADDRESS_RELEASE
 *
 * The URI Path for Address Release.
 *
 */
#define OPENTHREAD_URI_ADDRESS_RELEASE  "a/ar"

/**
 * @def OPENTHREAD_URI_ADDRESS_SOLICIT
 *
 * The URI Path for Address Solicit.
 *
 */
#define OPENTHREAD_URI_ADDRESS_SOLICIT  "a/as"

/**
 * @def OPENTHREAD_URI_ACTIVE_GET
 *
 * The URI Path for MGMT_ACTIVE_GET
 *
 */
#define OPENTHREAD_URI_ACTIVE_GET       "c/ag"

/**
 * @def OPENTHREAD_URI_ACTIVE_SET
 *
 * The URI Path for MGMT_ACTIVE_SET
 *
 */
#define OPENTHREAD_URI_ACTIVE_SET       "c/as"

/**
 * @def OPENTHREAD_URI_DATASET_CHANGED
 *
 * The URI Path for MGMT_DATASET_CHANGED
 *
 */
#define OPENTHREAD_URI_DATASET_CHANGED  "c/dc"

/**
 * @def OPENTHREAD_URI_ENERGY_SCAN
 *
 * The URI Path for Energy Scan
 *
 */
#define OPENTHREAD_URI_ENERGY_SCAN      "c/es"

/**
 * @def OPENTHREAD_URI_ENERGY_REPORT
 *
 * The URI Path for Energy Report
 *
 */
#define OPENTHREAD_URI_ENERGY_REPORT    "c/er"

/**
 * @def OPENTHREAD_URI_PENDING_GET
 *
 * The URI Path for MGMT_PENDING_GET
 *
 */
#define OPENTHREAD_URI_PENDING_GET      "c/pg"

/**
 * @def OPENTHREAD_URI_PENDING_SET
 *
 * The URI Path for MGMT_PENDING_SET
 *
 */
#define OPENTHREAD_URI_PENDING_SET      "c/ps"

/**
 * @def OPENTHREAD_URI_SERVER_DATA
 *
 * The URI Path for Server Data Registration.
 *
 */
#define OPENTHREAD_URI_SERVER_DATA      "a/sd"

/**
 * @def OPENTHREAD_URI_ANNOUNCE_BEGIN
 *
 * The URI Path for Announce Begin.
 *
 */
#define OPENTHREAD_URI_ANNOUNCE_BEGIN   "c/ab"

/**
 * @def OPENTHREAD_URI_RELAY_RX
 *
 * The URI Path for Relay RX.
 *
 */
#define OPENTHREAD_URI_RELAY_RX         "c/rx"

/**
 * @def OPENTHREAD_URI_RELAY_TX
 *
 * The URI Path for Relay TX.
 *
 */
#define OPENTHREAD_URI_RELAY_TX         "c/tx"

/**
 * @def OPENTHREAD_URI_JOINER_FINALIZE
 *
 * The URI Path for Joiner Finalize
 *
 */
#define OPENTHREAD_URI_JOINER_FINALIZE  "c/jf"

/**
 * @def OPENTHREAD_URI_JOINER_ENTRUST
 *
 * The URI Path for Joiner Entrust
 *
 */
#define OPENTHREAD_URI_JOINER_ENTRUST   "c/je"

/**
 * @def OPENTHREAD_URI_LEADER_PETITION
 *
 * The URI Path for Leader Petition
 *
 */
#define OPENTHREAD_URI_LEADER_PETITION  "c/lp"

/**
 * @def OPENTHREAD_URI_LEADER_KEEP_ALIVE
 *
 * The URI Path for Leader Keep Alive
 *
 */
#define OPENTHREAD_URI_LEADER_KEEP_ALIVE "c/la"

/**
 * @def OPENTHREAD_URI_PANID_CONFLICT
 *
 * The URI Path for PAN ID Conflict
 *
 */
#define OPENTHREAD_URI_PANID_CONFLICT   "c/pc"

/**
 * @def OPENTHREAD_URI_PANID_QUERY
 *
 * The URI Path for PAN ID Query
 *
 */
#define OPENTHREAD_URI_PANID_QUERY      "c/pq"

/**
 * @def OPENTHREAD_URI_COMMISSIONER_GET
 *
 * The URI Path for MGMT_COMMISSIONER_GET
 *
 */
#define OPENTHREAD_URI_COMMISSIONER_GET "c/cg"

/**
 * @def OPENTHREAD_URI_COMMISSIONER_SET
 *
 * The URI Path for MGMT_COMMISSIONER_SET
 *
 */
#define OPENTHREAD_URI_COMMISSIONER_SET "c/cs"

/**
 * @def OPENTHREAD_URI_DIAGNOSTIC_GET
 *
 * The URI Path for Network Diagnostic Get.
 *
 */
#define OPENTHREAD_URI_DIAGNOSTIC_GET   "d/dg"

/**
 * @def OPENTHREAD_URI_DIAG_RST
 *
 * The URI Path for Network Diagnostic Reset.
 *
 */
#define OPENTHREAD_URI_DIAGNOSTIC_RESET "d/dr"

}  // namespace Thread

#endif  // THREAD_URIS_HPP_
