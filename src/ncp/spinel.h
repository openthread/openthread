/*
 *    Copyright (c) 2016, Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SPINEL_HEADER_INCLUDED
#define SPINEL_HEADER_INCLUDED 1

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

// ----------------------------------------------------------------------------

#ifndef DOXYGEN_SHOULD_SKIP_THIS

# if defined(__GNUC__)
#  define SPINEL_API_EXTERN              extern __attribute__ ((visibility ("default")))
#  define SPINEL_API_NONNULL_ALL         __attribute__((nonnull))
#  define SPINEL_API_WARN_UNUSED_RESULT  __attribute__((warn_unused_result))
# endif // ifdef __GNUC__

# if !defined(__BEGIN_DECLS) || !defined(__END_DECLS)
#  if defined(__cplusplus)
#   define __BEGIN_DECLS   extern "C" {
#   define __END_DECLS     }
#  else // if defined(__cplusplus)
#   define __BEGIN_DECLS
#   define __END_DECLS
#  endif // else defined(__cplusplus)
# endif // if !defined(__BEGIN_DECLS) || !defined(__END_DECLS)

#endif // ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifndef SPINEL_API_EXTERN
# define SPINEL_API_EXTERN               extern
#endif

#ifndef SPINEL_API_NONNULL_ALL
# define SPINEL_API_NONNULL_ALL
#endif

#ifndef SPINEL_API_WARN_UNUSED_RESULT
# define SPINEL_API_WARN_UNUSED_RESULT
#endif

// ----------------------------------------------------------------------------

#define SPINEL_PROTOCOL_VERSION_THREAD_MAJOR    4
#define SPINEL_PROTOCOL_VERSION_THREAD_MINOR    1

#define SPINEL_FRAME_MAX_SIZE                   1300

// ----------------------------------------------------------------------------

__BEGIN_DECLS

typedef enum
{
    SPINEL_STATUS_OK                = 0, ///< Operation has completed successfully.
    SPINEL_STATUS_FAILURE           = 1, ///< Operation has failed for some undefined reason.

    SPINEL_STATUS_UNIMPLEMENTED     = 2, ///< Given operation has not been implemented.
    SPINEL_STATUS_INVALID_ARGUMENT  = 3, ///< An argument to the operation is invalid.
    SPINEL_STATUS_INVALID_STATE     = 4, ///< This operation is invalid for the current device state.
    SPINEL_STATUS_INVALID_COMMAND   = 5, ///< This command is not recognized.
    SPINEL_STATUS_INVALID_INTERFACE = 6, ///< This interface is not supported.
    SPINEL_STATUS_INTERNAL_ERROR    = 7, ///< An internal runtime error has occured.
    SPINEL_STATUS_SECURITY_ERROR    = 8, ///< A security/authentication error has occured.
    SPINEL_STATUS_PARSE_ERROR       = 9, ///< A error has occured while parsing the command.
    SPINEL_STATUS_IN_PROGRESS       = 10, ///< This operation is in progress.
    SPINEL_STATUS_NOMEM             = 11, ///< Operation prevented due to memory pressure.
    SPINEL_STATUS_BUSY              = 12, ///< The device is currently performing a mutually exclusive operation
    SPINEL_STATUS_PROP_NOT_FOUND    = 13, ///< The given property is not recognized.
    SPINEL_STATUS_DROPPED           = 14, ///< A/The packet was dropped.
    SPINEL_STATUS_EMPTY             = 15, ///< The result of the operation is empty.
    SPINEL_STATUS_CMD_TOO_BIG       = 16, ///< The command was too large to fit in the internal buffer.
    SPINEL_STATUS_NO_ACK            = 17, ///< The packet was not acknowledged.
    SPINEL_STATUS_CCA_FAILURE       = 18, ///< The packet was not sent due to a CCA failure.
    SPINEL_STATUS_ALREADY           = 19, ///< The operation is already in progress.
    SPINEL_STATUS_ITEM_NOT_FOUND    = 20, ///< The given item could not be found.

    SPINEL_STATUS_RESET__BEGIN      = 112,
    SPINEL_STATUS_RESET_POWER_ON    = SPINEL_STATUS_RESET__BEGIN + 0,
    SPINEL_STATUS_RESET_EXTERNAL    = SPINEL_STATUS_RESET__BEGIN + 1,
    SPINEL_STATUS_RESET_SOFTWARE    = SPINEL_STATUS_RESET__BEGIN + 2,
    SPINEL_STATUS_RESET_FAULT       = SPINEL_STATUS_RESET__BEGIN + 3,
    SPINEL_STATUS_RESET_CRASH       = SPINEL_STATUS_RESET__BEGIN + 4,
    SPINEL_STATUS_RESET_ASSERT      = SPINEL_STATUS_RESET__BEGIN + 5,
    SPINEL_STATUS_RESET_OTHER       = SPINEL_STATUS_RESET__BEGIN + 6,
    SPINEL_STATUS_RESET_UNKNOWN     = SPINEL_STATUS_RESET__BEGIN + 7,
    SPINEL_STATUS_RESET_WATCHDOG    = SPINEL_STATUS_RESET__BEGIN + 8,
    SPINEL_STATUS_RESET__END        = 128,

    SPINEL_STATUS_VENDOR__BEGIN     = 15360,
    SPINEL_STATUS_VENDOR__END       = 16384,

    SPINEL_STATUS_STACK_NATIVE__BEGIN = 16384,
    SPINEL_STATUS_STACK_NATIVE__END   = 81920,

    SPINEL_STATUS_EXPERIMENTAL__BEGIN = 2000000,
    SPINEL_STATUS_EXPERIMENTAL__END   = 2097152,
} spinel_status_t;

typedef enum
{
    SPINEL_NET_ROLE_DETACHED   = 0,
    SPINEL_NET_ROLE_CHILD      = 1,
    SPINEL_NET_ROLE_ROUTER     = 2,
    SPINEL_NET_ROLE_LEADER     = 3,
} spinel_net_role_t;

typedef enum
{
    SPINEL_SCAN_STATE_IDLE     = 0,
    SPINEL_SCAN_STATE_BEACON   = 1,
    SPINEL_SCAN_STATE_ENERGY   = 2,
} spinel_scan_state_t;

typedef enum
{
    SPINEL_POWER_STATE_OFFLINE    = 0,
    SPINEL_POWER_STATE_DEEP_SLEEP = 1,
    SPINEL_POWER_STATE_STANDBY    = 2,
    SPINEL_POWER_STATE_LOW_POWER  = 3,
    SPINEL_POWER_STATE_ONLINE     = 4,
} spinel_power_state_t;

enum
{
    SPINEL_PROTOCOL_TYPE_ZIGBEE    = 1,
    SPINEL_PROTOCOL_TYPE_ZIGBEE_IP = 2,
    SPINEL_PROTOCOL_TYPE_THREAD    = 3,
};

enum
{
    SPINEL_MAC_FILTER_MODE_NORMAL      = 0, ///< Normal MAC filtering is in place.
    SPINEL_MAC_FILTER_MODE_PROMISCUOUS = 1, ///< All MAC packets matching network are passed up the stack.
    SPINEL_MAC_FILTER_MODE_MONITOR     = 2, ///< All decoded MAC packets are passed up the stack.

    /// 802.15.4's definition of "Promiscuous" mode.
    /** 802.15.4 defines promiscuous mode to be what
     *  is generally considered to be "Monitor" mode.
     *  This definition will hopefully help people who
     *  are familiar with the 802.15.4 spec from being
     *  confused about what they need to set this
     *  property to in order to get the desired behavior.
     */
    SPINEL_MAC_FILTER_MODE_15_4_PROMISCUOUS = SPINEL_MAC_FILTER_MODE_MONITOR,
};

typedef struct
{
    uint8_t bytes[8];
} spinel_eui64_t;

typedef struct
{
    uint8_t bytes[8];
} spinel_net_xpanid_t;

typedef struct
{
    uint8_t bytes[6];
} spinel_eui48_t;

typedef struct
{
    uint8_t bytes[16];
} spinel_ipv6addr_t;

typedef int spinel_ssize_t;
typedef unsigned int spinel_size_t;
typedef uint8_t spinel_tid_t;
typedef unsigned int spinel_cid_t;

enum
{
    SPINEL_CMD_NOOP                 = 0,
    SPINEL_CMD_RESET                = 1,
    SPINEL_CMD_PROP_VALUE_GET       = 2,
    SPINEL_CMD_PROP_VALUE_SET       = 3,
    SPINEL_CMD_PROP_VALUE_INSERT    = 4,
    SPINEL_CMD_PROP_VALUE_REMOVE    = 5,
    SPINEL_CMD_PROP_VALUE_IS        = 6,
    SPINEL_CMD_PROP_VALUE_INSERTED  = 7,
    SPINEL_CMD_PROP_VALUE_REMOVED   = 8,

    SPINEL_CMD_NET_SAVE             = 9,
    SPINEL_CMD_NET_CLEAR            = 10,
    SPINEL_CMD_NET_RECALL           = 11,

    SPINEL_CMD_HBO_OFFLOAD          = 12,
    SPINEL_CMD_HBO_RECLAIM          = 13,
    SPINEL_CMD_HBO_DROP             = 14,
    SPINEL_CMD_HBO_OFFLOADED        = 15,
    SPINEL_CMD_HBO_RECLAIMED        = 16,
    SPINEL_CMD_HBO_DROPED           = 17,

    SPINEL_CMD_NEST__BEGIN          = 15296,
    SPINEL_CMD_NEST__END            = 15360,

