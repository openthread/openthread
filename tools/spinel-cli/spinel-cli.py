#!/usr/bin/python
#
#  Copyright (c) 2016, Nest Labs, Inc.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#
""" 
spinel-cli.py

vailable commands (type help <name> for more information):
============================================================
channel            diag-sleep  ifconfig          q                     
child              diag-start  ipaddr            quit                  
childtimeout       diag-stats  keysequence       releaserouterid       
clear              diag-stop   leaderdata        rloc16                
contextreusedelay  discover    leaderweight      route                 
counter            eidcache    masterkey         router                
debug              enabled     mode              routerupgradethreshold
debug-term         exit        netdataregister   scan                  
diag               extaddr     networkidtimeout  state                 
diag-channel       extpanid    networkname       thread                
diag-power         h           panid             v                     
diag-repeat        help        ping              version               
diag-send          history     prefix            whitelist      
"""

__copyright__   = "Copyright (c) 2016 Nest Labs, Inc."
__version__     = "0.1.0"


FEATURE_USE_HDLC = 1

DEBUG_ENABLE = 0

DEBUG_LOG_TX = 0
DEBUG_LOG_RX = 0
DEBUG_LOG_HDLC = 0
DEBUG_LOG_PKT = DEBUG_ENABLE
DEBUG_LOG_PROP = DEBUG_ENABLE
DEBUG_LOG_TUN = 0
DEBUG_TERM = 0
DEBUG_CMD_RESPONSE = 0


TIMEOUT_PING = 2
TIMEOUT_PROP = 1

gWpanApi = None

def goodbye(signum=None, frame=None):
    logger.info('\nQuitting')
    if gWpanApi:
        gWpanApi.serial.close()
    exit(1)

import os
import sys
import time
import threading
import traceback

import blessed

import optparse
from optparse import OptionParser, Option, OptionValueError

import string
import shlex
import base64
import textwrap
import ipaddress

import logging
import logging.config
import logging.handlers
import traceback
import subprocess
import Queue

import serial
import socket
import signal
import fcntl

from cmd import Cmd
from copy import copy
from struct import pack
from struct import unpack
from select import select

logging.getLogger("scapy.runtime").setLevel(logging.ERROR)
from scapy.all import IPv6
from scapy.all import ICMPv6EchoRequest

try:
    import readline
except:
    pass

MASTER_PROMPT  = "spinel-cli"

DEFAULT_NODE_TYPE = 2    

logging.config.dictConfig({
    'version': 1,              
    'disable_existing_loggers': False, 

    'formatters': {
        'minimal': {
            'format': '%(message)s'
        },
        'standard': {
            'format': '%(asctime)s [%(levelname)s] %(name)s: %(message)s'
        },
    },
    'handlers': {
        'console': {
            #'level':'INFO',    
            'level':'DEBUG',    
            'class':'logging.StreamHandler',
        },  
        #'syslog': {
        #    'level':'DEBUG',    
        #    'class':'logging.handlers.SysLogHandler',
        #    'address': '/dev/log'
        #},  
    },
    'loggers': {
        '': {                  
            'handlers': ['console'], #,'syslog'],        
            'level': 'DEBUG',  
            'propagate': True  
        }
    }
})

logger = logging.getLogger(__name__)


# Terminal macros

class Color:
    END          = '\033[0m'
    BOLD         = '\033[1m'
    DIM          = '\033[2m'
    UNDERLINE    = '\033[4m'
    BLINK        = '\033[5m'
    REVERSE      = '\033[7m'

    CYAN         = '\033[96m'
    PURPLE       = '\033[95m'
    BLUE         = '\033[94m'
    YELLOW       = '\033[93m'
    GREEN        = '\033[92m'
    RED          = '\033[91m'

    BLACK        = "\033[30m"
    DARKRED      = "\033[31m"
    DARKGREEN    = "\033[32m"
    DARKYELLOW   = "\033[33m"
    DARKBLUE     = "\033[34m"
    DARKMAGENTA  = "\033[35m"
    DARKCYAN     = "\033[36m"
    WHITE        = "\033[37m"

IP_TYPE_UDP = 17
IP_TYPE_ICMPv6 = 58


SPINEL_RSSI_OVERRIDE            = 127

SPINEL_HEADER_ASYNC             = 0x80
SPINEL_HEADER_DEFAULT           = 0x81

# Spinel Commands

SPINEL_CMD_NOOP                 = 0
SPINEL_CMD_RESET                = 1
SPINEL_CMD_PROP_VALUE_GET       = 2
SPINEL_CMD_PROP_VALUE_SET       = 3
SPINEL_CMD_PROP_VALUE_INSERT    = 4
SPINEL_CMD_PROP_VALUE_REMOVE    = 5

SPINEL_RSP_PROP_VALUE_IS        = 6
SPINEL_RSP_PROP_VALUE_INSERTED  = 7
SPINEL_RSP_PROP_VALUE_REMOVED   = 8

SPINEL_CMD_NET_SAVE             = 9
SPINEL_CMD_NET_CLEAR            = 10
SPINEL_CMD_NET_RECALL           = 11

SPINEL_RSP_HBO_OFFLOAD          = 12
SPINEL_RSP_HBO_RECLAIM          = 13
SPINEL_RSP_HBO_DROP             = 14

SPINEL_CMD_HBO_OFFLOADED        = 15
SPINEL_CMD_HBO_RECLAIMED        = 16
SPINEL_CMD_HBO_DROPPED          = 17

SPINEL_CMD_NEST__BEGIN          = 15296
SPINEL_CMD_NEST__END            = 15360

SPINEL_CMD_VENDOR__BEGIN        = 15360
SPINEL_CMD_VENDOR__END          = 16384

SPINEL_CMD_EXPERIMENTAL__BEGIN  = 2000000
SPINEL_CMD_EXPERIMENTAL__END    = 2097152

# Spinel Properties

SPINEL_PROP_LAST_STATUS             = 0        #< status [i]
SPINEL_PROP_PROTOCOL_VERSION        = 1        #< major, minor [i,i]
SPINEL_PROP_NCP_VERSION             = 2        #< version string [U]
SPINEL_PROP_INTERFACE_TYPE          = 3        #< [i]
SPINEL_PROP_VENDOR_ID               = 4        #< [i]
SPINEL_PROP_CAPS                    = 5        #< capability list [A(i)]
SPINEL_PROP_INTERFACE_COUNT         = 6        #< Interface count [C]
SPINEL_PROP_POWER_STATE             = 7        #< PowerState [C]
SPINEL_PROP_HWADDR                  = 8        #< PermEUI64 [E]
SPINEL_PROP_LOCK                    = 9        #< PropLock [b]
SPINEL_PROP_HBO_MEM_MAX             = 10       #< Max offload mem [S]
SPINEL_PROP_HBO_BLOCK_MAX           = 11       #< Max offload block [S]

SPINEL_PROP_PHY__BEGIN              = 0x20
SPINEL_PROP_PHY_ENABLED             = SPINEL_PROP_PHY__BEGIN + 0 #< [b]
SPINEL_PROP_PHY_CHAN                = SPINEL_PROP_PHY__BEGIN + 1 #< [C]
SPINEL_PROP_PHY_CHAN_SUPPORTED      = SPINEL_PROP_PHY__BEGIN + 2 #< [A(C)]
SPINEL_PROP_PHY_FREQ                = SPINEL_PROP_PHY__BEGIN + 3 #< kHz [L]
SPINEL_PROP_PHY_CCA_THRESHOLD       = SPINEL_PROP_PHY__BEGIN + 4 #< dBm [c]
SPINEL_PROP_PHY_TX_POWER            = SPINEL_PROP_PHY__BEGIN + 5 #< [c]
SPINEL_PROP_PHY_RSSI                = SPINEL_PROP_PHY__BEGIN + 6 #< dBm [c]
SPINEL_PROP_PHY__END                = 0x30

SPINEL_PROP_MAC__BEGIN             = 0x30
SPINEL_PROP_MAC_SCAN_STATE         = SPINEL_PROP_MAC__BEGIN + 0 #< [C]
SPINEL_PROP_MAC_SCAN_MASK          = SPINEL_PROP_MAC__BEGIN + 1 #< [A(C)]
SPINEL_PROP_MAC_SCAN_PERIOD        = SPINEL_PROP_MAC__BEGIN + 2 #< ms-per-channel [S]
SPINEL_PROP_MAC_SCAN_BEACON        = SPINEL_PROP_MAC__BEGIN + 3 #< chan,rssi,(laddr,saddr,panid,lqi),(proto,xtra) [CcT(ESSC.)T(i).]
SPINEL_PROP_MAC_15_4_LADDR         = SPINEL_PROP_MAC__BEGIN + 4 #< [E]
SPINEL_PROP_MAC_15_4_SADDR         = SPINEL_PROP_MAC__BEGIN + 5 #< [S]
SPINEL_PROP_MAC_15_4_PANID         = SPINEL_PROP_MAC__BEGIN + 6 #< [S]
SPINEL_PROP_MAC_RAW_STREAM_ENABLED = SPINEL_PROP_MAC__BEGIN + 7 #< [C]
SPINEL_PROP_MAC_FILTER_MODE        = SPINEL_PROP_MAC__BEGIN + 8 #< [C]
SPINEL_PROP_MAC__END               = 0x40

SPINEL_PROP_MAC_EXT__BEGIN         = 0x1300
# Format: `A(T(Ec))`
# * `E`: EUI64 address of node
# * `c`: Optional fixed RSSI. -127 means not set.
SPINEL_PROP_MAC_WHITELIST          = SPINEL_PROP_MAC_EXT__BEGIN + 0
SPINEL_PROP_MAC_WHITELIST_ENABLED  = SPINEL_PROP_MAC_EXT__BEGIN + 1  #< [b]
SPINEL_PROP_MAC_EXT__END           = 0x1400
    
SPINEL_PROP_NET__BEGIN           = 0x40
SPINEL_PROP_NET_SAVED            = SPINEL_PROP_NET__BEGIN + 0 #< [b]
SPINEL_PROP_NET_IF_UP            = SPINEL_PROP_NET__BEGIN + 1 #< [b]
SPINEL_PROP_NET_STACK_UP         = SPINEL_PROP_NET__BEGIN + 2 #< [C]
SPINEL_PROP_NET_ROLE             = SPINEL_PROP_NET__BEGIN + 3 #< [C]
SPINEL_PROP_NET_NETWORK_NAME     = SPINEL_PROP_NET__BEGIN + 4 #< [U]
SPINEL_PROP_NET_XPANID           = SPINEL_PROP_NET__BEGIN + 5 #< [D]
SPINEL_PROP_NET_MASTER_KEY       = SPINEL_PROP_NET__BEGIN + 6 #< [D]
SPINEL_PROP_NET_KEY_SEQUENCE     = SPINEL_PROP_NET__BEGIN + 7 #< [L]
SPINEL_PROP_NET_PARTITION_ID     = SPINEL_PROP_NET__BEGIN + 8 #< [L]
SPINEL_PROP_NET__END             = 0x50

SPINEL_PROP_THREAD__BEGIN          = 0x50
SPINEL_PROP_THREAD_LEADER_ADDR     = SPINEL_PROP_THREAD__BEGIN + 0 #< [6]
SPINEL_PROP_THREAD_PARENT          = SPINEL_PROP_THREAD__BEGIN + 1 #< LADDR, SADDR [ES]
SPINEL_PROP_THREAD_CHILD_TABLE     = SPINEL_PROP_THREAD__BEGIN + 2 #< [A(T(ES))]
SPINEL_PROP_THREAD_LEADER_RID      = SPINEL_PROP_THREAD__BEGIN + 3 #< [C]
SPINEL_PROP_THREAD_LEADER_WEIGHT   = SPINEL_PROP_THREAD__BEGIN + 4 #< [C]
SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT = SPINEL_PROP_THREAD__BEGIN + 5 #< [C]
SPINEL_PROP_THREAD_NETWORK_DATA    = SPINEL_PROP_THREAD__BEGIN + 6 #< [D]
SPINEL_PROP_THREAD_NETWORK_DATA_VERSION = SPINEL_PROP_THREAD__BEGIN + 7 #< [S]
SPINEL_PROP_THREAD_STABLE_NETWORK_DATA  = SPINEL_PROP_THREAD__BEGIN + 8 #< [D]
SPINEL_PROP_THREAD_STABLE_NETWORK_DATA_VERSION = SPINEL_PROP_THREAD__BEGIN + 9  #< [S]
SPINEL_PROP_THREAD_ON_MESH_NETS    = SPINEL_PROP_THREAD__BEGIN + 10 #< array(ipv6prefix,prefixlen,stable,flags) [A(T(6CbC))]
SPINEL_PROP_THREAD_LOCAL_ROUTES    = SPINEL_PROP_THREAD__BEGIN + 11 #< array(ipv6prefix,prefixlen,stable,flags) [A(T(6CbC))]
SPINEL_PROP_THREAD_ASSISTING_PORTS = SPINEL_PROP_THREAD__BEGIN + 12 #< array(portn) [A(S)]
SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE = SPINEL_PROP_THREAD__BEGIN + 13 #< [b]
SPINEL_PROP_THREAD_MODE            = SPINEL_PROP_THREAD__BEGIN + 14
SPINEL_PROP_THREAD__END            = 0x60

