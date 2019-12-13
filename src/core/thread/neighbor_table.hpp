/*
 *  Copyright (c) 2016-2019, The OpenThread Authors.
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
 *   This file includes definitions for Thread neighbor table.
 */

#ifndef NEIGHBOR_TABLE_HPP_
#define NEIGHBOR_TABLE_HPP_

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "net/ip6_address.hpp"
#include "thread/topology.hpp"

namespace ot {

/**
 * This class represents the Thread neighbor table.
 *
 */
class NeighborTable : public InstanceLocator
{
public:
    /**
     * This constructor initializes a `NeighborTable` instance.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit NeighborTable(Instance &aInstance);

    /**
     * This method searches among parent and parent candidate for a `Neighbor` matching a given short address and a
     * state filter.
     *
     * @param[in]  aAddress A short address
     * @param[in]  aFilter  A neighbor state filter.
     *
     * @returns  A pointer to the `Neighbor` entry if one is found, or `NULL` otherwise.
     *
     */
    Neighbor *FindParentNeighbor(Mac::ShortAddress aAddress, Neighbor::StateFilter aFilter);

    /**
     * This method searches among parent and parent candidate for a `Neighbor` matching an extended address and a state
     * filter.
     *
     * @param[in]  aAddress A reference to an extended address.
     * @param[in]  aFilter  A neighbor state filter.
     *
     * @returns  A pointer to the `Neighbor` entry if one is found, or `NULL` otherwise.
     *
     */
    Neighbor *FindParentNeighbor(const Mac::ExtAddress &aAddress, Neighbor::StateFilter aFilter);

    /**
     * This method searches among parent and parent candidate for a `Neighbor` matching a given address and a state
     * filter.
     *
     * @param[in]  aAddress A reference to a MAC address.
     * @param[in]  aFilter  A neighbor state filter.
     *
     * @returns  A pointer to the `Neighbor` entry if one is found, or `NULL` otherwise.
     *
     */
    Neighbor *FindParentNeighbor(const Mac::Address &aAddress, Neighbor::StateFilter aFilter);

    /**
     * This method searches for a `Neighbor` matching a given short address and a state filter.
     *
     * @param[in]  aAddress A short address
     * @param[in]  aFilter  A neighbor state filter.
     *
     * @returns  A pointer to the `Neighbor` entry if one is found, or `NULL` otherwise.
     *
     */
    Neighbor *FindNeighbor(Mac::ShortAddress aAddress, Neighbor::StateFilter aFilter);

    /**
     * This method searches for a `Neighbor` matching an extended address and a state filter.
     *
     * @param[in]  aAddress A reference to an extended address.
     * @param[in]  aFilter  A neighbor state filter.
     *
     * @returns  A pointer to the `Neighbor` entry if one is found, or `NULL` otherwise.
     *
     */
    Neighbor *FindNeighbor(const Mac::ExtAddress &aAddress, Neighbor::StateFilter aFilter);

    /**
     * This method searches for a `Neighbor` matching a given address and a state filter.
     *
     * @param[in]  aAddress A reference to a MAC address.
     * @param[in]  aFilter  A neighbor state filter.
     *
     * @returns  A pointer to the `Neighbor` entry if one is found, or `NULL` otherwise.
     *
     */
    Neighbor *FindNeighbor(const Mac::Address &aAddress, Neighbor::StateFilter aFilter);

#if OPENTHREAD_FTD

    /**
     * This method searches for a `Neighbor` matching a given IPv6 address and a state filter.
     *
     * @param[in]  aAddress A reference to an IPv6 address.
     * @param[in]  aFilter  A neighbor state filter.
     *
     * @returns  A pointer to the `Neighbor` entry if one is found, or `NULL` otherwise.
     *
     */
    Neighbor *FindNeighbor(const Ip6::Address &aAddress, Neighbor::StateFilter aFilter);

    /**
     * This method searches for a `Neighbor` router (where a one-way link is maintained) matching a given address and a
     * state filter.
     *
     * This is used while and FTD device is in  child role to allow it to receive multicast/broadcast frames from
     * other routers.
     *
     * @param[in]  aAddress  The address of the Neighbor.
     * @param[in]  aFilter   A neighbor state filter
     *
     * @returns A pointer to the `Neighbor` entry if one is found, or `NULL` otherwise.
     *
     */
    Neighbor *FindRxOnlyNeighbor(const Mac::Address &aAddress, Neighbor::StateFilter aFilter);

#endif // OPENTHREAD_FTD
};

} // namespace ot

#endif // NEIGHBOR_TABLE_HPP_
