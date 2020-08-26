/*
 * fifo.h
 *
 *  Created on: May 23, 2020
 *      Author: Telink
 */

#ifndef FIFO_H_
#define FIFO_H_

#include "types.h"

typedef struct
{
    u32 size;
    u16 num;
    u8  wptr;
    u8  rptr;
    u8 *p;
} my_fifo_t;

#endif /* FIFO_H_ */
