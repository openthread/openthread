/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file implements the Mesh Diagnostics public APIs.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD

#include <openthread/mesh_diag.h>

#include "common/as_core_type.hpp"
#include "common/locator_getters.hpp"
#include "utils/mesh_diag.hpp"

using namespace ot;

otError otMeshDiagDiscoverTopology(otInstance                     *aInstance,
                                   const otMeshDiagDiscoverConfig *aConfig,
                                   otMeshDiagDiscoverCallback      aCallback,
                                   void                           *aContext)
{
    AssertPointerIsNotNull(aConfig);
    return AsCoreType(aInstance).Get<Utils::MeshDiag>().DiscoverTopology(*aConfig, aCallback, aContext);
}

void otMeshDiagCancel(otInstance *aInstance) { AsCoreType(aInstance).Get<Utils::MeshDiag>().Cancel(); }

otError otMeshDiagGetNextIp6Address(otMeshDiagIp6AddrIterator *aIterator, otIp6Address *aIp6Address)
{
    return AsCoreType(aIterator).GetNextAddress(AsCoreType(aIp6Address));
}

otError otMeshDiagGetNextChildInfo(otMeshDiagChildIterator *aIterator, otMeshDiagChildInfo *aChildInfo)
{
    return AsCoreType(aIterator).GetNextChildInfo(AsCoreType(aChildInfo));
}

otError otMeshDiagQueryChildTable(otInstance                       *aInstance,
                                  uint16_t                          aRloc16,
                                  otMeshDiagQueryChildTableCallback aCallback,
                                  void                             *aContext)
{
    return AsCoreType(aInstance).Get<Utils::MeshDiag>().QueryChildTable(aRloc16, aCallback, aContext);
}

otError otMeshDiagQueryChildrenIp6Addrs(otInstance                     *aInstance,
                                        uint16_t                        aRloc16,
                                        otMeshDiagChildIp6AddrsCallback aCallback,
                                        void                           *aContext)
{
    return AsCoreType(aInstance).Get<Utils::MeshDiag>().QueryChildrenIp6Addrs(aRloc16, aCallback, aContext);
}

otError otMeshDiagQueryRouterNeighborTable(otInstance                                *aInstance,
                                           uint16_t                                   aRloc16,
                                           otMeshDiagQueryRouterNeighborTableCallback aCallback,
                                           void                                      *aContext)
{
    return AsCoreType(aInstance).Get<Utils::MeshDiag>().QueryRouterNeighborTable(aRloc16, aCallback, aContext);
}

#endif // OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD
