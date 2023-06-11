#
#  Copyright (c) 2021, The OpenThread Authors.
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

project("TCPlp" C)

set(src_tcplp
    bsdtcp/cc/cc_newreno.c
    bsdtcp/tcp_fastopen.c
    bsdtcp/tcp_input.c
    bsdtcp/tcp_output.c
    bsdtcp/tcp_reass.c
    bsdtcp/tcp_sack.c
    bsdtcp/tcp_subr.c
    bsdtcp/tcp_timer.c
    bsdtcp/tcp_timewait.c
    bsdtcp/tcp_usrreq.c
    lib/bitmap.c
    lib/cbuf.c
    lib/lbuf.c
)


string(REPLACE "-Wsign-compare" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

string(REPLACE "-Wconversion" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

string(REPLACE "-Wunused-parameter" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

if(OT_FTD)
    add_library(tcplp-ftd STATIC ${src_tcplp})
    target_compile_options(tcplp-ftd
        PRIVATE
            "-Wno-sign-compare"
            "-Wno-unused-parameter"
    )
    set_target_properties(tcplp-ftd PROPERTIES OUTPUT_NAME tcplp-ftd)
    target_include_directories(tcplp-ftd
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/bsdtcp
            ${CMAKE_CURRENT_SOURCE_DIR}/lib
        PRIVATE
            ${OT_PUBLIC_INCLUDES}
    )

    target_link_libraries(tcplp-ftd
        PRIVATE
            ot-config
    )

    target_link_libraries(tcplp-ftd
        PRIVATE
            openthread-ftd
    )

endif()

if(OT_MTD)
    add_library(tcplp-mtd STATIC ${src_tcplp})
    target_compile_options(tcplp-mtd
        PRIVATE
            "-Wno-sign-compare"
            "-Wno-unused-parameter"
    )
    set_target_properties(tcplp-mtd PROPERTIES OUTPUT_NAME tcplp-mtd)
    target_include_directories(tcplp-mtd
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/bsdtcp
            ${CMAKE_CURRENT_SOURCE_DIR}/lib
        PRIVATE
            ${OT_PUBLIC_INCLUDES}
    )

    target_link_libraries(tcplp-mtd
        PRIVATE
            ot-config
    )

    target_link_libraries(tcplp-mtd
        PRIVATE
            openthread-mtd
    )

endif()
