#
#    Copyright 2015-2016 Nest Labs Inc. All Rights Reserved.
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
#      This file is the GNU automake template for the Nest memory-
#      mapped I/O library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

AM_MAKEFLAGS                        = --no-print-directory

SUBDIRS                             = \
    src                               \
    doc                               \
    $(NULL)

EXTRA_DIST                          = \
    CONTRIBUTING.md                   \
    LICENSE                           \
    README.md                         \
    .default-version                  \
    bootstrap                         \
    bootstrap-configure               \
    $(srcdir)/third_party             \
    $(NULL)

BUILT_SOURCES                       = \
    .local-version                    \
    $(NULL)

dist_doc_DATA                       = \
    $(NULL)

DISTCLEANFILES                      = \
    .local-version                    \
    $(NULL)

#
# Package version files:
#
# .default-version - The default package version. This file is ALWAYS checked
#                    in and should always represent the current baseline
#                    version of the package.
#
# .dist-version    - The distributed package version. This file is NEVER
#                    checked in within the upstream repository, is auto-
#                    generated, and is only found in the package distribution.
#
# .local-version   - The current source code controlled package version. This
#                    file is NEVER checked in within the upstream repository,
#                    is auto-generated, and can always be found in both the
#                    build tree and distribution.
#
# When present, the .local-version file is preferred first, the
# .dist-version second, and the .default-version last.
#

VERSION_FILE                      := $(if $(wildcard $(builddir)/.local-version),$(builddir)/.local-version,$(if $(wildcard $(srcdir)/.dist-version),$(srcdir)/.dist-version,$(srcdir)/.default-version))

#
# Override autotool's default notion of the package version variables.
# This ensures that when we create a source distribution that the
# version is always the current version, not the version when the
# package was bootstrapped.
#

NLUNITTEST_VERSION                ?= $(shell cat $(VERSION_FILE) 2> /dev/null)

PACKAGE_VERSION                    = $(NLUNITTEST_VERSION)
VERSION                            = $(PACKAGE_VERSION)

#
# check-file-.local-version
#
# Speculatively regenerate .local-version and check to see if it needs
# to be updated.
#
# If NLUNITTEST_VERSION has been supplied anywhere other than in this file
# (which is implicitly the contents of .local-version), then use that;
# otherwise, attempt to generate it from the SCM system.
#
# This is called from $(call check-file,.local-version).
#
define check-file-.local-version
if [ "$(origin NLUNITTEST_VERSION)" != "file" ]; then  \
    echo "$(NLUNITTEST_VERSION)" > "$(2)";             \
else                                                   \
    $(abs_top_nlbuild_autotools_dir)/scripts/mkversion \
        -b "$(NLUNITTEST_VERSION)" "$(top_srcdir)"     \
        > "$(2)";                                      \
fi
endef

#
# check-file-.dist-version
#
# Speculatively regenerate .dist-version and check to see if it needs
# to be updated.
#
# This is called from $(call check-file,.dist-version).
#
define check-file-.dist-version
cat "$(1)" > "$(2)"
endef

#
# A convenience target to allow package users to easily rerun the
# package configuration according to the current configuration.
#
.PHONY: reconfigure
reconfigure: $(builddir)/config.status
	$(AM_V_at)$(<) --recheck

#
# Version file regeneration rules.
#
.PHONY: force

$(builddir)/.local-version: $(srcdir)/.default-version force

$(distdir)/.dist-version: $(builddir)/.local-version force

$(distdir)/.dist-version $(builddir)/.local-version:
	$(call check-file,$(@F))

dist distcheck: $(BUILT_SOURCES)

dist-hook: $(distdir)/.dist-version

#
# Top-level convenience target for making a documentation-only
# distribution whose results appear at the top level of the build tree
# in the same fashion that the distribution would be for 'make dist'.
#

.PHONY: docdist
docdist: $(BUILT_SOURCES)
	$(MAKE) -C doc docdistdir=$(abs_builddir) $(@)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
