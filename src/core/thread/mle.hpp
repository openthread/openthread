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

#include "openthread-core-config.h"

#include "common/encoding.hpp"
#include "common/locator.hpp"
#include "common/timer.hpp"
#include "mac/mac.hpp"
#include "meshcop/joiner_router.hpp"
#include "net/udp6.hpp"
#include "thread/mle_constants.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/topology.hpp"

namespace ot {

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
    kAttachAny           = 0, ///< Attach to any Thread partition.
    kAttachSame1         = 1, ///< Attach to the same Thread partition (attempt 1 when losing connectivity).
    kAttachSame2         = 2, ///< Attach to the same Thread partition (attempt 2 when losing connectivity).
    kAttachBetter        = 3, ///< Attach to a better (i.e. higher weight/partition id) Thread partition.
    kAttachSameDowngrade = 4, ///< Attach to the same Thread partition during downgrade process.
};

/**
 * This enumeration represents the allocation of the ALOC Space
 *
 */
enum AlocAllocation
{
    kAloc16Leader                      = 0xfc00,
    kAloc16DhcpAgentStart              = 0xfc01,
    kAloc16DhcpAgentEnd                = 0xfc0f,
    kAloc16DhcpAgentMask               = 0x000f,
    kAloc16ServiceStart                = 0xfc10,
    kAloc16ServiceEnd                  = 0xfc2f,
    kAloc16CommissionerStart           = 0xfc30,
    kAloc16CommissionerEnd             = 0xfc37,
    kAloc16CommissionerMask            = 0x0007,
    kAloc16NeighborDiscoveryAgentStart = 0xfc40,
    kAloc16NeighborDiscoveryAgentEnd   = 0xfc4e,
};

/**
 * Service IDs
 *
 */