SPINEL_PROP_THREAD_EXT__BEGIN      = 0x1500
SPINEL_PROP_THREAD_CHILD_TIMEOUT   = SPINEL_PROP_THREAD_EXT__BEGIN + 0  #< [L]
SPINEL_PROP_THREAD_RLOC16          = SPINEL_PROP_THREAD_EXT__BEGIN + 1  #< [S]
SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD = SPINEL_PROP_THREAD_EXT__BEGIN + 2 #< [C]
SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY = SPINEL_PROP_THREAD_EXT__BEGIN + 3 #< [L]
SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT =  SPINEL_PROP_THREAD_EXT__BEGIN + 4 #< [b]
SPINEL_PROP_THREAD_ACTIVE_ROUTER_IDS = SPINEL_PROP_THREAD_EXT__BEGIN + 5 #< [A(b)]
SPINEL_PROP_THREAD_RLOC16_DEBUG_PASSTHRU = SPINEL_PROP_THREAD_EXT__BEGIN + 6 #< [b]

SPINEL_PROP_THREAD_EXT__END        = 0x1600,


SPINEL_PROP_IPV6__BEGIN          = 0x60
SPINEL_PROP_IPV6_LL_ADDR         = SPINEL_PROP_IPV6__BEGIN + 0 #< [6]
SPINEL_PROP_IPV6_ML_ADDR         = SPINEL_PROP_IPV6__BEGIN + 1 #< [6C]
SPINEL_PROP_IPV6_ML_PREFIX       = SPINEL_PROP_IPV6__BEGIN + 2 #< [6C]
SPINEL_PROP_IPV6_ADDRESS_TABLE   = SPINEL_PROP_IPV6__BEGIN + 3 #< array(ipv6addr,prefixlen,valid,preferred,flags) [A(T(6CLLC))]
SPINEL_PROP_IPV6_ROUTE_TABLE     = SPINEL_PROP_IPV6__BEGIN + 4 #< array(ipv6prefix,prefixlen,iface,flags) [A(T(6CCC))]
SPINEL_PROP_IPv6_ICMP_PING_OFFLOAD = SPINEL_PROP_IPV6__BEGIN + 5 #< [b]

SPINEL_PROP_STREAM__BEGIN       = 0x70
SPINEL_PROP_STREAM_DEBUG        = SPINEL_PROP_STREAM__BEGIN + 0 #< [U]
SPINEL_PROP_STREAM_RAW          = SPINEL_PROP_STREAM__BEGIN + 1 #< [D]
SPINEL_PROP_STREAM_NET          = SPINEL_PROP_STREAM__BEGIN + 2 #< [D]
SPINEL_PROP_STREAM_NET_INSECURE = SPINEL_PROP_STREAM__BEGIN + 3 #< [D]
SPINEL_PROP_STREAM__END         = 0x80

# UART Bitrate
# Format: `L`
SPINEL_PROP_UART_BITRATE    = 0x100

# UART Software Flow Control
# Format: `b`
SPINEL_PROP_UART_XON_XOFF   = 0x101

SPINEL_PROP_15_4_PIB__BEGIN     = 1024
SPINEL_PROP_15_4_PIB_PHY_CHANNELS_SUPPORTED = SPINEL_PROP_15_4_PIB__BEGIN + 0x01 #< [A(L)]
SPINEL_PROP_15_4_PIB_MAC_PROMISCUOUS_MODE   = SPINEL_PROP_15_4_PIB__BEGIN + 0x51 #< [b]
SPINEL_PROP_15_4_PIB_MAC_SECURITY_ENABLED   = SPINEL_PROP_15_4_PIB__BEGIN + 0x5d #< [b]
SPINEL_PROP_15_4_PIB__END       = 1280

SPINEL_PROP_CNTR__BEGIN        = 1280

# Counter reset behavior
# Format: `C`
SPINEL_PROP_CNTR_RESET             = SPINEL_PROP_CNTR__BEGIN + 0

# The total number of transmissions.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_TX_PKT_TOTAL      = SPINEL_PROP_CNTR__BEGIN + 1

# The number of transmissions with ack request.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_TX_PKT_ACK_REQ    = SPINEL_PROP_CNTR__BEGIN + 2

# The number of transmissions that were acked.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_TX_PKT_ACKED      = SPINEL_PROP_CNTR__BEGIN + 3

# The number of transmissions without ack request.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_TX_PKT_NO_ACK_REQ = SPINEL_PROP_CNTR__BEGIN + 4

# The number of transmitted data.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_TX_PKT_DATA       = SPINEL_PROP_CNTR__BEGIN + 5

# The number of transmitted data poll.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_TX_PKT_DATA_POLL  = SPINEL_PROP_CNTR__BEGIN + 6

# The number of transmitted beacon.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_TX_PKT_BEACON     = SPINEL_PROP_CNTR__BEGIN + 7

# The number of transmitted beacon request.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_TX_PKT_BEACON_REQ = SPINEL_PROP_CNTR__BEGIN + 8

# The number of transmitted other types of frames.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_TX_PKT_OTHER      = SPINEL_PROP_CNTR__BEGIN + 9

# The number of retransmission times.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_TX_PKT_RETRY      = SPINEL_PROP_CNTR__BEGIN + 10

# The number of CCA failure times.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_TX_ERR_CCA        = SPINEL_PROP_CNTR__BEGIN + 11

# The total number of received packets.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_PKT_TOTAL      = SPINEL_PROP_CNTR__BEGIN + 100

# The number of received data.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_PKT_DATA       = SPINEL_PROP_CNTR__BEGIN + 101

# The number of received data poll.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_PKT_DATA_POLL  = SPINEL_PROP_CNTR__BEGIN + 102

# The number of received beacon.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_PKT_BEACON     = SPINEL_PROP_CNTR__BEGIN + 103

# The number of received beacon request.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_PKT_BEACON_REQ = SPINEL_PROP_CNTR__BEGIN + 104

# The number of received other types of frames.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_PKT_OTHER      = SPINEL_PROP_CNTR__BEGIN + 105

# The number of received packets filtered by whitelist.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_PKT_FILT_WL    = SPINEL_PROP_CNTR__BEGIN + 106

# The number of received packets filtered by destination check.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_PKT_FILT_DA    = SPINEL_PROP_CNTR__BEGIN + 107

# The number of received packets that are empty.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_ERR_EMPTY      = SPINEL_PROP_CNTR__BEGIN + 108

# The number of received packets from an unknown neighbor.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_ERR_UKWN_NBR   = SPINEL_PROP_CNTR__BEGIN + 109

# The number of received packets whose source address is invalid.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_ERR_NVLD_SADDR = SPINEL_PROP_CNTR__BEGIN + 110

# The number of received packets with a security error.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_ERR_SECURITY   = SPINEL_PROP_CNTR__BEGIN + 111

# The number of received packets with a checksum error.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_ERR_BAD_FCS    = SPINEL_PROP_CNTR__BEGIN + 112
    
# The number of received packets with other errors.
# Format: `L` (Read-only) */
SPINEL_PROP_CNTR_RX_ERR_OTHER      = SPINEL_PROP_CNTR__BEGIN + 113


#=========================================

kDeviceRoleDisabled = 0  #< The Thread stack is disabled.
kDeviceRoleDetached = 1  #< Not currently in a Thread network/partition.
kDeviceRoleChild = 2     #< The Thread Child role.
kDeviceRoleRouter = 3    #< The Thread Router role.
kDeviceRoleLeader = 4    #< The Thread Leader role.

THREAD_STATE_MAP = {
    kDeviceRoleDisabled: "disabled",
    kDeviceRoleDetached: "detached",
    kDeviceRoleChild: "child",
    kDeviceRoleRouter: "router",
    kDeviceRoleLeader: "leader",
}

THREAD_STATE_NAME_MAP = {
    "disabled": kDeviceRoleDisabled,
    "detached": kDeviceRoleDetached,
    "child": kDeviceRoleChild,
    "router": kDeviceRoleRouter,
    "leader": kDeviceRoleLeader,
}

#=========================================

kThreadPrefixPreferenceOffset  = 6
kThreadPrefixPreferredFlag     = 1 << 5
kThreadPrefixSlaacFlag         = 1 << 4
kThreadPrefixDhcpFlag          = 1 << 3
kThreadPrefixConfigureFlag     = 1 << 2
kThreadPrefixDefaultRouteFlag  = 1 << 1
kThreadPrefixOnMeshFlag        = 1 << 0

#=========================================

SPINEL_LAST_STATUS_MAP = {
    0: "STATUS_OK: Operation has completed successfully.",
    1: "STATUS_FAILURE: Operation has failed for some undefined reason.",
    2: "STATUS_UNIMPLEMENTED: The given operation has not been implemented.",
    3: "STATUS_INVALID_ARGUMENT: An argument to the given operation is invalid.",
    4: "STATUS_INVALID_STATE : The given operation is invalid for the current state of the device.",
    5: "STATUS_INVALID_COMMAND: The given command is not recognized.",
    6: "STATUS_INVALID_INTERFACE: The given Spinel interface is not supported.",
    7: "STATUS_INTERNAL_ERROR: An internal runtime error has occured.",
    8: "STATUS_SECURITY_ERROR: A security or authentication error has occured.",
    9: "STATUS_PARSE_ERROR: An error has occured while parsing the command.",
    10: "STATUS_IN_PROGRESS: The operation is in progress and will be completed asynchronously.",
    11: "STATUS_NOMEM: The operation has been prevented due to memory pressure.",
    12: "STATUS_BUSY: The device is currently performing a mutually exclusive operation.",
    13: "STATUS_PROPERTY_NOT_FOUND: The given property is not recognized.",
    14: "STATUS_PACKET_DROPPED: The packet was dropped.",
    15: "STATUS_EMPTY: The result of the operation is empty.",
    16: "STATUS_CMD_TOO_BIG: The command was too large to fit in the internal buffer.",
    17: "STATUS_NO_ACK: The packet was not acknowledged.",
    18: "STATUS_CCA_FAILURE: The packet was not sent due to a CCA failure.",

    112: "STATUS_RESET_POWER_ON",
    113: "STATUS_RESET_EXTERNAL",
    114: "STATUS_RESET_SOFTWARE",
    115: "STATUS_RESET_FAULT",
    116: "STATUS_RESET_CRASH",
    117: "STATUS_RESET_ASSERT",
    118: "STATUS_RESET_OTHER",
    119: "STATUS_RESET_UNKNOWN",
    120: "STATUS_RESET_WATCHDOG",

    0x4000: "kThreadError_None",
    0x4001: "kThreadError_Failed",
    0x4002: "kThreadError_Drop",
    0x4003: "kThreadError_NoBufs",
    0x4004: "kThreadError_NoRoute",
    0x4005: "kThreadError_Busy",
    0x4006: "kThreadError_Parse",
    0x4007: "kThreadError_InvalidArgs",
    0x4008: "kThreadError_Security",
    0x4009: "kThreadError_AddressQuery",
    0x400A: "kThreadError_NoAddress",
    0x400B: "kThreadError_NotReceiving",
    0x400C: "kThreadError_Abort",
    0x400D: "kThreadError_NotImplemented",
    0x400E: "kThreadError_InvalidState",
    0x400F: "kThreadError_NoTasklets",

}

#=========================================


