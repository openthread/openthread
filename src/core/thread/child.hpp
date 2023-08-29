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

#include "thread/neighbor.hpp"

namespace ot {

#if OPENTHREAD_FTD

/**
 * Represents a Thread Child.
 *
 */
class Child : public Neighbor,
              public IndirectSender::ChildInfo,
              public DataPollHandler::ChildInfo
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    ,
              public CslTxScheduler::ChildInfo
#endif
{
    class AddressIteratorBuilder;

public:
    static constexpr uint8_t kMaxRequestTlvs = 6;

    /**
     * Represents diagnostic information for a Thread Child.
     *
     */
    class Info : public otChildInfo, public Clearable<Info>
    {
    public:
        /**
         * Sets the `Info` instance from a given `Child`.
         *
         * @param[in] aChild   A neighbor.
         *
         */
        void SetFrom(const Child &aChild);
    };

    /**
     * Defines an iterator used to go through IPv6 address entries of a child.
     *
     */
    class AddressIterator : public Unequatable<AddressIterator>
    {
        friend class AddressIteratorBuilder;

    public:
        /**
         * Represents an index indicating the current IPv6 address entry to which the iterator is pointing.
         *
         */
        typedef otChildIp6AddressIterator Index;

        /**
         * Initializes the iterator associated with a given `Child` starting from beginning of the
         * IPv6 address list.
         *
         * @param[in] aChild    A reference to a child entry.
         * @param[in] aFilter   An IPv6 address type filter restricting iterator to certain type of addresses.
         *
         */
        explicit AddressIterator(const Child &aChild, Ip6::Address::TypeFilter aFilter = Ip6::Address::kTypeAny)
            : AddressIterator(aChild, 0, aFilter)
        {
        }

        /**
         * Initializes the iterator associated with a given `Child` starting from a given index
         *
         * @param[in]  aChild   A reference to the child entry.
         * @param[in]  aIndex   An index (`Index`) with which to initialize the iterator.
         * @param[in]  aFilter  An IPv6 address type filter restricting iterator to certain type of addresses.
         *
         */
        AddressIterator(const Child &aChild, Index aIndex, Ip6::Address::TypeFilter aFilter = Ip6::Address::kTypeAny)
            : mChild(aChild)
            , mFilter(aFilter)
            , mIndex(aIndex)
        {
            Update();
        }

        /**
         * Converts the iterator into an index.
         *
         * @returns An index corresponding to the iterator.
         *
         */
        Index GetAsIndex(void) const { return mIndex; }

        /**
         * Gets the iterator's associated `Child` entry.
         *
         * @returns The associated child entry.
         *
         */
        const Child &GetChild(void) const { return mChild; }

        /**
         * Gets the current `Child` IPv6 Address to which the iterator is pointing.
         *
         * @returns  A pointer to the associated IPv6 Address, or `nullptr` if iterator is done.
         *
         */
        const Ip6::Address *GetAddress(void) const;

        /**
         * Indicates whether the iterator has reached end of the list.
         *
         * @retval TRUE   There are no more entries in the list (reached end of the list).
         * @retval FALSE  The current entry is valid.
         *
         */
        bool IsDone(void) const { return (mIndex >= kMaxIndex); }

        /**
         * Overloads `++` operator (pre-increment) to advance the iterator.
         *
         * The iterator is moved to point to the next `Address` entry.  If there are no more `Ip6::Address` entries
         * `IsDone()` returns `true`.
         *
         */
        void operator++(void) { mIndex++, Update(); }

        /**
         * Overloads `++` operator (post-increment) to advance the iterator.
         *
         * The iterator is moved to point to the next `Address` entry.  If there are no more `Ip6::Address` entries
         *  `IsDone()` returns `true`.
         *
         */
        void operator++(int) { mIndex++, Update(); }

        /**
         * Overloads the `*` dereference operator and gets a reference to `Ip6::Address` to which the
         * iterator is currently pointing.
         *
         * MUST be used when the iterator is not done (i.e., `IsDone()` returns `false`).
         *
         * @returns A reference to the `Ip6::Address` entry currently pointed by the iterator.
         *
         */
        const Ip6::Address &operator*(void) const { return *GetAddress(); }

        /**
         * Overloads operator `==` to evaluate whether or not two `Iterator` instances are equal.
         *
         * MUST be used when the two iterators are associated with the same `Child` entry.
         *
         * @param[in]  aOther  The other `Iterator` to compare with.
         *
         * @retval TRUE   If the two `Iterator` objects are equal.
         * @retval FALSE  If the two `Iterator` objects are not equal.
         *
         */
        bool operator==(const AddressIterator &aOther) const { return (mIndex == aOther.mIndex); }

    private:
        enum IteratorType : uint8_t
        {
            kEndIterator,
        };

        static constexpr uint16_t kMaxIndex = OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD;

        AddressIterator(const Child &aChild, IteratorType)
            : mChild(aChild)
            , mIndex(kMaxIndex)
        {
        }

        void Update(void);

        const Child             &mChild;
        Ip6::Address::TypeFilter mFilter;
        Index                    mIndex;
        Ip6::Address             mMeshLocalAddress;
    };

    /**
     * Initializes the `Child` object.
     *
     * @param[in] aInstance  A reference to OpenThread instance.
     *
     */
    void Init(Instance &aInstance) { Neighbor::Init(aInstance); }

    /**
     * Clears the child entry.
     *
     */
    void Clear(void);

    /**
     * Clears the IPv6 address list for the child.
     *
     */
    void ClearIp6Addresses(void);

    /**
     * Sets the device mode flags.
     *
     * @param[in]  aMode  The device mode flags.
     *
     */
    void SetDeviceMode(Mle::DeviceMode aMode);

    /**
     * Gets the mesh-local IPv6 address.
     *
     * @param[out]   aAddress            A reference to an IPv6 address to provide address (if any).
     *
     * @retval kErrorNone      Successfully found the mesh-local address and updated @p aAddress.
     * @retval kErrorNotFound  No mesh-local IPv6 address in the IPv6 address list.
     *
     */
    Error GetMeshLocalIp6Address(Ip6::Address &aAddress) const;

    /**
     * Returns the Mesh Local Interface Identifier.
     *
     * @returns The Mesh Local Interface Identifier.
     *
     */
    const Ip6::InterfaceIdentifier &GetMeshLocalIid(void) const { return mMeshLocalIid; }

    /**
     * Enables range-based `for` loop iteration over all (or a subset of) IPv6 addresses.
     *
     * Should be used as follows: to iterate over all addresses
     *
     *     for (const Ip6::Address &address : child.IterateIp6Addresses()) { ... }
     *
     * or to iterate over a subset of IPv6 addresses determined by a given address type filter
     *
     *     for (const Ip6::Address &address : child.IterateIp6Addresses(Ip6::Address::kTypeMulticast)) { ... }
     *
     * @param[in] aFilter  An IPv6 address type filter restricting iteration to certain type of addresses (default is
     *                     to accept any address type).
     *
     * @returns An IteratorBuilder instance.
     *
     */
    AddressIteratorBuilder IterateIp6Addresses(Ip6::Address::TypeFilter aFilter = Ip6::Address::kTypeAny) const
    {
        return AddressIteratorBuilder(*this, aFilter);
    }

    /**
     * Adds an IPv6 address to the list.
     *
     * @param[in]  aAddress           A reference to IPv6 address to be added.
     *
     * @retval kErrorNone          Successfully added the new address.
     * @retval kErrorAlready       Address is already in the list.
     * @retval kErrorNoBufs        Already at maximum number of addresses. No entry available to add the new address.
     * @retval kErrorInvalidArgs   Address is invalid (it is the Unspecified Address).
     *
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
     *
     */
    Error RemoveIp6Address(const Ip6::Address &aAddress);

    /**
     * Indicates whether an IPv6 address is in the list of IPv6 addresses of the child.
     *
     * @param[in]  aAddress   A reference to IPv6 address.
     *
     * @retval TRUE           The address exists on the list.
     * @retval FALSE          Address was not found in the list.
     *
     */
    bool HasIp6Address(const Ip6::Address &aAddress) const;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    /**
     * Retrieves the Domain Unicast Address registered by the child.
     *
     * @returns A pointer to Domain Unicast Address registered by the child if there is.
     *
     */
    const Ip6::Address *GetDomainUnicastAddress(void) const;
#endif

    /**
     * Gets the child timeout.
     *
     * @returns The child timeout.
     *
     */
    uint32_t GetTimeout(void) const { return mTimeout; }

    /**
     * Sets the child timeout.
     *
     * @param[in]  aTimeout  The child timeout.
     *
     */
    void SetTimeout(uint32_t aTimeout) { mTimeout = aTimeout; }

    /**
     * Gets the network data version.
     *
     * @returns The network data version.
     *
     */
    uint8_t GetNetworkDataVersion(void) const { return mNetworkDataVersion; }

    /**
     * Sets the network data version.
     *
     * @param[in]  aVersion  The network data version.
     *
     */
    void SetNetworkDataVersion(uint8_t aVersion) { mNetworkDataVersion = aVersion; }

    /**
     * Generates a new challenge value to use during a child attach.
     *
     */
    void GenerateChallenge(void) { mAttachChallenge.GenerateRandom(); }

    /**
     * Gets the current challenge value used during attach.
     *
     * @returns The current challenge value.
     *
     */
    const Mle::TxChallenge &GetChallenge(void) const { return mAttachChallenge; }

    /**
     * Clears the requested TLV list.
     *
     */
    void ClearRequestTlvs(void) { memset(mRequestTlvs, Mle::Tlv::kInvalid, sizeof(mRequestTlvs)); }

    /**
     * Returns the requested TLV at index @p aIndex.
     *
     * @param[in]  aIndex  The index into the requested TLV list.
     *
     * @returns The requested TLV at index @p aIndex.
     *
     */
    uint8_t GetRequestTlv(uint8_t aIndex) const { return mRequestTlvs[aIndex]; }

    /**
     * Sets the requested TLV at index @p aIndex.
     *
     * @param[in]  aIndex  The index into the requested TLV list.
     * @param[in]  aType   The TLV type.
     *
     */
    void SetRequestTlv(uint8_t aIndex, uint8_t aType) { mRequestTlvs[aIndex] = aType; }

    /**
     * Returns the supervision interval (in seconds).
     *
     * @returns The supervision interval (in seconds).
     *
     */
    uint16_t GetSupervisionInterval(void) const { return mSupervisionInterval; }

    /**
     * Sets the supervision interval.
     *
     * @param[in] aInterval  The supervision interval (in seconds).
     *
     */
    void SetSupervisionInterval(uint16_t aInterval) { mSupervisionInterval = aInterval; }

    /**
     * Increments the number of seconds since last supervision of the child.
     *
     */
    void IncrementSecondsSinceLastSupervision(void) { mSecondsSinceSupervision++; }

    /**
     * Returns the number of seconds since last supervision of the child (last message to the child)
     *
     * @returns Number of seconds since last supervision of the child.
     *
     */
    uint16_t GetSecondsSinceLastSupervision(void) const { return mSecondsSinceSupervision; }

    /**
     * Resets the number of seconds since last supervision of the child to zero.
     *
     */
    void ResetSecondsSinceLastSupervision(void) { mSecondsSinceSupervision = 0; }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    /**
     * Returns MLR state of an IPv6 multicast address.
     *
     * @note The @p aAddress reference MUST be from `IterateIp6Addresses()` or `AddressIterator`.
     *
     * @param[in] aAddress  The IPv6 multicast address.
     *
     * @returns MLR state of the IPv6 multicast address.
     *
     */
    MlrState GetAddressMlrState(const Ip6::Address &aAddress) const;

    /**
     * Sets MLR state of an IPv6 multicast address.
     *
     * @note The @p aAddress reference MUST be from `IterateIp6Addresses()` or `AddressIterator`.
     *
     * @param[in] aAddress  The IPv6 multicast address.
     * @param[in] aState    The target MLR state.
     *
     */
    void SetAddressMlrState(const Ip6::Address &aAddress, MlrState aState);

    /**
     * Returns if the Child has IPv6 address @p aAddress of MLR state `kMlrStateRegistered`.
     *
     * @param[in] aAddress  The IPv6 address.
     *
     * @retval true   If the Child has IPv6 address @p aAddress of MLR state `kMlrStateRegistered`.
     * @retval false  If the Child does not have IPv6 address @p aAddress of MLR state `kMlrStateRegistered`.
     *
     */
    bool HasMlrRegisteredAddress(const Ip6::Address &aAddress) const;

    /**
     * Returns if the Child has any IPv6 address of MLR state `kMlrStateRegistered`.
     *
     * @retval true   If the Child has any IPv6 address of MLR state `kMlrStateRegistered`.
     * @retval false  If the Child does not have any IPv6 address of MLR state `kMlrStateRegistered`.
     *
     */
    bool HasAnyMlrRegisteredAddress(void) const { return mMlrRegisteredMask.HasAny(); }

    /**
     * Returns if the Child has any IPv6 address of MLR state `kMlrStateToRegister`.
     *
     * @retval true   If the Child has any IPv6 address of MLR state `kMlrStateToRegister`.
     * @retval false  If the Child does not have any IPv6 address of MLR state `kMlrStateToRegister`.
     *
     */
    bool HasAnyMlrToRegisterAddress(void) const { return mMlrToRegisterMask.HasAny(); }
#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

private:
#if OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD < 2
#error OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD should be at least set to 2.
#endif

    static constexpr uint16_t kNumIp6Addresses = OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD - 1;

    typedef BitVector<kNumIp6Addresses> ChildIp6AddressMask;

    class AddressIteratorBuilder
    {
    public:
        AddressIteratorBuilder(const Child &aChild, Ip6::Address::TypeFilter aFilter)
            : mChild(aChild)
            , mFilter(aFilter)
        {
        }

        AddressIterator begin(void) { return AddressIterator(mChild, mFilter); }
        AddressIterator end(void) { return AddressIterator(mChild, AddressIterator::kEndIterator); }

    private:
        const Child             &mChild;
        Ip6::Address::TypeFilter mFilter;
    };

    Ip6::InterfaceIdentifier mMeshLocalIid;                 ///< IPv6 address IID for mesh-local address
    Ip6::Address             mIp6Address[kNumIp6Addresses]; ///< Registered IPv6 addresses
    uint32_t                 mTimeout;                      ///< Child timeout

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    ChildIp6AddressMask mMlrToRegisterMask;
    ChildIp6AddressMask mMlrRegisteredMask;
#endif

    uint8_t mNetworkDataVersion; ///< Current Network Data version

    union
    {
        uint8_t          mRequestTlvs[kMaxRequestTlvs]; ///< Requested MLE TLVs
        Mle::TxChallenge mAttachChallenge;              ///< The challenge value
    };

    uint16_t mSupervisionInterval;     // Supervision interval for the child (in sec).
    uint16_t mSecondsSinceSupervision; // Number of seconds since last supervision of the child.

    static_assert(OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS < 8192, "mQueuedMessageCount cannot fit max required!");
};

DefineCoreType(otChildInfo, Child::Info);

#endif // OPENTHREAD_FTD

} // namespace ot

#endif // CHILD_HPP_