    SPINEL_CMD_VENDOR__BEGIN        = 15360,
    SPINEL_CMD_VENDOR__END          = 16384,

    SPINEL_CMD_EXPERIMENTAL__BEGIN  = 2000000,
    SPINEL_CMD_EXPERIMENTAL__END    = 2097152,
};

enum
{
    SPINEL_CAP_LOCK                  = 1,
    SPINEL_CAP_NET_SAVE              = 2,
    SPINEL_CAP_HBO                   = 3,
    SPINEL_CAP_POWER_SAVE            = 4,

    SPINEL_CAP_COUNTERS              = 5,

    SPINEL_CAP_802_15_4__BEGIN        = 16,
    SPINEL_CAP_802_15_4_2003          = (SPINEL_CAP_802_15_4__BEGIN + 0),
    SPINEL_CAP_802_15_4_2006          = (SPINEL_CAP_802_15_4__BEGIN + 1),
    SPINEL_CAP_802_15_4_2011          = (SPINEL_CAP_802_15_4__BEGIN + 2),
    SPINEL_CAP_802_15_4_PIB           = (SPINEL_CAP_802_15_4__BEGIN + 5),
    SPINEL_CAP_802_15_4_2450MHZ_OQPSK = (SPINEL_CAP_802_15_4__BEGIN + 8),
    SPINEL_CAP_802_15_4_915MHZ_OQPSK  = (SPINEL_CAP_802_15_4__BEGIN + 9),
    SPINEL_CAP_802_15_4_868MHZ_OQPSK  = (SPINEL_CAP_802_15_4__BEGIN + 10),
    SPINEL_CAP_802_15_4_915MHZ_BPSK   = (SPINEL_CAP_802_15_4__BEGIN + 11),
    SPINEL_CAP_802_15_4_868MHZ_BPSK   = (SPINEL_CAP_802_15_4__BEGIN + 12),
    SPINEL_CAP_802_15_4_915MHZ_ASK    = (SPINEL_CAP_802_15_4__BEGIN + 13),
    SPINEL_CAP_802_15_4_868MHZ_ASK    = (SPINEL_CAP_802_15_4__BEGIN + 14),
    SPINEL_CAP_802_15_4__END          = 32,

    SPINEL_CAP_ROLE__BEGIN           = 48,
    SPINEL_CAP_ROLE_ROUTER           = (SPINEL_CAP_ROLE__BEGIN + 0),
    SPINEL_CAP_ROLE_SLEEPY           = (SPINEL_CAP_ROLE__BEGIN + 1),
    SPINEL_CAP_ROLE__END             = 52,

    SPINEL_CAP_NET__BEGIN            = 52,
    SPINEL_CAP_NET_THREAD_1_0        = (SPINEL_CAP_NET__BEGIN + 0),
    SPINEL_CAP_NET__END              = 64,

    SPINEL_CAP_OPENTHREAD__BEGIN     = 512,
    SPINEL_CAP_MAC_WHITELIST         = (SPINEL_CAP_OPENTHREAD__BEGIN + 0),
    SPINEL_CAP_OPENTHREAD__END       = 640,

    SPINEL_CAP_NEST__BEGIN           = 15296,
    SPINEL_CAP_NEST_LEGACY_INTERFACE = (SPINEL_CAP_NEST__BEGIN + 0),
    SPINEL_CAP_NEST_LEGACY_NET_WAKE  = (SPINEL_CAP_NEST__BEGIN + 1),
    SPINEL_CAP_NEST_TRANSMIT_HOOK    = (SPINEL_CAP_NEST__BEGIN + 2),
    SPINEL_CAP_NEST__END             = 15360,

    SPINEL_CAP_VENDOR__BEGIN         = 15360,
    SPINEL_CAP_VENDOR__END           = 16384,

    SPINEL_CAP_EXPERIMENTAL__BEGIN   = 2000000,
    SPINEL_CAP_EXPERIMENTAL__END     = 2097152,
};

