# Copyright (C) 2003-2013 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

package Automake::Options;

use 5.006;
use strict;
use Exporter;
use Automake::Config;
use Automake::ChannelDefs;
use Automake::Channels;
use Automake::Version;

use vars qw (@ISA @EXPORT);

@ISA = qw (Exporter);
@EXPORT = qw (option global_option
              set_option set_global_option
              unset_option unset_global_option
              process_option_list process_global_option_list
              set_strictness $strictness $strictness_name
              &FOREIGN &GNU &GNITS);

=head1 NAME

Automake::Options - keep track of Automake options

=head1 SYNOPSIS

  use Automake::Options;

  # Option lookup and setting.
  $opt = option 'name';
  $opt = global_option 'name';
  set_option 'name', 'value';
  set_global_option 'name', 'value';
  unset_option 'name';
  unset_global_option 'name';

  # Batch option setting.
  process_option_list $location, @names;
  process_global_option_list $location, @names;

  # Strictness lookup and setting.
  set_strictness 'foreign';
  set_strictness 'gnu';
  set_strictness 'gnits';
  if ($strictness >= GNU) { ... }
  print "$strictness_name\n";

=head1 DESCRIPTION

This packages manages Automake's options and strictness settings.
Options can be either local or global.  Local options are set using an
C<AUTOMAKE_OPTIONS> variable in a F<Makefile.am> and apply only to
this F<Makefile.am>.  Global options are set from the command line or
passed as an argument to C<AM_INIT_AUTOMAKE>, they apply to all
F<Makefile.am>s.

=cut

# Values are the Automake::Location of the definition.
use vars '%_options';        # From AUTOMAKE_OPTIONS
use vars '%_global_options'; # From AM_INIT_AUTOMAKE or the command line.

# Whether process_option_list has already been called for the current
# Makefile.am.
use vars '$_options_processed';
# Whether process_global_option_list has already been called.
use vars '$_global_options_processed';

=head2 Constants

=over 4

=item FOREIGN

=item GNU

=item GNITS

Strictness constants used as values for C<$strictness>.

=back

=cut

# Constants to define the "strictness" level.
use constant FOREIGN => 0;
use constant GNU     => 1;
use constant GNITS   => 2;

=head2 Variables

=over 4

=item C<$strictness>

The current strictness.  One of C<FOREIGN>, C<GNU>, or C<GNITS>.

=item C<$strictness_name>

The current strictness name.  One of C<'foreign'>, C<'gnu'>, or C<'gnits'>.

=back

=cut

# Strictness levels.
use vars qw ($strictness $strictness_name);

# Strictness level as set on command line.
use vars qw ($_default_strictness $_default_strictness_name);


=head2 Functions

=over 4

=item C<Automake::Options::reset>

Reset the options variables for the next F<Makefile.am>.

In other words, this gets rid of all local options in use by the
previous F<Makefile.am>.

=cut

sub reset ()
{
  $_options_processed = 0;
  %_options = %_global_options;
  # The first time we are run,
  # remember the current setting as the default.
  if (defined $_default_strictness)
    {
      $strictness = $_default_strictness;
      $strictness_name = $_default_strictness_name;
    }
  else
    {
      $_default_strictness = $strictness;
      $_default_strictness_name = $strictness_name;
    }
}

=item C<$value = option ($name)>

=item C<$value = global_option ($name)>

Query the state of an option.  If the option is unset, this
returns the empty list.  Otherwise it returns the option's value,
as set by C<set_option> or C<set_global_option>.

Note that C<global_option> should be used only when it is
important to make sure an option hasn't been set locally.
Otherwise C<option> should be the standard function to
check for options (be they global or local).

=cut

sub option ($)
{
  my ($name) = @_;
  return () unless defined $_options{$name};
  return $_options{$name};
}

sub global_option ($)
{
  my ($name) = @_;
  return () unless defined $_global_options{$name};
  return $_global_options{$name};
}

