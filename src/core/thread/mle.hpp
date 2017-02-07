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
 *   This file includes definitions for MLE functionality required by the Thread Child, Router, and Leader roles.
 */

#ifndef MLE_HPP_
#define MLE_HPP_

#include <openthread.h>
#include <common/encoding.hpp>
#include <common/timer.hpp>
#include <mac/mac.hpp>
#include <net/udp6.hpp>
#include <thread/mle_constants.hpp>
#include <thread/mle_tlvs.hpp>
#include <thread/topology.hpp>
#include <meshcop/joiner_router.hpp>

namespace Thread {

class ThreadNetif;
class AddressResolver;
class KeyManager;
class MeshForwarder;

namespace Mac { class Mac; }
namespace NetworkData { class Leader; }

/**
 * @addtogroup core-mle MLE
 *
 * @brief
 *   This module includes definitions for the MLE protocol.
 *
 * @{
 *
 * @defgroup core-mle-core Core
 * @defgroup core-mle-router Router
 * @defgroup core-mle-tlvs TLVs
 *
 * @}
 */

/**
 * @namespace Thread::Mle
 *
 * @brief
 *   This namespace includes definitions for the MLE protocol.
 */

namespace Mle {

class MleRouter;

/**
 * @addtogroup core-mle-core
 *
 * @brief
 *   This module includes definitions for MLE functionality required by the Thread Child, Router, and Leader roles.
 *
 * @{
 *
 */

/**
 * MLE Device states
 *
 */
enum DeviceState
{
    kDeviceStateDisabled = 0,   ///< Thread interface is disabled.
    kDeviceStateDetached = 1,   ///< Thread interface is not attached to a partition.
    kDeviceStateChild    = 2,   ///< Thread interface participating as a Child.
    kDeviceStateRouter   = 3,   ///< Thread interface participating as a Router.
    kDeviceStateLeader   = 4,   ///< Thread interface participating as a Leader.
};

/**
 * This enumeration represents the allocation of the ALOC Space
 *
 */
enum AlocAllocation
{
    kAloc16Mask                         = 0xfc,
    kAloc16Leader                       = 0xfc00,
    kAloc16DhcpAgentStart               = 0xfc01,
    kAloc16DhcpAgentEnd                 = 0xfc0f,
    kAloc16DhcpAgentMask                = 0x03ff,
    kAloc16ServiceStart                 = 0xfc10,
    kAloc16ServiceEnd                   = 0xfc2f,
    kAloc16CommissionerStart            = 0xfc30,
    kAloc16CommissionerEnd              = 0xfc37,
    kAloc16NeighborDiscoveryAgentStart  = 0xfc40,
    kAloc16NeighborDiscoveryAgentEnd    = 0xfc4e,
};

/**
 * This class implements MLE Header generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Header
{
public:
    /**
     * This method initializes the MLE header.
     *
     */
    void Init(void) { mSecuritySuite = k154Security; mSecurityControl = Mac::Frame::kSecEncMic32; }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const {
        return (mSecuritySuite == kNoSecurity) ||
               (mSecuritySuite == k154Security &&
                mSecurityControl == (Mac::Frame::kKeyIdMode2 | Mac::Frame::kSecEncMic32));
    }

    /**
     * This method returns the MLE header and Command Type length.
     *
     * @returns The MLE header and Command Type length.
     *
     */
    uint8_t GetLength(void) const {
        uint8_t rval = sizeof(mSecuritySuite) + sizeof(mCommand);

        if (mSecuritySuite == k154Security) {
            rval += sizeof(mSecurityControl) + sizeof(mFrameCounter) + sizeof(mKeySource) + sizeof(mKeyIndex);
        }

        return rval;
    }

    enum SecuritySuite
    {
        k154Security = 0,    ///< IEEE 802.15.4-2006 security.
        kNoSecurity  = 255,  ///< No security enabled.
    };

    /**
     * This method returns the Security Suite value.
     *
     * @returns The Security Suite value.
     *
     */
    SecuritySuite GetSecuritySuite(void) const { return static_cast<SecuritySuite>(mSecuritySuite); }

    /**
     * This method sets the Security Suite value.
     *
     * @param[in]  aSecuritySuite  The Security Suite value.
     *
     */
    void SetSecuritySuite(SecuritySuite aSecuritySuite) { mSecuritySuite = static_cast<uint8_t>(aSecuritySuite); }

    /**
     * This method returns the MLE header length (excluding the Command Type).
     *
     * @returns The MLE header length (excluding the Command Type).
     *
     */
    uint8_t GetHeaderLength(void) const {
        return sizeof(mSecurityControl) + sizeof(mFrameCounter) + sizeof(mKeySource) + sizeof(mKeyIndex);
    }

    /**
     * This method returns a pointer to first byte of the MLE header.
     *
     * @returns A pointer to the first byte of the MLE header.
     *
     */
    const uint8_t *GetBytes(void) const {
        return reinterpret_cast<const uint8_t *>(&mSecuritySuite);
    }

