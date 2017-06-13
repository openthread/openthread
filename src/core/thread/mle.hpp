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

#include <openthread/openthread.h>

#include "common/encoding.hpp"
#include "common/timer.hpp"
#include "mac/mac.hpp"
#include "meshcop/joiner_router.hpp"
#include "net/udp6.hpp"
#include "thread/mle_constants.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/topology.hpp"

namespace ot {

class ThreadNetif;
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
 * @namespace ot::Mle
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
 * MLE Attach modes
 *
 */
enum AttachMode
{
    kAttachAny    = 0, ///< Attach to any Thread partition.
    kAttachSame1  = 1, ///< Attach to the same Thread partition (attempt 1).
    kAttachSame2  = 2, ///< Attach to the same Thread partition (attempt 2).
    kAttachBetter = 3, ///< Attach to a better (i.e. higher weight/partition id) Thread partition.
};

/**
 * This enumeration represents the allocation of the ALOC Space
 *
 */
enum AlocAllocation
{
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
    void Init(void) {
        mSecuritySuite = k154Security; mSecurityControl = Mac::Frame::kSecEncMic32;
    }

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

        if (mSecuritySuite == k154Security)
        {
            rval += sizeof(mSecurityControl) + sizeof(mFrameCounter) + sizeof(mKeySource) + sizeof(mKeyIndex);
        }

        return rval;
    }

    enum SecuritySuite
    {
        k154Security = 0,   ///< IEEE 802.15.4-2006 security.
        kNoSecurity  = 255, ///< No security enabled.
    };

    /**
     * This method returns the Security Suite value.
     *
     * @returns The Security Suite value.
     *
     */
    SecuritySuite GetSecuritySuite(void) const {
        return static_cast<SecuritySuite>(mSecuritySuite);
    }

    /**
     * This method sets the Security Suite value.
     *
     * @param[in]  aSecuritySuite  The Security Suite value.
     *
     */
    void SetSecuritySuite(SecuritySuite aSecuritySuite) {
        mSecuritySuite = static_cast<uint8_t>(aSecuritySuite);
    }

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
    uint8_t GetSecurityControl(void) const {
        return mSecurityControl;
    }

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
        kCommandLinkRequest          = 0,  ///< Link Reject
        kCommandLinkAccept           = 1,  ///< Link Accept
        kCommandLinkAcceptAndRequest = 2,  ///< Link Accept and Reject
        kCommandLinkReject           = 3,  ///< Link Reject
        kCommandAdvertisement        = 4,  ///< Advertisement
        kCommandUpdate               = 5,  ///< Update
        kCommandUpdateRequest        = 6,  ///< Update Request
        kCommandDataRequest          = 7,  ///< Data Request
        kCommandDataResponse         = 8,  ///< Data Response
        kCommandParentRequest        = 9,  ///< Parent Request
        kCommandParentResponse       = 10, ///< Parent Response
        kCommandChildIdRequest       = 11, ///< Child ID Request
        kCommandChildIdResponse      = 12, ///< Child ID Response
        kCommandChildUpdateRequest   = 13, ///< Child Update Request
        kCommandChildUpdateResponse  = 14, ///< Child Update Response
        kCommandAnnounce             = 15, ///< Announce
        kCommandDiscoveryRequest     = 16, ///< Discovery Request
        kCommandDiscoveryResponse    = 17, ///< Discovery Response
    };

    /**
     * This method returns the Command Type value.
     *
     * @returns The Command Type value.
     *
     */
    Command GetCommand(void) const {
        if (mSecuritySuite == kNoSecurity)
        {
            return static_cast<Command>(mSecurityControl);
        }
        else
        {
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
        if (mSecuritySuite == kNoSecurity)
        {
            mSecurityControl = static_cast<uint8_t>(aCommand);
        }
        else
        {
            mCommand = static_cast<uint8_t>(aCommand);
        }
    }

private:
    uint8_t  mSecuritySuite;
    uint8_t  mSecurityControl;
    uint32_t mFrameCounter;
    uint32_t mKeySource;
    uint8_t  mKeyIndex;
    uint8_t  mCommand;
} OT_TOOL_PACKED_END;

