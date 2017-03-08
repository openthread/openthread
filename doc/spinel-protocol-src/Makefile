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

XML2RFC_CACHE_DIR ?= $(HOME)/.cache/xml2rfc

TOOL_PREFIX        = $(DOCKER) run --rm --user=`id -u`:`id -g` -v `pwd`:/rfc -v $(XML2RFC_CACHE_DIR):/var/cache/xml2rfc paulej/rfctools

DOCKER            ?= docker
MD2RFC            ?= $(TOOL_PREFIX) md2rfc
XML2RFC           ?= $(TOOL_PREFIX) xml2rfc
MMARK             ?= $(TOOL_PREFIX) mmark
SED               ?= sed
RM_F		      ?= rm -f
MKDIR_P		      ?= mkdir -p

SOURCE_DATE       := $(shell (TZ=UTC git log -n 1 --date=iso-strict-local --pretty=format:%ad 2> /dev/null || date -u +"%Y-%m-%dT%H:%M:%SZ" ) | sed 's/+00:00$$/Z/')
SOURCE_VERSION    := $(shell git describe --dirty --always --match "--PoIsOn--" 2> /dev/null)

# -------------

SRC   := $(wildcard draft-*.md) $(wildcard draft-*.md.in)
XML   := $(patsubst %.md,%.xml,$(patsubst %.md.in,%.xml,$(SRC)))
TXT   := $(patsubst %.md,%.txt,$(patsubst %.md.in,%.txt,$(SRC)))
HTML  := $(patsubst %.md,%.html,$(patsubst %.md.in,%.html,$(SRC)))

all: $(XML) $(TXT) $(HTML)

clean:
	$(RM_F) $(XML) $(TXT) $(HTML) $(patsubst %.md.in,%.md,$(wildcard draft-*.md.in))

$(XML2RFC_CACHE_DIR):
	$(MKDIR_P) "$(XML2RFC_CACHE_DIR)"

%.md: %.md.in
	$(SED) 's/@SOURCE_VERSION@/$(SOURCE_VERSION)/g;s/@SOURCE_DATE@/$(SOURCE_DATE)/g' < $< > $@

%.xml: %.md
	$(MMARK) -xml2 -page $< $@

%.html: %.xml $(XML2RFC_CACHE_DIR)
	$(XML2RFC) --html $<

%.txt: %.xml $(XML2RFC_CACHE_DIR)
	$(XML2RFC) --text $<

# -------------

draft-spinel-protocol-bis.xml: \
	draft-spinel-protocol-bis.md \
	spinel-commands.md \
	spinel-data-packing.md \
	spinel-example-sessions.md \
	spinel-feature-host-buffer-offload.md \
	spinel-feature-network-save.md \
	spinel-frame-format.md \
	spinel-framing.md \
	spinel-prop-core.md \
	spinel-prop-debug.md \
	spinel-prop-ipv6.md \
	spinel-prop-mac.md \
	spinel-prop-net.md \
	spinel-prop-phy.md \
	spinel-prop.md \
	spinel-status-codes.md \
	spinel-tech-thread.md \
	spinel-test-vectors.md \
	$(NULL)
