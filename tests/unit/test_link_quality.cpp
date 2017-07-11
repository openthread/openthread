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

#include <openthread/openthread.h>

#include "thread/link_quality.hpp"
#include "utils/wrap_string.h"

#include "test_platform.h"
#include "test_util.h"

namespace ot {

enum
{
    kMaxRssValue        = 0,
    kMinRssValue        = -128,

    kStringBuffferSize  = 80,

    kRssAverageMaxDiff  = 16,
    kNumRssAdds         = 300,

    kRawAverageBitShift = 3,
    kRawAverageMultiple = (1 << kRawAverageBitShift),
    kRawAverageBitMask  = (1 << kRawAverageBitShift) - 1,
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

int8_t sNoiseFloor = -100;  // dBm

// Check and verify the raw average RSS value to match the value from GetAverage().
void VerifyRawRssValue(int8_t aAverage, uint16_t aRawValue)
{
    if (aAverage != OT_RADIO_RSSI_INVALID)
    {
        VerifyOrQuit(aAverage == -static_cast<int16_t>((aRawValue + (kRawAverageMultiple / 2)) >> kRawAverageBitShift),
                     "TestLinkQualityInfo failed - Raw value does not match the average.");
    }
    else
    {
        VerifyOrQuit(aRawValue == 0, "TestLinkQualityInfo failed - Raw value does not match the average.");
    }
}

// This function prints the values in the passed in link info instance. It is invoked as the final step in test-case.
void PrintOutcome(LinkQualityInfo &aLinkInfo)
{
    char stringBuf[LinkQualityInfo::kInfoStringSize];

    VerifyOrQuit(aLinkInfo.ToInfoString(stringBuf, sizeof(stringBuf)) != NULL, "ToInfoString() returned NULL");

    printf("%s", stringBuf);

    // This test-case succeeded.
    printf(" -> PASS\n");
}

void TestLinkQualityData(RssTestData aRssData)
{
    LinkQualityInfo linkInfo;
    int8_t rss, ave, min, max;
    size_t i;

    printf("- - - - - - - - - - - - - - - - - -\n");
    min = kMinRssValue;
    max = kMaxRssValue;

    for (i = 0; i < aRssData.mRssListSize; i++)
    {
        rss = aRssData.mRssList[i];
        min = MIN_RSS(rss, min);
        max = MAX_RSS(rss, max);
        linkInfo.AddRss(sNoiseFloor, rss);
        ave = linkInfo.GetAverageRss();
        VerifyOrQuit(ave >= min, "TestLinkQualityInfo failed - GetAverageRss() is smaller than min value.");
        VerifyOrQuit(ave <= max, "TestLinkQualityInfo failed - GetAverageRss() is larger than min value");
        VerifyRawRssValue(linkInfo.GetAverageRss(), linkInfo.GetAverageRssRaw());
        printf("%02u) AddRss(%4d): ", (unsigned int)i, rss);
        PrintOutcome(linkInfo);
    }

    VerifyOrQuit(linkInfo.GetLinkQuality() == aRssData.mExpectedLinkQuality,
                 "TestLinkQualityInfo failed - GetLinkQuality() is incorrect");
}

// Check and verify the raw average RSS value to match the value from GetAverage().
void VerifyRawRssValue(RssAverager &aRssAverager)
{
    int8_t average = aRssAverager.GetAverage();
    uint16_t rawValue = aRssAverager.GetRaw();

    if (average != OT_RADIO_RSSI_INVALID)
    {
        VerifyOrQuit(average == -static_cast<int16_t>((rawValue + (kRawAverageMultiple / 2)) >> kRawAverageBitShift),
                     "TestLinkQualityInfo failed - Raw value does not match the average.");
    }
    else
    {
        VerifyOrQuit(rawValue == 0, "TestLinkQualityInfo failed - Raw value does not match the average.");
    }
}

// This function prints the values in the passed in link info instance. It is invoked as the final step in test-case.
void PrintOutcome(RssAverager &aRssAverager)
{
    char stringBuf[RssAverager::kStringSize];

    VerifyOrQuit(aRssAverager.ToString(stringBuf, sizeof(stringBuf)) != NULL, "ToString() returned NULL");
    printf("%s", stringBuf);

    // This test-case succeeded.
    printf(" -> PASS\n");
}


int8_t GetRandomRss(void)
{
    uint32_t value;

    value = otPlatRandomGet() % 128;
    return static_cast<int8_t>(-value);
}

void TestRssAveraging(void)
{
    RssAverager     rssAverager;
    int8_t          rss, rss2, ave;
    int16_t         diff;
    size_t          i, j, k;
    const int8_t    rssValues[] = { kMinRssValue, -70, -40, -41, -10, kMaxRssValue};
    int16_t         sum;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Values after initialization/reset.

    rssAverager.Reset();

    printf("\nAfter Reset: ");
    VerifyOrQuit(rssAverager.GetAverage() == OT_RADIO_RSSI_INVALID,
                 "TestLinkQualityInfo failed - Initial value from GetAverage() is incorrect.");
    VerifyRawRssValue(rssAverager);
    PrintOutcome(rssAverager);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Adding a single value
    rss = -70;
    printf("AddRss(%d): ", rss);
    rssAverager.Add(rss);
    VerifyOrQuit(rssAverager.GetAverage() == rss,
                 "TestLinkQualityInfo - GetAverage() failed after a single AddRss().");
    VerifyRawRssValue(rssAverager);
    PrintOutcome(rssAverager);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Reset

    printf("Reset(): ");
    rssAverager.Reset();
    VerifyOrQuit(rssAverager.GetAverage() == OT_RADIO_RSSI_INVALID,
                 "TestLinkQualityInfo failed - GetAverage() after Reset() is incorrect.");
    VerifyRawRssValue(rssAverager);
    PrintOutcome(rssAverager);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Adding the same value many times.

    printf("- - - - - - - - - - - - - - - - - -\n");

    for (j = 0; j < sizeof(rssValues); j++)
    {
        rssAverager.Reset();
        rss = rssValues[j];
        printf("AddRss(%4d) %d times: ", rss, kNumRssAdds);

        for (i = 0; i < kNumRssAdds; i++)
        {
            rssAverager.Add(rss);
            VerifyOrQuit(rssAverager.GetAverage() == rss,
                         "TestLinkQualityInfo failed - GetAverage() returned incorrect value.");
            VerifyRawRssValue(rssAverager);
        }

        PrintOutcome(rssAverager);

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
            rssAverager.Reset();
            rssAverager.Add(rss);
            rssAverager.Add(rss2);
            printf("AddRss(%4d), AddRss(%4d): ", rss, rss2);
            VerifyOrQuit(rssAverager.GetAverage() == ((rss + rss2) >> 1),
                         "TestLinkQualityInfo failed - GetAverage() returned incorrect value.");
            VerifyRawRssValue(rssAverager);
            PrintOutcome(rssAverager);
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
            rssAverager.Reset();

            for (i = 0; i < kNumRssAdds; i++)
            {
                rssAverager.Add(rss);
            }

            rssAverager.Add(rss2);
            printf("AddRss(%4d) %d times, AddRss(%4d): ", rss, kNumRssAdds, rss2);
            ave = rssAverager.GetAverage();
            VerifyOrQuit(ave >= MIN_RSS(rss, rss2),
                         "TestLinkQualityInfo failed - GetAverage() returned incorrect value.");
            VerifyOrQuit(ave <= MAX_RSS(rss, rss2),
                         "TestLinkQualityInfo failed - GetAverage() returned incorrect value.");
            VerifyRawRssValue(rssAverager);
            PrintOutcome(rssAverager);
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
            rssAverager.Reset();

            for (i = 0; i < kNumRssAdds; i++)
            {
                rssAverager.Add(rss);
                rssAverager.Add(rss2);
                ave = rssAverager.GetAverage();
                VerifyOrQuit(ave >= MIN_RSS(rss, rss2),
                             "TestLinkQualityInfo failed - GetAverage() is smaller than min value.");
                VerifyOrQuit(ave <= MAX_RSS(rss, rss2),
                             "TestLinkQualityInfo failed - GetAverage() is larger than min value.");
                diff = ave;
                diff -= (rss + rss2) >> 1;
                VerifyOrQuit(ABS(diff) <= kRssAverageMaxDiff,
                             "TestLinkQualityInfo failed - GetAverage() is incorrect");
                VerifyRawRssValue(rssAverager);
            }

            printf("[AddRss(%4d),  AddRss(%4d)] %d times: ", rss, rss2, kNumRssAdds);
            PrintOutcome(rssAverager);
        }
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // For the first 8 values the average should be the arithmetic mean.

    printf("- - - - - - - - - - - - - - - - - -\n");

    for (i = 0; i < 1000; i++)
    {
        double mean;

        rssAverager.Reset();
        sum = 0;

        printf("\n");

        for (j = 1; j <= 8; j++)
        {
            rss = GetRandomRss();
            rssAverager.Add(rss);
            sum += rss;
            mean = static_cast<double>(sum) / j;
            VerifyOrQuit(ABS(rssAverager.GetAverage() - mean) < 1, "Average does not match the arithmetic mean!");
            VerifyRawRssValue(rssAverager);
            printf("AddRss(%4d) sum=%-5d, mean=%-8.2f RssAverager=", rss, sum, mean);
            PrintOutcome(rssAverager);
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

}  // namespace ot

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    ot::TestRssAveraging();
    ot::TestLinkQualityCalculations();
    printf("All tests passed\n");
    return 0;
}
#endif
