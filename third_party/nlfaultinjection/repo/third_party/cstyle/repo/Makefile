#
#    Copyright (c) 2001-2016 Grant Erickson
#
#    Description:
#      Makefile for 'cstyle' coding style and conventions enforcement
#      tool.
#

TARGETS		= cstyle

SUBDIRS		= tests

#
# Implicit rules
#

.SUFFIXES: .pl

.pl:
	rm -f $@ $@-t
	cp $< $@-t
	chmod +x $@-t
	mv $@-t $@

all: all-recursive all-local

all-local: $(TARGETS)

check: check-recursive

install: install-recursive

all-recursive clean-recursive check-recursive install-recursive:
	@for subdir in $(SUBDIRS); do \
	  target=`echo $@ | sed s/-recursive//`; \
	  echo "Making $$target in $$subdir"; \
	  (cd $$subdir && $(MAKE) $$target) \
	   || case "$(MFLAGS)" in *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

clean: clean-recursive
	rm -f $(TARGETS)
	rm -f *.CKP *.ln *.BAK *.bak core errs ,* *.a *.o .emacs_* \
	tags TAGS make.log .*make.state MakeOut *~ "#"*
