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
#include <pthread.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <platform/posix/cmdline.h>

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <mac/mac.hpp>
#include <platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct gengetopt_args_info args_info;

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
    uint8_t mPsdu[Mac::Frame::kMTU];
} __attribute__((packed));

static void *phy_receive_thread(void *arg);

static PhyState s_state = kStateDisabled;
static RadioPacket *s_receive_frame = NULL;
static RadioPacket *s_transmit_frame = NULL;
static RadioPacket m_ack_packet;
static bool s_data_pending = false;

static uint8_t s_extended_address[8];
static uint16_t s_short_address;
static uint16_t s_panid;

static pthread_t s_pthread;
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_condition_variable = PTHREAD_COND_INITIALIZER;
static int s_sockfd;

ThreadError otPlatRadioSetPanId(uint16_t panid)
{
    s_panid = panid;
    return kThreadError_None;
}

ThreadError otPlatRadioSetExtendedAddress(uint8_t *address)
{
    for (unsigned i = 0; i < sizeof(s_extended_address); i++)
    {
        s_extended_address[i] = address[7 - i];
    }

    return kThreadError_None;
}

ThreadError otPlatRadioSetShortAddress(uint16_t address)
{
    s_short_address = address;
    return kThreadError_None;
}

void hwRadioInit()
{
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(9000 + args_info.nodeid_arg);
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    s_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bind(s_sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

    pthread_create(&s_pthread, NULL, &phy_receive_thread, NULL);
}

ThreadError otPlatRadioEnable()
{
    ThreadError error = kThreadError_None;

    pthread_mutex_lock(&s_mutex);
    VerifyOrExit(s_state == kStateDisabled, error = kThreadError_Busy);
    s_state = kStateSleep;
    pthread_cond_signal(&s_condition_variable);

exit:
    pthread_mutex_unlock(&s_mutex);
    return error;
}

ThreadError otPlatRadioDisable()
{
    pthread_mutex_lock(&s_mutex);
    s_state = kStateDisabled;
    pthread_cond_signal(&s_condition_variable);
    pthread_mutex_unlock(&s_mutex);
    return kThreadError_None;
}

ThreadError otPlatRadioSleep()
{
    ThreadError error = kThreadError_None;

    pthread_mutex_lock(&s_mutex);
    VerifyOrExit(s_state == kStateIdle, error = kThreadError_Busy);
    s_state = kStateSleep;
    pthread_cond_signal(&s_condition_variable);

exit:
    pthread_mutex_unlock(&s_mutex);
    return error;
}

ThreadError otPlatRadioIdle()
{
    ThreadError error = kThreadError_None;

    pthread_mutex_lock(&s_mutex);

    switch (s_state)
    {
    case kStateSleep:
        s_state = kStateIdle;
        pthread_cond_signal(&s_condition_variable);
        break;

    case kStateIdle:
        break;

    case kStateListen:
    case kStateTransmit:
    case kStateAckWait:
        s_state = kStateIdle;
        pthread_cond_signal(&s_condition_variable);
        break;

    case kStateDisabled:
    case kStateReceive:
        ExitNow(error = kThreadError_Busy);
    }

exit:
    pthread_mutex_unlock(&s_mutex);
    return error;
}

ThreadError otPlatRadioReceive(RadioPacket *packet)
{
    ThreadError error = kThreadError_None;

    pthread_mutex_lock(&s_mutex);
    VerifyOrExit(s_state == kStateIdle, error = kThreadError_Busy);
    s_state = kStateListen;
    pthread_cond_signal(&s_condition_variable);

    s_receive_frame = packet;

exit:
    pthread_mutex_unlock(&s_mutex);
    return error;
}

ThreadError otPlatRadioTransmit(RadioPacket *packet)
{
    ThreadError error = kThreadError_None;
    struct sockaddr_in sockaddr;
    RadioMessage message;

    pthread_mutex_lock(&s_mutex);
    VerifyOrExit(s_state == kStateIdle, error = kThreadError_Busy);
    s_state = kStateTransmit;
    pthread_cond_signal(&s_condition_variable);

    s_transmit_frame = packet;
    s_data_pending = false;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr);

    message.mChannel = s_transmit_frame->mChannel;
    memcpy(message.mPsdu, s_transmit_frame->mPsdu, s_transmit_frame->mLength);

    for (int i = 1; i < 34; i++)
    {
        if (args_info.nodeid_arg == i)
        {
            continue;
        }

        sockaddr.sin_port = htons(9000 + i);
        sendto(s_sockfd, &message, 1 + s_transmit_frame->mLength, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    }

    if (reinterpret_cast<Mac::Frame *>(s_transmit_frame)->GetAckRequest())
    {
        s_state = kStateAckWait;
    }
    else
    {
        otPlatRadioSignalTransmitDone();
    }

exit:
    pthread_mutex_unlock(&s_mutex);
    return error;
}

int8_t otPlatRadioGetNoiseFloor()
{
    return 0;
}

otRadioCaps otPlatRadioGetCaps()
{
    return kRadioCapsNone;
}

ThreadError otPlatRadioHandleTransmitDone(bool *rxPending)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(s_state == kStateTransmit || s_state == kStateAckWait, error = kThreadError_InvalidState);

    pthread_mutex_lock(&s_mutex);
    s_state = kStateIdle;
    pthread_cond_signal(&s_condition_variable);
    pthread_mutex_unlock(&s_mutex);

    if (rxPending != NULL)
    {
        *rxPending = s_data_pending;
    }

exit:
    return error;
}

