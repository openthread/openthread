#!/usr/bin/perl

#
#    Copyright (c) 2001-2017 Grant Erickson
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#
#    Description:
#      This program is used to flexibly and parametrically check for code
#      formatting style compliance.
#
#      Philosophically, this assumes it is working on compilable
#      code. Consequently, this does not endeavor to be nor is it a
#      grammatically-correct source code parser.
#
#      Where possible, effort is expended to make the syntax, option
#      invocation, and output familiar to users of compilers and other
#      linting and formatting tools for ease of use and efficient
#      system integration.
#

use strict 'refs';

use File::Basename;
use File::Path;
use Getopt::Long qw(:config gnu_getopt);
use POSIX;

# Global Variables

my($program);

# Default program options. Eventually, the vision is to allow these to
# be overridden by command-line options and via a global
# (e.g. /usr/local/etc/style.conf) or local (e.g. ~/.style) or
# arbitrary configuration file. However, at present, only command-line
# options are supported since loading and parsing configuration files
# will, undoubtably, slow the program down considerably.

my(%defaults) = ();

my(%options) = ();

my(%warnings) = ();

# Mappings of supported languages to style handlers. Supported
# languages as translated by file extension or via the '-x' option. In
# the absence of any better nomenclature, the language names from GCC
# are used.

my(%handlers) = (
		 "assembler",			\&asmstyle,
		 "assembler-with-cpp",		\&asmstyle,
		 "c",				\&cstyle,
		 "c-header",			\&cstyle,
		 "c++",				\&cxxstyle,
		 "c++-header",			\&cxxstyle,
		 "objective-c",			\&objcstyle,
		 "objective-c-header",		\&objcstyle,
		 "objective-c++",		\&objcxxstyle,
		 "objective-c++-header",	\&objcxxstyle
		 );

my(@languages) = sort(keys(%handlers));

# Mappings of file extensions to source language type. As with the list
# of supported languages, language names from GCC are used.

my(%extensions) = (
		   "C",		"c++",
		   "H",		"c++-header",
		   "M",		"objective-c++",
		   "S",		"assembler-with-cpp",
		   "c",		"c",
		   "c++",	"c++",
		   "cc",	"c++",
		   "cp",	"c++",
		   "cpp",	"c++",	
		   "cxx",	"c++",
		   "h",		"c-header",
		   "hh",	"c++-header",
		   "hpp",	"c++-header",
		   "m",		"objective-c",
		   "mm",	"objective-c++",
		   "s",		"assembler"
		  );

# Some freqeuently-used, precompiled regular expression patterns.

my($storage_specifiers_re) = qr/((extern|static)\s+)/;
my($type_qualifiers_re) = qr/((const|volatile)\s+)/;
my($method_qualifiers_re) = qr/((const|volatile)\s*)/;
my($type_declarator_re) = qr/([_[:alpha:]][_[:alnum:]]*\s*)/;
my($type_re) = qr/$type_qualifiers_re*$type_declarator_re*\s*($type_qualifiers_re*[*&])*/;
my($function_declarator_re) = qr/([_~[:alpha:]][_[:alnum:]]*)/;
my($argument_declarator_re) = qr/([_[:alpha:]][_[:alnum:]]*)/;
my($array_declarator_re) = qr/(\[\w+\])/;
my($argument_re) = qr/$type_re\s*($argument_declarator_re\s*$array_declarator_re*)*/;
my($argument_list_re) = qr/(void|($argument_re\s*,*\s*)*)/;
my($function_declaration_re) = qr/$storage_specifiers_re*\s*$type_re\s*$function_declarator_re\s*\($argument_list_re\)\s*$method_qualifiers_re*;\s*/;

#
# usage()
#
# Description:
#   This routine prints out the proper command line usage for this program
#
# Input(s):
#   status - Flag determining what usage information will be printed and what
#            the exit status of the program will be after the information is
#            printed.
#
# Output(s):
#   N/A
#
# Returns:
#   This subroutine does not return.
#
sub usage {
  my($status) = $_[0];

  print(STDERR "Usage: style [ options... ] [ files... ]\n");

  if ($status != 0) {
    print(STDERR "Try `style --help' for more information.\n");
  }

  if ($status != 1) {
    my($usage) =
"General Options:
  -d, --debug                Display debug execution information.
  --help                     Display this information.
  -v, --verbose              Display verbose execution information.
  --version                  Display version information.
  -W<WARNING>                Select a comma-separated WARNING option.
  -x, --language=<LANGUAGE>  Specify LANGUAGE as source language of input
                             files.

Language Options:
  Permissible languages include:
    %s

    'none' means revert to the default behavior of guessing the language
    based on the input file's extension.

Style Options:
  --copyright=<PATTERN>           Copyright pattern to search for.
  --file-length=<LENGTH>          Maximum file length is LENGTH lines.
  --line-length=<LENGTH>          Maximum line length is LENGTH characters.
  --tab-size=<SIZE>               Interpret tab characters as SIZE spaces.
  --cpp-lines-between-conditionals=<LINES>
                                  Initiate preprocessor conditional checks when
                                  separation between conditionals is more than
                                  LINES lines.

Warning Options:
  -Wall                           Enable all warning options.
  -Werror                         Make all warnings into errors.
  -Wimplicit-void-declaration     Warn about implicit void declarations [EXPERIMENTAL].
  -Winterpolated-space            Warn about interpolated spaces and tabs.
  -Wfile-length                   Warn about long files.
  -Wline-length                   Warn about long lines.
  -Wtrailing-line                 Warn about blank line(s) at the end of a file.
  -Wblank-trailing-space          Warn about white space at the end of a blank line.
  -Wtrailing-space                Warn about white space at the end of a non-blank
                                  line.
  -Wmissing-cpp-conditional-labels
                                  Warn about missing labels on conditionals.
  -Wcpp-constant-conditionals
                                  Warn about constant conditional expressions.
  -Wcpp-directive-leading-space
                                  Warn about leading space before directives.
  -Wmultiple-returns
                                  Warn about multiple return statements per function or method.
  -Wmissing-copyright             Warn about a missing copyright declaration.
  -Wmissing-newline-at-eof        Warn about missing new line at the end of a file.
  -Wmissing-space-after-comma     Warn about missing space after a comma.
  -Wmissing-space-after-else-if   Warn about missing space after the 'else if' keyword
  -Wmissing-space-after-for       Warn about missing space after the 'for' keyword
  -Wmissing-space-after-if        Warn about missing space after the 'if' keyword
  -Wmissing-space-after-operator  Warn about missing space after the 'operator' keyword.
  -Wmissing-space-after-semicolon
                                  Warn about missing space after a semicolon.
  -Wmissing-space-after-switch    Warn about missing space after the 'switch' keyword
  -Wmissing-space-after-while     Warn about missing space after the 'while' keyword
  -Wmissing-space-around-binary-operators
                                  Warn about missing space around binary operators [EXPERIMENTAL].
  -Wmissing-space-around-braces   Warn about missing space around braces.
  -Wspace-around-unary-operators  Warn about space around unary operators.
";
    # Build up a string of the permissible languages

    my($languages) = "'" . join("', '", @languages) . "' and 'none'";

    # Display the usage, substituting in the permissible languages

    printf(STDERR $usage, $languages);
  }

  exit($status);
}