typedef enum
{
    SPINEL_PROP_LAST_STATUS             = 0,        ///< status [i]
    SPINEL_PROP_PROTOCOL_VERSION        = 1,        ///< major, minor [i,i]
    SPINEL_PROP_NCP_VERSION             = 2,        ///< version string [U]
    SPINEL_PROP_INTERFACE_TYPE          = 3,        ///< [i]
    SPINEL_PROP_VENDOR_ID               = 4,        ///< [i]
    SPINEL_PROP_CAPS                    = 5,        ///< capability list [A(i)]
    SPINEL_PROP_INTERFACE_COUNT         = 6,        ///< Interface count [C]
    SPINEL_PROP_POWER_STATE             = 7,        ///< PowerState [C]
    SPINEL_PROP_HWADDR                  = 8,        ///< PermEUI64 [E]
    SPINEL_PROP_LOCK                    = 9,        ///< PropLock [b]
    SPINEL_PROP_HBO_MEM_MAX             = 10,       ///< Max offload mem [S]
    SPINEL_PROP_HBO_BLOCK_MAX           = 11,       ///< Max offload block [S]

    SPINEL_PROP_PHY__BEGIN              = 0x20,
    SPINEL_PROP_PHY_ENABLED             = SPINEL_PROP_PHY__BEGIN + 0, ///< [b]
    SPINEL_PROP_PHY_CHAN                = SPINEL_PROP_PHY__BEGIN + 1, ///< [C]
    SPINEL_PROP_PHY_CHAN_SUPPORTED      = SPINEL_PROP_PHY__BEGIN + 2, ///< [A(C)]
    SPINEL_PROP_PHY_FREQ                = SPINEL_PROP_PHY__BEGIN + 3, ///< kHz [L]
    SPINEL_PROP_PHY_CCA_THRESHOLD       = SPINEL_PROP_PHY__BEGIN + 4, ///< dBm [c]
    SPINEL_PROP_PHY_TX_POWER            = SPINEL_PROP_PHY__BEGIN + 5, ///< [c]
    SPINEL_PROP_PHY_RSSI                = SPINEL_PROP_PHY__BEGIN + 6, ///< dBm [c]
    SPINEL_PROP_PHY__END                = 0x30,

    SPINEL_PROP_MAC__BEGIN             = 0x30,
    SPINEL_PROP_MAC_SCAN_STATE         = SPINEL_PROP_MAC__BEGIN + 0, ///< [C]
    SPINEL_PROP_MAC_SCAN_MASK          = SPINEL_PROP_MAC__BEGIN + 1, ///< [A(C)]
    SPINEL_PROP_MAC_SCAN_PERIOD        = SPINEL_PROP_MAC__BEGIN + 2, ///< ms-per-channel [S]
    SPINEL_PROP_MAC_SCAN_BEACON        = SPINEL_PROP_MAC__BEGIN + 3, ///< chan,rssi,(laddr,saddr,panid,lqi),(proto,xtra) [CcT(ESSC.)T(i).]
    SPINEL_PROP_MAC_15_4_LADDR         = SPINEL_PROP_MAC__BEGIN + 4, ///< [E]
    SPINEL_PROP_MAC_15_4_SADDR         = SPINEL_PROP_MAC__BEGIN + 5, ///< [S]
    SPINEL_PROP_MAC_15_4_PANID         = SPINEL_PROP_MAC__BEGIN + 6, ///< [S]
    SPINEL_PROP_MAC_RAW_STREAM_ENABLED = SPINEL_PROP_MAC__BEGIN + 7, ///< [C]
    SPINEL_PROP_MAC_FILTER_MODE        = SPINEL_PROP_MAC__BEGIN + 8, ///< [C]
    SPINEL_PROP_MAC__END               = 0x40,

    SPINEL_PROP_MAC_EXT__BEGIN         = 0x1300,
    /// MAC Whitelist
    /** Format: `A(T(Ec))`
     *
     * Structure Parameters:
     *
     * * `E`: EUI64 address of node
     * * `c`: Optional fixed RSSI. 127 means not set.
     */
    SPINEL_PROP_MAC_WHITELIST          = SPINEL_PROP_MAC_EXT__BEGIN + 0,

    /// MAC Whitelist Enabled Flag
    /** Format: `b`
     */
    SPINEL_PROP_MAC_WHITELIST_ENABLED  = SPINEL_PROP_MAC_EXT__BEGIN + 1,
    SPINEL_PROP_MAC_EXT__END           = 0x1400,

    SPINEL_PROP_NET__BEGIN           = 0x40,
    SPINEL_PROP_NET_SAVED            = SPINEL_PROP_NET__BEGIN + 0, ///< [b]
    SPINEL_PROP_NET_IF_UP            = SPINEL_PROP_NET__BEGIN + 1, ///< [b]
    SPINEL_PROP_NET_STACK_UP         = SPINEL_PROP_NET__BEGIN + 2, ///< [C]
    SPINEL_PROP_NET_ROLE             = SPINEL_PROP_NET__BEGIN + 3, ///< [C]
    SPINEL_PROP_NET_NETWORK_NAME     = SPINEL_PROP_NET__BEGIN + 4, ///< [U]
    SPINEL_PROP_NET_XPANID           = SPINEL_PROP_NET__BEGIN + 5, ///< [D]
    SPINEL_PROP_NET_MASTER_KEY       = SPINEL_PROP_NET__BEGIN + 6, ///< [D]
    SPINEL_PROP_NET_KEY_SEQUENCE     = SPINEL_PROP_NET__BEGIN + 7, ///< [L]
    SPINEL_PROP_NET_PARTITION_ID     = SPINEL_PROP_NET__BEGIN + 8, ///< [L]
    SPINEL_PROP_NET__END             = 0x50,

    SPINEL_PROP_THREAD__BEGIN          = 0x50,
    SPINEL_PROP_THREAD_LEADER_ADDR     = SPINEL_PROP_THREAD__BEGIN + 0, ///< [6]
    SPINEL_PROP_THREAD_PARENT          = SPINEL_PROP_THREAD__BEGIN + 1, ///< LADDR, SADDR [ES]
    SPINEL_PROP_THREAD_CHILD_TABLE     = SPINEL_PROP_THREAD__BEGIN + 2, ///< [A(T(ES))]
    SPINEL_PROP_THREAD_LEADER_RID      = SPINEL_PROP_THREAD__BEGIN + 3, ///< [C]
    SPINEL_PROP_THREAD_LEADER_WEIGHT   = SPINEL_PROP_THREAD__BEGIN + 4, ///< [C]
    SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT
                                       = SPINEL_PROP_THREAD__BEGIN + 5, ///< [C]
    SPINEL_PROP_THREAD_NETWORK_DATA    = SPINEL_PROP_THREAD__BEGIN + 6, ///< [D]
    SPINEL_PROP_THREAD_NETWORK_DATA_VERSION
                                       = SPINEL_PROP_THREAD__BEGIN + 7, ///< [S]
    SPINEL_PROP_THREAD_STABLE_NETWORK_DATA
                                       = SPINEL_PROP_THREAD__BEGIN + 8, ///< [D]
    SPINEL_PROP_THREAD_STABLE_NETWORK_DATA_VERSION
                                       = SPINEL_PROP_THREAD__BEGIN + 9,  ///< [S]
    SPINEL_PROP_THREAD_ON_MESH_NETS    = SPINEL_PROP_THREAD__BEGIN + 10, ///< array(ipv6prefix,prefixlen,stable,flags) [A(T(6CbC))]
    SPINEL_PROP_THREAD_LOCAL_ROUTES    = SPINEL_PROP_THREAD__BEGIN + 11, ///< array(ipv6prefix,prefixlen,stable,flags) [A(T(6CbC))]
    SPINEL_PROP_THREAD_ASSISTING_PORTS = SPINEL_PROP_THREAD__BEGIN + 12, ///< array(portn) [A(S)]
    SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE
                                       = SPINEL_PROP_THREAD__BEGIN + 13, ///< [b]

    /// Thread Mode
    /** Format: `C`
     *
     *  This property contains the value of the mode
     *  TLV for this node. The meaning of the bits in this
     *  bitfield are defined by section 4.5.2 of the Thread
     *  specification.
     */
    SPINEL_PROP_THREAD_MODE            = SPINEL_PROP_THREAD__BEGIN + 14,
    SPINEL_PROP_THREAD__END            = 0x60,

    SPINEL_PROP_THREAD_EXT__BEGIN      = 0x1500,

    /// Thread Child Timeout
    /** Format: `L`
     *
     *  Used when operating in the Child role.
     */
    SPINEL_PROP_THREAD_CHILD_TIMEOUT   = SPINEL_PROP_THREAD_EXT__BEGIN + 0,

    /// Thread RLOC16
    /** Format: `S`
     */
    SPINEL_PROP_THREAD_RLOC16          = SPINEL_PROP_THREAD_EXT__BEGIN + 1,

    /// Thread Router Upgrade Threshold
    /** Format: `C`
     */
    SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD
                                       = SPINEL_PROP_THREAD_EXT__BEGIN + 2,

    /// Thread Context Reuse Delay
    /** Format: `L`
     */
    SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY
                                       = SPINEL_PROP_THREAD_EXT__BEGIN + 3,

    /// Thread Network ID Timeout
    /** Format: `C`
     */
    SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT
                                       = SPINEL_PROP_THREAD_EXT__BEGIN + 4,

    SPINEL_PROP_THREAD_EXT__END        = 0x1600,

    SPINEL_PROP_IPV6__BEGIN          = 0x60,
    SPINEL_PROP_IPV6_LL_ADDR         = SPINEL_PROP_IPV6__BEGIN + 0, ///< [6]
    SPINEL_PROP_IPV6_ML_ADDR         = SPINEL_PROP_IPV6__BEGIN + 1, ///< [6C]
    SPINEL_PROP_IPV6_ML_PREFIX       = SPINEL_PROP_IPV6__BEGIN + 2, ///< [6C]
    SPINEL_PROP_IPV6_ADDRESS_TABLE   = SPINEL_PROP_IPV6__BEGIN + 3, ///< array(ipv6addr,prefixlen,valid,preferred,flags) [A(T(6CLLC))]
    SPINEL_PROP_IPV6_ROUTE_TABLE     = SPINEL_PROP_IPV6__BEGIN + 4, ///< array(ipv6prefix,prefixlen,iface,flags) [A(T(6CCC))]
    SPINEL_PROP_IPV6__END            = 0x70,

    SPINEL_PROP_STREAM__BEGIN       = 0x70,
    SPINEL_PROP_STREAM_DEBUG        = SPINEL_PROP_STREAM__BEGIN + 0, ///< [U]
    SPINEL_PROP_STREAM_RAW          = SPINEL_PROP_STREAM__BEGIN + 1, ///< [D]
    SPINEL_PROP_STREAM_NET          = SPINEL_PROP_STREAM__BEGIN + 2, ///< [D]
    SPINEL_PROP_STREAM_NET_INSECURE = SPINEL_PROP_STREAM__BEGIN + 3, ///< [D]
    SPINEL_PROP_STREAM__END         = 0x80,


    /// UART Bitrate
    /** Format: `L`
     *
     *  If the NCP is using a UART to communicate with the host,
     *  this property allows the host to change the bitrate
     *  of the serial connection. The value encoding is `L`,
     *  which is a little-endian 32-bit unsigned integer.
     *  The host should not assume that all possible numeric values
     *  are supported.
     *
     *  If implemented by the NCP, this property should be persistent
     *  across software resets and forgotten upon hardware resets.
     *
     *  This property is only implemented when a UART is being
     *  used for Spinel. This property is optional.
     *
     *  When changing the bitrate, all frames will be received
     *  at the previous bitrate until the response frame to this command
     *  is received. Once a successful response frame is received by
     *  the host, all further frames will be transmitted at the new
     *  bitrate.
     */
    SPINEL_PROP_UART_BITRATE    = 0x100,

    /// UART Software Flow Control
    /** Format: `b`
     *
     *  If the NCP is using a UART to communicate with the host,
     *  this property allows the host to determine if software flow
     *  control (XON/XOFF style) should be used and (optionally) to
     *  turn it on or off.
     *
     *  This property is only implemented when a UART is being
     *  used for Spinel. This property is optional.
     */
    SPINEL_PROP_UART_XON_XOFF   = 0x101,

    SPINEL_PROP_15_4_PIB__BEGIN     = 1024,
    // For direct access to the 802.15.4 PID.
    // Individual registers are fetched using
    // `SPINEL_PROP_15_4_PIB__BEGIN+[PIB_IDENTIFIER]`
    // Only supported if SPINEL_CAP_15_4_PIB is set.
    //
    // For brevity, the entire 802.15.4 PIB space is
    // not defined here, but a few choice attributes
    // are defined for illustration and convenience.
    SPINEL_PROP_15_4_PIB_PHY_CHANNELS_SUPPORTED = SPINEL_PROP_15_4_PIB__BEGIN + 0x01, ///< [A(L)]
    SPINEL_PROP_15_4_PIB_MAC_PROMISCUOUS_MODE   = SPINEL_PROP_15_4_PIB__BEGIN + 0x51, ///< [b]
    SPINEL_PROP_15_4_PIB_MAC_SECURITY_ENABLED   = SPINEL_PROP_15_4_PIB__BEGIN + 0x5d, ///< [b]
    SPINEL_PROP_15_4_PIB__END       = 1280,

    SPINEL_PROP_CNTR__BEGIN        = 1280,

    /// Counter reset behavior
    /** Format: `C`
     *  Writing a '1' to this property will reset
     *  all of the counters to zero. */
    SPINEL_PROP_CNTR_RESET             = SPINEL_PROP_CNTR__BEGIN + 0,

    /// The total number of transmissions.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_PKT_TOTAL      = SPINEL_PROP_CNTR__BEGIN + 1,

    /// The number of transmissions with ack request.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_PKT_ACK_REQ    = SPINEL_PROP_CNTR__BEGIN + 2,

    /// The number of transmissions that were acked.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_PKT_ACKED      = SPINEL_PROP_CNTR__BEGIN + 3,

    /// The number of transmissions without ack request.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_PKT_NO_ACK_REQ = SPINEL_PROP_CNTR__BEGIN + 4,

    /// The number of transmitted data.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_PKT_DATA       = SPINEL_PROP_CNTR__BEGIN + 5,

    /// The number of transmitted data poll.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_PKT_DATA_POLL  = SPINEL_PROP_CNTR__BEGIN + 6,

    /// The number of transmitted beacon.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_PKT_BEACON     = SPINEL_PROP_CNTR__BEGIN + 7,

    /// The number of transmitted beacon request.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_PKT_BEACON_REQ = SPINEL_PROP_CNTR__BEGIN + 8,

    /// The number of transmitted other types of frames.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_PKT_OTHER      = SPINEL_PROP_CNTR__BEGIN + 9,

    /// The number of retransmission times.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_PKT_RETRY      = SPINEL_PROP_CNTR__BEGIN + 10,

    /// The number of CCA failure times.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_ERR_CCA        = SPINEL_PROP_CNTR__BEGIN + 11,

    /// The total number of received packets.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_PKT_TOTAL      = SPINEL_PROP_CNTR__BEGIN + 100,

    /// The number of received data.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_PKT_DATA       = SPINEL_PROP_CNTR__BEGIN + 101,

    /// The number of received data poll.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_PKT_DATA_POLL  = SPINEL_PROP_CNTR__BEGIN + 102,

    /// The number of received beacon.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_PKT_BEACON     = SPINEL_PROP_CNTR__BEGIN + 103,

    /// The number of received beacon request.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_PKT_BEACON_REQ = SPINEL_PROP_CNTR__BEGIN + 104,

    /// The number of received other types of frames.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_PKT_OTHER      = SPINEL_PROP_CNTR__BEGIN + 105,

    /// The number of received packets filtered by whitelist.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_PKT_FILT_WL    = SPINEL_PROP_CNTR__BEGIN + 106,

    /// The number of received packets filtered by destination check.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_PKT_FILT_DA    = SPINEL_PROP_CNTR__BEGIN + 107,

    /// The number of received packets that are empty.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_ERR_EMPTY      = SPINEL_PROP_CNTR__BEGIN + 108,

    /// The number of received packets from an unknown neighbor.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_ERR_UKWN_NBR   = SPINEL_PROP_CNTR__BEGIN + 109,

    /// The number of received packets whose source address is invalid.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_ERR_NVLD_SADDR = SPINEL_PROP_CNTR__BEGIN + 110,

    /// The number of received packets with a security error.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_ERR_SECURITY   = SPINEL_PROP_CNTR__BEGIN + 111,

    /// The number of received packets with a checksum error.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_ERR_BAD_FCS    = SPINEL_PROP_CNTR__BEGIN + 112,

    /// The number of received packets with other errors.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_ERR_OTHER      = SPINEL_PROP_CNTR__BEGIN + 113,

    /// The total number of secure transmitted IP messages.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_IP_SEC_TOTAL   = SPINEL_PROP_CNTR__BEGIN + 200,

    /// The total number of insecure transmitted IP messages.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_IP_INSEC_TOTAL = SPINEL_PROP_CNTR__BEGIN + 201,

    /// The number of dropped (not transmitted) IP messages.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_IP_DROPPED     = SPINEL_PROP_CNTR__BEGIN + 202,

    /// The total number of secure received IP message.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_IP_SEC_TOTAL   = SPINEL_PROP_CNTR__BEGIN + 203,

    /// The total number of insecure received IP message.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_IP_INSEC_TOTAL = SPINEL_PROP_CNTR__BEGIN + 204,

    /// The number of dropped received IP messages.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_IP_DROPPED     = SPINEL_PROP_CNTR__BEGIN + 205,

    /// The number of transmitted spinel frames.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_TX_SPINEL_TOTAL   = SPINEL_PROP_CNTR__BEGIN + 300,

    /// The number of received spinel frames.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_SPINEL_TOTAL   = SPINEL_PROP_CNTR__BEGIN + 301,

    /// The number of received spinel frames with error.
    /** Format: `L` (Read-only) */
    SPINEL_PROP_CNTR_RX_SPINEL_ERR     = SPINEL_PROP_CNTR__BEGIN + 302,

    SPINEL_PROP_CNTR__END       = 2048,

    SPINEL_PROP_NEST__BEGIN         = 15296,
    SPINEL_PROP_NEST__END           = 15360,

    SPINEL_PROP_VENDOR__BEGIN       = 15360,
    SPINEL_PROP_VENDOR__END         = 16384,

    SPINEL_PROP_EXPERIMENTAL__BEGIN = 2000000,
    SPINEL_PROP_EXPERIMENTAL__END   = 2097152,
} spinel_prop_key_t;

