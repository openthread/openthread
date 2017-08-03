/*
 *    Copyright (c) 2016-17, The OpenThread Authors.
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
 *   This file contains list of NCP Spinel properties supporting "insert" operation.
 */

/**
 * This file is used/included for two purposes:
 *
 *    1) From `NcpBase` class definition to declare the property handler methods.
 *    2) From the definition of `mInsertPropertyHandlerTable` to match the properties with their handler methods.
 *
 * The following macro is used by this file:
 *
 *    NCP_INSERT_PROP(name)
 *
 * `name` is the name of spinel property (excluding the `SPINEL_PROP_` prefix).

 * NOTE: At the end of this file the above macro is `#undef`ed.
 *
 */

#ifndef NCP_INSERT_PROP
#error Undefined `NCP_INSERT_PROP` macro.
#endif

// Properties supporting "insert" operation

    NCP_INSERT_PROP(UNSOL_UPDATE_FILTER)
#if OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_INSERT_PROP(MAC_SRC_MATCH_SHORT_ADDRESSES)
    NCP_INSERT_PROP(MAC_SRC_MATCH_EXTENDED_ADDRESSES)
#endif
    NCP_INSERT_PROP(IPV6_ADDRESS_TABLE)
    NCP_INSERT_PROP(IPV6_MULTICAST_ADDRESS_TABLE)
    NCP_INSERT_PROP(THREAD_ASSISTING_PORTS)
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_INSERT_PROP(THREAD_OFF_MESH_ROUTES)
    NCP_INSERT_PROP(THREAD_ON_MESH_NETS)
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    NCP_INSERT_PROP(THREAD_JOINERS)
#endif
#if OPENTHREAD_ENABLE_MAC_FILTER
    NCP_INSERT_PROP(MAC_WHITELIST)
    NCP_INSERT_PROP(MAC_BLACKLIST)
    NCP_INSERT_PROP(MAC_FIXED_RSS)
#endif

#undef NCP_INSERT_PROP
