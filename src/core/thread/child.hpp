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
 *   This file includes definitions for a Thread `Child`.
 */

#ifndef CHILD_HPP_
#define CHILD_HPP_

#include "openthread-core-config.h"

#include "common/bit_set.hpp"
#include "thread/neighbor.hpp"

namespace ot {

#if OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD < 2
#error OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD should be at least set to 2.
#endif

/**
 * Represents a Thread Child.
 */
class Child : public Neighbor,
              public IndirectSender::ChildInfo,
              public DataPollHandler::ChildInfo
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    ,
              public CslTxScheduler::ChildInfo
#endif
{
public:
    static constexpr uint8_t kMaxRequestTlvs = 6;

    /**
     * Maximum number of registered IPv6 addresses per child (excluding the mesh-local EID).
     */
    static constexpr uint16_t kNumIp6Addresses = OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD - 1;

    /**
     * Represents the iterator for registered IPv6 address list of an MTD child.
     */
    typedef otChildIp6AddressIterator AddressIterator;

    /**
     * The initial value for an `AddressIterator`.
     */
    static constexpr AddressIterator kAddressIteratorInit = OT_CHILD_IP6_ADDRESS_ITERATOR_INIT;

    /**
     * Represents diagnostic information for a Thread Child.
     */
    class Info : public otChildInfo, public Clearable<Info>
    {
    public:
        /**
         * Sets the `Info` instance from a given `Child`.
         *
         * @param[in] aChild   A neighbor.
         */
        void SetFrom(const Child &aChild);
    };

    /**
     * Represents an IPv6 address entry registered by an MTD child.
     */
    class Ip6AddrEntry : public Ip6::Address
    {
    public:
        /**
         * Indicates whether the entry matches a given IPv6 address.
         *
         * @param[in] aAddress   The IPv6 address.
         *
         * @retval TRUE   The entry matches @p aAddress.
         * @retval FALSE  The entry does not match @p aAddress.
         */
        bool Matches(const Ip6::Address &aAddress) const { return (*this == aAddress); }

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
        /**
         * Gets the MLR state of the IPv6 address entry.
         *
         * @param[in] aChild  The child owning this address entry.
         *
         * @returns The MLR state of IPv6 address entry.
         */
        MlrState GetMlrState(const Child &aChild) const;

        /**
         * Sets the MLR state of the IPv6 address entry.
         *
         * @param[in] aState    The MLR state.
         * @param[in] aChild    The child owning this address entry.
         */
        void SetMlrState(MlrState aState, Child &aChild);
#endif
    };

    /**
     * Represents an array of IPv6 address entries registered by an MTD child.
     *
     * This array does not include the mesh-local EID.
     */
    typedef Array<Ip6AddrEntry, kNumIp6Addresses> Ip6AddressArray;

    /**
     * Initializes the `Child` object.
     *
     * @param[in] aInstance  A reference to OpenThread instance.
     */
    void Init(Instance &aInstance) { Neighbor::Init(aInstance); }

    /**
     * Clears the child entry.
     */
    void Clear(void);

    /**
     * Clears the IPv6 address list for the child.
     */
    void ClearIp6Addresses(void);

    /**
     * Sets the device mode flags.
     *
     * @param[in]  aMode  The device mode flags.
     */
    void SetDeviceMode(Mle::DeviceMode aMode);

    /**
     * Gets the mesh-local IPv6 address.
     *
     * @param[out]   aAddress            A reference to an IPv6 address to provide address (if any).
     *
     * @retval kErrorNone      Successfully found the mesh-local address and updated @p aAddress.
     * @retval kErrorNotFound  No mesh-local IPv6 address in the IPv6 address list.
     */
    Error GetMeshLocalIp6Address(Ip6::Address &aAddress) const;

    /**
     * Returns the Mesh Local Interface Identifier.
     *
     * @returns The Mesh Local Interface Identifier.
     */
    const Ip6::InterfaceIdentifier &GetMeshLocalIid(void) const { return mMeshLocalIid; }

    /**
     * Gets an array of registered IPv6 address entries by the child.
     *
     * The array does not include the mesh-local EID. The ML-EID can retrieved using  `GetMeshLocalIp6Address()`.
     *
     * @returns The array of registered IPv6 addresses by the child.
     */
    const Ip6AddressArray &GetIp6Addresses(void) const { return mIp6Addresses; }

    /**
     * Gets an array of registered IPv6 address entries by the child.
     *
     * The array does not include the mesh-local EID. The ML-EID can retrieved using  `GetMeshLocalIp6Address()`.
     *
     * @returns The array of registered IPv6 addresses by the child.
     */
    Ip6AddressArray &GetIp6Addresses(void) { return mIp6Addresses; }

    /**
     * Iterates over all registered IPv6 addresses (using an iterator).
     *
     * @param[in,out]  aIterator  The iterator to use. On success the iterator will be updated.
     *                            To get the first IPv6 address the iterator should be set to `kAddressIteratorInit`
     * @param[out]     aAddress   A reference to an IPv6 address to return the address.
     *
     * @retval kErrorNone        Successfully got the next IPv6 address. @p aIterator and @p aAddress are updated.
     * @retval kErrorNotFound    No more address.
     */
    Error GetNextIp6Address(AddressIterator &aIterator, Ip6::Address &aAddress) const;

    /**
     * Adds an IPv6 address to the list.
     *
     * @param[in]  aAddress           A reference to IPv6 address to be added.
     *
     * @retval kErrorNone          Successfully added the new address.
     * @retval kErrorAlready       Address is already in the list.
     * @retval kErrorNoBufs        Already at maximum number of addresses. No entry available to add the new address.
     * @retval kErrorInvalidArgs   Address is invalid (it is the Unspecified Address).
     */
    Error AddIp6Address(const Ip6::Address &aAddress);

    /**
     * Removes an IPv6 address from the list.
     *
     * @param[in]  aAddress               A reference to IPv6 address to be removed.
     *
     * @retval kErrorNone             Successfully removed the address.
     * @retval kErrorNotFound         Address was not found in the list.
     * @retval kErrorInvalidArgs      Address is invalid (it is the Unspecified Address).
     */
    Error RemoveIp6Address(const Ip6::Address &aAddress);

    /**
     * Indicates whether an IPv6 address is in the list of IPv6 addresses of the child.
     *
     * @param[in]  aAddress   A reference to IPv6 address.
     *
     * @retval TRUE           The address exists on the list.
     * @retval FALSE          Address was not found in the list.
     */
    bool HasIp6Address(const Ip6::Address &aAddress) const;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    /**
     * Retrieves the Domain Unicast Address registered by the child.
     *
     * @param[out] aAddress    A reference to return the DUA address.
     *
     * @retval kErrorNone      Successfully retrieved the DUA address, @p aAddress is updated.
     * @retval kErrorNotFound  Could not find any DUA address.
     */
    Error GetDomainUnicastAddress(Ip6::Address &aAddress) const;
#endif

    /**
     * Gets the child timeout.
     *
     * @returns The child timeout.
     */
    uint32_t GetTimeout(void) const { return mTimeout; }

    /**
     * Sets the child timeout.
     *
     * @param[in]  aTimeout  The child timeout.
     */
    void SetTimeout(uint32_t aTimeout) { mTimeout = aTimeout; }

    /**
     * Gets the network data version.
     *
     * @returns The network data version.
     */
    uint8_t GetNetworkDataVersion(void) const { return mNetworkDataVersion; }

    /**
     * Sets the network data version.
     *
     * @param[in]  aVersion  The network data version.
     */
    void SetNetworkDataVersion(uint8_t aVersion) { mNetworkDataVersion = aVersion; }

    /**
     * Generates a new challenge value to use during a child attach.
     */
    void GenerateChallenge(void) { mAttachChallenge.GenerateRandom(); }

    /**
     * Gets the current challenge value used during attach.
     *
     * @returns The current challenge value.
     */
    const Mle::TxChallenge &GetChallenge(void) const { return mAttachChallenge; }

    /**
     * Clears the requested TLV list.
     */
    void ClearRequestTlvs(void) { memset(mRequestTlvs, Mle::Tlv::kInvalid, sizeof(mRequestTlvs)); }

    /**
     * Returns the requested TLV at index @p aIndex.
     *
     * @param[in]  aIndex  The index into the requested TLV list.
     *
     * @returns The requested TLV at index @p aIndex.
     */
    uint8_t GetRequestTlv(uint8_t aIndex) const { return mRequestTlvs[aIndex]; }

    /**
     * Sets the requested TLV at index @p aIndex.
     *
     * @param[in]  aIndex  The index into the requested TLV list.
     * @param[in]  aType   The TLV type.
     */
    void SetRequestTlv(uint8_t aIndex, uint8_t aType) { mRequestTlvs[aIndex] = aType; }

    /**
     * Returns the supervision interval (in seconds).
     *
     * @returns The supervision interval (in seconds).
     */
    uint16_t GetSupervisionInterval(void) const { return mSupervisionInterval; }

    /**
     * Sets the supervision interval.
     *
     * @param[in] aInterval  The supervision interval (in seconds).
     */
    void SetSupervisionInterval(uint16_t aInterval) { mSupervisionInterval = aInterval; }

    /**
     * Increments the number of seconds since last supervision of the child.
     */
    void IncrementSecondsSinceLastSupervision(void) { mSecondsSinceSupervision++; }

    /**
     * Returns the number of seconds since last supervision of the child (last message to the child)
     *
     * @returns Number of seconds since last supervision of the child.
     */
    uint16_t GetSecondsSinceLastSupervision(void) const { return mSecondsSinceSupervision; }

    /**
     * Resets the number of seconds since last supervision of the child to zero.
     */
    void ResetSecondsSinceLastSupervision(void) { mSecondsSinceSupervision = 0; }

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    /**
     * Returns if the Child has IPv6 address @p aAddress of MLR state `kMlrStateRegistered`.
     *
     * @param[in] aAddress  The IPv6 address.
     *
     * @retval true   If the Child has IPv6 address @p aAddress of MLR state `kMlrStateRegistered`.
     * @retval false  If the Child does not have IPv6 address @p aAddress of MLR state `kMlrStateRegistered`.
     */
    bool HasMlrRegisteredAddress(const Ip6::Address &aAddress) const;

    /**
     * Returns if the Child has any IPv6 address of MLR state `kMlrStateRegistered`.
     *
     * @retval true   If the Child has any IPv6 address of MLR state `kMlrStateRegistered`.
     * @retval false  If the Child does not have any IPv6 address of MLR state `kMlrStateRegistered`.
     */
    bool HasAnyMlrRegisteredAddress(void) const { return !mMlrRegisteredSet.IsEmpty(); }

    /**
     * Returns if the Child has any IPv6 address of MLR state `kMlrStateToRegister`.
     *
     * @retval true   If the Child has any IPv6 address of MLR state `kMlrStateToRegister`.
     * @retval false  If the Child does not have any IPv6 address of MLR state `kMlrStateToRegister`.
     */
    bool HasAnyMlrToRegisterAddress(void) const { return !mMlrToRegisterSet.IsEmpty(); }
#endif // OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

private:
    typedef BitSet<kNumIp6Addresses> ChildIp6AddressSet;

    uint32_t mTimeout;

    Ip6::InterfaceIdentifier mMeshLocalIid;
    Ip6AddressArray          mIp6Addresses;
#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    ChildIp6AddressSet mMlrToRegisterSet;
    ChildIp6AddressSet mMlrRegisteredSet;
#endif

    uint8_t mNetworkDataVersion;

    union
    {
        uint8_t          mRequestTlvs[kMaxRequestTlvs];
        Mle::TxChallenge mAttachChallenge;
    };

    uint16_t mSupervisionInterval;
    uint16_t mSecondsSinceSupervision;

    static_assert(OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS < 8192, "mQueuedMessageCount cannot fit max required!");
};

DefineCoreType(otChildInfo, Child::Info);

#endif // OPENTHREAD_FTD

} // namespace ot

#endif // CHILD_HPP_
