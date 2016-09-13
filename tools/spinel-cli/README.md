# Spinel CLI Reference

The Spinel CLI exposes the OpenThread configuration and management APIs 
running on an NCP build via a command line interface.  Spinel CLI is primarily
targeted for driving the automated continuous integration tests, and is
suitable for manual experimentation with controlling OpenThread NCP instances.
For a production grade host driver, see [wpantund]: https://github.com/openthread/wpantund.

Use the CLI to play with NCP builds of OpenThread on a Linux or Mac OS 
platform, including starting a basic tunnel interface to allow IPv6 
applications to run on the HOST and use the Thread network.

The power of this tool is three fold:

1. As a path to add testing of the NCP in simulation to continuous integration
2. As a path to automated testing of testbeds running NCP firmware on hardware
3. As a simple debugging tool for NCP builds of OpenThread

## System Requirements

| OS     | Minimum Version  |
|--------|------------------|
| Ubuntu | 14.04 Trusty     |
| Mac OS | 10.11 El Capitan |

| Language | Minimum Version  |
|----------|------------------|
| Python   | 2.7.10           |

### Package Installation

```
sudo easy_install pip
sudo pip install blessed
sudo pip install ipaddress
sudo pip install scapy
```

## Usage

### NAME
    spinel-cli.py - shell tool for controlling OpenThread NCP instances

### SYNOPSIS
    spinel-cli.py [-hupsnqv]

### DESCRIPTION

```
    -h, --help            
    	Show this help message and exit

    -u <UART>, --uart=<UART>
       	Open a serial connection to the OpenThread NCP device
	where <UART> is a device path such as "/dev/ttyUSB0".

    -p <PIPE>, --pipe=<PIPE>
        Open a piped process connection to the OpenThread NCP device
        where <PIPE> is the command to start an emulator, such as 
        "ot-ncp".  Spinel-cli will communicate with the child process
        via stdin/stdout.

    -s <SOCKET>, --socket=<SOCKET>
        Open a socket connection to the OpenThread NCP device
        where <SOCKET> is the port to open.
	This is useful for SPI configurations when used in conjunction
	with a spinel spi-driver daemon.
	Note: <SOCKET> will eventually map to hostname:port tuple.

    -n NODEID, --nodeid=NODEID
        The unique nodeid for the HOST and NCP instance.

    -q, --quiet
        Minimize debug and log output.

    -v, --verbose
        Maximize debug and log output.
```

## Quick Start

The spinel-cli tool provides an intuitive command line interface, including
all the standard OpenThread CLI commands, plus full history accessible by 
pressing the up/down keys, or searchable via ^R.  There are a few commands
that spinel-cli provides as well that aren't part of the standard set 
documented in the command reference section.

```
openthread$ cd tools/spinel-cli/
spinel-cli$ ./spinel-cli.py
Opening pipe to ../../examples/apps/ncp/ot-ncp 1
spinel-cli > help

Available commands (type help <name> for more information):
============================================================
channel            extaddr       mode              route
child              extpanid      netdataregister   router
childtimeout       h             networkidtimeout  routerupgradethreshold
clear              help          networkname       scan
contextreusedelay  history       panid             state
counter            ifconfig      ping              thread
debug              ipaddr        prefix            v
debug-term         keysequence   q                 version
eidcache           leaderdata    quit              whitelist
enabled            leaderweight  releaserouterid
exit               masterkey     rloc16

spinel-cli > version
OPENTHREAD/gd4d4e9d-dirty; Aug 11 2016 14:40:44
Done
spinel-cli > thread start
Done
spinel-cli > state
leader
Done
spinel-cli >
```

## Running the NCP Tests

The OpenThread automated test suite can be run against any of the following 
node types by passing the NODE_TYPE environment variable:

| NODE_TYPE     |    Description                                     |
|---------------|----------------------------------------------------|
| sim (default) | Runs against ot-cli posix emulator                 |
| ncp-sim       | Runs against ot-ncp posix emulator with spinel-cli |
| soc           | Runs against CLI firmware on a device connected via /dev/ttyUSB<nodeid>  |

### Manual run of NCP thread-cert test

```
# From top-level of openthread tree
./bootstrap
./configure --with-examples=posix --enable-cli --enable-ncp=uart
make
cd tests/scripts/thread-cert
NODE_TYPE=ncp-sim top_builddir=../../.. python Cert_5_1_02_ChildAddressTimeout.py VERBOSE=1
```

### Run entire NCP thread-cert suite

```
# From top-level of openthread tree
make distclean
./bootstrap
NODE_TYPE=ncp-sim BUILD_TARGET=posix-distcheck DISTCHECK_CONFIGURE_FLAGS="--with-examples=posix --enable-cli --enable-ncp=uart --with-tests=all" make -f examples/Makefile-posix distcheck BuildJobs=10 VERBOSE=1
```

## Command Reference

### OpenThread CLI Commands

The primary intent of spinel-cli is to support the exact syntax and output
of the OpenThread CLI command set in order to seamlessly reapply the 
thread-cert automated test suite against NCP targets.

