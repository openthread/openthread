# Support AC_CONFIG_MACRO_DIRS with older autoconf.     -*- Autoconf -*-
# FIXME: To be removed in Automake 2.0, once we can assume autoconf
#        2.70 or later.
# FIXME: keep in sync with the contents of the variable
#        '$ac_config_macro_dirs_fallback' in aclocal.in.

# Copyright (C) 2012-2013 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

m4_ifndef([AC_CONFIG_MACRO_DIRS],
[m4_defun([_AM_CONFIG_MACRO_DIRS],[])]dnl
[m4_defun([AC_CONFIG_MACRO_DIRS],[_AM_CONFIG_MACRO_DIRS($@)])])
