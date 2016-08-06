
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <openthread.h>

#include <common/code_utils.hpp>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <platform.h>
#include <platform/radio.h>
#include "platform-windows.h"

enum
{
    IEEE802154_MIN_LENGTH         = 5,
    IEEE802154_MAX_LENGTH         = 127,
    IEEE802154_ACK_LENGTH         = 5,

    IEEE802154_BROADCAST          = 0xffff,

    IEEE802154_FRAME_TYPE_ACK     = 2 << 0,
    IEEE802154_FRAME_TYPE_MACCMD  = 3 << 0,
    IEEE802154_FRAME_TYPE_MASK    = 7 << 0,

    IEEE802154_SECURITY_ENABLED   = 1 << 3,
    IEEE802154_FRAME_PENDING      = 1 << 4,
    IEEE802154_ACK_REQUEST        = 1 << 5,
    IEEE802154_PANID_COMPRESSION  = 1 << 6,

    IEEE802154_DST_ADDR_NONE      = 0 << 2,
    IEEE802154_DST_ADDR_SHORT     = 2 << 2,
    IEEE802154_DST_ADDR_EXT       = 3 << 2,
    IEEE802154_DST_ADDR_MASK      = 3 << 2,

    IEEE802154_SRC_ADDR_NONE      = 0 << 6,
    IEEE802154_SRC_ADDR_SHORT     = 2 << 6,
    IEEE802154_SRC_ADDR_EXT       = 3 << 6,
    IEEE802154_SRC_ADDR_MASK      = 3 << 6,

    IEEE802154_DSN_OFFSET         = 2,
    IEEE802154_DSTPAN_OFFSET      = 3,
    IEEE802154_DSTADDR_OFFSET     = 5,

    IEEE802154_SEC_LEVEL_MASK     = 7 << 0,

    IEEE802154_KEY_ID_MODE_0      = 0 << 3,
    IEEE802154_KEY_ID_MODE_1      = 1 << 3,
    IEEE802154_KEY_ID_MODE_2      = 2 << 3,
    IEEE802154_KEY_ID_MODE_3      = 3 << 3,
    IEEE802154_KEY_ID_MODE_MASK   = 3 << 3,

    IEEE802154_MACCMD_DATA_REQ    = 4,
};

OT_TOOL_PACKED_BEGIN 
struct RadioMessage
{
    uint8_t mChannel;
    uint8_t mPsdu[kMaxPHYPacketSize];
};
OT_TOOL_PACKED_END

static void radioTransmit(const struct RadioMessage *msg, const struct RadioPacket *pkt);
static void radioSendMessage(void);
static void radioSendAck(void);
static void radioProcessFrame(void);

static PhyState sState = kStateDisabled;
static struct RadioMessage sReceiveMessage;
static struct RadioMessage sTransmitMessage;
static struct RadioMessage sAckMessage;
static RadioPacket sReceiveFrame;
static RadioPacket sTransmitFrame;
static RadioPacket sAckFrame;

static uint8_t sExtendedAddress[OT_EXT_ADDRESS_SIZE];
static uint16_t sShortAddress;
static uint16_t sPanid;
static int sSockFd;
static bool sPromiscuous = false;
static bool sAckWait = false;

static inline bool isFrameTypeAck(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_FRAME_TYPE_MASK) == IEEE802154_FRAME_TYPE_ACK;
}

static inline bool isFrameTypeMacCmd(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_FRAME_TYPE_MASK) == IEEE802154_FRAME_TYPE_MACCMD;
}

static inline bool isSecurityEnabled(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_SECURITY_ENABLED) != 0;
}

static inline bool isFramePending(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_FRAME_PENDING) != 0;
}

static inline bool isAckRequested(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_ACK_REQUEST) != 0;
}

static inline bool isPanIdCompressed(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_PANID_COMPRESSION) != 0;
}

