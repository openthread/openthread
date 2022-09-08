/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file includes simulation-event message formatting and parsing functions.
 */

#if OPENTHREAD_SIMULATION_VIRTUAL_TIME

#include "platform-simulation.h"

extern uint16_t sPortOffset;
extern int      sSockFd;

void otSimSendSleepEvent(void)
{
    struct Event event;
    assert(platformAlarmGetNext() > 0);

    event.mDelay      = platformAlarmGetNext();
    event.mEvent      = OT_SIM_EVENT_ALARM_FIRED;
    event.mDataLength = 8;
    memcpy(&event.mData, &event, 8);

    otSimSendEvent(&event);
}

void otSimSendRadioTxEvent(struct Event *aEvent, struct TxEventData *aTxEventData, uint8_t *aPayload, size_t aLenPayload) {

    aEvent->mEvent = OT_SIM_EVENT_RADIO_TX;
    memcpy(aEvent->mData, aTxEventData, sizeof(struct TxEventData));
    memcpy(aEvent->mData + sizeof(struct TxEventData), aPayload, aLenPayload);
    aEvent->mDataLength = sizeof(struct TxEventData) + aLenPayload;

    otSimSendEvent(aEvent);
}

void otSimSendEvent(struct Event *aEvent)
{
    ssize_t            rval;
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr);
    sockaddr.sin_port = htons(9000 + sPortOffset);

    // send header and data.
    rval = sendto(sSockFd, aEvent, offsetof(struct Event, mData) + aEvent->mDataLength, 0, (struct sockaddr *)&sockaddr,
                  sizeof(sockaddr));

    if (rval < 0)
    {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}

#endif // OPENTHREAD_SIMULATION_VIRTUAL_TIME
