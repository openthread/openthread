#
#    Copyright (c) 2016 Nest Labs, Inc.
#    All rights reserved.
#
#    This document is the property of Nest. It is considered
#    confidential and proprietary information.
#
#    This document may not be reproduced or transmitted in any form,
#    in whole or in part, without the express written permission of
#    Nest.
#
#    Description:
#      This file is the GNU automake template for the Nest Labs
#      runtime assertion library unit tests.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

#
# Local headers to build against and distribute but not to install
# since they are not part of the package.
#
noinst_HEADERS                                                = \
    nlassert-test-config.h                                      \
    $(NULL)

#
# Other files we do want to distribute with the package.
#
EXTRA_DIST                                                    = \
    $(NULL)

if NLASSERT_BUILD_TESTS
# C preprocessor option flags that will apply to all compiled objects in this
# makefile.

AM_CPPFLAGS                                                   = \
    $(ISYSTEM_CPPFLAGS)                                         \
    -I$(top_srcdir)/include                                     \
    $(NULL)

COMMON_CPPFLAGS                                               = \
    $(AM_CPPFLAGS)                                              \
    -DNL_ASSERT_TEST_WANT_ABORT=1                               \
    $(NULL)

NONPRODUCTION_CPPFLAGS                                        = \
    $(COMMON_CPPFLAGS)                                          \
    -DNL_ASSERT_TEST_WANT_PRODUCTION=0                          \
    $(NULL)

PRODUCTION_CPPFLAGS                                           = \
    $(COMMON_CPPFLAGS)                                          \
    -DNL_ASSERT_TEST_WANT_PRODUCTION=1                          \
    $(NULL)

COMMON_SOURCES                                                = \
    nlassert-test.c                                             \
    $(NULL)

STATIC_SOURCES                                                = \
    nlassert-static-test.c                                      \
    $(NULL)

STATIC_CXX_SOURCES                                            = \
    nlassert-static-test-cxx.cpp                                \
    $(NULL)

# Test applications that should be run when the 'check' target is run.

check_PROGRAMS                                                = \
    nlassert-static-test-nonproduction-success                  \
    nlassert-static-test-nonproduction-failure                  \
    nlassert-static-test-cxx-nonproduction-success              \
    nlassert-static-test-cxx-nonproduction-failure              \
    nlassert-test-nonproduction-defaults                        \
    nlassert-test-nonproduction-abort                           \
    nlassert-test-nonproduction-abort-backtrace                 \
    nlassert-test-nonproduction-abort-log                       \
    nlassert-test-nonproduction-abort-trap                      \
    nlassert-test-nonproduction-abort-backtrace-log             \
    nlassert-test-nonproduction-abort-backtrace-trap            \
    nlassert-test-nonproduction-abort-log-trap                  \
    nlassert-test-nonproduction-abort-backtrace-log-trap        \
    nlassert-static-test-production-success                     \
    nlassert-static-test-production-failure                     \
    nlassert-static-test-cxx-production-success                 \
    nlassert-static-test-cxx-production-failure                 \
    nlassert-test-production-defaults                           \
    nlassert-test-production-abort                              \
    nlassert-test-production-abort-backtrace                    \
    nlassert-test-production-abort-log                          \
    nlassert-test-production-abort-trap                         \
    nlassert-test-production-abort-backtrace-log                \
    nlassert-test-production-abort-backtrace-trap               \
    nlassert-test-production-abort-log-trap                     \
    nlassert-test-production-abort-backtrace-log-trap           \
    $(NULL)

# Test applications and scripts that should be built and run when the
# 'check' target is run.

TESTS                                                         = \
    $(check_PROGRAMS)                                           \
    $(NULL)

# The additional environment variables and their values that will be
# made available to all programs and scripts in TESTS.

TESTS_ENVIRONMENT                                             = \
    $(NULL)

#
# Source, compiler, and linker options for test programs.
#
# This enumerates mnemonics for all possible combinations of assert
# "features" (backtracing, logging, and trapping).
#

static_success_CPPFLAGS                                       = \
        -DNL_ASSERT_TEST_WANT_STATIC_SUCCESS=1                  \
        $(NULL)

