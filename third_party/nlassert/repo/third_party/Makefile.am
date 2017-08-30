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
#      This file is the GNU automake template for the Nest Labs runtime
#      assertion library first-party (e.g. Nest Labs) software directory.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

# Always package (e.g. for 'make dist') these subdirectories.

DIST_SUBDIRS             = \
    nlunit-test/repo       \
    $(NULL)

# Always build (e.g. for 'make all') these subdirectories.

SUBDIRS                  = \
    $(NULL)

# Only build (e.g. for 'make all') these subdirectories when requested.

if NLASSERT_WITH_NLUNIT_TEST_INTERNAL
SUBDIRS                 += \
    nlunit-test/repo       \
    $(NULL)
endif # NLASSERT_WITH_NLUNIT_TEST_INTERNAL

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
