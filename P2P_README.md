# Guide

This project implements a simple peer-to-peer demo.

- Note: the wake-up frame is using the data frame rather than the multipurpose frame in this demo.

# Demo

Demo topology:

```
 WC1 (SSED) <------\
                   +---> WED (SSED) <----------> LEADER (FTD)
 WC2 (FED) <------/
```

Demo steps:

1. Setup LEADER and wait for it becoming a leader

2. Setup WED as a thread sleepy child and wait for it becoming a child

3. Setup WC1

4. WC1 wakes up WED and establishs a P2P link with WED

5. WED pings WC1

6. WC1 checks the registered SRP server hosts

7. Setup WC2

8. WC2 wakes up WED and establishs a P2P link with WED

9. WED pings WC2

10. WC2 checks the registered SRP server hosts

11. WC1 tears down the P2P link from WED

12. WC2 tears down the P2P link from WED

# Get P2P Demo Code

```bash
$ git clone https://github.com/openthread/openthread.git
$ cd openthread
$ git fetch https://github.com/openthread/openthread.git pull/12238/head
$ git checkout -b p2p FETCH_HEAD
```

# Simulation

Run the P2P demo on simulation nodes.

## Build

```bash
$ ./build.sh
```

## Run demo

Run the peer-to-peer demo on the simulation nodes.

```bash
$ expect ./tests/scripts/expect/cli-p2p-demo.exp
```

## Log

Check log on Linux:

```bash
$ tail -f  /var/log/syslog | grep ot-cli > log.txt
```
