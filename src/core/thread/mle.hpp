/*
 *    Copyright (c) 2016, Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
 * This class implements MLE Header generation and parsing.
 *
 */
class Header
{
public:
    /**
     * This method initializes the MLE header.
     *
     */
    void Init(void) { mSecuritySuite = 0; mSecurityControl = Mac::Frame::kSecEncMic32; }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const {
        return mSecuritySuite == 0 &&
               (mSecurityControl == (Mac::Frame::kKeyIdMode1 | Mac::Frame::kSecEncMic32) ||
                mSecurityControl == (Mac::Frame::kKeyIdMode2 | Mac::Frame::kSecEncMic32));
    }

    /**
     * This method returns the MLE header and Command Type length.
     *
     * @returns The MLE header and Command Type length.
     *
     */
    uint8_t GetLength(void) const {
        return sizeof(mSecuritySuite) + sizeof(mSecurityControl) + sizeof(mFrameCounter) +
               (IsKeyIdMode1() ? 1 : 5) + sizeof(mCommand);
    }

    /**
     * This method returns the MLE header length (excluding the Command Type).
     *
     * @returns The MLE header length (excluding the Command Type).
     *
     */
    uint8_t GetHeaderLength(void) const {
        return sizeof(mSecurityControl) + sizeof(mFrameCounter) + (IsKeyIdMode1() ? 1 : 5);
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
     * This method indicates whether or not the Key ID Mode is set to 1.
     *
     * @retval TRUE   If the Key ID Mode is set to 1.
     * @retval FALSE  If the Key ID Mode is not set to 1.
     *
     */
    bool IsKeyIdMode1(void) const {
        return (mSecurityControl & Mac::Frame::kKeyIdModeMask) == Mac::Frame::kKeyIdMode1;
    }

    /**
     * This method sets the Key ID Mode to 1.
     *
     */
    void SetKeyIdMode1(void) {
        mSecurityControl = (mSecurityControl & ~Mac::Frame::kKeyIdModeMask) | Mac::Frame::kKeyIdMode1;
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
        return IsKeyIdMode1() ? mKeyIdentifier[0] - 1 :
               static_cast<uint32_t>(mKeyIdentifier[3]) << 0 |
               static_cast<uint32_t>(mKeyIdentifier[2]) << 8 |
               static_cast<uint32_t>(mKeyIdentifier[1]) << 16 |
               static_cast<uint32_t>(mKeyIdentifier[0]) << 24;
    }

    /**
     * This method sets the Key ID value.
     *
     * @param[in]  aKeySequence  The Key ID value.
     *
     */
    void SetKeyId(uint32_t aKeySequence) {
        if (IsKeyIdMode1()) {
            mKeyIdentifier[0] = (aKeySequence & 0x7f) + 1;
        }
        else {
            mKeyIdentifier[4] = (aKeySequence & 0x7f) + 1;
            mKeyIdentifier[3] = aKeySequence >> 0;
            mKeyIdentifier[2] = aKeySequence >> 8;
            mKeyIdentifier[1] = aKeySequence >> 16;
            mKeyIdentifier[0] = aKeySequence >> 24;
        }
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
    };

    /**
     * This method returns the Command Type value.
     *
     * @returns The Command Type value.
     *
     */
    Command GetCommand(void) const {
        const uint8_t *command = mKeyIdentifier + (IsKeyIdMode1() ? 1 : 5);
        return static_cast<Command>(*command);
    }

    /**
     * This method sets the Command Type value.
     *
     * @param[in]  aCommand  The Command Type value.
     *
     */
    void SetCommand(Command aCommand) {
        uint8_t *commandField = mKeyIdentifier + (IsKeyIdMode1() ? 1 : 5);
        *commandField = static_cast<uint8_t>(aCommand);
    }

    /**
     * Security suite identifiers.
     *
     */
    enum SecuritySuite
    {
        kSecurityEnabled  = 0x00,  ///< IEEE 802.15.4-2006 security
    };

private:
    uint8_t mSecuritySuite;
    uint8_t mSecurityControl;
    uint32_t mFrameCounter;
    uint8_t mKeyIdentifier[5];
    uint8_t mCommand;
} __attribute__((packed));

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
     * This method starts the MLE protocol operation.
     *
     * @retval kThreadError_None  Successfully started the protocol operation.
     * @retval kThreadError_Busy  The protocol operation was already started.
     *
     */
    ThreadError Start(void);

    /**
     * This method stops the MLE protocol operation.
     *
     * @retval kThreadError_None  Successfully stopped the protocol operation.
     * @retval kThreadError_Busy  The protocol operation was already stopped.
     *
     */
    ThreadError Stop(void);

    /**
     * This method causes the Thread interface to detach from the Thread network.
     *
     * @retval kThreadError_None  Successfully detached from the Thread network.
     * @retval kThreadError_Busy  The protocol operation was stopped.
     *
     */
    ThreadError BecomeDetached(void);

