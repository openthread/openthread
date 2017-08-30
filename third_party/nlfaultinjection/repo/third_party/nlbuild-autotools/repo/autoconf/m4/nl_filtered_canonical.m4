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
#      This file defines a GNU autoconf M4-style macro for filtering
#      the autoconf canonical build, host, or target.
#
#      Mac OS X / Darwin ends up putting some versioning cruft on the
#      end of its tuples that most users of these variables rarely
#      care about.
#

#
# _NL_FILTERED_CANONICAL(name)
#
#   name - The existing autoconf variable to filter
#
#   Mac OS X / Darwin ends up putting some versioning cruft on the end
#   of its tuples that most users of these variables rarely care about.
#
#   This filters such versioning cruft from the variable 'name'
#   generated from AC_CANONICAL_<NAME> and saves it in
#   'nl_filtered_<name>'.
#
_NL_FILTERED_CANONICAL(name)
AC_DEFUN([_NL_FILTERED_CANONICAL],
[
    AC_CACHE_CHECK([filtered $1 system type],
        nl_cv_filtered_$1,
        nl_cv_filtered_$1=`echo ${$1} | sed -e 's/[[[[:digit:].]]]*$//g'`
        nl_filtered_$1=${nl_cv_filtered_$1})
])

#
# NL_FILTERED_CANONICAL_BUILD
#
#   Mac OS X / Darwin ends up putting some versioning cruft on the end
#   of its tuples that most users of these variables rarely care about.
#
#   This filters such versioning cruft from the variable 'build'
#   generated from AC_CANONICAL_BUILD and saves it in
#   'nl_filtered_build'.
#
NL_FILTERED_CANONICAL_BUILD
AC_DEFUN([NL_FILTERED_CANONICAL_BUILD],
[
    AC_REQUIRE([AC_CANONICAL_BUILD])
    _NL_FILTERED_CANONICAL(build)
])

#
# NL_FILTERED_CANONICAL_HOST
#
#   Mac OS X / Darwin ends up putting some versioning cruft on the end
#   of its tuples that most users of these variables rarely care about.
#
#   This filters such versioning cruft from the variable 'host'
#   generated from AC_CANONICAL_HOST and saves it in
#   'nl_filtered_build'.
#
NL_FILTERED_CANONICAL_HOST
AC_DEFUN([NL_FILTERED_CANONICAL_HOST],
[
    AC_REQUIRE([AC_CANONICAL_HOST])
    _NL_FILTERED_CANONICAL(host)
])

#
# NL_FILTERED_CANONICAL_TARGET
#
#   Mac OS X / Darwin ends up putting some versioning cruft on the end
#   of its tuples that most users of these variables rarely care about.
#
#   This filters such versioning cruft from the variable 'target'
#   generated from AC_CANONICAL_TARGET and saves it in
#   'nl_filtered_target'.
#
NL_FILTERED_CANONICAL_TARGET
AC_DEFUN([NL_FILTERED_CANONICAL_TARGET],
[
    AC_REQUIRE([AC_CANONICAL_TARGET])
    _NL_FILTERED_CANONICAL(target)
])