void *phy_receive_thread(void *arg)
{
    fd_set fds;
    int rval;
    RadioPacket receive_frame;
    int length;
    uint8_t tx_sequence, rx_sequence;
    uint8_t command_id;
    RadioMessage message;

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(s_sockfd, &fds);

        rval = select(s_sockfd + 1, &fds, NULL, NULL, NULL);

        if (rval < 0 || !FD_ISSET(s_sockfd, &fds))
        {
            continue;
        }

        pthread_mutex_lock(&s_mutex);

        while (s_state == kStateIdle || s_state == kStateTransmit)
        {
            pthread_cond_wait(&s_condition_variable, &s_mutex);
        }

        switch (s_state)
        {
        case kStateDisabled:
        case kStateIdle:
        case kStateSleep:
            recvfrom(s_sockfd, NULL, 0, 0, NULL, NULL);
            break;

        case kStateTransmit:
            break;

        case kStateAckWait:
            length = recvfrom(s_sockfd, &message, sizeof(message), 0, NULL, NULL);
            receive_frame.mLength = length - 1;
            memcpy(receive_frame.mPsdu, message.mPsdu, receive_frame.mLength);

            if (length < 0)
            {
                assert(false);
            }

            if (reinterpret_cast<Mac::Frame *>(&receive_frame)->GetType() != Mac::Frame::kFcfFrameAck)
            {
                break;
            }

            tx_sequence = reinterpret_cast<Mac::Frame *>(s_transmit_frame)->GetSequence();

            rx_sequence = reinterpret_cast<Mac::Frame *>(&receive_frame)->GetSequence();

            if (tx_sequence != rx_sequence)
            {
                break;
            }

            if (reinterpret_cast<Mac::Frame *>(s_transmit_frame)->GetType() == Mac::Frame::kFcfFrameMacCmd)
            {
                reinterpret_cast<Mac::Frame *>(s_transmit_frame)->GetCommandId(command_id);

                if (command_id == Mac::Frame::kMacCmdDataRequest)
                {
                    s_data_pending = true;
                }
            }

            otPlatRadioSignalTransmitDone();
            break;

        case kStateListen:
            length = recvfrom(s_sockfd, &message, sizeof(message), 0, NULL, NULL);

            if (s_receive_frame->mChannel != message.mChannel)
            {
                break;
            }

            s_state = kStateReceive;
            s_receive_frame->mLength = length - 1;
            memcpy(s_receive_frame->mPsdu, message.mPsdu, s_receive_frame->mLength);

            otPlatRadioSignalReceiveDone();

            while (s_state == kStateReceive)
            {
                pthread_cond_wait(&s_condition_variable, &s_mutex);
            }

            break;

        case kStateReceive:
            assert(false);
            break;
        }

        pthread_mutex_unlock(&s_mutex);
    }

    return NULL;
}

void send_ack()
{
    Mac::Frame *ack_frame;
    RadioMessage message;
    uint8_t sequence;
    struct sockaddr_in sockaddr;

    sequence = reinterpret_cast<Mac::Frame *>(s_receive_frame)->GetSequence();

    ack_frame = reinterpret_cast<Mac::Frame *>(&m_ack_packet);
    ack_frame->InitMacHeader(Mac::Frame::kFcfFrameAck, Mac::Frame::kSecNone);
    ack_frame->SetSequence(sequence);

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr);

    message.mChannel = s_receive_frame->mChannel;
    memcpy(message.mPsdu, m_ack_packet.mPsdu, m_ack_packet.mLength);

    for (int i = 1; i < 34; i++)
    {
        if (args_info.nodeid_arg == i)
        {
            continue;
        }

        sockaddr.sin_port = htons(9000 + i);
        sendto(s_sockfd, &message, 1 + m_ack_packet.mLength, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    }
}

ThreadError otPlatRadioHandleReceiveDone()
{
    ThreadError error = kThreadError_None;
    Mac::Frame *receive_frame;
    uint16_t dstpan;
    Mac::Address dstaddr;

    VerifyOrExit(s_state == kStateReceive, error = kThreadError_InvalidState);

    receive_frame = reinterpret_cast<Mac::Frame *>(s_receive_frame);

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

    s_receive_frame->mPower = -20;
    s_receive_frame->mLqi = kPhyNoLqi;

    // generate acknowledgment
    if (reinterpret_cast<Mac::Frame *>(s_receive_frame)->GetAckRequest())
    {
        send_ack();
    }

exit:
    pthread_mutex_lock(&s_mutex);

    if (s_state != kStateDisabled)
    {
        s_state = kStateIdle;
    }

    pthread_cond_signal(&s_condition_variable);
    pthread_mutex_unlock(&s_mutex);

    return error;
}

#ifdef __cplusplus
}  // end of extern "C"
#endif

}  // namespace Thread