// ----------------------------------------------------------------------------

#define SPINEL_HEADER_FLAG               0x80

#define SPINEL_HEADER_TID_SHIFT          0
#define SPINEL_HEADER_TID_MASK           (15 << SPINEL_HEADER_TID_SHIFT)

#define SPINEL_HEADER_IID_SHIFT          4
#define SPINEL_HEADER_IID_MASK           (3 << SPINEL_HEADER_IID_SHIFT)

#define SPINEL_HEADER_IID_0              (0 << SPINEL_HEADER_IID_SHIFT)
#define SPINEL_HEADER_IID_1              (1 << SPINEL_HEADER_IID_SHIFT)
#define SPINEL_HEADER_IID_2              (2 << SPINEL_HEADER_IID_SHIFT)
#define SPINEL_HEADER_IID_3              (3 << SPINEL_HEADER_IID_SHIFT)

#define SPINEL_HEADER_GET_IID(x)            (((x) & SPINEL_HEADER_IID_MASK) >> SPINEL_HEADER_IID_SHIFT)
#define SPINEL_HEADER_GET_TID(x)            (spinel_tid_t)(((x)&SPINEL_HEADER_TID_MASK)>>SPINEL_HEADER_TID_SHIFT)

#define SPINEL_GET_NEXT_TID(x)      (spinel_tid_t)((x)>=0xF?1:(x)+1)