static inline bool isDataRequest(const uint8_t *frame)
{
    const uint8_t *cur = frame;
    uint8_t securityControl;
    bool rval;

    // FCF + DSN
    cur += 2 + 1;

    VerifyOrExit(isFrameTypeMacCmd(frame), rval = false);

    // Destination PAN + Address
    switch (frame[1] & IEEE802154_DST_ADDR_MASK)
    {
    case IEEE802154_DST_ADDR_SHORT:
        cur += sizeof(otPanId) + sizeof(otShortAddress);
        break;

    case IEEE802154_DST_ADDR_EXT:
        cur += sizeof(otPanId) + sizeof(otExtAddress);
        break;

    default:
        ExitNow(rval = false);
    }

    // Source PAN + Address
    switch (frame[1] & IEEE802154_SRC_ADDR_MASK)
    {
    case IEEE802154_SRC_ADDR_SHORT:
        if (!isPanIdCompressed(frame))
        {
            cur += sizeof(otPanId);
        }

        cur += sizeof(otShortAddress);
        break;

    case IEEE802154_SRC_ADDR_EXT:
        if (!isPanIdCompressed(frame))
        {
            cur += sizeof(otPanId);
        }

        cur += sizeof(otExtAddress);
        break;

    default:
        ExitNow(rval = false);
    }

    // Security Control + Frame Counter + Key Identifier
    if (isSecurityEnabled(frame))
    {
        securityControl = *cur;

        if (securityControl & IEEE802154_SEC_LEVEL_MASK)
        {
            cur += 1 + 4;
        }

        switch (securityControl & IEEE802154_KEY_ID_MODE_MASK)
        {
        case IEEE802154_KEY_ID_MODE_0:
            cur += 0;
            break;

        case IEEE802154_KEY_ID_MODE_1:
            cur += 1;
            break;

        case IEEE802154_KEY_ID_MODE_2:
            cur += 5;
            break;

        case IEEE802154_KEY_ID_MODE_3:
            cur += 9;
            break;
        }
    }

    // Command ID
    rval = cur[0] == IEEE802154_MACCMD_DATA_REQ;

exit:
    return rval;
}

static inline uint8_t getDsn(const uint8_t *frame)
{
    return frame[IEEE802154_DSN_OFFSET];
}

static inline otPanId getDstPan(const uint8_t *frame)
{
    return (((otPanId)frame[IEEE802154_DSTPAN_OFFSET + 1]) << 8) | frame[IEEE802154_DSTPAN_OFFSET];
}

static inline otShortAddress getShortAddress(const uint8_t *frame)
{
    return (((otShortAddress)frame[IEEE802154_DSTADDR_OFFSET + 1]) << 8) | frame[IEEE802154_DSTADDR_OFFSET];
}

static inline void getExtAddress(const uint8_t *frame, otExtAddress *address)
{
    size_t i;

    for (i = 0; i < sizeof(otExtAddress); i++)
    {
        address->m8[i] = frame[IEEE802154_DSTADDR_OFFSET + (sizeof(otExtAddress) - 1 - i)];
    }
}

ThreadError otPlatRadioSetPanId(otContext *, uint16_t panid)
{
    ThreadError error = kThreadError_Busy;

    if (sState != kStateTransmit)
    {
        sPanid = panid;
        error = kThreadError_None;
    }

    return error;
}

ThreadError otPlatRadioSetExtendedAddress(otContext *, uint8_t *address)
{
    ThreadError error = kThreadError_Busy;

    if (sState != kStateTransmit)
    {
        size_t i;

        for (i = 0; i < sizeof(sExtendedAddress); i++)
        {
            sExtendedAddress[i] = address[sizeof(sExtendedAddress) - 1 - i];
        }

        error = kThreadError_None;
    }

    return error;
}

ThreadError otPlatRadioSetShortAddress(otContext *, uint16_t address)
{
    ThreadError error = kThreadError_Busy;

    if (sState != kStateTransmit)
    {
        sShortAddress = address;
        error = kThreadError_None;
    }

    return error;
}

void otPlatRadioSetPromiscuous(otContext *, bool aEnable)
{
    sPromiscuous = aEnable;
}

