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

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <arpa/inet.h>

__BEGIN_DECLS

// ----------------------------------------------------------------------------

#ifndef DOXYGEN_SHOULD_SKIP_THIS
# if defined(__GNUC__) && !SPINEL_EMBEDDED
#  define SPINEL_API_EXTERN              extern __attribute__ ((visibility ("default")))
#  define SPINEL_API_NONNULL_ALL         __attribute__((nonnull))
#  define SPINEL_API_WARN_UNUSED_RESULT  __attribute__((warn_unused_result))
# endif // ifdef __GNUC__
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
#define SPINEL_PROTOCOL_VERSION_THREAD_MINOR    0

#define SPINEL_FRAME_MAX_SIZE                   1300

// ----------------------------------------------------------------------------

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
    SPINEL_STATUS_BUSY              = 12, ///< The device is currently performing an mutually exclusive operation
    SPINEL_STATUS_PROP_NOT_FOUND    = 12, ///< The given property is not recognized.
    SPINEL_STATUS_DROPPED           = 14, ///< A/The packet was dropped.
    SPINEL_STATUS_EMPTY             = 15, ///< The result of the operation is empty.
    SPINEL_STATUS_CMD_TOO_BIG       = 16, ///< The command was too large to fit in the internal buffer.
    SPINEL_STATUS_NO_ACK            = 17, ///< The packet was not acknowledged.
    SPINEL_STATUS_CCA_FAILURE       = 18, ///< The packet was not sent due to a CCA failure.

    SPINEL_STATUS_RESET__BEGIN      = 112,
    SPINEL_STATUS_RESET_POWER_ON    = SPINEL_STATUS_RESET__BEGIN + 0,
    SPINEL_STATUS_RESET_EXTERNAL    = SPINEL_STATUS_RESET__BEGIN + 1,
    SPINEL_STATUS_RESET_SOFTWARE    = SPINEL_STATUS_RESET__BEGIN + 2,
    SPINEL_STATUS_RESET_FAULT       = SPINEL_STATUS_RESET__BEGIN + 3,
    SPINEL_STATUS_RESET_CRASH       = SPINEL_STATUS_RESET__BEGIN + 4,
    SPINEL_STATUS_RESET_ASSERT      = SPINEL_STATUS_RESET__BEGIN + 5,
    SPINEL_STATUS_RESET_OTHER       = SPINEL_STATUS_RESET__BEGIN + 6,
    SPINEL_STATUS_RESET__END        = 128,


    SPINEL_STATUS_VENDOR__BEGIN     = 15360,
    SPINEL_STATUS_VENDOR__END       = 16384,

    SPINEL_STATUS_EXPERIMENTAL__BEGIN = 2000000,
    SPINEL_STATUS_EXPERIMENTAL__END   = 2097152,
} spinel_status_t;

typedef enum
{
    SPINEL_NET_STATE_OFFLINE   = 0,
    SPINEL_NET_STATE_DETACHED  = 1,
    SPINEL_NET_STATE_ATTACHING = 2,
    SPINEL_NET_STATE_ATTACHED  = 3,
} spinel_net_state_t;

