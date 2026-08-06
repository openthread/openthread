/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#ifndef OT_CORE_THREAD_MESH_MONITOR_HPP_
#define OT_CORE_THREAD_MESH_MONITOR_HPP_

#include "config/mesh_monitor.h"

namespace ot {

#if OPENTHREAD_FTD
class Child;
class Router;
#endif

namespace MeshMonitor {

#if OPENTHREAD_CONFIG_MESH_MONITOR_SERVER_ENABLE
/**
 * Implements the Mesh Monitor server functionality for both routers and end devices.
 */
class Server;
#endif // OPENTHREAD_CONFIG_MESH_MONITOR_SERVER_ENABLE

#if OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE
/**
 * Implements Mesh Monitor client functionality.
 */
class Client;
#endif // OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE

} // namespace MeshMonitor

} // namespace ot

#endif // OT_CORE_THREAD_MESH_MONITOR_HPP_