/**
 * This class implements functionality required for delaying MLE responses.
 *
 */
OT_TOOL_PACKED_BEGIN
class DelayedResponseHeader
{
public:
    /**
     * Default constructor for the object.
     *
     */
    DelayedResponseHeader(void) {
        memset(this, 0, sizeof(*this));
    }

    /**
     * This constructor initializes the object with specific values.
     *
     * @param[in]  aSendTime     Time when the message shall be sent.
     * @param[in]  aDestination  IPv6 address of the message destination.
     *
     */
    DelayedResponseHeader(uint32_t aSendTime, const Ip6::Address &aDestination) {
        mSendTime = aSendTime;
        mDestination = aDestination;
    }

    /**
     * This method appends delayed response header to the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the bytes.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to grow the message.
     *
     */
    otError AppendTo(Message &aMessage) {
        return aMessage.Append(this, sizeof(*this));
    }

    /**
     * This method reads delayed response header from the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @returns The number of bytes read.
     *
     */
    uint16_t ReadFrom(Message &aMessage) {
        return aMessage.Read(aMessage.GetLength() - sizeof(*this), sizeof(*this), this);
    }

    /**
     * This method removes delayed response header from the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE  Successfully removed the header.
     *
     */
    static otError RemoveFrom(Message &aMessage) {
        return aMessage.SetLength(aMessage.GetLength() - sizeof(DelayedResponseHeader));
    }

    /**
     * This method returns a time when the message shall be sent.
     *
     * @returns  A time when the message shall be sent.
     *
     */
    uint32_t GetSendTime(void) const {
        return mSendTime;
    }

    /**
     * This method returns a destination of the delayed message.
     *
     * @returns  A destination of the delayed message.
     *
     */
    const Ip6::Address &GetDestination(void) const {
        return mDestination;
    }

    /**
     * This method checks if the message shall be sent before the given time.
     *
     * @param[in]  aTime  A time to compare.
     *
     * @retval TRUE   If the message shall be sent before the given time.
     * @retval FALSE  Otherwise.
     */
    bool IsEarlier(uint32_t aTime) {
        return (static_cast<int32_t>(aTime - mSendTime) > 0);
    }

    /**
     * This method checks if the message shall be sent after the given time.
     *
     * @param[in]  aTime  A time to compare.
     *
     * @retval TRUE   If the message shall be sent after the given time.
     * @retval FALSE  Otherwise.
     */
    bool IsLater(uint32_t aTime) {
        return (static_cast<int32_t>(aTime - mSendTime) < 0);
    }

private:
    Ip6::Address mDestination; ///< IPv6 address of the message destination.
    uint32_t     mSendTime;    ///< Time when the message shall be sent.
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
     * This method returns the pointer to the parent otInstance structure.
     *
     * @returns The pointer to the parent otInstance structure.
     *
     */
    otInstance *GetInstance(void);

    /**
     * This method enables MLE.
     *
     * @retval OT_ERROR_NONE     Successfully enabled MLE.
     * @retval OT_ERROR_ALREADY  MLE was already enabled.
     *
     */
    otError Enable(void);

    /**
     * This method disables MLE.
     *
     * @retval OT_ERROR_NONE     Successfully disabled MLE.
     *
     */
    otError Disable(void);

    /**
     * This method starts the MLE protocol operation.
     *
     * @param[in]  aEnableReattach True if reattach using stored dataset, or False if not.
     * @param[in]  aAnnounceAttach True if attach on the announced thread network with newer active timestamp,
     *                             or False if not.
     *
     * @retval OT_ERROR_NONE     Successfully started the protocol operation.
     * @retval OT_ERROR_ALREADY  The protocol operation was already started.
     *
     */
    otError Start(bool aEnableReattach, bool aAnnounceAttach);

    /**
     * This method stops the MLE protocol operation.
     *
     * @param[in]  aClearNetworkDatasets  True to clear network datasets, False not.
     *
     * @retval OT_ERROR_NONE  Successfully stopped the protocol operation.
     *
     */
    otError Stop(bool aClearNetworkDatasets);

    /**
     * This method restores network information from non-volatile memory.
     *
     * @retval OT_ERROR_NONE       Successfully restore the network information.
     * @retval OT_ERROR_NOT_FOUND  There is no valid network information stored in non-volatile memory.
     *
     */
    otError Restore(void);

    /**
     * This method stores network information into non-volatile memory.
     *
     * @retval OT_ERROR_NONE       Successfully store the network information.
     * @retval OT_ERROR_NO_BUFS    Could not store the network information due to insufficient memory space.
     *
     */
    otError Store(void);

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
     * @param[in]  aScanChannels          A bit vector indicating which channels to scan.
     * @param[in]  aPanId                 The PAN ID filter (set to Broadcast PAN to disable filter).
     * @param[in]  aJoiner                Value of the Joiner Flag in the Discovery Request TLV.
     * @param[in]  aEnableEui64Filtering  Enable filtering out MLE discovery responses that don't match our factory
     *                                    assigned EUI64.
     * @param[in]  aHandler               A pointer to a function that is called on receiving an MLE Discovery Response.
     * @param[in]  aContext               A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE  Successfully started a Thread Discovery.
     * @retval OT_ERROR_BUSY  Thread Discovery is already in progress.
     *
     */
    otError Discover(uint32_t aScanChannels, uint16_t aPanId, bool aJoiner, bool aEnableEui64Filtering,
                     DiscoverHandler aCallback,
                     void *aContext);

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
     * @retval OT_ERROR_NONE     Successfully generated an MLE Announce message.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers to generate the MLE Announce message.
     *
     */
    otError SendAnnounce(uint8_t aChannel, bool aOrphanAnnounce);

    /**
     * This method causes the Thread interface to detach from the Thread network.
     *
     * @retval OT_ERROR_NONE           Successfully detached from the Thread network.
     * @retval OT_ERROR_INVALID_STATE  MLE is Disabled.
     *
     */
    otError BecomeDetached(void);

    /**
     * This method causes the Thread interface to attempt an MLE attach.
     *
     * @param[in]  aMode  Indicates what partitions to attach to.
     *
     * @retval OT_ERROR_NONE           Successfully began the attach process.
     * @retval OT_ERROR_INVALID_STATE  MLE is Disabled.
     * @retval OT_ERROR_BUSY           An attach process is in progress.
     *
     */
    otError BecomeChild(AttachMode aMode);

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
    otDeviceRole GetRole(void) const {
        return mRole;
    }

    /**
     * This method returns the Device Mode as reported in the Mode TLV.
     *
     * @returns The Device Mode as reported in the Mode TLV.
     *
     */
    uint8_t GetDeviceMode(void) const {
        return mDeviceMode;
    }

    /**
     * This method indicates whether or not the device is a Minimal End Device.
     *
     * @returns TRUE if the device is a Minimal End Device, FALSE otherwise.
     *
     */
    bool IsMinimalEndDevice(void) const {
        return (mDeviceMode & (ModeTlv::kModeFFD | ModeTlv::kModeRxOnWhenIdle)) !=
               (ModeTlv::kModeFFD | ModeTlv::kModeRxOnWhenIdle);
    }

    /**
     * This method sets the Device Mode as reported in the Mode TLV.
     *
     * @retval OT_ERROR_NONE          Successfully set the Mode TLV.
     * @retval OT_ERROR_INVALID_ARGS  The mode combination specified in @p aMode is invalid.
     *
     */
    otError SetDeviceMode(uint8_t aMode);

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
     * @retval OT_ERROR_NONE  Successfully set the Mesh Local Prefix.
     *
     */
    otError SetMeshLocalPrefix(const uint8_t *aPrefix);