#define SPINEL_BEACON_THREAD_FLAG_VERSION_SHIFT 4
#define SPINEL_BEACON_THREAD_FLAG_VERSION_MASK  (0xf << SPINEL_BEACON_THREAD_FLAG_VERSION_SHIFT)
#define SPINEL_BEACON_THREAD_FLAG_JOINABLE      (1 << 0)
#define SPINEL_BEACON_THREAD_FLAG_NATIVE        (1 << 3)

// ----------------------------------------------------------------------------

enum
{
    SPINEL_DATATYPE_NULL_C        = 0,
    SPINEL_DATATYPE_VOID_C        = '.',
    SPINEL_DATATYPE_BOOL_C        = 'b',
    SPINEL_DATATYPE_UINT8_C       = 'C',
    SPINEL_DATATYPE_INT8_C        = 'c',
    SPINEL_DATATYPE_UINT16_C      = 'S',
    SPINEL_DATATYPE_INT16_C       = 's',
    SPINEL_DATATYPE_UINT32_C      = 'L',
    SPINEL_DATATYPE_INT32_C       = 'l',
    SPINEL_DATATYPE_UINT_PACKED_C = 'i',
    SPINEL_DATATYPE_IPv6ADDR_C    = '6',
    SPINEL_DATATYPE_EUI64_C       = 'E',
    SPINEL_DATATYPE_EUI48_C       = 'e',
    SPINEL_DATATYPE_DATA_C        = 'D',
    SPINEL_DATATYPE_UTF8_C        = 'U', //!< Zero-Terminated UTF8-Encoded String
    SPINEL_DATATYPE_STRUCT_C      = 'T',
    SPINEL_DATATYPE_ARRAY_C       = 'A',
};

