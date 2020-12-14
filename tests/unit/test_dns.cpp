/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#include <string.h>

#include <openthread/config.h>

#include "test_platform.h"
#include "test_util.hpp"

#include "common/instance.hpp"
#include "net/dns_headers.hpp"

namespace ot {

void TestDnsName(void)
{
    enum
    {
        kMaxSize   = 300,
        kLabelSize = 64,
        kNameSize  = 256,
    };

    struct TestName
    {
        const char *   mName;
        uint16_t       mEncodedLength;
        const uint8_t *mEncodedData;
        const char **  mLabels;
        const char *   mExpectedReadName;
    };

    Instance *   instance;
    MessagePool *messagePool;
    Message *    message;
    uint8_t      buffer[kMaxSize];
    uint16_t     len;
    uint16_t     offset;
    char         label[kLabelSize];
    uint8_t      labelLength;
    char         name[kNameSize];

    static const uint8_t kEncodedName1[] = {7, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 3, 'c', 'o', 'm', 0};
    static const uint8_t kEncodedName2[] = {3, 'f', 'o', 'o', 1, 'a', 2, 'b', 'b', 3, 'e', 'd', 'u', 0};
    static const uint8_t kEncodedName3[] = {10, 'f', 'o', 'u', 'n', 'd', 'a', 't', 'i', 'o', 'n', 0};
    static const uint8_t kEncodedName4[] = {0};

    static const char *kLabels1[] = {"example", "com", nullptr};
    static const char *kLabels2[] = {"foo", "a", "bb", "edu", nullptr};
    static const char *kLabels3[] = {"foundation", nullptr};
    static const char *kLabels4[] = {nullptr};

    static const TestName kTestNames[] = {
        {"example.com", sizeof(kEncodedName1), kEncodedName1, kLabels1, "example.com."},
        {"example.com.", sizeof(kEncodedName1), kEncodedName1, kLabels1, "example.com."},
        {"foo.a.bb.edu", sizeof(kEncodedName2), kEncodedName2, kLabels2, "foo.a.bb.edu."},
        {"foo.a.bb.edu.", sizeof(kEncodedName2), kEncodedName2, kLabels2, "foo.a.bb.edu."},
        {"foundation", sizeof(kEncodedName3), kEncodedName3, kLabels3, "foundation."},
        {"foundation.", sizeof(kEncodedName3), kEncodedName3, kLabels3, "foundation."},
        {"", sizeof(kEncodedName4), kEncodedName4, kLabels4, "."},
        {".", sizeof(kEncodedName4), kEncodedName4, kLabels4, "."},
        {nullptr, sizeof(kEncodedName4), kEncodedName4, kLabels4, "."},
    };

    static const char *kInvalidNames[] = {
        "foo..bar",
        "..",
        "a..",
        "..b",

        // Long label
        "a.an-invalid-very-long-label-string-with-more-than-sixty-four-characters.com",

        // Long name (more than 255 characters)
        "HereIsSomeoneHidden.MyHoldFromMeTaken.FromSelfHasMeDriven.MyLeadFromMeTaken."
        "HereIsSomeoneHidden.AsLifeSweeterThanLife.TakesMeToGardenOfSoul.MyFortFromMeTaken."
        "HereIsSomeoneHidden.LikeSugarInSugarCane.ASweetSugarTrader.MyShopFromMeTaken."
        "SorcererAndMagician.NoEyesCanEverSee.AnArtfulConjurer.MySenseFromMeTaken."
        "MyEyesWillNeverSee.BeautiesOfTheWholeWorld.BeholdWhoseVisionFine.MySightFromMeTaken"
        "PoemByRumiMolana",
    };

    printf("================================================================\n");
    printf("TestDnsName()\n");

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr, "Null OpenThread instance");

    messagePool = &instance->Get<MessagePool>();
    VerifyOrQuit((message = messagePool->New(Message::kTypeIp6, 0)) != nullptr, "Message::New failed");

    printf("----------------------------------------------------------------\n");
    printf("Append names, check encoded bytes, parse name and read labels:\n");