    /**
     * This method returns the Security Control value.
     *
     * @returns The Security Control value.
     *
     */
    uint8_t GetSecurityControl(void) const { return mSecurityControl; }

    /**
     * This method indicates whether or not the Key ID Mode is set to 2.
     *
     * @retval TRUE   If the Key ID Mode is set to 2.
     * @retval FALSE  If the Key ID Mode is not set to 2.
     *
     */
    bool IsKeyIdMode2(void) const {
        return (mSecurityControl & Mac::Frame::kKeyIdModeMask) == Mac::Frame::kKeyIdMode2;
    }

    /**
     * This method sets the Key ID Mode to 2.
     *
     */
    void SetKeyIdMode2(void) {
        mSecurityControl = (mSecurityControl & ~Mac::Frame::kKeyIdModeMask) | Mac::Frame::kKeyIdMode2;
    }

    /**
     * This method returns the Key ID value.
     *
     * @returns The Key ID value.
     *
     */
    uint32_t GetKeyId(void) const {
        return Encoding::BigEndian::HostSwap32(mKeySource);
    }

    /**
     * This method sets the Key ID value.
     *
     * @param[in]  aKeySequence  The Key ID value.
     *
     */
    void SetKeyId(uint32_t aKeySequence) {
        mKeySource = Encoding::BigEndian::HostSwap32(aKeySequence);
        mKeyIndex = (aKeySequence & 0x7f) + 1;
    }

    /**
     * This method returns the Frame Counter value.
     *
     * @returns The Frame Counter value.
     *
     */
    uint32_t GetFrameCounter(void) const {
        return Encoding::LittleEndian::HostSwap32(mFrameCounter);
    }

    /**
     * This method sets the Frame Counter value.
     *
     * @param[in]  aFrameCounter  The Frame Counter value.
     *
     */
    void SetFrameCounter(uint32_t aFrameCounter) {
        mFrameCounter = Encoding::LittleEndian::HostSwap32(aFrameCounter);
    }

    /**
     * MLE Command Types.
     *
     */
    enum Command
    {
        kCommandLinkRequest          = 0,    ///< Link Reject
        kCommandLinkAccept           = 1,    ///< Link Accept
        kCommandLinkAcceptAndRequest = 2,    ///< Link Accept and Reject
        kCommandLinkReject           = 3,    ///< Link Reject
        kCommandAdvertisement        = 4,    ///< Advertisement
        kCommandUpdate               = 5,    ///< Update
        kCommandUpdateRequest        = 6,    ///< Update Request
        kCommandDataRequest          = 7,    ///< Data Request
        kCommandDataResponse         = 8,    ///< Data Response
        kCommandParentRequest        = 9,    ///< Parent Request
        kCommandParentResponse       = 10,   ///< Parent Response
        kCommandChildIdRequest       = 11,   ///< Child ID Request
        kCommandChildIdResponse      = 12,   ///< Child ID Response
        kCommandChildUpdateRequest   = 13,   ///< Child Update Request
        kCommandChildUpdateResponse  = 14,   ///< Child Update Response
        kCommandAnnounce             = 15,   ///< Announce
        kCommandDiscoveryRequest     = 16,   ///< Discovery Request
        kCommandDiscoveryResponse    = 17,   ///< Discovery Response
    };

    /**
     * This method returns the Command Type value.
     *
     * @returns The Command Type value.
     *
     */
    Command GetCommand(void) const {
        if (mSecuritySuite == kNoSecurity) {
            return static_cast<Command>(mSecurityControl);
        }
        else {
            return static_cast<Command>(mCommand);
        }
    }

    /**
     * This method sets the Command Type value.
     *
     * @param[in]  aCommand  The Command Type value.
     *
     */
    void SetCommand(Command aCommand) {
        if (mSecuritySuite == kNoSecurity) {
            mSecurityControl = static_cast<uint8_t>(aCommand);
        }
        else {
            mCommand = static_cast<uint8_t>(aCommand);
        }
    }

private:
    uint8_t mSecuritySuite;
    uint8_t mSecurityControl;
    uint32_t mFrameCounter;
    uint32_t mKeySource;
    uint8_t mKeyIndex;
    uint8_t mCommand;
} OT_TOOL_PACKED_END;

/**
 * This class implements MLE functionality required by the Thread EndDevices, Router, and Leader roles.
 *
 */
class Mle
{
public:
    /**
     * This constructor initializes the MLE object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    explicit Mle(ThreadNetif &aThreadNetif);

    /**
     * This method enables MLE.
     *
     * @retval kThreadError_None     Successfully enabled MLE.
     * @retval kThreadError_Already  MLE was already enabled.
     *
     */
    ThreadError Enable(void);

    /**
     * This method disables MLE.
     *
     * @retval kThreadError_None     Successfully disabled MLE.
     *
     */
    ThreadError Disable(void);