    /**
     * This method causes the Thread interface to attempt an MLE attach.
     *
     * @param[in]  aFilter  Indicates what partitions to attach to.
     *
     * @retval kThreadError_None  Successfully began the attach process.
     * @retval kThreadError_Busy  An attach process is in progress or the protocol operation was stopped.
     *
     */
    ThreadError BecomeChild(otMleAttachFilter aFilter);

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
     * This method returns the Child ID portion of an RLOC16.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     * @returns The Child ID portion of an RLOC16.
     *
     */
    const uint8_t GetChildId(uint16_t aRloc16) const;

    /**
     * This method returns the Router ID portion of an RLOC16.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     * @returns The Router ID portion of an RLOC16.
     *
     */
    const uint8_t GetRouterId(uint16_t aRloc16) const;

    /**
     * This method returns the RLOC16 of a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID value..
     *
     * @returns The RLOC16 of the given Router ID.
     *
     */
    const uint16_t GetRloc16(uint8_t aRouterId) const;

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
     * This method returns a pointer to the RLOC assigned to the Thread interface.
     *
     * @returns A pointer to the RLOC assigned to the Thread interface.
     *
     */
    const Ip6::Address *GetMeshLocal16(void) const;

    /**
     * This method returns a pointer to the ML-EID assigned to the Thread interface.
     *
     * @returns A pointer to the ML-EID assigned to the Thread interface.
     *
     */
    const Ip6::Address *GetMeshLocal64(void) const;

    /**
     * This method notifes MLE that the Network Data has changed.
     *
     */
    void HandleNetworkDataUpdate(void);

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
     * This method returns the most recently received Leader Data TLV.
     *
     * @returns  A reference to the most recently receievd Leader Data TLV.
     *
     */
    const LeaderDataTlv &GetLeaderDataTlv(void);

protected:
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
     * This method converts a link margin value to a link quality value.
     *
     * @param[in]  aLinkMargin  The Link Margin in dB.
     *
     * @returns The link quality value.
     *
     */
    uint8_t LinkMarginToQuality(uint8_t aLinkMargin);

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
     * This method generates an MLE Data Response message.
     *
     * @param[in]  aDestination  A reference to the IPv6 address of the destination.
     * @param[in]  aTlvs         A pointer to TLV types that should be included.
     * @param[in]  aTlvsLength   The number of TLV types in @p aTlvs.
     *
     * @retval kThreadError_None    Successfully generated an MLE Data Response message.
     * @retval kThreadError_NoBufs  Insufficient buffers to generate the MLE Data Response message.
     *
     */
    ThreadError SendDataResponse(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength);

    /**
     * This method generates an MLE Child Update Request message.
     *
     * @retval kThreadError_None    Successfully generated an MLE Child Update Request message..
     * @retval kThreadError_NoBufs  Insufficient buffers to generate the MLE Child Update Request message.
     *
     */
    ThreadError SendChildUpdateRequest(void);

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

    ThreadNetif         &mNetif;            ///< The Thread Network Interface object.
    AddressResolver     &mAddressResolver;  ///< The Address Resolver object.
    KeyManager          &mKeyManager;       ///< The Key Manager object.
    Mac::Mac            &mMac;              ///< The MAC object.
    MeshForwarder       &mMesh;             ///< The Mesh Forwarding object.
    MleRouter           &mMleRouter;        ///< The MLE Router object.
    NetworkData::Leader &mNetworkData;      ///< The Network Data object.

    LeaderDataTlv mLeaderData;              ///< Last received Leader Data TLV.

    DeviceState mDeviceState;               ///< Current Thread interface state.
    Router mParent;                         ///< Parent information.
    uint8_t mDeviceMode;                    ///< Device mode setting.

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

    Timer mParentRequestTimer;  ///< The timer for driving the Parent Request process.

private:
    enum
    {
        kAttachDataPollPeriod = OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD,
    };

    void GenerateNonce(const Mac::ExtAddress &aMacAddr, uint32_t aFrameCounter, uint8_t aSecurityLevel,
                       uint8_t *aNonce);

    static void HandleUnicastAddressesChanged(void *aContext);
    void HandleUnicastAddressesChanged(void);
    static void HandleParentRequestTimer(void *aContext);
    void HandleParentRequestTimer(void);
    static void HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo);
    void HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    ThreadError HandleAdvertisement(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleChildIdResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleChildUpdateResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleDataRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleDataResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError HandleParentResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                     uint32_t aKeySequence);

    ThreadError SendParentRequest(void);
    ThreadError SendChildIdRequest(void);

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
    uint32_t mParentConnectivity;

    Ip6::UdpSocket mSocket;
    uint32_t mTimeout;

    Ip6::NetifUnicastAddress mLinkLocal16;
    Ip6::NetifUnicastAddress mLinkLocal64;
    Ip6::NetifUnicastAddress mMeshLocal64;
    Ip6::NetifUnicastAddress mMeshLocal16;
    Ip6::NetifMulticastAddress mLinkLocalAllThreadNodes;
    Ip6::NetifMulticastAddress mRealmLocalAllThreadNodes;

    Ip6::NetifHandler mNetifHandler;
};

}  // namespace Mle

/**
 * @}
 *
 */

}  // namespace Thread

#endif  // MLE_HPP_