See [cli module][1] for more information on these commands.

[1]:../../src/cli/README.md

### Diagnostics CLI Commands

The Diagnostics module is enabled only when building OpenThread with 
the --enable-diag configure option.

See [diag module][2] for more information on these commands.

[2]:../../src/diag/README.md


### NCP CLI Commands

These commands extend beyond the core OpenThread CLI, and are specific to 
the spinel-cli tool for the purposes of debugging, access to NCP-specific
Spinel parameters, and support of advanced configurations.

* [help](#help)
* [?](#help)
* [v](#v)
* [exit](#exit)
* [quit](#quit)
* [q](#quit)
* [clear](#clear)
* [history](#history)
* [h](#history)
* [debug](#debug)
* [debug-term](#debug-term)
* [ncp-tun](#ncp-tun)
* [ncp-ml64](#ncp-ml64)
* [ncp-ll64](#ncp-ll64)


#### help

Display help all top-level commands supported by spinel-cli.

```bash
spinel-cli > help

Available commands (type help <name> for more information):
============================================================
channel            diag-start   leaderdata        quit                  
child              diag-stats   leaderweight      releaserouterid       
childtimeout       diag-stop    masterkey         rloc16                
clear              discover     mode              route                 
contextreusedelay  eidcache     ncp-ll64          router                
counter            exit         ncp-ml64          routerupgradethreshold
debug              extaddr      ncp-tun           scan                  
debug-term         extpanid     netdataregister   state                 
diag               h            networkidtimeout  thread                
diag-channel       help         networkname       tun                   
diag-power         history      panid             v                     
diag-repeat        ifconfig     ping              version               
diag-send          ipaddr       prefix            whitelist             
diag-sleep         keysequence  q               

```

#### help \<command\>

Display detailed help on a specific command.

```bash
spinel-cli > help version

version

    Print the build version information.

    > version
    OPENTHREAD/gf4f2f04; Jul  1 2016 17:00:09
    Done
```

#### v

Display version of spinel-cli tool.

```bash
spinel-cli > v
spinel-cli ver. 0.1.0
Copyright (c) 2016 The OpenThread Authors.
```

#### exit

Exit spinel-cli.  CTRL+C is also okay.

#### quit

Exit spinel-cli.  CTRL+C is also okay.

### clear

Clear screen.

#### history

Display history of most recent commands run.

```bash
spinel-cli > history
ping fd00::1
quit
help
history
```

#### debug

Get whether debug verbose output is enabled.

```bash
spinel-cli > debug
DEBUG_ENABLE = 0
```

#### debug \<enabled\>

Set whether debug verbose output is enabled.

spinel-cli > debug
DEBUG_ENABLE = 0

```bash
spinel-cli > debug 1
DEBUG_ENABLE = 1
spinel-cli > version
TX Pay: (3) ['81', '02', '02'] 
RX Pay: (53) ['81', '06', '02', '4F', '50', '45', '4E', '54', '48', '52', '45', '41', '44', '2F', '67', '38', '62', '63', '34', '62', '31', '64', '2D', '64', '69', '72', '74', '79', '3B', '20', '41', '75', '67', '20', '33', '31', '20', '32', '30', '31', '36', '20', '31', '30', '3A', '34', '38', '3A', '35', '33', '00', '40', '33'] 
OPENTHREAD/g8bc4b1d-dirty; Aug 31 2016 10:48:53
Done
```

#### debug-term

Get whether debug terminal title bar is enabled.

#### debug-term \<enabled\>

Set whether debug terminal title bar is enabled.

#### ncp-tun

Control sideband tunnel interface.

#### ncp-tun up
        
Bring up Thread TUN interface.

```bash
spinel-cli > ncp-tun up
Done
```

#### ncp-tun down
        
Bring down Thread TUN interface.

```bash
spinel-cli > ncp-tun down
Done
```

#### ncp-tun add \<ipaddr\>

Add an IPv6 address to the Thread TUN interface.

```bash
spinel-cli > ncp-tun add 2001::dead:beef:cafe
Done
```

#### ncp-tun del \<ipaddr\>

Delete an IPv6 address from the Thread TUN interface.
        
```bash
spinel-cli > ncp-tun del 2001::dead:beef:cafe
Done
```

#### ncp-tun ping \<ipaddr\> \[size\] \[count\] \[interval\]

Send an ICMPv6 Echo Request via a posix host system call.

```bash
spinel-cli > ncp-tun ping fdde:ad00:beef:0:558:f56b:d688:799
16 bytes from fdde:ad00:beef:0:558:f56b:d688:799: icmp_seq=1 hlim=64 time=28ms
```

#### ncp-ml64

Return the Mesh Local 64-bit IPv6 address for the node.

```
spinel-cli > ncp-ml64
fdde:ad00:beef:0:558:f56b:d688:799
Done
```

#### ncp-ll64

Return the Link Local 64-bit IPv6 address for the node.
