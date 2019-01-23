/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements the backbone link based radio transceiver.
 */

#include "radio_bblink.hpp"

#include "openthread-core-config.h"

#include <arpa/inet.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <openthread/platform/bblink.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>

#include "platform-posix.h"
#include "radio.h"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "thread/child_table.hpp"

#if OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE

enum
{
    IEEE802154_ACK_LENGTH = 5,

    IEEE802154_FRAME_TYPE_ACK = 2 << 0,
    IEEE802154_FRAME_PENDING  = 1 << 4,

    IEEE802154_BROADCAST = 0xffff,
};

static uint16_t crc16_citt(uint16_t aFcs, uint8_t aByte)
{
    // CRC-16/CCITT, CRC-16/CCITT-TRUE, CRC-CCITT
    // width=16 poly=0x1021 init=0x0000 refin=true refout=true xorout=0x0000 check=0x2189 name="KERMIT"
    // http://reveng.sourceforge.net/crc-catalogue/16.htm#crc.cat.kermit
    static const uint16_t sFcsTable[256] = {
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5,
        0xe97e, 0xf8f7, 0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 0x9cc9, 0x8d40, 0xbfdb, 0xae52,
        0xdaed, 0xcb64, 0xf9ff, 0xe876, 0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 0xad4a, 0xbcc3,
        0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5, 0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
        0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9,
        0x2732, 0x36bb, 0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 0x5285, 0x430c, 0x7197, 0x601e,
        0x14a1, 0x0528, 0x37b3, 0x263a, 0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 0x6306, 0x728f,
        0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862,
        0x9af9, 0x8b70, 0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 0x0840, 0x19c9, 0x2b52, 0x3adb,
        0x4e64, 0x5fed, 0x6d76, 0x7cff, 0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 0x18c1, 0x0948,
        0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
        0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226,
        0xd0bd, 0xc134, 0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c, 0xc60c, 0xd785, 0xe51e, 0xf497,
        0x8028, 0x91a1, 0xa33a, 0xb2b3, 0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 0xd68d, 0xc704,
        0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
        0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb,
        0x0e70, 0x1ff9, 0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 0x7bc7, 0x6a4e, 0x58d5, 0x495c,
        0x3de3, 0x2c6a, 0x1ef1, 0x0f78};
    return (aFcs >> 8) ^ sFcsTable[(aFcs ^ aByte) & 0xff];
}

static void radioComputeCrc(otRadioFrame &aFrame)
{
    uint16_t crc        = 0;
    uint16_t crc_offset = aFrame.mLength - sizeof(uint16_t);

    for (uint16_t i = 0; i < crc_offset; i++)
    {
        crc = crc16_citt(crc, aFrame.mPsdu[i]);
    }

    aFrame.mPsdu[crc_offset]     = crc & 0xff;
    aFrame.mPsdu[crc_offset + 1] = crc >> 8;
}

namespace ot {
namespace PosixApp {

void RadioBackboneLink::Init(const char *aBackboneLink)
{
    if (aBackboneLink != NULL)
    {
        mBackbboneLink    = inet_addr(aBackboneLink);
        mAckTxFrame.mPsdu = &mAckTxBuffer[1];
        mRxFrame.mPsdu    = &mRxBuffer[1];
        mState            = kStateSleep;
    }
    else // Disable backbone link type if BACKBONE_LINK is not set.
    {
        memset(&mBackbboneLink, 0, sizeof(mBackbboneLink));
        mState = kStateDisabled;
    }
}

void RadioBackboneLink::Enable(otInstance *aInstance)
{
    if (mState == kStateSleep)
    {
        mInstance = aInstance;
    }
}

void RadioBackboneLink::Disable(void)
{
    mInstance = NULL;
}

otError RadioBackboneLink::Sleep(void)
{
    VerifyOrExit(IsEnabled());

    if (mFd != -1)
    {
        close(mFd);
        mFd = -1;
    }

    mState = kStateSleep;

exit:
    return OT_ERROR_NONE;
}

otError RadioBackboneLink::Receive(uint8_t aChannel)
{
    int     fd    = -1;
    int     one   = 1;
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mFd == -1, mState = kStateReceive);

    VerifyOrDie((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) != -1, OT_EXIT_ERROR_ERRNO);

    VerifyOrDie(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &one, sizeof(one)) != -1, OT_EXIT_ERROR_ERRNO);

    VerifyOrDie(setsockopt(fd, IPPROTO_IP, IP_TTL, &one, sizeof(one)) != -1, OT_EXIT_ERROR_ERRNO);

#if __linux__
    {
        int priority = 6;

        VerifyOrDie(setsockopt(fd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority)) != -1, OT_EXIT_ERROR_ERRNO);
    }