    /**
     * This method returns a reference to the Thread link-local address.
     *
     * The Thread link local address is derived using IEEE802.15.4 Extended Address as Interface Identifier.
     *
     * @returns A reference to the Thread link local address.
     *
     */
    const Ip6::Address &GetLinkLocalAddress(void) const;

    /**
     * This method updates the link local address.
     *
     * Call this method when the IEEE 802.15.4 Extended Address has changed.
     *
     * @retval OT_ERROR_NONE  Successfully updated the link local address.
     *
     */
    otError UpdateLinkLocalAddress(void);

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
     * This method indicates whether or not an IPv6 address is a Mesh Local Address.
     *
     * @retval TRUE   If @p aAddress is a Mesh Local Address.
     * @retval FALSE  If @p aAddress is not a Mesh Local Address.
     *
     */
    bool IsMeshLocalAddress(const Ip6::Address &aAddress) const;

    /**
     * This method returns the MLE Timeout value.
     *
     * @returns The MLE Timeout value.
     *
     */
    uint32_t GetTimeout(void) const {
        return mTimeout;
    }

    /**
     * This method sets the MLE Timeout value.
     *
     */
    otError SetTimeout(uint32_t aTimeout);

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
     * @retval OT_ERROR_NONE      Successfully retrieved the Leader's RLOC.
     * @retval OT_ERROR_DETACHED  The Thread interface is not currently attached to a Thread Partition.
     *
     */
    otError GetLeaderAddress(Ip6::Address &aAddress) const;

    /**
     * This method retrieves the Leader's ALOC.
     *
     * @param[out]  aAddress  A reference to the Leader's ALOC.
     *
     * @retval OT_ERROR_NONE      Successfully retrieved the Leader's ALOC.
     * @retval OT_ERROR_DETACHED  The Thread interface is not currently attached to a Thread Partition.
     *
     */
    otError GetLeaderAloc(Ip6::Address &aAddress) const;

    /**
     * This method adds Leader's ALOC to its Thread interface.
     *
     * @retval OT_ERROR_NONE            Successfully added the Leader's ALOC.
     * @retval OT_ERROR_BUSY            The Leader's ALOC address was already added.
     * @retval OT_ERROR_INVALID_STATE   The device's role is not Leader.
     *
     */
    otError AddLeaderAloc(void);

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
     * @retval OT_ERROR_NONE         Successfully retrieved the leader data.
     * @retval OT_ERROR_DETACHED     Not currently attached.
     *
     */
    otError GetLeaderData(otLeaderData &aLeaderData);

    /**
     * This method returns the link quality on the link to a given extended address.
     *
     * @param[in]  aMacAddr  The IEEE 802.15.4 Extended Mac Address.
     * @param[in]  aLinkQuality A reference to the assigned link quality.
     *
     * @retval OT_ERROR_NONE          Successfully retrieve the link quality to aLinkQuality.
     * @retval OT_ERROR_INVALID_ARGS  No match found with a given extended address.
     *
     */
    otError GetAssignLinkQuality(const Mac::ExtAddress aMacAddr, uint8_t &aLinkQuality);

    /**
     * This method sets the link quality on the link to a given extended address.
     *
     * @param[in]  aMacAddr  The IEEE 802.15.4 Extended Mac Address.
     * @param[in]  aLinkQaulity The link quality to be set on the link.
     *
     */
    void SetAssignLinkQuality(const Mac::ExtAddress aMacAddr, uint8_t aLinkQuality);

    /**
     * This method returns the Child ID portion of an RLOC16.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     * @returns The Child ID portion of an RLOC16.
     *
     */
    static uint16_t GetChildId(uint16_t aRloc16) {
        return aRloc16 & kMaxChildId;
    }