class DiagsTerminal(blessed.Terminal):
    def print_title(self, strings=[]):
        clr = term.green_reverse
        title = term.white_reverse("  spinel-cli  ")
        with term.location(x=0, y=0):
            print (clr + term.center(title+clr))
            for string in strings:
                print (term.ljust(string))
            print (term.ljust(" ") + term.normal)

term = DiagsTerminal()

#=========================================
    
def hexify_chr(s): return "%02X" % ord(s)
def hexify_int(i): return "%02X" % i
def hexify_bytes(data): return str(map(hexify_chr,data))
def hexify_str(s,delim=':'): 
    return delim.join(x.encode('hex') for x in s)

def asciify_int(i): return "%c" % (i)

def hex_to_bytes(s):
    result = ''
    for i in xrange(0, len(s), 2):
        (b1, b2) = s[i:i+2]
        hex = b1+b2
        v = int(hex, 16)
        result += chr(v)
    return result

def print_stack():
    for line in traceback.format_stack():
        print line.strip()

#=========================================

class IStream():
    def read(self, size): pass
    def write(self, data): pass
    def close(self): pass

class StreamSerial(IStream):
    def __init__(self, dev, baudrate=115200):
        comm = serial.Serial(dev, baudrate, timeout=1)
        logger.debug("TX Raw: (%d) %s" % (len(data), hexify_bytes(data)))

    def read(self, size=1):
        b = self.sock.recv(size)
        if DEBUG_LOG_RX:
            logger.debug("RX Raw: "+hexify_bytes(b))        
        return b

class StreamPipe(IStream):
    def __init__(self, filename):        
        """ Create a stream object from a piped system call """
        self.pipe = subprocess.Popen(filename, shell = True,
                                     stdin = subprocess.PIPE,
                                     stdout = subprocess.PIPE)

    def write(self, data):
        if DEBUG_LOG_TX:
            logger.debug("TX Raw: (%d) %s" % (len(data), hexify_bytes(data)))
        self.pipe.stdin.write(data)

    def read(self, size=1):
        """ Blocking read on stream object """
        for b in iter(lambda: self.pipe.stdout.read(size), ''):
            if DEBUG_LOG_RX:
                logger.debug("RX Raw: "+hexify_bytes(b))        
            return ord(b)

    def close(self):
        if self.pipe:
            self.pipe.kill()
            self.pipe = None

def StreamOpen(type, descriptor):
    """ 
    Factory function that creates and opens a stream connection.

    type:
        'u' = uart (/dev/tty#)
        's' = socket (port #)
        'p' = pipe (stdin/stdout)

    descriptor:
        uart - filename of device (/dev/tty#)
        socket - port to open connection to on localhost
        pipe - filename of command to execute and bind via stdin/stdout
    """

    if (type == 'p'): 
        print "Opening pipe to "+str(descriptor)
        return StreamPipe(descriptor)

    elif (type == 's'):
        port = int(descriptor)
        hostname = "localhost"
        print "Opening socket to "+hostname+":"+str(port)
        return StreamSocket(hostname, port)

    elif (type == 'u'):        
        dev = str(descriptor)
        baudrate = 115200
        print "Opening serial to "+dev+" @ "+str(baudrate)
        return StreamSerial(dev, baudrate)
    
    else:
        return None

#=========================================

HDLC_FLAG     = 0x7e
HDLC_ESCAPE   = 0x7d

# RFC 1662 Appendix C

HDLC_FCS_INIT = 0xFFFF
HDLC_FCS_POLY = 0x8408
HDLC_FCS_GOOD = 0xF0B8

def mkfcstab():
    P = HDLC_FCS_POLY

    def valiter():
        for b in range(256):
            v = b
            i = 8
            while i:
                v = (v >> 1) ^ P if v & 1 else v >> 1
                i -= 1

            yield v & 0xFFFF

    return tuple(valiter())

fcstab = mkfcstab()


def fcs16(byte, fcs):
    fcs = (fcs >> 8) ^ fcstab[(fcs ^ byte) & 0xff]
    return fcs


class Hdlc(IStream):
    def __init__(self, stream):
        self.stream = stream
                    
    def collect(self):
        fcs = HDLC_FCS_INIT
        packet = []
        if DEBUG_LOG_HDLC: raw = []

        # Synchronize
        while 1:
            b = self.stream.read()
            if DEBUG_LOG_HDLC: raw.append(b)
            if (b == HDLC_FLAG): break

        # Read packet, updating fcs, and escaping bytes as needed
        while 1:
            b = self.stream.read()
            if DEBUG_LOG_HDLC: raw.append(b)
            if (b == HDLC_FLAG): break
            if (b == HDLC_ESCAPE):
                b = self.stream.read()
                if DEBUG_LOG_HDLC: raw.append(b)
                b ^= 0x20
            packet.append(b)
            fcs = fcs16(b, fcs)
            #print("State: "+str(b)+ "  FCS: 0x"+hexify_int(fcs))

        #print("Fcs: 0x"+hexify_int(fcs))

        if DEBUG_LOG_HDLC:
            logger.debug("RX Hdlc: "+str(map(hexify_int,raw)))

        if (fcs != HDLC_FCS_GOOD):
            packet = None

        return packet

    def encode_b(self, b, packet = []):
        if (b == HDLC_ESCAPE) or (b == HDLC_FLAG):
            packet.append(HDLC_ESCAPE)
            packet.append(b ^ 0x20)
        else:
            packet.append(b)
        return packet

    def encode(self, payload = ""):
        fcs = HDLC_FCS_INIT
        packet = []
        packet.append(HDLC_FLAG)
        for b in payload:  
            b = ord(b)
            fcs = fcs16(b, fcs)
            packet = self.encode_b(b, packet)

        fcs ^= 0xffff;
        b = fcs & 0xFF
        packet = self.encode_b(b, packet)
        b = fcs >> 8
        packet = self.encode_b(b, packet)
        packet.append(HDLC_FLAG)
        packet = pack("%dB" % len(packet), *packet)

        if DEBUG_LOG_HDLC:
            logger.debug("TX Hdlc: "+hexify_bytes(packet))
        return packet

#=========================================

#  0: DATATYPE_NULL
#'.': DATATYPE_VOID: Empty data type. Used internally.
#'b': DATATYPE_BOOL: Boolean value. Encoded in 8-bits as either 0x00 or 0x01. All other values are illegal.
#'C': DATATYPE_UINT8: Unsigned 8-bit integer.
#'c': DATATYPE_INT8: Signed 8-bit integer.
#'S': DATATYPE_UINT16: Unsigned 16-bit integer. (Little-endian)
#'s': DATATYPE_INT16: Signed 16-bit integer. (Little-endian)
#'L': DATATYPE_UINT32: Unsigned 32-bit integer. (Little-endian)
#'l': DATATYPE_INT32: Signed 32-bit integer. (Little-endian)
#'i': DATATYPE_UINT_PACKED: Packed Unsigned Integer. (See section 7.2)
#'6': DATATYPE_IPv6ADDR: IPv6 Address. (Big-endian)
#'E': DATATYPE_EUI64: EUI-64 Address. (Big-endian)
#'e': DATATYPE_EUI48: EUI-48 Address. (Big-endian)
#'D': DATATYPE_DATA: Arbitrary Data. (See section 7.3)
#'U': DATATYPE_UTF8: Zero-terminated UTF8-encoded string.
#'T': DATATYPE_STRUCT: Structured datatype. Compound type. (See section 7.4)
#'A': DATATYPE_ARRAY: Array of datatypes. Compound type. (See section 7.5)

class SpinelCodec():
    def parse_b(self, payload): return unpack("<B", payload[:1])[0]
    def parse_c(self, payload): return unpack("<b", payload[:1])[0]
    def parse_C(self, payload): return unpack("<B", payload[:1])[0]
    def parse_s(self, payload): return unpack("<h", payload[:2])[0]
    def parse_S(self, payload): return unpack("<H", payload[:2])[0]
    def parse_l(self, payload): return unpack("<l", payload[:4])[0]
    def parse_L(self, payload): return unpack("<L", payload[:4])[0]

    def parse_6(self, payload): return payload[:16]
    def parse_E(self, payload): return payload[:8]
    def parse_e(self, payload): return payload[:6]
    def parse_U(self, payload): return payload[:-2]   # remove FCS16 from end
    def parse_D(self, payload): return payload[:-2]   # remove FCS16 from end

    def parse_i(self, payload):
        """ Decode EXI integer format. """
        op = 0
        op_len = 0
        op_mul = 1
        
        while op_len < 4:
            b = ord(payload[op_len])
            op += (b & 0x7F) * op_mul
            if (b < 0x80): break
            op_mul *= 0x80
            op_len += 1

        return (op, op_len+1)
            
    def parse_field(self, payload, format):
        dispath_map = {
            'b': self.parse_b,
            'c': self.parse_c,
            'C': self.parse_C,
            's': self.parse_s,
            'S': self.parse_S,
            'L': self.parse_L,
            'l': self.parse_l,
            '6': self.parse_6,
            'E': self.parse_E,
            'e': self.parse_e,
            'U': self.parse_U,
            'D': self.parse_D,
            'i': self.parse_i,
        }
        try:
            return dispatch_map[format[0]](payload)
        except:
            print_stack()
            return None

    def encode_i(self, data):
        """ Encode EXI integer format. """
        result = ""
        while (data):
            v = data & 0x7F
            data >>= 7
            if data: v |= 0x80
            result = result + pack("<B", v)
        return result

    def encode(self, cmd_id, payload = None):
        """ Encode the given payload as a Spinel frame. """
        header = pack(">B", SPINEL_HEADER_DEFAULT)
        cmd = self.encode_i(cmd_id)
        pkt = header + cmd + payload
        return pkt


#=========================================