    /**
     * This method starts the MLE protocol operation.
     *
     * @param[in]  aEnableReattach  True to enable reattach process using stored dataset, False not.
     *
     * @retval kThreadError_None     Successfully started the protocol operation.
     * @retval kThreadError_Already  The protocol operation was already started.
     *
     */
    ThreadError Start(bool aEnableReattach);

    /**
     * This method stops the MLE protocol operation.
     *
     * @param[in]  aClearNetworkDatasets  True to clear network datasets, False not.
     *
     * @retval kThreadError_None  Successfully stopped the protocol operation.
     *
     */
    ThreadError Stop(bool aClearNetworkDatasets);

    /**
     * This method restores network information from non-volatile memory.
     *
     * @retval kThreadError_None      Successfully restore the network information.
     * @retval kThreadError_NotFound  There is no valid network information stored in non-volatile memory.
     *
     */
    ThreadError Restore(void);

    /**
     * This method stores network information into non-volatile memory.
     *
     * @retval kThreadError_None      Successfully store the network information.
     * @retval kThreadError_NoBufs    Could not store the network information due to insufficient memory space.
     *
     */
    ThreadError Store(void);

    /**
     * This function pointer is called on receiving an MLE Discovery Response message.
     *
     * @param[in]  aResult   A valid pointer to the Discovery Response information or NULL when the Discovery completes.
     * @param[in]  aContext  A pointer to application-specific context.
     *
     */
    typedef void (*DiscoverHandler)(otActiveScanResult *aResult, void *aContext);

    /**
     * This method initiates a Thread Discovery.
     *
     * @param[in]  aScanChannels  A bit vector indicating which channels to scan.
     * @param[in]  aScanDuration  The time in milliseconds to spend scanning each channel.
     * @param[in]  aPanId         The PAN ID filter (set to Broadcast PAN to disable filter).
     * @param[in]  aJoiner        Value of the Joiner Flag in the Discovery Request TLV.
     * @param[in]  aHandler       A pointer to a function that is called on receiving an MLE Discovery Response.
     * @param[in]  aContext       A pointer to arbitrary context information.
     *
     * @retval kThreadError_None  Successfully started a Thread Discovery.
     * @retval kThreadError_Busy  Thread Discovery is already in progress.
     *
     */
    ThreadError Discover(uint32_t aScanChannels, uint16_t aScanDuration, uint16_t aPanId, bool aJoiner,
                         DiscoverHandler aCallback, void *aContext);

    /**
     * This method indicates whether or not an MLE Thread Discovery is currently in progress.
     *
     * @returns true if an MLE Thread Discovery is in progress, false otherwise.
     *
     */
    bool IsDiscoverInProgress(void);

    /**
     * This method is called by the MeshForwarder to indicate that discovery is complete.
     *
     */
    void HandleDiscoverComplete(void);

    /**
     * This method generates an MLE Announce message.
     *
     * @param[in]  aChannel        The channel to use when transmitting.
     * @param[in]  aOrphanAnnounce To indicate if MLE Announce is sent from an orphan end device.
     *
     * @retval kThreadError_None    Successfully generated an MLE Announce message.
     * @retval kThreadError_NoBufs  Insufficient buffers to generate the MLE Announce message.
     *
     */
    ThreadError SendAnnounce(uint8_t aChannel, bool aOrphanAnnounce);

    /**
     * This method causes the Thread interface to detach from the Thread network.
     *
     * @retval kThreadError_None          Successfully detached from the Thread network.
     * @retval kThreadError_InvalidState  MLE is Disabled.
     *
     */
    ThreadError BecomeDetached(void);

    /**
     * This method causes the Thread interface to attempt an MLE attach.
     *
     * @param[in]  aFilter  Indicates what partitions to attach to.
     *
     * @retval kThreadError_None          Successfully began the attach process.
     * @retval kThreadError_InvalidState  MLE is Disabled.
     * @retval kThreadError_Busy          An attach process is in progress.
     *
     */
    ThreadError BecomeChild(otMleAttachFilter aFilter);

    /**
     * This method indicates whether or not the Thread device is attached to a Thread network.
     *
     * @retval TRUE   Attached to a Thread network.
     * @retval FALSE  Not attached to a Thread network.
     *
     */
    bool IsAttached(void) const;

    /**
     * This method returns the current Thread interface state.
     *
     * @returns The current Thread interface state.
     *
     */
    DeviceState GetDeviceState(void) const;

    /**
     * This method returns the Device Mode as reported in the Mode TLV.
     *
     * @returns The Device Mode as reported in the Mode TLV.
     *
     */
    uint8_t GetDeviceMode(void) const;

    /**
     * This method sets the Device Mode as reported in the Mode TLV.
     *
     * @retval kThreadError_None         Successfully set the Mode TLV.
     * @retval kThreadError_InvalidArgs  The mode combination specified in @p aMode is invalid.
     *
     */
    ThreadError SetDeviceMode(uint8_t aMode);