#
# NOTE: Change this to '0' from '1' if you want to demonstrate
#       compile-time failure of the assertion checks.
#

static_failure_CPPFLAGS                                       = \
        -DNL_ASSERT_TEST_WANT_STATIC_SUCCESS=1                  \
        $(NULL)

defaults_CPPFLAGS                                             = \
	-DNL_ASSERT_TEST_WANT_DEFAULTS=1                        \
        -DNL_ASSERT_TEST_WANT_BACKTRACE=0                       \
        -DNL_ASSERT_TEST_WANT_LOG=0                             \
        -DNL_ASSERT_TEST_WANT_TRAP=0                            \
	$(NULL)

abort_CPPFLAGS                                                = \
        -DNL_ASSERT_TEST_WANT_DEFAULTS=0                        \
        -DNL_ASSERT_TEST_WANT_BACKTRACE=0                       \
        -DNL_ASSERT_TEST_WANT_LOG=0                             \
        -DNL_ASSERT_TEST_WANT_TRAP=0                            \
	$(NULL)

abort_backtrace_CPPFLAGS                                      = \
        -DNL_ASSERT_TEST_WANT_DEFAULTS=0                        \
        -DNL_ASSERT_TEST_WANT_BACKTRACE=1                       \
        -DNL_ASSERT_TEST_WANT_LOG=0                             \
        -DNL_ASSERT_TEST_WANT_TRAP=0                            \
	$(NULL)

abort_log_CPPFLAGS                                            = \
        -DNL_ASSERT_TEST_WANT_DEFAULTS=0                        \
        -DNL_ASSERT_TEST_WANT_BACKTRACE=0                       \
        -DNL_ASSERT_TEST_WANT_LOG=1                             \
        -DNL_ASSERT_TEST_WANT_TRAP=0                            \
	$(NULL)

abort_trap_CPPFLAGS                                           = \
        -DNL_ASSERT_TEST_WANT_DEFAULTS=0                        \
        -DNL_ASSERT_TEST_WANT_BACKTRACE=0                       \
        -DNL_ASSERT_TEST_WANT_LOG=0                             \
        -DNL_ASSERT_TEST_WANT_TRAP=1                            \
	$(NULL)

abort_backtrace_log_CPPFLAGS                                  = \
        -DNL_ASSERT_TEST_WANT_DEFAULTS=0                        \
        -DNL_ASSERT_TEST_WANT_BACKTRACE=1                       \
        -DNL_ASSERT_TEST_WANT_LOG=1                             \
        -DNL_ASSERT_TEST_WANT_TRAP=0                            \
	$(NULL)

abort_backtrace_trap_CPPFLAGS                                 = \
        -DNL_ASSERT_TEST_WANT_DEFAULTS=0                        \
        -DNL_ASSERT_TEST_WANT_BACKTRACE=1                       \
        -DNL_ASSERT_TEST_WANT_LOG=0                             \
        -DNL_ASSERT_TEST_WANT_TRAP=1                            \
	$(NULL)

abort_log_trap_CPPFLAGS                                       = \
        -DNL_ASSERT_TEST_WANT_DEFAULTS=0                        \
        -DNL_ASSERT_TEST_WANT_BACKTRACE=0                       \
        -DNL_ASSERT_TEST_WANT_LOG=1                             \
        -DNL_ASSERT_TEST_WANT_TRAP=1                            \
	$(NULL)

abort_backtrace_log_trap_CPPFLAGS                             = \
        -DNL_ASSERT_TEST_WANT_DEFAULTS=0                        \
        -DNL_ASSERT_TEST_WANT_BACKTRACE=1                       \
        -DNL_ASSERT_TEST_WANT_LOG=1                             \
        -DNL_ASSERT_TEST_WANT_TRAP=1                            \
	$(NULL)

#
# Test Program Proprocessor Flags
#