    for (const TestName &test : kTestNames)
    {
        IgnoreError(message->SetLength(0));

        SuccessOrQuit(Dns::Name::AppendName(test.mName, *message), "Name::AppendName() failed");

        len = message->GetLength();
        SuccessOrQuit(message->Read(0, buffer, len), "Message::Read() failed");

        DumpBuffer(test.mName, buffer, len);

        VerifyOrQuit(len == test.mEncodedLength, "Encoded length does not match expected value");
        VerifyOrQuit(memcmp(buffer, test.mEncodedData, len) == 0, "Encoded name data does not match expected data");

        // Parse and skip over the name
        offset = 0;
        SuccessOrQuit(Dns::Name::ParseName(*message, offset), "Name::ParseName() failed");
        VerifyOrQuit(offset == len, "Name::ParseName() returned incorrect offset");

        // Read labels one by one.
        offset = 0;

        for (uint8_t index = 0; test.mLabels[index] != nullptr; index++)
        {
            labelLength = sizeof(label);
            SuccessOrQuit(Dns::Name::ReadLabel(*message, offset, 0, label, labelLength), "Name::ReadLabel() failed");

            printf("Label[%d] = \"%s\"\n", index, label);

            VerifyOrQuit(strcmp(label, test.mLabels[index]) == 0, "Name::ReadLabel() did not get expected label");
            VerifyOrQuit(labelLength == strlen(label), "Name::ReadLabel() returned incorrect label length");
        }

        labelLength = sizeof(label);
        VerifyOrQuit(Dns::Name::ReadLabel(*message, offset, 0, label, labelLength) == OT_ERROR_NOT_FOUND,
                     "Name::ReadLabel() failed at end of the name");

        // Read entire name
        offset = 0;
        SuccessOrQuit(Dns::Name::ReadName(*message, offset, 0, name, sizeof(name)), "Name::ReadName() failed");

        printf("Read name =\"%s\"\n", name);

        VerifyOrQuit(strcmp(name, test.mExpectedReadName) == 0, "Name::ReadName() did not get expected name");
        VerifyOrQuit(offset == len, "Name::ReadName() returned incorrect offset");

        // Read entire name with different name buffer sizes (just right and one byte off the expected size)
        offset = 0;
        SuccessOrQuit(
            Dns::Name::ReadName(*message, offset, 0, name, static_cast<uint16_t>(strlen(test.mExpectedReadName) + 1)),
            "Name::ReadName() failed with exact name buffer size");
        offset = 0;
        VerifyOrQuit(Dns::Name::ReadName(*message, offset, 0, name,
                                         static_cast<uint16_t>(strlen(test.mExpectedReadName))) == OT_ERROR_NO_BUFS,
                     "Name::ReadName() did not fail with too small name buffer size");
    }

    printf("----------------------------------------------------------------\n");
    printf("Invalid names:\n");

    for (const char *&invalidName : kInvalidNames)
    {
        IgnoreError(message->SetLength(0));

        printf("\"%s\"\n", invalidName);

        VerifyOrQuit(Dns::Name::AppendName(invalidName, *message) == OT_ERROR_INVALID_ARGS,
                     "Name::AppendName() did not fail with an invalid name");
    }

    printf("----------------------------------------------------------------\n");
    printf("Append as multiple labels and terminator instead of full name:\n");

    for (const TestName &test : kTestNames)
    {
        IgnoreError(message->SetLength(0));

        SuccessOrQuit(Dns::Name::AppendMultipleLabels(test.mName, *message), "Name::Append() failed");
        SuccessOrQuit(Dns::Name::AppendTerminator(*message), "Name::AppendTerminator() failed");

        len = message->GetLength();
        SuccessOrQuit(message->Read(0, buffer, len), "Message::Read() failed");

        DumpBuffer(test.mName, buffer, len);

        VerifyOrQuit(len == test.mEncodedLength, "Encoded length does not match expected value");
        VerifyOrQuit(memcmp(buffer, test.mEncodedData, len) == 0, "Encoded name data does not match expected data");
    }

    printf("----------------------------------------------------------------\n");
    printf("Append labels one by one:\n");

