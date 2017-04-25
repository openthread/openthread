/* ------------------------------------------
 * Copyright (c) 2017, Synopsys, Inc. All rights reserved.

 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:

 * 1) Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.

 * 3) Neither the name of the Synopsys, Inc., nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * \version 2017.03
 * \date 2016-01-18
 * \author Huaqi Fang(Huaqi.Fang@synopsys.com)
--------------------------------------------- */
#include "embARC_syscalls.h"
#include "device/device_hal/inc/dev_uart.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#ifdef __GNU__
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/times.h>
#include <sys/stat.h>
#endif

/**
 * \todo Consider hostlink and nsim support
 */

#ifndef _HOSTLINK_ /* Not using hostlink library */

#ifndef _ENVIRON_LEN
#define _ENVIRON_LEN    32
#endif

#ifndef BOARD_CONSOLE_UART_ID
#define BOARD_CONSOLE_UART_ID       1
#endif
#ifndef BOARD_CONSOLE_UART_BAUD
#define BOARD_CONSOLE_UART_BAUD     UART_BAUDRATE_115200
#endif

#ifndef EMBARC_CFG_STDIO_CRLF
#define EMBARC_CFG_STDIO_CRLF       1
#endif

#ifndef EMBARC_CFG_USE_RTCTIME
#define EMBARC_CFG_USE_RTCTIME      0
#endif

#define STDIO_FID_OFS           3

int stdio_uart_inited = 0;
DEV_UART *stdio_uart_obj;
static void init_stdio_serial(void)
{
    if (stdio_uart_inited) { return; }

    stdio_uart_inited = 1;
    stdio_uart_obj = uart_get_dev(BOARD_CONSOLE_UART_ID);

    if (stdio_uart_obj)
    {
        stdio_uart_obj->uart_open(BOARD_CONSOLE_UART_BAUD);
        stdio_uart_obj->uart_control(UART_CMD_SET_BAUD, CONV2VOID(BOARD_CONSOLE_UART_BAUD));
    }
    else
    {
        stdio_uart_inited = -1;
    }
}

static int stdio_read(char *buffer, unsigned int length)
{
    if (!stdio_uart_obj) { return -1; }

    unsigned int avail_len;
    stdio_uart_obj->uart_control(UART_CMD_GET_RXAVAIL, (void *)(&avail_len));
    length = (length < avail_len) ? length : avail_len;

    if (length == 0)
    {
        length = 1;
    }

    return (int)stdio_uart_obj->uart_read((void *)buffer, (uint32_t)length);
}

static void stdio_write_char(char data)
{
    stdio_uart_obj->uart_write((const void *)(&data), 1);
}


static int stdio_write(const char *buffer, unsigned int length)
{
    if (!stdio_uart_obj) { return -1; }

#if EMBARC_CFG_STDIO_CRLF
    unsigned int i = 0;

    while (i < length)
    {
        if (_arc_rarely(buffer[i] == '\n'))
        {
            stdio_write_char('\r');
        }

        stdio_write_char(buffer[i]);
        i++;
    }

    return (int)i;
#else
    return (int)stdio_uart_obj->uart_write((const void *)buffer, (uint32_t)length);
#endif
}
/////////////////////
// File Processing //
/////////////////////
#ifdef MID_FATFS
#include "ff.h"
#ifndef EMBARC_CFG_MAX_FILEHANDLES
#define EMBARC_CFG_MAX_FILEHANDLES  32
#endif
static FIL *file_handles[EMBARC_CFG_MAX_FILEHANDLES] = { NULL };
#endif

#if defined(MID_FATFS) && !_FS_READONLY
/** Posix mode to FatFS mode */
Inline BYTE conv_openmode(int flags)
{
    BYTE openmode = 0;

    if (flags == O_RDONLY)
    {
        openmode = FA_READ;
    }

    if (flags & O_WRONLY)
    {
        openmode |= FA_WRITE;
    }

    if (flags & O_RDWR)
    {
        openmode |= FA_READ | FA_WRITE;
    }

    if (flags & O_CREAT)
    {
        if (flags & O_TRUNC)
        {
            openmode |= FA_CREATE_ALWAYS;
        }
        else
        {
            openmode |= FA_OPEN_ALWAYS;
        }
    }

    return openmode;
}
#endif

