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

/**
 * @file
 *   This file implements methods for Network Diagnostics helper types and classes.
 */

#include "network_diagnostic_types.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace NetworkDiagnostic {

AnswerBuilder::AnswerBuilder(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mAnswer(nullptr)
    , mAnswerIndex(0)
    , mQueryId(0)
    , mHasQueryId(false)
    , mPriority(Message::kPriorityNormal)
{
}

AnswerBuilder::~AnswerBuilder(void) { mAnswers.DequeueAndFreeAll(); }

Error AnswerBuilder::Start(const Coap::Message &aRequest)
{
    mAnswerIndex = 0;
    mHasQueryId  = (Tlv::Find<QueryIdTlv>(aRequest, mQueryId) == kErrorNone);
    mPriority    = aRequest.GetPriority();

    return Allocate();
}

Error AnswerBuilder::Finish(void) { return AppendAnswerTlv(AnswerTlvValue::kIsLast); }

Error AnswerBuilder::Allocate(void)
{
    Error error = kErrorNone;

    mAnswer = Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriDiagnosticGetAnswer);
    VerifyOrExit(mAnswer != nullptr, error = kErrorNoBufs);

    IgnoreError(mAnswer->SetPriority(mPriority));

    mAnswers.Enqueue(*mAnswer);

    if (mHasQueryId)
    {
        SuccessOrExit(error = Tlv::Append<QueryIdTlv>(*mAnswer, mQueryId));
    }

exit:
    return error;
}

Error AnswerBuilder::AppendAnswerTlv(AnswerTlvValue::IsLastFlag aFlag)
{
    AnswerTlvValue value;

    value.Init(mAnswerIndex++, aFlag);

    return Tlv::Append<AnswerTlv>(*mAnswer, value);
}

Error AnswerBuilder::CheckAnswerLength(void)
{
    Error error = kErrorNone;

    VerifyOrExit(mAnswer->GetLength() >= kAnswerMessageLengthThreshold);

    SuccessOrExit(error = AppendAnswerTlv(AnswerTlvValue::kMoreToFollow));

    error = Allocate();

exit:
    return error;
}

} // namespace NetworkDiagnostic
} // namespace ot