#endif

    {
        struct ip_mreqn mreq;

        memset(&mreq, 0, sizeof(mreq));
        inet_pton(AF_INET, OT_BACKBONE_LINK_GROUP, &mreq.imr_multiaddr);

        // Always use loopback device to send simulation packets.
        mreq.imr_address.s_addr = mBackbboneLink;

        VerifyOrDie(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &mreq.imr_address, sizeof(mreq.imr_address)) != -1,
                    OT_EXIT_ERROR_ERRNO);
        VerifyOrDie(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != -1, OT_EXIT_ERROR_ERRNO);
    }

    {
        sockaddr_in sockaddr;

        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port   = htons(OT_BACKBONE_LINK_PORT);

        VerifyOrDie(bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) != -1, OT_EXIT_ERROR_ERRNO);
    }

    mFd      = fd;
    mChannel = aChannel;

    mState = kStateReceive;

exit:
    otLogInfoPlat("%s: %s", __PRETTY_FUNCTION__, otThreadErrorToString(error));
    return error;
}

otError RadioBackboneLink::Transmit(otRadioFrame &aFrame)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState == kStateReceive, error = OT_ERROR_INVALID_STATE);

    radioComputeCrc(aFrame);
    mTxFrame     = static_cast<Mac::TxFrame *>(&aFrame);
    mTxBuffer[0] = aFrame.mChannel;
    memcpy(&mTxBuffer[1], mTxFrame->mPsdu, mTxFrame->mLength);

    mState = kStateTransmit;

exit:
    otLogInfoPlat("%s: %s", __PRETTY_FUNCTION__, otThreadErrorToString(error));
    return error;
}

void RadioBackboneLink::DoReceive(void)
{
    bool isAck = mRxFrame.IsAck();

    otLogInfoPlat("%s channel=%u state=%d len=%u ack=%d rxc=%u", __PRETTY_FUNCTION__, mChannel, mState,
                  mRxFrame.GetLength(), isAck, mRxFrame.mChannel);

    VerifyOrExit(mRxFrame.mChannel == mChannel);

    if (mRxFrame.IsAck())
    {
        VerifyOrExit(mState == kStateTransmitAckPending && mTxFrame->GetAckRequest() &&
                     mRxFrame.GetSequence() == mTxFrame->GetSequence());

        mState = kStateReceive;
        platformOnRadioTxDone(mInstance, const_cast<Mac::TxFrame *>(mTxFrame), (isAck ? &mRxFrame : NULL),
                              OT_ERROR_NONE);
    }
    else
    {
        Mac::PanId   panid;
        Mac::Address dst;

        VerifyOrExit(mState == kStateReceive || mState == kStateTransmit || mState == kStateTransmitAckPending);

        if (mRxFrame.GetDstPanId(panid) == OT_ERROR_NONE && mRxFrame.GetDstAddr(dst) == OT_ERROR_NONE)
        {
            VerifyOrExit(panid == mPanId &&
                         ((dst.GetType() == Mac::Address::kTypeShort &&
                           (dst.GetShort() == mShortAddress || dst.GetShort() == IEEE802154_BROADCAST)) ||
                          (dst.GetType() == Mac::Address::kTypeExtended &&
                           !memcmp(&dst.GetExtended(), &mExtAddress, sizeof(mExtAddress)))));
        }

        mRxFrame.mInfo.mRxInfo.mRssi = -20;
        mRxFrame.mInfo.mRxInfo.mLqi  = OT_RADIO_LQI_NONE;

        mRxFrame.mInfo.mRxInfo.mAckedWithFramePending = false;

        // generate acknowledgment
        if (mRxFrame.GetAckRequest())
        {
            Mac::Address src;
            Child *      child;

            mAckTxFrame.mLength  = IEEE802154_ACK_LENGTH;
            mAckTxFrame.mPsdu[0] = IEEE802154_FRAME_TYPE_ACK;

            mRxFrame.GetSrcAddr(src);
            child = static_cast<Instance *>(mInstance)->Get<ChildTable>().FindChild(src, ChildTable::kInStateValid);
            if (mRxFrame.IsDataRequestCommand() && child != NULL && child->GetIndirectMessageCount() > 0)
            {
                mAckTxFrame.mPsdu[0] |= IEEE802154_FRAME_PENDING;
                mRxFrame.mInfo.mRxInfo.mAckedWithFramePending = true;
            }

            mAckTxFrame.mPsdu[1] = 0;
            mAckTxFrame.mPsdu[2] = mRxFrame.GetSequence();

            mAckTxFrame.mPsdu[-1]  = mRxFrame.mChannel;
            mAckTxFrame.mRadioInfo = mRxFrame.mRadioInfo;

            radioComputeCrc(mAckTxFrame);
            DoTransmit(mRxFrame.mRadioInfo, mAckTxBuffer, mAckTxFrame.mLength + 1);
        }

        platformOnRadioRxDone(mInstance, &mRxFrame, OT_ERROR_NONE);
    }

exit:
    otLogInfoPlat("%s", __PRETTY_FUNCTION__);
}

