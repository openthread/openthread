#
#    Copyright (c) 2001-2016 Grant Erickson
#
#    Description:
#      Makefile for 'cstyle' coding style and conventions enforcement
#      tool unit tests.
#

.SUFFIXES: .c .cpp .cmp .mm .stamp .test

CC	 ?= gcc
CXX	 ?= g++
OBJC     ?= gcc
OBJCXX   ?= g++

ifeq ($(DEBUG),1)
COMPARE      ?= comm -3 --nocheck-order
else
COMPARE      ?= cmp -s
endif
PERL     ?= perl
CSTYLE   ?= $(PWD)/../cstyle.pl
CSTYLE_V := $(CSTYLE) -v

%.stamp: %.c
	@echo "  CC      $<"
	@$(CC) -o /dev/null -c $<
	@touch $@

%.stamp: %.cpp
	@echo "  CXX     $<"
	@$(CXX) -o /dev/null -c $<
	@touch $@

%.stamp: %.m
	@echo "  OBJC    $<"
	@$(OBJC) -o /dev/null -c $<
	@touch $@

%.stamp: %.mm
	@echo "  OBJCXX  $<"
	@$(OBJCXX) -o /dev/null -c $<
	@touch $@

SOURCES                                          = \
    cpp-constant-conditionals.cpp                  \
    cpp-directive-leading-space.cpp                \
    file-length.cpp                                \
    implicit-void-declaration.cpp                  \
    interpolated-space.cpp                         \
    line-length.cpp                                \
    missing-copyright.cpp                          \
    missing-cpp-conditional-labels.cpp             \
    missing-newline-at-eof.cpp                     \
    missing-space-after-comma.cpp                  \
    missing-space-after-else-if.cpp                \
    missing-space-after-for.cpp                    \
    missing-space-after-if.cpp                     \
    missing-space-after-operator.cpp               \
    missing-space-after-semicolon.cpp              \
    missing-space-after-switch.cpp                 \
    missing-space-after-while.cpp                  \
    missing-space-around-binary-operators.cpp      \
    missing-space-around-braces.mm                 \
    multiple-returns.c                             \
    multiple-returns.cpp                           \
    no-missing-copyright.cpp                       \
    space-around-unary-operators.cpp               \
    trailing-line.cpp                              \
    trailing-space.cpp                             \
    $(NULL)

STAMPS                                           = \
    $(addsuffix .stamp,$(basename $(SOURCES)))
    $(NULL)

check_TARGETS                                    = \
    check-blank-trailing-space                     \
    check-no-blank-trailing-space                  \
    check-cpp-constant-conditionals                \
    check-no-cpp-constant-conditionals             \
    check-cpp-directive-leading-space              \
    check-no-cpp-directive-leading-space           \
    check-file-length                              \
    check-no-file-length                           \
    check-implicit-void-declaration                \
    check-no-implicit-void-declaration             \
    check-interpolated-space                       \
    check-no-interpolated-space                    \
    check-line-length                              \
    check-no-line-length                           \
    check-missing-copyright                        \
    check-no-missing-copyright                     \
    check-missing-cpp-conditional-labels           \
    check-no-missing-cpp-conditional-labels        \
    check-missing-newline-at-eof                   \
    check-no-missing-newline-at-eof                \
    check-missing-space-after-comma                \
    check-no-missing-space-after-comma             \
    check-missing-space-after-else-if              \
    check-no-missing-space-after-else-if           \
    check-missing-space-after-for                  \
    check-no-missing-space-after-for               \
    check-missing-space-after-if                   \
    check-no-missing-space-after-if                \
    check-missing-space-after-operator             \
    check-no-missing-space-after-operator          \
    check-missing-space-after-semicolon            \
    check-no-missing-space-after-semicolon         \
    check-missing-space-after-switch               \
    check-no-missing-space-after-switch            \
    check-missing-space-after-while                \
    check-no-missing-space-after-while             \
    check-missing-space-around-binary-operators    \
    check-no-missing-space-around-binary-operators \
    check-missing-space-around-braces              \
    check-no-missing-space-around-braces           \
    check-space-around-unary-operators             \
    check-no-space-around-unary-operators          \
    check-trailing-line                            \
    check-no-trailing-line                         \
    check-trailing-space                           \
    check-no-trailing-space                        \
    $(NULL)

