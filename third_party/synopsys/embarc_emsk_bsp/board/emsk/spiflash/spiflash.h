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
 * \date 2014-07-17
 * \author Huaqi Fang(Huaqi.Fang@synopsys.com)
--------------------------------------------- */
/**
 * \file
 * \ingroup BOARD_EMSK_DRV_SPIFLASH
 * \brief   header file of emsk onbard W25Q128BV spi flash driver
 */

/**
 * \addtogroup  BOARD_EMSK_DRV_SPIFLASH
 * @{
 */
#ifndef _EMSK_SPIFLASH_H_
#define _EMSK_SPIFLASH_H_

#define FLASH_PAGE_SIZE     0x100
#define FLASH_SECTOR_SIZE   0x1000

#include "inc/embARC_toolchain.h"
#include "inc/embARC_error.h"

typedef struct
{
    uint32_t head;      /*!< 0x68656164 ='head' */
    uint32_t cpu_type;  /*!< = 0 - all images, reserved for future */
    uint32_t start;     /*!< start address of application image in spi flash */
    uint32_t size;      /*!< size of image in bytes */
    uint32_t ramaddr;   /*!< address of ram for loading image */
    uint32_t ramstart;  /*!< start address of application in RAM !!!! */
    uint32_t checksum;  /*!< checksum of all bytes in image */
} image_t;

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t flash_read_id(void);
extern uint32_t flash_read_status(void);
extern int32_t flash_read(uint32_t address, uint32_t size, void *data);
extern int32_t flash_write_enable(void);
extern int32_t flash_wait_ready(void);
extern int32_t flash_erase(uint32_t address, uint32_t size);
extern int32_t flash_write(uint32_t address, uint32_t size, const void *data);
extern int32_t flash_init(void);
extern int32_t flash_change_freq(uint32_t freq);

#ifdef __cplusplus
}
#endif

#endif /* _EMSK_SPIFLASH_H_ */

/** @} end of group BOARD_EMSK_DRV_SPIFLASH */
