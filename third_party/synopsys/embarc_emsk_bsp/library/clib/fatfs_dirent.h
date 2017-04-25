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
 * \date 2016-01-20
 * \author Wayne Ren(Wei.Ren@synopsys.com)
--------------------------------------------- */

#ifndef _FATFS_DIRENT_H_
#define _FATFS_DIRENT_H_

#include "board/board.h"
#include <sys/stat.h>

#ifndef MID_FATFS
#error "FatFs should be enabled"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if _USE_LFN
#define MAXNAMLEN   _MAX_LFN
#else
#define MAXNAMLEN   64
#endif

struct fatfs_stat
{
    FILINFO fatfs_filinfo;
    unsigned char st_mode;
};

struct dirent
{
    unsigned short d_namlen;        /* length of string in d_name */
    char d_name[MAXNAMLEN + 1];     /* name (up to MAXNAMLEN + 1) */
};


typedef struct
{
    DIR dir; /* FatFS DIR struct */
    struct dirent d_dirent;
} FATFS_DIR;

extern FATFS_DIR *opendir(const char *path);
extern struct dirent *readdir(FATFS_DIR *dp);
extern int closedir(FATFS_DIR *dp);
extern int fatfs_stat(const char *path, struct fatfs_stat *buf);

#define FATFS_S_ISREG(m) (m == 0)

#ifdef __cplusplus
}
#endif

#endif /*_FATFS_DIRENT_H_*/