#
# version()
#
# Description:
#   This routine prints out program version information
#
# Input(s):
#   N/A
#
# Output(s):
#   N/A
#
# Returns:
#   This subroutine does not return.
#
sub version {
  print("cstyle Version 1.7.5d\n");
  print("Copyright (c) 2001-2017 Grant Erickson\n");

  exit (0);
}

#
# parse_warnings
#
# Description:
#   This routine handles generating a quick-reference hash to all options
#   invoked by the '-W' command line option.
#
# Input(s):
#   option   - This is the command option and should always be 'W'.
#   argument - This is the command argument and may contain one or
#              more comma-separated arguments.
#
# Output(s):
#   N/A
#
# Returns:
#   N/A
#
sub parse_warnings {
  my($warning);
  my($flag);

  # Options are either going to be singly specified or comma-separated.
  # In either case, split them up and add them to the warning hash.

  foreach $warning (split(/,/, $_[1])) {
    # Warning options may only use alphanumeric characters and the
    # dash (-) or underscore (_) characters.

    if ($warning =~ m/[^A-Za-z0-9_\-]/g) {
      die("Unknown or invalid warning option: `$warning'");
    }

    # Warnings that are of the form 'no-<warning>' have the effect of
    # disabling a warning (e.g. overrides -Wall or a previous enabling
    # of that warning.
    #
    # Attempt to match and if it matches, delete, the leading 'no-' to
    # a warning option and then deassert the warning. Otherwise, assert
    # the warning.

    if ($warning =~ s/^no-//g) {
      $flag = 0;

    } else {
      $flag = 1;

    }

    # Set the warning hash to true for this warning key.

    $warnings{$warning} = $flag;
  }
}

#
# decode_options()
#
# Description:
#   This routine steps through the command-line arguments, parsing out
#   recognzied options.
#
# Input(s):
#   N/A
#
# Output(s):
#   N/A
#
# Returns:
#   N/A
#
sub decode_options {
  my($errors) = 0;
  my(@dependencies);

  if (!&GetOptions(\%options,
		   "W=s@" => \&parse_warnings,
		   "debug|d+",
		   "help",
		   "language|x=s",
		   "copyright=s",
		   "cpp-lines-between-conditionals=i",
		   "file-length=i",
		   "line-length|l=i",
		   "tab-size|ts=i",
		   "verbose|v+",
		   "version"
		  )) {
    usage(1);
  }

  if ($options{"version"}) {
    version();
  }

  if ($options{"help"}) {
    usage(0);
  }

  # If -Wmissing-copyright was set, then so too must be --copyright.

  @dependencies = ({ OPTION => "copyright",
		     DESCRIPTION => "copyright regular expression pattern" });

  $errors += check_warning_deps("missing-copyright", \@dependencies);

  # If -Wfile-length was set, then so too must be --file-length.

  @dependencies = ({ OPTION => "file-length",
		     DESCRIPTION => "file length" });

  $errors += check_warning_deps("file-length", \@dependencies);

  # If -Wline-length was set, then so too must be --line-length and
  # --tab-size.

  @dependencies = ({ OPTION => "line-length",
		     DESCRIPTION => "line length" },
		   { OPTION => "tab-size",
		     DESCRIPTION => "tab size" });

  $errors += check_warning_deps("line-length", \@dependencies);

  # If -Wmissing-cpp-conditional-labels was set, then so too must be
  # --cpp-lines-between-conditionals.

  @dependencies = ({ OPTION => "cpp-lines-between-conditionals",
		     DESCRIPTION => "maximum number of lines between " .
		     "preprocessor conditionals" });

  $errors += check_warning_deps("missing-cpp-conditional-labels", \@dependencies);

  # At this point, we either have a list of one or more files to process
  # remaining in the argument list or we will process standard input. If
  # we are processing standard input, then a language must be specified.

  if ($#ARGV < 0 && !defined($options{"language"})) {
    print(STDERR "A language must be specified when using standard input!\n");
    $errors++;
  }

  usage(1) if ($errors);

  return;
}

