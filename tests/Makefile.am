#
#    Copyright 2016 The nlfaultinjection Authors.
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
#      This file is the GNU automake template for the
#      Nest Labs Fault Injection unit tests

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

#
# Local headers to build against and distribute but not to install
# since they are not part of the package.
#
noinst_HEADERS                                 = \
    $(NULL)

#
# Other files we do want to distribute with the NLFAULTINJECTION SDK.
#
EXTRA_DIST                                     = \
    $(NULL)

if NLFAULTINJECTION_BUILD_TESTS
# C preprocessor option flags that will apply to all compiled objects in this
# makefile.

AM_CPPFLAGS                                    = \
    -I$(top_srcdir)/include                      \
    $(NULL)

COMMON_LDADD                                   = \
	$(top_builddir)/src/libnlfaultinjection.a    \
    $(NULL)

# Test applications that should be run when the 'check' target is run.

check_PROGRAMS                                 = \
    fault-injection-test                         \
    $(NULL)

# Test applications and scripts that should be built and run when the
# 'check' target is run.

TESTS                                          = \
    $(check_PROGRAMS)                            \
    $(NULL)

# The additional environment variables and their values that will be
# made available to all programs and scripts in TESTS.

TESTS_ENVIRONMENT                              = \
    $(NULL)

# Source, compiler, and linker options for test programs.

fault_injection_test_LDADD                         = $(COMMON_LDADD)
fault_injection_test_SOURCES                       = TestFaultInjection.cpp

if NLFAULTINJECTION_BUILD_COVERAGE
CLEANFILES                                     = $(wildcard *.gcda *.gcno)

if NLFAULTINJECTION_BUILD_COVERAGE_REPORTS
# The bundle should positively be qualified with the absolute build
# path. Otherwise, VPATH will get auto-prefixed to it if there is
# already such a directory in the non-colocated source tree.

NLFAULTINJECTION_COVERAGE_BUNDLE                           = ${abs_builddir}/${PACKAGE}${NL_COVERAGE_BUNDLE_SUFFIX}
NLFAULTINJECTION_COVERAGE_INFO                             = ${NLFAULTINJECTION_COVERAGE_BUNDLE}/${PACKAGE}${NL_COVERAGE_INFO_SUFFIX}

$(NLFAULTINJECTION_COVERAGE_BUNDLE):
	$(call create-directory)

$(NLFAULTINJECTION_COVERAGE_INFO): check | $(NLFAULTINJECTION_COVERAGE_BUNDLE)
	$(call generate-coverage-report,${top_builddir})

coverage: $(NLFAULTINJECTION_COVERAGE_INFO)

clean-local: clean-local-coverage

.PHONY: clean-local-coverage
clean-local-coverage:
	-$(AM_V_at)rm -rf $(NLFAULTINJECTION_COVERAGE_BUNDLE)
endif # NLFAULTINJECTION_BUILD_COVERAGE_REPORTS
endif # NLFAULTINJECTION_BUILD_COVERAGE
endif # NLFAULTINJECTION_BUILD_TESTS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
