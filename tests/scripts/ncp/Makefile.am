#
#  Copyright (c) 2016, Nest Labs, Inc.
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

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

LOG_DRIVER=$(abs_top_srcdir)/third_party/openthread-test-driver/test-driver

EXTRA_DIST                                                         = \
    test_ncp_basics.py                                               \
    test_ncp_phy.py                                                  \
    test_ncp_mac.py                                                  \
    test_ncp_net.py                                                  \
    test_ncp_ipv6.py                                                 \
    node.py                                                          \
    $(NULL)

if OPENTHREAD_BUILD_TESTS

# First, list all essential program and script tests that MUST be run.

check_PROGRAMS =

check_SCRIPTS =

if OPENTHREAD_ENABLE_NCP

check_SCRIPTS                                                     += \
    test_ncp_basics.py                                               \
    test_ncp_phy.py                                                  \
    test_ncp_mac.py                                                  \
    test_ncp_net.py                                                  \
    test_ncp_ipv6.py                                                 \
    $(NULL)

endif

TESTS_ENVIRONMENT                                                  = \
    export                                                           \
    top_builddir='$(top_builddir)'                                   \
    VERBOSE=1;                                                       \
    $(NULL)

TESTS                                                              = \
    $(check_PROGRAMS)                                                \
    $(check_SCRIPTS)                                                 \
    $(NULL)


endif # OPENTHREAD_BUILD_TESTS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
