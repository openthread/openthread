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
        kMaxSize       = 300,
        kMaxNameLength = Dns::Name::kMaxNameSize - 1,
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
    char         label[Dns::Name::kMaxLabelSize];
    uint8_t      labelLength;
    char         name[Dns::Name::kMaxNameSize];

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

    static const char *kMaxLengthNames[] = {
        "HereIsSomeoneHidden.MyHoldFromMeTaken.FromSelfHasMeDriven.MyLeadFromMeTaken."
        "HereIsSomeoneHidden.AsLifeSweeterThanLife.TakesMeToGardenOfSoul.MyFortFromMeTaken."
        "HereIsSomeoneHidden.LikeSugarInSugarCane.ASweetSugarTrader.MyShopFromMeTaken."
        "SorcererAndMagicia.",

        "HereIsSomeoneHidden.MyHoldFromMeTaken.FromSelfHasMeDriven.MyLeadFromMeTaken."
        "HereIsSomeoneHidden.AsLifeSweeterThanLife.TakesMeToGardenOfSoul.MyFortFromMeTaken."
        "HereIsSomeoneHidden.LikeSugarInSugarCane.ASweetSugarTrader.MyShopFromMeTaken."
        "SorcererAndMagicia",
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

        // Long name of 255 characters which ends with a dot
        "HereIsSomeoneHidden.MyHoldFromMeTaken.FromSelfHasMeDriven.MyLeadFromMeTaken."
        "HereIsSomeoneHidden.AsLifeSweeterThanLife.TakesMeToGardenOfSoul.MyFortFromMeTaken."
        "HereIsSomeoneHidden.LikeSugarInSugarCane.ASweetSugarTrader.MyShopFromMeTaken."
        "SorcererAndMagician.",

        // Long name of 254 characters which does not end with a dot
        "HereIsSomeoneHidden.MyHoldFromMeTaken.FromSelfHasMeDriven.MyLeadFromMeTaken."
        "HereIsSomeoneHidden.AsLifeSweeterThanLife.TakesMeToGardenOfSoul.MyFortFromMeTaken."
        "HereIsSomeoneHidden.LikeSugarInSugarCane.ASweetSugarTrader.MyShopFromMeTaken."
        "SorcererAndMagician",

    };

    static const char kBadLabel[] = "badlabel";
    static const char kBadName[]  = "bad.name";

    printf("================================================================\n");
    printf("TestDnsName()\n");

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr, "Null OpenThread instance");

    messagePool = &instance->Get<MessagePool>();
    VerifyOrQuit((message = messagePool->New(Message::kTypeIp6, 0)) != nullptr, "Message::New failed");

    message->SetOffset(0);

    printf("----------------------------------------------------------------\n");
    printf("Verify domain name match:\n");

    {
        const char *subDomain;
        const char *domain;

        subDomain = "my-service._ipps._tcp.local.";
        domain    = "local.";
        VerifyOrQuit(Dns::Name::IsSubDomainOf(subDomain, domain), "Name::IsSubDomainOf() failed");

        subDomain = "my-service._ipps._tcp.local";
        domain    = "local.";
        VerifyOrQuit(Dns::Name::IsSubDomainOf(subDomain, domain), "Name::IsSubDomainOf() failed");

        subDomain = "my-service._ipps._tcp.local.";
        domain    = "local";
        VerifyOrQuit(Dns::Name::IsSubDomainOf(subDomain, domain), "Name::IsSubDomainOf() failed");

        subDomain = "my-service._ipps._tcp.local";
        domain    = "local";
        VerifyOrQuit(Dns::Name::IsSubDomainOf(subDomain, domain), "Name::IsSubDomainOf() failed");

        subDomain = "my-service._ipps._tcp.default.service.arpa.";
        domain    = "default.service.arpa.";
        VerifyOrQuit(Dns::Name::IsSubDomainOf(subDomain, domain), "Name::IsSubDomainOf() failed");

        subDomain = "my-service._ipps._tcp.default.service.arpa.";
        domain    = "service.arpa.";
        VerifyOrQuit(Dns::Name::IsSubDomainOf(subDomain, domain), "Name::IsSubDomainOf() failed");

        // Verify it doesn't match a portion of a label.
        subDomain = "my-service._ipps._tcp.default.service.arpa.";
        domain    = "vice.arpa.";
        VerifyOrQuit(!Dns::Name::IsSubDomainOf(subDomain, domain), "Name::IsSubDomainOf() succeed");
    }

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
            SuccessOrQuit(Dns::Name::ReadLabel(*message, offset, label, labelLength), "Name::ReadLabel() failed");

            printf("Label[%d] = \"%s\"\n", index, label);

            VerifyOrQuit(strcmp(label, test.mLabels[index]) == 0, "Name::ReadLabel() did not get expected label");
            VerifyOrQuit(labelLength == strlen(label), "Name::ReadLabel() returned incorrect label length");
        }

        labelLength = sizeof(label);
        VerifyOrQuit(Dns::Name::ReadLabel(*message, offset, label, labelLength) == OT_ERROR_NOT_FOUND,
                     "Name::ReadLabel() failed at end of the name");

        // Read entire name
        offset = 0;
        SuccessOrQuit(Dns::Name::ReadName(*message, offset, name, sizeof(name)), "Name::ReadName() failed");

        printf("Read name =\"%s\"\n", name);

        VerifyOrQuit(strcmp(name, test.mExpectedReadName) == 0, "Name::ReadName() did not get expected name");
        VerifyOrQuit(offset == len, "Name::ReadName() returned incorrect offset");

        // Read entire name with different name buffer sizes (just right and one byte off the expected size)
        offset = 0;
        SuccessOrQuit(
            Dns::Name::ReadName(*message, offset, name, static_cast<uint16_t>(strlen(test.mExpectedReadName) + 1)),
            "Name::ReadName() failed with exact name buffer size");
        offset = 0;
        VerifyOrQuit(Dns::Name::ReadName(*message, offset, name,
                                         static_cast<uint16_t>(strlen(test.mExpectedReadName))) == OT_ERROR_NO_BUFS,
                     "Name::ReadName() did not fail with too small name buffer size");

        // Compare labels one by one.
        offset = 0;

        for (uint8_t index = 0; test.mLabels[index] != nullptr; index++)
        {
            uint16_t startOffset = offset;

            SuccessOrQuit(Dns::Name::CompareLabel(*message, offset, test.mLabels[index]),
                          "Name::CompareLabel() failed");
            VerifyOrQuit(offset != startOffset, "Name::CompareLabel() did not change offset");

            VerifyOrQuit(Dns::Name::CompareLabel(*message, startOffset, kBadLabel) == OT_ERROR_NOT_FOUND,
                         "Name::CompareLabel() did not fail with incorrect label");
        }

        // Compare the whole name.
        offset = 0;
        SuccessOrQuit(Dns::Name::CompareName(*message, offset, test.mExpectedReadName), "Name::CompareName() failed");
        VerifyOrQuit(offset == len, "Name::CompareName() returned incorrect offset");

        offset = 0;
        VerifyOrQuit(Dns::Name::CompareName(*message, offset, kBadName) == OT_ERROR_NOT_FOUND,
                     "Name::CompareName() did not fail with incorrect name");
        VerifyOrQuit(offset == len, "Name::CompareName() returned incorrect offset");

        // Remove the terminating '.' in expected name and verify
        // that it can still be used by `CompareName()`.
        offset = 0;
        strcpy(name, test.mExpectedReadName);
        name[strlen(name) - 1] = '\0';
        SuccessOrQuit(Dns::Name::CompareName(*message, offset, name), "Name::CompareName() failed with root");
        VerifyOrQuit(offset == len, "Name::CompareName() returned incorrect offset");

        if (strlen(name) >= 1)
        {
            name[strlen(name) - 1] = '\0';
            offset                 = 0;
            VerifyOrQuit(Dns::Name::CompareName(*message, offset, name) == OT_ERROR_NOT_FOUND,
                         "Name::CompareName() did not fail with invalid name");
            VerifyOrQuit(offset == len, "Name::CompareName() returned incorrect offset");
        }

        // Compare the name with itself read from message.
        offset = 0;
        SuccessOrQuit(Dns::Name::CompareName(*message, offset, *message, offset),
                      "Name::CompareName() with itself failed");
        VerifyOrQuit(offset == len, "Name::CompareName() returned incorrect offset");
    }

    printf("----------------------------------------------------------------\n");
    printf("Max length names:\n");

    for (const char *&maxLengthName : kMaxLengthNames)
    {
        if (maxLengthName[strlen(maxLengthName) - 1] == '.')
        {
            VerifyOrQuit(strlen(maxLengthName) == kMaxNameLength, "invalid max length string");
        }
        else
        {
            VerifyOrQuit(strlen(maxLengthName) == kMaxNameLength - 1, "invalid max length string");
        }

        IgnoreError(message->SetLength(0));

        printf("\"%s\"\n", maxLengthName);

        VerifyOrQuit(Dns::Name::AppendName(maxLengthName, *message) == OT_ERROR_NONE,
                     "Name::AppendName() failed with max length name");
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

    static const char kBadName[] = "bad.name";

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

    message->SetOffset(kHeaderOffset);

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
        SuccessOrQuit(Dns::Name::ReadLabel(*message, offset, label, labelLength), "Name::ReadLabel() failed");

        printf("label: \"%s\"\n", label);
        VerifyOrQuit(strcmp(label, nameLabel) == 0, "Name::ReadLabel() did not get expected label");
        VerifyOrQuit(labelLength == strlen(label), "Name::ReadLabel() returned incorrect label length");
    }

    labelLength = sizeof(label);
    VerifyOrQuit(Dns::Name::ReadLabel(*message, offset, label, labelLength) == OT_ERROR_NOT_FOUND,
                 "Name::ReadLabel() failed at end of the name");

    offset = name1Offset;
    SuccessOrQuit(Dns::Name::ReadName(*message, offset, name, sizeof(name)), "Name::ReadName() failed");
    printf("Read name =\"%s\"\n", name);
    VerifyOrQuit(strcmp(name, kExpectedReadName1) == 0, "Name::ReadName() did not return expected name");
    VerifyOrQuit(offset == name1Offset + sizeof(kEncodedName), "Name::ReadName() returned incorrect offset");

    offset = name1Offset;

    for (const char *nameLabel : kName1Labels)
    {
        SuccessOrQuit(Dns::Name::CompareLabel(*message, offset, nameLabel), "Name::ComapreLabel() failed");
    }

    offset = name1Offset;
    SuccessOrQuit(Dns::Name::CompareName(*message, offset, kExpectedReadName1), "Name::CompareName() failed");
    VerifyOrQuit(offset == name1Offset + sizeof(kEncodedName), "Name::CompareName() returned incorrect offset");

    offset = name1Offset;
    VerifyOrQuit(Dns::Name::CompareName(*message, offset, kBadName) == OT_ERROR_NOT_FOUND,
                 "Name::CompareName() did not fail with incorrect name");
    VerifyOrQuit(offset == name1Offset + sizeof(kEncodedName), "Name::CompareName() returned incorrect offset");

    offset = name1Offset;
    SuccessOrQuit(Dns::Name::CompareName(*message, offset, *message, offset), "Name::CompareName() with itself failed");
    VerifyOrQuit(offset == name1Offset + sizeof(kEncodedName), "Name::CompareName() returned incorrect offset");

    offset = name1Offset;
    VerifyOrQuit(Dns::Name::CompareName(*message, offset, *message, name2Offset) == OT_ERROR_NOT_FOUND,
                 "Name::CompareName() did not fail with mismatching name");
    VerifyOrQuit(offset == name1Offset + sizeof(kEncodedName), "Name::CompareName() returned incorrect offset");

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
        SuccessOrQuit(Dns::Name::ReadLabel(*message, offset, label, labelLength), "Name::ReadLabel() failed");

        printf("label: \"%s\"\n", label);
        VerifyOrQuit(strcmp(label, nameLabel) == 0, "Name::ReadLabel() did not get expected label");
        VerifyOrQuit(labelLength == strlen(label), "Name::ReadLabel() returned incorrect label length");
    }

    labelLength = sizeof(label);
    VerifyOrQuit(Dns::Name::ReadLabel(*message, offset, label, labelLength) == OT_ERROR_NOT_FOUND,
                 "Name::ReadLabel() failed at end of the name");

    offset = name2Offset;
    SuccessOrQuit(Dns::Name::ReadName(*message, offset, name, sizeof(name)), "Name::ReadName() failed");
    printf("Read name =\"%s\"\n", name);
    VerifyOrQuit(strcmp(name, kExpectedReadName2) == 0, "Name::ReadName() did not return expected name");
    VerifyOrQuit(offset == name2Offset + kName2EncodedSize, "Name::ReadName() returned incorrect offset");

    offset = name2Offset;

    for (const char *nameLabel : kName2Labels)
    {
        SuccessOrQuit(Dns::Name::CompareLabel(*message, offset, nameLabel), "Name::ComapreLabel() failed");
    }

    offset = name2Offset;
    SuccessOrQuit(Dns::Name::CompareName(*message, offset, kExpectedReadName2), "Name::CompareName() failed");
    VerifyOrQuit(offset == name2Offset + kName2EncodedSize, "Name::CompareName() returned incorrect offset");

    offset = name2Offset;
    VerifyOrQuit(Dns::Name::CompareName(*message, offset, kBadName) == OT_ERROR_NOT_FOUND,
                 "Name::CompareName() did not fail with incorrect name");
    VerifyOrQuit(offset == name2Offset + kName2EncodedSize, "Name::CompareName() returned incorrect offset");

    offset = name2Offset;
    SuccessOrQuit(Dns::Name::CompareName(*message, offset, *message, offset), "Name::CompareName() with itself failed");
    VerifyOrQuit(offset == name2Offset + kName2EncodedSize, "Name::CompareName() returned incorrect offset");

    offset = name2Offset;
    VerifyOrQuit(Dns::Name::CompareName(*message, offset, *message, name3Offset) == OT_ERROR_NOT_FOUND,
                 "Name::CompareName() did not fail with mismatching name");
    VerifyOrQuit(offset == name2Offset + kName2EncodedSize, "Name::CompareName() returned incorrect offset");

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
        SuccessOrQuit(Dns::Name::ReadLabel(*message, offset, label, labelLength), "Name::ReadLabel() failed");

        printf("label: \"%s\"\n", label);
        VerifyOrQuit(strcmp(label, nameLabel) == 0, "Name::ReadLabel() did not get expected label");
        VerifyOrQuit(labelLength == strlen(label), "Name::ReadLabel() returned incorrect label length");
    }

    labelLength = sizeof(label);
    VerifyOrQuit(Dns::Name::ReadLabel(*message, offset, label, labelLength) == OT_ERROR_NOT_FOUND,
                 "Name::ReadLabel() failed at end of the name");

    offset = name3Offset;
    SuccessOrQuit(Dns::Name::ReadName(*message, offset, name, sizeof(name)), "Name::ReadName() failed");
    printf("Read name =\"%s\"\n", name);
    VerifyOrQuit(strcmp(name, kExpectedReadName3) == 0, "Name::ReadName() did not return expected name");
    VerifyOrQuit(offset == name3Offset + kName3EncodedSize, "Name::ReadName() returned incorrect offset");

    offset = name3Offset;

    for (const char *nameLabel : kName3Labels)
    {
        SuccessOrQuit(Dns::Name::CompareLabel(*message, offset, nameLabel), "Name::ComapreLabel() failed");
    }

    offset = name3Offset;
    SuccessOrQuit(Dns::Name::CompareName(*message, offset, kExpectedReadName3), "Name::CompareName() failed");
    VerifyOrQuit(offset == name3Offset + kName3EncodedSize, "Name::CompareName() returned incorrect offset");

    offset = name3Offset;
    VerifyOrQuit(Dns::Name::CompareName(*message, offset, kBadName) == OT_ERROR_NOT_FOUND,
                 "Name::CompareName() did not fail with incorrect name");
    VerifyOrQuit(offset == name3Offset + kName3EncodedSize, "Name::CompareName() returned incorrect offset");

    offset = name3Offset;
    SuccessOrQuit(Dns::Name::CompareName(*message, offset, *message, offset), "Name::CompareName() with itself failed");
    VerifyOrQuit(offset == name3Offset + kName3EncodedSize, "Name::CompareName() returned incorrect offset");

    offset = name3Offset;
    VerifyOrQuit(Dns::Name::CompareName(*message, offset, *message, name4Offset) == OT_ERROR_NOT_FOUND,
                 "Name::CompareName() did not fail with mismatching name");
    VerifyOrQuit(offset == name3Offset + kName3EncodedSize, "Name::CompareName() returned incorrect offset");

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
        SuccessOrQuit(Dns::Name::ReadLabel(*message, offset, label, labelLength), "Name::ReadLabel() failed");

        printf("label: \"%s\"\n", label);
        VerifyOrQuit(strcmp(label, nameLabel) == 0, "Name::ReadLabel() did not get expected label");
        VerifyOrQuit(labelLength == strlen(label), "Name::ReadLabel() returned incorrect label length");
    }

    // `ReadName()` for name-4 should fails due to first label containing dot char.
    offset = name4Offset;
    VerifyOrQuit(Dns::Name::ReadName(*message, offset, name, sizeof(name)) == OT_ERROR_PARSE,
                 "Name::ReadName() did not fail with invalid label");

    offset = name4Offset;

    for (const char *nameLabel : kName4Labels)
    {
        SuccessOrQuit(Dns::Name::CompareLabel(*message, offset, nameLabel), "Name::ComapreLabel() failed");
    }

    offset = name4Offset;
    SuccessOrQuit(Dns::Name::CompareName(*message, offset, *message, offset), "Name::CompareName() with itself failed");

    offset = name4Offset;
    VerifyOrQuit(Dns::Name::CompareName(*message, offset, *message, name1Offset) == OT_ERROR_NOT_FOUND,
                 "Name::CompareName() did not fail with mismatching name");

    message->Free();
    testFreeInstance(instance);
}

