/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <mac/mac.hpp>
#include <platform/radio.h>

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace Thread {

enum PhyState
{
    kStateDisabled = 0,
    kStateSleep = 1,
    kStateIdle = 2,
    kStateListen = 3,
    kStateReceive = 4,
    kStateTransmit = 5,
    kStateAckWait = 6,
};

struct RadioMessage
{
    uint8_t mChannel;
    uint8_t mPsdu[kMaxPHYPacketSize];
} __attribute__((packed));

static void radioSendAck(void);
static void radioProcessFrame(void);

static PhyState s_state = kStateDisabled;
static RadioMessage s_receive_message;
static RadioMessage s_transmit_message;
static RadioMessage s_ack_message;
static RadioPacket s_receive_frame;
static RadioPacket s_transmit_frame;
static RadioPacket s_ack_frame;
static bool s_data_pending = false;

static uint8_t s_extended_address[OT_EXT_ADDRESS_SIZE];
static uint16_t s_short_address;
static uint16_t s_panid;

static int s_sockfd;

static bool s_promiscuous = false;

ThreadError otPlatRadioSetPanId(uint16_t panid)
{
    s_panid = panid;
    return kThreadError_None;
}

ThreadError otPlatRadioSetExtendedAddress(uint8_t *address)
{
    for (unsigned i = 0; i < sizeof(s_extended_address); i++)
    {
        s_extended_address[i] = address[sizeof(s_extended_address) - 1 - i];
    }

    return kThreadError_None;
}

ThreadError otPlatRadioSetShortAddress(uint16_t address)
{
    s_short_address = address;
    return kThreadError_None;
}

void PlatformRadioInit(void)
{
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;

    if (s_promiscuous)
    {
        sockaddr.sin_port = htons(9000 + WELLKNOWN_NODE_ID);
    }
    else
    {
        sockaddr.sin_port = htons(9000 + NODE_ID);
    }

    sockaddr.sin_addr.s_addr = INADDR_ANY;

    s_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bind(s_sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

    s_receive_frame.mPsdu = s_receive_message.mPsdu;
    s_transmit_frame.mPsdu = s_transmit_message.mPsdu;
    s_ack_frame.mPsdu = s_ack_message.mPsdu;
}

ThreadError otPlatRadioEnable(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(s_state == kStateDisabled, error = kThreadError_Busy);
    s_state = kStateSleep;

exit:
    return error;
}

ThreadError otPlatRadioDisable(void)
{
    s_state = kStateDisabled;
    return kThreadError_None;
}

ThreadError otPlatRadioSleep(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(s_state == kStateIdle, error = kThreadError_Busy);
    s_state = kStateSleep;

exit:
    return error;
}

ThreadError otPlatRadioIdle(void)
{
    ThreadError error = kThreadError_None;

    switch (s_state)
    {
    case kStateSleep:
        s_state = kStateIdle;
        break;

    case kStateIdle:
        break;

    case kStateListen:
    case kStateTransmit:
    case kStateAckWait:
        s_state = kStateIdle;
        break;

    case kStateDisabled:
    case kStateReceive:
        ExitNow(error = kThreadError_Busy);
    }

exit:
    return error;
}

ThreadError otPlatRadioReceive(uint8_t aChannel)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(s_state == kStateIdle, error = kThreadError_Busy);
    s_state = kStateListen;
    s_receive_frame.mChannel = aChannel;

exit:
    return error;
}

RadioPacket *otPlatRadioGetTransmitBuffer(void)
{
    return &s_transmit_frame;
}

ThreadError otPlatRadioTransmit(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(s_state == kStateIdle, error = kThreadError_Busy);
    s_state = kStateTransmit;
    s_data_pending = false;

exit:
    return error;
}

int8_t otPlatRadioGetNoiseFloor(void)
{
    return 0;
}

otRadioCaps otPlatRadioGetCaps(void)
{
    return kRadioCapsNone;
}

void otPlatRadioSetPromiscuous(bool aEnable)
{
    s_promiscuous = aEnable;
}

bool otPlatRadioGetPromiscuous(void)
{
    return s_promiscuous;
}

ThreadError otPlatRadioHandleTransmitDone(bool *rxPending)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(s_state == kStateTransmit || s_state == kStateAckWait, error = kThreadError_InvalidState);

    s_state = kStateIdle;

    if (rxPending != NULL)
    {
        *rxPending = s_data_pending;
    }

exit:
    return error;
}