    /**
     * This method returns a pointer to the Mesh Local Prefix.
     *
     * @returns A pointer to the Mesh Local Prefix.
     *
     */
    const uint8_t *GetMeshLocalPrefix(void) const;

    /**
     * This method sets the Mesh Local Prefix.
     *
     * @param[in]  aPrefix  A pointer to the Mesh Local Prefix.
     *
     * @retval kThreadError_None  Successfully set the Mesh Local Prefix.
     *
     */
    ThreadError SetMeshLocalPrefix(const uint8_t *aPrefix);

    /**
     * This method updates the link local address.
     *
     * Call this method when the IEEE 802.15.4 Extended Address has changed.
     *
     * @retval kThreadError_None  Successfully updated the link local address.
     *
     */
    ThreadError UpdateLinkLocalAddress(void);

    /**
     * This method returns a pointer to the link-local all Thread nodes multicast address.
     *
     * @returns A pointer to the link-local all Thread nodes multicast address.
     *
     */
    const Ip6::Address *GetLinkLocalAllThreadNodesAddress(void) const;

    /**
     * This method returns a pointer to the realm-local all Thread nodes multicast address.
     *
     * @returns A pointer to the realm-local all Thread nodes multicast address.
     *
     */
    const Ip6::Address *GetRealmLocalAllThreadNodesAddress(void) const;

    /**
     * This method returns a pointer to the parent when operating in End Device mode.
     *
     * @returns A pointer to the parent.
     *
     */
    Router *GetParent(void);

    /**
     * This method indicates whether or not an IPv6 address is an RLOC.
     *
     * @retval TRUE   If @p aAddress is an RLOC.
     * @retval FALSE  If @p aAddress is not an RLOC.
     *
     */
    bool IsRoutingLocator(const Ip6::Address &aAddress) const;

    /**
     * This method indicates whether or not an IPv6 address is an ALOC.
     *
     * @retval TRUE   If @p aAddress is an ALOC.
     * @retval FALSE  If @p aAddress is not an ALOC.
     *
     */
    bool IsAnycastLocator(const Ip6::Address &aAddress) const;

    /**
     * This method returns the MLE Timeout value.
     *
     * @returns The MLE Timeout value.
     *
     */
    uint32_t GetTimeout(void) const;

    /**
     * This method sets the MLE Timeout value.
     *
     */
    ThreadError SetTimeout(uint32_t aTimeout);

    /**
     * This method returns the RLOC16 assigned to the Thread interface.
     *
     * @returns The RLOC16 assigned to the Thread interface.
     *
     */
    uint16_t GetRloc16(void) const;

    /**
     * This method returns a reference to the RLOC assigned to the Thread interface.
     *
     * @returns A reference to the RLOC assigned to the Thread interface.
     *
     */
    const Ip6::Address &GetMeshLocal16(void) const;

    /**
     * This method returns a reference to the ML-EID assigned to the Thread interface.
     *
     * @returns A reference to the ML-EID assigned to the Thread interface.
     *
     */
    const Ip6::Address &GetMeshLocal64(void) const;

    /**
     * This method returns the Router ID of the Leader.
     *
     * @returns The Router ID of the Leader.
     *
     */
    uint8_t GetLeaderId(void) const;

    /**
     * This method retrieves the Leader's RLOC.
     *
     * @param[out]  aAddress  A reference to the Leader's RLOC.
     *
     * @retval kThreadError_None   Successfully retrieved the Leader's RLOC.
     * @retval kThreadError_Error  The Thread interface is not currently attached to a Thread Partition.
     *
     */
    ThreadError GetLeaderAddress(Ip6::Address &aAddress) const;

    /**
     * This method retrieves the Leader's ALOC.
     *
     * @param[out]  aAddress  A reference to the Leader's ALOC.
     *
     * @retval kThreadError_None   Successfully retrieved the Leader's ALOC.
     * @retval kThreadError_Error  The Thread interface is not currently attached to a Thread Partition.
     *
     */
    ThreadError GetLeaderAloc(Ip6::Address &aAddress) const;

    /**
     * This method adds Leader's ALOC to its Thread interface.
     *
     * @retval kThreadError_None           Successfully added the Leader's ALOC.
     * @retval kThreadError_Busy           The Leader's ALOC address was already added.
     * @retval kThreadError_InvalidState   The device's role is not Leader.
     *
     */
    ThreadError AddLeaderAloc(void);

    /**
     * This method returns the most recently received Leader Data TLV.
     *
     * @returns  A reference to the most recently received Leader Data TLV.
     *
     */
    const LeaderDataTlv &GetLeaderDataTlv(void);

    /**
     * This method gets the Leader Data.
     *
     * @param[out]  aLeaderData  A reference to where the leader data is placed.
     *
     * @retval kThreadError_None         Successfully retrieved the leader data.
     * @retval kThreadError_Detached     Not currently attached.
     *
     */
    ThreadError GetLeaderData(otLeaderData &aLeaderData);