nlassert_static_test_nonproduction_success_CPPFLAGS           = $(NONPRODUCTION_CPPFLAGS) $(static_success_CPPFLAGS)
nlassert_static_test_nonproduction_failure_CPPFLAGS           = $(NONPRODUCTION_CPPFLAGS) $(static_failure_CPPFLAGS)
nlassert_static_test_cxx_nonproduction_success_CPPFLAGS       = $(NONPRODUCTION_CPPFLAGS) $(static_success_CPPFLAGS)
nlassert_static_test_cxx_nonproduction_failure_CPPFLAGS       = $(NONPRODUCTION_CPPFLAGS) $(static_failure_CPPFLAGS)

nlassert_test_nonproduction_defaults_CPPFLAGS                 = $(NONPRODUCTION_CPPFLAGS) $(defaults_CPPFLAGS)
nlassert_test_nonproduction_abort_CPPFLAGS                    = $(NONPRODUCTION_CPPFLAGS) $(abort_CPPFLAGS)
nlassert_test_nonproduction_abort_backtrace_CPPFLAGS          = $(NONPRODUCTION_CPPFLAGS) $(abort_backtrace_CPPFLAGS)
nlassert_test_nonproduction_abort_log_CPPFLAGS                = $(NONPRODUCTION_CPPFLAGS) $(abort_log_CPPFLAGS)
nlassert_test_nonproduction_abort_trap_CPPFLAGS               = $(NONPRODUCTION_CPPFLAGS) $(abort_trap_CPPFLAGS)
nlassert_test_nonproduction_abort_backtrace_log_CPPFLAGS      = $(NONPRODUCTION_CPPFLAGS) $(abort_backtrace_log_CPPFLAGS)
nlassert_test_nonproduction_abort_backtrace_trap_CPPFLAGS     = $(NONPRODUCTION_CPPFLAGS) $(abort_backtrace_trap_CPPFLAGS)
nlassert_test_nonproduction_abort_log_trap_CPPFLAGS           = $(NONPRODUCTION_CPPFLAGS) $(abort_log_trap_CPPFLAGS)
nlassert_test_nonproduction_abort_backtrace_log_trap_CPPFLAGS = $(NONPRODUCTION_CPPFLAGS) $(abort_backtrace_log_trap_CPPFLAGS)

nlassert_static_test_production_success_CPPFLAGS              = $(PRODUCTION_CPPFLAGS) $(static_success_CPPFLAGS)
nlassert_static_test_production_failure_CPPFLAGS              = $(PRODUCTION_CPPFLAGS) $(static_failure_CPPFLAGS)
nlassert_static_test_cxx_production_success_CPPFLAGS          = $(PRODUCTION_CPPFLAGS) $(static_success_CPPFLAGS)
nlassert_static_test_cxx_production_failure_CPPFLAGS          = $(PRODUCTION_CPPFLAGS) $(static_failure_CPPFLAGS)

nlassert_test_production_defaults_CPPFLAGS                    = $(PRODUCTION_CPPFLAGS)    $(defaults_CPPFLAGS)                
nlassert_test_production_abort_CPPFLAGS                       = $(PRODUCTION_CPPFLAGS)    $(abort_CPPFLAGS)                   
nlassert_test_production_abort_backtrace_CPPFLAGS             = $(PRODUCTION_CPPFLAGS)    $(abort_backtrace_CPPFLAGS)         
nlassert_test_production_abort_log_CPPFLAGS                   = $(PRODUCTION_CPPFLAGS)    $(abort_log_CPPFLAGS)               
nlassert_test_production_abort_trap_CPPFLAGS                  = $(PRODUCTION_CPPFLAGS)    $(abort_trap_CPPFLAGS)              
nlassert_test_production_abort_backtrace_log_CPPFLAGS         = $(PRODUCTION_CPPFLAGS)    $(abort_backtrace_log_CPPFLAGS)     
nlassert_test_production_abort_backtrace_trap_CPPFLAGS        = $(PRODUCTION_CPPFLAGS)    $(abort_backtrace_trap_CPPFLAGS)    
nlassert_test_production_abort_log_trap_CPPFLAGS              = $(PRODUCTION_CPPFLAGS)    $(abort_log_trap_CPPFLAGS)          
nlassert_test_production_abort_backtrace_log_trap_CPPFLAGS    = $(PRODUCTION_CPPFLAGS)    $(abort_backtrace_log_trap_CPPFLAGS)

