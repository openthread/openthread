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
 *   This file includes definitions for Thread child table.
 */

#ifndef SED_CAPABLE_NEIGHBOR_TABLE_HPP_
#define SED_CAPABLE_NEIGHBOR_TABLE_HPP_

#if OPENTHREAD_FTD || OPENTHREAD_MTD

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#if OPENTHREAD_FTD
#include "thread/child_table.hpp"
#endif
#include "thread/topology.hpp"

namespace ot {

/**
 * This class represents the Sed capable neighbor table.
 *
 * TODO: detail
 *
 */
class SedCapableNeighborTable : public InstanceLocator, private NonCopyable
{
#if OPENTHREAD_FTD
    typedef ChildTable::IteratorBuilder IteratorBuilder;
#elif OPENTHREAD_MTD
    class IteratorBuilder;
#endif
public:
#if OPENTHREAD_MTD
    /**
     * This class represents an iterator for iterating through the sed capable neighbor entries in the sed capable
     * neighbor table.
     *
     */
    class Iterator : public InstanceLocator, public ItemPtrIterator<SedCapableNeighbor, Iterator>
    {
        friend class ItemPtrIterator<SedCapableNeighbor, Iterator>;
        friend class IteratorBuilder;

    public:
        /**
         * This constructor initializes an `Iterator` instance.
         *
         * @param[in] aInstance  A reference to the OpenThread instance.
         * @param[in] aFilter    A child state filter.
         *
         */
        Iterator(Instance &aInstance, SedCapableNeighbor::StateFilter aFilter);

        /**
         * This method resets the iterator to start over.
         *
         */
        void Reset(void);

    private:
        explicit Iterator(Instance &aInstance)
            : InstanceLocator(aInstance)
            , mFilter(SedCapableNeighbor::StateFilter::kInStateValid)
        {
        }

        void Advance(void);

        SedCapableNeighbor::StateFilter mFilter;
    };
#endif // OPENTHREAD_MTD

    /**
     * This constructor initializes a `SedCapableTable` instance.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit SedCapableNeighborTable(Instance &aInstance);

    /**
     * This method enables range-based `for` loop iteration over all sed capable neighbor entries in the sed capable
     * neighbor table matching a given state filter.
     *
     * This method should be used as follows:
     *
     *     for (SedCapableNeighbor &neighbor : aSedCapableNeighborTable.Iterate(aFilter)) { ... }
     *
     * @param[in] aFilter  A child state filter.
     *
     * @returns An IteratorBuilder instance.
     *
     */
    IteratorBuilder Iterate(SedCapableNeighbor::StateFilter aFilter) { return IteratorBuilder(GetInstance(), aFilter); }

    /**
     * This method returns the neighbor table index for a given `SedCapableNeighbor` instance.
     *
     * @param[in]  aSedCapableNeighbor  A reference to a `SedCapableNeighbor`
     *
     * @returns The index corresponding to @p aSedCapableNeighbor.
     *
     */
    uint16_t GetSedCapableNeighborIndex(const SedCapableNeighbor &aSedCapableNeighbor) const;

    /**
     * This method searches the sed capable neighbor table for a `SedCapableNeighbor` with a given RLOC16 also matching
     * a given state filter.
     *
     * @param[in]  aRloc16  A RLOC16 address.
     * @param[in]  aFilter  A neighbor state filter.
     *
     * @returns  A pointer to the `SedCapableNeighbor` entry if one is found, or `nullptr` otherwise.
     *
     */
    SedCapableNeighbor *FindSedCapableNeighbor(uint16_t aRloc16, SedCapableNeighbor::StateFilter aFilter);

    /**
     * This method searches the sed capable neighbor table for a `SedCapableNeighbor` with a given address also matching
     * a given state filter.
     *
     * @param[in]  aMacAddress A reference to a MAC address.
     * @param[in]  aFilter     A neighbor state filter.
     *
     * @returns  A pointer to the `SedCapableNeighbor` entry if one is found, or `nullptr` otherwise.
     *
     */
    SedCapableNeighbor *FindSedCapableNeighbor(const Mac::Address &            aMacAddress,
                                               SedCapableNeighbor::StateFilter aFilter);

    /**
     * This method searches the sed capable neighbor table for a `SedCapableNeighbor` with a given Ip6 address also
     * matching a given state filter.
     *
     * @param[in]  aIp6Address A reference to a Ip6 address.
     * @param[in]  aFilter     A neighbor state filter.
     *
     * @returns  A pointer to the `SedCapableNeighbor` entry if one is found, or `nullptr` otherwise.
     *
     */
    SedCapableNeighbor *FindSedCapableNeighbor(
        const Ip6::Address &            aIp6Address,
        SedCapableNeighbor::StateFilter aFilter = SedCapableNeighbor::kInStateValidOrRestoring);

private:
#if OPENTHREAD_MTD
    enum {
        kMaxRxOffNeighbor = OPENTHREAD_CONFIG_MLE_MAX_CHILDREN,
    };

    class IteratorBuilder : public InstanceLocator
    {
    public:
        IteratorBuilder(Instance &aInstance, SedCapableNeighbor::StateFilter aFilter)
            : InstanceLocator(aInstance)
            , mFilter(aFilter)
        {
        }

        Iterator begin(void) { return Iterator(GetInstance(), mFilter); }
        Iterator end(void) { return Iterator(GetInstance()); }

    private:
        SedCapableNeighbor::StateFilter mFilter;
    };

    SedCapableNeighbor mNeighbors[kMaxRxOffNeighbor];

    SedCapableNeighbor *FindSedCapableNeighbor(const SedCapableNeighbor::AddressMatcher &aMatcher)
    {
        return const_cast<SedCapableNeighbor *>(
            const_cast<const SedCapableNeighborTable *>(this)->FindSedCapableNeighbor(aMatcher));
    }

    const SedCapableNeighbor *FindSedCapableNeighbor(const SedCapableNeighbor::AddressMatcher &aMatcher) const;
#endif // OPENTHREAD_MTD
};

} // namespace ot

#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

#endif // SED_CAPABLE_NEIGHBOR_TABLE_HPP_