class SpinelPropertyHandler(SpinelCodec):

    def LAST_STATUS(self, payload):           return self.parse_i(payload)[0]
    def PROTOCOL_VERSION(self, payload):      pass
    def NCP_VERSION(self, payload):           return self.parse_U(payload)
    def INTERFACE_TYPE(self, payload):        return self.parse_i(payload)[0]
    def VENDOR_ID(self, payload):             return self.parse_i(payload)[0]
    def CAPS(self, payload):                  pass
    def INTERFACE_COUNT(self, payload):       return self.parse_C(payload)
    def POWER_STATE(self, payload):           return self.parse_C(payload)
    def HWADDR(self, payload):                return self.parse_E(payload)
    def LOCK(self, payload):                  return self.parse_b(payload)

    def HBO_MEM_MAX(self, payload):           return self.parse_L(payload)
    def HBO_BLOCK_MAX(self, payload):         return self.parse_S(payload)

    def PHY_ENABLED(self, payload):           return self.parse_b(payload)
    def PHY_CHAN(self, payload):              return self.parse_C(payload)
    def PHY_CHAN_SUPPORTED(self, payload):    pass
    def PHY_FREQ(self, payload):              return self.parse_L(payload)
    def PHY_CCA_THRESHOLD(self, payload):     return self.parse_c(payload)
    def PHY_TX_POWER(self, payload):          return self.parse_c(payload)
    def PHY_RSSI(self, payload):              return self.parse_c(payload)

    def MAC_SCAN_STATE(self, payload):        return self.parse_C(payload)
    def MAC_SCAN_MASK(self, payload):         return self.parse_U(payload)
    def MAC_SCAN_PERIOD(self, payload):       return self.parse_S(payload)
    def MAC_SCAN_BEACON(self, payload):       return self.parse_U(payload)
    def MAC_15_4_LADDR(self, payload):        return self.parse_E(payload)
    def MAC_15_4_SADDR(self, payload):        return self.parse_S(payload)
    def MAC_15_4_PANID(self, payload):        return self.parse_S(payload)
    def MAC_RAW_STREAM_ENABLED(self, payload):return self.parse_b(payload)
    def MAC_FILTER_MODE(self, payload):       return self.parse_C(payload)

    def MAC_WHITELIST(self, payload):         pass
    def MAC_WHITELIST_ENABLED(self, payload): return self.parse_b(payload)

    def NET_SAVED(self, payload):             return self.parse_b(payload)
    def NET_IF_UP(self, payload):             return self.parse_b(payload)
    def NET_STACK_UP(self, payload):          return self.parse_C(payload)
    def NET_ROLE(self, payload):              return self.parse_C(payload)
    def NET_NETWORK_NAME(self, payload):      return self.parse_U(payload)
    def NET_XPANID(self, payload):            return self.parse_D(payload)
    def NET_MASTER_KEY(self, payload):        return self.parse_D(payload)
    def NET_KEY_SEQUENCE(self, payload):      return self.parse_L(payload)
    def NET_PARTITION_ID(self, payload):      return self.parse_L(payload)

    def THREAD_LEADER_ADDR(self, payload):    return self.parse_6(payload)
    def THREAD_PARENT(self, payload):         pass
    def THREAD_CHILD_TABLE(self, payload):    return self.parse_D(payload)

    def THREAD_LEADER_RID(self, payload):     return self.parse_C(payload)
    def THREAD_LEADER_WEIGHT(self, payload):  return self.parse_C(payload)
    def THREAD_LOCAL_LEADER_WEIGHT(self, payload): 
        return self.parse_C(payload)

    def THREAD_NETWORK_DATA(self, payload):          
        return self.parse_D(payload)

    def THREAD_NETWORK_DATA_VERSION(self, payload):  pass
    def THREAD_STABLE_NETWORK_DATA(self, payload):   pass
    def THREAD_STABLE_NETWORK_DATA_VERSION(self, payload): pass
    def THREAD_ON_MESH_NETS(self, payload):          pass
    def THREAD_LOCAL_ROUTES(self, payload):          pass
    def THREAD_ASSISTING_PORTS(self, payload):       pass
    def THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(self, payload): 
        return self.parse_b(payload)

    def THREAD_MODE(self, payload):            return self.parse_C(payload)

    def THREAD_CHILD_TIMEOUT(self, payload):   return self.parse_L(payload)
    def THREAD_RLOC16(self, payload):          return self.parse_S(payload)

    def THREAD_ROUTER_UPGRADE_THRESHOLD(self, payload): 
        return self.parse_C(payload)

    def THREAD_CONTEXT_REUSE_DELAY(self, payload):   
        return self.parse_L(payload)

    def THREAD_NETWORK_ID_TIMEOUT(self, payload): return self.parse_C(payload)
    def THREAD_ACTIVE_ROUTER_IDS(self, payload): return self.parse_D(payload)
    def THREAD_RLOC16_DEBUG_PASSTHRU(self, payload):   
        return self.parse_b(payload)
    
    def IPV6_LL_ADDR(self, payload):          return self.parse_6(payload)
    def IPV6_ML_ADDR(self, payload):          return self.parse_6(payload)
    def IPV6_ML_PREFIX(self, payload):        return self.parse_E(payload)

    def IPV6_ADDRESS_TABLE(self, payload):    return self.parse_D(payload)
    def IPV6_ROUTE_TABLE(self, payload):      return self.parse_D(payload)
    def IPv6_ICMP_PING_OFFLOAD(self, payload): return self.parse_b(payload)

    def STREAM_DEBUG(self, payload):          return self.parse_U(payload)
    def STREAM_RAW(self, payload):            return self.parse_D(payload)
    def STREAM_NET(self, payload):            return self.parse_D(payload)
    def STREAM_NET_INSECURE(self, payload):   return self.parse_D(payload)


#=========================================

class SpinelCommandHandler(SpinelCodec):

    def handle_prop(self, name, payload, tid): 
        global gWpanApi

        (prop_op, prop_len) = self.parse_i(payload)

        try:
            handler = SPINEL_PROP_DISPATCH[prop_op]
            prop_name = handler.__name__
            prop_value = handler(payload[prop_len:])
                
            if DEBUG_LOG_PROP:
                    
                # Generic output
                if isinstance(prop_value, basestring):
                    prop_str = hexify_str(prop_value)
                else:
                    prop_str = str(prop_value)
                    
                    logger.debug("PROP_VALUE_"+name+": "+prop_name+
                                 " = "+prop_str)

                # Extend output for certain properties.
                if (prop_op == SPINEL_PROP_LAST_STATUS):
                    logger.debug(SPINEL_LAST_STATUS_MAP[prop_value])

                elif (prop_op == SPINEL_PROP_STREAM_NET):
                    pkt = IPv6(prop_value[2:])
                    pkt.show()

            if gWpanApi: 
                gWpanApi.queue_add(prop_op, prop_value, tid)
            else:
                print "no wpanApi"

        except:
            prop_name = "Property Unknown"
            logger.info ("\n%s (%i): " % (prop_name, prop_op))
            print traceback.format_exc()            


    def PROP_VALUE_IS(self, payload, tid): 
        self.handle_prop("IS", payload, tid)

    def PROP_VALUE_INSERTED(self, payload, tid):
        self.handle_prop("INSERTED", payload, tid)

    def PROP_VALUE_REMOVED(self, payload, tid):
        self.handle_prop("REMOVED", payload, tid)


wpanHandler = SpinelCommandHandler()

SPINEL_COMMAND_DISPATCH = {
    SPINEL_RSP_PROP_VALUE_IS: wpanHandler.PROP_VALUE_IS,
    SPINEL_RSP_PROP_VALUE_INSERTED: wpanHandler.PROP_VALUE_INSERTED,
    SPINEL_RSP_PROP_VALUE_REMOVED: wpanHandler.PROP_VALUE_REMOVED,
}

wpanPropHandler = SpinelPropertyHandler()

SPINEL_PROP_DISPATCH = {
    SPINEL_PROP_LAST_STATUS:           wpanPropHandler.LAST_STATUS,
    SPINEL_PROP_PROTOCOL_VERSION:      wpanPropHandler.PROTOCOL_VERSION,
    SPINEL_PROP_NCP_VERSION:           wpanPropHandler.NCP_VERSION,
    SPINEL_PROP_INTERFACE_TYPE:        wpanPropHandler.INTERFACE_TYPE,
    SPINEL_PROP_VENDOR_ID:             wpanPropHandler.VENDOR_ID,
    SPINEL_PROP_CAPS:                  wpanPropHandler.CAPS,
    SPINEL_PROP_INTERFACE_COUNT:       wpanPropHandler.INTERFACE_COUNT,
    SPINEL_PROP_POWER_STATE:           wpanPropHandler.POWER_STATE,
    SPINEL_PROP_HWADDR:                wpanPropHandler.HWADDR, 
    SPINEL_PROP_LOCK:                  wpanPropHandler.LOCK,
    SPINEL_PROP_HBO_MEM_MAX:           wpanPropHandler.HBO_MEM_MAX,
    SPINEL_PROP_HBO_BLOCK_MAX:         wpanPropHandler.HBO_BLOCK_MAX,

    SPINEL_PROP_PHY_ENABLED:           wpanPropHandler.PHY_ENABLED,
    SPINEL_PROP_PHY_CHAN:              wpanPropHandler.PHY_CHAN,
    SPINEL_PROP_PHY_CHAN_SUPPORTED:    wpanPropHandler.PHY_CHAN_SUPPORTED,
    SPINEL_PROP_PHY_FREQ:              wpanPropHandler.PHY_FREQ,
    SPINEL_PROP_PHY_CCA_THRESHOLD:     wpanPropHandler.PHY_CCA_THRESHOLD,
    SPINEL_PROP_PHY_TX_POWER:          wpanPropHandler.PHY_TX_POWER,
    SPINEL_PROP_PHY_RSSI:              wpanPropHandler.PHY_RSSI,

    SPINEL_PROP_MAC_SCAN_STATE:        wpanPropHandler.MAC_SCAN_STATE,
    SPINEL_PROP_MAC_SCAN_MASK:         wpanPropHandler.MAC_SCAN_MASK,
    SPINEL_PROP_MAC_SCAN_PERIOD:       wpanPropHandler.MAC_SCAN_PERIOD,
    SPINEL_PROP_MAC_SCAN_BEACON:       wpanPropHandler.MAC_SCAN_BEACON,
    SPINEL_PROP_MAC_15_4_LADDR:        wpanPropHandler.MAC_15_4_LADDR,
    SPINEL_PROP_MAC_15_4_SADDR:        wpanPropHandler.MAC_15_4_SADDR,
    SPINEL_PROP_MAC_15_4_PANID:        wpanPropHandler.MAC_15_4_PANID,
    SPINEL_PROP_MAC_RAW_STREAM_ENABLED:wpanPropHandler.MAC_RAW_STREAM_ENABLED,
    SPINEL_PROP_MAC_FILTER_MODE:       wpanPropHandler.MAC_FILTER_MODE,

    SPINEL_PROP_MAC_WHITELIST:         wpanPropHandler.MAC_WHITELIST,
    SPINEL_PROP_MAC_WHITELIST_ENABLED: wpanPropHandler.MAC_WHITELIST_ENABLED,

    SPINEL_PROP_NET_SAVED:             wpanPropHandler.NET_SAVED,
    SPINEL_PROP_NET_IF_UP:             wpanPropHandler.NET_IF_UP,
    SPINEL_PROP_NET_STACK_UP:          wpanPropHandler.NET_STACK_UP,
    SPINEL_PROP_NET_ROLE:              wpanPropHandler.NET_ROLE,
    SPINEL_PROP_NET_NETWORK_NAME:      wpanPropHandler.NET_NETWORK_NAME,
    SPINEL_PROP_NET_XPANID:            wpanPropHandler.NET_XPANID,
    SPINEL_PROP_NET_MASTER_KEY:        wpanPropHandler.NET_MASTER_KEY,
    SPINEL_PROP_NET_KEY_SEQUENCE:      wpanPropHandler.NET_KEY_SEQUENCE,
    SPINEL_PROP_NET_PARTITION_ID:      wpanPropHandler.NET_PARTITION_ID,

    SPINEL_PROP_THREAD_LEADER_ADDR: wpanPropHandler.THREAD_LEADER_ADDR,
    SPINEL_PROP_THREAD_PARENT: wpanPropHandler.THREAD_PARENT,
    SPINEL_PROP_THREAD_CHILD_TABLE:  wpanPropHandler.THREAD_CHILD_TABLE,
    SPINEL_PROP_THREAD_LEADER_RID: wpanPropHandler.THREAD_LEADER_RID,      
    SPINEL_PROP_THREAD_LEADER_WEIGHT: wpanPropHandler.THREAD_LEADER_WEIGHT,
    SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT: wpanPropHandler.THREAD_LOCAL_LEADER_WEIGHT,
    SPINEL_PROP_THREAD_NETWORK_DATA: wpanPropHandler.THREAD_NETWORK_DATA,
    SPINEL_PROP_THREAD_NETWORK_DATA_VERSION: wpanPropHandler.THREAD_NETWORK_DATA_VERSION,
    SPINEL_PROP_THREAD_STABLE_NETWORK_DATA: wpanPropHandler.THREAD_STABLE_NETWORK_DATA,
    SPINEL_PROP_THREAD_STABLE_NETWORK_DATA_VERSION:wpanPropHandler.THREAD_STABLE_NETWORK_DATA_VERSION,
    SPINEL_PROP_THREAD_ON_MESH_NETS: wpanPropHandler.THREAD_STABLE_NETWORK_DATA,
    SPINEL_PROP_THREAD_LOCAL_ROUTES: wpanPropHandler.THREAD_LOCAL_ROUTES,
    SPINEL_PROP_THREAD_ASSISTING_PORTS: wpanPropHandler.THREAD_ASSISTING_PORTS,
    SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE: wpanPropHandler.THREAD_ALLOW_LOCAL_NET_DATA_CHANGE,
    SPINEL_PROP_THREAD_MODE: wpanPropHandler.THREAD_MODE,
    SPINEL_PROP_THREAD_CHILD_TIMEOUT: wpanPropHandler.THREAD_CHILD_TIMEOUT,
    SPINEL_PROP_THREAD_RLOC16: wpanPropHandler.THREAD_RLOC16,
    SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD: wpanPropHandler.THREAD_ROUTER_UPGRADE_THRESHOLD,
    SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY: wpanPropHandler.THREAD_CONTEXT_REUSE_DELAY,
    SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT: wpanPropHandler.THREAD_NETWORK_ID_TIMEOUT,
    SPINEL_PROP_THREAD_ACTIVE_ROUTER_IDS: wpanPropHandler.THREAD_ACTIVE_ROUTER_IDS,
    SPINEL_PROP_THREAD_RLOC16_DEBUG_PASSTHRU: wpanPropHandler.THREAD_RLOC16_DEBUG_PASSTHRU,

    SPINEL_PROP_IPV6_LL_ADDR:          wpanPropHandler.IPV6_LL_ADDR,
    SPINEL_PROP_IPV6_ML_ADDR:          wpanPropHandler.IPV6_ML_ADDR,
    SPINEL_PROP_IPV6_ML_PREFIX:        wpanPropHandler.IPV6_ML_PREFIX,
    SPINEL_PROP_IPV6_ADDRESS_TABLE:    wpanPropHandler.IPV6_ADDRESS_TABLE,
    SPINEL_PROP_IPV6_ROUTE_TABLE:      wpanPropHandler.IPV6_ROUTE_TABLE,
    SPINEL_PROP_IPv6_ICMP_PING_OFFLOAD: wpanPropHandler.IPv6_ICMP_PING_OFFLOAD,

    SPINEL_PROP_STREAM_DEBUG:          wpanPropHandler.STREAM_DEBUG,
    SPINEL_PROP_STREAM_RAW:            wpanPropHandler.STREAM_RAW,
    SPINEL_PROP_STREAM_NET:            wpanPropHandler.STREAM_NET,
    SPINEL_PROP_STREAM_NET_INSECURE:   wpanPropHandler.STREAM_NET_INSECURE,
}
       