void windowsRadioInit(void)
{
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    int err = WSAStartup(wVersionRequested, &wsaData);
    assert(err == 0);

    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;

    if (sPromiscuous)
    {
        sockaddr.sin_port = htons(9000 + WELLKNOWN_NODE_ID);
    }
    else
    {
        sockaddr.sin_port = htons(9000 + NODE_ID);
    }

    sockaddr.sin_addr.s_addr = INADDR_ANY;

    sSockFd = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(sSockFd != -1);
    err = bind(sSockFd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    assert(err == 0);
    
    sReceiveFrame.mPsdu = sReceiveMessage.mPsdu;
    sTransmitFrame.mPsdu = sTransmitMessage.mPsdu;
    sAckFrame.mPsdu = sAckMessage.mPsdu;
}

ThreadError otPlatRadioEnable(otContext *)
{
    ThreadError error = kThreadError_Busy;

    if (sState == kStateSleep || sState == kStateDisabled)
    {
        error = kThreadError_None;
        sState = kStateSleep;
    }

    return error;
}

ThreadError otPlatRadioDisable(otContext *)
{
    ThreadError error = kThreadError_Busy;

    if (sState == kStateDisabled || sState == kStateSleep)
    {
        error = kThreadError_None;
        sState = kStateDisabled;
    }

    return error;
}

ThreadError otPlatRadioSleep(otContext *)
{
    ThreadError error = kThreadError_Busy;

    if (sState == kStateSleep || sState == kStateReceive)
    {
        error = kThreadError_None;
        sState = kStateSleep;
    }

    return error;
}

ThreadError otPlatRadioReceive(otContext *, uint8_t aChannel)
{
    ThreadError error = kThreadError_Busy;

    if (sState != kStateDisabled)
    {
        error = kThreadError_None;
        sState = kStateReceive;
        sAckWait = false;
        sReceiveFrame.mChannel = aChannel;
    }

    return error;
}

ThreadError otPlatRadioTransmit(otContext *)
{
    ThreadError error = kThreadError_Busy;

    if ((sState == kStateTransmit && !sAckWait) || sState == kStateReceive)
    {
        error = kThreadError_None;
        sState = kStateTransmit;
    }

    return error;
}

RadioPacket *otPlatRadioGetTransmitBuffer(otContext *)
{
    return &sTransmitFrame;
}

int8_t otPlatRadioGetNoiseFloor(otContext *)
{
    return 0;
}

otRadioCaps otPlatRadioGetCaps(otContext *)
{
    return kRadioCapsNone;
}

bool otPlatRadioGetPromiscuous(otContext *)
{
    return sPromiscuous;
}

void radioReceive(void)
{
    if (sState != kStateTransmit || sAckWait)
    {
        int rval = recvfrom(sSockFd, (char*)&sReceiveMessage, sizeof(sReceiveMessage), 0, NULL, NULL);
        assert(rval >= 0);

        sReceiveFrame.mLength = rval - 1;

        if (sAckWait &&
            sTransmitFrame.mChannel == sReceiveMessage.mChannel &&
            isFrameTypeAck(sReceiveFrame.mPsdu))
        {
            uint8_t tx_sequence = getDsn(sTransmitFrame.mPsdu);
            uint8_t rx_sequence = getDsn(sReceiveFrame.mPsdu);

            if (tx_sequence == rx_sequence)
            {
                sState = kStateReceive;
                sAckWait = false;
                otPlatRadioTransmitDone(sContext, isFramePending(sReceiveFrame.mPsdu), kThreadError_None);
            }
        }
        else if (sState == kStateReceive &&
                 sReceiveFrame.mChannel == sReceiveMessage.mChannel)
        {
            radioProcessFrame();
        }
    }
}

void radioSendMessage(void)
{
    sTransmitMessage.mChannel = sTransmitFrame.mChannel;

    radioTransmit(&sTransmitMessage, &sTransmitFrame);

    sAckWait = isAckRequested(sTransmitFrame.mPsdu);

    if (!sAckWait)
    {
        sState = kStateReceive;
        otPlatRadioTransmitDone(sContext, false, kThreadError_None);
    }
}

void windowsRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd)
{
    if (aReadFdSet != NULL && (sState != kStateTransmit || sAckWait))
    {
        FD_SET(sSockFd, aReadFdSet);

        if (aMaxFd != NULL && *aMaxFd < sSockFd)
        {
            *aMaxFd = sSockFd;
        }
    }
    
    if (aWriteFdSet != NULL && sState == kStateTransmit && !sAckWait)
    {
        FD_SET(sSockFd, aWriteFdSet);

        if (aMaxFd != NULL && *aMaxFd < sSockFd)
        {
            *aMaxFd = sSockFd;
        }
    }
}