all: $(STAMPS)

.PHONY: $(check_TARGETS)
check: $(check_TARGETS)

define compare-target
$(COMPARE) $(basename $(1)).cmp $(1)
endef

# -W[no-]blank-trailing-space

blank-trailing-space.test: blank-trailing-space.cpp blank-trailing-space.cmp blank-trailing-space.stamp $(CSTYLE)
	@echo "  CHECK   -Wblank-trailing-space..."
	@$(PERL) $(CSTYLE_V) -Wblank-trailing-space blank-trailing-space.cpp 2> $(@)
	@$(call compare-target,$(@))

check-blank-trailing-space: blank-trailing-space.test

no-blank-trailing-space.test: blank-trailing-space.cpp no-blank-trailing-space.cmp blank-trailing-space.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-blank-trailing-space..."
	@$(PERL) $(CSTYLE_V) -Wno-blank-trailing-space blank-trailing-space.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-blank-trailing-space: no-blank-trailing-space.test

# -W[no-]missing-cpp-conditional-labels

missing-cpp-conditional-labels.test: missing-cpp-conditional-labels.cpp missing-cpp-conditional-labels.cmp missing-cpp-conditional-labels.stamp $(CSTYLE)
	@echo "  CHECK   -Wmissing-cpp-conditional-labels..."
	@$(PERL) $(CSTYLE_V) --cpp-lines-between-conditionals=1 -Wmissing-cpp-conditional-labels missing-cpp-conditional-labels.cpp 2> $(@)
	@$(call compare-target,$(@))

check-missing-cpp-conditional-labels: missing-cpp-conditional-labels.test

no-missing-cpp-conditional-labels.test: missing-cpp-conditional-labels.cpp no-missing-cpp-conditional-labels.cmp missing-cpp-conditional-labels.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-missing-cpp-conditional-labels..."
	@$(PERL) $(CSTYLE_V) --cpp-lines-between-conditionals=1 -Wno-missing-cpp-conditional-labels missing-cpp-conditional-labels.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-missing-cpp-conditional-labels: no-missing-cpp-conditional-labels.test

# -W[no-]cpp-constant-conditionals

cpp-constant-conditionals.test: cpp-constant-conditionals.cpp cpp-constant-conditionals.cmp cpp-constant-conditionals.stamp $(CSTYLE)
	@echo "  CHECK   -Wcpp-constant-conditionals..."
	@$(PERL) $(CSTYLE_V) -Wcpp-constant-conditionals cpp-constant-conditionals.cpp 2> $(@)
	@$(call compare-target,$(@))

check-cpp-constant-conditionals: cpp-constant-conditionals.test

no-cpp-constant-conditionals.test: cpp-constant-conditionals.cpp no-cpp-constant-conditionals.cmp cpp-constant-conditionals.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-cpp-constant-conditionals..."
	@$(PERL) $(CSTYLE_V) -Wno-cpp-constant-conditionals cpp-constant-conditionals.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-cpp-constant-conditionals: no-cpp-constant-conditionals.test

# -W[no-]cpp-directive-leading-space

cpp-directive-leading-space.test: cpp-directive-leading-space.cpp cpp-directive-leading-space.cmp cpp-directive-leading-space.stamp $(CSTYLE)
	@echo "  CHECK   -Wcpp-directive-leading-space..."
	@$(PERL) $(CSTYLE_V) -Wcpp-directive-leading-space cpp-directive-leading-space.cpp 2> $(@)
	@$(call compare-target,$(@))

check-cpp-directive-leading-space: cpp-directive-leading-space.test