/** Opening Files */
#ifdef __MW__
int SYSCALL_PREFIX(_open)(const char *path, int flags, /* int mode */ ...)
#else
int SYSCALL_PREFIX(_open)(const char *path, int flags, int mode)
#endif
{
#ifdef MID_FATFS
    int fid; /** file handle id */
    FIL *tmp_filep; /** temp file handle pointer */
    BYTE openmode;

    /* Find the first available file pointer in this handles */
    for (fid = 0; fid < EMBARC_CFG_MAX_FILEHANDLES; fid ++)
    {
        if (file_handles[fid] == NULL)
        {
            break;
        }
    }

    if (fid >= EMBARC_CFG_MAX_FILEHANDLES)
    {
        return -1; /* No more available file handler */
    }

    /* Malloc memory for FIL object */
    tmp_filep = (FIL *)malloc(sizeof(FIL));

    if (!tmp_filep)   /* No more memory */
    {
        return -1;
    }

    file_handles[fid] = tmp_filep;
#if _FS_READONLY

    if (flags == O_RDONLY)
    {
        openmode = FA_READ;
    }
    else
    {
        return -1;
    }

#else
    openmode = conv_openmode(flags);
#endif
    FRESULT res = f_open(tmp_filep, path, openmode);

    if (res)   /* open error */
    {
        free(tmp_filep);
        file_handles[fid] = NULL;
        return -1;
    }

    /** Append if O_APPEND flags is set */
    if (flags & O_APPEND)
    {
        f_lseek(tmp_filep, f_size(tmp_filep));
    }

    return fid + STDIO_FID_OFS; /* 0-2 are stdin/stdout/stderr, so plus 3 */

#else /* No File System */
    return -1;
#endif
}

/** Closing Files */
int SYSCALL_PREFIX(_close)(int handle)
{
    if (handle < STDIO_FID_OFS)   /* for stdin/stdout/stderr */
    {
        return 0;
    }

#ifdef MID_FATFS

    if ((handle - STDIO_FID_OFS) >= EMBARC_CFG_MAX_FILEHANDLES)
    {
        return -1; /* No more available file handler */
    }

    FIL *tmp_filep; /** temp file handle pointer */
    tmp_filep = file_handles[handle - STDIO_FID_OFS];

    if (tmp_filep)
    {
        file_handles[handle - STDIO_FID_OFS] = NULL;
        FRESULT res = f_close(tmp_filep);
        free(tmp_filep);

        if (res)
        {
            return -1;
        }

        return 0;
    }

#endif
    return -1;
}

/** Reading Files */
int SYSCALL_PREFIX(_read)(int handle, char *buffer, unsigned int length)
{
    if (handle < STDIO_FID_OFS)
    {
        init_stdio_serial();
        return stdio_read(buffer, length);
    }

#ifdef MID_FATFS

    if ((handle - STDIO_FID_OFS) >= EMBARC_CFG_MAX_FILEHANDLES)
    {
        return -1; /* No more available file handler */
    }

    FIL *tmp_filep; /** temp file handle pointer */
    tmp_filep = file_handles[handle - STDIO_FID_OFS];

    if (tmp_filep)
    {
        UINT br;
        FRESULT res = f_read(tmp_filep, (void *)buffer, (UINT)length, &br);

        if (res)
        {
            return -1;
        }

        return (int)br;
    }

#endif
    return -1;
}

/** Writing Files */
int SYSCALL_PREFIX(_write)(int handle, const char *buffer, unsigned int length)
{
    if (handle < STDIO_FID_OFS)
    {
        init_stdio_serial();
        return stdio_write(buffer, length);
    }

#ifdef MID_FATFS

    if ((handle - STDIO_FID_OFS) >= EMBARC_CFG_MAX_FILEHANDLES)
    {
        return -1; /* No more available file handler */
    }

    FIL *tmp_filep; /** temp file handle pointer */
    tmp_filep = file_handles[handle - STDIO_FID_OFS];

    if (tmp_filep)
    {
        UINT bw;
        FRESULT res = f_write(tmp_filep, (void *)buffer, (UINT)length, &bw);

        if (res)
        {
            return -1;
        }

        return (int)bw;
    }

#endif
    return -1;
}

