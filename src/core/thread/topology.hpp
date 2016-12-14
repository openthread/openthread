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
 *   This file includes definitions for maintaining Thread network topologies.
 */

#ifndef TOPOLOGY_HPP_
#define TOPOLOGY_HPP_

#include <openthread-core-config.h>
#include <mac/mac_frame.hpp>
#include <net/ip6.hpp>
#include <thread/mle_tlvs.hpp>
#include <thread/link_quality.hpp>

namespace Thread {

/**
 * This class represents a Thread neighbor.
 *
 */
class Neighbor
{
public:
    Mac::ExtAddress mMacAddr;            ///< The IEEE 802.15.4 Extended Address
    uint32_t        mLastHeard;          ///< Time when last heard.
    union
    {
        struct
        {
            uint32_t mLinkFrameCounter;  ///< The Link Frame Counter
            uint32_t mMleFrameCounter;   ///< The MLE Frame Counter
            uint16_t mRloc16;            ///< The RLOC16
        } mValid;
        struct
        {
            uint8_t mChallenge[Mle::ChallengeTlv::kMaxSize];  ///< The challenge value
            uint8_t mChallengeLength;    ///< The challenge length
        } mPending;
    };

    uint32_t mKeySequence;               ///< Current key sequence

    /**
     * Neighbor link states.
     *
     */
    enum State
#if _WIN32
        : unsigned int
#endif
    {
        kStateInvalid,                   ///< Neighbor link is invalid
        kStateRestored,                  ///< Neighbor is restored from non-volatile memory
        kStateParentRequest,             ///< Received an MLE Parent Request message
        kStateChildIdRequest,            ///< Received an MLE Child ID Request message
        kStateLinkRequest,               ///< Sent an MLE Link Request message
        kStateChildUpdateRequest,        ///< Sent an MLE Child Update Request message
        kStateValid,                     ///< Link is valid
    };

    State           mState : 3;          ///< The link state
    uint8_t         mMode : 4;           ///< The MLE device mode
    bool            mDataRequest : 1;    ///< Indicates whether or not a Data Poll was received
    uint8_t         mLinkFailures;       ///< Consecutive link failure count
    LinkQualityInfo mLinkInfo;           ///< Link quality info (contains average RSS, link margin and link quality)
};

/**
 * This class represents a Thread Child.
 *
 */
class Child : public Neighbor
{
public:
    enum
    {
        kMaxIp6AddressPerChild = OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD,
    };
    Ip6::Address mIp6Address[kMaxIp6AddressPerChild];  ///< Registered IPv6 addresses
    uint32_t     mTimeout;                             ///< Child timeout
    uint16_t     mFragmentOffset;                      ///< 6LoWPAN fragment offset
    uint8_t      mRequestTlvs[5];                      ///< Requested MLE TLVs
    uint8_t      mNetworkDataVersion;                  ///< Current Network Data version
    uint16_t     mQueuedIndirectMessageCnt;            ///< Count of queued messages
    bool         mAddSrcMatchEntryShort;               ///< Indicates whether or not to force add short address
    bool         mAddSrcMatchEntryPending;             ///< Indicates whether or not pending to add
};

/**
 * This class represents a Thread Router
 *
 */
class Router : public Neighbor
{
public:
    uint8_t mNextHop;             ///< The next hop towards this router
    uint8_t mLinkQualityOut : 2;  ///< The link quality out for this router
    uint8_t mCost : 4;            ///< The cost to this router
    bool    mAllocated : 1;       ///< Indicates whether or not this entry is allocated
    bool    mReclaimDelay : 1;    ///< Indicates whether or not this entry is waiting to be reclaimed
};

}  // namespace Thread

#endif  // TOPOLOGY_HPP_