no-cpp-directive-leading-space.test: cpp-directive-leading-space.cpp no-cpp-directive-leading-space.cmp cpp-directive-leading-space.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-cpp-directive-leading-space..."
	@$(PERL) $(CSTYLE_V) -Wno-cpp-directive-leading-space cpp-directive-leading-space.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-cpp-directive-leading-space: no-cpp-directive-leading-space.test

# -W[no-]file-length

file-length.test: file-length.cpp file-length.cmp file-length.stamp $(CSTYLE)
	@echo "  CHECK   -Wfile-length..."
	@$(PERL) $(CSTYLE_V) --file-length=5 -Wfile-length file-length.cpp 2> $(@)
	@$(call compare-target,$(@))

check-file-length: file-length.test

no-file-length.test: file-length.cpp no-file-length.cmp file-length.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-file-length..."
	@$(PERL) $(CSTYLE_V) --file-length=5 -Wno-file-length file-length.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-file-length: no-file-length.test

# -W[no-]implicit-void-declaration

implicit-void-declaration.test: implicit-void-declaration.cpp implicit-void-declaration.cmp implicit-void-declaration.stamp $(CSTYLE)
	@echo "  CHECK   -Wimplicit-void-declaration..."
	@$(PERL) $(CSTYLE_V) -Wimplicit-void-declaration implicit-void-declaration.cpp 2> $(@)
	@$(call compare-target,$(@))

check-implicit-void-declaration: implicit-void-declaration.test

no-implicit-void-declaration.test: implicit-void-declaration.cpp no-implicit-void-declaration.cmp implicit-void-declaration.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-implicit-void-declaration..."
	@$(PERL) $(CSTYLE_V) -Wno-implicit-void-declaration implicit-void-declaration.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-implicit-void-declaration: no-implicit-void-declaration.test

# -W[no-]interpolated-space

interpolated-space.test: interpolated-space.cpp interpolated-space.cmp interpolated-space.stamp $(CSTYLE)
	@echo "  CHECK   -Winterpolated-space..."
	@$(PERL) $(CSTYLE_V) -Winterpolated-space interpolated-space.cpp 2> $(@)
	@$(call compare-target,$(@))

check-interpolated-space: interpolated-space.test

no-interpolated-space.test: interpolated-space.cpp no-interpolated-space.cmp interpolated-space.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-interpolated-space..."
	@$(PERL) $(CSTYLE_V) -Wno-interpolated-space interpolated-space.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-interpolated-space: no-interpolated-space.test

# -W[no-]line-length

line-length.test: line-length.cpp line-length.cmp line-length.stamp $(CSTYLE)
	@echo "  CHECK   -Wline-length..."
	@$(PERL) $(CSTYLE_V) --line-length=80 --tab-size=4 -Wline-length line-length.cpp 2> $(@)
	@$(call compare-target,$(@))

check-line-length: line-length.test

no-line-length.test: line-length.cpp no-line-length.cmp line-length.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-line-length..."
	@$(PERL) $(CSTYLE_V) --line-length=80 --tab-size=4 -Wno-line-length line-length.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-line-length: no-line-length.test

# -W[no-]missing-copyright

missing-copyright.test: missing-copyright.cpp missing-copyright.cmp missing-copyright.stamp $(CSTYLE)
	@echo "  CHECK   -Wmissing-copyright..."
	@$(PERL) $(CSTYLE_V) --copyright='^( \*|\/\*|\/\/)    Copyright (\(c\) )*\d{4,}(-\d{4,})* Nest Labs, Inc\.' -Wmissing-copyright missing-copyright.cpp 2> $(@)
	@$(call compare-target,$(@))

check-missing-copyright: missing-copyright.test

no-missing-copyright.test: no-missing-copyright.cpp no-missing-copyright.cmp no-missing-copyright.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-missing-copyright..."
	@$(PERL) $(CSTYLE_V) --copyright='^( \*|\/\*|\/\/)    Copyright (\(c\) )*\d{4,}(-\d{4,})* Nest Labs, Inc\.' -Wno-missing-copyright no-missing-copyright.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-missing-copyright: no-missing-copyright.test