otError RadioBackboneLink::DoTransmit(const otRadioInfo &aRadioInfo, const uint8_t *aBuffer, uint16_t aLength)
{
    otError            error = OT_ERROR_NONE;
    struct sockaddr_in sockaddr;
    ssize_t            rval;
    bool               multicast = (aRadioInfo.mFields.m64[0] == -1ULL && aRadioInfo.mFields.m64[1] == -1ULL);

    assert(mFd != -1);

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port   = htons(OT_BACKBONE_LINK_PORT);

    if (!multicast)
    {
        memcpy(&sockaddr.sin_addr, &aRadioInfo.mFields.m8[12], sizeof(sockaddr.sin_addr));
    }
    else
    {
        sockaddr.sin_addr.s_addr = inet_addr(OT_BACKBONE_LINK_GROUP);
    }

    rval = sendto(mFd, aBuffer, aLength, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

    if (rval > 0)
    {
        assert(rval == aLength);
        mState = kStateReceive;
    }
    else if (rval == 0)
    {
        // TODO deal with this case
    }
    else if (rval == EINTR)
    {
        otLogWarnPlat("Transmit is interrupted, will try again later.");
    }

    otLogInfoPlat("%s 2", __PRETTY_FUNCTION__);
    return error;
}

void RadioBackboneLink::UpdateFdSet(fd_set &aReadFdSet, fd_set &aWriteFdSet, int &aMaxFd, struct timeval &aTimeout)
{
    OT_UNUSED_VARIABLE(aTimeout);

    VerifyOrExit(mFd != -1);

    FD_SET(mFd, &aReadFdSet);

    if (mState == kStateTransmit)
    {
        FD_SET(mFd, &aWriteFdSet);
    }

    // TODO poll error event when the backbone link interface changed

    if (aMaxFd < mFd)
    {
        aMaxFd = mFd;
    }

    if (mState == kStateTransmitAckPending)
    {
        TimeMilli now = TimerMilli::GetNow();

        if (now >= mAckTimeout)
        {
            aTimeout.tv_sec  = 0;
            aTimeout.tv_usec = 0;
        }
        else
        {
            uint32_t timeout = mAckTimeout - now;

            if (aTimeout.tv_sec > 0 || aTimeout.tv_usec > timeout * 1000)
            {
                aTimeout.tv_sec  = 0;
                aTimeout.tv_usec = timeout * 1000;
            }
        }
    }

exit:
    return;
}

void RadioBackboneLink::Process(const fd_set &aReadFdSet, const fd_set &aWriteFdSet)
{
    VerifyOrExit(mFd != -1);

    if (FD_ISSET(mFd, &aReadFdSet))
    {
        ssize_t            rval;
        struct sockaddr_in sockaddr;
        socklen_t          socklen = sizeof(sockaddr);

        // Channel is the receive channel.
        rval = recvfrom(mFd, &mRxFrame.mPsdu[-1], sizeof(mRxBuffer), 0, (struct sockaddr *)&sockaddr, &socklen);

        if (rval > 0)
        {
            mRxFrame.mLength  = static_cast<uint8_t>(rval - 1);
            mRxFrame.mChannel = mRxFrame.mPsdu[-1];
            // Unable to simulate SFD, so use the rx done timestamp instead.
            mRxFrame.mInfo.mRxInfo.mTimestamp = otPlatTimeGet();
        }
        else if (rval == 0)
        {
            // TODO figure when this can return 0 for an UDP socket
            ExitNow();
        }
        else if (errno == EINTR)
        {
            // interrupted, give up in case there are some high priority tasks.
            ExitNow();
        }
        else
        {
            // Oops, socket is broken
            // TODO Need to recover when the link is recovered.
            ExitNow(Sleep());
        }

        // IPv4 address ::ffff:<IPv4>
        mRxFrame.mRadioInfo.mFields.m8[10] = 0xff;
        mRxFrame.mRadioInfo.mFields.m8[11] = 0xff;
        memcpy(&mRxFrame.mRadioInfo.mFields.m8[12], &sockaddr.sin_addr, sizeof(sockaddr.sin_addr));

        if (htons(sockaddr.sin_port) != OT_BACKBONE_LINK_PORT)
        {
            // Silently drop packets with a wrong source port.
            otLogWarnPlat("Unexpected source address of backbone encapsulation");
        }
        else
        {
            DoReceive();
        }
    }

    // For simplicity, send message after both link types are writable
    if (mState == kStateTransmit && FD_ISSET(mFd, &aWriteFdSet))
    {
        otError error = DoTransmit(mTxFrame->mRadioInfo, mTxBuffer, mTxFrame->mLength + 1);

        platformOnRadioTxStarted(mInstance, const_cast<Mac::TxFrame *>(mTxFrame));

        if (error == OT_ERROR_NONE && mTxFrame->GetAckRequest())
        {
            mState      = kStateTransmitAckPending;
            mAckTimeout = TimerMilli::GetNow() + kAckTimeout;
        }
        else
        {
            mState = kStateReceive;
            platformOnRadioTxDone(mInstance, mTxFrame, NULL, error);
        }
    }

    if (mState == kStateTransmitAckPending && TimerMilli::GetNow() >= mAckTimeout)
    {
        mState = kStateReceive;
        platformOnRadioTxDone(mInstance, mTxFrame, NULL, OT_ERROR_NO_ACK);
    }

exit:
    return;
}

} // namespace PosixApp
} // namespace ot

#endif // OPENTHREAD_CONFIG_BACKBONE_LINK_TYPE_ENABLE