#
# line_violation_with_column()
#
# Description:
#   This routine is used in response to line style violations and
#   displays, to standard error, the file path, line number, column
#   number, and line enforcement violation. If the verbose option flag
#   is in effect, the actual text of the offending line is also
#   displayed along with a leader indicating the position of the
#   error, as specified by the provided column parameter.
#
# Input(s):
#   file    - Path name of the current input file being processed.
#   message - Violation message to display.
#   line    - The current input line being processed, in its pristine,
#             unmodified state.
#   column  - The column number (0-based) of the violation used to
#             generate the indicative leader in verbose mode.
#
# Output(s):
#   N/A
#
# Returns:
#   N/A
#
sub line_violation_with_column {
  my($file) = $_[0];
  my($message) = $_[1];
  my($line) = $_[2];
  my($column) = $_[3];
  my($format);
  my($status) = ($warnings{"error"} ? "error" : "warning");

  if ($options{"verbose"}) {
    $format = "%s:%d:%d: %s: %s\n%s\n%s%s\n";
  } else {
    $format = "%s:%d:%d: %s: %s\n";
  }

  printf(STDERR $format, $file, $., $column + 1, $status, $message, $line,
	 '-' x $column, "^");
}

#
# line_violation()
#
# Description:
#   This routine is used in response to line style violations and
#   displays, to standard error, the file path, line number, column
#   number, and line enforcement violation. If the verbose option flag
#   is in effect, the actual text of the offending line is also
#   displayed along with a leader indicating the position of the
#   error. The column to generate the indicating leader is extraced
#   from $-[1].
#
# Input(s):
#   file    - Path name of the current input file being processed.
#   message - Violation message to display.
#   line    - The current input line being processed, in its pristine,
#             unmodified state.
#
# Output(s):
#   N/A
#
# Returns:
#   N/A
#
sub line_violation {
  my($file) = $_[0];
  my($message) = $_[1];
  my($line) = $_[2];

  line_violation_with_column($file, $message, $line, $-[1]);
}

#
# file_violation()
#
# Description:
#   This routine is used in response to file style violations and
#   displays, to standard error, the file path, and file enforcement
#   violation.
#
# Input(s):
#   file    - Path name of the current input file being processed.
#   message - Violation message to display.
#
# Output(s):
#   N/A
#
# Returns:
#   N/A
#
sub file_violation {
  my($file) = $_[0];
  my($message) = $_[1];
  my($format) = "%s: %s: %s\n";
  my($status) = ($warnings{"error"} ? "error" : "warning");

  printf(STDERR $format, $file, $status, $message);
}

#
# check_warning()
#
# Description:
#   This routine checks whether or not the specified warning option has
#   been set as well as checking the 'all' warning option which implicitly
#   sets the specified warning.
#
# Input(s):
#   warning - The name of the warning to be checked.
#
# Output(s):
#   N/A
#
# Returns:
#   TRUE (1) if the warning is set; otherwise, FALSE (0).
#
sub check_warning {
  my($warning) = $_[0];
  my($flag);

  # A warning is considered enabled if: 1) The warning was
  # independently asserted OR 2) The 'all' warning was asserted and
  # the warning was not independently deasserted.
  #
  # So, if we are checking warning 'foo', the following truth table
  # results:
  #
  #                     -> False
  #   -Wall		-> True
  #   -Wall -Wfoo 	-> True
  #   -Wall -Wno-foo 	-> False
  #   -Wfoo		-> True
  #   -Wno-foo		-> False

  if (defined($warnings{$warning}) && $warnings{$warning}) {
    $flag = 1;

  } elsif (defined($warnings{$warning}) && !$warnings{$warnings}) {
    $flag = 0;

  } else {
    $flag = $warnings{'all'};

  }

  return ($flag);
}

#
# check_warning_deps()
#
# Description:
#   This routine checks for interdependencies between a warning option
#   and one or more style options settings by ensuring the if the
#   warning option has been asserted that the style options on which
#   it depends are defined.
#
# Input(s):
#   warning - The name of the warning to be checked.
#   depref - A reference to an array of dependency records, each
#            record a hash reference containing the style option and
#            description.
#
# Output(s):
#   N/A
#
# Returns:
#   The number of dependency errors encounterd.
#
sub check_warning_deps {
  my($errors) = 0;
  my($warning) = $_[0];
  my($depref) = $_[1];

  if (check_warning($warning)) {
    my($warnings) = "`-Wall' or `-W$warning'";

    foreach $dependency (@{$depref}) {

      if (!defined($options{$dependency->{"OPTION"}})) {
	printf(STDERR "The %s must be specified with " .
	       	      "`--%s' when used with %s!\n",
	       $dependency->{"DESCRIPTION"},
	       $dependency->{"OPTION"},
	       $warnings);
	$errors++;
      }
    }
  }

  return ($errors);
}