typedef char spinel_datatype_t;

#define SPINEL_DATATYPE_NULL_S        ""
#define SPINEL_DATATYPE_VOID_S        "."
#define SPINEL_DATATYPE_BOOL_S        "b"
#define SPINEL_DATATYPE_UINT8_S       "C"
#define SPINEL_DATATYPE_INT8_S        "c"
#define SPINEL_DATATYPE_UINT16_S      "S"
#define SPINEL_DATATYPE_INT16_S       "s"
#define SPINEL_DATATYPE_UINT32_S      "L"
#define SPINEL_DATATYPE_INT32_S       "l"
#define SPINEL_DATATYPE_UINT_PACKED_S "i"
#define SPINEL_DATATYPE_IPv6ADDR_S    "6"
#define SPINEL_DATATYPE_EUI64_S       "E"
#define SPINEL_DATATYPE_EUI48_S       "e"
#define SPINEL_DATATYPE_DATA_S        "D"
#define SPINEL_DATATYPE_UTF8_S        "U" //!< Zero-Terminated UTF8-Encoded String
#define SPINEL_DATATYPE_STRUCT_S      "T"
#define SPINEL_DATATYPE_ARRAY_S       "A"

SPINEL_API_EXTERN spinel_ssize_t spinel_datatype_pack(uint8_t *data_out, spinel_size_t data_len,
                                                      const char *pack_format, ...);