    /**
     * This method returns the link quality on the link to a given extended address.
     *
     * @param[in]  aMacAddr  The IEEE 802.15.4 Extended Mac Address.
     * @param[in]  aLinkQuality A reference to the assigned link quality.
     *
     * @retval kThreadError_None         Successfully retrieve the link quality to aLinkQuality.
     * @retval kThreadError_InvalidArgs  No match found with a given extended address.
     *
     */
    ThreadError GetAssignLinkQuality(const Mac::ExtAddress aMacAddr, uint8_t &aLinkQuality);

    /**
     * This method sets the link quality on the link to a given extended address.
     *
     * @param[in]  aMacAddr  The IEEE 802.15.4 Extended Mac Address.
     * @param[in]  aLinkQaulity The link quality to be set on the link.
     *
     */
    void SetAssignLinkQuality(const Mac::ExtAddress aMacAddr, uint8_t aLinkQuality);

    /**
     * This method returns the ROUTER_SELECTION_JITTER value.
     *
     * @returns The ROUTER_SELECTION_JITTER value.
     *
     */
    uint8_t GetRouterSelectionJitter(void) const;

    /**
     * This method sets the ROUTER_SELECTION_JITTER value.
     *
     * @returns The ROUTER_SELECTION_JITTER value.
     *
     */
    void SetRouterSelectionJitter(uint8_t aRouterJitter);

    /**
     * This method returns the Child ID portion of an RLOC16.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     * @returns The Child ID portion of an RLOC16.
     *
     */
    static uint16_t GetChildId(uint16_t aRloc16) { return aRloc16 & kMaxChildId; }

    /**
     * This method returns the Router ID portion of an RLOC16.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     * @returns The Router ID portion of an RLOC16.
     *
     */
    static uint8_t GetRouterId(uint16_t aRloc16) { return aRloc16 >> kRouterIdOffset; }

    /**
     * This method returns the RLOC16 of a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID value.
     *
     * @returns The RLOC16 of the given Router ID.
     *
     */
    static uint16_t GetRloc16(uint8_t aRouterId) { return static_cast<uint16_t>(aRouterId << kRouterIdOffset); }

    /**
     * This method indicates whether or not @p aRloc16 refers to an active router.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     * @retval TRUE   If @p aRloc16 refers to an active router.
     * @retval FALSE  If @p aRloc16 does not refer to an active router.
     *
     */
    static bool IsActiveRouter(uint16_t aRloc16) { return GetChildId(aRloc16) == 0; }

    /**
     * This method fills the NetworkDataTlv.
     *
     * @param[out] aTlv         The NetworkDataTlv.
     * @param[in]  aStableOnly  TRUE to append stable data, FALSE otherwise.
     *
     */
    void FillNetworkDataTlv(NetworkDataTlv &aTlv, bool aStableOnly);

protected:

    /**
     * This method allocates a new message buffer for preparing an MLE message.
     *
     * @returns A pointer to the message or NULL if insufficient message buffers are available.
     *
     */
    Message *NewMleMessage(void);

    /**
     * This method appends an MLE header to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     * @param[in]  aCommand  The MLE Command Type.
     *
     * @retval kThreadError_None    Successfully appended the header.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the header.
     *
     */
    ThreadError AppendHeader(Message &aMessage, Header::Command aCommand);

    /**
     * This method appends a Source Address TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None    Successfully appended the Source Address TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Source Address TLV.
     *
     */
    ThreadError AppendSourceAddress(Message &aMessage);

    /**
     * This method appends a Mode TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     * @param[in]  aMode     The Device Mode value.
     *
     * @retval kThreadError_None    Successfully appended the Mode TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Mode TLV.
     *
     */
    ThreadError AppendMode(Message &aMessage, uint8_t aMode);

    /**
     * This method appends a Timeout TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     * @param[in]  aTimeout  The Timeout value.
     *
     * @retval kThreadError_None    Successfully appended the Timeout TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Timeout TLV.
     *
     */
    ThreadError AppendTimeout(Message &aMessage, uint32_t aTimeout);

    /**
     * This method appends a Challenge TLV to a message.
     *
     * @param[in]  aMessage          A reference to the message.
     * @param[in]  aChallenge        A pointer to the Challenge value.
     * @param[in]  aChallengeLength  The length of the Challenge value in bytes.
     *
     * @retval kThreadError_None    Successfully appended the Challenge TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Challenge TLV.
     *
     */
    ThreadError AppendChallenge(Message &aMessage, const uint8_t *aChallenge, uint8_t aChallengeLength);

    /**
     * This method appends a Response TLV to a message.
     *
     * @param[in]  aMessage         A reference to the message.
     * @param[in]  aResponse        A pointer to the Response value.
     * @param[in]  aResponseLength  The length of the Response value in bytes.
     *
     * @retval kThreadError_None    Successfully appended the Response TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Response TLV.
     *
     */
    ThreadError AppendResponse(Message &aMessage, const uint8_t *aResponse, uint8_t aResponseLength);

