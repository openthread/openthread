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

#include "nexus_json.hpp"

namespace ot {
namespace Nexus {
namespace Json {

TestNameString ExtractTestName(const char *aFileName)
{
    TestNameString testName;
    const char    *start;
    const char    *slash;
    const char    *dot;

    start = aFileName;
    slash = strrchr(start, '/');

    if (slash != nullptr)
    {
        start = slash + 1;
    }

    dot = strrchr(start, '.');

    if (dot == nullptr)
    {
        testName.Append("%s", start);
    }
    else
    {
        testName.Append("%.*s", static_cast<int>(dot - start), start);
    }

    return testName;
}

//---------------------------------------------------------------------------------------------------------------------
// Writer

Writer::Writer(void)
    : mFile(nullptr)
    , mIndentation(0)
    , mShouldWriteComma(false)
{
}

Writer::~Writer(void) { CloseFile(); }

Error Writer::OpenFile(const char *aFileName)
{
    Error error = kErrorNone;

    if (mFile != nullptr)
    {
        CloseFile();
    }

    mFile = fopen(aFileName, "w");

    VerifyOrExit(mFile != nullptr, error = kErrorFailed);

    fprintf(mFile, "{");

    mShouldWriteComma = false;
    mIndentation      = kIndentSize;

exit:
    return error;
}

void Writer::CloseFile(void)
{
    VerifyOrExit(mFile != nullptr);

    fprintf(mFile, "\n}\n");

    fclose(mFile);
    mFile             = nullptr;
    mIndentation      = 0;
    mShouldWriteComma = false;

exit:
    return;
}

void Writer::WriteIndentation(void) { fprintf(mFile, "%*s", mIndentation, ""); }

void Writer::GoToNextLine(void) { fprintf(mFile, "%s\n", mShouldWriteComma ? "," : ""); }

void Writer::Write(const char *aName, const char *aValue)
{
    VerifyOrExit(mFile != nullptr);

    GoToNextLine();

    WriteIndentation();

    if (aName != nullptr)
    {
        fprintf(mFile, "\"%s\": ", aName);
    }

    fprintf(mFile, "\"%s\"", (aValue == nullptr) ? "" : aValue);

    mShouldWriteComma = true;

exit:
    return;
}

void Writer::Write(Node::Id aNodeId, const char *aValue) { Write(Node::IdToString(aNodeId).AsCString(), aValue); }

void Writer::Begin(const char *aName, char aBeginChar)
{
    VerifyOrExit(mFile != nullptr);

    GoToNextLine();

    WriteIndentation();
    mIndentation += kIndentSize;

    if (aName != nullptr)
    {
        fprintf(mFile, "\"%s\": ", aName);
    }

    fprintf(mFile, "%c", aBeginChar);

    mShouldWriteComma = false;

exit:
    return;
}

void Writer::Begin(Node::Id aNodeId, char aBeginChar) { Begin(Node::IdToString(aNodeId).AsCString(), aBeginChar); }

void Writer::End(char aEndChar)
{
    mShouldWriteComma = false;
    GoToNextLine();

    if (mIndentation >= kIndentSize)
    {
        mIndentation -= kIndentSize;
    }

    WriteIndentation();
    fprintf(mFile, "%c", aEndChar);

    mShouldWriteComma = true;
}

} // namespace Json
} // namespace Nexus
} // namespace ot
