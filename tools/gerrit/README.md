# git-squash-merge tool

`git-squash-merge` is a bash script to help squash merge a given branch into the current branch. This tool is helpful for synchronizing git repositories which work with `gerrit`.

This command squash merges all commits from a given branch into the current branch. By default the changes are committed with a commit message containing the list of all squashed commits.

## Syntax

```
git-squash-merge [--no-list] [--no-commit] <branch> [<commit msg>]"
```

## Parameters

- `<branch>` specifies the name of branch to merge into current branch.
- `<commit msg>` is an optional parameter specifying text to add to the commit message.

## Options

- `--no-list` when used the commit message will not include the list of squashed commits.
- `--no-commit` when used, the tool squashes and stages the changes but does not commit"

## Example of Use

```
~/sw/openthread $ ./tools/gerrit/git-squash-merge.sh github/master "OpenThread GitHub sync"
commit 977287e73a3fcae297306b1e68da50d4bbee0d14 (HEAD)
Author: Abtin Keshavarzian <abtink@google.com>
Date:   Tue Aug 14 14:53:33 2018 -0700

    Squash merge 'github/master' into 'HEAD'

    OpenThread GitHub sync

    3acd900bd [message] update comment/documentation (#2903)
    2aa335144 [mle] fix logging at NOTE level (#2933)
    a9932027f [toranj] test case for adding IPv6 addresses with same prefix on multiple nodes (#2915)
    c097cd2d3 [mle] remove unused LeaderDataTlv from HandleChildUpdateRequest (#2936)
    7be61df6e [message-info] remove unnecessary memset initialization (#2937)
    bcfa7edff [ip6] ensure slaac addresses are added back after reset (#2938)
    412f9740b [tlv] remove unnecessary user-specified constructor (#2940)
    ba4b238a0 [link-quality] explicitly clear link quality for new child/router (#2941)
    601b99b39 [android] use spi instead of uart (#2942)
    d67c4e2f6 [ncp] include prop set support for THREAD_CHILD_TIMEOUT in MTD build (#2943)
    44e583a55 [toranj] add new method to finalize all nodes at end of each test (#2930)
    c551e3035 [posix] handle signals SIGHUP or SIGTERM and exit (#2930)
    1ca81fbb1 [ncp] add support for child supervision in spinel/NcpBase (#2939)
    f0bb0982e [examples] change example platform namespacing (#2927)
    f089fc8e2 [gcc8] resolve compiler errors (#2944)
    a9d32b7be [ncp] add support for Commissioner APIs in spinel and NCP (#2911)
    8efb3c50e [meshcop] implement commissioning UDP proxy (#2926)
    492f0c3b1 [harness-automation] update read method for TopologyConfig.txt format change (#2950)
    e181f1f98 [posix-app] fix diag issues of radio only mode (#2948)
    90ee0a4d6 [ncp] add joiner spinel support (#2877)
    9d585edc4 [types] move types into specific headers (#2946)
    101e24337 [nrf52840] fix default parameters of FEM (#2945)
    c8d06c402 [efr32] fix em_system.h include (#2956)
    d2e59fd64 [posix-app] fix getting tx power (#2957)
    0c07c53ce [platform] attach timestamp in promiscuous mode (#2954)

    Change-Id: I2fd7b537fc1e0251082f091cb897b9ac25e105dd

Successfully squash merged branch 'github/master' into 'HEAD'
```