# -W[no-]missing-newline-at-eof

missing-newline-at-eof.test: missing-newline-at-eof.cpp missing-newline-at-eof.cmp missing-newline-at-eof.stamp $(CSTYLE)
	@echo "  CHECK   -Wmissing-newline-at-eof..."
	@$(PERL) $(CSTYLE_V) -Wmissing-newline-at-eof missing-newline-at-eof.cpp 2> $(@)
	@$(call compare-target,$(@))

check-missing-newline-at-eof: missing-newline-at-eof.test

no-missing-newline-at-eof.test: missing-newline-at-eof.cpp no-missing-newline-at-eof.cmp missing-newline-at-eof.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-missing-newline-at-eof..."
	@$(PERL) $(CSTYLE_V) -Wno-missing-newline-at-eof missing-newline-at-eof.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-missing-newline-at-eof: no-missing-newline-at-eof.test

# -W[no-]missing-space-after-comma

missing-space-after-comma.test: missing-space-after-comma.cpp missing-space-after-comma.cmp missing-space-after-comma.stamp $(CSTYLE)
	@echo "  CHECK   -Wmissing-space-after-comma..."
	@$(PERL) $(CSTYLE_V) -Wmissing-space-after-comma missing-space-after-comma.cpp 2> $(@)
	@$(call compare-target,$(@))

check-missing-space-after-comma: missing-space-after-comma.test

no-missing-space-after-comma.test: missing-space-after-comma.cpp no-missing-space-after-comma.cmp missing-space-after-comma.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-missing-space-after-comma..."
	@$(PERL) $(CSTYLE_V) -Wno-missing-space-after-comma missing-space-after-comma.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-missing-space-after-comma: no-missing-space-after-comma.test

# -W[no-]missing-space-after-else-if

missing-space-after-else-if.test: missing-space-after-else-if.cpp missing-space-after-else-if.cmp missing-space-after-else-if.stamp $(CSTYLE)
	@echo "  CHECK   -Wmissing-space-after-else-if..."
	@$(PERL) $(CSTYLE_V) -Wmissing-space-after-else-if missing-space-after-else-if.cpp 2> $(@)
	@$(call compare-target,$(@))

check-missing-space-after-else-if: missing-space-after-else-if.test

no-missing-space-after-else-if.test: missing-space-after-else-if.cpp no-missing-space-after-else-if.cmp missing-space-after-else-if.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-missing-space-after-else-if..."
	@$(PERL) $(CSTYLE_V) -Wno-missing-space-after-else-if missing-space-after-else-if.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-missing-space-after-else-if: no-missing-space-after-else-if.test

# -W[no-]missing-space-after-for

missing-space-after-for.test: missing-space-after-for.cpp missing-space-after-for.cmp missing-space-after-for.stamp $(CSTYLE)
	@echo "  CHECK   -Wmissing-space-after-for..."
	@$(PERL) $(CSTYLE_V) -Wmissing-space-after-for missing-space-after-for.cpp 2> $(@)
	@$(call compare-target,$(@))

check-missing-space-after-for: missing-space-after-for.test

no-missing-space-after-for.test: missing-space-after-for.cpp no-missing-space-after-for.cmp missing-space-after-for.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-missing-space-after-for..."
	@$(PERL) $(CSTYLE_V) -Wno-missing-space-after-for missing-space-after-for.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-missing-space-after-for: no-missing-space-after-for.test

# -W[no-]missing-space-after-if

missing-space-after-if.test: missing-space-after-if.cpp missing-space-after-if.cmp missing-space-after-if.stamp $(CSTYLE)
	@echo "  CHECK   -Wmissing-space-after-if..."
	@$(PERL) $(CSTYLE_V) -Wmissing-space-after-if missing-space-after-if.cpp 2> $(@)
	@$(call compare-target,$(@))

check-missing-space-after-if: missing-space-after-if.test