/** Seeking to a Location in a File */
long SYSCALL_PREFIX(_lseek)(int handle, long offset, int method)
{
    if (handle < STDIO_FID_OFS)
    {
        return 0;
    }

#ifdef MID_FATFS

    if ((handle - STDIO_FID_OFS) >= EMBARC_CFG_MAX_FILEHANDLES)
    {
        return -1; /* No more available file handler */
    }

    FIL *tmp_filep; /** temp file handle pointer */
    tmp_filep = file_handles[handle - STDIO_FID_OFS];

    if (tmp_filep)
    {
        FRESULT res;
        long ofs = offset;

        if (method == SEEK_CUR)
        {
            ofs = (long)f_tell(tmp_filep) + offset;
        }
        else if (method == SEEK_END)
        {
            ofs = (long)f_size(tmp_filep) + offset;
        }

        if (ofs >= 0)
        {
            res = f_lseek(tmp_filep, (DWORD)ofs);

            if (res) { return -1; }

            return f_tell(tmp_filep);
        }

    }

#endif
    return -1;
}

/** Testing File Access */
int SYSCALL_PREFIX(_access)(const char *name, int mode)
{
#ifdef MID_FATFS
    FRESULT res = f_stat((const TCHAR *)name, NULL);

    if (res) { return -1; }

    return 0;
#endif
    return -1;
}

/** Testing for an Interactive Display Device */
int SYSCALL_PREFIX(_isatty)(int handle)
{
    if (handle < STDIO_FID_OFS)
    {
        return 1;
    }

#ifdef MID_FATFS

    if ((handle - STDIO_FID_OFS) >= EMBARC_CFG_MAX_FILEHANDLES)
    {
        return -1; /* No more available file handler */
    }

    FIL *tmp_filep; /** temp file handle pointer */
    tmp_filep = file_handles[handle - STDIO_FID_OFS];

    if (tmp_filep)
    {
        return 0;
    }

#endif
    return -1;
}

/** Unlink Files */
int SYSCALL_PREFIX(_unlink)(const char *path)
{
#ifdef MID_FATFS

    if (f_unlink((const TCHAR *)path) == FR_OK)
    {
        return 0;
    }

#endif
    return -1;
}

/** Renaming Files */
int SYSCALL_PREFIX(rename)(const char *old, const char *new)
{
#ifdef MID_FATFS

    if (f_rename((const TCHAR *)old, (const TCHAR *)new) == FR_OK)
    {
        return 0;
    }

#endif
    return -1;
}

/** Remove Directory */
int SYSCALL_PREFIX(_rmdir)(const char *pathname)
{
    return SYSCALL_PREFIX(_unlink)(pathname);
}

/** Status of a file (by name). */
int SYSCALL_PREFIX(_stat)(const char *path, struct stat *buf)
{
    if (!buf) { return -1; }

#ifdef MID_FATFS
    FILINFO fno;

#if _USE_LFN
    fno.lfname = NULL;
    fno.lfsize = 0;
#endif

    if (f_stat((const TCHAR *)path, &fno) == FR_OK)
    {
        if (fno.fattrib & AM_DIR)
        {
            buf->st_mode = S_IFDIR;
        }
        else
        {
            buf->st_mode = S_IFREG;
        }

        buf->st_size = fno.fsize;
        buf->st_mtime = fno.ftime;
        return 0;
    }

#endif
    return -1;
}

/** Status of a file (by name). */
int SYSCALL_PREFIX(_lstat)(const char *path, struct stat *buf)
{
    return SYSCALL_PREFIX(_stat)(path, buf);
}

/** Status of an open file */
/** \todo fstat need more work to provide correct file stat */
int SYSCALL_PREFIX(_fstat)(int handle, struct stat *buf)
{
    if (!buf) { return -1; }

    if (handle < STDIO_FID_OFS)
    {
        buf->st_mode = S_IFCHR;
        buf->st_blksize = 1;
        return 0;
    }

    /** Can't provide unix file stat using fatfs API */
    return -1;
}

//////////////////////////////
// Directory Identification //
//////////////////////////////
/** Determining the Current Directory */
char *SYSCALL_PREFIX(_getcwd)(char *buf, int len)
{
#ifdef MID_FATFS

    if (f_getcwd((TCHAR *)buf, (UINT)len) == FR_OK)
    {
        return buf;
    }

#endif
    return NULL;
}

#ifdef __GNU__
char *SYSCALL_PREFIX(getcwd)(char *buf, int len)
{
    return SYSCALL_PREFIX(_getcwd)(buf, len);
}
#endif

#if __MW__
#pragma weak open = _open
#pragma weak close = _close
#pragma weak read = _read
#pragma weak write = _write
#pragma weak lseek = _lseek
#pragma weak unlink = _unlink
#pragma weak getcwd = _getcwd
#pragma weak isatty = _isatty
#pragma weak getpid = _getpid
#pragma weak access = _access
#pragma weak remove = _unlink