    /**
     * This method returns the Router ID portion of an RLOC16.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     * @returns The Router ID portion of an RLOC16.
     *
     */
    static uint8_t GetRouterId(uint16_t aRloc16) {
        return aRloc16 >> kRouterIdOffset;
    }

    /**
     * This method returns the RLOC16 of a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID value.
     *
     * @returns The RLOC16 of the given Router ID.
     *
     */
    static uint16_t GetRloc16(uint8_t aRouterId) {
        return static_cast<uint16_t>(aRouterId << kRouterIdOffset);
    }

    /**
     * This method indicates whether or not @p aRloc16 refers to an active router.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     * @retval TRUE   If @p aRloc16 refers to an active router.
     * @retval FALSE  If @p aRloc16 does not refer to an active router.
     *
     */
    static bool IsActiveRouter(uint16_t aRloc16) {
        return GetChildId(aRloc16) == 0;
    }

    /**
     * This method fills the NetworkDataTlv.
     *
     * @param[out] aTlv         The NetworkDataTlv.
     * @param[in]  aStableOnly  TRUE to append stable data, FALSE otherwise.
     *
     */
    void FillNetworkDataTlv(NetworkDataTlv &aTlv, bool aStableOnly);

    /**
     * This method returns a reference to the send queue.
     *
     * @returns A reference to the send queue.
     *
     */
    const MessageQueue &GetMessageQueue(void) const {
        return mDelayedResponses;
    }

protected:
    enum
    {
        kMleMaxResponseDelay = 1000u, ///< Maximum delay before responding to a multicast request.
    };

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
     * @retval OT_ERROR_NONE     Successfully appended the header.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the header.
     *
     */
    otError AppendHeader(Message &aMessage, Header::Command aCommand);

    /**
     * This method appends a Source Address TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Source Address TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Source Address TLV.
     *
     */
    otError AppendSourceAddress(Message &aMessage);

    /**
     * This method appends a Mode TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     * @param[in]  aMode     The Device Mode value.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Mode TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Mode TLV.
     *
     */
    otError AppendMode(Message &aMessage, uint8_t aMode);

    /**
     * This method appends a Timeout TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     * @param[in]  aTimeout  The Timeout value.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Timeout TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Timeout TLV.
     *
     */
    otError AppendTimeout(Message &aMessage, uint32_t aTimeout);

    /**
     * This method appends a Challenge TLV to a message.
     *
     * @param[in]  aMessage          A reference to the message.
     * @param[in]  aChallenge        A pointer to the Challenge value.
     * @param[in]  aChallengeLength  The length of the Challenge value in bytes.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Challenge TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Challenge TLV.
     *
     */
    otError AppendChallenge(Message &aMessage, const uint8_t *aChallenge, uint8_t aChallengeLength);

    /**
     * This method appends a Response TLV to a message.
     *
     * @param[in]  aMessage         A reference to the message.
     * @param[in]  aResponse        A pointer to the Response value.
     * @param[in]  aResponseLength  The length of the Response value in bytes.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Response TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Response TLV.
     *
     */
    otError AppendResponse(Message &aMessage, const uint8_t *aResponse, uint8_t aResponseLength);

    /**
     * This method appends a Link Frame Counter TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Link Frame Counter TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Link Frame Counter TLV.
     *
     */
    otError AppendLinkFrameCounter(Message &aMessage);

    /**
     * This method appends an MLE Frame Counter TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Frame Counter TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the MLE Frame Counter TLV.
     *
     */
    otError AppendMleFrameCounter(Message &aMessage);

    /**
     * This method appends an Address16 TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     * @param[in]  aRloc16   The RLOC16 value.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Address16 TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Address16 TLV.
     *
     */
    otError AppendAddress16(Message &aMessage, uint16_t aRloc16);

    /**
     * This method appends a Network Data TLV to the message.
     *
     * @param[in]  aMessage     A reference to the message.
     * @param[in]  aStableOnly  TRUE to append stable data, FALSE otherwise.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Network Data TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Network Data TLV.
     *
     */
    otError AppendNetworkData(Message &aMessage, bool aStableOnly);