no-missing-space-after-if.test: missing-space-after-if.cpp no-missing-space-after-if.cmp missing-space-after-if.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-missing-space-after-if..."
	@$(PERL) $(CSTYLE_V) -Wno-missing-space-after-if missing-space-after-if.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-missing-space-after-if: no-missing-space-after-if.test

# -W[no-]missing-space-after-operator

missing-space-after-operator.test: missing-space-after-operator.cpp missing-space-after-operator.cmp missing-space-after-operator.stamp $(CSTYLE)
	@echo "  CHECK   -Wmissing-space-after-operator..."
	@$(PERL) $(CSTYLE_V) -Wmissing-space-after-operator missing-space-after-operator.cpp 2> $(@)
	@$(call compare-target,$(@))

check-missing-space-after-operator: missing-space-after-operator.test

no-missing-space-after-operator.test: missing-space-after-operator.cpp no-missing-space-after-operator.cmp missing-space-after-operator.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-missing-space-after-operator..."
	@$(PERL) $(CSTYLE_V) -Wno-missing-space-after-operator missing-space-after-operator.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-missing-space-after-operator: no-missing-space-after-operator.test

# -W[no-]missing-space-after-semicolon

missing-space-after-semicolon.test: missing-space-after-semicolon.cpp missing-space-after-semicolon.cmp missing-space-after-semicolon.stamp $(CSTYLE)
	@echo "  CHECK   -Wmissing-space-after-semicolon..."
	@$(PERL) $(CSTYLE_V) -Wmissing-space-after-semicolon missing-space-after-semicolon.cpp 2> $(@)
	@$(call compare-target,$(@))

check-missing-space-after-semicolon: missing-space-after-semicolon.test

no-missing-space-after-semicolon.test: missing-space-after-semicolon.cpp no-missing-space-after-semicolon.cmp missing-space-after-semicolon.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-missing-space-after-semicolon..."
	@$(PERL) $(CSTYLE_V) -Wno-missing-space-after-semicolon missing-space-after-semicolon.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-missing-space-after-semicolon: no-missing-space-after-semicolon.test

# -W[no-]missing-space-after-switch

missing-space-after-switch.test: missing-space-after-switch.cpp missing-space-after-switch.cmp missing-space-after-switch.stamp $(CSTYLE)
	@echo "  CHECK   -Wmissing-space-after-switch..."
	@$(PERL) $(CSTYLE_V) -Wmissing-space-after-switch missing-space-after-switch.cpp 2> $(@)
	@$(call compare-target,$(@))

check-missing-space-after-switch: missing-space-after-switch.test

no-missing-space-after-switch.test: missing-space-after-switch.cpp no-missing-space-after-switch.cmp missing-space-after-switch.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-missing-space-after-switch..."
	@$(PERL) $(CSTYLE_V) -Wno-missing-space-after-switch missing-space-after-switch.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-missing-space-after-switch: no-missing-space-after-switch.test

# -W[no-]missing-space-after-while

missing-space-after-while.test: missing-space-after-while.cpp missing-space-after-while.cmp missing-space-after-while.stamp $(CSTYLE)
	@echo "  CHECK   -Wmissing-space-after-while..."
	@$(PERL) $(CSTYLE_V) -Wmissing-space-after-while missing-space-after-while.cpp 2> $(@)
	@$(call compare-target,$(@))

check-missing-space-after-while: missing-space-after-while.test

no-missing-space-after-while.test: missing-space-after-while.cpp no-missing-space-after-while.cmp missing-space-after-while.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-missing-space-after-while..."
	@$(PERL) $(CSTYLE_V) -Wno-missing-space-after-while missing-space-after-while.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-missing-space-after-while: no-missing-space-after-while.test

# -W[no-]missing-space-around-binary-operators

missing-space-around-binary-operators.test: missing-space-around-binary-operators.cpp missing-space-around-binary-operators.cmp missing-space-around-binary-operators.stamp $(CSTYLE)
	@echo "  CHECK   -Wmissing-space-around-binary-operators..."
	@$(PERL) $(CSTYLE_V) -Wmissing-space-around-binary-operators missing-space-around-binary-operators.cpp 2> $(@)
	@$(call compare-target,$(@))