void radioReceive(void)
{
    uint8_t tx_sequence, rx_sequence;
    uint8_t command_id;
    int rval;

    VerifyOrExit(s_state == kStateDisabled || s_state == kStateSleep || s_state == kStateListen ||
                 s_state == kStateAckWait, ;);

    rval = recvfrom(s_sockfd, &s_receive_message, sizeof(s_receive_message), 0, NULL, NULL);
    assert(rval >= 0);

    switch (s_state)
    {
    case kStateDisabled:
    case kStateSleep:
    case kStateIdle:
    case kStateTransmit:
        break;

    case kStateAckWait:
        s_receive_frame.mLength = rval - 1;

        if (reinterpret_cast<Mac::Frame *>(&s_receive_frame)->GetType() != Mac::Frame::kFcfFrameAck)
        {
            break;
        }

        tx_sequence = reinterpret_cast<Mac::Frame *>(&s_transmit_frame)->GetSequence();
        rx_sequence = reinterpret_cast<Mac::Frame *>(&s_receive_frame)->GetSequence();

        if (tx_sequence != rx_sequence)
        {
            break;
        }

        if (reinterpret_cast<Mac::Frame *>(&s_transmit_frame)->GetType() == Mac::Frame::kFcfFrameMacCmd)
        {
            reinterpret_cast<Mac::Frame *>(&s_transmit_frame)->GetCommandId(command_id);

            if (command_id == Mac::Frame::kMacCmdDataRequest)
            {
                s_data_pending = true;
            }
        }

        s_state = kStateIdle;
        otPlatRadioTransmitDone(s_data_pending, kThreadError_None);
        break;

    case kStateListen:
        if (s_receive_frame.mChannel != s_receive_message.mChannel)
        {
            break;
        }

        s_state = kStateReceive;
        s_receive_frame.mLength = rval - 1;

        radioProcessFrame();
        break;

    case kStateReceive:
        assert(false);
        break;
    }

exit:
    return;
}

int radioTransmit(void)
{
    struct sockaddr_in sockaddr;
    int rval = 0;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr);

    s_transmit_message.mChannel = s_transmit_frame.mChannel;

    for (unsigned i = 1; i < WELLKNOWN_NODE_ID; i++)
    {
        if (NODE_ID == i)
        {
            continue;
        }

        sockaddr.sin_port = htons(9000 + i);
        rval = sendto(s_sockfd, &s_transmit_message, 1 + s_transmit_frame.mLength, 0, (struct sockaddr *)&sockaddr,
                      sizeof(sockaddr));
        assert(rval >= 0);
    }

    sockaddr.sin_port = htons(9000 + WELLKNOWN_NODE_ID);
    rval = sendto(s_sockfd, &s_transmit_message, 1 + s_transmit_frame.mLength, 0, (struct sockaddr *)&sockaddr,
                  sizeof(sockaddr));
    assert(rval >= 0);

    if (reinterpret_cast<Mac::Frame *>(&s_transmit_frame)->GetAckRequest())
    {
        s_state = kStateAckWait;
    }
    else
    {
        s_state = kStateIdle;
        otPlatRadioTransmitDone(false, kThreadError_None);
    }

    return rval;
}

void PlatformRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd)
{
    if (aReadFdSet != NULL &&
        (s_state == kStateDisabled || s_state == kStateSleep || s_state == kStateListen || s_state == kStateAckWait))
    {
        FD_SET(s_sockfd, aReadFdSet);

        if (aMaxFd != NULL && *aMaxFd < s_sockfd)
        {
            *aMaxFd = s_sockfd;
        }
    }

    if (aWriteFdSet != NULL && s_state == kStateTransmit)
    {
        FD_SET(s_sockfd, aWriteFdSet);

        if (aMaxFd != NULL && *aMaxFd < s_sockfd)
        {
            *aMaxFd = s_sockfd;
        }
    }
}

void PlatformRadioProcess(void)
{
    const int flags = POLLRDNORM | POLLERR | POLLNVAL | POLLHUP;
    struct pollfd pollfd = { s_sockfd, flags, 0 };

    if (poll(&pollfd, 1, 0) > 0 && (pollfd.revents & flags) != 0)
    {
        radioReceive();
    }

    if (s_state == kStateTransmit)
    {
        radioTransmit();
    }
}

void radioSendAck(void)
{
    Mac::Frame *ack_frame;
    uint8_t sequence;
    struct sockaddr_in sockaddr;

    sequence = reinterpret_cast<Mac::Frame *>(&s_receive_frame)->GetSequence();

    ack_frame = reinterpret_cast<Mac::Frame *>(&s_ack_frame);
    ack_frame->InitMacHeader(Mac::Frame::kFcfFrameAck, Mac::Frame::kSecNone);
    ack_frame->SetSequence(sequence);

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr);

    s_ack_message.mChannel = s_receive_frame.mChannel;

    for (unsigned i = 1; i < WELLKNOWN_NODE_ID; i++)
    {
        if (NODE_ID == i)
        {
            continue;
        }

        sockaddr.sin_port = htons(9000 + i);
        sendto(s_sockfd, &s_ack_message, 1 + s_ack_frame.mLength, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    }

    sockaddr.sin_port = htons(9000 + WELLKNOWN_NODE_ID);
    sendto(s_sockfd, &s_ack_message, 1 + s_ack_frame.mLength, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
}

void radioProcessFrame(void)
{
    ThreadError error = kThreadError_None;
    Mac::Frame *receive_frame;
    uint16_t dstpan;
    Mac::Address dstaddr;

    VerifyOrExit(s_promiscuous == false, error = kThreadError_None);
    receive_frame = reinterpret_cast<Mac::Frame *>(&s_receive_frame);
    receive_frame->GetDstAddr(dstaddr);

    switch (dstaddr.mLength)
    {
    case 0:
        break;

    case 2:
        receive_frame->GetDstPanId(dstpan);
        VerifyOrExit((dstpan == Mac::kShortAddrBroadcast || dstpan == s_panid) &&
                     (dstaddr.mShortAddress == Mac::kShortAddrBroadcast || dstaddr.mShortAddress == s_short_address),
                     error = kThreadError_Abort);
        break;

    case 8:
        receive_frame->GetDstPanId(dstpan);
        VerifyOrExit((dstpan == Mac::kShortAddrBroadcast || dstpan == s_panid) &&
                     memcmp(&dstaddr.mExtAddress, s_extended_address, sizeof(dstaddr.mExtAddress)) == 0,
                     error = kThreadError_Abort);
        break;

    default:
        ExitNow(error = kThreadError_Abort);
    }

    s_receive_frame.mPower = -20;
    s_receive_frame.mLqi = kPhyNoLqi;

    // generate acknowledgment
    if (reinterpret_cast<Mac::Frame *>(&s_receive_frame)->GetAckRequest())
    {
        radioSendAck();
    }

exit:

    if (s_state != kStateDisabled)
    {
        s_state = kStateIdle;
    }

    if (error != kThreadError_None)
    {
        receive_frame = NULL;
    }

    otPlatRadioReceiveDone(receive_frame, error);
}

#ifdef __cplusplus
}  // end of extern "C"
#endif

}  // namespace Thread
