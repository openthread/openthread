# Copyright (C) 2012-2013 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

package Automake::Getopt;

=head1 NAME

Automake::Getopt - GCS conforming parser for command line options

=head1 SYNOPSIS

  use Automake::Getopt;

=head1 DESCRIPTION

Export a function C<parse_options>, performing parsing of command
line options in conformance to the GNU Coding standards.

=cut

use 5.006;
use strict;
use warnings FATAL => 'all';
use Exporter ();
use Getopt::Long ();
use Automake::ChannelDefs qw/fatal/;
use Carp qw/croak confess/;

use vars qw (@ISA @EXPORT);
@ISA = qw (Exporter);
@EXPORT= qw/getopt/;

=item C<parse_options (%option)>

Wrapper around C<Getopt::Long>, trying to conform to the GNU
Coding Standards for error messages.

=cut

sub parse_options (%)
{
  my %option = @_;

  Getopt::Long::Configure ("bundling", "pass_through");
  # Unrecognized options are passed through, so GetOption can only fail
  # due to internal errors or misuse of options specification.
  Getopt::Long::GetOptions (%option)
    or confess "error in options specification (likely)";

  if (@ARGV && $ARGV[0] =~ /^-./)
    {
      my %argopts;
      for my $k (keys %option)
	{
	  if ($k =~ /(.*)=s$/)
	    {
	      map { $argopts{(length ($_) == 1)
			     ? "-$_" : "--$_" } = 1; } (split (/\|/, $1));
	    }
	}
      if ($ARGV[0] eq '--')
	{
	  shift @ARGV;
	}
      elsif (exists $argopts{$ARGV[0]})
	{
	  fatal ("option '$ARGV[0]' requires an argument\n"
		 . "Try '$0 --help' for more information.");
	}
      else
	{
	  fatal ("unrecognized option '$ARGV[0]'.\n"
		 . "Try '$0 --help' for more information.");
	}
    }
}

=back

=head1 SEE ALSO

L<Getopt::Long>

=cut

1; # for require

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