typedef enum
{
    SPINEL_NET_ROLE_NONE       = 0,
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
    SPINEL_PHY_MODE_NORMAL       = 0, ///< Normal PHY filtering is in place.
    SPINEL_PHY_MODE_PROMISCUOUS  = 1, ///< All PHY packets matching network are passed up the stack.
    SPINEL_PHY_MODE_MONITOR      = 2, ///< All decoded PHY packets are passed up the stack.
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

typedef struct in6_addr spinel_ipv6addr_t;
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

    SPINEL_CAP_802_15_4__BEGIN       = 16,
    SPINEL_CAP_802_15_4_2003         = (SPINEL_CAP_802_15_4__BEGIN + 0),
    SPINEL_CAP_802_15_4_2006         = (SPINEL_CAP_802_15_4__BEGIN + 1),
    SPINEL_CAP_802_15_4_2011         = (SPINEL_CAP_802_15_4__BEGIN + 2),
    SPINEL_CAP_802_15_4_PIB          = (SPINEL_CAP_802_15_4__BEGIN + 5),
    SPINEL_CAP_802_15_4_2450MHZ_OQPSK = (SPINEL_CAP_802_15_4__BEGIN + 8),
    SPINEL_CAP_802_15_4_915MHZ_OQPSK = (SPINEL_CAP_802_15_4__BEGIN + 9),
    SPINEL_CAP_802_15_4_868MHZ_OQPSK = (SPINEL_CAP_802_15_4__BEGIN + 10),
    SPINEL_CAP_802_15_4_915MHZ_BPSK  = (SPINEL_CAP_802_15_4__BEGIN + 11),
    SPINEL_CAP_802_15_4_868MHZ_BPSK  = (SPINEL_CAP_802_15_4__BEGIN + 12),
    SPINEL_CAP_802_15_4_915MHZ_ASK   = (SPINEL_CAP_802_15_4__BEGIN + 13),
    SPINEL_CAP_802_15_4_868MHZ_ASK   = (SPINEL_CAP_802_15_4__BEGIN + 14),
    SPINEL_CAP_802_15_4__END         = 32,

    SPINEL_CAP_ROLE__BEGIN           = 48,
    SPINEL_CAP_ROLE_ROUTER           = (SPINEL_CAP_ROLE__BEGIN + 0),
    SPINEL_CAP_ROLE_SLEEPY           = (SPINEL_CAP_ROLE__BEGIN + 1),
    SPINEL_CAP_ROLE__END             = 52,

    SPINEL_CAP_NET__BEGIN            = 52,
    SPINEL_CAP_NET_THREAD_1_0        = (SPINEL_CAP_NET__BEGIN + 0),
    SPINEL_CAP_NET__END              = 64,

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
    SPINEL_PROP_PHY_RAW_STREAM_ENABLED  = SPINEL_PROP_PHY__BEGIN + 7, ///< [C]
    SPINEL_PROP_PHY_MODE                = SPINEL_PROP_PHY__BEGIN + 8, ///< [C]
    SPINEL_PROP_PHY__END                = 0x30,

    SPINEL_PROP_MAC__BEGIN           = 0x30,
    SPINEL_PROP_MAC_SCAN_STATE       = SPINEL_PROP_MAC__BEGIN + 0, ///< [C]
    SPINEL_PROP_MAC_SCAN_MASK        = SPINEL_PROP_MAC__BEGIN + 1, ///< [A(C)]
    SPINEL_PROP_MAC_SCAN_PERIOD      = SPINEL_PROP_MAC__BEGIN + 2, ///< ms-per-channel [S]
    SPINEL_PROP_MAC_SCAN_BEACON      = SPINEL_PROP_MAC__BEGIN + 3, ///< chan,rssi,(laddr,saddr,panid,lqi),(proto,xtra) [CcT(ESSC.)T(i).]
    SPINEL_PROP_MAC_15_4_LADDR       = SPINEL_PROP_MAC__BEGIN + 4, ///< [E]
    SPINEL_PROP_MAC_15_4_SADDR       = SPINEL_PROP_MAC__BEGIN + 5, ///< [S]
    SPINEL_PROP_MAC_15_4_PANID       = SPINEL_PROP_MAC__BEGIN + 6, ///< [S]
    SPINEL_PROP_MAC__END             = 0x40,

    SPINEL_PROP_NET__BEGIN           = 0x40,
    SPINEL_PROP_NET_SAVED            = SPINEL_PROP_NET__BEGIN + 0, ///< [b]
    SPINEL_PROP_NET_ENABLED          = SPINEL_PROP_NET__BEGIN + 1, ///< [b]
    SPINEL_PROP_NET_STATE            = SPINEL_PROP_NET__BEGIN + 2, ///< [C]
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
    SPINEL_PROP_THREAD_LEADER_WEIGHT   = SPINEL_PROP_THREAD__BEGIN + 4, ///< [6]
    SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT
                                       = SPINEL_PROP_THREAD__BEGIN + 5, ///< [6]
    SPINEL_PROP_THREAD_NETWORK_DATA    = SPINEL_PROP_THREAD__BEGIN + 6, ///< [D]
    SPINEL_PROP_THREAD_NETWORK_DATA_VERSION
                                       = SPINEL_PROP_THREAD__BEGIN + 7, ///< [S]
    SPINEL_PROP_THREAD_STABLE_NETWORK_DATA
                                       = SPINEL_PROP_THREAD__BEGIN + 8, ///< [D]
    SPINEL_PROP_THREAD_STABLE_NETWORK_DATA_VERSION
                                       = SPINEL_PROP_THREAD__BEGIN + 9,  ///< [S]
    SPINEL_PROP_THREAD__END            = 0x60,

    SPINEL_PROP_IPV6__BEGIN          = 0x60,
    SPINEL_PROP_IPV6_LL_ADDR         = SPINEL_PROP_IPV6__BEGIN + 0, ///< [6]
    SPINEL_PROP_IPV6_ML_ADDR         = SPINEL_PROP_IPV6__BEGIN + 1, ///< [6C]
    SPINEL_PROP_IPV6_ML_PREFIX       = SPINEL_PROP_IPV6__BEGIN + 2, ///< [6C]
    SPINEL_PROP_IPV6_ADDRESS_TABLE   = SPINEL_PROP_IPV6__BEGIN + 3, ///< array(ipv6addr,prefixlen,flags) [A(6CL)]
    SPINEL_PROP_IPV6_ROUTE_TABLE     = SPINEL_PROP_IPV6__BEGIN + 4, ///< array(ipv6prefix,prefixlen,iface,flags) [A(6CCL)]
    SPINEL_PROP_IPV6_EXT_ROUTE_TABLE = SPINEL_PROP_IPV6__BEGIN + 5, ///< array(ipv6prefix,prefixlen,flags) [A(6CL)]
    SPINEL_PROP_IPV6__END            = 0x70,

    SPINEL_PROP_STREAM__BEGIN       = 0x70,
    SPINEL_PROP_STREAM_DEBUG        = SPINEL_PROP_STREAM__BEGIN + 0, ///< [U]
    SPINEL_PROP_STREAM_RAW          = SPINEL_PROP_STREAM__BEGIN + 1, ///< [D]
    SPINEL_PROP_STREAM_NET          = SPINEL_PROP_STREAM__BEGIN + 2, ///< [D]
    SPINEL_PROP_STREAM_NET_INSECURE = SPINEL_PROP_STREAM__BEGIN + 3, ///< [D]
    SPINEL_PROP_STREAM__END         = 0x80,

    SPINEL_PROP_15_4_PIB__BEGIN     = 1024,
    // For direct access to the 802.15.4 PID.
    // Individual registers are fetched using
    // `SPINEL_PROP_15_4_PIB__BEGIN+[PIB_IDENTIFIER]`
    // Only supported if SPINEL_CAP_15_4_PIB is set.
    SPINEL_PROP_15_4_PIB__END       = 1280,

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

SPINEL_API_EXTERN const char* spinel_prop_key_to_cstr(spinel_prop_key_t prop_key);

// ----------------------------------------------------------------------------

__END_DECLS

#endif /* defined(SPINEL_HEADER_INCLUDED) */