#pragma weak fstat = _fstat
#pragma weak stat = _stat
#pragma weak lstat = _lstat

#endif

////////////////////////////////////////
// Process Management                 //
// \todo Integrate with FreeRTOS task //
////////////////////////////////////////
/** Determining a Process ID */
int SYSCALL_PREFIX(_getpid)(void)
{
    return 0;
}

extern void __attribute__((noreturn)) _exit_loop();

/** Terminating a Process */
void __attribute__((noreturn)) SYSCALL_PREFIX(_exit)(int status)
{
    _exit_loop(status);
}

/** Kill a process */
int SYSCALL_PREFIX(_kill)(int pid, int sig)
{
    SYSCALL_PREFIX(_exit)(sig);
    return -1;
}

////////////////////////////////
// Environment Identification //
// No environment provided    //
////////////////////////////////
static char *settings[_ENVIRON_LEN] =
{
    /*
     * Add your default environment here:
     */
    "PLATFORM=EMBARC",
    "POSIXLY_CORRECT",
    0
};

char **_environ = settings;

/** Getting Environment Variables */
char *SYSCALL_PREFIX(_getenv)(const char *var)
{
    int len;
    char **ep;

    if (!(ep = _environ)) { return NULL; }

    len = strlen(var);

    while (*ep)
    {
        if (memcmp(var, *ep, len) == 0 && (*ep)[len] == '=')
        {
            return *ep + len + 1;
        }

        ep++;
    }

    return NULL;
}

/** Putting Environment Variable */
int SYSCALL_PREFIX(_putenv)(char *string)
{
    int i, len;

    for (len = 0; string[len] && string[len] != '='; len++);

    for (i = 0; i < _ENVIRON_LEN; i++)
    {
        if (_environ[i] != 0 && strncmp(string, _environ[i], len) == 0)
        {
            _environ[i] = string;
            return 0;
        }
    }

    for (i = 0; i < _ENVIRON_LEN; i++)
    {
        if (_environ[i] == NULL)
        {
            _environ[i] = string;

            if ((i + 1) < _ENVIRON_LEN)
            {
                _environ[i + 1] = NULL;
            }

            return 0;
        }
    }

    return -1;
}

#if __MW__
char *_mwgetenv2(const char *var)
{
    return SYSCALL_PREFIX(_getenv)(var);
}
#pragma weak getenv = _getenv
#pragma weak putenv = _putenv
#endif

//////////////////////
// Time Calculation //
//////////////////////

/** Determining the Clock Value */
clock_t SYSCALL_PREFIX(clock)(void)
{
    uint64_t total_ticks;

    total_ticks = (uint64_t)OSP_GET_CUR_HWTICKS();

    return (clock_t)((uint64_t)(total_ticks * CLOCKS_PER_SEC) / BOARD_CPU_CLOCK);
}

#if !EMBARC_CFG_USE_RTCTIME
static int buildtime_ready = 0;
static struct tm build_tm;
static time_t build_time;
#endif
/** Determining the Time Value */
time_t SYSCALL_PREFIX(_time)(time_t *timer)
{
    if (!buildtime_ready)
    {
        build_time = get_build_timedate(&build_tm);
        buildtime_ready = 1;
    }

    time_t cur_time;
    cur_time = build_time + (OSP_GET_CUR_MS() / 1000);

    if (timer)
    {
        *timer = cur_time;
    }

    return cur_time;
}

#ifdef __GNU__
int SYSCALL_PREFIX(_times)(struct tms *buf)
{
    if (!buf) { return -1; }

    buf->tms_utime = OSP_GET_CUR_MS() / 1000;
    buf->tms_stime = 0;
    buf->tms_cutime = 0;
    buf->tms_cstime = 0;
    return 0;
}
#endif

int SYSCALL_PREFIX(_gettimeofday)(struct timeval *tv, void *tz)
{
    if (tv == NULL) { return -1; }

    if (!buildtime_ready)
    {
        build_time = get_build_timedate(&build_tm);
        buildtime_ready = 1;
    }

    tv->tv_sec = build_time + (OSP_GET_CUR_US() / 1000000);
    tv->tv_usec = OSP_GET_CUR_US() % 1000000;
    return 0;
}

#if __MW__
#pragma weak time = _time
#pragma weak gettimeofday = _gettimeofday
#endif