    for (const TestName &test : kTestNames)
    {
        IgnoreError(message->SetLength(0));

        for (uint8_t index = 0; test.mLabels[index] != nullptr; index++)
        {
            SuccessOrQuit(Dns::Name::AppendLabel(test.mLabels[index], *message), "Name::AppendLabel() failed");
        }

        SuccessOrQuit(Dns::Name::AppendTerminator(*message), "Name::AppendTerminator() failed");

        len = message->GetLength();
        SuccessOrQuit(message->Read(0, buffer, len), "Message::Read() failed");

        DumpBuffer(test.mName, buffer, len);

        VerifyOrQuit(len == test.mEncodedLength, "Encoded length does not match expected value");
        VerifyOrQuit(memcmp(buffer, test.mEncodedData, len) == 0, "Encoded name data does not match expected data");
    }

    message->Free();
    testFreeInstance(instance);
}

void TestDnsCompressedName(void)
{
    enum
    {
        kHeaderOffset   = 10,
        kGuardBlockSize = 20,
        kMaxBufferSize  = 100,
        kLabelSize      = 64,
        kNameSize       = 256,

        kName2EncodedSize = 4 + 2,  // encoded "FOO" + pointer label (2 bytes)
        kName3EncodedSize = 2,      // pointer label (2 bytes)
        kName4EncodedSize = 15 + 2, // encoded "Human.Readable" + pointer label (2 bytes).

    };

    const char kName[]          = "F.ISI.ARPA";
    const char kLabel1[]        = "FOO";
    const char kInstanceLabel[] = "Human.Readable";

    static const uint8_t kEncodedName[]    = {1, 'F', 3, 'I', 'S', 'I', 4, 'A', 'R', 'P', 'A', 0};
    static const uint8_t kIsiRelativeIndex = 2; // Index in kEncodedName to the start of "ISI.ARPA" portion.

    static const char *kName1Labels[] = {"F", "ISI", "ARPA"};
    static const char *kName2Labels[] = {"FOO", "F", "ISI", "ARPA"};
    static const char *kName3Labels[] = {"ISI", "ARPA"};
    static const char *kName4Labels[] = {"Human.Readable", "F", "ISI", "ARPA"};

    static const char kExpectedReadName1[] = "F.ISI.ARPA.";
    static const char kExpectedReadName2[] = "FOO.F.ISI.ARPA.";
    static const char kExpectedReadName3[] = "ISI.ARPA.";

    Instance *   instance;
    MessagePool *messagePool;
    Message *    message;
    uint16_t     offset;
    uint16_t     name1Offset;
    uint16_t     name2Offset;
    uint16_t     name3Offset;
    uint16_t     name4Offset;
    uint8_t      buffer[kMaxBufferSize];
    char         label[kLabelSize];
    uint8_t      labelLength;
    char         name[kNameSize];

    printf("================================================================\n");
    printf("TestDnsCompressedName()\n");

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr, "Null OpenThread instance");

    messagePool = &instance->Get<MessagePool>();
    VerifyOrQuit((message = messagePool->New(Message::kTypeIp6, 0)) != nullptr, "Message::New failed");

    // Append name1 "F.ISI.ARPA"

    for (uint8_t index = 0; index < kHeaderOffset + kGuardBlockSize; index++)
    {
        SuccessOrQuit(message->Append(index), "Message::Append() failed");
    }

    name1Offset = message->GetLength();
    SuccessOrQuit(Dns::Name::AppendName(kName, *message), "Name::AppendName() failed");

    // Append name2 "FOO.F.ISI.ARPA" as a compressed name after some guard/extra bytes

    for (uint8_t index = 0; index < kGuardBlockSize; index++)
    {
        uint8_t value = 0xff;
        SuccessOrQuit(message->Append(value), "Message::Append() failed");
    }

    name2Offset = message->GetLength();

    SuccessOrQuit(Dns::Name::AppendLabel(kLabel1, *message), "Name::AppendLabel() failed");
    SuccessOrQuit(Dns::Name::AppendPointerLabel(name1Offset - kHeaderOffset, *message),
                  "Name::AppendPointerLabel() failed");

    // Append name3 "ISI.ARPA" as a compressed name after some guard/extra bytes

    for (uint8_t index = 0; index < kGuardBlockSize; index++)
    {
        uint8_t value = 0xaa;
        SuccessOrQuit(message->Append(value), "Message::Append() failed");
    }

    name3Offset = message->GetLength();
    SuccessOrQuit(Dns::Name::AppendPointerLabel(name1Offset + kIsiRelativeIndex - kHeaderOffset, *message),
                  "Name::AppendPointerLabel() failed");

    name4Offset = message->GetLength();
    SuccessOrQuit(Dns::Name::AppendLabel(kInstanceLabel, *message), "Name::AppendLabel() failed");
    SuccessOrQuit(Dns::Name::AppendPointerLabel(name1Offset - kHeaderOffset, *message),
                  "Name::AppendPointerLabel() failed");

    printf("----------------------------------------------------------------\n");
    printf("Read and parse the uncompressed name-1 \"F.ISI.ARPA\"\n");

    SuccessOrQuit(message->Read(name1Offset, buffer, sizeof(kEncodedName)), "Message::Read() failed");

    DumpBuffer(kName, buffer, sizeof(kEncodedName));
    VerifyOrQuit(memcmp(buffer, kEncodedName, sizeof(kEncodedName)) == 0,
                 "Encoded name data does not match expected data");

    offset = name1Offset;
    SuccessOrQuit(Dns::Name::ParseName(*message, offset), "Name::ParseName() failed");

    VerifyOrQuit(offset == name1Offset + sizeof(kEncodedName), "Name::ParseName() returned incorrect offset");

    offset = name1Offset;

    for (const char *nameLabel : kName1Labels)
    {
        labelLength = sizeof(label);
        SuccessOrQuit(Dns::Name::ReadLabel(*message, offset, kHeaderOffset, label, labelLength),
                      "Name::ReadLabel() failed");

        printf("label: \"%s\"\n", label);
        VerifyOrQuit(strcmp(label, nameLabel) == 0, "Name::ReadLabel() did not get expected label");
        VerifyOrQuit(labelLength == strlen(label), "Name::ReadLabel() returned incorrect label length");
    }

    labelLength = sizeof(label);
    VerifyOrQuit(Dns::Name::ReadLabel(*message, offset, kHeaderOffset, label, labelLength) == OT_ERROR_NOT_FOUND,
                 "Name::ReadLabel() failed at end of the name");

    offset = name1Offset;
    SuccessOrQuit(Dns::Name::ReadName(*message, offset, kHeaderOffset, name, sizeof(name)), "Name::ReadName() failed");
    printf("Read name =\"%s\"\n", name);
    VerifyOrQuit(strcmp(name, kExpectedReadName1) == 0, "Name::ReadName() did not return expected name");
    VerifyOrQuit(offset == name1Offset + sizeof(kEncodedName), "Name::ReadName() returned incorrect offset");

    printf("----------------------------------------------------------------\n");
    printf("Read and parse compressed name-2 \"FOO.F.ISI.ARPA\"\n");

    SuccessOrQuit(message->Read(name2Offset, buffer, kName2EncodedSize), "Message::Read() failed");
    DumpBuffer("name2(compressed)", buffer, kName2EncodedSize);

    offset = name2Offset;
    SuccessOrQuit(Dns::Name::ParseName(*message, offset), "Name::ParseName() failed");
    VerifyOrQuit(offset == name2Offset + kName2EncodedSize, "Name::ParseName() returned incorrect offset");

    offset = name2Offset;

    for (const char *nameLabel : kName2Labels)
    {
        labelLength = sizeof(label);
        SuccessOrQuit(Dns::Name::ReadLabel(*message, offset, kHeaderOffset, label, labelLength),
                      "Name::ReadLabel() failed");

        printf("label: \"%s\"\n", label);
        VerifyOrQuit(strcmp(label, nameLabel) == 0, "Name::ReadLabel() did not get expected label");
        VerifyOrQuit(labelLength == strlen(label), "Name::ReadLabel() returned incorrect label length");
    }

    labelLength = sizeof(label);
    VerifyOrQuit(Dns::Name::ReadLabel(*message, offset, kHeaderOffset, label, labelLength) == OT_ERROR_NOT_FOUND,
                 "Name::ReadLabel() failed at end of the name");

    offset = name2Offset;
    SuccessOrQuit(Dns::Name::ReadName(*message, offset, kHeaderOffset, name, sizeof(name)), "Name::ReadName() failed");
    printf("Read name =\"%s\"\n", name);
    VerifyOrQuit(strcmp(name, kExpectedReadName2) == 0, "Name::ReadName() did not return expected name");
    VerifyOrQuit(offset == name2Offset + kName2EncodedSize, "Name::ReadName() returned incorrect offset");

    printf("----------------------------------------------------------------\n");
    printf("Read and parse compressed name-3 \"ISI.ARPA\"\n");

    SuccessOrQuit(message->Read(name3Offset, buffer, kName3EncodedSize), "Message::Read() failed");
    DumpBuffer("name2(compressed)", buffer, kName3EncodedSize);

    offset = name3Offset;
    SuccessOrQuit(Dns::Name::ParseName(*message, offset), "Name::ParseName() failed");
    VerifyOrQuit(offset == name3Offset + kName3EncodedSize, "Name::ParseName() returned incorrect offset");

    offset = name3Offset;

    for (const char *nameLabel : kName3Labels)
    {
        labelLength = sizeof(label);
        SuccessOrQuit(Dns::Name::ReadLabel(*message, offset, kHeaderOffset, label, labelLength),
                      "Name::ReadLabel() failed");

        printf("label: \"%s\"\n", label);
        VerifyOrQuit(strcmp(label, nameLabel) == 0, "Name::ReadLabel() did not get expected label");
        VerifyOrQuit(labelLength == strlen(label), "Name::ReadLabel() returned incorrect label length");
    }

    labelLength = sizeof(label);
    VerifyOrQuit(Dns::Name::ReadLabel(*message, offset, kHeaderOffset, label, labelLength) == OT_ERROR_NOT_FOUND,
                 "Name::ReadLabel() failed at end of the name");

    offset = name3Offset;
    SuccessOrQuit(Dns::Name::ReadName(*message, offset, kHeaderOffset, name, sizeof(name)), "Name::ReadName() failed");
    printf("Read name =\"%s\"\n", name);
    VerifyOrQuit(strcmp(name, kExpectedReadName3) == 0, "Name::ReadName() did not return expected name");
    VerifyOrQuit(offset == name3Offset + kName3EncodedSize, "Name::ReadName() returned incorrect offset");

    printf("----------------------------------------------------------------\n");
    printf("Read and parse the uncompressed name-4 \"Human\\.Readable.F.ISI.ARPA\"\n");

    SuccessOrQuit(message->Read(name4Offset, buffer, kName4EncodedSize), "Message::Read() failed");
    DumpBuffer("name4(compressed)", buffer, kName4EncodedSize);

    offset = name4Offset;
    SuccessOrQuit(Dns::Name::ParseName(*message, offset), "Name::ParseName() failed");
    VerifyOrQuit(offset == name4Offset + kName4EncodedSize, "Name::ParseName() returned incorrect offset");

    offset = name4Offset;

    for (const char *nameLabel : kName4Labels)
    {
        labelLength = sizeof(label);
        SuccessOrQuit(Dns::Name::ReadLabel(*message, offset, kHeaderOffset, label, labelLength),
                      "Name::ReadLabel() failed");

        printf("label: \"%s\"\n", label);
        VerifyOrQuit(strcmp(label, nameLabel) == 0, "Name::ReadLabel() did not get expected label");
        VerifyOrQuit(labelLength == strlen(label), "Name::ReadLabel() returned incorrect label length");
    }

    // `ReadName()` for name-4 should fails due to first label containing dot char.
    offset = name4Offset;
    VerifyOrQuit(Dns::Name::ReadName(*message, offset, kHeaderOffset, name, sizeof(name)) == OT_ERROR_PARSE,
                 "Name::ReadName() did not fail with invalid label");

    message->Free();
    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestDnsName();
    ot::TestDnsCompressedName();

    printf("All tests passed\n");
    return 0;
}
