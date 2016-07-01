Nest Labs Build - GNU Autotools
===============================

Introduction
------------

The Nest Labs Build - GNU Autotools (nlbuild-autotools) provides a
customized, turnkey build system framework, based on GNU autotools, for
standalone Nest Labs software packages that need to support not only
building on and targeting against standalone build host systems but
also embedded target systems using GCC-based or -compatible toolchains.

Getting Started
---------------

This project is typically subtreed into a target project repository
and serves as the seed for that project's build system.

Assuming that you already have a project repository established in
git, perform the following in your project repository:

    > 1. % git remote add nlbuild-autotools ssh://<PATH_TO_REPOSITORY>/nlbuild-autotools.git
    > 2. % git fetch nlbuild-autotools

You can place the nlbuild-autotools package anywhere in your project;
however, by convention, "third_party/nlbuild-autotools/repo" is recommended:

    > 3. % mkdir third_party
    > 4. % git subtree add --prefix=third_party/nlbuild-autotools/repo --squash --message="Add subtree mirror of repository 'ssh://<PATH_TO_REPOSITORY>/nlbuild-autotools.git' branch 'master' at commit 'HEAD'." nlbuild-autotools HEAD

At this point, you now have the nlbuild-autotools package integrated
into your project. The next step is using the
nlbuild-autotools-provided examples as templates. To do this, a
convenience script has been provided that will get you started. You
can tune and customize the results, as needed, for your project. From
the top level of your project tree:

    > 5. % third_party/nlbuild-autotools/repo/scripts/mkskeleton -I third_party/nlbuild-autotools/repo --package-description "My Fantastic Package" --package-name "mfp"

Supported Build Host Systems
----------------------------

The nlbuild-autotools system supports the following POSIX-based build
host systems:

  * i686-pc-cygwin
  * i686-pc-linux-gnu
  * x86_64-apple-darwin
  * x86_64-unknown-linux-gnu

Support for these systems includes a set of pre-built, qualified
versions of GNU autotools along with integration and automation
scripts to run them.

If support is required for a new POSIX-compatible build host system,
use the 'build' script in 'tools/packages' to unarchive, build, and
install the tools for your system.

Please see the FAQ section for more background on why this package
provides these pre-built tools.

Package Organization
--------------------

The nlbuild-autotools package is laid out as follows:

| Directory                         | Description                                                                              |
|-----------------------------------|------------------------------------------------------------------------------------------|
| autoconf/                         | GNU autoconf infrastructure provided by nlbuild-autotools.                               |
| autoconf/m4/                      | GNU m4 macros for configure.ac provided by nlbuild-autotools.                            |
| automake/                         | GNU automake Makefile.am header and footer infrastructure provided by nlbuild-autotools. |
| automake/post/                    | GNU automake Makefile.am footers.                                                        |
| automake/post.am                  | GNU automake Makefile.am footer included by every makefile.                              |
| automake/pre/                     | GNU automake Makefile.am headers.                                                        |
| automake/pre.am                   | GNU automake Makefile.am header included by every makefile.                              |
| examples/                         | Example template files for starting your own nlbuild-autotools-based project.            |
| scripts/                          | Automation scripts for regenerating the build system and for managing package versions.  |
| tools/                            | Qualified packages of and pre-built instances of GNU autotools.                          |
| tools/host/                          | Pre-built instances of GNU autotools.                                                    |
| tools/host/i686-pc-cygwin/           | Pre-built instances of GNU autotools for 32-bit Cygwin.                                  |
| tools/host/i686-pc-linux-gnu/        | Pre-built instances of GNU autotools for 32-bit Linux.                                   |
| tools/host/x86_64-apple-darwin/      | Pre-built instances of GNU autotools for 64-bit Mac OS X.                                |
| tools/host/x86_64-unknown-linux-gnu/ | Pre-built instances of GNU autotools for 64-bit Linux.                                   |
| tools/packages/                   | Qualified packages for GNU autotools.                                                    |

FAQ
---

Q: Why does nlbuild-autotools have its own built versions of GNU
   autotools rather than leveraging whatever versions exist on the build
   host system?

A: Some build host systems such as Mac OS X may not have GNU autotools
   at all. Other build host systems, such as Linux, may have different
   distributions or different versions of those distributions in which
   the versions of GNU autotools are apt to be different.

   This differences lead to different primary and secondary autotools
   output and, as a consequence, a divergent user and support
   experience. To avoid this, this package provides a pre-built,
   qualified set of GNU autotools along with a comprehensive,
   standalone set of scripts to drive them without reliance on those
   versions of the tools on the build host system.