    /**
     * This method appends a TLV Request TLV to a message.
     *
     * @param[in]  aMessage     A reference to the message.
     * @param[in]  aTlvs        A pointer to the list of TLV types.
     * @param[in]  aTlvsLength  The number of TLV types in @p aTlvs
     *
     * @retval OT_ERROR_NONE     Successfully appended the TLV Request TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the TLV Request TLV.
     *
     */
    otError AppendTlvRequest(Message &aMessage, const uint8_t *aTlvs, uint8_t aTlvsLength);

    /**
     * This method appends a Leader Data TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Leader Data TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Leader Data TLV.
     *
     */
    otError AppendLeaderData(Message &aMessage);

    /**
     * This method appends a Scan Mask TLV to a message.
     *
     * @param[in]  aMessage   A reference to the message.
     * @param[in]  aScanMask  The Scan Mask value.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Scan Mask TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Scan Mask TLV.
     *
     */
    otError AppendScanMask(Message &aMessage, uint8_t aScanMask);

    /**
     * This method appends a Status TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     * @param[in]  aStatus   The Status value.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Status TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Status TLV.
     *
     */
    otError AppendStatus(Message &aMessage, StatusTlv::Status aStatus);

    /**
     * This method appends a Link Margin TLV to a message.
     *
     * @param[in]  aMessage     A reference to the message.
     * @param[in]  aLinkMargin  The Link Margin value.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Link Margin TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Link Margin TLV.
     *
     */
    otError AppendLinkMargin(Message &aMessage, uint8_t aLinkMargin);

    /**
     * This method appends a Version TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Version TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Version TLV.
     *
     */
    otError AppendVersion(Message &aMessage);

    /**
     * This method appends an Address Registration TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Address Registration TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Address Registration TLV.
     *
     */
    otError AppendAddressRegistration(Message &aMessage);

    /**
     * This method appends a Active Timestamp TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     * @param[in]  aCouldUseLocal  True to use local Active Timestamp when network Active Timestamp is not available,
     *                             False not.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Active Timestamp TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Active Timestamp TLV.
     *
     */
    otError AppendActiveTimestamp(Message &aMessage, bool aCouldUseLocal);

    /**
     * This method appends a Pending Timestamp TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Pending Timestamp TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Pending Timestamp TLV.
     *
     */
    otError AppendPendingTimestamp(Message &aMessage);

    /**
     * This method appends a Thread Discovery TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Thread Discovery TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Address Registration TLV.
     *
     */
    otError AppendDiscovery(Message &aMessage);

    /**
     * This method checks if the destination is reachable.
     *
     * @param[in]  aMeshSource  The RLOC16 of the source.
     * @param[in]  aMeshDest    The RLOC16 of the destination.
     * @param[in]  aIp6Header   The IPv6 header of the message.
     *
     * @retval OT_ERROR_NONE  The destination is reachable.
     * @retval OT_ERROR_DROP  The destination is not reachable and the message should be dropped.
     *
     */
    otError CheckReachability(uint16_t aMeshSource, uint16_t aMeshDest, Ip6::Header &aIp6Header);

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
    Neighbor *GetNeighbor(const Ip6::Address &aAddress) {
        (void)aAddress; return NULL;
    }

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
     * @param[in]  aDelay        Delay in milliseconds before the Data Request message is sent.
     *
     * @retval OT_ERROR_NONE     Successfully generated an MLE Data Request message.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers to generate the MLE Data Request message.
     *
     */
    otError SendDataRequest(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength,
                            uint16_t aDelay);

    /**
     * This method generates an MLE Child Update Request message.
     *
     * @retval OT_ERROR_NONE     Successfully generated an MLE Child Update Request message.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers to generate the MLE Child Update Request message.
     *
     */
    otError SendChildUpdateRequest(void);