#
# cppstyle()
#
# Description:
#   This routine performs coding style checking for lines identified
#   as C preprocessor input.
#
#   At minimum, the following preprocessor directives are possible and
#   expected:
#
#     <null>
#     assert <predicate> <answer>
#     define <macro> [<expression>]
#     elif <expression>
#     else
#     endif
#     error <string>
#     ident <string>
#     if <expression>
#     ifdef <macro>
#     ifndef <expression>
#     import <string>
#     include <string>
#     line [ (<number> [<string>]) | <expression> ]
#     sccs <string>
#     unassert <predicate>
#     undef <macro>
#     warning <string>
#
# Input(s):
#   file    - Path name of the current input file being processed.
#   line    - The current input line being processed, in its pristine,
#             unmodified state.
#   record  - A reference to the record defining the current C preprocessor
#             directive being checked.
#   records - A stack of C preprocessor conditional directives
#             encountered thus far in the input file. This is used as state
#             for various checks.
#
# Output(s):
#   records - The C preprocessor conditional directive stack, possibly with
#             records pushed onto or popped off.
#
# Returns:
#   The number of preprocessor violations encountered.
#
sub cppstyle {
  my($file) = $_[0];
  my($line) = $_[1];
  my($record) = $_[2];
  my($records) = $_[3];
  my($violations) = 0;


  # If enabled, check for leading white space before the
  # preprocessor directive token ('#').

  if (check_warning("cpp-directive-leading-space") &&
      (length($record->{'LEADING'}) > 0)) {
    line_violation($file, "leading space before preprocessor directive", $line);
    $violations++;
  }

  # Perform directive-specific checking, accumulating state for those
  # directives which have context-specific checks.

  DIRECTIVE: for ($record->{'DIRECTIVE'}) {

      /^(else|endif)$/	&& do {
	# Check to ensure that conditionals, separated by
	# `cpp-lines-between-conditionals' lines or more have
	# comment labels.

	my($top) = pop(@{$records});
	my($ifline) = $top->{'LINE'};

	# If the directive is an 'else', put the line the matching
	# 'if' directive was on back on the stack since we cannot
	# permanently pop it until we see the matching 'endif'.

	if ($_ =~ m/^else$/g) {
	  push(@{$records}, $top);
	}

	# Compute the distance from the last seen if/ifdef/ifndef
	# directive.

	my($distance) = $record->{'LINE'} - $ifline;
	my($threshold) = $options{"cpp-lines-between-conditionals"};

	# If the warning is asserted, the match to anything other than
	# white space following the directive hits, and the distance
	# exceeds the threshold, flag a violation.

	if (check_warning("missing-cpp-conditional-labels") &&
	    ($record->{'REST'} =~ m/^\s*$/g) && ($distance > $threshold)) {

	  # Rematch on the entire original line so that @- is set
	  # correctly and the '-v' leader behavior works to identify
	  # the offending column.

	  $line =~ m/^\s*#\s*(else|endif)\s*$/g;

	  line_violation($file,
			 "Missing preprocessor conditional comment label " .
			 "for '$_' matching '$top->{'DIRECTIVE'}' at line " .
			 "$ifline, which is more than $threshold line(s) away",
			 $line);
	  $violations++;
	}

	last DIRECTIVE;
      };

      /^(if|elif)$/	&& do {
	# Push the current line onto the if/ifdef/ifndef
	# conditional stack so that it can be used for subsequent
	# checks.

	if ($_ =~ m/^if$/g) {
	  push(@{$records}, $record);
	}

	# Check for constant numeric conditional expressions which
	# indicate that the code is being unconditionally compiled
	# out or in. Any sequence of digits following the directive
	# hits the match.

	if (check_warning("cpp-constant-conditionals")) {
	  my($cpp_constant_conditional_re) = qr/[([:space:]]*(\d+)[)[:space:]]*/;

	  if ($record->{'REST'} =~ m/^$cpp_constant_conditional_re$/g) {

	    # Rematch on the entire original line so that @- is set
	    # correctly and the '-v' leader behavior works to identify
	    # the offending column.

	    $line =~ m/$cpp_constant_conditional_re$/g;

	    # Now flag the violation.

	    line_violation($file, "constant numeric preprocessor expression",
			   $line);
	    $violations++;
	  }
	}

	last DIRECTIVE;
      };

      /^if(n)*def$/	&& do {
	# Push the current line onto the if/ifdef/ifndef
	# conditional stack so that it can be used for subsequent
	# checks.
	
	push(@{$records}, $record);

	last DIRECTIVE;
      };

      # Default

      last DIRECTIVE;
  }

  return ($violations);
}

#
# asmstyle()
#
# Description:
#   This routine performs the actual coding style checking for assembler
#   source files.
#
# Input(s):
#   file - Path name of the current input file being processed.
#
# Output(s):
#   N/A
#
# Returns:
#   N/A
#
sub asmstyle {
  my($file) = $_[0];

  # In the absence of any language-specific checks, just call cstyle...

  return (cstyle($file));
}

#
# objcstyle()
#
# Description:
#   This routine performs the actual coding style checking for
#   Objective C source files.
#
# Input(s):
#   file - Path name of the current input file being processed.
#
# Output(s):
#   N/A
#
# Returns:
#   N/A
#
sub objcstyle {
  my($file) = $_[0];

  # In the absence of any language-specific checks, just call cstyle...

  return (cstyle($file));
}

#
# objcxxstyle()
#
# Description:
#   This routine performs the actual coding style checking for
#   Objective C++ source files.
#
# Input(s):
#   file - Path name of the current input file being processed.
#
# Output(s):
#   N/A
#
# Returns:
#   N/A
#
sub objcxxstyle {
  my($file) = $_[0];

  # In the absence of any language-specific checks, just call cstyle...

  return (cstyle($file));
}

