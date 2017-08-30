#
#    Copyright 2015-2017 Nest Labs Inc. All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    Description:
#      This file is the GNU automake template for the Nest Labs Unit Test
#      library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

include_HEADERS             = \
    nltest.h                  \
    nlunit-test.h             \
    $(NULL)

lib_LIBRARIES               = \
    libnlunit-test.a          \
    $(NULL)

libnlunit_test_a_SOURCES    = \
    nlunit-test.c             \
    $(NULL)

install-headers: install-data

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