    /**
     * This method generates an MLE Child Update Response message.
     *
     * @param[in]  aTlvs         A pointer to requested TLV types.
     * @param[in]  aNumTlvs      The number of TLV types in @p aTlvs.
     * @param[in]  aChallenge    The Challenge TLV for the response.
     *
     * @retval OT_ERROR_NONE     Successfully generated an MLE Child Update Response message.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers to generate the MLE Child Update Response message.
     *
     */
    otError SendChildUpdateResponse(const uint8_t *aTlvs, uint8_t aNumTlvs, const ChallengeTlv &aChallenge);

    /**
     * This method submits an MLE message to the UDP socket.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aDestination  A reference to the IPv6 address of the destination.
     *
     * @retval OT_ERROR_NONE     Successfully submitted the MLE message.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers to form the rest of the MLE message.
     *
     */
    otError SendMessage(Message &aMessage, const Ip6::Address &aDestination);

    /**
     * This method sets the RLOC16 assigned to the Thread interface.
     *
     * @param[in]  aRloc16  The RLOC16 to set.
     *
     * @retval OT_ERROR_NONE  Successfully set the RLOC16.
     *
     */
    otError SetRloc16(uint16_t aRloc16);

    /**
     * This method sets the Device State to Detached.
     *
     * @retval OT_ERROR_NONE  Successfully set the Device State to Detached.
     *
     */
    otError SetStateDetached(void);

    /**
     * This method sets the Device State to Child.
     *
     * @retval OT_ERROR_NONE  Successfully set the Device State to Child.
     *
     */
    otError SetStateChild(uint16_t aRloc16);

    /**
     * This method sets the Leader's Partition ID, Weighting, and Router ID values.
     *
     * @param[in]  aPartitionId     The Leader's Partition ID value.
     * @param[in]  aWeighting       The Leader's Weighting value.
     * @param[in]  aLeaderRouterId  The Leader's Router ID value.
     *
     */
    void SetLeaderData(uint32_t aPartitionId, uint8_t aWeighting, uint8_t aLeaderRouterId);

    /**
     * This method adds a message to the message queue. The queued message will be transmitted after given delay.
     *
     * @param[in]  aMessage      The message to transmit after given delay.
     * @param[in]  aDestination  The IPv6 address of the recipient of the message.
     * @param[in]  aDelay        The delay in milliseconds before transmission of the message.
     *
     * @retval OT_ERROR_NONE     Successfully queued the message to transmit after the delay.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers to queue the message.
     *
     */
    otError AddDelayedResponse(Message &aMessage, const Ip6::Address &aDestination, uint16_t aDelay);

    ThreadNetif    &mNetif;                  ///< The Thread Network Interface object.

    LeaderDataTlv   mLeaderData;             ///< Last received Leader Data TLV.
    bool            mRetrieveNewNetworkData; ///< Indicating new Network Data is needed if set.

    otDeviceRole    mRole;                   ///< Current Thread role.
    Router          mParent;                 ///< Parent information.
    uint8_t         mDeviceMode;             ///< Device mode setting.

    bool            isAssignLinkQuality;     ///< Indicating an assigned link quality is used on the link
    uint8_t         mAssignLinkQuality;      ///< The assigned link quality value
    uint8_t         mAssignLinkMargin;       ///< The maximum link margin corresponding to mAssignLinkQuality
    Mac::ExtAddress mAddr64;                 ///< A given IEEE 802.15.4 Extended Address

    /**
     * States when searching for a parent.
     *
     */
    enum ParentRequestState
    {
        kParentIdle,                        ///< Not currently searching for a parent.
        kParentSynchronize,                 ///< Looking to synchronize with a parent (after reset).
        kParentRequestStart,                ///< Starting to look for a parent.
        kParentRequestRouter,               ///< Searching for a Router to attach to.
        kParentRequestChild,                ///< Searching for Routers or REEDs to attach to.
        kChildIdRequest,                    ///< Sending a Child ID Request message.
    };
    ParentRequestState mParentRequestState; ///< The parent request state.