    /**
     * This method appends a Link Frame Counter TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None    Successfully appended the Link Frame Counter TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Link Frame Counter TLV.
     *
     */
    ThreadError AppendLinkFrameCounter(Message &aMessage);

    /**
     * This method appends an MLE Frame Counter TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None    Successfully appended the Frame Counter TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the MLE Frame Counter TLV.
     *
     */
    ThreadError AppendMleFrameCounter(Message &aMessage);

    /**
     * This method appends an Address16 TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     * @param[in]  aRloc16   The RLOC16 value.
     *
     * @retval kThreadError_None    Successfully appended the Address16 TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Address16 TLV.
     *
     */
    ThreadError AppendAddress16(Message &aMessage, uint16_t aRloc16);

    /**
     * This method appends a Network Data TLV to the message.
     *
     * @param[in]  aMessage     A reference to the message.
     * @param[in]  aStableOnly  TRUE to append stable data, FALSE otherwise.
     *
     * @retval kThreadError_None    Successfully appended the Network Data TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Network Data TLV.
     *
     */
    ThreadError AppendNetworkData(Message &aMessage, bool aStableOnly);

    /**
     * This method appends a TLV Request TLV to a message.
     *
     * @param[in]  aMessage     A reference to the message.
     * @param[in]  aTlvs        A pointer to the list of TLV types.
     * @param[in]  aTlvsLength  The number of TLV types in @p aTlvs
     *
     * @retval kThreadError_None    Successfully appended the TLV Request TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the TLV Request TLV.
     *
     */
    ThreadError AppendTlvRequest(Message &aMessage, const uint8_t *aTlvs, uint8_t aTlvsLength);

    /**
     * This method appends a Leader Data TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None    Successfully appended the Leader Data TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Leader Data TLV.
     *
     */
    ThreadError AppendLeaderData(Message &aMessage);

    /**
     * This method appends a Scan Mask TLV to a message.
     *
     * @param[in]  aMessage   A reference to the message.
     * @param[in]  aScanMask  The Scan Mask value.
     *
     * @retval kThreadError_None    Successfully appended the Scan Mask TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Scan Mask TLV.
     *
     */
    ThreadError AppendScanMask(Message &aMessage, uint8_t aScanMask);

    /**
     * This method appends a Status TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     * @param[in]  aStatus   The Status value.
     *
     * @retval kThreadError_None    Successfully appended the Status TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Status TLV.
     *
     */
    ThreadError AppendStatus(Message &aMessage, StatusTlv::Status aStatus);

    /**
     * This method appends a Link Margin TLV to a message.
     *
     * @param[in]  aMessage     A reference to the message.
     * @param[in]  aLinkMargin  The Link Margin value.
     *
     * @retval kThreadError_None    Successfully appended the Link Margin TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Link Margin TLV.
     *
     */
    ThreadError AppendLinkMargin(Message &aMessage, uint8_t aLinkMargin);

    /**
     * This method appends a Version TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None    Successfully appended the Version TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Version TLV.
     *
     */
    ThreadError AppendVersion(Message &aMessage);

    /**
     * This method appends an Address Registration TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None    Successfully appended the Address Registration TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Address Registration TLV.
     *
     */
    ThreadError AppendAddressRegistration(Message &aMessage);

    /**
     * This method appends a Active Timestamp TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     * @param[in]  aCouldUseLocal  True to use local Active Timestamp when network Active Timestamp is not available, False not.
     *
     * @retval kThreadError_None    Successfully appended the Active Timestamp TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Active Timestamp TLV.
     *
     */
    ThreadError AppendActiveTimestamp(Message &aMessage, bool aCouldUseLocal);

    /**
     * This method appends a Pending Timestamp TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None    Successfully appended the Pending Timestamp TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Pending Timestamp TLV.
     *
     */
    ThreadError AppendPendingTimestamp(Message &aMessage);

    /**
     * This method appends a Thread Discovery TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None    Successfully appended the Thread Discovery TLV.
     * @retval kThreadError_NoBufs  Insufficient buffers available to append the Address Registration TLV.
     *
     */
    ThreadError AppendDiscovery(Message &aMessage);

    /**
     * This method checks if the destination is reachable.
     *
     * @param[in]  aMeshSource  The RLOC16 of the source.
     * @param[in]  aMeshDest    The RLOC16 of the destination.
     * @param[in]  aIp6Header   The IPv6 header of the message.
     *
     * @retval kThreadError_None  The destination is reachable.
     * @retval kThreadError_Drop  The destination is not reachable and the message should be dropped.
     *
     */
    ThreadError CheckReachability(uint16_t aMeshSource, uint16_t aMeshDest, Ip6::Header &aIp6Header);

    /**
     * This method returns a pointer to the neighbor object.
     *
     * @param[in]  aAddress  A reference to the MAC address.
     *
     * @returns A pointer to the neighbor object.
     *
     */
    Neighbor *GetNeighbor(const Mac::Address &aAddress);

