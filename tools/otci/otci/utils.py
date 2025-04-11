#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#
import functools
from typing import Any, Callable, Collection, Generator, Pattern, Union


def match_line(line: str, expect_line: Union[str, Pattern[Any], Collection[Any]]) -> bool:
    """Checks if a line is expected (matched by one of the given patterns)."""
    if isinstance(expect_line, Pattern):
        match = expect_line.match(line) is not None
    elif isinstance(expect_line, str):
        match = (line == expect_line)
    else:
        match = any(match_line(line, x) for x in expect_line)

    return match


def cached(func: Callable[[Any], Any]):
    """Decorator cached makes the function to cache its result and return it in duplicate calls."""
    prop_name = str('__cached_' + func.__name__)

    @functools.wraps(func)
    def _cached_func(self: Any):
        try:
            return getattr(self, prop_name)
        except AttributeError:
            val = func(self)
            setattr(self, prop_name, val)
            return val

    return _cached_func


def constant_property(func: Callable[[Any], Any]) -> property:
    """A constant property is a property that only evaluated once."""
    return property(cached(func))


def bits_set(number: int) -> Generator[int, int, None]:
    """Find all occurrences of a pattern in a string."""
    idx = 0
    while number != 0:
        if number & 1:
            yield idx
        number >>= 1
        idx += 1