class TunInterface():
    def __init__(self, id):
        self.id = id
        self.ifname = "tun"+str(self.id)

        platform = sys.platform
        if platform == "linux" or platform == "linux2":
            self.__init_linux()
        elif platform == "darwin":
            self.__init_osx()

        self.ifconfig("up")
        #self.ifconfig("inet6 add fd00::1/64")
        self.__start_reader()
        
    def __init_osx(self):
        logger.info("TUN: Starting osx "+self.ifname)
        filename = "/dev/"+self.ifname
        self.tun = os.open(filename, os.O_RDWR)
        self.fd = self.tun
        # trick osx to auto-assign a link local address
        self.addr_add("fe80::1")
        self.addr_del("fe80::1")

    def __init_linux(self):
        logger.info("TUN: Starting linux "+self.ifname)
        self.tun = open("/dev/net/tun", "r+b")
        self.fd = self.tun.fileno()

        IFF_TUNSETIFF = 0x400454ca
        IFF_TUNSETOWNER = IFF_TUNSETIFF + 2
        IFF_TUN = 0x0001
        IFF_TAP = 0x0002
        IFF_NO_PI = 0x1000

        ifr = pack("16sH", self.ifname, IFF_TUN | IFF_NO_PI)
        fcntl.ioctl(self.tun, IFF_TUNSETIFF, ifr)      # Name interface tun#
        fcntl.ioctl(self.tun, IFF_TUNSETOWNER, 1000)   # Allow non-sudo access

    def close(self):
        if self.tun: 
            os.close(self.fd)
            self.fd = None
            self.tun = None

    def command(self, cmd):
        subprocess.check_call(cmd, shell=True)

    def ifconfig(self, args):
        """ Bring interface up and/or assign addresses. """
        self.command('ifconfig '+self.ifname+' '+args)

    def ping6(self, args):
        """ Ping an address. """
        cmd = 'ping6 '+args
        print cmd
        self.command(cmd)

    def addr_add(self, addr):
        self.ifconfig('inet6 add '+addr)

    def addr_del(self, addr):
        platform = sys.platform
        if platform == "linux" or platform == "linux2":
            self.ifconfig('inet6 del '+addr)
        elif platform == "darwin":
            self.ifconfig('inet6 delete '+addr)

    def write(self, packet):
        global gWpanApi
        gWpanApi.ip_send(packet)
        #os.write(self.fd, packet)    # Loop back
        if DEBUG_LOG_TUN:
            logger.debug("\nTUN: TX ("+str(len(packet))+") "+hexify_str(packet))

    def __run_read_thread(self):
        while self.fd:
            try:
                r = select([self.fd],[],[])[0][0]
                if r == self.fd:
                    packet = os.read(self.fd, 4000)
                    if DEBUG_LOG_TUN:
                        logger.debug("\nTUN: RX ("+str(len(packet))+") "+
                                  hexify_str(packet))
                    self.write(packet)
            except:
                print traceback.format_exc()            
                break

        logger.info("TUN: exiting")
        if self.fd:
            os.close(self.fd)
            self.fd = None

    def __start_reader(self):
        """Start reader thread"""
        self._reader_alive = True
        # start serial->console thread
        self.receiver_thread = threading.Thread(target=self.__run_read_thread)
        self.receiver_thread.setDaemon(True)
        self.receiver_thread.start()

 
class WpanApi(SpinelCodec):
    """ Helper class to format wpan command packets """

    def __init__(self, stream, useHdlc=FEATURE_USE_HDLC):

        self.tun_if = None
        self.serial = stream

        self.useHdlc = useHdlc
        if self.useHdlc:
            self.hdlc = Hdlc(self.serial)

        # PARSER state
        self.rx_pkt = []

        # Fire up threads
        self.__queue_prop = Queue.Queue()
        self.__start_reader()
        self.tid_filter = SPINEL_HEADER_DEFAULT 
        self.ip_type_filter = IP_TYPE_ICMPv6

    def __del__(self):
        self._reader_alive = False

    def __start_reader(self):
        """Start reader thread"""
        self._reader_alive = True
        # start serial->console thread
        self.receiver_thread = threading.Thread(target=self.serial_rx)
        self.receiver_thread.setDaemon(True)
        self.receiver_thread.start()

    def transact(self, cmd_id, payload = ""):
        pkt = self.encode(cmd_id, payload)
        if DEBUG_LOG_PKT:
            msg = "TX Pay: (%i) %s " % (len(pkt), hexify_bytes(pkt))
            logger.debug(msg)

        if self.useHdlc: pkt = self.hdlc.encode(pkt)
        self.serial_tx(pkt)

    def parse_rx(self, pkt):
        if DEBUG_LOG_PKT:
            msg = "RX Pay: (%i) %s " % (len(pkt), str(map(hexify_int,pkt)))
            logger.debug(msg)

        if not pkt: return

        length = len(pkt) - 2
        if (length < 0): return

        spkt = "".join(map(chr, pkt))

        tid = self.parse_C(spkt[:1])
        (cmd_op, cmd_len) = self.parse_i(spkt[1:])
        pay_start = cmd_len + 1
        payload = spkt[pay_start:]

        try:
            handler = SPINEL_COMMAND_DISPATCH[cmd_op]
            cmd_name = handler.__name__
            handler(payload, tid)
            
        except:
            print traceback.format_exc()            
            cmd_name = "CB_Unknown"
            logger.info ("\n%s (%i): " % (cmd_name, cmd_op))

        if DEBUG_CMD_RESPONSE:
            logger.info ("\n%s (%i): " % (cmd_name, cmd_op))
            logger.info ("===> %s" % hexify_str(payload))
        
         
    def serial_tx(self, pkt):
        # Encapsulate lagging and Framer support in self.serial class.
        self.serial.write(pkt)


    def serial_rx(self):
        # Recieve thread and parser

        while self._reader_alive:
            if self.useHdlc:
                self.rx_pkt = self.hdlc.collect()

            self.parse_rx(self.rx_pkt)

            # Output RX status window
            if DEBUG_TERM:
                msg = str(map(hexify_int,self.rx_pkt))
                term.print_title(["RX: "+msg])

    class PropertyItem(object):
        """ Queue item for NCP response to property commands. """
        def __init__(self, prop, value, tid):
            self.prop = prop
            self.value = value
            self.tid = tid

    def queue_wait_prepare(self, prop_id, tid=SPINEL_HEADER_DEFAULT):
        self.tid_filter = tid
        self.prop_filter = prop_id
        #print "Q tid_filter = "+str(self.tid_filter)
        self.queue_clear()

    def queue_add(self, prop, value, tid):
        #print "Q add: tid="+str(tid)+" prop="+str(prop)+" tid_filter="+str(self.tid_filter)
        if (tid != self.tid_filter) or (prop != self.prop_filter): return
        if (self.prop_filter == SPINEL_PROP_STREAM_NET):
            pkt = IPv6(value[2:])
            if pkt.nh != self.ip_type_filter: return
        item = self.PropertyItem(prop, value, tid)
        self.__queue_prop.put_nowait(item)
        
    def queue_clear(self):
        with self.__queue_prop.mutex:
            self.__queue_prop.queue.clear()

    def queue_wait_for_prop(self, prop, timeout=TIMEOUT_PROP):
        try:
            item = self.__queue_prop.get(True, timeout)
        except:
            return None

        while (item):
            #print "Q rx: tid="+str(item.tid)+" prop="+str(item.prop)
            if (item.tid == self.tid_filter) and (item.prop == prop):
                return item
            if (self.__queue_prop.empty()):
                #logger.debug("Q rx: wrong response")
                return None
            else:
                item = self.__queue_prop.get_nowait()
        #logger.debug("Q rx: null item")
        return None


    def if_up(self, nodeid='1'):
        if os.geteuid() == 0:
            self.tun_if = TunInterface(nodeid)
        else:
            print "Warning: superuser required to start tun interface."

    def if_down(self):
        if self.tun_if: self.tun_if.close()
        self.tun_if = None

    def ip_send(self, pkt):
        pay = self.encode_i(SPINEL_PROP_STREAM_NET)

        pkt_len = len(pkt)
        pay += pack("<H",pkt_len)          # Start with length of IPv6 packet

        pkt_len += 2                       # Increment to include length word
        pay += pack("%ds" % pkt_len, pkt)  # Append packet after length

        self.transact(SPINEL_CMD_PROP_VALUE_SET, pay)

    def __prop_change_value(self, cmd, prop_id, value, format='B'):
        """ Utility routine to change a property value over Spinel. """
        self.queue_wait_prepare(prop_id)

        pay = self.encode_i(prop_id)
        if format != None:
            pay += pack(format, value)
        self.transact(cmd, pay)

        result = self.queue_wait_for_prop(prop_id)
        if result:
            return result.value
        else:
            return None
    
    def prop_get_value(self, prop_id):
        """ Blocking routine to get a property value over Spinel. """
        self.queue_wait_prepare(prop_id)

        pay = self.encode_i(prop_id)
        self.transact(SPINEL_CMD_PROP_VALUE_GET, pay)

        result = self.queue_wait_for_prop(prop_id)
        if result: 
            return result.value
        else:
            return None

    def prop_set_value(self, prop_id, value, format='B'):
        """ Blocking routine to set a property value over Spinel. """
        return self.__prop_change_value(SPINEL_CMD_PROP_VALUE_SET, prop_id, 
                                        value, format)

    def prop_insert_value(self, prop_id, value, format='B'):
        """ Blocking routine to insert a property value over Spinel. """
        return self.__prop_change_value(SPINEL_CMD_PROP_VALUE_INSERT, prop_id, 
                                        value, format)
        
    def prop_remove_value(self, prop_id, value, format='B'):
        """ Blocking routine to remove a property value over Spinel. """
        return self.__prop_change_value(SPINEL_CMD_PROP_VALUE_REMOVE, prop_id, 
                                        value, format)

#=========================================