    /**
     * This method returns a pointer to the neighbor object.
     *
     * @param[in]  aAddress  A reference to the MAC short address.
     *
     * @returns A pointer to the neighbor object.
     *
     */
    Neighbor *GetNeighbor(Mac::ShortAddress aAddress);

    /**
     * This method returns a pointer to the neighbor object.
     *
     * @param[in]  aAddress  A reference to the MAC extended address.
     *
     * @returns A pointer to the neighbor object.
     *
     */
    Neighbor *GetNeighbor(const Mac::ExtAddress &aAddress);

    /**
     * This method returns a pointer to the neighbor object.
     *
     * @param[in]  aAddress  A reference to the IPv6 address.
     *
     * @returns A pointer to the neighbor object.
     *
     */
    Neighbor *GetNeighbor(const Ip6::Address &aAddress);

    /**
     * This method returns the next hop towards an RLOC16 destination.
     *
     * @param[in]  aDestination  The RLOC16 of the destination.
     *
     * @returns A RLOC16 of the next hop if a route is known, kInvalidRloc16 otherwise.
     *
     */
    Mac::ShortAddress GetNextHop(uint16_t aDestination) const;

    /**
     * This method generates an MLE Data Request message.
     *
     * @param[in]  aDestination  A reference to the IPv6 address of the destination.
     * @param[in]  aTlvs         A pointer to requested TLV types.
     * @param[in]  aTlvsLength   The number of TLV types in @p aTlvs.
     *
     * @retval kThreadError_None    Successfully generated an MLE Data Request message.
     * @retval kThreadError_NoBufs  Insufficient buffers to generate the MLE Data Request message.
     *
     */
    ThreadError SendDataRequest(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength);

    /**
     * This method generates an MLE Child Update Request message.
     *
     * @retval kThreadError_None    Successfully generated an MLE Child Update Request message.
     * @retval kThreadError_NoBufs  Insufficient buffers to generate the MLE Child Update Request message.
     *
     */
    ThreadError SendChildUpdateRequest(void);

    /**
     * This method generates an MLE Child Update Response message.
     *
     * @param[in]  aTlvs         A pointer to requested TLV types.
     * @param[in]  aNumTlvs      The number of TLV types in @p aTlvs.
     * @param[in]  aChallenge    The Challenge TLV for the response.
     *
     * @retval kThreadError_None    Successfully generated an MLE Child Update Response message.
     * @retval kThreadError_NoBufs  Insufficient buffers to generate the MLE Child Update Response message.
     *
     */
    ThreadError SendChildUpdateResponse(const uint8_t *aTlvs, uint8_t aNumTlvs, const ChallengeTlv &aChallenge);

    /**
     * This method submits an MLE message to the UDP socket.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aDestination  A reference to the IPv6 address of the destination.
     *
     * @retval kThreadError_None    Successfully submitted the MLE message.
     * @retval kThreadError_NoBufs  Insufficient buffers to form the rest of the MLE message.
     *
     */
    ThreadError SendMessage(Message &aMessage, const Ip6::Address &aDestination);

    /**
     * This method sets the RLOC16 assigned to the Thread interface.
     *
     * @param[in]  aRloc16  The RLOC16 to set.
     *
     * @retval kThreadError_None  Successfully set the RLOC16.
     *
     */
    ThreadError SetRloc16(uint16_t aRloc16);

    /**
     * This method sets the Device State to Detached.
     *
     * @retval kThreadError_None  Successfully set the Device State to Detached.
     *
     */
    ThreadError SetStateDetached(void);

    /**
     * This method sets the Device State to Child.
     *
     * @retval kThreadError_None  Successfully set the Device State to Child.
     *
     */
    ThreadError SetStateChild(uint16_t aRloc16);

    /**
     * This method sets the Leader's Partition ID, Weighting, and Router ID values.
     *
     * @param[in]  aPartitionId     The Leader's Partition ID value.
     * @param[in]  aWeighting       The Leader's Weighting value.
     * @param[in]  aLeaderRouterId  The Leader's Router ID value.
     *
     */
    void SetLeaderData(uint32_t aPartitionId, uint8_t aWeighting, uint8_t aLeaderRouterId);

    ThreadNetif           &mNetif;            ///< The Thread Network Interface object.

    LeaderDataTlv mLeaderData;              ///< Last received Leader Data TLV.
    bool mRetrieveNewNetworkData;           ///< Indicating new Network Data is needed if set.

    DeviceState mDeviceState;               ///< Current Thread interface state.
    Router mParent;                         ///< Parent information.
    uint8_t mDeviceMode;                    ///< Device mode setting.

    bool isAssignLinkQuality;    ///< Indicating an assigned link quality is used on the link
    uint8_t mAssignLinkQuality;  ///< The assigned link quality value
    uint8_t mAssignLinkMargin;   ///< The maximum link margin corresponding to mAssignLinkQuality
    Mac::ExtAddress mAddr64;     ///< A given IEEE 802.15.4 Extended Address

