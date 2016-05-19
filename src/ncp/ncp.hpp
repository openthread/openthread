/*
 *    Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file contains definitions for an FLEN/HDLC interface to the OpenThread stack.
 */

#ifndef NCP_HPP_
#define NCP_HPP_

#include <ncp/ncp_base.hpp>
#include <ncp/flen.hpp>
#include <ncp/hdlc.hpp>

namespace Thread {

class Ncp : public NcpBase
{
    typedef NcpBase super_t;

public:
    Ncp();

    ThreadError Start();
    ThreadError Stop();


    virtual ThreadError OutboundFrameBegin(void);
    virtual uint16_t OutboundFrameGetRemaining(void);
    virtual ThreadError OutboundFrameFeedData(const uint8_t *frame, uint16_t frameLength);
    virtual ThreadError OutboundFrameFeedMessage(Message &message);
    virtual ThreadError OutboundFrameSend(void);

    static void HandleFrame(void *context, uint8_t *aBuf, uint16_t aBufLength);
    static void SendDoneTask(void *context);
    static void ReceiveTask(void *context);

private:
    void HandleFrame(uint8_t *aBuf, uint16_t aBufLength);
    void SendDoneTask();
    void ReceiveTask();

    Hdlc::Encoder mFrameEncoder;
    Hdlc::Decoder mFrameDecoder;

    uint8_t mSendFrame[1500];
    uint8_t mReceiveFrame[1500];
    uint8_t *mSendFrameIter;
    Message *mSendMessage;
};

}  // namespace Thread

#endif  // NCP_HPP_