class WpanDiagsCmd(Cmd, SpinelCodec):
    
    def __init__(self, device, *a, **kw):
        Cmd.__init__(self, a, kw)
        Cmd.identchars = string.ascii_letters + string.digits + '-'

        if (sys.stdin.isatty()):
            self.prompt = MASTER_PROMPT+" > "
        else:
            self.use_rawinput = 0
            self.prompt = ""
        
        WpanDiagsCmd.command_names.sort()
            
        self.historyFileName = os.path.expanduser("~/.spinel-cli-history")

        try:
            import readline
            #import rlcompleter
            if 'libedit' in readline.__doc__:
                readline.parse_and_bind("bind '\t' rl_complete")
            else:
                readline.parse_and_bind("\t: complete")
            readline.set_completer_delims(' ')
            #readline.set_completer(self.complete)
            try:
                readline.read_history_file(self.historyFileName)
            except IOError:
                pass
        except ImportError:
            pass

        # === Initialize Shell with some important parameters ==
        self.wpanApi = WpanApi(device)
        global gWpanApi
        gWpanApi = self.wpanApi

        self.nodeid = kw.get('nodeid','1')
        self.prop_set_value(SPINEL_PROP_THREAD_RLOC16_DEBUG_PASSTHRU, 1)
        self.prop_set_value(SPINEL_PROP_IPv6_ICMP_PING_OFFLOAD, 1)


 
    command_names = [
        # Shell commands
        'exit',
        'quit',
        'clear',
        'history',
        'debug',
        'debug-term',

        'v',
        'h',
        'q',
        
        # OpenThread CLI commands
        'help', 
        'channel', 
        'child', 
        'childtimeout', 
        'contextreusedelay', 
        'counter', 
        'discover', 
        'eidcache', 
        'extaddr', 
        'extpanid', 
        'ifconfig', 
        'ipaddr', 
        'keysequence', 
        'leaderdata', 
        'leaderweight', 
        'masterkey', 
        'mode', 
        'netdataregister', 
        'networkidtimeout', 
        'networkname', 
        'panid', 
        'ping', 
        'prefix', 
        'releaserouterid', 
        'rloc16', 
        'route', 
        'router', 
        'routerupgradethreshold', 
        'scan', 
        'state', 
        'thread', 
        'tun', 
        'version', 
        'whitelist', 

        # OpenThread Diagnostics Module CLI
        'diag', 
        'diag-start', 
        'diag-channel', 
        'diag-power', 
        'diag-send', 
        'diag-repeat', 
        'diag-sleep', 
        'diag-stats', 
        'diag-stop', 

        # OpenThread Spinel-specific commands
        'ncp-ml64', 
        'ncp-ll64', 
        'ncp-tun', 

    ]


    def log(self,text) :
        logger.info(text)

    def parseline(self, line):
        cmd, arg, line = Cmd.parseline(self, line)
        if (cmd):
            cmd = self.shortCommandName(cmd)
            line = cmd + ' ' + arg
        return cmd, arg, line
        
    def completenames(self, text, *ignored):
        return [ name + ' ' for name in WpanDiagsCmd.command_names \
                 if name.startswith(text) \
                 or self.shortCommandName(name).startswith(text) ]

    def shortCommandName(self, cmd):
        return cmd.replace('-', '')

    def precmd(self, line):
        if (not self.use_rawinput and line != 'EOF' and line != ''):
            #logger.info('>>> ' + line)
            self.log('>>> ' + line)
        return line
        
    def postcmd(self, stop, line):
        if (not stop and self.use_rawinput):
            self.prompt = MASTER_PROMPT+" > "
        return stop
    
    def postloop(self):
        try:
            import readline
            try:
                readline.write_history_file(self.historyFileName)
            except IOError:
                pass
        except ImportError:
            pass

    def prop_get_value(self, prop_id): 
        return self.wpanApi.prop_get_value(prop_id)

    def prop_set_value(self, prop_id, value, format='B'):
        return self.wpanApi.prop_set_value(prop_id, value, format)

    def prop_insert_value(self, prop_id, value, format='B'):
        return self.wpanApi.prop_insert_value(prop_id, value, format)
        
    def prop_remove_value(self, prop_id, value, format='B'):
        return self.wpanApi.prop_remove_value(prop_id, value, format)

    def prop_get_or_set_value(self, prop_id, line, format='B'):
        """ Helper to get or set a property value based on line arguments. """
        if line:
            value = self.prop_set_value(prop_id, self.prep_line(line), format)
        else:    
            value = self.prop_get_value(prop_id)
        return value

    def prep_line(self, line):
        """ Convert a line argument to proper type """
        if line != None: 
            line = int(line)
        return line

    def prop_get(self, prop_id, format='B'):
        """ Helper to get a propery and output the value with Done or Error. """
        value = self.prop_get_value(prop_id)
        if value == None:
            print("Error")
            return None
            
        if (format == 'D') or (format == 'E'):
            print hexify_str(value,'')
        else:
            print str(value)
        print("Done")

        return value

    def prop_set(self, prop_id, line, format='B'):        
        """ Helper to set a propery and output Done or Error. """
        arg = self.prep_line(line)
        value = self.prop_set_value(prop_id, arg, format)

        if (value == None):
            print("Error")
        else:
            print("Done")

        return value

    def handle_property(self, line, prop_id, format='B', output=True):
        value = self.prop_get_or_set_value(prop_id, line, format)
        if not output: return value

        if value == None or value == "":
            print("Error")
            return None

        if line == None or line == "":
            # Only print value on PROP_VALUE_GET
            if (format == '6'):
                print str(ipaddress.IPv6Address(value))
            elif (format == 'D') or (format == 'E'):
                print hexify_str(value,'')
            else:
                print str(value)

        print("Done")
        return value

    def do_help(self, line):
        if (line):
            cmd, arg, unused = self.parseline(line)
            try:
                doc = getattr(self, 'do_' + cmd).__doc__
            except AttributeError:
                doc = None
            if doc:
                self.log("%s\n" % textwrap.dedent(doc))
            else:
                self.log("No help on %s\n" % (line))
        else:
            self.print_topics("\nAvailable commands (type help <name> for more information):", WpanDiagsCmd.command_names, 15, 80)
            
        
    def do_v(self, line):
        """
        version
            Shows detailed version information on spinel-cli tool:
        """
        logger.info(MASTER_PROMPT+" ver. "+__version__)
        logger.info(__copyright__)


    def do_clear(self, line):
        """ Clean up the display. """
        os.system('reset')


    def do_history(self, line):
        """
        history
          
          Show previously executed commands.
        """

        try:
            import readline
            h = readline.get_current_history_length()
            for n in range(1,h+1):
                logger.info(readline.get_history_item(n))
        except ImportError:
            pass


    def do_h(self, line):
        self.do_history(line)


    def do_exit(self, line):
        logger.info("exit")
        return True

    
    def do_quit(self, line):
        logger.info("quit")
        return True


    def do_q(self, line):
        return True


    def do_EOF(self, line):
        self.log("\n")
        goodbye()
        return True


    def emptyline(self):
        pass

    def default(self, line):
        if line[0] == "#":
            logger.debug(line)
        else:
            logger.info(line + ": command not found")
            #exec(line)


    def do_debug(self, line):
        """
        Enables detail logging of bytes over the wire to the radio modem.
        Usage: debug <1=enable | 0=disable>
        """
        global DEBUG_ENABLE, DEBUG_LOG_PKT, DEBUG_LOG_PROP
        global DEBUG_LOG_TX, DEBUG_LOG_RX

        if line: line = int(line)
        if line:
            DEBUG_ENABLE = 1
        else:
            DEBUG_ENABLE = 0
        #DEBUG_LOG_TX = DEBUG_ENABLE
        DEBUG_LOG_PKT = DEBUG_ENABLE
        DEBUG_LOG_PROP = DEBUG_ENABLE
        print "DEBUG_ENABLE = "+str(DEBUG_ENABLE)

    def do_debugterm(self, line):
        """
        Enables a debug terminal display in the title bar for viewing 
        raw NCP packets.
        Usage: debug_term <1=enable | 0=disable>
        """
        global DEBUG_TERM
        if line: line = int(line)
        if line: 
            DEBUG_TERM = 1
        else:
            DEBUG_TERM = 0


    def do_channel(self, line):
        """
        \033[1mchannel\033[0m

            Get the IEEE 802.15.4 Channel value.
        \033[2m
            > channel
            11
            Done
        \033[0m
        \033[1mchannel <channel>\033[0m

            Set the IEEE 802.15.4 Channel value.        
        \033[2m
            > channel 11
            Done
        \033[0m
        """
        self.handle_property(line, SPINEL_PROP_PHY_CHAN)

    def do_child(self, line): 
        """\033[1m
        child list
        \033[0m
            List attached Child IDs
        \033[2m
            > child list
            1 2 3 6 7 8
            Done
        \033[0m\033[1m
        child <id>
        \033[0m
            Print diagnostic information for an attached Thread Child. 
            The id may be a Child ID or an RLOC16.
        \033[2m
            > child 1
            Child ID: 1
            Rloc: 9c01
            Ext Addr: e2b3540590b0fd87
            Mode: rsn
            Net Data: 184
            Timeout: 100
            Age: 0
            LQI: 3
            RSSI: -20
            Done        
        \033[0m
        """
        pass

    def do_childtimeout(self, line): 
        """\033[1m
        childtimeout
        \033[0m
            Get the Thread Child Timeout value.
        \033[2m
            > childtimeout
            300
            Done
        \033[0m\033[1m
        childtimeout <timeout>
        \033[0m
            Set the Thread Child Timeout value.
        \033[2m
            > childtimeout 300
            Done
        \033[0m
        """
        self.handle_property(line, SPINEL_PROP_THREAD_CHILD_TIMEOUT, 'L')

    def do_contextreusedelay(self, line):
        """
        contextreusedelay

            Get the CONTEXT_ID_REUSE_DELAY value.

            > contextreusedelay
            11
            Done

        contextreusedelay <delay>

            Set the CONTEXT_ID_REUSE_DELAY value.

            > contextreusedelay 11
            Done
        """
        self.handle_property(line, SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY, 'L')

    def do_counter(self, line): 
        """
        counter

            Get the supported counter names.

            >counter
            mac
            Done

        counter <countername>
        
            Get the counter value.

            >counter mac
            TxTotal: 10
            TxAckRequested: 4
            TxAcked: 4
            TxNoAckRequested: 6
            TxData: 10
            TxDataPoll: 0
            TxBeacon: 0
            TxBeaconRequest: 0
            TxOther: 0
            TxRetry: 0
            TxErrCca: 0
            RxTotal: 11
            RxData: 11
            RxDataPoll: 0
            RxBeacon: 0
            RxBeaconRequest: 0
            RxOther: 0
            RxWhitelistFiltered: 0
            RxDestAddrFiltered: 0
            RxErrNoFrame: 0
            RxErrNoUnknownNeighbor: 0
            RxErrInvalidSrcAddr: 0
            RxErrSec: 0
            RxErrFcs: 0
            RxErrOther: 0
        """
        pass 

    def do_discover(self, line):
        """
        discover [channel]

             Perform an MLE Discovery operation.

        channel: The channel to discover on. If no channel is provided, the discovery will cover all valid channels.
        > discover
        | J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |
        +---+------------------+------------------+------+------------------+----+-----+-----+
        | 0 | OpenThread       | dead00beef00cafe | ffff | f1d92a82c8d8fe43 | 11 | -20 |   0 |
        Done
        """
        pass 

    def do_eidcache(self, line): 
        """
        eidcache

            Print the EID-to-RLOC cache entries.

            > eidcache
            fdde:ad00:beef:0:bb1:ebd6:ad10:f33 ac00
            fdde:ad00:beef:0:110a:e041:8399:17cd 6000
            Done
        """
        pass 

    def do_extaddr(self, line):
        """
        extaddr

            Get the IEEE 802.15.4 Extended Address.

            > extaddr
            dead00beef00cafe
            Done

        extaddr <extaddr>

            Set the IEEE 802.15.4 Extended Address.

            > extaddr dead00beef00cafe
            dead00beef00cafe
            Done
        """
        self.handle_property(line, SPINEL_PROP_MAC_15_4_LADDR, 'E')

    def do_extpanid(self, line):
        """
        extpanid

            Get the Thread Extended PAN ID value.

            > extpanid
            dead00beef00cafe
            Done

        extpanid <extpanid>
        
            Set the Thread Extended PAN ID value.
        
            > extpanid dead00beef00cafe
            Done        
        """
        self.handle_property(line, SPINEL_PROP_NET_XPANID, 'D')

    def do_ifconfig(self, line): 
        """
        ifconfig up
        
            Bring up the IPv6 interface.
        
            > ifconfig up
            Done

        ifconfig down
        
            Bring down the IPv6 interface.
        
            > ifconfig down
            Done

        ifconfig
        
            Show the status of the IPv6 interface.
        
            > ifconfig
            down
            Done
        """

        args = line.split(" ")

        if args[0] == "":
            value = self.prop_get_value(SPINEL_PROP_NET_IF_UP)
            if value != None:
                ARG_MAP_VALUE = {
                    0: "down",
                    1: "up",
                }
                print ARG_MAP_VALUE[value]

        elif args[0] == "up":
            self.prop_set(SPINEL_PROP_NET_IF_UP, '1')
            return

        elif args[0] == "down":
            self.prop_set(SPINEL_PROP_NET_IF_UP, '0')
            return

        print("Done")

    def do_ipaddr(self, line): 
        """
        ipaddr

            List all IPv6 addresses assigned to the Thread interface.

            > ipaddr
            fdde:ad00:beef:0:0:ff:fe00:0
            fe80:0:0:0:0:ff:fe00:0
            fdde:ad00:beef:0:558:f56b:d688:799
            fe80:0:0:0:f3d9:2a82:c8d8:fe43
            Done

        ipaddr add <ipaddr>
        
            Add an IPv6 address to the Thread interface.

            > ipaddr add 2001::dead:beef:cafe
            Done

        ipaddr del <ipaddr>
        
            Delete an IPv6 address from the Thread interface.
        
            > ipaddr del 2001::dead:beef:cafe
            Done
        """
        args = line.split(" ")
        valid = 0
        preferred = 0
        flags = 0

        num = len(args)
        if (num > 1):
            ipaddr = args[1]
            prefix = ipaddress.IPv6Interface(unicode(ipaddr))
            arr = prefix.ip.packed

        if args[0] == "":
            v = self.prop_get_value(SPINEL_PROP_IPV6_ADDRESS_TABLE)
            # TODO: parse the list of addresses and display.
            sz = 0x1B
            addrs = [v[i:i+sz] for i in xrange(0, len(v), sz)]
            for addr in addrs:
                addr = addr[2:18]
                print str(ipaddress.IPv6Address(addr))

        elif args[0] == "add":
            arr += pack('B', prefix.network.prefixlen)
            arr += pack('<L', valid)
            arr += pack('<L', preferred)
            arr += pack('B', flags)
            value = self.prop_insert_value(SPINEL_PROP_IPV6_ADDRESS_TABLE, 
                                           arr, str(len(arr))+'s')
            if self.wpanApi.tun_if:
                self.wpanApi.tun_if.addr_add(ipaddr)

        elif args[0] == "remove":
            value = self.prop_remove_value(SPINEL_PROP_IPV6_ADDRESS_TABLE, 
                                           arr, str(len(arr))+'s')
            if self.wpanApi.tun_if:
                self.wpanApi.tun_if.addr_del(ipaddr)

        print("Done")

    def do_keysequence(self, line): 
        """
        keysequence

            Get the Thread Key Sequence.

            > keysequence
            10
            Done

        keysequence <keysequence>

            Set the Thread Key Sequence.

            > keysequence 10
            Done
        """
        self.handle_property(line, SPINEL_PROP_NET_KEY_SEQUENCE, 'L')

    def do_leaderdata(self, line): 
        pass 

    def do_leaderweight(self, line):
        """
        leaderweight

            Get the Thread Leader Weight.

            > leaderweight
            128
            Done

        leaderweight <weight>
        
            Set the Thread Leader Weight.
        
            > leaderweight 128
            Done
        """
        self.handle_property(line, SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT)

    def do_masterkey(self, line): 
        """
        masterkey
        
            Get the Thread Master Key value.
        
            > masterkey
            00112233445566778899aabbccddeeff
            Done

        masterkey <key>
        
            Set the Thread Master Key value.
        
            > masterkey 00112233445566778899aabbccddeeff
            Done
        """
        pass 

    def do_mode(self, line): 
        """
        mode
        
            Get the Thread Device Mode value.
        
              r: rx-on-when-idle
              s: Secure IEEE 802.15.4 data requests
              d: Full Function Device
              n: Full Network Data

            > mode
            rsdn
            Done

        mode [rsdn]

            Set the Thread Device Mode value.

              r: rx-on-when-idle
              s: Secure IEEE 802.15.4 data requests
              d: Full Function Device
              n: Full Network Data

            > mode rsdn
            Done
        """
        THREAD_MODE_MAP_VALUE = {
            0x00: "0",
            0x01: "n",
            0x02: "d",
            0x03: "dn",
            0x04: "s",
            0x05: "sn",
            0x06: "sd",
            0x07: "sdn",
            0x08: "r",
            0x09: "rn",
            0x0A: "rd",
            0x0B: "rdn",
            0x0C: "rs",
            0x0D: "rsn",
            0x0E: "rsd",
            0x0F: "rsdn",
        }

        THREAD_MODE_MAP_NAME = {
            "0": 0x00,
            "n": 0x01,
            "d": 0x02,
            "dn": 0x03,
            "s": 0x04,
            "sn": 0x05,
            "sd": 0x06,
            "sdn": 0x07,
            "r": 0x08,
            "rn": 0x09,
            "rd": 0x0A,
            "rdn": 0x0B,
            "rs": 0x0C,
            "rsn": 0x0D,
            "rsd": 0x0E,
            "rsdn": 0x0F
        }

        if line:
            try:
                # remap string state names to integer
                line = THREAD_MODE_MAP_NAME[line]
            except:
                print("Error")
                return

        result = self.prop_get_or_set_value(SPINEL_PROP_THREAD_MODE, line)
        if result != None:
            if not line:
                print THREAD_MODE_MAP_VALUE[result]
            print("Done")
        else:
            print("Error")


    def do_netdataregister(self, line): 
        """
        netdataregister

            Register local network data with Thread Leader.

            > netdataregister
            Done
        """
        self.prop_set_value(SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE, 1)
        self.handle_property("0", SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE)
        pass 

    def do_networkidtimeout(self, line): 
        """
        networkidtimeout

            Get the NETWORK_ID_TIMEOUT parameter used in the Router role.

            > networkidtimeout
            120
            Done

        networkidtimeout <timeout>
        
            Set the NETWORK_ID_TIMEOUT parameter used in the Router role.
        
            > networkidtimeout 120
            Done
        """
        self.handle_property(line, SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT)

    def do_networkname(self, line): 
        """
        networkname

            Get the Thread Network Name.

            > networkname
            OpenThread
            Done

        networkname <name>
        
            Set the Thread Network Name.
        
            > networkname OpenThread
            Done
        """
        pass 

    def do_panid(self, line):
        """
        panid

            Get the IEEE 802.15.4 PAN ID value.

            > panid
            0xdead
            Done
        
        panid <panid>
        
            Set the IEEE 802.15.4 PAN ID value.
        
            > panid 0xdead
            Done
        """
        self.handle_property(line, SPINEL_PROP_MAC_15_4_PANID, 'H')

    def do_ping(self, line): 
        """
        ping <ipaddr> [size] [count] [interval]

            Send an ICMPv6 Echo Request.

            > ping fdde:ad00:beef:0:558:f56b:d688:799
            16 bytes from fdde:ad00:beef:0:558:f56b:d688:799: icmp_seq=1 hlim=64 time=28ms
        """
        args = line.split(" ")
        addr = "::1"
        size = "56"
        count = "1"
        interval = "1"
        if (len(args) > 0): addr = args[0]
        if (len(args) > 1): size = args[1]
        if (len(args) > 2): count = args[2]
        if (len(args) > 3): interval = args[3]
        
        try:
            # Generate local ping packet and send directly via spinel.
            ML64 = self.prop_get_value(SPINEL_PROP_IPV6_ML_ADDR)
            ML64 = str(ipaddress.IPv6Address(ML64))
            ping_req = str(IPv6(src=ML64, dst=addr)/ICMPv6EchoRequest())
            self.wpanApi.queue_wait_prepare(SPINEL_PROP_STREAM_NET,
                                            SPINEL_HEADER_ASYNC)
            self.wpanApi.ip_send(ping_req)
            result = self.wpanApi.queue_wait_for_prop(SPINEL_PROP_STREAM_NET,
                                                      TIMEOUT_PING)
            if result:
                # TODO: parse result and confirm it is ICMPv6EchoResponse...
                pkt = IPv6(result.value[2:])
                print "%d bytes from %s: icmp_seq=%d hlim=%d time=%dms" % (
                    pkt.plen, pkt.src, pkt.seq, pkt.hlim, 80)
                print("Done") 
            else:
                # Don't output anything when ping fails
                #print "Fail"
                pass
        except:
            print "Fail"
            print traceback.format_exc()

    def do_prefix(self, line): 
        """
        prefix add <prefix> [pvdcsr] [prf]
        
            Add a valid prefix to the Network Data.
        
              p: Preferred flag
              a: Stateless IPv6 Address Autoconfiguration flag
              d: DHCPv6 IPv6 Address Configuration flag
              c: DHCPv6 Other Configuration flag
              r: Default Route flag
              o: On Mesh flag
              s: Stable flag
              prf: Default router preference, which may be 'high', 'med', or 'low'.
            > prefix add 2001:dead:beef:cafe::/64 paros med
            Done

        prefix remove <prefix>
        
            Invalidate a prefix in the Network Data.

            > prefix remove 2001:dead:beef:cafe::/64
            Done
        """
        args = line.split(" ")
        stable = 0
        flags = 0

        num = len(args)
        if num > 1:
            prefix = ipaddress.IPv6Interface(unicode(args[1]))
            arr = prefix.ip.packed
        if num > 2:
            for c in args[2]:
                if c == 'p': flags |= kThreadPrefixPreferredFlag
                if c == 'a': flags |= kThreadPrefixSlaacFlag
                if c == 'd': flags |= kThreadPrefixDhcpFlag
                if c == 'c': flags |= kThreadPrefixConfigureFlag
                if c == 'r': flags |= kThreadPrefixDefaultRouteFlag
                if c == 'o': flags |= kThreadPrefixOnMeshFlag
                if c == 's': stable = 1  # Stable flag
        if num > 3:
            ARG_MAP_NAME = {
                "high": 2,
                "med": 1,
                "low": 0,
            }
            prf = ARG_MAP_NAME[args[3]]
            flags |= (prf << kThreadPrefixPreferenceOffset)

        if args[0] == "":
            value = self.prop_get_value(SPINEL_PROP_THREAD_ON_MESH_NETS)

        elif args[0] == "add":
            arr += pack('B', prefix.network.prefixlen)
            arr += pack('B', stable)
            arr += pack('B', flags)
            value = self.prop_insert_value(SPINEL_PROP_THREAD_ON_MESH_NETS, 
                                           arr, str(len(arr))+'s')

        elif args[0] == "remove":
            value = self.prop_remove_value(SPINEL_PROP_THREAD_ON_MESH_NETS, 
                                           arr, str(len(arr))+'s')

        print("Done")

    def do_releaserouterid(self, line): 
        """
        releaserouterid <routerid>

            Release a Router ID that has been allocated by the device in the Leader role.
        
            > releaserouterid 16
            Done
        """
        if line:
            value = int(line)
            self.prop_remove_value(SPINEL_PROP_THREAD_ACTIVE_ROUTER_IDS, value)
        print("Done")


    def do_rloc16(self, line):
        """
        rloc16

            Get the Thread RLOC16 value.

            > rloc16
            0xdead
            Done
        """
        self.handle_property(line, SPINEL_PROP_THREAD_RLOC16, 'H')

    def do_route(self, line):
        """
        route add <prefix> [s] [prf]

            Add a valid prefix to the Network Data.

              s: Stable flag
              prf: Default Router Preference, which may be: 'high', 'med', or 'low'.

            > route add 2001:dead:beef:cafe::/64 s med
            Done

        route remove <prefix>
        
            Invalidate a prefix in the Network Data.

            > route remove 2001:dead:beef:cafe::/64
            Done
        """
        args = line.split(" ")
        stable = 0
        prf = 0

        num = len(args)
        if (num > 1):
            prefix = ipaddress.IPv6Interface(unicode(args[1]))
            arr = prefix.ip.packed

        if args[0] == "":
            value = self.prop_get_value(SPINEL_PROP_THREAD_LOCAL_ROUTES)

        elif args[0] == "add":
            arr += pack('B', prefix.network.prefixlen)
            arr += pack('B', stable)
            arr += pack('B', prf)
            value = self.prop_insert_value(SPINEL_PROP_THREAD_LOCAL_ROUTES, 
                                           arr, str(len(arr))+'s')

        elif args[0] == "remove":
            value = self.prop_remove_value(SPINEL_PROP_THREAD_LOCAL_ROUTES, 
                                           arr, str(len(arr))+'s')

        print("Done")

    def do_router(self, line): 
        """
        router list

            List allocated Router IDs
        
            > router list
            8 24 50
            Done

        router <id>
        
            Print diagnostic information for a Thread Router. The id may be a Router ID or an RLOC16.

            > router 50
            Alloc: 1
            Router ID: 50
            Rloc: c800
            Next Hop: c800
            Link: 1
            Ext Addr: e2b3540590b0fd87
            Cost: 0
            LQI In: 3
            LQI Out: 3
            Age: 3
            Done

            > router 0xc800
            Alloc: 1
            Router ID: 50
            Rloc: c800
            Next Hop: c800
            Link: 1
            Ext Addr: e2b3540590b0fd87
            Cost: 0
            LQI In: 3
            LQI Out: 3
            Age: 7
            Done
        """
        pass 

    def do_routerupgradethreshold(self, line):
        """
        routerupgradethreshold

            Get the ROUTER_UPGRADE_THRESHOLD value.

            > routerupgradethreshold
            16
            Done

        routerupgradethreshold <threshold>
        
            Set the ROUTER_UPGRADE_THRESHOLD value.
        
            > routerupgradethreshold 16
            Done        
        """
        self.handle_property(line, SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD)

    def do_scan(self, line):
        """
        scan [channel]
        
            Perform an IEEE 802.15.4 Active Scan.
        
              channel: The channel to scan on. If no channel is provided, the active scan will cover all valid channels.

            > scan
            | J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |
            +---+------------------+------------------+------+------------------+----+-----+-----+
            | 0 | OpenThread       | dead00beef00cafe | ffff | f1d92a82c8d8fe43 | 11 | -20 |   0 |
        Done
        """
        # Initial mock-up of scan
        self.handle_property("15", SPINEL_PROP_MAC_SCAN_MASK)
        self.handle_property("4", SPINEL_PROP_MAC_SCAN_PERIOD, 'H')
        self.handle_property("1", SPINEL_PROP_MAC_SCAN_STATE)
        import time
        time.sleep(5) 
        self.handle_property("", SPINEL_PROP_MAC_SCAN_BEACON, 'U')
        

    def do_thread(self, line):
        """
        thread start

            Enable Thread protocol operation and attach to a Thread network.

            > thread start
            Done

        thread stop
        
            Disable Thread protocol operation and detach from a Thread network.
        
            > thread stop
            Done
        """
        ARG_MAP_VALUE = {
            0: "stop",
            1: "start",
            2: "start",
            3: "start",
        }

        ARG_MAP_NAME = {
            "stop": "0",
            "start": "2",
        }

        if line:
            try:
                # remap string state names to integer
                line = ARG_MAP_NAME[line]
            except:
                print("Error")
                return

        result = self.prop_get_or_set_value(SPINEL_PROP_NET_STACK_UP, line)
        if result != None:
            if not line:
                print ARG_MAP_VALUE[result]
            print("Done")
        else:
            print("Error")

    def do_state(self, line): 
        ROLE_MAP_VALUE = {
            0: "detached",
            1: "child",
            2: "router",
            3: "leader",
        }

        ROLE_MAP_NAME = {
            "disabled": "0",
            "detached": "0",
            "child": "1",
            "router": "2",
            "leader": "3",
        }

        if line:
            try:
                # remap string state names to integer
                line = ROLE_MAP_NAME[line]
            except:
                print("Error")
                return

        result = self.prop_get_or_set_value(SPINEL_PROP_NET_ROLE, line)
        if result != None:
            if not line:
                state = ROLE_MAP_VALUE[result] 
                # TODO: if state="disabled": get NET_STATE to determine 
                #       whether "disabled" or "detached"
                print state
            print("Done")
        else:
            print("Error")

    def do_version(self, line):
        """
        version

            Print the build version information.
        
            > version
            OPENTHREAD/gf4f2f04; Jul  1 2016 17:00:09
            Done
        """
        self.handle_property(line, SPINEL_PROP_NCP_VERSION, 'U')

    def do_whitelist(self, line):
        """
        whitelist

            List the whitelist entries.

            > whitelist
            Enabled
            e2b3540590b0fd87
            d38d7f875888fccb
            c467a90a2060fa0e
            Done

        whitelist add <extaddr> [rssi]
        
            Add an IEEE 802.15.4 Extended Address to the whitelist.
        
            > whitelist add dead00beef00cafe
            Done

        whitelist clear
        
            Clear all entries from the whitelist.
        
            > whitelist clear
            Done

        whitelist disable
        
            Disable MAC whitelist filtering.
        
            > whitelist disable
            Done

        whitelist enable
        
            Enable MAC whitelist filtering.
        
            > whitelist enable
            Done

        whitelist remove <extaddr>
        
            Remove an IEEE 802.15.4 Extended Address from the whitelist.
        
            > whitelist remove dead00beef00cafe
            Done
        """
        ARG_MAP_VALUE = {
            0: "Disabled",
            1: "Enabled",
        }

        args = line.split(" ")

        if args[0] == "":
            value = self.prop_get_value(SPINEL_PROP_MAC_WHITELIST_ENABLED)
            if value != None:
                print ARG_MAP_VALUE[value]
            value = self.prop_get_value(SPINEL_PROP_MAC_WHITELIST)

        elif args[0] == "enable":
            self.prop_set(SPINEL_PROP_MAC_WHITELIST_ENABLED, '1')
            return

        elif args[0] == "disable":
            self.prop_set(SPINEL_PROP_MAC_WHITELIST_ENABLED, '0')
            return

        elif args[0] == "clear":
            value = self.prop_set_value(SPINEL_PROP_MAC_WHITELIST, None, None)

        elif args[0] == "add":
            arr = hex_to_bytes(args[1])            
            try:
                rssi = int(args[2])
            except:
                rssi = SPINEL_RSSI_OVERRIDE
            arr += pack('b', rssi)
            value = self.prop_insert_value(SPINEL_PROP_MAC_WHITELIST, arr, 
                                           str(len(arr))+'s')

        elif args[0] == "remove":
            arr = hex_to_bytes(args[1])
            arr += pack('b', SPINEL_RSSI_OVERRIDE)
            value = self.prop_remove_value(SPINEL_PROP_MAC_WHITELIST, arr,
                                           str(len(arr))+'s')
        
        print("Done")

    def do_ncpll64(self, line):
        """ Display the link local IPv6 address. """
        self.handle_property(line, SPINEL_PROP_IPV6_LL_ADDR, '6')

    def do_ncpml64(self, line):
        """ Display the mesh local IPv6 address. """
        self.handle_property(line, SPINEL_PROP_IPV6_ML_ADDR, '6')

    def do_ncptun(self, line):
        """
        ncp-tun

            Control sideband tunnel interface.

        ncp-tun up
        
            Bring up Thread TUN interface.
        
            > ncp-tun up
            Done

        ncp-tun down
        
            Bring down Thread TUN interface.
        
            > ncp-tun down
            Done

        ncp-tun add <ipaddr>
        
            Add an IPv6 address to the Thread TUN interface.

            > ncp-tun add 2001::dead:beef:cafe
            Done

        ncp-tun del <ipaddr>
        
            Delete an IPv6 address from the Thread TUN interface.
        
            > ncp-tun del 2001::dead:beef:cafe
            Done

        ncp-tun ping <ipaddr> [size] [count] [interval]

            Send an ICMPv6 Echo Request.

            > ncp-tun ping fdde:ad00:beef:0:558:f56b:d688:799
            16 bytes from fdde:ad00:beef:0:558:f56b:d688:799: icmp_seq=1 hlim=64 time=28ms
        """
        args = line.split(" ")

        num = len(args)
        if (num > 1):
            ipaddr = args[1]
            prefix = ipaddress.IPv6Interface(unicode(ipaddr))
            arr = prefix.ip.packed

        if args[0] == "":
            pass

        elif args[0] == "add":
            if self.wpanApi.tun_if:
                self.wpanApi.tun_if.addr_add(ipaddr)

        elif args[0] == "remove":
            if self.wpanApi.tun_if:
                self.wpanApi.tun_if.addr_del(ipaddr)

        elif args[0] == "up":
            self.wpanApi.if_up(self.nodeid)

        elif args[0] == "down":
            self.wpanApi.if_down()

        elif args[0] == "ping":
            # Use tunnel to send ping
            size = "56"
            count = "1"
            interval = "1"
            if (len(args) > 1): size = args[1]
            if (len(args) > 2): count = args[2]
            if (len(args) > 3): interval = args[3]

            if self.wpanApi.tun_if:
                self.wpanApi.tun_if.ping6(" -c "+count+" -s "+size+" "+addr)

        print("Done")


    def do_diag(self, line):
        """
        diag

            Show diagnostics mode status.

            > diag
            diagnostics mode is disabled
        """
        pass

    def do_diagstart(self, line):
        """
        diag-start

            Start diagnostics mode.

            > diag-start
            start diagnostics mode
            status 0x00
        """
        pass
        
    def do_diagchannel(self, line):
        """
        diag-channel

            Get the IEEE 802.15.4 Channel value for diagnostics module.
        
            > diag-channel
            channel: 11

        diag-channel <channel>
        
            Set the IEEE 802.15.4 Channel value for diagnostics module.
        
            > diag-channel 11
            set channel to 11
            status 0x00
        """
        pass

    def do_diagpower(self, line):
        """
        diag-power

            Get the tx power value(dBm) for diagnostics module.

            > diag-power
            tx power: -10 dBm

        diag-power <power>
        
            Set the tx power value(dBm) for diagnostics module.
        
            > diag-power -10
            set tx power to -10 dBm
            status 0x00
        """
        pass

    def do_diagsend(self, line):
        """
        diag-send <packets> <length>

            Transmit a fixed number of packets with fixed length.
        
            > diag-send 20 100
            sending 0x14 packet(s), length 0x64
            status 0x00
        """
        pass

    def do_diagrepeat(self, line):
        """
        diag-repeat <delay> <length>

            Transmit packets repeatedly with a fixed interval.

            > diag repeat 100 100
            sending packets of length 0x64 at the delay of 0x64 ms
            status 0x00

        diag-repeat stop

            Stop repeated packet transmission.
        
            > diag-repeat stop
            repeated packet transmission is stopped
            status 0x00
        """
        pass

    def do_diagsleep(self, line):
        """
        diag-sleep

            Enter radio sleep mode.
        
            > diag-sleep
            sleeping now...
        """
        pass

    def do_diagstats(self, line):
        """
        diag-stats

            Print statistics during diagnostics mode.
        
            > diag-stats
            received packets: 10
            sent packets: 10
            first received packet: rssi=-65, lqi=101
        """
        pass

    def do_diagstop(self, line):
        """
        diag stop

            Stop diagnostics mode and print statistics.
        
            > diag-stop
            received packets: 10
            sent packets: 10
            first received packet: rssi=-65, lqi=101
        
            stop diagnostics mode
            status 0x00
        """
        pass