#
# is_function_or_method_declaration()
#
# Description:
#   This routine attempts to perform a match on the current line ($_)
#   and determines whether or not it contains a C or C++ function or
#   method declaration.
#
# Input(s):
#   N/A
#
# Output(s):
#   N/A
#
# Returns:
#   True (1) if the current line ($_) is thought to contain a C or C++
#   function or method declaration; otherwise, false.
#
sub is_function_or_method_declaration {
  return (/$function_declaration_re/);
}

#
# is_function_or_method_declaration()
#
# Description:
#   This routine attempts to perform a match on the current line ($_)
#   and determines whether or not it contains an implicit void (e.g. foo())
#   C or C++ function or method declaration.
#
# Input(s):
#   N/A
#
# Output(s):
#   N/A
#
# Returns:
#   True (1) if the current line ($_) is thought to contain an
#   implicit void C or C++ function or method declaration; otherwise,
#   false.
#
sub is_implicit_void_function_or_method_declaration {
  my($virtual_specifier_re) = qr/((virtual)\s+)/;
  my($implicit_void_function_declaration_re) = qr/($virtual_specifier_re|$storage_specifiers_re)*\s*$type_re\s*$function_declarator_re\s*\(\s*\)\s*$method_qualifiers_re*(\s*=\s*0)*;\s*/;
  my($retval);

  $retval = m/$implicit_void_function_declaration_re/gp;

  # Filter out:
  #
  #   - Any member pointer or member reference selection void method
  #     calls.
  #
  #   - Any return statements embedding a void function or method
  #     call.
  #
  #   - Any assignments with a void function or method call as an rvalue.

  if ($retval) {
    if ((${^PREMATCH} =~ m/(\.|->)$/gp) ||
	(${^PREMATCH} =~ m/\s*return\s/gp) ||
	(${^PREMATCH} =~ m/\s*=\s*/gp)) {
      $retval = undef;
    }
  }

  return ($retval);
}

