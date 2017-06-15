Nest Labs Build - GNU Autotools
===============================

# Introduction

The Nest Labs Build - GNU Autotools (nlbuild-autotools) provides a
customized, turnkey build system framework, based on GNU autotools, for
standalone Nest Labs (or other) software packages that need to support
not only building on and targeting against standalone build host
systems but also embedded target systems using GCC-based or
-compatible toolchains.

# Users and System Integrators

## Getting Started

This project is typically subtreed (or git submoduled) into a target
project repository and serves as the seed for that project's build
system.

Assuming that you already have a project repository established in
git, perform the following in your project repository:

```
1. % git remote add nlbuild-autotools ssh://<PATH_TO_REPOSITORY>/nlbuild-autotools.git
2. % git fetch nlbuild-autotools
```

You can place the nlbuild-autotools package anywhere in your project;
however, by convention, "third_party/nlbuild-autotools/repo" is recommended:

```
3. % mkdir third_party
4. % git subtree add --prefix=third_party/nlbuild-autotools/repo --squash --message="Add subtree mirror of repository 'ssh://<PATH_TO_REPOSITORY>/nlbuild-autotools.git' branch 'master' at commit 'HEAD'." nlbuild-autotools HEAD
```

At this point, you now have the nlbuild-autotools package integrated
into your project. The next step is using the
nlbuild-autotools-provided examples as templates. To do this, a
convenience script has been provided that will get you started. You
can tune and customize the results, as needed, for your project. From
the top level of your project tree:

```
5. % third_party/nlbuild-autotools/repo/scripts/mkskeleton -I third_party/nlbuild-autotools/repo --package-description "My Fantastic Package" --package-name "mfp"
```

## Supported Build Host Systems

For Linux users, you likely already have GNU autotools installed through your system's distribution (e.g. Ubuntu). However, if your GNU autotools packages are incomplete or downrevision relative to what's required from nlbuild-autotools, nlbuild-autotools can be built and installed or can be downloaded and expanded in your local nlbuild-autotools tree.

The nlbuild-autotools system supports and has been tested against the
following POSIX-based build host systems:

  * i686-pc-cygwin
  * i686-pc-linux-gnu
  * x86_64-apple-darwin
  * x86_64-unknown-linux-gnu

### Build and Install

Simply invoke `make tools` at the top-level of your nlbuild-autotools tree.

### Download and Expand

Alongside nlbuild-autotools distributions are pre-built
architecture-independent and -dependent distributions of the form:

  * nlbuild-autotools-common-_version_.tar.{gz,xz}
  * nlbuild-autotools-_host_-_version_.tar.{gz,xz}

Find and download the appropriate pair of nlbuild-autotools-common and
nlbuild-autotools-_host_ for your system and then expand them from the
top-level of your nlbuild-autotools tree with a command similar to the
following example:

```
% tar --directory tools/host -zxvf nlbuild-autotools-common-1.1.tar.gz
% tar --directory tools/host -zxvf nlbuild-autotools-x86_64-unknown-linux-gnu-1.1.tar.gz
```

Please see the [FAQ](#FAQ) section for more background on why this package
provides options for these pre-built tools.

## Package Organization

The nlbuild-autotools package is laid out as follows:

| Directory                            | Description                                                                              |
|--------------------------------------|------------------------------------------------------------------------------------------|
| autoconf/                            | GNU autoconf infrastructure provided by nlbuild-autotools.                               |
| autoconf/m4/                         | GNU m4 macros for configure.ac provided by nlbuild-autotools.                            |
| automake/                            | GNU automake Makefile.am header and footer infrastructure provided by nlbuild-autotools. |
| automake/post/                       | GNU automake Makefile.am footers.                                                        |
| automake/post.am                     | GNU automake Makefile.am footer included by every makefile.                              |
| automake/pre/                        | GNU automake Makefile.am headers.                                                        |
| automake/pre.am                      | GNU automake Makefile.am header included by every makefile.                              |
| examples/                            | Example template files for starting your own nlbuild-autotools-based project.            |
| scripts/                             | Automation scripts for regenerating the build system and for managing package versions.  |
| tools/                               | Qualified packages of and pre-built instances of GNU autotools.                          |
| tools/host/                          | Pre-built instances of GNU autotools (if installed).                                     |
| tools/host/i686-pc-cygwin/           | Pre-built instances of GNU autotools for 32-bit Cygwin (if installed).                   |
| tools/host/i686-pc-linux-gnu/        | Pre-built instances of GNU autotools for 32-bit Linux (if installed).                    |
| tools/host/x86_64-apple-darwin/      | Pre-built instances of GNU autotools for 64-bit Mac OS X (if installed).                 |
| tools/host/x86_64-unknown-linux-gnu/ | Pre-built instances of GNU autotools for 64-bit Linux (if installed).                    |
| tools/packages/                      | Qualified packages for GNU autotools.                                                    |

# FAQ {#FAQ}

Q: Why does nlbuild-autotools have an option for its own built versions
   of GNU autotools rather than leveraging whatever versions exist on
   the build host system?

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

## Maintainers

If you are maintaining nlbuild-autotools, you have several key things to know:

1. Generating nlbuild-autotools distributions
2. Generating optional nlbuild-autotools prebuilt binary distributions.
3. Upgrading GNU autotools packages.

### Generating Distributions

To generate a nlbuild-autotools distribution, simply invoke:

```
% make dist
```

The package version will come from tags in the source code control
system, if available; otherwise, from '.default-version'. The version
can be overridden from the PACKAGE_VERSION or VERSION environment
variables.

The resulting archive will be at the top of the package build
directory.

### Generating Optional Prebuilt Binary Distributions

To generate an optional nlbuild-autotools prebuilt binary
distribution, simply invoke:

```
% make toolsdist
```

The package version will come from the source code control system, if
available; otherwise, from `.default-version`. The version can be
overridden from the _PACKAGE_VERSION_ or _VERSION_ environment variables.

The resulting archives will be at the top of the package build
directory.

### Upgrading GNU Autotools Packages

To change or upgrade GNU autotools packages used by nlbuild-autotools,
edit the metadata for each package in tools/packages/_package_/
_package_.{url,version}.

# Versioning

nlbuild-autools follows the [Semantic Versioning guidelines](http://semver.org/) 
for release cycle transparency and to maintain backwards compatibility.

# License

nlbuild-autools is released under the [Apache License, Version 2.0 license](https://opensource.org/licenses/Apache-2.0). 
See the `LICENSE` file for more information.