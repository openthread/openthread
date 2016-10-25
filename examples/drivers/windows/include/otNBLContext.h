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
 * @brief
 *   This file defines the context structure for NBLs send between otLwf and it's miniport.
 */

#ifndef __OT_NBL_CONTEXT_H__
#define __OT_NBL_CONTEXT_H__

#ifndef _NDIS_
#define NET_BUFFER_LIST_INFO(_NBL, _Id)             ((_NBL)->NetBufferListInfo[(_Id)])
#endif

// Flag that indicates the ACK received had the Frame pending flag
#define OT_NBL_FLAG_ACK_FRAME_PENDING   0x01

// Represents the data necessary for the MAC layer to send out the NetBufferList
// Must be saved in: NET_BUFFER_LIST_INFO(NetBufferList, MediaSpecificInformationEx)
typedef struct _OT_NBL_CONTEXT
{
    // Flags
    UCHAR Flags;

    // Channel used to transmit/receive the frame.
    UCHAR Channel;

    // Transmit/receive power in dBm.
    CHAR  Power;

    // Link Quality Indicator for received frames.
    UCHAR Lqi;

} OT_NBL_CONTEXT, *POT_NBL_CONTEXT;

// OT_NBL_CONTEXT must fit in the pointer used for MediaSpecificInformationEx in the NBL
C_ASSERT(sizeof(OT_NBL_CONTEXT) <= sizeof(PVOID));

// Helper to set the OT_NBL_CONTEXT attached to the NetBufferList
__forceinline VOID SetNBLContext(_In_ PNET_BUFFER_LIST NetBufferList, _In_ POT_NBL_CONTEXT Context)
{
    *(POT_NBL_CONTEXT)(&NET_BUFFER_LIST_INFO(NetBufferList, MediaSpecificInformationEx)) = *Context;
}

// Helper to return the OT_NBL_CONTEXT attached to the NetBufferList
__forceinline POT_NBL_CONTEXT GetNBLContext(_In_ PNET_BUFFER_LIST NetBufferList)
{
    return (POT_NBL_CONTEXT)(&NET_BUFFER_LIST_INFO(NetBufferList, MediaSpecificInformationEx));
}

#endif //__OT_NBL_CONTEXT_H__