SPINEL_API_EXTERN spinel_ssize_t spinel_datatype_vpack(uint8_t *data_out, spinel_size_t data_len,
                                                       const char *pack_format, va_list args);
SPINEL_API_EXTERN spinel_ssize_t spinel_datatype_unpack(const uint8_t *data_in, spinel_size_t data_len,
                                                        const char *pack_format, ...);
SPINEL_API_EXTERN spinel_ssize_t spinel_datatype_vunpack(const uint8_t *data_in, spinel_size_t data_len,
                                                         const char *pack_format, va_list args);

SPINEL_API_EXTERN spinel_ssize_t spinel_packed_uint_decode(const uint8_t *bytes, spinel_size_t len,
                                                           unsigned int *value);
SPINEL_API_EXTERN spinel_ssize_t spinel_packed_uint_encode(uint8_t *bytes, spinel_size_t len, unsigned int value);
SPINEL_API_EXTERN spinel_ssize_t spinel_packed_uint_size(unsigned int value);

SPINEL_API_EXTERN const char *spinel_next_packed_datatype(const char *pack_format);

// ----------------------------------------------------------------------------

SPINEL_API_EXTERN const char *spinel_prop_key_to_cstr(spinel_prop_key_t prop_key);

SPINEL_API_EXTERN const char *spinel_status_to_cstr(spinel_status_t status);

// ----------------------------------------------------------------------------

__END_DECLS

#endif /* defined(SPINEL_HEADER_INCLUDED) */
