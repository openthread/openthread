
#include <windows.h>
#include <assert.h>
#include <openthread.h>
#include <platform.h>
#include <platform/alarm.h>
#include <platform/uart.h>
#include "platform-windows.h"

uint32_t NODE_ID = 1;
uint32_t WELLKNOWN_NODE_ID = 34;

EXTERN_C void PlatformInit(int argc, char *argv[])
{
    if (argc != 2)
    {
        exit(1);
    }

    NODE_ID = atoi(argv[1]);

    windowsAlarmInit();
    windowsRadioInit();
    windowsRandomInit();
    otPlatUartEnable();
}

EXTERN_C void PlatformProcessDrivers(void)
{
    fd_set read_fds;
    fd_set write_fds;
    int max_fd = -1;
    struct timeval timeout;
    int rval;

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    windowsRadioUpdateFdSet(&read_fds, &write_fds, &max_fd);
    windowsAlarmUpdateTimeout(&timeout);

    if (!otAreTaskletsPending(sContext))
    {
        rval = select(max_fd + 1, &read_fds, &write_fds, NULL, &timeout);
        assert(rval >= 0 && errno != ETIME);
    }

    windowsRadioProcess();
    windowsAlarmProcess();
}

