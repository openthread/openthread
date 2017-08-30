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
#    Description:
#      This file is the GNU automake template for the Nest Labs
#      runtime assertion library public headers.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

# Package public headers that need no additional transforms on installation.

include_HEADERS =             \
    nlassert.h                \
    nlassert-internal.h       \
    nlassert-nonproduction.h  \
    nlassert-production.h     \
    $(NULL)

# Package public headers, for Standard C Library workalike
# functionality, that require additional relocation transform on
# installation.

nlassert_stdc_source_dirstem                 = stdc
nlassert_stdc_destination_dirstem            = stdc

nlassert_stdc_header_sources                 = \
    $(nlassert_stdc_source_dirstem)/assert.h   \
    $(NULL)

nlassert_stdcdir                             = \
    $(includedir)/$(nlassert_stdc_destination_dirstem)

dist_nlassert_stdc_HEADERS                   = \
    $(nlassert_stdc_header_sources)

install-headers: install-data

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
