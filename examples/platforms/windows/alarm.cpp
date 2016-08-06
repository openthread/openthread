
#include <windows.h>
#include <openthread.h>

#include <platform.h>
#include <platform\alarm.h>
#include "platform-windows.h"

static bool s_is_running = false;
static ULONG s_alarm = 0;
static ULONG s_start;

void windowsAlarmInit(void)
{
    s_start = GetTickCount();
}

EXTERN_C uint32_t otPlatAlarmGetNow()
{
    return GetTickCount();
}

EXTERN_C void otPlatAlarmStartAt(otContext *, uint32_t t0, uint32_t dt)
{
    s_alarm = t0 + dt;
    s_is_running = true;
}

EXTERN_C void otPlatAlarmStop(otContext *)
{
    s_is_running = false;
}

void windowsAlarmUpdateTimeout(struct timeval *aTimeout)
{
    int32_t remaining;

    if (aTimeout == NULL)
    {
        return;
    }

    if (s_is_running)
    {
        remaining = s_alarm - GetTickCount();

        if (remaining > 0)
        {
            aTimeout->tv_sec = remaining / 1000;
            aTimeout->tv_usec = (remaining % 1000) * 1000;
        }
        else
        {
            aTimeout->tv_sec = 0;
            aTimeout->tv_usec = 0;
        }
    }
    else
    {
        aTimeout->tv_sec = 10;
        aTimeout->tv_usec = 0;
    }
}

void windowsAlarmProcess(void)
{
    int32_t remaining;

    if (s_is_running)
    {
        remaining = s_alarm - GetTickCount();

        if (remaining <= 0)
        {
            s_is_running = false;
            otPlatAlarmFired(sContext);
        }
    }
}