enum ServiceID
{
    kServiceMinId = 0x00, ///< Minimal Service ID.
    kServiceMaxId = 0x0f, ///< Maximal Service ID.
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
    void Init(void)
    {
        mSecuritySuite   = k154Security;
        mSecurityControl = Mac::Frame::kSecEncMic32;
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const
    {
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
    uint8_t GetLength(void) const
    {
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
    uint8_t GetHeaderLength(void) const
    {
        return sizeof(mSecurityControl) + sizeof(mFrameCounter) + sizeof(mKeySource) + sizeof(mKeyIndex);
    }

    /**
     * This method returns a pointer to first byte of the MLE header.
     *
     * @returns A pointer to the first byte of the MLE header.
     *
     */
    const uint8_t *GetBytes(void) const { return reinterpret_cast<const uint8_t *>(&mSecuritySuite); }

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
    bool IsKeyIdMode2(void) const { return (mSecurityControl & Mac::Frame::kKeyIdModeMask) == Mac::Frame::kKeyIdMode2; }

    /**
     * This method sets the Key ID Mode to 2.
     *
     */
    void SetKeyIdMode2(void)
    {
        mSecurityControl = (mSecurityControl & ~Mac::Frame::kKeyIdModeMask) | Mac::Frame::kKeyIdMode2;
    }

    /**
     * This method returns the Key ID value.
     *
     * @returns The Key ID value.
     *
     */
    uint32_t GetKeyId(void) const { return Encoding::BigEndian::HostSwap32(mKeySource); }

    /**
     * This method sets the Key ID value.
     *
     * @param[in]  aKeySequence  The Key ID value.
     *
     */
    void SetKeyId(uint32_t aKeySequence)
    {
        mKeySource = Encoding::BigEndian::HostSwap32(aKeySequence);
        mKeyIndex  = (aKeySequence & 0x7f) + 1;
    }

    /**
     * This method returns the Frame Counter value.
     *
     * @returns The Frame Counter value.
     *
     */
    uint32_t GetFrameCounter(void) const { return Encoding::LittleEndian::HostSwap32(mFrameCounter); }

    /**
     * This method sets the Frame Counter value.
     *
     * @param[in]  aFrameCounter  The Frame Counter value.
     *
     */
    void SetFrameCounter(uint32_t aFrameCounter) { mFrameCounter = Encoding::LittleEndian::HostSwap32(aFrameCounter); }

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

        /**
         * Applicable/Required only when time synchronization service
         * (`OPENTHREAD_CONFIG_ENABLE_TIME_SYNC`) is enabled.
         *
         */
        kCommandTimeSync = 99, ///< Time Synchronization
    };

    /**
     * This method returns the Command Type value.
     *
     * @returns The Command Type value.
     *
     */
    Command GetCommand(void) const
    {
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
    void SetCommand(Command aCommand)
    {
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
    DelayedResponseHeader(void) { memset(this, 0, sizeof(*this)); };

    /**
     * This constructor initializes the object with specific values.
     *
     * @param[in]  aSendTime     Time when the message shall be sent.
     * @param[in]  aDestination  IPv6 address of the message destination.
     *
     */
    DelayedResponseHeader(uint32_t aSendTime, const Ip6::Address &aDestination)
    {
        mSendTime    = aSendTime;
        mDestination = aDestination;
    };

    /**
     * This method appends delayed response header to the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the bytes.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to grow the message.
     *
     */
    otError AppendTo(Message &aMessage) { return aMessage.Append(this, sizeof(*this)); };

    /**
     * This method reads delayed response header from the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @returns The number of bytes read.
     *
     */
    uint16_t ReadFrom(Message &aMessage)
    {
        return aMessage.Read(aMessage.GetLength() - sizeof(*this), sizeof(*this), this);
    };

    /**
     * This method removes delayed response header from the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE  Successfully removed the header.
     *
     */
    static otError RemoveFrom(Message &aMessage)
    {
        return aMessage.SetLength(aMessage.GetLength() - sizeof(DelayedResponseHeader));
    };

    /**
     * This method returns a time when the message shall be sent.
     *
     * @returns  A time when the message shall be sent.
     *
     */
    uint32_t GetSendTime(void) const { return mSendTime; };

    /**
     * This method returns a destination of the delayed message.
     *
     * @returns  A destination of the delayed message.
     *
     */
    const Ip6::Address &GetDestination(void) const { return mDestination; };

    /**
     * This method checks if the message shall be sent before the given time.
     *
     * @param[in]  aTime  A time to compare.
     *
     * @retval TRUE   If the message shall be sent before the given time.
     * @retval FALSE  Otherwise.
     */
    bool IsEarlier(uint32_t aTime) { return (static_cast<int32_t>(aTime - mSendTime) > 0); };

    /**
     * This method checks if the message shall be sent after the given time.
     *
     * @param[in]  aTime  A time to compare.
     *
     * @retval TRUE   If the message shall be sent after the given time.
     * @retval FALSE  Otherwise.
     */
    bool IsLater(uint32_t aTime) { return (static_cast<int32_t>(aTime - mSendTime) < 0); };

private:
    Ip6::Address mDestination; ///< IPv6 address of the message destination.
    uint32_t     mSendTime;    ///< Time when the message shall be sent.
} OT_TOOL_PACKED_END;

/**
 * This class implements MLE functionality required by the Thread EndDevices, Router, and Leader roles.
 *
 */
class Mle : public InstanceLocator
{
public:
    /**
     * This constructor initializes the MLE object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Mle(Instance &aInstance);

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
     * @param[in]  aAnnounceAttach True if attach on the announced thread network with newer active timestamp,
     *                             or False if not.
     *
     * @retval OT_ERROR_NONE     Successfully started the protocol operation.
     * @retval OT_ERROR_ALREADY  The protocol operation was already started.
     *
     */
    otError Start(bool aAnnounceAttach);

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
    otError Discover(const Mac::ChannelMask &aScanChannels,
                     uint16_t                aPanId,
                     bool                    aJoiner,
                     bool                    aEnableEui64Filtering,
                     DiscoverHandler         aCallback,
                     void *                  aContext);

    /**
     * This method indicates whether or not an MLE Thread Discovery is currently in progress.
     *
     * @returns true if an MLE Thread Discovery is in progress, false otherwise.
     *
     */
    bool IsDiscoverInProgress(void) const { return mIsDiscoverInProgress; }

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
    otDeviceRole GetRole(void) const { return mRole; }

    /**
     * This method returns the Device Mode as reported in the Mode TLV.
     *
     * @returns The Device Mode as reported in the Mode TLV.
     *
     */
    uint8_t GetDeviceMode(void) const { return mDeviceMode; }

    /**
     * This method sets the Device Mode as reported in the Mode TLV.
     *
     * @retval OT_ERROR_NONE          Successfully set the Mode TLV.
     * @retval OT_ERROR_INVALID_ARGS  The mode combination specified in @p aMode is invalid.
     *
     */
    otError SetDeviceMode(uint8_t aMode);

    /**
     * This method indicates whether or not the device is rx-on-when-idle.
     *
     * @returns TRUE if rx-on-when-idle, FALSE otherwise.
     *
     */
    bool IsRxOnWhenIdle(void) const { return (mDeviceMode & ModeTlv::kModeRxOnWhenIdle) != 0; }

    /**
     * This method indicates whether or not the device is a Full Thread Device.
     *
     * @returns TRUE if a Full Thread Device, FALSE otherwise.
     *
     */
    bool IsFullThreadDevice(void) const { return (mDeviceMode & ModeTlv::kModeFullThreadDevice) != 0; }

    /**
     * This method indicates whether or not the device uses secure IEEE 802.15.4 Data Request messages.
     *
     * @returns TRUE if using secure IEEE 802.15.4 Data Request messages, FALSE otherwise.
     *
     */
    bool IsSecureDataRequest(void) const { return (mDeviceMode & ModeTlv::kModeSecureDataRequest) != 0; }

    /**
     * This method indicates whether or not the device requests Full Network Data.
     *
     * @returns TRUE if requests Full Network Data, FALSE otherwise.
     *
     */
    bool IsFullNetworkData(void) const { return (mDeviceMode & ModeTlv::kModeFullNetworkData) != 0; }

    /**
     * This method indicates whether or not the device is a Minimal End Device.
     *
     * @returns TRUE if the device is a Minimal End Device, FALSE otherwise.
     *
     */
    bool IsMinimalEndDevice(void) const
    {
        return (mDeviceMode & (ModeTlv::kModeFullThreadDevice | ModeTlv::kModeRxOnWhenIdle)) !=
               (ModeTlv::kModeFullThreadDevice | ModeTlv::kModeRxOnWhenIdle);
    }

    /**
     * This method returns a pointer to the Mesh Local Prefix.
     *
     * @returns A reference to the Mesh Local Prefix.
     *
     */
    const otMeshLocalPrefix &GetMeshLocalPrefix(void) const
    {
        return reinterpret_cast<const otMeshLocalPrefix &>(mMeshLocal16.GetAddress());
    }

    /**
     * This method sets the Mesh Local Prefix.
     *
     * @param[in]  aPrefix  A reference to the Mesh Local Prefix.
     *
     */
    void SetMeshLocalPrefix(const otMeshLocalPrefix &aPrefix);

    /**
     * This method applies the Mesh Local Prefix.
     *
     * @param[in]  aPrefix  A reference to the Mesh Local Prefix.
     *
     */
    void ApplyMeshLocalPrefix(void);

    /**
     * This method returns a reference to the Thread link-local address.
     *
     * The Thread link local address is derived using IEEE802.15.4 Extended Address as Interface Identifier.
     *
     * @returns A reference to the Thread link local address.
     *
     */
    const Ip6::Address &GetLinkLocalAddress(void) const { return mLinkLocal64.GetAddress(); }

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
     * This method returns a reference to the link-local all Thread nodes multicast address.
     *
     * @returns A reference to the link-local all Thread nodes multicast address.
     *
     */
    const Ip6::Address &GetLinkLocalAllThreadNodesAddress(void) const { return mLinkLocalAllThreadNodes.GetAddress(); }

    /**
     * This method returns a reference to the realm-local all Thread nodes multicast address.
     *
     * @returns A reference to the realm-local all Thread nodes multicast address.
     *
     */
    const Ip6::Address &GetRealmLocalAllThreadNodesAddress(void) const
    {
        return mRealmLocalAllThreadNodes.GetAddress();
    }

    /**
     * This method returns a pointer to the parent when operating in End Device mode.
     *
     * @returns A pointer to the parent.
     *
     */
    Router *GetParent(void);

    /**
     * This method returns a pointer to the parent candidate or parent.
     *
     * This method is useful when sending IEEE 802.15.4 Data Request frames while attempting to attach to a new parent.
     *
     * If attempting to attach to a new parent, this method returns the parent candidate.
     * If not attempting to attach, this method returns the parent.
     *
     */
    Router *GetParentCandidate(void);

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
     * @returns The MLE Timeout value in seconds.
     *
     */
    uint32_t GetTimeout(void) const { return mTimeout; }

    /**
     * This method sets the MLE Timeout value.
     *
     * @param[in]  aTimeout  The Timeout value in seconds.
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
    const Ip6::Address &GetMeshLocal16(void) const { return mMeshLocal16.GetAddress(); }

    /**
     * This method returns a reference to the ML-EID assigned to the Thread interface.
     *
     * @returns A reference to the ML-EID assigned to the Thread interface.
     *
     */
    const Ip6::Address &GetMeshLocal64(void) const { return mMeshLocal64.GetAddress(); }

    /**
     * This method returns the Router ID of the Leader.
     *
     * @returns The Router ID of the Leader.
     *
     */
    uint8_t GetLeaderId(void) const { return mLeaderData.GetLeaderRouterId(); }

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
    otError GetLeaderAloc(Ip6::Address &aAddress) const { return GetAlocAddress(aAddress, kAloc16Leader); }

    /**
     * This method computes the Commissioner's ALOC.
     *
     * @param[out]  aAddress        A reference to the Commissioner's ALOC.
     * @param[in]   aSessionId      Commissioner session id.
     *
     * @retval OT_ERROR_NONE      Successfully retrieved the Commissioner's ALOC.
     * @retval OT_ERROR_DETACHED  The Thread interface is not currently attached to a Thread Partition.
     *
     */
    otError GetCommissionerAloc(Ip6::Address &aAddress, uint16_t aSessionId) const
    {
        return GetAlocAddress(aAddress, GetCommissionerAloc16FromId(aSessionId));
    }

#if OPENTHREAD_ENABLE_SERVICE
    /**
     * This method retrieves the Service ALOC for given Service ID.
     *
     * @param[in]   aServiceID Service ID to get ALOC for.
     * @param[out]  aAddress   A reference to the Service ALOC.
     *
     * @retval OT_ERROR_NONE      Successfully retrieved the Service ALOC.
     * @retval OT_ERROR_DETACHED  The Thread interface is not currently attached to a Thread Partition.
     *
     */
    otError GetServiceAloc(uint8_t aServiceId, Ip6::Address &aAddress) const;
#endif

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
     * This method returns whether the two RLOC16 have the same Router ID.
     *
     * @param[in]  aRloc16A  The first RLOC16 value.
     * @param[in]  aRloc16B  The second RLOC16 value.
     *
     * @returns true if the two RLOC16 have the same Router ID, false otherwise.
     *
     */
    static bool RouterIdMatch(uint16_t aRloc16A, uint16_t aRloc16B)
    {
        return ((aRloc16A >> kRouterIdOffset) == (aRloc16B >> kRouterIdOffset));
    }

    /**
     * This method returns the Service ID corresponding to a Service ALOC16.
     *
     * @param[in]  aAloc16  The Servicer ALOC16 value.
     *
     * @returns The Service ID corresponding to given ALOC16.
     *
     */
    static uint8_t GetServiceIdFromAloc(uint16_t aAloc16)
    {
        return static_cast<uint8_t>(aAloc16 - kAloc16ServiceStart);
    }

    /**
     * This method returns the Service Aloc corresponding to a Service ID.
     *
     * @param[in]  aServiceId  The Service ID value.
     *
     * @returns The Service ALOC16 corresponding to given ID.
     *
     */
    static uint16_t GetServiceAlocFromId(uint8_t aServiceId)
    {
        return static_cast<uint16_t>(aServiceId + kAloc16ServiceStart);
    }

    /**
     * This method returns the Commissioner Aloc corresponding to a Commissioner Session ID.
     *
     * @param[in]  aSessionId   The Commissioner Session ID value.
     *
     * @returns The Commissioner ALOC16 corresponding to given ID.
     *
     */
    static uint16_t GetCommissionerAloc16FromId(uint16_t aSessionId)
    {
        return static_cast<uint16_t>((aSessionId & kAloc16CommissionerMask) + kAloc16CommissionerStart);
    }

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

    /**
     * This method returns a reference to the send queue.
     *
     * @returns A reference to the send queue.
     *
     */
    const MessageQueue &GetMessageQueue(void) const { return mDelayedResponses; }

    /**
     * This method frees multicast MLE Data Response from Delayed Message Queue if any.
     *
     */
    void RemoveDelayedDataResponseMessage(void);

    /**
     * This method converts a device role into a human-readable string.
     *
     */
    static const char *RoleToString(otDeviceRole aRole);

    /**
     * This method gets the MLE counters.
     *
     * @returns A reference to the MLE counters.
     *
     */
    const otMleCounters &GetCounters(void) const { return mCounters; }

    /**
     * This method resets the MLE counters.
     *
     */
    void ResetCounters(void) { memset(&mCounters, 0, sizeof(mCounters)); }

    /**
     * This function registers the client callback that is called when processing an MLE Parent Response message.
     *
     * @param[in]  aCallback A pointer to a function that is called to deliver MLE Parent Response data.
     * @param[in]  aContext  A pointer to application-specific context.
     *
     */
    void RegisterParentResponseStatsCallback(otThreadParentResponseCallback aCallback, void *aContext);

protected:
    /**
     * States during attach (when searching for a parent).
     *
     */
    enum AttachState
    {
        kAttachStateIdle,                ///< Not currently searching for a parent.
        kAttachStateSynchronize,         ///< Looking to synchronize with a parent (after reset).
        kAttachStateProcessAnnounce,     ///< Waiting to process a received Announce (to switch channel/pan-id).
        kAttachStateStart,               ///< Starting to look for a parent.
        kAttachStateParentRequestRouter, ///< Searching for a Router to attach to.
        kAttachStateParentRequestReed,   ///< Searching for Routers or REEDs to attach to.
        kAttachStateAnnounce,            ///< Send Announce messages
        kAttachStateChildIdRequest,      ///< Sending a Child ID Request message.
    };

    /**
     * States when reattaching network using stored dataset
     *
     */
    enum ReattachState
    {
        kReattachStop    = 0, ///< Reattach process is disabled or finished
        kReattachStart   = 1, ///< Start reattach process
        kReattachActive  = 2, ///< Reattach using stored Active Dataset
        kReattachPending = 3, ///< Reattach using stored Pending Dataset
    };

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
     * This method sets the device role.
     *
     * @param[in] aRole A device role.
     *
     */
    void SetRole(otDeviceRole aRole);

    /**
     * This method sets the attach state
     *
     * @param[in] aState An attach state
     *
     */
    void SetAttachState(AttachState aState);

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

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    /**
     * This method appends a Time Request TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Time Request TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Time Request TLV.
     *
     */
    otError AppendTimeRequest(Message &aMessage);

    /**
     * This method appends a Time Parameter TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Time Parameter TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Time Parameter TLV.
     *
     */
    otError AppendTimeParameter(Message &aMessage);

    /**
     * This method appends a XTAL Accuracy TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the XTAL Accuracy TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the XTAl Accuracy TLV.
     *
     */
    otError AppendXtalAccuracy(Message &aMessage);
#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC

    /**
     * This method appends a Active Timestamp TLV to a message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the Active Timestamp TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Active Timestamp TLV.
     *
     */
    otError AppendActiveTimestamp(Message &aMessage);

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
    Neighbor *GetNeighbor(const Ip6::Address &aAddress)
    {
        OT_UNUSED_VARIABLE(aAddress);
        return NULL;
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
    otError SendDataRequest(const Ip6::Address &aDestination,
                            const uint8_t *     aTlvs,
                            uint8_t             aTlvsLength,
                            uint16_t            aDelay);

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

    /**
     * This method prints an MLE log message with an IPv6 address.
     *
     * @param[in]  aLogMessage  The log message string.
     * @param[in]  aAddress     The IPv6 address of the peer.
     *
     */
    void LogMleMessage(const char *aLogMessage, const Ip6::Address &aAddress) const;

    /**
     * This method prints an MLE log message with an IPv6 address and RLOC16.
     *
     * @param[in]  aLogMessage  The log message string.
     * @param[in]  aAddress     The IPv6 address of the peer.
     * @param[in]  aRloc        The RLOC16.
     *
     */
    void LogMleMessage(const char *aLogMessage, const Ip6::Address &aAddress, uint16_t aRloc) const;

    /**
     * This method triggers MLE Announce on previous channel after the Thread device successfully
     * attaches and receives the new Active Commissioning Dataset if needed.
     *
     * MTD would send Announce immediately after attached.
     * FTD would delay to send Announce after tried to become Router or decided to stay in REED role.
     *
     */
    void InformPreviousChannel(void);

    /**
     * This method indicates whether or not in announce attach process.
     *
     * @retval true if attaching/attached on the announced parameters, false otherwise.
     *
     */
    bool IsAnnounceAttach(void) const { return mAlternatePanId != Mac::kPanIdBroadcast; }

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_MLE == 1)
    /**
     * This method converts an `AttachMode` enumeration value into a human-readable string.
     *
     * @param[in] aMode An attach mode
     *
     * @returns A human-readable string corresponding to the attach mode.
     *
     */
    static const char *AttachModeToString(AttachMode aMode);

    /**
     * This method converts an `AttachState` enumeration value into a human-readable string.
     *
     * @param[in] aState An attach state
     *
     * @returns A human-readable string corresponding to the attach state.
     *
     */
    static const char *AttachStateToString(AttachState aState);

    /**
     * This method converts a `ReattachState` enumeration value into a human-readable string.
     *
     * @param[in] aState A reattach state
     *
     * @returns A human-readable string corresponding to the reattach state.
     *
     */
    static const char *ReattachStateToString(ReattachState aState);
#endif

    LeaderDataTlv mLeaderData;                    ///< Last received Leader Data TLV.
    bool          mRetrieveNewNetworkData;        ///< Indicating new Network Data is needed if set.
    otDeviceRole  mRole;                          ///< Current Thread role.
    Router        mParent;                        ///< Parent information.
    uint8_t       mDeviceMode;                    ///< Device mode setting.
    AttachState   mAttachState;                   ///< The parent request state.
    ReattachState mReattachState;                 ///< Reattach state
    uint16_t      mAttachCounter;                 ///< Attach attempt counter.
    uint16_t      mAnnounceDelay;                 ///< Delay in between sending Announce messages during attach.
    TimerMilli    mAttachTimer;                   ///< The timer for driving the attach process.
    TimerMilli    mDelayedResponseTimer;          ///< The timer to delay MLE responses.
    TimerMilli    mMessageTransmissionTimer;      ///< The timer for (re-)sending of MLE messages (e.g. Child Update).
    uint32_t      mLastPartitionId;               ///< The partition ID of the previous Thread partition
    uint8_t       mLastPartitionRouterIdSequence; ///< The router ID sequence from the previous Thread partition
    uint8_t       mLastPartitionIdTimeout;        ///< The time remaining to avoid the previous Thread partition
    uint8_t       mParentLeaderCost;

private:
    enum
    {
        kMleMessagePriority = Message::kPriorityNet,
        kMleHopLimit        = 255,

        // Parameters related to "periodic parent search" feature (CONFIG_ENABLE_PERIODIC_PARENT_SEARCH).
        // All timer intervals are converted to milliseconds.
        kParentSearchCheckInterval   = (OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL * 1000u),
        kParentSearchBackoffInterval = (OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL * 1000u),
        kParentSearchJitterInterval  = (15 * 1000u),
        kParentSearchRssThreadhold   = OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD,

        // Parameters for "attach backoff" feature (CONFIG_ENABLE_ATTACH_BACKOFF) - Intervals are in milliseconds.
        kAttachBackoffMinInterval = OPENTHREAD_CONFIG_ATTACH_BACKOFF_MINIMUM_INTERVAL,
        kAttachBackoffMaxInterval = OPENTHREAD_CONFIG_ATTACH_BACKOFF_MAXIMUM_INTERVAL,
        kAttachBackoffJitter      = OPENTHREAD_CONFIG_ATTACH_BACKOFF_JITTER_INTERVAL,
    };

    enum ParentRequestType
    {
        kParentRequestTypeRouters,         ///< Parent Request to all routers.
        kParentRequestTypeRoutersAndReeds, ///< Parent Request to all routers and REEDs.
    };

    enum ChildUpdateRequestState
    {
        kChildUpdateRequestNone,    ///< No pending or active Child Update Request.
        kChildUpdateRequestPending, ///< Pending Child Update Request due to relative OT_CHANGED event.
        kChildUpdateRequestActive,  ///< Child Update Request has been sent and Child Update Response is expected.
    };

    enum DataRequestState
    {
        kDataRequestNone,   ///< Not waiting for a Data Response.
        kDataRequestActive, ///< Data Request has been sent, Data Response is expected.
    };

    void GenerateNonce(const Mac::ExtAddress &aMacAddr,
                       uint32_t               aFrameCounter,
                       uint8_t                aSecurityLevel,
                       uint8_t *              aNonce);

    static void HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags);
    void        HandleStateChanged(otChangedFlags aFlags);
    static void HandleAttachTimer(Timer &aTimer);
    void        HandleAttachTimer(void);
    static void HandleDelayedResponseTimer(Timer &aTimer);
    void        HandleDelayedResponseTimer(void);
    static void HandleMessageTransmissionTimer(Timer &aTimer);
    void        HandleMessageTransmissionTimer(void);
    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void        ScheduleMessageTransmissionTimer(void);

    otError HandleAdvertisement(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleChildIdResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleChildUpdateRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleChildUpdateResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleDataResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleParentResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, uint32_t aKeySequence);
    otError HandleAnnounce(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleDiscoveryResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError HandleLeaderData(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void    ProcessAnnounce(void);

    uint32_t GetAttachStartDelay(void) const;
    otError  SendParentRequest(ParentRequestType aType);
    otError  SendChildIdRequest(void);
    otError  SendOrphanAnnounce(void);
    bool     PrepareAnnounceState(void);
    otError  SendAnnounce(uint8_t aChannel, bool aOrphanAnnounce, const Ip6::Address &aDestination);
    uint32_t Reattach(void);

    bool IsBetterParent(uint16_t aRloc16, uint8_t aLinkQuality, uint8_t aLinkMargin, ConnectivityTlv &aConnectivityTlv);
    void ResetParentCandidate(void);

    otError GetAlocAddress(Ip6::Address &aAddress, uint16_t aAloc16) const;
#if OPENTHREAD_ENABLE_SERVICE
    /**
     * This method scans for network data from the leader and updates IP addresses assigned to this
     * interface to make sure that all Service ALOCs (0xfc10-0xfc1f) are properly set.
     */
    void UpdateServiceAlocs(void);
#endif

#if OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH
    otError InformPreviousParent(void);
#endif

#if OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH
    static void HandleParentSearchTimer(Timer &aTimer);
    void        HandleParentSearchTimer(void);
    void        StartParentSearchTimer(void);
    void        UpdateParentSearchState(void);
#endif

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

    AttachMode mParentRequestMode;
    int8_t     mParentPriority;
    uint8_t    mParentLinkQuality3;
    uint8_t    mParentLinkQuality2;
    uint8_t    mParentLinkQuality1;

    uint8_t                 mChildUpdateAttempts;
    ChildUpdateRequestState mChildUpdateRequestState;
    uint8_t                 mDataRequestAttempts;
    DataRequestState        mDataRequestState;

    uint8_t       mParentLinkMargin;
    bool          mParentIsSingleton;
    bool          mReceivedResponseFromParent;
    LeaderDataTlv mParentLeaderData;

    Router mParentCandidate;

    Ip6::UdpSocket mSocket;
    uint32_t       mTimeout;

    DiscoverHandler mDiscoverHandler;
    void *          mDiscoverContext;
    bool            mIsDiscoverInProgress;
    bool            mEnableEui64Filtering;

#if OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH
    uint16_t mPreviousParentRloc;
#endif

#if OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH
    bool       mParentSearchIsInBackoff : 1;
    bool       mParentSearchBackoffWasCanceled : 1;
    bool       mParentSearchRecentlyDetached : 1;
    uint32_t   mParentSearchBackoffCancelTime;
    TimerMilli mParentSearchTimer;
#endif

    uint8_t  mAnnounceChannel;
    uint8_t  mAlternateChannel;
    uint16_t mAlternatePanId;
    uint64_t mAlternateTimestamp;

    Ip6::NetifUnicastAddress mLeaderAloc;

#if OPENTHREAD_ENABLE_SERVICE
    Ip6::NetifUnicastAddress mServiceAlocs[OPENTHREAD_CONFIG_MAX_SERVER_ALOCS];
#endif

    otMleCounters mCounters;

    Ip6::NetifUnicastAddress   mLinkLocal64;
    Ip6::NetifUnicastAddress   mMeshLocal64;
    Ip6::NetifUnicastAddress   mMeshLocal16;
    Ip6::NetifMulticastAddress mLinkLocalAllThreadNodes;
    Ip6::NetifMulticastAddress mRealmLocalAllThreadNodes;

    Notifier::Callback mNotifierCallback;

    otThreadParentResponseCallback mParentResponseCb;
    void *                         mParentResponseCbContext;
};

} // namespace Mle

/**
 * @}
 *
 */

} // namespace ot

#endif // MLE_HPP_