/////////////////////////
// Argument Processing //
/////////////////////////
/** Counting Arguments */
int SYSCALL_PREFIX(__argc)(void)
{
    return 0;
}
/** Finding the Value of an Argument */
char *SYSCALL_PREFIX(__argv)(int num)
{
    return NULL;
}

#endif /* !defined(_HOSTLINK_) */

#if defined(__GNU__) && !defined(_HAVE_LIBGLOSS_) && !defined(_HOSTLINK_)
/* Support newlibc for newlibc(<= ARC GNU 2015.06) */
typedef struct exc_frame
{
    uint32_t erbta;
    uint32_t eret;
    uint32_t erstatus;
    uint32_t lpcount;
    uint32_t lpend;
    uint32_t lpstart;
    uint32_t blink;
    uint32_t r30;
    uint32_t ilink;
    uint32_t fp;
    uint32_t gp;
    uint32_t r12;
    uint32_t r11;
    uint32_t r10;
    uint32_t r9;
    uint32_t r8;
    uint32_t r7;
    uint32_t r6;
    uint32_t r5;
    uint32_t r4;
    uint32_t r3;
    uint32_t r2;
    uint32_t r1;
    uint32_t r0;
} EXC_FRAME;
/**
 * \brief syscall functions
 *        partly implemented for gnu in swi exception handler
 * \param ptr exception handle stack pointer
 */
void syscall_swi(void *ptr)
{
    EXC_FRAME *swi_frame = (EXC_FRAME *)ptr;

    switch (swi_frame->r8)
    {
    case SYS_read:
        swi_frame->r0 = SYSCALL_PREFIX(_read)((int)(swi_frame->r0), (char *)(swi_frame->r1), (int)(swi_frame->r2));
        break;

    case SYS_write:
        swi_frame->r0 = SYSCALL_PREFIX(_write)((int)(swi_frame->r0), (char *)(swi_frame->r1), (int)(swi_frame->r2));
        break;

    case SYS_exit:
        SYSCALL_PREFIX(_exit)(1);
        break;

    case SYS_open:
        swi_frame->r0 = SYSCALL_PREFIX(_open)((const char *)(swi_frame->r0), (int)(swi_frame->r1), (int)(swi_frame->r2));
        break;

    case SYS_close:
        swi_frame->r0 = SYSCALL_PREFIX(_close)((int)(swi_frame->r0));
        break;

    case SYS_lseek:
        swi_frame->r0 = SYSCALL_PREFIX(_lseek)((int)(swi_frame->r0), (long)(swi_frame->r1), (int)(swi_frame->r2));
        break;

    case SYS_fstat:
        swi_frame->r0 = SYSCALL_PREFIX(_fstat)((int)(swi_frame->r0), (struct stat *)(swi_frame->r1));
        break;

    case SYS_unlink:
        swi_frame->r0 = SYSCALL_PREFIX(_unlink)((const char *)(swi_frame->r0));
        break;

    case SYS_time:
        swi_frame->r0 = SYSCALL_PREFIX(_time)((time_t *)(swi_frame->r0));
        break;

    case SYS_gettimeofday:
        swi_frame->r0 = SYSCALL_PREFIX(_gettimeofday)((struct timeval *)(swi_frame->r0), (void *)(swi_frame->r1));
        break;

    case SYS_access:
        swi_frame->r0 = SYSCALL_PREFIX(_access)((const char *)(swi_frame->r0), (int)(swi_frame->r1));
        break;

    case SYS_kill:
        swi_frame->r0 = SYSCALL_PREFIX(_kill)((int)(swi_frame->r0), (int)(swi_frame->r1));
        break;

    case SYS_rename:
        swi_frame->r0 = SYSCALL_PREFIX(rename)((const char *)(swi_frame->r0), (const char *)(swi_frame->r1));
        break;

    case SYS_times:
        swi_frame->r0 = SYSCALL_PREFIX(_times)((struct tms *)(swi_frame->r0));
        break;

    case SYS_getcwd:
        swi_frame->r0 = (uint32_t)SYSCALL_PREFIX(_getcwd)((char *)(swi_frame->r0), (int)(swi_frame->r1));
        break;

    default:
        swi_frame->r0 = -1;
        break;
    }

    swi_frame->eret = swi_frame->eret + 4;
}
#endif /* __GNU__ && !_HAVE_LIBGLOSS_ && _HOSTLINK_ */