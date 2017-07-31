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
 *   This file contains list of NCP Spinel commands
 */

/**
 *  This file is included from two places:
 *
 *    1) From the `NcpBase` class definition to to declare the command handler methods.
 *    2) From the defintion of `mCommandHandlerTable` to match the commands with their corresponding handler methods.
 *
 * The following macro should be defined:
 *
 *    NCP_COMMAND(name)
 *
 * `name` should be the name the command (excluding the `SPINEL_CMD_` prefix).
 *
 * NOTE: At the end of this file the above macro is `#undef`ed.
 *
 */

#ifndef NCP_COMMAND
#error Undefined `NCP_COMMAND` macro.
#endif

// List of spinel commands

    NCP_COMMAND(NOOP)
    NCP_COMMAND(RESET)
    NCP_COMMAND(PROP_VALUE_GET)
    NCP_COMMAND(PROP_VALUE_SET)
    NCP_COMMAND(PROP_VALUE_INSERT)
    NCP_COMMAND(PROP_VALUE_REMOVE)
    NCP_COMMAND(NET_SAVE)
    NCP_COMMAND(NET_CLEAR)
    NCP_COMMAND(NET_RECALL)
#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    NCP_COMMAND(PEEK)
    NCP_COMMAND(POKE)
#endif

#undef NCP_COMMAND