check-missing-space-around-binary-operators: missing-space-around-binary-operators.test

no-missing-space-around-binary-operators.test: missing-space-around-binary-operators.cpp no-missing-space-around-binary-operators.cmp missing-space-around-binary-operators.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-missing-space-around-binary-operators..."
	@$(PERL) $(CSTYLE_V) -Wno-missing-space-around-binary-operators missing-space-around-binary-operators.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-missing-space-around-binary-operators: no-missing-space-around-binary-operators.test

# -W[no-]missing-space-around-braces

missing-space-around-braces.test: missing-space-around-braces.mm missing-space-around-braces.cmp missing-space-around-braces.stamp $(CSTYLE)
	@echo "  CHECK   -Wmissing-space-around-braces..."
	@$(PERL) $(CSTYLE_V) -Wmissing-space-around-braces missing-space-around-braces.mm 2> $(@)
	@$(call compare-target,$(@))

check-missing-space-around-braces: missing-space-around-braces.test

no-missing-space-around-braces.test: missing-space-around-braces.mm no-missing-space-around-braces.cmp missing-space-around-braces.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-missing-space-around-braces..."
	@$(PERL) $(CSTYLE_V) -Wno-missing-space-around-braces missing-space-around-braces.mm 2> $(@)
	@$(call compare-target,$(@))

check-no-missing-space-around-braces: no-missing-space-around-braces.test

# -W[no-]space-around-unary-operators

space-around-unary-operators.test: space-around-unary-operators.cpp space-around-unary-operators.cmp space-around-unary-operators.stamp $(CSTYLE)
	@echo "  CHECK   -Wspace-around-unary-operators..."
	@$(PERL) $(CSTYLE_V) -Wspace-around-unary-operators space-around-unary-operators.cpp 2> $(@)
	@$(call compare-target,$(@))

check-space-around-unary-operators: space-around-unary-operators.test

no-space-around-unary-operators.test: space-around-unary-operators.cpp no-space-around-unary-operators.cmp space-around-unary-operators.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-space-around-unary-operators..."
	@$(PERL) $(CSTYLE_V) -Wno-space-around-unary-operators space-around-unary-operators.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-space-around-unary-operators: no-space-around-unary-operators.test

# -W[no-]trailing-line

trailing-line.test: trailing-line.cpp trailing-line.cmp trailing-line.stamp $(CSTYLE)
	@echo "  CHECK   -Wtrailing-line..."
	@$(PERL) $(CSTYLE_V) -Wtrailing-line trailing-line.cpp 2> $(@)
	@$(call compare-target,$(@))

check-trailing-line: trailing-line.test

no-trailing-line.test: trailing-line.cpp no-trailing-line.cmp trailing-line.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-trailing-line..."
	@$(PERL) $(CSTYLE_V) -Wno-trailing-line trailing-line.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-trailing-line: no-trailing-line.test

# -W[no-]trailing-space

trailing-space.test: trailing-space.cpp trailing-space.cmp trailing-space.stamp $(CSTYLE)
	@echo "  CHECK   -Wtrailing-space..."
	@$(PERL) $(CSTYLE_V) -Wtrailing-space trailing-space.cpp 2> $(@)
	@$(call compare-target,$(@))

check-trailing-space: trailing-space.test

no-trailing-space.test: trailing-space.cpp no-trailing-space.cmp trailing-space.stamp $(CSTYLE)
	@echo "  CHECK   -Wno-trailing-space..."
	@$(PERL) $(CSTYLE_V) -Wno-trailing-space trailing-space.cpp 2> $(@)
	@$(call compare-target,$(@))

check-no-trailing-space: no-trailing-space.test

install:
	true

clean:
	@echo "Cleaning..."
	@rm -f $(STAMPS) *.test
	@rm -f *.CKP *.ln *.BAK *.bak core errs ,* *.a *.o .emacs_* \
	tags TAGS make.log .*make.state MakeOut *~ "#"*
