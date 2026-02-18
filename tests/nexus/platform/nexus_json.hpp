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

#ifndef OT_NEXUS_PLATFORM_NEXUS_JSON_HPP_
#define OT_NEXUS_PLATFORM_NEXUS_JSON_HPP_

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nexus_node.hpp"

namespace ot {
namespace Nexus {
namespace Json {

constexpr uint16_t kTestNameStringSize = 32;

using TestNameString = String<kTestNameStringSize>;

TestNameString ExtractTestName(const char *aFileName);

class Writer
{
public:
    // Simple JSON writer (used by `Core::SaveTestInfo()`).

    Writer(void);
    ~Writer(void);

    Error OpenFile(const char *aFileName);
    void  CloseFile(void);

    // If `aValue` is `nullptr`, empty string is ("") used.
    void WriteNameValue(const char *aName, const char *aValue) { Write(aName, aValue); }
    void WriteNameValue(Node::Id aNodeId, const char *aValue) { Write(aNodeId, aValue); }
    void WriteValue(const char *aValue) { Write(/* aName */ nullptr, aValue); }

    void BeginObject(const char *aName) { Begin(aName, '{'); }
    void BeginObject(Node::Id aNodeId) { Begin(aNodeId, '{'); }
    void EndObject(void) { End('}'); }

    void BeginArray(const char *aName) { Begin(aName, '['); }
    void BeginArray(Node::Id aNodeId) { Begin(aNodeId, '['); }
    void EndArray(void) { End(']'); }

private:
    static constexpr uint16_t kIndentSize = 2;

    void WriteIndentation(void);
    void GoToNextLine(void);
    void Write(const char *aName, const char *aValue);
    void Write(Node::Id aNodeId, const char *aValue);
    void Begin(const char *aName, char aBeginChar);
    void Begin(Node::Id aNodeId, char aBeginChar);
    void End(char aEndChar);

    FILE    *mFile;
    uint16_t mIndentation;
    bool     mShouldWriteComma;
};

} // namespace Json
} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_PLATFORM_NEXUS_JSON_HPP_
