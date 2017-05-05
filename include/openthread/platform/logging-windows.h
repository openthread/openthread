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
        WPP_DEFINE_BIT(OT_MESHCOP)          /* 0x00002000 */    \
        WPP_DEFINE_BIT(OT_DEFAULT)          /* 0x00004000 */    \
        WPP_DEFINE_BIT(OT_MBEDTLS)          /* 0x00008000 */    \
        WPP_DEFINE_BIT(OT_DUMP)             /* 0x00010000 */    \
        WPP_DEFINE_BIT(OT_NDIAG)            /* 0x00020000 */    \
        WPP_DEFINE_BIT(OT_COAP)             /* 0x00040000 */    \
        WPP_DEFINE_BIT(API_DEFAULT)         /* 0x00080000 */    \
        )

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flag)                        WPP_FLAG_LOGGER(flag)
#define WPP_LEVEL_FLAGS_EXP_ENABLED(LEVEL,FLAGS,EXP)            WPP_LEVEL_FLAGS_ENABLED (LEVEL,FLAGS)
#define WPP_LEVEL_FLAGS_CTX_ENABLED(LEVEL,FLAGS,CTX)            WPP_LEVEL_FLAGS_ENABLED (LEVEL,FLAGS)
#define WPP_LEVEL_FLAGS_CTX_EXP_ENABLED(LEVEL,FLAGS,CTX,EXP)    WPP_LEVEL_FLAGS_ENABLED (LEVEL,FLAGS)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flag) \
    (WPP_FLAG_ENABLED(flag) && WPP_CONTROL(WPP_BIT_ ## flag).Level >= lvl)
#define WPP_LEVEL_FLAGS_EXP_LOGGER(LEVEL,FLAGS,EXP)             WPP_LEVEL_FLAGS_LOGGER (LEVEL,FLAGS)
#define WPP_LEVEL_FLAGS_CTX_LOGGER(LEVEL,FLAGS,CTX)             WPP_LEVEL_FLAGS_LOGGER (LEVEL,FLAGS)
#define WPP_LEVEL_FLAGS_CTX_EXP_LOGGER(LEVEL,FLAGS,CTX,EXP)     WPP_LEVEL_FLAGS_LOGGER (LEVEL,FLAGS)

#define WPP_LOGIPV6(x) WPP_LOGPAIR(16, (x))

// begin_wpp config
// DEFINE_CPLX_TYPE(IPV6ADDR, WPP_LOGIPV6, IN6_ADDR *, ItemIPV6Addr, "s", _IPV6_, 0);
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
// USEPREFIX (LogError, " ");
// LogError{LEVEL=TRACE_LEVEL_ERROR}(FLAGS, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (LogWarning, " ");
// LogWarning{LEVEL=TRACE_LEVEL_WARNING}(FLAGS, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (LogInfo, " ");
// LogInfo{LEVEL=TRACE_LEVEL_INFORMATION}(FLAGS, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (LogVerbose, " ");
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
// USEPREFIX (otLogCritApi, "[%p]API%!SPACE!", CTX);
// otLogCritApi{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_API}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnApi, "[%p]API%!SPACE!", CTX);
// otLogWarnApi{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_API}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoApi, "[%p]API%!SPACE!", CTX);
// otLogInfoApi{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_API}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgApi, "[%p]API%!SPACE!", CTX);
// otLogDebgApi{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_API}(CTX, MSG, ...);
// end_wpp

// ==NCP==

// begin_wpp config
// USEPREFIX (otLogCritNcp, "[%p]NCP%!SPACE!", CTX);
// otLogCritNcp{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_NCP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnNcp, "[%p]NCP%!SPACE!", CTX);
// otLogWarnNcp{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_NCP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoNcp, "[%p]NCP%!SPACE!", CTX);
// otLogInfoNcp{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_NCP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgNcp, "[%p]NCP%!SPACE!", CTX);
// otLogDebgNcp{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_NCP}(CTX, MSG, ...);
// end_wpp

// ==MESHCOP==

// begin_wpp config
// USEPREFIX (otLogCritMeshCoP, "[%p]MESHCOP%!SPACE!", CTX);
// otLogCritMeshCoP{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_MESHCOP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnMeshCoP, "[%p]MESHCOP%!SPACE!", CTX);
// otLogWarnMeshCoP{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_MESHCOP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoMeshCoP, "[%p]MESHCOP%!SPACE!", CTX);
// otLogInfoMeshCoP{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_MESHCOP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgMeshCoP, "[%p]MESHCOP%!SPACE!", CTX);
// otLogDebgMeshCoP{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_MESHCOP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogCertMeshCoP, "[%p]MESHCOP%!SPACE!", CTX);
// otLogCertMeshCoP{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_MESHCOP}(CTX, MSG, ...);
// end_wpp

// ==MBEDTLS==

// begin_wpp config
// USEPREFIX (otLogCritMbedTls, "[%p]MBED%!SPACE!", CTX);
// otLogCritMbedTls{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_MBEDTLS}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnMbedTls, "[%p]MBED%!SPACE!", CTX);
// otLogWarnMbedTls{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_MBEDTLS}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoMbedTls, "[%p]MBED%!SPACE!", CTX);
// otLogInfoMbedTls{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_MBEDTLS}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgMbedTls, "[%p]MBED%!SPACE!", CTX);
// otLogDebgMbedTls{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_MBEDTLS}(CTX, MSG, ...);
// end_wpp

// ==MLE==

// begin_wpp config
// USEPREFIX (otLogCritMle, "[%p]MLE%!SPACE!", CTX);
// otLogCritMle{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_MLE}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnMle, "[%p]MLE%!SPACE!", CTX);
// otLogWarnMle{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_MLE}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnMleErr, "[%p]MLE%!SPACE!", CTX);
// otLogWarnMleErr{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_MLE}(CTX, EXP, MSG, ...);
// USESUFFIX(otLogWarnMleErr, ", %!otError!", EXP);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoMle, "[%p]MLE%!SPACE!", CTX);
// otLogInfoMle{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_MLE}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgMle, "[%p]MLE%!SPACE!", CTX);
// otLogDebgMle{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_MLE}(CTX, MSG, ...);
// end_wpp

// ==ARP==

// begin_wpp config
// USEPREFIX (otLogCritArp, "[%p]ARP%!SPACE!", CTX);
// otLogCritArp{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_ARP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnArp, "[%p]ARP%!SPACE!", CTX);
// otLogWarnArp{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_ARP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoArp, "[%p]ARP%!SPACE!", CTX);
// otLogInfoArp{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_ARP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgArp, "[%p]ARP%!SPACE!", CTX);
// otLogDebgArp{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_ARP}(CTX, MSG, ...);
// end_wpp

// ==NETD==

// begin_wpp config
// USEPREFIX (otLogCritNetData, "[%p]NETD%!SPACE!", CTX);
// otLogCritNetData{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_NETD}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnNetData, "[%p]NETD%!SPACE!", CTX);
// otLogWarnNetData{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_NETD}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoNetData, "[%p]NETD%!SPACE!", CTX);
// otLogInfoNetData{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_NETD}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgNetData, "[%p]NETD%!SPACE!", CTX);
// otLogDebgNetData{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_NETD}(CTX, MSG, ...);
// end_wpp

// ==ICMP==

// begin_wpp config
// USEPREFIX (otLogCritIcmp, "[%p]ICMP%!SPACE!", CTX);
// otLogCritIcmp{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_ICMP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnIcmp, "[%p]ICMP%!SPACE!", CTX);
// otLogWarnIcmp{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_ICMP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoIcmp, "[%p]ICMP%!SPACE!", CTX);
// otLogInfoIcmp{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_ICMP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgIcmp, "[%p]ICMP%!SPACE!", CTX);
// otLogDebgIcmp{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_ICMP}(CTX, MSG, ...);
// end_wpp

// ==IPV6==

// begin_wpp config
// USEPREFIX (otLogCritIp6, "[%p]IP6%!SPACE!", CTX);
// otLogCritIp6{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_IPV6}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnIp6, "[%p]IP6%!SPACE!", CTX);
// otLogWarnIp6{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_IPV6}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoIp6, "[%p]IP6%!SPACE!", CTX);
// otLogInfoIp6{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_IPV6}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgIp6, "[%p]IP6%!SPACE!", CTX);
// otLogDebgIp6{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_IPV6}(CTX, MSG, ...);
// end_wpp

// ==MAC==

// begin_wpp config
// USEPREFIX (otLogCritMac, "[%p]MAC%!SPACE!", CTX);
// otLogCritMac{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_MAC}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnMac, "[%p]MAC%!SPACE!", CTX);
// otLogWarnMac{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_MAC}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoMac, "[%p]MAC%!SPACE!", CTX);
// otLogInfoMac{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_MAC}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgMac, "[%p]MAC%!SPACE!", CTX);
// otLogDebgMac{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_MAC}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgMacErr, "[%p]MAC%!SPACE!", CTX);
// otLogDebgMacErr{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_MAC}(CTX, EXP, MSG, ...);
// USESUFFIX(otLogDebgMacErr, ", %!otError!", EXP);
// end_wpp

// ==MEM==

// begin_wpp config
// USEPREFIX (otLogCritMem, "[%p]MEM%!SPACE!", CTX);
// otLogCritMem{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_MEM}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnMem, "[%p]MEM%!SPACE!", CTX);
// otLogWarnMem{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_MEM}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoMem, "[%p]MEM%!SPACE!", CTX);
// otLogInfoMem{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_MEM}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgMem, "[%p]MEM%!SPACE!", CTX);
// otLogDebgMem{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_MEM}(CTX, MSG, ...);
// end_wpp

// ==DUMP==

// begin_wpp config
// otLogDump{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_DUMP}(MSG, ...);
// end_wpp

// ==MEM==

// begin_wpp config
// USEPREFIX (otLogCritNetDiag, "[%p]NETD%!SPACE!", CTX);
// otLogCritNetDiag{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_NDIAG}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnNetDiag, "[%p]NETD%!SPACE!", CTX);
// otLogWarnNetDiag{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_NDIAG}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoNetDiag, "[%p]NETD%!SPACE!", CTX);
// otLogInfoNetDiag{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_NDIAG}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgNetDiag, "[%p]NETD%!SPACE!", CTX);
// otLogDebgNetDiag{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_NDIAG}(CTX, MSG, ...);
// end_wpp

// ==COAP==

// begin_wpp config
// USEPREFIX (otLogCritCoap, "[%p]COAP%!SPACE!", CTX);
// otLogCritCoap{LEVEL=TRACE_LEVEL_ERROR,FLAGS=OT_COAP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogWarnCoap, "[%p]COAP%!SPACE!", CTX);
// otLogWarnCoap{LEVEL=TRACE_LEVEL_WARNING,FLAGS=OT_COAP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoCoap, "[%p]COAP%!SPACE!", CTX);
// otLogInfoCoap{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_COAP}(CTX, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogInfoCoapErr, "[%p]COAP%!SPACE!", CTX);
// otLogInfoCoapErr{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=OT_COAP}(CTX, EXP, MSG, ...);
// USESUFFIX(otLogInfoCoapErr, ", %!otError!", EXP);
// end_wpp

// begin_wpp config
// USEPREFIX (otLogDebgCoap, "[%p]COAP%!SPACE!", CTX);
// otLogDebgCoap{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=OT_COAP}(CTX, MSG, ...);
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
