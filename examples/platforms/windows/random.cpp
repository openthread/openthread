
/**
 * @file
 *   This file implements a pseudo-random number generator.
 *
 * @warning
 *   This implementation is not a true random number generator and does @em satisfy the Thread requirements.
 */

#include <windows.h>
#include <openthread.h>

#include <platform/random.h>
#include "platform-windows.h"

static uint32_t s_state = 1;

void windowsRandomInit(void)
{
    s_state = NODE_ID;
}

uint32_t otPlatRandomGet(void)
{
    uint32_t mlcg, p, q;
    uint64_t tmpstate;

    tmpstate = (uint64_t)33614 * (uint64_t)s_state;
    q = tmpstate & 0xffffffff;
    q = q >> 1;
    p = tmpstate >> 32;
    mlcg = p + q;

    if (mlcg & 0x80000000)
    {
        mlcg &= 0x7fffffff;
        mlcg++;
    }

    s_state = mlcg;

    return mlcg;
}