    /**
     * States when searching for a parent.
     *
     */
    enum ParentRequestState
    {
        kParentIdle,           ///< Not currently searching for a parent.
        kParentSynchronize,    ///< Looking to synchronize with a parent (after reset).
        kParentRequestStart,   ///< Starting to look for a parent.
        kParentRequestRouter,  ///< Searching for a Router to attach to.
        kParentRequestChild,   ///< Searching for Routers or REEDs to attach to.
        kChildIdRequest,       ///< Sending a Child ID Request message.
    };
    ParentRequestState mParentRequestState;  ///< The parent request state.

    /**
     * States when reattaching network using stored dataset
     *
     */
    enum ReattachState
    {
        kReattachStop       = 0,   ///< Reattach process is disabled or finished
        kReattachStart      = 1,   ///< Start reattach process
        kReattachActive     = 2,   ///< Reattach using stored Active Dataset
        kReattachPending    = 3,   ///< Reattach using stored Pending Dataset
    };
    ReattachState mReattachState;

    Timer mParentRequestTimer;  ///< The timer for driving the Parent Request process.

    uint8_t mRouterSelectionJitter;         ///< The variable to save the assigned jitter value.
    uint8_t mRouterSelectionJitterTimeout;  ///< The Timeout prior to request/release Router ID.

    uint8_t mLastPartitionRouterIdSequence;
    uint32_t mLastPartitionId;
private:
    enum
    {
        kAttachDataPollPeriod = OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD,
        kMleMessagePriority = Message::kPriorityHigh,
    };

    void GenerateNonce(const Mac::ExtAddress &aMacAddr, uint32_t aFrameCounter, uint8_t aSecurityLevel,
                       uint8_t *aNonce);

    static void HandleNetifStateChanged(uint32_t aFlags, void *aContext);
    void HandleNetifStateChanged(uint32_t aFlags);
    static void HandleParentRequestTimer(void *aContext);
    void HandleParentRequestTimer(void);
    static void HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo);
    void HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    static void HandleSendChildUpdateRequest(void *aContext);
    void HandleSendChildUpdateRequest(void);

    ThreadError HandleAdvertisement(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleChildIdResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleChildUpdateRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleChildUpdateResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleDataResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleParentResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                     uint32_t aKeySequence);
    ThreadError HandleAnnounce(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleDiscoveryResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleLeaderData(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    ThreadError SendParentRequest(void);
    ThreadError SendChildIdRequest(void);
    void SendOrphanAnnounce(void);

    bool IsBetterParent(uint16_t aRloc16, uint8_t aLinkQuality, ConnectivityTlv &aConnectivityTlv) const;
    void ResetParentCandidate(void);

    /**
     * This struct represents the device's own network information for persistent storage.
     *
     */
    typedef struct NetworkInfo
    {
        DeviceState          mDeviceState;                ///< Current Thread interface state.

        uint8_t              mDeviceMode;                 ///< Device mode setting.
        uint16_t             mRloc16;                     ///< RLOC16
        uint32_t             mKeySequence;                ///< Key Sequence
        uint32_t             mMleFrameCounter;            ///< MLE Frame Counter
        uint32_t             mMacFrameCounter;            ///< MAC Frame Counter
        uint32_t             mPreviousPartitionId;        ///< PartitionId
        Mac::ExtAddress      mExtAddress;                 ///< Extended Address
    } NetworkInfo;

    struct
    {
        uint8_t mChallenge[ChallengeTlv::kMaxSize];
        uint8_t mChallengeLength;
    } mChildIdRequest;

    struct
    {
        uint8_t mChallenge[ChallengeTlv::kMaxSize];
    } mParentRequest;

    otMleAttachFilter mParentRequestMode;
    uint8_t mParentLinkQuality;
    int8_t mParentPriority;
    uint8_t mParentLinkQuality3;
    uint8_t mParentLinkQuality2;
    uint8_t mParentLinkQuality1;
    LeaderDataTlv mParentLeaderData;
    bool mParentIsSingleton;

    Router mParentCandidate;

    Ip6::UdpSocket mSocket;
    uint32_t mTimeout;

    Tasklet mSendChildUpdateRequest;

    DiscoverHandler mDiscoverHandler;
    void *mDiscoverContext;
    bool mIsDiscoverInProgress;

    uint8_t mAnnounceChannel;
    uint8_t mPreviousChannel;
    uint16_t mPreviousPanId;

    Ip6::NetifUnicastAddress mLeaderAloc;

    Ip6::NetifUnicastAddress mLinkLocal64;
    Ip6::NetifUnicastAddress mMeshLocal64;
    Ip6::NetifUnicastAddress mMeshLocal16;
    Ip6::NetifMulticastAddress mLinkLocalAllThreadNodes;
    Ip6::NetifMulticastAddress mRealmLocalAllThreadNodes;

    Ip6::NetifCallback mNetifCallback;
};

}  // namespace Mle

/**
 * @}
 *
 */

}  // namespace Thread

#endif  // MLE_HPP_