void TestHeaderAndResourceRecords(void)
{
    enum
    {
        kHeaderOffset    = 0,
        kQuestionCount   = 1,
        kAnswerCount     = 2,
        kAdditionalCount = 5,
        kTtl             = 7200,
        kTxtTtl          = 7300,
        kSrvPort         = 1234,
        kSrvPriority     = 1,
        kSrvWeight       = 2,
        kMaxSize         = 600,
    };

    const char    kMessageString[]  = "DnsMessage";
    const char    kDomainName[]     = "example.com.";
    const char    kServiceLabels[]  = "_service._udp";
    const char    kServiceName[]    = "_service._udp.example.com.";
    const char    kInstance1Label[] = "inst1";
    const char    kInstance2Label[] = "instance2";
    const char    kInstance1Name[]  = "inst1._service._udp.example.com.";
    const char    kInstance2Name[]  = "instance2._service._udp.example.com.";
    const char    kHostName[]       = "host.example.com.";
    const uint8_t kTxtData[]        = {9, 'k', 'e', 'y', '=', 'v', 'a', 'l', 'u', 'e', 0};
    const char    kHostAddress[]    = "fd00::abcd:";

    const char *kInstanceLabels[] = {kInstance1Label, kInstance2Label};
    const char *kInstanceNames[]  = {kInstance1Name, kInstance2Name};

    Instance *          instance;
    MessagePool *       messagePool;
    Message *           message;
    Dns::Header         header;
    uint16_t            messageId;
    uint16_t            headerOffset;
    uint16_t            offset;
    uint16_t            numRecords;
    uint16_t            len;
    uint16_t            serviceNameOffset;
    uint16_t            hostNameOffset;
    uint16_t            answerSectionOffset;
    uint16_t            additionalSectionOffset;
    uint16_t            index;
    Dns::PtrRecord      ptrRecord;
    Dns::SrvRecord      srvRecord;
    Dns::TxtRecord      txtRecord;
    Dns::AaaaRecord     aaaaRecord;
    Dns::ResourceRecord record;
    Ip6::Address        hostAddress;

    char    label[Dns::Name::kMaxLabelSize];
    char    name[Dns::Name::kMaxNameSize];
    uint8_t buffer[kMaxSize];

    printf("================================================================\n");
    printf("TestHeaderAndResourceRecords()\n");

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr, "Null OpenThread instance");

    messagePool = &instance->Get<MessagePool>();
    VerifyOrQuit((message = messagePool->New(Message::kTypeIp6, 0)) != nullptr, "Message::New failed");

    printf("----------------------------------------------------------------\n");
    printf("Preparing the message\n");

    SuccessOrQuit(message->Append(kMessageString), "Message::Append() failed");

    // Header

    headerOffset = message->GetLength();
    SuccessOrQuit(header.SetRandomMessageId(), "Header::SetRandomMessageId() failed");
    messageId = header.GetMessageId();
    header.SetType(Dns::Header::kTypeResponse);
    header.SetQuestionCount(kQuestionCount);
    header.SetAnswerCount(kAnswerCount);
    header.SetAdditionalRecordCount(kAdditionalCount);
    SuccessOrQuit(message->Append(header), "Message::Append() failed");
    message->SetOffset(headerOffset);

    // Question section

    serviceNameOffset = message->GetLength() - headerOffset;
    SuccessOrQuit(Dns::Name::AppendMultipleLabels(kServiceLabels, *message), "AppendMultipleLabels() failed");
    SuccessOrQuit(Dns::Name::AppendName(kDomainName, *message), "AppendName() failed");
    SuccessOrQuit(message->Append(Dns::Question(Dns::ResourceRecord::kTypePtr)), "Message::Append() failed");

    // Answer section

    answerSectionOffset = message->GetLength();

    for (const char *instanceLabel : kInstanceLabels)
    {
        SuccessOrQuit(Dns::Name::AppendPointerLabel(serviceNameOffset, *message), "AppendPointerLabel() failed");
        ptrRecord.Init();
        ptrRecord.SetTtl(kTtl);
        offset = message->GetLength();
        SuccessOrQuit(message->Append(ptrRecord), "Message::Append() failed");
        SuccessOrQuit(Dns::Name::AppendLabel(instanceLabel, *message), "AppendLabel failed");
        SuccessOrQuit(Dns::Name::AppendPointerLabel(serviceNameOffset, *message), "AppendPointerLabel() failed");
        ptrRecord.SetLength(message->GetLength() - offset - sizeof(Dns::ResourceRecord));
        message->Write(offset, ptrRecord);
    }

    // Additional section

    additionalSectionOffset = message->GetLength();

    for (const char *instanceName : kInstanceNames)
    {
        uint16_t instanceNameOffset = message->GetLength() - headerOffset;

        // SRV record
        SuccessOrQuit(Dns::Name::AppendName(instanceName, *message), "AppendName() failed");
        srvRecord.Init();
        srvRecord.SetTtl(kTtl);
        srvRecord.SetPort(kSrvPort);
        srvRecord.SetWeight(kSrvWeight);
        srvRecord.SetPriority(kSrvPriority);
        offset = message->GetLength();
        SuccessOrQuit(message->Append(srvRecord), "Message::Append() failed");
        hostNameOffset = message->GetLength() - headerOffset;
        SuccessOrQuit(Dns::Name::AppendName(kHostName, *message), "AppendName() failed");
        srvRecord.SetLength(message->GetLength() - offset - sizeof(Dns::ResourceRecord));
        message->Write(offset, srvRecord);

        // TXT record
        SuccessOrQuit(Dns::Name::AppendPointerLabel(instanceNameOffset, *message), "AppendPointerLabel() failed");
        txtRecord.Init();
        txtRecord.SetTtl(kTxtTtl);
        txtRecord.SetLength(sizeof(kTxtData));
        SuccessOrQuit(message->Append(txtRecord), "Message::Append() failed");
        SuccessOrQuit(message->Append(kTxtData), "Message::Append() failed");
    }

    SuccessOrQuit(hostAddress.FromString(kHostAddress), "Address::FromString() failed");
    SuccessOrQuit(Dns::Name::AppendPointerLabel(hostNameOffset, *message), "AppendPointerLabel() failed");
    aaaaRecord.Init();
    aaaaRecord.SetTtl(kTtl);
    aaaaRecord.SetAddress(hostAddress);
    SuccessOrQuit(message->Append(aaaaRecord), "Message::Append()");

    // Dump the entire message

    VerifyOrQuit(message->GetLength() < kMaxSize, "Message is too long");
    SuccessOrQuit(message->Read(0, buffer, message->GetLength()), "Message::Read() failed");
    DumpBuffer("message", buffer, message->GetLength());

    printf("----------------------------------------------------------------\n");
    printf("Parse and verify the message\n");

    offset = 0;
    VerifyOrQuit(message->Compare(offset, kMessageString), "Message header does not match");
    offset += sizeof(kMessageString);

    // Header

    VerifyOrQuit(offset == headerOffset, "headerOffset is incorrect");
    SuccessOrQuit(message->Read(offset, header), "Message::Read() failed");
    offset += sizeof(header);

    VerifyOrQuit(header.GetMessageId() == messageId, "Header::GetMessageId() failed");
    VerifyOrQuit(header.GetType() == Dns::Header::kTypeResponse, "Header::GetType() failed");
    VerifyOrQuit(header.GetQuestionCount() == kQuestionCount, "Header::GetQuestionCount() failed");
    VerifyOrQuit(header.GetAnswerCount() == kAnswerCount, "Header::GetAnswerCount() failed");
    VerifyOrQuit(header.GetAdditionalRecordCount() == kAdditionalCount, "Header::GetAdditionalRecordCount() failed");

    printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    printf("Question Section\n");

    SuccessOrQuit(Dns::Name::CompareName(*message, offset, kServiceName), "Question name does not match");
    VerifyOrQuit(message->Compare(offset, Dns::Question(Dns::ResourceRecord::kTypePtr)), "Question does not match");
    offset += sizeof(Dns::Question);

    printf("PTR for \"%s\"\n", kServiceName);

    printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    printf("Answer Section\n");

    VerifyOrQuit(offset == answerSectionOffset, "answer section offset is incorrect");

    for (const char *instanceName : kInstanceNames)
    {
        SuccessOrQuit(Dns::Name::CompareName(*message, offset, kServiceName), "ServiceName is incorrect");
        SuccessOrQuit(Dns::ResourceRecord::ReadRecord(*message, offset, ptrRecord), "ReadRecord() failed");
        VerifyOrQuit(ptrRecord.GetTtl() == kTtl, "Read PTR is incorrect");

        SuccessOrQuit(ptrRecord.ReadPtrName(*message, offset, name, sizeof(name)), "ReadName() failed");
        VerifyOrQuit(strcmp(name, instanceName) == 0, "Inst1 name is incorrect");

        printf("    \"%s\" PTR %u %d \"%s\"\n", kServiceName, ptrRecord.GetTtl(), ptrRecord.GetLength(), name);
    }

    VerifyOrQuit(offset == additionalSectionOffset, "offset is incorrect after answer section parse");

    offset = answerSectionOffset;
    SuccessOrQuit(Dns::ResourceRecord::ParseRecords(*message, offset, kAnswerCount), "ParseRecords() failed");
    VerifyOrQuit(offset == additionalSectionOffset, "offset is incorrect after answer section parse");

    printf("Use FindRecord() to find and iterate through all the records:\n");

    offset     = answerSectionOffset;
    numRecords = kAnswerCount;

    while (numRecords > 0)
    {
        uint16_t prevNumRecords = numRecords;

        SuccessOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, numRecords, Dns::Name(kServiceName)),
                      "FindRecord failed");
        VerifyOrQuit(numRecords == prevNumRecords - 1, "Incorrect num records");
        SuccessOrQuit(Dns::ResourceRecord::ReadRecord(*message, offset, ptrRecord), "ReadRecord() failed");
        VerifyOrQuit(ptrRecord.GetTtl() == kTtl, "Read PTR is incorrect");
        SuccessOrQuit(ptrRecord.ReadPtrName(*message, offset, label, sizeof(label), name, sizeof(name)),
                      "ReadName() failed");
        printf("    \"%s\" PTR %u %d inst:\"%s\" at \"%s\"\n", kServiceName, ptrRecord.GetTtl(), ptrRecord.GetLength(),
               label, name);
    }

    VerifyOrQuit(offset == additionalSectionOffset, "offset is incorrect after answer section parse");
    VerifyOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, numRecords, Dns::Name(kServiceName)) ==
                     OT_ERROR_NOT_FOUND,
                 "FindRecord did not fail with no records");

    // Use `ReadRecord()` with a non-matching record type. Verify that it correct skips over the record.

    offset     = answerSectionOffset;
    numRecords = kAnswerCount;

    while (numRecords > 0)
    {
        SuccessOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, numRecords, Dns::Name(kServiceName)),
                      "FindRecord failed");
        VerifyOrQuit(Dns::ResourceRecord::ReadRecord(*message, offset, srvRecord) == OT_ERROR_NOT_FOUND,
                     "ReadRecord() did not fail with non-matching type");
    }

    VerifyOrQuit(offset == additionalSectionOffset, "offset is incorrect after answer section parse");

    // Use `FindRecord` with a non-matching name. Verify that it correctly skips over all records.

    offset     = answerSectionOffset;
    numRecords = kAnswerCount;
    VerifyOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, numRecords, Dns::Name(kInstance1Name)) ==
                     OT_ERROR_NOT_FOUND,
                 "FindRecord did not fail with non-matching name");
    VerifyOrQuit(numRecords == 0, "Incorrect num records");
    VerifyOrQuit(offset == additionalSectionOffset, "offset is incorrect after answer section parse");

    printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    printf("Additional Section\n");

    for (const char *instanceName : kInstanceNames)
    {
        // SRV record
        SuccessOrQuit(Dns::Name::CompareName(*message, offset, instanceName), "Instance is incorrect");
        SuccessOrQuit(Dns::ResourceRecord::ReadRecord(*message, offset, srvRecord), "ReadRecord() failed");
        VerifyOrQuit(srvRecord.GetTtl() == kTtl, "Read SRV is incorrect");
        VerifyOrQuit(srvRecord.GetPort() == kSrvPort, "Read SRV port is incorrect");
        VerifyOrQuit(srvRecord.GetWeight() == kSrvWeight, "Read SRV weight is incorrect");
        VerifyOrQuit(srvRecord.GetPriority() == kSrvPriority, "Read SRV priority is incorrect");
        SuccessOrQuit(srvRecord.ReadTargetHostName(*message, offset, name, sizeof(name)),
                      "ReadTargetHostName() failed");
        VerifyOrQuit(strcmp(name, kHostName) == 0, "Inst1 name is incorrect");
        printf("    \"%s\" SRV %u %d %d %d %d \"%s\"\n", instanceName, srvRecord.GetTtl(), srvRecord.GetLength(),
               srvRecord.GetPort(), srvRecord.GetWeight(), srvRecord.GetPriority(), name);

        // TXT record
        SuccessOrQuit(Dns::Name::CompareName(*message, offset, instanceName), "Instance is incorrect");
        SuccessOrQuit(Dns::ResourceRecord::ReadRecord(*message, offset, txtRecord), "ReadRecord() failed");
        VerifyOrQuit(txtRecord.GetTtl() == kTxtTtl, "Read TXT is incorrect");
        len = sizeof(buffer);
        SuccessOrQuit(txtRecord.ReadTxtData(*message, offset, buffer, len), "ReadTxtData() failed");
        VerifyOrQuit(len == sizeof(kTxtData), "TXT data length is not valid");
        VerifyOrQuit(memcmp(buffer, kTxtData, len) == 0, "TXT data is not valid");
        printf("    \"%s\" TXT %u %d \"%s\"\n", instanceName, txtRecord.GetTtl(), txtRecord.GetLength(),
               reinterpret_cast<const char *>(buffer));
    }

    SuccessOrQuit(Dns::Name::CompareName(*message, offset, kHostName), "HostName is incorrect");
    SuccessOrQuit(Dns::ResourceRecord::ReadRecord(*message, offset, aaaaRecord), "ReadRecord() failed");
    VerifyOrQuit(aaaaRecord.GetTtl() == kTtl, "Read AAAA is incorrect");
    VerifyOrQuit(aaaaRecord.GetAddress() == hostAddress, "Read host address is incorrect");
    printf("    \"%s\" AAAA %u %d \"%s\"\n", kHostName, aaaaRecord.GetTtl(), aaaaRecord.GetLength(),
           aaaaRecord.GetAddress().ToString().AsCString());

    VerifyOrQuit(offset == message->GetLength(), "offset is incorrect after additional section parse");

    // Use `ParseRecords()` to parse all records
    offset = additionalSectionOffset;
    SuccessOrQuit(Dns::ResourceRecord::ParseRecords(*message, offset, kAdditionalCount), "ParseRecords() failed");
    VerifyOrQuit(offset == message->GetLength(), "offset is incorrect after additional section parse");

    printf("Use FindRecord() to search for specific name:\n");

    for (const char *instanceName : kInstanceNames)
    {
        offset     = additionalSectionOffset;
        numRecords = kAdditionalCount;

        SuccessOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, numRecords, Dns::Name(instanceName)),
                      "FindRecord failed");
        SuccessOrQuit(Dns::ResourceRecord::ReadRecord(*message, offset, srvRecord), "ReadRecord() failed");
        SuccessOrQuit(Dns::Name::ParseName(*message, offset), "ParseName() failed");
        printf("    \"%s\" SRV %u %d %d %d %d\n", instanceName, srvRecord.GetTtl(), srvRecord.GetLength(),
               srvRecord.GetPort(), srvRecord.GetWeight(), srvRecord.GetPriority());

        SuccessOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, numRecords, Dns::Name(instanceName)),
                      "FindRecord failed");
        SuccessOrQuit(Dns::ResourceRecord::ReadRecord(*message, offset, txtRecord), "ReadRecord() failed");
        offset += txtRecord.GetLength();
        printf("    \"%s\" TXT %u %d\n", instanceName, txtRecord.GetTtl(), txtRecord.GetLength());

        VerifyOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, numRecords, Dns::Name(instanceName)) ==
                         OT_ERROR_NOT_FOUND,
                     "FindRecord() did not fail with no more records");

        VerifyOrQuit(offset == message->GetLength(), "offset is incorrect after additional section parse");
    }

    offset     = additionalSectionOffset;
    numRecords = kAdditionalCount;
    SuccessOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, numRecords, Dns::Name(kHostName)),
                  "FindRecord() failed");
    SuccessOrQuit(Dns::ResourceRecord::ReadRecord(*message, offset, record), "ReadRecord() failed");
    VerifyOrQuit(record.GetType() == Dns::ResourceRecord::kTypeAaaa, "Read record has incorrect type");
    offset += record.GetLength();
    VerifyOrQuit(offset == message->GetLength(), "offset is incorrect after additional section parse");

    printf("Use FindRecord() to search for specific records:\n");
    printf(" Answer Section\n");

    for (index = 0; index < OT_ARRAY_LENGTH(kInstanceNames); index++)
    {
        offset = answerSectionOffset;
        SuccessOrQuit(
            Dns::ResourceRecord::FindRecord(*message, offset, kAnswerCount, index, Dns::Name(kServiceName), ptrRecord),
            "FindRecord() failed");

        printf("   index:%d -> \"%s\" PTR %u %d\n", index, kServiceName, ptrRecord.GetTtl(), ptrRecord.GetLength());
    }

    // Check `FindRecord()` failure with non-matching name, record type, or bad index.

    offset = answerSectionOffset;
    VerifyOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, kAnswerCount, index, Dns::Name(kServiceName),
                                                 ptrRecord) == OT_ERROR_NOT_FOUND,
                 "FindRecord() did not fail with bad index");
    VerifyOrQuit(offset == answerSectionOffset, "FindRecord() changed offset on failure");

    offset = answerSectionOffset;
    VerifyOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, kAnswerCount, index, Dns::Name(kInstance1Name),
                                                 ptrRecord) == OT_ERROR_NOT_FOUND,
                 "FindRecord() did not fail with bad index");
    VerifyOrQuit(offset == answerSectionOffset, "FindRecord() changed offset on failure");

    offset = answerSectionOffset;
    VerifyOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, kAnswerCount, index, Dns::Name(kServiceName),
                                                 txtRecord) == OT_ERROR_NOT_FOUND,
                 "FindRecord() did not fail with bad index");
    VerifyOrQuit(offset == answerSectionOffset, "FindRecord() changed offset on failure");

    printf(" Additional Section\n");

    for (const char *instanceName : kInstanceNames)
    {
        // There is a single SRV and TXT entry for each instance
        offset = additionalSectionOffset;
        SuccessOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, kAdditionalCount, /* aIndex */ 0,
                                                      Dns::Name(instanceName), srvRecord),
                      "FindRecord() failed");
        printf("    \"%s\" SRV %u %d %d %d %d \n", instanceName, srvRecord.GetTtl(), srvRecord.GetLength(),
               srvRecord.GetPort(), srvRecord.GetWeight(), srvRecord.GetPriority());

        offset = additionalSectionOffset;
        SuccessOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, kAdditionalCount, /* aIndex */ 0,
                                                      Dns::Name(instanceName), txtRecord),
                      "FindRecord() failed");
        printf("    \"%s\" TXT %u %d\n", instanceName, txtRecord.GetTtl(), txtRecord.GetLength());

        offset = additionalSectionOffset;
        VerifyOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, kAdditionalCount, /* aIndex */ 1,
                                                     Dns::Name(instanceName), srvRecord) == OT_ERROR_NOT_FOUND,
                     "FindRecord() did not fail with bad index");

        offset = additionalSectionOffset;
        VerifyOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, kAdditionalCount, /* aIndex */ 1,
                                                     Dns::Name(instanceName), txtRecord) == OT_ERROR_NOT_FOUND,
                     "FindRecord() did not fail with bad index");
    }

    for (index = 0; index < kAdditionalCount; index++)
    {
        offset = additionalSectionOffset;
        // Find record with empty name (matching any) and any type.
        SuccessOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, kAdditionalCount, index, Dns::Name(), record),
                      "FindRecord() failed");
    }

    offset = additionalSectionOffset;
    VerifyOrQuit(Dns::ResourceRecord::FindRecord(*message, offset, kAdditionalCount, index, Dns::Name(), record) ==
                     OT_ERROR_NOT_FOUND,
                 "FindRecord() did not fail with bad index");

    message->Free();
    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestDnsName();
    ot::TestDnsCompressedName();
    ot::TestHeaderAndResourceRecords();

    printf("All tests passed\n");
    return 0;
}
