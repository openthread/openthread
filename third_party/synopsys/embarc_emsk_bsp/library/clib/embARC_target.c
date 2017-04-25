/* ------------------------------------------
 * Copyright (c) 2017, Synopsys, Inc. All rights reserved.

 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:

 * 1) Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.

 * 3) Neither the name of the Synopsys, Inc., nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * \version 2017.03
 * \date 2016-03-02
 * \author Huaqi Fang(Huaqi.Fang@synopsys.com)
--------------------------------------------- */
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "embARC_syscalls.h"

//////////////////////////////
// OTHER REQUIRED FUNCTIONS //
//////////////////////////////

static const char szEnglishMonth[12][4] = { \
                                            "Jan", "Feb", "Mar", "Apr", \
                                            "May", "Jun", "Jul", "Aug", \
                                            "Sep", "Oct", "Nov", "Dec"
                                          };

time_t get_build_timedate(struct tm *build_tm)
{
    if (!build_tm) { return 0; }

    char szMonth[5];
    int tm_year, tm_mon = 0, tm_day;
    int tm_hour, tm_min, tm_sec;
    time_t build_time;

    /** Get Build Date */
    sscanf(__DATE__, "%s %d %d", szMonth, &tm_day, &tm_year) ;

    for (int i = 0; i < 12; i++)
    {
        if (strncmp(szMonth, szEnglishMonth[i], 3) == 0)
        {
            tm_mon = i;
            break;
        }
    }

    /** Get Build Time */
    sscanf(__TIME__, "%d:%d:%d", &tm_hour, &tm_min, &tm_sec);
    build_tm->tm_sec = tm_sec;
    build_tm->tm_min = tm_min;
    build_tm->tm_hour = tm_hour;
    build_tm->tm_mday = tm_day;
    build_tm->tm_mon = tm_mon;
    build_tm->tm_year = tm_year - 1900;
    build_tm->tm_isdst = 0;

    build_time = mktime(build_tm);
    return build_time;
}