=item C<set_option ($name, $value)>

=item C<set_global_option ($name, $value)>

Set an option.  By convention, C<$value> is usually the location
of the option definition.

=cut

sub set_option ($$)
{
  my ($name, $value) = @_;
  $_options{$name} = $value;
}

sub set_global_option ($$)
{
  my ($name, $value) = @_;
  $_global_options{$name} = $value;
}


=item C<unset_option ($name)>

=item C<unset_global_option ($name)>

Unset an option.

=cut

sub unset_option ($)
{
  my ($name) = @_;
  delete $_options{$name};
}

sub unset_global_option ($)
{
  my ($name) = @_;
  delete $_global_options{$name};
}


=item C<process_option_list (@list)>

=item C<process_global_option_list (@list)>

Process Automake's option lists.  C<@list> should be a list of hash
references with keys C<option> and C<where>, where C<option> is an
option as they occur in C<AUTOMAKE_OPTIONS> or C<AM_INIT_AUTOMAKE>,
and C<where> is the location where that option occurred.

These functions should be called at most once for each set of options
having the same precedence; i.e., do not call it twice for two options
from C<AM_INIT_AUTOMAKE>.

Return 0 on error, 1 otherwise.

=cut

# $BOOL
# _option_is_from_configure ($OPTION, $WHERE)
# ----------------------------------------------
# Check that the $OPTION given in location $WHERE is specified with
# AM_INIT_AUTOMAKE, not with AUTOMAKE_OPTIONS.
sub _option_is_from_configure ($$)
{
  my ($opt, $where)= @_;
  return 1
    if $where->get =~ /^configure\./;
  error $where,
        "option '$opt' can only be used as argument to AM_INIT_AUTOMAKE\n" .
        "but not in AUTOMAKE_OPTIONS makefile statements";
  return 0;
}

# $BOOL
# _is_valid_easy_option ($OPTION)
# -------------------------------
# Explicitly recognize valid automake options that require no
# special handling by '_process_option_list' below.
sub _is_valid_easy_option ($)
{
  my $opt = shift;
  return scalar grep { $opt eq $_ } qw(
    check-news
    color-tests
    dejagnu
    dist-bzip2
    dist-lzip
    dist-xz
    dist-zip
    info-in-builddir
    no-define
    no-dependencies
    no-dist
    no-dist-gzip
    no-exeext
    no-installinfo
    no-installman
    no-texinfo.tex
    nostdinc
    readme-alpha
    serial-tests
    parallel-tests
    silent-rules
    std-options
    subdir-objects
  );
}