void windowsRadioProcess(void)
{
    const int flags = POLLRDNORM | POLLERR | POLLNVAL | POLLHUP;
    struct pollfd pollfd = { (SOCKET)sSockFd, POLLRDNORM, 0 };

    int err = WSAPoll(&pollfd, 1, 0);
    if (err == SOCKET_ERROR)
    {
        err = WSAGetLastError();
        assert(err == SOCKET_ERROR);
    }
    if (err > 0 && (pollfd.revents & flags) != 0)
    {
        radioReceive();
    }

    if (sState == kStateTransmit)
    {
        radioSendMessage();
    }
}

void radioTransmit(const struct RadioMessage *msg, const struct RadioPacket *pkt)
{
    uint32_t i;
    struct sockaddr_in sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr);

    for (i = 1; i <= WELLKNOWN_NODE_ID; i++)
    {
        int rval;

        if (NODE_ID == i)
        {
            continue;
        }

        sockaddr.sin_port = htons(9000 + i);
        rval = sendto(sSockFd, (char*)msg, 1 + pkt->mLength,
                      0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
        assert(rval >= 0);
    }
}

void radioSendAck(void)
{
    sAckFrame.mLength = IEEE802154_ACK_LENGTH;
    sAckMessage.mPsdu[0] = IEEE802154_FRAME_TYPE_ACK;

    if (isDataRequest(sReceiveFrame.mPsdu))
    {
        sAckMessage.mPsdu[0] |= IEEE802154_FRAME_PENDING;
    }

    sAckMessage.mPsdu[1] = 0;
    sAckMessage.mPsdu[2] = getDsn(sReceiveFrame.mPsdu);

    sAckMessage.mChannel = sReceiveFrame.mChannel;

    radioTransmit(&sAckMessage, &sAckFrame);
}

void radioProcessFrame(void)
{
    ThreadError error = kThreadError_None;
    otPanId dstpan;
    otShortAddress short_address;
    otExtAddress ext_address;

    VerifyOrExit(sPromiscuous == false, error = kThreadError_None);

    switch (sReceiveFrame.mPsdu[1] & IEEE802154_DST_ADDR_MASK)
    {
    case IEEE802154_DST_ADDR_NONE:
        break;

    case IEEE802154_DST_ADDR_SHORT:
        dstpan = getDstPan(sReceiveFrame.mPsdu);
        short_address = getShortAddress(sReceiveFrame.mPsdu);
        VerifyOrExit((dstpan == IEEE802154_BROADCAST || dstpan == sPanid) &&
                     (short_address == IEEE802154_BROADCAST || short_address == sShortAddress),
                     error = kThreadError_Abort);
        break;

    case IEEE802154_DST_ADDR_EXT:
        dstpan = getDstPan(sReceiveFrame.mPsdu);
        getExtAddress(sReceiveFrame.mPsdu, &ext_address);
        VerifyOrExit((dstpan == IEEE802154_BROADCAST || dstpan == sPanid) &&
                     memcmp(&ext_address, sExtendedAddress, sizeof(ext_address)) == 0,
                     error = kThreadError_Abort);
        break;

    default:
        ExitNow(error = kThreadError_Abort);
    }

    sReceiveFrame.mPower = -20;
    sReceiveFrame.mLqi = kPhyNoLqi;

    // generate acknowledgment
    if (isAckRequested(sReceiveFrame.mPsdu))
    {
        radioSendAck();
    }

exit:

    otPlatRadioReceiveDone(sContext, error == kThreadError_None ? &sReceiveFrame : NULL, error);
}
