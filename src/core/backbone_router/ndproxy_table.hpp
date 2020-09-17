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
 *   This file includes definitions for NdProxy Table on Thread Backbone Border Router.
 */

#ifndef NDPROXY_TABLE_HPP_
#define NDPROXY_TABLE_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#include <openthread/backbone_router_ftd.h>

#include "backbone_router/bbr_leader.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/time.hpp"
#include "net/ip6_address.hpp"

namespace ot {

namespace BackboneRouter {

/**
 * This class implements NdProxy Table maintenance on Primary Backbone Router.
 *
 */
class NdProxyTable : public InstanceLocator, private NonCopyable
{
public:
    /**
     * This constructor initializes the `NdProxyTable` object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit NdProxyTable(Instance &aInstance)
        : InstanceLocator(aInstance)
    {
    }

    /**
     * This method registers a given Ip6 address IID with related information to the NdProxy table.
     *
     * @param[in] aAddressIid                The Ip6 address IID.
     * @param[in] aMeshLocalIid              The Mesh-Local IID.
     * @param[in] aRloc16                    The RLOC16.
     * @param[in] aTimeSinceLastTransaction  Time since last transaction (in seconds).
     *
     * @retval OT_ERROR_NONE        If registered successfully.
     * @retval OT_ERROR_DUPLICATED  If the Ip6 address IID is a duplicate.
     * @retval OT_ERROR_NO_BUFS     Insufficient buffer space available to register.
     *
     */
    otError Register(const Ip6::InterfaceIdentifier &aAddressIid,
                     const Ip6::InterfaceIdentifier &aMeshLocalIid,
                     uint16_t                        aRloc16,
                     const uint32_t *                aTimeSinceLastTransaction);

    /**
     * This method checks if a given Ip6 address IID was registered.
     *
     * @param[in] aAddressIid  The Ip6 address IID.
     *
     * @retval TRUE   If the Ip6 address IID was registered.
     * @retval FALSE  If the Ip6 address IID was not registered.
     *
     */
    bool IsRegistered(const Ip6::InterfaceIdentifier &aAddressIid) { return FindByAddressIid(aAddressIid) != nullptr; }

    /**
     * This method notifies Domain Prefix status.
     *
     * @param[in]  aState  The Domain Prefix state or state change.
     *
     */
    void HandleDomainPrefixUpdate(Leader::DomainPrefixState aState);

private:
    enum
    {
        kMaxNdProxyNum = OPENTHREAD_CONFIG_NDPROXY_TABLE_ENTRY_NUM,
    };

    enum Filter : uint8_t
    {
        kFilterInvalid,
        kFilterValid,
    };

    class NdProxy : private Clearable<NdProxy>
    {
        friend class NdProxyTable;

    private:
        NdProxy(void) { Clear(); }

        void Init(const Ip6::InterfaceIdentifier &aAddressIid,
                  const Ip6::InterfaceIdentifier &aMeshLocalIid,
                  uint16_t                        aRloc16,
                  uint32_t                        aTimeSinceLastTransaction);

        void Update(uint16_t aRloc16, uint32_t aTimeSinceLastTransaction);

        Ip6::InterfaceIdentifier mAddressIid;
        Ip6::InterfaceIdentifier mMeshLocalIid;
        TimeMilli                mLastRegistrationTime; ///< in milliseconds
        uint16_t                 mRloc16;
        bool                     mValid : 1;
    };

    /**
     * This class represents an iterator for iterating through the NdProxy Table.
     *
     */
    class Iterator : public InstanceLocator
    {
        friend class NdProxyTable;
        friend class IteratorBuilder;

    private:
        enum IteratorType
        {
            kEndIterator,
        };

        Iterator(Instance &aInstance, Filter aFilter);
        Iterator(Instance &aInstance, IteratorType);

        bool           IsDone(void) const { return (mCurrent == nullptr); }
        void           Advance(void);
        void           operator++(void) { Advance(); }
        void           operator++(int) { Advance(); }
        const NdProxy &operator*(void)const { return *mCurrent; }
        bool           operator==(const Iterator &aOther) const { return mCurrent == aOther.mCurrent; }
        bool           operator!=(const Iterator &aOther) const { return !(*this == aOther); }
        NdProxy *      operator->(void) { return mCurrent; }
        NdProxy &      operator*(void) { return *mCurrent; }

        NdProxy *mCurrent;
        Filter   mFilter;
    };

    class IteratorBuilder : public InstanceLocator
    {
        friend class NdProxyTable;

    private:
        IteratorBuilder(Instance &aInstance, Filter aFilter)
            : InstanceLocator(aInstance)
            , mFilter(aFilter)
        {
        }

        Iterator begin(void) { return Iterator(GetInstance(), mFilter); }
        Iterator end(void) { return Iterator(GetInstance(), Iterator::kEndIterator); }

        Filter mFilter;
    };

    IteratorBuilder Iterate(Filter aFilter) { return IteratorBuilder(GetInstance(), aFilter); }
    void            Clear(void);
    static bool     MatchesFilter(const NdProxy &aProxy, Filter aFilter);
    NdProxy *       FindByAddressIid(const Ip6::InterfaceIdentifier &aAddressIid);
    NdProxy *       FindByMeshLocalIid(const Ip6::InterfaceIdentifier &aMeshLocalIid);
    NdProxy *       FindInvalid(void);
    static void     Erase(NdProxy &aProxy);

    NdProxy mProxies[kMaxNdProxyNum];
};

} // namespace BackboneRouter

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#endif // NDPROXY_TABLE_HPP_
