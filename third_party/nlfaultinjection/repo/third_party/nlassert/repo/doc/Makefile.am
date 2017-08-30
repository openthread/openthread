#
#    Copyright (c) 2015 Nest Labs, Inc.
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
#      runtime assertion library in-package, documentation.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

EXTRA_DIST                                      = \
    $(srcdir)/Doxyfile.in                         \
    $(NULL)

#
# Override autotool's default notion of the package version variables.
# This ensures that when we create a doc distribution that the
# version is always the current version, not the version when the
# package was bootstrapped.
#
PACKAGE_VERSION                                 = $(shell cat $(top_builddir)/.local-version)
VERSION                                         = $(PACKAGE_VERSION)


docdistdir                                     ?= .

nlassert_docdist_alias                          = \
    $(PACKAGE_TARNAME)-docs

nlassert_docdist_name                           = \
    $(nlassert_docdist_alias)-$(VERSION)

nlassert_docdist_archive                        = \
    $(docdistdir)/$(nlassert_docdist_name).tar.gz

CLEANFILES                                      = \
    Doxyfile                                      \
    $(nlassert_docdist_archive)                   \
    $(NULL)

if NLASSERT_BUILD_DOCS

all-local: html/index.html

#
# We choose to manually transform Doxyfile.in into Doxyfile here in
# the makefile rather than in the configure script so that we can take
# advantage of live, at build time (rather than at configure time),
# updating of the package version number.
#

Doxyfile: $(srcdir)/Doxyfile.in Makefile
	$(AM_V_GEN)$(SED)                                     \
	    -e "s,\@DOXYGEN_USE_DOT\@,$(DOXYGEN_USE_DOT),g"   \
	    -e "s,\@PACKAGE_NAME\@,$(PACKAGE_NAME),g"         \
	    -e "s,\@PACKAGE_VERSION\@,$(PACKAGE_VERSION),g"   \
	    -e "s,\@PERL\@,$(PERL),g"                         \
	    -e "s,\@abs_builddir\@,$(abs_builddir),g"         \
	    -e "s,\@abs_srcdir\@,$(abs_srcdir),g"             \
	    -e "s,\@abs_top_builddir\@,$(abs_top_builddir),g" \
	    -e "s,\@abs_top_srcdir\@,$(abs_top_srcdir),g"     \
	    < "$(srcdir)/Doxyfile.in" > "$(@)"

html/index.html: Doxyfile
	$(AM_V_GEN)$(DOXYGEN) $(<)

#
# Addition rules and commands to create a documentation-only
# distribution of nlassert
#

$(nlassert_docdist_name): html/index.html
	$(AM_V_at)rm -f -r $(@)
	$(call create-directory)
	$(AM_V_at)cp -R html $(@)

$(nlassert_docdist_archive): $(nlassert_docdist_name)
	$(AM_V_at)echo "  TAR      $(@)"
	$(AM_V_at)tardir="$(<)" && $(am__tar) | GZIP=$(GZIP_ENV) gzip -c > "$(@)" && rm -rf $(<)

docdist $(nlassert_docdist_alias): $(nlassert_docdist_archive)

clean-local:
	$(AM_V_at)rm -f -r html

endif # NLASSERT_BUILD_DOCS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