#
# Test Program Source Code
#

nlassert_static_test_nonproduction_success_SOURCES            = $(STATIC_SOURCES)
nlassert_static_test_nonproduction_failure_SOURCES            = $(STATIC_SOURCES)

nlassert_static_test_cxx_nonproduction_success_SOURCES        = $(STATIC_CXX_SOURCES)
nlassert_static_test_cxx_nonproduction_failure_SOURCES        = $(STATIC_CXX_SOURCES)

nlassert_test_nonproduction_defaults_SOURCES                  = $(COMMON_SOURCES)
nlassert_test_nonproduction_abort_SOURCES                     = $(COMMON_SOURCES)
nlassert_test_nonproduction_abort_backtrace_SOURCES           = $(COMMON_SOURCES)
nlassert_test_nonproduction_abort_log_SOURCES                 = $(COMMON_SOURCES)
nlassert_test_nonproduction_abort_trap_SOURCES                = $(COMMON_SOURCES)
nlassert_test_nonproduction_abort_backtrace_log_SOURCES       = $(COMMON_SOURCES)
nlassert_test_nonproduction_abort_backtrace_trap_SOURCES      = $(COMMON_SOURCES)
nlassert_test_nonproduction_abort_log_trap_SOURCES            = $(COMMON_SOURCES)
nlassert_test_nonproduction_abort_backtrace_log_trap_SOURCES  = $(COMMON_SOURCES)

nlassert_static_test_production_success_SOURCES               = $(STATIC_SOURCES)
nlassert_static_test_production_failure_SOURCES               = $(STATIC_SOURCES)

nlassert_static_test_cxx_production_success_SOURCES           = $(STATIC_CXX_SOURCES)
nlassert_static_test_cxx_production_failure_SOURCES           = $(STATIC_CXX_SOURCES)

nlassert_test_production_defaults_SOURCES                     = $(COMMON_SOURCES)
nlassert_test_production_abort_SOURCES                        = $(COMMON_SOURCES)
nlassert_test_production_abort_backtrace_SOURCES              = $(COMMON_SOURCES)
nlassert_test_production_abort_log_SOURCES                    = $(COMMON_SOURCES)
nlassert_test_production_abort_trap_SOURCES                   = $(COMMON_SOURCES)
nlassert_test_production_abort_backtrace_log_SOURCES          = $(COMMON_SOURCES)
nlassert_test_production_abort_backtrace_trap_SOURCES         = $(COMMON_SOURCES)
nlassert_test_production_abort_log_trap_SOURCES               = $(COMMON_SOURCES)
nlassert_test_production_abort_backtrace_log_trap_SOURCES     = $(COMMON_SOURCES)

if NLASSERT_BUILD_COVERAGE
CLEANFILES                                                    = $(wildcard *.gcda *.gcno)

if NLASSERT_BUILD_COVERAGE_REPORTS
# The bundle should positively be qualified with the absolute build
# path. Otherwise, VPATH will get auto-prefixed to it if there is
# already such a directory in the non-colocated source tree.

NLASSERT_COVERAGE_BUNDLE                                      = ${abs_builddir}/${PACKAGE}${NL_COVERAGE_BUNDLE_SUFFIX}
NLASSERT_COVERAGE_INFO                                        = ${NLASSERT_COVERAGE_BUNDLE}/${PACKAGE}${NL_COVERAGE_INFO_SUFFIX}

$(NLASSERT_COVERAGE_BUNDLE):
	$(call create-directory)

$(NLASSERT_COVERAGE_INFO): check | $(dir $(NLASSERT_COVERAGE_INFO))
	$(call generate-coverage-report,${top_builddir})

coverage: $(NLASSERT_COVERAGE_INFO)

clean-local: clean-local-coverage

.PHONY: clean-local-coverage
clean-local-coverage:
	-$(AM_V_at)rm -rf $(NLASSERT_COVERAGE_BUNDLE)
endif # NLASSERT_BUILD_COVERAGE_REPORTS
endif # NLASSERT_BUILD_COVERAGE
endif # NLASSERT_BUILD_TESTS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