if __name__ == "__main__":

    args = sys.argv[1:] 

    optParser = OptionParser()

    optParser = OptionParser(usage=optparse.SUPPRESS_USAGE)
    optParser.add_option("-u", "--uart", action="store", 
                         dest="uart", type="string")
    optParser.add_option("-p", "--pipe", action="store", 
                         dest="pipe", type="string")
    optParser.add_option("-s", "--socket", action="store", 
                         dest="socket", type="string")
    optParser.add_option("-n", "--nodeid", action="store", 
                         dest="nodeid", type="string", default="1")
    optParser.add_option("-q", "--quiet", action="store_true", dest="quiet")
    optParser.add_option("-v", "--verbose", action="store_false",dest="verbose")

    (options, remainingArgs) = optParser.parse_args(args)
        

    # Set default stream to pipe
    streamType = 'p'
    streamDescriptor = "../../examples/apps/ncp/ot-ncp "+options.nodeid

    if options.uart: 
        streamType = 'u'
        streamDescriptor = options.uart
    elif options.socket:
        streamType = 's'
        streamDescriptor = options.socket
    elif options.pipe:
        streamType = 'p'
        streamDescriptor = options.pipe
        if options.nodeid: streamDescriptor += " "+str(options.nodeid)
    else:
        if len(remainingArgs) > 0: 
            streamDescriptor = " ".join(remainingArgs)

    stream = StreamOpen(streamType, streamDescriptor)
    shell = WpanDiagsCmd(stream, nodeid=options.nodeid)

    # register clean exit handlers.
    signal.signal(signal.SIGHUP, goodbye)
    signal.signal(signal.SIGINT, goodbye)
    signal.signal(signal.SIGABRT, goodbye)
    signal.signal(signal.SIGTERM, goodbye)
    signal.signal(signal.SIGPIPE, goodbye)

    #print os.system('ps aux | grep ot-ncp')


    try:
        shell.cmdloop()
    except KeyboardInterrupt:
        logger.info('\nCTRL+C Pressed')

    goodbye()
