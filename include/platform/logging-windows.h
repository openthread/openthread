/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief
 *  This file defines the WPP Tracing Definitions.
 */

#ifndef _LOGGING_WINDOWS_H
#define _LOGGING_WINDOWS_H

#define OPENTHREAD_ENABLE_CERT_LOG 1

//
// Tracing Definitions: {1AA98926-2E40-43D1-9D83-34C6BE816365}
//

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID( \
        OpenThreadGUID, (1AA98926,2E40,43D1,9D83,34C6BE816365), \
        WPP_DEFINE_BIT(DRIVER_DEFAULT)      /* 0x00000001 */    \
        WPP_DEFINE_BIT(DRIVER_IOCTL)        /* 0x00000002 */    \
        WPP_DEFINE_BIT(DRIVER_OID)          /* 0x00000004 */    \
        WPP_DEFINE_BIT(DRIVER_DATA_PATH)    /* 0x00000008 */    \
        WPP_DEFINE_BIT(OT_API)              /* 0x00000010 */    \
        WPP_DEFINE_BIT(OT_MLE)              /* 0x00000020 */    \
        WPP_DEFINE_BIT(OT_ARP)              /* 0x00000040 */    \
        WPP_DEFINE_BIT(OT_NETD)             /* 0x00000080 */    \
        WPP_DEFINE_BIT(OT_ICMP)             /* 0x00000100 */    \
        WPP_DEFINE_BIT(OT_IPV6)             /* 0x00000200 */    \
        WPP_DEFINE_BIT(OT_MAC)              /* 0x00000400 */    \
        WPP_DEFINE_BIT(OT_MEM)              /* 0x00000800 */    \
        WPP_DEFINE_BIT(OT_NCP)              /* 0x00001000 */    \
        WPP_DEFINE_BIT(OT_COAP)             /* 0x00002000 */    \
        WPP_DEFINE_BIT(OT_DEFAULT)          /* 0x00004000 */    \
        WPP_DEFINE_BIT(OT_MBEDTLS)          /* 0x00008000 */    \
        WPP_DEFINE_BIT(OT_DUMP)             /* 0x00010000 */    \
        WPP_DEFINE_BIT(OT_NDIAG)            /* 0x00020000 */    \
        )

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flag) \
    WPP_FLAG_LOGGER(flag)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flag) \
    (WPP_FLAG_ENABLED(flag) && WPP_CONTROL(WPP_BIT_ ## flag).Level >= lvl)

// Suppress warnings about constants in logical expressions because the
// level is often a constant
#define WPP_LEVEL_FLAGS_PRE(LEVEL,FLAGS) __pragma(warning(suppress:25039 25040 25078 25080))

// Map no-argument macros (dummy) back to regular macros
#define WPP_LEVEL_FLAGS__PRE(lvl, flags, dummy)             WPP_LEVEL_FLAGS_PRE(lvl, flags)
#define WPP_LEVEL_FLAGS__POST(lvl, flags, dummy)            WPP_LEVEL_FLAGS_POST(lvl, flags)

#define WPP_LEVEL_FLAGS_EXP_ENABLED(LEVEL,FLAGS,EXP)  \
    WPP_LEVEL_FLAGS_ENABLED (LEVEL,FLAGS)
#define WPP_LEVEL_FLAGS_EXP_LOGGER(LEVEL,FLAGS,EXP)   \
    WPP_LEVEL_FLAGS_LOGGER (LEVEL,FLAGS)

#define WPP_LOGIPV6(x) WPP_LOGPAIR(16, (x))

// begin_wpp config
// DEFINE_CPLX_TYPE(IPV6ADDR, WPP_LOGIPV6, IN6_ADDR *, ItemIPV6Addr, "s", _IPV6_, 0);
// end_wpp

// begin_wpp config
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX(LogFuncEntry, "---> %!FUNC!");
// FUNC LogFuncEntry{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS);
// end_wpp

// begin_wpp config
// USEPREFIX(LogFuncEntryMsg, "---> %!FUNC!%!SPACE!");
// FUNC LogFuncEntryMsg{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX(LogFuncExit, "<--- %!FUNC!");
// FUNC LogFuncExit{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS);
// end_wpp

// begin_wpp config
// USEPREFIX(LogFuncExitMsg, "<--- %!FUNC!%!SPACE!");
// FUNC LogFuncExitMsg{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX(LogFuncExitNT, "<--- %!FUNC!");
// FUNC LogFuncExitNT{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS, EXP);
// USESUFFIX(LogFuncExitNT, " %!STATUS!", EXP);
// end_wpp

// begin_wpp config
// USEPREFIX(LogFuncExitNDIS, "<--- %!FUNC!");
// FUNC LogFuncExitNDIS{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS, EXP);
// USESUFFIX(LogFuncExitNDIS, " %!NDIS_STATUS!", EXP);
// end_wpp  

// begin_wpp config
// USEPREFIX(LogFuncExitWIN, "<--- %!FUNC!");
// FUNC LogFuncExitWIN{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS, EXP);
// USESUFFIX(LogFuncExitWIN, " %!WINERROR!", EXP);
// end_wpp

// begin_wpp config
// LogError{LEVEL=TRACE_LEVEL_ERROR}(FLAGS, MSG, ...);
// end_wpp

// begin_wpp config
// LogWarning{LEVEL=TRACE_LEVEL_WARNING}(FLAGS, MSG, ...);
// end_wpp

// begin_wpp config
// LogInfo{LEVEL=TRACE_LEVEL_INFORMATION}(FLAGS, MSG, ...);
// end_wpp

// begin_wpp config
// LogVerbose{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS, MSG, ...);
// end_wpp

//
// Custom types
//

// begin_wpp config
// CUSTOM_TYPE(otError, ItemEnum(ThreadError));
// CUSTOM_TYPE(otDeviceRole, ItemEnum(otDeviceRole));
// end_wpp

//
// otCore Definitions
//

// ==API==

// begin_wpp config
// USEPREFIX (otLogCritApi, "API%!SPACE!");
// otLogCritApi{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_API}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnApi, "API%!SPACE!");
// otLogWarnApi{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_API}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoApi, "API%!SPACE!");
// otLogInfoApi{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_API}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgApi, "API%!SPACE!");
// otLogDebgApi{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_API}(MSG, ...);
// end_wpp

// ==NCP==

// begin_wpp config
// USEPREFIX (otLogCritNcp, "NCP%!SPACE!");
// otLogCritNcp{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_NCP}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnNcp, "NCP%!SPACE!");
// otLogWarnNcp{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_NCP}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoNcp, "NCP%!SPACE!");
// otLogInfoNcp{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_NCP}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgNcp, "NCP%!SPACE!");
// otLogDebgNcp{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_NCP}(MSG, ...);
// end_wpp

// ==COAP==

// begin_wpp config
// USEPREFIX (otLogCritMeshCoP, "COAP%!SPACE!");
// otLogCritMeshCoP{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_COAP}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnMeshCoP, "COAP%!SPACE!");
// otLogWarnMeshCoP{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_COAP}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoMeshCoP, "COAP%!SPACE!");
// otLogInfoMeshCoP{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_COAP}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgMeshCoP, "COAP%!SPACE!");
// otLogDebgMeshCoP{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_COAP}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogCertMeshCoP, "COAP%!SPACE!");
// otLogCertMeshCoP{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_COAP}(MSG, ...);
// end_wpp

// ==MBEDTLS==

// begin_wpp config
// USEPREFIX (otLogCritMbedTls, "MBED%!SPACE!");
// otLogCritMbedTls{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_MBEDTLS}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnMbedTls, "MBED%!SPACE!");
// otLogWarnMbedTls{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_MBEDTLS}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoMbedTls, "MBED%!SPACE!");
// otLogInfoMbedTls{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_MBEDTLS}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgMbedTls, "MBED%!SPACE!");
// otLogDebgMbedTls{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_MBEDTLS}(MSG, ...);
// end_wpp

// ==MLE==

// begin_wpp config
// USEPREFIX (otLogCritMle, "MLE%!SPACE!");
// otLogCritMle{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_MLE}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnMle, "MLE%!SPACE!");
// otLogWarnMle{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_MLE}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnMleErr, "MLE%!SPACE!");
// otLogWarnMleErr{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_MLE}(EXP, MSG, ...);
// USESUFFIX(otLogWarnMleErr, ", %!otError!", EXP);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoMle, "MLE%!SPACE!");
// otLogInfoMle{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_MLE}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgMle, "MLE%!SPACE!");
// otLogDebgMle{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_MLE}(MSG, ...);
// end_wpp

// ==ARP==

// begin_wpp config
// USEPREFIX (otLogCritArp, "ARP%!SPACE!");
// otLogCritArp{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_ARP}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnArp, "ARP%!SPACE!");
// otLogWarnArp{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_ARP}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoArp, "ARP%!SPACE!");
// otLogInfoArp{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_ARP}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgArp, "ARP%!SPACE!");
// otLogDebgArp{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_ARP}(MSG, ...);
// end_wpp

// ==NETD==

// begin_wpp config
// USEPREFIX (otLogCritNetData, "NETD%!SPACE!");
// otLogCritNetData{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_NETD}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnNetData, "NETD%!SPACE!");
// otLogWarnNetData{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_NETD}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoNetData, "NETD%!SPACE!");
// otLogInfoNetData{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_NETD}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgNetData, "NETD%!SPACE!");
// otLogDebgNetData{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_NETD}(MSG, ...);
// end_wpp

// ==ICMP==

// begin_wpp config
// USEPREFIX (otLogCritIcmp, "ICMP%!SPACE!");
// otLogCritIcmp{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_ICMP}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnIcmp, "ICMP%!SPACE!");
// otLogWarnIcmp{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_ICMP}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoIcmp, "ICMP%!SPACE!");
// otLogInfoIcmp{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_ICMP}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgIcmp, "ICMP%!SPACE!");
// otLogDebgIcmp{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_ICMP}(MSG, ...);
// end_wpp

// ==IPV6==

// begin_wpp config
// USEPREFIX (otLogCritIp6, "IPV6%!SPACE!");
// otLogCritIp6{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_IPV6}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnIp6, "IPV6%!SPACE!");
// otLogWarnIp6{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_IPV6}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoIp6, "IPV6%!SPACE!");
// otLogInfoIp6{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_IPV6}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgIp6, "IPV6%!SPACE!");
// otLogDebgIp6{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_IPV6}(MSG, ...);
// end_wpp

// ==MAC==

// begin_wpp config
// USEPREFIX (otLogCritMac, "MAC%!SPACE!");
// otLogCritMac{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_MAC}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnMac, "MAC%!SPACE!");
// otLogWarnMac{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_MAC}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoMac, "MAC%!SPACE!");
// otLogInfoMac{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_MAC}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgMac, "MAC%!SPACE!");
// otLogDebgMac{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_MAC}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgMacErr, "MAC%!SPACE!");
// otLogDebgMacErr{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_MAC}(EXP, MSG, ...);
// USESUFFIX(otLogDebgMacErr, ", %!otError!", EXP);
// end_wpp

// ==MEM==

// begin_wpp config
// USEPREFIX (otLogCritMem, "MEM%!SPACE!");
// otLogCritMem{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_MEM}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnMem, "MEM%!SPACE!");
// otLogWarnMem{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_MEM}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoMem, "MEM%!SPACE!");
// otLogInfoMem{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_MEM}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgMem, "MEM%!SPACE!");
// otLogDebgMem{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_MEM}(MSG, ...);
// end_wpp

// ==DUMP==

// begin_wpp config
// otLogDump{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_DUMP}(MSG, ...);
// end_wpp

// ==MEM==

// begin_wpp config
// USEPREFIX (otLogCritNetDiag, "NETDIAG%!SPACE!");
// otLogCritNetDiag{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_NDIAG}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnNetDiag, "NETDIAG%!SPACE!");
// otLogWarnNetDiag{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_NDIAG}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoNetDiag, "NETDIAG%!SPACE!");
// otLogInfoNetDiag{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_NDIAG}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgNetDiag, "NETDIAG%!SPACE!");
// otLogDebgNetDiag{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_NDIAG}(MSG, ...);
// end_wpp

// ==FUNC==

// begin_wpp config
// USEPREFIX(otLogFuncEntry, "---> %!FUNC!");
// FUNC otLogFuncEntry{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_DEFAULT}(...);
// end_wpp

// begin_wpp config
// USEPREFIX(otLogFuncEntryMsg, "---> %!FUNC!%!SPACE!");
// FUNC otLogFuncEntryMsg{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_DEFAULT}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX(otLogFuncExit, "<--- %!FUNC!");
// FUNC otLogFuncExit{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_DEFAULT}(...);
// end_wpp

// begin_wpp config
// USEPREFIX(otLogFuncExitMsg, "<--- %!FUNC!%!SPACE!");
// FUNC otLogFuncExitMsg{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_DEFAULT}(MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX(otLogFuncExitErr, "<--- %!FUNC!");
// FUNC otLogFuncExitErr{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_DEFAULT}(EXP);
// USESUFFIX(otLogFuncExitErr, " %!otError!", EXP);
// end_wpp

#endif // _LOGGING_WINDOWS_H