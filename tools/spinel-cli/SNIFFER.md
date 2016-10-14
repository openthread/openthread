# Spinel Sniffer Reference

Any Spinel NCP node can be made into a promiscuous packet sniffer, and this 
tool both intializes a device into this mode and outputs a pcap stream that
can be saved or piped directly into Wireshark.

## System Requirements

The tool has been tested on the following platforms:

| Platforms | Version          |
|-----------|------------------|
| Ubuntu    | 14.04 Trusty     |
| Mac OS    | 10.11 El Capitan |

| Language  | Version          |
|-----------|------------------|
| Python    | 2.7.10           |

### Package Installation

```
sudo easy_install pip
sudo pip install ipaddress
sudo pip install scapy
sudo pip install pyserial
```

## Usage

### NAME
    sniffer.py - shell tool for controlling OpenThread NCP instances

### SYNOPSIS
    sniffer.py [-hupsnqvdxc]

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

    -n NODEID, --nodeid=<NODEID>
        The unique nodeid for the HOST and NCP instance.

    -q, --quiet
        Minimize debug and log output.

    -v, --verbose
        Maximize debug and log output.

    -d <DEBUG_LEVEL>, --debug=<DEBUG_LEVEL>
        Set the debug level.  Enabling debug output is typically coupled with -x.
           0: Supress all debug output.  Required to stream to Wireshark.
           1: Show spinel property changes and values.
           2: Show spinel IPv6 packet bytes.
           3: Show spinel raw packet bytes (after HDLC decoding).
           4: Show spinel HDLC bytes.
           5: Show spinel raw stream bytes: all serial traffic to NCP.

    -x, --hex
        Output packets as ASCII HEX rather than pcap.

    -c, --channel
        Set the channel upon which to listen.
```

## Quick Start

From openthread root:

```
    sudo ./tools/spinel-cli/sniffer.py -c 11 -n 1 -u /dev/ttyUSB0 | wireshark -k -i -
```

This will connect to stock openthread ncp firmware over the given UART, 
make the node into a promiscuous mode sniffer on the given channel, 
open up wireshark, and start streaming packets into wireshark. 

