
#include <windows.h>
#include <openthread.h>
#include <stdio.h>
#include <time.h>

#include <platform/logging.h>

EXTERN_C void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    char timeString[40];
    va_list args;

    time_t now = time(NULL);
    tm tmLocalTime;
    (void)localtime_s(&tmLocalTime, &now); 

    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S ", &tmLocalTime);
    fprintf(stderr, "%s ", timeString);

    switch (aLogLevel)
    {
    case kLogLevelNone:
        fprintf(stderr, "NONE ");
        break;

    case kLogLevelCrit:
        fprintf(stderr, "CRIT ");
        break;

    case kLogLevelWarn:
        fprintf(stderr, "WARN ");
        break;

    case kLogLevelInfo:
        fprintf(stderr, "INFO ");
        break;

    case kLogLevelDebg:
        fprintf(stderr, "DEBG ");
        break;
    }

    switch (aLogRegion)
    {
    case kLogRegionApi:
        fprintf(stderr, "API  ");
        break;

    case kLogRegionMle:
        fprintf(stderr, "MLE  ");
        break;

    case kLogRegionArp:
        fprintf(stderr, "ARP  ");
        break;

    case kLogRegionNetData:
        fprintf(stderr, "NETD ");
        break;

    case kLogRegionIp6:
        fprintf(stderr, "IPV6 ");
        break;

    case kLogRegionIcmp:
        fprintf(stderr, "ICMP ");
        break;

    case kLogRegionMac:
        fprintf(stderr, "MAC  ");
        break;

    case kLogRegionMem:
        fprintf(stderr, "MEM  ");
        break;

    case kLogRegionNcp:
        fprintf(stderr, "NCP  ");
        break;

    case kLogRegionMeshCoP:
        fprintf(stderr, "MCOP ");
        break;
    }

    va_start(args, aFormat);
    vfprintf(stderr, aFormat, args);
    fprintf(stderr, "\r");
    va_end(args);
}

