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
 * ANY FATFS_DIRECT, INFATFS_DIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
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

#ifdef MID_FATFS

#include <stdlib.h>
#include <string.h>
#include "fatfs_dirent.h"

FATFS_DIR *opendir(const char *path)
{
    FRESULT res;
    FATFS_DIR *dp;

    /* Malloc memory for dir object */
    dp = (FATFS_DIR *)malloc(sizeof(FATFS_DIR));

    if (!dp)   /* No more memory */
    {
        return NULL;
    }

    res = f_opendir(&dp->dir, path);

    if (res != FR_OK)
    {
        free(dp);
        return NULL;
    }

    return dp;
}

extern struct dirent *readdir(FATFS_DIR *dp)
{
    if (!dp) { return NULL; }

    FRESULT res;
    FILINFO info;

#if _USE_LFN
    info.lfname = dp->d_dirent.d_name;
    info.lfsize = sizeof(dp->d_dirent.d_name);
#endif
    res = f_readdir(&dp->dir, &info);

    if (res != FR_OK || info.fname[0] == 0)
    {
        return  NULL;
    }

#if _USE_LFN

    if (*(info.lfname) == 0)
    {
        dp->d_dirent.d_namlen = 13; /* 8.3 format */
        memcpy(dp->d_dirent.d_name, info.fname, 13);
    }

#else
    dp->d_dirent.d_namlen = 13; /* 8.3 format */
    memcpy(dp->d_dirent.d_name, info.fname, 13);
#endif

    return &(dp->d_dirent);
}

extern int closedir(FATFS_DIR *dp)
{
    FRESULT res;

    if (!dp) { return -1; }

    res = f_closedir(&dp->dir);
    free(dp);

    if (res != FR_OK)
    {
        return -1;
    }

    return 0;
}


extern int fatfs_stat(const char *path, struct fatfs_stat *buf)
{
    FRESULT res;

    res = f_stat(path, &(buf->fatfs_filinfo));

    if (res != FR_OK)
    {
        return -1;
    }

    buf->st_mode = buf->fatfs_filinfo.fattrib;
    return 0;
}
#endif