    /**
     * States when reattaching network using stored dataset
     *
     */
    enum ReattachState
    {
        kReattachStop       = 0, ///< Reattach process is disabled or finished
        kReattachStart      = 1, ///< Start reattach process
        kReattachActive     = 2, ///< Reattach using stored Active Dataset
        kReattachPending    = 3, ///< Reattach using stored Pending Dataset
    };
    ReattachState mReattachState;

    Timer         mParentRequestTimer;   ///< The timer for driving the Parent Request process.
    Timer         mDelayedResponseTimer; ///< The timer to delay MLE responses.
    uint8_t       mLastPartitionRouterIdSequence;
    uint32_t      mLastPartitionId;

protected:
    uint8_t mParentLeaderCost;

private:
    enum
    {
        kMleMessagePriority = Message::kPriorityHigh,
    };

    void GenerateNonce(const Mac::ExtAddress &aMacAddr, uint32_t aFrameCounter, uint8_t aSecurityLevel,
                       uint8_t *aNonce);

    static void HandleNetifStateChanged(uint32_t aFlags, void *aContext);
    void HandleNetifStateChanged(uint32_t aFlags);
    static void HandleParentRequestTimer(void *aContext);
    void HandleParentRequestTimer(void);
    static void HandleDelayedResponseTimer(void *aContext);
    void HandleDelayedResponseTimer(void);
    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    static void HandleSendChildUpdateRequest(void *aContext);
    void HandleSendChildUpdateRequest(void);

    otError HandleAdvertisement(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleChildIdResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleChildUpdateRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleChildUpdateResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleDataResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleParentResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                 uint32_t aKeySequence);
    otError HandleAnnounce(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleDiscoveryResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleLeaderData(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    otError SendParentRequest(void);
    otError SendChildIdRequest(void);
    void SendOrphanAnnounce(void);

    bool IsBetterParent(uint16_t aRloc16, uint8_t aLinkQuality, ConnectivityTlv &aConnectivityTlv);
    void ResetParentCandidate(void);

    MessageQueue mDelayedResponses;

    struct
    {
        uint8_t mChallenge[ChallengeTlv::kMaxSize];
        uint8_t mChallengeLength;
    } mChildIdRequest;

    struct
    {
        uint8_t mChallenge[ChallengeTlv::kMaxSize];
    } mParentRequest;

    AttachMode                 mParentRequestMode;
    int8_t                     mParentPriority;
    uint8_t                    mParentLinkQuality3;
    uint8_t                    mParentLinkQuality2;
    uint8_t                    mParentLinkQuality1;
    uint8_t                    mChildUpdateAttempts;
    LeaderDataTlv              mParentLeaderData;
    bool                       mParentIsSingleton;

    Router                     mParentCandidate;

    Ip6::UdpSocket             mSocket;
    uint32_t                   mTimeout;

    Tasklet                    mSendChildUpdateRequest;

    DiscoverHandler            mDiscoverHandler;
    void                      *mDiscoverContext;
    bool                       mIsDiscoverInProgress;
    bool                       mEnableEui64Filtering;

    uint8_t                    mAnnounceChannel;
    uint8_t                    mPreviousChannel;
    uint16_t                   mPreviousPanId;

    Ip6::NetifUnicastAddress   mLeaderAloc;

    Ip6::NetifUnicastAddress   mLinkLocal64;
    Ip6::NetifUnicastAddress   mMeshLocal64;
    Ip6::NetifUnicastAddress   mMeshLocal16;
    Ip6::NetifMulticastAddress mLinkLocalAllThreadNodes;
    Ip6::NetifMulticastAddress mRealmLocalAllThreadNodes;

    Ip6::NetifCallback         mNetifCallback;
};

} // namespace Mle

/**
 * @}
 *
 */

} // namespace ot

#endif  // MLE_HPP_