#
# cstyle()
#
# Description:
#   This routine performs the actual coding style checking for C source
#   and header files.
#
# Input(s):
#   file - Path name of the current input file being processed.
#
# Output(s):
#   N/A
#
# Returns:
#   N/A
#
sub cstyle {
  my($file) = $_[0];
  my($neolchars) = 0;
  my($line, $prev);
  my($violations) = 0;
  my($return_count) = 0;
  my(%state) = ();
  my(@cppstack);
  my(@bracestack);
  my($in_function_at_depth) = -1;
  my($in_comment) = 0;
  my($saw_copyright) = 0;

LINE: while (<STDIN>) {
    # Attempt to automatically detect end-of-line markers based on what is
    # encountered on the first line.

    if ($. == 1) {
      if (m/(\015?\012?)$/) {
	$/ = $1
      }
    }

    # Strip the end-of-line marker. Note that chomp should always be
    # used rather than chop, since chomp acts intelligently based on
    # '$/' whereas chop just always consumes that last character on a
    # line without regard to whether or not its actually a newline
    # character at all.

    $neolchars = chomp;

    # Cache the input line in its pristine, unmodified state.

    $line = $_;

    # Clean-up and ignore things we don't want to further check:
    #
    # 1) Text, including escaped characters, w/i single ('') and double ("") quotes.

    s/(["'])(?:(?=(\\?))\2.)*?\1/\1\1/g;

    # 2) Trailing backslashes.

    s/\s*\\$//;

    # Check file length

    if (check_warning("file-length")) {
      if ($. > $options{"file-length"}) {
	line_violation($file, "file > " .
		       $options{"file-length"} .
		       " lines", $line);
	$violations++;
      }
    }

    # Inline "/* *STYLE-OFF* */" or "// *STYLE-OFF*" directives on a
    # line by themselves exempt subsequent lines from style checking
    # until a similar "/* *STYLE-ON* */ or "// *STYLE-ON*" directive
    # on a line by itself is encountered.

    if ((/^\s*\/\*\s*\*STYLE-(ON|OFF)\*\s*\*\/$/) ||	# C-style directive
	(/^\s*\/\/\s*\*STYLE-(ON|OFF)\*$/)) {		# C++-style directive

      # Set the state based on the positional match argument of "ON" or "OFF"

      $state{"style-disable"} = ($1 eq "OFF") ? 1 : 0;
    }

    # If style checking has been disabled by a style directive, skip ahead
    # to the next line.

    if (defined($state{"style-disable"}) && $state{"style-disable"} == 1) {
      $prev = $line;

      next LINE;
    }

    #
    # Check line length
    #

    if (check_warning("line-length")) {
      # First, a quick check to see if there is any chance of being too long.

      if ($line =~ tr/\t/\t/ *
	  ($options{"tab-size"} - 1) +
	  length($line) > $options{"line-length"}) {

	# Confirmed. Interpolate spaces for tabs and check again.

	$eline = $line;

	1 while $eline =~ s/\t+/" " x (length($&) *
				       $options{"tab-size"} - length($`) %
				       $options{"tab-size"})/e;

	if (length($eline) > $options{"line-length"}) {
	  line_violation($file, "line > " .
			 $options{"line-length"} .
			 " characters", $line);
	  $violations++;
	}
      }
    }

    # Check for unnecessary trailing white-space at the end of a
    # non-blank line.

    if (check_warning("trailing-space")) {
      if (/[^[:space:]]+(\s+)$/) {
	line_violation($file, "white space at end of a non-blank line", $line);
	$violations++;
      }
    }

    # Check for unnecessary trailing white-space at the end of a blank
    # line.

    if (check_warning("blank-trailing-space")) {
      if (/^(\s+)$/ && !/^\f$/) {
	line_violation($file, "white space at end of a blank line", $line);
	$violations++;
      }
    }

    # Check for interpolation of tabs with spaces.

    if (check_warning("interpolated-space")) {
      if (/\t([ ]+)\t/) {
	line_violation($file, "spaces between tabs", $line);
	$violations++;
      }

      if (/ ([\t]+) /) {
	line_violation($file, "tabs between spaces", $line);
	$violations++;
      }
    }

    # Delete any trailing whitespace; we have already checked for that.

    s/\s*$//;

    # Check preprocessor directives.
    #
    # The following regular expression breaks up the directive as follows
    #
    #   1: Optional leading white space, up to the '#'.
    #   2: Optional trailing white space, after the '#'.
    #   3: The directive itself.
    #   4: Optional trailing white space, after the directive.
    #   5: Optional directive-specific tokens.
    #

    if (/^(\s*)#(\s*)([a-z]+)(\s*)(.*)$/) {

      %cpprecord = ( 'LEADING' => $1,
		     'TRAILING' => $2,
		     'DIRECTIVE' => $3,
		     'REST' => $5,
		     'LINE' => $. );

      $violations += cppstyle($file, $line, {%cpprecord}, \@cppstack);

      $prev = $line;

      next LINE;
    }

    # Check for a missing copyright declaration

    if (check_warning("missing-copyright") && $saw_copyright != 1) {
      if (/$options{"copyright"}/) {
        $saw_copyright = 1;
      }
    }

    # Search for a start of a multi-line comment

    if (/\/\*/ && $in_comment != 1) {
      $in_comment = 1;

      # This might be an one line comment (e.g. "/* ... */")

      if (/\*\//) {
        $in_comment = 0;
      }

      # Swallow it.

      s/\/\*.+$//;
    }

    # Search for the end of a multi-line comment

    if (/\*\// && $in_comment != 0) {
      $in_comment = 0;

      # Swallow it.

      s/^.+\*\///;
    }

    # If we are in a multi-line comment, there is no further style
    # checking at tis point. Simply iterate to the next line.

    if ($in_comment != 0) {
      $prev = $line;

      next LINE;
    }

    # Swallow, at this point, any inline comments (i.e. "// ...")

    s/\/\/.+$//;

    # Check for missing white space after commas

    if (check_warning("missing-space-after-comma")) {
      if (/(,)\S/) {
        line_violation($file, "missing space after comma", $line);
        $violations++;
      }
    }

    # Check for missing white space after semicolons

    if (check_warning("missing-space-after-semicolon")) {
      if (/(;)\S/) {
        line_violation($file, "missing space after semicolon", $line);
        $violations++;
      }
    }

    # Check for missing space after keywords

    if (check_warning("missing-space-after-if")) {
      if (/(?<!else)\sif(\()/) {
        line_violation($file, "missing space after 'if' keyword", $line);
        $violations++;
      }
    }

    if (check_warning("missing-space-after-else-if")) {
      if (/else if(\()/) {
        line_violation($file, "missing space after 'else if' keyword", $line);
        $violations++;
      }
    }

    if (check_warning("missing-space-after-for")) {
      if (/\sfor(\()/) {
        line_violation($file, "missing space after 'for' keyword", $line);
        $violations++;
      }
    }

    if (check_warning("missing-space-after-operator")) {
      if (/[[:space:]&*]operator(new|delete|[.\-+*\/%><=!|^&~\[,(])/) {
        line_violation($file, "missing space after 'operator' keyword", $line);
        $violations++;
      }
    }

    if (check_warning("missing-space-after-switch")) {
      if (/\sswitch(\()/) {
        line_violation($file, "missing space after 'switch' keyword", $line);
        $violations++;
      }
    }

    if (check_warning("missing-space-after-while")) {
      if (/}*\s*while(\()/) {
        line_violation($file, "missing space after 'while' keyword", $line);
        $violations++;
      }
    }

    if (check_warning("missing-space-around-braces")) {
      my($opening_brace_re) = qr/(^\{\S|[^\s@^]\{\s|[^\s@^]\{\S|\s\{\S|[^\s@^]\{$)/;
      my($closing_brace_re) = qr/(^}[^\s,;)]|\S}\s|\S}\S|\S}$|\s}[^\s,;)])/;

      if ((/${opening_brace_re}/) || (/${closing_brace_re}/)) {
        my($column) = $-[1];
        my($which_brace);

        if ($1 =~ m/{/) {
          $which_brace = "opening";
        } else {
          $which_brace = "closing";
        }

        line_violation_with_column($file, "missing space around $which_brace brace", $line, $column);
        $violations++;
      }
    }

    # Check for missing space around binary operators:
    #
    #   [space]<binary operator>[space]
    #
    # including:
    #
    #   - Comparison operators: ==, !=, <=, >=, <, >
    #   - Logical operators: &&, ||
    #   - Bitwise operators: &, |, ^
    #   - Arithmetic operators: +, *, -, /, %
    #   - Shift operators: <<, >>
    #   - Assignment operator: =
    #   - Compound operators: &=, |=, ^=, +=, *=, -=, /=, %=, <<=, >>=

    if (check_warning("missing-space-around-binary-operators")) {
      my($eq_operator_re)                        = qr/==/;
      my($ne_operator_re)                        = qr/!=/;
      my($le_operator_re)                        = qr/<=/;
      my($ge_operator_re)                        = qr/>=/;
      my($lt_operator_re)                        = qr/(?<!([ct]_cast))(?<!<)<(?![=<])/;
      my($gt_operator_re)                        = qr/(?<![>-])>(?![=>])/;
      my($logical_and_operator_re)               = qr/&&/;
      my($logical_or_operator_re)                = qr/\|\|/;
      my($bitwise_and_operator_re)               = qr/(?<![=*&,][[:space:]])(?<![&*(])&(?![&=)])/;
      my($bitwise_or_operator_re)                = qr/(?<!\|)\|(?![\|=])/;
      my($bitwise_xor_operator_re)               = qr/\^(?!=)/;
      my($addition_operator_re)                  = qr/(?<!\+)\+(?![\+=])/;
      my($multiplication_operator_re)            = qr/(?<![=*&,][[:space:]])(?<![(\*])\*(?![=\*&>)])/;
      my($subtraction_operator_re)               = qr/(?<![=][[:space:](])(?<![-\[\(])-(?![-=>])/;
      my($division_operator_re)                  = qr/\/(?!=)/;
      my($modulus_operator_re)                   = qr/%(?!=)/;
      my($left_shift_operator_re)                = qr/<</;
      my($right_shift_operator_re)               = qr/>>/;
      my($assignment_operator_re)                = qr/(?<![=!<>+*\-\/%&|^])=(?!=)/;
      my($bitwise_and_assignment_operator_re)    = qr/&=/;
      my($bitwise_or_assignment_operator_re)     = qr/\|=/;
      my($bitwise_xor_assignment_operator_re)    = qr/\^=/;
      my($addition_assignment_operator_re)       = qr/\+=/;
      my($multiplication_assignment_operator_re) = qr/\*=/;
      my($subtraction_assignment_operator_re)    = qr/-=/;
      my($division_assignment_operator_re)       = qr/\/=/;
      my($modulus_assignment_operator_re)        = qr/%=/;
      my($left_shift_assignment_operator_re)     = qr/<<=/;
      my($right_shift_assignment_operator_re)    = qr/>>=/;
      my($binop_core_re)                         = qr/(?<!operator)
						      (?<!operator\s)
						      (?<!operator[><])
						      (${eq_operator_re}|
						       ${ne_operator_re}|
						       ${le_operator_re}|
						       ${ge_operator_re}|
						       ${lt_operator_re}|
						       ${gt_operator_re}|
						       ${logical_and_operator_re}|
						       ${logical_or_operator_re}|
						       ${bitwise_and_operator_re}|
						       ${bitwise_or_operator_re}|
						       ${bitwise_xor_operator_re}|
						       ${addition_operator_re}|
						       ${multiplication_operator_re}|
						       ${subtraction_operator_re}|
						       ${division_operator_re}|
						       ${modulus_operator_re}|
						       ${left_shift_operator_re}|
						       ${right_shift_operator_re}|
						       ${assignment_operator_re}|
						       ${bitwise_and_assignment_operator_re}|
						       ${bitwise_or_assignment_operator_re}|
						       ${bitwise_xor_assignment_operator_re}|
						       ${addition_assignment_operator_re}|
						       ${multiplication_assignment_operator_re}|
						       ${subtraction_assignment_operator_re}|
						       ${division_assignment_operator_re}|
						       ${modulus_assignment_operator_re}|
						       ${left_shift_assignment_operator_re}|
						       ${right_shift_assignment_operator_re})/x;
      if (/\S${binop_core_re}\S/ || /\s${binop_core_re}\S/ || /\S${binop_core_re}\s/) {
        line_violation($file, "missing space around binary operator '$1'", $line);
        $violations++;
      }
    }

    # Check for space around unary operators:
    #
    #   [space]<unary operator>[space]
    #
    # including:
    #
    #   - Logical negation operator: !
    #   - Bitwise negation operator: ~
    #   - Post- and prefix increment operator: ++
    #   - Post- and prefix decrement operator: --

    if (check_warning("space-around-unary-operators")) {
      my($logical_negation_re)  = qr/(!)\s+/;
      my($bitwise_negation_re)  = qr/(~)\s+/;
      my($prefix_increment_re)  = qr/(\+\+)\s+[[:alnum:]_(\[]/;
      my($postfix_increment_re) = qr/[[:alnum:]_)\]]\s+(\+\+)/;
      my($prefix_decrement_re)  = qr/(--)\s+[[:alnum:]_(\[]/;
      my($postfix_decrement_re) = qr/[[:alnum:]_)\]]\s+(--)/;

      if (/${logical_negation_re}/  ||
	  /${bitwise_negation_re}/  ||
	  /${prefix_increment_re}/  ||
	  /${postfix_increment_re}/ ||
	  /${prefix_decrement_re}/  ||
	  /${postfix_decrement_re}/) {
        line_violation($file, "space around unary operator '$1'", $line);
        $violations++;
      }
    }

    # Search for implicit void function or method declarations:
    #
    #   [extern|static ][return type ][name]()[[const ][volatile ]];

    if (check_warning("implicit-void-declaration")) {
      if (is_implicit_void_function_or_method_declaration()) {
        line_violation($file, "implicit void declaration", $line);
        $violations++;
      }
    }

    # Search for function or method implementations:
    #
    #   [static ][inline ][return type ][name](argument[, [argument]])[ [const ][volatile]]

    if (/((static|inline)\s)*([_A-Za-z]\w+)*(?!if|else|for|while|switch|return|sizeof)([_A-Za-z]\w+)\s*\(\s*.+\)\s*(?!;)(\s(const|volatile)\s)*/) {
      print("$file: $.: found a possible function implementation\n") if ($options{"debug"});
      $in_function_at_depth = $#bracestack + 1;
    }

    # Match and push opening brace lines onto the brace parsing stack.

    if (/{/) {
      print("$file: $.: encountered opening brace.\n") if ($options{"debug"});
      push(@bracestack, $line);
    }

    # Increment the return count (ostensibly within the outermost function or
    # method scope) where return statements are encountered.

    if (/return\s*\(*.*\)*;/) {
      $return_count++;
      printf("$file: $.: Saw a return statement! Return count %u\n", $return_count) if ($options{"debug"});
      if (check_warning("multiple-returns") && ($return_count > 1)) {
        line_violation($file, "multiple return statements", $line);
        $violations++;
      }
    }

    # Match and pop closing brace lines off of the brace parsing
    # stack, resetting the return statement count if we have exited
    # the current function or method scope.

    if (/}/) {
      print("$file: $.: encountered closing brace.\n") if ($options{"debug"});
      pop(@bracestack);
      if ($in_function_at_depth == $#bracestack + 1) {
	print("Probably out of function!\n") if ($options{"debug"});
	$return_count = 0;
      }
    }

    # Cache the last line and move onto the next line.

    $prev = $line;
  }

  if (!defined($state{"style-disable"}) || $state{"style-disable"} != 1) {
    # Check to make sure that the last line of the file contains a new
    # line (depends on the Perl execution environment and is stored in
    # '$/').

    if (check_warning("missing-newline-at-eof")) {
      if ($neolchars == 0) {
        line_violation($file, "missing new line at the end of file", $line);
        $violations++;
      }
    }

    # Check to make sure that the last line in the file is not blank

    if (check_warning("trailing-line")) {
      if ($prev eq "") {
        line_violation($file, "last line in file is blank", $line);
        $violations++;
      }
    }

    # Check to make sure we saw a matching copyright declaration if
    # requested.

    if (check_warning("missing-copyright") && $saw_copyright != 1) {
      file_violation($file, "missing copyright declaration");
      $violations++;
    }
  }

  return ($violations);
}

#
# cxxstyle()
#
# Description:
#   This routine performs the actual coding style checking for C source
#   and header files.
#
# Input(s):
#   file - Path name of the current input file being processed.
#
# Output(s):
#   N/A
#
# Returns:
#   N/A
#
sub cxxstyle {
  my($file) = $_[0];

  # In the absence of any language-specific checks, just call cstyle...

  return (cstyle($file));
}

#
# style()
#
# Description:
#   This routine performs the actual coding style checking for all language
#   independent checks and calls a handler for the specified language to
#   perform language-specific checks.
#
# Input(s):
#   file     - Path name of the current input file being processed.
#   language - Language-specific handler reference.
#
# Output(s):
#   N/A
#
# Returns:
#   N/A
#
sub style {
  my($file) = $_[0];
  my($language) = $_[1];
  my($eol, $handler, $status);

  # Identify whether or not a valid handler is registered for
  # this language.

  $handler = $handlers{$language};

  if (!defined($handler)) {
    printf(STDERR "%s: Unknown or unsupported language!\n", $language);
    return (1);
  }

  # The EOL variable may get set (differently) from file to file when
  # the program is operating on multiple files. Save the initial EOL
  # marker and reset it on entry and exit, respectively. Failure to do
  # so, leads to interesting and incorrect program behavior when one
  # file has one line ending type (e.g DS) and the subsequent one
  # another (e.g. UNIX).

  $eol = $/;

  $status = &{$handler}($file);

  $/ = $eol;

  return ($status);
}

#
# Main program body
#
{
  my($file);
  my($status) = 0;
  my($language);
  my($useextensions) = 0;

  # Set the program name

  $program = basename($0);

  # Parse non-file program options from the command line

  decode_options();

  # Determine whether or not a default language has been specified.
  # This is required when working from standard input; otherwise, the
  # language is determined file-by-file based on the extension.
  #
  # If the language option is explicitly "none", then it is the same
  # as if it had not been defined at all.

  $language = $options{"language"};

  if (!defined($language) || ($language eq "none")) {
    $useextensions = 1;
  }

  # Work on each file specified on the command line. Otherwise, just
  # process standard input.

  if ($#ARGV >= 0) {
      SOURCEFILE: foreach $file (@ARGV) {

	  if ($useextensions) {
	    # Determine which parser/handler to use by mapping the
	    # file extension. Extensions are split at the last dot (.)
	    # in the extension, so "foo.c.N" yields ".N" and
	    # "bar.tar.gz" yields ".gz" for extensions, respectively.

	    my($extension) = (fileparse($file, qr(\.[^.]+)))[2];

	    # Strip the dot off the beginning of the extension.

	    $extension =~ s/^\.//;

	    # Attempt to map the extension to a language through the
	    # extensions hash.

	    $language = $extensions{$extension};

	    if (!defined($language)) {
	      printf(STDERR "%s: Unknown or unsupported file type!\n", $file);
	      $status++;
	      next SOURCEFILE;
	    }
	  }

	  # Attempt to open the file

	  if (!open(STDIN, $file)) {
	    printf(STDERR "%s: Cannot open!\n", $file);
	    $status++;
	    next SOURCEFILE;
	  }

	  # Style check the file, note the status and close the stream

	  $status += style($file, $language);
	  close(STDIN);
	}

  } else {
    $status += style("<standard input>", $language);
  }

  # If the caller invoked with '-Werror', return the accumulated
  # status; otherwise, return zero (0).

  exit($warnings{"error"} ? $status : 0);
}
