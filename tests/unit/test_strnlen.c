/*
 *    Copyright (c) 2016-2017, The OpenThread Authors.
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

#include <openthread/config.h>

#include <stdio.h>
#include <stdlib.h>
#include "utils/wrap_string.h"

static void fail(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    char string_a[5] = "\0foo";
    char string_b[8] = "foo\0bar";

    (void)argc;
    (void)argv;

    if (0 != missing_strnlen(string_a, 0))
    {
        fail("0len 0 fails");
    }

    if (0 != missing_strnlen(string_a, 1))
    {
        fail("0len 1 fails");
    }

    if (0 != missing_strnlen(string_a, 2))
    {
        fail("0len 2 fails");
    }

    if (0 != missing_strnlen(string_b, 0))
    {
        fail("3len 0 fails");
    }

    if (1 != missing_strnlen(string_b, 1))
    {
        fail("3len 1 fails");
    }

    if (2 != missing_strnlen(string_b, 2))
    {
        fail("3len 2 fails");
    }

    if (3 != missing_strnlen(string_b, 3))
    {
        fail("3len 3 fails");
    }

    if (3 != missing_strnlen(string_b, 4))
    {
        fail("3len 4 fails");
    }

    if (3 != missing_strnlen(string_b, 5))
    {
        fail("3len 5 fails");
    }

    if (3 != missing_strnlen(string_b, 6))
    {
        fail("3len 6 fails");
    }

    printf("OK\n");
    return EXIT_SUCCESS;
}
