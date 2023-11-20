/*
 *  Copyright (c) 2023, The OpenThread Authors.
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

#include "test_util.h"
#include "lib/url/url.hpp"

namespace ot {
namespace Url {

void TestSimple(void)
{
    char url[] = "spinel:///dev/ttyUSB0?baudrate=115200";
    Url  args;

    VerifyOrQuit(!args.Init(url));

    VerifyOrQuit(!strcmp(args.GetPath(), "/dev/ttyUSB0"));
    VerifyOrQuit(!strcmp(args.GetValue("baudrate"), "115200"));
    VerifyOrQuit(args.GetValue("not-exists") == nullptr);
    VerifyOrQuit(args.GetValue("last-value-wrong-position", url) == nullptr);
    VerifyOrQuit(args.GetValue("last-value-before-url", url - 1) == nullptr);
    VerifyOrQuit(args.GetValue("last-value-after-url", url + sizeof(url)) == nullptr);

    printf("PASS %s\r\n", __func__);
}

void TestSimpleNoQueryString(void)
{
    char url[] = "spinel:///dev/ttyUSB0";
    Url  args;

    VerifyOrQuit(!args.Init(url));
    VerifyOrQuit(!strcmp(args.GetPath(), "/dev/ttyUSB0"));
    VerifyOrQuit(args.GetValue("last-value-wrong-position", url) == nullptr);
    VerifyOrQuit(args.GetValue("last-value-before-url", url - 1) == nullptr);
    VerifyOrQuit(args.GetValue("last-value-after-url", url + sizeof(url)) == nullptr);

    printf("PASS %s\r\n", __func__);
}

void TestEmptyValue(void)
{
    char        url[] = "spinel:///dev/ttyUSB0?rtscts&baudrate=115200&verbose&verbose&verbose";
    Url         args;
    const char *arg = nullptr;

    VerifyOrQuit(!args.Init(url));
    VerifyOrQuit(!strcmp(args.GetPath(), "/dev/ttyUSB0"));
    VerifyOrQuit((arg = args.GetValue("rtscts")) != nullptr);
    VerifyOrQuit(args.GetValue("rtscts", arg) == nullptr);
    VerifyOrQuit((arg = args.GetValue("verbose", arg)) != nullptr);
    VerifyOrQuit((arg = args.GetValue("verbose", arg)) != nullptr);
    VerifyOrQuit((arg = args.GetValue("verbose", arg)) != nullptr);
    VerifyOrQuit((arg = args.GetValue("verbose", arg)) == nullptr);

    printf("PASS %s\r\n", __func__);
}

void TestMultipleProtocols(void)
{
    char url[] = "spinel+spi:///dev/ttyUSB0?baudrate=115200";
    Url  args;

    VerifyOrQuit(!args.Init(url));
    VerifyOrQuit(!strcmp(args.GetPath(), "/dev/ttyUSB0"));
    VerifyOrQuit(!strcmp(args.GetValue("baudrate"), "115200"));

    printf("PASS %s\r\n", __func__);
}

void TestMultipleProtocolsAndDuplicateParameters(void)
{
    char        url[] = "spinel+exec:///path/to/ot-rcp?arg=1&arg=arg2&arg=3";
    Url         args;
    const char *arg = nullptr;

    VerifyOrQuit(!args.Init(url));
    VerifyOrQuit(!strcmp(args.GetPath(), "/path/to/ot-rcp"));

    arg = args.GetValue("arg");
    VerifyOrQuit(!strcmp(arg, "1"));

    arg = args.GetValue("arg", arg);
    VerifyOrQuit(!strcmp(arg, "arg2"));

    arg = args.GetValue("arg", arg);
    VerifyOrQuit(!strcmp(arg, "3"));

    VerifyOrQuit(args.GetValue("arg", url) == nullptr);
    VerifyOrQuit(args.GetValue("arg", url - 1) == nullptr);
    VerifyOrQuit(args.GetValue("arg", url + sizeof(url)) == nullptr);

    printf("PASS %s\r\n", __func__);
}

void TestIntValue(void)
{
    char int8url[]  = "spinel:///dev/ttyUSB0?no-reset&val1=1&val2=0x02&val3=-0X03&val4=-4&val5=+5&val6=128&val7=-129";
    char int16url[] = "spinel:///dev/ttyUSB0?val1=1&val2=0x02&val3=-0X03&val4=-400&val5=+500&val6=32768&val7=-32769";
    char int32url[] =
        "spinel:///dev/ttyUSB0?val1=1&val2=0x02&val3=-0X03&val4=-40000&val5=+50000&val6=2147483648&val7=-2147483649";
    Url     args;
    int8_t  int8val;
    int16_t int16val;
    int32_t int32val;

    VerifyOrQuit(!args.Init(int8url));
    VerifyOrQuit(!strcmp(args.GetPath(), "/dev/ttyUSB0"));
    VerifyOrQuit(args.HasParam("no-reset"));
    VerifyOrQuit(!args.HasParam("reset"));
    SuccessOrQuit(args.ParseInt8("val1", int8val));
    VerifyOrQuit(int8val == 1);
    SuccessOrQuit(args.ParseInt8("val2", int8val));
    VerifyOrQuit(int8val == 2);
    SuccessOrQuit(args.ParseInt8("val3", int8val));
    VerifyOrQuit(int8val == -3);
    SuccessOrQuit(args.ParseInt8("val4", int8val));
    VerifyOrQuit(int8val == -4);
    SuccessOrQuit(args.ParseInt8("val5", int8val));
    VerifyOrQuit(int8val == 5);
    VerifyOrQuit(args.ParseInt8("val6", int8val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(int8val == 5);
    VerifyOrQuit(args.ParseInt8("val7", int8val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(int8val == 5);
    VerifyOrQuit(args.ParseInt8("val8", int8val) == OT_ERROR_NOT_FOUND);
    VerifyOrQuit(int8val == 5);

    VerifyOrQuit(!args.Init(int16url));
    VerifyOrQuit(!strcmp(args.GetPath(), "/dev/ttyUSB0"));
    SuccessOrQuit(args.ParseInt16("val1", int16val));
    VerifyOrQuit(int16val == 1);
    SuccessOrQuit(args.ParseInt16("val2", int16val));
    VerifyOrQuit(int16val == 2);
    SuccessOrQuit(args.ParseInt16("val3", int16val));
    VerifyOrQuit(int16val == -3);
    SuccessOrQuit(args.ParseInt16("val4", int16val));
    VerifyOrQuit(int16val == -400);
    SuccessOrQuit(args.ParseInt16("val5", int16val));
    VerifyOrQuit(int16val == 500);
    VerifyOrQuit(args.ParseInt16("val6", int16val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(int16val == 500);
    VerifyOrQuit(args.ParseInt16("val7", int16val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(int16val == 500);
    VerifyOrQuit(args.ParseInt16("val8", int16val) == OT_ERROR_NOT_FOUND);
    VerifyOrQuit(int16val == 500);

    VerifyOrQuit(!args.Init(int32url));
    VerifyOrQuit(!strcmp(args.GetPath(), "/dev/ttyUSB0"));
    SuccessOrQuit(args.ParseInt32("val1", int32val));
    VerifyOrQuit(int32val == 1);
    SuccessOrQuit(args.ParseInt32("val2", int32val));
    VerifyOrQuit(int32val == 2);
    SuccessOrQuit(args.ParseInt32("val3", int32val));
    VerifyOrQuit(int32val == -3);
    SuccessOrQuit(args.ParseInt32("val4", int32val));
    VerifyOrQuit(int32val == -40000);
    SuccessOrQuit(args.ParseInt32("val5", int32val));
    VerifyOrQuit(int32val == 50000);
    VerifyOrQuit(args.ParseInt32("val6", int32val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(int32val == 50000);
    VerifyOrQuit(args.ParseInt32("val7", int32val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(int32val == 50000);
    VerifyOrQuit(args.ParseInt32("val8", int32val) == OT_ERROR_NOT_FOUND);
    VerifyOrQuit(int32val == 50000);

    printf("PASS %s\r\n", __func__);
}

void TestUintValue(void)
{
    char uint8url[]  = "spinel:///dev/ttyUSB0?no-reset&val1=1&val2=0x02&val3=0X03&val4=-4&val5=+5&val6=256&val7=-1";
    char uint16url[] = "spinel:///dev/ttyUSB0?val1=1&val2=0x02&val3=0X03&val4=-400&val5=+500&val6=65536&val7=-1";
    char uint32url[] =
        "spinel:///dev/ttyUSB0?val1=1&val2=0x02&val3=0X03&val4=-40000&val5=+70000&val6=4294967296&val7=-1";
    Url      args;
    uint8_t  uint8val;
    uint16_t uint16val;
    uint32_t uint32val;

    VerifyOrQuit(!args.Init(uint8url));
    VerifyOrQuit(!strcmp(args.GetPath(), "/dev/ttyUSB0"));
    SuccessOrQuit(args.ParseUint8("val1", uint8val));
    VerifyOrQuit(uint8val == 1);
    SuccessOrQuit(args.ParseUint8("val2", uint8val));
    VerifyOrQuit(uint8val == 2);
    SuccessOrQuit(args.ParseUint8("val3", uint8val));
    VerifyOrQuit(uint8val == 3);
    VerifyOrQuit(args.ParseUint8("val4", uint8val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(uint8val == 3);
    SuccessOrQuit(args.ParseUint8("val5", uint8val));
    VerifyOrQuit(uint8val == 5);
    VerifyOrQuit(args.ParseUint8("val6", uint8val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(uint8val == 5);
    VerifyOrQuit(args.ParseUint8("val7", uint8val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(uint8val == 5);
    VerifyOrQuit(args.ParseUint8("val8", uint8val) == OT_ERROR_NOT_FOUND);
    VerifyOrQuit(uint8val == 5);

    VerifyOrQuit(!args.Init(uint16url));
    VerifyOrQuit(!strcmp(args.GetPath(), "/dev/ttyUSB0"));
    SuccessOrQuit(args.ParseUint16("val1", uint16val));
    VerifyOrQuit(uint16val == 1);
    SuccessOrQuit(args.ParseUint16("val2", uint16val));
    VerifyOrQuit(uint16val == 2);
    SuccessOrQuit(args.ParseUint16("val3", uint16val));
    VerifyOrQuit(uint16val == 3);
    VerifyOrQuit(args.ParseUint16("val4", uint16val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(uint16val == 3);
    SuccessOrQuit(args.ParseUint16("val5", uint16val));
    VerifyOrQuit(uint16val == 500);
    VerifyOrQuit(args.ParseUint16("val6", uint16val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(uint16val == 500);
    VerifyOrQuit(args.ParseUint16("val7", uint16val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(uint16val == 500);
    VerifyOrQuit(args.ParseUint16("val8", uint16val) == OT_ERROR_NOT_FOUND);
    VerifyOrQuit(uint16val == 500);

    VerifyOrQuit(!args.Init(uint32url));
    VerifyOrQuit(!strcmp(args.GetPath(), "/dev/ttyUSB0"));
    SuccessOrQuit(args.ParseUint32("val1", uint32val));
    VerifyOrQuit(uint32val == 1);
    SuccessOrQuit(args.ParseUint32("val2", uint32val));
    VerifyOrQuit(uint32val == 2);
    SuccessOrQuit(args.ParseUint32("val3", uint32val));
    VerifyOrQuit(uint32val == 3);
    VerifyOrQuit(args.ParseUint32("val4", uint32val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(uint32val == 3);
    SuccessOrQuit(args.ParseUint32("val5", uint32val));
    VerifyOrQuit(uint32val == 70000);
    VerifyOrQuit(args.ParseUint32("val6", uint32val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(uint32val == 70000);
    VerifyOrQuit(args.ParseUint32("val7", uint32val) == OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(uint32val == 70000);
    VerifyOrQuit(args.ParseUint32("val8", uint32val) == OT_ERROR_NOT_FOUND);
    VerifyOrQuit(uint32val == 70000);

    printf("PASS %s\r\n", __func__);
}

} // namespace Url
} // namespace ot

int main(void)
{
    ot::Url::TestSimple();
    ot::Url::TestSimpleNoQueryString();
    ot::Url::TestEmptyValue();
    ot::Url::TestMultipleProtocols();
    ot::Url::TestMultipleProtocolsAndDuplicateParameters();
    ot::Url::TestIntValue();
    ot::Url::TestUintValue();

    return 0;
}
