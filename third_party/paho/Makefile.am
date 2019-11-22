#
#  Copyright (c) 2019, Vit Holasek.
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

EXTRA_DIST                                    = \
    paho/MQTTSNPacket/src                       \
    $(NULL)

lib_LIBRARIES                                 = libpaho.a

# Do not enable -pedantic-errors for paho
override CFLAGS                              := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -pedantic-errors,$(CXXFLAGS))

# Do not enable -Wshadow for paho
override CFLAGS                              := $(filter-out -Wshadow,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -Wshadow,$(CXXFLAGS))

# Do not enable -Wextra for paho
override CFLAGS                              := $(filter-out -Wextra,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -Wextra,$(CXXFLAGS))

# Do not enable -Wsign-compare for paho
override CFLAGS                              := $(filter-out -Wsign-compare,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -Wsign-compare,$(CXXFLAGS))

PAHO_SRCDIR                                = $(top_srcdir)/third_party/paho/paho

libpaho_a_CPPFLAGS                      = \
    $(NULL)

libpaho_a_SOURCES                       = \
    paho/MQTTSNPacket/src/MQTTSNConnectClient.c         \
    paho/MQTTSNPacket/src/MQTTSNConnectServer.c         \
    paho/MQTTSNPacket/src/MQTTSNDeserializePublish.c    \
    paho/MQTTSNPacket/src/MQTTSNPacket.c                \
    paho/MQTTSNPacket/src/MQTTSNSearchClient.c          \
    paho/MQTTSNPacket/src/MQTTSNSearchServer.c          \
    paho/MQTTSNPacket/src/MQTTSNSerializePublish.c      \
    paho/MQTTSNPacket/src/MQTTSNSubscribeClient.c       \
    paho/MQTTSNPacket/src/MQTTSNSubscribeServer.c       \
    paho/MQTTSNPacket/src/MQTTSNUnsubscribeClient.c     \
    paho/MQTTSNPacket/src/MQTTSNUnsubscribeServer.c     \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