# $BOOL
# _process_option_list (\%OPTIONS, @LIST)
# ------------------------------------------
# Process a list of options.  \%OPTIONS is the hash to fill with options
# data.  @LIST is a list of options as get passed to public subroutines
# process_option_list() and process_global_option_list() (see POD
# documentation above).
sub _process_option_list (\%@)
{
  my ($options, @list) = @_;
  my @warnings = ();
  my $ret = 1;

  foreach my $h (@list)
    {
      local $_ = $h->{'option'};
      my $where = $h->{'where'};
      $options->{$_} = $where;
      if ($_ eq 'gnits' || $_ eq 'gnu' || $_ eq 'foreign')
        {
          set_strictness ($_);
        }
      # TODO: Remove this special check in Automake 3.0.
      elsif (/^(.*\/)?ansi2knr$/)
        {
          # Obsolete (and now removed) de-ANSI-fication support.
          error ($where,
                 "automatic de-ANSI-fication support has been removed");
          $ret = 0;
        }
      # TODO: Remove this special check in Automake 3.0.
      elsif ($_ eq 'cygnus')
        {
          error $where, "support for Cygnus-style trees has been removed";
          $ret = 0;
        }
      # TODO: Remove this special check in Automake 3.0.
      elsif ($_ eq 'dist-lzma')
        {
          error ($where, "support for lzma-compressed distribution " .
                         "archives has been removed");
          $ret = 0;
        }
      # TODO: Make this a fatal error in Automake 2.0.
      elsif ($_ eq 'dist-shar')
        {
          msg ('obsolete', $where,
               "support for shar distribution archives is deprecated.\n" .
               "  It will be removed in Automake 2.0");
        }
      # TODO: Make this a fatal error in Automake 2.0.
      elsif ($_ eq 'dist-tarZ')
        {
          msg ('obsolete', $where,
               "support for distribution archives compressed with " .
               "legacy program 'compress' is deprecated.\n" .
               "  It will be removed in Automake 2.0");
        }
      elsif (/^filename-length-max=(\d+)$/)
        {
          delete $options->{$_};
          $options->{'filename-length-max'} = [$_, $1];
        }
      elsif ($_ eq 'tar-v7' || $_ eq 'tar-ustar' || $_ eq 'tar-pax')
        {
          if (not _option_is_from_configure ($_, $where))
            {
              $ret = 0;
            }
          for my $opt ('tar-v7', 'tar-ustar', 'tar-pax')
            {
              next
                if $opt eq $_ or ! exists $options->{$opt};
              error ($where,
                     "options '$_' and '$opt' are mutually exclusive");
              $ret = 0;
            }
        }
      elsif (/^\d+\.\d+(?:\.\d+)?[a-z]?(?:-[A-Za-z0-9]+)?$/)
        {
          # Got a version number.
          if (Automake::Version::check ($VERSION, $&))
            {
              error ($where, "require Automake $_, but have $VERSION");
              $ret = 0;
            }
        }
      elsif (/^(?:--warnings=|-W)(.*)$/)
        {
          my @w = map { { cat => $_, loc => $where} } split (',', $1);
          push @warnings, @w;
        }
      elsif (! _is_valid_easy_option $_)
        {
          error ($where, "option '$_' not recognized");
          $ret = 0;
        }
    }

  # We process warnings here, so that any explicitly-given warning setting
  # will take precedence over warning settings defined implicitly by the
  # strictness.
  foreach my $w (@warnings)
    {
      msg 'unsupported', $w->{'loc'},
          "unknown warning category '$w->{'cat'}'"
        if switch_warning $w->{cat};
    }

  return $ret;
}

sub process_option_list (@)
{
  prog_error "local options already processed"
    if $_options_processed;
  $_options_processed = 1;
  _process_option_list (%_options, @_);
}

sub process_global_option_list (@)
{
  prog_error "global options already processed"
    if $_global_options_processed;
  $_global_options_processed = 1;
  _process_option_list (%_global_options, @_);
}

=item C<set_strictness ($name)>

Set the current strictness level.
C<$name> should be one of C<'foreign'>, C<'gnu'>, or C<'gnits'>.

=cut

# Set strictness.
sub set_strictness ($)
{
  $strictness_name = $_[0];

  Automake::ChannelDefs::set_strictness ($strictness_name);

  if ($strictness_name eq 'gnu')
    {
      $strictness = GNU;
    }
  elsif ($strictness_name eq 'gnits')
    {
      $strictness = GNITS;
    }
  elsif ($strictness_name eq 'foreign')
    {
      $strictness = FOREIGN;
    }
  else
    {
      prog_error "level '$strictness_name' not recognized";
    }
}

1;

### Setup "GNU" style for perl-mode and cperl-mode.
## Local Variables:
## perl-indent-level: 2
## perl-continued-statement-offset: 2
## perl-continued-brace-offset: 0
## perl-brace-offset: 0
## perl-brace-imaginary-offset: 0
## perl-label-offset: -2
## cperl-indent-level: 2
## cperl-brace-offset: 0
## cperl-continued-brace-offset: 0
## cperl-label-offset: -2
## cperl-extra-newline-before-brace: t
## cperl-merge-trailing-else: nil
## cperl-continued-statement-offset: 2
## End:
