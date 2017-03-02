/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#include "test_util.h"
#include "openthread/openthread.h"
#include <thread/link_quality.hpp>

#include <string.h>

namespace Thread {

enum
{
    kMaxRssValue        = 0,
    kMinRssValue        = -128,

    kStringBuffferSize  = 80,

    kRssAverageMaxDiff  = 16,
    kNumRssAdds         = 300,

    kEncodedAverageBitShift = 3,
    kEncodedAverageMultiple = (1 << kEncodedAverageBitShift),
    kEncodedAverageBitMask  = (1 << kEncodedAverageBitShift) - 1,
};

#define MIN_RSS(_rss1, _rss2)   (((_rss1) < (_rss2)) ? (_rss1) : (_rss2))
#define MAX_RSS(_rss1, _rss2)   (((_rss1) < (_rss2)) ? (_rss2) : (_rss1))
#define ABS(value)              (((value) >= 0) ? (value) : -(value))

// This struct contains RSS values and test data for checking link quality info calss.
struct RssTestData
{
    const int8_t *mRssList;                 // Array of RSS values.
    size_t        mRssListSize;             // Size of RSS list.
    uint8_t       mExpectedLinkQuality;     // Expected final link quality value.
};

static LinkQualityInfo sNoiseFloor;

// Checks the encoded average RSS value to match the value from GetAverageRss().
void VerifyEncodedRssValue(LinkQualityInfo &aLinkInfo)
{
    int8_t   rss = aLinkInfo.GetAverageRss();
    uint16_t encodedRss = aLinkInfo.GetAverageRssAsEncodedWord();

    if (rss != LinkQualityInfo::kUnknownRss)
    {
        VerifyOrQuit(rss == -static_cast<int16_t>((encodedRss + (kEncodedAverageMultiple / 2)) >> kEncodedAverageBitShift),
                     "TestLinkQualityInfo failed - Ecoded RSS does not match the value from GetAverageRss().");
    }
    else
    {
        VerifyOrQuit(encodedRss == 0,
                     "TestLinkQualityInfo failed - Ecoded RSS does not match the value from GetAverageRss().");
    }
}

// This function prints the values in the passed in link info instance. It is invoked as the final step in test-case.
void PrintOutcome(LinkQualityInfo &aLinkInfo)
{
    char     stringBuf[kStringBuffferSize];

    SuccessOrQuit(aLinkInfo.GetAverageRssAsString(stringBuf, sizeof(stringBuf)),
                  "TestLinkQualityInfo failed - GetAverageRssAsString() failed.");

    printf("AveRss = %-4d, \"%-14s\", ", aLinkInfo.GetAverageRss(), stringBuf);
    printf("LinkMargin = %-4d, LinkQuality = %d", aLinkInfo.GetLinkMargin(sNoiseFloor),
           aLinkInfo.GetLinkQuality(sNoiseFloor));

    // This test-case succeeded.
    printf(" -> PASS\n");
}

void TestLinkQualityData(RssTestData anRssData)
{
    LinkQualityInfo linkInfo;
    int8_t rss, ave, min, max;
    size_t i;

    printf("- - - - - - - - - - - - - - - - - -\n");
    min = kMinRssValue;
    max = kMaxRssValue;

    for (i = 0; i < anRssData.mRssListSize; i++)
    {
        rss = anRssData.mRssList[i];
        min = MIN_RSS(rss, min);
        max = MAX_RSS(rss, max);
        linkInfo.AddRss(sNoiseFloor, rss);
        ave = linkInfo.GetAverageRss();
        VerifyOrQuit(ave >= min,
                     "TestLinkQualityInfo failed - GetAverageRss() is smaller than min value.");
        VerifyOrQuit(ave <= max,
                     "TestLinkQualityInfo failed - GetAverageRss() is larger than min value");
        VerifyEncodedRssValue(linkInfo);
        printf("%02u) AddRss(%4d): ", (unsigned int)i, rss);
        PrintOutcome(linkInfo);
    }

    VerifyOrQuit(linkInfo.GetLinkQuality(sNoiseFloor) == anRssData.mExpectedLinkQuality,
                 "TestLinkQualityInfo failed - GetLinkQuality() is incorrect");
}

void TestRssAveraging(void)
{
    LinkQualityInfo linkInfo;
    int8_t          rss, rss2, ave;
    int16_t         diff;
    size_t          i, j, k;
    const int8_t    rssValues[] = { kMinRssValue, -70, -40, -41, -10, kMaxRssValue};

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Values after initialization.

    printf("\nAfter Initialization: ");
    VerifyOrQuit(linkInfo.GetAverageRss() == LinkQualityInfo::kUnknownRss,
                 "TestLinkQualityInfo failed - Inital value from GetAverageRss() is incorrect.");
    VerifyOrQuit(linkInfo.GetLinkMargin(sNoiseFloor) == 0,
                 "TestLinkQualityInfo failed - Inital value for link margin is incorrect.");
    VerifyEncodedRssValue(linkInfo);
    PrintOutcome(linkInfo);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Adding a single value
    rss = -70;
    printf("AddRss(%d): ", rss);
    linkInfo.AddRss(sNoiseFloor, rss);
    VerifyOrQuit(linkInfo.GetAverageRss() == rss,
                 "TestLinkQualityInfo - GetAverageRss() failed after a single AddRss().");
    VerifyEncodedRssValue(linkInfo);
    PrintOutcome(linkInfo);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Clear

    printf("Clear(): ");
    linkInfo.Clear();
    VerifyOrQuit(linkInfo.GetAverageRss() == LinkQualityInfo::kUnknownRss,
                 "TestLinkQualityInfo failed - GetAverageRss() after Clear() is incorrect.");
    VerifyOrQuit(linkInfo.GetLinkMargin(sNoiseFloor) == 0,
                 "TestLinkQualityInfo failed - link margin value after Clear() is incorrect.");
    VerifyEncodedRssValue(linkInfo);
    PrintOutcome(linkInfo);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Adding the same value many times.

    printf("- - - - - - - - - - - - - - - - - -\n");

    for (j = 0; j < sizeof(rssValues); j++)
    {
        linkInfo.Clear();
        rss = rssValues[j];
        printf("AddRss(%4d) %d times: ", rss, kNumRssAdds);

        for (i = 0; i < kNumRssAdds; i++)
        {
            linkInfo.AddRss(sNoiseFloor, rss);
            VerifyOrQuit(linkInfo.GetAverageRss() == rss,
                         "TestLinkQualityInfo failed - GetAverageRss() returned incorrect value.");
            VerifyEncodedRssValue(linkInfo);
        }

        PrintOutcome(linkInfo);

    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Adding two RSS values:

    printf("- - - - - - - - - - - - - - - - - -\n");

    for (j = 0; j < sizeof(rssValues); j++)
    {
        rss = rssValues[j];

        for (k = 0; k < sizeof(rssValues); k++)
        {
            if (k == j)
            {
                continue;
            }

            rss2 = rssValues[k];
            linkInfo.Clear();
            linkInfo.AddRss(sNoiseFloor, rss);
            linkInfo.AddRss(sNoiseFloor, rss2);
            printf("AddRss(%4d), AddRss(%4d): ", rss, rss2);
            VerifyOrQuit(linkInfo.GetAverageRss() == ((rss + rss2) >> 1),
                         "TestLinkQualityInfo failed - GetAverageRss() returned incorrect value.");
            VerifyEncodedRssValue(linkInfo);
            PrintOutcome(linkInfo);
        }
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Adding one value many times and a different value once:

    printf("- - - - - - - - - - - - - - - - - -\n");

    for (j = 0; j < sizeof(rssValues); j++)
    {
        rss = rssValues[j];

        for (k = 0; k < sizeof(rssValues); k++)
        {
            if (k == j)
            {
                continue;
            }

            rss2 = rssValues[k];
            linkInfo.Clear();

            for (i = 0; i < kNumRssAdds; i++)
            {
                linkInfo.AddRss(sNoiseFloor, rss);
            }

            linkInfo.AddRss(sNoiseFloor, rss2);
            printf("AddRss(%4d) %d times, AddRss(%4d): ", rss, kNumRssAdds, rss2);
            ave = linkInfo.GetAverageRss();
            VerifyOrQuit(ave >= MIN_RSS(rss, rss2),
                         "TestLinkQualityInfo failed - GetAverageRss() returned incorrect value.");
            VerifyOrQuit(ave <= MAX_RSS(rss, rss2),
                         "TestLinkQualityInfo failed - GetAverageRss() returned incorrect value.");
            VerifyEncodedRssValue(linkInfo);
            PrintOutcome(linkInfo);
        }
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Adding two alteraing values many times:

    printf("- - - - - - - - - - - - - - - - - -\n");

    for (j = 0; j < sizeof(rssValues); j++)
    {
        rss = rssValues[j];

        for (k = 0; k < sizeof(rssValues); k++)
        {
            if (k == j)
            {
                continue;
            }

            rss2 = rssValues[k];
            linkInfo.Clear();

            for (i = 0; i < kNumRssAdds; i++)
            {
                linkInfo.AddRss(sNoiseFloor, rss);
                linkInfo.AddRss(sNoiseFloor, rss2);
                ave = linkInfo.GetAverageRss();
                VerifyOrQuit(ave >= MIN_RSS(rss, rss2),
                             "TestLinkQualityInfo failed - GetAverageRss() is smaller than min value.");
                VerifyOrQuit(ave <= MAX_RSS(rss, rss2),
                             "TestLinkQualityInfo failed - GetAverageRss() is larger than min value.");
                diff = ave;
                diff -= (rss + rss2) >> 1;
                VerifyOrQuit(ABS(diff) <= kRssAverageMaxDiff,
                             "TestLinkQualityInfo failed - GetAverageRss() is incorrect");
                VerifyEncodedRssValue(linkInfo);
            }

            printf("[AddRss(%4d),  AddRss(%4d)] %d times: ", rss, rss2, kNumRssAdds);
            PrintOutcome(linkInfo);
        }
    }
}

void TestLinkQualityCalculations(void)
{
    const int8_t  rssList1[] = { -81, -80, -79, -78, -76, -80, -77, -75, -77, -76, -77, -74};
    const RssTestData rssData1 =
    {
        rssList1,           // mRssList
        sizeof(rssList1),   // mRssListSize
        3                   // mExpectedLinkQuality
    };

    const int8_t  rssList2[] = { -90, -80, -85 };
    const RssTestData rssData2 =
    {
        rssList2,           // mRssList
        sizeof(rssList2),   // mRssListSize
        2                   // mExpectedLinkQuality
    };

    const int8_t  rssList3[] = { -95, -96, -98, -99, -100, -100, -98, -99, -100, -100, -100, -100, -100 };
    const RssTestData rssData3 =
    {
        rssList3,           // mRssList
        sizeof(rssList3),   // mRssListSize
        0                   // mExpectedLinkQuality
    };

    const int8_t  rssList4[] = { -75, -100, -100, -100, -100, -100, -95, -92, -93, -94, -93, -93 };
    const RssTestData rssData4 =
    {
        rssList4,           // mRssList
        sizeof(rssList4),   // mRssListSize
        1                   // mExpectedLinkQuality
    };

    TestLinkQualityData(rssData1);
    TestLinkQualityData(rssData2);
    TestLinkQualityData(rssData3);
    TestLinkQualityData(rssData4);
}

}  // namespace Thread

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    Thread::TestRssAveraging();
    Thread::TestLinkQualityCalculations();
    printf("All tests passed\n");
    return 0;
}
#endif
